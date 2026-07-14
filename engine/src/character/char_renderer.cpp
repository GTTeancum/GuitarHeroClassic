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
#include <limits>
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

enum MiloBlend : uint8_t {
  kBlendDest = 0,
  kBlendSrc = 1,
  kBlendAdd = 2,
  kBlendSrcAlpha = 3,
  kBlendSrcAlphaAdd = 4,
  kBlendSubtract = 5,
  kBlendMultiply = 6,
};

struct BlendState {
  DWORD src = D3DBLEND_SRCALPHA;
  DWORD dest = D3DBLEND_INVSRCALPHA;
  DWORD op = D3DBLENDOP_ADD;
  bool additive = false;
};

BlendState character_blend_state_for(uint8_t blend) {
  switch (blend) {
    case kBlendDest:
      return {D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLENDOP_ADD, false};
    case kBlendSrc:
      return {D3DBLEND_ONE, D3DBLEND_ZERO, D3DBLENDOP_ADD, false};
    case kBlendAdd:
      return {D3DBLEND_ONE, D3DBLEND_ONE, D3DBLENDOP_ADD, true};
    case kBlendSrcAlpha:
      return {D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLENDOP_ADD,
              false};
    case kBlendSrcAlphaAdd:
      return {D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLENDOP_ADD, true};
    case kBlendSubtract:
      return {D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLENDOP_REVSUBTRACT,
              true};
    case kBlendMultiply:
      return {D3DBLEND_DESTCOLOR, D3DBLEND_ZERO, D3DBLENDOP_ADD, false};
    default:
      return {};
  }
}

bool char_env_enabled(const char* name) {
  char* value = nullptr;
  size_t len = 0;
  if (_dupenv_s(&value, &len, name) != 0 || !value) return false;
  const bool enabled = value[0] && value[0] != '0';
  std::free(value);
  return enabled;
}

bool source_material_alpha_state_enabled() {
  return !char_env_enabled("GHOGX_DISABLE_SOURCE_MAT_ALPHA_STATE");
}

bool source_material_zmode_depth_enabled() {
  return !char_env_enabled("GHOGX_DISABLE_SOURCE_MAT_ZMODE_DEPTH");
}

bool source_group_draw_order_enabled() {
  return !char_env_enabled("GHOGX_DISABLE_SOURCE_GROUP_DRAW_ORDER");
}

bool material_depth_write_enabled(const milo_scene::MatObj* material) {
  if (material && material->has_render_state &&
      source_material_zmode_depth_enabled()) {
    switch (material->z_mode) {
      case 1:  // kZModeNormal
      case 3:  // kZModeForce
        return true;
      case 0:  // kZModeDisable
      case 2:  // kZModeTransparent
      case 4:  // kZModeDecal
      default:
        return false;
    }
  }
  // RB3 RndMat defaults to kNormal z mode. If an older/partial material row
  // does not expose zMode, keep that source default instead of deriving render
  // state from mesh or material names.
  return true;
}

DWORD texture_address_for_wrap(uint8_t tex_wrap) {
  switch (tex_wrap) {
    case 0:  // kTexWrapClamp
      return D3DTADDRESS_CLAMP;
    case 4:  // kTexWrapMirror
      return D3DTADDRESS_MIRROR;
    case 1:  // kTexWrapRepeat
    default:
      return D3DTADDRESS_WRAP;
  }
}

float char_env_float_or(const char* name, float fallback, float min_value,
                        float max_value) {
  char* value = nullptr;
  size_t len = 0;
  if (_dupenv_s(&value, &len, name) != 0 || !value) return fallback;
  char* end = nullptr;
  const float parsed = std::strtof(value, &end);
  std::free(value);
  if (end == value || !std::isfinite(parsed)) return fallback;
  if (parsed < min_value || parsed > max_value) return fallback;
  return parsed;
}

D3DCOLOR character_clear_color() {
  if (char_env_enabled("GHOGX_CHARACTER_SOFT_GREEN_BG")) {
    return D3DCOLOR_XRGB(116, 151, 124);
  }
  return D3DCOLOR_XRGB(24, 26, 38);
}

struct Bounds3 {
  float mn[3] = {0, 0, 0};
  float mx[3] = {0, 0, 0};
  bool valid = false;
};

void add_bounds(Bounds3& b, const std::array<float, 3>& p) {
  if (!b.valid) {
    for (int k = 0; k < 3; ++k) b.mn[k] = b.mx[k] = p[k];
    b.valid = true;
    return;
  }
  for (int k = 0; k < 3; ++k) {
    b.mn[k] = std::min(b.mn[k], p[k]);
    b.mx[k] = std::max(b.mx[k], p[k]);
  }
}

std::array<float, 3> xform_point(const std::array<float, 16>& m,
                                 const std::array<float, 3>& p) {
  return {
      p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12],
      p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13],
      p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14],
  };
}

std::array<float, 3> basis_delta(const std::array<float, 16>& basis,
                                 const std::array<float, 3>& p) {
  const float dx = p[0] - basis[12];
  const float dy = p[1] - basis[13];
  const float dz = p[2] - basis[14];
  return {
      basis[0] * dx + basis[1] * dy + basis[2] * dz,
      basis[4] * dx + basis[5] * dy + basis[6] * dz,
      basis[8] * dx + basis[9] * dy + basis[10] * dz,
  };
}

float aabb_distance_sq(const milo_scene::MeshObj& mesh,
                       const std::array<float, 3>& local) {
  float d2 = 0.0f;
  for (int k = 0; k < 3; ++k) {
    float d = 0.0f;
    if (local[k] < mesh.bb_min[k]) {
      d = mesh.bb_min[k] - local[k];
    } else if (local[k] > mesh.bb_max[k]) {
      d = local[k] - mesh.bb_max[k];
    }
    d2 += d * d;
  }
  return d2;
}

std::array<float, 3> sub3(const std::array<float, 3>& a,
                          const std::array<float, 3>& b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

std::array<float, 3> add3(const std::array<float, 3>& a,
                          const std::array<float, 3>& b) {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

std::array<float, 3> scale3(const std::array<float, 3>& a, float s) {
  return {a[0] * s, a[1] * s, a[2] * s};
}

float dot3(const std::array<float, 3>& a, const std::array<float, 3>& b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

std::array<float, 3> cross3(const std::array<float, 3>& a,
                            const std::array<float, 3>& b) {
  return {a[1] * b[2] - a[2] * b[1],
          a[2] * b[0] - a[0] * b[2],
          a[0] * b[1] - a[1] * b[0]};
}

std::array<float, 3> normalize3(const std::array<float, 3>& a) {
  const float len2 = dot3(a, a);
  if (len2 <= 1.0e-12f) return {0, 0, 0};
  return scale3(a, 1.0f / std::sqrt(len2));
}

std::array<float, 3> closest_point_on_triangle(
    const std::array<float, 3>& p, const std::array<float, 3>& a,
    const std::array<float, 3>& b, const std::array<float, 3>& c) {
  const auto ab = sub3(b, a);
  const auto ac = sub3(c, a);
  const auto ap = sub3(p, a);
  const float d1 = dot3(ab, ap);
  const float d2 = dot3(ac, ap);
  if (d1 <= 0.0f && d2 <= 0.0f) return a;

  const auto bp = sub3(p, b);
  const float d3 = dot3(ab, bp);
  const float d4 = dot3(ac, bp);
  if (d3 >= 0.0f && d4 <= d3) return b;

  const float vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
    const float v = d1 / (d1 - d3);
    return add3(a, scale3(ab, v));
  }

  const auto cp = sub3(p, c);
  const float d5 = dot3(ab, cp);
  const float d6 = dot3(ac, cp);
  if (d6 >= 0.0f && d5 <= d6) return c;

  const float vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
    const float w = d2 / (d2 - d6);
    return add3(a, scale3(ac, w));
  }

  const float va = d3 * d6 - d5 * d4;
  if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
    const auto bc = sub3(c, b);
    const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    return add3(b, scale3(bc, w));
  }

  const float denom = 1.0f / (va + vb + vc);
  const float v = vb * denom;
  const float w = vc * denom;
  return add3(add3(a, scale3(ab, v)), scale3(ac, w));
}

struct MeshSurfaceClosest {
  bool valid = false;
  float dist_sq = std::numeric_limits<float>::infinity();
  float signed_dist = 0.0f;
  size_t tri = 0;
  std::array<float, 3> closest{0, 0, 0};
  std::array<float, 3> normal{0, 0, 0};
};

MeshSurfaceClosest closest_mesh_surface(const milo_scene::MeshObj& mesh,
                                        const std::array<float, 3>& local) {
  MeshSurfaceClosest best;
  for (size_t tri = 0; tri + 2 < mesh.indices.size(); tri += 3) {
    const uint16_t ia = mesh.indices[tri + 0];
    const uint16_t ib = mesh.indices[tri + 1];
    const uint16_t ic = mesh.indices[tri + 2];
    if (ia >= mesh.verts.size() || ib >= mesh.verts.size() ||
        ic >= mesh.verts.size()) {
      continue;
    }
    const auto a = std::array<float, 3>{mesh.verts[ia].px, mesh.verts[ia].py,
                                        mesh.verts[ia].pz};
    const auto b = std::array<float, 3>{mesh.verts[ib].px, mesh.verts[ib].py,
                                        mesh.verts[ib].pz};
    const auto c = std::array<float, 3>{mesh.verts[ic].px, mesh.verts[ic].py,
                                        mesh.verts[ic].pz};
    const auto normal = normalize3(cross3(sub3(b, a), sub3(c, a)));
    if (dot3(normal, normal) <= 0.0f) continue;
    const auto closest = closest_point_on_triangle(local, a, b, c);
    const auto delta = sub3(local, closest);
    const float d2 = dot3(delta, delta);
    if (!best.valid || d2 < best.dist_sq) {
      best.valid = true;
      best.dist_sq = d2;
      best.signed_dist = dot3(delta, normal);
      best.tri = tri / 3;
      best.closest = closest;
      best.normal = normal;
    }
  }
  return best;
}

// A mesh is the flat blob-shadow decal (drawn on the floor) if its name starts
// with "shadow". Skipping it keeps the character clean against the backdrop.
bool is_shadow(const std::string& n) { return n.rfind("shadow", 0) == 0; }

const milo_scene::GroupObj* find_character_group(const Character& character,
                                                 const std::string& name) {
  for (const auto& group : character.groups) {
    if (group.name == name) return &group;
  }
  return nullptr;
}

int character_direct_group_rank(const Character& character,
                                const std::string& group_name,
                                const std::string& mesh_name) {
  const milo_scene::GroupObj* group = find_character_group(character, group_name);
  if (!group) return -1;
  for (size_t i = 0; i < group->children.size(); ++i) {
    if (group->children[i] == mesh_name) return static_cast<int>(i);
  }
  return -1;
}

int character_active_lod_group_rank(const Character& character,
                                    const std::string& mesh_name,
                                    int min_lod) {
  return character_direct_group_rank(character,
                                     min_lod >= 1 ? "lod1.grp" : "lod0.grp",
                                     mesh_name);
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

bool is_hidden_by_character_lod_selection(const Character& character,
                                          const SkinnedMesh& mesh,
                                          int min_lod) {
  const bool has_lod1 = find_character_group(character, "lod1.grp") != nullptr;
  if (min_lod >= 1 && has_lod1) {
    return !character_group_contains_mesh(character, "lod1.grp", mesh.name);
  }
  return is_hidden_by_character_lod_group(character, mesh);
}

bool is_guitar_strings_prop_mesh(const std::string& name) {
  return name == "guitar_strings.mesh";
}

std::string lower_copy(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

bool has_hair_token(const std::string& text) {
  return lower_copy(text).find("hair") != std::string::npos;
}

bool is_hair_two_sided_surface(
    const Character& character,
    const SkinnedMesh* mesh,
    const ghogx::milo_scene::MatObj* material = nullptr) {
  if (mesh) {
    if (character_mesh_uses_char_hair_point_bone(character, *mesh)) {
      return true;
    }
    if (has_hair_token(mesh->name) || has_hair_token(mesh->material)) {
      return true;
    }
  }
  return material && (has_hair_token(material->name) ||
                      has_hair_token(material->diffuse_tex));
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

DWORD character_cull_mode(
    const ghogx::milo_scene::MatObj* material = nullptr) {
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
  if (material && material->has_cull && !material->cull) {
    return D3DCULL_NONE;
  }
  return D3DCULL_CW;
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

bool debug_face_rows_enabled() {
  return char_env_enabled("GHOGX_DEBUG_FACE_ROWS");
}

bool is_mouth_probe_mesh(const SkinnedMesh& m) {
  return contains_case_insensitive(m.name, "teeth") ||
         contains_case_insensitive(m.name, "tounge") ||
         contains_case_insensitive(m.name, "tongue");
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

bool debug_surface_contact_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_SURFACE_CONTACT") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_SURFACE_CONTACT");
  return value && value[0];
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

int character_bone_index(const Character& character, const std::string& name) {
  for (size_t i = 0; i < character.bones.size(); ++i) {
    if (character.bones[i].name == name) return static_cast<int>(i);
  }
  return -1;
}

const milo_scene::TransObj* scene_trans(
    const milo_scene::Scene& scene,
    const std::string& name) {
  for (const auto& t : scene.transes) {
    if (t.name == name) return &t;
  }
  return nullptr;
}

void reconcile_instrument_anchor(Character& character,
                                 std::vector<milo_scene::Xfm>& original_locals,
                                 const milo_scene::Scene& prop_scene,
                                 const std::string& attach_bone,
                                 const char* anchor_name) {
  const auto* prop_anchor = scene_trans(prop_scene, anchor_name);
  if (!prop_anchor) return;

  const int bone_i = character_bone_index(character, anchor_name);
  if (bone_i < 0) return;
  auto& bone = character.bones[static_cast<size_t>(bone_i)];
  if (bone.parent != prop_anchor->parent) return;
  if (prop_anchor->parent != attach_bone &&
      character_bone_index(character, prop_anchor->parent) < 0) {
    return;
  }

  const auto old = bone.local;
  bone.local = prop_anchor->local;
  if (character.bind_bone_local.size() > static_cast<size_t>(bone_i))
    character.bind_bone_local[static_cast<size_t>(bone_i)] =
        prop_anchor->local;
  if (original_locals.size() > static_cast<size_t>(bone_i))
    original_locals[static_cast<size_t>(bone_i)] = prop_anchor->local;

  std::fprintf(stderr,
               "[char3d] instrument anchor %s from prop '%s': "
               "parent=%s local=(%.4f %.4f %.4f) "
               "was=(%.4f %.4f %.4f) source=prop-asset\n",
               anchor_name, prop_scene.dir_name.c_str(),
               prop_anchor->parent.c_str(), prop_anchor->local.pos[0],
               prop_anchor->local.pos[1], prop_anchor->local.pos[2],
               old.pos[0], old.pos[1], old.pos[2]);
}

void reconcile_instrument_fret_targets(
    Character& character, std::vector<milo_scene::Xfm>& original_locals,
    const milo_scene::Scene& prop_scene, const std::string& attach_bone) {
  char anchor[32];
  for (int fret = 1; fret <= 20; ++fret) {
    std::snprintf(anchor, sizeof(anchor), "spot_neck_fret%02d.mesh", fret);
    reconcile_instrument_anchor(character, original_locals, prop_scene,
                                attach_bone, anchor);
  }
}

std::array<float, 16> mul16(const std::array<float, 16>& a,
                            const std::array<float, 16>& b);
std::array<float, 16> affine_inverse(const std::array<float, 16>& m);
std::array<float, 16> xfm16(const milo_scene::Xfm& x);
std::array<float, 16> scene_object_world(const milo_scene::Scene& scene,
                                         const std::string& name);
std::optional<std::array<float, 16>> scene_object_stored_world(
    const milo_scene::Scene& scene,
    const std::string& name);

std::array<float, 16> prop_attach_world(const Character& character,
                                        const std::string& attach_bone) {
  // Accepted prop traces route guitars/mics through the same moving Trans rows
  // as skinned character output. Keep instruments in the character local-chain
  // basis instead of the stored-world correction used for a few rigid meshes.
  return character.bone_world_local_chain(attach_bone);
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

std::array<float, 3> transform_point(const std::array<float, 3>& p,
                                     const std::array<float, 16>& m) {
  return {p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12],
          p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13],
          p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14]};
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

void log_prop_texture_alpha_stats(const milo_scene::MeshObj& mesh,
                                  const milo_scene::MatObj* material,
                                  const ghogx::asset::Image* image,
                                  bool texture_alpha_enabled) {
  if (!image || !image->valid() || mesh.verts.empty()) return;
  int vert_zero = 0;
  int vert_lt32 = 0;
  int vert_lt96 = 0;
  int vert_opaque = 0;
  for (const auto& vert : mesh.verts) {
    const uint8_t alpha = sample_alpha(*image, vert.u, vert.v);
    if (alpha == 0) ++vert_zero;
    if (alpha < 32) ++vert_lt32;
    if (alpha < 96) ++vert_lt96;
    if (alpha >= 250) ++vert_opaque;
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
               "[prop-alpha] mesh=%s mat=%s tex=%s size=%dx%d "
               "textureAlpha=%d verts=%u a0=%d a<32=%d a<96=%d opaque=%d "
               "tris=%d a0=%d a<32=%d a<96=%d opaque=%d\n",
               mesh.name.c_str(), mesh.material.c_str(),
               material ? material->diffuse_tex.c_str() : "", image->width,
               image->height, texture_alpha_enabled ? 1 : 0,
               static_cast<unsigned>(mesh.verts.size()), vert_zero, vert_lt32,
               vert_lt96, vert_opaque, tri_samples, tri_zero, tri_lt32,
               tri_lt96, tri_opaque);
  std::fprintf(stderr,
               "[prop-alpha-counts] mesh=%s vertZero=%d vertLt32=%d "
               "vertLt96=%d vertOpaque=%d triSamples=%d triZero=%d "
               "triLt32=%d triLt96=%d triOpaque=%d\n",
               mesh.name.c_str(), vert_zero, vert_lt32, vert_lt96,
               vert_opaque, tri_samples, tri_zero, tri_lt32, tri_lt96,
               tri_opaque);
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
  std::map<std::string, ghogx::asset::Image> prop_tex_images;
  std::set<std::string> logged_prop_texture_alpha_meshes;
  std::string prop_attach_bone;
  bool has_prop = false;
  bool logged_prop_debug = false;
  float bb_min[3] = {0, 0, 0};
  float bb_max[3] = {0, 0, 0};
  bool have_bounds = false;
  float world_offset[3] = {0.0f, 0.0f, 0.0f};
  std::array<float, 16> world_transform = {1, 0, 0, 0, 0, 1, 0, 0,
                                           0, 0, 1, 0, 0, 0, 0, 1};
  int min_lod = 0;
  bool use_scene_lighting = false;
  float color_mod[4] = {1.0f, 1.0f, 1.0f, 1.0f};

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

void CharRenderer::set_min_lod(int min_lod) {
  const int clamped = std::max(0, min_lod);
  if (impl_->min_lod == clamped) return;
  impl_->min_lod = clamped;
  if (debug_meshes_enabled()) {
    std::fprintf(stderr, "[char3d] min_lod active: %d\n", impl_->min_lod);
  }
}

void CharRenderer::set_use_scene_lighting(bool enabled) {
  impl_->use_scene_lighting = enabled;
}

void CharRenderer::set_color_modulation(float r, float g, float b, float a) {
  impl_->color_mod[0] = std::clamp(r, 0.0f, 4.0f);
  impl_->color_mod[1] = std::clamp(g, 0.0f, 4.0f);
  impl_->color_mod[2] = std::clamp(b, 0.0f, 4.0f);
  impl_->color_mod[3] = std::clamp(a, 0.0f, 1.0f);
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
  impl_->prop_tex_images = textures;
  impl_->logged_prop_texture_alpha_meshes.clear();
  impl_->prop_scene = std::move(scene);
  impl_->prop_attach_bone = attach_bone;
  impl_->has_prop = true;
  reconcile_instrument_anchor(impl_->character, impl_->original_bone_local,
                              impl_->prop_scene, impl_->prop_attach_bone,
                              "bone_fret.mesh");
  reconcile_instrument_anchor(impl_->character, impl_->original_bone_local,
                              impl_->prop_scene, impl_->prop_attach_bone,
                              "bone_fret_hand.mesh");
  reconcile_instrument_fret_targets(impl_->character,
                                    impl_->original_bone_local,
                                    impl_->prop_scene,
                                    impl_->prop_attach_bone);
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
        is_hidden_by_character_lod_selection(impl_->character, m,
                                             impl_->min_lod)) continue;
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

  const float backbuffer_aspect =
      win->bb_height() > 0
          ? static_cast<float>(win->bb_width()) /
                static_cast<float>(win->bb_height())
          : 16.0f / 9.0f;
  const float aspect =
      char_env_float_or("GHOGX_CAMERA_ASPECT", backbuffer_aspect, 0.5f, 3.0f);

  float eye[3];
  cam.eye(eye);
  float result_at[3] = {};
  const float* at = cam.authored ? cam.authored_at : cam.target;
  const float* up = cam.authored ? cam.authored_up : nullptr;
  if (cam.result_frame.valid) {
    for (int k = 0; k < 3; ++k) {
      result_at[k] = cam.result_frame.position[k] +
                     cam.result_frame.forward[k] * 100.0f;
    }
    at = result_at;
    up = cam.result_frame.up;
  }
  Mat4 view = Mat4::look_at_lh(eye[0], eye[1], eye[2], at[0], at[1], at[2],
                               up ? up[0] : 0.0f, up ? up[1] : 0.0f,
                               up ? up[2] : 1.0f);
  Mat4 proj = Mat4::perspective_lh(cam.fov, aspect, cam.near_z, cam.far_z);
  proj.m[0][0] = -proj.m[0][0];  // RH world -> LH clip (no left/right flip)
  if ((cam.authored || cam.result_frame.valid) &&
      !char_env_enabled("GHOGX_DISABLE_CAMERA_SCREEN_OFFSET")) {
    constexpr float kScreenOffsetToClip = 1.0f / 768.0f;
    proj.m[2][0] += cam.screen_offset[0] * kScreenOffsetToClip;
    proj.m[2][1] += cam.screen_offset[1] * kScreenOffsetToClip;
  }

  if (clear_target) {
    dev->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
               character_clear_color(), 1.0f, 0);
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

  // Standalone viewer lighting uses bright ambient plus two opposed directional
  // lights. Venue composites can opt into the existing scene light state.
  dev->SetRenderState(D3DRS_LIGHTING, TRUE);
  dev->SetRenderState(D3DRS_COLORVERTEX, TRUE);
  dev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
  dev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
  dev->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
  dev->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
  if (!impl.use_scene_lighting) {
    dev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(150, 150, 158));
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
    set_light(1, -0.4f, 0.6f, -0.3f, 0.3f);  // fill from behind
  }
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
  const bool use_source_group_order = source_group_draw_order_enabled();
  std::stable_sort(draw_meshes.begin(), draw_meshes.end(),
                   [&](const SkinnedMesh* a, const SkinnedMesh* b) {
                     if (use_source_group_order) {
                       const int ar = character_active_lod_group_rank(
                           impl.character, a->name, impl.min_lod);
                       const int br = character_active_lod_group_rank(
                           impl.character, b->name, impl.min_lod);
                       if (ar >= 0 && br >= 0 && ar != br) return ar < br;
                       if ((ar >= 0) != (br >= 0)) return ar >= 0;
                     }
                     if (std::fabs(a->draw_order - b->draw_order) >
                             1.0e-5f) {
                       return a->draw_order < b->draw_order;
                     }
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
    if (is_shadow(m.name) ||
        is_hidden_by_character_lod_selection(impl.character, m,
                                             impl.min_lod)) continue;
    const bool eye_mesh = is_eye_mesh(m.name);
    const milo_scene::MatObj* material = impl.character.find_mat(m.material);
    const bool hair_point_bone =
        character_mesh_uses_char_hair_point_bone(impl.character, m);
    const bool hair_two_sided =
        is_hair_two_sided_surface(impl.character, &m, material);
    const DWORD mesh_cull_mode =
        hair_two_sided ? D3DCULL_NONE : character_cull_mode(material);
    dev->SetRenderState(D3DRS_CULLMODE, mesh_cull_mode);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    dev->SetRenderState(
        D3DRS_LIGHTING,
        impl.use_scene_lighting ? FALSE : (eye_mesh ? FALSE : TRUE));

    // Skin the mesh using linear-blend skinning.
    skin_to_pose(m, impl.character, spos, snrm);

    const char* world_mode = "identity";
    std::array<float, 16> mw{};
    if (m.bone_palette.empty()) {
      world_mode = "source-trans-world";
      mw = impl.character.mesh_world(m);
    } else {
      world_mode = "identity-source-skinned";
      mw = {1, 0, 0, 0, 0, 1, 0, 0,
            0, 0, 1, 0, 0, 0, 0, 1};
    }
    mw = mul16(mw, impl.world_transform);
    if (debug_face_rows_enabled() && (is_mouth_probe_mesh(m) || eye_mesh) &&
        !spos.empty()) {
      auto log_candidate = [&](const char* candidate,
                               std::array<float, 16> row) {
        row = mul16(row, impl.world_transform);
        Bounds3 b;
        for (const auto& p : spos) add_bounds(b, transform_point(p, row));
        std::fprintf(
            stderr,
            "[face-row-candidate] mesh=%s parent=%s mat=%s candidate=%s "
            "pos=(%.3f %.3f %.3f) bbox=(%.3f %.3f %.3f)..(%.3f %.3f %.3f) "
            "rows=[%.3f %.3f %.3f|%.3f %.3f %.3f|%.3f %.3f %.3f]\n",
            m.name.c_str(), m.parent.c_str(), m.material.c_str(), candidate,
            row[12], row[13], row[14], b.mn[0], b.mn[1], b.mn[2], b.mx[0],
            b.mx[1], b.mx[2], row[0], row[1], row[2], row[4], row[5],
            row[6], row[8], row[9], row[10]);
      };
      log_candidate("source-world", impl.character.mesh_world(m));
      log_candidate("source-bind-world",
                    impl.character.bone_world_bind_local_chain(m.name));
      log_candidate("stored-world", xfm16(m.world_stored));
      if (!m.parent.empty()) {
        log_candidate("parent-source-world",
                      impl.character.bone_world_local_chain(m.parent));
        log_candidate("local-parent-source-world",
                      mul16(xfm16(m.local),
                            impl.character.bone_world_local_chain(m.parent)));
        log_candidate("local-parent-bind-world",
                      mul16(xfm16(m.local),
                            impl.character.bone_world_bind_local_chain(
                                m.parent)));
      }
    }
    if (debug_mesh_mode_enabled(m.name)) {
      std::fprintf(stderr,
                   "[mesh-mode] %-24s parent=%-18s mat=%-18s palette=%zu "
                   "world=%s constraint=%u target=%s pos=(%.3f %.3f %.3f)\n",
                   m.name.c_str(), m.parent.c_str(), m.material.c_str(),
                   m.bone_palette.size(), world_mode,
                   m.constraint, m.target.empty() ? "<none>" : m.target.c_str(),
                   mw[12], mw[13], mw[14]);
      if (!spos.empty()) {
        size_t min_z_i = 0;
        size_t max_z_i = 0;
        std::array<float, 3> mn = transform_point(spos[0], mw);
        std::array<float, 3> mx = mn;
        for (size_t vi = 1; vi < spos.size(); ++vi) {
          const auto wp = transform_point(spos[vi], mw);
          if (wp[2] < transform_point(spos[min_z_i], mw)[2]) min_z_i = vi;
          if (wp[2] > transform_point(spos[max_z_i], mw)[2]) max_z_i = vi;
          mn[0] = std::min(mn[0], wp[0]);
          mn[1] = std::min(mn[1], wp[1]);
          mn[2] = std::min(mn[2], wp[2]);
          mx[0] = std::max(mx[0], wp[0]);
          mx[1] = std::max(mx[1], wp[1]);
          mx[2] = std::max(mx[2], wp[2]);
        }
        const auto v0 = transform_point(spos[0], mw);
        const auto min_z = transform_point(spos[min_z_i], mw);
        const auto max_z = transform_point(spos[max_z_i], mw);
        std::fprintf(stderr,
                     "[mesh-world-verts] mesh=%s v0=(%.4f %.4f %.4f) "
                     "bbox=(%.4f %.4f %.4f)..(%.4f %.4f %.4f) "
                     "minZ_i=%zu minZ=(%.4f %.4f %.4f) "
                     "maxZ_i=%zu maxZ=(%.4f %.4f %.4f)\n",
                     m.name.c_str(), v0[0], v0[1], v0[2], mn[0], mn[1],
                     mn[2], mx[0], mx[1], mx[2], min_z_i, min_z[0],
                     min_z[1], min_z[2], max_z_i, max_z[0], max_z[1],
                     max_z[2]);
      }
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
    if (debug_surface_contact_enabled() && debug_mesh_mode_enabled(m.name) &&
        !spos.empty()) {
      auto log_ref_space = [&](const char* ref_name) {
        const int ref_i = character_bone_index(impl.character, ref_name);
        if (ref_i < 0) return;
        auto ref_world = impl.character.bone_world_local_chain(ref_name);
        ref_world = mul16(ref_world, impl.world_transform);
        Bounds3 b;
        size_t max_front_i = 0;
        std::array<float, 3> max_front{0, 0, 0};
        for (size_t vi = 0; vi < spos.size(); ++vi) {
          const auto world = xform_point(mw, spos[vi]);
          const auto d = basis_delta(ref_world, world);
          add_bounds(b, d);
          if (vi == 0 || d[2] > max_front[2]) {
            max_front_i = vi;
            max_front = d;
          }
        }
        const SkinVertex& v = m.verts[max_front_i];
        std::fprintf(stderr,
                     "[surface-contact] mesh=%s ref=%s verts=%zu "
                     "localBounds=(%.4f %.4f %.4f)..(%.4f %.4f %.4f) "
                     "maxFront_i=%zu local=(%.4f %.4f %.4f) "
                     "raw=(%.4f %.4f %.4f) weights=(%.4f %.4f %.4f %.4f)\n",
                     m.name.c_str(), ref_name, spos.size(), b.mn[0], b.mn[1],
                     b.mn[2], b.mx[0], b.mx[1], b.mx[2], max_front_i,
                     max_front[0], max_front[1], max_front[2], v.px, v.py,
                     v.pz, v.w[0], v.w[1], v.w[2], v.w[3]);
      };
      log_ref_space("bone_fret_hand.mesh");
      log_ref_space("bone_fret.mesh");

      if (impl.has_prop && !impl.prop_attach_bone.empty()) {
        const auto attach_world =
            prop_attach_world(impl.character, impl.prop_attach_bone);
        const auto prop_anchor_world =
            scene_object_world(impl.prop_scene, impl.prop_attach_bone);
        const auto prop_to_attach =
            mul16(affine_inverse(prop_anchor_world), attach_world);
        for (const auto& pm : impl.prop_scene.meshes) {
          if (!pm.decoded || pm.verts.empty()) continue;
          auto prop_world = mul16(impl.prop_scene.world_matrix(pm),
                                  prop_to_attach);
          prop_world = mul16(prop_world, impl.world_transform);
          const auto inv_prop_world = affine_inverse(prop_world);
          float best_d2 = 0.0f;
          size_t best_i = 0;
          int inside_count = 0;
          std::array<float, 3> best_local{0, 0, 0};
          const bool tri_contact =
              (pm.name == "guitar.mesh" ||
               pm.name == "guitar_strings.mesh") &&
              !pm.indices.empty();
          size_t tri_samples = 0;
          float min_dist = std::numeric_limits<float>::infinity();
          float max_dist = 0.0f;
          float sum_dist = 0.0f;
          float min_signed = std::numeric_limits<float>::infinity();
          float max_signed = -std::numeric_limits<float>::infinity();
          int negative_signed = 0;
          size_t nearest_tri_i = 0;
          size_t nearest_tri = 0;
          size_t min_signed_i = 0;
          size_t min_signed_tri = 0;
          std::array<float, 3> nearest_tri_local{0, 0, 0};
          std::array<float, 3> nearest_tri_closest{0, 0, 0};
          std::array<float, 3> nearest_tri_normal{0, 0, 0};
          std::array<float, 3> min_signed_local{0, 0, 0};
          std::array<float, 3> min_signed_closest{0, 0, 0};
          std::array<float, 3> min_signed_normal{0, 0, 0};
          for (size_t vi = 0; vi < spos.size(); ++vi) {
            const auto world = xform_point(mw, spos[vi]);
            const auto local = xform_point(inv_prop_world, world);
            const float d2 = aabb_distance_sq(pm, local);
            if (vi == 0 || d2 < best_d2) {
              best_d2 = d2;
              best_i = vi;
              best_local = local;
            }
            if (d2 == 0.0f) ++inside_count;
            if (tri_contact) {
              const auto hit = closest_mesh_surface(pm, local);
              if (hit.valid) {
                const float dist = std::sqrt(hit.dist_sq);
                if (tri_samples == 0 || dist < min_dist) {
                  min_dist = dist;
                  nearest_tri_i = vi;
                  nearest_tri = hit.tri;
                  nearest_tri_local = local;
                  nearest_tri_closest = hit.closest;
                  nearest_tri_normal = hit.normal;
                }
                max_dist = std::max(max_dist, dist);
                sum_dist += dist;
                if (tri_samples == 0 || hit.signed_dist < min_signed) {
                  min_signed = hit.signed_dist;
                  min_signed_i = vi;
                  min_signed_tri = hit.tri;
                  min_signed_local = local;
                  min_signed_closest = hit.closest;
                  min_signed_normal = hit.normal;
                }
                max_signed = std::max(max_signed, hit.signed_dist);
                if (hit.signed_dist < -1.0e-4f) ++negative_signed;
                ++tri_samples;
              }
            }
          }
          const SkinVertex& v = m.verts[best_i];
          std::fprintf(stderr,
                       "[surface-prop] mesh=%s prop=%s mat=%s verts=%zu "
                       "inside=%d nearest_i=%zu dist=%.4f "
                       "propLocal=(%.4f %.4f %.4f) "
                       "propBBox=(%.4f %.4f %.4f)..(%.4f %.4f %.4f) "
                       "raw=(%.4f %.4f %.4f) weights=(%.4f %.4f %.4f %.4f)\n",
                       m.name.c_str(), pm.name.c_str(), pm.material.c_str(),
                       spos.size(), inside_count, best_i, std::sqrt(best_d2),
                       best_local[0], best_local[1], best_local[2],
                       pm.bb_min[0], pm.bb_min[1], pm.bb_min[2],
                       pm.bb_max[0], pm.bb_max[1], pm.bb_max[2], v.px, v.py,
                       v.pz, v.w[0], v.w[1], v.w[2], v.w[3]);
          if (tri_contact && tri_samples > 0) {
            const SkinVertex& nearest_v = m.verts[nearest_tri_i];
            const SkinVertex& signed_v = m.verts[min_signed_i];
            std::fprintf(
                stderr,
                "[surface-prop-tri] mesh=%s prop=%s verts=%zu tris=%zu "
                "minDist=%.4f meanDist=%.4f maxDist=%.4f "
                "minSigned=%.4f maxSigned=%.4f negSigned=%d "
                "nearest_i=%zu tri=%zu local=(%.4f %.4f %.4f) "
                "closest=(%.4f %.4f %.4f) normal=(%.4f %.4f %.4f) "
                "raw=(%.4f %.4f %.4f) weights=(%.4f %.4f %.4f %.4f) "
                "minSigned_i=%zu minSignedTri=%zu "
                "minSignedLocal=(%.4f %.4f %.4f) "
                "minSignedClosest=(%.4f %.4f %.4f) "
                "minSignedNormal=(%.4f %.4f %.4f) "
                "minSignedRaw=(%.4f %.4f %.4f) "
                "minSignedWeights=(%.4f %.4f %.4f %.4f)\n",
                m.name.c_str(), pm.name.c_str(), spos.size(),
                pm.indices.size() / 3, min_dist,
                sum_dist / static_cast<float>(tri_samples), max_dist,
                min_signed, max_signed, negative_signed, nearest_tri_i,
                nearest_tri, nearest_tri_local[0], nearest_tri_local[1],
                nearest_tri_local[2], nearest_tri_closest[0],
                nearest_tri_closest[1], nearest_tri_closest[2],
                nearest_tri_normal[0], nearest_tri_normal[1],
                nearest_tri_normal[2], nearest_v.px, nearest_v.py,
                nearest_v.pz, nearest_v.w[0], nearest_v.w[1], nearest_v.w[2],
                nearest_v.w[3], min_signed_i, min_signed_tri,
                min_signed_local[0], min_signed_local[1],
                min_signed_local[2], min_signed_closest[0],
                min_signed_closest[1], min_signed_closest[2],
                min_signed_normal[0], min_signed_normal[1],
                min_signed_normal[2], signed_v.px, signed_v.py, signed_v.pz,
                signed_v.w[0], signed_v.w[1], signed_v.w[2], signed_v.w[3]);
          }
        }
      }
    }
    D3DMATRIX dm{}; std::memcpy(&dm, mw.data(), 64);
    dev->SetTransform(D3DTS_WORLD, &dm);

    auto color_byte = [](float f) -> int {
      int i = static_cast<int>(std::clamp(f, 0.0f, 1.0f) * 255.0f + 0.5f);
      return i < 0 ? 0 : (i > 255 ? 255 : i);
    };
    const bool highlight_mesh = highlight_mesh_enabled(m.name);
    uint8_t material_blend =
        material ? material->blend : static_cast<uint8_t>(kBlendSrcAlpha);
    if (highlight_mesh) material_blend = kBlendSrc;
    const BlendState blend_state = character_blend_state_for(material_blend);
    const bool use_source_alpha =
        material && material->has_render_state &&
        source_material_alpha_state_enabled();
    const bool alpha_test = use_source_alpha ? material->alpha_cut : true;
    const DWORD alpha_ref =
        use_source_alpha
            ? static_cast<DWORD>(
                  std::clamp(material->alpha_threshold, 0, 255))
            : 96u;
    const bool depth_write = material_depth_write_enabled(material);
    dev->SetRenderState(D3DRS_BLENDOP, blend_state.op);
    dev->SetRenderState(D3DRS_SRCBLEND, blend_state.src);
    dev->SetRenderState(D3DRS_DESTBLEND, blend_state.dest);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, depth_write ? TRUE : FALSE);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, alpha_test ? TRUE : FALSE);
    dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    dev->SetRenderState(D3DRS_ALPHAREF, alpha_ref);
    const DWORD tex_address =
        material && material->has_render_state
            ? texture_address_for_wrap(material->tex_wrap)
            : D3DTADDRESS_WRAP;
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, tex_address);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, tex_address);
    if (debug_mesh_mode_enabled(m.name)) {
      const int group_rank = character_active_lod_group_rank(
          impl.character, m.name, impl.min_lod);
      std::fprintf(stderr,
                   "[mesh-render] %-24s mat=%-18s blend=%d "
                   "zwrite=%d ngCull=%d cullMode=%lu src=%lu dst=%lu "
                   "op=%lu drawOrder=%.3f groupRank=%d alphaTest=%d alphaCut=%d "
                   "alphaRef=%lu zMode=%u texWrap=%u hairTwoSided=%d "
                   "hairPointBone=%d\n",
                   m.name.c_str(), m.material.c_str(),
                    static_cast<int>(material_blend),
                    depth_write ? 1 : 0,
                   material && material->has_cull ? (material->cull ? 1 : 0)
                                                   : -1,
                   static_cast<unsigned long>(mesh_cull_mode),
                   static_cast<unsigned long>(blend_state.src),
                   static_cast<unsigned long>(blend_state.dest),
                   static_cast<unsigned long>(blend_state.op),
                   m.draw_order, group_rank, alpha_test ? 1 : 0,
                   material && material->has_render_state &&
                           material->alpha_cut
                       ? 1
                       : 0,
                   static_cast<unsigned long>(alpha_ref),
                   material && material->has_render_state
                       ? static_cast<unsigned>(material->z_mode)
                       : 0,
                   material && material->has_render_state
                       ? static_cast<unsigned>(material->tex_wrap)
                       : 1,
                   hair_two_sided ? 1 : 0, hair_point_bone ? 1 : 0);
    }
    float mesh_alpha =
        material ? material->color[3] * impl.color_mod[3] : impl.color_mod[3];
    float mesh_r =
        material ? material->color[0] * impl.color_mod[0] : impl.color_mod[0];
    float mesh_g =
        material ? material->color[1] * impl.color_mod[1] : impl.color_mod[1];
    float mesh_b =
        material ? material->color[2] * impl.color_mod[2] : impl.color_mod[2];
    if (material_blend == kBlendAdd && mesh_alpha < 0.999f) {
      // ONE/ONE additive blending ignores vertex alpha; GH2 Mat alpha acts as
      // intensity for faded additive surfaces in the general MILO path.
      mesh_r *= mesh_alpha;
      mesh_g *= mesh_alpha;
      mesh_b *= mesh_alpha;
      mesh_alpha = 1.0f;
    }
    const D3DCOLOR mesh_color =
        highlight_mesh
            ? D3DCOLOR_ARGB(255, 255, 0, 255)
            : D3DCOLOR_ARGB(color_byte(mesh_alpha), color_byte(mesh_r),
                             color_byte(mesh_g), color_byte(mesh_b));

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
  dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
  dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

  if (impl.has_prop && !impl.prop_attach_bone.empty() &&
      !hide_attached_props_enabled()) {
    // Most instrument prop surfaces are opaque, but the authored string card is
    // a texture-alpha cutout and must keep its diffuse texture alpha.
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
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
      static constexpr const char* kPropDebugObjects[] = {
          "bone_pos_guitar.mesh",
          "bone_fret.mesh",
          "bone_fret_hand.mesh",
          "guitar.mesh",
          "guitar_strings.mesh",
          "shadow_guitar.mesh",
      };
      for (const char* object_name : kPropDebugObjects) {
        const auto stored_world =
            scene_object_stored_world(impl.prop_scene, object_name);
        if (!stored_world) continue;
        const auto composed_world =
            scene_object_world(impl.prop_scene, object_name);
        const auto composed_char = mul16(composed_world, prop_to_attach);
        const auto stored_char = mul16(*stored_world, prop_to_attach);
        std::fprintf(stderr,
                     "[prop-rel] obj=%s comp=(%.3f %.3f %.3f)\n",
                     object_name, composed_world[12], composed_world[13],
                     composed_world[14]);
        std::fprintf(stderr,
                     "[prop-rel] obj=%s stored=(%.3f %.3f %.3f)\n",
                     object_name, (*stored_world)[12], (*stored_world)[13],
                     (*stored_world)[14]);
        std::fprintf(stderr,
                     "[prop-rel] obj=%s char_comp=(%.3f %.3f %.3f)\n",
                     object_name, composed_char[12], composed_char[13],
                     composed_char[14]);
        std::fprintf(stderr,
                     "[prop-rel] obj=%s char_stored=(%.3f %.3f %.3f)\n",
                     object_name, stored_char[12], stored_char[13],
                     stored_char[14]);
      }
    }
    D3DMATRIX wm;
    for (const auto& m : impl.prop_scene.meshes) {
      if (!m.decoded || m.vertex_count == 0 || m.face_count == 0) continue;
      if (is_shadow(m.name)) continue;

      auto local_world = impl.prop_scene.world_matrix(m);
      auto character_world = mul16(local_world, prop_to_attach);
      auto world = mul16(character_world, impl.world_transform);
      if (debug_prop_enabled() && impl.logged_prop_debug &&
          m.name == "guitar.mesh") {
        std::fprintf(stderr,
                     "[prop] mesh %s local=(%.3f %.3f %.3f) "
                     "char=(%.3f %.3f %.3f) world=(%.3f %.3f %.3f)\n",
                     m.name.c_str(), local_world[12], local_world[13],
                     local_world[14], character_world[12], character_world[13],
                     character_world[14], world[12], world[13], world[14]);
        impl.logged_prop_debug = false;
      }
      std::memcpy(&wm, world.data(), 64);
      dev->SetTransform(D3DTS_WORLD, &wm);

      IDirect3DTexture9* texture = nullptr;
      const milo_scene::MatObj* mat = impl.prop_scene.find_mat(m.material);
      const ghogx::asset::Image* texture_image = nullptr;
      if (mat) {
        auto it = impl.prop_tex.find(mat->diffuse_tex);
        if (it != impl.prop_tex.end()) texture = it->second;
        auto image_it = impl.prop_tex_images.find(mat->diffuse_tex);
        if (image_it != impl.prop_tex_images.end())
          texture_image = &image_it->second;
      }

      const bool string_texture_alpha =
          texture && is_guitar_strings_prop_mesh(m.name);
      if (debug_texture_alpha_enabled() &&
          impl.logged_prop_texture_alpha_meshes.insert(m.name).second) {
        log_prop_texture_alpha_stats(m, mat, texture_image,
                                     string_texture_alpha);
      }
      if (texture) {
        dev->SetTexture(0, texture);
        dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        if (string_texture_alpha) {
          dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
          dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
          dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
          dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
          dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
          dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
          dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
          dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        } else {
          dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
          dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
          dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
          dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
          dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        }
      } else {
        dev->SetTexture(0, nullptr);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
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
  dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
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

std::optional<std::array<float, 16>> scene_object_stored_world(
    const milo_scene::Scene& scene,
    const std::string& name) {
  for (const auto& m : scene.meshes) {
    if (m.name == name) return scene.world_matrix(m);
  }
  for (const auto& t : scene.transes) {
    if (t.name == name) return xfm16(t.world_stored);
  }
  return std::nullopt;
}

}  // namespace

void skin_to_pose(const SkinnedMesh& mesh, const Character& character,
                  std::vector<std::array<float, 3>>& out_pos,
                  std::vector<std::array<float, 3>>& out_nrm) {
  out_pos.assign(mesh.verts.size(), {0, 0, 0});
  out_nrm.assign(mesh.verts.size(), {0, 0, 0});
  const size_t nb = mesh.bone_palette.size();
  if (nb == 0) {
    if (debug_mesh_mode_enabled(mesh.name)) {
      std::fprintf(stderr,
                   "[skin-mode] %-24s mat=%-18s palette=%zu bind=%zu mode=unskinned-source-trans\n",
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

  // RndMesh::SetBone stores mesh WorldXfm * inverse(bone WorldXfm). Skinning
  // consumes that decoded offset directly: vertex * offset * currentBoneWorld.
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
  const bool has_source_bone_transforms = mesh.bind.size() >= nb;
  if (!has_source_bone_transforms) {
    if (debug_mesh_mode_enabled(mesh.name)) {
      std::fprintf(stderr,
                   "[skin-mode] %-24s mat=%-18s palette=%zu bind=%zu "
                   "mode=missing-source-offset-unsupported\n",
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
  const char* skin_mode = "source-offset";
  if (debug_mesh_mode_enabled(mesh.name)) {
    std::fprintf(stderr,
                 "[skin-mode] %-24s mat=%-18s palette=%zu bind=%zu mode=%s\n",
                 mesh.name.c_str(), mesh.material.c_str(), nb,
                 mesh.bind.size(), skin_mode);
  }
  for (size_t i = 0; i < nb; ++i) {
    const std::string& bone_name = mesh.bone_palette[i];
    if (bone_name.empty() || !character.has_transform(bone_name)) {
      if (debug_skin_matrix_enabled() && debug_mesh_mode_enabled(mesh.name)) {
        std::fprintf(stderr,
                     "[skin-slot-skip] %-24s slot=%zu bone=%s reason=unresolved\n",
                     mesh.name.c_str(), i, bone_name.c_str());
      }
      continue;
    }
    const std::array<float, 16> curr_world =
        character.bone_world_local_chain(bone_name);
    skin[i] = mul16(xfm16(mesh.bind[i]), curr_world);
    const bool debug_this_skin =
        debug_skin_matrix_enabled() &&
        (debug_skin_matrix_all_enabled() || debug_mesh_mode_enabled(mesh.name));
    if (debug_this_skin && (i == 0 || debug_mesh_mode_enabled(mesh.name))) {
      const auto& s = skin[i];
      std::fprintf(stderr,
                   "[skin-matrix] %-24s bone=%-24s mode=%s "
                   "diag=(%.3f %.3f %.3f) pos=(%.3f %.3f %.3f)\n",
                   mesh.name.c_str(), bone_name.c_str(), skin_mode,
                   s[0], s[5], s[10], s[12], s[13], s[14]);
      if (debug_mesh_mode_enabled(mesh.name)) {
        const auto bind_lc =
            character.bone_world_bind_local_chain(bone_name);
        const auto curr_lc =
            character.bone_world_local_chain(bone_name);
        const auto bind_stored = character.bone_world_bind(bone_name);
        const auto curr_stored = character.bone_world(bone_name);
        const auto mesh_lc = character.bone_world_local_chain(mesh.name);
        log_compact_matrix_rows("bind-lc", mesh.name, bone_name, bind_lc);
        log_compact_matrix_rows("curr-lc", mesh.name, bone_name, curr_lc);
        log_compact_matrix_rows("bind-stored", mesh.name, bone_name,
                                bind_stored);
        log_compact_matrix_rows("curr-stored", mesh.name, bone_name,
                                curr_stored);
        log_compact_matrix_rows("mesh-lc", mesh.name, bone_name, mesh_lc);
        std::fprintf(stderr,
                     "[skin-matrix-row] %-24s bone=%-24s "
                     "r0=(%.4f %.4f %.4f %.4f) "
                     "r1=(%.4f %.4f %.4f %.4f) "
                     "r2=(%.4f %.4f %.4f %.4f) "
                     "pos=(%.4f %.4f %.4f %.4f)\n",
                     mesh.name.c_str(), bone_name.c_str(),
                     s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
                     s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15]);
        log_compact_matrix_rows("skin", mesh.name, bone_name, s);
      }
    }
  }

  for (size_t vi = 0; vi < mesh.verts.size(); ++vi) {
    const SkinVertex& v = mesh.verts[vi];
    std::array<float, 3> p{0, 0, 0}, n{0, 0, 0};
    bool any = false;
    for (size_t i = 0; i < nb && i < 4; ++i) {
      const float wgt = v.w[i];
      if (wgt == 0.0f) continue;
      if (i >= skin.size() || mesh.bone_palette[i].empty() ||
          !character.has_transform(mesh.bone_palette[i])) {
        continue;
      }
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
