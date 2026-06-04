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
#include <cstdlib>
#include <cstring>
#include <stdexcept>

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
  ch.angle = (comp ? read_snorm16(c) : c.f32()) * 3.14159265358979323846f;
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
        std::fprintf(stderr, "[clip] '%s': %zu frames, %zu channels/frame\n",
                     clip_name.c_str(), result.frames.size(),
                     result.frames.empty() ? 0 : result.frames[0].size());
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
  rot[0][0] = 1 - 2*(y*y + z*z);  rot[0][1] = 2*(x*y + z*w);      rot[0][2] = 2*(x*z - y*w);
  rot[1][0] = 2*(x*y - z*w);      rot[1][1] = 1 - 2*(x*x + z*z);  rot[1][2] = 2*(y*z + x*w);
  rot[2][0] = 2*(x*z + y*w);      rot[2][1] = 2*(y*z - x*w);      rot[2][2] = 1 - 2*(x*x + y*y);
}

static bool channel_matches_bone(const std::string& bone_name,
                                 const std::string& channel_bone_name) {
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

static bool has_transform(const Character& character, const std::string& name) {
  for (const auto& b : character.bones)
    if (b.name == name || channel_matches_bone(b.name, name)) return true;
  for (const auto& m : character.meshes)
    if (m.name == name || channel_matches_bone(m.name, name)) return true;
  return false;
}

static std::array<float, 16> mat4_mul(const std::array<float, 16>& a,
                                      const std::array<float, 16>& b);

static std::array<float, 16> xfm_to_mat4(const milo_scene::Xfm& x) {
  std::array<float, 16> m{};
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) m[r * 4 + c] = x.rot[r][c];
  m[12] = x.pos[0];
  m[13] = x.pos[1];
  m[14] = x.pos[2];
  m[15] = 1.0f;
  return m;
}

static std::array<float, 16> raw_bone_world(const Character& character,
                                            const std::string& name) {
  int idx = find_bone_index(character, name);
  if (idx < 0) {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  }
  std::array<float, 16> world = xfm_to_mat4(character.bones[(size_t)idx].local);
  std::string parent = character.bones[(size_t)idx].parent;
  int guard = 0;
  while (!parent.empty() && guard++ < 128) {
    idx = find_bone_index(character, parent);
    if (idx < 0) break;
    world = mat4_mul(world, xfm_to_mat4(character.bones[(size_t)idx].local));
    parent = character.bones[(size_t)idx].parent;
  }
  return world;
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
      out = character.mesh_world(m);
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
static void set_local_from_world(milo_scene::Xfm& local,
                                 const std::array<float, 16>& desired_world,
                                 const std::array<float, 16>& parent_world) {
  mat4_to_xfm(mat4_mul(desired_world, affine_inverse(parent_world)), local);
}

static void copy_local(const milo_scene::Xfm& src, milo_scene::Xfm& dst) {
  dst = src;
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

static void apply_driven_twists(Character& character,
                                const std::vector<milo_scene::Xfm>& bind_bones) {
  // GH2 CharUpperTwist distributes only local-X roll across helper bones. The
  // helpers are not keyed directly, but arm meshes are weighted to them.
  for (const auto& ut : character.upper_twists) {
    const int upper_i = find_bone_index(character, ut.upper_arm);
    const int twist1_i = find_bone_index(character, ut.twist1);
    const int twist2_i = find_bone_index(character, ut.twist2);
    if (upper_i < 0 || twist1_i < 0 || twist2_i < 0) continue;
    if ((size_t)upper_i >= bind_bones.size() ||
        (size_t)twist1_i >= bind_bones.size() ||
        (size_t)twist2_i >= bind_bones.size()) continue;
    const float roll =
        local_x_roll_delta(bind_bones[(size_t)upper_i],
                           character.bones[(size_t)upper_i].local);
    copy_local(character.bones[(size_t)upper_i].local,
               character.bones[(size_t)twist1_i].local);
    character.bones[(size_t)twist2_i].local = bind_bones[(size_t)twist2_i];
    post_rotate_axis(character.bones[(size_t)twist1_i].local,
                     ClipChannel::kRotX, roll / 3.0f);
    post_rotate_axis(character.bones[(size_t)twist2_i].local,
                     ClipChannel::kRotX, roll * 2.0f / 3.0f);
  }

  for (const auto& ft : character.fore_twists) {
    const int hand_i = find_bone_index(character, ft.hand);
    const int twist2_i = find_bone_index(character, ft.twist2);
    if (hand_i < 0 || twist2_i < 0) continue;
    const int fore_i = find_bone_index(character, character.bones[(size_t)hand_i].parent);
    const int twist1_i = find_bone_index(character, character.bones[(size_t)twist2_i].parent);
    if (fore_i < 0 || twist1_i < 0) continue;
    if ((size_t)hand_i >= bind_bones.size() ||
        (size_t)twist2_i >= bind_bones.size()) continue;

    // foreTwist1 is a sibling of foreArm at the forearm pivot, so it must carry
    // the forearm bend. foreTwist2 then carries the wrist roll component.
    copy_local(character.bones[(size_t)fore_i].local,
               character.bones[(size_t)twist1_i].local);
    character.bones[(size_t)twist2_i].local = bind_bones[(size_t)twist2_i];
    const float roll =
        local_x_roll_delta(bind_bones[(size_t)hand_i],
                           character.bones[(size_t)hand_i].local);
    post_rotate_axis(character.bones[(size_t)twist2_i].local,
                     ClipChannel::kRotX, roll * 0.5f);
  }
}

static void apply_ik_hands(Character& character) {
  for (const auto& ik : character.ik_hands) {
    const int hand_i = find_bone_index(character, ik.hand);
    if (hand_i < 0 || ik.weight <= 0.0f) continue;
    if (!has_transform(character, ik.target)) continue;
    auto& hand = character.bones[(size_t)hand_i];
    const int fore_i = find_bone_index(character, hand.parent);
    if (fore_i < 0) continue;
    auto& fore = character.bones[(size_t)fore_i];
    const int upper_i = find_bone_index(character, fore.parent);
    if (upper_i < 0) continue;
    auto& upper = character.bones[(size_t)upper_i];
    if (upper.parent.empty()) continue;

    const auto target_world = raw_bone_world(character, ik.target);
    const auto upper_parent_world = character.bone_world(upper.parent);
    const auto upper_world0 = character.bone_world(upper.name);
    const auto fore_world0 = character.bone_world(fore.name);
    const auto hand_world0 = character.bone_world(hand.name);
    if (debug_ik_enabled()) {
      const Vec3 hp = mat_pos(hand_world0);
      const Vec3 tp = mat_pos(target_world);
      const Vec3 raw_tp = mat_pos(raw_bone_world(character, ik.target));
      std::fprintf(stderr,
                   "[ik] %s hand=%s target=%s hand0=[%.2f %.2f %.2f] target=[%.2f %.2f %.2f] raw_target=[%.2f %.2f %.2f]\n",
                   ik.name.c_str(), ik.hand.c_str(), ik.target.c_str(),
                   hp.x, hp.y, hp.z, tp.x, tp.y, tp.z,
                   raw_tp.x, raw_tp.y, raw_tp.z);
    }

    const Vec3 shoulder = mat_pos(upper_world0);
    const Vec3 target = mat_pos(target_world);
    const float upper_len = std::max(0.001f, vlen({fore.local.pos[0], fore.local.pos[1], fore.local.pos[2]}));
    const float fore_len = std::max(0.001f, vlen({hand.local.pos[0], hand.local.pos[1], hand.local.pos[2]}));
    Vec3 to_target = vsub(target, shoulder);
    float dist = vlen(to_target);
    const float max_reach = upper_len + fore_len - 0.001f;
    const float min_reach = std::fabs(upper_len - fore_len) + 0.001f;
    dist = std::clamp(dist, min_reach, max_reach);
    const Vec3 target_dir = vnorm(to_target, mat_row(upper_world0, 0));

    Vec3 bend = vsub(mat_pos(fore_world0), shoulder);
    bend = vsub(bend, vscale(target_dir, vdot(bend, target_dir)));
    bend = vnorm(bend, mat_row(upper_world0, 1));

    const float a = (upper_len*upper_len - fore_len*fore_len + dist*dist) /
                    (2.0f * dist);
    const float h2 = std::max(0.0f, upper_len*upper_len - a*a);
    const Vec3 elbow = vadd(shoulder,
                            vadd(vscale(target_dir, a),
                                 vscale(bend, std::sqrt(h2))));

    const auto upper_desired_world =
        aim_preserve_xfm(shoulder, vsub(mat_pos(fore_world0), shoulder),
                         vsub(elbow, shoulder), upper_world0);
    set_local_from_world(upper.local, upper_desired_world, upper_parent_world);

    const auto upper_world1 = character.bone_world(upper.name);
    const Vec3 elbow1 = mat_pos(character.bone_world(fore.name));
    const auto fore_desired_world =
        aim_preserve_xfm(elbow1, vsub(mat_pos(hand_world0), mat_pos(fore_world0)),
                         vsub(target, elbow1), fore_world0);
    set_local_from_world(fore.local, fore_desired_world, upper_world1);

    if (ik.enable_rot) {
      const auto fore_world1 = character.bone_world(fore.name);
      milo_scene::Xfm solved_hand = hand.local;
      set_local_from_world(solved_hand, target_world, fore_world1);
      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
          hand.local.rot[r][c] = solved_hand.rot[r][c];
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

static void apply_pending_pose(const PendingPose& pose, milo_scene::Xfm& local,
                               bool relative = false) {
  if (pose.quat) {
    const bool quat_relative =
        relative || (relative_face_quat_enabled() &&
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
    if (quat_relative) {
      float out[3][3] = {};
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          for (int k = 0; k < 3; ++k)
            out[r][c] += local.rot[r][k] * rot[k][c];
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
      (!is_hand_bone(pose.pos->bone_name) ||
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
        relative || (relative_face_quat_enabled() &&
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
    if (quat_relative) {
      float out[3][3] = {};
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          for (int k = 0; k < 3; ++k)
            out[r][c] += local.rot[r][k] * rot[k][c];
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
      (!is_hand_bone(pose.pos->bone_name) ||
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

void apply_character_controllers(Character& character, float time_seconds,
                                 FaceFxEyeProperties* eye_props) {
  (void)time_seconds;
  if (eye_props) *eye_props = {};
  std::vector<milo_scene::Xfm> bind_bones = character.bind_bone_local;
  if (bind_bones.size() != character.bones.size()) {
    bind_bones.clear();
    bind_bones.reserve(character.bones.size());
    for (const auto& b : character.bones) bind_bones.push_back(b.local);
  }
  apply_ik_hands(character);
  apply_driven_twists(character, bind_bones);

  if (debug_face_enabled()) {
    for (const auto& m : character.meshes) {
      if (m.name != "eye-L.mesh" && m.name != "eye-R.mesh") continue;
      const auto parent_world = character.bone_world(m.parent);
      const auto eye_world = character.mesh_world(m);
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

  if (disable_lookat_enabled()) return;

  for (const auto& look : character.lookats) {
    if (look.weight <= 0.0f || look.driven.empty()) continue;
    const int driven_i = find_mesh_index(character, look.driven);
    if (driven_i < 0 || static_cast<size_t>(driven_i) >= character.meshes.size())
      continue;
    auto& eye = character.meshes[static_cast<size_t>(driven_i)];
    if (eye.parent.empty()) continue;

    const auto parent_world = character.bone_world(eye.parent);
    const auto eye_world = character.mesh_world(eye);
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
      const float dist = 100.0f;
      source_pos = vadd(vadd(target_pos, vscale(head_front, dist)),
                        vscale(head_up, -dist * 0.45f));
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
        if (v >= 0.0f) return pos > 1e-5f ? std::clamp(v / pos, 0.0f, 1.0f)
                                          : 0.0f;
        return neg < -1e-5f ? -std::clamp(v / neg, 0.0f, 1.0f) : 0.0f;
      };
      const float x = norm_axis(yaw, hz_min, hz_max);
      const float z = norm_axis(pitch, vt_min, vt_max);
      if (look.driven == "eye-L.mesh" || look.target == "eye-L.mesh") {
        eye_props->l_eye_x = x;
        eye_props->l_eye_z = z;
        eye_props->has_l_eye_x = true;
        eye_props->has_l_eye_z = true;
      } else if (look.driven == "eye-R.mesh" || look.target == "eye-R.mesh") {
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

    const auto desired_world =
        aim_preserve_xfm(eye_pos, mat_row(eye_world, 1), target_dir, eye_world);
    set_local_from_world(eye.local, desired_world, parent_world);
  }
}

void apply_clip_pose(const std::vector<ClipChannel>& channels, Character& character) {
  apply_clip_pose_weighted(channels, 1.0f, character);
}

void apply_clip_pose_sampled(const std::vector<ClipChannel>& channels,
                             float weight, Character& character,
                             bool relative) {
  std::vector<PendingPose> poses(character.bones.size());
  std::vector<PendingPose> mesh_poses(character.meshes.size());
  for (const auto& ch : channels) {
    bool matched = false;
    for (size_t i = 0; i < character.bones.size(); ++i) {
      if (!channel_matches_bone(character.bones[i].name, ch.bone_name)) continue;
      switch (ch.type) {
        case ClipChannel::kPos: poses[i].pos = &ch; break;
        case ClipChannel::kScale: poses[i].scale = &ch; break;
        case ClipChannel::kQuat: poses[i].quat = &ch; break;
        case ClipChannel::kRotX: poses[i].rotx = &ch; break;
        case ClipChannel::kRotY: poses[i].roty = &ch; break;
        case ClipChannel::kRotZ: poses[i].rotz = &ch; break;
      }
      matched = true;
      break;
    }
    if (matched) continue;
    for (size_t i = 0; i < character.meshes.size(); ++i) {
      if (!channel_matches_bone(character.meshes[i].name, ch.bone_name)) continue;
      switch (ch.type) {
        case ClipChannel::kPos: mesh_poses[i].pos = &ch; break;
        case ClipChannel::kScale: mesh_poses[i].scale = &ch; break;
        case ClipChannel::kQuat: mesh_poses[i].quat = &ch; break;
        case ClipChannel::kRotX: mesh_poses[i].rotx = &ch; break;
        case ClipChannel::kRotY: mesh_poses[i].roty = &ch; break;
        case ClipChannel::kRotZ: mesh_poses[i].rotz = &ch; break;
      }
      break;
    }
  }

  for (size_t i = 0; i < character.bones.size(); ++i) {
    apply_pending_pose_weighted(poses[i], character.bones[i].local, weight,
                                relative);
  }
  for (size_t i = 0; i < character.meshes.size(); ++i) {
    apply_pending_pose_weighted(mesh_poses[i], character.meshes[i].local, weight,
                                relative);
  }
}

void apply_clip_pose_weighted(const std::vector<ClipChannel>& channels,
                              float weight, Character& character,
                              bool relative) {
  apply_clip_pose_sampled(channels, weight, character, relative);
  std::vector<milo_scene::Xfm> bind_bones;
  bind_bones.reserve(character.bones.size());
  for (const auto& b : character.bones) bind_bones.push_back(b.local);
  apply_ik_hands(character);
  apply_driven_twists(character, bind_bones);
}

void apply_clip_frame(const CharClip& clip, int frame_idx, Character& character) {
  if (clip.frames.empty()) return;
  int fi = std::clamp(frame_idx, 0, (int)clip.frames.size() - 1);
  apply_clip_pose_sampled(clip.frames[(size_t)fi], 1.0f, character,
                          clip.relative);
}

void apply_clip_frame_weighted(const CharClip& clip, int frame_idx,
                               float weight, Character& character) {
  if (clip.frames.empty()) return;
  int fi = std::clamp(frame_idx, 0, (int)clip.frames.size() - 1);
  apply_clip_pose_sampled(clip.frames[(size_t)fi], weight, character,
                          clip.relative);
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
    switch (ch.type) {
      case ClipChannel::kPos:
        for (int i = 0; i < 3; ++i)
          ch.pos[i] = ch.pos[i] * (1.0f - t) + b->pos[i] * t;
        break;
      case ClipChannel::kScale:
        for (int i = 0; i < 3; ++i)
          ch.scale[i] = ch.scale[i] * (1.0f - t) + b->scale[i] * t;
        break;
      case ClipChannel::kQuat: {
        float dot = 0.0f;
        for (int i = 0; i < 4; ++i) dot += ch.quat[i] * b->quat[i];
        float len = 0.0f;
        for (int i = 0; i < 4; ++i) {
          const float rhs = dot < 0.0f ? -b->quat[i] : b->quat[i];
          ch.quat[i] = ch.quat[i] * (1.0f - t) + rhs * t;
          len += ch.quat[i] * ch.quat[i];
        }
        len = std::sqrt(std::max(len, 1e-8f));
        for (float& q : ch.quat) q /= len;
        break;
      }
      case ClipChannel::kRotX:
      case ClipChannel::kRotY:
      case ClipChannel::kRotZ:
        ch.angle = ch.angle * (1.0f - t) + b->angle * t;
        break;
    }
  }
  return out;
}

}  // namespace

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
      apply_clip_pose_sampled(interpolate_frame(*previous.clip, ff),
                              weight * (1.0f - current_weight), character,
                              previous.clip->relative);
    }
  }

  if (current.clip) {
    const float ff = clip_frame_float_at_time(*current.clip,
                                              current.time_seconds,
                                              current.flags);
    apply_clip_pose_sampled(interpolate_frame(*current.clip, ff),
                            weight * current_weight, character,
                            current.clip->relative);
  }
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
