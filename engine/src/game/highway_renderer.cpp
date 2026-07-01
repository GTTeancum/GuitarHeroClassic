// engine/src/game/highway_renderer.cpp
//
// Geometry and camera are taken VERBATIM from the GH2 data, not eyeballed:
//   * config/gen/track_graphics.dtb : track_width 20, horizon_y 110,
//     remove_y -15, alpha_dist 40, track_speed per difficulty, slot_colors.
//   * track/gen/track.milo_ps2 -> Cam 'track.cam' : position (0.197,-63.22,
//     17.94), 6.53deg downward pitch, near 50, far 250, fov 0.4102 rad.
// World axes (Harmonix track space): X = lanes (across), Y = depth/scroll
// (notes travel from +Y toward 0 = strikeline), Z = up.

#include "game/highway_renderer.h"
#include "game/gameplay_session.h"
#include "ark_v3.h"
#include "milo_scene/milo_scene.h"
#include "milo.h"
#include "render/window_d3d9.h"
#include "render/scene_d3d9.h"   // Mat4
#include "asset/milo_image.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace ghogx::game {

namespace {

struct V3 { float x, y, z; D3DCOLOR c; float u, v; };
constexpr DWORD kFVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;

// --- Geometry constants, all from track_graphics.dtb / track.cam ----------
constexpr float kLaneSpacing = 4.0f;     // track_width 20 / 5 lanes
constexpr float kBoardHalfX  = 10.0f;    // track_width 20 -> half = 10
constexpr float kStrikeY     = 0.0f;     // strikeline (hit line) depth
constexpr float kTopY        = 110.0f;   // horizon_y: spawn / push-on distance
constexpr float kRemoveY     = -15.0f;   // remove_y: prune distance
constexpr float kAlphaDist   = 40.0f;    // alpha_dist: gem fade-in band at horizon
constexpr float kBoardZ      = 0.0f;     // board surface height
constexpr float kGemZ        = 0.12f;    // gems just above the board
constexpr float kGemHalf     = 1.7f;     // ~lane width (gem-mesh-exact decode pending)

// Camera (track.cam, decoded). Harmonix cams look down local +Y, up = local +Z.
constexpr float kCamPos[3] = { 0.197f, -63.22f, 17.94f };
constexpr float kCamFwd[3] = { 0.0f, 0.99342f, -0.11378f };
constexpr float kCamUp [3] = { 0.0f, 0.11378f,  0.99342f };
constexpr float kCamNear   = 50.0f;
constexpr float kCamFar    = 250.0f;
constexpr float kCamFov    = 0.4102f;    // vertical fov, radians

// Per-difficulty scroll multiplier (track_graphics.dtb track_speed).
constexpr float kTrackSpeed[4] = { 1.0f, 1.0f, 1.4f, 1.4f };
// Base scroll rate (world-units/sec). y_per_second is a TrackDir instance prop
// in the milo (PanelDir property table) not yet decoded; documented placeholder
// giving GH-like lead time. PIN from TrackDir y_per_second.
constexpr float kYPerSecond  = 80.0f;

inline float lane_x(int lane) { return (static_cast<float>(lane) - 2.0f) * kLaneSpacing; }

const char* gem_tex_name(int lane) {
  switch (lane) {
    case 0: return "gem_green.tex";
    case 1: return "gem_red.tex";
    case 2: return "gem_yellow.tex";
    case 3: return "gem_blue.tex";
    default: return "gem_orange.tex";
  }
}

const char* lane_name(int lane) {
  switch (lane) {
    case 0: return "green";
    case 1: return "red";
    case 2: return "yellow";
    case 3: return "blue";
    default: return "orange";
  }
}

bool is_lane_gem_tex_name(const std::string& name) {
  return name == "gem_green.tex" || name == "gem_red.tex" ||
         name == "gem_yellow.tex" || name == "gem_blue.tex" ||
         name == "gem_orange.tex";
}

uint8_t lane_gem_alpha(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  if (r <= 8 && g <= 8 && b <= 8) return 0;
  return a;
}

bool env_enabled(const char* name) {
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, name) == 0 && value && value[0] != '\0';
  std::free(value);
  return enabled;
}

struct MatAnimColorKey {
  float rgb[3] = {1.0f, 1.0f, 1.0f};
  float frame = 0.0f;
};

struct MatAnimColorKeys {
  std::string material;
  std::vector<MatAnimColorKey> keys;
};

float clamp_color(float value) {
  if (!std::isfinite(value)) return 1.0f;
  return std::clamp(value, 0.0f, 1.0f);
}

bool read_u32(const uint8_t* body, size_t size, size_t& pos, uint32_t& out) {
  if (pos + 4 > size) return false;
  std::memcpy(&out, body + pos, sizeof(out));
  pos += 4;
  return true;
}

bool read_f32(const uint8_t* body, size_t size, size_t& pos, float& out) {
  if (pos + 4 > size) return false;
  std::memcpy(&out, body + pos, sizeof(out));
  pos += 4;
  return std::isfinite(out);
}

std::optional<std::string> read_milo_string(const uint8_t* body, size_t size,
                                            size_t& pos) {
  uint32_t len = 0;
  if (!read_u32(body, size, pos, len) || len == 0 || len > 128 ||
      pos + len > size) {
    return std::nullopt;
  }
  std::string s(reinterpret_cast<const char*>(body + pos), len);
  pos += len;
  return s;
}

std::map<std::string, MatAnimColorKeys> load_track_mat_anim_colors(
    const std::string& hdr_path, const std::string& ark_path) {
  std::map<std::string, MatAnimColorKeys> out;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find("track/gen/track.milo_ps2");
    if (!entry) return out;
    auto bytes = ark.read_entry(*entry, {ark_path});
    auto hdr = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, hdr);
    auto dir = gh::milo::parse_directory(payload);
    for (const auto& de : dir.entries) {
      if (de.type != "MatAnim" || de.offset + de.size > payload.size())
        continue;
      const auto* body = payload.data() + de.offset;
      const size_t size = static_cast<size_t>(de.size);
      if (size < 48) continue;
      uint32_t version = 0;
      std::memcpy(&version, body, sizeof(version));
      if (version != 7) continue;
      size_t pos = 25;
      auto material = read_milo_string(body, size, pos);
      auto anim_name = read_milo_string(body, size, pos);
      if (!material || !anim_name ||
          (*material != "track_side_rails.mat" &&
           *material != "track_surface.mat"))
        continue;
      uint32_t color_count = 0;
      if (!read_u32(body, size, pos, color_count) || color_count > 16)
        continue;
      MatAnimColorKeys anim;
      anim.material = *material;
      for (uint32_t i = 0; i < color_count; ++i) {
        MatAnimColorKey key;
        float ignored_alpha = 1.0f;
        if (!read_f32(body, size, pos, key.rgb[0]) ||
            !read_f32(body, size, pos, key.rgb[1]) ||
            !read_f32(body, size, pos, key.rgb[2]) ||
            !read_f32(body, size, pos, ignored_alpha) ||
            !read_f32(body, size, pos, key.frame)) {
          anim.keys.clear();
          break;
        }
        key.rgb[0] = clamp_color(key.rgb[0]);
        key.rgb[1] = clamp_color(key.rgb[1]);
        key.rgb[2] = clamp_color(key.rgb[2]);
        anim.keys.push_back(key);
      }
      if (!anim.keys.empty()) out[*anim_name] = std::move(anim);
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[highway] track side-rail MatAnim load: %s\n",
                 ex.what());
  }
  return out;
}

HighwayRenderer::SideRailColorState side_rail_color_from_anim(
    const std::map<std::string, MatAnimColorKeys>& anims,
    const std::string& name,
    bool use_last_key) {
  HighwayRenderer::SideRailColorState out;
  const auto it = anims.find(name);
  if (it == anims.end() || it->second.keys.empty()) return out;
  const auto& key = use_last_key ? it->second.keys.back()
                                : it->second.keys.front();
  out.r = key.rgb[0];
  out.g = key.rgb[1];
  out.b = key.rgb[2];
  out.ok = true;
  return out;
}

HighwayRenderer::SideRailColorState mat_anim_color_key(
    const std::map<std::string, MatAnimColorKeys>& anims,
    const std::string& name,
    size_t key_index) {
  HighwayRenderer::SideRailColorState out;
  const auto it = anims.find(name);
  if (it == anims.end() || it->second.keys.empty()) return out;
  const auto& keys = it->second.keys;
  const auto& key = keys[std::min(key_index, keys.size() - 1)];
  out.r = key.rgb[0];
  out.g = key.rgb[1];
  out.b = key.rgb[2];
  out.ok = true;
  return out;
}

HighwayRenderer::SideRailColorState lerp_side_rail_color(
    HighwayRenderer::SideRailColorState a,
    HighwayRenderer::SideRailColorState b,
    float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  if (!a.ok) a = {};
  if (!b.ok) b = a;
  HighwayRenderer::SideRailColorState out;
  out.r = a.r + (b.r - a.r) * t;
  out.g = a.g + (b.g - a.g) * t;
  out.b = a.b + (b.b - a.b) * t;
  out.ok = a.ok || b.ok;
  return out;
}

D3DCOLOR side_rail_d3d_color(HighwayRenderer::SideRailColorState color) {
  const int r = std::clamp(static_cast<int>(color.r * 255.0f), 0, 255);
  const int g = std::clamp(static_cast<int>(color.g * 255.0f), 0, 255);
  const int b = std::clamp(static_cast<int>(color.b * 255.0f), 0, 255);
  return D3DCOLOR_ARGB(255, r, g, b);
}

D3DCOLOR multiply_rgb(D3DCOLOR base, HighwayRenderer::SideRailColorState color,
                      float strength) {
  strength = std::clamp(strength, 0.0f, 1.0f);
  if (!color.ok || strength <= 0.0f) return base;
  const float cr = 1.0f + (color.r - 1.0f) * strength;
  const float cg = 1.0f + (color.g - 1.0f) * strength;
  const float cb = 1.0f + (color.b - 1.0f) * strength;
  const int a = static_cast<int>((base >> 24) & 0xff);
  const int r = std::clamp(
      static_cast<int>(static_cast<float>((base >> 16) & 0xff) * cr), 0, 255);
  const int g = std::clamp(
      static_cast<int>(static_cast<float>((base >> 8) & 0xff) * cg), 0, 255);
  const int b = std::clamp(
      static_cast<int>(static_cast<float>(base & 0xff) * cb), 0, 255);
  return D3DCOLOR_ARGB(a, r, g, b);
}

const char* now_ring_name(int lane) {
  switch (lane) {
    case 0: return "now_green_add.tex";
    case 1: return "now_red_add.tex";
    case 2: return "now_yellow_add.tex";
    case 3: return "now_blue_add.tex";
    default: return "now_orange_add.tex";
  }
}

// Build a quad on the board (XY plane at height z), centered at (cx, cy),
// half-width hx (X across) and half-depth hy (Y along the track). Far = +Y.
void flat_quad(V3 out[4], float cx, float cy, float z, float hx, float hy,
               D3DCOLOR col) {
  out[0] = { cx - hx, cy + hy, z, col, 0.0f, 0.0f };  // far-left
  out[1] = { cx + hx, cy + hy, z, col, 1.0f, 0.0f };  // far-right
  out[2] = { cx - hx, cy - hy, z, col, 0.0f, 1.0f };  // near-left
  out[3] = { cx + hx, cy - hy, z, col, 1.0f, 1.0f };  // near-right
}

void draw_quad(IDirect3DDevice9* dev,
               IDirect3DTexture9* texture,
               const V3 c[4],
               bool use_texture_alpha = true) {
  const V3 tris[6] = { c[0], c[1], c[2], c[1], c[3], c[2] };
  dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
  if (texture) {
    dev->SetTexture(0, texture);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP,
                              use_texture_alpha ? D3DTOP_MODULATE
                                                : D3DTOP_SELECTARG2);
  } else {
    dev->SetTexture(0, nullptr);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
  }
  dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tris, sizeof(V3));
}

// Fade factor 0..1 for a note/board point at depth y (1 near the strike, fading
// out over the alpha_dist band just before the horizon).
inline float depth_fade(float y) {
  const float start = kTopY - kAlphaDist;  // begin fading here
  if (y <= start) return 1.0f;
  if (y >= kTopY) return 0.0f;
  return 1.0f - (y - start) / kAlphaDist;
}

}  // namespace

using ghogx::render::Mat4;

HighwayRenderer::HighwayRenderer(ghogx::render::Window& win) : win_(&win) {
  dev_ = static_cast<IDirect3DDevice9*>(win.device_ptr());
}

HighwayRenderer::~HighwayRenderer() {
  release_textures();
}

void HighwayRenderer::release_textures() {
  for (auto& kv : textures_) {
    if (kv.second) kv.second->Release();
  }
  textures_.clear();
  loaded_surface_ref_.clear();
  selected_surface_loaded_ = false;
  loaded_ = false;
}

IDirect3DTexture9* HighwayRenderer::tex(const std::string& name) const {
  auto it = textures_.find(name);
  return it == textures_.end() ? nullptr : it->second;
}

void HighwayRenderer::draw_runtime_mesh(const RuntimeMesh& mesh,
                                        float cx,
                                        float cy,
                                        uint32_t tint,
                                        float scale,
                                        bool use_texture_alpha) const {
  draw_runtime_mesh_with_texture(mesh, mesh.texture_name, cx, cy, tint, scale,
                                 use_texture_alpha);
}

void HighwayRenderer::draw_runtime_mesh_with_texture(
    const RuntimeMesh& mesh,
    const std::string& texture_name,
    float cx,
    float cy,
    uint32_t tint,
    float scale,
    bool use_texture_alpha) const {
  draw_runtime_mesh_scaled_with_texture(mesh, texture_name, cx, cy, tint, scale,
                                        scale, scale, use_texture_alpha);
}

void HighwayRenderer::draw_runtime_mesh_scaled_with_texture(
    const RuntimeMesh& mesh,
    const std::string& texture_name,
    float cx,
    float cy,
    uint32_t tint,
    float scale_x,
    float scale_y,
    float scale_z,
    bool use_texture_alpha) const {
  if (!dev_ || !mesh.ok || mesh.indices.empty() || mesh.verts.empty()) return;
  std::vector<V3> tris;
  tris.reserve(mesh.indices.size());
  const float ta = static_cast<float>((tint >> 24) & 0xff) / 255.0f;
  const float tr = static_cast<float>((tint >> 16) & 0xff) / 255.0f;
  const float tg = static_cast<float>((tint >> 8) & 0xff) / 255.0f;
  const float tb = static_cast<float>(tint & 0xff) / 255.0f;
  for (uint16_t idx : mesh.indices) {
    if (idx >= mesh.verts.size()) continue;
    const auto& v = mesh.verts[idx];
    const int a = std::clamp(static_cast<int>(v.a * ta * 255.0f), 0, 255);
    const int r = std::clamp(static_cast<int>(v.r * tr * 255.0f), 0, 255);
    const int g = std::clamp(static_cast<int>(v.g * tg * 255.0f), 0, 255);
    const int b = std::clamp(static_cast<int>(v.b * tb * 255.0f), 0, 255);
    tris.push_back({cx + v.x * scale_x, cy + v.y * scale_y, v.z * scale_z,
                    D3DCOLOR_ARGB(a, r, g, b), v.u, v.v});
  }
  if (tris.empty()) return;
  IDirect3DTexture9* texture = tex(texture_name);
  if (texture) {
    dev_->SetTexture(0, texture);
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP,
                               use_texture_alpha ? D3DTOP_MODULATE
                                                 : D3DTOP_SELECTARG2);
  } else {
    dev_->SetTexture(0, nullptr);
    dev_->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    dev_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
  }
  dev_->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                        static_cast<UINT>(tris.size() / 3),
                        tris.data(), sizeof(V3));
}

void HighwayRenderer::draw_centered_runtime_mesh_scaled(
    const RuntimeMesh& mesh,
    float cx,
    float cy,
    uint32_t tint,
    float scale_x,
    float scale_y,
    float scale_z,
    bool use_texture_alpha) const {
  draw_runtime_mesh_scaled_with_texture(
      mesh, mesh.texture_name, cx - mesh.center_x * scale_x,
      cy - mesh.center_y * scale_y, tint, scale_x, scale_y, scale_z,
      use_texture_alpha);
}

void HighwayRenderer::draw_centered_runtime_mesh(const RuntimeMesh& mesh,
                                                 float cx,
                                                 float cy,
                                                 uint32_t tint,
                                                 float scale,
                                                 bool use_texture_alpha) const {
  draw_runtime_mesh(mesh, cx - mesh.center_x * scale,
                    cy - mesh.center_y * scale, tint, scale,
                    use_texture_alpha);
}

void HighwayRenderer::draw_centered_runtime_mesh_with_texture(
    const RuntimeMesh& mesh,
    const std::string& texture_name,
    float cx,
    float cy,
    uint32_t tint,
    float scale,
    bool use_texture_alpha) const {
  draw_runtime_mesh_with_texture(mesh, texture_name, cx - mesh.center_x * scale,
                                 cy - mesh.center_y * scale, tint, scale,
                                 use_texture_alpha);
}

bool HighwayRenderer::load_textures(const std::string& hdr_path,
                                    const std::string& ark_path,
                                    const std::string& surface_ref) {
  if (!dev_) return false;
  if (!textures_.empty()) release_textures();
  for (auto& mesh : gem_mesh_) mesh = RuntimeMesh{};
  for (auto& mesh : hopo_mesh_) mesh = RuntimeMesh{};
  for (auto& mesh : star_mesh_) mesh = RuntimeMesh{};
  for (auto& mesh : star_top_mesh_) mesh = RuntimeMesh{};
  for (auto& mesh : tail_mesh_) mesh = RuntimeMesh{};
  star_base_mesh_ = RuntimeMesh{};
  gem_glow_mesh_ = RuntimeMesh{};
  held_tail_mesh_ = RuntimeMesh{};
  star_tail_mesh_ = RuntimeMesh{};
  bonus_tail_mesh_ = RuntimeMesh{};
  bonus_gem_mesh_ = RuntimeMesh{};
  bonus_gem_overlay_mesh_ = RuntimeMesh{};
  gem_sparkle_mesh_ = RuntimeMesh{};
  bonus_spark1_mesh_ = RuntimeMesh{};
  bonus_spark2_mesh_ = RuntimeMesh{};
  track_surface_mesh_ = RuntimeMesh{};
  track_mask_mesh_ = RuntimeMesh{};
  surface_flash_2x_ = SideRailColorState{};
  surface_flash_3x_ = SideRailColorState{};
  surface_flash_4x_ = SideRailColorState{};
  track_side_rails_mesh_ = RuntimeMesh{};
  side_rails_none_ = SideRailColorState{};
  side_rails_warning_ = SideRailColorState{};
  side_rails_star_ = SideRailColorState{};
  side_rails_warning_star_ = SideRailColorState{};
  track_lane_lines_mesh_ = RuntimeMesh{};
  star_power_track_glow_mesh_ = RuntimeMesh{};
  bar_line_mesh_ = RuntimeMesh{};
  beat_line_mesh_ = RuntimeMesh{};
  half_beat_line_mesh_ = RuntimeMesh{};
  quarter_beat_line_mesh_ = RuntimeMesh{};
  gem_smasher_mesh_ = RuntimeMesh{};
  smasher_rim_mesh_ = RuntimeMesh{};
  smasher_shadow_mesh_ = RuntimeMesh{};
  hit_flame_mesh_ = RuntimeMesh{};
  star_collect_flame_mesh_ = RuntimeMesh{};
  bonus_hit_flame_mesh_ = RuntimeMesh{};
  miss_mesh_ = RuntimeMesh{};
  miss_top_mesh_ = RuntimeMesh{};
  for (auto& mesh : combo_lightning_mesh_) mesh = RuntimeMesh{};
  track_explode_meshes_.clear();
  smasher_texture_names_.fill({});
  smasher_add_texture_names_.fill({});
  bonus_smasher_texture_name_.clear();
  bonus_smasher_add_texture_name_.clear();
  selected_surface_loaded_ = false;

  std::set<std::string> texture_names = {
      "track_surface.tex", "track_fade.tex", "barline_gw.tex",
      "gem_green.tex", "gem_red.tex", "gem_yellow.tex", "gem_blue.tex", "gem_orange.tex",
      "gem.tex", "gem_glow.tex", "gem_shadow.tex", "stargem.tex",
      "now_green_add.tex", "now_red_add.tex", "now_yellow_add.tex",
      "now_blue_add.tex", "now_orange_add.tex", "now_ring_add.tex",
      "smasher_on.tex", "smasher_off.tex",
      "tail2.tex", "tail_tight.tex", "flame_part.tex",
  };

  ghogx::milo_scene::Scene track_scene;
  if (ghogx::milo_scene::load_scene(hdr_path, ark_path,
                                    "track/gen/track.milo_ps2",
                                    track_scene)) {
    auto find_mesh = [&](const std::string& mesh_name)
        -> const ghogx::milo_scene::MeshObj* {
      for (const auto& mesh : track_scene.meshes) {
        if (mesh.name == mesh_name && mesh.decoded) return &mesh;
      }
      return nullptr;
    };
    auto convert_mesh = [&](const std::string& mesh_name,
                            const std::string& material_override = std::string()) {
      RuntimeMesh out;
      const auto* mesh = find_mesh(mesh_name);
      if (!mesh || mesh->verts.empty() || mesh->indices.empty()) return out;
      const std::string& material_name =
          material_override.empty() ? mesh->material : material_override;
      const auto* mat = track_scene.find_mat(material_name);
      if (!mat) return out;
      out.texture_name = mat->diffuse_tex;
      if (!out.texture_name.empty()) texture_names.insert(out.texture_name);
      out.verts.reserve(mesh->verts.size());
      float min_x = 0.0f;
      float max_x = 0.0f;
      float min_y = 0.0f;
      float max_y = 0.0f;
      bool have_bounds = false;
      for (const auto& src : mesh->verts) {
        MeshVertex dst;
        dst.x = src.px * mesh->local.rot[0][0] +
                src.py * mesh->local.rot[1][0] +
                src.pz * mesh->local.rot[2][0] + mesh->local.pos[0];
        dst.y = src.px * mesh->local.rot[0][1] +
                src.py * mesh->local.rot[1][1] +
                src.pz * mesh->local.rot[2][1] + mesh->local.pos[1];
        dst.z = src.px * mesh->local.rot[0][2] +
                src.py * mesh->local.rot[1][2] +
                src.pz * mesh->local.rot[2][2] + mesh->local.pos[2];
        dst.r = src.r * mat->color[0];
        dst.g = src.g * mat->color[1];
        dst.b = src.b * mat->color[2];
        dst.a = src.a * mat->color[3];
        dst.u = src.u;
        dst.v = src.v;
        if (!have_bounds) {
          min_x = max_x = dst.x;
          min_y = max_y = dst.y;
          have_bounds = true;
        } else {
          min_x = std::min(min_x, dst.x);
          max_x = std::max(max_x, dst.x);
          min_y = std::min(min_y, dst.y);
          max_y = std::max(max_y, dst.y);
        }
        out.verts.push_back(dst);
      }
      out.min_x = min_x;
      out.max_x = max_x;
      out.min_y = min_y;
      out.max_y = max_y;
      out.center_x = (min_x + max_x) * 0.5f;
      out.center_y = (min_y + max_y) * 0.5f;
      out.indices = mesh->indices;
      out.ok = !out.verts.empty() && !out.indices.empty();
      return out;
    };
    auto material_texture = [&](const std::string& mat_name) {
      const auto* mat = track_scene.find_mat(mat_name);
      if (!mat || mat->diffuse_tex.empty()) return std::string{};
      texture_names.insert(mat->diffuse_tex);
      return mat->diffuse_tex;
    };
    for (int lane = 0; lane < 5; ++lane) {
      const std::string name = lane_name(lane);
      gem_mesh_[lane] = convert_mesh(name + "_gem.mesh");
      hopo_mesh_[lane] = convert_mesh(name + "_hopo.mesh");
      star_mesh_[lane] = convert_mesh(name + "_star.mesh");
      star_top_mesh_[lane] = convert_mesh(name + "_top_star.mesh");
      tail_mesh_[lane] = convert_mesh("tail02.mesh", "tail_" + name + ".mat");
      smasher_texture_names_[lane] =
          material_texture("gem_smasher_" + name + ".mat");
      smasher_add_texture_names_[lane] =
          material_texture("gem_smasher_" + name + "_1.mat");
    }
    star_base_mesh_ = convert_mesh("star_base.mesh");
    gem_glow_mesh_ = convert_mesh("glow.mesh");
    held_tail_mesh_ = convert_mesh("tail02.mesh", "tail_white.mat");
    star_tail_mesh_ = convert_mesh("tail02.mesh", "tail_glow_tight.mat");
    if (!star_tail_mesh_.ok) {
      star_tail_mesh_ = convert_mesh("tail02.mesh", "tail_star.mat");
    }
    bonus_tail_mesh_ = convert_mesh("tail02.mesh", "tail_bonus.mat");
    bonus_gem_mesh_ = convert_mesh("gem_bonus.mesh");
    bonus_gem_overlay_mesh_ = convert_mesh("gem_bonus2.mesh");
    gem_sparkle_mesh_ = convert_mesh("gem_sparkle.mesh");
    bonus_spark1_mesh_ = convert_mesh("gem_bonus_spark1.mesh");
    bonus_spark2_mesh_ = convert_mesh("gem_bonus_spark2.mesh");
    track_surface_mesh_ = convert_mesh("track_surface5.mesh");
    track_mask_mesh_ = convert_mesh("track_mask.mesh");
    track_side_rails_mesh_ = convert_mesh("track_side_rails5.mesh");
    track_lane_lines_mesh_ = convert_mesh("track_lane_lines5.mesh");
    star_power_track_glow_mesh_ = convert_mesh("lightning_trackglow.mesh");
    bar_line_mesh_ = convert_mesh("bar_line5.mesh");
    beat_line_mesh_ = convert_mesh("beat_line5.mesh");
    half_beat_line_mesh_ = convert_mesh("half_beat_line5.mesh");
    quarter_beat_line_mesh_ = convert_mesh("quarter_beat_line5.mesh");
    gem_smasher_mesh_ = convert_mesh("gem_smasher.mesh");
    smasher_rim_mesh_ = convert_mesh("smasher_rim.mesh");
    smasher_shadow_mesh_ = convert_mesh("smasher shadow.mesh");
    hit_flame_mesh_ = convert_mesh("smash_flamelight.mesh");
    star_collect_flame_mesh_ = convert_mesh("smash_flamelight_starcollect.mesh");
    bonus_hit_flame_mesh_ = convert_mesh("smash_flamelight_bonus.mesh");
    miss_mesh_ = convert_mesh("miss.mesh", "gem_miss.mat");
    miss_top_mesh_ = convert_mesh("top_miss.mesh", "gem_miss_1.mat");
    bonus_smasher_texture_name_ = material_texture("gem_smasher_bonus.mat");
    bonus_smasher_add_texture_name_ =
        material_texture("gem_smasher_bonus_1.mat");
    for (int i = 0; i < 3; ++i) {
      combo_lightning_mesh_[i] = convert_mesh(
          "smash_combo_lightning0" + std::to_string(i + 1) + ".mesh");
    }
    std::vector<std::string> explode_names;
    for (const auto& mesh : track_scene.meshes) {
      if (mesh.name.rfind("track_explode", 0) != 0) continue;
      if (!mesh.decoded || mesh.verts.empty() || mesh.indices.empty()) continue;
      explode_names.push_back(mesh.name);
    }
    std::sort(explode_names.begin(), explode_names.end());
    explode_names.erase(std::unique(explode_names.begin(), explode_names.end()),
                        explode_names.end());
    for (const auto& mesh_name : explode_names) {
      RuntimeMesh mesh = convert_mesh(mesh_name);
      if (mesh.ok) track_explode_meshes_.push_back(std::move(mesh));
    }
    const auto side_rail_anims = load_track_mat_anim_colors(hdr_path, ark_path);
    side_rails_none_ =
        side_rail_color_from_anim(side_rail_anims, "side_rails_none.mnm",
                                  false);
    side_rails_warning_ =
        side_rail_color_from_anim(side_rail_anims, "side_rails_warning.mnm",
                                  true);
    side_rails_star_ =
        side_rail_color_from_anim(side_rail_anims, "side_rails_star.mnm",
                                  false);
    side_rails_warning_star_ = side_rail_color_from_anim(
        side_rail_anims, "side_rails_warning_star.mnm", true);
    surface_flash_2x_ =
        mat_anim_color_key(side_rail_anims, "surface_flash_2x.mnm", 1);
    surface_flash_3x_ =
        mat_anim_color_key(side_rail_anims, "surface_flash_3x.mnm", 1);
    surface_flash_4x_ =
        mat_anim_color_key(side_rail_anims, "surface_flash_4x.mnm", 1);
    std::fprintf(stderr,
                 "[highway] native note meshes: gems=%d hopos=%d stars=%d glow=%d\n",
                 static_cast<int>(std::count_if(
                     gem_mesh_.begin(), gem_mesh_.end(),
                     [](const RuntimeMesh& m) { return m.ok; })),
                 static_cast<int>(std::count_if(
                     hopo_mesh_.begin(), hopo_mesh_.end(),
                     [](const RuntimeMesh& m) { return m.ok; })),
                 static_cast<int>(std::count_if(
                     star_mesh_.begin(), star_mesh_.end(),
                     [](const RuntimeMesh& m) { return m.ok; })),
                 gem_glow_mesh_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] native track meshes: surface=%d mask=%d rails=%d lines=%d spglow=%d smasher=%d hitflame=%d starcollect=%d miss=%d combo=%d explode=%zu\n",
                 track_surface_mesh_.ok ? 1 : 0,
                 track_mask_mesh_.ok ? 1 : 0,
                 track_side_rails_mesh_.ok ? 1 : 0,
                 track_lane_lines_mesh_.ok ? 1 : 0,
                 star_power_track_glow_mesh_.ok ? 1 : 0,
                 gem_smasher_mesh_.ok ? 1 : 0,
                 hit_flame_mesh_.ok ? 1 : 0,
                 star_collect_flame_mesh_.ok ? 1 : 0,
                 miss_mesh_.ok ? 1 : 0,
                 static_cast<int>(std::count_if(
                     combo_lightning_mesh_.begin(),
                     combo_lightning_mesh_.end(),
                     [](const RuntimeMesh& m) { return m.ok; })),
                 track_explode_meshes_.size());
    std::fprintf(stderr,
                 "[highway] side-rail MatAnim states: none=%d warning=%d star=%d warning_star=%d\n",
                 side_rails_none_.ok ? 1 : 0,
                 side_rails_warning_.ok ? 1 : 0,
                 side_rails_star_.ok ? 1 : 0,
                 side_rails_warning_star_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] surface flash MatAnim states: 2x=%d 3x=%d 4x=%d\n",
                 surface_flash_2x_.ok ? 1 : 0,
                 surface_flash_3x_.ok ? 1 : 0,
                 surface_flash_4x_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] native smasher lane materials: base=%d add=%d\n",
                 static_cast<int>(std::count_if(
                     smasher_texture_names_.begin(), smasher_texture_names_.end(),
                     [](const std::string& name) { return !name.empty(); })),
                 static_cast<int>(std::count_if(
                     smasher_add_texture_names_.begin(),
                     smasher_add_texture_names_.end(),
                     [](const std::string& name) { return !name.empty(); })));
    std::fprintf(stderr,
                 "[highway] native timing meshes: quarter=%d half=%d beat=%d bar=%d\n",
                 quarter_beat_line_mesh_.ok ? 1 : 0,
                 half_beat_line_mesh_.ok ? 1 : 0,
                 beat_line_mesh_.ok ? 1 : 0,
                 bar_line_mesh_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] native sustain tail meshes: lanes=%d held=%d star=%d\n",
                 static_cast<int>(std::count_if(
                     tail_mesh_.begin(), tail_mesh_.end(),
                     [](const RuntimeMesh& m) { return m.ok; })),
                 held_tail_mesh_.ok ? 1 : 0,
                 star_tail_mesh_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] native bonus meshes: gem=%d overlay=%d tail=%d smasher=%d flame=%d\n",
                 bonus_gem_mesh_.ok ? 1 : 0,
                 bonus_gem_overlay_mesh_.ok ? 1 : 0,
                 bonus_tail_mesh_.ok ? 1 : 0,
                 !bonus_smasher_texture_name_.empty() ? 1 : 0,
                 bonus_hit_flame_mesh_.ok ? 1 : 0);
    std::fprintf(stderr,
                 "[highway] native gem sparkle meshes: star=%d bonus1=%d bonus2=%d\n",
                 gem_sparkle_mesh_.ok ? 1 : 0,
                 bonus_spark1_mesh_.ok ? 1 : 0,
                 bonus_spark2_mesh_.ok ? 1 : 0);
  }

  const std::vector<std::string> names(texture_names.begin(),
                                       texture_names.end());
  auto imgs = ghogx::asset::load_milo_textures(hdr_path, ark_path,
                                               "track/gen/track.milo_ps2", names);
  if (imgs.empty()) { std::fprintf(stderr, "[highway] no track textures\n"); return false; }

  auto upload_image = [&](const ghogx::asset::Image& img,
                          bool color_key_lane_gem) -> IDirect3DTexture9* {
    if (!img.valid()) return nullptr;
    IDirect3DTexture9* t = nullptr;
    if (FAILED(dev_->CreateTexture(static_cast<UINT>(img.width),
                                   static_cast<UINT>(img.height), 1, 0,
                                   D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t, nullptr)))
      return nullptr;
    D3DLOCKED_RECT lr;
    if (SUCCEEDED(t->LockRect(0, &lr, nullptr, 0))) {
      for (int y = 0; y < img.height; ++y) {
        auto* dst = static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch;
        const uint8_t* src = img.rgba.data() + static_cast<size_t>(y) * img.width * 4;
        for (int x = 0; x < img.width; ++x) {
          const uint8_t r = src[x*4+0];
          const uint8_t g = src[x*4+1];
          const uint8_t b = src[x*4+2];
          const uint8_t a = color_key_lane_gem
              ? lane_gem_alpha(r, g, b, src[x*4+3])
              : src[x*4+3];
          dst[x*4+0] = b; dst[x*4+1] = g;
          dst[x*4+2] = r; dst[x*4+3] = a;
        }
      }
      t->UnlockRect(0);
    }
    return t;
  };

  for (auto& kv : imgs) {
    const bool color_key_lane_gem = is_lane_gem_tex_name(kv.first);
    if (IDirect3DTexture9* t = upload_image(kv.second, color_key_lane_gem)) {
      textures_[kv.first] = t;
    }
  }

  std::string surface_path;
  const ghogx::asset::Image surface =
      ghogx::asset::load_track_surface_bitmap(
          hdr_path, ark_path, surface_ref, &surface_path);
  if (!surface_path.empty()) {
    if (IDirect3DTexture9* t =
            upload_image(surface, false)) {
      auto existing = textures_.find("track_surface.tex");
      if (existing != textures_.end() && existing->second) {
        existing->second->Release();
      }
      textures_["track_surface.tex"] = t;
      selected_surface_loaded_ = true;
      std::fprintf(stderr, "[highway] selected guitarist surface: %s\n",
                   surface_path.c_str());
    } else {
      std::fprintf(stderr,
                   "[highway] selected surface unavailable: %s; using track.milo default\n",
                   surface_path.c_str());
    }
  }
  loaded_ = !textures_.empty();
  loaded_surface_ref_ = loaded_ ? surface_ref : std::string{};
  std::fprintf(stderr, "[highway] %zu track textures -> D3D\n", textures_.size());
  return loaded_;
}

void HighwayRenderer::draw(double song_time, const ghogx::chart::Chart& chart,
                           int difficulty, uint32_t fret_held_mask,
                           const float hit_flash[5], float lookahead_sec,
                           const std::vector<uint8_t>* consumed_notes,
                           const std::vector<FoFiXSessionSustain>* active_sustains,
                           bool star_power_active,
                           const float star_collect_flash[5],
                           const float miss_flash[5],
                           int combo_multiplier,
                            float bad_feedback_flash,
                            float surface_flash) {
  draw_impl(song_time, chart, difficulty, fret_held_mask, hit_flash,
            lookahead_sec, true, consumed_notes, active_sustains,
            star_power_active, star_collect_flash, miss_flash,
            combo_multiplier, bad_feedback_flash, surface_flash);
}

void HighwayRenderer::draw_over_scene(double song_time,
                                      const ghogx::chart::Chart& chart,
                                      int difficulty,
                                      uint32_t fret_held_mask,
                                      const float hit_flash[5],
                                      float lookahead_sec,
                                      const std::vector<uint8_t>* consumed_notes,
                                      const std::vector<FoFiXSessionSustain>* active_sustains,
                                       bool star_power_active,
                                       const float star_collect_flash[5],
                                       const float miss_flash[5],
                                       int combo_multiplier,
                                       float bad_feedback_flash,
                                       float surface_flash) {
  draw_impl(song_time, chart, difficulty, fret_held_mask, hit_flash,
            lookahead_sec, false, consumed_notes, active_sustains,
            star_power_active, star_collect_flash, miss_flash,
            combo_multiplier, bad_feedback_flash, surface_flash);
}

void HighwayRenderer::draw_impl(double song_time,
                                const ghogx::chart::Chart& chart,
                                int difficulty,
                                uint32_t fret_held_mask,
                                const float hit_flash[5],
                                float lookahead_sec,
                                bool clear_target,
                                const std::vector<uint8_t>* consumed_notes,
                                const std::vector<FoFiXSessionSustain>* active_sustains,
                                 bool star_power_active,
                                 const float star_collect_flash[5],
                                 const float miss_flash[5],
                                 int combo_multiplier,
                                 float bad_feedback_flash,
                                 float surface_flash) {
  if (!dev_) return;
  if (clear_target) {
    dev_->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
  }
  dev_->BeginScene();

  // --- Camera: the exact track.cam transform ---
  const float aspect = win_->bb_height() > 0
      ? static_cast<float>(win_->bb_width()) / static_cast<float>(win_->bb_height())
      : 16.0f / 9.0f;
  Mat4 view = Mat4::look_at_lh(kCamPos[0], kCamPos[1], kCamPos[2],
                               kCamPos[0]+kCamFwd[0], kCamPos[1]+kCamFwd[1], kCamPos[2]+kCamFwd[2],
                               kCamUp[0], kCamUp[1], kCamUp[2]);
  Mat4 proj = Mat4::perspective_lh(kCamFov, aspect, kCamNear, kCamFar);
  // GH2 track space is right-handed (X right, +Y forward/into-screen, Z up);
  // our D3D pipeline is left-handed. Mirror clip-X so lane order matches the
  // original (Green leftmost, Orange rightmost) instead of mirrored.
  proj.m[0][0] = -proj.m[0][0];
  D3DMATRIX dv, dp, id; Mat4 ident = Mat4::identity();
  std::memcpy(&dv, &view, 64); std::memcpy(&dp, &proj, 64); std::memcpy(&id, &ident, 64);
  dev_->SetTransform(D3DTS_VIEW, &dv);
  dev_->SetTransform(D3DTS_PROJECTION, &dp);
  dev_->SetTransform(D3DTS_WORLD, &id);

  // Render states: alpha-blended, no depth, painter's order.
  dev_->SetFVF(kFVF);
  dev_->SetRenderState(D3DRS_LIGHTING, FALSE);
  dev_->SetRenderState(D3DRS_ZENABLE, FALSE);
  dev_->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  dev_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  dev_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  dev_->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  dev_->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  dev_->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  dev_->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
  dev_->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  dev_->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  dev_->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

  const float speed = kYPerSecond *
      kTrackSpeed[(difficulty >= 0 && difficulty < 4) ? difficulty : 0];
  auto note_y = [&](double t) {
    return kStrikeY + static_cast<float>(t - song_time) * speed;
  };

  // --- 1) Board surface / rails / lane lines ---
  auto draw_track_surface_quad = [&]() {
    IDirect3DTexture9* board = tex("track_surface.tex");
    const float tile = 18.0f;                       // world units per tile along Y
    const float voff = static_cast<float>(song_time) * speed / tile;
    const float yN = kRemoveY, yF = kTopY;
    const float vN = yN / tile - voff, vF = yF / tile - voff;
    SideRailColorState surface_flash_color;
    const int surface_flash_mult =
        env_enabled("GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_4X") ? 4 :
        env_enabled("GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_3X") ? 3 :
        env_enabled("GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_2X") ? 2 :
        std::clamp(combo_multiplier, 1, 4);
    if (surface_flash_mult >= 4) {
      surface_flash_color = surface_flash_4x_;
    } else if (surface_flash_mult == 3) {
      surface_flash_color = surface_flash_3x_;
    } else if (surface_flash_mult == 2) {
      surface_flash_color = surface_flash_2x_;
    }
    const float surface_flash_strength =
        (env_enabled("GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_2X") ||
         env_enabled("GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_3X") ||
         env_enabled("GHOGX_FORCE_HIGHWAY_SURFACE_FLASH_4X"))
            ? 1.0f
            : std::clamp(surface_flash, 0.0f, 1.0f);
    const D3DCOLOR near_c = multiply_rgb(
        D3DCOLOR_ARGB(255, 120, 120, 130), surface_flash_color,
        surface_flash_strength);
    const D3DCOLOR far_c = multiply_rgb(
        D3DCOLOR_ARGB(255, 6, 6, 9), surface_flash_color,
        surface_flash_strength);
    V3 q[4] = {
        { -kBoardHalfX, yF, kBoardZ, far_c,  0.0f, vF },
        {  kBoardHalfX, yF, kBoardZ, far_c,  1.0f, vF },
        { -kBoardHalfX, yN, kBoardZ, near_c, 0.0f, vN },
        {  kBoardHalfX, yN, kBoardZ, near_c, 1.0f, vN },
    };
    draw_quad(dev_, board, q, false);
  };

  const bool native_track_enabled =
      !env_enabled("GHOGX_DISABLE_HIGHWAY_NATIVE_TRACK");
  const bool side_rail_force_warning =
      env_enabled("GHOGX_FORCE_HIGHWAY_SIDE_RAIL_WARNING") ||
      env_enabled("GHOGX_FORCE_HIGHWAY_SIDE_RAIL_WARNING_STAR");
  const bool side_rail_force_star =
      env_enabled("GHOGX_FORCE_HIGHWAY_SIDE_RAIL_STAR") ||
      env_enabled("GHOGX_FORCE_HIGHWAY_SIDE_RAIL_WARNING_STAR");
  const float side_rail_warning = side_rail_force_warning
      ? 1.0f
      : std::clamp(bad_feedback_flash, 0.0f, 1.0f);
  const bool side_rail_star_active =
      star_power_active || side_rail_force_star ||
      env_enabled("GHOGX_FORCE_HIGHWAY_STARPOWER_GLOW");
  SideRailColorState side_rail_color = side_rails_none_;
  if (side_rail_warning > 0.01f && side_rail_star_active &&
      side_rails_warning_star_.ok) {
    side_rail_color = lerp_side_rail_color(side_rails_none_,
                                           side_rails_warning_star_,
                                           side_rail_warning);
  } else if (side_rail_warning > 0.01f && side_rails_warning_.ok) {
    side_rail_color = lerp_side_rail_color(side_rails_none_,
                                           side_rails_warning_,
                                           side_rail_warning);
  } else if (side_rail_star_active && side_rails_star_.ok) {
    side_rail_color = side_rails_star_;
  }
  if (native_track_enabled && track_surface_mesh_.ok) {
    if (selected_surface_loaded_) {
      draw_track_surface_quad();
    } else {
      draw_runtime_mesh(track_surface_mesh_, 0.0f, 0.0f,
                        D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, false);
    }
    if (track_mask_mesh_.ok &&
        !env_enabled("GHOGX_DISABLE_HIGHWAY_TRACK_MASK")) {
      draw_runtime_mesh(track_mask_mesh_, 0.0f, 0.0f,
                        D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, false);
    }
    if (track_side_rails_mesh_.ok) {
      draw_runtime_mesh(track_side_rails_mesh_, 0.0f, 0.0f,
                        side_rail_d3d_color(side_rail_color), 1.0f, false);
    }
    if (track_lane_lines_mesh_.ok) {
      draw_runtime_mesh(track_lane_lines_mesh_, 0.0f, 0.0f,
                        D3DCOLOR_ARGB(230, 255, 255, 255), 1.0f, false);
    }
  } else {
    draw_track_surface_quad();
  }

  if (!native_track_enabled || !track_lane_lines_mesh_.ok) {
    for (int i = 0; i <= 5; ++i) {
      const float x = (static_cast<float>(i) - 2.5f) * kLaneSpacing;
      const D3DCOLOR nc = D3DCOLOR_ARGB(130, 0, 0, 0), fc = D3DCOLOR_ARGB(15, 0, 0, 0);
      V3 q[4] = {
          { x - 0.10f, kTopY,    kBoardZ + 0.01f, fc, 0,0 },
          { x + 0.10f, kTopY,    kBoardZ + 0.01f, fc, 1,0 },
          { x - 0.10f, kRemoveY, kBoardZ + 0.01f, nc, 0,1 },
          { x + 0.10f, kRemoveY, kBoardZ + 0.01f, nc, 1,1 },
      };
      draw_quad(dev_, nullptr, q);
    }
  }
  dev_->SetTexture(0, nullptr);

  const bool star_power_glow_active =
      (star_power_active ||
       env_enabled("GHOGX_FORCE_HIGHWAY_STARPOWER_GLOW")) &&
      !env_enabled("GHOGX_DISABLE_HIGHWAY_STARPOWER_GLOW");
  const bool bonus_highway_active =
      (star_power_active || env_enabled("GHOGX_FORCE_HIGHWAY_BONUS")) &&
      !env_enabled("GHOGX_DISABLE_HIGHWAY_BONUS");
  if (native_track_enabled && star_power_glow_active &&
      star_power_track_glow_mesh_.ok) {
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    draw_runtime_mesh(star_power_track_glow_mesh_, 0.0f, 0.0f,
                      D3DCOLOR_ARGB(180, 255, 255, 255), 1.0f, true);
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  }

  const bool track_explode_active =
      (bad_feedback_flash > 0.01f ||
       env_enabled("GHOGX_FORCE_HIGHWAY_TRACK_EXPLODE")) &&
      !env_enabled("GHOGX_DISABLE_HIGHWAY_TRACK_EXPLODE");
  if (native_track_enabled && track_explode_active &&
      !track_explode_meshes_.empty()) {
    const float f = env_enabled("GHOGX_FORCE_HIGHWAY_TRACK_EXPLODE")
                        ? 1.0f
                        : std::clamp(bad_feedback_flash, 0.0f, 1.0f);
    const int alpha =
        std::clamp(static_cast<int>(96.0f + f * 159.0f), 0, 255);
    for (const auto& mesh : track_explode_meshes_) {
      draw_runtime_mesh(mesh, 0.0f, 0.0f,
                        D3DCOLOR_ARGB(alpha, 255, 255, 255), 1.0f, true);
    }
  }

  if (difficulty < 0 || difficulty > 3) { dev_->EndScene(); return; }
  const auto& notes = chart.notes[difficulty];
  const float authored_lead = (kTopY - kStrikeY) / speed; // seconds from spawn to strike
  const float lead = std::min(authored_lead, std::max(0.0f, lookahead_sec));
  const float trail = (kStrikeY - kRemoveY) / speed;   // seconds strike to prune

  // --- 3) Beat/subdivision lines (native track.milo timing meshes) ---
  {
    IDirect3DTexture9* bar = tex("barline_gw.tex");
    if ((bar || beat_line_mesh_.ok || half_beat_line_mesh_.ok ||
         quarter_beat_line_mesh_.ok) &&
        chart.ticks_per_beat > 0) {
      const double first_sec = std::max(0.0, song_time - trail);
      const double last_sec = std::max(first_sec, song_time + lead);
      const uint32_t first_tick = chart.sec_to_tick(first_sec);
      const uint32_t last_tick = chart.sec_to_tick(last_sec);
      const uint32_t subdiv = std::max<uint32_t>(1, chart.ticks_per_beat / 4);
      uint32_t line_tick = (first_tick / subdiv) * subdiv;
      for (int b = 0; b < 1024 && line_tick <= last_tick; ++b) {
        const double bt = chart.tick_to_sec(line_tick);
        const float y = note_y(bt);
        if (y >= kRemoveY && y <= kTopY) {
          const uint32_t subdiv_index = line_tick / subdiv;
          const bool downbeat = (subdiv_index % 16u) == 0;
          const bool beat = (subdiv_index % 4u) == 0;
          const bool half = (subdiv_index % 2u) == 0;
          const RuntimeMesh* line_mesh = nullptr;
          float alpha = 40.0f;
          if (downbeat && bar_line_mesh_.ok) {
            line_mesh = &bar_line_mesh_;
            alpha = 120.0f;
          } else if (beat && beat_line_mesh_.ok) {
            line_mesh = &beat_line_mesh_;
            alpha = 92.0f;
          } else if (half && half_beat_line_mesh_.ok) {
            line_mesh = &half_beat_line_mesh_;
            alpha = 62.0f;
          } else if (quarter_beat_line_mesh_.ok) {
            line_mesh = &quarter_beat_line_mesh_;
            alpha = 40.0f;
          }
          const int a = static_cast<int>(alpha * depth_fade(y));
          if (line_mesh) {
            draw_runtime_mesh(*line_mesh, -line_mesh->center_x,
                              y - line_mesh->center_y,
                              D3DCOLOR_ARGB(a, 255, 255, 255));
          } else if (bar && (downbeat || beat)) {
            V3 q[4]; flat_quad(q, 0.0f, y, kBoardZ + 0.02f, kBoardHalfX, 0.5f,
                               D3DCOLOR_ARGB(a, 255, 255, 255));
            draw_quad(dev_, bar, q);
          }
        }
        if (line_tick > UINT32_MAX - subdiv) break;
        line_tick += subdiv;
      }
    }
  }

  // --- 4) Sustain tails (before gems) ---
  if (!env_enabled("GHOGX_DISABLE_HIGHWAY_SUSTAINS")) {
    IDirect3DTexture9* raw_tail = tex("tail2.tex");
    IDirect3DTexture9* held_tail = tex("tail_tight.tex");
    if (!held_tail) held_tail = raw_tail;
    const uint32_t sustain_min = chart.ticks_per_beat / 4;
    static const D3DCOLOR lane_rgb[5] = {
        D3DCOLOR_ARGB(225,60,230,70), D3DCOLOR_ARGB(225,235,60,50),
        D3DCOLOR_ARGB(225,240,210,40), D3DCOLOR_ARGB(225,60,150,235),
        D3DCOLOR_ARGB(225,245,140,30) };
    auto draw_tail_segment = [&](int lane, double on, double off,
                                 const RuntimeMesh* mesh,
                                 IDirect3DTexture9* texture,
                                 float half_width, D3DCOLOR color) {
      if (lane < 0 || lane >= 5) return;
      if (off < song_time - trail) return;
      if (on > song_time + lead) return;
      float y0 = std::max(note_y(on), kStrikeY);     // clamp near to strike
      float y1 = std::min(note_y(off), kTopY);
      if (y1 <= y0) return;
      const float cy = (y0 + y1) * 0.5f, hy = (y1 - y0) * 0.5f;
      if (mesh && mesh->ok) {
        const float mesh_hx =
            std::max(0.001f, (mesh->max_x - mesh->min_x) * 0.5f);
        const float mesh_hy =
            std::max(0.001f, (mesh->max_y - mesh->min_y) * 0.5f);
        draw_centered_runtime_mesh_scaled(
            *mesh, lane_x(lane), cy, color, half_width / mesh_hx,
            hy / mesh_hy, 1.0f);
        return;
      }
      V3 q[4];
      flat_quad(q, lane_x(lane), cy, kGemZ - 0.02f, half_width, hy, color);
      draw_quad(dev_, texture, q);
    };
    for (size_t note_index = 0; note_index < notes.size(); ++note_index) {
      if (consumed_notes && note_index < consumed_notes->size() &&
          (*consumed_notes)[note_index]) {
        continue;
      }
      const auto& n = notes[note_index];
      const double on = chart.tick_to_sec(n.tick_on), off = chart.tick_to_sec(n.tick_off);
      if (off < song_time - trail) continue;
      if (on > song_time + lead) break;
      if (n.tick_off <= n.tick_on + sustain_min) continue;
      const int lane = std::clamp(n.lane, 0, 4);
      const RuntimeMesh* mesh = nullptr;
      if (bonus_highway_active && bonus_tail_mesh_.ok) {
        mesh = &bonus_tail_mesh_;
      } else if (tail_mesh_[lane].ok) {
        mesh = &tail_mesh_[lane];
      }
      draw_tail_segment(lane, on, off, mesh, raw_tail, 0.55f,
                        mesh ? D3DCOLOR_ARGB(225, 255, 255, 255)
                             : lane_rgb[lane]);
    }
    if (active_sustains) {
      for (const auto& sustain : *active_sustains) {
        for (int lane = 0; lane < 5; ++lane) {
          if ((sustain.mask & (1u << lane)) == 0) continue;
          const D3DCOLOR color = sustain.star_power_tail
              ? D3DCOLOR_ARGB(245, 150, 225, 255)
              : D3DCOLOR_ARGB(245, 255, 255, 255);
          const RuntimeMesh* mesh =
              bonus_highway_active && bonus_tail_mesh_.ok
                  ? &bonus_tail_mesh_
                  : sustain.star_power_tail && star_tail_mesh_.ok
                        ? &star_tail_mesh_
                        : held_tail_mesh_.ok ? &held_tail_mesh_
                                             : nullptr;
          if ((sustain.star_power_tail || bonus_highway_active) && mesh) {
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
          }
          draw_tail_segment(lane, sustain.start_time, sustain.end_time, mesh,
                            held_tail, 0.40f, color);
          if ((sustain.star_power_tail || bonus_highway_active) && mesh) {
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
          }
        }
      }
    }
  }

  // --- 5) Fret-target rings at the strikeline (additive) ---
  if (!env_enabled("GHOGX_DISABLE_HIGHWAY_RINGS")) {
    const bool drew_native_smashers =
        !env_enabled("GHOGX_DISABLE_HIGHWAY_NATIVE_SMASHERS") &&
        gem_smasher_mesh_.ok;
    if (drew_native_smashers) {
      for (int lane = 0; lane < 5; ++lane) {
        const bool held = (fret_held_mask >> lane) & 1;
        const float x = lane_x(lane);
        const D3DCOLOR base =
            held ? D3DCOLOR_ARGB(255, 255, 255, 255)
                 : D3DCOLOR_ARGB(215, 205, 205, 205);
        if (smasher_shadow_mesh_.ok) {
          draw_centered_runtime_mesh(smasher_shadow_mesh_, x, kStrikeY,
                                     D3DCOLOR_ARGB(135, 255, 255, 255));
        }
        const std::string& smasher_texture =
            bonus_highway_active && !bonus_smasher_texture_name_.empty()
                ? bonus_smasher_texture_name_
                : smasher_texture_names_[lane];
        if (!smasher_texture.empty()) {
          draw_centered_runtime_mesh_with_texture(gem_smasher_mesh_,
                                                  smasher_texture, x, kStrikeY,
                                                  base);
        } else {
          draw_centered_runtime_mesh(gem_smasher_mesh_, x, kStrikeY, base);
        }
        const std::string& smasher_add_texture =
            bonus_highway_active && !bonus_smasher_add_texture_name_.empty()
                ? bonus_smasher_add_texture_name_
                : smasher_add_texture_names_[lane];
        if (held && !smasher_add_texture.empty()) {
          dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
          draw_centered_runtime_mesh_with_texture(
              gem_smasher_mesh_, smasher_add_texture, x, kStrikeY,
              D3DCOLOR_ARGB(180, 255, 255, 255));
          dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        }
        if (smasher_rim_mesh_.ok) {
          draw_centered_runtime_mesh(smasher_rim_mesh_, x, kStrikeY,
                                     D3DCOLOR_ARGB(235, 255, 255, 255));
        }
      }
    }
    if (!drew_native_smashers) {
      dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
      for (int lane = 0; lane < 5; ++lane) {
        const bool held = (fret_held_mask >> lane) & 1;
        IDirect3DTexture9* ring = tex(now_ring_name(lane));
        if (!ring) ring = tex("now_ring_add.tex");
        const int b = held ? 255 : 150;
        V3 q[4];
        flat_quad(q, lane_x(lane), kStrikeY, kGemZ, kGemHalf * 1.5f,
                  kGemHalf * 1.5f, D3DCOLOR_ARGB(255, b, b, b));
        draw_quad(dev_, ring, q);
      }
      dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    }
  }

  // --- 6) Gems (far -> near) ---
  {
    struct VG { float y; int lane; bool star; bool hopo; };
    std::vector<VG> vis;
    for (size_t note_index = 0; note_index < notes.size(); ++note_index) {
      if (consumed_notes && note_index < consumed_notes->size() &&
          (*consumed_notes)[note_index]) {
        continue;
      }
      const auto& n = notes[note_index];
      const double on = chart.tick_to_sec(n.tick_on);
      if (on < song_time - trail) continue;
      if (on > song_time + lead) break;
      vis.push_back({ note_y(on), std::clamp(n.lane, 0, 4),
                      n.star_power, n.is_hopo });
    }
    std::sort(vis.begin(), vis.end(), [](const VG& a, const VG& b){ return a.y > b.y; });
    IDirect3DTexture9* shadow = tex("gem_shadow.tex");
    for (const auto& g : vis) {
      const int a = static_cast<int>(255 * depth_fade(g.y));
      const float x = lane_x(g.lane);
      if (shadow && !env_enabled("GHOGX_DISABLE_HIGHWAY_GEM_SHADOWS")) {
        V3 s[4]; flat_quad(s, x, g.y, kBoardZ + 0.03f, kGemHalf*1.2f, kGemHalf*1.2f,
                           D3DCOLOR_ARGB(a*3/5, 255, 255, 255));
        draw_quad(dev_, shadow, s);
      }
      if (!env_enabled("GHOGX_DISABLE_HIGHWAY_GEMS")) {
        const D3DCOLOR tint = D3DCOLOR_ARGB(a, 255, 255, 255);
        bool drew_native = false;
        if (bonus_highway_active && bonus_gem_mesh_.ok) {
          draw_centered_runtime_mesh(bonus_gem_mesh_, x, g.y, tint);
          if (bonus_gem_overlay_mesh_.ok) {
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            draw_centered_runtime_mesh(bonus_gem_overlay_mesh_, x, g.y, tint);
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
          }
          if (bonus_spark1_mesh_.ok || bonus_spark2_mesh_.ok) {
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            if (bonus_spark1_mesh_.ok) {
              draw_centered_runtime_mesh(bonus_spark1_mesh_, x, g.y, tint);
            }
            if (bonus_spark2_mesh_.ok) {
              draw_centered_runtime_mesh(bonus_spark2_mesh_, x, g.y, tint);
            }
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
          }
          drew_native = true;
        } else if (g.star && star_mesh_[g.lane].ok) {
          if (star_base_mesh_.ok) {
            draw_centered_runtime_mesh(star_base_mesh_, x, g.y, tint);
          }
          draw_centered_runtime_mesh(star_mesh_[g.lane], x, g.y, tint);
          if (star_top_mesh_[g.lane].ok) {
            draw_centered_runtime_mesh(star_top_mesh_[g.lane], x, g.y, tint);
          }
          if (gem_sparkle_mesh_.ok) {
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            draw_centered_runtime_mesh(gem_sparkle_mesh_, x, g.y, tint);
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
          }
          drew_native = true;
        } else if (g.hopo && hopo_mesh_[g.lane].ok) {
          draw_centered_runtime_mesh(hopo_mesh_[g.lane], x, g.y, tint);
          if (gem_glow_mesh_.ok) {
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            draw_centered_runtime_mesh(gem_glow_mesh_, x, g.y, tint);
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
          }
          drew_native = true;
        } else if (gem_mesh_[g.lane].ok) {
          draw_centered_runtime_mesh(gem_mesh_[g.lane], x, g.y, tint);
          if (gem_glow_mesh_.ok) {
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            draw_centered_runtime_mesh(gem_glow_mesh_, x, g.y, tint);
            dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
          }
          drew_native = true;
        }
        if (!drew_native) {
          IDirect3DTexture9* gt =
              tex(g.star ? "stargem.tex" : gem_tex_name(g.lane));
          if (!gt) gt = tex("gem.tex");
          V3 q[4]; flat_quad(q, x, g.y, kGemZ, kGemHalf, kGemHalf, tint);
          draw_quad(dev_, gt, q);
        }
      }
    }
  }

  // --- 7) Miss feedback (native miss gem at the strikeline) ---
  if (!env_enabled("GHOGX_DISABLE_HIGHWAY_MISS_FLASH") &&
      (miss_flash || env_enabled("GHOGX_FORCE_HIGHWAY_MISS_FLASH"))) {
    const bool force_miss = env_enabled("GHOGX_FORCE_HIGHWAY_MISS_FLASH");
    for (int lane = 0; lane < 5; ++lane) {
      float f = miss_flash ? std::clamp(miss_flash[lane], 0.0f, 1.0f) : 0.0f;
      if (force_miss && lane == 2) f = std::max(f, 1.0f);
      if (f <= 0.01f) continue;
      const int a = static_cast<int>(f * 255.0f);
      const bool forced_lane = force_miss && lane == 2;
      const float scale = forced_lane ? 2.1f : 1.0f + 0.18f * f;
      const float y = kStrikeY + kGemHalf * (forced_lane ? 3.8f : 1.6f);
      if (miss_mesh_.ok) {
        draw_centered_runtime_mesh_scaled(
            miss_mesh_, lane_x(lane), y,
            D3DCOLOR_ARGB(a, 255, 255, 255), scale, scale, scale,
            !forced_lane);
      }
      if (miss_top_mesh_.ok) {
        dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        draw_centered_runtime_mesh_scaled(
            miss_top_mesh_, lane_x(lane), y,
            D3DCOLOR_ARGB(static_cast<int>(a * 0.75f), 255, 255, 255),
            scale, scale, scale, !forced_lane);
        dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
      }
    }
  }

  // --- 8) Hit flames (additive) ---
  if (hit_flash) {
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    IDirect3DTexture9* flame = tex("flame_part.tex");
    const bool force_combo_lightning =
        env_enabled("GHOGX_FORCE_HIGHWAY_COMBO_LIGHTNING");
    const int combo_tier = force_combo_lightning
                               ? 3
                               : std::clamp(combo_multiplier - 1, 0, 3);
    for (int lane = 0; lane < 5; ++lane) {
      const float f = hit_flash[lane];
      if (f <= 0.01f) continue;
      const int a = static_cast<int>(std::min(1.0f, f) * 255);
      if (combo_tier > 0 &&
          !env_enabled("GHOGX_DISABLE_HIGHWAY_COMBO_LIGHTNING")) {
        for (int i = 0; i < combo_tier; ++i) {
          if (!combo_lightning_mesh_[i].ok) continue;
          const float layer_scale =
              1.0f + 0.18f * static_cast<float>(i) +
              0.25f * std::min(1.0f, f);
          const int layer_alpha =
              std::clamp(a - i * 45, 0, 255);
          draw_centered_runtime_mesh_scaled(
              combo_lightning_mesh_[i], lane_x(lane), kStrikeY,
              D3DCOLOR_ARGB(layer_alpha, 255, 255, 255),
              layer_scale, layer_scale, layer_scale);
        }
      }
      const float star_f = star_collect_flash
                               ? std::clamp(star_collect_flash[lane], 0.0f, 1.0f)
                               : 0.0f;
      const RuntimeMesh* flame_mesh =
          bonus_highway_active && bonus_hit_flame_mesh_.ok
              ? &bonus_hit_flame_mesh_
              : star_f > 0.01f && star_collect_flame_mesh_.ok
              ? &star_collect_flame_mesh_
              : hit_flame_mesh_.ok ? &hit_flame_mesh_ : nullptr;
      if (flame_mesh) {
        const float scale = 1.0f + 0.35f * std::min(1.0f, f);
        draw_centered_runtime_mesh_scaled(
            *flame_mesh, lane_x(lane), kStrikeY,
            D3DCOLOR_ARGB(a, 255, 255, 255), scale, scale, scale);
      } else if (flame) {
        const float sz = kGemHalf * (1.5f + 0.9f * f);
        V3 q[4]; flat_quad(q, lane_x(lane), kStrikeY, kGemZ + 0.05f, sz, sz,
                           D3DCOLOR_ARGB(a, 255, 230, 180));
        draw_quad(dev_, flame, q);
      }
    }
    dev_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  }

  dev_->SetTexture(0, nullptr);
  dev_->EndScene();
}

}  // namespace ghogx::game
