// engine/src/milo_scene/milo_scene.cpp — see milo_scene.h for the byte layouts.

#include "milo_scene/milo_scene.h"

#include "ark_v3.h"
#include "milo.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

namespace ghogx::milo_scene {

namespace {

// Bounds-checked little-endian cursor over a single entry body.
struct Reader {
  const uint8_t* p;
  size_t n;
  size_t pos = 0;

  Reader(const uint8_t* data, size_t len) : p(data), n(len) {}

  void need(size_t k) const {
    if (pos + k > n) throw std::runtime_error("milo_scene: read past end of entry");
  }
  void skip(size_t k) { need(k); pos += k; }
  uint8_t u8() { need(1); return p[pos++]; }
  uint32_t u32() {
    need(4);
    uint32_t v;
    std::memcpy(&v, p + pos, 4);
    pos += 4;
    return v;
  }
  int32_t i32() { return static_cast<int32_t>(u32()); }
  uint16_t u16() {
    need(2);
    uint16_t v;
    std::memcpy(&v, p + pos, 2);
    pos += 2;
    return v;
  }
  float f32() {
    need(4);
    float v;
    std::memcpy(&v, p + pos, 4);
    pos += 4;
    return v;
  }
  // Length-prefixed UTF-8 string. Harmonix caps names well under this; a length
  // beyond the remaining bytes means we lost alignment, so reject it.
  std::string str() {
    uint32_t len = u32();
    if (len > n - pos || len > (1u << 20)) {
      throw std::runtime_error("milo_scene: implausible string length");
    }
    std::string s(reinterpret_cast<const char*>(p + pos), len);
    pos += len;
    return s;
  }

  std::string utf8_z() {
    const size_t start = pos;
    while (pos < n && p[pos] != 0) ++pos;
    if (pos >= n) {
      throw std::runtime_error("milo_scene: unterminated UTF-8 string");
    }
    if (pos - start > (1u << 20)) {
      throw std::runtime_error("milo_scene: implausible UTF-8 string length");
    }
    std::string s(reinterpret_cast<const char*>(p + start), pos - start);
    ++pos;
    return s;
  }

  // Read a Harmonix 3x4 matrix: 9 rotation floats (row-major) + 3 translation.
  Xfm matrix() {
    Xfm m;
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) m.rot[r][c] = f32();
    for (int c = 0; c < 3; ++c) m.pos[c] = f32();
    return m;
  }
};

// The 9-byte block that follows every object version int (Hmx::Object base
// metadata). GH2 Trans blocks then store the source RndTransformable fields as
// u32 constraint, empty target string, u8 preserve_scale before parent.
constexpr size_t kObjMeta = 9;
constexpr size_t kGh2MeshVertexSourceStride = 48;

float convert_fov_like_miloeditor(float fov, float aspect_ratio) {
  return std::atan(aspect_ratio * std::tan(0.5f * fov)) * 2.0f;
}

bool debug_worldcrowd_decode_enabled() {
#if defined(_WIN32)
  char* value = nullptr;
  size_t len = 0;
  if (_dupenv_s(&value, &len, "GHOGX_DEBUG_WORLDCROWD") != 0) return false;
  const bool enabled =
      value != nullptr && value[0] != '\0' && std::strcmp(value, "0") != 0;
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_WORLDCROWD");
  return value != nullptr && value[0] != '\0' && std::strcmp(value, "0") != 0;
#endif
}

bool debug_particle_decode_enabled() {
#if defined(_WIN32)
  char* value = nullptr;
  size_t len = 0;
  if (_dupenv_s(&value, &len, "GHOGX_DEBUG_PARTICLE_DECODE") != 0) return false;
  const bool enabled =
      value != nullptr && value[0] != '\0' && std::strcmp(value, "0") != 0;
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_PARTICLE_DECODE");
  return value != nullptr && value[0] != '\0' && std::strcmp(value, "0") != 0;
#endif
}

// Legacy spotlight parsing below still uses the observed empty ObjectFields byte
// count because that entry stores its Trans metadata in a nonstandard position.
void read_dtb_node(Reader& r);

void read_dtb_array_parent(Reader& r) {
  const uint16_t child_count = r.u16();
  (void)r.u32();  // id
  for (uint16_t i = 0; i < child_count; ++i) read_dtb_node(r);
}

void read_dtb_parent(Reader& r) {
  const bool has_tree = r.u8() != 0;
  if (!has_tree) return;
  read_dtb_array_parent(r);
}

void read_dtb_node(Reader& r) {
  const uint32_t type = r.u32();
  const SourceMiloEditorDtbNodePayloadPlan plan =
      source_milo_editor_dtb_node_payload_plan(static_cast<int32_t>(type));
  if (plan.reads_uint32) {
    (void)r.u32();
  } else if (plan.reads_float) {
    (void)r.f32();
  } else if (plan.reads_symbol) {
    (void)r.str();
  } else if (plan.reads_array_parent) {
    read_dtb_array_parent(r);
  }
}

void read_object_fields(Reader& r) {
  const uint32_t combined_revision = r.u32();
  const uint16_t revision = static_cast<uint16_t>(combined_revision & 0xffffu);
  (void)r.str();
  read_dtb_parent(r);
  if (revision > 0) (void)r.str();
}

bool is_environ_light_ref(std::string_view ref) {
  if (ref.empty()) return false;
  if (ref.size() >= 4 && ref.compare(ref.size() - 4, 4, ".lit") == 0)
    return true;
  for (char c : ref) {
    const unsigned char uc = static_cast<unsigned char>(c);
    if (!(std::isalnum(uc) || c == '_')) return false;
  }
  return true;
}

// Read the RndTrans portion that standalone Trans and transformable subclasses
// share, leaving the cursor just past the Trans parent string.
void read_trans_block(Reader& r, Xfm& local, Xfm& world, uint32_t& constraint,
                      std::string& target, bool& preserve_scale,
                      std::string& parent, bool read_object_meta,
                      uint16_t* out_revision = nullptr) {
  int32_t ver = r.i32();
  if (out_revision)
    *out_revision = static_cast<uint16_t>(
        static_cast<uint32_t>(ver) & 0xffffu);
  (void)ver;                 // = 9 for GH2 RndTrans
  if (read_object_meta) {
    r.skip(kObjMeta);        // Hmx::Object base metadata (standalone Trans)
  }
  local = r.matrix();        // matrix 1 (local)
  world = r.matrix();        // matrix 2 (world as stored)
  constraint = r.u32();      // RndTransformable::Constraint
  target = r.str();          // target name; empty in the GH2 venue props seen
  preserve_scale = r.u8() != 0;
  parent = r.str();          // parent / target name
}

struct TransFields {
  Xfm local;
  Xfm world;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  std::string parent;
  std::vector<std::string> legacy_children;
};

// MiloLib RndTrans.Read order:
// combined revision, optional Hmx::Object fields, local/world matrices, legacy
// child list for rev < 9, constraint, target, preserve-scale, parent.
// Standalone Trans entries carry Object fields; embedded Trans bases
// (Mesh/Group/etc.) do not.
TransFields read_trans_block(Reader& r,
                             bool standalone,
                             int32_t parent_dir_revision) {
  TransFields out;
  const uint32_t combined_revision = r.u32();
  const uint16_t ver = static_cast<uint16_t>(combined_revision & 0xffffu);
  const SourceRndTransLoadPlan plan =
      source_rndtrans_load_plan(ver, parent_dir_revision, standalone);
  if (plan.reads_object_fields) {
    read_object_fields(r);
  }
  out.local = r.matrix();    // matrix 1 (local)
  out.world = r.matrix();    // matrix 2 (world as stored)
  if (plan.reads_old_child_list) {
    const uint32_t trans_count = r.u32();
    for (uint32_t i = 0; i < trans_count; ++i) {
      if (plan.old_child_list_is_null_terminated_strings) {
        out.legacy_children.push_back(r.utf8_z());
      } else {
        out.legacy_children.push_back(r.str());
      }
    }
  }
  if (plan.reads_constraint) out.constraint = r.u32();
  if (plan.reads_target) out.target = r.str();
  if (plan.reads_preserve_scale) out.preserve_scale = r.u8() != 0;
  out.parent = r.str();
  return out;
}

void read_animatable_block(Reader& r,
                           std::vector<std::string>* anim_refs = nullptr) {
  const uint32_t combined_revision = r.u32();
  const uint16_t ver = static_cast<uint16_t>(combined_revision & 0xffffu);
  if (ver > 1) (void)r.f32();
  if (ver < 4) {
    if (ver > 2) (void)r.u8();
  } else {
    (void)r.u32();
    return;
  }
  if (ver < 1) {
    const uint32_t anim_entry_count = r.u32();
    for (uint32_t i = 0; i < anim_entry_count; ++i) {
      (void)r.str();
      (void)r.f32();
      (void)r.f32();
    }
    const uint32_t anim_count = r.u32();
    for (uint32_t i = 0; i < anim_count; ++i) {
      std::string ref = r.str();
      if (anim_refs) anim_refs->push_back(std::move(ref));
    }
  }
}

void read_drawable_block(Reader& r, int32_t parent_dir_revision,
                         bool& showing, float& draw_order,
                         std::vector<std::string>* drawable_children = nullptr) {
  const uint32_t combined_revision = r.u32();
  const uint16_t ver = static_cast<uint16_t>(combined_revision & 0xffffu);
  const SourceRndDrawableLoadPlan plan =
      source_rnddrawable_load_plan(ver, parent_dir_revision);
  if (!plan.accepted_revision) {
    throw std::runtime_error("milo_scene: RndDrawable revision outside source range");
  }
  if (plan.reads_showing) showing = r.u8() != 0;
  if (plan.reads_old_drawable_list) {
    const uint32_t drawable_count = r.u32();
    if (drawable_children) {
      drawable_children->clear();
      drawable_children->reserve(drawable_count);
    }
    for (uint32_t i = 0; i < drawable_count; ++i) {
      std::string ref;
      if (plan.old_list_is_null_terminated_strings) {
        ref = r.utf8_z();
      } else {
        ref = r.str();
      }
      if (drawable_children && !ref.empty())
        drawable_children->push_back(std::move(ref));
    }
  }
  if (plan.reads_sphere) r.skip(16);
  if (plan.reads_draw_order) draw_order = r.f32();
  if (plan.reads_clip_planes) {
    const uint32_t clip_plane_count = r.u32();
    for (uint32_t i = 0; i < clip_plane_count; ++i) r.str();
  }
}

void read_spotlight_trans_block(Reader& r, Xfm& local, Xfm& world,
                                std::string& parent) {
  int32_t ver = r.i32();
  (void)ver;                 // = 20 for GH2 PS2 Spotlight
  // Spotlight embeds its Trans block after a draw/property header. Raw GH2 PS2
  // spotlights place the local matrix at entry+0x2a and the resolved world at
  // entry+0x5a, followed by the normal 9-byte Trans metadata and parent string.
  r.skip(0x26);
  local = r.matrix();
  world = r.matrix();
  r.skip(kObjMeta);
  parent = r.str();
}

Xfm read_matrix_at(const std::vector<uint8_t>& body, size_t offset) {
  if (offset > body.size()) {
    throw std::runtime_error("milo_scene: matrix offset past end");
  }
  Reader r(body.data() + offset, body.size() - offset);
  return r.matrix();
}

float read_f32_at(const std::vector<uint8_t>& body, size_t offset) {
  if (offset + 4 > body.size()) {
    throw std::runtime_error("milo_scene: f32 offset past end");
  }
  float value = 0.0f;
  std::memcpy(&value, body.data() + offset, sizeof(value));
  return value;
}

uint32_t read_u32_at(const std::vector<uint8_t>& body, size_t offset) {
  if (offset + 4 > body.size()) {
    throw std::runtime_error("milo_scene: u32 offset past end");
  }
  uint32_t value = 0;
  std::memcpy(&value, body.data() + offset, sizeof(value));
  return value;
}

std::string read_string_at(const std::vector<uint8_t>& body, size_t& offset) {
  const uint32_t len = read_u32_at(body, offset);
  offset += 4;
  if (len > body.size() - offset || len > (1u << 20)) {
    throw std::runtime_error("milo_scene: implausible string length");
  }
  std::string s(reinterpret_cast<const char*>(body.data() + offset), len);
  offset += len;
  return s;
}

std::vector<std::string> scan_strings(const std::vector<uint8_t>& body) {
  std::vector<std::string> out;
  for (size_t o = 0; o + 4 <= body.size(); ++o) {
    uint32_t len;
    std::memcpy(&len, body.data() + o, 4);
    if (len == 0 || len > 96 || o + 4 + len > body.size()) continue;
    const char* s = reinterpret_cast<const char*>(body.data() + o + 4);
    bool printable = true;
    bool has_alpha = false;
    for (uint32_t k = 0; k < len; ++k) {
      char c = s[k];
      if (c < 0x20 || c >= 0x7f) {
        printable = false;
        break;
      }
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) has_alpha = true;
    }
    if (!printable || !has_alpha) continue;
    std::string value(s, len);
    if (out.empty() || out.back() != value) out.push_back(std::move(value));
    o += 3 + len;
  }
  return out;
}
struct ScannedString {
  size_t offset = 0;
  std::string value;
};

std::vector<ScannedString> scan_strings_with_offsets(
    const std::vector<uint8_t>& body) {
  std::vector<ScannedString> out;
  for (size_t o = 0; o + 4 <= body.size(); ++o) {
    uint32_t len;
    std::memcpy(&len, body.data() + o, 4);
    if (len == 0 || len > 96 || o + 4 + len > body.size()) continue;
    const char* s = reinterpret_cast<const char*>(body.data() + o + 4);
    bool printable = true;
    bool has_alpha = false;
    for (uint32_t k = 0; k < len; ++k) {
      char c = s[k];
      if (c < 0x20 || c >= 0x7f) {
        printable = false;
        break;
      }
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) has_alpha = true;
    }
    if (!printable || !has_alpha) continue;
    out.push_back({o, std::string(s, len)});
    o += 3 + len;
  }
  return out;
}

std::string lower_ascii(std::string_view s) {
  std::string out(s);
  for (char& c : out) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return out;
}

bool is_spotlight_target_mesh(std::string_view name) {
  constexpr size_t kSuffixLength = 12;
  if (name.size() <= kSuffixLength) return false;
  const std::string suffix =
      lower_ascii(name.substr(name.size() - kSuffixLength));
  return suffix == "_target.mesh" || suffix == ".target.mesh";
}

bool f32_in_unit_range(float v) {
  return std::isfinite(v) && v >= 0.0f && v <= 1.01f;
}

bool is_plausible_matrix_at(const std::vector<uint8_t>& body, size_t offset) {
  if (offset + 48 > body.size()) return false;
  float m[12];
  for (int i = 0; i < 12; ++i) {
    m[i] = read_f32_at(body, offset + static_cast<size_t>(i) * 4);
    if (!std::isfinite(m[i]) || std::fabs(m[i]) > 100000.0f) return false;
  }
  for (int r = 0; r < 3; ++r) {
    const float x = m[r * 3 + 0];
    const float y = m[r * 3 + 1];
    const float z = m[r * 3 + 2];
    const float mag = std::sqrt(x * x + y * y + z * z);
    if (mag < 0.01f || mag > 100.0f) return false;
  }
  return std::fabs(m[9]) < 50000.0f && std::fabs(m[10]) < 50000.0f &&
         std::fabs(m[11]) < 50000.0f;
}

bool is_plausible_group_parent(std::string_view ref) {
  if (ref.empty() || ref.size() > 96) return false;
  bool has_alpha = false;
  for (char c : ref) {
    const unsigned char uc = static_cast<unsigned char>(c);
    if (uc < 0x20 || uc >= 0x7f) return false;
    if (std::isalpha(uc)) has_alpha = true;
  }
  if (!has_alpha) return false;
  const std::string lower = lower_ascii(ref);
  static constexpr std::string_view kKnownSuffixes[] = {
      ".view", ".grp", ".mesh", ".trans", ".cam", ".lit", ".env", ".lst"};
  for (std::string_view suffix : kKnownSuffixes) {
    if (lower.size() >= suffix.size() &&
        lower.compare(lower.size() - suffix.size(), suffix.size(), suffix) ==
            0) {
      return true;
    }
  }
  return false;
}

struct GroupTransformDecode {
  size_t matrix_offset = 0;
  size_t after_trans_offset = 0;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  std::string parent;
};

std::optional<GroupTransformDecode> read_group_transform_tail(
    const std::vector<uint8_t>& body, size_t matrix_offset) {
  size_t cursor = matrix_offset + 96;
  if (cursor + 4 > body.size()) return std::nullopt;
  GroupTransformDecode out;
  out.matrix_offset = matrix_offset;
  out.constraint = read_u32_at(body, cursor);
  cursor += 4;
  try {
    out.target = read_string_at(body, cursor);
  } catch (const std::exception&) {
    return std::nullopt;
  }
  if (cursor >= body.size()) return std::nullopt;
  out.preserve_scale = body[cursor++] != 0;
  try {
    out.parent = read_string_at(body, cursor);
  } catch (const std::exception&) {
    return std::nullopt;
  }
  out.after_trans_offset = cursor;
  return out;
}

std::optional<GroupTransformDecode> find_group_transform_layout(
    const std::vector<uint8_t>& body) {
  int best_score = -1;
  std::optional<GroupTransformDecode> best;

  for (size_t m = 4; m + 96 + 4 + 4 + 1 + 4 <= body.size(); ++m) {
    if (!is_plausible_matrix_at(body, m) ||
        !is_plausible_matrix_at(body, m + 48)) {
      continue;
    }
    auto decoded = read_group_transform_tail(body, m);
    if (!decoded) continue;
    const bool root_parent =
        decoded->parent.empty() && (m == 0x1d || m == 4 + kObjMeta);
    if (!root_parent && !is_plausible_group_parent(decoded->parent)) continue;

    int score = root_parent ? 1 : 0;
    if (!decoded->target.empty()) score += 3;
    if (decoded->parent.find(".view") != std::string::npos) score += 20;
    if (decoded->parent.find(".grp") != std::string::npos) score += 12;
    if (m == 0x1d) score += 8;
    if (m == 4 + kObjMeta) score += 4;
    if (m < 0x40) score += 2;
    if (score > best_score) {
      best_score = score;
      best = std::move(decoded);
    }
  }

  return best;
}

bool decode_group_transform(const std::vector<uint8_t>& body, GroupObj& group,
                            size_t* after_trans_offset = nullptr) {
  const auto layout = find_group_transform_layout(body);
  if (!layout) return false;
  group.local = read_matrix_at(body, layout->matrix_offset);
  group.world_stored = read_matrix_at(body, layout->matrix_offset + 48);
  group.constraint = layout->constraint;
  group.target = layout->target;
  group.preserve_scale = layout->preserve_scale;
  group.parent = layout->parent;
  group.has_transform = true;
  if (after_trans_offset) *after_trans_offset = layout->after_trans_offset;
  return true;
}

uint16_t low_revision(uint32_t combined_revision) {
  return static_cast<uint16_t>(combined_revision & 0xffffu);
}

uint16_t read_rnd_animatable_source_layout(Reader& r) {
  const uint16_t anim_revision = low_revision(r.u32());
  if (anim_revision == 0 || anim_revision > 4) {
    throw std::runtime_error("milo_scene: unsupported RndAnimatable revision");
  }
  if (anim_revision > 1) (void)r.f32();
  if (anim_revision > 3) {
    (void)r.u32();
  } else if (anim_revision > 2) {
    (void)r.u8();
  }
  return anim_revision;
}

bool parse_group_source_layout(const std::vector<uint8_t>& body,
                               uint16_t group_revision,
                               size_t after_trans_offset,
                               GroupObj& group) {
  try {
    Reader r(body.data(), body.size());
    r.pos = after_trans_offset;

    const uint16_t draw_revision = low_revision(r.u32());
    group.showing = r.u8() != 0;
    if (draw_revision < 2) {
      const uint32_t drawable_count = r.u32();
      if (drawable_count > 1024) return false;
      for (uint32_t i = 0; i < drawable_count; ++i) (void)r.str();
    }
    if (draw_revision > 0) r.skip(16);
    if (draw_revision > 2) group.draw_order = r.f32();
    if (draw_revision >= 4) {
      const uint32_t clip_plane_count = r.u32();
      if (clip_plane_count > 6) return false;
      for (uint32_t i = 0; i < clip_plane_count; ++i) (void)r.str();
    }

    std::vector<std::string> objects;
    if (group_revision > 10) {
      const uint32_t object_count = r.u32();
      if (object_count > 2048) return false;
      objects.reserve(object_count);
      for (uint32_t i = 0; i < object_count; ++i) {
        objects.push_back(r.str());
      }

      if (group_revision < 16) group.environment_ref = r.str();
      if (group_revision > 12) group.draw_only = r.str();
    } else if (group_revision == 4) {
      (void)r.u32();
      const uint32_t object_count = r.u32();
      if (object_count > 2048) return false;
      objects.reserve(object_count);
      for (uint32_t i = 0; i < object_count; ++i) {
        objects.push_back(r.str());
      }
      (void)r.str();
      (void)r.u32();
      (void)r.u32();
    }

    if (group_revision > 11 && group_revision < 16) {
      group.lod = r.str();
      group.lod_screen_size = r.f32();
    } else if (group_revision == 7) {
      (void)r.str();
      (void)r.f32();
      (void)r.f32();
    }
    if (group_revision > 13) group.sort_in_world = r.u8() != 0;

    group.children.clear();
    for (auto& object : objects) {
      if (!object.empty()) group.children.push_back(std::move(object));
    }
    group.decoded = true;
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

bool decode_group_source_order(const std::vector<uint8_t>& body,
                               GroupObj& group) {
  try {
    Reader r(body.data(), body.size());
    const uint16_t group_revision = low_revision(r.u32());
    if (group_revision <= 7 || group_revision > 14) return false;

    GroupObj decoded;
    decoded.name = group.name;
    r.skip(kObjMeta);
    read_rnd_animatable_source_layout(r);
    read_trans_block(r, decoded.local, decoded.world_stored,
                     decoded.constraint, decoded.target,
                     decoded.preserve_scale, decoded.parent,
                     false);
    decoded.has_transform = true;
    if (!parse_group_source_layout(body, group_revision, r.pos, decoded)) {
      return false;
    }
    decoded.source_order_decoded = true;
    group = std::move(decoded);
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

bool name_has_suffix(std::string_view name, std::string_view suffix) {
  return name.size() >= suffix.size() &&
         name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
}

const GroupObj* find_group_obj(const Scene& scene, const std::string& name) {
  for (const GroupObj& group : scene.groups) {
    if (group.name == name) return &group;
  }
  return nullptr;
}

bool scene_has_mesh(const Scene& scene, const std::string& name) {
  for (const MeshObj& mesh : scene.meshes) {
    if (mesh.name == name) return true;
  }
  return false;
}

void append_group_draw_order(const Scene& scene, const GroupObj& group,
                             std::unordered_set<std::string>& visiting_groups,
                             std::unordered_set<std::string>& emitted_meshes,
                             std::vector<std::string>& order) {
  if (!group.showing) return;
  if (!visiting_groups.insert(group.name).second) return;
  const auto append_mesh = [&](auto&& self, const std::string& name) -> void {
    const auto mesh_it = std::find_if(
        scene.meshes.begin(), scene.meshes.end(),
        [&](const MeshObj& mesh) { return mesh.name == name; });
    if (mesh_it == scene.meshes.end()) return;
    if (emitted_meshes.insert(name).second) order.push_back(name);
    for (const auto& drawable_child : mesh_it->drawable_children)
      self(self, drawable_child);
  };
  const auto visit_child = [&](const std::string& child) {
    if (name_has_suffix(child, ".mesh") && scene_has_mesh(scene, child)) {
      append_mesh(append_mesh, child);
    } else if (const GroupObj* child_group = find_group_obj(scene, child)) {
      append_group_draw_order(scene, *child_group, visiting_groups,
                              emitted_meshes, order);
    }
  };
  if (!group.draw_only.empty()) {
    visit_child(group.draw_only);
  } else {
    for (const std::string& child : group.children) visit_child(child);
  }
  visiting_groups.erase(group.name);
}

void rebuild_group_authored_draw_order(Scene& scene) {
  scene.draw_order.clear();
  scene.grouped_meshes.clear();
  if (scene.groups.empty()) return;

  std::unordered_set<std::string> referenced_groups;
  std::unordered_set<std::string> grouped_mesh_set;
  for (const GroupObj& group : scene.groups) {
    for (const std::string& child : group.children) {
      if (find_group_obj(scene, child)) referenced_groups.insert(child);
      if (name_has_suffix(child, ".mesh") && scene_has_mesh(scene, child)) {
        grouped_mesh_set.insert(child);
      }
    }
    if (!group.draw_only.empty() && name_has_suffix(group.draw_only, ".mesh") &&
        scene_has_mesh(scene, group.draw_only)) {
      grouped_mesh_set.insert(group.draw_only);
    }
  }
  scene.grouped_meshes.assign(grouped_mesh_set.begin(), grouped_mesh_set.end());
  std::sort(scene.grouped_meshes.begin(), scene.grouped_meshes.end());

  struct DrawRoot {
    float order = 0.0f;
    size_t dir_index = 0;
    const GroupObj* group = nullptr;
    const MeshObj* mesh = nullptr;
  };
  std::unordered_set<std::string> emitted_meshes;
  std::vector<DrawRoot> roots;
  roots.reserve(scene.groups.size() + scene.meshes.size());
  for (const GroupObj& group : scene.groups) {
    if (referenced_groups.find(group.name) == referenced_groups.end()) {
      roots.push_back(DrawRoot{group.draw_order, group.dir_index, &group,
                               nullptr});
    }
  }
  for (const MeshObj& mesh : scene.meshes) {
    if (grouped_mesh_set.find(mesh.name) == grouped_mesh_set.end()) {
      roots.push_back(
          DrawRoot{mesh.draw_order, mesh.dir_index, nullptr, &mesh});
    }
  }
  std::stable_sort(roots.begin(), roots.end(),
                   [](const DrawRoot& a, const DrawRoot& b) {
                     if (a.order != b.order) return a.order < b.order;
                     return a.dir_index < b.dir_index;
                   });
  for (const DrawRoot& root : roots) {
    if (root.group) {
      std::unordered_set<std::string> visiting;
      append_group_draw_order(scene, *root.group, visiting, emitted_meshes,
                              scene.draw_order);
    } else if (root.mesh && root.mesh->showing &&
               emitted_meshes.insert(root.mesh->name).second) {
      scene.draw_order.push_back(root.mesh->name);
    }
  }
}

bool read_spotlight_default_state(const std::vector<uint8_t>& body,
                                  const std::vector<ScannedString>& strings,
                                  float color[3], float& intensity) {
  const auto parent_it = std::find_if(strings.begin(), strings.end(),
                                      [](const ScannedString& s) {
                                        return s.value.rfind("_RndDir") !=
                                               std::string::npos;
                                      });
  if (parent_it == strings.end()) return false;
  const size_t after_parent =
      parent_it->offset + 4 + static_cast<size_t>(parent_it->value.size());
  const auto first_payload_it =
      std::find_if(strings.begin(), strings.end(),
                   [&](const ScannedString& s) {
                     return s.offset >= after_parent &&
                            s.value.rfind(".mat") == std::string::npos;
                   });
  if (first_payload_it == strings.end()) return false;
  const std::string lower = lower_ascii(first_payload_it->value);
  if (lower.rfind(".mesh") != std::string::npos ||
      lower.rfind(".mat") != std::string::npos) {
    return false;
  }
  const bool group_payload =
      lower.size() >= 4 && lower.compare(lower.size() - 4, 4, ".grp") == 0;
  const size_t string_end = first_payload_it->offset + 4 +
                            static_cast<size_t>(first_payload_it->value.size());
  const size_t color_offset = string_end + (group_payload ? 8 : 4);
  if (color_offset + 16 > body.size()) return false;
  const float r = read_f32_at(body, color_offset);
  const float g = read_f32_at(body, color_offset + 4);
  const float b = read_f32_at(body, color_offset + 8);
  const float a = read_f32_at(body, color_offset + 12);
  if (!f32_in_unit_range(r) || !f32_in_unit_range(g) ||
      !f32_in_unit_range(b) || !f32_in_unit_range(a)) {
    return false;
  }
  if (r + g + b <= 0.001f || a <= 0.001f) return false;
  color[0] = r;
  color[1] = g;
  color[2] = b;
  intensity = a;
  return true;
}

void add_unique_ref(std::vector<std::string>& refs, const std::string& ref) {
  if (ref.empty()) return;
  if (std::find(refs.begin(), refs.end(), ref) == refs.end())
    refs.push_back(ref);
}

bool ref_has_suffix(std::string_view ref, std::string_view suffix) {
  return ref.size() >= suffix.size() &&
         lower_ascii(ref.substr(ref.size() - suffix.size())) ==
             std::string(suffix);
}

void assign_scanned_spotlight_material(SpotlightObj& s,
                                       const std::string& ref) {
  const std::string lower = lower_ascii(ref);
  if (lower.find("spot_circle") != std::string::npos) {
    s.circle_material = ref;
  } else if (lower.find("lens") != std::string::npos) {
    s.lens_material = ref;
  } else if (s.material.empty()) {
    s.material = ref;
  }
}

void assign_scanned_spotlight_mesh(SpotlightObj& s, const std::string& ref,
                                   bool& has_authored_target) {
  const std::string lower = lower_ascii(ref);
  if (lower.rfind("spot_circle", 0) == 0) s.circle_mesh = ref;
  const bool authored_target = is_spotlight_target_mesh(ref);
  has_authored_target = has_authored_target || authored_target;
  if (authored_target || s.target.empty()) s.target = ref;
  if (!authored_target) add_unique_ref(s.instance_meshes, ref);
}

uint16_t read_rnd_drawable_source_layout(Reader& r, bool& showing,
                                         float& draw_order) {
  const uint16_t draw_revision = low_revision(r.u32());
  if (draw_revision > 4) {
    throw std::runtime_error("milo_scene: unsupported RndDrawable revision");
  }
  showing = r.u8() != 0;
  if (draw_revision < 2) {
    const uint32_t drawable_count = r.u32();
    if (drawable_count > 2048) {
      throw std::runtime_error("milo_scene: implausible Drawable refs");
    }
    for (uint32_t i = 0; i < drawable_count; ++i) (void)r.str();
  }
  if (draw_revision > 0) r.skip(16);
  if (draw_revision > 2) draw_order = r.f32();
  if (draw_revision >= 4) {
    const uint32_t clip_plane_count = r.u32();
    if (clip_plane_count > 6) {
      throw std::runtime_error("milo_scene: implausible Drawable clip planes");
    }
    for (uint32_t i = 0; i < clip_plane_count; ++i) (void)r.str();
  }
  return draw_revision;
}

std::string read_source_ref_list_item(Reader& r) { return r.str(); }

std::vector<std::string> read_source_ref_list(Reader& r, uint32_t max_count,
                                              const char* label) {
  const int32_t signed_count = r.i32();
  if (signed_count < 0 || static_cast<uint32_t>(signed_count) > max_count) {
    throw std::runtime_error(std::string("milo_scene: implausible ") + label +
                             " ref count");
  }
  std::vector<std::string> refs;
  refs.reserve(static_cast<size_t>(signed_count));
  for (int32_t i = 0; i < signed_count; ++i) {
    refs.push_back(read_source_ref_list_item(r));
  }
  return refs;
}

void read_spotlight_beam_def_source_order(Reader& r, uint16_t revision,
                                          SpotlightObj& s) {
  s.beam_is_cone = r.u8() != 0;
  s.beam_length = r.f32();
  s.beam_bottom_radius = r.f32();
  s.beam_top_radius = r.f32();
  s.beam_top_side_border = r.f32();
  s.beam_bottom_side_border = r.f32();
  s.beam_bottom_border = r.f32();
  const std::string beam_material = r.str();
  if (!beam_material.empty()) s.material = beam_material;
  if (revision == 0x12) {
    throw std::runtime_error("milo_scene: unsupported Spotlight rev18 string");
  }
  s.beam_offset = r.f32();
  if (revision < 10) {
    (void)r.f32();
    (void)r.f32();
    (void)r.f32();
    (void)r.f32();
  }
  s.beam_target_offset[0] = r.f32();
  s.beam_target_offset[1] = r.f32();
  if (revision > 0x14) {
    (void)r.f32();
    (void)r.u32();
  }
  if (revision > 0x17) (void)r.f32();
  if (revision > 0x1a) (void)r.u32();
  if (revision > 0x18) {
    (void)read_source_ref_list(r, 64, "Spotlight beam cutout");
  }
  if (revision > 0x1f) {
    (void)r.u32();
    (void)r.u32();
  }
}

bool decode_spotlight_source_order(const std::vector<uint8_t>& body,
                                   SpotlightObj& out) {
  SpotlightObj s;
  s.name = out.name;
  try {
    Reader r(body.data(), body.size());
    const uint16_t revision = low_revision(r.u32());
    s.revision = revision;
    if (revision != 20) {
      throw std::runtime_error("milo_scene: unsupported Spotlight revision");
    }
    r.skip(kObjMeta);
    s.draw_revision =
        read_rnd_drawable_source_layout(r, s.showing, s.draw_order);
    read_trans_block(r, s.local, s.world_stored, s.constraint,
                     s.trans_target, s.preserve_scale, s.parent, false,
                     &s.trans_revision);
    s.has_transform = true;

    s.spot_scale = r.f32();
    s.spot_height = r.f32();
    const uint32_t beam_count = r.u32();
    if (beam_count > 64) {
      throw std::runtime_error(
          "milo_scene: implausible Spotlight beam count=" +
          std::to_string(beam_count) + " draw_rev=" +
          std::to_string(s.draw_revision) + " trans_rev=" +
          std::to_string(s.trans_revision));
    }
    if (beam_count == 0) {
      s.beam_length = 0.0f;
    }
    for (uint32_t i = 0; i < beam_count; ++i) {
      if (i == 0) {
        read_spotlight_beam_def_source_order(r, revision, s);
      } else {
        SpotlightObj ignored;
        read_spotlight_beam_def_source_order(r, revision, ignored);
      }
    }

    s.light_can_group = r.str();
    s.group = s.light_can_group;
    s.target = r.str();
    s.light_can_offset = r.f32();
    s.default_color[0] = r.f32();
    s.default_color[1] = r.f32();
    s.default_color[2] = r.f32();
    (void)r.f32();  // Hmx::Color32 load alpha; Spotlight forces it opaque.
    s.default_intensity = r.f32();
    s.has_default_state = true;
    s.disc_material = r.str();
    if (!s.disc_material.empty()) s.circle_material = s.disc_material;
    s.damping_constant = r.f32();
    (void)r.str();  // pre-rev33 legacy symbol.

    s.flare_material = r.str();
    if (!s.flare_material.empty() && s.material.empty()) {
      s.material = s.flare_material;
    }
    s.flare_size[0] = r.f32();
    s.flare_size[1] = r.f32();
    s.flare_range[0] = r.f32();
    s.flare_range[1] = r.f32();
    s.flare_steps = r.i32();
    s.flare_offset = r.f32();
    s.flare_enabled = r.u8() != 0;
    s.flare_visibility_test = r.u8() != 0;
    s.lens_size = r.f32();
    s.lens_offset = r.f32();
    s.lens_material = r.str();

    for (const auto& ref : read_source_ref_list(r, 2048,
                                                "Spotlight additional object")) {
      if (ref_has_suffix(ref, ".mesh")) {
        const std::string lower = lower_ascii(ref);
        if (lower.rfind("spot_circle", 0) == 0) s.circle_mesh = ref;
        add_unique_ref(s.instance_meshes, ref);
      }
    }
    s.target_shadow = r.u8() != 0;
    s.animate_color_from_preset = r.u8() != 0;
    s.animate_orientation_from_preset = s.animate_color_from_preset;
    if (r.pos != r.n) {
      throw std::runtime_error(
          "milo_scene: Spotlight source reader did not consume EOF");
    }
    s.source_order_decoded = true;
    s.decoded = true;
    out = std::move(s);
    return true;
  } catch (const std::exception& ex) {
    out.error = ex.what();
    return false;
  }
}

}  // namespace

PanelDirConfig decode_panel_dir_config(const std::vector<uint8_t>& body) {
  PanelDirConfig out;
  if (body.size() < 8) return out;
  uint32_t panel_combined = 0;
  uint32_t rnd_combined = 0;
  std::memcpy(&panel_combined, body.data(), sizeof(panel_combined));
  std::memcpy(&rnd_combined, body.data() + 4, sizeof(rnd_combined));
  out.panel_revision = low_revision(panel_combined);
  out.rnd_dir_revision = low_revision(rnd_combined);

  // GH2's PanelDir rev 2 contains a RndDir rev 8. The exact source tail is
  // mEnv, mTestEvent, two legacy RndDir symbols, mCam, and PanelDir::testEvent.
  // The inherited ObjectDir block is variable-sized, so identify the unique
  // six-symbol suffix and require it to consume the root body exactly.
  if (out.panel_revision != 2 || out.rnd_dir_revision != 8) return out;
  const auto sane_symbol = [](const std::string& value) {
    if (value.size() > 256) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
      return c >= 32 && c < 127;
    });
  };
  for (size_t start = 8; start + 24 <= body.size(); ++start) {
    try {
      Reader r(body.data() + start, body.size() - start);
      std::array<std::string, 6> fields;
      for (std::string& field : fields) field = r.str();
      if (r.pos != r.n) continue;
      if (!std::all_of(fields.begin(), fields.end(), sane_symbol)) continue;
      const bool environment_valid =
          fields[0].empty() ||
          (fields[0].size() >= 4 &&
           fields[0].compare(fields[0].size() - 4, 4, ".env") == 0);
      const bool camera_valid =
          fields[4].empty() ||
          (fields[4].size() >= 4 &&
           fields[4].compare(fields[4].size() - 4, 4, ".cam") == 0);
      if (!environment_valid || !camera_valid || fields[5].empty()) continue;
      out.environment = std::move(fields[0]);
      out.camera = std::move(fields[4]);
      out.enter_event = std::move(fields[5]);
      out.valid = true;
      return out;
    } catch (const std::exception&) {
    }
  }
  return out;
}

std::string legacy_directory_root_view_name(const std::string& milo_path) {
  const size_t slash = milo_path.find_last_of("/\\");
  std::string leaf = milo_path.substr(
      slash == std::string::npos ? 0 : slash + 1);
  for (const std::string_view suffix : {std::string_view{".rnd_ps2"},
                                        std::string_view{".milo_ps2"},
                                        std::string_view{".rnd"},
                                        std::string_view{".milo"}}) {
    if (name_has_suffix(leaf, suffix)) {
      leaf.resize(leaf.size() - suffix.size());
      break;
    }
  }
  return leaf.empty() ? std::string{} : leaf + ".view";
}

EnvAnimObj decode_env_anim(const std::string& entry_name,
                           const std::vector<uint8_t>& body) {
  EnvAnimObj out;
  out.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    out.revision = low_revision(r.u32());
    if (out.revision != 4)
      throw std::runtime_error("unsupported EnvAnim revision");
    r.skip(kObjMeta);  // rev 4 loads Hmx::Object before RndAnimatable.
    out.anim_revision = low_revision(r.u32());
    if (out.anim_revision != 4)
      throw std::runtime_error("unsupported EnvAnim RndAnimatable revision");
    out.frame = r.f32();
    out.rate = r.u32();
    out.environment = r.str();

    const auto read_color_keys = [&](std::vector<EnvAnimColorKey>& keys) {
      const uint32_t count = r.u32();
      if (count > 4096) throw std::runtime_error("implausible EnvAnim color key count");
      keys.reserve(count);
      for (uint32_t i = 0; i < count; ++i) {
        EnvAnimColorKey key;
        for (float& component : key.value) component = r.f32();
        key.frame = r.f32();
        keys.push_back(key);
      }
    };
    read_color_keys(out.ambient_color_keys);
    out.keys_owner = r.str();
    read_color_keys(out.fog_color_keys);
    const uint32_t range_count = r.u32();
    if (range_count > 4096)
      throw std::runtime_error("implausible EnvAnim fog-range key count");
    out.fog_range_keys.reserve(range_count);
    for (uint32_t i = 0; i < range_count; ++i) {
      EnvAnimRangeKey key;
      key.value[0] = r.f32();
      key.value[1] = r.f32();
      key.frame = r.f32();
      out.fog_range_keys.push_back(key);
    }
    if (r.pos != r.n) throw std::runtime_error("trailing EnvAnim bytes");
    out.decoded = true;
  } catch (const std::exception& ex) {
    out.error = ex.what();
  }
  return out;
}

ScreenMaskObj decode_screen_mask(const std::string& entry_name,
                                 const std::vector<uint8_t>& body) {
  ScreenMaskObj out;
  out.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    out.revision = low_revision(r.u32());
    if (out.revision > 2)
      throw std::runtime_error("unsupported ScreenMask revision");
    r.skip(kObjMeta);
    out.drawable_revision = low_revision(r.u32());
    if (out.drawable_revision > 3)
      throw std::runtime_error("unsupported ScreenMask RndDrawable revision");
    out.showing = r.u8() != 0;
    if (out.drawable_revision < 2) {
      const uint32_t legacy_count = r.u32();
      for (uint32_t i = 0; i < legacy_count; ++i) (void)r.str();
    }
    if (out.drawable_revision > 0)
      for (float& component : out.sphere) component = r.f32();
    if (out.drawable_revision > 2) out.draw_order = r.f32();
    out.material = r.str();
    for (float& component : out.color) component = r.f32();
    if (out.revision > 0)
      for (float& component : out.rect) component = r.f32();
    if (out.revision > 1) out.use_camera_rect = r.u8() != 0;
    if (r.pos != r.n) throw std::runtime_error("trailing ScreenMask bytes");
    out.decoded = true;
  } catch (const std::exception& ex) {
    out.error = ex.what();
  }
  return out;
}

SourceMiloEditorDtbNodePayloadPlan
source_milo_editor_dtb_node_payload_plan(int32_t node_type) {
  SourceMiloEditorDtbNodePayloadPlan plan;
  plan.node_type = node_type;
  switch (node_type) {
    case 0x00:
      plan.node_type_name = "Int";
      plan.known_node_type = true;
      plan.reads_uint32 = true;
      break;
    case 0x01:
      plan.node_type_name = "Float";
      plan.known_node_type = true;
      plan.reads_float = true;
      break;
    case 0x02:
      plan.node_type_name = "Variable";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x03:
      plan.node_type_name = "Func";
      plan.known_node_type = true;
      plan.consumes_no_payload = true;
      break;
    case 0x04:
      plan.node_type_name = "Object";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x05:
      plan.node_type_name = "Symbol";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x06:
      plan.node_type_name = "Unhandled";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x07:
      plan.node_type_name = "IfDef";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x08:
      plan.node_type_name = "Else";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x09:
      plan.node_type_name = "EndIf";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x10:
      plan.node_type_name = "Array";
      plan.known_node_type = true;
      plan.reads_array_parent = true;
      break;
    case 0x11:
      plan.node_type_name = "Command";
      plan.known_node_type = true;
      plan.reads_array_parent = true;
      break;
    case 0x12:
      plan.node_type_name = "String";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x13:
      plan.node_type_name = "Property";
      plan.known_node_type = true;
      plan.reads_array_parent = true;
      break;
    case 0x20:
      plan.node_type_name = "Define";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x21:
      plan.node_type_name = "Include";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x22:
      plan.node_type_name = "Merge";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x23:
      plan.node_type_name = "IfNDef";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x24:
      plan.node_type_name = "Autorun";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    case 0x25:
      plan.node_type_name = "Undef";
      plan.known_node_type = true;
      plan.reads_symbol = true;
      break;
    default:
      plan.node_type_name = "Unknown";
      plan.consumes_no_payload = true;
      break;
  }
  return plan;
}

SourceRndTransLoadPlan source_rndtrans_load_plan(
    int32_t revision,
    int32_t parent_revision,
    bool standalone) {
  SourceRndTransLoadPlan plan;
  plan.revision = revision;
  plan.parent_revision = parent_revision;
  plan.standalone = standalone;
  plan.reads_object_fields = standalone;
  plan.reads_old_child_list = revision < 9;
  plan.old_child_list_is_null_terminated_strings =
      plan.reads_old_child_list && parent_revision <= 6;
  plan.old_child_list_is_symbols =
      plan.reads_old_child_list && parent_revision > 6;
  plan.reads_constraint = revision > 6;
  plan.reads_target = revision > 5;
  plan.reads_preserve_scale = revision > 6;
  return plan;
}

SourceMiloEditorRndTransNewPlan source_milo_editor_rndtrans_new_plan(
    int32_t revision,
    int32_t alt_revision) {
  SourceMiloEditorRndTransNewPlan plan;
  plan.revision = revision;
  plan.alt_revision = alt_revision;
  plan.local_xfm = Xfm{};
  plan.world_xfm = Xfm{};
  return plan;
}

SourceRndTransformableCppLoadPlan source_rndtransformable_cpp_load_plan(
    int32_t revision,
    bool loading_proxy_from_disk,
    bool class_is_static) {
  SourceRndTransformableCppLoadPlan plan;
  plan.revision = revision;
  plan.loading_proxy_from_disk = loading_proxy_from_disk;
  plan.class_is_static = class_is_static;
  plan.accepted_revision = revision >= 0 && revision <= 9;
  if (!plan.accepted_revision) return plan;

  plan.reads_object_fields_for_static_class = class_is_static;
  plan.reads_proxy_temp_transforms = loading_proxy_from_disk;
  plan.reads_stored_local_world = !loading_proxy_from_disk;
  plan.reads_old_child_list = revision < 9;
  plan.old_child_list_sets_parent = revision < 9;
  plan.rev6_reads_constraint = revision == 6;
  plan.rev6_preserve_scale_from_target_world = revision == 6;
  plan.reads_legacy_assert_vector = revision != 0 && revision < 7;
  plan.reads_legacy_bool = revision >= 2 && revision <= 4;
  plan.reads_sphere = revision == 6 || revision == 7;
  plan.may_set_drawable_sphere = plan.reads_sphere;
  plan.reads_target = revision > 5;
  plan.proxy_loads_target_ref = plan.reads_target && loading_proxy_from_disk;
  plan.reads_preserve_scale = revision > 6;
  plan.reads_parent = revision > 6;
  plan.proxy_loads_parent_ref = revision > 8 && loading_proxy_from_disk;
  plan.parent_sets_trans_parent =
      revision > 8 ? !loading_proxy_from_disk : revision > 6;
  plan.rev7_8_parent_sets_constraint_parent_world =
      revision > 6 && revision <= 8;
  plan.rev6_parent_from_target_when_constraint_parent_world = revision == 6;
  return plan;
}

SourceRndTransformableDefaultState source_rndtransformable_default_state() {
  return SourceRndTransformableDefaultState{};
}

SourceRndTransformableSavePlan source_rndtransformable_save_plan() {
  return SourceRndTransformableSavePlan{};
}

SourceRndTransformableDirtyPlan source_rndtransformable_set_dirty_plan(
    bool cache_already_dirty,
    bool has_children) {
  SourceRndTransformableDirtyPlan plan;
  plan.cache_already_dirty = cache_already_dirty;
  plan.set_dirty_force = !cache_already_dirty;
  plan.sets_last_bit = !cache_already_dirty;
  plan.propagates_to_children = !cache_already_dirty && has_children;
  return plan;
}

SourceRndTransformableParentPlan source_rndtransformable_set_parent_plan(
    bool same_parent,
    bool preserve_world,
    bool had_old_parent,
    bool has_new_parent) {
  SourceRndTransformableParentPlan plan;
  plan.same_parent = same_parent;
  plan.preserve_world = preserve_world;
  plan.had_old_parent = had_old_parent;
  plan.has_new_parent = has_new_parent;
  if (same_parent) {
    plan.same_parent_sets_dirty = true;
    plan.calls_set_dirty = true;
    return plan;
  }
  plan.computes_reparent_delta = preserve_world;
  plan.transforms_local_xfm = preserve_world;
  plan.transforms_trans_anims = preserve_world;
  plan.removes_from_old_parent = had_old_parent;
  plan.assigns_parent = true;
  plan.cache_set_to_new_parent_or_zero = true;
  plan.adds_to_new_parent_children = has_new_parent;
  plan.calls_set_dirty = true;
  return plan;
}

SourceRndTransformableWorldWritePlan
source_rndtransformable_world_write_plan(
    const std::string& setter,
    bool has_children) {
  SourceRndTransformableWorldWritePlan plan;
  plan.setter = setter;
  if (setter == "SetWorldXfm") {
    plan.writes_world_xfm = true;
    plan.clears_cache_dirty_bit = true;
    plan.calls_updated_world_xfm = true;
    plan.dirties_children = has_children;
  } else if (setter == "SetWorldPos") {
    plan.writes_world_position_only = true;
    plan.calls_updated_world_xfm = true;
    plan.dirties_children = has_children;
  }
  return plan;
}

SourceRndTransformableLocalWritePlan
source_rndtransformable_local_write_plan(const std::string& setter) {
  SourceRndTransformableLocalWritePlan plan;
  plan.setter = setter;
  if (setter == "ResetLocalXfm") {
    plan.resets_local_xfm = true;
    plan.calls_set_dirty = true;
  } else if (setter == "SetLocalXfm") {
    plan.writes_local_xfm = true;
    plan.calls_set_dirty = true;
  } else if (setter == "SetLocalRot") {
    plan.writes_local_rotation = true;
    plan.calls_set_dirty = true;
  } else if (setter == "SetLocalPos") {
    plan.writes_local_position = true;
    plan.calls_set_dirty = true;
  } else if (setter == "DirtyLocalXfm") {
    plan.calls_set_dirty = true;
    plan.returns_dirty_local_ref = true;
  }
  return plan;
}

SourceRndTransformableConstraintPlan
source_rndtransformable_set_constraint_plan(
    int32_t constraint,
    const std::string& target,
    bool preserve_scale) {
  SourceRndTransformableConstraintPlan plan;
  plan.constraint = constraint;
  plan.target = target;
  plan.preserve_scale = preserve_scale;
  return plan;
}

SourceRndTransformableCopyPlan source_rndtransformable_copy_plan() {
  SourceRndTransformableCopyPlan plan;
  plan.member_steps = {
      "COPY_MEMBER(mWorldXfm)",
      "COPY_MEMBER(mLocalXfm)",
      "if(ty != kCopyFromMax) COPY_MEMBER(mPreserveScale)",
      "if(ty != kCopyFromMax) COPY_MEMBER(mConstraint)",
      "if(ty != kCopyFromMax) COPY_MEMBER(mTarget)",
      "else if(mConstraint == c->mConstraint) COPY_MEMBER(mTarget)",
      "SetTransParent(c->mParent, false)",
  };
  return plan;
}

SourceRndTransformableHandlerPlan source_rndtransformable_handler_plan() {
  SourceRndTransformableHandlerPlan plan;
  plan.handlers = {
      "copy_local_to:OnCopyLocalTo",
      "set_constraint:OnSetTransConstraint",
      "set_local_rot:OnSetLocalRot",
      "set_local_rot_index:OnSetLocalRotIndex",
      "set_local_rot_mat:OnSetLocalRotMat",
      "set_local_pos:OnSetLocalPos",
      "set_local_pos_index:OnSetLocalPosIndex",
      "get_local_rot:OnGetLocalRot",
      "get_local_rot_index:OnGetLocalRotIndex",
      "get_local_pos:OnGetLocalPos",
      "get_local_pos_index:OnGetLocalPosIndex",
      "set_local_scale:OnSetLocalScale",
      "set_local_scale_index:OnSetLocalScaleIndex",
      "get_local_scale:OnGetLocalScale",
      "get_local_scale_index:OnGetLocalScaleIndex",
      "get_world_forward:OnGetWorldForward",
      "get_world_pos:OnGetWorldPos",
      "get_world_rot:OnGetWorldRot",
      "get_children:OnGetChildren",
  };
  plan.actions = {
      "normalize_local:Normalize(mLocalXfm.m,mLocalXfm.m)",
      "set_trans_parent:SetTransParent",
      "reset_xfm:DirtyLocalXfm().Reset()",
      "distribute_children:DistributeChildren",
  };
  plan.exprs = {"trans_parent:mParent"};
  plan.superclasses = {"Hmx::Object"};
  return plan;
}

SourceRndTransformablePropSyncPlan
source_rndtransformable_prop_sync_plan() {
  SourceRndTransformablePropSyncPlan plan;
  plan.set_properties = {
      "trans_parent:SetTransParent(_val.Obj<RndTransformable>(0), true)",
      "trans_constraint:SetTransConstraint((Constraint)_val.Int(0), mTarget, mPreserveScale)",
      "trans_target:SetTransConstraint((Constraint)mConstraint, _val.Obj<RndTransformable>(0), mPreserveScale)",
      "preserve_scale:SetTransConstraint((Constraint)mConstraint, mTarget, _val.Int(0))",
  };
  return plan;
}

SourceRndTransformableDistributeChildrenPlan
source_rndtransformable_distribute_children_plan(
    bool horizontal,
    float spacing,
    const std::vector<SourceRndTransformableChildRow>& children) {
  SourceRndTransformableDistributeChildrenPlan plan;
  plan.horizontal = horizontal;
  plan.spacing = spacing;
  plan.axis = horizontal ? 0 : 2;
  std::vector<size_t> order(children.size());
  for (size_t i = 0; i < children.size(); ++i) order[i] = i;
  if (children.size() < 2) return plan;

  plan.entered = true;
  if (horizontal) {
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
      return children[a].local_x < children[b].local_x;
    });
  } else {
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
      return children[a].local_z > children[b].local_z;
    });
  }

  auto axis_value = [&](size_t index) {
    return horizontal ? children[index].local_x : children[index].local_z;
  };
  plan.base_axis_value = axis_value(order.front());
  for (size_t sorted_index : order) {
    plan.sorted_children.push_back(children[sorted_index].name);
  }
  for (size_t i = 1; i < order.size(); ++i) {
    const size_t source_index = order[i];
    SourceRndTransformableDistributedChild write;
    write.name = children[source_index].name;
    write.source_index = static_cast<int32_t>(source_index);
    write.original_axis_value = axis_value(source_index);
    write.assigned_axis_value =
        spacing * static_cast<float>(i) + plan.base_axis_value;
    plan.writes.push_back(write);
  }
  return plan;
}

SourceRndTransformableCopyLocalToPlan
source_rndtransformable_copy_local_to_plan(
    const std::vector<std::string>& targets) {
  SourceRndTransformableCopyLocalToPlan plan;
  for (auto it = targets.rbegin(); it != targets.rend(); ++it) {
    plan.write_order.push_back(*it);
  }
  return plan;
}

SourceRndTransProxyDefaultState source_rndtrans_proxy_default_state() {
  return SourceRndTransProxyDefaultState{};
}

SourceRndTransProxyLoadPlan source_rndtrans_proxy_load_plan(int32_t revision) {
  SourceRndTransProxyLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 1;
  plan.reads_transformable = revision != 0;
  return plan;
}

SourceRndTransProxySyncPlan source_rndtrans_proxy_sync_plan(
    bool has_proxy,
    bool part_null,
    bool proxy_is_transformable,
    bool part_lookup_found_transformable) {
  SourceRndTransProxySyncPlan plan;
  plan.has_proxy = has_proxy;
  plan.part_null = part_null;
  plan.attempts_direct_proxy_parent = has_proxy && part_null;
  plan.uses_direct_proxy_parent =
      plan.attempts_direct_proxy_parent && proxy_is_transformable;
  plan.attempts_part_lookup = has_proxy && !plan.uses_direct_proxy_parent;
  plan.uses_part_lookup_parent =
      plan.attempts_part_lookup && part_lookup_found_transformable;
  if (plan.uses_direct_proxy_parent) {
    plan.clears_parent_final = false;
    plan.resolved_parent_source = "proxy";
  } else if (plan.uses_part_lookup_parent) {
    plan.clears_parent_final = false;
    plan.resolved_parent_source = "part";
  }
  return plan;
}

SourceRndTransProxySetterPlan source_rndtrans_proxy_setter_plan(
    bool value_changed) {
  SourceRndTransProxySetterPlan plan;
  plan.value_changed = value_changed;
  plan.assigns_value = value_changed;
  plan.calls_sync = value_changed;
  return plan;
}

SourceRndTransProxySavePlan source_rndtrans_proxy_save_plan() {
  return SourceRndTransProxySavePlan{};
}

SourceRndTransProxyCopyPlan source_rndtrans_proxy_copy_plan() {
  SourceRndTransProxyCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndTransformable"};
  plan.member_order = {"mProxy", "mPart"};
  return plan;
}

SourceRndTransProxyHandlerPlan source_rndtrans_proxy_handler_plan() {
  SourceRndTransProxyHandlerPlan plan;
  plan.superclasses = {"RndTransformable", "Hmx::Object"};
  return plan;
}

SourceRndTransProxyPropSyncPlan source_rndtrans_proxy_prop_sync_plan() {
  SourceRndTransProxyPropSyncPlan plan;
  plan.props = {"proxy:Sync", "part:Sync"};
  plan.superclasses = {"RndTransformable"};
  return plan;
}

SourceRndTransAnimDefaultState source_rndtrans_anim_default_state() {
  return SourceRndTransAnimDefaultState{};
}

DecodedRndTransAnimBody decode_rndtrans_anim_body_source_order(
    const uint8_t* body, size_t size) {
  DecodedRndTransAnimBody out;
  if (body == nullptr && size != 0) {
    out.error = "RndTransAnim null body";
    return out;
  }

  Reader r(body, size);
  try {
    const uint32_t combined_revision = r.u32();
    out.revision = low_revision(combined_revision);
    out.alt_revision = static_cast<uint16_t>(combined_revision >> 16);
    if (out.revision < 6 || out.revision > 7) {
      throw std::runtime_error(
          "milo_scene: exact RndTransAnim reader requires revision 6 or 7");
    }

    // MiloEditor RndTransAnim.Read: revision > 4 embeds Hmx::Object fields.
    read_object_fields(r);

    const uint32_t combined_anim_revision = r.u32();
    out.anim_revision = low_revision(combined_anim_revision);
    out.anim_alt_revision =
        static_cast<uint16_t>(combined_anim_revision >> 16);
    const SourceRndAnimatableLoadPlan anim_plan =
        source_rndanimatable_load_plan(out.anim_revision);
    if (!anim_plan.accepted_revision) {
      throw std::runtime_error(
          "milo_scene: unsupported embedded RndAnimatable revision");
    }
    if (anim_plan.reads_frame) out.anim_frame = r.f32();
    if (anim_plan.reads_int_rate) {
      out.anim_rate = r.i32();
    } else if (anim_plan.reads_legacy_byte_rate) {
      out.anim_rate = r.u8() == 0 ? 0 : 1;
    } else if (anim_plan.reads_legacy_rev0_filter_rows ||
               anim_plan.reads_legacy_rev0_anim_list) {
      throw std::runtime_error(
          "milo_scene: legacy RndAnimatable revision 0 is not used by GH2 track TransAnim");
    }

    out.target = r.str();

    const uint32_t rotation_count = r.u32();
    if (rotation_count > 512) {
      throw std::runtime_error("milo_scene: RndTransAnim rotation count invalid");
    }
    out.rotation_keys.reserve(rotation_count);
    for (uint32_t i = 0; i < rotation_count; ++i) {
      RndTransAnimQuatKey key;
      for (float& value : key.value) value = r.f32();
      key.frame = r.f32();
      out.rotation_keys.push_back(key);
    }

    const uint32_t translation_count = r.u32();
    if (translation_count > 2048) {
      throw std::runtime_error(
          "milo_scene: RndTransAnim translation count invalid");
    }
    out.translation_keys.reserve(translation_count);
    for (uint32_t i = 0; i < translation_count; ++i) {
      RndTransAnimVec3Key key;
      for (float& value : key.value) value = r.f32();
      key.frame = r.f32();
      out.translation_keys.push_back(key);
    }

    out.keys_owner = r.str();
    out.trans_spline = r.u8() != 0;
    out.repeat_trans = r.u8() != 0;

    const uint32_t scale_count = r.u32();
    if (scale_count > 512) {
      throw std::runtime_error("milo_scene: RndTransAnim scale count invalid");
    }
    out.scale_keys.reserve(scale_count);
    for (uint32_t i = 0; i < scale_count; ++i) {
      RndTransAnimVec3Key key;
      for (float& value : key.value) value = r.f32();
      key.frame = r.f32();
      out.scale_keys.push_back(key);
    }
    out.scale_spline = r.u8() != 0;
    out.follow_path = r.u8() != 0;
    out.rot_slerp = r.u8() != 0;
    if (out.revision > 6) out.rot_spline = r.u8() != 0;

    out.bytes_consumed = r.pos;
    out.exact_eof = r.pos == r.n;
    if (!out.exact_eof) {
      throw std::runtime_error(
          "milo_scene: RndTransAnim source-order reader did not consume EOF");
    }
    out.decoded = true;
  } catch (const std::exception& ex) {
    out.bytes_consumed = r.pos;
    out.error = ex.what();
  }
  return out;
}

std::array<float, 3> sample_rndtrans_anim_translation(
    const DecodedRndTransAnimBody& anim, float frame,
    const std::array<float, 3>& fallback) {
  const auto& keys = anim.translation_keys;
  if (keys.empty()) return fallback;

  float sample_frame = frame;
  std::array<float, 3> repeat_offset = {0.0f, 0.0f, 0.0f};
  if (anim.repeat_trans && keys.size() >= 2 && std::isfinite(frame)) {
    const float first_frame = keys.front().frame;
    const float last_frame = keys.back().frame;
    const float span = last_frame - first_frame;
    if (std::isfinite(span) && span > 0.001f && frame >= last_frame) {
      const float cycles = std::floor((frame - first_frame) / span);
      if (std::isfinite(cycles) && cycles > 0.0f) {
        sample_frame = frame - cycles * span;
        for (size_t axis = 0; axis < repeat_offset.size(); ++axis) {
          repeat_offset[axis] =
              (keys.back().value[axis] - keys.front().value[axis]) * cycles;
        }
      }
    }
  }

  size_t a = 0;
  size_t b = 0;
  float t = 0.0f;
  if (keys.size() > 1 && sample_frame >= keys.front().frame) {
    if (sample_frame >= keys.back().frame) {
      a = b = keys.size() - 1;
    } else {
      for (size_t i = 1; i < keys.size(); ++i) {
        if (sample_frame < keys[i].frame) break;
        a = i;
      }
      while (a + 1 < keys.size() && keys[a + 1].frame == keys[a].frame) ++a;
      b = std::min(a + 1, keys.size() - 1);
      const float span = keys[b].frame - keys[a].frame;
      if (span > 0.0001f) {
        t = std::clamp((sample_frame - keys[a].frame) / span, 0.0f, 1.0f);
      }
    }
  }

  std::array<float, 3> value = keys[a].value;
  // Harmonix InterpVector explicitly disables spline interpolation when fewer
  // than three keys exist.  That makes GH2's two-key track.cam move linear
  // even though extend_track_normal.tnm stores transSpline=true.
  if (a != b && anim.trans_spline && keys.size() >= 3) {
    auto tangent = [&](size_t index) {
      std::array<float, 3> out_tangent = {0.0f, 0.0f, 0.0f};
      if (index == 0) {
        for (size_t axis = 0; axis < out_tangent.size(); ++axis) {
          out_tangent[axis] =
              (keys[1].value[axis] - keys[0].value[axis]) * 1.5f -
              (keys[2].value[axis] - keys[0].value[axis]) * 0.25f;
        }
      } else if (index >= keys.size() - 1) {
        for (size_t axis = 0; axis < out_tangent.size(); ++axis) {
          out_tangent[axis] =
              (keys.back().value[axis] -
               keys[keys.size() - 2].value[axis]) *
                  1.5f -
              (keys.back().value[axis] -
               keys[keys.size() - 3].value[axis]) *
                  0.25f;
        }
      } else {
        for (size_t axis = 0; axis < out_tangent.size(); ++axis) {
          out_tangent[axis] =
              (keys[index + 1].value[axis] -
               keys[index - 1].value[axis]) *
              0.5f;
        }
      }
      return out_tangent;
    };
    const auto ta = tangent(a);
    const auto tb = tangent(b);
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    const float h10 = t3 - 2.0f * t2 + t;
    const float h01 = -2.0f * t3 + 3.0f * t2;
    const float h11 = t3 - t2;
    for (size_t axis = 0; axis < value.size(); ++axis) {
      value[axis] = keys[a].value[axis] * h00 + ta[axis] * h10 +
                    keys[b].value[axis] * h01 + tb[axis] * h11;
    }
  } else if (a != b) {
    for (size_t axis = 0; axis < value.size(); ++axis) {
      value[axis] = keys[a].value[axis] +
                    (keys[b].value[axis] - keys[a].value[axis]) * t;
    }
  }
  for (size_t axis = 0; axis < value.size(); ++axis) {
    value[axis] += repeat_offset[axis];
  }
  return value;
}

float rndtrans_anim_start_frame(const DecodedRndTransAnimBody& anim) {
  const float trans = anim.translation_keys.empty()
                          ? 0.0f
                          : anim.translation_keys.front().frame;
  const float rot = anim.rotation_keys.empty()
                        ? 0.0f
                        : anim.rotation_keys.front().frame;
  const float scale =
      anim.scale_keys.empty() ? 0.0f : anim.scale_keys.front().frame;
  return std::min({trans, rot, scale});
}

float rndtrans_anim_end_frame(const DecodedRndTransAnimBody& anim) {
  const float trans = anim.translation_keys.empty()
                          ? 0.0f
                          : anim.translation_keys.back().frame;
  const float rot =
      anim.rotation_keys.empty() ? 0.0f : anim.rotation_keys.back().frame;
  const float scale =
      anim.scale_keys.empty() ? 0.0f : anim.scale_keys.back().frame;
  return std::max({trans, rot, scale});
}

SourceRndTransAnimLoadPlan source_rndtrans_anim_load_plan(int32_t revision) {
  SourceRndTransAnimLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 7;
  plan.reads_object_fields = revision > 4;
  plan.dumps_drawable = revision < 6;
  plan.reads_rot_and_trans_keys = revision != 2;
  plan.reads_legacy_int = revision < 3;
  plan.reads_follow_path = revision > 1;
  plan.follow_path_from_keys_owner = revision <= 1;
  plan.reads_rot_slerp = revision > 3;
  plan.reads_rot_spline = revision > 6;
  return plan;
}

SourceRndTransAnimSetKeysOwnerPlan source_rndtrans_anim_set_keys_owner_plan() {
  return SourceRndTransAnimSetKeysOwnerPlan{};
}

SourceRndTransAnimReplacePlan source_rndtrans_anim_replace_plan(
    bool keys_owner_matches_from,
    bool replacement_null) {
  SourceRndTransAnimReplacePlan plan;
  plan.keys_owner_matches_from = keys_owner_matches_from;
  plan.replacement_null = replacement_null;
  if (keys_owner_matches_from) {
    plan.assigns_self = replacement_null;
    plan.copies_replacement_keys_owner = !replacement_null;
  }
  return plan;
}

SourceRndTransAnimCopyPlan source_rndtrans_anim_copy_plan(
    bool copy_shallow,
    bool copy_from_max,
    bool source_keys_owner_is_self) {
  SourceRndTransAnimCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndAnimatable"};
  plan.copies_keys_owner_ref =
      copy_shallow || (copy_from_max && !source_keys_owner_is_self);
  plan.assigns_self_as_keys_owner = !plan.copies_keys_owner_ref;
  if (plan.assigns_self_as_keys_owner) {
    plan.copied_owned_members = {"mTransKeys",   "mRotKeys",    "mScaleKeys",
                                 "mTransSpline", "mRepeatTrans", "mScaleSpline",
                                 "mFollowPath",  "mRotSlerp",   "mRotSpline"};
  }
  return plan;
}

SourceRndTransAnimFramePlan source_rndtrans_anim_set_frame_plan(bool has_trans) {
  SourceRndTransAnimFramePlan plan;
  plan.has_trans = has_trans;
  plan.make_transform_assert_body_only = true;
  plan.copies_local_transform = has_trans;
  plan.calls_make_transform = has_trans;
  plan.writes_local_transform = has_trans;
  return plan;
}

SourceRndTransAnimSetKeyPlan source_rndtrans_anim_set_key_plan(bool has_trans) {
  SourceRndTransAnimSetKeyPlan plan;
  plan.has_trans = has_trans;
  if (has_trans) {
    plan.operations = {"add_trans_key_from_local_translation",
                       "normalize_local_matrix",
                       "add_rot_key_from_quat",
                       "add_scale_key_from_local_matrix"};
  }
  return plan;
}

SourceRndTransAnimHandlerPlan source_rndtrans_anim_handler_plan() {
  SourceRndTransAnimHandlerPlan plan;
  plan.handlers = {"trans",           "splice",
                   "linearize",       "set_trans",
                   "remove_rot_keys", "remove_trans_keys",
                   "num_trans_keys",  "num_rot_keys",
                   "num_scale_keys",  "add_trans_key",
                   "add_rot_key",     "add_scale_key",
                   "set_trans_spline","set_scale_spline",
                   "set_rot_slerp"};
  plan.superclasses = {"RndAnimatable", "Hmx::Object"};
  return plan;
}

SourceRndTransAnimPropSyncPlan source_rndtrans_anim_prop_sync_plan() {
  SourceRndTransAnimPropSyncPlan plan;
  plan.props = {"keys_owner:SetKeysOwner"};
  plan.superclasses = {"RndAnimatable"};
  return plan;
}

SourceRndAnimatableDefaultState source_rndanimatable_default_state() {
  return SourceRndAnimatableDefaultState{};
}

SourceRndAnimatableLoadPlan source_rndanimatable_load_plan(
    int32_t revision) {
  SourceRndAnimatableLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 4;
  plan.reads_frame = revision > 1;
  plan.reads_int_rate = revision > 3;
  plan.reads_legacy_byte_rate = revision > 2 && revision <= 3;
  plan.reads_legacy_rev0_filter_rows = revision < 1;
  plan.reads_legacy_rev0_anim_list = revision < 1;
  return plan;
}

SourceRndAnimatableRatePlan source_rndanimatable_rate_plan(
    SourceRndAnimRate rate) {
  SourceRndAnimatableRatePlan plan;
  plan.rate = rate;
  switch (rate) {
    case kSourceRndAnimRate30Fps:
      plan.valid_rate = true;
      plan.task_units = "seconds";
      plan.frames_per_unit = 30.0f;
      break;
    case kSourceRndAnimRate480Fpb:
      plan.valid_rate = true;
      plan.task_units = "beats";
      plan.frames_per_unit = 480.0f;
      break;
    case kSourceRndAnimRate30FpsUi:
      plan.valid_rate = true;
      plan.task_units = "ui_seconds";
      plan.frames_per_unit = 30.0f;
      break;
    case kSourceRndAnimRate1Fpb:
      plan.valid_rate = true;
      plan.task_units = "beats";
      plan.frames_per_unit = 1.0f;
      break;
    case kSourceRndAnimRate30FpsTutorial:
      plan.valid_rate = true;
      plan.task_units = "tutorial_seconds";
      plan.frames_per_unit = 30.0f;
      break;
    default:
      break;
  }
  return plan;
}

SourceRndAnimatableConvertFramesPlan
source_rndanimatable_convert_frames_plan(
    SourceRndAnimRate rate,
    float input_frames) {
  SourceRndAnimatableConvertFramesPlan plan;
  plan.rate = rate;
  plan.input_frames = input_frames;
  const SourceRndAnimatableRatePlan rate_plan =
      source_rndanimatable_rate_plan(rate);
  plan.task_units = rate_plan.task_units;
  if (rate_plan.frames_per_unit != 0.0f) {
    plan.output_units = input_frames / rate_plan.frames_per_unit;
  }
  plan.returns_converted = rate_plan.task_units != "beats";
  return plan;
}

SourceRndAnimatableCopyPlan source_rndanimatable_copy_plan() {
  return SourceRndAnimatableCopyPlan{};
}

SourceRndAnimatableHandlerPlan source_rndanimatable_handler_plan() {
  SourceRndAnimatableHandlerPlan plan;
  plan.handlers = {"set_frame",      "frame",        "set_key",
                   "end_frame",      "start_frame",  "animate",
                   "stop_animation", "is_animating", "convert_frames"};
  return plan;
}

SourceRndAnimatablePropSyncPlan source_rndanimatable_prop_sync_plan() {
  SourceRndAnimatablePropSyncPlan plan;
  plan.props = {"rate", "frame:SetFrame"};
  return plan;
}

SourceRndAnimatableAnimatePlan source_rndanimatable_on_animate_plan() {
  SourceRndAnimatableAnimatePlan plan;
  plan.defaults = {"blend=0",       "start=StartFrame",
                   "end=EndFrame",  "loop=Loop",
                   "units=Units",   "period=FramesPerUnit",
                   "delay=0",       "name=null",
                   "wait=false"};
  plan.data_keys = {"blend", "delay", "units", "name", "wait"};
  plan.mode_rows = {"range:start/end/no-loop",
                    "loop:optional-start/optional-end/loop",
                    "dest:current-frame-to-dest/no-loop",
                    "period:abs-end-minus-start-over-period"};
  return plan;
}

SourceAnimTaskInitPlan source_anim_task_init_plan(
    float start,
    float end,
    float frames_per_unit,
    bool loop,
    float blend_period,
    bool has_blend_task) {
  SourceAnimTaskInitPlan plan;
  plan.start = start;
  plan.end = end;
  plan.frames_per_unit = frames_per_unit;
  plan.loop = loop;
  plan.blend_period = blend_period;
  plan.min_frame = std::min(start, end);
  plan.max_frame = std::max(start, end);
  if (start < end) {
    plan.scale = frames_per_unit;
    plan.offset = plan.min_frame;
  } else {
    plan.scale = -frames_per_unit;
    plan.offset = plan.max_frame;
  }
  plan.marks_blend_task_when_blending =
      blend_period != 0.0f && has_blend_task;
  return plan;
}

SourceAnimTaskTimePlan source_anim_task_time_until_end_plan(
    float min_frame,
    float max_frame,
    float current_frame,
    float frames_per_unit,
    float scale) {
  SourceAnimTaskTimePlan plan;
  plan.min_frame = min_frame;
  plan.max_frame = max_frame;
  plan.current_frame = current_frame;
  plan.frames_per_unit = frames_per_unit;
  plan.scale = scale;
  if (frames_per_unit == 0.0f) return plan;
  if (scale > 0.0f) {
    plan.time_until_end = (max_frame - current_frame) / frames_per_unit;
  } else {
    plan.time_until_end = (current_frame - min_frame) / frames_per_unit;
  }
  return plan;
}

SourceRndPollableHandlerPlan source_rndpollable_handler_plan() {
  SourceRndPollableHandlerPlan plan;
  plan.actions = {"enter", "poll"};
  plan.static_actions = {"exit"};
  return plan;
}

SourceRndPollableBasePlan source_rndpollable_base_plan() {
  return SourceRndPollableBasePlan{};
}

SourceRndPollAnimDefaultState source_rndpollanim_default_state() {
  return SourceRndPollAnimDefaultState{};
}

SourceRndPollAnimEndFramePlan source_rndpollanim_end_frame_plan(
    const std::vector<float>& child_end_frames) {
  SourceRndPollAnimEndFramePlan plan;
  plan.child_end_frames = child_end_frames;
  for (float child_frame : child_end_frames) {
    if (plan.result < child_frame) plan.result = child_frame;
  }
  return plan;
}

SourceRndPollAnimChildListPlan source_rndpollanim_child_list_plan(
    int32_t child_count) {
  SourceRndPollAnimChildListPlan plan;
  plan.child_count = child_count;
  plan.published_children = std::max(0, child_count);
  return plan;
}

SourceRndPollAnimLifecyclePlan source_rndpollanim_enter_plan(
    int32_t child_count) {
  SourceRndPollAnimLifecyclePlan plan;
  plan.child_count = child_count;
  plan.start_anim_calls = std::max(0, child_count);
  return plan;
}

SourceRndPollAnimLifecyclePlan source_rndpollanim_exit_plan(
    int32_t child_count) {
  SourceRndPollAnimLifecyclePlan plan;
  plan.child_count = child_count;
  plan.end_anim_calls = std::max(0, child_count);
  return plan;
}

SourceRndPollAnimRateFramePlan source_rndpollanim_rate_frame_plan(
    SourceRndAnimRate rate,
    float seconds,
    float ui_seconds,
    float tutorial_seconds,
    float beat) {
  SourceRndPollAnimRateFramePlan plan;
  plan.rate = rate;
  switch (rate) {
    case kSourceRndAnimRate30Fps:
      plan.recognized = true;
      plan.uses_seconds = true;
      plan.multiplier = 30.0f;
      plan.frame = 30.0f * seconds;
      break;
    case kSourceRndAnimRate480Fpb:
      plan.recognized = true;
      plan.uses_beat = true;
      plan.multiplier = 480.0f;
      plan.frame = 480.0f * beat;
      break;
    case kSourceRndAnimRate30FpsUi:
      plan.recognized = true;
      plan.uses_ui_seconds = true;
      plan.multiplier = 30.0f;
      plan.frame = 30.0f * ui_seconds;
      break;
    case kSourceRndAnimRate1Fpb:
      plan.recognized = true;
      plan.uses_beat = true;
      plan.multiplier = 1.0f;
      plan.frame = beat;
      break;
    case kSourceRndAnimRate30FpsTutorial:
      plan.recognized = true;
      plan.uses_tutorial_seconds = true;
      plan.multiplier = 30.0f;
      plan.frame = 30.0f * tutorial_seconds;
      break;
    case kSourceRndAnimRateUnknown:
    default:
      break;
  }
  return plan;
}

SourceRndPollAnimPollPlan source_rndpollanim_poll_plan(int32_t child_count) {
  SourceRndPollAnimPollPlan plan;
  plan.child_count = child_count;
  plan.calls_set_frame = child_count > 0;
  return plan;
}

SourceRndPollAnimLoadPlan source_rndpollanim_load_plan(int32_t revision) {
  SourceRndPollAnimLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision == 0;
  plan.superclasses = {"Hmx::Object", "RndAnimatable", "RndPollable"};
  return plan;
}

SourceRndPollAnimCopyPlan source_rndpollanim_copy_plan() {
  SourceRndPollAnimCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndAnimatable", "RndPollable"};
  return plan;
}

SourceRndPollAnimEmptyBodyPlan source_rndpollanim_empty_body_plan() {
  return SourceRndPollAnimEmptyBodyPlan{};
}

SourceRndPollAnimHandlerPlan source_rndpollanim_handler_plan() {
  SourceRndPollAnimHandlerPlan plan;
  plan.superclasses = {"RndAnimatable", "RndPollable", "Hmx::Object"};
  return plan;
}

SourceRndPollAnimPropSyncPlan source_rndpollanim_prop_sync_plan() {
  SourceRndPollAnimPropSyncPlan plan;
  plan.props = {"anims"};
  plan.superclass_order = {"RndAnimatable", "RndPollable"};
  return plan;
}

SourceRndPropAnimDefaultState source_rndpropanim_default_state() {
  return SourceRndPropAnimDefaultState{};
}

SourceRndPropAnimLoadPlan source_rndpropanim_load_plan(int32_t revision) {
  SourceRndPropAnimLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 0xD;
  plan.superclasses = {"Hmx::Object", "RndAnimatable"};
  plan.uses_pre7_loader = revision < 7;
  plan.reads_key_count = revision >= 7;
  plan.reads_key_type_per_entry = revision >= 7;
  plan.loads_prop_keys_per_entry = revision >= 7;
  plan.reads_loop = revision > 0xB;
  return plan;
}

SourceRndPropAnimPre7LoadPlan source_rndpropanim_pre7_load_plan(
    int32_t revision) {
  SourceRndPropAnimPre7LoadPlan plan;
  plan.revision = revision;
  plan.reads_legacy_owner_before_count = revision < 2;
  plan.reads_owner_per_entry = revision >= 2;
  plan.reads_symbol_property = revision < 1;
  plan.reads_dataarray_property = revision >= 1;
  plan.reads_float_keys_only = revision < 3;
  plan.reads_anim_type = revision >= 3;
  plan.reads_color_keys = revision >= 3;
  plan.reads_object_keys_with_owner_stage = revision > 3;
  plan.reads_bool_keys = revision > 4;
  plan.reads_quat_keys = revision > 5;
  return plan;
}

SourceRndPropAnimCopyPlan source_rndpropanim_copy_plan() {
  SourceRndPropAnimCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndAnimatable"};
  return plan;
}

SourceRndPropAnimFrameBoundsPlan source_rndpropanim_start_frame_plan(
    const std::vector<float>& key_start_frames) {
  SourceRndPropAnimFrameBoundsPlan plan;
  plan.key_frames = key_start_frames;
  for (float frame : key_start_frames) {
    plan.result = std::min(plan.result, frame);
  }
  return plan;
}

SourceRndPropAnimFrameBoundsPlan source_rndpropanim_end_frame_plan(
    const std::vector<float>& key_end_frames) {
  SourceRndPropAnimFrameBoundsPlan plan;
  plan.key_frames = key_end_frames;
  for (float frame : key_end_frames) {
    plan.result = std::max(plan.result, frame);
  }
  return plan;
}

SourceRndPropAnimAdvanceFramePlan source_rndpropanim_advance_frame_plan(
    bool loop) {
  SourceRndPropAnimAdvanceFramePlan plan;
  plan.loop = loop;
  plan.applies_mod_range = loop;
  return plan;
}

SourceRndPropAnimSetFramePlan source_rndpropanim_set_frame_plan(
    bool already_in_set_frame,
    int32_t key_count,
    int32_t dir_event_key_count) {
  SourceRndPropAnimSetFramePlan plan;
  plan.already_in_set_frame = already_in_set_frame;
  plan.key_count = key_count;
  plan.dir_event_key_count = dir_event_key_count;
  if (!already_in_set_frame) {
    plan.enters_set_frame_guard = true;
    plan.calls_advance_frame = true;
    plan.scans_dir_event_keys = dir_event_key_count > 0;
    plan.sets_each_key_frame = key_count > 0;
    plan.updates_last_frame = true;
    plan.clears_set_frame_guard = true;
  }
  return plan;
}

SourceRndPropAnimKeyListPlan source_rndpropanim_set_key_plan(
    int32_t key_count) {
  SourceRndPropAnimKeyListPlan plan;
  plan.key_count = key_count;
  plan.calls = std::max(0, key_count);
  return plan;
}

SourceRndPropAnimKeyListPlan source_rndpropanim_start_anim_plan(
    int32_t key_count) {
  SourceRndPropAnimKeyListPlan plan;
  plan.key_count = key_count;
  plan.calls = std::max(0, key_count);
  return plan;
}

SourceRndPropAnimKeyListPlan source_rndpropanim_remove_all_keys_plan(
    int32_t key_count) {
  SourceRndPropAnimKeyListPlan plan;
  plan.key_count = key_count;
  plan.calls = std::max(0, key_count);
  return plan;
}

SourceRndPropAnimFindKeysPlan source_rndpropanim_find_keys_plan(
    bool property_null,
    bool target_matches,
    bool property_matches,
    bool row_property_null) {
  SourceRndPropAnimFindKeysPlan plan;
  plan.property_null = property_null;
  plan.target_matches = target_matches;
  plan.property_matches = property_matches;
  plan.matches_null_property_row = property_null && row_property_null;
  plan.found =
      plan.matches_null_property_row || (target_matches && property_matches);
  return plan;
}

SourceRndPropAnimChangePropPathPlan source_rndpropanim_change_prop_path_plan(
    bool new_path_empty,
    bool found_existing_keys) {
  SourceRndPropAnimChangePropPathPlan plan;
  plan.new_path_empty = new_path_empty;
  plan.calls_remove_keys = new_path_empty;
  plan.found_existing_keys = found_existing_keys;
  plan.sets_new_prop = !new_path_empty && found_existing_keys;
  plan.result = found_existing_keys;
  return plan;
}

namespace {

std::string source_propkeys_output_kind(SourcePropKeysAnimKeysType type) {
  switch (type) {
    case kSourcePropKeysFloat:
      return "float";
    case kSourcePropKeysColor:
      return "packed_color";
    case kSourcePropKeysObject:
      return "object";
    case kSourcePropKeysBool:
      return "bool";
    case kSourcePropKeysQuat:
      return "quat_array";
    case kSourcePropKeysVector3:
      return "vector3_array";
    case kSourcePropKeysSymbol:
      return "symbol";
  }
  return "zero";
}

}  // namespace

SourceRndPropAnimValuePlan source_rndpropanim_value_from_index_plan(
    SourcePropKeysAnimKeysType type,
    bool has_keys,
    bool valid_index) {
  SourceRndPropAnimValuePlan plan;
  plan.type = type;
  plan.has_keys = has_keys;
  plan.valid_index = valid_index;
  plan.result = has_keys && valid_index;
  plan.output_kind = plan.result ? source_propkeys_output_kind(type) : "zero";
  return plan;
}

SourceRndPropAnimValuePlan source_rndpropanim_value_from_frame_plan(
    SourcePropKeysAnimKeysType type,
    bool has_keys) {
  SourceRndPropAnimValuePlan plan;
  plan.type = type;
  plan.has_keys = has_keys;
  plan.valid_index = has_keys;
  plan.result = has_keys;
  plan.output_kind = has_keys ? source_propkeys_output_kind(type) : "index_-1";
  return plan;
}

SourceRndPropAnimHandlerPlan source_rndpropanim_handler_plan() {
  SourceRndPropAnimHandlerPlan plan;
  plan.expressions = {"remove_keys", "has_keys", "keys_type", "interp_type",
                      "interp_handler", "change_prop_path"};
  plan.actions = {"add_keys", "set_key", "set_key_val", "set_interp_type",
                  "set_interp_handler", "replace_target"};
  plan.handlers = {"foreach_target",     "forall_keyframes",
                   "foreach_keyframe",   "foreach_frame",
                   "replace_keyframe",   "replace_frame",
                   "index_from_frame",   "frame_from_index",
                   "value_from_index",   "value_from_frame"};
  plan.superclasses = {"RndAnimatable", "Hmx::Object"};
  return plan;
}

SourceRndPropAnimPropSyncPlan source_rndpropanim_prop_sync_plan() {
  SourceRndPropAnimPropSyncPlan plan;
  plan.props = {"loop"};
  plan.superclasses = {"RndAnimatable"};
  return plan;
}

SourcePropKeysDefaultState source_propkeys_default_state() {
  return SourcePropKeysDefaultState{};
}

SourcePropKeysLoadPlan source_propkeys_load_plan(int32_t revision,
                                                 int32_t interpolation_row) {
  SourcePropKeysLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 7;
  plan.fails_pre7 = revision < 7;
  if (!plan.accepted_revision) return plan;
  plan.reads_keys_type = true;
  plan.reads_target = true;
  plan.reads_prop = true;
  plan.reads_interpolation = revision >= 8;
  plan.derives_legacy_interpolation = revision < 8;
  plan.legacy_macro_exception_branch =
      revision < 0xB && interpolation_row == 4;
  plan.reads_interp_handler = revision > 9;
  plan.reads_exception_id = revision > 10;
  plan.reads_last_bit = revision > 0xC;
  plan.calls_set_prop_exception_id = true;
  return plan;
}

SourcePropKeysExceptionPlan source_propkeys_exception_plan(
    const std::string& property,
    bool target_is_trans,
    bool target_is_object_dir) {
  SourcePropKeysExceptionPlan plan;
  plan.property = property;
  plan.target_is_trans = target_is_trans;
  plan.target_is_object_dir = target_is_object_dir;
  if (property == "rotation" && target_is_trans) {
    plan.exception_id = kSourcePropKeysTransQuat;
  } else if (property == "scale" && target_is_trans) {
    plan.exception_id = kSourcePropKeysTransScale;
  } else if (property == "position" && target_is_trans) {
    plan.exception_id = kSourcePropKeysTransPos;
  } else if (property == "event" && target_is_object_dir) {
    plan.exception_id = kSourcePropKeysDirEvent;
  }
  return plan;
}

SourcePropKeysSetPropExceptionPlan source_propkeys_set_prop_exception_plan(
    bool interp_handler_null,
    SourcePropKeysExceptionId current_exception,
    SourcePropKeysExceptionId property_exception) {
  SourcePropKeysSetPropExceptionPlan plan;
  plan.interp_handler_null = interp_handler_null;
  plan.current_exception = current_exception;
  plan.property_exception = property_exception;
  if (!interp_handler_null) {
    plan.result_exception = kSourcePropKeysHandleInterp;
  } else if (current_exception == kSourcePropKeysMacro) {
    plan.result_exception = kSourcePropKeysMacro;
  } else {
    plan.result_exception = property_exception;
  }
  plan.updates_transform_cache =
      plan.result_exception == kSourcePropKeysTransQuat ||
      plan.result_exception == kSourcePropKeysTransScale ||
      plan.result_exception == kSourcePropKeysTransPos;
  return plan;
}

SourceRndMeshAnimDefaultState source_rndmeshanim_default_state() {
  return SourceRndMeshAnimDefaultState{};
}

SourceRndMeshAnimNumVertsPlan source_rndmeshanim_num_verts_plan(
    int32_t points_keys,
    int32_t normals_keys,
    int32_t texs_keys,
    int32_t colors_keys) {
  SourceRndMeshAnimNumVertsPlan plan;
  plan.points_keys = points_keys;
  plan.normals_keys = normals_keys;
  plan.texs_keys = texs_keys;
  plan.colors_keys = colors_keys;
  if (points_keys != 0) {
    plan.result = std::max(plan.result, points_keys);
    plan.nonempty_sources.push_back("points");
  }
  if (normals_keys != 0) {
    plan.result = std::max(plan.result, normals_keys);
    plan.nonempty_sources.push_back("normals");
  }
  if (texs_keys != 0) {
    plan.result = std::max(plan.result, texs_keys);
    plan.nonempty_sources.push_back("texs");
  }
  if (colors_keys != 0) {
    plan.result = std::max(plan.result, colors_keys);
    plan.nonempty_sources.push_back("colors");
  }
  return plan;
}

SourceRndMeshAnimReplacePlan source_rndmeshanim_replace_plan(
    bool keys_owner_matches_from,
    bool replacement_null) {
  SourceRndMeshAnimReplacePlan plan;
  plan.keys_owner_matches_from = keys_owner_matches_from;
  plan.replacement_null = replacement_null;
  plan.assigns_self = keys_owner_matches_from && replacement_null;
  plan.copies_replacement_keys_owner =
      keys_owner_matches_from && !replacement_null;
  return plan;
}

SourceRndMeshAnimLoadPlan source_rndmeshanim_load_plan(int32_t revision) {
  SourceRndMeshAnimLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 2;
  plan.reads_object_fields = revision != 0;
  plan.reads_vert_normals_keys = revision > 1;
  return plan;
}

SourceRndMeshAnimCopyPlan source_rndmeshanim_copy_plan(
    bool copy_shallow,
    bool copy_from_max,
    bool source_keys_owner_is_self) {
  SourceRndMeshAnimCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndAnimatable"};
  plan.copies_keys_owner_ref =
      copy_shallow || (copy_from_max && !source_keys_owner_is_self);
  plan.assigns_self_as_keys_owner = !plan.copies_keys_owner_ref;
  if (plan.assigns_self_as_keys_owner) {
    plan.copied_owned_members = {"mVertPointsKeys", "mVertNormalsKeys",
                                 "mVertTexsKeys", "mVertColorsKeys"};
  }
  return plan;
}

SourceRndMeshAnimEndFramePlan source_rndmeshanim_end_frame_plan(
    float points_last,
    float normals_last,
    float texs_last,
    float colors_last) {
  SourceRndMeshAnimEndFramePlan plan;
  plan.points_last = points_last;
  plan.normals_last = normals_last;
  plan.texs_last = texs_last;
  plan.colors_last = colors_last;
  plan.result = std::max(std::max(points_last, normals_last),
                         std::max(texs_last, colors_last));
  return plan;
}

SourceRndMeshAnimInterpPlan source_rndmeshanim_interp_plan(
    float ref,
    float blend,
    int32_t source_values,
    int32_t mesh_verts) {
  SourceRndMeshAnimInterpPlan plan;
  plan.ref = ref;
  plan.blend = blend;
  plan.source_values = source_values;
  plan.mesh_verts = mesh_verts;
  plan.affected_verts = std::max(0, std::min(source_values, mesh_verts));
  plan.uses_first_key = ref != 1.0f;
  plan.uses_second_key = ref != 0.0f;
  plan.interpolates_between_keys = ref != 0.0f && ref != 1.0f;
  plan.blends_with_existing_vert = blend != 1.0f;
  return plan;
}

SourceRndMeshAnimSetFramePlan source_rndmeshanim_set_frame_plan(
    bool has_mesh,
    uint32_t mesh_mutable_mask,
    bool has_points_keys,
    bool has_normals_keys,
    bool has_texs_keys,
    bool has_colors_keys) {
  SourceRndMeshAnimSetFramePlan plan;
  plan.has_mesh = has_mesh;
  plan.mesh_mutable_mask = mesh_mutable_mask;
  plan.mesh_mutable = (mesh_mutable_mask & 0x1Fu) != 0;
  plan.notifies_not_mutable = has_mesh && !plan.mesh_mutable;
  if (has_mesh && plan.mesh_mutable) {
    plan.evaluates_points = has_points_keys;
    plan.evaluates_normals = has_normals_keys;
    plan.evaluates_texs = has_texs_keys;
    plan.evaluates_colors = has_colors_keys;
    if (plan.evaluates_points || plan.evaluates_normals ||
        plan.evaluates_texs || plan.evaluates_colors) {
      plan.sync_mask = 0x1F;
      plan.calls_mesh_sync = true;
    }
  }
  return plan;
}

SourceRndMeshAnimSetKeyPlan source_rndmeshanim_set_key_plan() {
  return SourceRndMeshAnimSetKeyPlan{};
}

SourceRndMeshAnimShrinkPlan source_rndmeshanim_shrink_verts_plan(
    int32_t requested_count,
    bool points_nonempty,
    bool normals_nonempty,
    bool texs_nonempty,
    bool colors_nonempty) {
  SourceRndMeshAnimShrinkPlan plan;
  plan.requested_count = requested_count;
  plan.points_nonempty = points_nonempty;
  plan.normals_nonempty = normals_nonempty;
  plan.texs_nonempty = texs_nonempty;
  plan.colors_nonempty = colors_nonempty;
  if (points_nonempty) plan.resized_streams.push_back("points_values");
  if (normals_nonempty) plan.resized_streams.push_back("normals_values");
  if (texs_nonempty) plan.resized_streams.push_back("texs_values");
  if (colors_nonempty) plan.resized_streams.push_back("colors_values");
  return plan;
}

SourceRndMeshAnimShrinkPlan source_rndmeshanim_shrink_keys_plan(
    int32_t requested_count,
    bool points_nonempty,
    bool normals_nonempty,
    bool texs_nonempty,
    bool colors_nonempty) {
  SourceRndMeshAnimShrinkPlan plan;
  plan.requested_count = requested_count;
  plan.points_nonempty = points_nonempty;
  plan.normals_nonempty = normals_nonempty;
  plan.texs_nonempty = texs_nonempty;
  plan.colors_nonempty = colors_nonempty;
  if (points_nonempty) plan.resized_streams.push_back("points_keys");
  if (normals_nonempty) plan.resized_streams.push_back("normals_keys");
  if (texs_nonempty) plan.resized_streams.push_back("texs_keys");
  if (colors_nonempty) plan.resized_streams.push_back("colors_keys");
  return plan;
}

SourceRndMeshAnimHandlerPlan source_rndmeshanim_handler_plan() {
  SourceRndMeshAnimHandlerPlan plan;
  plan.expressions = {"num_verts"};
  plan.actions = {"shrink_verts", "shrink_keys"};
  plan.superclasses = {"RndAnimatable", "Hmx::Object"};
  return plan;
}

SourceRndMeshAnimPropSyncPlan source_rndmeshanim_prop_sync_plan() {
  SourceRndMeshAnimPropSyncPlan plan;
  plan.props = {"mesh"};
  plan.superclasses = {"RndAnimatable"};
  return plan;
}

TransObj decode_trans(const std::string& entry_name,
                      const std::vector<uint8_t>& body,
                      int32_t parent_dir_revision) {
  Reader r(body.data(), body.size());
  TransObj t;
  t.name = entry_name;
  const TransFields trans = read_trans_block(r, true, parent_dir_revision);
  t.local = trans.local;
  t.world_stored = trans.world;
  t.constraint = trans.constraint;
  t.target = trans.target;
  t.preserve_scale = trans.preserve_scale;
  t.parent = trans.parent;
  return t;
}

CamObj decode_cam(const std::string& entry_name,
                  const std::vector<uint8_t>& body,
                  int32_t parent_dir_revision) {
  CamObj c;
  c.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    const uint32_t combined_revision = r.u32();
    const uint16_t version = static_cast<uint16_t>(combined_revision & 0xffff);
    c.revision = version;
    c.alt_revision = static_cast<uint16_t>((combined_revision >> 16) & 0xffff);
    if (version > 10) read_object_fields(r);

    const TransFields trans = read_trans_block(r, false, parent_dir_revision);
    c.trans_revision = static_cast<uint16_t>(
        parent_dir_revision < 24 ? 8 : 9);
    c.local = trans.local;
    c.world_stored = trans.world;
    c.constraint = trans.constraint;
    c.target = trans.target;
    c.preserve_scale = trans.preserve_scale;
    c.parent = trans.parent;

    if (version < 10) {
      bool showing = true;
      float draw_order = 0.0f;
      read_drawable_block(r, parent_dir_revision, showing, draw_order);
      if (version == 8) {
        const uint32_t objects = r.u32();
        for (uint32_t i = 0; i < objects; ++i) (void)r.str();
      }
    }

    c.near_plane = r.f32();
    c.far_plane = r.f32();
    c.fov = r.f32();
    if (version < 12) c.fov = convert_fov_like_miloeditor(c.fov, 0.75f);
    if (version < 2) (void)r.u32();
    for (float& v : c.screen_rect) v = r.f32();
    if ((version - 1) <= 1) (void)r.u32();
    if (version > 3) {
      c.z_range[0] = r.f32();
      c.z_range[1] = r.f32();
    }
    if (version > 4) c.target_tex = r.str();
    if (version == 6) (void)r.u32();

    if (!std::isfinite(c.near_plane) || !std::isfinite(c.far_plane) ||
        !std::isfinite(c.fov) || c.near_plane <= 0.0f ||
        c.far_plane <= c.near_plane || c.fov <= 0.0f) {
      throw std::runtime_error("milo_scene: invalid Cam projection fields");
    }
    c.source_order_decoded = true;
    c.decoded = true;
  } catch (const std::exception&) {
    // Leave defaults; caller falls back to a framed orbit camera.
  }
  return c;
}

WaypointObj decode_waypoint(const std::string& entry_name,
                            const std::vector<uint8_t>& body) {
  WaypointObj w;
  w.name = entry_name;
  try {
    // GH2 PS2 Waypoint is a Trans subclass, but its Trans block is embedded
    // after Waypoint properties rather than at byte 0. In arena_chars.milo_ps2
    // the embedded block is:
    //   i32 9, local matrix, world matrix, 13 bytes, i32 flags
    // The flags line up with macros.dta: guitarist0mp=512, singer=4,
    // bassist=16, drummer=32. Some authored walk/interact waypoints append
    // extra property strings after the Trans rows; reject values outside the
    // documented bitfield so those string bytes cannot masquerade as starts.
    size_t trans_ver_at = body.size();
    for (size_t o = 0; o + 4 <= body.size(); ++o) {
      int32_t v;
      std::memcpy(&v, body.data() + o, 4);
      if (v == 9 && o + 4 + 48 <= body.size()) {
        trans_ver_at = o;
        break;
      }
    }
    if (trans_ver_at == body.size()) return w;
    Reader r(body.data() + trans_ver_at + 4,
             body.size() - (trans_ver_at + 4));
    w.local = r.matrix();
    if (r.pos + 48 <= r.n) w.world_stored = r.matrix();
    const size_t flags_at = trans_ver_at + 4 + 109;
    if (flags_at + 4 <= body.size()) {
      uint32_t flags = 0;
      std::memcpy(&flags, body.data() + flags_at, 4);
      constexpr uint32_t kWaypointFlagsMask = 0x00000fff;
      if ((flags & ~kWaypointFlagsMask) == 0) w.flags = flags;
    }
    w.decoded = true;
  } catch (const std::exception&) {
  }
  return w;
}

SpotlightObj decode_spotlight(const std::string& entry_name,
                              const std::vector<uint8_t>& body) {
  SpotlightObj s;
  s.name = entry_name;
  s.revision = body.size() >= 4 ? low_revision(read_u32_at(body, 0)) : 0;
  if (decode_spotlight_source_order(body, s)) return s;
  const std::string source_order_error = s.error;
  try {
    Reader r(body.data(), body.size());
    read_spotlight_trans_block(r, s.local, s.world_stored, s.parent);
    s.has_transform = true;
  } catch (const std::exception&) {
    s.parent.clear();
  }
  const std::vector<ScannedString> strings = scan_strings_with_offsets(body);
  bool has_authored_target = false;
  for (const auto& scanned : strings) {
    const auto& ref = scanned.value;
    if (ref.rfind(".mat") != std::string::npos) {
      assign_scanned_spotlight_material(s, ref);
    } else if (ref.rfind(".grp") != std::string::npos) {
      s.group = ref;
    } else if (ref.rfind(".mesh") != std::string::npos) {
      const bool authored_target = is_spotlight_target_mesh(ref);
      (void)authored_target;
      assign_scanned_spotlight_mesh(s, ref, has_authored_target);
    }
  }
  if (!has_authored_target) {
    s.has_default_state =
        read_spotlight_default_state(body, strings, s.default_color,
                                     s.default_intensity);
  }
  s.decoded = true;
  s.error = source_order_error;
  return s;
}

FlareObj decode_flare(const std::string& entry_name,
                      const std::vector<uint8_t>& body,
                      int32_t parent_dir_revision) {
  FlareObj flare;
  flare.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    flare.revision = low_revision(r.u32());
    if (flare.revision > 7) {
      throw std::runtime_error("milo_scene: unsupported Flare revision");
    }
    if (flare.revision > 3) read_object_fields(r);

    const size_t trans_at = r.pos;
    flare.trans_revision = low_revision(read_u32_at(body, trans_at));
    const TransFields trans =
        read_trans_block(r, false, parent_dir_revision);
    flare.local = trans.local;
    flare.world_stored = trans.world;
    flare.constraint = trans.constraint;
    flare.target = trans.target;
    flare.preserve_scale = trans.preserve_scale;
    flare.parent = trans.parent;
    flare.draw_revision =
        read_rnd_drawable_source_layout(r, flare.showing, flare.draw_order);
    if (flare.revision != 0) flare.material = r.str();
    flare.sizes[0] = r.f32();
    flare.sizes[1] =
        flare.revision > 2 ? r.f32() : flare.sizes[0];
    if (flare.revision > 1) {
      flare.range[0] = r.f32();
      flare.range[1] = r.f32();
      flare.steps = r.i32();
    }
    if (flare.revision > 4) flare.point_test = r.u8() != 0;
    if (flare.revision > 6) flare.offset = r.f32();
    if (r.pos != r.n) {
      throw std::runtime_error(
          "milo_scene: Flare source reader did not consume EOF");
    }
    flare.source_order_decoded = true;
    flare.decoded = true;
  } catch (const std::exception& e) {
    flare.error = e.what();
  }
  return flare;
}

LightObj decode_light(const std::string& entry_name,
                      const std::vector<uint8_t>& body) {
  LightObj light;
  light.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    const uint16_t light_revision = low_revision(r.u32());
    if (light_revision != 3 && light_revision != 6) {
      throw std::runtime_error("milo_scene: unsupported Light version");
    }
    if (light_revision == 3) {
      const TransFields trans = read_trans_block(r, false, 10);
      light.local = trans.local;
      light.world_stored = trans.world;
      light.constraint = trans.constraint;
      light.target = trans.target;
      light.preserve_scale = trans.preserve_scale;
      light.parent = trans.parent;
    } else {
      r.skip(kObjMeta);
      read_trans_block(r, light.local, light.world_stored, light.constraint,
                       light.target, light.preserve_scale, light.parent,
                       false);
    }
    if (light.parent == light.name) light.parent.clear();
    for (int i = 0; i < 4; ++i) {
      light.color[i] = r.f32();
      if (!std::isfinite(light.color[i])) {
        throw std::runtime_error("milo_scene: non-finite Light color");
      }
    }
    light.range = r.f32();
    if (!std::isfinite(light.range) || light.range < 0.0f) {
      throw std::runtime_error("milo_scene: invalid Light range");
    }
    int32_t type = r.i32();
    if (light_revision < 0x0e && type > 1) --type;
    if (type < 0 || type > 4) type = 0;
    light.type = type;
    if (light_revision >= 6) {
      light.animate_color_from_preset = r.u8() != 0;
      light.animate_position_from_preset = r.u8() != 0;
      light.animate_range_from_preset = light.animate_color_from_preset;
    }
    light.source_order_decoded = true;
    light.decoded = true;
  } catch (const std::exception& ex) {
    light.error = ex.what();
  }
  return light;
}

EnvironObj decode_environ(const std::string& entry_name,
                          const std::vector<uint8_t>& body) {
  EnvironObj env;
  env.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    const uint16_t revision = low_revision(r.u32());
    env.revision = revision;
    if (revision != 1 && revision != 5) {
      throw std::runtime_error("milo_scene: unsupported Environ version");
    }
    if (revision == 1) {
      // GH1 Environ revision 1 still serializes an obsolete RndDrawable block.
      // Harmonix's later matching loader calls RndDrawable::DumpLoad here:
      // showing, old drawable list, and sphere are consumed but do not confer
      // membership. Preserve the refs for format evidence only.
      const uint16_t drawable_revision = low_revision(r.u32());
      if (drawable_revision >= 4) {
        throw std::runtime_error(
            "milo_scene: unsupported GH1 Environ drawable version");
      }
      env.legacy_drawable_showing = r.u8() != 0;
      if (drawable_revision < 2) {
        const uint32_t drawable_count = r.u32();
        if (drawable_count > 4096) {
          throw std::runtime_error(
              "milo_scene: implausible GH1 Environ drawable count");
        }
        env.legacy_drawable_refs.reserve(drawable_count);
        for (uint32_t i = 0; i < drawable_count; ++i) {
          std::string ref = r.str();
          if (!name_has_suffix(ref, ".mesh")) {
            throw std::runtime_error(
                "milo_scene: invalid GH1 Environ drawable ref");
          }
          env.legacy_drawable_refs.push_back(std::move(ref));
        }
      }
      if (drawable_revision > 0) r.skip(16);  // RndDrawable bounding sphere.
    } else {
      read_object_fields(r);
    }
    const uint32_t light_count = r.u32();
    if (light_count > 64) {
      throw std::runtime_error("milo_scene: implausible Environ light count");
    }
    env.lights.reserve(light_count);
    for (uint32_t i = 0; i < light_count; ++i) {
      std::string ref = r.str();
      if (!is_environ_light_ref(ref)) {
        throw std::runtime_error("milo_scene: invalid Environ light ref");
      }
      env.lights.push_back(std::move(ref));
    }

    for (int i = 0; i < 4; ++i) {
      env.color_a[i] = r.f32();
      if (!std::isfinite(env.color_a[i])) {
        throw std::runtime_error("milo_scene: non-finite Environ color_a");
      }
    }
    env.range_a = r.f32();
    env.range_b = r.f32();
    env.fog_start = env.range_a;
    env.fog_end = env.range_b;
    for (int i = 0; i < 4; ++i) {
      env.color_b[i] = r.f32();
      if (!std::isfinite(env.color_b[i])) {
        throw std::runtime_error("milo_scene: non-finite Environ color_b");
      }
      env.fog_color[i] = env.color_b[i];
    }
    env.fog_enabled = r.u8() != 0;
    if (revision >= 5) {
      env.animate_from_preset = r.u8() != 0;
      env.fade_out = r.u8() != 0;
      env.fade_start = r.f32();
      env.fade_end = r.f32();
      env.range = env.fade_end;
    } else {
      env.fade_start = env.range_a;
      env.fade_end = env.range_b;
      env.range = env.range_b;
    }
    if (!std::isfinite(env.range_a) || !std::isfinite(env.range_b) ||
        !std::isfinite(env.fade_start) || !std::isfinite(env.fade_end) ||
        env.range_a < 0.0f || env.range_b < 0.0f ||
        env.fade_start < 0.0f || env.fade_end < 0.0f) {
      throw std::runtime_error("milo_scene: invalid Environ range");
    }
    env.source_order_decoded = true;
    env.decoded = true;
  } catch (const std::exception& ex) {
    env.error = ex.what();
  }
  return env;
}

GroupObj decode_group(const std::string& entry_name,
                      const std::vector<uint8_t>& body,
                      int32_t parent_dir_revision) {
  GroupObj group;
  group.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    const uint32_t combined_revision = r.u32();
    const uint16_t ver = static_cast<uint16_t>(combined_revision & 0xffffu);
    if (parent_dir_revision == 10 && ver == 7) {
      group.legacy_view = true;
      // GH1 `View` is remapped to RndGroup by ObjectDir::DirLoader.  Its old
      // serialized layout carries a revision-0 RndAnimatable payload whose
      // animation-reference vector propagates SetFrame to nested
      // Views/animations. The drawable member vector follows embedded Trans8.
      auto decode_view_tail = [&](Reader& view_reader) {
        const TransFields trans = read_trans_block(
            view_reader, false, parent_dir_revision);
        group.local = trans.local;
        group.world_stored = trans.world;
        group.constraint = trans.constraint;
        group.target = trans.target;
        group.preserve_scale = trans.preserve_scale;
        group.parent = trans.parent;
        if (group.parent == group.name) group.parent.clear();
        group.has_transform = true;
        group.showing = view_reader.u8() != 0;
        view_reader.skip(4);  // legacy view flags
        const uint32_t object_count = view_reader.u32();
        if (object_count > 4096) {
          throw std::runtime_error(
              "milo_scene: implausible GH1 View member count");
        }
        group.children.clear();
        group.children.reserve(object_count);
        for (uint32_t i = 0; i < object_count; ++i) {
          std::string ref = view_reader.str();
          if (name_has_suffix(ref, ".env")) group.environment_ref = ref;
          group.children.push_back(std::move(ref));
        }
      };
      try {
        group.anim_children.clear();
        read_animatable_block(r, &group.anim_children);
        for (const auto& ref : group.anim_children)
          if (name_has_suffix(ref, ".env")) group.environment_ref = ref;
        decode_view_tail(r);
      } catch (const std::exception&) {
        // Animated GH1 Views serialize a legacy animatable payload between
        // View7 and embedded Trans8. Locate that strongly typed Trans block
        // rather than assuming the empty-animatable layout used by most
        // character Views.
        bool decoded_tail = false;
        for (size_t offset = 4; offset + 4 + 96 <= body.size(); ++offset) {
          uint32_t revision = 0;
          std::memcpy(&revision, body.data() + offset, sizeof(revision));
          if ((revision & 0xffffu) != 8 ||
              !is_plausible_matrix_at(body, offset + 4) ||
              !is_plausible_matrix_at(body, offset + 52)) {
            continue;
          }
          try {
            Reader candidate(body.data() + offset, body.size() - offset);
            decode_view_tail(candidate);
            decoded_tail = true;
            break;
          } catch (const std::exception&) {
          }
        }
        if (!decoded_tail) {
          throw std::runtime_error(
              "milo_scene: no valid GH1 View embedded Trans8");
        }
      }
      group.source_order_decoded = true;
      group.decoded = true;
      return group;
    }
    if (ver > 7) read_object_fields(r);
    read_animatable_block(r);
    const TransFields trans = read_trans_block(r, false, parent_dir_revision);
    group.local = trans.local;
    group.world_stored = trans.world;
    group.constraint = trans.constraint;
    group.target = trans.target;
    group.preserve_scale = trans.preserve_scale;
    group.parent = trans.parent;
    group.has_transform = true;
    read_drawable_block(r, parent_dir_revision, group.showing,
                        group.draw_order);

    if (ver > 10) {
      const uint32_t object_count = r.u32();
      if (object_count > 4096) {
        throw std::runtime_error("milo_scene: implausible RndGroup object count");
      }
      group.children.reserve(object_count);
      for (uint32_t i = 0; i < object_count; ++i) {
        group.children.push_back(r.str());
      }
      if (ver < 16) group.environment_ref = r.str();
      if (ver > 12) group.draw_only = r.str();
    }

    if (ver > 11 && ver < 16) {
      group.lod = r.str();
      group.lod_screen_size = r.f32();
    } else if (ver == 4) {
      (void)r.u32();
      const uint32_t object_count = r.u32();
      if (object_count > 4096) {
        throw std::runtime_error("milo_scene: implausible RndGroup v4 object count");
      }
      group.children.reserve(object_count);
      for (uint32_t i = 0; i < object_count; ++i) {
        group.children.push_back(r.str());
      }
      (void)r.str();
      (void)r.u32();
      (void)r.u32();
    }

    if (ver == 7) {
      (void)r.str();
      (void)r.f32();
      (void)r.f32();
    }
    if (ver > 13) group.sort_in_world = r.u8() != 0;
    group.source_order_decoded = true;
    group.decoded = true;
  } catch (const std::exception& ex) {
    group.error = ex.what();
  }
  return group;
}

SourceRndDrawableLoadPlan source_rnddrawable_load_plan(
    int32_t revision,
    int32_t parent_revision) {
  SourceRndDrawableLoadPlan plan;
  plan.revision = revision;
  plan.parent_revision = parent_revision;
  plan.accepted_revision = revision >= 0 && revision <= 3;
  if (!plan.accepted_revision) {
    plan.reads_showing = false;
    return plan;
  }
  plan.reads_old_drawable_list = revision < 2;
  plan.old_list_is_null_terminated_strings =
      plan.reads_old_drawable_list && parent_revision <= 6;
  plan.old_list_is_symbols =
      plan.reads_old_drawable_list && parent_revision > 6;
  plan.reads_sphere = revision > 0;
  plan.reads_draw_order = revision > 2;
  return plan;
}

SourceRndDrawableDefaultState source_rnddrawable_default_state() {
  return SourceRndDrawableDefaultState{};
}

SourceMiloEditorRndDrawableNewPlan source_milo_editor_rnddrawable_new_plan(
    int32_t revision,
    int32_t alt_revision) {
  SourceMiloEditorRndDrawableNewPlan plan;
  plan.revision = revision;
  plan.alt_revision = alt_revision;
  return plan;
}

SourceRndDrawableSavePlan source_rnddrawable_save_plan() {
  return SourceRndDrawableSavePlan{};
}

SourceRndDrawableDrawPlan source_rnddrawable_draw_plan(
    bool showing,
    bool has_world_sphere,
    bool sphere_culled) {
  SourceRndDrawableDrawPlan plan;
  plan.showing = showing;
  plan.has_world_sphere = has_world_sphere;
  plan.sphere_culled = sphere_culled;
  if (!showing) return plan;
  plan.calls_make_world_sphere = true;
  plan.calls_draw_showing = !has_world_sphere || !sphere_culled;
  return plan;
}

SourceRndDrawableBudgetPlan source_rnddrawable_budget_plan(
    bool showing,
    bool has_world_sphere,
    bool sphere_culled) {
  SourceRndDrawableBudgetPlan plan;
  if (!showing) return plan;
  plan.calls_make_world_sphere = true;
  plan.calls_draw_showing_budget = !has_world_sphere || !sphere_culled;
  return plan;
}

SourceRndDrawableCopyPlan source_rnddrawable_copy_plan(
    bool copy_from_max,
    bool dest_sphere_nonzero,
    bool source_sphere_nonzero) {
  SourceRndDrawableCopyPlan plan;
  if (!copy_from_max) {
    plan.normal_members = {"mShowing", "mOrder", "mSphere"};
    return plan;
  }
  if (dest_sphere_nonzero && source_sphere_nonzero) {
    plan.from_max_members = {"mSphere"};
  }
  return plan;
}

SourceRndDrawableCollidePlan source_rnddrawable_collide_plan(
    bool showing,
    bool has_world_sphere,
    bool sphere_intersects,
    float plane_dot,
    float sphere_radius) {
  SourceRndDrawableCollidePlan plan;
  plan.showing = showing;
  plan.has_world_sphere = has_world_sphere;
  plan.sphere_intersects = sphere_intersects;
  if (showing) {
    plan.collide_sphere_result = !has_world_sphere || sphere_intersects;
    plan.collide_calls_showing = plan.collide_sphere_result;
  }
  if (!showing || !has_world_sphere) {
    plan.collide_plane_result = -1;
  } else if (plane_dot >= sphere_radius) {
    plan.collide_plane_result = 1;
  } else {
    plan.collide_plane_result = sphere_radius < -plane_dot ? -1 : 0;
  }
  return plan;
}

SourceRndDrawableHandlerPlan source_rnddrawable_handler_plan() {
  SourceRndDrawableHandlerPlan plan;
  plan.handlers = {"set_showing", "showing", "zero_sphere", "update_sphere",
                   "get_sphere",  "copy_sphere"};
  plan.check = 0x168;
  return plan;
}

SourceRndDrawablePropSyncPlan source_rnddrawable_prop_sync_plan() {
  SourceRndDrawablePropSyncPlan plan;
  plan.properties = {"draw_order", "showing", "sphere"};
  plan.showing_ops = {"set:mShowing=_val.Int(0)!=0",
                      "get:_val=DataNode(mShowing)"};
  return plan;
}

SourceRndGroupLoadPlan source_rndgroup_load_plan(int32_t revision) {
  SourceRndGroupLoadPlan plan;
  plan.revision = revision;
  plan.reads_object_fields = revision > 7;
  plan.reads_objects = revision > 10;
  plan.reads_environ = revision > 10 && revision < 16;
  plan.reads_draw_only = revision > 12;
  plan.reads_lod = revision > 11 && revision < 16;
  plan.reads_legacy_rev4_objects = revision == 4;
  plan.reads_rev7_lod_dimensions = revision == 7;
  plan.reads_sort_in_world = revision > 13;
  return plan;
}

SourceRndGroupDefaultState source_rndgroup_default_state() {
  return SourceRndGroupDefaultState{};
}

SourceMiloEditorRndGroupNewPlan source_milo_editor_rndgroup_new_plan(
    int32_t revision,
    int32_t alt_revision) {
  SourceMiloEditorRndGroupNewPlan plan;
  plan.revision = revision;
  plan.alt_revision = alt_revision;
  return plan;
}

SourceRndGroupSavePlan source_rndgroup_save_plan() {
  return SourceRndGroupSavePlan{};
}

SourceRndGroupCopyPlan source_rndgroup_copy_plan() {
  SourceRndGroupCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndAnimatable", "RndDrawable",
                       "RndTransformable"};
  plan.member_order = {"mEnv", "mDrawOnly", "mLod", "mLodScreenSize",
                       "mSortInWorld", "mObjects"};
  return plan;
}

SourceRndGroupReplacePlan source_rndgroup_replace_plan(bool object_found) {
  SourceRndGroupReplacePlan plan;
  plan.add_object_when_found = object_found;
  plan.sets_in_replace_around_remove = object_found;
  plan.remove_object_when_found = object_found;
  plan.no_object_no_membership_change = !object_found;
  return plan;
}

SourceRndGroupHandlerPlan source_rndgroup_handler_plan() {
  SourceRndGroupHandlerPlan plan;
  plan.actions = {"sort_draws", "add_object", "remove_object",
                  "clear_objects"};
  plan.queries = {"get_draws", "has_object"};
  plan.superclasses = {"RndAnimatable", "RndDrawable", "RndTransformable",
                       "Hmx::Object"};
  return plan;
}

SourceRndGroupPropSyncPlan source_rndgroup_prop_sync_plan() {
  SourceRndGroupPropSyncPlan plan;
  plan.props = {"objects", "environ", "draw_only", "lod",
                "lod_screen_size", "sort_in_world"};
  plan.side_effects = {"objects:Update", "lod:Update",
                       "lod_screen_size:UpdateLODState"};
  plan.superclasses = {"RndDrawable", "RndTransformable", "RndAnimatable"};
  return plan;
}

SourceRndMeshDeformVertArrayState
source_rndmesh_deform_vert_array_default_state(bool parent_provided) {
  SourceRndMeshDeformVertArrayState state;
  state.parent_set = parent_provided;
  return state;
}

SourceRndMeshDeformVertArraySetSizePlan
source_rndmesh_deform_vert_array_set_size_plan(int32_t old_size,
                                               int32_t new_size) {
  SourceRndMeshDeformVertArraySetSizePlan plan;
  plan.old_size = old_size;
  plan.new_size = new_size;
  plan.changes_size = old_size != new_size;
  plan.frees_existing_data = plan.changes_size;
  plan.allocates_requested_size = plan.changes_size;
  return plan;
}

SourceRndMeshDeformClearPlan source_rndmesh_deform_clear_plan(
    int32_t old_size) {
  SourceRndMeshDeformClearPlan plan;
  plan.changes_size = old_size != 0;
  return plan;
}

SourceRndMeshDeformDefaultState source_rndmesh_deform_default_state() {
  return SourceRndMeshDeformDefaultState{};
}

SourceRndMeshDeformSetMeshPlan source_rndmesh_deform_set_mesh_plan() {
  return SourceRndMeshDeformSetMeshPlan{};
}

SourceRndMeshDeformHandlerPlan source_rndmesh_deform_handler_plan() {
  SourceRndMeshDeformHandlerPlan plan;
  plan.superclasses = {"Hmx::Object"};
  return plan;
}

SourceRndMeshDeformBodyAvailability
source_rndmesh_deform_body_availability() {
  return SourceRndMeshDeformBodyAvailability{};
}

SourceRndMultiMeshDefaultState source_rndmultimesh_default_state() {
  return SourceRndMultiMeshDefaultState{};
}

SourceRndMultiMeshInstanceDefaultState
source_rndmultimesh_instance_default_state() {
  return SourceRndMultiMeshInstanceDefaultState{};
}

SourceRndMultiMeshLoadPlan source_rndmultimesh_load_plan(int32_t revision) {
  SourceRndMultiMeshLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 4;
  plan.reads_object_fields = revision != 0;
  plan.reads_legacy_transform_dump_and_returns = revision < 2;
  plan.reads_instances = revision >= 2;
  plan.reads_legacy_tail_byte = revision >= 2 && revision < 4;
  return plan;
}

SourceRndMultiMeshCopyPlan source_rndmultimesh_copy_plan(
    bool copy_from_max) {
  SourceRndMultiMeshCopyPlan plan;
  plan.superclasses = {"Hmx::Object", "RndDrawable"};
  plan.copies_mesh = !copy_from_max;
  return plan;
}

SourceRndMultiMeshSetMeshPlan source_rndmultimesh_set_mesh_plan() {
  return SourceRndMultiMeshSetMeshPlan{};
}

SourceRndMultiMeshHandlerPlan source_rndmultimesh_handler_plan() {
  SourceRndMultiMeshHandlerPlan plan;
  plan.handlers = {"move_xfms",  "scale_xfms",  "sort_xfms",
                   "random_xfms", "scramble_xfms", "distribute",
                   "get_pos",    "set_pos",     "get_rot",
                   "set_rot",    "get_scale",   "set_scale",
                   "mesh",       "add_xfm",     "add_xfms",
                   "remove_xfm", "num_xfms"};
  plan.actions = {"set_mesh"};
  plan.superclasses = {"RndDrawable", "Hmx::Object"};
  return plan;
}

SourceRndMultiMeshSetPosPlan source_rndmultimesh_set_pos_plan(
    int32_t requested_index) {
  SourceRndMultiMeshSetPosPlan plan;
  plan.requested_index = requested_index;
  plan.assignment_order = {"read_z", "read_y", "read_x",
                           "write_x", "write_y", "write_z"};
  return plan;
}

SourceRndMultiMeshPropSyncPlan source_rndmultimesh_prop_sync_plan() {
  SourceRndMultiMeshPropSyncPlan plan;
  plan.superclasses = {"RndDrawable"};
  return plan;
}

SourceRndMultiMeshProxyDefaultState
source_rndmultimesh_proxy_default_state() {
  return SourceRndMultiMeshProxyDefaultState{};
}

SourceRndMultiMeshProxySetPlan source_rndmultimesh_proxy_set_plan(
    bool has_mesh) {
  SourceRndMultiMeshProxySetPlan plan;
  plan.has_mesh = has_mesh;
  plan.copies_instance_local_transform = has_mesh;
  return plan;
}

SourceRndMultiMeshProxyDrawPlan source_rndmultimesh_proxy_draw_plan(
    bool has_multimesh,
    bool has_mesh) {
  SourceRndMultiMeshProxyDrawPlan plan;
  plan.has_multimesh = has_multimesh;
  plan.has_mesh = has_mesh;
  plan.reads_multimesh_mesh = has_multimesh;
  plan.sets_mesh_world_from_instance = has_multimesh && has_mesh;
  plan.draws_mesh = has_multimesh && has_mesh;
  return plan;
}

SourceRndMultiMeshProxyUpdatedWorldPlan
source_rndmultimesh_proxy_updated_world_plan(bool has_multimesh) {
  SourceRndMultiMeshProxyUpdatedWorldPlan plan;
  plan.has_multimesh = has_multimesh;
  plan.writes_instance_from_world = has_multimesh;
  return plan;
}

SourceRndMultiMeshProxyFailurePlan
source_rndmultimesh_proxy_failure_plan() {
  return SourceRndMultiMeshProxyFailurePlan{};
}

SourceRndMultiMeshProxyHandlerPlan
source_rndmultimesh_proxy_handler_plan() {
  return SourceRndMultiMeshProxyHandlerPlan{};
}

SourceRndMultiMeshProxyPropSyncPlan
source_rndmultimesh_proxy_prop_sync_plan() {
  return SourceRndMultiMeshProxyPropSyncPlan{};
}

SourceRndWindDefaultState source_rndwind_default_state() {
  return SourceRndWindDefaultState{};
}

SourceRndWindSetDefaultsPlan source_rndwind_set_defaults_plan() {
  return SourceRndWindSetDefaultsPlan{};
}

SourceRndWindZeroPlan source_rndwind_zero_plan() {
  return SourceRndWindZeroPlan{};
}

SourceRndWindLoopRatePlan source_rndwind_sync_loops(float time_loop,
                                                    float space_loop) {
  SourceRndWindLoopRatePlan plan;
  plan.time_loop = time_loop;
  plan.space_loop = space_loop;
  const float time_rate = time_loop == 0.0f ? 0.0f : 1.0f / time_loop;
  const float space_rate = space_loop == 0.0f ? 0.0f : 1.0f / space_loop;
  plan.time_loop_zero = time_loop == 0.0f;
  plan.space_loop_zero = space_loop == 0.0f;
  plan.time_rate[0] = time_rate;
  plan.time_rate[1] = time_rate * 0.773437f;
  plan.time_rate[2] = time_rate * 1.38484f;
  plan.space_rate[0] = space_rate;
  plan.space_rate[1] = space_rate * 0.773437f;
  plan.space_rate[2] = space_rate * 1.38484f;
  return plan;
}

SourceRndWindSavePlan source_rndwind_save_plan() {
  return SourceRndWindSavePlan{};
}

SourceRndWindLoadPlan source_rndwind_load_plan(int32_t revision) {
  SourceRndWindLoadPlan plan;
  plan.revision = revision;
  plan.accepted_revision = revision >= 0 && revision <= 2;
  plan.reads_wind_owner = revision > 1;
  plan.calls_set_wind_owner = plan.reads_wind_owner;
  return plan;
}

SourceRndWindSetOwnerPlan source_rndwind_set_owner_plan(
    bool input_owner_present) {
  SourceRndWindSetOwnerPlan plan;
  plan.input_owner_present = input_owner_present;
  plan.assigns_input_owner = input_owner_present;
  plan.assigns_self = !input_owner_present;
  return plan;
}

SourceRndWindCopyPlan source_rndwind_copy_plan(bool copy_shallow) {
  SourceRndWindCopyPlan plan;
  plan.copy_shallow = copy_shallow;
  plan.shallow_copies_wind_owner = copy_shallow;
  plan.resets_wind_owner_to_self = !copy_shallow;
  plan.copies_wind_owner = !copy_shallow;
  plan.copies_prevailing = !copy_shallow;
  plan.copies_random = !copy_shallow;
  plan.copies_time_loop = !copy_shallow;
  plan.copies_space_loop = !copy_shallow;
  plan.calls_sync_loops = !copy_shallow;
  return plan;
}

SourceRndWindReplacePlan source_rndwind_replace_plan(
    bool wind_owner_matches_from,
    bool replacement_is_wind) {
  SourceRndWindReplacePlan plan;
  plan.wind_owner_matches_from = wind_owner_matches_from;
  plan.replacement_is_wind = replacement_is_wind;
  plan.calls_set_wind_owner = wind_owner_matches_from;
  plan.assigns_replacement_wind =
      wind_owner_matches_from && replacement_is_wind;
  plan.assigns_self = wind_owner_matches_from && !replacement_is_wind;
  return plan;
}

SourceRndWindRuntimeBoundary source_rndwind_runtime_boundary() {
  return SourceRndWindRuntimeBoundary{};
}

SourceRndWindHandlerPlan source_rndwind_handler_plan() {
  SourceRndWindHandlerPlan plan;
  plan.actions = {"set_defaults", "set_zero"};
  plan.superclasses = {"Hmx::Object"};
  return plan;
}

SourceRndWindPropSyncPlan source_rndwind_prop_sync_plan() {
  SourceRndWindPropSyncPlan plan;
  plan.direct_rows = {"prevailing", "random"};
  plan.set_rows = {"wind_owner"};
  plan.modify_rows = {"time_loop", "space_loop"};
  return plan;
}

SourceRndMatLoadPlan source_rndmat_load_plan(int32_t revision) {
  SourceRndMatLoadPlan plan;
  plan.revision = revision;
  plan.reads_modern_render_state = revision > 21;
  plan.reads_use_environ = plan.reads_modern_render_state;
  plan.reads_prelit = plan.reads_modern_render_state;
  plan.reads_z_mode = plan.reads_modern_render_state;
  plan.reads_alpha_cut = plan.reads_modern_render_state;
  plan.reads_alpha_threshold = revision > 0x25;
  plan.reads_alpha_write = plan.reads_modern_render_state;
  plan.reads_tex_gen = plan.reads_modern_render_state;
  plan.reads_tex_wrap = plan.reads_modern_render_state;
  plan.reads_tex_xfm = plan.reads_modern_render_state;
  plan.reads_diffuse_tex = plan.reads_modern_render_state;
  plan.reads_next_pass = plan.reads_modern_render_state;
  plan.reads_intensify = plan.reads_modern_render_state;
  plan.reads_cull = plan.reads_modern_render_state;
  plan.reads_emissive_multiplier = plan.reads_modern_render_state;
  plan.gh2_v27_has_no_alpha_threshold =
      revision == 27 && !plan.reads_alpha_threshold;
  if (plan.reads_modern_render_state) {
    plan.modern_order = {
        "blend",       "color",       "use_environ", "prelit",
        "z_mode",      "alpha_cut",   "alpha_write", "tex_gen",
        "tex_wrap",    "tex_xfm",     "diffuse_tex", "next_pass",
        "intensify",   "cull",        "emissive_multiplier"};
    if (plan.reads_alpha_threshold) {
      plan.modern_order.insert(plan.modern_order.begin() + 6,
                               "alpha_threshold");
    }
  }
  return plan;
}

SourceRndMatDefaultState source_rndmat_default_state() {
  return SourceRndMatDefaultState{};
}

SourceRndMatSavePlan source_rndmat_save_plan() {
  return SourceRndMatSavePlan{};
}

SourceMiloEditorRndMatNewPlan source_milo_editor_rndmat_new_plan(
    int32_t revision,
    int32_t alt_revision) {
  SourceMiloEditorRndMatNewPlan plan;
  plan.revision = revision;
  plan.alt_revision = alt_revision;
  return plan;
}

SourceMatShaderOptionsDefaultState source_mat_shader_options_default_state() {
  return SourceMatShaderOptionsDefaultState{};
}

SourceMatPerfSettingsDefaultState source_mat_perf_settings_default_state() {
  return SourceMatPerfSettingsDefaultState{};
}

SourceMatPerfSettingsLoadPlan source_mat_perf_settings_load_plan(
    int32_t revision) {
  SourceMatPerfSettingsLoadPlan plan;
  plan.revision = revision;
  plan.read_order = {"recv_proj_lights", "ps3_force_trilinear"};
  plan.reads_recv_point_cube_tex = revision > 0x41;
  if (plan.reads_recv_point_cube_tex) {
    plan.read_order.push_back("recv_point_cube_tex");
  }
  return plan;
}

SourceRndMatAccessorResult source_rndmat_accessors(const MatObj& mat) {
  SourceRndMatAccessorResult out;
  out.blend = mat.blend;
  out.z_mode = mat.z_mode;
  out.tex_wrap = mat.tex_wrap;
  out.diffuse_tex = mat.diffuse_tex;
  out.next_pass = mat.next_pass;
  out.alpha = mat.color[3];
  return out;
}

SourceRndMatSetterPlan source_rndmat_setter_plan(
    const std::string& setter) {
  SourceRndMatSetterPlan plan;
  plan.setter = setter;
  if (setter == "SetTexXfm" || setter == "SetZMode" ||
      setter == "SetDiffuseTex" || setter == "SetUseEnv" ||
      setter == "SetPreLit" || setter == "SetBlend" ||
      setter == "SetAlphaCut" || setter == "SetTexWrap" ||
      setter == "SetPerPixelLit") {
    plan.writes_member = true;
    plan.dirty_or_mask = 2;
  } else if (setter == "SetAlpha") {
    plan.writes_member = true;
    plan.writes_alpha_only = true;
    plan.dirty_or_mask = 1;
  } else if (setter == "SetColor") {
    plan.writes_member = true;
    plan.writes_rgb_only = true;
    plan.dirty_or_mask = 1;
  } else if (setter == "SetAlphaThreshold" ||
             setter == "SetPointLights") {
    plan.writes_member = true;
  }
  return plan;
}

SourceRndMatColorModPlan source_rndmat_set_color_mod_plan(int32_t index) {
  SourceRndMatColorModPlan plan;
  plan.index = index;
  plan.assertion_would_fail = index < 0 || index >= 3;
  if (!plan.assertion_would_fail) {
    plan.writes_color_mod = true;
    plan.dirty_or_mask = 2;
  }
  return plan;
}

SourceRndMatRefractEnabledPlan source_rndmat_get_refract_enabled_plan(
    bool refract_enabled,
    float refract_strength,
    bool has_refract_normal_map,
    bool allow_without_current_frame_tex,
    bool has_current_frame_tex) {
  SourceRndMatRefractEnabledPlan plan;
  plan.refract_enabled = refract_enabled;
  plan.refract_strength = refract_strength;
  plan.has_refract_normal_map = has_refract_normal_map;
  plan.allow_without_current_frame_tex = allow_without_current_frame_tex;
  plan.has_current_frame_tex = has_current_frame_tex;
  plan.base_gate =
      refract_enabled && refract_strength > 0.0f && has_refract_normal_map;
  plan.frame_gate = allow_without_current_frame_tex || has_current_frame_tex;
  plan.result = plan.base_gate && plan.frame_gate;
  return plan;
}

SourceRndMatRefractAccessorPlan source_rndmat_refract_accessor_plan() {
  return SourceRndMatRefractAccessorPlan{};
}

SourceRndMatIsNextPassPlan source_rndmat_is_next_pass_plan(
    const std::vector<std::string>& next_pass_chain,
    const std::string& candidate) {
  SourceRndMatIsNextPassPlan plan;
  plan.candidate = candidate;
  plan.chain = next_pass_chain;
  plan.found =
      std::find(next_pass_chain.begin(), next_pass_chain.end(), candidate) !=
      next_pass_chain.end();
  return plan;
}

SourceRndMatAllowedNextPassPlan source_rndmat_allowed_next_pass_plan(
    const std::vector<std::string>& directory_mats,
    const std::string& current_next_pass,
    const std::vector<std::string>& recursive_next_passes) {
  SourceRndMatAllowedNextPassPlan plan;
  plan.mat_count = static_cast<int32_t>(directory_mats.size());
  plan.allocated_node_count = plan.mat_count + 2;
  plan.allowed_order.push_back("<null>");
  if (!current_next_pass.empty()) {
    plan.preserves_current_next_pass = true;
    plan.allowed_order.push_back(current_next_pass);
  }

  for (const std::string& mat : directory_mats) {
    const SourceRndMatIsNextPassPlan next_pass =
        source_rndmat_is_next_pass_plan(recursive_next_passes, mat);
    if (!next_pass.found) {
      plan.allowed_order.push_back(mat);
    }
  }
  plan.resized_node_count = static_cast<int32_t>(plan.allowed_order.size());
  return plan;
}

SourceRndMatAllowedNormalMapPlan source_rndmat_allowed_normal_map_plan() {
  return SourceRndMatAllowedNormalMapPlan{};
}

SourceRndMatHandlerPlan source_rndmat_handler_plan() {
  SourceRndMatHandlerPlan plan;
  plan.handlers = {"allowed_next_pass", "allowed_normal_map"};
  plan.superclasses = {"Hmx::Object"};
  return plan;
}

SourceRndMatCopyPlan source_rndmat_copy_plan(bool copy_from_max) {
  SourceRndMatCopyPlan plan;
  plan.copy_from_max = copy_from_max;
  plan.copies_diffuse_tex = copy_from_max;
  return plan;
}

SourceRndMatPropSyncPlan source_rndmat_prop_sync_plan() {
  SourceRndMatPropSyncPlan plan;
  plan.dirty_color_rows = {"color", "alpha"};
  plan.dirty_render_rows = {
      "intensify",       "use_environ",      "blend",
      "z_mode",          "stencil_mode",     "tex_gen",
      "tex_wrap",        "shader_variation", "tex_xfm",
      "diffuse_tex",     "prelit",           "alpha_cut",
      "alpha_threshold", "alpha_write",      "cull",
      "per_pixel_lit",   "emissive_multiplier",
      "emissive_map",    "refract_enabled",  "refract_strength",
      "refract_normal_map", "screen_aligned"};
  plan.direct_no_dirty_rows = {"next_pass", "point_lights", "fog",
                               "fade_out", "color_adjust"};
  plan.perf_setting_rows = {"recv_proj_lights", "recv_point_cube_tex",
                            "ps3_force_trilinear"};
  return plan;
}

MatObj decode_mat(const std::string& entry_name,
                  const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  MatObj m;
  m.name = entry_name;
  const int32_t ver = r.i32();     // = 27 for GH2 PS2 stock character mats.
  const SourceRndMatLoadPlan plan = source_rndmat_load_plan(ver);
  if (ver == 21) {
    // GH1 version-10 directories do not serialize Hmx::Object metadata inside
    // entry bodies. RndMat revision 21 begins immediately with a texture-map
    // array, followed by blend, RGB, and alpha. This is the source order used
    // by MiloLib/Grim's explicit GH1 reader.
    bool has_legacy_environment_texture = false;
    const uint32_t tex_count = r.u32();
    if (tex_count > 64) {
      throw std::runtime_error("milo_scene: implausible GH1 Mat texture count");
    }
    for (uint32_t i = 0; i < tex_count; ++i) {
      const uint32_t map_slot = r.u32();
      const uint32_t map_type = r.u32();
      float tex_xfm[12] = {};
      for (float& value : tex_xfm) value = r.f32();
      const uint32_t tex_wrap = r.u32();
      const std::string texture = r.str();
      (void)map_slot;
      // GH1 Mat21 uses both selector 0 and selector 1 for the material's
      // sampled 2-D texture. Real small_club selector-1 entries include
      // smokemat, color_plane, and spot_beam_mat; dropping them produces
      // untextured additive polygons. Selector 5 is the sphere/environment
      // map family. Mackiloha likewise exposes the first non-empty legacy
      // TextureEntry as the material texture rather than requiring selector 0.
      if ((map_type == 0 || map_type == 1) && m.diffuse_tex.empty()) {
        m.diffuse_tex = texture;
        m.tex_wrap = static_cast<uint8_t>(
            tex_wrap <= 4 ? tex_wrap : 1);
        m.tex_xfm[0][0] = tex_xfm[0];
        m.tex_xfm[0][1] = tex_xfm[1];
        m.tex_xfm[0][2] = 0.0f;
        m.tex_xfm[1][0] = tex_xfm[3];
        m.tex_xfm[1][1] = tex_xfm[4];
        m.tex_xfm[1][2] = 0.0f;
        m.tex_xfm[2][0] = tex_xfm[9];
        m.tex_xfm[2][1] = tex_xfm[10];
        m.tex_xfm[2][2] = 1.0f;
        m.tex_scale[0] = m.tex_xfm[0][0];
        m.tex_scale[1] = m.tex_xfm[1][1];
        m.tex_offset[0] = m.tex_xfm[2][0];
        m.tex_offset[1] = m.tex_xfm[2][1];
      } else if (map_type == 5) {
        has_legacy_environment_texture =
            has_legacy_environment_texture || !texture.empty();
      }
    }
    const uint32_t primary_blend = r.u32();
    if (primary_blend <= 6) m.legacy_primary_blend =
        static_cast<uint8_t>(primary_blend);
    m.color[0] = r.f32();
    m.color[1] = r.f32();
    m.color[2] = r.f32();
    m.color[3] = r.f32();
    // Mat21 legacy tail (gh1_mat.bt / Mackiloha MatSerializer): the first
    // three bytes retain the old compact RndMat state ordering documented by
    // RndMat::Load after colour: use-environ, prelit, then ZMode.  Reading the
    // latter pair as one little-endian short explains the retail 0x0000,
    // 0x0001, 0x0100, and 0x0101 values without inventing material-name rules.
    m.use_environ = (r.u8() != 0) || has_legacy_environment_texture;
    const uint16_t prelit_zmode = r.u16();
    m.prelit = (prelit_zmode & 0xffu) != 0;
    const uint8_t z_mode = static_cast<uint8_t>(prelit_zmode >> 8);
    if (z_mode <= 4) m.z_mode = z_mode;
    (void)r.i32();
    (void)r.u16();
    const uint32_t blend = r.u32();
    if (blend <= 6) {
      m.blend = static_cast<uint8_t>(blend);
      m.legacy_tail_blend = static_cast<uint8_t>(blend);
    }
    m.has_legacy_blends = primary_blend <= 6 && blend <= 6;
    (void)r.u16();
    m.decoded = true;
    return m;
  }
  read_object_fields(r);     // base metadata
  const uint32_t blend = plan.reads_blend ? r.u32() : 0;
  if (blend <= 6) {
    // macros.dta BLEND_ENUM. This precedes colour in real GH2 PS2 Mat entries:
    // kBlendDest/Src/Add/SrcAlpha/SrcAlphaAdd/Subtract/Multiply.
    m.blend = static_cast<uint8_t>(blend);
  }
  m.color[0] = r.f32();
  m.color[1] = r.f32();
  m.color[2] = r.f32();
  m.color[3] = r.f32();
  const size_t flag_pos = r.pos;
  // Diffuse texcoord transform: 16 bytes of flags, then a 12-float source
  // matrix block. Renderers consume the 2-D UV rows as [u v 1] * a 3x3 matrix;
  // the source third-axis slot can carry non-UV scale, so force homogeneous
  // [2][2] to one instead of rejecting the authored UV transform.
  // The flag / texture-state bytes follow the colour. We don't need their exact split
  // to draw; the diffuse texture name is the load-bearing field. Scan forward
  // from here for the first length-prefixed ".tex" string — robust against the
  // version-specific flag block between colour and the texture reference.
  if (plan.reads_modern_render_state) {
    m.use_environ = r.u8() != 0;
    m.prelit = r.u8() != 0;
    const int32_t z_mode = r.i32();
    m.z_mode = static_cast<uint8_t>(
        (z_mode >= 0 && z_mode <= 4) ? z_mode : 1);
    m.alpha_cut = r.u8() != 0;
    if (plan.reads_alpha_threshold) m.alpha_threshold = r.i32();
    m.alpha_write = r.u8() != 0;
    const int32_t tex_gen = r.i32();
    m.tex_gen = static_cast<uint8_t>(
        (tex_gen >= 0 && tex_gen <= 5) ? tex_gen : 0);
    const int32_t tex_wrap = r.i32();
    m.tex_wrap = static_cast<uint8_t>(
        (tex_wrap >= 0 && tex_wrap <= 4) ? tex_wrap : 1);
    const float tex_xfm[12] = {r.f32(), r.f32(), r.f32(),
                               r.f32(), r.f32(), r.f32(),
                               r.f32(), r.f32(), r.f32(),
                               r.f32(), r.f32(), r.f32()};
    bool sane_xfm = true;
    for (float value : tex_xfm) {
      if (!std::isfinite(value) || std::fabs(value) > 128.0f) {
        sane_xfm = false;
      }
    }
    if (sane_xfm) {
      m.tex_xfm[0][0] = tex_xfm[0];
      m.tex_xfm[0][1] = tex_xfm[1];
      m.tex_xfm[0][2] = 0.0f;
      m.tex_xfm[1][0] = tex_xfm[3];
      m.tex_xfm[1][1] = tex_xfm[4];
      m.tex_xfm[1][2] = 0.0f;
      m.tex_xfm[2][0] = tex_xfm[9];
      m.tex_xfm[2][1] = tex_xfm[10];
      m.tex_xfm[2][2] = 1.0f;
      m.tex_scale[0] = m.tex_xfm[0][0];
      m.tex_scale[1] = m.tex_xfm[1][1];
      m.tex_offset[0] = m.tex_xfm[2][0];
      m.tex_offset[1] = m.tex_xfm[2][1];
    }
    m.diffuse_tex_offset = static_cast<uint32_t>(r.pos);
    m.diffuse_tex = r.str();
    m.pre_diffuse_tex_bytes.assign(
        body.begin() + static_cast<std::ptrdiff_t>(flag_pos),
        body.begin() + static_cast<std::ptrdiff_t>(m.diffuse_tex_offset));
    const size_t after_diffuse = r.pos;
    if (after_diffuse < body.size()) {
      m.post_diffuse_tex_bytes.assign(
          body.begin() + static_cast<std::ptrdiff_t>(after_diffuse),
          body.end());
    }
    m.next_pass = r.str();
    m.intensify = r.u8() != 0;
    m.has_cull = true;
    m.cull = r.u8() != 0;
    m.emissive_multiplier = r.f32();
    m.has_render_state = true;
  }
  m.decoded = true;
  return m;
}

BandPlacerObj decode_band_placer(const std::string& entry_name,
                                 const std::vector<uint8_t>& body) {
  BandPlacerObj placer;
  placer.name = entry_name;
  try {
    if (body.size() > 0x10) {
      for (size_t kind_offset : {size_t(0x08), size_t(0x0c)}) {
        try {
          size_t probe = kind_offset;
          std::string kind = read_string_at(body, probe);
          if (!kind.empty()) {
            placer.kind = std::move(kind);
            break;
          }
        } catch (const std::exception&) {
        }
      }
    }

    int best_score = -1;
    size_t best_matrix = body.size();
    std::string best_parent;
    for (size_t version_offset = 4;
         version_offset + 4 + 96 + kObjMeta + 4 <= body.size();
         ++version_offset) {
      if (read_u32_at(body, version_offset) != 9) continue;
      const size_t matrix_offset = version_offset + 4;
      if (!is_plausible_matrix_at(body, matrix_offset) ||
          !is_plausible_matrix_at(body, matrix_offset + 48)) {
        continue;
      }
      size_t parent_offset = matrix_offset + 96 + kObjMeta;
      std::string parent;
      try {
        parent = read_string_at(body, parent_offset);
      } catch (const std::exception&) {
        continue;
      }
      if (!is_plausible_group_parent(parent)) continue;

      int score = 1;
      if (parent.find(".view") != std::string::npos) score += 20;
      if (parent.find(".grp") != std::string::npos) score += 12;
      if (parent.find(".mesh") != std::string::npos) score += 4;
      if (version_offset == 0x2a) score += 8;
      if (score > best_score) {
        best_score = score;
        best_matrix = matrix_offset;
        best_parent = std::move(parent);
      }
    }

    if (best_score < 0) {
      throw std::runtime_error("milo_scene: no BandPlacer transform pair");
    }
    placer.local = read_matrix_at(body, best_matrix);
    placer.world_stored = read_matrix_at(body, best_matrix + 48);
    placer.parent = std::move(best_parent);
    placer.decoded = true;
  } catch (const std::exception& ex) {
    placer.error = ex.what();
  }
  return placer;
}

MeshObj decode_mesh(const std::string& entry_name,
                    const std::vector<uint8_t>& body,
                    int32_t parent_dir_revision) {
  MeshObj mesh;
  mesh.name = entry_name;
  const char* decode_stage = "header";
  try {
    Reader r(body.data(), body.size());
    int32_t ver = r.i32();   // GH1=25, GH2=28.
    if (ver != 25 && ver != 28) {
      // Not fatal — some mesh variants exist — but record it.
      mesh.error = "unexpected mesh version " + std::to_string(ver);
    }
    // GH1 version-10 directories omit per-entry Hmx::Object metadata.
    if (parent_dir_revision >= 24) read_object_fields(r);
    decode_stage = "transform";
    const TransFields trans = read_trans_block(r, false, parent_dir_revision);
    mesh.local = trans.local;
    mesh.world_stored = trans.world;
    mesh.constraint = trans.constraint;
    mesh.target = trans.target;
    mesh.preserve_scale = trans.preserve_scale;
    mesh.parent = trans.parent;
    mesh.legacy_children = trans.legacy_children;

    // GH1 embeds Drawable revision 1; GH2 embeds revision 3.
    decode_stage = "drawable";
    read_drawable_block(r, parent_dir_revision, mesh.showing,
                        mesh.draw_order, &mesh.drawable_children);

    // Mesh fields.
    decode_stage = "mesh fields";
    mesh.material = r.str();           // material name
    if (ver == 27) r.str();            // legacy secondary material name
    mesh.geometry_owner = r.str();     // geometry-owner name (usually self)
    if (ver < 13) r.str();             // alt geom owner
    if (ver < 15) r.str();             // trans parent reference
    if (ver < 14) {
      r.str();
      r.str();
    }
    if (ver < 3) {
      (void)r.f32();
      (void)r.f32();
      (void)r.f32();
    }
    if (ver < 15) r.skip(16);
    if (ver < 8) (void)r.u8();
    if (ver < 15) {
      r.str();
      (void)r.f32();
    }
    if (ver < 16) {
      if (ver > 11) (void)r.u8();
    } else {
      mesh.mutable_flags = r.u32();
    }
    if (ver > 17) (void)r.u32();  // volume
    if (ver > 18) {
      const bool bsp_has_value = r.u8() != 0;
      if (bsp_has_value) {
        mesh.error = "unsupported non-empty BSP tree";
        return mesh;
      }
    }
    if (ver == 7) (void)r.u8();
    if (ver < 11) (void)r.u32();
    decode_stage = "vertex count";
    uint32_t vcount = r.u32();

    // Sanity-gate the vertex count against the remaining bytes: we need at least
    // vcount*48 + 4 (face count) more bytes in GH2 PS2 rev 28 source layout.
    if (static_cast<uint64_t>(vcount) * kGh2MeshVertexSourceStride + 4 >
        body.size() - r.pos) {
      mesh.error = "vertex_count " + std::to_string(vcount) + " exceeds entry";
      return mesh;
    }
    mesh.vertex_count = vcount;
    mesh.verts.resize(vcount);
    decode_stage = "vertices";
    for (uint32_t i = 0; i < vcount; ++i) {
      Vertex& v = mesh.verts[i];
      v.px = r.f32(); v.py = r.f32(); v.pz = r.f32();
      v.nx = r.f32(); v.ny = r.f32(); v.nz = r.f32();
      // ihatecompvir's RndMesh reader treats GH2 rev 28's pre-separate-color
      // slot as color first, then copies it into boneWeights when mBones exists.
      const float weight0 = r.f32();
      const float weight1 = r.f32();
      const float weight2 = r.f32();
      const float weight3 = r.f32();
      v.w[0] = weight0;
      v.w[1] = weight1;
      v.w[2] = weight2;
      v.w[3] = weight3;
      v.r = 1.0f; v.g = 1.0f; v.b = 1.0f; v.a = 1.0f;
      v.u  = r.f32(); v.v  = r.f32();
    }

    decode_stage = "faces";
    uint32_t fcount = r.u32();
    if (static_cast<uint64_t>(fcount) * 6 > body.size() - r.pos) {
      mesh.error = "face_count " + std::to_string(fcount) + " exceeds entry";
      return mesh;
    }
    mesh.face_count = fcount;
    mesh.indices.resize(static_cast<size_t>(fcount) * 3);
    for (uint32_t i = 0; i < fcount; ++i) {
      mesh.indices[i * 3 + 0] = r.u16();
      mesh.indices[i * 3 + 1] = r.u16();
      mesh.indices[i * 3 + 2] = r.u16();
    }

    // Validate all indices reference real vertices.
    for (uint16_t idx : mesh.indices) {
      if (idx >= vcount) {
        mesh.error = "face index out of range";
        return mesh;
      }
    }

    if (ver > 0x17 && r.pos + 4 <= body.size()) {
      const size_t group_size_pos = r.pos;
      const uint32_t group_sizes_count = r.u32();
      if (group_sizes_count <= body.size() - r.pos) {
        r.skip(group_sizes_count);
      } else {
        r.pos = group_size_pos;
      }
    }

    if (r.pos + 4 <= body.size()) {
      const size_t bone_probe = r.pos;
      const int32_t bone_marker = r.i32();
      if (bone_marker > 0) {
        r.pos = bone_probe;
        if (ver >= 33) {
          const uint32_t bone_count = r.u32();
          if (bone_count <= 64 &&
              static_cast<uint64_t>(bone_count) * (4 + 48) <= body.size() - r.pos) {
            mesh.bones.reserve(bone_count);
            for (uint32_t i = 0; i < bone_count; ++i) {
              BoneTransform bone;
              bone.name = r.str();
              bone.offset = r.matrix();
              if (!bone.name.empty()) mesh.bones.push_back(std::move(bone));
            }
          }
        } else if (r.pos < body.size()) {
          std::array<std::string, 4> names{};
          std::array<Xfm, 4> offsets{};
          for (std::string& name : names) name = r.str();
          for (Xfm& offset : offsets) offset = r.matrix();
          for (size_t i = 0; i < names.size(); ++i) {
            if (names[i].empty()) break;
            mesh.bones.push_back(BoneTransform{std::move(names[i]), offsets[i]});
          }
        }
      }
    }

    // Bounding box.
    if (vcount > 0) {
      mesh.bb_min[0] = mesh.bb_max[0] = mesh.verts[0].px;
      mesh.bb_min[1] = mesh.bb_max[1] = mesh.verts[0].py;
      mesh.bb_min[2] = mesh.bb_max[2] = mesh.verts[0].pz;
      for (const Vertex& v : mesh.verts) {
        const float xyz[3] = {v.px, v.py, v.pz};
        for (int k = 0; k < 3; ++k) {
          if (!std::isfinite(xyz[k])) { mesh.error = "non-finite vertex"; return mesh; }
          if (xyz[k] < mesh.bb_min[k]) mesh.bb_min[k] = xyz[k];
          if (xyz[k] > mesh.bb_max[k]) mesh.bb_max[k] = xyz[k];
        }
      }
    }
    mesh.decoded = mesh.error.empty();
  } catch (const std::exception& ex) {
    mesh.error = std::string(decode_stage) + ": " + ex.what();
  }
  return mesh;
}

ParticleSysObj decode_particle_sys(const std::string& entry_name,
                                   const std::vector<uint8_t>& body) {
  ParticleSysObj part;
  part.name = entry_name;
  const char* decode_stage = "header";
  Reader r(body.data(), body.size());
  try {
    part.revision = low_revision(r.u32());
    const bool gh1_revision = part.revision == 22;
    if (!gh1_revision && part.revision != 27) {
      throw std::runtime_error("milo_scene: unsupported ParticleSys revision");
    }

    if (!gh1_revision) r.skip(kObjMeta);
    if (gh1_revision) {
      part.anim_revision = low_revision(r.u32());
      if (part.anim_revision != 0) {
        throw std::runtime_error(
            "milo_scene: unsupported GH1 ParticleSys Animatable");
      }
      // GH1's embedded Animatable rev 0 carries the legacy named-range and
      // animation-reference lists. Most venue particles leave both empty;
      // Arena's stage flames contain a named-range row.
      const uint32_t range_count = r.u32();
      if (range_count > 2048) {
        throw std::runtime_error(
            "milo_scene: implausible GH1 ParticleSys animation ranges");
      }
      for (uint32_t i = 0; i < range_count; ++i) {
        (void)r.str();
        (void)r.f32();
        (void)r.f32();
      }
      const uint32_t animation_count = r.u32();
      if (animation_count > 2048) {
        throw std::runtime_error(
            "milo_scene: implausible GH1 ParticleSys animations");
      }
      for (uint32_t i = 0; i < animation_count; ++i) (void)r.str();
    } else {
      part.anim_revision = read_rnd_animatable_source_layout(r);
    }
    if (gh1_revision) {
      part.trans_revision = low_revision(r.u32());
      part.local = r.matrix();
      part.world_stored = r.matrix();
      const uint32_t legacy_child_count = r.u32();
      if (legacy_child_count > 2048) {
        throw std::runtime_error(
            "milo_scene: implausible GH1 ParticleSys Trans children");
      }
      for (uint32_t i = 0; i < legacy_child_count; ++i) (void)r.str();
      part.constraint = r.u32();
      part.target = r.str();
      part.preserve_scale = r.u8() != 0;
      part.parent = r.str();
    } else {
      read_trans_block(r, part.local, part.world_stored, part.constraint,
                       part.target, part.preserve_scale, part.parent, false,
                       &part.trans_revision);
    }
    if (part.trans_revision != (gh1_revision ? 8 : 9)) {
      throw std::runtime_error("milo_scene: unsupported ParticleSys Trans");
    }
    part.draw_revision = low_revision(r.u32());
    r.pos -= sizeof(uint32_t);
    (void)read_rnd_drawable_source_layout(r, part.showing, part.draw_order);
    if (part.draw_revision != (gh1_revision ? 1 : 3)) {
      throw std::runtime_error("milo_scene: unsupported ParticleSys Drawable");
    }

    auto read_f = [&]() {
      const float value = r.f32();
      return std::isfinite(value) ? value : 0.0f;
    };
    auto read_vec2 = [&]() {
      std::array<float, 2> v{};
      v[0] = read_f();
      v[1] = read_f();
      return v;
    };
    auto read_vec3_to = [&](float (&dst)[3]) {
      for (float& value : dst) value = read_f();
    };
    auto read_color = [&]() {
      std::array<float, 4> color{};
      for (float& channel : color) channel = read_f();
      return color;
    };

    const std::array<float, 2> life = read_vec2();
    part.life_min_frames = std::max(1.0f, std::min(life[0], life[1]));
    part.life_max_frames =
        std::max(part.life_min_frames, std::max(life[0], life[1]));
    read_vec3_to(part.box_extent_min);
    read_vec3_to(part.box_extent_max);
    for (int i = 0; i < 3; ++i) {
      part.velocity_min[i] = part.box_extent_min[i];
      part.velocity_max[i] = part.box_extent_max[i];
    }
    const std::array<float, 2> speed = read_vec2();
    part.speed_min = std::max(0.0f, std::min(speed[0], speed[1]));
    part.speed_max = std::max(part.speed_min, std::max(speed[0], speed[1]));
    const std::array<float, 2> pitch = read_vec2();
    part.pitch_min = pitch[0];
    part.pitch_max = pitch[1];
    const std::array<float, 2> yaw = read_vec2();
    part.yaw_min = yaw[0];
    part.yaw_max = yaw[1];
    const std::array<float, 2> emit_rate = read_vec2();
    part.emit_rate_min = std::max(0.0f, std::min(emit_rate[0], emit_rate[1]));
    part.emit_rate_max =
        std::max(part.emit_rate_min, std::max(emit_rate[0], emit_rate[1]));
    const std::array<float, 2> start_size = read_vec2();
    part.start_size_min = std::max(0.0f, start_size[0]);
    part.start_size_max = std::max(part.start_size_min, start_size[1]);
    const std::array<float, 2> delta_size = read_vec2();
    part.delta_size_min = delta_size[0];
    part.delta_size_max = delta_size[1];
    part.lifetime_min = std::max(0.05f, part.life_min_frames / 30.0f);
    part.lifetime_max =
        std::max(part.lifetime_min, part.life_max_frames / 30.0f);
    part.size_start = std::max(0.01f,
                               (part.start_size_min + part.start_size_max) *
                                   0.5f);
    part.size_end = std::max(0.01f,
                             part.size_start +
                                 (part.delta_size_min + part.delta_size_max) *
                                     0.5f);
    part.start_color_low = read_color();
    part.start_color_high = read_color();
    part.end_color_low = read_color();
    part.end_color_high = read_color();

    decode_stage = "bounce";
    part.bounce = r.str();
    read_vec3_to(part.force_dir);
    if (gh1_revision) {
      // GH1 rev 22 has two observed legacy pre-material encodings. Most
      // objects carry one additional byte before the three floats; fest's
      // nuke_toxic omits it. Select the source shape by validating the
      // following length-prefixed material reference, never by object name.
      const auto plausible_ref_at = [&](size_t offset) {
        if (offset + 4 > body.size()) return false;
        const uint32_t len =
            static_cast<uint32_t>(body[offset]) |
            (static_cast<uint32_t>(body[offset + 1]) << 8) |
            (static_cast<uint32_t>(body[offset + 2]) << 16) |
            (static_cast<uint32_t>(body[offset + 3]) << 24);
        if (len == 0 || len > 127 || offset + 4 + len > body.size())
          return false;
        for (uint32_t i = 0; i < len; ++i) {
          const uint8_t c = body[offset + 4 + i];
          if (c < 0x20 || c > 0x7e) return false;
        }
        return true;
      };
      const bool has_legacy_prefix_byte =
          plausible_ref_at(r.pos + 13) && !plausible_ref_at(r.pos + 12);
      if (has_legacy_prefix_byte) (void)r.u8();
      (void)read_f();
      (void)read_f();
      (void)read_f();
    }
    decode_stage = "material";
    part.material = r.str();
    decode_stage = "post-material";
    part.particle_flags = r.u32();
    part.grow_ratio = std::clamp(read_f(), 0.0f, 1.0f);
    part.shrink_ratio = std::clamp(read_f(), 0.0f, 1.0f);
    if (part.shrink_ratio < part.grow_ratio) {
      part.shrink_ratio = part.grow_ratio;
    }
    part.mid_color_ratio = std::clamp(read_f(), 0.0f, 1.0f);
    part.mid_color_low = read_color();
    part.mid_color_high = read_color();
    part.max_particles = r.u32();
    const std::array<float, 2> bubble_period = read_vec2();
    part.bubble_period_min = std::max(0.001f, bubble_period[0]);
    part.bubble_period_max =
        std::max(part.bubble_period_min, bubble_period[1]);
    const std::array<float, 2> bubble_size = read_vec2();
    part.bubble_size_min = bubble_size[0];
    part.bubble_size_max = bubble_size[1];
    part.bubble = r.u8() != 0;
    part.relative_motion = read_f();
    decode_stage = "relative-parent";
    part.relative_parent = r.str();
    if (!gh1_revision) part.emitter_mesh = r.str();
    part.preserve_particles = r.u8() != 0;
    if (part.preserve_particles) {
      part.preserved_particle_count = r.u32();
      constexpr size_t kPreservedParticleRb3Bytes = 9 * sizeof(float);
      constexpr size_t kPreservedParticleGh2Bytes = 8 * sizeof(float);
      const size_t count = static_cast<size_t>(part.preserved_particle_count);
      const size_t remaining = body.size() - r.pos;
      const bool rb3_exact =
          count <= body.size() / kPreservedParticleRb3Bytes &&
          count * kPreservedParticleRb3Bytes == remaining;
      const bool gh2_exact =
          count <= body.size() / kPreservedParticleGh2Bytes &&
          count * kPreservedParticleGh2Bytes == remaining;
      const bool rb3_fits =
          count <= body.size() / kPreservedParticleRb3Bytes &&
          count * kPreservedParticleRb3Bytes <= remaining;
      const bool gh2_fits =
          count <= body.size() / kPreservedParticleGh2Bytes &&
          count * kPreservedParticleGh2Bytes <= remaining;
      if (rb3_exact || (!gh2_exact && rb3_fits)) {
        part.preserved_particle_stride_bytes =
            static_cast<uint32_t>(kPreservedParticleRb3Bytes);
      } else if (gh2_exact || gh2_fits) {
        part.preserved_particle_stride_bytes =
            static_cast<uint32_t>(kPreservedParticleGh2Bytes);
      } else {
        throw std::runtime_error("milo_scene: preserved ParticleSys list past end");
      }
      const size_t preserved_bytes =
          count * static_cast<size_t>(part.preserved_particle_stride_bytes);
      r.skip(preserved_bytes);
    }
    if (part.material.empty()) {
      throw std::runtime_error("milo_scene: ParticleSys has no material ref");
    }
    part.source_order_decoded = true;
    part.decoded = true;
  } catch (const std::exception& ex) {
    part.error = std::string(decode_stage) + "@" + std::to_string(r.pos) +
                 ": " + ex.what();
  }
  return part;
}

WorldCrowdObj decode_world_crowd(const std::string& entry_name,
                                 const std::vector<uint8_t>& body) {
  WorldCrowdObj crowd;
  crowd.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    const uint32_t combined = r.u32();
    const uint16_t revision = static_cast<uint16_t>(combined & 0xffffu);

    const uint32_t draw_combined = r.u32();
    const uint16_t draw_revision =
        static_cast<uint16_t>(draw_combined & 0xffffu);
    (void)r.u8();  // RndDrawable.showing
    if (draw_revision < 2) {
      const uint32_t drawable_count = r.u32();
      for (uint32_t i = 0; i < drawable_count; ++i) (void)r.str();
    }
    if (draw_revision > 0) r.skip(16);  // bounding sphere
    if (draw_revision > 2) r.skip(4);   // draw order
    if (draw_revision >= 4) {
      const uint32_t clip_plane_count = r.u32();
      for (uint32_t i = 0; i < clip_plane_count; ++i) (void)r.str();
    }

    crowd.area_mesh = r.str();
    if (crowd.area_mesh.empty()) {
      throw std::runtime_error("milo_scene: WorldCrowd has no area mesh ref");
    }

    if (revision < 3) r.skip(4);
    crowd.total_placements = r.u32();
    if (revision < 8) crowd.rotate_to_camera = r.u8() != 0;

    const uint32_t actor_count = r.u32();
    if (actor_count == 0 || actor_count > 128) {
      throw std::runtime_error("milo_scene: implausible WorldCrowd actor count");
    }

    crowd.actors.reserve(actor_count);
    for (uint32_t i = 0; i < actor_count; ++i) {
      WorldCrowdActor actor;
      actor.name = r.str();
      if (actor.name.empty()) {
        throw std::runtime_error("milo_scene: empty WorldCrowd actor name");
      }
      actor.params[0] = r.f32();  // height
      actor.params[1] = r.f32();  // density
      if (revision > 1) actor.params[2] = r.f32();  // radius
      if (revision > 8) (void)r.u8();               // useRandomColor
      crowd.actors.push_back(std::move(actor));
    }

    if (revision > 6) (void)r.str();  // environ
    if (revision > 9) (void)r.str();  // environ3D

    uint32_t decoded_placements = 0;
    crowd.placement_sets.reserve(crowd.actors.size());
    const bool debug_decode = debug_worldcrowd_decode_enabled();
    if (revision > 1) {
      for (size_t actor_index = 0; actor_index < crowd.actors.size();
           ++actor_index) {
        const auto& actor = crowd.actors[actor_index];
        const uint32_t count = r.u32();
        // GH2 PS2 revision 6 stores matrix-only placement rows in the source
        // MILOs. Later pre-0x0e rows match ihatecompvir's OldMultiMeshInstance
        // class shape with a trailing color.
        const bool old_instance_has_color = revision > 6 && revision < 0x0e;
        const size_t bytes_per_instance = old_instance_has_color ? 64u : 48u;
        if (debug_decode) {
          std::fprintf(stderr,
                       "[milo_scene]   WorldCrowd '%s' actor[%zu]=%s "
                       "count=%u cursor=%zu stride=%zu\n",
                       entry_name.c_str(), actor_index, actor.name.c_str(),
                       count, r.pos, bytes_per_instance);
        }
        if (count > 4096 ||
            static_cast<uint64_t>(count) * bytes_per_instance >
                body.size() - r.pos) {
          throw std::runtime_error(
              "milo_scene: implausible WorldCrowd placement count actor=" +
              std::to_string(actor_index) + " name=" + actor.name +
              " count=" + std::to_string(count) +
              " cursor=" + std::to_string(r.pos));
        }
        WorldCrowdPlacementSet set;
        set.actor_name = actor.name;
        set.placements.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
          set.placements.push_back(r.matrix());
          if (old_instance_has_color) r.skip(16);
        }
        decoded_placements += count;
        crowd.placement_sets.push_back(std::move(set));
      }
    } else {
      for (const auto& actor : crowd.actors) {
        WorldCrowdPlacementSet set;
        set.actor_name = actor.name;
        crowd.placement_sets.push_back(std::move(set));
      }
    }

    if (revision > 4) r.skip(4);       // modifyStamp
    if (revision > 0x0c) (void)r.u8(); // force3DCrowd
    if (revision > 5) (void)r.u8();    // show3DOnly
    if (revision > 0x0b) (void)r.str();

    crowd.decoded_placement_count = decoded_placements;
    if (debug_decode) {
      std::fprintf(stderr,
                   "[milo_scene]   WorldCrowd '%s' mNum=%u "
                   "decoded_placements=%u actors=%zu\n",
                   entry_name.c_str(), crowd.total_placements,
                   crowd.decoded_placement_count, crowd.actors.size());
    }
    crowd.decoded = true;
  } catch (const std::exception& ex) {
    crowd.error = ex.what();
  }
  return crowd;
}

// ---------------------------------------------------------------------------
// Scene assembly
// ---------------------------------------------------------------------------

namespace {

// 4x4 row-major multiply: out = a * b (row-vector convention, same as Mat4).
std::array<float, 16> mat4_mul(const std::array<float, 16>& a,
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

// Xfm (3x3 rot row-major + translation) -> 4x4 row-major with translation in
// row 3 (matching render::Mat4: a point row-vector p*M transforms correctly).
std::array<float, 16> xfm_to_mat4(const Xfm& x) {
  std::array<float, 16> m{};
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) m[r * 4 + c] = x.rot[r][c];
  m[0 * 4 + 3] = 0.0f;
  m[1 * 4 + 3] = 0.0f;
  m[2 * 4 + 3] = 0.0f;
  m[3 * 4 + 0] = x.pos[0];
  m[3 * 4 + 1] = x.pos[1];
  m[3 * 4 + 2] = x.pos[2];
  m[3 * 4 + 3] = 1.0f;
  return m;
}

bool xfm_nearly_equal(const Xfm& a, const Xfm& b) {
  constexpr float kEps = 1.0e-4f;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      if (std::fabs(a.rot[r][c] - b.rot[r][c]) > kEps) return false;
  for (int c = 0; c < 3; ++c)
    if (std::fabs(a.pos[c] - b.pos[c]) > kEps) return false;
  return true;
}

constexpr uint32_t kConstraintLocalRotate = 1;
constexpr uint32_t kConstraintParentWorld = 2;

std::array<float, 16> apply_transform_constraint(
    const std::array<float, 16>& local,
    const std::array<float, 16>& parent_world,
    uint32_t constraint) {
  if (constraint == kConstraintParentWorld) return parent_world;
  if (constraint == kConstraintLocalRotate) {
    std::array<float, 16> out = local;
    const float x = local[12];
    const float y = local[13];
    const float z = local[14];
    out[12] = x * parent_world[0] + y * parent_world[4] +
              z * parent_world[8] + parent_world[12];
    out[13] = x * parent_world[1] + y * parent_world[5] +
              z * parent_world[9] + parent_world[13];
    out[14] = x * parent_world[2] + y * parent_world[6] +
              z * parent_world[10] + parent_world[14];
    return out;
  }
  return mat4_mul(local, parent_world);
}

struct TransformNode {
  Xfm local;
  Xfm world_stored;
  bool world_xfm_override = false;
  std::string parent;
  uint32_t constraint = 0;
};

bool find_transform_node(const Scene& scene, const std::string& name,
                         TransformNode& out) {
  for (const TransObj& t : scene.transes) {
    if (t.name == name) {
      out.local = t.local;
      out.world_stored = t.world_stored;
      out.world_xfm_override = t.world_xfm_override;
      out.parent = t.parent;
      out.constraint = t.constraint;
      return true;
    }
  }
  for (const MeshObj& mesh : scene.meshes) {
    if (mesh.name == name) {
      out.local = mesh.local;
      out.world_stored = mesh.world_stored;
      out.world_xfm_override = mesh.world_xfm_override;
      out.parent = mesh.parent;
      out.constraint = mesh.constraint;
      return true;
    }
  }
  for (const GroupObj& group : scene.groups) {
    if (group.name == name) {
      out.local = group.has_transform ? group.local : Xfm{};
      out.world_stored = group.has_transform ? group.world_stored : Xfm{};
      out.world_xfm_override = group.world_xfm_override;
      out.parent = group.parent;
      out.constraint = group.has_transform ? group.constraint : 0;
      return true;
    }
  }
  for (const BandPlacerObj& placer : scene.band_placers) {
    if (placer.name == name && placer.decoded) {
      out.local = placer.local;
      out.world_stored = placer.world_stored;
      out.parent = placer.parent;
      out.constraint = 0;
      return true;
    }
  }
  for (const CamObj& camera : scene.cams) {
    if (camera.name == name && camera.decoded) {
      out.local = camera.local;
      out.world_stored = camera.world_stored;
      out.parent = camera.parent;
      out.constraint = camera.constraint;
      return true;
    }
  }
  return false;
}

bool source_world_from_node(const Scene& scene, const TransformNode& node,
                            std::array<float, 16>& world, int guard);

bool source_world_for_name(const Scene& scene, const std::string& name,
                           std::array<float, 16>& world, int guard) {
  if (guard >= 64) return false;
  TransformNode node;
  if (!find_transform_node(scene, name, node)) return false;
  return source_world_from_node(scene, node, world, guard + 1);
}

bool source_world_from_node(const Scene& scene, const TransformNode& node,
                            std::array<float, 16>& world, int guard) {
  if (node.world_xfm_override) {
    world = xfm_to_mat4(node.world_stored);
    return true;
  }
  const std::array<float, 16> local = xfm_to_mat4(node.local);
  if (node.parent.empty()) {
    world = local;
    return true;
  }
  std::array<float, 16> parent_world{};
  if (!source_world_for_name(scene, node.parent, parent_world, guard)) {
    return false;
  }
  world = apply_transform_constraint(local, parent_world, node.constraint);
  return true;
}

}  // namespace

void rebuild_group_authored_draw_order_for_test(Scene& scene) {
  rebuild_group_authored_draw_order(scene);
}

const MatObj* Scene::find_mat(const std::string& name) const {
  for (const MatObj& m : mats)
    if (m.name == name) return &m;
  return nullptr;
}

const LightObj* Scene::find_light(const std::string& name) const {
  for (const LightObj& light : lights)
    if (light.name == name && light.decoded) return &light;
  return nullptr;
}

const EnvironObj* Scene::find_environ(const std::string& name) const {
  for (const EnvironObj& env : environs)
    if (env.name == name && env.decoded) return &env;
  return nullptr;
}

const BandPlacerObj* Scene::find_band_placer(const std::string& name) const {
  for (const BandPlacerObj& placer : band_placers)
    if (placer.name == name && placer.decoded) return &placer;
  return nullptr;
}

std::array<float, 16> Scene::world_matrix(const MeshObj& mesh) const {
  if (mesh.world_xfm_override) return xfm_to_mat4(mesh.world_stored);

  // Mirror RndTransformable::WorldXfm_Force: compose local through the parent
  // chain, honoring kLocalRotate and kParentWorld instead of treating those
  // source fields as padding.
  TransformNode node;
  node.local = mesh.local;
  node.world_stored = mesh.world_stored;
  node.parent = mesh.parent;
  node.constraint = mesh.constraint;
  std::array<float, 16> composed{};
  const bool resolved_parent = source_world_from_node(*this, node, composed, 0);
  // GH1 Drawable1 aggregate roots (main_hall.mesh, floor shells, etc.) carry
  // a stale serialized world cache even though they have no transform parent.
  // Native WorldXfm_Force rebuilds those roots from local before their listed
  // drawable children inherit from them. Keep child caches authoritative;
  // forcing every revision-10 mesh local moves theatre across its camera path.
  if (resolved_parent && dir_revision == 10 &&
      !mesh.drawable_children.empty()) {
    return composed;
  }
  if (resolved_parent && mesh.constraint != 0) return composed;

  // The PS2 Trans block also carries the resolved world matrix immediately
  // after local. Trust it when it has authored hierarchy state; when it is still
  // identical to local and we can resolve a native parent chain, mirror the PS2
  // dirty-world helper by composing the parent rows now.
  if (!resolved_parent || !xfm_nearly_equal(mesh.local, mesh.world_stored))
    return xfm_to_mat4(mesh.world_stored);
  return composed;
}

std::array<float, 16> Scene::world_matrix(const ParticleSysObj& particle) const {
  TransformNode node;
  node.local = particle.local;
  node.world_stored = particle.world_stored;
  node.parent = particle.parent;
  node.constraint = particle.constraint;
  std::array<float, 16> composed{};
  const bool resolved_parent = source_world_from_node(*this, node, composed, 0);
  if (resolved_parent && particle.constraint != 0) return composed;

  if (!resolved_parent ||
      !xfm_nearly_equal(particle.world_stored, particle.local)) {
    return xfm_to_mat4(particle.world_stored);
  }
  return composed;
}

std::array<float, 16> Scene::world_matrix(const CamObj& camera) const {
  return world_matrix(camera, xfm_to_mat4(camera.local));
}

std::array<float, 16> Scene::world_matrix(
    const CamObj& camera,
    const std::array<float, 16>& local_override) const {
  if (camera.parent.empty()) return local_override;
  std::array<float, 16> parent_world{};
  if (!source_world_for_name(*this, camera.parent, parent_world, 0)) {
    if (!xfm_nearly_equal(camera.local, camera.world_stored))
      return xfm_to_mat4(camera.world_stored);
    return local_override;
  }
  return apply_transform_constraint(local_override, parent_world,
                                    camera.constraint);
}

bool load_scene(const std::string& hdr_path, const std::string& ark_path,
                const std::string& milo_path, Scene& out) {
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find(milo_path);
    if (!entry) entry = ark.find("../../system/run/" + milo_path);
    if (!entry) {
      std::fprintf(stderr, "[milo_scene] not in ARK: %s\n", milo_path.c_str());
      return false;
    }
    auto bytes = ark.read_entry(*entry, {ark_path});
    auto hdr = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, hdr);
    auto dir = gh::milo::parse_directory(payload);
    out.dir_name = dir.dir_name;
    out.dir_type = dir.dir_type;
    out.dir_revision = dir.dir_version;
    if (dir.dir_type == "PanelDir" &&
        dir.dir_entry_offset <= payload.size() &&
        dir.dir_entry_size <= payload.size() - dir.dir_entry_offset) {
      std::vector<uint8_t> root(
          payload.begin() + static_cast<std::ptrdiff_t>(dir.dir_entry_offset),
          payload.begin() + static_cast<std::ptrdiff_t>(dir.dir_entry_offset +
                                                        dir.dir_entry_size));
      const PanelDirConfig config = decode_panel_dir_config(root);
      out.panel_dir_config_valid = config.valid;
      out.panel_environment = config.environment;
      out.panel_camera = config.camera;
      out.panel_enter_event = config.enter_event;
    }

    int mesh_ok = 0, mesh_fail = 0;
    std::string first_mesh_error;
    int particle_ok = 0, particle_fail = 0;
    size_t source_order_particles = 0;
    int world_crowd_ok = 0, world_crowd_fail = 0;
    for (const auto& de : dir.entries) {
      std::vector<uint8_t> b(payload.data() + de.offset,
                             payload.data() + de.offset + de.size);
      try {
        if (de.type == "Mesh") {
          MeshObj m = decode_mesh(de.name, b, dir.dir_version);
          m.dir_index = &de - dir.entries.data();
          if (m.decoded) {
            ++mesh_ok;
          } else {
            ++mesh_fail;
            if (first_mesh_error.empty()) {
              first_mesh_error = de.name + ": " + m.error;
            }
          }
          out.meshes.push_back(std::move(m));
        } else if (de.type == "Trans") {
          out.transes.push_back(decode_trans(de.name, b, dir.dir_version));
        } else if (de.type == "Mat") {
          out.mats.push_back(decode_mat(de.name, b));
        } else if (de.type == "Cam") {
          out.cams.push_back(decode_cam(de.name, b, dir.dir_version));
        } else if (de.type == "Waypoint") {
          out.waypoints.push_back(decode_waypoint(de.name, b));
        } else if (de.type == "Flare") {
          out.flares.push_back(
              decode_flare(de.name, b, dir.dir_version));
        } else if (de.type == "Spotlight") {
          out.spotlights.push_back(decode_spotlight(de.name, b));
        } else if (de.type == "Light") {
          out.lights.push_back(decode_light(de.name, b));
        } else if (de.type == "Environ") {
          out.environs.push_back(decode_environ(de.name, b));
        } else if (de.type == "EnvAnim") {
          out.env_anims.push_back(decode_env_anim(de.name, b));
        } else if (de.type == "ScreenMask") {
          out.screen_masks.push_back(decode_screen_mask(de.name, b));
        } else if (de.type == "Group" || de.type == "View") {
          GroupObj group = decode_group(de.name, b, dir.dir_version);
          group.dir_index = &de - dir.entries.data();
          if (!group.decoded) {
            std::fprintf(stderr, "[milo_scene]   Group '%s' decode: %s\n",
                         de.name.c_str(), group.error.c_str());
          }
          out.groups.push_back(std::move(group));
        } else if (de.type == "BandPlacer") {
          out.band_placers.push_back(decode_band_placer(de.name, b));
        } else if (de.type == "ParticleSys") {
          ParticleSysObj p = decode_particle_sys(de.name, b);
          p.dir_index = &de - dir.entries.data();
          if (p.decoded) ++particle_ok; else ++particle_fail;
          if (p.source_order_decoded) ++source_order_particles;
          if (p.decoded && debug_particle_decode_enabled()) {
            const auto avg = [](const std::array<float, 4>& low,
                                const std::array<float, 4>& high,
                                size_t i) {
              return (low[i] + high[i]) * 0.5f;
            };
            std::fprintf(
                stderr,
                "[milo_scene] ParticleSys %s source_order=%d rev=%u anim_rev=%u trans_rev=%u draw_rev=%u material=%s max_parts=%u life_frames=(%.3f %.3f) speed=(%.3f %.3f) emit_rate=(%.3f %.3f) start_size=(%.3f %.3f) delta_size=(%.3f %.3f) start_low=(%.3f %.3f %.3f %.3f) start_high=(%.3f %.3f %.3f %.3f) start_avg=(%.3f %.3f %.3f %.3f) mid_ratio=%.3f mid_low=(%.3f %.3f %.3f %.3f) mid_high=(%.3f %.3f %.3f %.3f) mid_avg=(%.3f %.3f %.3f %.3f) end_low=(%.3f %.3f %.3f %.3f) end_high=(%.3f %.3f %.3f %.3f) end_avg=(%.3f %.3f %.3f %.3f) force=(%.3f %.3f %.3f) grow=%.3f shrink=%.3f bubble=%d bubble_period=(%.3f %.3f) bubble_size=(%.3f %.3f) bounce=%s rel=(%.3f,%s) mesh=%s preserve=%d preserved=%u preserved_stride=%u\n",
                p.name.c_str(), p.source_order_decoded ? 1 : 0,
                p.revision, p.anim_revision, p.trans_revision,
                p.draw_revision, p.material.c_str(), p.max_particles,
                p.life_min_frames, p.life_max_frames, p.speed_min,
                p.speed_max, p.emit_rate_min, p.emit_rate_max,
                p.start_size_min, p.start_size_max, p.delta_size_min,
                p.delta_size_max,
                p.start_color_low[0], p.start_color_low[1],
                p.start_color_low[2], p.start_color_low[3],
                p.start_color_high[0], p.start_color_high[1],
                p.start_color_high[2], p.start_color_high[3],
                avg(p.start_color_low, p.start_color_high, 0),
                avg(p.start_color_low, p.start_color_high, 1),
                avg(p.start_color_low, p.start_color_high, 2),
                avg(p.start_color_low, p.start_color_high, 3),
                p.mid_color_ratio,
                p.mid_color_low[0], p.mid_color_low[1],
                p.mid_color_low[2], p.mid_color_low[3],
                p.mid_color_high[0], p.mid_color_high[1],
                p.mid_color_high[2], p.mid_color_high[3],
                avg(p.mid_color_low, p.mid_color_high, 0),
                avg(p.mid_color_low, p.mid_color_high, 1),
                avg(p.mid_color_low, p.mid_color_high, 2),
                avg(p.mid_color_low, p.mid_color_high, 3),
                p.end_color_low[0], p.end_color_low[1],
                p.end_color_low[2], p.end_color_low[3],
                p.end_color_high[0], p.end_color_high[1],
                p.end_color_high[2], p.end_color_high[3],
                avg(p.end_color_low, p.end_color_high, 0),
                avg(p.end_color_low, p.end_color_high, 1),
                avg(p.end_color_low, p.end_color_high, 2),
                avg(p.end_color_low, p.end_color_high, 3),
                p.force_dir[0], p.force_dir[1], p.force_dir[2],
                p.grow_ratio, p.shrink_ratio, p.bubble ? 1 : 0,
                p.bubble_period_min, p.bubble_period_max,
                p.bubble_size_min, p.bubble_size_max,
                p.bounce.empty() ? "-" : p.bounce.c_str(),
                p.relative_motion,
                p.relative_parent.empty() ? "-" : p.relative_parent.c_str(),
                p.emitter_mesh.empty() ? "-" : p.emitter_mesh.c_str(),
                p.preserve_particles ? 1 : 0, p.preserved_particle_count,
                p.preserved_particle_stride_bytes);
          } else if (!p.error.empty() && debug_particle_decode_enabled()) {
            std::fprintf(stderr,
                         "[milo_scene] ParticleSys %s decode failed: %s "
                         "(rev=%u anim_rev=%u trans_rev=%u draw_rev=%u)\n",
                         p.name.c_str(), p.error.c_str(), p.revision,
                         p.anim_revision, p.trans_revision, p.draw_revision);
            unsigned printed_refs = 0;
            for (size_t off = 0; off + 4 <= b.size() && printed_refs < 16;
                 ++off) {
              const uint32_t len =
                  static_cast<uint32_t>(b[off]) |
                  (static_cast<uint32_t>(b[off + 1]) << 8) |
                  (static_cast<uint32_t>(b[off + 2]) << 16) |
                  (static_cast<uint32_t>(b[off + 3]) << 24);
              if (len < 3 || len > 127 || off + 4 + len > b.size()) continue;
              bool printable = true;
              for (uint32_t i = 0; i < len; ++i) {
                if (b[off + 4 + i] < 0x20 || b[off + 4 + i] > 0x7e) {
                  printable = false;
                  break;
                }
              }
              if (!printable) continue;
              std::fprintf(stderr,
                           "[milo_scene]   ParticleSys raw ref @%zu: %.*s "
                           "(prefix",
                           off, static_cast<int>(len),
                           reinterpret_cast<const char*>(b.data() + off + 4));
              const size_t prefix_begin = off > 16 ? off - 16 : 0;
              for (size_t i = prefix_begin; i < off; ++i) {
                std::fprintf(stderr, " %02x", static_cast<unsigned>(b[i]));
              }
              std::fprintf(stderr, ")\n");
              ++printed_refs;
              off += 3 + len;
            }
          }
          out.particles.push_back(std::move(p));
        } else if (de.type == "WorldCrowd") {
          WorldCrowdObj c = decode_world_crowd(de.name, b);
          if (c.decoded) {
            ++world_crowd_ok;
          } else {
            ++world_crowd_fail;
            if (debug_worldcrowd_decode_enabled()) {
              std::fprintf(stderr,
                           "[milo_scene]   WorldCrowd '%s' decode: %s\n",
                           de.name.c_str(), c.error.c_str());
            }
          }
          out.world_crowds.push_back(std::move(c));
        }
      } catch (const std::exception& ex) {
        std::fprintf(stderr, "[milo_scene]   %s '%s' decode: %s\n",
                     de.type.c_str(), de.name.c_str(), ex.what());
      }
    }
    if (dir.dir_version == 10) {
      for (auto& mesh : out.meshes) {
        if (mesh.parent == mesh.name) mesh.parent.clear();
      }
      for (auto& flare : out.flares) {
        if (flare.parent == flare.name) flare.parent.clear();
      }
      for (auto& trans : out.transes) {
        if (trans.parent == trans.name) trans.parent.clear();
      }
      for (auto& cam : out.cams) {
        if (cam.parent == cam.name) cam.parent.clear();
      }
      for (auto& spotlight : out.spotlights) {
        if (spotlight.parent == spotlight.name) spotlight.parent.clear();
      }
      for (auto& light : out.lights) {
        if (light.parent == light.name) light.parent.clear();
      }
      for (auto& group : out.groups) {
        if (group.parent == group.name) group.parent.clear();
      }
      const auto set_legacy_transform_parent =
          [&](const std::string& child_name,
              const std::string& parent_name) {
            for (auto& mesh : out.meshes) {
              if (mesh.name == child_name) {
                mesh.parent = parent_name;
                return;
              }
            }
            for (auto& flare : out.flares) {
              if (flare.name == child_name) {
                flare.parent = parent_name;
                return;
              }
            }
            for (auto& trans : out.transes) {
              if (trans.name == child_name) {
                trans.parent = parent_name;
                return;
              }
            }
            for (auto& cam : out.cams) {
              if (cam.name == child_name) {
                cam.parent = parent_name;
                return;
              }
            }
            for (auto& spotlight : out.spotlights) {
              if (spotlight.name == child_name) {
                spotlight.parent = parent_name;
                return;
              }
            }
            for (auto& light : out.lights) {
              if (light.name == child_name) {
                light.parent = parent_name;
                return;
              }
            }
            for (auto& group : out.groups) {
              if (group.name == child_name) {
                group.parent = parent_name;
                return;
              }
            }
          };
      for (const auto& parent : out.meshes) {
        for (const auto& child_name : parent.legacy_children) {
          set_legacy_transform_parent(child_name, parent.name);
        }
      }
      if (std::getenv("GHOGX_DEBUG_GH1_TRANSFORMS")) {
        size_t parented = 0;
        size_t local_equals_stored = 0;
        size_t composed_position_differs = 0;
        size_t reported = 0;
        for (const auto& mesh : out.meshes) {
          if (mesh.parent.empty()) continue;
          ++parented;
          const bool equal = xfm_nearly_equal(mesh.local, mesh.world_stored);
          if (equal) ++local_equals_stored;
          const auto stored = xfm_to_mat4(mesh.world_stored);
          const auto runtime = out.world_matrix(mesh);
          const float dx = runtime[12] - stored[12];
          const float dy = runtime[13] - stored[13];
          const float dz = runtime[14] - stored[14];
          const float delta = std::sqrt(dx * dx + dy * dy + dz * dz);
          if (delta > 0.001f) ++composed_position_differs;
          if (reported < 32 && (equal || delta > 0.001f)) {
            std::fprintf(
                stderr,
                "[milo_scene] GH1 transform mesh=%s parent=%s mat=%s geom=%s verts=%u faces=%u bb=(%.2f %.2f %.2f)..(%.2f %.2f %.2f) equal=%d local=(%.3f %.3f %.3f) stored=(%.3f %.3f %.3f) runtime=(%.3f %.3f %.3f) delta=%.3f\n",
                mesh.name.c_str(), mesh.parent.c_str(),
                mesh.material.empty() ? "<none>" : mesh.material.c_str(),
                mesh.geometry_owner.empty() ? "<none>"
                                            : mesh.geometry_owner.c_str(),
                mesh.vertex_count, mesh.face_count, mesh.bb_min[0],
                mesh.bb_min[1], mesh.bb_min[2], mesh.bb_max[0],
                mesh.bb_max[1], mesh.bb_max[2], equal ? 1 : 0,
                mesh.local.pos[0], mesh.local.pos[1], mesh.local.pos[2],
                stored[12], stored[13], stored[14], runtime[12], runtime[13],
                runtime[14], delta);
            ++reported;
          }
        }
        std::fprintf(
            stderr,
            "[milo_scene] GH1 transform summary: parented=%zu local_equals_stored=%zu runtime_position_differs=%zu\n",
            parented, local_equals_stored, composed_position_differs);
      }
    }
    struct SharedSceneGeometry {
      size_t index = 0;
      uint32_t vertex_count = 0;
      uint32_t face_count = 0;
      std::vector<Vertex> verts;
      std::vector<uint16_t> indices;
      std::vector<BoneTransform> bones;
      float bb_min[3] = {};
      float bb_max[3] = {};
    };
    std::map<std::string, const MeshObj*> geometry_owners;
    for (const auto& mesh : out.meshes) geometry_owners[mesh.name] = &mesh;
    std::vector<SharedSceneGeometry> shared_geometry;
    for (size_t i = 0; i < out.meshes.size(); ++i) {
      const auto& mesh = out.meshes[i];
      const auto owner = geometry_owners.find(mesh.geometry_owner);
      if (owner == geometry_owners.end() || owner->second == &mesh) continue;
      // GH1 RndMesh accessors always forward to mGeomOwner, even when the
      // alias body also serialized stale backing arrays.
      if (dir.dir_version != 10 && mesh.vertex_count != 0) continue;
      SharedSceneGeometry row;
      row.index = i;
      row.vertex_count = owner->second->vertex_count;
      row.face_count = owner->second->face_count;
      row.verts = owner->second->verts;
      row.indices = owner->second->indices;
      row.bones = owner->second->bones;
      std::memcpy(row.bb_min, owner->second->bb_min, sizeof(row.bb_min));
      std::memcpy(row.bb_max, owner->second->bb_max, sizeof(row.bb_max));
      shared_geometry.push_back(std::move(row));
    }
    for (auto& row : shared_geometry) {
      auto& mesh = out.meshes[row.index];
      mesh.vertex_count = row.vertex_count;
      mesh.face_count = row.face_count;
      mesh.verts = std::move(row.verts);
      mesh.indices = std::move(row.indices);
      mesh.bones = std::move(row.bones);
      std::memcpy(mesh.bb_min, row.bb_min, sizeof(mesh.bb_min));
      std::memcpy(mesh.bb_max, row.bb_max, sizeof(mesh.bb_max));
      mesh.decoded = true;
      mesh.error.clear();
    }
    rebuild_group_authored_draw_order(out);
    if (dir.dir_version == 10) {
      // A legacy ObjectDir draws through its authored root View.  Subdirs such
      // as lighting.rnd_ps2 contain unreferenced editor/helper meshes (notably
      // crowd_limits*.mesh); treating every ungrouped object as a root makes
      // those helpers visible.  Top-level venues conventionally use
      // venue.view, while nested directories use <directory-name>.view.
      const GroupObj* authored_root = find_group_obj(out, "venue.view");
      if (!authored_root) {
        authored_root = find_group_obj(
            out, legacy_directory_root_view_name(milo_path));
      }
      if (authored_root) {
        out.draw_order.clear();
        std::unordered_set<std::string> visiting;
        std::unordered_set<std::string> emitted;
        append_group_draw_order(out, *authored_root, visiting, emitted,
                                out.draw_order);
        // Treat every directory mesh as group-owned for this legacy root so
        // the renderer does not append unreferenced editor/reference meshes
        // after the authored venue list.
        out.grouped_meshes.clear();
        out.grouped_meshes.reserve(out.meshes.size());
        for (const auto& mesh : out.meshes)
          out.grouped_meshes.push_back(mesh.name);
        std::sort(out.grouped_meshes.begin(), out.grouped_meshes.end());
      }
    }
    size_t source_order_groups = 0;
    for (const auto& group : out.groups) {
      if (group.source_order_decoded) ++source_order_groups;
    }
    std::fprintf(stderr,
                 "[milo_scene] %s: %zu meshes (%d ok / %d fail), %zu particles (%d ok / %d fail, %zu source-order), %zu trans, %zu mat, %zu cam, %zu waypoint, %zu group (%zu source-order), %zu world_crowd (%d ok / %d fail)\n",
                 milo_path.c_str(), out.meshes.size(), mesh_ok, mesh_fail,
                 out.particles.size(), particle_ok, particle_fail,
                 source_order_particles, out.transes.size(), out.mats.size(),
                 out.cams.size(), out.waypoints.size(), out.groups.size(),
                 source_order_groups, out.world_crowds.size(), world_crowd_ok,
                 world_crowd_fail);
    if (!first_mesh_error.empty()) {
      std::fprintf(stderr, "[milo_scene]   first mesh failure: %s\n",
                   first_mesh_error.c_str());
    }
    if (!out.spotlights.empty()) {
      size_t transformed = 0;
      size_t source_order = 0;
      for (const auto& spot : out.spotlights) {
        if (spot.has_transform) ++transformed;
        if (spot.source_order_decoded) ++source_order;
      }
      std::fprintf(
          stderr,
          "[milo_scene]   %zu spotlights decoded (%zu with Trans base, %zu source-order)\n",
          out.spotlights.size(), transformed, source_order);
      for (const auto& spot : out.spotlights) {
        std::fprintf(
            stderr,
            "[milo_scene]   Spotlight object decoded: %s:%s source_order=%d rev=%u trans_rev=%u target=%s group=%s material=%s color=(%.3f %.3f %.3f) intensity=%.3f additional=%zu error=%s\n",
            milo_path.c_str(), spot.name.c_str(),
            spot.source_order_decoded ? 1 : 0, spot.revision,
            spot.trans_revision, spot.target.c_str(), spot.group.c_str(),
            spot.material.c_str(), spot.default_color[0],
            spot.default_color[1], spot.default_color[2],
            spot.default_intensity, spot.instance_meshes.size(),
            spot.error.c_str());
      }
    }
    if (!out.lights.empty()) {
      size_t decoded = 0;
      size_t source_order = 0;
      for (const auto& light : out.lights) {
        if (!light.decoded) continue;
        ++decoded;
        if (light.source_order_decoded) ++source_order;
      }
      std::fprintf(stderr,
                   "[milo_scene]   %zu lights decoded (%zu source-order)\n",
                   decoded, source_order);
      for (const auto& light : out.lights) {
        if (light.decoded) {
          std::fprintf(
              stderr,
              "[milo_scene]   Light object decoded: %s:%s source_order=%d type=%d anim_color=%d anim_pos=%d anim_range=%d parent=%s pos=(%.3f %.3f %.3f) color=(%.3f %.3f %.3f %.3f) range=%.3f\n",
              milo_path.c_str(), light.name.c_str(),
              light.source_order_decoded ? 1 : 0, light.type,
              light.animate_color_from_preset ? 1 : 0,
              light.animate_position_from_preset ? 1 : 0,
              light.animate_range_from_preset ? 1 : 0,
              light.parent.empty() ? "-" : light.parent.c_str(),
              light.world_stored.pos[0], light.world_stored.pos[1],
              light.world_stored.pos[2], light.color[0], light.color[1],
              light.color[2], light.color[3], light.range);
        } else {
          std::fprintf(stderr,
                       "[milo_scene]   Light object decode failed: %s:%s %s\n",
                       milo_path.c_str(), light.name.c_str(),
                       light.error.c_str());
        }
      }
    }
    if (!out.environs.empty()) {
      size_t decoded = 0;
      size_t source_order = 0;
      for (const auto& env : out.environs) {
        if (!env.decoded) continue;
        ++decoded;
        if (env.source_order_decoded) ++source_order;
      }
      std::fprintf(stderr,
                   "[milo_scene]   %zu environs decoded (%zu source-order)\n",
                   decoded, source_order);
      for (const auto& env : out.environs) {
        if (env.decoded) {
          std::fprintf(
              stderr,
              "[milo_scene]   Environ object decoded: %s:%s source_order=%d rev=%u lights=%zu fog=%d animate_preset=%d fade_out=%d fade=(%.3f %.3f) color_a=(%.3f %.3f %.3f %.3f) refs=",
              milo_path.c_str(), env.name.c_str(),
              env.source_order_decoded ? 1 : 0, env.revision,
              env.lights.size(), env.fog_enabled ? 1 : 0,
              env.animate_from_preset ? 1 : 0, env.fade_out ? 1 : 0,
              env.fade_start, env.fade_end, env.color_a[0], env.color_a[1],
              env.color_a[2], env.color_a[3]);
          if (env.lights.empty()) {
            std::fprintf(stderr, "-");
          } else {
            for (size_t i = 0; i < env.lights.size(); ++i) {
              std::fprintf(stderr, "%s%s", i == 0 ? "" : ",",
                           env.lights[i].c_str());
            }
          }
          std::fprintf(stderr, "\n");
        } else {
          std::fprintf(stderr,
                       "[milo_scene]   Environ object decode failed: %s:%s %s\n",
                       milo_path.c_str(), env.name.c_str(),
                       env.error.c_str());
        }
      }
    }
    return true;
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[milo_scene] load_scene(%s): %s\n", milo_path.c_str(),
                 ex.what());
    return false;
  }
}

}  // namespace ghogx::milo_scene
