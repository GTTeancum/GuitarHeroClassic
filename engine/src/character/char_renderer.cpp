// engine/src/character/char_renderer.cpp — see char_renderer.h.

#include "character/char_renderer.h"
#include "render/milo_scene_renderer.h"  // OrbitCamera
#include "render/window_d3d9.h"
#include "render/scene_d3d9.h"           // Mat4
#include "asset/milo_image.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <set>
#include <vector>

namespace ghogx::character {

using ghogx::render::Mat4;
using ghogx::render::OrbitCamera;
using ghogx::render::Window;

namespace {

struct SVtx {
  float x, y, z;
  float nx, ny, nz;
  D3DCOLOR color;
  float u, v;
};
constexpr DWORD kFVF =
    D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;

// A mesh is the flat blob-shadow decal (drawn on the floor) if its name starts
// with "shadow". Skipping it keeps the character clean against the backdrop.
bool is_shadow(const std::string& n) { return n.rfind("shadow", 0) == 0; }

// "_lod1" meshes are lower-quality duplicate body parts drawn alongside the full
// quality mesh. Drawing both causes z-fighting / garbling. Skip LOD1 entirely —
// the bind-pose viewer always uses the highest-quality geometry.
bool is_lod1(const std::string& n) {
  return n.find("_lod1") != std::string::npos || n.rfind("lod_", 0) == 0;
}

const milo_scene::GroupObj* find_character_group(const Character& character,
                                                 const std::string& name) {
  for (const auto& group : character.groups) {
    if (group.name == name) return &group;
  }
  return nullptr;
}

bool character_group_contains_mesh(const Character& character,
                                   const std::string& group_name,
                                   const std::string& mesh_name,
                                   int depth = 0) {
  if (depth > 8) return false;
  const milo_scene::GroupObj* group = find_character_group(character, group_name);
  if (!group) return false;
  for (const auto& child : group->children) {
    if (child == mesh_name) return true;
    if (child.size() >= 4 &&
        child.compare(child.size() - 4, 4, ".grp") == 0 &&
        character_group_contains_mesh(character, child, mesh_name, depth + 1)) {
      return true;
    }
  }
  return false;
}

bool is_hidden_by_character_lod_group(const Character& character,
                                      const SkinnedMesh& mesh) {
  const bool has_lod0 = find_character_group(character, "lod0.grp") != nullptr;
  const bool has_lod1 = find_character_group(character, "lod1.grp") != nullptr;
  if (!has_lod0 || !has_lod1) return false;
  if (character_group_contains_mesh(character, "lod0.grp", mesh.name)) return false;
  return character_group_contains_mesh(character, "lod1.grp", mesh.name);
}

// Numbered dot-variant hair meshes can be real draw members. Rock2's decoded
// PS2 lod0.grp includes hair-back.1.mesh through hair-back.6.mesh, so a global
// "numbered hair is hidden" rule drops authored visible hair. Keep the name
// detector only as a fallback for meshes not explicitly selected by lod0.grp.
bool is_hair_numbered_variant(const std::string& n) {
  if (n.find("hair") == std::string::npos &&
      n.find("Hair") == std::string::npos) {
    return false;
  }
  // Look for ".<digit>." in the name (not at position 0).
  for (size_t i = 1; i + 3 <= n.size(); ++i) {
    if (n[i] == '.' && std::isdigit(static_cast<unsigned char>(n[i+1])) && n[i+2] == '.') {
      return true;
    }
  }
  return false;
}

bool is_hidden_numbered_hair_variant(const Character& character,
                                     const SkinnedMesh& mesh) {
  if (!is_hair_numbered_variant(mesh.name)) return false;
  const bool has_lod0 = find_character_group(character, "lod0.grp") != nullptr;
  if (!has_lod0) return true;
  return !character_group_contains_mesh(character, "lod0.grp", mesh.name);
}

bool is_hair_mesh_name(const std::string& n) {
  std::string lower = n;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return lower.find("hair") != std::string::npos;
}

bool is_hair_material_name(const std::string& n) {
  std::string lower = n;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return lower.find("hair") != std::string::npos;
}

bool is_hair_render_mesh(const SkinnedMesh& m) {
  return is_hair_mesh_name(m.name) || is_hair_material_name(m.material);
}

bool is_bone_parent_name(const std::string& n) {
  return n.rfind("bone_", 0) == 0 || n.rfind("spot_", 0) == 0 ||
         n.find(".mesh") != std::string::npos ||
         n.find(".trans") != std::string::npos;
}

bool is_root_parent_hair_piece(const SkinnedMesh& m) {
  return is_hair_mesh_name(m.name) && !m.bone_palette.empty() &&
         !is_bone_parent_name(m.parent);
}

bool is_unsupported_dynamic_hair(const std::string& n) {
  (void)n;
  return false;
}

bool debug_meshes_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_MESHES") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_MESHES");
  return value && value[0];
#endif
}

bool debug_texture_alpha_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_TEXTURE_ALPHA") == 0 && value &&
      value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_TEXTURE_ALPHA");
  return value && value[0];
#endif
}

DWORD character_cull_mode(const SkinnedMesh* mesh = nullptr) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool has =
      _dupenv_s(&value, &len, "GHOGX_CHARACTER_CULL") == 0 && value && value[0];
  std::string mode = has ? value : "";
  std::free(value);
#else
  const char* raw = std::getenv("GHOGX_CHARACTER_CULL");
  std::string mode = raw ? raw : "";
#endif
  std::transform(mode.begin(), mode.end(), mode.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  if (mode == "cw") return D3DCULL_CW;
  if (mode == "ccw") return D3DCULL_CCW;
  if (mode == "none") return D3DCULL_NONE;
  if (mesh) {
    std::string name = mesh->name;
    std::string material = mesh->material;
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    std::transform(material.begin(), material.end(), material.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    if (name.find("eye") != std::string::npos ||
        name == "lashes.mesh" ||
        material.find("eye") != std::string::npos) {
      return D3DCULL_NONE;
    }
  }
  return D3DCULL_CW;
}

bool disable_mesh_local_arm_skin_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_MESH_LOCAL_ARM_SKIN") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_MESH_LOCAL_ARM_SKIN");
  return value && value[0];
#endif
}

bool debug_bones_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_BONES") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_BONES");
  return value && value[0];
#endif
}

bool debug_prop_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_PROP") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_PROP");
  return value && value[0];
#endif
}

bool hide_attached_props_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_HIDE_ATTACHED_PROPS") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_HIDE_ATTACHED_PROPS");
  return value && value[0];
#endif
}

bool hide_eyes_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_HIDE_EYES") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_HIDE_EYES");
  return value && value[0];
#endif
}

bool skip_material_enabled(const std::string& material) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool has =
      _dupenv_s(&value, &len, "GHOGX_SKIP_MATERIAL") == 0 && value && value[0];
  std::string needle = has ? value : "";
  std::free(value);
#else
  const char* raw = std::getenv("GHOGX_SKIP_MATERIAL");
  std::string needle = raw ? raw : "";
#endif
  if (needle.empty()) return false;
  return material.find(needle) != std::string::npos;
}

bool skip_mesh_enabled(const std::string& mesh) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool has =
      _dupenv_s(&value, &len, "GHOGX_SKIP_MESH") == 0 && value && value[0];
  std::string spec = has ? value : "";
  std::free(value);
#else
  const char* raw = std::getenv("GHOGX_SKIP_MESH");
  std::string spec = raw ? raw : "";
#endif
  if (spec.empty()) return false;
  size_t start = 0;
  while (start <= spec.size()) {
    const size_t comma = spec.find(',', start);
    std::string needle = spec.substr(
        start, comma == std::string::npos ? std::string::npos : comma - start);
    if (!needle.empty() && mesh.find(needle) != std::string::npos) return true;
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  return false;
}

bool only_mesh_enabled(const std::string& mesh) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool has =
      _dupenv_s(&value, &len, "GHOGX_ONLY_MESH") == 0 && value && value[0];
  std::string spec = has ? value : "";
  std::free(value);
#else
  const char* raw = std::getenv("GHOGX_ONLY_MESH");
  std::string spec = raw ? raw : "";
#endif
  if (spec.empty()) return true;
  size_t start = 0;
  while (start <= spec.size()) {
    const size_t comma = spec.find(',', start);
    std::string needle = spec.substr(
        start, comma == std::string::npos ? std::string::npos : comma - start);
    if (!needle.empty() && mesh.find(needle) != std::string::npos) return true;
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  return false;
}

bool highlight_mesh_enabled(const std::string& mesh) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool has =
      _dupenv_s(&value, &len, "GHOGX_HIGHLIGHT_MESH") == 0 && value && value[0];
  std::string spec = has ? value : "";
  std::free(value);
#else
  const char* raw = std::getenv("GHOGX_HIGHLIGHT_MESH");
  std::string spec = raw ? raw : "";
#endif
  if (spec.empty()) return false;
  size_t start = 0;
  while (start <= spec.size()) {
    const size_t comma = spec.find(',', start);
    std::string needle = spec.substr(
        start, comma == std::string::npos ? std::string::npos : comma - start);
    if (!needle.empty() && mesh.find(needle) != std::string::npos) return true;
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  return false;
}

bool debug_mesh_mode_enabled(const std::string& mesh) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool has =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_MESH_MODE") == 0 && value && value[0];
  std::string spec = has ? value : "";
  std::free(value);
#else
  const char* raw = std::getenv("GHOGX_DEBUG_MESH_MODE");
  std::string spec = raw ? raw : "";
#endif
  if (spec.empty()) return false;
  if (spec == "1" || spec == "all") return true;
  size_t start = 0;
  while (start <= spec.size()) {
    const size_t comma = spec.find(',', start);
    std::string needle = spec.substr(
        start, comma == std::string::npos ? std::string::npos : comma - start);
    if (!needle.empty() && mesh.find(needle) != std::string::npos) return true;
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  return false;
}

bool raw_mesh_enabled(const std::string& mesh) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool has =
      _dupenv_s(&value, &len, "GHOGX_RAW_MESH") == 0 && value && value[0];
  std::string spec = has ? value : "";
  std::free(value);
#else
  const char* raw = std::getenv("GHOGX_RAW_MESH");
  std::string spec = raw ? raw : "";
#endif
  if (spec.empty()) return false;
  size_t start = 0;
  while (start <= spec.size()) {
    const size_t comma = spec.find(',', start);
    std::string needle = spec.substr(
        start, comma == std::string::npos ? std::string::npos : comma - start);
    if (!needle.empty() && mesh.find(needle) != std::string::npos) return true;
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  return false;
}

bool is_terminal_lower_leg_palette(const SkinnedMesh& m) {
  if (m.bone_palette.size() != 3) return false;
  const bool left = m.bone_palette[0] == "bone_L-knee.mesh" &&
                    m.bone_palette[1] == "bone_L-ankle.mesh" &&
                    m.bone_palette[2] == "bone_L-toe.mesh";
  const bool right = m.bone_palette[0] == "bone_R-knee.mesh" &&
                     m.bone_palette[1] == "bone_R-ankle.mesh" &&
                     m.bone_palette[2] == "bone_R-toe.mesh";
  return left || right;
}

bool is_mesh_local_terminal_lower_leg_piece(const SkinnedMesh& m) {
  if (m.bone_palette.empty() || m.bind.empty()) return false;
  if (is_shadow(m.name) || is_hair_mesh_name(m.name)) return false;
  if (!is_terminal_lower_leg_palette(m)) return false;
  return m.bb_max[2] < -25.0f;
}

bool is_weighted_root_parent_hair_piece(const SkinnedMesh& m) {
  return is_root_parent_hair_piece(m) && !m.bone_palette.empty();
}

bool is_mesh_local_bind_space_piece(const SkinnedMesh& m) {
  if (!m.mesh_local_bind_space || m.bone_palette.empty() || m.bind.empty()) {
    return false;
  }
  if (is_shadow(m.name)) return false;
  return true;
}

bool has_compact_authored_bounds_near_origin(const SkinnedMesh& m,
                                             float limit = 25.0f) {
  for (int axis = 0; axis < 3; ++axis) {
    if (std::max(std::fabs(m.bb_min[axis]), std::fabs(m.bb_max[axis])) >
        limit) {
      return false;
    }
  }
  return true;
}

bool is_mesh_local_root_hair_piece(const SkinnedMesh& m) {
  return is_weighted_root_parent_hair_piece(m) && !m.bind.empty() &&
         has_compact_authored_bounds_near_origin(m);
}

bool has_suffix(const std::string& n, const char* suffix) {
  const size_t len = std::strlen(suffix);
  return n.size() >= len && n.compare(n.size() - len, len, suffix) == 0;
}

bool contains_case_insensitive(const std::string& n, const char* needle) {
  std::string lower = n;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  std::string lower_needle = needle ? needle : "";
  std::transform(lower_needle.begin(), lower_needle.end(),
                 lower_needle.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return !lower_needle.empty() &&
         lower.find(lower_needle) != std::string::npos;
}

bool is_ankle_toe_palette(const SkinnedMesh& m) {
  if (m.bone_palette.size() != 2) return false;
  const bool left = m.bone_palette[0] == "bone_L-ankle.mesh" &&
                    m.bone_palette[1] == "bone_L-toe.mesh";
  const bool right = m.bone_palette[0] == "bone_R-ankle.mesh" &&
                     m.bone_palette[1] == "bone_R-toe.mesh";
  return left || right;
}

bool is_terminal_leg_overlay_duplicate(const SkinnedMesh& m) {
  if (m.bone_palette.empty() || m.bind.empty()) return false;
  if (is_shadow(m.name) || is_hair_mesh_name(m.name)) return false;
  if (!is_ankle_toe_palette(m)) return false;
  if (!has_suffix(m.parent, ".mesh")) return false;
  return contains_case_insensitive(m.name, "leg") &&
         contains_case_insensitive(m.parent, "leg") &&
         contains_case_insensitive(m.material, "leg");
}

bool contains_arm_token(const std::string& n) {
  std::string lower = n;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return lower.find("arm") != std::string::npos;
}

bool palette_contains(const SkinnedMesh& m, const char* fragment) {
  for (const auto& bone : m.bone_palette) {
    if (bone.find(fragment) != std::string::npos) return true;
  }
  return false;
}

bool is_mesh_local_arm_piece(const SkinnedMesh& m) {
  if (m.bone_palette.empty() || m.bind.empty()) return false;
  if (is_shadow(m.name) || is_hair_mesh_name(m.name)) return false;
  if (!has_compact_authored_bounds_near_origin(m)) return false;
  return contains_arm_token(m.material) &&
         (contains_arm_token(m.name) || contains_arm_token(m.parent));
}

bool is_far_negative_mesh_parented_arm_piece(const SkinnedMesh& m) {
  if (m.bone_palette.empty()) return false;
  if (is_shadow(m.name) || is_hair_mesh_name(m.name)) return false;
  if (!has_suffix(m.parent, ".mesh")) return false;
  if (contains_arm_token(m.parent)) return false;
  if (!contains_arm_token(m.material)) return false;
  if (!(m.bb_min[2] < -10.0f && m.bb_max[2] < -4.0f)) return false;
  return palette_contains(m, "clavicle") ||
         palette_contains(m, "upperArm") ||
         palette_contains(m, "foreArm") ||
         palette_contains(m, "hand") ||
         palette_contains(m, "upperTwist");
}

bool is_compact_mesh_parented_head_detail_piece(const SkinnedMesh& m) {
  if (m.bone_palette.size() != 3) return false;
  if (is_shadow(m.name) || is_hair_mesh_name(m.name)) return false;
  if (!has_suffix(m.parent, ".mesh")) return false;
  if (!has_compact_authored_bounds_near_origin(m, 8.0f)) return false;
  return palette_contains(m, "bone_neck.mesh") &&
         palette_contains(m, "bone_head.mesh");
}

bool is_raw_mesh_world_authored_piece(const SkinnedMesh& m) {
  return is_far_negative_mesh_parented_arm_piece(m) ||
         is_compact_mesh_parented_head_detail_piece(m);
}

bool is_parent_local_ankle_attachment(const SkinnedMesh& m) {
  if (!m.bone_palette.empty()) return false;
  if (is_shadow(m.name) || is_hair_mesh_name(m.name)) {
    return false;
  }
  std::string material = m.material;
  std::transform(material.begin(), material.end(), material.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  const bool ankle_parent = m.parent == "bone_L-ankle.mesh" ||
                            m.parent == "bone_R-ankle.mesh";
  return ankle_parent && material.find("leg") != std::string::npos;
}

bool is_head_attachment_mesh(const SkinnedMesh& m) {
  return m.name == "lashes.mesh" && m.bone_palette.empty();
}

bool is_model_space_head_hair_attachment(const SkinnedMesh& m) {
  if (!is_hair_mesh_name(m.name) || !m.bone_palette.empty()) return false;
  if (m.parent != "bone_head.mesh") return false;
  return m.bb_min[0] > -25.0f && m.bb_max[0] < 25.0f &&
         m.bb_min[1] > -25.0f && m.bb_max[1] < 25.0f &&
         m.bb_min[2] > 30.0f;
}

bool is_local_space_head_hair_attachment(const SkinnedMesh& m) {
  if (!is_hair_mesh_name(m.name) || !m.bone_palette.empty()) return false;
  if (m.parent != "bone_head.mesh") return false;
  float mn[3] = {0, 0, 0};
  float mx[3] = {0, 0, 0};
  bool first = true;
  for (int xi = 0; xi < 2; ++xi) {
    for (int yi = 0; yi < 2; ++yi) {
      for (int zi = 0; zi < 2; ++zi) {
        const float x = xi ? m.bb_max[0] : m.bb_min[0];
        const float y = yi ? m.bb_max[1] : m.bb_min[1];
        const float z = zi ? m.bb_max[2] : m.bb_min[2];
        const float p[3] = {
            x * m.local.rot[0][0] + y * m.local.rot[1][0] +
                z * m.local.rot[2][0] + m.local.pos[0],
            x * m.local.rot[0][1] + y * m.local.rot[1][1] +
                z * m.local.rot[2][1] + m.local.pos[1],
            x * m.local.rot[0][2] + y * m.local.rot[1][2] +
                z * m.local.rot[2][2] + m.local.pos[2],
        };
        if (first) {
          for (int i = 0; i < 3; ++i) mn[i] = mx[i] = p[i];
          first = false;
        } else {
          for (int i = 0; i < 3; ++i) {
            mn[i] = std::min(mn[i], p[i]);
            mx[i] = std::max(mx[i], p[i]);
          }
        }
      }
    }
  }
  const float cx = (mn[0] + mx[0]) * 0.5f;
  const float cy = (mn[1] + mx[1]) * 0.5f;
  const float cz = (mn[2] + mx[2]) * 0.5f;
  return mn[0] > -25.0f && mx[0] < 25.0f &&
         mn[1] > -25.0f && mx[1] < 25.0f &&
         mn[2] > -25.0f && mx[2] < 25.0f &&
         (cx * cx + cy * cy + cz * cz) < (25.0f * 25.0f);
}

bool debug_skin_bounds_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_SKIN_BOUNDS") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_SKIN_BOUNDS");
  return value && value[0];
#endif
}

bool debug_weight_stats_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_WEIGHT_STATS") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_WEIGHT_STATS");
  return value && value[0];
#endif
}

bool debug_skin_matrix_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_SKIN_MATRIX") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_SKIN_MATRIX");
  return value && value[0];
#endif
}

bool debug_skin_matrix_all_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_SKIN_MATRIX_ALL") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_SKIN_MATRIX_ALL");
  return value && value[0];
#endif
}

bool debug_hair_space_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_HAIR_SPACE") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_HAIR_SPACE");
  return value && value[0];
#endif
}

bool use_mesh_bind_material_enabled(const std::string& material) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool has =
      _dupenv_s(&value, &len, "GHOGX_USE_MESH_BIND_MATERIAL") == 0 && value && value[0];
  std::string needle = has ? value : "";
  std::free(value);
#else
  const char* raw = std::getenv("GHOGX_USE_MESH_BIND_MATERIAL");
  std::string needle = raw ? raw : "";
#endif
  if (needle.empty()) return false;
  return material.find(needle) != std::string::npos;
}

bool use_mesh_bind_inverse_material_enabled(const std::string& material) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool has =
      _dupenv_s(&value, &len, "GHOGX_USE_MESH_BIND_INVERSE_MATERIAL") == 0 && value && value[0];
  std::string needle = has ? value : "";
  std::free(value);
#else
  const char* raw = std::getenv("GHOGX_USE_MESH_BIND_INVERSE_MATERIAL");
  std::string needle = raw ? raw : "";
#endif
  if (needle.empty()) return false;
  return material.find(needle) != std::string::npos;
}

bool disable_local_hair_attachment_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_LOCAL_HAIR_ATTACHMENT") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_LOCAL_HAIR_ATTACHMENT");
  return value && value[0];
#endif
}

bool reverse_skin_weight_slots_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_REVERSE_SKIN_WEIGHT_SLOTS") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_REVERSE_SKIN_WEIGHT_SLOTS");
  return value && value[0];
#endif
}

std::string skin_matrix_mode() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool has =
      _dupenv_s(&value, &len, "GHOGX_SKIN_MATRIX_MODE") == 0 && value && value[0];
  std::string mode = has ? value : "";
  std::free(value);
  return mode;
#else
  const char* raw = std::getenv("GHOGX_SKIN_MATRIX_MODE");
  return raw ? raw : "";
#endif
}

std::string local_hair_skin_matrix_mode() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool has =
      _dupenv_s(&value, &len, "GHOGX_LOCAL_HAIR_SKIN_MATRIX_MODE") == 0 &&
      value && value[0];
  std::string mode = has ? value : "";
  std::free(value);
  return mode;
#else
  const char* raw = std::getenv("GHOGX_LOCAL_HAIR_SKIN_MATRIX_MODE");
  return raw ? raw : "";
#endif
}

std::string local_hair_world_mode() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool has =
      _dupenv_s(&value, &len, "GHOGX_LOCAL_HAIR_WORLD_MODE") == 0 &&
      value && value[0];
  std::string mode = has ? value : "";
  std::free(value);
  return mode;
#else
  const char* raw = std::getenv("GHOGX_LOCAL_HAIR_WORLD_MODE");
  return raw ? raw : "";
#endif
}

bool env_eye_inset(float& out) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool found =
      _dupenv_s(&value, &len, "GHOGX_EYE_INSET") == 0 && value && value[0];
  if (found)
    out = std::strtof(value, nullptr);
  std::free(value);
  return found;
#else
  const char* value = std::getenv("GHOGX_EYE_INSET");
  if (!value || !value[0]) return false;
  out = std::strtof(value, nullptr);
  return true;
#endif
}

bool is_eye_mesh(const std::string& n) {
  std::string lower = n;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return lower.rfind("eye-", 0) == 0 ||
         lower.rfind("l-eye.", 0) == 0 ||
         lower.rfind("r-eye.", 0) == 0 ||
         lower.find("_eye") != std::string::npos ||
         lower.find("eyel.") != std::string::npos ||
         lower.find("eyer.") != std::string::npos;
}

float eye_surface_inset(const SkinnedMesh& m) {
  (void)m;
  float override_inset = 0.0f;
  if (env_eye_inset(override_inset)) return override_inset;
  return 0.0f;
}

bool is_front_hair_mesh(const std::string& n) {
  return n.rfind("hair_front", 0) == 0 || n.rfind("Hair_front", 0) == 0;
}

bool uses_local_attachment_skin(const SkinnedMesh& m) {
  if (disable_local_hair_attachment_enabled()) return false;
  if (!is_hair_mesh_name(m.name) || m.bone_palette.empty()) return false;

  if (m.parent != "bone_head.mesh") return false;

  // CharHair pieces are authored as head-local attachments. Some are stored in
  // a compact head-local range around the origin; others are offset farther out
  // in the same parent space. Body-space hair should stay on normal LBS.
  const bool compact_head_local =
      m.bb_min[0] > -25.0f && m.bb_max[0] < 25.0f &&
      m.bb_min[1] > -25.0f && m.bb_max[1] < 25.0f &&
      m.bb_min[2] > -25.0f && m.bb_max[2] < 25.0f;
  const bool offset_head_local =
      m.bb_max[0] > 20.0f && m.bb_min[2] > -25.0f && m.bb_max[2] < 25.0f;
  return compact_head_local || offset_head_local;
}

bool runtime_hair_world_override(const Character& character,
                                 const std::string& bone_name,
                                 std::array<float, 16>& out) {
  for (const auto& point : character.runtime_hair.points) {
    if (!point.initialized || !point.has_world) continue;
    if (point.mesh != bone_name) continue;
    out = point.world;
    return true;
  }
  return false;
}

std::array<float, 16> mul16(const std::array<float, 16>& a,
                            const std::array<float, 16>& b);
std::array<float, 16> affine_inverse(const std::array<float, 16>& m);
std::array<float, 16> xfm16(const milo_scene::Xfm& x);
std::array<float, 16> raw_current_world(const Character& character,
                                        const std::string& name);
std::array<float, 16> scene_object_world(const milo_scene::Scene& scene,
                                         const std::string& name);

std::array<float, 16> prop_attach_world(const Character& character,
                                        const std::string& attach_bone) {
  // Accepted prop traces route guitars/mics through the same moving Trans rows
  // as skinned character output. Keep instruments in the character local-chain
  // basis instead of the stored-world correction used for a few rigid meshes.
  return character.bone_world_local_chain(attach_bone);
}

void log_matrix_row(const char* tag,
                    const std::string& mesh,
                    const std::string& bone,
                    const std::array<float, 16>& m) {
  std::fprintf(stderr,
               "[hair-space] %-12s mesh=%-16s bone=%-20s "
               "r0=(%.4f %.4f %.4f %.4f) "
               "r1=(%.4f %.4f %.4f %.4f) "
               "r2=(%.4f %.4f %.4f %.4f) "
               "pos=(%.4f %.4f %.4f %.4f)\n",
               tag, mesh.c_str(), bone.c_str(),
               m[0], m[1], m[2], m[3],
               m[4], m[5], m[6], m[7],
               m[8], m[9], m[10], m[11],
               m[12], m[13], m[14], m[15]);
}

void log_compact_matrix_rows(const char* tag,
                             const std::string& mesh,
                             const std::string& bone,
                             const std::array<float, 16>& m) {
  for (int row = 0; row < 4; ++row) {
    std::fprintf(stderr,
                 "[skin-row-%s] mesh=%s bone=%s row=%d "
                 "%.5f %.5f %.5f %.5f\n",
                 tag, mesh.c_str(), bone.c_str(), row, m[row * 4 + 0],
                 m[row * 4 + 1], m[row * 4 + 2], m[row * 4 + 3]);
  }
}

int wrapped_texel_coord(float uv, int size) {
  if (size <= 0 || !std::isfinite(uv)) return 0;
  const float wrapped = uv - std::floor(uv);
  int coord = static_cast<int>(wrapped * static_cast<float>(size));
  if (coord < 0) coord = 0;
  if (coord >= size) coord = size - 1;
  return coord;
}

int clamped_texel_coord(float uv, int size) {
  if (size <= 0 || !std::isfinite(uv)) return 0;
  const float clamped = std::clamp(uv, 0.0f, 1.0f);
  int coord = static_cast<int>(clamped * static_cast<float>(size - 1));
  if (coord < 0) coord = 0;
  if (coord >= size) coord = size - 1;
  return coord;
}

int mirrored_texel_coord(float uv, int size) {
  if (size <= 0 || !std::isfinite(uv)) return 0;
  float t = std::fmod(std::abs(uv), 2.0f);
  if (t > 1.0f) t = 2.0f - t;
  int coord = static_cast<int>(t * static_cast<float>(size - 1));
  if (coord < 0) coord = 0;
  if (coord >= size) coord = size - 1;
  return coord;
}

uint8_t sample_alpha_mode(const ghogx::asset::Image& img, float u, float v,
                          const char* mode) {
  if (!img.valid()) return 255;
  int x = 0;
  int y = 0;
  if (std::strcmp(mode, "clamp") == 0) {
    x = clamped_texel_coord(u, img.width);
    y = clamped_texel_coord(v, img.height);
  } else if (std::strcmp(mode, "mirror") == 0) {
    x = mirrored_texel_coord(u, img.width);
    y = mirrored_texel_coord(v, img.height);
  } else {
    x = wrapped_texel_coord(u, img.width);
    y = wrapped_texel_coord(v, img.height);
  }
  const size_t idx = (static_cast<size_t>(y) * img.width + x) * 4 + 3;
  return idx < img.rgba.size() ? img.rgba[idx] : 255;
}

uint8_t sample_alpha(const ghogx::asset::Image& img, float u, float v) {
  return sample_alpha_mode(img, u, v, "wrap");
}

void log_texture_alpha_stats(const SkinnedMesh& mesh,
                             const milo_scene::MatObj* material,
                             const ghogx::asset::Image* image) {
  if (!image || !image->valid() || mesh.verts.empty()) return;
  float min_u = mesh.verts[0].u;
  float max_u = mesh.verts[0].u;
  float min_v = mesh.verts[0].v;
  float max_v = mesh.verts[0].v;
  int vert_zero = 0;
  int vert_lt32 = 0;
  int vert_lt96 = 0;
  int vert_opaque = 0;
  for (const auto& vert : mesh.verts) {
    min_u = std::min(min_u, vert.u);
    max_u = std::max(max_u, vert.u);
    min_v = std::min(min_v, vert.v);
    max_v = std::max(max_v, vert.v);
    const uint8_t a = sample_alpha(*image, vert.u, vert.v);
    if (a == 0) ++vert_zero;
    if (a < 32) ++vert_lt32;
    if (a < 96) ++vert_lt96;
    if (a >= 250) ++vert_opaque;
  }

  int tri_zero = 0;
  int tri_lt32 = 0;
  int tri_lt96 = 0;
  int tri_opaque = 0;
  int tri_samples = 0;
  for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
    const uint16_t ia = mesh.indices[i + 0];
    const uint16_t ib = mesh.indices[i + 1];
    const uint16_t ic = mesh.indices[i + 2];
    if (ia >= mesh.verts.size() || ib >= mesh.verts.size() ||
        ic >= mesh.verts.size()) {
      continue;
    }
    const auto& a = mesh.verts[ia];
    const auto& b = mesh.verts[ib];
    const auto& c = mesh.verts[ic];
    const float u = (a.u + b.u + c.u) / 3.0f;
    const float v = (a.v + b.v + c.v) / 3.0f;
    const uint8_t alpha = sample_alpha(*image, u, v);
    ++tri_samples;
    if (alpha == 0) ++tri_zero;
    if (alpha < 32) ++tri_lt32;
    if (alpha < 96) ++tri_lt96;
    if (alpha >= 250) ++tri_opaque;
  }

  std::fprintf(stderr,
               "[tex-alpha] %-24s mat=%-18s tex=%-24s size=%dx%d",
               mesh.name.c_str(), mesh.material.c_str(),
               material ? material->diffuse_tex.c_str() : "", image->width,
               image->height);
  std::fprintf(stderr, " uv=(%.3f..%.3f, %.3f..%.3f)", min_u, max_u,
               min_v, max_v);
  std::fprintf(stderr, " verts=%llu a0=%d a<32=%d a<96=%d opaque=%d",
               static_cast<unsigned long long>(mesh.verts.size()), vert_zero,
               vert_lt32, vert_lt96, vert_opaque);
  std::fprintf(stderr, " tris=%d a0=%d a<32=%d a<96=%d opaque=%d\n",
               tri_samples, tri_zero, tri_lt32, tri_lt96, tri_opaque);

  auto count_mode = [&](const char* mode, int& zero, int& lt32, int& lt96,
                        int& opaque) {
    zero = lt32 = lt96 = opaque = 0;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
      const uint16_t ia = mesh.indices[i + 0];
      const uint16_t ib = mesh.indices[i + 1];
      const uint16_t ic = mesh.indices[i + 2];
      if (ia >= mesh.verts.size() || ib >= mesh.verts.size() ||
          ic >= mesh.verts.size()) {
        continue;
      }
      const auto& a = mesh.verts[ia];
      const auto& b = mesh.verts[ib];
      const auto& c = mesh.verts[ic];
      const float u = (a.u + b.u + c.u) / 3.0f;
      const float v = (a.v + b.v + c.v) / 3.0f;
      const uint8_t alpha = sample_alpha_mode(*image, u, v, mode);
      if (alpha == 0) ++zero;
      if (alpha < 32) ++lt32;
      if (alpha < 96) ++lt96;
      if (alpha >= 250) ++opaque;
    }
  };
  int cz = 0, c32 = 0, c96 = 0, co = 0;
  int mz = 0, m32 = 0, m96 = 0, mo = 0;
  count_mode("clamp", cz, c32, c96, co);
  count_mode("mirror", mz, m32, m96, mo);
  std::fprintf(stderr,
               "[tex-alpha-address] %-24s wrap(a<96=%d opaque=%d) "
               "clamp(a<96=%d opaque=%d) mirror(a<96=%d opaque=%d)\n",
               mesh.name.c_str(), tri_lt96, tri_opaque, c96, co, m96, mo);
}

void log_vertex_float_stats(const SkinnedMesh& mesh) {
  if (mesh.verts.empty()) return;
  float mn[4] = {mesh.verts[0].w[0], mesh.verts[0].w[1], mesh.verts[0].w[2],
                 mesh.verts[0].w[3]};
  float mx[4] = {mn[0], mn[1], mn[2], mn[3]};
  int zeros[4] = {};
  int ones[4] = {};
  for (const auto& v : mesh.verts) {
    for (int i = 0; i < 4; ++i) {
      mn[i] = std::min(mn[i], v.w[i]);
      mx[i] = std::max(mx[i], v.w[i]);
      if (std::abs(v.w[i]) < 1e-5f) ++zeros[i];
      if (std::abs(v.w[i] - 1.0f) < 1e-5f) ++ones[i];
    }
  }
  std::fprintf(stderr,
               "[vertex-floats] %-24s palette=%zu mat=%-18s "
               "f0=(%.3f..%.3f z=%d one=%d) "
               "f1=(%.3f..%.3f z=%d one=%d) "
               "f2=(%.3f..%.3f z=%d one=%d) "
               "f3=(%.3f..%.3f z=%d one=%d)\n",
               mesh.name.c_str(), mesh.bone_palette.size(),
               mesh.material.c_str(), mn[0], mx[0], zeros[0], ones[0], mn[1],
               mx[1], zeros[1], ones[1], mn[2], mx[2], zeros[2], ones[2],
               mn[3], mx[3], zeros[3], ones[3]);
}

}  // namespace

struct CharRenderer::Impl {
  Window* win = nullptr;
  IDirect3DDevice9* dev = nullptr;
  Character character;
  OrbitCamera cam;
  std::map<std::string, IDirect3DTexture9*> tex;  // keyed by .tex entry name
  std::map<std::string, ghogx::asset::Image> tex_images;
  std::set<std::string> logged_texture_alpha_meshes;
  milo_scene::Scene prop_scene;
  std::map<std::string, IDirect3DTexture9*> prop_tex;
  std::string prop_attach_bone;
  bool has_prop = false;
  bool logged_prop_debug = false;
  float bb_min[3] = {0, 0, 0};
  float bb_max[3] = {0, 0, 0};
  bool have_bounds = false;
  float world_offset[3] = {0.0f, 0.0f, 0.0f};
  std::array<float, 16> world_transform = {1, 0, 0, 0, 0, 1, 0, 0,
                                           0, 0, 1, 0, 0, 0, 0, 1};

  // Procedural idle animation time (seconds).
  float anim_t = 0.0f;
  // Original bone local Xfm values saved at set_character() for restore-before-update.
  std::vector<milo_scene::Xfm> original_bone_local;
  std::vector<milo_scene::Xfm> original_mesh_local;
};

CharRenderer::CharRenderer(Window& win) : impl_(new Impl) {
  impl_->win = &win;
  impl_->dev = static_cast<IDirect3DDevice9*>(win.device_ptr());
}

CharRenderer::~CharRenderer() {
  for (auto& kv : impl_->tex)
    if (kv.second) kv.second->Release();
  for (auto& kv : impl_->prop_tex)
    if (kv.second) kv.second->Release();
  delete impl_;
}

OrbitCamera& CharRenderer::camera() { return impl_->cam; }
Character& CharRenderer::character() { return impl_->character; }

void CharRenderer::set_world_offset(float x, float y, float z) {
  impl_->world_offset[0] = x;
  impl_->world_offset[1] = y;
  impl_->world_offset[2] = z;
  impl_->world_transform = {1, 0, 0, 0, 0, 1, 0, 0,
                            0, 0, 1, 0, x, y, z, 1};
}

void CharRenderer::set_world_transform(const std::array<float, 16>& m) {
  impl_->world_transform = m;
  impl_->world_offset[0] = impl_->world_offset[1] = impl_->world_offset[2] = 0.0f;
}

std::optional<std::array<float, 16>> CharRenderer::attached_prop_world(
    std::string_view object_name) const {
  const auto& impl = *impl_;
  if (!impl.has_prop || impl.prop_attach_bone.empty()) return std::nullopt;

  auto matches = [&](std::string_view candidate) {
    if (candidate == object_name) return true;
    constexpr std::string_view suffix = ".mesh";
    if (candidate.size() > suffix.size() &&
        candidate.substr(candidate.size() - suffix.size()) == suffix &&
        candidate.substr(0, candidate.size() - suffix.size()) == object_name) {
      return true;
    }
    if (object_name.size() > suffix.size() &&
        object_name.substr(object_name.size() - suffix.size()) == suffix &&
        object_name.substr(0, object_name.size() - suffix.size()) ==
            candidate) {
      return true;
    }
    return false;
  };

  for (const auto& mesh : impl.prop_scene.meshes) {
    if (!mesh.decoded || !matches(mesh.name)) continue;
    const auto attach_world =
        prop_attach_world(impl.character, impl.prop_attach_bone);
    const auto prop_anchor_world =
        scene_object_world(impl.prop_scene, impl.prop_attach_bone);
    const auto prop_to_attach =
        mul16(affine_inverse(prop_anchor_world), attach_world);
    auto world = mul16(impl.prop_scene.world_matrix(mesh), prop_to_attach);
    world = mul16(world, impl.world_transform);
    return world;
  }
  return std::nullopt;
}

IDirect3DTexture9* CharRenderer::upload(const ghogx::asset::Image& img) {
  if (!impl_->dev || !img.valid()) return nullptr;
  IDirect3DTexture9* t = nullptr;
  if (FAILED(impl_->dev->CreateTexture(static_cast<UINT>(img.width),
                                       static_cast<UINT>(img.height), 1, 0,
                                       D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t,
                                       nullptr)))
    return nullptr;
  D3DLOCKED_RECT lr;
  if (SUCCEEDED(t->LockRect(0, &lr, nullptr, 0))) {
    for (int y = 0; y < img.height; ++y) {
      auto* dst = static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch;
      const uint8_t* src =
          img.rgba.data() + static_cast<size_t>(y) * img.width * 4;
      for (int x = 0; x < img.width; ++x) {
        dst[x * 4 + 0] = src[x * 4 + 2];  // R->B
        dst[x * 4 + 1] = src[x * 4 + 1];  // G
        dst[x * 4 + 2] = src[x * 4 + 0];  // B->R
        dst[x * 4 + 3] = src[x * 4 + 3];  // A
      }
    }
    t->UnlockRect(0);
  }
  return t;
}

void CharRenderer::set_character(
    Character character,
    const std::map<std::string, ghogx::asset::Image>& textures) {
  impl_->character = std::move(character);
  impl_->tex_images = textures;
  impl_->logged_texture_alpha_meshes.clear();

  // Save original bind-pose bone local transforms for restore-before-update.
  impl_->original_bone_local.clear();
  impl_->original_bone_local.reserve(impl_->character.bones.size());
  for (const auto& b : impl_->character.bones)
    impl_->original_bone_local.push_back(b.local);
  impl_->original_mesh_local.clear();
  impl_->original_mesh_local.reserve(impl_->character.meshes.size());
  for (const auto& m : impl_->character.meshes)
    impl_->original_mesh_local.push_back(m.local);
  if (debug_bones_enabled()) {
    for (const auto& b : impl_->character.bones) {
      std::fprintf(stderr,
                   "[bone] %-28s parent=%-28s local=(%.3f %.3f %.3f) "
                   "world=(%.3f %.3f %.3f)\n",
                   b.name.c_str(), b.parent.c_str(), b.local.pos[0],
                   b.local.pos[1], b.local.pos[2], b.world_stored.pos[0],
                   b.world_stored.pos[1], b.world_stored.pos[2]);
    }
  }
  if (debug_meshes_enabled()) {
    for (const auto& mat : impl_->character.mats) {
      std::fprintf(stderr,
                   "[mat] %-24s tex=%-24s color=(%.3f %.3f %.3f %.3f) "
                   "blend=0x%02x\n",
                   mat.name.c_str(), mat.diffuse_tex.c_str(), mat.color[0],
                   mat.color[1], mat.color[2], mat.color[3], mat.blend);
    }
    for (const auto& group : impl_->character.groups) {
      std::fprintf(stderr, "[char-group] %s children=%zu", group.name.c_str(),
                   group.children.size());
      for (const auto& child : group.children)
        std::fprintf(stderr, " %s", child.c_str());
      std::fprintf(stderr, "\n");
    }
    for (const auto& m : impl_->character.meshes) {
      float mn[3] = {0, 0, 0};
      float mx[3] = {0, 0, 0};
      if (!m.verts.empty()) {
        mn[0] = mx[0] = m.verts[0].px;
        mn[1] = mx[1] = m.verts[0].py;
        mn[2] = mx[2] = m.verts[0].pz;
        for (const auto& v : m.verts) {
          mn[0] = std::min(mn[0], v.px); mx[0] = std::max(mx[0], v.px);
          mn[1] = std::min(mn[1], v.py); mx[1] = std::max(mx[1], v.py);
          mn[2] = std::min(mn[2], v.pz); mx[2] = std::max(mx[2], v.pz);
        }
      }
      std::fprintf(stderr,
                   "[mesh] %-32s parent=%-24s mat=%-20s show=%d verts=%zu faces=%zu palette=%zu bbox=(%.3f %.3f %.3f)..(%.3f %.3f %.3f)\n",
                   m.name.c_str(), m.parent.c_str(), m.material.c_str(),
                   m.showing ? 1 : 0, m.verts.size(), m.indices.size() / 3,
                   m.bone_palette.size(), mn[0], mn[1], mn[2], mx[0], mx[1],
                   mx[2]);
      if (debug_mesh_mode_enabled(m.name)) {
        std::fprintf(stderr,
                     "[mesh-xfm] %-32s local=(%.3f %.3f %.3f) "
                     "worldStored=(%.3f %.3f %.3f)\n",
                     m.name.c_str(), m.local.pos[0], m.local.pos[1],
                     m.local.pos[2], m.world_stored.pos[0],
                     m.world_stored.pos[1], m.world_stored.pos[2]);
      }
      if (!m.bone_palette.empty()) {
        float wsum[4] = {};
        for (const auto& v : m.verts) {
          for (int wi = 0; wi < 4; ++wi) wsum[wi] += v.w[wi];
        }
        std::fprintf(stderr, "[mesh-palette] %s weights=(%.3f %.3f %.3f %.3f)",
                     m.name.c_str(), wsum[0], wsum[1], wsum[2], wsum[3]);
        for (const auto& p : m.bone_palette) {
          std::fprintf(stderr, " %s", p.c_str());
        }
        std::fprintf(stderr, "\n");
      }
    }
  }

  for (auto& kv : impl_->tex)
    if (kv.second) kv.second->Release();
  impl_->tex.clear();
  for (const auto& kv : textures) {
    IDirect3DTexture9* t = upload(kv.second);
    if (t) impl_->tex[kv.first] = t;
  }
  std::fprintf(stderr, "[char3d] uploaded %zu/%zu textures\n", impl_->tex.size(),
               textures.size());
  frame_camera();
}

void CharRenderer::set_attached_prop(
    milo_scene::Scene scene,
    const std::map<std::string, ghogx::asset::Image>& textures,
    const std::string& attach_bone) {
  for (auto& kv : impl_->prop_tex)
    if (kv.second) kv.second->Release();
  impl_->prop_tex.clear();
  impl_->prop_scene = std::move(scene);
  impl_->prop_attach_bone = attach_bone;
  impl_->has_prop = true;
  for (const auto& kv : textures) {
    IDirect3DTexture9* t = upload(kv.second);
    if (t) impl_->prop_tex[kv.first] = t;
  }
  std::fprintf(stderr, "[char3d] prop '%s' attached to %s, textures %zu/%zu\n",
               impl_->prop_scene.dir_name.c_str(), attach_bone.c_str(),
               impl_->prop_tex.size(), textures.size());
}

void CharRenderer::frame_camera() {
  impl_->have_bounds = false;
  // Bind pose = stored model-space positions; bound the non-shadow body meshes.
  for (const auto& m : impl_->character.meshes) {
    if (!m.decoded || !m.showing || m.verts.empty() || is_shadow(m.name) ||
        is_hidden_by_character_lod_group(impl_->character, m) ||
        is_lod1(m.name) ||
        is_hidden_numbered_hair_variant(impl_->character, m) ||
        is_unsupported_dynamic_hair(m.name) ||
        is_terminal_leg_overlay_duplicate(m)) continue;
    for (const auto& v : m.verts) {
      const float p[3] = {v.px, v.py, v.pz};
      if (!impl_->have_bounds) {
        for (int k = 0; k < 3; ++k) impl_->bb_min[k] = impl_->bb_max[k] = p[k];
        impl_->have_bounds = true;
      } else {
        for (int k = 0; k < 3; ++k) {
          impl_->bb_min[k] = std::min(impl_->bb_min[k], p[k]);
          impl_->bb_max[k] = std::max(impl_->bb_max[k], p[k]);
        }
      }
    }
  }
  OrbitCamera& c = impl_->cam;
  if (!impl_->have_bounds) return;
  float ctr[3], ext = 0.0f;
  for (int k = 0; k < 3; ++k) {
    ctr[k] = 0.5f * (impl_->bb_min[k] + impl_->bb_max[k]);
    ext = std::max(ext, impl_->bb_max[k] - impl_->bb_min[k]);
  }
  for (int k = 0; k < 3; ++k) c.target[k] = ctr[k];
  // A character is tall in Z; frame the full height with headroom.
  c.distance = std::max(ext * 1.85f, 5.0f);
  c.near_z = std::max(c.distance * 0.01f, 0.2f);
  c.far_z = c.distance * 6.0f + ext * 2.0f;
  // Face the front, slightly elevated. In this camera convention yaw=pi looks
  // from +Y toward the model, which matches the character's authored facing.
  c.pitch = 0.18f;
  c.yaw = 3.14159265f;
  c.fov = 0.6f;
  std::fprintf(stderr,
               "[char3d] bind-pose extent [%.1f %.1f %.1f]..[%.1f %.1f %.1f] "
               "target=(%.1f %.1f %.1f) dist=%.1f\n",
               impl_->bb_min[0], impl_->bb_min[1], impl_->bb_min[2],
               impl_->bb_max[0], impl_->bb_max[1], impl_->bb_max[2],
               c.target[0], c.target[1], c.target[2], c.distance);
}

void CharRenderer::update(float dt) {
  impl_->anim_t += dt;

  // Restore ALL bones to their bind-pose local transforms. The clip (applied by
  // the caller AFTER update()) then modifies only the bones it animates, starting
  // from a clean bind each frame. No procedural animation here — earlier code
  // applied a bone_spine2 sine-wave sway every frame, which contaminated clip
  // playback (the torso wobbled independently of the clip). Removed.
  for (size_t i = 0; i < impl_->character.bones.size() &&
                     i < impl_->original_bone_local.size(); ++i)
    impl_->character.bones[i].local = impl_->original_bone_local[i];
  for (size_t i = 0; i < impl_->character.meshes.size() &&
                     i < impl_->original_mesh_local.size(); ++i)
    impl_->character.meshes[i].local = impl_->original_mesh_local[i];
}

void CharRenderer::draw_impl(bool clear_target) {
  auto& impl = *impl_;
  IDirect3DDevice9* dev = impl.dev;
  if (!dev) return;
  Window* win = impl.win;
  OrbitCamera& cam = impl.cam;

  const float aspect =
      win->bb_height() > 0
          ? static_cast<float>(win->bb_width()) / static_cast<float>(win->bb_height())
          : 16.0f / 9.0f;

  float eye[3];
  cam.eye(eye);
  const float* at = cam.authored ? cam.authored_at : cam.target;
  const float* up = cam.authored ? cam.authored_up : nullptr;
  Mat4 view = Mat4::look_at_lh(eye[0], eye[1], eye[2], at[0], at[1], at[2],
                               up ? up[0] : 0.0f, up ? up[1] : 0.0f,
                               up ? up[2] : 1.0f);
  Mat4 proj = Mat4::perspective_lh(cam.fov, aspect, cam.near_z, cam.far_z);
  proj.m[0][0] = -proj.m[0][0];  // RH world -> LH clip (no left/right flip)

  if (clear_target) {
    dev->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
               D3DCOLOR_XRGB(24, 26, 38), 1.0f, 0);
  }
  dev->BeginScene();

  D3DMATRIX dv, dp;
  std::memcpy(&dv, &view, 64);
  std::memcpy(&dp, &proj, 64);
  dev->SetTransform(D3DTS_VIEW, &dv);
  dev->SetTransform(D3DTS_PROJECTION, &dp);

  dev->SetFVF(kFVF);
  dev->SetRenderState(D3DRS_ZENABLE, TRUE);
  dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  dev->SetRenderState(D3DRS_CULLMODE, character_cull_mode());
  dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
  dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
  dev->SetRenderState(D3DRS_ALPHAREF, 96);

  // Lighting: bright ambient (textures carry their own shading) + two opposed
  // directional lights so the skin/outfit has shape regardless of facing.
  dev->SetRenderState(D3DRS_LIGHTING, TRUE);
  dev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(150, 150, 158));
  dev->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
  dev->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
  auto set_light = [&](DWORD i, float x, float y, float z, float b) {
    D3DLIGHT9 l{};
    l.Type = D3DLIGHT_DIRECTIONAL;
    l.Diffuse.r = l.Diffuse.g = l.Diffuse.b = b;
    float ll = std::sqrt(x * x + y * y + z * z);
    l.Direction = {x / ll, y / ll, z / ll};
    dev->SetLight(i, &l);
    dev->LightEnable(i, TRUE);
  };
  set_light(0, 0.3f, -0.6f, -0.5f, 0.6f);  // key from front-above
  set_light(1, -0.4f, 0.6f, -0.3f, 0.3f);   // fill from behind
  D3DMATERIAL9 mtrl{};
  mtrl.Diffuse.r = mtrl.Diffuse.g = mtrl.Diffuse.b = mtrl.Diffuse.a = 1.0f;
  mtrl.Ambient.r = mtrl.Ambient.g = mtrl.Ambient.b = mtrl.Ambient.a = 1.0f;
  dev->SetMaterial(&mtrl);

  dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
  dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
  dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

  std::vector<SVtx> vb;
  std::vector<std::array<float, 3>> spos, snrm;
  std::vector<const SkinnedMesh*> draw_meshes;
  draw_meshes.reserve(impl.character.meshes.size());
  for (const auto& m : impl.character.meshes) draw_meshes.push_back(&m);
  std::stable_sort(draw_meshes.begin(), draw_meshes.end(),
                   [](const SkinnedMesh* a, const SkinnedMesh* b) {
                     const bool a_eye = is_eye_mesh(a->name);
                     const bool b_eye = is_eye_mesh(b->name);
                     if (a_eye != b_eye) return a_eye;
                     const bool a_hair = is_hair_render_mesh(*a);
                     const bool b_hair = is_hair_render_mesh(*b);
                     if (a_hair != b_hair) return !a_hair;
                     return false;
                   });

  for (const SkinnedMesh* mp : draw_meshes) {
    const auto& m = *mp;
    if (!m.decoded || m.verts.empty() || m.indices.empty()) continue;
    if (!m.showing) continue;
    if (!only_mesh_enabled(m.name)) continue;
    if (skip_mesh_enabled(m.name)) continue;
    if (skip_material_enabled(m.material)) continue;
    if (hide_eyes_enabled() && is_eye_mesh(m.name)) {
      if (debug_meshes_enabled())
        std::fprintf(stderr, "[skip-eye] %s\n", m.name.c_str());
      continue;
    }
    if (is_shadow(m.name) || is_lod1(m.name) ||
        is_hidden_numbered_hair_variant(impl.character, m) ||
        is_hidden_by_character_lod_group(impl.character, m) ||
        is_unsupported_dynamic_hair(m.name) ||
        is_terminal_leg_overlay_duplicate(m)) continue;
    const bool eye_mesh = is_eye_mesh(m.name);
    dev->SetRenderState(D3DRS_CULLMODE, character_cull_mode(&m));
    dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    dev->SetRenderState(D3DRS_LIGHTING, eye_mesh ? FALSE : TRUE);

    // Skin the mesh using linear-blend skinning.
    skin_to_pose(m, impl.character, spos, snrm);

    // Local attachment hair is authored in its parent-bone space; normal body
    // and singer hair meshes are authored in skinned character space.
    const bool draw_hair_as_attachment =
        is_hair_mesh_name(m.name) &&
        (m.bone_palette.empty() || uses_local_attachment_skin(m));
    const bool root_parent_hair_bypass =
        is_root_parent_hair_piece(m) &&
        !is_weighted_root_parent_hair_piece(m);
    const char* world_mode = "identity";
    std::array<float, 16> mw{};
    if (is_head_attachment_mesh(m) || is_model_space_head_hair_attachment(m)) {
      world_mode = "head-model-delta";
      mw = impl.character.model_space_parent_delta("bone_head.mesh");
    } else if (is_local_space_head_hair_attachment(m)) {
      world_mode = "head-local-attachment";
      mw = impl.character.mesh_attachment_world(m, false);
    } else if (is_parent_local_ankle_attachment(m)) {
      world_mode = "parent-local-chain";
      mw = impl.character.bone_world_local_chain(m.parent);
    } else if (eye_mesh) {
      world_mode = "mesh-attachment";
      mw = impl.character.mesh_attachment_world(m, false);
    } else if (uses_local_attachment_skin(m) &&
               local_hair_world_mode() == "identity") {
      world_mode = "local-hair-identity-env";
      mw = {1, 0, 0, 0, 0, 1, 0, 0,
            0, 0, 1, 0, 0, 0, 0, 1};
    } else if (uses_local_attachment_skin(m) &&
               local_hair_world_mode() == "parent") {
      world_mode = "local-hair-parent-env";
      mw = impl.character.bone_world_local_chain(m.parent);
    } else if (uses_local_attachment_skin(m) &&
               local_hair_world_mode() == "attachment_parent") {
      world_mode = "local-hair-attachment-parent-env";
      mw = impl.character.attachment_parent_world(m.parent);
    } else if (m.bone_palette.empty() || draw_hair_as_attachment ||
               raw_mesh_enabled(m.name) || root_parent_hair_bypass ||
               is_mesh_local_arm_piece(m) ||
               is_raw_mesh_world_authored_piece(m)) {
      world_mode = "mesh-world";
      mw = impl.character.mesh_world(m);
    } else {
      world_mode = "identity-skinned";
      mw = {1, 0, 0, 0, 0, 1, 0, 0,
            0, 0, 1, 0, 0, 0, 0, 1};
    }
    if (eye_mesh) {
      const float inset = eye_surface_inset(m);
      if (inset != 0.0f) {
        mw[12] -= mw[4] * inset;
        mw[13] -= mw[5] * inset;
        mw[14] -= mw[6] * inset;
      }
    }
    mw = mul16(mw, impl.world_transform);
    if (debug_mesh_mode_enabled(m.name)) {
      std::fprintf(stderr,
                   "[mesh-mode] %-24s parent=%-18s mat=%-18s palette=%zu "
                   "world=%s hairAttach=%d rootBypass=%d localAttach=%d "
                   "headModel=%d raw=%d pos=(%.3f %.3f %.3f)\n",
                   m.name.c_str(), m.parent.c_str(), m.material.c_str(),
                   m.bone_palette.size(), world_mode,
                   draw_hair_as_attachment ? 1 : 0,
                   root_parent_hair_bypass ? 1 : 0,
                   uses_local_attachment_skin(m) ? 1 : 0,
                   (is_model_space_head_hair_attachment(m) ||
                    is_local_space_head_hair_attachment(m))
                       ? 1
                       : 0,
                   raw_mesh_enabled(m.name) ? 1 : 0, mw[12], mw[13], mw[14]);
    }
    if (eye_mesh && debug_skin_bounds_enabled()) {
      std::fprintf(stderr,
                   "[eye-world] %-12s parent=%-16s pos=(%.3f %.3f %.3f) "
                   "rows=[%.3f %.3f %.3f|%.3f %.3f %.3f|%.3f %.3f %.3f]\n",
                   m.name.c_str(), m.parent.c_str(), mw[12], mw[13], mw[14],
                   mw[0], mw[1], mw[2], mw[4], mw[5], mw[6], mw[8], mw[9],
                   mw[10]);
    }
    if (debug_skin_bounds_enabled() && !spos.empty()) {
      float mn[3] = {0, 0, 0};
      float mx[3] = {0, 0, 0};
      for (size_t vi = 0; vi < spos.size(); ++vi) {
        const auto& p = spos[vi];
        const float x = p[0] * mw[0] + p[1] * mw[4] + p[2] * mw[8] + mw[12];
        const float y = p[0] * mw[1] + p[1] * mw[5] + p[2] * mw[9] + mw[13];
        const float z = p[0] * mw[2] + p[1] * mw[6] + p[2] * mw[10] + mw[14];
        if (vi == 0) {
          mn[0] = mx[0] = x;
          mn[1] = mx[1] = y;
          mn[2] = mx[2] = z;
        } else {
          mn[0] = std::min(mn[0], x); mx[0] = std::max(mx[0], x);
          mn[1] = std::min(mn[1], y); mx[1] = std::max(mx[1], y);
          mn[2] = std::min(mn[2], z); mx[2] = std::max(mx[2], z);
        }
      }
      std::fprintf(stderr,
                   "[skin-bounds] %-24s mat=%-18s verts=%zu bbox=(%.2f %.2f %.2f)..(%.2f %.2f %.2f)\n",
                   m.name.c_str(), m.material.c_str(), spos.size(), mn[0],
                   mn[1], mn[2], mx[0], mx[1], mx[2]);
    }
    D3DMATRIX dm{}; std::memcpy(&dm, mw.data(), 64);
    dev->SetTransform(D3DTS_WORLD, &dm);

    auto color_byte = [](float f) -> int {
      int i = static_cast<int>(std::clamp(f, 0.0f, 1.0f) * 255.0f + 0.5f);
      return i < 0 ? 0 : (i > 255 ? 255 : i);
    };
    const milo_scene::MatObj* material = impl.character.find_mat(m.material);
    const bool highlight_mesh = highlight_mesh_enabled(m.name);
    const bool blended_hair =
        material && material->blend != 0 && is_hair_render_mesh(m);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, blended_hair ? FALSE : TRUE);
    if (debug_mesh_mode_enabled(m.name)) {
      std::fprintf(stderr,
                   "[mesh-render] %-24s mat=%-18s hairRender=%d blend=%d "
                   "zwrite=%d\n",
                   m.name.c_str(), m.material.c_str(),
                   is_hair_render_mesh(m) ? 1 : 0,
                   material ? material->blend : 0,
                   blended_hair ? 0 : 1);
    }
    const D3DCOLOR mesh_color =
        highlight_mesh
            ? D3DCOLOR_ARGB(255, 255, 0, 255)
            : material ? D3DCOLOR_ARGB(color_byte(material->color[3]),
                                       color_byte(material->color[0]),
                                       color_byte(material->color[1]),
                                       color_byte(material->color[2]))
                       : D3DCOLOR_ARGB(255, 255, 255, 255);

    IDirect3DTexture9* texture = nullptr;
    const ghogx::asset::Image* texture_image = nullptr;
    if (material && !highlight_mesh) {
      const auto* mat = material;
      auto it = impl.tex.find(mat->diffuse_tex);
      if (it != impl.tex.end()) texture = it->second;
      auto image_it = impl.tex_images.find(mat->diffuse_tex);
      if (image_it != impl.tex_images.end()) texture_image = &image_it->second;
    }
    if (debug_texture_alpha_enabled() &&
        impl.logged_texture_alpha_meshes.insert(m.name).second) {
      log_vertex_float_stats(m);
      log_texture_alpha_stats(m, material, texture_image);
    }
    if (texture) {
      dev->SetTexture(0, texture);
      dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
      dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
      dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    } else {
      dev->SetTexture(0, nullptr);
      dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
      dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
      dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
      dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    }

    vb.clear();
    vb.reserve(m.verts.size());
    for (size_t vi = 0; vi < m.verts.size(); ++vi) {
      SVtx s;
      s.x = spos[vi][0]; s.y = spos[vi][1]; s.z = spos[vi][2];
      s.nx = snrm[vi][0]; s.ny = snrm[vi][1]; s.nz = snrm[vi][2];
      s.color = mesh_color;
      s.u = m.verts[vi].u;
      s.v = m.verts[vi].v;
      vb.push_back(s);
    }
    dev->DrawIndexedPrimitiveUP(
        D3DPT_TRIANGLELIST, 0, static_cast<UINT>(m.verts.size()),
        static_cast<UINT>(m.indices.size() / 3), m.indices.data(),
        D3DFMT_INDEX16, vb.data(), sizeof(SVtx));
  }
  dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

  if (impl.has_prop && !impl.prop_attach_bone.empty() &&
      !hide_attached_props_enabled()) {
    // Instrument textures are authored as opaque props in this path. Some PS2
    // prop texture alpha decodes as low/zero, so carrying the character alpha
    // test into prop drawing can discard a correctly loaded guitar entirely.
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    const auto attach_world =
        prop_attach_world(impl.character, impl.prop_attach_bone);
    const auto prop_anchor_world =
        scene_object_world(impl.prop_scene, impl.prop_attach_bone);
    const auto prop_to_attach =
        mul16(affine_inverse(prop_anchor_world), attach_world);
    if (debug_prop_enabled() && !impl.logged_prop_debug) {
      impl.logged_prop_debug = true;
      std::fprintf(stderr,
                   "[prop] attach %s pos=(%.3f %.3f %.3f) "
                   "anchor=(%.3f %.3f %.3f) delta=(%.3f %.3f %.3f)\n",
                   impl.prop_attach_bone.c_str(), attach_world[12],
                   attach_world[13], attach_world[14],
                   prop_anchor_world[12], prop_anchor_world[13],
                   prop_anchor_world[14], prop_to_attach[12],
                   prop_to_attach[13], prop_to_attach[14]);
    }
    D3DMATRIX wm;
    for (const auto& m : impl.prop_scene.meshes) {
      if (!m.decoded || m.vertex_count == 0 || m.face_count == 0) continue;
      if (is_shadow(m.name)) continue;

      auto local_world = impl.prop_scene.world_matrix(m);
      auto world = mul16(local_world, prop_to_attach);
      world = mul16(world, impl.world_transform);
      if (debug_prop_enabled() && impl.logged_prop_debug &&
          m.name == "guitar.mesh") {
        std::fprintf(stderr,
                     "[prop] mesh %s local=(%.3f %.3f %.3f) world=(%.3f %.3f %.3f)\n",
                     m.name.c_str(), local_world[12], local_world[13],
                     local_world[14], world[12], world[13], world[14]);
        impl.logged_prop_debug = false;
      }
      std::memcpy(&wm, world.data(), 64);
      dev->SetTransform(D3DTS_WORLD, &wm);

      IDirect3DTexture9* texture = nullptr;
      if (const auto* mat = impl.prop_scene.find_mat(m.material)) {
        auto it = impl.prop_tex.find(mat->diffuse_tex);
        if (it != impl.prop_tex.end()) texture = it->second;
      }
      if (texture) {
        dev->SetTexture(0, texture);
        dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
      } else {
        dev->SetTexture(0, nullptr);
        dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
        dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
      }

      vb.clear();
      vb.reserve(m.verts.size());
      for (const auto& v : m.verts) {
        SVtx s;
        s.x = v.px; s.y = v.py; s.z = v.pz;
        s.nx = v.nx; s.ny = v.ny; s.nz = v.nz;
        const auto cc = [](float f) -> int {
          int i = static_cast<int>(f * 255.0f + 0.5f);
          return i < 0 ? 0 : (i > 255 ? 255 : i);
        };
        s.color = D3DCOLOR_ARGB(255, cc(v.r), cc(v.g), cc(v.b));
        s.u = v.u; s.v = v.v;
        vb.push_back(s);
      }
      dev->DrawIndexedPrimitiveUP(
          D3DPT_TRIANGLELIST, 0, static_cast<UINT>(m.vertex_count),
          static_cast<UINT>(m.face_count), m.indices.data(), D3DFMT_INDEX16,
          vb.data(), sizeof(SVtx));
    }
  }

  dev->SetTexture(0, nullptr);
  dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  dev->EndScene();
}

void CharRenderer::draw() {
  draw_impl(true);
}

void CharRenderer::draw_over_scene(const OrbitCamera& cam) {
  impl_->cam = cam;
  draw_impl(false);
}

// ---------------------------------------------------------------------------
// Linear-blend skinning — called by draw() each frame.
// ---------------------------------------------------------------------------
namespace {

// Invert a 3x4 affine (rigid-ish bind matrix) given as row-major 4x4. Uses the
// 3x3 cofactor inverse + translated origin; robust for rotation+uniform-scale.
std::array<float, 16> affine_inverse(const std::array<float, 16>& m) {
  // 3x3 block.
  const float a = m[0], b = m[1], c = m[2];
  const float d = m[4], e = m[5], f = m[6];
  const float g = m[8], h = m[9], i = m[10];
  const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
  const float id = (std::fabs(det) > 1e-12f) ? 1.0f / det : 0.0f;
  std::array<float, 16> r{};
  r[0] = (e * i - f * h) * id; r[1] = (c * h - b * i) * id; r[2] = (b * f - c * e) * id;
  r[4] = (f * g - d * i) * id; r[5] = (a * i - c * g) * id; r[6] = (c * d - a * f) * id;
  r[8] = (d * h - e * g) * id; r[9] = (b * g - a * h) * id; r[10] = (a * e - b * d) * id;
  // translation: -T * inv3x3
  const float tx = m[12], ty = m[13], tz = m[14];
  r[12] = -(tx * r[0] + ty * r[4] + tz * r[8]);
  r[13] = -(tx * r[1] + ty * r[5] + tz * r[9]);
  r[14] = -(tx * r[2] + ty * r[6] + tz * r[10]);
  r[15] = 1.0f;
  return r;
}

std::array<float, 16> mul16(const std::array<float, 16>& a,
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

std::array<float, 16> xfm16(const milo_scene::Xfm& x) {
  std::array<float, 16> m{};
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) m[r * 4 + c] = x.rot[r][c];
  m[12] = x.pos[0]; m[13] = x.pos[1]; m[14] = x.pos[2]; m[15] = 1.0f;
  return m;
}

std::array<float, 16> transpose_xfm_rotation(const milo_scene::Xfm& x) {
  auto m = xfm16(x);
  for (int r = 0; r < 3; ++r) {
    for (int c = r + 1; c < 3; ++c) {
      std::swap(m[r * 4 + c], m[c * 4 + r]);
    }
  }
  return m;
}

std::array<float, 16> raw_current_world(const Character& character,
                                        const std::string& name) {
  auto find_xfm = [&](const std::string& object_name,
                      const milo_scene::Xfm*& xfm,
                      std::string& parent) -> bool {
    for (const auto& b : character.bones) {
      if (b.name == object_name) {
        xfm = &b.local;
        parent = b.parent;
        return true;
      }
    }
    for (const auto& m : character.meshes) {
      if (m.name == object_name) {
        xfm = &m.local;
        parent = m.parent;
        return true;
      }
    }
    return false;
  };

  const milo_scene::Xfm* xfm = nullptr;
  std::string parent;
  if (!find_xfm(name, xfm, parent)) {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  }

  std::array<float, 16> world = xfm16(*xfm);
  int guard = 0;
  while (!parent.empty() && guard++ < 128) {
    const milo_scene::Xfm* parent_xfm = nullptr;
    std::string next_parent;
    if (!find_xfm(parent, parent_xfm, next_parent)) break;
    world = mul16(world, xfm16(*parent_xfm));
    parent = next_parent;
  }
  return world;
}

std::array<float, 16> scene_object_world(const milo_scene::Scene& scene,
                                         const std::string& name) {
  auto find_xfm = [&](const std::string& object_name,
                      const milo_scene::Xfm*& xfm,
                      std::string& parent) -> bool {
    for (const auto& t : scene.transes) {
      if (t.name == object_name) {
        xfm = &t.local;
        parent = t.parent;
        return true;
      }
    }
    for (const auto& m : scene.meshes) {
      if (m.name == object_name) {
        xfm = &m.local;
        parent = m.parent;
        return true;
      }
    }
    return false;
  };

  const milo_scene::Xfm* xfm = nullptr;
  std::string parent;
  if (!find_xfm(name, xfm, parent)) {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  }

  std::array<float, 16> world = xfm16(*xfm);
  int guard = 0;
  while (!parent.empty() && guard++ < 128) {
    const milo_scene::Xfm* parent_xfm = nullptr;
    std::string next_parent;
    if (!find_xfm(parent, parent_xfm, next_parent)) break;
    world = mul16(world, xfm16(*parent_xfm));
    parent = next_parent;
  }
  return world;
}

}  // namespace

void skin_to_pose(const SkinnedMesh& mesh, const Character& character,
                  std::vector<std::array<float, 3>>& out_pos,
                  std::vector<std::array<float, 3>>& out_nrm) {
  out_pos.assign(mesh.verts.size(), {0, 0, 0});
  out_nrm.assign(mesh.verts.size(), {0, 0, 0});
  const size_t nb = mesh.bone_palette.size();
  if (is_eye_mesh(mesh.name)) {
    if (debug_mesh_mode_enabled(mesh.name)) {
      std::fprintf(stderr,
                   "[skin-mode] %-24s mat=%-18s palette=%zu bind=%zu mode=eye-raw-local\n",
                   mesh.name.c_str(), mesh.material.c_str(), nb,
                   mesh.bind.size());
    }
    for (size_t vi = 0; vi < mesh.verts.size(); ++vi) {
      const SkinVertex& v = mesh.verts[vi];
      out_pos[vi] = {v.px, v.py, v.pz};
      out_nrm[vi] = {v.nx, v.ny, v.nz};
    }
    return;
  }
  if (is_hair_mesh_name(mesh.name) && mesh.bone_palette.empty()) {
    if (debug_mesh_mode_enabled(mesh.name)) {
      std::fprintf(stderr,
                   "[skin-mode] %-24s mat=%-18s palette=%zu bind=%zu mode=hair-raw-local\n",
                   mesh.name.c_str(), mesh.material.c_str(), nb,
                   mesh.bind.size());
    }
    for (size_t vi = 0; vi < mesh.verts.size(); ++vi) {
      const SkinVertex& v = mesh.verts[vi];
      out_pos[vi] = {v.px, v.py, v.pz};
      out_nrm[vi] = {v.nx, v.ny, v.nz};
    }
    return;
  }
  if (raw_mesh_enabled(mesh.name) || is_head_attachment_mesh(mesh) ||
      (is_root_parent_hair_piece(mesh) &&
       !is_weighted_root_parent_hair_piece(mesh)) ||
      is_raw_mesh_world_authored_piece(mesh)) {
    if (debug_mesh_mode_enabled(mesh.name)) {
      std::fprintf(stderr,
                   "[skin-mode] %-24s mat=%-18s palette=%zu bind=%zu mode=raw-bypass\n",
                   mesh.name.c_str(), mesh.material.c_str(), nb,
                   mesh.bind.size());
    }
    for (size_t vi = 0; vi < mesh.verts.size(); ++vi) {
      const SkinVertex& v = mesh.verts[vi];
      out_pos[vi] = {v.px, v.py, v.pz};
      out_nrm[vi] = {v.nx, v.ny, v.nz};
    }
    return;
  }

  // Per-palette-bone skinning matrix. Character vertices are authored in the
  // raw local-chain skeleton basis, so bind and current matrices must both use
  // that basis.
  if (debug_weight_stats_enabled()) {
    float min_sum = 999999.0f;
    float max_sum = -999999.0f;
    float max_weight = 0.0f;
    int nonzero_counts[5] = {};
    for (const auto& v : mesh.verts) {
      float sum = 0.0f;
      int nonzero = 0;
      for (int wi = 0; wi < 4; ++wi) {
        sum += v.w[wi];
        max_weight = std::max(max_weight, v.w[wi]);
        if (v.w[wi] != 0.0f) ++nonzero;
      }
      min_sum = std::min(min_sum, sum);
      max_sum = std::max(max_sum, sum);
      if (nonzero >= 0 && nonzero <= 4) ++nonzero_counts[nonzero];
    }
    std::fprintf(stderr,
                 "[weight-stats] %-24s mat=%-18s verts=%zu palette=%zu "
                 "sum=(%.3f..%.3f) maxw=%.3f nonzero=(%d,%d,%d,%d,%d)\n",
                 mesh.name.c_str(), mesh.material.c_str(), mesh.verts.size(),
                 mesh.bone_palette.size(), min_sum, max_sum, max_weight,
                 nonzero_counts[0], nonzero_counts[1], nonzero_counts[2],
                 nonzero_counts[3], nonzero_counts[4]);
  }
  std::vector<std::array<float, 16>> skin(nb);
  const bool use_stored_mesh_bind =
      use_mesh_bind_material_enabled(mesh.material) &&
      !is_mesh_local_terminal_lower_leg_piece(mesh) &&
      mesh.bind.size() >= nb;
  const bool use_mesh_local_bind =
      (is_mesh_local_root_hair_piece(mesh) ||
       is_mesh_local_bind_space_piece(mesh)) &&
      !is_mesh_local_terminal_lower_leg_piece(mesh) &&
      mesh.bind.size() >= nb;
  const bool use_mesh_bind_inverse =
      use_mesh_bind_inverse_material_enabled(mesh.material) &&
      mesh.bind.size() >= nb;
  const std::string matrix_mode = skin_matrix_mode();
  const std::string local_hair_matrix_mode =
      uses_local_attachment_skin(mesh) ? local_hair_skin_matrix_mode()
                                       : std::string{};
  const char* skin_mode = "lbs-local-chain";
  if (is_mesh_local_terminal_lower_leg_piece(mesh)) {
    skin_mode = "mesh-local-terminal-lower-leg";
  } else if (is_mesh_local_arm_piece(mesh) &&
             !disable_mesh_local_arm_skin_enabled()) {
    skin_mode = "mesh-local-arm-space";
  } else if (uses_local_attachment_skin(mesh) && mesh.bind.size() >= nb) {
    skin_mode = local_hair_matrix_mode.empty()
                    ? "local-attachment"
                    : local_hair_matrix_mode.c_str();
  } else if (use_mesh_local_bind) {
    skin_mode = "mesh-local-bind";
  } else if (use_stored_mesh_bind) {
    skin_mode = "mesh-bind";
  } else if (use_mesh_bind_inverse) {
    skin_mode = "mesh-bind-inverse";
  } else if (matrix_mode == "stored_bind") {
    skin_mode = "stored-bind";
  } else if (matrix_mode == "curr_invbind") {
    skin_mode = "curr-invbind";
  } else if (matrix_mode == "meshbind_local" && mesh.bind.size() >= nb) {
    skin_mode = "meshbind-local-env";
  } else if (matrix_mode == "meshbind_stored" && mesh.bind.size() >= nb) {
    skin_mode = "meshbind-stored-env";
  }
  if (debug_mesh_mode_enabled(mesh.name)) {
    std::fprintf(stderr,
                 "[skin-mode] %-24s mat=%-18s palette=%zu bind=%zu mode=%s "
                 "matrixEnv=%s\n",
                 mesh.name.c_str(), mesh.material.c_str(), nb,
                 mesh.bind.size(), skin_mode,
                 local_hair_matrix_mode.empty() ? matrix_mode.c_str()
                                                : local_hair_matrix_mode.c_str());
  }
  for (size_t i = 0; i < nb; ++i) {
    std::array<float, 16> curr_world =
        character.bone_world_local_chain(mesh.bone_palette[i]);
    const std::array<float, 16> raw_curr_world = curr_world;
    std::array<float, 16> hair_override{};
    const bool has_hair_override =
        is_hair_mesh_name(mesh.name) &&
        runtime_hair_world_override(character, mesh.bone_palette[i],
                                    hair_override);
    if (has_hair_override) curr_world = hair_override;
    if (is_mesh_local_terminal_lower_leg_piece(mesh)) {
      const std::array<float, 16> mesh_bind =
          character.bone_world_bind_local_chain(mesh.name);
      const std::array<float, 16> bone_bind =
          character.bone_world_bind_local_chain(mesh.bone_palette[i]);
      skin[i] = mul16(mul16(mesh_bind, affine_inverse(bone_bind)), curr_world);
    } else if (is_mesh_local_arm_piece(mesh) &&
               !disable_mesh_local_arm_skin_enabled()) {
      const std::array<float, 16> mesh_world =
          character.bone_world_local_chain(mesh.name);
      const std::array<float, 16> inv_mesh_world = affine_inverse(mesh_world);
      skin[i] = mul16(mul16(xfm16(mesh.bind[i]), curr_world),
                      inv_mesh_world);
    } else if (uses_local_attachment_skin(mesh) && i < mesh.bind.size()) {
      const bool log_hair_space =
          debug_hair_space_enabled() &&
          (i == 0 || debug_mesh_mode_enabled(mesh.name));
      if (local_hair_matrix_mode == "meshbind_local") {
        skin[i] = mul16(xfm16(mesh.bind[i]), curr_world);
      } else if (local_hair_matrix_mode == "meshbind_transpose_invmesh") {
        const std::array<float, 16> mesh_world =
            character.bone_world_local_chain(mesh.name);
        skin[i] = mul16(mul16(transpose_xfm_rotation(mesh.bind[i]),
                              curr_world),
                        affine_inverse(mesh_world));
      } else if (local_hair_matrix_mode == "meshbind_stored") {
        const auto stored_curr = character.bone_world(mesh.bone_palette[i]);
        skin[i] = mul16(xfm16(mesh.bind[i]), stored_curr);
      } else if (local_hair_matrix_mode == "stored_bind") {
        const auto stored_curr = character.bone_world(mesh.bone_palette[i]);
        const auto stored_bind = character.bone_world_bind(mesh.bone_palette[i]);
        skin[i] = mul16(affine_inverse(stored_bind), stored_curr);
      } else if (local_hair_matrix_mode == "curr_invbind") {
        const auto bone_bind =
            character.bone_world_bind_local_chain(mesh.bone_palette[i]);
        skin[i] = mul16(curr_world, affine_inverse(bone_bind));
      } else {
        const std::array<float, 16> mesh_bind =
            character.bone_world_bind_local_chain(mesh.name);
        const std::array<float, 16> mesh_world =
            character.bone_world_local_chain(mesh.name);
        const std::array<float, 16> bone_bind =
            character.bone_world_bind_local_chain(mesh.bone_palette[i]);
        skin[i] = mul16(mul16(mul16(mesh_bind, affine_inverse(bone_bind)),
                              curr_world),
                        affine_inverse(mesh_world));
        if (log_hair_space) {
          if (has_hair_override) {
            log_matrix_row("raw_current", mesh.name, mesh.bone_palette[i],
                           raw_curr_world);
          }
          log_matrix_row("mesh_bind", mesh.name, mesh.bone_palette[i],
                         mesh_bind);
          log_matrix_row("bone_bind", mesh.name, mesh.bone_palette[i],
                         bone_bind);
          log_matrix_row("curr_world", mesh.name, mesh.bone_palette[i],
                         curr_world);
          log_matrix_row("mesh_world", mesh.name, mesh.bone_palette[i],
                         mesh_world);
        }
      }
      if (log_hair_space) {
        log_matrix_row("mesh_bind_i", mesh.name, mesh.bone_palette[i],
                       xfm16(mesh.bind[i]));
        log_matrix_row("skin", mesh.name, mesh.bone_palette[i], skin[i]);
      }
    } else if ((use_mesh_local_bind || use_stored_mesh_bind) &&
               i < mesh.bind.size()) {
      if (use_stored_mesh_bind)
        curr_world = character.bone_world(mesh.bone_palette[i]);
      skin[i] = mul16(xfm16(mesh.bind[i]), curr_world);
    } else if (use_mesh_bind_inverse && i < mesh.bind.size()) {
      skin[i] = mul16(affine_inverse(xfm16(mesh.bind[i])), curr_world);
    } else if (matrix_mode == "stored_bind") {
      curr_world = character.bone_world(mesh.bone_palette[i]);
      const std::array<float, 16> bind_world =
          character.bone_world_bind(mesh.bone_palette[i]);
      skin[i] = mul16(affine_inverse(bind_world), curr_world);
    } else if (matrix_mode == "curr_invbind") {
      std::array<float, 16> bind_world =
          character.bone_world_bind_local_chain(mesh.bone_palette[i]);
      skin[i] = mul16(curr_world, affine_inverse(bind_world));
    } else if (matrix_mode == "meshbind_local" && i < mesh.bind.size()) {
      skin[i] = mul16(xfm16(mesh.bind[i]), curr_world);
    } else if (matrix_mode == "meshbind_stored" && i < mesh.bind.size()) {
      curr_world = character.bone_world(mesh.bone_palette[i]);
      skin[i] = mul16(xfm16(mesh.bind[i]), curr_world);
    } else {
      std::array<float, 16> bind_world =
          character.bone_world_bind_local_chain(mesh.bone_palette[i]);
      const std::array<float, 16> inv_bind = affine_inverse(bind_world);
      skin[i] = mul16(inv_bind, curr_world);
    }
    const bool debug_this_skin =
        debug_skin_matrix_enabled() &&
        (debug_skin_matrix_all_enabled() || debug_mesh_mode_enabled(mesh.name));
    if (debug_this_skin && (i == 0 || debug_mesh_mode_enabled(mesh.name))) {
      const auto& s = skin[i];
      std::fprintf(stderr,
                   "[skin-matrix] %-24s bone=%-24s mode=%s "
                   "hairOverride=%d diag=(%.3f %.3f %.3f) "
                   "pos=(%.3f %.3f %.3f)\n",
                   mesh.name.c_str(), mesh.bone_palette[i].c_str(),
                   skin_mode, has_hair_override ? 1 : 0, s[0],
                   s[5], s[10], s[12], s[13], s[14]);
      if (debug_mesh_mode_enabled(mesh.name)) {
        const auto bind_lc =
            character.bone_world_bind_local_chain(mesh.bone_palette[i]);
        const auto curr_lc =
            character.bone_world_local_chain(mesh.bone_palette[i]);
        const auto bind_stored = character.bone_world_bind(mesh.bone_palette[i]);
        const auto curr_stored = character.bone_world(mesh.bone_palette[i]);
        const auto mesh_lc = character.bone_world_local_chain(mesh.name);
        log_compact_matrix_rows("bind-lc", mesh.name, mesh.bone_palette[i],
                                bind_lc);
        log_compact_matrix_rows("curr-lc", mesh.name, mesh.bone_palette[i],
                                curr_lc);
        log_compact_matrix_rows("bind-stored", mesh.name, mesh.bone_palette[i],
                                bind_stored);
        log_compact_matrix_rows("curr-stored", mesh.name, mesh.bone_palette[i],
                                curr_stored);
        log_compact_matrix_rows("mesh-lc", mesh.name, mesh.bone_palette[i],
                                mesh_lc);
        std::fprintf(stderr,
                     "[skin-matrix-row] %-24s bone=%-24s "
                     "r0=(%.4f %.4f %.4f %.4f) "
                     "r1=(%.4f %.4f %.4f %.4f) "
                     "r2=(%.4f %.4f %.4f %.4f) "
                     "pos=(%.4f %.4f %.4f %.4f)\n",
                     mesh.name.c_str(), mesh.bone_palette[i].c_str(),
                     s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
                     s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15]);
        log_compact_matrix_rows("skin", mesh.name, mesh.bone_palette[i], s);
      }
    }
  }

  for (size_t vi = 0; vi < mesh.verts.size(); ++vi) {
    const SkinVertex& v = mesh.verts[vi];
    std::array<float, 3> p{0, 0, 0}, n{0, 0, 0};
    bool any = false;
    for (size_t i = 0; i < nb && i < 4; ++i) {
      const bool reverse_slots =
          reverse_skin_weight_slots_enabled() ||
          is_mesh_local_terminal_lower_leg_piece(mesh);
      const size_t wi = reverse_slots ? (std::min<size_t>(nb, 4) - 1 - i) : i;
      const float wgt = v.w[wi];
      if (wgt == 0.0f) continue;
      const auto& s = skin[i];
      p[0] += wgt * (v.px * s[0] + v.py * s[4] + v.pz * s[8] + s[12]);
      p[1] += wgt * (v.px * s[1] + v.py * s[5] + v.pz * s[9] + s[13]);
      p[2] += wgt * (v.px * s[2] + v.py * s[6] + v.pz * s[10] + s[14]);
      n[0] += wgt * (v.nx * s[0] + v.ny * s[4] + v.nz * s[8]);
      n[1] += wgt * (v.nx * s[1] + v.ny * s[5] + v.nz * s[9]);
      n[2] += wgt * (v.nx * s[2] + v.ny * s[6] + v.nz * s[10]);
      any = true;
    }
    if (!any) { p = {v.px, v.py, v.pz}; n = {v.nx, v.ny, v.nz}; }
    out_pos[vi] = p;
    out_nrm[vi] = n;
    if (vi == 0 && debug_skin_matrix_enabled() &&
        debug_mesh_mode_enabled(mesh.name)) {
      std::fprintf(stderr,
                   "[skin-vertex0] %-24s raw=(%.4f %.4f %.4f) "
                   "weights=(%.4f %.4f %.4f %.4f) "
                   "out=(%.4f %.4f %.4f) any=%d\n",
                   mesh.name.c_str(), v.px, v.py, v.pz, v.w[0], v.w[1],
                   v.w[2], v.w[3], p[0], p[1], p[2], any ? 1 : 0);
      std::fprintf(stderr,
                   "[skin-vtx] mesh=%s vi=0 raw=%.5f %.5f %.5f "
                   "weights=%.5f %.5f %.5f %.5f\n",
                   mesh.name.c_str(), v.px, v.py, v.pz, v.w[0], v.w[1],
                   v.w[2], v.w[3]);
      std::fprintf(stderr,
                   "[skin-vtx-out] mesh=%s vi=0 out=%.5f %.5f %.5f any=%d\n",
                   mesh.name.c_str(), p[0], p[1], p[2], any ? 1 : 0);
    }
  }
  if (debug_skin_matrix_enabled() && debug_mesh_mode_enabled(mesh.name) &&
      !out_pos.empty()) {
    size_t min_z_i = 0;
    size_t max_z_i = 0;
    for (size_t vi = 1; vi < out_pos.size(); ++vi) {
      if (out_pos[vi][2] < out_pos[min_z_i][2]) min_z_i = vi;
      if (out_pos[vi][2] > out_pos[max_z_i][2]) max_z_i = vi;
    }
    const SkinVertex& min_v = mesh.verts[min_z_i];
    const SkinVertex& max_v = mesh.verts[max_z_i];
    std::fprintf(stderr,
                 "[skin-z-extreme] %-24s min_i=%zu raw=(%.4f %.4f %.4f) "
                 "weights=(%.4f %.4f %.4f %.4f) out=(%.4f %.4f %.4f) "
                 "max_i=%zu raw=(%.4f %.4f %.4f) "
                 "weights=(%.4f %.4f %.4f %.4f) out=(%.4f %.4f %.4f)\n",
                 mesh.name.c_str(), min_z_i, min_v.px, min_v.py, min_v.pz,
                 min_v.w[0], min_v.w[1], min_v.w[2], min_v.w[3],
                 out_pos[min_z_i][0], out_pos[min_z_i][1],
                 out_pos[min_z_i][2], max_z_i, max_v.px, max_v.py, max_v.pz,
                 max_v.w[0], max_v.w[1], max_v.w[2], max_v.w[3],
                 out_pos[max_z_i][0], out_pos[max_z_i][1],
                 out_pos[max_z_i][2]);
  }
}

}  // namespace ghogx::character
