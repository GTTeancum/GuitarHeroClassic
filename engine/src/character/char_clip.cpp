// engine/src/character/char_clip.cpp
//
// CharClipSamples / CharBonesSamples decoder.
//
// Decoder evidence is bounded by ihatecompvir source. rb3-latest exposes
// CharClip/CharBones/CharBonesSamples layouts and call flow, while the
// rb3-retail-old RB2 dump maps CharClipSamples runtime functions. Some exact
// sample math bodies are still absent from the checked public C++ source. Keep
// broad output writes diagnostic until the corresponding runtime path is ported
// from source or proved by direct trace.
//
// Observed format:
//  A clip contains the CharClip base (a neighbor/transition list of CLIP names)
//  followed by 1..3 CharBonesSamples "bone lists". Each bone list is:
//      uint32 bone_count
//      bone_count x { length-prefixed name (ends .pos/.scale/.quat/.rotx/.roty/.rotz),
//                     float32 weight }     (weight present for gRev>10)
//      uint32 cum_counts[10]   cumulative bone count per category (0..9)
//      uint32 compression      source CharBones::CompressionType:
//                              0 none, 1 rotations, 2 vectors,
//                              3 quats, 4 all
//      uint32 numSamples       number of frames
//  Then, AFTER every bone-list header, the sample data blocks follow in list
//  order (two-pass: all defs, then all data). Each list's block is:
//      numSamples x frame, where each frame is (bones grouped BY CATEGORY):
//         vectors (.pos + .scale):  float32x3 (12B) or int16x3 (6B)
//         quats   (.quat):          float32x4 (16B), int16x4 (8B), or
//                                   source ByteQuat (4B)
//         angles  (.rotx/.roty/.rotz): float32 (4B) or int16 (2B)
//
// Source-backed bone classification: .pos=0 .scale=1 .quat=2
// .rotx=3 .roty=4 .rotz=5, matching ihatecompvir CharBones::Type.

#include "character/char_clip.h"

#include "ark_v3.h"
#include "milo.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ghogx::character {

// Strip ".pos"/".quat"/etc to bone base name. Defined below; forward-declared
// so the anonymous-namespace parser can use it.
std::string strip_suffix(const std::string& channel);

int source_char_bones_type_of(const std::string& channel) {
  const size_t dot = channel.find('.');
  if (dot == std::string::npos || dot + 1 >= channel.size()) {
    return kSourceCharBonesTypeEnd;
  }
  switch (channel[dot + 1]) {
    case 'p':
      return kSourceCharBonesTypePos;
    case 's':
      return kSourceCharBonesTypeScale;
    case 'q':
      return kSourceCharBonesTypeQuat;
    case 'r':
      if (dot + 4 < channel.size()) {
        const char axis = channel[dot + 4];
        if (axis >= 'x' && axis <= 'z') {
          return kSourceCharBonesTypeRotX + (axis - 'x');
        }
      }
      break;
    default:
      break;
  }
  return kSourceCharBonesTypeEnd;
}

const char* source_char_bones_suffix_of(int type) {
  static const char* suffixes[kSourceCharBonesTypeEnd] = {
      "pos", "scale", "quat", "rotx", "roty", "rotz"};
  if (type < 0 || type >= kSourceCharBonesTypeEnd) return "";
  return suffixes[type];
}

std::string source_char_bones_channel_name(const std::string& name, int type) {
  const char* suffix = source_char_bones_suffix_of(type);
  if (!suffix[0]) return name;
  std::string out = name;
  size_t dot = out.find('.');
  if (dot == std::string::npos) {
    out.push_back('.');
    dot = out.size() - 1;
  }
  out.resize(dot + 1);
  out += suffix;
  return out;
}

size_t source_char_bones_type_size(int type, int compression) {
  if (type < 0 || type >= kSourceCharBonesTypeEnd) return 0u;
  if (type < kSourceCharBonesTypeQuat) {
    return compression < 2 ? 12u : 6u;
  }
  if (type != kSourceCharBonesTypeQuat) {
    return compression == 0 ? 4u : 2u;
  }
  if (compression > 2) return 4u;
  if (compression == 0) return 16u;
  return 8u;
}

SourceCharBonesLayout source_char_bones_recompute_layout(
    const std::array<int, kSourceCharBonesTypeEnd + 1>& counts,
    int compression) {
  SourceCharBonesLayout layout;
  layout.counts = counts;
  layout.offsets[0] = 0;
  for (int type = 0; type < kSourceCharBonesTypeEnd; ++type) {
    const int diff = counts[type + 1] - counts[type];
    const int size = static_cast<int>(
        source_char_bones_type_size(type, compression));
    layout.offsets[type + 1] = layout.offsets[type] + diff * size;
  }
  layout.total_size = (layout.offsets[kSourceCharBonesTypeEnd] + 0xF) &
                      ~0xF;
  return layout;
}

SourceCharBonesCompressionUpdate source_char_bones_set_compression(
    int current_compression,
    const SourceCharBonesLayout& current_layout,
    int requested_compression) {
  SourceCharBonesCompressionUpdate update;
  update.compression = current_compression;
  update.layout = current_layout;
  if (requested_compression != current_compression) {
    update.compression = requested_compression;
    update.layout = source_char_bones_recompute_layout(current_layout.counts,
                                                       requested_compression);
    update.changed = true;
  }
  return update;
}

SourceCharBonesState source_char_bones_empty_state() {
  return SourceCharBonesState{};
}

void source_char_bones_clear(SourceCharBonesState& state) {
  state.bones.clear();
  state.layout = SourceCharBonesLayout{};
  state.compression = 0;
}

void source_char_bones_set_weights(std::vector<SourceCharBonesBone>& bones,
                                   float weight) {
  for (SourceCharBonesBone& bone : bones) {
    bone.weight = weight;
  }
}

void source_char_bones_set_weights(SourceCharBonesState& state, float weight) {
  source_char_bones_set_weights(state.bones, weight);
}

void source_char_bones_list_bones(const SourceCharBonesState& state,
                                  std::vector<SourceCharBonesBone>& bones) {
  for (const SourceCharBonesBone& bone : state.bones) {
    bones.push_back(bone);
  }
}

std::optional<size_t> source_char_bone_find_weight_index(
    const CharClip::OutputBone& bone, int context_mask) {
  for (size_t i = 0; i < bone.weights.size(); ++i) {
    if ((bone.weights[i].context & context_mask) != 0) return i;
  }
  return std::nullopt;
}

float source_char_bone_get_weight(const CharClip::OutputBone& bone,
                                  int context_mask) {
  const std::optional<size_t> index =
      source_char_bone_find_weight_index(bone, context_mask);
  if (index) return bone.weights[*index].weight;
  return 1.0f;
}

void source_char_bone_clear_context(CharClip::OutputBone& bone,
                                    int context_mask) {
  const int mask = ~context_mask;
  bone.position_context &= mask;
  bone.scale_context &= mask;
  bone.rotation_context &= mask;
}

void source_char_bone_stuff_bones(const CharClip::OutputBone& bone,
                                  int context_mask,
                                  std::vector<SourceCharBonesBone>& bones) {
  if ((bone.position_context & context_mask) != 0) {
    bones.push_back({source_char_bones_channel_name(
                         bone.name, kSourceCharBonesTypePos),
                     source_char_bone_get_weight(bone, context_mask)});
  }
  if ((bone.scale_context & context_mask) != 0) {
    bones.push_back({source_char_bones_channel_name(
                         bone.name, kSourceCharBonesTypeScale),
                     source_char_bone_get_weight(bone, context_mask)});
  }
  if (bone.rotation_type != kSourceCharBonesTypeEnd &&
      (bone.rotation_context & context_mask) != 0) {
    bones.push_back({source_char_bones_channel_name(bone.name,
                                                   bone.rotation_type),
                     source_char_bone_get_weight(bone, context_mask)});
  }
}

void source_char_bone_dir_list_bones(
    const std::vector<CharClip::OutputBone>& output_bones,
    int move_context,
    int context_mask,
    bool include_delta_facing,
    std::vector<SourceCharBonesBone>& bones) {
  if ((move_context & context_mask) != 0) {
    bones.push_back({"bone_facing.pos", 1.0f});
    bones.push_back({"bone_facing.rotz", 1.0f});
    if (include_delta_facing) {
      bones.push_back({"bone_facing_delta.pos", 1.0f});
      bones.push_back({"bone_facing_delta.rotz", 1.0f});
    }
  }
  for (const CharClip::OutputBone& output_bone : output_bones) {
    source_char_bone_stuff_bones(output_bone, context_mask, bones);
  }
}

SourceCharBonesSamplesState source_char_bones_samples_empty_state() {
  return SourceCharBonesSamplesState{};
}

int source_char_bones_samples_allocate_size(
    const SourceCharBonesSamplesState& samples) {
  return samples.bones.layout.total_size * samples.num_samples;
}

bool source_char_bones_samples_set_preview(
    SourceCharBonesSamplesState& samples, int requested_sample) {
  if (samples.num_samples <= 0) return false;
  const int last = samples.num_samples - 1;
  const int clamped = std::max(0, std::min(last, requested_sample));
  samples.preview_sample = clamped;
  samples.start_offset = samples.bones.layout.total_size * clamped;
  return true;
}

std::vector<SourceCharBonesSampleStep> source_char_bones_samples_split_steps(
    const SourceCharBonesSamplesState& samples,
    int sample,
    float weight,
    float frac) {
  std::vector<SourceCharBonesSampleStep> steps;
  steps.push_back(
      {samples.bones.layout.total_size * sample, (1.0f - frac) * weight});
  if (frac > 0.0f) {
    steps.push_back(
        {samples.bones.layout.total_size * (sample + 1), frac * weight});
  }
  return steps;
}

bool source_char_bones_samples_load_version_known(int version) {
  return version > 12 && version <= 16;
}

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
  const bool suffix = ends(".pos") || ends(".scale") || ends(".quat") ||
                      ends(".rotx") || ends(".roty") || ends(".rotz");
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

enum SourceCharBonesCompression {
  kSourceCompressNone = 0,
  kSourceCompressRots = 1,
  kSourceCompressVects = 2,
  kSourceCompressQuats = 3,
  kSourceCompressAll = 4,
};

bool is_valid_category_name(const std::string& name) {
  int c = source_char_bones_type_of(name);
  return c >= 0 && c < kSourceCharBonesTypeEnd;
}

float env_float_or(const char* name, float fallback) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  if (_dupenv_s(&value, &len, name) != 0 || !value || !value[0]) {
    std::free(value);
    return fallback;
  }
  char* end = nullptr;
  const float parsed = std::strtof(value, &end);
  std::free(value);
#else
  const char* value = std::getenv(name);
  if (!value || !value[0]) return fallback;
  char* end = nullptr;
  const float parsed = std::strtof(value, &end);
#endif
  return end && end != value && std::isfinite(parsed) ? parsed : fallback;
}

bool source_char_bones_compression_known(int compression) {
  return compression >= kSourceCompressNone &&
         compression <= kSourceCompressAll;
}

const char* source_char_bones_compression_name(int compression) {
  switch (compression) {
    case kSourceCompressNone: return "kCompressNone";
    case kSourceCompressRots: return "kCompressRots";
    case kSourceCompressVects: return "kCompressVects";
    case kSourceCompressQuats: return "kCompressQuats";
    case kSourceCompressAll: return "kCompressAll";
    default: return "unknown";
  }
}

bool uses_source_byte_quat(const BoneList& list) {
  if (list.compression < kSourceCompressQuats) return false;
  return std::find(list.cats.begin(), list.cats.end(), 2) != list.cats.end();
}

bool debug_clip_parse_enabled() {
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
  if (!source_char_bones_compression_known(out.compression)) return false;
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
    out.cats.push_back(source_char_bones_type_of(name));
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
  if (!source_char_bones_compression_known(out.compression)) return false;
  if (out.num_samples < 0 || out.num_samples > 100000) return false;

  // Category breakdown from cumulative counts.
  auto cat_n = [&](int cat) -> int {
    uint32_t lo = out.cum[cat];
    uint32_t hi = (cat + 1 < 10) ? out.cum[cat + 1] : count;
    return (int)(hi - lo);
  };
  out.n_vec   = cat_n(0) + cat_n(1);                       // pos + scale
  out.n_quat  = cat_n(2);                                  // quat
  out.n_angle = cat_n(3) + cat_n(4) + cat_n(5);            // rot*
  out.frame_bytes = 0;
  for (int cat : out.cats) {
    out.frame_bytes += source_char_bones_type_size(cat, out.compression);
  }

  // Source TypeSize proves the 4-byte ByteQuat row for kCompressQuats and
  // kCompressAll, but the checked source snapshot does not expose the exact
  // ByteQuat-to-Quat conversion body. Refuse those lists rather than silently
  // reading four bytes as a ShortQuat.
  if (uses_source_byte_quat(out)) return false;

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
  ch.type = cat == kSourceCharBonesTypeRotX ? ClipChannel::kRotX
          : cat == kSourceCharBonesTypeRotY ? ClipChannel::kRotY
                                            : ClipChannel::kRotZ;
  // Community tooling decodes compressed single-axis rotations as signed
  // normalized values, then applies them as a pi-scaled axis rotation.
  ch.angle = (comp ? read_snorm16(c) : c.f32()) *
             3.14159265358979323846f;
}

// Parse the whole clip entry into frames.
std::vector<std::vector<ClipChannel>> parse_all(const uint8_t* d, size_t n,
                                                int& num_samples_out) {
  num_samples_out = 0;
  if (n < 4) return {};
  uint32_t samples_version = 0;
  std::memcpy(&samples_version, d, 4);
  if (!source_char_bones_samples_load_version_known(
          static_cast<int>(samples_version))) {
    return {};
  }

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

  if (debug_clip_parse_enabled()) {
    for (size_t i = 0; i < lists.size(); ++i) {
      const BoneList& bl = lists[i];
      std::fprintf(stderr,
                   "[clip-source-bones] list=%zu comp=%d(%s) samples=%d "
                   "channels=%zu bytes=%zu byteQuat=%d\n",
                   i, bl.compression,
                   source_char_bones_compression_name(bl.compression),
                   bl.num_samples, bl.names.size(), bl.frame_bytes,
                   uses_source_byte_quat(bl) ? 1 : 0);
      std::fprintf(stderr,
                   "[clip-source-bones-counts] list=%zu vec=%d quat=%d "
                   "angle=%d\n",
                   i, bl.n_vec, bl.n_quat, bl.n_angle);
    }
  }

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
        if (bl.cats[bi] >= 3 && bl.cats[bi] <= 5) {
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

// Strip ".pos"/".quat"/etc. Source CharBones::ChannelName uses the first dot
// as the suffix marker.
std::string strip_suffix(const std::string& channel) {
  auto dot = channel.find('.');
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

bool debug_clip_hair_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_CLIP_HAIR") == 0 && value &&
      value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_CLIP_HAIR");
  return value && value[0];
#endif
}

bool clip_hair_debug_name(const std::string& name) {
  std::string lower = name;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return lower.find("hair") != std::string::npos ||
         lower.find("bang") != std::string::npos ||
         lower.find("pony") != std::string::npos ||
         lower.find("coat") != std::string::npos ||
         lower.find("chain") != std::string::npos ||
         lower.find("wing") != std::string::npos ||
         lower.find("lantern") != std::string::npos;
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

void log_char_hair_source_once(const Character& character,
                               const CharHair& hair) {
  if (!debug_char_hair_enabled()) return;
  static std::unordered_set<std::string> logged;
  const std::string key = character.dir_name + "|" + hair.name;
  if (!logged.insert(key).second) return;

  size_t point_count = 0;
  for (const auto& strand : hair.strands) {
    point_count += strand.points.size();
  }

  std::fprintf(stderr,
               "[charhair-source] character=%s hair=%s "
               "source=ihatecompvir-CharHair version=%d simulate=%d "
               "strands=%zu points=%zu "
               "globals=(stiff=%.4f torsion=%.4f inertia=%.4f "
               "gravity=%.4f weight=%.4f friction=%.4f "
               "minSlack=%.4f maxSlack=%.4f)\n",
               character.dir_name.c_str(), hair.name.c_str(), hair.version,
               hair.simulate ? 1 : 0, hair.strands.size(), point_count,
               hair.stiffness, hair.torsion, hair.inertia, hair.gravity,
               hair.weight, hair.friction, hair.min_slack, hair.max_slack);

  for (size_t strand_i = 0; strand_i < hair.strands.size(); ++strand_i) {
    const auto& strand = hair.strands[strand_i];
    std::fprintf(stderr,
                 "[charhair-source-strand] hair=%s strand=%zu root=%s "
                 "points=%zu angle=%.4f hookup=0x%08x "
                 "baseRow0=(%.4f %.4f %.4f) rootRow0=(%.4f %.4f %.4f)\n",
                 hair.name.c_str(), strand_i, strand.root.c_str(),
                 strand.points.size(), strand.angle,
                 static_cast<unsigned>(strand.hookup_flags),
                 strand.base_mat[0], strand.base_mat[1], strand.base_mat[2],
                 strand.root_mat[0], strand.root_mat[1], strand.root_mat[2]);
    for (size_t point_i = 0; point_i < strand.points.size(); ++point_i) {
      const auto& point = strand.points[point_i];
      std::fprintf(stderr,
                   "[charhair-source-point] hair=%s strand=%zu point=%zu "
                   "bone=%s collision=%s collideType=%u legacyInline=loggedOnly "
                   "pos=(%.4f %.4f %.4f) len=%.4f "
                   "radius=%.4f outer=%.4f side=%.4f "
                   "unk5c=(%.4f %.4f %.4f)\n",
                   hair.name.c_str(), strand_i, point_i, point.bone.c_str(),
                   point.collision.c_str(),
                   static_cast<unsigned>(point.collide_type), point.pos[0],
                   point.pos[1], point.pos[2], point.length, point.radius,
                   point.outer_radius, point.side_length, point.unk5c[0],
                   point.unk5c[1], point.unk5c[2]);
    }
  }
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
               "hair=%zu collide=%zu posConstraint=%zu animFilter=%zu "
               "lookAt=%zu eyes=%zu sourceIK=%s hairPoll=%s "
               "lookAt=%s\n",
               character.dir_name.c_str(), character.bones.size(),
               character.meshes.size(), character.drivers.size(),
               character.weight_setters.size(), character.ik_hands.size(),
               character.ik_midis.size(), character.fore_twists.size(),
               character.upper_twists.size(), character.hairs.size(),
               character.collides.size(), character.pos_constraints.size(),
               character.anim_filters.size(), character.lookats.size(),
               character.eyes.size(),
               "CharIKHand", "CharHair::Poll-missingHookup", "decode-only");
  for (const auto& ik : character.ik_hands) {
    std::fprintf(stderr,
                 "[chargraph]   ik %s version=%d hand=%s finger=%s "
                 "target=%s targets=%zu weight=%.3f weightProp=%s "
                 "orientation=%d stretch=%d scalable=%d moveElbow=%d "
                 "elbowSwing=%.3f alwaysElbow=%d constrainWrist=%d "
                 "wristRadians=%.3f elbowCollide=%s clockwise=%d "
                 "unreadBytes=%zu\n",
                 ik.name.c_str(), ik.version, ik.hand.c_str(),
                 ik.finger.empty() ? "<none>" : ik.finger.c_str(),
                 ik.target.c_str(), ik.targets.size(), ik.weight,
                 ik.weight_prop.c_str(),
                 ik.orientation ? 1 : 0, ik.stretch ? 1 : 0,
                 ik.scalable ? 1 : 0, ik.move_elbow ? 1 : 0,
                 ik.elbow_swing, ik.always_ik_elbow ? 1 : 0,
                 ik.constrain_wrist ? 1 : 0, ik.wrist_radians,
                 ik.elbow_collide.empty() ? "<none>"
                                           : ik.elbow_collide.c_str(),
                 ik.clockwise ? 1 : 0, ik.unread_bytes);
  }
  for (const auto& driver : character.drivers) {
    std::fprintf(stderr,
                 "[chargraph]   driver %s version=%d "
                 "weightableVersion=%d target=%s clipMilo=%s "
                 "weight=%.3f weightOwner=%s weightProp=%s enabled=%d "
                 "midi=%d midiVersion=%d midiUnreadBytes=%zu "
                 "midiParser=%s midiFlagParser=%s "
                 "midiBlendOverridePct=%.3f\n",
                 driver.name.c_str(), driver.version,
                 driver.weightable_version, driver.target.c_str(),
                 driver.clip_milo.c_str(), driver.weight,
                 driver.weight_owner.c_str(), driver.weight_prop.c_str(),
                 driver.enabled ? 1 : 0, driver.midi ? 1 : 0,
                 driver.midi_version, driver.midi_unread_bytes,
                 driver.midi_parser.empty() ? "<none>"
                                            : driver.midi_parser.c_str(),
                 driver.midi_flag_parser.empty()
                     ? "<none>"
                     : driver.midi_flag_parser.c_str(),
                 driver.midi_blend_override_pct);
  }
  for (const auto& ik : character.ik_midis) {
    std::fprintf(stderr,
                 "[chargraph]   ikMidi %s version=%d bone=%s "
                 "legacySpots=%zu legacyString=%s animBlender=%s "
                 "maxAnimBlend=%.3f unreadBytes=%zu\n",
                 ik.name.c_str(), ik.version,
                 ik.bone.empty() ? "<none>" : ik.bone.c_str(),
                 ik.legacy_spots.size(),
                 ik.legacy_string.empty() ? "<none>"
                                          : ik.legacy_string.c_str(),
                 ik.anim_blender.empty() ? "<none>"
                                         : ik.anim_blender.c_str(),
                 ik.max_anim_blend, ik.unread_bytes);
  }
  for (const auto& setter : character.weight_setters) {
    std::fprintf(stderr,
                 "[chargraph]   weightSetter %s version=%d "
                 "weightableVersion=%d weight=%.3f weightOwner=%s "
                 "driver=%s flags=0x%08x offset=%.3f scale=%.3f "
                 "baseWeight=%.3f beatsPerWeight=%.3f unreadBytes=%zu\n",
                 setter.name.c_str(), setter.version,
                 setter.weightable_version, setter.weight,
                 setter.weight_owner.c_str(), setter.driver.c_str(),
                 setter.flags, setter.offset, setter.scale,
                 setter.base_weight, setter.beats_per_weight,
                 setter.unread_bytes);
  }
  for (const auto& filter : character.anim_filters) {
    std::fprintf(stderr,
                 "[chargraph]   animFilter %s version=%d "
                 "animatableVersion=%d anim=%s frame=%.3f rate=%d "
                 "scale=%.3f offset=%.3f start=%.3f end=%.3f "
                 "type=%d period=%.3f snap=%.3f jitter=%.3f "
                 "unreadBytes=%zu\n",
                 filter.name.c_str(), filter.version,
                 filter.animatable_version,
                 filter.anim.empty() ? "<none>" : filter.anim.c_str(),
                 filter.frame, filter.rate, filter.scale, filter.offset,
                 filter.start, filter.end, filter.type, filter.period,
                 filter.snap, filter.jitter, filter.unread_bytes);
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
  for (const auto& ut : character.upper_twists) {
    std::fprintf(stderr,
                 "[chargraph]   upperTwist %s upper=%s twist1=%s twist2=%s\n",
                 ut.name.c_str(), ut.upper_arm.c_str(), ut.twist1.c_str(),
                 ut.twist2.c_str());
  }
  for (const auto& hair : character.hairs) {
    size_t point_count = 0;
    for (const auto& strand : hair.strands) point_count += strand.points.size();
    std::fprintf(stderr,
                 "[chargraph]   hair %s strands=%zu points=%zu simulate=%d "
                 "globals=[%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f]\n",
                 hair.name.c_str(), hair.strands.size(), point_count,
                 hair.simulate ? 1 : 0, hair.stiffness, hair.torsion,
                 hair.inertia, hair.gravity, hair.weight, hair.friction,
                 hair.min_slack, hair.max_slack);
    for (const auto& strand : hair.strands) {
      std::fprintf(stderr,
                   "[chargraph]     hairStrand root=%s angle=%.3f "
                   "points=%zu rows=[%.3f %.3f %.3f | %.3f %.3f %.3f] "
                   "all=[%.3f %.3f %.3f %.3f %.3f %.3f "
                   "%.3f %.3f %.3f %.3f %.3f %.3f "
                   "%.3f %.3f %.3f %.3f %.3f %.3f]\n",
                   strand.root.c_str(), strand.angle,
                   strand.points.size(), strand.base_mat[0],
                   strand.base_mat[1], strand.base_mat[2],
                   strand.root_mat[0], strand.root_mat[1],
                   strand.root_mat[2], strand.base_mat[0],
                   strand.base_mat[1], strand.base_mat[2],
                   strand.base_mat[3], strand.base_mat[4],
                   strand.base_mat[5], strand.base_mat[6],
                   strand.base_mat[7], strand.base_mat[8],
                   strand.root_mat[0], strand.root_mat[1],
                   strand.root_mat[2], strand.root_mat[3],
                   strand.root_mat[4], strand.root_mat[5],
                   strand.root_mat[6], strand.root_mat[7],
                   strand.root_mat[8]);
      for (const auto& point : strand.points) {
        std::fprintf(stderr,
                     "[chargraph]       hairPoint bone=%s "
                     "collision=%s collideType=%u "
                     "pos=[%.3f %.3f %.3f] length=%.3f radius=%.3f "
                     "outer=%.3f side=%.3f unk5c=[%.3f %.3f %.3f]\n",
                     point.bone.c_str(), point.collision.c_str(),
                     static_cast<unsigned>(point.collide_type), point.pos[0],
                     point.pos[1], point.pos[2], point.length, point.radius,
                     point.outer_radius, point.side_length, point.unk5c[0],
                     point.unk5c[1], point.unk5c[2]);
      }
    }
  }
  for (const auto& collide : character.collides) {
    std::fprintf(stderr,
                 "[chargraph]   collide %s version=%d shape=%d flags=0x%08x "
                 "mesh=%s parent=%s radius=(%.3f %.3f) "
                 "length=(%.3f %.3f) curRadius=(%.3f %.3f) "
                 "curLength=(%.3f %.3f) meshYBias=%d\n",
                 collide.name.c_str(), collide.version, collide.shape,
                 static_cast<unsigned>(collide.flags), collide.mesh.c_str(),
                 collide.parent.c_str(), collide.orig_radius[0],
                 collide.orig_radius[1], collide.orig_length[0],
                 collide.orig_length[1], collide.cur_radius[0],
                 collide.cur_radius[1], collide.cur_length[0],
                 collide.cur_length[1], collide.mesh_y_bias ? 1 : 0);
  }
  for (const auto& constraint : character.pos_constraints) {
    std::fprintf(stderr,
                 "[chargraph]   posConstraint %s version=%d source=%s "
                 "targets=%zu boxMin=[%.3f %.3f %.3f] "
                 "boxMax=[%.3f %.3f %.3f]\n",
                 constraint.name.c_str(), constraint.version,
                 constraint.source.empty() ? "<none>"
                                           : constraint.source.c_str(),
                 constraint.targets.size(), constraint.box_min[0],
                 constraint.box_min[1], constraint.box_min[2],
                 constraint.box_max[0], constraint.box_max[1],
                 constraint.box_max[2]);
    for (const auto& target : constraint.targets) {
      std::fprintf(stderr, "[chargraph]     posTarget %s\n",
                   target.empty() ? "<none>" : target.c_str());
    }
  }
  for (const auto& look : character.lookats) {
    std::fprintf(stderr,
                 "[chargraph]   lookAt %s version=%d "
                 "weightableVersion=%d weight=%.3f weightOwner=%s "
                 "source=%s pivot=%s dest=%s halfTime=%.3f "
                 "yaw=(%.3f %.3f) pitch=(%.3f %.3f) "
                 "weightYaw=(%.3f %.3f speed=%.3f) allowRoll=%d "
                 "jitter=%d sourceRadius=%.3f unreadBytes=%zu\n",
                 look.name.c_str(), look.version, look.weightable_version,
                 look.weight,
                 look.weight_owner.empty() ? "<none>" : look.weight_owner.c_str(),
                 look.source.empty() ? "<none>" : look.source.c_str(),
                 look.pivot.empty() ? "<none>" : look.pivot.c_str(),
                 look.dest.empty() ? "<none>" : look.dest.c_str(),
                 look.half_time, look.min_yaw, look.max_yaw,
                 look.min_pitch, look.max_pitch, look.min_weight_yaw,
                 look.max_weight_yaw, look.weight_yaw_speed,
                 look.allow_roll ? 1 : 0, look.enable_jitter ? 1 : 0,
                 look.source_radius, look.unread_bytes);
  }
  for (const auto& eyes : character.eyes) {
    std::fprintf(stderr,
                 "[chargraph]   eyes %s version=%d lookats=%zu "
                 "legacyTransform=%s unreadBytes=%zu\n",
                 eyes.name.c_str(), eyes.version, eyes.lookats.size(),
                 eyes.legacy_transform.empty() ? "<none>"
                                               : eyes.legacy_transform.c_str(),
                 eyes.unread_bytes);
    for (const auto& lookat : eyes.lookats) {
      std::fprintf(stderr, "[chargraph]     eyesLookAt %s\n",
                   lookat.c_str());
    }
  }
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

static uint8_t read_u8_at(const uint8_t* data, size_t size, size_t& pos,
                          const char* label) {
  if (pos + 1 > size) {
    throw std::runtime_error(std::string("short ") + label);
  }
  return data[pos++];
}

static void skip_bytes_at(const uint8_t* data, size_t size, size_t& pos,
                          size_t count, const char* label) {
  (void)data;
  if (pos + count > size) {
    throw std::runtime_error(std::string("short ") + label);
  }
  pos += count;
}

static uint32_t read_u32_at(const uint8_t* data, size_t size, size_t& pos,
                            const char* label) {
  if (pos + 4 > size) {
    throw std::runtime_error(std::string("short ") + label);
  }
  uint32_t value = 0;
  std::memcpy(&value, data + pos, 4);
  pos += 4;
  return value;
}

static int32_t read_i32_at(const uint8_t* data, size_t size, size_t& pos,
                           const char* label) {
  return static_cast<int32_t>(read_u32_at(data, size, pos, label));
}

static float read_f32_at(const uint8_t* data, size_t size, size_t& pos,
                         const char* label) {
  const uint32_t raw = read_u32_at(data, size, pos, label);
  float value = 0.0f;
  std::memcpy(&value, &raw, 4);
  return value;
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
  size_t pos = 0;
  CharClip::OutputBone out;
  out.name = entry_name;
  out.char_bone_version = read_u32_at(body, size, pos, "CharBone version");
  skip_bytes_at(body, size, pos, 9, "CharBone object fields");
  if (out.char_bone_version < 9) {
    out.trans_version = read_u32_at(body, size, pos, "RndTransformable version");
    out.local = read_xfm_at(body, size, pos);
    out.world_stored = read_xfm_at(body, size, pos);
    if (out.trans_version < 9) {
      const uint32_t child_count =
          read_u32_at(body, size, pos, "legacy trans child count");
      for (uint32_t i = 0; i < child_count; ++i) {
        (void)read_len_string(body, size, pos);
      }
    }
    if (out.trans_version > 6) {
      out.trans_constraint =
          read_u32_at(body, size, pos, "RndTransformable constraint");
    }
    if (out.trans_version > 5) {
      out.trans_target = read_len_string(body, size, pos);
    }
    if (out.trans_version > 6) {
      out.preserve_scale =
          read_u8_at(body, size, pos, "RndTransformable preserve scale") != 0;
    }
    out.parent = read_len_string(body, size, pos);
  }

  if (out.char_bone_version > 6) {
    out.position_context =
        read_i32_at(body, size, pos, "CharBone position context");
  } else {
    out.position_context =
        read_u8_at(body, size, pos, "CharBone legacy position context") ? 1 : 0;
  }
  if (out.char_bone_version > 6) {
    out.scale_context =
        read_i32_at(body, size, pos, "CharBone scale context");
  } else if (out.char_bone_version > 1) {
    out.scale_context =
        read_u8_at(body, size, pos, "CharBone legacy scale context") ? 1 : 0;
  }
  out.rotation_type = read_i32_at(body, size, pos, "CharBone rotation type");
  if (out.char_bone_version < 5) {
    out.legacy_pre_rev5_int =
        read_i32_at(body, size, pos, "CharBone pre-rev5 legacy int");
    out.has_legacy_pre_rev5_int = true;
  }
  if (out.char_bone_version < 2) {
    out.scale_context = 0;
    ++out.rotation_type;
  }
  constexpr int32_t kSourceTypeEnd = 6;
  if (out.char_bone_version < 5 && out.rotation_type > kSourceTypeEnd) {
    out.rotation_type = kSourceTypeEnd;
  }
  if (out.char_bone_version > 6) {
    out.rotation_context =
        read_i32_at(body, size, pos, "CharBone rotation context");
  } else {
    out.rotation_context = out.rotation_type != kSourceTypeEnd ? 1 : 0;
  }
  if (out.char_bone_version == 3 || out.char_bone_version == 4 ||
      out.char_bone_version == 5 || out.char_bone_version == 6 ||
      out.char_bone_version == 7) {
    out.legacy_rev3_to_7_int =
        read_i32_at(body, size, pos, "CharBone rev3-7 legacy int");
    out.has_legacy_rev3_to_7_int = true;
  }
  if (out.char_bone_version > 3) {
    out.target = read_len_string(body, size, pos);
  }
  if (out.char_bone_version == 6) {
    const int32_t ctx =
        read_i32_at(body, size, pos, "CharBone rev6 shared context");
    if (out.position_context != 0) out.position_context = ctx;
    if (out.scale_context != 0) out.scale_context = ctx;
    if (out.rotation_context != 0) out.rotation_context = ctx;
  }
  if (out.char_bone_version > 7) {
    const uint32_t count =
        read_u32_at(body, size, pos, "CharBone weight count");
    out.weights.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
      CharClip::OutputBone::WeightContext weight;
      weight.context = read_i32_at(body, size, pos, "CharBone weight context");
      weight.weight = read_f32_at(body, size, pos, "CharBone weight value");
      out.weights.push_back(weight);
    }
  }
  if (out.char_bone_version > 8) {
    out.trans = read_len_string(body, size, pos);
  }
  if (out.char_bone_version > 9) {
    out.bake_out_as_top_level =
        read_u8_at(body, size, pos, "CharBone bake out flag") != 0;
  }
  out.unread_bytes = size - pos;
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
                       "[clip-output] %-28s sourceCharBone version=%u "
                       "transVersion=%u constraint=%u target=%s preserve=%d "
                       "parent=%-28s posCtx=%d scaleCtx=%d rotType=%d "
                       "rotCtx=%d charTarget=%s weights=%zu trans=%s "
                       "bakeOut=%d unreadBytes=%zu "
                       "localPos=(%.3f %.3f %.3f)\n",
                       out.name.c_str(), out.char_bone_version,
                       out.trans_version, out.trans_constraint,
                       out.trans_target.empty() ? "<none>"
                                                : out.trans_target.c_str(),
                       out.preserve_scale ? 1 : 0, out.parent.c_str(),
                       out.position_context, out.scale_context,
                       out.rotation_type, out.rotation_context,
                       out.target.empty() ? "<none>" : out.target.c_str(),
                       out.weights.size(),
                       out.trans.empty() ? "<none>" : out.trans.c_str(),
                       out.bake_out_as_top_level ? 1 : 0, out.unread_bytes,
                       out.local.pos[0], out.local.pos[1], out.local.pos[2]);
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
        if (debug_clip_hair_enabled()) {
          for (size_t i = 0; i < result.output_bones.size(); ++i) {
            const auto& out = result.output_bones[i];
            if (!clip_hair_debug_name(out.name) &&
                !clip_hair_debug_name(out.parent)) {
              continue;
            }
            std::fprintf(stderr,
                         "[clip-hair-output] clip=%s index=%zu name=%s parent=%s "
                         "local=(%.4f %.4f %.4f)\n",
                         clip_name.c_str(), i, out.name.c_str(),
                         out.parent.c_str(), out.local.pos[0],
                         out.local.pos[1], out.local.pos[2]);
          }
          if (!result.frames.empty()) {
            const auto& frame0 = result.frames[0];
            for (size_t i = 0; i < frame0.size(); ++i) {
              const auto& ch = frame0[i];
              if (!clip_hair_debug_name(ch.bone_name)) continue;
              const char* type = ch.type == ClipChannel::kPos ? "pos" :
                                 ch.type == ClipChannel::kScale ? "scale" :
                                 ch.type == ClipChannel::kQuat ? "quat" :
                                 ch.type == ClipChannel::kRotX ? "rotx" :
                                 ch.type == ClipChannel::kRotY ? "roty" : "rotz";
              if (ch.type == ClipChannel::kQuat) {
                std::fprintf(stderr,
                             "[clip-hair-channel] clip=%s index=%zu type=%s "
                             "name=%s value=(%.5f %.5f %.5f %.5f)\n",
                             clip_name.c_str(), i, type, ch.bone_name.c_str(),
                             ch.quat[0], ch.quat[1], ch.quat[2], ch.quat[3]);
              } else if (ch.type == ClipChannel::kPos) {
                std::fprintf(stderr,
                             "[clip-hair-channel] clip=%s index=%zu type=%s "
                             "name=%s value=(%.5f %.5f %.5f)\n",
                             clip_name.c_str(), i, type, ch.bone_name.c_str(),
                             ch.pos[0], ch.pos[1], ch.pos[2]);
              } else {
                std::fprintf(stderr,
                             "[clip-hair-channel] clip=%s index=%zu type=%s "
                             "name=%s value=%.5f\n",
                             clip_name.c_str(), i, type, ch.bone_name.c_str(),
                             ch.angle);
              }
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

CharClipGroup load_clip_group(
    const std::string& hdr_path, const std::string& ark_path,
    const std::vector<std::string>& milo_paths,
    const std::string& group_name) {
  CharClipGroup group;
  group.name = group_name;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    for (const auto& milo_path : milo_paths) {
      auto entry = ark.find(milo_path);
      if (!entry) entry = ark.find("../../system/run/" + milo_path);
      if (!entry) continue;
      auto bytes = ark.read_entry(*entry, {ark_path});
      auto hdr = gh::milo::parse_header(bytes);
      auto payload = gh::milo::inflate_payload(bytes, hdr);
      auto dir = gh::milo::parse_directory(payload);
      for (const auto& de : dir.entries) {
        if (de.type != "CharClipGroup" || de.name != group_name ||
            de.offset + de.size > payload.size()) {
          continue;
        }

        const uint8_t* body = payload.data() + de.offset;
        const size_t size = static_cast<size_t>(de.size);
        size_t pos = 0;
        if (size < 4) throw std::runtime_error("short CharClipGroup");
        uint32_t version = 0;
        std::memcpy(&version, body + pos, 4);
        pos += 4;
        if (version > 2) {
          throw std::runtime_error("unexpected CharClipGroup version");
        }

        // Source CharClipGroup::Load calls Hmx::Object::Load before mClips.
        if (pos + 4 > size) throw std::runtime_error("short object fields");
        pos += 4;  // Hmx::Object revision.
        (void)read_len_string(body, size, pos);  // Hmx::Object subtype symbol.
        if (pos >= size) throw std::runtime_error("short object tree terminator");
        ++pos;  // Hmx::Object empty property-tree terminator for stock rows.

        if (pos + 4 > size) throw std::runtime_error("short clip vector count");
        uint32_t count = 0;
        std::memcpy(&count, body + pos, 4);
        pos += 4;
        group.clips.clear();
        group.clips.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
          group.clips.push_back(read_len_string(body, size, pos));
        }

        if (pos + 4 > size) throw std::runtime_error("short CharClipGroup which");
        std::memcpy(&group.which, body + pos, 4);
        pos += 4;
        group.flags = 0;
        if (version > 1) {
          if (pos + 4 > size) throw std::runtime_error("short CharClipGroup flags");
          std::memcpy(&group.flags, body + pos, 4);
          pos += 4;
        }

        group.version = version;
        group.milo_path = milo_path;
        group.loaded = true;
        std::fprintf(stderr,
                     "[clip-group-source] group=%s milo=%s version=%u "
                     "clips=%zu which=%d flags=0x%08x\n",
                     group_name.c_str(), milo_path.c_str(), version,
                     group.clips.size(), group.which,
                     static_cast<unsigned>(group.flags));
        return group;
      }
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[clip-group-source] group=%s error=%s\n",
                 group_name.c_str(), ex.what());
  }
  return group;
}

std::optional<size_t> char_clip_group_get_clip_index(CharClipGroup& group) {
  if (group.clips.empty()) return std::nullopt;
  const int32_t before = group.which;
  ++group.which;
  if (group.which >= static_cast<int32_t>(group.clips.size())) {
    group.which = 0;
  }
  if (group.which < 0) group.which = 0;
  const size_t index = static_cast<size_t>(group.which);
  std::fprintf(stderr,
               "[clip-group-source-select] group=%s before=%d after=%d "
               "index=%zu clip=%s\n",
               group.name.c_str(), before, group.which, index,
               group.clips[index].c_str());
  return index;
}

std::vector<std::string> load_clip_group_names(
    const std::string& hdr_path, const std::string& ark_path,
    const std::vector<std::string>& milo_paths,
    const std::string& group_name) {
  return load_clip_group(hdr_path, ark_path, milo_paths, group_name).clips;
}

int source_char_clip_group_num_flag_duplicates(
    const std::vector<uint32_t>& clip_flags,
    size_t clip_index,
    uint32_t mask) {
  if (clip_index >= clip_flags.size()) return 0;
  const uint32_t flags = clip_flags[clip_index];
  int count = 0;
  for (size_t i = 0; i < clip_flags.size(); ++i) {
    if (i != clip_index && (mask & flags) == (mask & clip_flags[i])) {
      ++count;
    }
  }
  return count;
}

uint32_t char_clip_driver_masked_play_flags(const CharClip& clip,
                                            uint32_t mask) {
  uint32_t play_flags = clip.default_play_flags;
  if (mask & 0xF0u) play_flags = (play_flags & 0xffffff0fu) | (mask & 0xF0u);
  if (mask & 0x0Fu) play_flags = (play_flags & 0xfffffff0u) | (mask & 0x0Fu);
  if (mask & 0xF600u) {
    play_flags = (play_flags & 0xffff09ffu) | (mask & 0xF600u);
  }
  return play_flags;
}

const char* source_char_clip_beat_align_string(uint32_t mask) {
  switch (mask & 0xF600u) {
    case kCharPlayRealTime:
      return "RealTime";
    case kCharPlayUserTime:
      return "UserTime";
    case 0x1000u:
      return "BeatAlign1";
    case 0x2000u:
      return "BeatAlign2";
    case 0x4000u:
      return "BeatAlign4";
    case 0x8000u:
      return "BeatAlign8";
    default:
      return "NoAlign";
  }
}

SourceCharClipFlagUpdate source_char_clip_set_flags(uint32_t current_flags,
                                                    bool current_dirty,
                                                    uint32_t requested_flags) {
  SourceCharClipFlagUpdate update;
  update.value = current_flags;
  update.dirty = current_dirty;
  if (requested_flags != current_flags) {
    update.value = requested_flags;
    update.dirty = true;
    update.changed = true;
  }
  return update;
}

SourceCharClipDefaultState source_char_clip_default_state() {
  return SourceCharClipDefaultState{};
}

SourceCharClipFlagUpdate source_char_clip_set_play_flags(
    uint32_t current_play_flags,
    bool current_dirty,
    uint32_t requested_play_flags) {
  SourceCharClipFlagUpdate update;
  update.value = current_play_flags;
  update.dirty = current_dirty;
  if (requested_play_flags != current_play_flags) {
    update.value = requested_play_flags;
    update.dirty = true;
    update.changed = true;
  }
  return update;
}

bool source_char_driver_starved(bool has_first, bool first_has_next,
                                uint32_t first_play_flags) {
  if (has_first) {
    if (first_has_next) return false;
    if ((first_play_flags & 0xF0u) == kCharPlayNoLoop) return false;
  }
  return true;
}

float source_char_driver_resolve_blend_width(float requested_blend_width,
                                             float driver_blend_width) {
  return requested_blend_width == -1.0f ? driver_blend_width
                                        : requested_blend_width;
}

bool source_char_driver_should_start_clip(bool play_multiple_clips,
                                          bool clip_already_playing) {
  if (play_multiple_clips && clip_already_playing) return false;
  return true;
}

std::optional<size_t> source_char_driver_first_playing_index(
    const std::vector<float>& source_stack_blend_fracs) {
  for (size_t i = 0; i < source_stack_blend_fracs.size(); ++i) {
    if (source_stack_blend_fracs[i] != 0.0f) return i;
  }
  return std::nullopt;
}

// ---- pose application ----------------------------------------------------

static void quat_to_rot(const float q[4], float rot[3][3]) {
  float x = q[0], y = q[1], z = q[2], w = q[3];
  float len2 = x*x + y*y + z*z + w*w;
  if (len2 > 1e-8f) { float inv = 1.0f / std::sqrt(len2); x*=inv; y*=inv; z*=inv; w*=inv; }
  float m[3][3];
  m[0][0] = 1 - 2*(y*y + z*z);  m[0][1] = 2*(x*y + z*w);      m[0][2] = 2*(x*z - y*w);
  m[1][0] = 2*(x*y - z*w);      m[1][1] = 1 - 2*(x*x + z*z);  m[1][2] = 2*(y*z + x*w);
  m[2][0] = 2*(x*z + y*w);      m[2][1] = 2*(y*z - x*w);      m[2][2] = 1 - 2*(x*x + y*y);
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      rot[r][c] = m[r][c];
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

[[maybe_unused]] static bool is_eye_mesh_name(const std::string& name) {
  std::string lower = name;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return lower.rfind("eye-", 0) == 0 ||
         lower.rfind("l-eye.", 0) == 0 ||
         lower.rfind("r-eye.", 0) == 0 ||
         lower.find("_eyel") != std::string::npos ||
         lower.find("_eyer") != std::string::npos ||
         lower.find("eye_l") != std::string::npos ||
         lower.find("eye_r") != std::string::npos ||
         lower.find("eyel.") != std::string::npos ||
         lower.find("eyer.") != std::string::npos;
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

static bool transform_local_chain_world(const Character& character,
                                        const std::string& name,
                                        std::array<float, 16>& out) {
  const auto runtime_it = character.runtime_world_overrides.find(name);
  if (runtime_it != character.runtime_world_overrides.end()) {
    out = runtime_it->second;
    return true;
  }
  for (const auto& b : character.bones) {
    if (b.name == name || channel_matches_bone(b.name, name)) {
      out = character.bone_world_local_chain(b.name);
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

static Vec3 vec_from_array3(const float v[3]) {
  return {v[0], v[1], v[2]};
}

static Vec3 vec_from_array3(const std::array<float, 3>& v) {
  return {v[0], v[1], v[2]};
}

static void array3_from_vec(std::array<float, 3>& out, Vec3 v) {
  out[0] = v.x;
  out[1] = v.y;
  out[2] = v.z;
}

static void set_mat_row(std::array<float, 16>& m, int r, Vec3 v) {
  m[r * 4 + 0] = v.x;
  m[r * 4 + 1] = v.y;
  m[r * 4 + 2] = v.z;
}

static Vec3 local_vec_from_world_rows(const std::array<float, 16>& basis_world,
                                      Vec3 world_vec) {
  return {vdot(mat_row(basis_world, 0), world_vec),
          vdot(mat_row(basis_world, 1), world_vec),
          vdot(mat_row(basis_world, 2), world_vec)};
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

static Vec3 rotate_vec_by_quat(Vec3 v, const float q_in[4]) {
  float rot[3][3] = {};
  quat_to_rot(q_in, rot);
  return {
      v.x * rot[0][0] + v.y * rot[1][0] + v.z * rot[2][0],
      v.x * rot[0][1] + v.y * rot[1][1] + v.z * rot[2][1],
      v.x * rot[0][2] + v.y * rot[1][2] + v.z * rot[2][2],
  };
}

static void source_look_at_rows(std::array<float, 16>& m) {
  const Vec3 x = mat_row(m, 0);
  const Vec3 z = vnorm(vcross(x, mat_row(m, 1)), {0.0f, 0.0f, 1.0f});
  const Vec3 y = vcross(z, x);
  set_mat_row(m, 1, y);
  set_mat_row(m, 2, z);
}

static void normalize_mat3_rows(std::array<float, 16>& m);

static std::array<float, 16> source_transform_row_mat4(
    const float xfm[4][3]) {
  return {xfm[0][0], xfm[0][1], xfm[0][2], 0.0f,
          xfm[1][0], xfm[1][1], xfm[1][2], 0.0f,
          xfm[2][0], xfm[2][1], xfm[2][2], 0.0f,
          xfm[3][0], xfm[3][1], xfm[3][2], 1.0f};
}

bool source_char_ik_rod_compute_world(const CharIKRod& rod,
                                      const Character& character,
                                      std::array<float, 16>& dest_world) {
  if (rod.dest.empty() || rod.left_end.empty() || rod.right_end.empty()) {
    return false;
  }
  if (!character.has_transform(rod.dest)) return false;

  std::array<float, 16> left_world{};
  std::array<float, 16> right_world{};
  if (!transform_local_chain_world(character, rod.left_end, left_world) ||
      !transform_local_chain_world(character, rod.right_end, right_world)) {
    return false;
  }

  std::array<float, 16> rod_world =
      {1, 0, 0, 0, 0, 1, 0, 0,
       0, 0, 1, 0, 0, 0, 0, 1};
  const float t = rod.dest_pos;
  const Vec3 left_pos = mat_pos(left_world);
  const Vec3 right_pos = mat_pos(right_world);
  const Vec3 pos =
      vadd(vscale(left_pos, 1.0f - t), vscale(right_pos, t));
  rod_world[12] = pos.x;
  rod_world[13] = pos.y;
  rod_world[14] = pos.z;

  const Vec3 x = rod.vertical
                     ? Vec3{0.0f, 0.0f, -1.0f}
                     : vnorm(vadd(vscale(mat_row(left_world, 0), 1.0f - t),
                                  vscale(mat_row(right_world, 0), t)));
  Vec3 z{};
  if (!rod.side_axis.empty()) {
    std::array<float, 16> side_world{};
    if (transform_local_chain_world(character, rod.side_axis, side_world)) {
      z = mat_row(side_world, 2);
    } else {
      z = vsub(left_pos, right_pos);
    }
  } else {
    z = vsub(left_pos, right_pos);
  }
  Vec3 y = vnorm(vcross(z, x));
  z = vcross(x, y);
  set_mat_row(rod_world, 0, x);
  set_mat_row(rod_world, 1, y);
  set_mat_row(rod_world, 2, z);
  normalize_mat3_rows(rod_world);

  dest_world = mat4_mul(source_transform_row_mat4(rod.xfm), rod_world);
  normalize_mat3_rows(dest_world);
  return true;
}

static void normalize_xfm_rows(milo_scene::Xfm& xfm);

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

static void set_local_from_world(milo_scene::Xfm& local,
                                 const std::array<float, 16>& desired_world,
                                 const std::array<float, 16>& parent_world) {
  mat4_to_xfm(mat4_mul(desired_world, affine_inverse(parent_world)), local);
}

static std::array<float, 16> rotation_about_x_world(float angle) {
  const float ca = std::cos(angle);
  const float sa = std::sin(angle);
  return {1, 0, 0, 0, 0, ca, -sa, 0, 0, sa, ca, 0, 0, 0, 0, 1};
}

static std::array<float, 16> source_matrix_multiply_rotation(
    const std::array<float, 16>& rot,
    const std::array<float, 16>& world) {
  std::array<float, 16> out = mat4_mul(rot, world);
  out[12] = world[12];
  out[13] = world[13];
  out[14] = world[14];
  out[15] = 1.0f;
  return out;
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

static float wrap_ps2_angle(float radians) {
  constexpr float kPi = 3.1415927410125732f;
  constexpr float kTwoPi = 6.2831854820251465f;
  float wrapped = std::fmod(radians + kPi, kTwoPi);
  if (wrapped < 0.0f) wrapped += kTwoPi;
  wrapped -= kPi;
  return wrapped;
}

static float source_limit_ang(float radians) {
  constexpr float kPi = 3.14159265358979323846f;
  constexpr float kTwoPi = 6.28318530717958647692f;
  while (radians > kPi) radians -= kTwoPi;
  while (radians < -kPi) radians += kTwoPi;
  return radians;
}

static void write_source_elbow_z_bend(milo_scene::Xfm& dst,
                                      const milo_scene::Xfm& base,
                                      float cos_angle,
                                      float sin_angle) {
  dst = base;
  // CharIKHand::IKElbow writes the bend on the hand parent while preserving the
  // authored local position. Native expresses the source loc210/sqrted branch
  // as a row-vector Z bend.
  dst.rot[0][0] = cos_angle;
  dst.rot[0][1] = -sin_angle;
  dst.rot[0][2] = 0.0f;
  dst.rot[1][0] = sin_angle;
  dst.rot[1][1] = cos_angle;
  dst.rot[1][2] = 0.0f;
  dst.rot[2][0] = 0.0f;
  dst.rot[2][1] = 0.0f;
  dst.rot[2][2] = 1.0f;
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

static Vec3 source_transform_point(Vec3 local,
                                   const std::array<float, 16>& world) {
  return {local.x * world[0] + local.y * world[4] + local.z * world[8] +
              world[12],
          local.x * world[1] + local.y * world[5] + local.z * world[9] +
              world[13],
          local.x * world[2] + local.y * world[6] + local.z * world[10] +
              world[14]};
}

static std::array<float, 16> source_matrix3_to_mat4(const float m[9]) {
  return {m[0], m[1], m[2], 0.0f,
          m[3], m[4], m[5], 0.0f,
          m[6], m[7], m[8], 0.0f,
          0.0f, 0.0f, 0.0f, 1.0f};
}

static std::array<float, 16> source_char_hair_root_world(
    const CharHairStrand& strand, const std::array<float, 16>& root_world,
    const std::array<float, 16>& parent_world) {
  auto root_mat = source_matrix3_to_mat4(strand.root_mat);
  auto parent_rot = parent_world;
  parent_rot[12] = parent_rot[13] = parent_rot[14] = 0.0f;
  auto out = mat4_mul(root_mat, parent_rot);
  out[12] = root_world[12];
  out[13] = root_world[13];
  out[14] = root_world[14];
  out[15] = 1.0f;
  normalize_mat3_rows(out);
  return out;
}

static const std::string* source_transform_parent(
    const Character& character, const std::string& name) {
  for (const auto& bone : character.bones) {
    if (bone.name == name || channel_matches_bone(bone.name, name))
      return &bone.parent;
  }
  for (const auto& mesh : character.meshes) {
    if (mesh.name == name || channel_matches_bone(mesh.name, name))
      return &mesh.parent;
  }
  return nullptr;
}

static SourceCharHairRuntime& ensure_source_char_hair_runtime(
    Character& character, const CharHair& hair) {
  SourceCharHairRuntime& state = character.source_char_hair_runtime[hair.name];
  if (state.strands.size() != hair.strands.size()) {
    state.strands.clear();
    state.strands.resize(hair.strands.size());
    state.initialized = false;
    state.reset = 1;
  }
  for (size_t si = 0; si < hair.strands.size(); ++si) {
    auto& runtime_strand = state.strands[si];
    if (runtime_strand.points.size() != hair.strands[si].points.size()) {
      runtime_strand.points.clear();
      runtime_strand.points.resize(hair.strands[si].points.size());
      state.initialized = false;
      state.reset = 1;
    }
    for (size_t pi = 0; pi < hair.strands[si].points.size(); ++pi) {
      auto& runtime_point = runtime_strand.points[pi];
      if (runtime_point.initialized) continue;
      array3_from_vec(runtime_point.pos,
                      vec_from_array3(hair.strands[si].points[pi].pos));
      runtime_point.force = {0.0f, 0.0f, 0.0f};
      runtime_point.last_friction = {0.0f, 0.0f, 0.0f};
      runtime_point.last_z = {0.0f, 0.0f, 0.0f};
      runtime_point.initialized = true;
    }
  }
  if (!state.initialized) {
    state.initialized = true;
    state.reset = 1;
  }
  return state;
}

static void source_char_hair_do_reset(Character& character, const CharHair& hair,
                                      SourceCharHairRuntime& state,
                                      int reset_count);

static bool source_char_hair_has_resolved_collides(const CharHairPoint& point) {
  (void)point;
  return false;
}

static int source_char_hair_simulate_internal(Character& character,
                                              const CharHair& hair,
                                              SourceCharHairRuntime& state,
                                              float fps, float inertia,
                                              float friction) {
  if (!hair.simulate || hair.strands.empty()) return 0;
  const float safe_fps = fps > 0.0f ? fps : 60.0f;
  const float sixty_over = 60.0f / safe_fps;
  const float f19 = (1.0f / safe_fps) * sixty_over;
  const float powed =
      std::pow(1.0f - hair.stiffness, sixty_over * sixty_over);
  Vec3 wind_gravity{0.0f, 0.0f, hair.gravity * f19 * -3.858268f};
  int write_count = 0;

  for (size_t si = 0; si < hair.strands.size(); ++si) {
    const auto& strand = hair.strands[si];
    if (strand.root.empty()) continue;
    const std::string* parent_name =
        source_transform_parent(character, strand.root);
    if (!parent_name || parent_name->empty()) continue;

    std::array<float, 16> root_world{};
    std::array<float, 16> parent_world{};
    if (!transform_local_chain_world(character, strand.root, root_world) ||
        !transform_local_chain_world(character, *parent_name, parent_world)) {
      continue;
    }

    std::array<float, 16> t100 =
        source_char_hair_root_world(strand, root_world, parent_world);
    auto& runtime_strand = state.strands[si];
    if (runtime_strand.points.size() != strand.points.size()) continue;
    auto& next_runtime_strand =
        state.strands[(si + 1) % std::max<size_t>(state.strands.size(), 1)];
    const auto& next_strand =
        hair.strands[(si + 1) % std::max<size_t>(hair.strands.size(), 1)];

    for (size_t pi = 0; pi < strand.points.size(); ++pi) {
      const auto& point = strand.points[pi];
      auto& runtime_point = runtime_strand.points[pi];
      Vec3 point_pos = vec_from_array3(runtime_point.pos);
      const Vec3 v140 = point_pos;
      point_pos = vadd(point_pos, vec_from_array3(runtime_point.force));
      point_pos = vadd(point_pos, wind_gravity);

      if (point.side_length >= 0.0f && pi < next_strand.points.size() &&
          pi < next_runtime_strand.points.size()) {
        auto& next_runtime_point = next_runtime_strand.points[pi];
        Vec3 next_pos = vec_from_array3(next_runtime_point.pos);
        Vec3 v_res = vsub(point_pos, next_pos);
        const float len_sq = vdot(v_res, v_res);
        const float min_len = point.side_length - hair.min_slack;
        const float min_len_sq = min_len * min_len;
        if (len_sq < min_len_sq) {
          v_res = vscale(v_res, min_len_sq / (min_len_sq + len_sq) - 0.5f);
          point_pos = vadd(point_pos, v_res);
          next_pos = vsub(next_pos, v_res);
          array3_from_vec(next_runtime_point.pos, next_pos);
        } else {
          const float max_len = point.side_length + hair.max_slack;
          const float max_len_sq = max_len * max_len;
          if (max_len > max_len_sq) {
            v_res =
                vscale(v_res, max_len_sq / (max_len_sq + len_sq) - 0.5f);
            point_pos = vadd(point_pos, v_res);
            next_pos = vsub(next_pos, v_res);
            array3_from_vec(next_runtime_point.pos, next_pos);
          }
        }
      }

      Vec3 m128_y = vsub(point_pos, mat_pos(t100));
      const float len_sq = std::max(vdot(m128_y, m128_y), 1.0e-8f);
      const float rsa = 1.0f / std::sqrt(len_sq);
      const float rsalen = point.length * rsa - 1.0f;
      if (pi > 0) {
        auto& prev_runtime_point = runtime_strand.points[pi - 1];
        const Vec3 prev_force = vec_from_array3(prev_runtime_point.force);
        array3_from_vec(prev_runtime_point.force,
                        vadd(prev_force,
                             vscale(m128_y, -sixty_over * 0.5f * rsalen)));
      }
      point_pos = vadd(point_pos, vscale(m128_y, rsalen));
      array3_from_vec(runtime_point.pos, point_pos);

      const Vec3 v158 =
          vadd(mat_pos(t100), vscale(mat_row(t100, 1), point.length));
      Vec3 m128_z =
          vadd(vscale(vec_from_array3(runtime_point.last_z),
                      1.0f - hair.torsion),
               vscale(mat_row(t100, 2), hair.torsion));

      if (source_char_hair_has_resolved_collides(point)) {
        Vec3 y = vscale(m128_y, rsa);
        Vec3 x = vnorm(vcross(y, m128_z), mat_row(t100, 0));
        Vec3 z = vcross(x, y);
        set_mat_row(t100, 0, x);
        set_mat_row(t100, 1, y);
        set_mat_row(t100, 2, z);
        t100[12] = point_pos.x;
        t100[13] = point_pos.y;
        t100[14] = point_pos.z;
        normalize_mat3_rows(t100);
        array3_from_vec(runtime_point.last_z, mat_row(t100, 2));
        if (!point.bone.empty()) {
          character.runtime_world_overrides[point.bone] = t100;
          ++write_count;
        }
        Vec3 force = vsub(v158, point_pos);
        Vec3 v170 = vsub(vec_from_array3(runtime_point.last_friction), force);
        array3_from_vec(runtime_point.last_friction, force);
        force = vscale(force, 1.0f - powed);
        force = vadd(force, vscale(v170, -friction));
        Vec3 v17c = vsub(point_pos, v140);
        force = vadd(force, vscale(v17c, inertia));
        array3_from_vec(runtime_point.force, force);
      }
    }
  }

  return write_count;
}

static int source_char_hair_simulate_loops(Character& character,
                                           const CharHair& hair,
                                           SourceCharHairRuntime& state,
                                           int count, float fps,
                                           float inertia, float friction) {
  if (!hair.simulate || hair.strands.empty()) return 0;
  int write_count = 0;
  for (int i = 0; i < count; ++i) {
    write_count += source_char_hair_simulate_internal(
        character, hair, state, fps, inertia, friction);
  }
  return write_count;
}

static void source_char_hair_do_reset(Character& character, const CharHair& hair,
                                      SourceCharHairRuntime& state,
                                      int reset_count) {
  for (size_t si = 0; si < hair.strands.size(); ++si) {
    const auto& strand = hair.strands[si];
    if (strand.root.empty()) continue;
    const std::string* parent_name =
        source_transform_parent(character, strand.root);
    if (!parent_name || parent_name->empty()) continue;
    if (si >= state.strands.size()) continue;
    auto& runtime_strand = state.strands[si];

    std::array<float, 16> root_world{};
    std::array<float, 16> parent_world{};
    if (!transform_local_chain_world(character, strand.root, root_world) ||
        !transform_local_chain_world(character, *parent_name, parent_world)) {
      continue;
    }

    Vec3 v80 = mat_pos(root_world);
    Vec3 v8c = mat_row(root_world, 0);
    for (size_t pi = 0; pi < strand.points.size() &&
                        pi < runtime_strand.points.size();
         ++pi) {
      const auto& point = strand.points[pi];
      auto& runtime_point = runtime_strand.points[pi];
      const Vec3 pos = source_transform_point(vec_from_array3(point.unk5c),
                                              parent_world);
      array3_from_vec(runtime_point.pos, pos);
      const Vec3 v98 = vsub(pos, v80);
      v80 = pos;
      const Vec3 last_z = vnorm(vcross(v8c, v98), {0.0f, 0.0f, 1.0f});
      array3_from_vec(runtime_point.last_z, last_z);
      v8c = vcross(v98, last_z);
      runtime_point.force = {0.0f, 0.0f, 0.0f};
      runtime_point.last_friction = {0.0f, 0.0f, 0.0f};
    }
  }

  source_char_hair_simulate_loops(character, hair, state,
                                  std::max(reset_count, 0), 60.0f, 0.0f,
                                  0.0f);
  state.reset = 0;
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

static void log_debug_xfm_row_short(const char* tag, const char* name,
                                    const milo_scene::Xfm& local) {
  std::fprintf(stderr, "[%s-pos] %s pos=[%.5f %.5f %.5f]\n", tag, name,
               local.pos[0], local.pos[1], local.pos[2]);
  std::fprintf(stderr, "[%s-r0] %s r0=[%.5f %.5f %.5f]\n", tag, name,
               local.rot[0][0], local.rot[0][1], local.rot[0][2]);
  std::fprintf(stderr, "[%s-r1] %s r1=[%.5f %.5f %.5f]\n", tag, name,
               local.rot[1][0], local.rot[1][1], local.rot[1][2]);
  std::fprintf(stderr, "[%s-r2] %s r2=[%.5f %.5f %.5f]\n", tag, name,
               local.rot[2][0], local.rot[2][1], local.rot[2][2]);
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

static bool apply_source_fore_twist(Character& character,
                                    const CharForeTwist& ft) {
  const int hand_i = find_bone_index(character, ft.hand);
  const int twist2_i = find_bone_index(character, ft.twist2);
  if (hand_i < 0 || twist2_i < 0) return false;
  auto& hand = character.bones[static_cast<size_t>(hand_i)];
  auto& twist2 = character.bones[static_cast<size_t>(twist2_i)];
  if (hand.parent.empty() || twist2.parent.empty()) return false;
  const int parent_i = find_bone_index(character, hand.parent);
  const int twist1_i = find_bone_index(character, twist2.parent);
  if (parent_i < 0 || twist1_i < 0) return false;
  auto& twist1 = character.bones[static_cast<size_t>(twist1_i)];

  const auto parent_world = character.bone_world_local_chain(
      character.bones[static_cast<size_t>(parent_i)].name);
  const auto hand_world = character.bone_world_local_chain(hand.name);
  const float clamped =
      std::clamp(vdot(mat_row(hand_world, 2), mat_row(parent_world, 1)),
                 -1.0f, 1.0f);
  const Vec3 cross = vcross(mat_row(parent_world, 1), mat_row(hand_world, 2));
  const float clamped2 =
      std::clamp(vdot(mat_row(parent_world, 0), cross), -1.0f, 1.0f);
  constexpr float kDegToRad = 0.01745329238474369049f;
  const float bias = ft.bias_degrees * kDegToRad;
  const float angle = source_limit_ang(
      ft.offset_degrees * kDegToRad + std::atan2(clamped2, clamped) + bias);
  const auto rot = rotation_about_x_world((angle - bias) * 0.33333f);

  std::array<float, 16> twist1_world =
      source_matrix_multiply_rotation(rot, parent_world);
  if (!twist1.parent.empty()) {
    set_local_from_world(twist1.local, twist1_world,
                         character.bone_world_local_chain(twist1.parent));
  } else {
    mat4_to_xfm(twist1_world, twist1.local);
  }

  const float ratio =
      std::fabs(hand.local.pos[0]) > 1.0e-6f
          ? twist2.local.pos[0] / hand.local.pos[0]
          : 0.0f;
  std::array<float, 16> twist2_world =
      source_matrix_multiply_rotation(rot, twist1_world);
  const Vec3 twist1_pos = mat_pos(twist1_world);
  const Vec3 hand_pos = mat_pos(hand_world);
  const Vec3 interp_pos =
      vadd(vscale(twist1_pos, 1.0f - ratio), vscale(hand_pos, ratio));
  twist2_world[12] = interp_pos.x;
  twist2_world[13] = interp_pos.y;
  twist2_world[14] = interp_pos.z;
  set_local_from_world(twist2.local, twist2_world, twist1_world);

  if (debug_ik_enabled()) {
    std::fprintf(stderr,
                 "[twist-fore-source] %s hand=%s twist1=%s twist2=%s "
                 "offset=%.3f bias=%.3f angle=%.5f ratio=%.5f\n",
                 ft.name.c_str(), ft.hand.c_str(), twist1.name.c_str(),
                 ft.twist2.c_str(), ft.offset_degrees, ft.bias_degrees,
                 angle, ratio);
  }
  return true;
}

static void apply_source_fore_twists(Character& character) {
  for (const auto& ft : character.fore_twists) {
    apply_source_fore_twist(character, ft);
  }
}

static void apply_source_upper_twists(
    Character& character, const std::vector<milo_scene::Xfm>& bind_bones) {
  (void)bind_bones;
  for (const auto& ut : character.upper_twists) {
    const int upper_i = find_bone_index(character, ut.upper_arm);
    const int twist1_i = find_bone_index(character, ut.twist1);
    const int twist2_i = find_bone_index(character, ut.twist2);
    if (upper_i < 0 || twist1_i < 0 || twist2_i < 0) continue;
    auto& upper = character.bones[static_cast<size_t>(upper_i)];
    auto& twist1 = character.bones[static_cast<size_t>(twist1_i)];
    auto& twist2 = character.bones[static_cast<size_t>(twist2_i)];
    if (upper.parent.empty()) continue;

    const auto upper_parent_world = character.bone_world_local_chain(upper.parent);
    const auto upper_world = character.bone_world_local_chain(upper.name);
    float q[4] = {};
    quat_from_vec_to_vec(mat_row(upper_parent_world, 0),
                         mat_row(upper_world, 0), q);
    const Vec3 v68 = rotate_vec_by_quat(mat_row(upper_parent_world, 1), q);

    auto write_output = [&](milo_scene::TransObj& bone, float weight) {
      std::array<float, 16> out_world = upper_world;
      set_mat_row(out_world, 0, mat_row(upper_world, 0));
      set_mat_row(out_world, 1,
                  vadd(vscale(v68, 1.0f - weight),
                       vscale(mat_row(upper_world, 1), weight)));
      out_world[12] = character.bone_world_local_chain(bone.name)[12];
      out_world[13] = character.bone_world_local_chain(bone.name)[13];
      out_world[14] = character.bone_world_local_chain(bone.name)[14];
      source_look_at_rows(out_world);
      if (!bone.parent.empty()) {
        set_local_from_world(bone.local, out_world,
                             character.bone_world_local_chain(bone.parent));
      } else {
        mat4_to_xfm(out_world, bone.local);
      }
    };

    write_output(twist1, 0.333f);
    write_output(twist2, 0.666f);
    if (debug_ik_enabled()) {
      std::fprintf(stderr,
                   "[twist-upper-source] %s upper=%s twist1=%s twist2=%s\n",
                   ut.name.c_str(), ut.upper_arm.c_str(), ut.twist1.c_str(),
                   ut.twist2.c_str());
      log_debug_xfm_row("twist-upper-upper", ut.upper_arm.c_str(),
                        upper.local,
                        character.bone_world_local_chain(ut.upper_arm));
      log_debug_xfm_row("twist-upper-out", ut.twist1.c_str(),
                        twist1.local,
                        character.bone_world_local_chain(ut.twist1));
      log_debug_xfm_row("twist-upper-out", ut.twist2.c_str(),
                        twist2.local,
                        character.bone_world_local_chain(ut.twist2));
    }
  }
}

static void apply_source_pos_constraints(Character& character) {
  for (const auto& constraint : character.pos_constraints) {
    if (constraint.source.empty() || constraint.targets.empty()) continue;
    std::array<float, 16> source_world{};
    if (!transform_local_chain_world(character, constraint.source,
                                     source_world)) {
      continue;
    }
    const Vec3 source_pos = mat_pos(source_world);
    for (const auto& target : constraint.targets) {
      if (target.empty()) continue;
      std::array<float, 16> target_world{};
      if (!transform_local_chain_world(character, target, target_world)) {
        continue;
      }
      Vec3 delta = vsub(mat_pos(target_world), source_pos);
      if (constraint.box_min[0] <= constraint.box_max[0]) {
        delta.x = std::clamp(delta.x, constraint.box_min[0],
                             constraint.box_max[0]);
      }
      if (constraint.box_min[1] <= constraint.box_max[1]) {
        delta.y = std::clamp(delta.y, constraint.box_min[1],
                             constraint.box_max[1]);
      }
      if (constraint.box_min[2] <= constraint.box_max[2]) {
        delta.z = std::clamp(delta.z, constraint.box_min[2],
                             constraint.box_max[2]);
      }
      const Vec3 target_pos = vadd(source_pos, delta);
      target_world[12] = target_pos.x;
      target_world[13] = target_pos.y;
      target_world[14] = target_pos.z;
      character.runtime_world_overrides[target] = target_world;
      if (controller_audit_enabled()) {
        std::fprintf(stderr,
                     "[posconstraint-source] %s source=%s target=%s "
                     "world=(%.3f %.3f %.3f) delta=(%.3f %.3f %.3f) "
                     "boxMin=(%.3f %.3f %.3f) boxMax=(%.3f %.3f %.3f)\n",
                     constraint.name.c_str(), constraint.source.c_str(),
                     target.c_str(), target_pos.x, target_pos.y, target_pos.z,
                     delta.x, delta.y, delta.z, constraint.box_min[0],
                     constraint.box_min[1], constraint.box_min[2],
                     constraint.box_max[0], constraint.box_max[1],
                     constraint.box_max[2]);
      }
    }
  }
}

float source_char_weightable_weight(
    const CharWeightSetter& setter,
    const std::unordered_map<std::string, float>& weights_by_name) {
  if (!setter.weight_owner.empty()) {
    const auto owner = weights_by_name.find(setter.weight_owner);
    if (owner != weights_by_name.end()) return owner->second;
  }
  return setter.weight;
}

bool source_char_weight_setter_poll(
    const CharWeightSetter& setter,
    const std::unordered_map<std::string, float>& weights_by_name,
    float delta_beats,
    float& out_weight) {
  float base_weight = setter.base_weight;
  if (!setter.driver.empty()) {
    return false;
  }
  if (!setter.base.empty()) {
    const auto base = weights_by_name.find(setter.base);
    if (base == weights_by_name.end()) return false;
    base_weight = setter.scale * base->second + setter.offset;
  }

  for (const auto& min_name : setter.min_weights) {
    const auto min_weight = weights_by_name.find(min_name);
    if (min_weight == weights_by_name.end()) return false;
    base_weight = std::min(base_weight, min_weight->second);
  }
  for (const auto& max_name : setter.max_weights) {
    const auto max_weight = weights_by_name.find(max_name);
    if (max_weight == weights_by_name.end()) return false;
    base_weight = std::max(base_weight, max_weight->second);
  }

  const float current = source_char_weightable_weight(setter, weights_by_name);
  if (base_weight == current) {
    out_weight = current;
    return true;
  }
  if (setter.beats_per_weight <= 0.0f) {
    out_weight = base_weight;
    return true;
  }

  const float step = delta_beats / setter.beats_per_weight;
  if (step > 0.0f) {
    const float delta = std::clamp(base_weight - current, -step, step);
    out_weight = current + delta;
  } else {
    out_weight = current;
  }
  return true;
}

static void apply_source_weight_setters(Character& character,
                                        float delta_beats) {
  std::unordered_map<std::string, float> weights_by_name;
  for (const auto& driver : character.drivers) {
    weights_by_name[driver.name] = driver.weight;
    if (!driver.weight_owner.empty()) {
      weights_by_name[driver.weight_owner] = driver.weight;
    }
  }
  for (const auto& setter : character.weight_setters) {
    weights_by_name[setter.name] = setter.weight;
    if (!setter.weight_owner.empty()) {
      weights_by_name[setter.weight_owner] = setter.weight;
    }
  }

  for (const auto& setter : character.weight_setters) {
    float weight = setter.weight;
    if (!source_char_weight_setter_poll(setter, weights_by_name, delta_beats,
                                        weight)) {
      if (controller_audit_enabled()) {
        std::fprintf(stderr,
                     "[weightsetter-source-skip] %s driver=%s base=%s "
                     "reason=missing-source-CharDriver-EvaluateFlags\n",
                     setter.name.c_str(),
                     setter.driver.empty() ? "<none>" : setter.driver.c_str(),
                     setter.base.empty() ? "<none>" : setter.base.c_str());
      }
      continue;
    }
    weights_by_name[setter.name] = weight;
    if (!setter.weight_owner.empty()) weights_by_name[setter.weight_owner] = weight;
    character.runtime_weight_props[setter.name] = weight;
    if (!setter.weight_owner.empty()) {
      character.runtime_weight_props[setter.weight_owner] = weight;
    }
    if (controller_audit_enabled()) {
      std::fprintf(stderr,
                   "[weightsetter-source] %s weight=%.5f driver=%s base=%s "
                   "mins=%zu maxs=%zu beatsPerWeight=%.5f\n",
                   setter.name.c_str(), weight,
                   setter.driver.empty() ? "<none>" : setter.driver.c_str(),
                   setter.base.empty() ? "<none>" : setter.base.c_str(),
                   setter.min_weights.size(), setter.max_weights.size(),
                   setter.beats_per_weight);
    }
  }
}

static void apply_source_ik_rods(Character& character) {
  for (const auto& rod : character.ik_rods) {
    std::array<float, 16> dest_world{};
    if (!source_char_ik_rod_compute_world(rod, character, dest_world)) {
      if (controller_audit_enabled()) {
        std::fprintf(stderr,
                     "[ikrod-source-skip] %s left=%s right=%s dest=%s "
                     "reason=missing-source-required-transform\n",
                     rod.name.c_str(),
                     rod.left_end.empty() ? "<none>" : rod.left_end.c_str(),
                     rod.right_end.empty() ? "<none>" : rod.right_end.c_str(),
                     rod.dest.empty() ? "<none>" : rod.dest.c_str());
      }
      continue;
    }
    character.runtime_world_overrides[rod.dest] = dest_world;
    if (controller_audit_enabled()) {
      std::fprintf(stderr,
                   "[ikrod-source] %s left=%s right=%s side=%s dest=%s "
                   "destPos=%.4f vertical=%d world=(%.3f %.3f %.3f)\n",
                   rod.name.c_str(), rod.left_end.c_str(),
                   rod.right_end.c_str(),
                   rod.side_axis.empty() ? "<none>" : rod.side_axis.c_str(),
                   rod.dest.c_str(), rod.dest_pos, rod.vertical ? 1 : 0,
                   dest_world[12], dest_world[13], dest_world[14]);
    }
  }
}

static float effective_ik_hand_solver_weight(const Character& character,
                                             const CharIKHand& ik) {
  if (!ik.weight_prop.empty()) {
    const auto runtime = character.runtime_weight_props.find(ik.weight_prop);
    if (runtime != character.runtime_weight_props.end()) {
      // MIDI hand-driver code writes the live left/right scalar each tick.
      // That live row overrides the decoded CharIKHand weight.
      return std::clamp(runtime->second, 0.0f, 1.0f);
    }
  }
  return std::clamp(ik.weight, 0.0f, 1.0f);
}

static float effective_ik_hand_target_blend_weight(const Character& character,
                                                   const CharIKHand& ik) {
  if (!ik.weight_prop.empty()) {
    const auto runtime = character.runtime_weight_props.find(ik.weight_prop);
    if (runtime != character.runtime_weight_props.end()) {
      // CharIKHand::Poll blends the hand world position toward the target using
      // the live CharWeightable scalar before the elbow and final hand writes.
      return std::clamp(runtime->second, 0.0f, 1.0f);
    }
  }
  return effective_ik_hand_solver_weight(character, ik);
}

static void apply_source_ik_hands(
    Character& character, const std::vector<milo_scene::Xfm>& bind_bones) {
  // Port of ihatecompvir's CharIKHand::Poll/IKElbow dataflow: resolve the
  // authored hand/target Trans rows, blend mWorldDst, solve the elbow chain,
  // then publish the final hand world row when orientation or stretch is set.
  for (const CharIKHand& ik : character.ik_hands) {
    const int hand_i = find_bone_index(character, ik.hand);
    const float solver_weight =
        effective_ik_hand_solver_weight(character, ik);
    const float target_blend_weight =
        effective_ik_hand_target_blend_weight(character, ik);
    if (hand_i < 0 || solver_weight <= 0.0f) {
      if (debug_ik_enabled()) {
        std::fprintf(stderr,
                     "[ik-source] %s skipped hand=%s solveWeight=%.3f "
                     "targetBlend=%.5f hand_found=%d\n",
                     ik.name.c_str(), ik.hand.c_str(), solver_weight,
                     target_blend_weight, hand_i >= 0 ? 1 : 0);
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
    const auto hand_world = character.bone_world_local_chain(hand.name);
    const Vec3 shoulder = mat_pos(upper_world0);
    const Vec3 raw_target = mat_pos(target_world);
    const std::string live_key =
        !ik.name.empty() ? ik.name : (ik.hand + "->" + ik.target);
    Vec3 previous_live = mat_pos(hand_world);
    if (const auto it = character.runtime_ik_hand_targets.find(live_key);
        it != character.runtime_ik_hand_targets.end()) {
      previous_live = {it->second[0], it->second[1], it->second[2]};
    }
    Vec3 target = raw_target;
    if (target_blend_weight < 0.999f) {
      target = vadd(vscale(previous_live, 1.0f - target_blend_weight),
                    vscale(raw_target, target_blend_weight));
    }
    character.runtime_ik_hand_targets[live_key] =
        {target.x, target.y, target.z};
    target_world[12] = target.x;
    target_world[13] = target.y;
    target_world[14] = target.z;
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
    const float authored_fore_len = std::max(
        0.001f, vlen({hand_setup.pos[0], hand_setup.pos[1],
                      hand_setup.pos[2]}));
    float fore_len = authored_fore_len;
    const Vec3 to_target = vsub(target, shoulder);
    const float raw_dist = vlen(to_target);
    // `stretch` is the final CharIKHand hand-world write. It does not rewrite
    // the hand child local length used by IKElbow or later foretwist rows.
    const float max_reach = upper_len + fore_len - 0.001f;
    const float min_reach = std::fabs(upper_len - fore_len) + 0.001f;
    const float dist = std::clamp(raw_dist, min_reach, max_reach);
    const float dist2 = dist * dist;
    // CharIKHand::MeasureLengths computes len^2 + parentlen^2 and
    // 1/(2*len*parentlen); IKElbow derives the cosine/sine bend from the
    // current shoulder-to-destination distance.
    const float cos_elbow = std::clamp(
        (dist2 - upper_len * upper_len - fore_len * fore_len) /
            (2.0f * upper_len * fore_len),
        -0.9850000143f, 0.9850000143f);
    const float sin_elbow =
        std::sqrt(std::max(0.0f, 1.0f - cos_elbow * cos_elbow));

    milo_scene::Xfm solved_fore = fore.local;
    write_source_elbow_z_bend(solved_fore, fore_local0, cos_elbow, sin_elbow);
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c)
        fore.local.rot[r][c] =
            fore_local0.rot[r][c] * (1.0f - solver_weight) +
            solved_fore.rot[r][c] * solver_weight;
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
    // CharIKHand::IKElbow calls MakeRotQuat/MakeRotMatrix and writes
    // Multiply(ma0, trans2->LocalXfm().m, trans2->DirtyLocalXfm().m).
    pre_multiply_local_rot(upper.local, upper_local0, swing_rot,
                           solver_weight);

    const auto hand_world_before_final =
        character.bone_world_local_chain(hand.name);
    const float pre_final_error =
        vlen(vsub(mat_pos(hand_world_before_final), target));

    const bool write_final = ik.stretch || ik.orientation;
    if (write_final) {
      std::array<float, 16> solved_world = character.bone_world_local_chain(hand.name);
      if (ik.orientation) {
        std::array<float, 16> desired_orientation = target_world;
        for (int r = 0; r < 3; ++r) {
          for (int c = 0; c < 3; ++c) {
            solved_world[r * 4 + c] =
                solved_world[r * 4 + c] * (1.0f - solver_weight) +
                desired_orientation[r * 4 + c] * solver_weight;
          }
        }
      }
      if (ik.stretch) {
        solved_world[12] = solved_world[12] * (1.0f - solver_weight) +
                           target_world[12] * solver_weight;
        solved_world[13] = solved_world[13] * (1.0f - solver_weight) +
                           target_world[13] * solver_weight;
        solved_world[14] = solved_world[14] * (1.0f - solver_weight) +
                           target_world[14] * solver_weight;
      }
      normalize_mat3_rows(solved_world);
      // CharIKHand::Poll closes with SetWorldXfm(tf). Native keeps the
      // authored local row available to later source controllers and exposes
      // the live hand world row through the transient Trans bridge.
      character.runtime_world_overrides[hand.name] = solved_world;
    }

    if (debug_ik_enabled()) {
      const Vec3 hp = mat_pos(hand_world);
      const Vec3 tp = mat_pos(target_world);
      const auto upper_world_post = character.bone_world_local_chain(upper.name);
      const auto fore_world_post = character.bone_world_local_chain(fore.name);
      const auto hand_world_post = character.bone_world_local_chain(hand.name);
      std::fprintf(stderr,
                   "[ik-source-swing] %s hand=%s target=%s currentLocal=[%.5f %.5f %.5f] targetLocal=[%.5f %.5f %.5f] quat=[%.5f %.5f %.5f %.5f]\n",
                   ik.name.c_str(), ik.hand.c_str(), ik.target.c_str(),
                   current_local.x, current_local.y, current_local.z,
                   target_local.x, target_local.y, target_local.z,
                   swing_quat[0], swing_quat[1], swing_quat[2],
                   swing_quat[3]);
      std::fprintf(stderr,
                   "[ik-swing-cur] %s current=[%.5f %.5f %.5f]\n",
                   ik.name.c_str(), current_local.x, current_local.y,
                   current_local.z);
      std::fprintf(stderr,
                   "[ik-swing-target] %s target=[%.5f %.5f %.5f]\n",
                   ik.name.c_str(), target_local.x, target_local.y,
                   target_local.z);
      std::fprintf(stderr,
                   "[ik-swing-quat] %s quat=[%.5f %.5f %.5f %.5f]\n",
                   ik.name.c_str(), swing_quat[0], swing_quat[1],
                   swing_quat[2], swing_quat[3]);
      std::fprintf(stderr,
                   "[ik-solve-len] %s upper=%.5f authoredFore=%.5f "
                   "fore=%.5f\n",
                   ik.name.c_str(), upper_len, authored_fore_len, fore_len);
      std::fprintf(stderr,
                   "[ik-solve-dist] %s raw=%.5f dist=%.5f cos=%.5f\n",
                   ik.name.c_str(), raw_dist, dist, cos_elbow);
      std::fprintf(stderr,
                   "[ik-solve-flags] %s stretch=%d orient=%d final=%d\n",
                   ik.name.c_str(), ik.stretch ? 1 : 0,
                   ik.orientation ? 1 : 0, write_final ? 1 : 0);
      std::fprintf(stderr,
                   "[ik-live-target] %s raw=[%.5f %.5f %.5f] live=[%.5f %.5f %.5f] prev=[%.5f %.5f %.5f] weight=%.5f\n",
                   ik.name.c_str(), raw_target.x, raw_target.y,
                   raw_target.z, target.x, target.y, target.z,
                   previous_live.x, previous_live.y, previous_live.z,
                   target_blend_weight);
      log_debug_world_row("ik-source-preswing-upper", upper.name.c_str(),
                          upper_world_after_bend);
      log_debug_world_row("ik-source-preswing-hand", hand.name.c_str(),
                          hand_world_after_bend);
      std::fprintf(stderr,
                   "[ik-source] %s hand=%s target=%s solveWeight=%.3f targetBlend=%.5f hand=[%.2f %.2f %.2f] target=[%.2f %.2f %.2f] len=(%.2f %.2f) dist=%.2f cos=%.3f swing=source-pre final=%d orient=%d stretch=%d bendParent=%s upper=%s\n",
                   ik.name.c_str(), ik.hand.c_str(), ik.target.c_str(),
                   solver_weight, target_blend_weight, hp.x, hp.y, hp.z,
                   tp.x, tp.y, tp.z, upper_len, fore_len, raw_dist, cos_elbow,
                   write_final ? 1 : 0, ik.orientation ? 1 : 0,
                   ik.stretch ? 1 : 0,
                   fore.name.c_str(), upper.name.c_str());
      std::fprintf(stderr,
                   "[ik-source-error] %s preFinalError=%.4f "
                   "preFinal=[%.2f %.2f %.2f]\n",
                   ik.name.c_str(), pre_final_error,
                   hand_world_before_final[12], hand_world_before_final[13],
                   hand_world_before_final[14]);
      log_debug_xfm_row("ik-source-row", upper.name.c_str(), upper.local,
                        upper_world_post);
      log_debug_xfm_row_short("ik-source-row", upper.name.c_str(),
                              upper.local);
      log_debug_xfm_row("ik-source-row", fore.name.c_str(), fore.local,
                        fore_world_post);
      log_debug_xfm_row_short("ik-source-row", fore.name.c_str(),
                              fore.local);
      log_debug_xfm_row("ik-source-row", hand.name.c_str(), hand.local,
                        hand_world_post);
      log_debug_xfm_row_short("ik-source-row", hand.name.c_str(),
                              hand.local);
      log_debug_world_row("ik-source-target", ik.target.c_str(), target_world);
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
    float scale[3] = {};
    for (int r = 0; r < 3; ++r) {
      scale[r] = std::sqrt(local.rot[r][0] * local.rot[r][0] +
                           local.rot[r][1] * local.rot[r][1] +
                           local.rot[r][2] * local.rot[r][2]);
      if (scale[r] <= 1e-8f) scale[r] = 1.0f;
    }
    float rot[3][3];
    quat_to_rot(pose.quat->quat, rot);
    if (relative) {
      float out[3][3] = {};
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          for (int k = 0; k < 3; ++k) {
            out[r][c] += local.rot[r][k] * rot[k][c];
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
  // offsets tears the forearm/hand chain. CharIKHand publishes the live hand
  // world row after the clip pass.
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
    float scale[3] = {};
    for (int r = 0; r < 3; ++r) {
      scale[r] = std::sqrt(local.rot[r][0] * local.rot[r][0] +
                           local.rot[r][1] * local.rot[r][1] +
                           local.rot[r][2] * local.rot[r][2]);
      if (scale[r] <= 1e-8f) scale[r] = 1.0f;
    }
    float rot[3][3];
    quat_to_rot(pose.quat->quat, rot);
    if (relative) {
      float out[3][3] = {};
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          for (int k = 0; k < 3; ++k) {
            out[r][c] += local.rot[r][k] * rot[k][c];
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

static bool hand_output_layer_disabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool disabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_HAND_OUTPUT_LAYER") == 0 &&
      value && value[0];
  std::free(value);
  return disabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_HAND_OUTPUT_LAYER");
  return value && value[0];
#endif
}

static bool is_hand_driver_root_key(const std::string& key) {
  return key == "bone_strum" || key == "bone_strum_hand" ||
         key == "bone_fret" || key == "bone_fret_hand";
}

static bool is_constant_fret_hand_target_key(const std::string& key) {
  return key == "bone_fret_hand";
}

static bool is_hand_driver_output_key(const std::string& key) {
  if (is_hand_driver_root_key(key)) return true;
  const bool left_or_right =
      key.rfind("bone_L-", 0) == 0 || key.rfind("bone_R-", 0) == 0;
  if (!left_or_right) return false;
  return key.find("-hand") != std::string::npos ||
         key.find("-index") != std::string::npos ||
         key.find("-middlefinger") != std::string::npos ||
         key.find("-ringfinger") != std::string::npos ||
         key.find("-pinky") != std::string::npos ||
         key.find("-thumb") != std::string::npos;
}

static bool output_bones_have_hand_driver_root(
    const std::vector<CharClip::OutputBone>& output_bones) {
  for (const auto& out : output_bones) {
    if (is_hand_driver_root_key(strip_transform_suffix(out.name))) {
      return true;
    }
  }
  return false;
}

enum class HandDriverOutputGroup {
  Fret,
  Strum,
};

static bool hand_driver_key_matches_group(const std::string& key,
                                          HandDriverOutputGroup group) {
  if (group == HandDriverOutputGroup::Fret) {
    return key == "bone_fret" || key == "bone_fret_hand" ||
           key.rfind("bone_L-", 0) == 0;
  }
  return key == "bone_strum" || key == "bone_strum_hand" ||
         key.rfind("bone_R-", 0) == 0;
}

static bool output_bones_have_hand_driver_group_root(
    const std::vector<CharClip::OutputBone>& output_bones,
    HandDriverOutputGroup group) {
  for (const auto& out : output_bones) {
    const std::string key = strip_transform_suffix(out.name);
    if (group == HandDriverOutputGroup::Fret &&
        (key == "bone_fret" || key == "bone_fret_hand")) {
      return true;
    }
    if (group == HandDriverOutputGroup::Strum &&
        (key == "bone_strum" || key == "bone_strum_hand")) {
      return true;
    }
  }
  return false;
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

static bool charbone_lower_body_output_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len,
                "GHOGX_ENABLE_CHARBONE_LOWER_BODY_OUTPUT") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_ENABLE_CHARBONE_LOWER_BODY_OUTPUT");
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
  return is_hand_driver_root_key(key) ||
         key == "bone_facing" || key == "bone_pelvis" ||
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
         key.find("-thumb") != std::string::npos ||
         key.find("-index") != std::string::npos ||
         key.find("-middlefinger") != std::string::npos ||
         key.find("-ringfinger") != std::string::npos ||
         key.find("-pinky") != std::string::npos ||
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
    bool relative, const std::vector<CharClip::OutputBone>& output_bones,
    bool force_selected_output = false) {
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
      charbone_lower_body_output_enabled();
  for (const auto& ch : channels) {
    const auto it = by_key.find(strip_transform_suffix(ch.bone_name));
    if (it == by_key.end()) {
      direct_channels.push_back(ch);
      continue;
    }
    const bool lower_body_output =
        lower_body_only && output_map_lower_body_bone(it->first);
    const bool face_output =
        face_output_layer && output_map_face_bone(it->first);
    const bool driven_by_selected_output =
        force_selected_output || full_output_layer || lower_body_output ||
        face_output;
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

  if (force_selected_output) {
    for (size_t i = 0; i < nodes.size(); ++i) {
      if (node_driven[i]) continue;
      if (is_constant_fret_hand_target_key(nodes[i].key)) {
        node_driven[i] = true;
      }
    }
  }

  dump_charbone_output_map(character, nodes, by_key, node_driven);

  if (!force_selected_output && !full_output_layer && !lower_body_only &&
      !face_output_layer) {
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
    // Only explicitly selected hand output and opt-in diagnostics write this
    // reconstructed CharBone output graph until the source runtime is known.
    target.local = update.desired_local;
  }

  if (!direct_channels.empty()) {
    apply_clip_pose_sampled_direct(direct_channels, weight, character,
                                   relative);
  }
  return true;
}

static void apply_hand_driver_output_layer(
    const std::vector<ClipChannel>& frame, Character& character, bool relative,
    const std::vector<CharClip::OutputBone>& source_output_bones) {
  if (hand_output_layer_disabled() || frame.empty() ||
      !output_bones_have_hand_driver_root(source_output_bones)) {
    return;
  }

  std::vector<CharClip::OutputBone> hand_output_bones;
  std::unordered_set<std::string> hand_keys;
  for (const auto& out : source_output_bones) {
    const std::string key = strip_transform_suffix(out.name);
    if (!is_hand_driver_output_key(key)) continue;
    if (!hand_keys.insert(key).second) continue;
    hand_output_bones.push_back(out);
  }
  if (hand_output_bones.empty()) return;

  std::vector<ClipChannel> hand_channels;
  hand_channels.reserve(frame.size());
  for (const auto& ch : frame) {
    if (hand_keys.find(strip_transform_suffix(ch.bone_name)) ==
        hand_keys.end()) {
      continue;
    }
    hand_channels.push_back(ch);
  }
  if (hand_channels.empty()) return;

  // Hand-driver clips carry their own CharBone output graph. The first-level
  // fingers are authored under bone_strum_hand/bone_fret_hand, while the live
  // mesh skeleton keeps them under bone_R-hand/bone_L-hand. CharIKHand mounts
  // the live hand onto that target after this clip pass, so the child rows must
  // stay in hand-local space here; bridging through the pre-IK parent applies
  // the offset a second time once the hand reaches the target.
  apply_clip_pose_output_layer(hand_channels, 1.0f, character, relative,
                               hand_output_bones, true);
}

static void apply_hand_driver_output_layers(
    const std::vector<ClipChannel>& frame, Character& character, bool relative,
    const std::vector<ClipChannelLayer>& layers) {
  if (hand_output_layer_disabled()) return;
  (void)frame;
  (void)relative;

  auto apply_group = [&](HandDriverOutputGroup group) {
    bool has_hand_driver_overlay = false;
    bool hand_relative = false;
    bool hand_relative_set = false;
    std::vector<ClipChannelLayer> hand_source_layers;
    for (const auto& layer : layers) {
      if (!layer.overlay_override || !layer.output_bones) continue;
      if (!output_bones_have_hand_driver_group_root(*layer.output_bones,
                                                    group)) {
        continue;
      }

      std::vector<ClipChannel> group_channels;
      group_channels.reserve(layer.channels.size());
      for (const auto& ch : layer.channels) {
        const std::string key = strip_transform_suffix(ch.bone_name);
        if (!is_hand_driver_output_key(key) ||
            !hand_driver_key_matches_group(key, group)) {
          continue;
        }
        group_channels.push_back(ch);
      }
      if (group_channels.empty()) continue;

      has_hand_driver_overlay = true;
      ClipChannelLayer group_layer = layer;
      group_layer.channels = std::move(group_channels);
      hand_source_layers.push_back(std::move(group_layer));
      if (!hand_relative_set) {
        hand_relative = layer.relative;
        hand_relative_set = true;
      } else if (hand_relative != layer.relative) {
        hand_relative = false;
      }
    }
    if (!has_hand_driver_overlay) return;

    std::vector<CharClip::OutputBone> hand_output_bones;
    std::unordered_set<std::string> hand_keys;
    for (const auto& layer : hand_source_layers) {
      if (!layer.output_bones) continue;
      for (const auto& out : *layer.output_bones) {
        const std::string key = strip_transform_suffix(out.name);
        if (!is_hand_driver_output_key(key) ||
            !hand_driver_key_matches_group(key, group)) {
          continue;
        }
        if (!hand_keys.insert(key).second) continue;
        hand_output_bones.push_back(out);
      }
    }
    if (hand_output_bones.empty()) return;

    const auto hand_frame = blend_channel_layers(hand_source_layers);
    if (hand_frame.empty()) return;

    std::vector<ClipChannel> hand_channels;
    hand_channels.reserve(hand_frame.size());
    for (const auto& ch : hand_frame) {
      if (hand_keys.find(strip_transform_suffix(ch.bone_name)) ==
          hand_keys.end()) {
        continue;
      }
      hand_channels.push_back(ch);
    }
    if (hand_channels.empty()) return;

    apply_clip_pose_output_layer(hand_channels, 1.0f, character, hand_relative,
                                 hand_output_bones, true);
  };

  apply_group(HandDriverOutputGroup::Strum);
  apply_group(HandDriverOutputGroup::Fret);
}

static void apply_char_hair(Character& character, float time_seconds) {
  if (character.hairs.empty()) return;
  for (const auto& hair : character.hairs) {
    log_char_hair_source_once(character, hair);
    SourceCharHairRuntime& state =
        ensure_source_char_hair_runtime(character, hair);
    const bool first_poll = state.last_time_seconds < 0.0f;
    const bool nonzero_delta =
        first_poll || time_seconds != state.last_time_seconds;
    if (state.reset > 0) {
      source_char_hair_do_reset(character, hair, state, state.reset);
    }

    int write_count = 0;
    if (nonzero_delta) {
      write_count = source_char_hair_simulate_loops(character, hair, state, 1,
                                                   60.0f, hair.inertia,
                                                   hair.friction);
    }
    state.last_time_seconds = time_seconds;

    if (debug_char_hair_enabled()) {
      std::fprintf(
          stderr,
          "[charhair-source-sim] character=%s hair=%s "
          "source=ihatecompvir-CharHair::Poll/DoReset/SimulateInternal "
          "runtimeWriteback=%d resolvedPointCollides=0 "
          "missingHookupObjPtrList=1 zeroTimeBodyAvailable=0 "
          "nonzeroDelta=%d firstPoll=%d time=%.4f\n",
          character.dir_name.c_str(), hair.name.c_str(), write_count,
          nonzero_delta ? 1 : 0, first_poll ? 1 : 0, time_seconds);
    }
  }
}

static std::array<float, 16> blend_world_rows(
    const std::array<float, 16>& a, const std::array<float, 16>& b,
    float weight) {
  weight = std::clamp(weight, 0.0f, 1.0f);
  std::array<float, 16> out{};
  for (size_t i = 0; i < out.size(); ++i) {
    out[i] = a[i] * (1.0f - weight) + b[i] * weight;
  }
  out[15] = 1.0f;
  normalize_mat3_rows(out);
  return out;
}

void apply_ik_midi_fret_target(Character& character,
                               const std::string& spot_name,
                               float time_seconds) {
  if (spot_name.empty()) return;
  std::array<float, 16> spot_world{};
  if (!transform_local_chain_world(character, spot_name, spot_world)) return;

  const float blend_seconds =
      std::clamp(env_float_or("GHOGX_IKMIDI_BLEND_SECONDS", 0.08f), 0.0f,
                 0.22f);
  for (const auto& ik : character.ik_midis) {
    if (ik.bone.empty()) continue;
    const int bone_i = find_bone_index(character, ik.bone);
    if (bone_i < 0 ||
        static_cast<size_t>(bone_i) >= character.bones.size()) {
      continue;
    }
    auto& bone = character.bones[static_cast<size_t>(bone_i)];
    RuntimeIKMidiState& state = character.runtime_ik_midi_states[ik.name];
    if (!state.initialized || state.active_spot != spot_name) {
      state.initialized = true;
      state.active_spot = spot_name;
      state.spot_start_time_seconds = time_seconds;
      state.start_world = character.bone_world_local_chain(bone.name);
    }

    const float age = std::max(0.0f, time_seconds - state.spot_start_time_seconds);
    const float weight = blend_seconds > 0.0f ? age / blend_seconds : 1.0f;
    const auto desired_world =
        blend_world_rows(state.start_world, spot_world, weight);
    std::array<float, 16> parent_world =
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    if (!bone.parent.empty()) {
      parent_world = character.bone_world_local_chain(bone.parent);
    }
    set_local_from_world(bone.local, desired_world, parent_world);
    if (debug_ik_enabled()) {
      std::fprintf(stderr,
                   "[ikmidi] %s bone=%s spot=%s age=%.3f blend=%.3f "
                   "weight=%.3f "
                   "target=[%.3f %.3f %.3f]\n",
                   ik.name.c_str(), bone.name.c_str(), spot_name.c_str(),
                   age, blend_seconds, std::clamp(weight, 0.0f, 1.0f),
                   desired_world[12], desired_world[13], desired_world[14]);
    }
  }
}

void clear_runtime_ik_weights(Character& character) {
  character.runtime_weight_props.clear();
  character.runtime_world_overrides.clear();
}

void set_runtime_ik_weight(Character& character, const std::string& weight_prop,
                           float weight) {
  if (weight_prop.empty()) return;
  character.runtime_weight_props[weight_prop] = std::clamp(weight, 0.0f, 1.0f);
}

void clear_runtime_trans_worlds(Character& character) {
  character.runtime_world_overrides.clear();
}

void apply_character_controllers(Character& character, float time_seconds,
                                 FaceFxEyeProperties* eye_props) {
  (void)time_seconds;
  if (eye_props) *eye_props = {};
  character.runtime_world_overrides.clear();
  log_character_controller_graph_once(character);
  std::vector<milo_scene::Xfm> bind_bones = character.bind_bone_local;
  if (bind_bones.size() != character.bones.size()) {
    bind_bones.clear();
    bind_bones.reserve(character.bones.size());
    for (const auto& b : character.bones) bind_bones.push_back(b.local);
  }
  apply_source_weight_setters(character, 0.0f);
  apply_source_ik_hands(character, bind_bones);
  apply_source_fore_twists(character);
  apply_char_hair(character, time_seconds);
  apply_source_upper_twists(character, bind_bones);
  apply_source_pos_constraints(character);
  apply_source_ik_rods(character);

  if (debug_face_enabled()) {
    for (const auto& b : character.bones) {
      if (b.name != "bone_L-upperlid.mesh" &&
          b.name != "bone_R-upperlid.mesh") {
        continue;
      }
      const auto world = character.bone_world_local_chain(b.name);
      std::fprintf(stderr,
                   "[face] upperlid %s parent=%s world=(%.3f %.3f %.3f) "
                   "local=(%.3f %.3f %.3f) "
                   "rows=[%.3f %.3f %.3f|%.3f %.3f %.3f|%.3f %.3f %.3f]\n",
                   b.name.c_str(), b.parent.c_str(), world[12], world[13],
                   world[14], b.local.pos[0], b.local.pos[1], b.local.pos[2],
                   b.local.rot[0][0], b.local.rot[0][1], b.local.rot[0][2],
                   b.local.rot[1][0], b.local.rot[1][1], b.local.rot[1][2],
                   b.local.rot[2][0], b.local.rot[2][1], b.local.rot[2][2]);
    }
    for (const auto& m : character.meshes) {
      if (m.name != "eye-L.mesh" && m.name != "eye-R.mesh") continue;
      const auto parent_world = character.bone_world_local_chain(m.parent);
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
}

void apply_clip_pose(const std::vector<ClipChannel>& channels, Character& character) {
  apply_clip_pose_weighted(channels, 1.0f, character);
}

static void apply_clip_pose_sampled_direct(
    const std::vector<ClipChannel>& channels, float weight, Character& character,
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

float blend_axis_angle(float a, float b, float t) {
  return wrap_ps2_angle(a + wrap_ps2_angle(b - a) * t);
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
      out.angle = blend_axis_angle(out.angle, rhs.angle, t);
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
  const bool body_channel =
      bone_name.find("hand") != std::string::npos ||
      bone_name.find("finger") != std::string::npos ||
      bone_name.find("thumb") != std::string::npos ||
      bone_name.find("fret") != std::string::npos ||
      bone_name.find("strum") != std::string::npos ||
      bone_name.find("clavicle") != std::string::npos ||
      bone_name.find("upperArm") != std::string::npos ||
      bone_name.find("foreArm") != std::string::npos ||
      bone_name.find("foreTwist") != std::string::npos ||
      bone_name.find("upperTwist") != std::string::npos;
  if (body_channel) return true;
  return debug_face_enabled() && is_face_quat_bone(bone_name);
}

void dump_lane_channel_value(const ClipChannel& ch) {
  switch (ch.type) {
    case ClipChannel::kPos:
      std::fprintf(stderr, " pos=(%.4f %.4f %.4f)", ch.pos[0], ch.pos[1],
                   ch.pos[2]);
      break;
    case ClipChannel::kScale:
      std::fprintf(stderr, " scale=(%.4f %.4f %.4f)", ch.scale[0],
                   ch.scale[1], ch.scale[2]);
      break;
    case ClipChannel::kQuat:
      std::fprintf(stderr, " quat=(%.4f %.4f %.4f %.4f)", ch.quat[0],
                   ch.quat[1], ch.quat[2], ch.quat[3]);
      break;
    case ClipChannel::kRotX:
    case ClipChannel::kRotY:
    case ClipChannel::kRotZ:
      std::fprintf(stderr, " angle=%.4f", ch.angle);
      break;
  }
}

bool is_axis_rot_channel(const ClipChannel& ch) {
  return ch.type == ClipChannel::kRotX || ch.type == ClipChannel::kRotY ||
         ch.type == ClipChannel::kRotZ;
}

bool is_quat_channel(const ClipChannel& ch) {
  return ch.type == ClipChannel::kQuat;
}

ClipChannel weighted_first_layer_channel(const ClipChannel& ch, float weight) {
  ClipChannel out = ch;
  if (is_axis_rot_channel(out)) {
    out.angle = wrap_ps2_angle(out.angle * weight);
  } else if (is_quat_channel(out)) {
    for (float& q : out.quat) q *= weight;
  }
  return out;
}

void accumulate_quat_channel(ClipChannel& out, const ClipChannel& rhs,
                             float weight) {
  float dot = 0.0f;
  for (int i = 0; i < 4; ++i) dot += out.quat[i] * rhs.quat[i];
  const float sign = dot < 0.0f ? -1.0f : 1.0f;
  for (int i = 0; i < 4; ++i) out.quat[i] += rhs.quat[i] * weight * sign;
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
                 "outputBones=%zu relative=%d overlay=%d\n",
                 i, name.c_str(), layer.weight, layer.channels.size(),
                 layer.output_bones ? layer.output_bones->size() : 0,
                 layer.relative ? 1 : 0,
                 layer.overlay_override ? 1 : 0);
    for (const auto& ch : layer.channels) {
      if (!lane_mixer_interesting_channel(ch.bone_name)) continue;
      const std::string key =
          std::string(channel_type_name(ch.type)) + ":" + ch.bone_name;
      owners[key].push_back(name);
      if (debug_face_enabled() && is_face_quat_bone(ch.bone_name)) {
        std::fprintf(stderr, "[lane-mix]     face %s %s",
                     name.c_str(), key.c_str());
        dump_lane_channel_value(ch);
        std::fprintf(stderr, "\n");
      }
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
        out.push_back(weighted_first_layer_channel(ch, layer_weight));
        continue;
      }

      AccumRef& acc = it->second;
      (void)layer.overlay_override;
      if (is_quat_channel(ch)) {
        // SLUS 0x00168320 accumulates quaternion rows into the shared
        // destination block with sign correction. quat_to_rot() normalizes the
        // accumulated row later when it becomes a transform.
        accumulate_quat_channel(out[acc.index], ch, layer_weight);
        acc.weight += layer_weight;
        continue;
      }
      if (is_axis_rot_channel(ch)) {
        // SLUS 0x00168320 accumulates scalar output rows into the shared
        // destination block. Duplicated forearm axis rows from body + hand
        // lanes must therefore add, not normalize by total source count.
        out[acc.index].angle =
            wrap_ps2_angle(out[acc.index].angle + ch.angle * layer_weight);
        acc.weight += layer_weight;
        continue;
      }
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
  auto collect_output_bones = [&](bool overlay_sources) {
    for (const auto& layer : layers) {
      if (layer.overlay_override != overlay_sources || !layer.output_bones) {
        continue;
      }
      for (const auto& out : *layer.output_bones) {
        const std::string key = strip_transform_suffix(out.name);
        if (!output_keys.insert(key).second) continue;
        output_bones.push_back(out);
      }
    }
  };
  collect_output_bones(false);
  if (output_bones.empty()) {
    collect_output_bones(true);
  }

  if (apply_clip_pose_output_layer(frame, 1.0f, character, relative,
                                   output_bones)) {
    apply_hand_driver_output_layers(frame, character, relative, layers);
    return;
  }
  apply_clip_pose_sampled_direct(frame, 1.0f, character, relative);
  apply_hand_driver_output_layers(frame, character, relative, layers);
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
  const uint32_t play_flags = char_clip_driver_masked_play_flags(clip, flags);
  const float resolved_blend =
      source_char_driver_resolve_blend_width(blend_width,
                                             source_driver_blend_width_);
  bool clip_already_playing = false;
  if (source_play_multiple_clips_) {
    for (const Layer& layer : layers_) {
      if (layer.clip == &clip) {
        clip_already_playing = true;
        break;
      }
    }
  }
  if (!source_char_driver_should_start_clip(source_play_multiple_clips_,
                                            clip_already_playing)) {
    return;
  }
  const bool no_blend =
      play_mode(clip, play_flags) == kCharPlayNoBlend ||
      resolved_blend <= 0.0f || layers_.empty();

  Layer next;
  next.clip = &clip;
  next.flags = play_flags;
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

void CharClipPlayer::set_source_driver_blend_width(float blend_width) {
  if (!std::isfinite(blend_width)) blend_width = 1.0f;
  source_driver_blend_width_ = std::max(0.0f, blend_width);
}

void CharClipPlayer::set_source_play_multiple_clips(bool play_multiple_clips) {
  source_play_multiple_clips_ = play_multiple_clips;
}

void CharClipPlayer::set_speed(float speed) {
  if (!std::isfinite(speed) || speed <= 0.0f) speed = 1.0f;
  for (auto& layer : layers_) {
    layer.speed = speed;
  }
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
  if (current && apply_clip_pose_output_layer(frame, weight, character, relative,
                                              current->output_bones)) {
    apply_hand_driver_output_layer(frame, character, relative,
                                   current->output_bones);
    return;
  }
  apply_clip_pose_sampled_direct(frame, weight, character, relative);
  if (current) {
    apply_hand_driver_output_layer(frame, character, relative,
                                   current->output_bones);
  }
}

std::vector<ClipChannel> CharClipPlayer::sampled_pose() const {
  if (layers_.empty()) return {};
  const Layer& current = layers_.back();
  const float current_weight = current_blend_weight();

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

float CharClipPlayer::current_blend_weight() const {
  if (layers_.empty()) return 0.0f;
  const Layer& current = layers_.back();
  if (current.blend_width <= 0.0f) return 1.0f;
  return std::clamp(current.blend_progress / current.blend_width, 0.0f,
                    1.0f);
}

bool CharClipPlayer::source_starved() const {
  if (layers_.empty()) return source_char_driver_starved(false, false, 0);
  return source_char_driver_starved(true, layers_.size() > 1,
                                    layers_.back().flags);
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
