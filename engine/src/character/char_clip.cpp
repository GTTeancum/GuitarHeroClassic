// engine/src/character/char_clip.cpp
//
// CharClipSamples / CharBonesSamples decoder.
//
// Reverse-engineered from the actual GH2 game code (NOT guessed):
//   sub_82162C68 = CharClipSamples::Read
//   sub_821A3030 = CharBones::Read         (bone names + weights + cumulative counts)
//   sub_821A1500 = CharBonesSamples::ReadSamples (the per-frame data)  ⭐
//   sub_8215D520 = bone-name → category classifier
//
// FORMAT (per the recomp):
//  A clip contains the CharClip base (a neighbor/transition list of CLIP names)
//  followed by 1..3 CharBonesSamples "bone lists". Each bone list is:
//      uint32 bone_count
//      bone_count × { length-prefixed name (ends .pos/.scale/.quat/.rotx/.roty/.rotz),
//                     float32 weight }     (weight present for gRev>10)
//      uint32 cum_counts[10]   — cumulative bone count per category (0..9)
//      uint32 compression      — 0 = float32, non-0 = int16 (quantized)
//      uint32 numSamples       — number of frames
//  Then, AFTER every bone-list header, the sample data blocks follow in list
//  order (two-pass: all defs, then all data). Each list's block is:
//      numSamples × frame, where each frame is (bones grouped BY CATEGORY):
//         vectors (.pos + .scale):  3 × float32              = 12 bytes
//         quats   (.quat):          compressed 4×int16 (8B)  | uncompressed 4×float32 (16B)
//         angles  (.rotx/.roty/.rotz): compressed 1×int16 (2B) | uncompressed 1×float32 (4B)
//
// Bone classification (sub_8215D520): .pos=0 .scale=1 .quat=2 .rotx=3 .roty=4 .rotz=5
//   .d?x/.d?y/.d?z = 6/7/8, no dot = 10.

#include "character/char_clip.h"

#include "ark_v3.h"
#include "milo.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace ghogx::character {

// Strip ".pos"/".quat"/etc → bone base name. Defined below; forward-declared
// so the anonymous-namespace parser can use it.
std::string strip_suffix(const std::string& channel);

namespace {

// ---- little-endian cursor over the entry body ----------------------------
struct Cur {
  const uint8_t* p;
  size_t n;
  size_t pos = 0;
  Cur(const uint8_t* d, size_t l) : p(d), n(l) {}

  bool can(size_t k) const { return pos + k <= n; }
  uint8_t  u8()  { uint8_t v = p[pos]; pos += 1; return v; }
  uint16_t u16() { uint16_t v; std::memcpy(&v, p + pos, 2); pos += 2; return v; }
  int16_t  i16() { return (int16_t)u16(); }
  uint32_t u32() { uint32_t v; std::memcpy(&v, p + pos, 4); pos += 4; return v; }
  float    f32() { float v; std::memcpy(&v, p + pos, 4); pos += 4; return v; }
  uint32_t peek_u32(size_t at) const {
    uint32_t v; std::memcpy(&v, p + at, 4); return v;
  }
};

// Bone category from name suffix (mirrors sub_8215D520).
int bone_category(const std::string& name) {
  auto dot = name.rfind('.');
  if (dot == std::string::npos) return 10;
  std::string suf = name.substr(dot + 1);
  if (suf == "pos")   return 0;
  if (suf == "scale") return 1;
  if (suf == "quat")  return 2;
  if (suf == "rotx")  return 3;
  if (suf == "roty")  return 4;
  if (suf == "rotz")  return 5;
  if (suf.size() == 3 && suf[0] == 'd' && suf[2] >= 'x' && suf[2] <= 'z')
    return static_cast<int>(suf[2] - 'x') + 6;
  return 10;
}

// Is there a valid length-prefixed bone name at byte offset `at`?
bool is_bone_name_at(const uint8_t* d, size_t n, size_t at) {
  if (at + 4 > n) return false;
  uint32_t len; std::memcpy(&len, d + at, 4);
  if (len < 6 || len > 48 || at + 4 + len > n) return false;
  const char* s = reinterpret_cast<const char*>(d + at + 4);
  for (uint32_t i = 0; i < len; ++i)
    if (s[i] < 0x20 || s[i] >= 0x7f) return false;
  std::string cand(s, len);
  auto ends = [&](const char* x) {
    size_t sl = std::strlen(x);
    return cand.size() >= sl && cand.compare(cand.size() - sl, sl, x) == 0;
  };
  bool suffix = ends(".pos") || ends(".scale") || ends(".quat") ||
                ends(".rotx") || ends(".roty") || ends(".rotz");
  if (!suffix) {
    const size_t dot = cand.rfind('.');
    if (dot != std::string::npos) {
      const std::string suf = cand.substr(dot + 1);
      suffix = suf.size() == 3 && suf[0] == 'd' &&
               suf[2] >= 'x' && suf[2] <= 'z';
    }
  }
  bool bone = cand.rfind("bone_", 0) == 0 || cand.rfind("spot_", 0) == 0;
  return suffix && bone;
}

// One decoded bone list (CharBonesSamples header).
struct BoneList {
  std::vector<std::string> names;   // full names, file order
  std::vector<int>         cats;    // category per bone
  uint32_t cum[10] = {};
  int      compression = 1;
  int      num_samples = 0;
  int      n_vec = 0, n_quat = 0, n_angle = 0;
  size_t   frame_bytes = 0;
};

bool is_valid_category_name(const std::string& name) {
  int c = bone_category(name);
  return c >= 0 && c <= 8;
}

size_t channel_size(int cat, int compression) {
  if (cat == 0 || cat == 1) return compression < 2 ? 12u : 6u;
  if (cat == 2) {
    if (compression == 0) return 16u;
    return compression < 3 ? 8u : 4u;
  }
  if (cat >= 3 && cat <= 8) return compression == 0 ? 4u : 2u;
  return 0u;
}

bool read_zero_bone_list(const uint8_t* d, size_t n, size_t& at,
                         BoneList& out) {
  if (at + 52 > n) return false;
  Cur c(d, n);
  c.pos = at;
  uint32_t count = c.u32();
  if (count != 0) return false;

  out = BoneList{};
  for (int i = 0; i < 10; ++i) out.cum[i] = c.u32();
  for (int i = 1; i < 10; ++i) if (out.cum[i] < out.cum[i - 1]) return false;
  if (out.cum[9] != 0) return false;

  out.compression = (int)c.u32();
  out.num_samples = (int)c.u32();
  if (out.compression < 0 || out.compression > 3) return false;
  if (out.num_samples < 0 || out.num_samples > 100000) return false;
  at = c.pos;
  return true;
}

// Try to read a bone-list header starting at byte `at`. On success advances
// `at` past the header and fills `out`. Returns false if not a valid list.
bool read_bone_list(const uint8_t* d, size_t n, size_t& at, BoneList& out) {
  if (at + 4 > n) return false;
  Cur c(d, n);
  c.pos = at;
  uint32_t count = c.u32();
  if (count == 0) return read_zero_bone_list(d, n, at, out);
  if (count < 1 || count > 300) return false;
  if (!is_bone_name_at(d, n, c.pos)) return false;  // first entry must be a bone

  out = BoneList{};
  out.names.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    if (!is_bone_name_at(d, n, c.pos)) return false;
    uint32_t len = c.u32();
    std::string name(reinterpret_cast<const char*>(d + c.pos), len);
    c.pos += len;
    if (!is_valid_category_name(name)) return false;
    out.names.push_back(name);
    out.cats.push_back(bone_category(name));
    // No per-bone weight in GH2 PS2 (gRev<=10): names are back-to-back.
  }

  // cum_counts[10]
  if (c.pos + 10 * 4 > n) return false;
  for (int i = 0; i < 10; ++i) out.cum[i] = c.u32();
  // Validate: cum[0]==0, non-decreasing, cum[9] <= count.
  if (out.cum[0] != 0) return false;
  for (int i = 1; i < 10; ++i) if (out.cum[i] < out.cum[i - 1]) return false;
  if (out.cum[9] > count) return false;

  if (c.pos + 8 > n) return false;
  out.compression = (int)c.u32();
  out.num_samples = (int)c.u32();
  if (out.compression < 0 || out.compression > 3) return false;
  if (out.num_samples < 0 || out.num_samples > 100000) return false;

  // Category breakdown from cumulative counts.
  auto cat_n = [&](int cat) -> int {
    uint32_t lo = out.cum[cat];
    uint32_t hi = (cat + 1 < 10) ? out.cum[cat + 1] : count;
    return (int)(hi - lo);
  };
  out.n_vec   = cat_n(0) + cat_n(1);                       // pos + scale
  out.n_quat  = cat_n(2);                                  // quat
  out.n_angle = cat_n(3) + cat_n(4) + cat_n(5) +
                cat_n(6) + cat_n(7) + cat_n(8);            // rot*
  out.frame_bytes = 0;
  for (int cat : out.cats) out.frame_bytes += channel_size(cat, out.compression);

  at = c.pos;
  return true;
}

// Decode one bone's quaternion from the data cursor (advances it).
// VERIFIED against the game's decompression sub_8215D338: it reads the 4 int16
// in order (offsets 0,2,4,6) and writes dst[0..3] = src[0..3] * (1/32767) — a
// STRAIGHT copy, no reordering. Harmonix Hmx::Quat stores {x,y,z,w} (w last).
void read_quat(Cur& c, bool comp, ClipChannel& ch) {
  ch.type = ClipChannel::kQuat;
  const float k = 1.0f / 32767.0f;
  if (comp) {
    ch.quat[0] = c.i16() * k;  // x
    ch.quat[1] = c.i16() * k;  // y
    ch.quat[2] = c.i16() * k;  // z
    ch.quat[3] = c.i16() * k;  // w
  } else {
    ch.quat[0] = c.f32();      // x
    ch.quat[1] = c.f32();      // y
    ch.quat[2] = c.f32();      // z
    ch.quat[3] = c.f32();      // w
  }
}

float read_snorm16(Cur& c) {
  return std::max(c.i16() / 32767.0f, -1.0f);
}

bool axis_rot_no_pi_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_AXIS_ROT_NO_PI") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_AXIS_ROT_NO_PI");
  return value && value[0];
#endif
}

bool file_order_clip_samples_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_FILE_ORDER_CLIP_SAMPLES") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_FILE_ORDER_CLIP_SAMPLES");
  return value && value[0];
#endif
}

void read_vec(Cur& c, int cat, int compression, ClipChannel& ch) {
  float x, y, z;
  if (compression < 2) {
    x = c.f32();
    y = c.f32();
    z = c.f32();
  } else {
    x = read_snorm16(c) * 1345.0f;
    y = read_snorm16(c) * 1345.0f;
    z = read_snorm16(c) * 1345.0f;
  }
  if (cat == 1) {
    ch.type = ClipChannel::kScale;
    ch.scale[0] = x;
    ch.scale[1] = y;
    ch.scale[2] = z;
  } else {
    ch.type = ClipChannel::kPos;
    ch.pos[0] = x;
    ch.pos[1] = y;
    ch.pos[2] = z;
  }
}

void read_angle(Cur& c, bool comp, int cat, ClipChannel& ch) {
  ch.type = (cat == 3 || cat == 6) ? ClipChannel::kRotX
          : (cat == 4 || cat == 7) ? ClipChannel::kRotY
                                   : ClipChannel::kRotZ;
  // Community tooling decodes compressed single-axis rotations as signed
  // normalized values, then applies them as a pi-scaled axis rotation.
  ch.angle = (comp ? read_snorm16(c) : c.f32()) *
             (axis_rot_no_pi_enabled() ? 1.0f : 3.14159265358979323846f);
}

// Parse the whole clip entry into frames.
std::vector<std::vector<ClipChannel>> parse_all(const uint8_t* d, size_t n,
                                                int& num_samples_out) {
  num_samples_out = 0;

  std::vector<BoneList> lists;
  size_t p = SIZE_MAX;

  // GH2 CharClipSamples entries begin with the samples version, then a CharClip
  // base payload, then exactly three CharBonesSamples headers for version 8+.
  // The CharClip base contains arbitrary transition names, so finding the first
  // "bone_*.pos" is not reliable; `neutral` even has an empty list first.
  // Instead, accept only a candidate whose three declared sample blocks consume
  // the remaining entry bytes exactly.
  for (size_t at = 4; at + 52 <= n; ++at) {
    std::vector<BoneList> candidate;
    size_t q = at;
    bool ok = true;
    for (int i = 0; i < 3; ++i) {
      BoneList bl;
      if (!read_bone_list(d, n, q, bl)) { ok = false; break; }
      candidate.push_back(std::move(bl));
    }
    if (!ok || candidate.empty()) continue;
    bool has_channels = false;
    uint64_t sample_bytes = 0;
    for (const auto& bl : candidate) {
      if (!bl.names.empty()) has_channels = true;
      sample_bytes += static_cast<uint64_t>(bl.frame_bytes) *
                      static_cast<uint64_t>(bl.num_samples);
    }
    if (!has_channels) continue;
    if (sample_bytes == n - q) {
      lists = std::move(candidate);
      p = q;
      break;
    }
  }
  if (lists.empty() || p == SIZE_MAX) return {};

  // Sample data begins at p (after the third header). Use the max declared
  // frame count; one-sample lists are constant channels repeated across frames.
  int num_samples = 0;
  for (auto& bl : lists) num_samples = std::max(num_samples, bl.num_samples);
  if (num_samples <= 0) return {};
  num_samples_out = num_samples;

  std::vector<std::vector<ClipChannel>> frames(num_samples);

  // For each list, its block is frames_here frames laid out by category.
  // GH2 clips commonly include a full-rate list plus a one-sample list for
  // constant channels; repeat that single sample so every frame is a complete
  // pose instead of silently losing those channels after frame 0.
  size_t data = p;
  for (auto& bl : lists) {
    int frames_here = bl.num_samples > 0 ? bl.num_samples : num_samples;
    bool comp = bl.compression != 0;
    for (int f = 0; f < num_samples; ++f) {
      if (frames_here != 1 && f >= frames_here) break;
      int sample_idx = (frames_here == 1) ? 0 : f;
      size_t frame_off = data + (size_t)sample_idx * bl.frame_bytes;
      if (frame_off + bl.frame_bytes > n) break;
      Cur c(d, n);
      c.pos = frame_off;

      if (file_order_clip_samples_enabled()) {
        for (size_t bi = 0; bi < bl.names.size(); ++bi) {
          ClipChannel ch;
          ch.bone_name = strip_suffix(bl.names[bi]);
          if (bl.cats[bi] == 0 || bl.cats[bi] == 1) {
            read_vec(c, bl.cats[bi], bl.compression, ch);
          } else if (bl.cats[bi] == 2) {
            read_quat(c, comp, ch);
          } else if (bl.cats[bi] >= 3 && bl.cats[bi] <= 8) {
            read_angle(c, comp, bl.cats[bi], ch);
          } else {
            continue;
          }
          frames[f].push_back(ch);
        }
      } else {
        // Walk bones, but read by CATEGORY GROUP in the data: all vectors,
        // then all quats, then all angles. We bucket bone indices by category.
        // The data order is category-ascending; within a category, file order.
        // Vectors first (cat 0,1):
        for (size_t bi = 0; bi < bl.names.size(); ++bi) {
          if (bl.cats[bi] == 0 || bl.cats[bi] == 1) {
            ClipChannel ch; ch.bone_name = strip_suffix(bl.names[bi]);
            read_vec(c, bl.cats[bi], bl.compression, ch);
            frames[f].push_back(ch);
          }
        }
        for (size_t bi = 0; bi < bl.names.size(); ++bi) {
          if (bl.cats[bi] == 2) {
            ClipChannel ch; ch.bone_name = strip_suffix(bl.names[bi]);
            read_quat(c, comp, ch);
            frames[f].push_back(ch);
          }
        }
        for (size_t bi = 0; bi < bl.names.size(); ++bi) {
          if (bl.cats[bi] >= 3 && bl.cats[bi] <= 8) {
            ClipChannel ch; ch.bone_name = strip_suffix(bl.names[bi]);
            read_angle(c, comp, bl.cats[bi], ch);
            frames[f].push_back(ch);
          }
        }
      }
    }
    data += (size_t)frames_here * bl.frame_bytes;
  }

  return frames;
}

}  // namespace

// strip ".pos"/".quat"/etc. → bone base name, e.g. "bone_R-clavicle".
std::string strip_suffix(const std::string& channel) {
  auto dot = channel.rfind('.');
  return dot == std::string::npos ? channel : channel.substr(0, dot);
}

bool debug_clip_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_CLIP") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_CLIP");
  return value && value[0];
#endif
}

bool debug_lane_mixer_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_LANE_MIXER") == 0 && value &&
      value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_LANE_MIXER");
  return value && value[0];
#endif
}

bool debug_ik_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_IK") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_IK");
  return value && value[0];
#endif
}

bool debug_leg_pose_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_LEG_POSE") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_LEG_POSE");
  return value && value[0];
#endif
}

bool arm_ik_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_ENABLE_ARM_IK") == 0 && value && value[0];
  std::free(value);
  if (!enabled) return false;
  value = nullptr;
  len = 0;
  const bool disabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_ARM_IK") == 0 && value && value[0];
  std::free(value);
  return !disabled;
#else
  const char* enable = std::getenv("GHOGX_ENABLE_ARM_IK");
  if (!enable || !enable[0]) return false;
  const char* value = std::getenv("GHOGX_DISABLE_ARM_IK");
  return !value || !value[0];
#endif
}

bool ps2_ik_hand_position_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_ENABLE_PS2_IK_HAND_POS") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_ENABLE_PS2_IK_HAND_POS");
  return value && value[0];
#endif
}

bool ps2_ik_hand_final_disabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool disabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_PS2_IK_HAND_FINAL") == 0 &&
      value && value[0];
  std::free(value);
  return disabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_PS2_IK_HAND_FINAL");
  return value && value[0];
#endif
}

bool ps2_ik_hand_final_orientation_disabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool disabled =
      _dupenv_s(&value, &len,
                "GHOGX_DISABLE_PS2_IK_HAND_FINAL_ORIENTATION") == 0 &&
      value && value[0];
  std::free(value);
  return disabled;
#else
  const char* value =
      std::getenv("GHOGX_DISABLE_PS2_IK_HAND_FINAL_ORIENTATION");
  return value && value[0];
#endif
}

bool ps2_ik_hand_final_position_disabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool disabled =
      _dupenv_s(&value, &len,
                "GHOGX_DISABLE_PS2_IK_HAND_FINAL_POSITION") == 0 &&
      value && value[0];
  std::free(value);
  return disabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_PS2_IK_HAND_FINAL_POSITION");
  return value && value[0];
#endif
}

bool ps2_ik_hands_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool disabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_PS2_IK_HANDS") == 0 &&
      value && value[0];
  std::free(value);
  return !disabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_PS2_IK_HANDS");
  return !value || !value[0];
#endif
}

bool ps2_ik_swing_postmultiply_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_PS2_IK_POSTMULTIPLY_SWING") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_PS2_IK_POSTMULTIPLY_SWING");
  return value && value[0];
#endif
}

bool ps2_ik_swing_transpose_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_PS2_IK_TRANSPOSE_SWING") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_PS2_IK_TRANSPOSE_SWING");
  return value && value[0];
#endif
}

bool ps2_ik_aimed_swing_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_PS2_IK_AIMED_SWING") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_PS2_IK_AIMED_SWING");
  return value && value[0];
#endif
}

bool controller_audit_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_AUDIT_CHARACTER_GRAPH") == 0 && value &&
      value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_AUDIT_CHARACTER_GRAPH");
  return value && value[0];
#endif
}

bool ik_hand_rotation_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_ENABLE_IK_HAND_ROT") == 0 && value &&
      value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_ENABLE_IK_HAND_ROT");
  return value && value[0];
#endif
}

bool ik_visible_stretch_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_ENABLE_IK_VISIBLE_STRETCH") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_ENABLE_IK_VISIBLE_STRETCH");
  return value && value[0];
#endif
}

bool disable_approx_upper_twist_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_APPROX_UPPER_TWIST") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_APPROX_UPPER_TWIST");
  return value && value[0];
#endif
}

bool disable_approx_fore_twist_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_APPROX_FORE_TWIST") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_APPROX_FORE_TWIST");
  return value && value[0];
#endif
}

bool ignore_approx_fore_twist_offset_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_IGNORE_APPROX_FORE_TWIST_OFFSET") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_IGNORE_APPROX_FORE_TWIST_OFFSET");
  return value && value[0];
#endif
}

bool local_hand_fore_twist_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_APPROX_FORE_TWIST_LOCAL_HAND") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_APPROX_FORE_TWIST_LOCAL_HAND");
  return value && value[0];
#endif
}

bool disable_lookat_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_LOOKAT") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_LOOKAT");
  return value && value[0];
#endif
}

bool disable_char_hair_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_CHAR_HAIR") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_CHAR_HAIR");
  return value && value[0];
#endif
}

bool debug_char_hair_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_CHAR_HAIR") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_CHAR_HAIR");
  return value && value[0];
#endif
}

void log_character_controller_graph_once(const Character& character) {
  if (!controller_audit_enabled()) return;
  static std::unordered_set<std::string> logged;
  const std::string key = character.dir_name.empty()
                              ? ("<unnamed>@" + std::to_string(
                                                 reinterpret_cast<uintptr_t>(
                                                     &character)))
                              : character.dir_name;
  if (!logged.insert(key).second) return;

  std::fprintf(stderr,
               "[chargraph] %s bones=%zu meshes=%zu drivers=%zu "
               "weightSetters=%zu ik=%zu ikMidi=%zu foreTwist=%zu upperTwist=%zu "
               "hair=%zu lookAt=%zu eyes=%zu armIK=%s ikRot=%s hairPoll=%s "
               "lookAt=%s\n",
               character.dir_name.c_str(), character.bones.size(),
               character.meshes.size(), character.drivers.size(),
               character.weight_setters.size(), character.ik_hands.size(),
               character.ik_midis.size(), character.fore_twists.size(),
               character.upper_twists.size(), character.hairs.size(), character.lookats.size(),
               character.eyes.size(), arm_ik_enabled() ? "on" : "off",
               ik_hand_rotation_enabled() ? "on" : "guarded",
               disable_char_hair_enabled() ? "off" : "on",
               disable_lookat_enabled() ? "off" : "on");
  for (const auto& ik : character.ik_hands) {
    std::fprintf(stderr,
                 "[chargraph]   ik %s hand=%s target=%s weight=%.3f "
                 "weightProp=%s orientation=%d stretch=%d scalable=%d\n",
                 ik.name.c_str(), ik.hand.c_str(), ik.target.c_str(),
                 ik.weight, ik.weight_prop.c_str(),
                 ik.orientation ? 1 : 0, ik.stretch ? 1 : 0,
                 ik.scalable ? 1 : 0);
  }
  for (const auto& ik : character.ik_midis) {
    std::fprintf(stderr, "[chargraph]   ikMidi %s bone=%s\n",
                 ik.name.c_str(), ik.bone.c_str());
  }
  for (const auto& setter : character.weight_setters) {
    std::fprintf(stderr,
                 "[chargraph]   weightSetter %s weight=%.3f weightProp=%s "
                 "driver=%s mask=0x%08x\n",
                 setter.name.c_str(), setter.weight,
                 setter.weight_prop.c_str(), setter.driver.c_str(),
                 setter.mask);
  }
  for (const auto& bone : character.bones) {
    if (bone.name.rfind("spot_", 0) == 0 ||
        bone.name.find("fret") != std::string::npos ||
        bone.name.find("strum") != std::string::npos) {
      std::fprintf(stderr,
                   "[chargraph]   trans %s parent=%s local=[%.2f %.2f %.2f]\n",
                   bone.name.c_str(), bone.parent.c_str(), bone.local.pos[0],
                   bone.local.pos[1], bone.local.pos[2]);
    }
  }
  for (const auto& ft : character.fore_twists) {
    std::fprintf(stderr,
                 "[chargraph]   foreTwist %s hand=%s twist2=%s offset=%.3f\n",
                 ft.name.c_str(), ft.hand.c_str(), ft.twist2.c_str(),
                 ft.offset_degrees);
  }
  if (!disable_approx_upper_twist_enabled()) for (const auto& ut : character.upper_twists) {
    std::fprintf(stderr,
                 "[chargraph]   upperTwist %s upper=%s twist1=%s twist2=%s\n",
                 ut.name.c_str(), ut.upper_arm.c_str(), ut.twist1.c_str(),
                 ut.twist2.c_str());
  }
  for (const auto& hair : character.hairs) {
    size_t point_count = 0;
    for (const auto& group : hair.groups) point_count += group.points.size();
    std::fprintf(stderr,
                 "[chargraph]   hair %s groups=%zu points=%zu enabled=%d\n",
                 hair.name.c_str(), hair.groups.size(), point_count,
                 hair.enabled ? 1 : 0);
    for (const auto& group : hair.groups) {
      std::fprintf(stderr,
                   "[chargraph]     hairGroup root=%s rootOffset=%.3f "
                   "points=%zu\n",
                   group.root_mesh.c_str(), group.root_offset,
                   group.points.size());
      for (const auto& point : group.points) {
        std::fprintf(stderr,
                     "[chargraph]       hairPoint mesh=%s parent=%s "
                     "pos=[%.3f %.3f %.3f] length=%.3f radius=%.3f "
                     "flags=0x%08x extra=%.3f\n",
                     point.mesh.c_str(), point.parent.c_str(), point.pos[0],
                     point.pos[1], point.pos[2], point.length, point.radius,
                     point.flags_or_mode, point.extra);
      }
    }
  }
  for (const auto& look : character.lookats) {
    std::fprintf(stderr,
                 "[chargraph]   lookAt %s source=%s target=%s driven=%s "
                 "weight=%.3f\n",
                 look.name.c_str(), look.source.c_str(), look.target.c_str(),
                 look.driven.c_str(), look.weight);
  }
  for (const auto& eyes : character.eyes) {
    std::fprintf(stderr,
                 "[chargraph]   eyes %s lookats=%zu upperlid=%s\n",
                 eyes.name.c_str(), eyes.lookats.size(),
                 eyes.upperlid_or_blink_bone.c_str());
    for (const auto& lookat : eyes.lookats) {
      std::fprintf(stderr, "[chargraph]     eyesLookAt %s\n",
                   lookat.c_str());
    }
  }
}

bool relative_face_quat_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_RELATIVE_FACE_QUAT") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_RELATIVE_FACE_QUAT");
  return value && value[0];
#endif
}

bool relative_clip_quat_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_RELATIVE_CLIP_QUAT") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_RELATIVE_CLIP_QUAT");
  return value && value[0];
#endif
}

bool disable_finger_clip_channels_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_FINGER_CLIPS") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_FINGER_CLIPS");
  return value && value[0];
#endif
}

bool disable_driven_twists_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_DRIVEN_TWISTS") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_DRIVEN_TWISTS");
  return value && value[0];
#endif
}

bool apply_hand_pos_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_APPLY_HAND_POS") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_APPLY_HAND_POS");
  return value && value[0];
#endif
}

bool approximate_driven_twists_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_ENABLE_APPROX_DRIVEN_TWISTS") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_ENABLE_APPROX_DRIVEN_TWISTS");
  return value && value[0];
#endif
}

bool disable_axis_rot_channels_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_AXIS_ROT_CHANNELS") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_AXIS_ROT_CHANNELS");
  return value && value[0];
#endif
}

bool disable_thigh_quat_channels_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_THIGH_QUATS") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_THIGH_QUATS");
  return value && value[0];
#endif
}

bool disable_foot_quat_channels_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_FOOT_QUATS") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_FOOT_QUATS");
  return value && value[0];
#endif
}

bool disable_leg_axis_channels_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_LEG_AXIS") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_LEG_AXIS");
  return value && value[0];
#endif
}

bool relative_thigh_quat_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_RELATIVE_THIGH_QUAT") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_RELATIVE_THIGH_QUAT");
  return value && value[0];
#endif
}

bool pre_relative_thigh_quat_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_PRE_RELATIVE_THIGH_QUAT") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_PRE_RELATIVE_THIGH_QUAT");
  return value && value[0];
#endif
}

bool swap_thigh_quats_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_SWAP_THIGH_QUATS") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_SWAP_THIGH_QUATS");
  return value && value[0];
#endif
}

bool invert_thigh_quats_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_INVERT_THIGH_QUATS") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_INVERT_THIGH_QUATS");
  return value && value[0];
#endif
}

bool pre_relative_clip_quat_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_PRE_RELATIVE_CLIP_QUAT") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_PRE_RELATIVE_CLIP_QUAT");
  return value && value[0];
#endif
}

bool world_clip_quat_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_WORLD_CLIP_QUAT") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_WORLD_CLIP_QUAT");
  return value && value[0];
#endif
}

bool is_finger_bone_name(const std::string& name) {
  return name.find("finger") != std::string::npos ||
         name.find("thumb") != std::string::npos ||
         name.find("pinky") != std::string::npos ||
         name.find("ringfinger") != std::string::npos ||
         name.find("index") != std::string::npos;
}

bool debug_face_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_FACE") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_FACE");
  return value && value[0];
#endif
}

bool is_face_quat_bone(const std::string& name) {
  return name.find("upperlid") != std::string::npos ||
         name.find("brow") != std::string::npos ||
         name.find("cheek") != std::string::npos ||
         name.find("jaw") != std::string::npos;
}

static std::string read_len_string(const uint8_t* data, size_t size,
                                   size_t& pos) {
  if (pos + 4 > size) throw std::runtime_error("short string length");
  uint32_t len = 0;
  std::memcpy(&len, data + pos, 4);
  pos += 4;
  if (len > size - pos || len > (1u << 20))
    throw std::runtime_error("implausible string length");
  std::string s(reinterpret_cast<const char*>(data + pos), len);
  pos += len;
  return s;
}

static milo_scene::Xfm read_xfm_at(const uint8_t* data, size_t size,
                                   size_t& pos) {
  if (pos + 48 > size) throw std::runtime_error("short matrix");
  milo_scene::Xfm x;
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      std::memcpy(&x.rot[r][c], data + pos, 4);
      pos += 4;
    }
  }
  for (int c = 0; c < 3; ++c) {
    std::memcpy(&x.pos[c], data + pos, 4);
    pos += 4;
  }
  return x;
}

static CharClip::OutputBone decode_output_bone(
    const std::string& entry_name, const uint8_t* body, size_t size) {
  // CharBone entries in GH2 animation MILOs are version 2 objects with an
  // embedded Trans v9 block:
  //   i32 CharBone version, 9 object-meta bytes, i32 Trans version,
  //   local matrix, stored/world matrix, 9 Trans-meta bytes, parent string.
  if (size < 4 + 9 + 4 + 48 + 48 + 9 + 4)
    throw std::runtime_error("short CharBone");
  size_t pos = 0;
  uint32_t version = 0;
  std::memcpy(&version, body + pos, 4);
  pos += 4;
  if (version != 2) throw std::runtime_error("unexpected CharBone version");
  pos += 9;
  uint32_t trans_version = 0;
  std::memcpy(&trans_version, body + pos, 4);
  pos += 4;
  if (trans_version != 9)
    throw std::runtime_error("unexpected CharBone Trans version");
  CharClip::OutputBone out;
  out.name = entry_name;
  out.local = read_xfm_at(body, size, pos);
  out.world_stored = read_xfm_at(body, size, pos);
  pos += 9;
  out.parent = read_len_string(body, size, pos);
  return out;
}

CharClip load_clip(const std::string& hdr_path, const std::string& ark_path,
                   const std::string& milo_path, const std::string& clip_name) {
  CharClip result;
  result.name = clip_name;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find(milo_path);
    if (!entry) entry = ark.find("../../system/run/" + milo_path);
    if (!entry) { std::fprintf(stderr, "[clip] milo not in ARK: %s\n", milo_path.c_str()); return result; }
    auto bytes = ark.read_entry(*entry, {ark_path});
    auto hdr = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, hdr);
    auto dir = gh::milo::parse_directory(payload);

    for (const auto& de : dir.entries) {
      if (de.type != "CharBone") continue;
      try {
        const uint8_t* body = payload.data() + de.offset;
        result.output_bones.push_back(
            decode_output_bone(de.name, body, static_cast<size_t>(de.size)));
        if (debug_clip_enabled()) {
          const auto& out = result.output_bones.back();
          std::fprintf(stderr,
                       "[clip-output] %-28s parent=%-28s localPos=(%.3f %.3f %.3f)\n",
                       out.name.c_str(), out.parent.c_str(), out.local.pos[0],
                       out.local.pos[1], out.local.pos[2]);
        }
      } catch (const std::exception& ex) {
        if (debug_clip_enabled()) {
          std::fprintf(stderr, "[clip] CharBone '%s' decode: %s\n",
                       de.name.c_str(), ex.what());
        }
      }
    }

    for (const auto& de : dir.entries) {
      if (de.type != "CharClipSamples" || de.name != clip_name) continue;
      const uint8_t* body = payload.data() + de.offset;
      size_t sz = (size_t)de.size;
      int ns = 0;
      result.frames = parse_all(body, sz, ns);
      result.fps = 30;  // CharClipSamples are authored at 30 fps; refine if needed.
      result.start_frame = 0.0f;
      result.end_frame = result.frames.empty()
                             ? 0.0f
                             : static_cast<float>(result.frames.size() - 1);
      result.default_play_flags = kCharPlayLoop;
      result.relative = clip_name == "visemes";
      result.loaded = !result.frames.empty();
      if (result.loaded) {
        std::fprintf(stderr,
                     "[clip] '%s': %zu frames, %zu channels/frame, %zu output bones\n",
                     clip_name.c_str(), result.frames.size(),
                     result.frames.empty() ? 0 : result.frames[0].size(),
                     result.output_bones.size());
        if (debug_clip_enabled() && !result.frames.empty()) {
          const auto& frame0 = result.frames[0];
          const size_t limit = std::min<size_t>(frame0.size(), 128);
          for (size_t i = 0; i < limit; ++i) {
            const auto& ch = frame0[i];
            const char* type = ch.type == ClipChannel::kPos ? "pos" :
                               ch.type == ClipChannel::kScale ? "scale" :
                               ch.type == ClipChannel::kQuat ? "quat" :
                               ch.type == ClipChannel::kRotX ? "rotx" :
                               ch.type == ClipChannel::kRotY ? "roty" : "rotz";
            if (ch.type == ClipChannel::kQuat) {
              std::fprintf(stderr,
                           "[clip]   %03zu %-5s %-28s [%.5f %.5f %.5f %.5f]\n",
                           i, type, ch.bone_name.c_str(), ch.quat[0],
                           ch.quat[1], ch.quat[2], ch.quat[3]);
            } else if (ch.type == ClipChannel::kPos) {
              std::fprintf(stderr,
                           "[clip]   %03zu %-5s %-28s [%.5f %.5f %.5f]\n",
                           i, type, ch.bone_name.c_str(), ch.pos[0],
                           ch.pos[1], ch.pos[2]);
            } else {
              std::fprintf(stderr, "[clip]   %03zu %-5s %-28s %.5f\n",
                           i, type, ch.bone_name.c_str(), ch.angle);
            }
          }
        }
      } else {
        std::fprintf(stderr, "[clip] '%s': parse failed\n", clip_name.c_str());
      }
      return result;
    }
    std::fprintf(stderr, "[clip] '%s' not found in %s\n", clip_name.c_str(), milo_path.c_str());
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[clip] load_clip: %s\n", ex.what());
  }
  return result;
}

// ---- pose application ----------------------------------------------------

static void quat_to_rot(const float q[4], float rot[3][3]) {
  float x = q[0], y = q[1], z = q[2], w = q[3];
  float len2 = x*x + y*y + z*z + w*w;
  if (len2 > 1e-8f) { float inv = 1.0f / std::sqrt(len2); x*=inv; y*=inv; z*=inv; w*=inv; }
  const bool transpose =
#ifdef _MSC_VER
      [] {
        char* value = nullptr;
        size_t len = 0;
        const bool enabled =
            _dupenv_s(&value, &len, "GHOGX_TRANSPOSE_CLIP_QUAT") == 0 &&
            value && value[0];
        std::free(value);
        return enabled;
      }();
#else
      (std::getenv("GHOGX_TRANSPOSE_CLIP_QUAT") &&
       std::getenv("GHOGX_TRANSPOSE_CLIP_QUAT")[0]);
#endif
  float m[3][3];
  m[0][0] = 1 - 2*(y*y + z*z);  m[0][1] = 2*(x*y + z*w);      m[0][2] = 2*(x*z - y*w);
  m[1][0] = 2*(x*y - z*w);      m[1][1] = 1 - 2*(x*x + z*z);  m[1][2] = 2*(y*z + x*w);
  m[2][0] = 2*(x*z + y*w);      m[2][1] = 2*(y*z - x*w);      m[2][2] = 1 - 2*(x*x + y*y);
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      rot[r][c] = transpose ? m[c][r] : m[r][c];
}

static bool channel_matches_bone(const std::string& bone_name,
                                 const std::string& channel_bone_name) {
  auto equivalent = [](std::string a, std::string b) {
    auto strip_mesh = [](std::string& s) {
      constexpr std::string_view suffix = ".mesh";
      if (s.size() >= suffix.size() &&
          s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0) {
        s.resize(s.size() - suffix.size());
      }
    };
    strip_mesh(a);
    strip_mesh(b);
    auto replace_once = [](std::string& s, std::string_view from,
                           std::string_view to) {
      const size_t p = s.find(from);
      if (p != std::string::npos) s.replace(p, from.size(), to);
    };
    std::string a_alias = a;
    std::string b_alias = b;
    replace_once(a_alias, "-toe0", "-toe");
    replace_once(b_alias, "-toe0", "-toe");
    return a_alias == b_alias;
  };
  if (equivalent(bone_name, channel_bone_name)) return true;
  return (bone_name.size() > channel_bone_name.size() &&
          bone_name.compare(0, channel_bone_name.size(), channel_bone_name) == 0 &&
          bone_name[channel_bone_name.size()] == '.') ||
         bone_name == channel_bone_name;
}

static bool is_hand_bone(const std::string& name) {
  return name.find("-hand") != std::string::npos;
}

static bool is_ik_hand_target_bone(const std::string& name) {
  return name.find("_hand") != std::string::npos;
}

static int find_bone_index(const Character& character, const std::string& name) {
  for (size_t i = 0; i < character.bones.size(); ++i)
    if (character.bones[i].name == name ||
        channel_matches_bone(character.bones[i].name, name))
      return (int)i;
  return -1;
}

static int find_mesh_index(const Character& character, const std::string& name) {
  for (size_t i = 0; i < character.meshes.size(); ++i)
    if (character.meshes[i].name == name ||
        channel_matches_bone(character.meshes[i].name, name))
      return (int)i;
  return -1;
}

static bool is_eye_mesh_name(const std::string& name) {
  return name == "eye-L.mesh" || name == "eye-R.mesh" ||
         name == "eyel.mesh" || name == "eyer.mesh";
}

static milo_scene::TransObj* find_bone(Character& character,
                                       const std::string& name) {
  for (auto& b : character.bones)
    if (b.name == name || channel_matches_bone(b.name, name)) return &b;
  return nullptr;
}

struct TransformTarget {
  milo_scene::Xfm* local = nullptr;
  const std::string* name = nullptr;
  const std::string* parent = nullptr;
};

static TransformTarget find_transform_target(Character& character,
                                             const std::string& name) {
  for (auto& b : character.bones) {
    if (b.name == name || channel_matches_bone(b.name, name)) {
      return {&b.local, &b.name, &b.parent};
    }
  }
  for (auto& m : character.meshes) {
    if (m.name == name || channel_matches_bone(m.name, name)) {
      return {&m.local, &m.name, &m.parent};
    }
  }
  return {};
}

static std::array<float, 16> mat4_mul(const std::array<float, 16>& a,
                                      const std::array<float, 16>& b);

static void dump_leg_pose(const Character& character) {
  auto dump = [&](const char* name) {
    const int i = find_bone_index(character, name);
    if (i < 0 || static_cast<size_t>(i) >= character.bones.size()) return;
    const auto& local = character.bones[static_cast<size_t>(i)].local;
    const auto& exact_name = character.bones[static_cast<size_t>(i)].name;
    const auto cur = character.bone_world_local_chain(exact_name);
    const auto bind = character.bone_world_bind_local_chain(exact_name);
    std::fprintf(stderr,
                 "[legpose] %-18s exact=%-24s localPos=(%.3f %.3f %.3f) "
                 "world=(%.3f %.3f %.3f) bind=(%.3f %.3f %.3f) "
                 "rows=[%.3f %.3f %.3f|%.3f %.3f %.3f|%.3f %.3f %.3f]\n",
                 name, exact_name.c_str(), local.pos[0], local.pos[1], local.pos[2],
                 cur[12], cur[13], cur[14], bind[12], bind[13], bind[14],
                 local.rot[0][0], local.rot[0][1], local.rot[0][2],
                 local.rot[1][0], local.rot[1][1], local.rot[1][2],
                 local.rot[2][0], local.rot[2][1], local.rot[2][2]);
  };
  dump("bone_facing");
  dump("bone_pelvis");
  dump("bone_L-thigh");
  dump("bone_L-knee");
  dump("bone_L-foot");
  dump("bone_L-toe");
  dump("bone_R-thigh");
  dump("bone_R-knee");
  dump("bone_R-foot");
  dump("bone_R-toe");
}

static bool transform_world(const Character& character, const std::string& name,
                            std::array<float, 16>& out) {
  for (const auto& b : character.bones) {
    if (b.name == name || channel_matches_bone(b.name, name)) {
      out = character.bone_world(b.name);
      return true;
    }
  }
  for (const auto& m : character.meshes) {
    if (m.name == name || channel_matches_bone(m.name, name)) {
      out = is_eye_mesh_name(m.name) ? character.mesh_attachment_world(m, false)
                                     : character.mesh_world(m);
      return true;
    }
  }
  return false;
}

static bool transform_local_chain_world(const Character& character,
                                        const std::string& name,
                                        std::array<float, 16>& out) {
  for (const auto& b : character.bones) {
    if (b.name == name || channel_matches_bone(b.name, name)) {
      out = character.bone_world_local_chain(b.name);
      return true;
    }
  }
  for (const auto& m : character.meshes) {
    if (m.name == name || channel_matches_bone(m.name, name)) {
      out = is_eye_mesh_name(m.name) ? character.mesh_attachment_world(m, false)
                                     : character.mesh_world(m);
      return true;
    }
  }
  return false;
}

static std::array<float, 16> mat4_mul(const std::array<float, 16>& a,
                                      const std::array<float, 16>& b) {
  std::array<float, 16> r{};
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      float s = 0.0f;
      for (int k = 0; k < 4; ++k) s += a[i * 4 + k] * b[k * 4 + j];
      r[i * 4 + j] = s;
    }
  return r;
}

static std::array<float, 16> affine_inverse(const std::array<float, 16>& m) {
  const float a=m[0], b=m[1], c=m[2];
  const float d=m[4], e=m[5], f=m[6];
  const float g=m[8], h=m[9], i=m[10];
  const float det = a*(e*i - f*h) - b*(d*i - f*g) + c*(d*h - e*g);
  if (std::fabs(det) < 1e-8f)
    return {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  const float inv = 1.0f / det;
  std::array<float, 16> r{};
  r[0] =  (e*i - f*h) * inv; r[1] = -(b*i - c*h) * inv; r[2] =  (b*f - c*e) * inv;
  r[4] = -(d*i - f*g) * inv; r[5] =  (a*i - c*g) * inv; r[6] = -(a*f - c*d) * inv;
  r[8] =  (d*h - e*g) * inv; r[9] = -(a*h - b*g) * inv; r[10]=  (a*e - b*d) * inv;
  r[15] = 1.0f;
  const float tx=m[12], ty=m[13], tz=m[14];
  r[12] = -(tx*r[0] + ty*r[4] + tz*r[8]);
  r[13] = -(tx*r[1] + ty*r[5] + tz*r[9]);
  r[14] = -(tx*r[2] + ty*r[6] + tz*r[10]);
  return r;
}

static void mat4_to_xfm(const std::array<float, 16>& m, milo_scene::Xfm& x) {
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      x.rot[r][c] = m[r * 4 + c];
  x.pos[0] = m[12];
  x.pos[1] = m[13];
  x.pos[2] = m[14];
}

struct Vec3 {
  float x = 0, y = 0, z = 0;
};

static Vec3 vsub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static Vec3 vadd(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static Vec3 vscale(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
static float vdot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static Vec3 vcross(Vec3 a, Vec3 b) {
  return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
static float vlen(Vec3 a) { return std::sqrt(vdot(a, a)); }
static Vec3 vnorm(Vec3 a, Vec3 fallback = {1, 0, 0}) {
  const float len = vlen(a);
  return len > 1e-6f ? vscale(a, 1.0f / len) : fallback;
}
static Vec3 mat_pos(const std::array<float, 16>& m) {
  return {m[12], m[13], m[14]};
}
static Vec3 mat_row(const std::array<float, 16>& m, int r) {
  return {m[r * 4 + 0], m[r * 4 + 1], m[r * 4 + 2]};
}

static Vec3 local_vec_from_world_rows(const std::array<float, 16>& basis_world,
                                      Vec3 world_vec) {
  return {vdot(mat_row(basis_world, 0), world_vec),
          vdot(mat_row(basis_world, 1), world_vec),
          vdot(mat_row(basis_world, 2), world_vec)};
}

static std::array<float, 16> aim_preserve_xfm(
    Vec3 pos, Vec3 from_dir, Vec3 to_dir, const std::array<float, 16>& source) {
  Vec3 a = vnorm(from_dir);
  Vec3 b = vnorm(to_dir, a);
  Vec3 axis = vcross(a, b);
  const float s = vlen(axis);
  const float c = std::clamp(vdot(a, b), -1.0f, 1.0f);
  std::array<float, 16> r = source;
  if (s > 1e-5f) {
    axis = vscale(axis, 1.0f / s);
    const float x = axis.x, y = axis.y, z = axis.z;
    const float t = 1.0f - c;
    // Row-vector rotation matrix: transpose of the usual column-vector form.
    const float m[3][3] = {
        {t*x*x + c,     t*x*y + s*z,   t*x*z - s*y},
        {t*x*y - s*z,   t*y*y + c,     t*y*z + s*x},
        {t*x*z + s*y,   t*y*z - s*x,   t*z*z + c},
    };
    for (int row = 0; row < 3; ++row) {
      const Vec3 src = mat_row(source, row);
      r[row * 4 + 0] = src.x * m[0][0] + src.y * m[1][0] + src.z * m[2][0];
      r[row * 4 + 1] = src.x * m[0][1] + src.y * m[1][1] + src.z * m[2][1];
      r[row * 4 + 2] = src.x * m[0][2] + src.y * m[1][2] + src.z * m[2][2];
    }
  }
  r[12] = pos.x;
  r[13] = pos.y;
  r[14] = pos.z;
  r[15] = 1.0f;
  return r;
}

static void quat_from_vec_to_vec(Vec3 from, Vec3 to, float q[4]) {
  from = vnorm(from);
  to = vnorm(to, from);
  Vec3 axis = vcross(from, to);
  float dot = std::clamp(vdot(from, to), -1.0f, 1.0f);
  if (dot < -0.9999f) {
    axis = vcross(from, {1.0f, 0.0f, 0.0f});
    if (vlen(axis) <= 1e-5f) axis = vcross(from, {0.0f, 1.0f, 0.0f});
    axis = vnorm(axis, {0.0f, 0.0f, 1.0f});
    q[0] = axis.x;
    q[1] = axis.y;
    q[2] = axis.z;
    q[3] = 0.0f;
    return;
  }
  const float s = std::sqrt((1.0f + dot) * 2.0f);
  if (s <= 1e-6f) {
    q[0] = q[1] = 0.0f;
    q[2] = 0.0f;
    q[3] = 1.0f;
    return;
  }
  const float inv = 1.0f / s;
  q[0] = axis.x * inv;
  q[1] = axis.y * inv;
  q[2] = axis.z * inv;
  q[3] = 0.5f * s;
}

static void normalize_xfm_rows(milo_scene::Xfm& xfm);

static void post_multiply_local_rot(milo_scene::Xfm& dst,
                                    const milo_scene::Xfm& source,
                                    const float rot[3][3], float weight) {
  weight = std::clamp(weight, 0.0f, 1.0f);
  for (int r = 0; r < 3; ++r) {
    float solved[3] = {};
    for (int c = 0; c < 3; ++c) {
      solved[c] = source.rot[r][0] * rot[0][c] +
                  source.rot[r][1] * rot[1][c] +
                  source.rot[r][2] * rot[2][c];
    }
    for (int c = 0; c < 3; ++c) {
      dst.rot[r][c] = source.rot[r][c] * (1.0f - weight) +
                      solved[c] * weight;
    }
  }
  normalize_xfm_rows(dst);
}

static void pre_multiply_local_rot(milo_scene::Xfm& dst,
                                   const milo_scene::Xfm& source,
                                   const float rot[3][3], float weight) {
  weight = std::clamp(weight, 0.0f, 1.0f);
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      const float solved = rot[r][0] * source.rot[0][c] +
                           rot[r][1] * source.rot[1][c] +
                           rot[r][2] * source.rot[2][c];
      dst.rot[r][c] = source.rot[r][c] * (1.0f - weight) +
                      solved * weight;
    }
  }
  normalize_xfm_rows(dst);
}

static void transpose_rot3(float rot[3][3]) {
  for (int r = 0; r < 3; ++r) {
    for (int c = r + 1; c < 3; ++c) {
      std::swap(rot[r][c], rot[c][r]);
    }
  }
}

static void set_local_from_world(milo_scene::Xfm& local,
                                 const std::array<float, 16>& desired_world,
                                 const std::array<float, 16>& parent_world) {
  mat4_to_xfm(mat4_mul(desired_world, affine_inverse(parent_world)), local);
}

static void normalized_rot(const milo_scene::Xfm& x, float r[3][3]) {
  for (int row = 0; row < 3; ++row) {
    float len = std::sqrt(x.rot[row][0] * x.rot[row][0] +
                          x.rot[row][1] * x.rot[row][1] +
                          x.rot[row][2] * x.rot[row][2]);
    if (len <= 1e-8f) len = 1.0f;
    for (int c = 0; c < 3; ++c) r[row][c] = x.rot[row][c] / len;
  }
}

static float local_x_roll_delta(const milo_scene::Xfm& bind,
                                const milo_scene::Xfm& current) {
  float b[3][3], c[3][3], d[3][3] = {};
  normalized_rot(bind, b);
  normalized_rot(current, c);
  // Delta for row-vector local matrices: current = bind * delta.
  // For an orthonormal rotation, inverse(bind) is transpose(bind).
  for (int row = 0; row < 3; ++row)
    for (int col = 0; col < 3; ++col)
      for (int k = 0; k < 3; ++k)
        d[row][col] += b[k][row] * c[k][col];
  return std::atan2(d[1][2], d[1][1]);
}

static void post_rotate_axis(milo_scene::Xfm& xfm, ClipChannel::Type axis,
                             float angle) {
  const float ca = std::cos(angle);
  const float sa = std::sin(angle);
  for (int r = 0; r < 3; ++r) {
    const float x = xfm.rot[r][0];
    const float y = xfm.rot[r][1];
    const float z = xfm.rot[r][2];
    switch (axis) {
      case ClipChannel::kRotX:
        xfm.rot[r][1] = ca * y - sa * z;
        xfm.rot[r][2] = sa * y + ca * z;
        break;
      case ClipChannel::kRotY:
        xfm.rot[r][0] = ca * x + sa * z;
        xfm.rot[r][2] = -sa * x + ca * z;
        break;
      case ClipChannel::kRotZ:
        xfm.rot[r][0] = ca * x - sa * y;
        xfm.rot[r][1] = sa * x + ca * y;
        break;
      default:
        break;
    }
  }
}

static void set_rot_x_preserve_pos(milo_scene::Xfm& xfm, float angle) {
  const float ca = std::cos(angle);
  const float sa = std::sin(angle);
  // PS2 CharForeTwist/CharUpperTwist writes a pure row-vector X rotation into
  // the driven Trans local rows before marking descendants dirty. Accepted
  // Trans-row samples show row1.z = -sin and row2.y = +sin.
  xfm.rot[0][0] = 1.0f;
  xfm.rot[0][1] = 0.0f;
  xfm.rot[0][2] = 0.0f;
  xfm.rot[1][0] = 0.0f;
  xfm.rot[1][1] = ca;
  xfm.rot[1][2] = -sa;
  xfm.rot[2][0] = 0.0f;
  xfm.rot[2][1] = sa;
  xfm.rot[2][2] = ca;
}

static float ps2_twist_angle_from_local_rows(const milo_scene::Xfm& source) {
  float r0[3] = {source.rot[0][0], source.rot[0][1], source.rot[0][2]};
  float r1[3] = {source.rot[1][0], source.rot[1][1], source.rot[1][2]};
  const float r0_len = std::sqrt(r0[0] * r0[0] + r0[1] * r0[1] +
                                 r0[2] * r0[2]);
  const float r1_len = std::sqrt(r1[0] * r1[0] + r1[1] * r1[1] +
                                 r1[2] * r1[2]);
  if (r0_len > 1e-6f) {
    r0[0] /= r0_len;
    r0[1] /= r0_len;
    r0[2] /= r0_len;
  }
  if (r1_len > 1e-6f) {
    r1[0] /= r1_len;
    r1[1] /= r1_len;
    r1[2] /= r1_len;
  }

  // Mirrors SLUS 0x002dadf8 + 0x002dae80 + atan2 in the accepted traces:
  // build the swing-removal quaternion from local row 0, rotate local row 1
  // through it, then read the residual twist about local X from z/y.
  const float half = 0.5f;
  float w = std::sqrt(std::max((r0[0] + 1.0f) * half, 0.0f));
  float qx = 0.0f;
  float qy = 0.0f;
  float qz = 0.0f;
  if (w > 1e-6f) {
    const float inv = half / w;
    qy = r0[2] * inv;
    qz = -r0[1] * inv;
  } else {
    // 180 degree swing fallback: choose a stable axis perpendicular to X.
    qy = 1.0f;
    w = 0.0f;
  }

  const float q_len = std::sqrt(qx * qx + qy * qy + qz * qz + w * w);
  if (q_len > 1e-6f) {
    qx /= q_len;
    qy /= q_len;
    qz /= q_len;
    w /= q_len;
  }

  const Vec3 v{r1[0], r1[1], r1[2]};
  const Vec3 qv{qx, qy, qz};
  const Vec3 t = vscale(vcross(qv, v), 2.0f);
  const Vec3 rotated = vadd(vadd(v, vscale(t, w)), vcross(qv, t));
  return std::atan2(rotated.z, rotated.y);
}

static float wrap_ps2_angle(float radians) {
  constexpr float kPi = 3.1415927410125732f;
  constexpr float kTwoPi = 6.2831854820251465f;
  float wrapped = std::fmod(radians + kPi, kTwoPi);
  if (wrapped < 0.0f) wrapped += kTwoPi;
  wrapped -= kPi;
  return wrapped;
}

static void write_ps2_x_twist(milo_scene::Xfm& dst,
                              const milo_scene::Xfm& bind,
                              float angle) {
  dst = bind;
  const float ca = std::cos(angle);
  const float sa = std::sin(angle);
  // Accepted Trans-row samples show the helper pre-applies the X twist to the
  // authored local basis: rows whose bind row0 is not identity keep that row.
  for (int c = 0; c < 3; ++c) {
    dst.rot[0][c] = bind.rot[0][c];
    dst.rot[1][c] = ca * bind.rot[1][c] - sa * bind.rot[2][c];
    dst.rot[2][c] = sa * bind.rot[1][c] + ca * bind.rot[2][c];
  }
}

static void write_ps2_z_bend(milo_scene::Xfm& dst,
                             const milo_scene::Xfm& base,
                             float cos_angle,
                             float sin_angle) {
  dst = base;
  // Same basis rule as the traced X-twist rows: the helper rotation is applied
  // to the current/authored local basis, not written as an identity-basis
  // matrix. Identity input still yields the sampled PS2 Z-bend rows.
  for (int c = 0; c < 3; ++c) {
    dst.rot[0][c] = cos_angle * base.rot[0][c] - sin_angle * base.rot[1][c];
    dst.rot[1][c] = sin_angle * base.rot[0][c] + cos_angle * base.rot[1][c];
    dst.rot[2][c] = base.rot[2][c];
  }
}

static void normalize_xfm_rows(milo_scene::Xfm& xfm) {
  for (int r = 0; r < 3; ++r) {
    float len = std::sqrt(xfm.rot[r][0] * xfm.rot[r][0] +
                          xfm.rot[r][1] * xfm.rot[r][1] +
                          xfm.rot[r][2] * xfm.rot[r][2]);
    if (len <= 1e-6f) continue;
    for (int c = 0; c < 3; ++c) xfm.rot[r][c] /= len;
  }
}

static void normalize_mat3_rows(std::array<float, 16>& m) {
  for (int r = 0; r < 3; ++r) {
    const float len = std::sqrt(m[r * 4 + 0] * m[r * 4 + 0] +
                                m[r * 4 + 1] * m[r * 4 + 1] +
                                m[r * 4 + 2] * m[r * 4 + 2]);
    if (len <= 1e-6f) continue;
    m[r * 4 + 0] /= len;
    m[r * 4 + 1] /= len;
    m[r * 4 + 2] /= len;
  }
}

static std::array<float, 16> local_chain_world_for_bone(
    const Character& character, int bone_index) {
  if (bone_index < 0 || static_cast<size_t>(bone_index) >= character.bones.size()) {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  }
  return character.bone_world_local_chain(character.bones[(size_t)bone_index].name);
}

static void log_debug_xfm_row(const char* tag, const char* name,
                              const milo_scene::Xfm& local,
                              const std::array<float, 16>& world) {
  const Vec3 wp = mat_pos(world);
  std::fprintf(stderr,
               "[%s] %s local_pos=[%.3f %.3f %.3f] "
               "local_r0=[%.4f %.4f %.4f] local_r1=[%.4f %.4f %.4f] "
               "local_r2=[%.4f %.4f %.4f] world_pos=[%.3f %.3f %.3f] "
               "world_r0=[%.4f %.4f %.4f] world_r1=[%.4f %.4f %.4f] "
               "world_r2=[%.4f %.4f %.4f]\n",
               tag, name, local.pos[0], local.pos[1], local.pos[2],
               local.rot[0][0], local.rot[0][1], local.rot[0][2],
               local.rot[1][0], local.rot[1][1], local.rot[1][2],
               local.rot[2][0], local.rot[2][1], local.rot[2][2],
               wp.x, wp.y, wp.z, world[0], world[1], world[2],
               world[4], world[5], world[6], world[8], world[9], world[10]);
}

static void log_debug_world_row(const char* tag, const char* name,
                                const std::array<float, 16>& world) {
  const Vec3 wp = mat_pos(world);
  std::fprintf(stderr,
               "[%s] %s world_pos=[%.3f %.3f %.3f] "
               "world_r0=[%.4f %.4f %.4f] world_r1=[%.4f %.4f %.4f] "
               "world_r2=[%.4f %.4f %.4f]\n",
               tag, name, wp.x, wp.y, wp.z, world[0], world[1], world[2],
               world[4], world[5], world[6], world[8], world[9], world[10]);
}

static float local_x_roll_delta_between_worlds(
    const std::array<float, 16>& bind_world,
    const std::array<float, 16>& current_world) {
  milo_scene::Xfm bind;
  milo_scene::Xfm current;
  mat4_to_xfm(bind_world, bind);
  mat4_to_xfm(current_world, current);
  return local_x_roll_delta(bind, current);
}

static bool apply_ps2_fore_twist(Character& character,
                                 const std::vector<milo_scene::Xfm>& bind_bones,
                                 const CharForeTwist& ft) {
  const int hand_i = find_bone_index(character, ft.hand);
  const int twist2_i = find_bone_index(character, ft.twist2);
  if (hand_i < 0 || twist2_i < 0) return false;
  const int fore_i = find_bone_index(
      character, character.bones[(size_t)hand_i].parent);
  const int twist1_i =
      find_bone_index(character, character.bones[(size_t)twist2_i].parent);
  if (fore_i < 0 || twist1_i < 0) return false;
  if ((size_t)hand_i >= bind_bones.size() ||
      (size_t)fore_i >= bind_bones.size() ||
      (size_t)twist1_i >= bind_bones.size() ||
      (size_t)twist2_i >= bind_bones.size())
    return false;

  float roll = ps2_twist_angle_from_local_rows(character.bones[(size_t)hand_i].local);
  roll = wrap_ps2_angle(roll + ft.offset_degrees * 0.01745329238474369f);

  // In pcsx2_arm_ik_twist_trans_rows_20260611, glam1's foreTwist1 row keeps
  // the live foreArm local basis/position and receives the same local-X twist
  // as foreTwist2. Only upperTwist uses the traced split constants below.
  write_ps2_x_twist(character.bones[(size_t)twist2_i].local,
                    bind_bones[(size_t)twist2_i], roll);
  write_ps2_x_twist(character.bones[(size_t)twist1_i].local,
                    character.bones[(size_t)fore_i].local, roll);
  if (debug_ik_enabled()) {
    std::fprintf(stderr,
                 "[twist-fore] %s source=%s out1=%s out2=%s offset=%.3f roll=%.4f factors=[%.4f %.4f]\n",
                 ft.name.c_str(), ft.hand.c_str(),
                 character.bones[(size_t)twist1_i].name.c_str(),
                 ft.twist2.c_str(), ft.offset_degrees, roll,
                 1.0f, 1.0f);
    log_debug_xfm_row("twist-fore-src", ft.hand.c_str(),
                      character.bones[(size_t)hand_i].local,
                      character.bone_world_local_chain(ft.hand));
    log_debug_xfm_row("twist-fore-out",
                      character.bones[(size_t)twist1_i].name.c_str(),
                      character.bones[(size_t)twist1_i].local,
                      character.bone_world_local_chain(
                          character.bones[(size_t)twist1_i].name));
    log_debug_xfm_row("twist-fore-out", ft.twist2.c_str(),
                      character.bones[(size_t)twist2_i].local,
                      character.bone_world_local_chain(ft.twist2));
  }
  return true;
}

static void apply_driven_twists(
    Character& character, const std::vector<milo_scene::Xfm>& bind_bones,
    const std::unordered_set<std::string>& fore_twists_already_applied) {
  if (disable_driven_twists_enabled()) return;
  if (approximate_driven_twists_enabled()) {
    // The previous direct-roll path is retained only as a diagnostic switch.
    // Native captures showed it can create visible forearm ribboning, so the
    // normal path below follows the traced PS2 helper-row writes instead.
    for (const auto& ut : character.upper_twists) {
      const int upper_i = find_bone_index(character, ut.upper_arm);
      const int twist1_i = find_bone_index(character, ut.twist1);
      const int twist2_i = find_bone_index(character, ut.twist2);
      if (upper_i < 0 || twist1_i < 0 || twist2_i < 0) continue;
      if ((size_t)upper_i >= bind_bones.size() ||
          (size_t)twist1_i >= bind_bones.size() ||
          (size_t)twist2_i >= bind_bones.size()) continue;

      const auto bind_world = character.bone_world_bind_local_chain(ut.twist2);
      const auto current_world = local_chain_world_for_bone(character, twist2_i);
      const float roll = local_x_roll_delta_between_worlds(bind_world, current_world);
      const bool twist1_parent_is_upper =
          character.bones[(size_t)twist1_i].parent == ut.upper_arm;
      const float first_factor = twist1_parent_is_upper ? -0.6660000086f : -0.5f;
      const float second_factor = twist1_parent_is_upper ? 0.3330000043f
                                                         : 0.3333329856f;

      character.bones[(size_t)twist1_i].local = bind_bones[(size_t)twist1_i];
      character.bones[(size_t)twist2_i].local = bind_bones[(size_t)twist2_i];
      set_rot_x_preserve_pos(character.bones[(size_t)twist1_i].local,
                             roll * first_factor);
      set_rot_x_preserve_pos(character.bones[(size_t)twist2_i].local,
                             roll * second_factor);
    }

    if (!disable_approx_fore_twist_enabled()) for (const auto& ft : character.fore_twists) {
      if (fore_twists_already_applied.count(ft.name)) continue;
      const int hand_i = find_bone_index(character, ft.hand);
      const int twist2_i = find_bone_index(character, ft.twist2);
      if (hand_i < 0 || twist2_i < 0) continue;
      const int fore_i = find_bone_index(character, character.bones[(size_t)hand_i].parent);
      const int twist1_i = find_bone_index(character, character.bones[(size_t)twist2_i].parent);
      if (fore_i < 0 || twist1_i < 0) continue;
      if ((size_t)hand_i >= bind_bones.size() ||
          (size_t)twist1_i >= bind_bones.size() ||
          (size_t)twist2_i >= bind_bones.size()) continue;

      float roll = 0.0f;
      if (local_hand_fore_twist_enabled()) {
        roll = local_x_roll_delta(bind_bones[(size_t)hand_i],
                                  character.bones[(size_t)hand_i].local);
      } else {
        const auto bind_world = character.bone_world_bind_local_chain(ft.hand);
        const auto current_world = local_chain_world_for_bone(character, hand_i);
        roll = local_x_roll_delta_between_worlds(bind_world, current_world);
      }
      if (!ignore_approx_fore_twist_offset_enabled())
        roll += ft.offset_degrees * 0.01745329238474369f;
      roll = std::remainder(roll, 6.2831854820251465f);

      character.bones[(size_t)twist1_i].local = bind_bones[(size_t)twist1_i];
      character.bones[(size_t)twist2_i].local = bind_bones[(size_t)twist2_i];
      set_rot_x_preserve_pos(character.bones[(size_t)twist1_i].local,
                             roll * 0.3333329856f);
      set_rot_x_preserve_pos(character.bones[(size_t)twist2_i].local,
                             roll * 0.5f);
    }
    return;
  }

  // SLUS 0x00175678: source is controller +0x0c (hand), output is +0x18
  // (twist2). The traced function applies the authored side offset in degrees,
  // wraps around +/- pi, then writes X-rotation helper rows.
  for (const auto& ft : character.fore_twists) {
    if (fore_twists_already_applied.count(ft.name)) continue;
    apply_ps2_fore_twist(character, bind_bones, ft);
  }

}

static void apply_ps2_upper_twists(
    Character& character, const std::vector<milo_scene::Xfm>& bind_bones) {
  if (disable_driven_twists_enabled() || approximate_driven_twists_enabled())
    return;
  // SLUS cadence resolves upper twist pairs later in the poll sequence, after
  // hand IK, foretwist, hair, and look-at have had their controller ticks.
  for (const auto& ut : character.upper_twists) {
    const int upper_i = find_bone_index(character, ut.upper_arm);
    const int twist1_i = find_bone_index(character, ut.twist1);
    const int twist2_i = find_bone_index(character, ut.twist2);
    if (upper_i < 0 || twist1_i < 0 || twist2_i < 0) continue;
    if ((size_t)upper_i >= bind_bones.size() ||
        (size_t)twist1_i >= bind_bones.size() ||
        (size_t)twist2_i >= bind_bones.size()) continue;

    const milo_scene::Xfm source_local =
        character.bones[(size_t)twist2_i].local;
    float roll = ps2_twist_angle_from_local_rows(source_local);
    const bool twist1_parent_is_upper =
        character.bones[(size_t)twist1_i].parent == ut.upper_arm;
    const float first_factor = twist1_parent_is_upper ? -0.6660000086f : -0.5f;
    const float second_factor = twist1_parent_is_upper ? 0.3330000043f
                                                       : 0.3333329856f;

    write_ps2_x_twist(character.bones[(size_t)twist1_i].local,
                      bind_bones[(size_t)twist1_i], roll * first_factor);
    write_ps2_x_twist(character.bones[(size_t)twist2_i].local,
                      bind_bones[(size_t)twist2_i], roll * second_factor);
    if (debug_ik_enabled()) {
      std::fprintf(stderr,
                   "[twist-upper] %s source=%s upper=%s out1=%s out2=%s roll=%.4f factors=[%.4f %.4f]\n",
                   ut.name.c_str(), ut.twist2.c_str(), ut.upper_arm.c_str(), ut.twist1.c_str(),
                   ut.twist2.c_str(), roll, first_factor, second_factor);
      log_debug_xfm_row("twist-upper-upper", ut.upper_arm.c_str(),
                        character.bones[(size_t)upper_i].local,
                        character.bone_world_local_chain(ut.upper_arm));
      log_debug_xfm_row("twist-upper-src", ut.twist2.c_str(),
                        source_local,
                        character.bone_world_local_chain(ut.twist2));
      log_debug_xfm_row("twist-upper-out", ut.twist1.c_str(),
                        character.bones[(size_t)twist1_i].local,
                        character.bone_world_local_chain(ut.twist1));
      log_debug_xfm_row("twist-upper-out", ut.twist2.c_str(),
                        character.bones[(size_t)twist2_i].local,
                        character.bone_world_local_chain(ut.twist2));
    }
  }
}

static float effective_ik_hand_weight(const Character& character,
                                      const CharIKHand& ik) {
  if (!ik.weight_prop.empty()) {
    const auto runtime = character.runtime_weight_props.find(ik.weight_prop);
    if (runtime != character.runtime_weight_props.end()) {
      // PS2 CharIKHand reads the live scalar row reached from base+0x10.
      // Once gameplay publishes that row, it is authoritative for this tick.
      return std::clamp(runtime->second, 0.0f, 1.0f);
    }
  }

  bool has_weight_row = false;
  float row_weight = 0.0f;
  for (const auto& setter : character.weight_setters) {
    if ((!ik.weight_prop.empty() &&
         (setter.name == ik.weight_prop ||
          setter.weight_prop == ik.weight_prop)) ||
        setter.name == ik.name) {
      has_weight_row = true;
      row_weight = std::max(row_weight, setter.weight);
    }
  }
  if (has_weight_row) {
    // Accepted traces resolve left.weight/right.weight as runtime rows. Their
    // value gates the hand IK; a hand driver's serialized weight is not a live
    // replacement for the scheduler-raised property.
    return std::clamp(row_weight, 0.0f, 1.0f);
  }

  float weight = ik.weight;
  for (const auto& driver : character.drivers) {
    if (ik.weight_prop.empty() || driver.weight_prop != ik.weight_prop)
      continue;
    weight = std::max(weight, driver.weight);
  }
  return std::clamp(weight, 0.0f, 1.0f);
}

static void apply_ps2_ik_hand_targets(
    Character& character, const std::vector<milo_scene::Xfm>& bind_bones,
    std::unordered_set<std::string>& fore_twists_applied) {
  // SLUS 0x0017a080 resolves the controller +0x20/+0x2c Trans refs, blends the
  // target world row into controller +0x50, writes a cosine-law Z bend into the
  // hand parent, then uses 0x002dad00/0x002daa30 vector-to-vector quaternion
  // rows to swing the upper arm before dirtying the Trans chain.
  for (const auto& ik : character.ik_hands) {
    const int hand_i = find_bone_index(character, ik.hand);
    const float ik_weight = effective_ik_hand_weight(character, ik);
    if (hand_i < 0 || ik_weight <= 0.0f) {
      if (debug_ik_enabled()) {
        std::fprintf(stderr,
                     "[ik-ps2] %s skipped hand=%s weight=%.3f hand_found=%d\n",
                     ik.name.c_str(), ik.hand.c_str(), ik_weight,
                     hand_i >= 0 ? 1 : 0);
      }
      continue;
    }
    auto& hand = character.bones[(size_t)hand_i];
    if (hand.parent.empty()) continue;
    const int fore_i = find_bone_index(character, hand.parent);
    if (fore_i < 0) continue;
    auto& fore = character.bones[(size_t)fore_i];
    if (fore.parent.empty()) continue;
    const int upper_i = find_bone_index(character, fore.parent);
    if (upper_i < 0) continue;
    auto& upper = character.bones[(size_t)upper_i];
    if (upper.parent.empty()) continue;

    std::array<float, 16> target_world{};
    if (!transform_local_chain_world(character, ik.target, target_world))
      continue;

    const milo_scene::Xfm upper_local0 = upper.local;
    const milo_scene::Xfm fore_local0 = fore.local;
    const auto upper_world0 = character.bone_world_local_chain(upper.name);
    const auto fore_world0 = character.bone_world_local_chain(fore.name);
    const auto hand_world = character.bone_world_local_chain(hand.name);
    const Vec3 shoulder = mat_pos(upper_world0);
    const Vec3 target = mat_pos(target_world);
    const milo_scene::Xfm& fore_setup =
        (static_cast<size_t>(fore_i) < bind_bones.size())
            ? bind_bones[static_cast<size_t>(fore_i)]
            : fore.local;
    const milo_scene::Xfm& hand_setup =
        (static_cast<size_t>(hand_i) < bind_bones.size())
            ? bind_bones[static_cast<size_t>(hand_i)]
            : hand.local;
    const float upper_len = std::max(
        0.001f, vlen({fore_setup.pos[0], fore_setup.pos[1],
                      fore_setup.pos[2]}));
    const float fore_len = std::max(
        0.001f, vlen({hand_setup.pos[0], hand_setup.pos[1],
                      hand_setup.pos[2]}));
    const Vec3 to_target = vsub(target, shoulder);
    const float raw_dist = vlen(to_target);
    const float max_reach = upper_len + fore_len - 0.001f;
    const float min_reach = std::fabs(upper_len - fore_len) + 0.001f;
    const float dist = std::clamp(raw_dist, min_reach, max_reach);
    const float dist2 = dist * dist;
    // SLUS 0x0017a558 precomputes (upper^2 + fore^2) and 1/(2*upper*fore);
    // 0x0017a080 then writes cos = (dist^2 - sum) * inv into the forearm
    // Z-bend rows. The sampled PS2 rows are [cos, -sin; sin, cos].
    const float cos_elbow = std::clamp(
        (dist2 - upper_len * upper_len - fore_len * fore_len) /
            (2.0f * upper_len * fore_len),
        -0.9850000143f, 0.9850000143f);
    const float sin_elbow =
        std::sqrt(std::max(0.0f, 1.0f - cos_elbow * cos_elbow));

    milo_scene::Xfm solved_fore = fore.local;
    write_ps2_z_bend(solved_fore, fore_local0, cos_elbow, sin_elbow);
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c)
        fore.local.rot[r][c] =
            fore_local0.rot[r][c] * (1.0f - ik_weight) +
            solved_fore.rot[r][c] * ik_weight;
    normalize_xfm_rows(fore.local);

    const auto upper_world_after_bend =
        character.bone_world_local_chain(upper.name);
    const auto hand_world_after_bend =
        character.bone_world_local_chain(hand.name);
    const Vec3 current_local = local_vec_from_world_rows(
        upper_world_after_bend,
        vsub(mat_pos(hand_world_after_bend), mat_pos(upper_world_after_bend)));
    const Vec3 target_local = local_vec_from_world_rows(
        upper_world_after_bend,
        vsub(target, mat_pos(upper_world_after_bend)));
    float swing_quat[4] = {};
    quat_from_vec_to_vec(current_local, target_local, swing_quat);
    float swing_rot[3][3] = {};
    quat_to_rot(swing_quat, swing_rot);
    const bool transpose_swing = ps2_ik_swing_transpose_enabled();
    const bool postmultiply_swing = ps2_ik_swing_postmultiply_enabled();
    const bool aimed_swing = ps2_ik_aimed_swing_enabled();
    if (aimed_swing) {
      const auto upper_parent_world =
          character.bone_world_local_chain(upper.parent);
      const auto upper_desired_world = aim_preserve_xfm(
          shoulder, vsub(mat_pos(hand_world_after_bend), shoulder),
          vsub(target, shoulder), upper_world_after_bend);
      milo_scene::Xfm solved_upper = upper.local;
      set_local_from_world(solved_upper, upper_desired_world,
                           upper_parent_world);
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          upper.local.rot[r][c] =
              upper.local.rot[r][c] * (1.0f - ik_weight) +
              solved_upper.rot[r][c] * ik_weight;
        }
      }
      normalize_xfm_rows(upper.local);
    } else if (postmultiply_swing) {
      if (transpose_swing) transpose_rot3(swing_rot);
      post_multiply_local_rot(upper.local, upper_local0, swing_rot, ik_weight);
    } else {
      if (transpose_swing) transpose_rot3(swing_rot);
      // The accepted PS2 traces call vector-to-quat/quat-to-matrix and then
      // dirty the driven Trans row. In the native row-vector skeleton this
      // corresponds to applying the helper matrix before the authored local
      // row; postmultiply is retained only for A/B diagnostics.
      pre_multiply_local_rot(upper.local, upper_local0, swing_rot, ik_weight);
    }

    const auto hand_world_before_final =
        character.bone_world_local_chain(hand.name);
    const float pre_final_error =
        vlen(vsub(mat_pos(hand_world_before_final), target));

    const bool write_final =
        !ps2_ik_hand_final_disabled() &&
        (ik.stretch || ik.orientation || ps2_ik_hand_position_enabled());
    if (write_final) {
      const auto parent_world = character.bone_world_local_chain(hand.parent);
      std::array<float, 16> solved_world = character.bone_world_local_chain(hand.name);
      if (ik.orientation && !ps2_ik_hand_final_orientation_disabled()) {
        for (int r = 0; r < 3; ++r) {
          for (int c = 0; c < 3; ++c) {
            solved_world[r * 4 + c] =
                solved_world[r * 4 + c] * (1.0f - ik_weight) +
                target_world[r * 4 + c] * ik_weight;
          }
        }
      }
      if ((ik.stretch || ps2_ik_hand_position_enabled()) &&
          !ps2_ik_hand_final_position_disabled()) {
        solved_world[12] = solved_world[12] * (1.0f - ik_weight) +
                           target_world[12] * ik_weight;
        solved_world[13] = solved_world[13] * (1.0f - ik_weight) +
                           target_world[13] * ik_weight;
        solved_world[14] = solved_world[14] * (1.0f - ik_weight) +
                           target_world[14] * ik_weight;
      }
      normalize_mat3_rows(solved_world);
      milo_scene::Xfm solved_local = hand.local;
      set_local_from_world(solved_local, solved_world, parent_world);
      hand.local = solved_local;
      normalize_xfm_rows(hand.local);
    }

    if (debug_ik_enabled()) {
      const Vec3 hp = mat_pos(hand_world);
      const Vec3 tp = mat_pos(target_world);
      const auto upper_world_post = character.bone_world_local_chain(upper.name);
      const auto fore_world_post = character.bone_world_local_chain(fore.name);
      const auto hand_world_post = character.bone_world_local_chain(hand.name);
      std::fprintf(stderr,
                   "[ik-ps2] %s hand=%s target=%s weight=%.3f hand=[%.2f %.2f %.2f] target=[%.2f %.2f %.2f] len=(%.2f %.2f) dist=%.2f cos=%.3f swing=%s%s final=%d orient=%d stretch=%d bendParent=%s upper=%s\n",
                   ik.name.c_str(), ik.hand.c_str(), ik.target.c_str(),
                   ik_weight, hp.x, hp.y, hp.z, tp.x, tp.y, tp.z,
                   upper_len, fore_len, raw_dist, cos_elbow,
                   aimed_swing ? "aim" : (postmultiply_swing ? "post" : "pre"),
                   transpose_swing ? "+T" : "",
                   write_final ? 1 : 0, ik.orientation ? 1 : 0,
                   ik.stretch ? 1 : 0,
                   fore.name.c_str(), upper.name.c_str());
      std::fprintf(stderr,
                   "[ik-ps2-error] %s preFinalError=%.4f "
                   "preFinal=[%.2f %.2f %.2f]\n",
                   ik.name.c_str(), pre_final_error,
                   hand_world_before_final[12], hand_world_before_final[13],
                   hand_world_before_final[14]);
      log_debug_xfm_row("ik-ps2-row", upper.name.c_str(), upper.local,
                        upper_world_post);
      log_debug_xfm_row("ik-ps2-row", fore.name.c_str(), fore.local,
                        fore_world_post);
      log_debug_xfm_row("ik-ps2-row", hand.name.c_str(), hand.local,
                        hand_world_post);
      log_debug_world_row("ik-ps2-target", ik.target.c_str(), target_world);
    }

    // Accepted active-song traces require each hand IK to be followed by its
    // matching foretwist. The side order itself is asset/driver order, not a
    // hard-coded left/right preference.
    for (const auto& ft : character.fore_twists) {
      if (!channel_matches_bone(ft.hand, ik.hand)) continue;
      if (apply_ps2_fore_twist(character, bind_bones, ft))
        fore_twists_applied.insert(ft.name);
    }
  }
}

static void apply_legacy_ik_hands(Character& character) {
  for (const auto& ik : character.ik_hands) {
    const int hand_i = find_bone_index(character, ik.hand);
    const float ik_weight = effective_ik_hand_weight(character, ik);
    if (hand_i < 0 || ik_weight <= 0.0f) continue;
    auto& hand = character.bones[(size_t)hand_i];
    const int fore_i = find_bone_index(character, hand.parent);
    if (fore_i < 0) continue;
    auto& fore = character.bones[(size_t)fore_i];
    const int upper_i = find_bone_index(character, fore.parent);
    if (upper_i < 0) continue;
    auto& upper = character.bones[(size_t)upper_i];
    if (upper.parent.empty()) continue;

    std::array<float, 16> target_world{};
    if (!transform_local_chain_world(character, ik.target, target_world))
      continue;
    const auto upper_parent_world =
        character.bone_world_local_chain(upper.parent);
    const auto upper_world0 = character.bone_world_local_chain(upper.name);
    const auto fore_world0 = character.bone_world_local_chain(fore.name);
    const auto hand_world0 = character.bone_world_local_chain(hand.name);
    if (debug_ik_enabled()) {
      const Vec3 hp = mat_pos(hand_world0);
      const Vec3 tp = mat_pos(target_world);
      std::fprintf(stderr,
                   "[ik] %s hand=%s target=%s hand0=[%.2f %.2f %.2f] target=[%.2f %.2f %.2f]\n",
                   ik.name.c_str(), ik.hand.c_str(), ik.target.c_str(),
                   hp.x, hp.y, hp.z, tp.x, tp.y, tp.z);
    }

    const Vec3 shoulder = mat_pos(upper_world0);
    const Vec3 target = mat_pos(target_world);
    const float upper_len = std::max(0.001f, vlen({fore.local.pos[0], fore.local.pos[1], fore.local.pos[2]}));
    const float authored_fore_len = std::max(0.001f, vlen({hand.local.pos[0], hand.local.pos[1], hand.local.pos[2]}));
    Vec3 to_target = vsub(target, shoulder);
    const float raw_dist = vlen(to_target);
    float fore_len = authored_fore_len;
    if (ik.stretch && ik_visible_stretch_enabled() &&
        raw_dist > upper_len + authored_fore_len) {
      fore_len = std::max(authored_fore_len, raw_dist - upper_len);
    }
    float dist = raw_dist;
    const float max_reach = upper_len + fore_len - 0.001f;
    const float min_reach = std::fabs(upper_len - fore_len) + 0.001f;
    dist = std::clamp(dist, min_reach, max_reach);
    const Vec3 target_dir = vnorm(to_target, mat_row(upper_world0, 0));

    Vec3 bend = vsub(mat_pos(fore_world0), shoulder);
    bend = vsub(bend, vscale(target_dir, vdot(bend, target_dir)));
    bend = vnorm(bend, mat_row(upper_world0, 1));
    if (debug_ik_enabled()) {
      const Vec3 elbow0 = mat_pos(fore_world0);
      const Vec3 hand0 = mat_pos(hand_world0);
      std::fprintf(stderr,
                   "[ik-solve] %s shoulder=[%.2f %.2f %.2f] elbow0=[%.2f %.2f %.2f] hand0=[%.2f %.2f %.2f] target=[%.2f %.2f %.2f] len=(%.2f %.2f) dist=%.2f clamp=%.2f bend=[%.3f %.3f %.3f]\n",
                   ik.name.c_str(), shoulder.x, shoulder.y, shoulder.z,
                   elbow0.x, elbow0.y, elbow0.z, hand0.x, hand0.y, hand0.z,
                   target.x, target.y, target.z, upper_len, fore_len,
                   raw_dist, dist, bend.x, bend.y, bend.z);
    }

    const float a = (upper_len*upper_len - fore_len*fore_len + dist*dist) /
                    (2.0f * dist);
    const float h2 = std::max(0.0f, upper_len*upper_len - a*a);
    const Vec3 elbow = vadd(shoulder,
                            vadd(vscale(target_dir, a),
                                 vscale(bend, std::sqrt(h2))));

    const auto upper_desired_world =
        aim_preserve_xfm(shoulder, vsub(mat_pos(fore_world0), shoulder),
                         vsub(elbow, shoulder), upper_world0);
    milo_scene::Xfm solved_upper = upper.local;
    set_local_from_world(solved_upper, upper_desired_world, upper_parent_world);
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c)
        upper.local.rot[r][c] =
            upper.local.rot[r][c] * (1.0f - ik_weight) +
            solved_upper.rot[r][c] * ik_weight;
    normalize_xfm_rows(upper.local);

    const auto upper_world1 = character.bone_world_local_chain(upper.name);
    const Vec3 elbow1 =
        mat_pos(character.bone_world_local_chain(fore.name));
    const auto fore_desired_world =
        aim_preserve_xfm(elbow1, vsub(mat_pos(hand_world0), mat_pos(fore_world0)),
                         vsub(target, elbow1), fore_world0);
    milo_scene::Xfm solved_fore = fore.local;
    set_local_from_world(solved_fore, fore_desired_world, upper_world1);
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c)
        fore.local.rot[r][c] =
            fore.local.rot[r][c] * (1.0f - ik_weight) +
            solved_fore.rot[r][c] * ik_weight;
    normalize_xfm_rows(fore.local);

    if (ik.stretch && ik_visible_stretch_enabled() &&
        fore_len > authored_fore_len + 0.0005f) {
      const Vec3 local_hand_dir =
          vnorm({hand.local.pos[0], hand.local.pos[1], hand.local.pos[2]},
                {1.0f, 0.0f, 0.0f});
      const Vec3 stretched =
          vscale(local_hand_dir,
                 authored_fore_len * (1.0f - ik_weight) + fore_len * ik_weight);
      hand.local.pos[0] = stretched.x;
      hand.local.pos[1] = stretched.y;
      hand.local.pos[2] = stretched.z;
    }

    if (ik.orientation && ik_hand_rotation_enabled()) {
      const auto fore_world1 = character.bone_world_local_chain(fore.name);
      milo_scene::Xfm solved_hand = hand.local;
      set_local_from_world(solved_hand, target_world, fore_world1);
      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
          hand.local.rot[r][c] =
              hand.local.rot[r][c] * (1.0f - ik_weight) +
              solved_hand.rot[r][c] * ik_weight;
      normalize_xfm_rows(hand.local);
    }
  }
}

struct PendingPose {
  const ClipChannel* pos = nullptr;
  const ClipChannel* scale = nullptr;
  const ClipChannel* quat = nullptr;
  const ClipChannel* rotx = nullptr;
  const ClipChannel* roty = nullptr;
  const ClipChannel* rotz = nullptr;
};

static void apply_clip_pose_sampled_direct(
    const std::vector<ClipChannel>& channels, float weight, Character& character,
    bool relative);

static void apply_pending_pose(const PendingPose& pose, milo_scene::Xfm& local,
                               bool relative = false) {
  if (pose.quat) {
    const bool quat_relative =
        relative || relative_clip_quat_enabled() ||
        (relative_thigh_quat_enabled() &&
         pose.quat->bone_name.find("-thigh") != std::string::npos) ||
        (relative_face_quat_enabled() &&
                     is_face_quat_bone(pose.quat->bone_name));
    float scale[3] = {};
    for (int r = 0; r < 3; ++r) {
      scale[r] = std::sqrt(local.rot[r][0] * local.rot[r][0] +
                           local.rot[r][1] * local.rot[r][1] +
                           local.rot[r][2] * local.rot[r][2]);
      if (scale[r] <= 1e-8f) scale[r] = 1.0f;
    }
    float rot[3][3];
    quat_to_rot(pose.quat->quat, rot);
    const bool pre_relative =
        pre_relative_clip_quat_enabled() ||
        (pre_relative_thigh_quat_enabled() &&
         pose.quat->bone_name.find("-thigh") != std::string::npos);
    if (quat_relative || pre_relative) {
      float out[3][3] = {};
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          for (int k = 0; k < 3; ++k) {
            out[r][c] += pre_relative
                             ? rot[r][k] * local.rot[k][c]
                             : local.rot[r][k] * rot[k][c];
          }
        }
      }
      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
          local.rot[r][c] = out[r][c];
    } else {
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          const float target = rot[r][c] * scale[r];
          local.rot[r][c] = target;
        }
      }
    }
  }
  if (pose.rotx) {
    post_rotate_axis(local, ClipChannel::kRotX, pose.rotx->angle);
  }
  if (pose.roty) {
    post_rotate_axis(local, ClipChannel::kRotY, pose.roty->angle);
  }
  if (pose.rotz) {
    post_rotate_axis(local, ClipChannel::kRotZ, pose.rotz->angle);
  }
  if (pose.scale) {
    for (int r = 0; r < 3; ++r) {
      local.rot[r][0] *= pose.scale->scale[0];
      local.rot[r][1] *= pose.scale->scale[1];
      local.rot[r][2] *= pose.scale->scale[2];
    }
  }
  // Hand .pos channels are authored as IK targets; applying them as local FK
  // offsets tears the forearm/hand chain. Until the arm IK solver is present,
  // keep the bind translation and apply the keyed hand rotation only.
  if (pose.pos &&
      (apply_hand_pos_enabled() || !is_hand_bone(pose.pos->bone_name) ||
       is_ik_hand_target_bone(pose.pos->bone_name))) {
    if (relative) {
      local.pos[0] += pose.pos->pos[0];
      local.pos[1] += pose.pos->pos[1];
      local.pos[2] += pose.pos->pos[2];
    } else {
      local.pos[0] = pose.pos->pos[0];
      local.pos[1] = pose.pos->pos[1];
      local.pos[2] = pose.pos->pos[2];
    }
  }
}

static void renormalize_rows(milo_scene::Xfm& local) {
  for (int r = 0; r < 3; ++r) {
    float len = std::sqrt(local.rot[r][0] * local.rot[r][0] +
                          local.rot[r][1] * local.rot[r][1] +
                          local.rot[r][2] * local.rot[r][2]);
    if (len <= 1e-6f) continue;
    for (int c = 0; c < 3; ++c) local.rot[r][c] /= len;
  }
}

static void apply_pending_pose_weighted(const PendingPose& pose,
                                        milo_scene::Xfm& local,
                                        float weight,
                                        bool relative = false) {
  weight = std::clamp(weight, 0.0f, 1.0f);
  if (weight <= 0.0f) return;
  if (weight >= 0.999f) {
    apply_pending_pose(pose, local, relative);
    return;
  }

  if (pose.quat) {
    const bool quat_relative =
        relative || relative_clip_quat_enabled() ||
        (relative_thigh_quat_enabled() &&
         pose.quat->bone_name.find("-thigh") != std::string::npos) ||
        (relative_face_quat_enabled() &&
                     is_face_quat_bone(pose.quat->bone_name));
    float scale[3] = {};
    for (int r = 0; r < 3; ++r) {
      scale[r] = std::sqrt(local.rot[r][0] * local.rot[r][0] +
                           local.rot[r][1] * local.rot[r][1] +
                           local.rot[r][2] * local.rot[r][2]);
      if (scale[r] <= 1e-8f) scale[r] = 1.0f;
    }
    float rot[3][3];
    quat_to_rot(pose.quat->quat, rot);
    const bool pre_relative =
        pre_relative_clip_quat_enabled() ||
        (pre_relative_thigh_quat_enabled() &&
         pose.quat->bone_name.find("-thigh") != std::string::npos);
    if (quat_relative || pre_relative) {
      float out[3][3] = {};
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          for (int k = 0; k < 3; ++k) {
            out[r][c] += pre_relative
                             ? rot[r][k] * local.rot[k][c]
                             : local.rot[r][k] * rot[k][c];
          }
        }
      }
      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
          local.rot[r][c] =
              local.rot[r][c] * (1.0f - weight) + out[r][c] * weight;
    } else {
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          const float target = rot[r][c] * scale[r];
          local.rot[r][c] = local.rot[r][c] * (1.0f - weight) + target * weight;
        }
      }
    }
    renormalize_rows(local);
  }
  if (pose.rotx) post_rotate_axis(local, ClipChannel::kRotX, pose.rotx->angle * weight);
  if (pose.roty) post_rotate_axis(local, ClipChannel::kRotY, pose.roty->angle * weight);
  if (pose.rotz) post_rotate_axis(local, ClipChannel::kRotZ, pose.rotz->angle * weight);
  if (pose.scale) {
    const float sx = 1.0f + (pose.scale->scale[0] - 1.0f) * weight;
    const float sy = 1.0f + (pose.scale->scale[1] - 1.0f) * weight;
    const float sz = 1.0f + (pose.scale->scale[2] - 1.0f) * weight;
    for (int r = 0; r < 3; ++r) {
      local.rot[r][0] *= sx;
      local.rot[r][1] *= sy;
      local.rot[r][2] *= sz;
    }
  }
  if (pose.pos &&
      (apply_hand_pos_enabled() || !is_hand_bone(pose.pos->bone_name) ||
       is_ik_hand_target_bone(pose.pos->bone_name))) {
    if (relative) {
      local.pos[0] += pose.pos->pos[0] * weight;
      local.pos[1] += pose.pos->pos[1] * weight;
      local.pos[2] += pose.pos->pos[2] * weight;
    } else {
      local.pos[0] = local.pos[0] * (1.0f - weight) + pose.pos->pos[0] * weight;
      local.pos[1] = local.pos[1] * (1.0f - weight) + pose.pos->pos[1] * weight;
      local.pos[2] = local.pos[2] * (1.0f - weight) + pose.pos->pos[2] * weight;
    }
  }
}

static std::string strip_transform_suffix(std::string s) {
  auto strip = [&](const char* suffix) {
    const size_t n = std::strlen(suffix);
    if (s.size() >= n && s.compare(s.size() - n, n, suffix) == 0) {
      s.resize(s.size() - n);
      return true;
    }
    return false;
  };
  strip(".mesh") || strip(".trans");
  return s;
}

struct OutputPoseNode {
  std::string name;
  std::string key;
  std::string parent_key;
  milo_scene::Xfm bind_local;
  milo_scene::Xfm current_local;
  milo_scene::Xfm world_stored;
};

static std::array<float, 16> xfm_to_mat4_local(const milo_scene::Xfm& x) {
  return {x.rot[0][0], x.rot[0][1], x.rot[0][2], 0.0f,
          x.rot[1][0], x.rot[1][1], x.rot[1][2], 0.0f,
          x.rot[2][0], x.rot[2][1], x.rot[2][2], 0.0f,
          x.pos[0],    x.pos[1],    x.pos[2],    1.0f};
}

static std::array<float, 16> output_node_local_chain(
    const std::vector<OutputPoseNode>& nodes,
    const std::unordered_map<std::string, size_t>& by_key,
    size_t index, bool bind_pose) {
  std::array<float, 16> world =
      xfm_to_mat4_local(bind_pose ? nodes[index].bind_local
                                  : nodes[index].current_local);
  std::string parent = nodes[index].parent_key;
  int guard = 0;
  while (!parent.empty() && guard++ < 128) {
    const auto it = by_key.find(parent);
    if (it == by_key.end()) break;
    const auto& node = nodes[it->second];
    world = mat4_mul(
        world, xfm_to_mat4_local(bind_pose ? node.bind_local
                                           : node.current_local));
    parent = node.parent_key;
  }
  return world;
}

static std::array<float, 16> output_node_corrected_world(
    const std::vector<OutputPoseNode>& nodes,
    const std::unordered_map<std::string, size_t>& by_key,
    size_t index) {
  const auto current_chain = output_node_local_chain(nodes, by_key, index, false);
  const auto bind_chain = output_node_local_chain(nodes, by_key, index, true);
  const auto stored_world = xfm_to_mat4_local(nodes[index].world_stored);
  return mat4_mul(current_chain,
                  mat4_mul(affine_inverse(bind_chain), stored_world));
}

static int output_depth(const std::vector<OutputPoseNode>& nodes,
                        const std::unordered_map<std::string, size_t>& by_key,
                        size_t index) {
  int depth = 0;
  std::string parent = nodes[index].parent_key;
  while (!parent.empty() && depth < 128) {
    const auto it = by_key.find(parent);
    if (it == by_key.end()) break;
    ++depth;
    parent = nodes[it->second].parent_key;
  }
  return depth;
}

static bool charbone_output_layer_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_ENABLE_CHARBONE_OUTPUT_LAYER") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_ENABLE_CHARBONE_OUTPUT_LAYER");
  return value && value[0];
#endif
}

static bool charbone_lower_body_output_disabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool disabled =
      _dupenv_s(&value, &len,
                "GHOGX_DISABLE_CHARBONE_LOWER_BODY_OUTPUT") == 0 &&
      value && value[0];
  std::free(value);
  return disabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_CHARBONE_LOWER_BODY_OUTPUT");
  return value && value[0];
#endif
}

static bool charbone_output_world_bridge_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_CHARBONE_OUTPUT_WORLD_BRIDGE") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_CHARBONE_OUTPUT_WORLD_BRIDGE");
  return value && value[0];
#endif
}

static bool charbone_output_bind_delta_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_CHARBONE_OUTPUT_BIND_DELTA") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_CHARBONE_OUTPUT_BIND_DELTA");
  return value && value[0];
#endif
}

static bool charbone_output_lower_body_only_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_CHARBONE_OUTPUT_LOWER_BODY_ONLY") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_CHARBONE_OUTPUT_LOWER_BODY_ONLY");
  return value && value[0];
#endif
}

static bool charbone_output_compare_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_CHARBONE_OUTPUT_MAP") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_CHARBONE_OUTPUT_MAP");
  return value && value[0];
#endif
}

static bool output_map_interesting_bone(const std::string& key) {
  return key == "bone_facing" || key == "bone_pelvis" ||
         key.find("-thigh") != std::string::npos ||
         key.find("-knee") != std::string::npos ||
         key.find("-ankle") != std::string::npos ||
         key.find("-foot") != std::string::npos ||
         key.find("-toe") != std::string::npos ||
         key.find("-arm") != std::string::npos ||
         key.find("-Arm") != std::string::npos ||
         key.find("-forearm") != std::string::npos ||
         key.find("-foreArm") != std::string::npos ||
         key.find("-clavicle") != std::string::npos ||
         key.find("-hand") != std::string::npos ||
         key.find("face") != std::string::npos ||
         key.find("mouth") != std::string::npos ||
         key.find("lip") != std::string::npos ||
         key.find("jaw") != std::string::npos ||
         key.find("brow") != std::string::npos ||
         key.find("lid") != std::string::npos ||
         key.find("eye") != std::string::npos;
}

static bool output_map_lower_body_bone(const std::string& key) {
  return key == "bone_facing" || key == "bone_pelvis" ||
         key.find("-thigh") != std::string::npos ||
         key.find("-knee") != std::string::npos ||
         key.find("-ankle") != std::string::npos ||
         key.find("-foot") != std::string::npos ||
         key.find("-toe") != std::string::npos;
}

static bool output_map_face_bone(const std::string& key) {
  return key.find("face") != std::string::npos ||
         key.find("mouth") != std::string::npos ||
         key.find("lip") != std::string::npos ||
         key.find("jaw") != std::string::npos ||
         key.find("brow") != std::string::npos ||
         key.find("lid") != std::string::npos ||
         key.find("eye") != std::string::npos;
}

static bool charbone_face_output_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_ENABLE_CHARBONE_FACE_OUTPUT") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_ENABLE_CHARBONE_FACE_OUTPUT");
  return value && value[0];
#endif
}

static void dump_charbone_output_map(
    const Character& character, const std::vector<OutputPoseNode>& nodes,
    const std::unordered_map<std::string, size_t>& by_key,
    const std::vector<bool>& node_driven) {
  if (!charbone_output_compare_enabled()) return;
  for (size_t i = 0; i < nodes.size(); ++i) {
    const auto& node = nodes[i];
    if (!output_map_interesting_bone(node.key)) continue;
    const int target_i = find_bone_index(character, node.key);
    if (target_i < 0 ||
        static_cast<size_t>(target_i) >= character.bones.size()) {
      std::fprintf(stderr,
                   "[out-map] %-18s output=%-24s parent=%-18s driven=%d "
                   "target=<none> outLocal=(%.3f %.3f %.3f)\n",
                   node.key.c_str(), node.name.c_str(),
                   node.parent_key.c_str(), node_driven[i] ? 1 : 0,
                   node.current_local.pos[0], node.current_local.pos[1],
                   node.current_local.pos[2]);
      continue;
    }
    const auto& target = character.bones[static_cast<size_t>(target_i)];
    const auto& target_bind =
        character.bind_bone_local.size() > static_cast<size_t>(target_i)
            ? character.bind_bone_local[static_cast<size_t>(target_i)]
            : target.local;
    const auto out_bind_world = output_node_local_chain(nodes, by_key, i, true);
    const auto out_pose_world = output_node_local_chain(nodes, by_key, i, false);
    const auto target_bind_world =
        character.bone_world_bind_local_chain(target.name);
    const auto target_pose_world =
        character.bone_world_local_chain(target.name);
    std::fprintf(
        stderr,
        "[out-map] %-18s output=%-24s parent=%-18s driven=%d "
        "target=%-24s tParent=%-24s "
        "outLocal=(%.3f %.3f %.3f) meshLocal=(%.3f %.3f %.3f) "
        "outBindW=(%.3f %.3f %.3f) meshBindW=(%.3f %.3f %.3f) "
        "outPoseW=(%.3f %.3f %.3f) meshPoseW=(%.3f %.3f %.3f) "
        "bindLocal=(%.3f %.3f %.3f)\n",
        node.key.c_str(), node.name.c_str(), node.parent_key.c_str(),
        node_driven[i] ? 1 : 0, target.name.c_str(), target.parent.c_str(),
        node.current_local.pos[0], node.current_local.pos[1],
        node.current_local.pos[2], target.local.pos[0], target.local.pos[1],
        target.local.pos[2], out_bind_world[12], out_bind_world[13],
        out_bind_world[14], target_bind_world[12], target_bind_world[13],
        target_bind_world[14], out_pose_world[12], out_pose_world[13],
        out_pose_world[14], target_pose_world[12], target_pose_world[13],
        target_pose_world[14], target_bind.pos[0], target_bind.pos[1],
        target_bind.pos[2]);
  }
}

static bool apply_clip_pose_output_layer(
    const std::vector<ClipChannel>& channels, float weight, Character& character,
    bool relative, const std::vector<CharClip::OutputBone>& output_bones) {
  if (output_bones.empty()) {
    return false;
  }

  std::vector<OutputPoseNode> nodes;
  nodes.reserve(output_bones.size());
  std::unordered_map<std::string, size_t> by_key;
  for (const auto& out : output_bones) {
    OutputPoseNode node;
    node.name = out.name;
    node.key = strip_transform_suffix(out.name);
    node.parent_key = strip_transform_suffix(out.parent);
    node.bind_local = out.local;
    node.current_local = out.local;
    node.world_stored = out.world_stored;
    by_key[node.key] = nodes.size();
    nodes.push_back(std::move(node));
  }

  std::vector<PendingPose> poses(nodes.size());
  std::vector<bool> node_driven(nodes.size(), false);
  std::vector<ClipChannel> direct_channels;
  direct_channels.reserve(channels.size());
  const bool full_output_layer = charbone_output_layer_enabled();
  const bool face_output_layer = charbone_face_output_enabled();
  const bool lower_body_only =
      charbone_output_lower_body_only_enabled() ||
      (!full_output_layer && !charbone_lower_body_output_disabled());
  for (const auto& ch : channels) {
    const auto it = by_key.find(strip_transform_suffix(ch.bone_name));
    if (it == by_key.end()) {
      direct_channels.push_back(ch);
      continue;
    }
    const bool driven_by_selected_output =
        full_output_layer || output_map_lower_body_bone(it->first) ||
        (face_output_layer && output_map_face_bone(it->first));
    if (!driven_by_selected_output) {
      direct_channels.push_back(ch);
      continue;
    }
    PendingPose& pose = poses[it->second];
    node_driven[it->second] = true;
    switch (ch.type) {
      case ClipChannel::kPos: pose.pos = &ch; break;
      case ClipChannel::kScale: pose.scale = &ch; break;
      case ClipChannel::kQuat: pose.quat = &ch; break;
      case ClipChannel::kRotX: pose.rotx = &ch; break;
      case ClipChannel::kRotY: pose.roty = &ch; break;
      case ClipChannel::kRotZ: pose.rotz = &ch; break;
    }
  }

  for (size_t i = 0; i < nodes.size(); ++i) {
    if (!node_driven[i]) continue;
    apply_pending_pose_weighted(poses[i], nodes[i].current_local, weight,
                                relative);
  }

  dump_charbone_output_map(character, nodes, by_key, node_driven);

  if (!full_output_layer && !lower_body_only && !face_output_layer) {
    return false;
  }

  struct TargetUpdate {
    int depth = 0;
    int target_index = -1;
    milo_scene::Xfm desired_local;
  };
  std::vector<TargetUpdate> updates;
  updates.reserve(nodes.size());

  for (size_t i = 0; i < nodes.size(); ++i) {
    if (!node_driven[i]) continue;
    const int target_i = find_bone_index(character, nodes[i].key);
    if (target_i < 0 ||
        static_cast<size_t>(target_i) >= character.bones.size()) {
      continue;
    }
    TargetUpdate update;
    update.depth = output_depth(nodes, by_key, i);
    update.target_index = target_i;
    if (charbone_output_world_bridge_enabled()) {
      const auto desired_world = output_node_corrected_world(nodes, by_key, i);
      std::array<float, 16> parent_world{1, 0, 0, 0, 0, 1, 0, 0,
                                         0, 0, 1, 0, 0, 0, 0, 1};
      const auto& target = character.bones[static_cast<size_t>(target_i)];
      if (!target.parent.empty()) {
        parent_world = character.bone_world_local_chain(target.parent);
      }
      mat4_to_xfm(mat4_mul(desired_world, affine_inverse(parent_world)),
                  update.desired_local);
    } else if (charbone_output_bind_delta_enabled()) {
      const auto target_bind =
          xfm_to_mat4_local(character.bind_bone_local.size() >
                                    static_cast<size_t>(target_i)
                                ? character.bind_bone_local[static_cast<size_t>(target_i)]
                                : character.bones[static_cast<size_t>(target_i)].local);
      const auto output_bind = xfm_to_mat4_local(nodes[i].bind_local);
      const auto output_current = xfm_to_mat4_local(nodes[i].current_local);
      mat4_to_xfm(mat4_mul(mat4_mul(target_bind, affine_inverse(output_bind)),
                           output_current),
                  update.desired_local);
    } else {
      update.desired_local = nodes[i].current_local;
    }
    updates.push_back(update);
  }

  std::sort(updates.begin(), updates.end(),
            [](const TargetUpdate& a, const TargetUpdate& b) {
              return a.depth < b.depth;
            });
  for (const auto& update : updates) {
    auto& target = character.bones[static_cast<size_t>(update.target_index)];
    // Accepted GH2/GH1 bridge traces show the clip/work-block output reaches
    // live Trans rows as local-row writes followed by dirty/world propagation.
    target.local = update.desired_local;
  }

  if (!direct_channels.empty()) {
    apply_clip_pose_sampled_direct(direct_channels, weight, character,
                                   relative);
  }
  return true;
}

static size_t char_hair_point_count(const Character& character) {
  size_t count = 0;
  for (const auto& hair : character.hairs) {
    for (const auto& group : hair.groups) count += group.points.size();
  }
  return count;
}

static Vec3 runtime_point_pos(const RuntimeHairPoint& p) {
  return {p.curr_world[0], p.curr_world[1], p.curr_world[2]};
}

static Vec3 runtime_point_prev(const RuntimeHairPoint& p) {
  return {p.prev_world[0], p.prev_world[1], p.prev_world[2]};
}

static void set_runtime_point_pos(RuntimeHairPoint& p, Vec3 curr, Vec3 prev) {
  p.curr_world[0] = curr.x;
  p.curr_world[1] = curr.y;
  p.curr_world[2] = curr.z;
  p.prev_world[0] = prev.x;
  p.prev_world[1] = prev.y;
  p.prev_world[2] = prev.z;
}

static void set_runtime_point_world(RuntimeHairPoint& p,
                                    const std::array<float, 16>& world) {
  p.has_world = true;
  p.world = world;
  const Vec3 pos = mat_pos(world);
  p.curr_world[0] = pos.x;
  p.curr_world[1] = pos.y;
  p.curr_world[2] = pos.z;
}

static bool local_chain_position(const Character& character,
                                 const std::string& name,
                                 Vec3& out) {
  std::array<float, 16> world{};
  if (!transform_local_chain_world(character, name, world)) return false;
  out = mat_pos(world);
  return true;
}

static bool hair_anchor_position(const Character& character,
                                 const CharHairGroup& group,
                                 const CharHairPoint& point,
                                 const milo_scene::TransObj& bone,
                                 bool first_point,
                                 Vec3 previous_point,
                                 Vec3& out) {
  if (!first_point) {
    out = previous_point;
    return true;
  }
  if (!group.root_mesh.empty() && group.root_mesh != point.mesh &&
      local_chain_position(character, group.root_mesh, out)) {
    return true;
  }
  if (!bone.parent.empty() && local_chain_position(character, bone.parent, out)) {
    return true;
  }
  return local_chain_position(character, point.mesh, out);
}

static Vec3 blend_vec(Vec3 a, Vec3 b, float t) {
  return vadd(vscale(a, 1.0f - t), vscale(b, t));
}

static Vec3 enforce_hair_collision(const Character& character,
                                   const CharHairPoint& point,
                                   Vec3 solved) {
  if (point.parent.empty() || point.radius <= 0.0f) return solved;
  std::array<float, 16> collision_world{};
  if (!transform_local_chain_world(character, point.parent, collision_world)) {
    return solved;
  }
  const Vec3 center = mat_pos(collision_world);
  Vec3 from_center = vsub(solved, center);
  float dist = vlen(from_center);
  if (dist <= 1e-5f) {
    from_center = {1.0f, 0.0f, 0.0f};
    dist = 1.0f;
  }
  const Vec3 dir = vscale(from_center, 1.0f / dist);
  switch (point.flags_or_mode) {
    case 1:  // kCollideSphere
      if (dist < point.radius) solved = vadd(center, vscale(dir, point.radius));
      break;
    case 2:  // kCollideInsideSphere
      if (dist > point.radius) solved = vadd(center, vscale(dir, point.radius));
      break;
    case 3: {  // kCollideCylinder
      const Vec3 axis = vnorm(mat_row(collision_world, 0), {0.0f, 0.0f, 1.0f});
      const float along = vdot(from_center, axis);
      const Vec3 on_axis = vadd(center, vscale(axis, along));
      Vec3 radial = vsub(solved, on_axis);
      float radial_len = vlen(radial);
      if (radial_len <= 1e-5f) {
        radial = vnorm(vcross(axis, {0.0f, 1.0f, 0.0f}), {1.0f, 0.0f, 0.0f});
        radial_len = 1.0f;
      }
      const Vec3 radial_dir = vscale(radial, 1.0f / radial_len);
      if (radial_len < point.radius) {
        solved = vadd(on_axis, vscale(radial_dir, point.radius));
      } else if (point.extra > 0.0f &&
                 radial_len < point.radius + point.extra) {
        const float t =
            1.0f - ((radial_len - point.radius) / point.extra);
        const Vec3 surface = vadd(on_axis, vscale(radial_dir, point.radius));
        solved = blend_vec(solved, surface, std::clamp(t, 0.0f, 1.0f));
      }
      break;
    }
    default:
      break;
  }
  return solved;
}

static void apply_char_hair(Character& character, float time_seconds) {
  if (disable_char_hair_enabled()) return;
  const size_t total_points = char_hair_point_count(character);
  if (total_points == 0) return;
  if (character.runtime_hair.points.size() != total_points) {
    character.runtime_hair.points.assign(total_points, RuntimeHairPoint{});
    character.runtime_hair.last_time_seconds = -1.0f;
  }

  float dt = 0.0f;
  if (character.runtime_hair.last_time_seconds >= 0.0f &&
      time_seconds >= character.runtime_hair.last_time_seconds) {
    dt = std::clamp(time_seconds - character.runtime_hair.last_time_seconds,
                    0.0f, 1.0f / 60.0f);
  }
  character.runtime_hair.last_time_seconds = time_seconds;

  size_t runtime_index = 0;
  for (const auto& hair : character.hairs) {
    const float stiffness = std::clamp(hair.globals[0], 0.0f, 1.0f);
    const float inertia = std::clamp(hair.globals[2], 0.0f, 1.0f);
    const float gravity = std::max(0.0f, hair.globals[3] * hair.globals[4]);
    const float friction = std::clamp(hair.globals[5], 0.0f, 1.0f);
    if (!hair.enabled) {
      for (const auto& group : hair.groups) runtime_index += group.points.size();
      continue;
    }
    for (const auto& group : hair.groups) {
      bool first_point = true;
      Vec3 previous_point{};
      const bool follow_only_group = group.points.size() == 1;
      for (const auto& point : group.points) {
        RuntimeHairPoint& state = character.runtime_hair.points[runtime_index++];
        TransformTarget target = find_transform_target(character, point.mesh);
        if (!target.local || !target.name || !target.parent ||
            target.parent->empty()) {
          continue;
        }

        const auto live_world_xfm =
            character.bone_world_local_chain(*target.name);
        const Vec3 live_world = mat_pos(live_world_xfm);
        if (!state.initialized || state.mesh != point.mesh) {
          state.initialized = true;
          state.mesh = point.mesh;
          set_runtime_point_pos(state, live_world, live_world);
        }

        if (follow_only_group) {
          set_runtime_point_world(state, live_world_xfm);
          previous_point = live_world;
          first_point = false;
          if (debug_char_hair_enabled()) {
            std::fprintf(stderr,
                         "[charhair-follow] %s point=%s root=%s coll=%s "
                         "live=(%.3f %.3f %.3f) len=%.3f mode=%u\n",
                         hair.name.c_str(), point.mesh.c_str(),
                         group.root_mesh.c_str(), point.parent.c_str(),
                         live_world.x, live_world.y, live_world.z,
                         point.length, point.flags_or_mode);
          }
          continue;
        }

        Vec3 anchor{};
        milo_scene::TransObj target_proxy;
        target_proxy.name = *target.name;
        target_proxy.parent = *target.parent;
        if (!hair_anchor_position(character, group, point, target_proxy,
                                  first_point, previous_point, anchor)) {
          continue;
        }

        Vec3 solved = live_world;
        if (dt > 0.0f) {
          const Vec3 curr = runtime_point_pos(state);
          const Vec3 prev = runtime_point_prev(state);
          const float damping =
              std::clamp(inertia * (1.0f - friction), 0.0f, 0.98f);
          Vec3 predicted = vadd(curr, vscale(vsub(curr, prev), damping));
          predicted.z -= 980.0f * gravity * dt * dt;
          predicted = blend_vec(predicted, live_world, stiffness);

          const float length =
              std::max(0.001f, point.length > 0.0f
                                   ? point.length
                                   : vlen(vsub(live_world, anchor)));
          Vec3 dir = vsub(predicted, anchor);
          if (vlen(dir) <= 1e-5f) dir = vsub(live_world, anchor);
          dir = vnorm(dir, {0.0f, 0.0f, -1.0f});
          solved = vadd(anchor, vscale(dir, length));
          solved = enforce_hair_collision(character, point, solved);
          solved = blend_vec(solved, live_world, stiffness * 0.35f);
        }

        const auto actual_parent_world =
            character.bone_world_local_chain(*target.parent);
        auto desired_world = live_world_xfm;
        desired_world[12] = solved.x;
        desired_world[13] = solved.y;
        desired_world[14] = solved.z;
        set_runtime_point_world(state, desired_world);
        set_local_from_world(*target.local, desired_world, actual_parent_world);
        const Vec3 old_curr = runtime_point_pos(state);
        set_runtime_point_pos(state, solved, old_curr);
        previous_point = solved;
        first_point = false;

        if (debug_char_hair_enabled()) {
          const Vec3 authored{point.pos[0], point.pos[1], point.pos[2]};
          const Vec3 authored_from_anchor = vsub(authored, anchor);
          const Vec3 live_from_authored = vsub(live_world, authored);
          std::fprintf(stderr,
                       "[charhair] %s point=%s root=%s coll=%s mode=%u "
                       "authored=(%.3f %.3f %.3f) "
                       "anchor=(%.3f %.3f %.3f) live=(%.3f %.3f %.3f) "
                       "solved=(%.3f %.3f %.3f) "
                       "auth-anchor=(%.3f %.3f %.3f) "
                       "live-auth=(%.3f %.3f %.3f) "
                       "len=%.3f radius=%.3f align=%.3f dt=%.4f\n",
                       hair.name.c_str(), point.mesh.c_str(),
                       group.root_mesh.c_str(), point.parent.c_str(),
                       point.flags_or_mode, authored.x, authored.y, authored.z,
                       anchor.x, anchor.y, anchor.z,
                       live_world.x, live_world.y, live_world.z,
                       solved.x, solved.y, solved.z, authored_from_anchor.x,
                       authored_from_anchor.y, authored_from_anchor.z,
                       live_from_authored.x, live_from_authored.y,
                       live_from_authored.z, point.length, point.radius,
                       point.extra, dt);
        }
      }
    }
  }
}

static int fret_spot_for_note_mask(uint32_t note_mask,
                                   const std::string& hand_map) {
  if ((note_mask & 0x1fu) == 0) return 0;
  int lane = 0;
  while (lane < 5 && ((note_mask & (1u << lane)) == 0)) ++lane;
  if (lane >= 5) return 0;

  // The authored neutral row for GH2 guitarist bodies places bone_fret.mesh
  // at spot_neck_fret04.mesh. Spread the five lanes over the 20 authored neck
  // spots from that source row; hand-map events can shift this base position.
  int spot = 4 + lane * 3;
  if (hand_map.find("DropD2") != std::string::npos) spot += 2;
  return std::clamp(spot, 1, 20);
}

void apply_ik_midi_fret_target(Character& character, uint32_t note_mask,
                               const std::string& hand_map) {
  const int spot_index = fret_spot_for_note_mask(note_mask, hand_map);
  if (spot_index <= 0) return;
  char spot_name[32];
  std::snprintf(spot_name, sizeof(spot_name), "spot_neck_fret%02d.mesh",
                spot_index);
  const milo_scene::TransObj* spot = find_bone(character, spot_name);
  if (!spot) return;

  for (const auto& ik : character.ik_midis) {
    milo_scene::TransObj* bone = find_bone(character, ik.bone);
    if (!bone) continue;
    if (bone->parent == spot->parent) {
      bone->local = spot->local;
    } else {
      milo_scene::Xfm moved = bone->local;
      const auto spot_world = character.bone_world_local_chain(spot->name);
      const auto parent_world =
          bone->parent.empty()
              ? std::array<float, 16>{1, 0, 0, 0, 0, 1, 0, 0,
                                      0, 0, 1, 0, 0, 0, 0, 1}
              : character.bone_world_local_chain(bone->parent);
      set_local_from_world(moved, spot_world, parent_world);
      bone->local = moved;
    }
    if (debug_ik_enabled()) {
      std::fprintf(stderr, "[ikmidi] %s bone=%s -> %s map=%s mask=0x%02x\n",
                   ik.name.c_str(), ik.bone.c_str(), spot_name,
                   hand_map.c_str(), note_mask & 0x1fu);
    }
  }
}

void clear_runtime_ik_weights(Character& character) {
  character.runtime_weight_props.clear();
}

void set_runtime_ik_weight(Character& character, const std::string& weight_prop,
                           float weight) {
  if (weight_prop.empty()) return;
  character.runtime_weight_props[weight_prop] = std::clamp(weight, 0.0f, 1.0f);
}

void apply_character_controllers(Character& character, float time_seconds,
                                 FaceFxEyeProperties* eye_props) {
  (void)time_seconds;
  if (eye_props) *eye_props = {};
  log_character_controller_graph_once(character);
  std::vector<milo_scene::Xfm> bind_bones = character.bind_bone_local;
  if (bind_bones.size() != character.bones.size()) {
    bind_bones.clear();
    bind_bones.reserve(character.bones.size());
    for (const auto& b : character.bones) bind_bones.push_back(b.local);
  }
  std::unordered_set<std::string> fore_twists_applied;
  if (ps2_ik_hands_enabled())
    apply_ps2_ik_hand_targets(character, bind_bones, fore_twists_applied);
  if (arm_ik_enabled()) apply_legacy_ik_hands(character);
  apply_driven_twists(character, bind_bones, fore_twists_applied);
  apply_char_hair(character, time_seconds);

  if (debug_face_enabled()) {
    for (const auto& m : character.meshes) {
      if (m.name != "eye-L.mesh" && m.name != "eye-R.mesh") continue;
      const auto parent_world = character.attachment_parent_world(m.parent);
      const auto eye_world = character.mesh_attachment_world(m, false);
      const Vec3 head_pos = mat_pos(parent_world);
      const Vec3 eye_pos = mat_pos(eye_world);
      std::fprintf(stderr,
                   "[face] eye %s parent=%s head=(%.3f %.3f %.3f) eye=(%.3f %.3f %.3f) local=(%.3f %.3f %.3f) rows=[%.3f %.3f %.3f|%.3f %.3f %.3f|%.3f %.3f %.3f]\n",
                   m.name.c_str(), m.parent.c_str(), head_pos.x, head_pos.y,
                   head_pos.z, eye_pos.x, eye_pos.y, eye_pos.z, m.local.pos[0],
                   m.local.pos[1], m.local.pos[2], m.local.rot[0][0],
                   m.local.rot[0][1], m.local.rot[0][2], m.local.rot[1][0],
                   m.local.rot[1][1], m.local.rot[1][2], m.local.rot[2][0],
                   m.local.rot[2][1], m.local.rot[2][2]);
    }
  }

  if (!disable_lookat_enabled()) {
    for (const auto& look : character.lookats) {
      if (look.weight <= 0.0f || look.driven.empty()) continue;
      const int driven_i = find_mesh_index(character, look.driven);
      if (driven_i < 0 ||
          static_cast<size_t>(driven_i) >= character.meshes.size())
        continue;
      auto& eye = character.meshes[static_cast<size_t>(driven_i)];
      if (eye.parent.empty()) continue;

      const auto parent_world = character.attachment_parent_world(eye.parent);
      const auto eye_world = character.mesh_attachment_world(eye, false);
      const Vec3 eye_pos = mat_pos(eye_world);

      std::array<float, 16> target_world{};
      if (!transform_world(character, look.target, target_world)) {
        continue;
      }

      const Vec3 target_pos = mat_pos(target_world);
      const Vec3 head_front = vnorm(mat_row(parent_world, 1), {0, 1, 0});
      const Vec3 head_right = vnorm(mat_row(parent_world, 2), {1, 0, 0});
      const Vec3 head_up = vnorm(mat_row(parent_world, 0), {0, 0, 1});

      std::array<float, 16> source_world{};
      Vec3 source_pos{};
      if (transform_world(character, look.source, source_world)) {
        source_pos = mat_pos(source_world);
      } else if (look.source == look.name) {
        // PS2 traces for self-sourced CharLookAt objects update source eye rows
        // through the shared CharEyes pivot/head graph; they do not carry a
        // large authored downward source offset. Keep the synthetic native
        // fallback forward-facing and let the decoded look-at offsets/limits
        // provide the small per-character eye bias.
        const float dist = 100.0f;
        source_pos = vadd(target_pos, vscale(head_front, dist));
      } else {
        continue;
      }

      Vec3 raw_dir = vsub(source_pos, target_pos);
      if (vlen(raw_dir) <= 1e-5f) raw_dir = vsub(source_pos, eye_pos);
      if (vdot(raw_dir, head_front) < 0.0f) raw_dir = vscale(raw_dir, -1.0f);
      raw_dir = vnorm(raw_dir, head_front);

      const float deg = 3.14159265358979323846f / 180.0f;
      const float hz_min = look.min_x * deg;
      const float hz_max = look.max_x * deg;
      const float vt_min = look.min_z * deg;
      const float vt_max = look.max_z * deg;
      const float hz_offset = look.offset_x * deg;
      const float vt_offset = look.offset_z * deg;
      const float f = std::max(1e-5f, vdot(raw_dir, head_front));
      const float yaw = std::clamp(std::atan2(vdot(raw_dir, head_right), f) +
                                       hz_offset,
                                   hz_min, hz_max);
      const float pitch = std::clamp(std::atan2(vdot(raw_dir, head_up), f) +
                                         vt_offset,
                                     vt_min, vt_max);
      if (eye_props) {
        auto norm_axis = [](float v, float neg, float pos) {
          if (v >= 0.0f)
            return pos > 1e-5f ? std::clamp(v / pos, 0.0f, 1.0f) : 0.0f;
          return neg < -1e-5f ? -std::clamp(v / neg, 0.0f, 1.0f) : 0.0f;
        };
        const float x = norm_axis(yaw, hz_min, hz_max);
        const float z = norm_axis(pitch, vt_min, vt_max);
        if (look.driven == "eye-L.mesh" || look.target == "eye-L.mesh") {
          eye_props->l_eye_x = x;
          eye_props->l_eye_z = z;
          eye_props->has_l_eye_x = true;
          eye_props->has_l_eye_z = true;
        } else if (look.driven == "eye-R.mesh" ||
                   look.target == "eye-R.mesh") {
          eye_props->r_eye_x = x;
          eye_props->r_eye_z = z;
          eye_props->has_r_eye_x = true;
          eye_props->has_r_eye_z = true;
        }
      }

      Vec3 target_dir = vadd(head_front,
                             vadd(vscale(head_right, std::tan(yaw)),
                                  vscale(head_up, std::tan(pitch))));
      target_dir = vnorm(target_dir, head_front);

      const auto desired_world = aim_preserve_xfm(
          eye_pos, mat_row(eye_world, 1), target_dir, eye_world);
      set_local_from_world(eye.local, desired_world, parent_world);
      if (debug_face_enabled()) {
        std::fprintf(
            stderr,
            "[lookat] %s driven=%s source=%s target=%s eye=(%.3f %.3f %.3f) "
            "source_pos=(%.3f %.3f %.3f) target_pos=(%.3f %.3f %.3f) "
            "raw_dir=(%.4f %.4f %.4f) target_dir=(%.4f %.4f %.4f) "
            "yaw=%.4f pitch=%.4f\n",
            look.name.c_str(), look.driven.c_str(), look.source.c_str(),
            look.target.c_str(), eye_pos.x, eye_pos.y, eye_pos.z,
            source_pos.x, source_pos.y, source_pos.z, target_pos.x,
            target_pos.y, target_pos.z, raw_dir.x, raw_dir.y, raw_dir.z,
            target_dir.x, target_dir.y, target_dir.z, yaw, pitch);
      }
    }
  }

  apply_ps2_upper_twists(character, bind_bones);
}

void apply_clip_pose(const std::vector<ClipChannel>& channels, Character& character) {
  apply_clip_pose_weighted(channels, 1.0f, character);
}

static void apply_clip_pose_sampled_direct(
    const std::vector<ClipChannel>& channels, float weight, Character& character,
    bool relative) {
  std::vector<PendingPose> poses(character.bones.size());
  std::vector<PendingPose> mesh_poses(character.meshes.size());
  std::vector<ClipChannel> channel_overrides;
  channel_overrides.reserve(channels.size());
  for (const auto& ch : channels) {
    const ClipChannel* source = &ch;
    if (swap_thigh_quats_enabled() && ch.type == ClipChannel::kQuat) {
      const auto l = ch.bone_name.find("bone_L-thigh");
      const auto r = ch.bone_name.find("bone_R-thigh");
      if (l != std::string::npos || r != std::string::npos) {
        channel_overrides.push_back(ch);
        channel_overrides.back().bone_name =
            (l != std::string::npos) ? "bone_R-thigh" : "bone_L-thigh";
        source = &channel_overrides.back();
      }
    }
    if (invert_thigh_quats_enabled() && source->type == ClipChannel::kQuat &&
        source->bone_name.find("-thigh") != std::string::npos) {
      channel_overrides.push_back(*source);
      channel_overrides.back().quat[0] = -channel_overrides.back().quat[0];
      channel_overrides.back().quat[1] = -channel_overrides.back().quat[1];
      channel_overrides.back().quat[2] = -channel_overrides.back().quat[2];
      source = &channel_overrides.back();
    }
    const ClipChannel& applied = *source;
    if (disable_finger_clip_channels_enabled() &&
        is_finger_bone_name(applied.bone_name)) {
      continue;
    }
    if (disable_axis_rot_channels_enabled() &&
        (applied.type == ClipChannel::kRotX || applied.type == ClipChannel::kRotY ||
         applied.type == ClipChannel::kRotZ)) {
      continue;
    }
    if (disable_thigh_quat_channels_enabled() &&
        applied.type == ClipChannel::kQuat &&
        applied.bone_name.find("-thigh") != std::string::npos) {
      continue;
    }
    if (disable_foot_quat_channels_enabled() &&
        applied.type == ClipChannel::kQuat &&
        applied.bone_name.find("-foot") != std::string::npos) {
      continue;
    }
    if (disable_leg_axis_channels_enabled() &&
        (applied.type == ClipChannel::kRotX || applied.type == ClipChannel::kRotY ||
         applied.type == ClipChannel::kRotZ) &&
        (applied.bone_name.find("-knee") != std::string::npos ||
         applied.bone_name.find("-toe") != std::string::npos)) {
      continue;
    }
    bool matched = false;
    for (size_t i = 0; i < character.bones.size(); ++i) {
      if (!channel_matches_bone(character.bones[i].name, applied.bone_name)) continue;
      switch (applied.type) {
        case ClipChannel::kPos: poses[i].pos = source; break;
        case ClipChannel::kScale: poses[i].scale = source; break;
        case ClipChannel::kQuat: poses[i].quat = source; break;
        case ClipChannel::kRotX: poses[i].rotx = source; break;
        case ClipChannel::kRotY: poses[i].roty = source; break;
        case ClipChannel::kRotZ: poses[i].rotz = source; break;
      }
      matched = true;
      break;
    }
    if (matched) continue;
    for (size_t i = 0; i < character.meshes.size(); ++i) {
      if (!channel_matches_bone(character.meshes[i].name, applied.bone_name)) continue;
      switch (applied.type) {
        case ClipChannel::kPos: mesh_poses[i].pos = source; break;
        case ClipChannel::kScale: mesh_poses[i].scale = source; break;
        case ClipChannel::kQuat: mesh_poses[i].quat = source; break;
        case ClipChannel::kRotX: mesh_poses[i].rotx = source; break;
        case ClipChannel::kRotY: mesh_poses[i].roty = source; break;
        case ClipChannel::kRotZ: mesh_poses[i].rotz = source; break;
      }
      break;
    }
  }

  for (size_t i = 0; i < character.bones.size(); ++i) {
    if (world_clip_quat_enabled() && poses[i].quat && weight >= 0.999f) {
      PendingPose non_quat = poses[i];
      non_quat.quat = nullptr;
      apply_pending_pose_weighted(non_quat, character.bones[i].local, weight,
                                  relative);

      float target_world[3][3];
      quat_to_rot(poses[i].quat->quat, target_world);
      std::array<float, 16> parent_world{1, 0, 0, 0, 0, 1, 0, 0,
                                         0, 0, 1, 0, 0, 0, 0, 1};
      if (!character.bones[i].parent.empty()) {
        parent_world = character.bone_world(character.bones[i].parent);
      }
      float parent_rot[3][3];
      for (int r = 0; r < 3; ++r) {
        float len = std::sqrt(parent_world[r * 4 + 0] * parent_world[r * 4 + 0] +
                              parent_world[r * 4 + 1] * parent_world[r * 4 + 1] +
                              parent_world[r * 4 + 2] * parent_world[r * 4 + 2]);
        if (len <= 1e-6f) len = 1.0f;
        for (int c = 0; c < 3; ++c) parent_rot[r][c] = parent_world[r * 4 + c] / len;
      }
      float local_rot[3][3] = {};
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          for (int k = 0; k < 3; ++k) {
            local_rot[r][c] += target_world[r][k] * parent_rot[c][k];
          }
        }
      }
      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
          character.bones[i].local.rot[r][c] = local_rot[r][c];
    } else {
      apply_pending_pose_weighted(poses[i], character.bones[i].local, weight,
                                  relative);
    }
  }
  for (size_t i = 0; i < character.meshes.size(); ++i) {
    apply_pending_pose_weighted(mesh_poses[i], character.meshes[i].local, weight,
                                relative);
  }
  if (debug_leg_pose_enabled()) dump_leg_pose(character);
}

void apply_clip_pose_sampled(const std::vector<ClipChannel>& channels,
                             float weight, Character& character,
                             bool relative) {
  apply_clip_pose_sampled_direct(channels, weight, character, relative);
}

void apply_clip_pose_weighted(const std::vector<ClipChannel>& channels,
                              float weight, Character& character,
                              bool relative) {
  apply_clip_pose_sampled(channels, weight, character, relative);
}

void apply_clip_frame(const CharClip& clip, int frame_idx, Character& character) {
  if (clip.frames.empty()) return;
  int fi = std::clamp(frame_idx, 0, (int)clip.frames.size() - 1);
  if (!apply_clip_pose_output_layer(clip.frames[(size_t)fi], 1.0f, character,
                                    clip.relative, clip.output_bones)) {
    apply_clip_pose_sampled_direct(clip.frames[(size_t)fi], 1.0f, character,
                                   clip.relative);
  }
}

void apply_clip_frame_weighted(const CharClip& clip, int frame_idx,
                               float weight, Character& character) {
  if (clip.frames.empty()) return;
  int fi = std::clamp(frame_idx, 0, (int)clip.frames.size() - 1);
  if (!apply_clip_pose_output_layer(clip.frames[(size_t)fi], weight, character,
                                    clip.relative, clip.output_bones)) {
    apply_clip_pose_sampled_direct(clip.frames[(size_t)fi], weight, character,
                                   clip.relative);
  }
}

float CharClip::duration_seconds() const {
  if (frames.empty()) return 0.0f;
  const float rate = fps > 0 ? static_cast<float>(fps) : 30.0f;
  float first = start_frame;
  float last = end_frame;
  if (last < first || last <= 0.0f) {
    first = 0.0f;
    last = static_cast<float>(frames.size() - 1);
  }
  return std::max(0.0f, (last - first + 1.0f) / rate);
}

namespace {

bool play_flags_loop(const CharClip& clip, uint32_t flags) {
  uint32_t loop_mode = flags & 0x70u;
  if (loop_mode == 0) loop_mode = clip.default_play_flags & 0x70u;
  return loop_mode != kCharPlayNoLoop;
}

uint32_t play_mode(const CharClip& clip, uint32_t flags) {
  uint32_t mode = flags & 0x0fu;
  if (mode == kCharPlayNoDefault) mode = clip.default_play_flags & 0x0fu;
  return mode;
}

int clip_frame_at_time(const CharClip& clip, float seconds, uint32_t flags) {
  if (clip.frames.empty()) return 0;
  const float rate = clip.fps > 0 ? static_cast<float>(clip.fps) : 30.0f;
  float first = clip.start_frame;
  float last = clip.end_frame;
  if (last < first || last <= 0.0f) {
    first = 0.0f;
    last = static_cast<float>(clip.frames.size() - 1);
  }

  const float frame_count = std::max(1.0f, last - first + 1.0f);
  float frame = first + seconds * rate;
  if (play_flags_loop(clip, flags)) {
    frame = std::fmod(frame - first, frame_count);
    if (frame < 0.0f) frame += frame_count;
    frame += first;
  } else {
    frame = std::clamp(frame, first, last);
  }

  return std::clamp(static_cast<int>(std::floor(frame)), 0,
                    static_cast<int>(clip.frames.size()) - 1);
}

float clip_frame_float_at_time(const CharClip& clip, float seconds,
                               uint32_t flags) {
  if (clip.frames.empty()) return 0.0f;
  const float rate = clip.fps > 0 ? static_cast<float>(clip.fps) : 30.0f;
  float first = clip.start_frame;
  float last = clip.end_frame;
  if (last < first || last <= 0.0f) {
    first = 0.0f;
    last = static_cast<float>(clip.frames.size() - 1);
  }

  const float frame_count = std::max(1.0f, last - first + 1.0f);
  float frame = first + seconds * rate;
  if (play_flags_loop(clip, flags)) {
    frame = std::fmod(frame - first, frame_count);
    if (frame < 0.0f) frame += frame_count;
    frame += first;
  } else {
    frame = std::clamp(frame, first, last);
  }
  return std::clamp(frame, 0.0f,
                    static_cast<float>(clip.frames.size() - 1));
}

const ClipChannel* matching_channel(const std::vector<ClipChannel>& frame,
                                    const ClipChannel& needle) {
  for (const auto& ch : frame) {
    if (ch.type == needle.type && ch.bone_name == needle.bone_name) return &ch;
  }
  return nullptr;
}

void blend_channel_into(ClipChannel& out, const ClipChannel& rhs, float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  switch (out.type) {
    case ClipChannel::kPos:
      for (int i = 0; i < 3; ++i)
        out.pos[i] = out.pos[i] * (1.0f - t) + rhs.pos[i] * t;
      break;
    case ClipChannel::kScale:
      for (int i = 0; i < 3; ++i)
        out.scale[i] = out.scale[i] * (1.0f - t) + rhs.scale[i] * t;
      break;
    case ClipChannel::kQuat: {
      float dot = 0.0f;
      for (int i = 0; i < 4; ++i) dot += out.quat[i] * rhs.quat[i];
      float len = 0.0f;
      for (int i = 0; i < 4; ++i) {
        const float b = dot < 0.0f ? -rhs.quat[i] : rhs.quat[i];
        out.quat[i] = out.quat[i] * (1.0f - t) + b * t;
        len += out.quat[i] * out.quat[i];
      }
      len = std::sqrt(std::max(len, 1e-8f));
      for (float& q : out.quat) q /= len;
      break;
    }
    case ClipChannel::kRotX:
    case ClipChannel::kRotY:
    case ClipChannel::kRotZ:
      out.angle = out.angle * (1.0f - t) + rhs.angle * t;
      break;
  }
}

std::vector<ClipChannel> interpolate_frame(const CharClip& clip, float frame) {
  if (clip.frames.empty()) return {};
  const int f0 = std::clamp(static_cast<int>(std::floor(frame)), 0,
                            static_cast<int>(clip.frames.size()) - 1);
  const int f1 = std::min(f0 + 1, static_cast<int>(clip.frames.size()) - 1);
  const float t = std::clamp(frame - static_cast<float>(f0), 0.0f, 1.0f);
  if (f0 == f1 || t <= 1e-4f) return clip.frames[(size_t)f0];

  std::vector<ClipChannel> out = clip.frames[(size_t)f0];
  const auto& next = clip.frames[(size_t)f1];
  for (auto& ch : out) {
    const ClipChannel* b = matching_channel(next, ch);
    if (!b) continue;
    blend_channel_into(ch, *b, t);
  }
  return out;
}

std::vector<ClipChannel> blend_channel_sets(std::vector<ClipChannel> previous,
                                            const std::vector<ClipChannel>& current,
                                            float current_weight) {
  current_weight = std::clamp(current_weight, 0.0f, 1.0f);
  if (previous.empty() || current_weight >= 0.999f) return current;
  if (current.empty() || current_weight <= 0.001f) return previous;

  for (const auto& ch : current) {
    bool matched = false;
    for (auto& out : previous) {
      if (out.type != ch.type || out.bone_name != ch.bone_name) continue;
      blend_channel_into(out, ch, current_weight);
      matched = true;
      break;
    }
    if (!matched) previous.push_back(ch);
  }
  return previous;
}

const char* channel_type_name(ClipChannel::Type type) {
  switch (type) {
    case ClipChannel::kPos: return "pos";
    case ClipChannel::kScale: return "scale";
    case ClipChannel::kQuat: return "quat";
    case ClipChannel::kRotX: return "rotx";
    case ClipChannel::kRotY: return "roty";
    case ClipChannel::kRotZ: return "rotz";
  }
  return "?";
}

bool lane_mixer_interesting_channel(const std::string& bone_name) {
  return bone_name.find("hand") != std::string::npos ||
         bone_name.find("finger") != std::string::npos ||
         bone_name.find("thumb") != std::string::npos ||
         bone_name.find("fret") != std::string::npos ||
         bone_name.find("strum") != std::string::npos ||
         bone_name.find("clavicle") != std::string::npos ||
         bone_name.find("upperArm") != std::string::npos ||
         bone_name.find("foreArm") != std::string::npos ||
         bone_name.find("foreTwist") != std::string::npos ||
         bone_name.find("upperTwist") != std::string::npos;
}

void dump_lane_mixer_layers(const std::vector<ClipChannelLayer>& layers) {
  if (!debug_lane_mixer_enabled() || layers.empty()) return;

  std::string signature;
  for (size_t i = 0; i < layers.size(); ++i) {
    const auto& layer = layers[i];
    const std::string name = layer.debug_name.empty()
                                 ? ("layer" + std::to_string(i))
                                 : layer.debug_name;
    signature += name + ":" + std::to_string(layer.channels.size()) + ":" +
                 std::to_string(static_cast<int>(layer.weight * 1000.0f)) +
                 ";";
  }

  static std::unordered_set<std::string> seen_signatures;
  if (!seen_signatures.insert(signature).second) return;

  std::fprintf(stderr, "[lane-mix] layers=%zu signature=%s\n", layers.size(),
               signature.c_str());
  std::unordered_map<std::string, std::vector<std::string>> owners;
  for (size_t i = 0; i < layers.size(); ++i) {
    const auto& layer = layers[i];
    const std::string name = layer.debug_name.empty()
                                 ? ("layer" + std::to_string(i))
                                 : layer.debug_name;
    std::fprintf(stderr,
                 "[lane-mix]   layer %zu name=%s weight=%.3f channels=%zu "
                 "outputBones=%zu\n",
                 i, name.c_str(), layer.weight, layer.channels.size(),
                 layer.output_bones ? layer.output_bones->size() : 0);
    for (const auto& ch : layer.channels) {
      if (!lane_mixer_interesting_channel(ch.bone_name)) continue;
      const std::string key =
          std::string(channel_type_name(ch.type)) + ":" + ch.bone_name;
      owners[key].push_back(name);
    }
  }

  int printed = 0;
  for (const auto& [key, names] : owners) {
    if (names.size() < 2) continue;
    if (printed++ >= 96) {
      std::fprintf(stderr, "[lane-mix]   collision <truncated>\n");
      break;
    }
    std::fprintf(stderr, "[lane-mix]   collision %s <-", key.c_str());
    for (const auto& name : names) std::fprintf(stderr, " %s", name.c_str());
    std::fprintf(stderr, "\n");
  }
}

}  // namespace

std::vector<ClipChannel> blend_channel_layers(
    const std::vector<ClipChannelLayer>& layers) {
  dump_lane_mixer_layers(layers);
  struct AccumRef {
    size_t index = 0;
    float weight = 0.0f;
  };
  auto key_for = [](const ClipChannel& ch) {
    return std::to_string(static_cast<int>(ch.type)) + "\n" + ch.bone_name;
  };

  std::vector<ClipChannel> out;
  std::unordered_map<std::string, AccumRef> by_key;
  for (const auto& layer : layers) {
    const float layer_weight = std::max(0.0f, layer.weight);
    if (layer_weight <= 0.0f) continue;
    for (const auto& ch : layer.channels) {
      const std::string key = key_for(ch);
      const auto it = by_key.find(key);
      if (it == by_key.end()) {
        by_key.emplace(key, AccumRef{out.size(), layer_weight});
        out.push_back(ch);
        continue;
      }

      AccumRef& acc = it->second;
      const float total = acc.weight + layer_weight;
      if (total <= 0.0f) continue;
      blend_channel_into(out[acc.index], ch, layer_weight / total);
      acc.weight = total;
    }
  }
  return out;
}

void apply_clip_channel_layers(const std::vector<ClipChannelLayer>& layers,
                               Character& character, bool relative) {
  const auto frame = blend_channel_layers(layers);
  if (frame.empty()) return;

  std::vector<CharClip::OutputBone> output_bones;
  std::unordered_set<std::string> output_keys;
  for (const auto& layer : layers) {
    if (!layer.output_bones) continue;
    for (const auto& out : *layer.output_bones) {
      const std::string key = strip_transform_suffix(out.name);
      if (!output_keys.insert(key).second) continue;
      output_bones.push_back(out);
    }
  }

  if (apply_clip_pose_output_layer(frame, 1.0f, character, relative,
                                   output_bones)) {
    return;
  }
  apply_clip_pose_sampled_direct(frame, 1.0f, character, relative);
}

void CharClipPlayer::clear() {
  layers_.clear();
}

const CharClip* CharClipPlayer::current_clip() const {
  return layers_.empty() ? nullptr : layers_.back().clip;
}

void CharClipPlayer::play(const CharClip& clip, uint32_t flags,
                          float blend_width, float speed) {
  if (!clip.loaded || clip.frames.empty()) return;
  const float resolved_blend =
      blend_width >= 0.0f ? blend_width : std::max(0.0f, clip.blend_width);
  const bool no_blend =
      play_mode(clip, flags) == kCharPlayNoBlend ||
      resolved_blend <= 0.0f || layers_.empty();

  Layer next;
  next.clip = &clip;
  next.flags = flags;
  next.blend_width = no_blend ? 0.0f : resolved_blend;
  next.speed = speed;

  if (no_blend) {
    layers_.clear();
  } else if (layers_.size() > 1) {
    Layer prev = layers_.back();
    layers_.clear();
    layers_.push_back(prev);
  }
  layers_.push_back(next);
}

void CharClipPlayer::advance(float dt_seconds) {
  if (layers_.empty()) return;
  for (auto& layer : layers_) {
    if (!layer.clip) continue;
    layer.time_seconds += dt_seconds * layer.speed;
    const float duration = layer.clip->duration_seconds();
    if (duration > 0.0f) {
      if (play_flags_loop(*layer.clip, layer.flags)) {
        layer.time_seconds = std::fmod(layer.time_seconds, duration);
        if (layer.time_seconds < 0.0f) layer.time_seconds += duration;
      } else {
        layer.time_seconds = std::clamp(layer.time_seconds, 0.0f, duration);
      }
    }
  }
  Layer& current = layers_.back();
  if (current.blend_width > 0.0f) {
    current.blend_progress += std::max(0.0f, dt_seconds);
    if (current.blend_progress >= current.blend_width && layers_.size() > 1) {
      Layer keep = current;
      keep.blend_width = 0.0f;
      keep.blend_progress = 0.0f;
      layers_.clear();
      layers_.push_back(keep);
    }
  }
}

void CharClipPlayer::apply(Character& character, float weight) const {
  if (layers_.empty() || weight <= 0.0f) return;
  weight = std::clamp(weight, 0.0f, 1.0f);
  const auto frame = sampled_pose();
  if (frame.empty()) return;
  const bool relative = sampled_pose_relative();
  const CharClip* current = current_clip();
  if (current &&
      apply_clip_pose_output_layer(frame, weight, character, relative,
                                   current->output_bones)) {
    return;
  }
  apply_clip_pose_sampled_direct(frame, weight, character, relative);
}

std::vector<ClipChannel> CharClipPlayer::sampled_pose() const {
  if (layers_.empty()) return {};
  const Layer& current = layers_.back();
  float current_weight = 1.0f;
  if (current.blend_width > 0.0f) {
    current_weight =
        std::clamp(current.blend_progress / current.blend_width, 0.0f, 1.0f);
  }

  if (layers_.size() > 1) {
    const Layer& previous = layers_[layers_.size() - 2];
    if (previous.clip) {
      const float ff = clip_frame_float_at_time(*previous.clip,
                                                previous.time_seconds,
                                                previous.flags);
      const auto previous_frame = interpolate_frame(*previous.clip, ff);
      if (current.clip) {
        const float current_ff = clip_frame_float_at_time(*current.clip,
                                                          current.time_seconds,
                                                          current.flags);
        const auto current_frame = interpolate_frame(*current.clip, current_ff);
        return blend_channel_sets(previous_frame, current_frame,
                                  current_weight);
      }
      return previous_frame;
    }
  }

  if (current.clip) {
    const float ff = clip_frame_float_at_time(*current.clip,
                                              current.time_seconds,
                                              current.flags);
    return interpolate_frame(*current.clip, ff);
  }
  return {};
}

bool CharClipPlayer::sampled_pose_relative() const {
  const CharClip* clip = current_clip();
  return clip && clip->relative;
}

// Legacy single-frame entry point (frame 0).
std::vector<ClipChannel> load_clip_pose(const std::string& hdr_path,
                                        const std::string& ark_path,
                                        const std::string& milo_path,
                                        const std::string& clip_name) {
  CharClip c = load_clip(hdr_path, ark_path, milo_path, clip_name);
  if (c.frames.empty()) return {};
  return c.frames[0];
}

}  // namespace ghogx::character
