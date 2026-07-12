// engine/src/milo_scene/milo_scene.cpp — see milo_scene.h for the byte layouts.

#include "milo_scene/milo_scene.h"

#include "ark_v3.h"
#include "milo.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
                      std::string& parent, bool read_object_meta) {
  int32_t ver = r.i32();
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

void read_rnd_animatable_source_layout(Reader& r) {
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
  const auto visit_child = [&](const std::string& child) {
    if (name_has_suffix(child, ".mesh") && scene_has_mesh(scene, child)) {
      if (emitted_meshes.insert(child).second) order.push_back(child);
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

}  // namespace

TransObj decode_trans(const std::string& entry_name,
                      const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  TransObj t;
  t.name = entry_name;
  read_trans_block(r, t.local, t.world_stored, t.constraint, t.target,
                   t.preserve_scale, t.parent, true);
  return t;
}

CamObj decode_cam(const std::string& entry_name,
                  const std::vector<uint8_t>& body) {
  CamObj c;
  c.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    const uint32_t combined_revision = r.u32();
    const uint16_t version = static_cast<uint16_t>(combined_revision & 0xffff);
    if (version > 10) r.skip(kObjMeta);

    const uint32_t trans_revision = r.u32();
    c.local = r.matrix();
    c.world_stored = r.matrix();
    if (trans_revision > 6) c.constraint = r.u32();
    if (trans_revision > 5) c.target = r.str();
    if (trans_revision > 6) c.preserve_scale = r.u8() != 0;
    c.parent = r.str();

    if (version < 10) {
      r.i32();       // Draw revision.
      r.skip(21);    // Draw payload: showing + bounds + draw order byte.
      if (version == 8) {
        const uint32_t objects = r.u32();
        for (uint32_t i = 0; i < objects; ++i) (void)r.str();
      }
    }

    c.near_plane = r.f32();
    c.far_plane = r.f32();
    c.fov = r.f32();
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
      if (ref.find("spot_circle") != std::string::npos) {
        s.circle_material = ref;
      } else if (ref.find("lens") != std::string::npos) {
        s.lens_material = ref;
      } else if (s.material.empty()) {
        s.material = ref;
      }
    } else if (ref.rfind(".grp") != std::string::npos) {
      s.group = ref;
    } else if (ref.rfind(".mesh") != std::string::npos) {
      if (ref.rfind("SPOT_circle", 0) == 0) s.circle_mesh = ref;
      const bool authored_target = is_spotlight_target_mesh(ref);
      has_authored_target = has_authored_target || authored_target;
      if (authored_target || s.target.empty()) s.target = ref;
      if (!authored_target &&
          std::find(s.instance_meshes.begin(), s.instance_meshes.end(), ref) ==
              s.instance_meshes.end()) {
        s.instance_meshes.push_back(ref);
      }
    }
  }
  if (!has_authored_target) {
    s.has_default_state =
        read_spotlight_default_state(body, strings, s.default_color,
                                     s.default_intensity);
  }
  s.decoded = true;
  return s;
}

LightObj decode_light(const std::string& entry_name,
                      const std::vector<uint8_t>& body) {
  LightObj light;
  light.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    const uint16_t light_revision = low_revision(r.u32());
    if (light_revision != 6) {
      throw std::runtime_error("milo_scene: unsupported Light version");
    }
    r.skip(kObjMeta);
    read_trans_block(r, light.local, light.world_stored, light.constraint,
                     light.target, light.preserve_scale, light.parent, false);
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
    light.animate_color_from_preset = r.u8() != 0;
    light.animate_position_from_preset = r.u8() != 0;
    light.animate_range_from_preset = light.animate_color_from_preset;
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
    const int32_t version = r.i32();
    if (version != 5) {
      throw std::runtime_error("milo_scene: unsupported Environ version");
    }
    r.skip(kObjMeta);
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

    const size_t base = r.pos;
    for (int i = 0; i < 4; ++i) {
      env.color_a[i] = read_f32_at(body, base + static_cast<size_t>(i) * 4);
      if (!std::isfinite(env.color_a[i])) {
        throw std::runtime_error("milo_scene: non-finite Environ color_a");
      }
    }
    env.range_a = read_f32_at(body, base + 0x10);
    env.range_b = read_f32_at(body, base + 0x14);
    env.fog_start = env.range_a;
    env.fog_end = env.range_b;
    for (int i = 0; i < 4; ++i) {
      env.color_b[i] =
          read_f32_at(body, base + 0x18 + static_cast<size_t>(i) * 4);
      if (!std::isfinite(env.color_b[i])) {
        throw std::runtime_error("milo_scene: non-finite Environ color_b");
      }
      env.fog_color[i] = env.color_b[i];
    }
    if (base + 0x29 < body.size()) {
      env.fog_enabled = body[base + 0x28] != 0;
      env.animate_from_preset = body[base + 0x29] != 0;
    }
    env.range = read_f32_at(body, base + 0x2f);
    if (!std::isfinite(env.range_a) || !std::isfinite(env.range_b) ||
        !std::isfinite(env.range) || env.range_a < 0.0f ||
        env.range_b < 0.0f || env.range < 0.0f) {
      throw std::runtime_error("milo_scene: invalid Environ range");
    }
    env.decoded = true;
  } catch (const std::exception& ex) {
    env.error = ex.what();
  }
  return env;
}

MatObj decode_mat(const std::string& entry_name,
                  const std::vector<uint8_t>& body) {
  Reader r(body.data(), body.size());
  MatObj m;
  m.name = entry_name;
  const uint16_t ver = low_revision(r.u32());  // = 27 in GH2 PS2 venues
  r.skip(kObjMeta);          // base metadata
  const uint32_t blend = r.u32();
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
  const size_t mat_state_bytes = (ver > 0x25) ? 20u : 16u;
  auto read_i32_field = [&](size_t offset) {
    int32_t value = 0;
    std::memcpy(&value, body.data() + offset, sizeof(value));
    return value;
  };
  if (ver > 21 && flag_pos + mat_state_bytes <= body.size()) {
    size_t state = flag_pos;
    m.use_environ = body[state++] != 0;
    m.prelit = body[state++] != 0;
    const int32_t z_mode = read_i32_field(state);
    state += 4;
    if (z_mode >= 0 && z_mode <= 4) m.z_mode = static_cast<uint8_t>(z_mode);
    m.alpha_cut = body[state++] != 0;
    if (ver > 0x25) state += 4;  // alpha threshold, absent in GH2 rev 27
    m.alpha_write = body[state++] != 0;
    const int32_t tex_gen = read_i32_field(state);
    state += 4;
    if (tex_gen >= 0 && tex_gen <= 5) m.tex_gen = static_cast<uint8_t>(tex_gen);
    const int32_t tex_wrap = read_i32_field(state);
    if (tex_wrap >= 0 && tex_wrap <= 4) {
      m.tex_wrap = static_cast<uint8_t>(tex_wrap);
    }
  } else if (flag_pos + 2 <= body.size()) {
    m.use_environ = body[flag_pos] != 0;
    m.prelit = body[flag_pos + 1] != 0;
  }
  // Diffuse texcoord transform: 16 bytes of flags, then a 12-float source
  // matrix block. Renderers consume the 2-D UV rows as [u v 1] * a 3x3 matrix;
  // the source third-axis slot can carry non-UV scale, so force homogeneous
  // [2][2] to one instead of rejecting the authored UV transform.
  {
    const size_t txf = r.pos + mat_state_bytes;
    auto rf = [&](size_t o) { float f; std::memcpy(&f, body.data() + o, 4); return f; };
    if (txf + 48 <= body.size()) {
      float xfm[3][3] = {};
      bool sane = true;
      for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
          const float value =
              rf(txf + static_cast<size_t>(row * 3 + col) * 4);
          xfm[row][col] = value;
          if (!std::isfinite(value) || std::fabs(value) > 128.0f) sane = false;
        }
      }
      if (sane) {
        m.tex_xfm[0][0] = xfm[0][0];
        m.tex_xfm[0][1] = xfm[0][1];
        m.tex_xfm[0][2] = 0.0f;
        m.tex_xfm[1][0] = xfm[1][0];
        m.tex_xfm[1][1] = xfm[1][1];
        m.tex_xfm[1][2] = 0.0f;
        m.tex_xfm[2][0] = xfm[2][0];
        m.tex_xfm[2][1] = xfm[2][1];
        m.tex_xfm[2][2] = 1.0f;
        m.tex_scale[0] = m.tex_xfm[0][0];
        m.tex_scale[1] = m.tex_xfm[1][1];
        m.tex_offset[0] = m.tex_xfm[2][0];
        m.tex_offset[1] = m.tex_xfm[2][1];
      }
    }
  }
  // The flag / texture-state bytes follow the colour. We don't need their exact split
  // to draw; the diffuse texture name is the load-bearing field. Scan forward
  // from here for the first length-prefixed ".tex" string — robust against the
  // version-specific flag block between colour and the texture reference.
  size_t start = r.pos;
  for (size_t o = start; o + 4 <= body.size(); ++o) {
    uint32_t len;
    std::memcpy(&len, body.data() + o, 4);
    if (len < 4 || len > 64 || o + 4 + len > body.size()) continue;
    const char* s = reinterpret_cast<const char*>(body.data() + o + 4);
    // Must be printable and end in ".tex".
    bool printable = true;
    for (uint32_t k = 0; k < len; ++k) {
      char c = s[k];
      if (c < 0x20 || c >= 0x7f) { printable = false; break; }
    }
    if (!printable) continue;
    std::string cand(s, len);
    if (cand.size() >= 4 && cand.compare(cand.size() - 4, 4, ".tex") == 0) {
      m.diffuse_tex = cand;
      size_t cursor = o + 4 + len;
      try {
        (void)read_string_at(body, cursor);
        if (cursor + 2 <= body.size()) {
          m.intensify = body[cursor] != 0;
          ++cursor;
          m.cull = body[cursor] != 0;
        }
      } catch (const std::exception&) {
      }
      break;
    }
  }
  m.decoded = true;
  return m;
}

GroupObj decode_group(const std::string& entry_name,
                      const std::vector<uint8_t>& body) {
  GroupObj group;
  group.name = entry_name;
  if (decode_group_source_order(body, group)) return group;
  size_t after_trans_offset = body.size();
  decode_group_transform(body, group, &after_trans_offset);
  const uint16_t group_revision =
      body.size() >= 4 ? low_revision(read_u32_at(body, 0)) : 0;
  if (group.has_transform) {
    parse_group_source_layout(body, group_revision, after_trans_offset, group);
  }
  return group;
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
                    const std::vector<uint8_t>& body) {
  MeshObj mesh;
  mesh.name = entry_name;
  try {
    Reader r(body.data(), body.size());
    int32_t ver = r.i32();   // mesh version = 28 (0x1c)
    if (ver != 28) {
      // Not fatal — some mesh variants exist — but record it.
      mesh.error = "unexpected mesh version " + std::to_string(ver);
    }
    // RndMesh object base, then embedded RndTrans base.
    r.skip(kObjMeta);
    std::string trans_parent;
    read_trans_block(r, mesh.local, mesh.world_stored, mesh.constraint,
                     mesh.target, mesh.preserve_scale, trans_parent, false);
    mesh.parent = trans_parent;

    // Draw base: version (= 3), showing flag, then sphere + draw-order.
    int32_t draw_ver = r.i32();
    (void)draw_ver;
    mesh.showing = r.u8() != 0;
    r.skip(16);
    mesh.draw_order = r.f32();

    // Mesh fields.
    mesh.material = r.str();           // material name
    mesh.geometry_owner = r.str();     // geometry-owner name (usually self)
    mesh.mutable_flags = r.u32();      // RndMesh::mMutable
    r.skip(5);                         // volume + null BSP-tree owner
    uint32_t vcount = r.u32();

    // Sanity-gate the vertex count against the remaining bytes: we need at least
    // vcount*48 + 4 (face count) more bytes.
    if (static_cast<uint64_t>(vcount) * sizeof(Vertex) + 4 > body.size() - r.pos) {
      mesh.error = "vertex_count " + std::to_string(vcount) + " exceeds entry";
      return mesh;
    }
    mesh.vertex_count = vcount;
    mesh.verts.resize(vcount);
    for (uint32_t i = 0; i < vcount; ++i) {
      Vertex& v = mesh.verts[i];
      v.px = r.f32(); v.py = r.f32(); v.pz = r.f32();
      v.nx = r.f32(); v.ny = r.f32(); v.nz = r.f32();
      // ihatecompvir's RndMesh reader treats GH2 rev 28's next four floats as
      // weights, not vertex colors. Keep venue diffuse neutral until skinned
      // mesh weights are used by the native renderer.
      const float weight0 = r.f32();
      const float weight1 = r.f32();
      const float weight2 = r.f32();
      const float weight3 = r.f32();
      (void)weight0;
      (void)weight1;
      (void)weight2;
      (void)weight3;
      v.r = 1.0f; v.g = 1.0f; v.b = 1.0f; v.a = 1.0f;
      v.u  = r.f32(); v.v  = r.f32();
    }

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
    mesh.error = ex.what();
  }
  return mesh;
}

ParticleSysObj decode_particle_sys(const std::string& entry_name,
                                   const std::vector<uint8_t>& body) {
  ParticleSysObj part;
  part.name = entry_name;
  try {
    if (body.size() < 0x19 + 4 + 48 + 48 + kObjMeta + 4 + 25) {
      throw std::runtime_error("milo_scene: ParticleSys body too small");
    }
    Reader head(body.data(), body.size());
    const int32_t version = head.i32();
    if (version != 27) {
      throw std::runtime_error("milo_scene: unsupported ParticleSys version");
    }

    // ParticleSys is a Trans subclass, but GH2 PS2 embeds the Trans block after
    // the object/draw header. The embedded Trans version is at raw +0x19 and is
    // followed immediately by local/world matrices, the normal 9 Trans bytes,
    // and the parent string.
    constexpr size_t kParticleTransAt = 0x19;
    Reader tr(body.data() + kParticleTransAt, body.size() - kParticleTransAt);
    const int32_t trans_version = tr.i32();
    if (trans_version != 9) {
      throw std::runtime_error("milo_scene: missing ParticleSys Trans block");
    }
    part.local = tr.matrix();
    part.world_stored = tr.matrix();
    part.constraint = tr.u32();
    part.target = tr.str();
    part.preserve_scale = tr.u8() != 0;
    part.parent = tr.str();

    const size_t draw_base = kParticleTransAt + tr.pos;
    if (draw_base + 25 > body.size()) {
      throw std::runtime_error("milo_scene: truncated ParticleSys Draw block");
    }
    int32_t draw_version = 0;
    std::memcpy(&draw_version, body.data() + draw_base, sizeof(draw_version));
    if (draw_version != 3) {
      throw std::runtime_error("milo_scene: unsupported ParticleSys Draw block");
    }
    part.showing = body[draw_base + 4] != 0;
    part.draw_order = read_f32_at(body, draw_base + 21);

    const size_t prop_base = draw_base + 25;
    auto safe_f = [&](size_t off, float fallback) {
      if (prop_base + off + 4 > body.size()) return fallback;
      float value = read_f32_at(body, prop_base + off);
      return std::isfinite(value) ? value : fallback;
    };
    auto safe_color = [&](size_t off,
                          const std::array<float, 4>& fallback) {
      std::array<float, 4> color = fallback;
      for (int i = 0; i < 4; ++i) {
        color[static_cast<size_t>(i)] =
            safe_f(off + static_cast<size_t>(i) * 4,
                   fallback[static_cast<size_t>(i)]);
      }
      return color;
    };
    part.life_min_frames = std::max(1.0f, safe_f(0x00, part.life_min_frames));
    part.life_max_frames =
        std::max(part.life_min_frames, safe_f(0x04, part.life_min_frames));
    for (int i = 0; i < 3; ++i) {
      part.box_extent_min[i] = safe_f(0x08 + static_cast<size_t>(i) * 4, 0.0f);
      part.box_extent_max[i] = safe_f(0x14 + static_cast<size_t>(i) * 4, 0.0f);
      part.velocity_min[i] = part.box_extent_min[i];
      part.velocity_max[i] = part.box_extent_max[i];
    }
    part.speed_min = std::max(0.0f, safe_f(0x20, part.speed_min));
    part.speed_max = std::max(part.speed_min, safe_f(0x24, part.speed_min));
    part.pitch_min = safe_f(0x28, part.pitch_min);
    part.pitch_max = safe_f(0x2c, part.pitch_min);
    part.yaw_min = safe_f(0x30, part.yaw_min);
    part.yaw_max = safe_f(0x34, part.yaw_min);
    part.emit_rate_min = std::max(0.0f, safe_f(0x38, part.emit_rate_min));
    part.emit_rate_max =
        std::max(part.emit_rate_min, safe_f(0x3c, part.emit_rate_min));
    part.start_size_min = std::max(0.0f, safe_f(0x40, part.start_size_min));
    part.start_size_max =
        std::max(part.start_size_min, safe_f(0x44, part.start_size_min));
    part.delta_size_min = safe_f(0x48, part.delta_size_min);
    part.delta_size_max = safe_f(0x4c, part.delta_size_min);
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
    part.start_color_low = safe_color(0x50, part.start_color_low);
    part.start_color_high = safe_color(0x60, part.start_color_high);
    part.end_color_low = safe_color(0x70, part.end_color_low);
    part.end_color_high = safe_color(0x80, part.end_color_high);

    size_t particle_pos = prop_base + 0x90;
    auto read_cursor_f = [&]() {
      const float value = read_f32_at(body, particle_pos);
      particle_pos += 4;
      return std::isfinite(value) ? value : 0.0f;
    };
    auto read_cursor_u32 = [&]() {
      const uint32_t value = read_u32_at(body, particle_pos);
      particle_pos += 4;
      return value;
    };
    auto read_cursor_bool = [&]() {
      if (particle_pos >= body.size()) {
        throw std::runtime_error("milo_scene: ParticleSys bool past end");
      }
      return body[particle_pos++] != 0;
    };
    auto read_cursor_string = [&]() {
      return read_string_at(body, particle_pos);
    };
    auto read_cursor_vec2 = [&]() {
      std::array<float, 2> v{};
      v[0] = read_cursor_f();
      v[1] = read_cursor_f();
      return v;
    };
    auto read_cursor_color = [&]() {
      std::array<float, 4> color{};
      for (float& channel : color) channel = read_cursor_f();
      return color;
    };
    part.bounce = read_cursor_string();
    for (float& force : part.force_dir) force = read_cursor_f();
    part.material = read_cursor_string();
    part.particle_flags = read_cursor_u32();
    part.grow_ratio = std::clamp(read_cursor_f(), 0.0f, 1.0f);
    part.shrink_ratio = std::clamp(read_cursor_f(), 0.0f, 1.0f);
    if (part.shrink_ratio < part.grow_ratio) {
      part.shrink_ratio = part.grow_ratio;
    }
    part.mid_color_ratio = std::clamp(read_cursor_f(), 0.0f, 1.0f);
    part.mid_color_low = read_cursor_color();
    part.mid_color_high = read_cursor_color();
    part.max_particles = read_cursor_u32();
    const std::array<float, 2> bubble_period = read_cursor_vec2();
    part.bubble_period_min = std::max(0.001f, bubble_period[0]);
    part.bubble_period_max =
        std::max(part.bubble_period_min, bubble_period[1]);
    const std::array<float, 2> bubble_size = read_cursor_vec2();
    part.bubble_size_min = bubble_size[0];
    part.bubble_size_max = bubble_size[1];
    part.bubble = read_cursor_bool();
    part.relative_motion = read_cursor_f();
    part.relative_parent = read_cursor_string();
    part.emitter_mesh = read_cursor_string();
    part.preserve_particles = read_cursor_bool();
    if (part.preserve_particles) {
      part.preserved_particle_count = read_cursor_u32();
      constexpr size_t kPreservedParticleBytes = 9 * sizeof(float);
      const size_t preserved_bytes =
          static_cast<size_t>(part.preserved_particle_count) *
          kPreservedParticleBytes;
      if (particle_pos + preserved_bytes > body.size()) {
        throw std::runtime_error("milo_scene: preserved ParticleSys list past end");
      }
      particle_pos += preserved_bytes;
    }
    if (part.material.empty()) {
      throw std::runtime_error("milo_scene: ParticleSys has no material ref");
    }
    part.decoded = true;
  } catch (const std::exception& ex) {
    part.error = ex.what();
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
    if (revision < 8) (void)r.u8();

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

    if (crowd.total_placements != 0 &&
        decoded_placements != crowd.total_placements) {
      throw std::runtime_error(
          "milo_scene: WorldCrowd placement total mismatch");
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
  std::string parent;
  uint32_t constraint = 0;
};

bool find_transform_node(const Scene& scene, const std::string& name,
                         TransformNode& out) {
  for (const TransObj& t : scene.transes) {
    if (t.name == name) {
      out.local = t.local;
      out.world_stored = t.world_stored;
      out.parent = t.parent;
      out.constraint = t.constraint;
      return true;
    }
  }
  for (const MeshObj& mesh : scene.meshes) {
    if (mesh.name == name) {
      out.local = mesh.local;
      out.world_stored = mesh.world_stored;
      out.parent = mesh.parent;
      out.constraint = mesh.constraint;
      return true;
    }
  }
  for (const GroupObj& group : scene.groups) {
    if (group.name == name) {
      out.local = group.has_transform ? group.local : Xfm{};
      out.world_stored = group.has_transform ? group.world_stored : Xfm{};
      out.parent = group.parent;
      out.constraint = group.has_transform ? group.constraint : 0;
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

    int mesh_ok = 0, mesh_fail = 0;
    int particle_ok = 0, particle_fail = 0;
    int world_crowd_ok = 0, world_crowd_fail = 0;
    for (const auto& de : dir.entries) {
      std::vector<uint8_t> b(payload.data() + de.offset,
                             payload.data() + de.offset + de.size);
      try {
        if (de.type == "Mesh") {
          MeshObj m = decode_mesh(de.name, b);
          m.dir_index = &de - dir.entries.data();
          if (m.decoded) ++mesh_ok; else ++mesh_fail;
          out.meshes.push_back(std::move(m));
        } else if (de.type == "Trans") {
          out.transes.push_back(decode_trans(de.name, b));
        } else if (de.type == "Mat") {
          out.mats.push_back(decode_mat(de.name, b));
        } else if (de.type == "Cam") {
          out.cams.push_back(decode_cam(de.name, b));
        } else if (de.type == "Waypoint") {
          out.waypoints.push_back(decode_waypoint(de.name, b));
        } else if (de.type == "Spotlight") {
          out.spotlights.push_back(decode_spotlight(de.name, b));
        } else if (de.type == "Light") {
          out.lights.push_back(decode_light(de.name, b));
        } else if (de.type == "Environ") {
          out.environs.push_back(decode_environ(de.name, b));
        } else if (de.type == "Group") {
          GroupObj group = decode_group(de.name, b);
          group.dir_index = &de - dir.entries.data();
          out.groups.push_back(std::move(group));
        } else if (de.type == "BandPlacer") {
          out.band_placers.push_back(decode_band_placer(de.name, b));
        } else if (de.type == "ParticleSys") {
          ParticleSysObj p = decode_particle_sys(de.name, b);
          p.dir_index = &de - dir.entries.data();
          if (p.decoded) ++particle_ok; else ++particle_fail;
          if (p.decoded && debug_particle_decode_enabled()) {
            const auto avg = [](const std::array<float, 4>& low,
                                const std::array<float, 4>& high,
                                size_t i) {
              return (low[i] + high[i]) * 0.5f;
            };
            std::fprintf(
                stderr,
                "[milo_scene] ParticleSys %s material=%s max_parts=%u life_frames=(%.3f %.3f) speed=(%.3f %.3f) emit_rate=(%.3f %.3f) start_size=(%.3f %.3f) delta_size=(%.3f %.3f) start_low=(%.3f %.3f %.3f %.3f) start_high=(%.3f %.3f %.3f %.3f) start_avg=(%.3f %.3f %.3f %.3f) mid_ratio=%.3f mid_low=(%.3f %.3f %.3f %.3f) mid_high=(%.3f %.3f %.3f %.3f) mid_avg=(%.3f %.3f %.3f %.3f) end_low=(%.3f %.3f %.3f %.3f) end_high=(%.3f %.3f %.3f %.3f) end_avg=(%.3f %.3f %.3f %.3f) force=(%.3f %.3f %.3f) grow=%.3f shrink=%.3f bubble=%d bubble_period=(%.3f %.3f) bubble_size=(%.3f %.3f) bounce=%s rel=(%.3f,%s) mesh=%s preserve=%d preserved=%u\n",
                p.name.c_str(), p.material.c_str(),
                p.max_particles,
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
                p.preserve_particles ? 1 : 0, p.preserved_particle_count);
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
    for (auto& m : out.meshes) {
      if (m.vertex_count != 0 || m.geometry_owner.empty() || m.geometry_owner == m.name) continue;
      for (const auto& owner : out.meshes) {
        if (owner.name != m.geometry_owner || owner.vertex_count == 0) continue;
        m.vertex_count = owner.vertex_count;
        m.face_count = owner.face_count;
        m.verts = owner.verts;
        m.indices = owner.indices;
        std::memcpy(m.bb_min, owner.bb_min, sizeof(m.bb_min));
        std::memcpy(m.bb_max, owner.bb_max, sizeof(m.bb_max));
        m.decoded = true;
        m.error.clear();
        break;
      }
    }
    rebuild_group_authored_draw_order(out);
    size_t source_order_groups = 0;
    for (const auto& group : out.groups) {
      if (group.source_order_decoded) ++source_order_groups;
    }
    std::fprintf(stderr,
                 "[milo_scene] %s: %zu meshes (%d ok / %d fail), %zu particles (%d ok / %d fail), %zu trans, %zu mat, %zu cam, %zu waypoint, %zu group (%zu source-order), %zu world_crowd (%d ok / %d fail)\n",
                 milo_path.c_str(), out.meshes.size(), mesh_ok, mesh_fail,
                 out.particles.size(), particle_ok, particle_fail,
                 out.transes.size(), out.mats.size(), out.cams.size(),
                 out.waypoints.size(), out.groups.size(),
                 source_order_groups, out.world_crowds.size(), world_crowd_ok,
                 world_crowd_fail);
    if (!out.spotlights.empty()) {
      size_t transformed = 0;
      for (const auto& spot : out.spotlights)
        if (spot.has_transform) ++transformed;
      std::fprintf(stderr, "[milo_scene]   %zu spotlights decoded (%zu with Trans base)\n",
                   out.spotlights.size(), transformed);
    }
    return true;
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[milo_scene] load_scene(%s): %s\n", milo_path.c_str(),
                 ex.what());
    return false;
  }
}

}  // namespace ghogx::milo_scene
