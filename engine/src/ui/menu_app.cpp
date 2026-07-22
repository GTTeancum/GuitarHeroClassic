// engine/src/ui/menu_app.cpp -- see menu_app.h.

#include "ui/menu_app.h"

#include "ui/config_db.h"
#include "ui/menu_font.h"
#include "ui/menu_labels.h"
#include "ui/meta_objects.h"
#include "ui/pss_video_player_win32.h"
#include "ui/screen_loader.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include "asset/milo_image.h"
#include "character/char_mesh.h"
#include "character/char_clip.h"
#include "character/char_renderer.h"
#include "game/gameplay.h"
#include "hud/hud_renderer.h"
#include "milo_scene/milo_scene.h"
#include "render/milo_scene_renderer.h"
#include "render/scene_d3d9.h"
#include "render/window_d3d9.h"

#include "dtb.h"
#include "dtb_bridge/dtb_bridge.h"
#include "core/data_node.h"
#include "core/symbol.h"

#include "ark_v3.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d9.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <cstring>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ghogx::ui {

namespace {

using Action = ghogx::render::Window::Action;

struct OverlayVertex {
  float x, y, z, rhw;
  D3DCOLOR color;
  float u, v;
};

constexpr DWORD kOverlayFvf =
    D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

IDirect3DTexture9* upload_overlay_texture(IDirect3DDevice9* dev,
                                          const asset::Image& image) {
  if (!dev || !image.valid()) return nullptr;
  IDirect3DTexture9* texture = nullptr;
  if (FAILED(dev->CreateTexture(static_cast<UINT>(image.width),
                                static_cast<UINT>(image.height), 1, 0,
                                D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture,
                                nullptr))) {
    return nullptr;
  }
  D3DLOCKED_RECT locked{};
  if (FAILED(texture->LockRect(0, &locked, nullptr, 0))) {
    texture->Release();
    return nullptr;
  }
  for (int y = 0; y < image.height; ++y) {
    auto* dst = static_cast<std::uint8_t*>(locked.pBits) + y * locked.Pitch;
    const auto* src = image.rgba.data() +
                      static_cast<std::size_t>(y) * image.width * 4u;
    for (int x = 0; x < image.width; ++x) {
      dst[x * 4 + 0] = src[x * 4 + 2];
      dst[x * 4 + 1] = src[x * 4 + 1];
      dst[x * 4 + 2] = src[x * 4 + 0];
      dst[x * 4 + 3] = src[x * 4 + 3];
    }
  }
  texture->UnlockRect(0);
  return texture;
}

class YouRockOverlay {
 public:
  ~YouRockOverlay() {
    if (atlas_) atlas_->Release();
  }

  bool load(IDirect3DDevice9* dev, const MenuFont& font) {
    if (atlas_) return true;
    font_ = &font;
    atlas_ = upload_overlay_texture(dev, font.atlas());
    return atlas_ != nullptr;
  }

  void draw(IDirect3DDevice9* dev, int width, int height, float phase) {
    if (!dev || !font_ || !atlas_) return;
    float native_width = 0.0f;
    const auto glyphs = font_->layout("YOU ROCK!", &native_width);
    if (glyphs.empty() || native_width <= 0.0f) return;

    const float pulse = 1.0f + 0.035f * std::sin(phase * 8.0f);
    const float target_width = static_cast<float>(width) * 0.68f * pulse;
    const float scale = target_width / native_width;
    const float cap = font_->line_height() * scale;
    const float origin_x = (static_cast<float>(width) - target_width) * 0.5f;
    const float origin_y = static_cast<float>(height) * 0.28f - cap * 0.5f;

    dev->BeginScene();
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    dev->SetFVF(kOverlayFvf);
    dev->SetTexture(0, atlas_);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    for (const auto& glyph : glyphs) {
      const float x0 = origin_x + glyph.x0 * scale;
      const float y0 = origin_y + glyph.y0 * scale;
      const float x1 = origin_x + glyph.x1 * scale;
      const float y1 = origin_y + glyph.y1 * scale;
      const OverlayVertex quad[4] = {
          {x0 - 0.5f, y0 - 0.5f, 0.0f, 1.0f, 0xffffffffu,
           glyph.u0, glyph.v0},
          {x1 - 0.5f, y0 - 0.5f, 0.0f, 1.0f, 0xffffffffu,
           glyph.u1, glyph.v0},
          {x0 - 0.5f, y1 - 0.5f, 0.0f, 1.0f, 0xffffffffu,
           glyph.u0, glyph.v1},
          {x1 - 0.5f, y1 - 0.5f, 0.0f, 1.0f, 0xffffffffu,
           glyph.u1, glyph.v1},
      };
      dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad,
                           sizeof(OverlayVertex));
    }
    dev->SetTexture(0, nullptr);
    dev->EndScene();
  }

 private:
  const MenuFont* font_ = nullptr;
  IDirect3DTexture9* atlas_ = nullptr;
};

std::unordered_set<std::string> compute_disabled(ScreenManager& mgr,
                                                 Object* screen = nullptr);  // fwd
bool node_bool(const DataNode& node);  // fwd
std::vector<Symbol> screen_panel_names(Object* screen);  // fwd
bool cancel_focused_slider(ScreenManager& mgr, Object* panel);  // fwd
std::string panel_file(Object* panel);  // fwd

DataArray one_arg(DataNode n) {
  DataArray a;
  a.push(std::move(n));
  return a;
}

bool string_ends_with(const std::string& value, const char* suffix) {
  const std::size_t suffix_len = std::strlen(suffix);
  return value.size() >= suffix_len &&
         value.compare(value.size() - suffix_len, suffix_len, suffix) == 0;
}

std::string menu_milo_path_for_file(const std::string& file) {
  if (file.empty()) return {};
  if (string_ends_with(file, "_ps2")) return "ui/gen/" + file;
  if (string_ends_with(file, ".milo")) return "ui/gen/" + file + "_ps2";
  return {};
}

std::string milo_leaf_for_panel_file(const std::string& file) {
  if (file.empty()) return {};
  const std::size_t slash = file.find_last_of("/\\");
  const std::string leaf =
      slash == std::string::npos ? file : file.substr(slash + 1);
  if (string_ends_with(leaf, "_ps2")) return leaf;
  if (string_ends_with(leaf, ".milo")) return leaf + "_ps2";
  return {};
}

std::string stock_milo_path_for_panel_file(const gh::ark::ArkV3Reader& ark,
                                           const std::string& file) {
  const std::string ui_path = menu_milo_path_for_file(file);
  if (!ui_path.empty() &&
      (ark.find(ui_path) || ark.find("../../system/run/" + ui_path)))
    return ui_path;

  // In-game UIScreens list the gameplay-owned TrackPanel/HudPanel beside
  // ordinary UI panels. Their authored `file` leaf is still canonical, but it
  // resolves outside ui/gen (track/gen/track.milo_ps2 and
  // hud/gen/hud.milo_ps2 in GH2). Locate the exact shipped leaf rather than
  // counting those panels as missing menu resources.
  const std::string leaf = milo_leaf_for_panel_file(file);
  if (!leaf.empty()) {
    const std::string suffix = "/" + leaf;
    for (const auto& entry : ark.entries()) {
      if (entry.full_path == leaf ||
          (entry.full_path.size() > suffix.size() &&
           entry.full_path.compare(entry.full_path.size() - suffix.size(),
                                   suffix.size(), suffix) == 0))
        return entry.full_path;
    }
  }

  // A directory FilePath such as HudPanel's authored "../hud/" resolves to
  // that subsystem's platform-generated directory, not to one UI MILO.
  std::string relative = file;
  while (relative.rfind("../", 0) == 0) relative.erase(0, 3);
  while (!relative.empty() &&
         (relative.back() == '/' || relative.back() == '\\'))
    relative.pop_back();
  if (!relative.empty() && file.find("../") != std::string::npos) {
    const std::string generated_prefix = relative + "/gen/";
    for (const auto& entry : ark.entries()) {
      if (entry.full_path.rfind(generated_prefix, 0) == 0)
        return generated_prefix;
    }
  }
  return {};
}

std::vector<std::string> stock_dynamic_ui_milo_paths(
    const gh::ark::ArkV3Reader& ark, const std::string& file) {
  std::vector<std::string> out;
  if (!string_ends_with(file, ".milo")) return out;
  const std::string stem = file.substr(0, file.size() - std::strlen(".milo"));
  std::size_t digit = stem.size();
  while (digit > 0 && stem[digit - 1] >= '0' && stem[digit - 1] <= '9') --digit;
  if (digit == stem.size() || stem.substr(digit) != "0") return out;
  const std::string prefix = "ui/gen/" + stem.substr(0, digit);
  for (const auto& entry : ark.entries()) {
    if (entry.full_path.rfind(prefix, 0) != 0 ||
        !string_ends_with(entry.full_path, ".milo_ps2"))
      continue;
    const std::size_t number_begin = prefix.size();
    const std::size_t number_end = entry.full_path.size() -
                                   std::strlen(".milo_ps2");
    if (number_begin >= number_end) continue;
    bool positive_number = entry.full_path[number_begin] != '0';
    for (std::size_t i = number_begin; i < number_end; ++i)
      positive_number = positive_number && entry.full_path[i] >= '0' &&
                        entry.full_path[i] <= '9';
    if (positive_number) out.push_back(entry.full_path);
  }
  std::sort(out.begin(), out.end());
  return out;
}

std::vector<std::string> texture_sources_for_panel(
    const std::string& panel_path,
    const std::unordered_set<std::string>& wanted_textures) {
  std::vector<std::string> sources{panel_path};
  // Stock pause-card panel MILOs keep their shared tile art in this companion
  // RndDir. The panel mats source-reference pl_tile.tex; pause.milo itself only
  // owns the four meshes/mats/buttons.
  if (wanted_textures.find("pl_tile.tex") != wanted_textures.end())
    sources.push_back("ui/gen/pause_lose_tex.milo_ps2");
  return sources;
}

// Append a panel's MILO (its (file) value, e.g. "main.milo" -> ui/gen/main.milo_ps2)
// into the combined scene + texture set the renderer draws.
void add_panel_milo(const std::string& hdr, const std::string& ark,
                    const std::string& file, milo_scene::Scene& combined,
                    std::map<std::string, asset::Image>& textures) {
  if (file.empty()) return;
  // The helpbar panel is rebuilt from its authored tokens/textures in the overlay
  // footer; drawing its raw MILO meshes leaves the source icons at scene origin.
  if (file == "helpbar.milo") return;
  const std::string path = "ui/gen/" + file + "_ps2";  // "main.milo" -> ".milo_ps2"
  milo_scene::Scene s;
  if (!milo_scene::load_scene(hdr, ark, path, s)) return;

  if (s.panel_dir_config_valid && !combined.panel_dir_config_valid) {
    combined.panel_dir_config_valid = true;
    combined.panel_environment = s.panel_environment;
    combined.panel_camera = s.panel_camera;
    combined.panel_enter_event = s.panel_enter_event;
  } else if (s.panel_dir_config_valid &&
             (!s.panel_camera.empty() &&
              s.panel_camera != combined.panel_camera)) {
    std::fprintf(stderr,
                 "[menu] PanelDir camera conflict: keeping %s, panel %s asks "
                 "for %s\n",
                 combined.panel_camera.c_str(), file.c_str(),
                 s.panel_camera.c_str());
  }

  // Collect the diffuse-texture names BEFORE moving the mats out (otherwise the
  // moved-from strings are empty and nothing loads).
  std::unordered_set<std::string> want;
  for (const auto& m : s.mats)
    if (!m.diffuse_tex.empty()) want.insert(m.diffuse_tex);
  for (const MenuMaterialAnim& material_anim :
       extract_menu_material_anims(hdr, ark, path)) {
    for (const MenuMaterialTextureKey& key : material_anim.texture_keys) {
      if (!key.texture.empty()) want.insert(key.texture);
    }
  }

  for (auto& m : s.meshes) combined.meshes.push_back(std::move(m));
  for (auto& mt : s.mats) combined.mats.push_back(std::move(mt));
  for (auto& tr : s.transes) combined.transes.push_back(std::move(tr));
  for (auto& g : s.groups) combined.groups.push_back(std::move(g));
  for (auto& c : s.cams) combined.cams.push_back(std::move(c));
  for (auto& waypoint : s.waypoints)
    combined.waypoints.push_back(std::move(waypoint));
  for (auto& spot : s.spotlights)
    combined.spotlights.push_back(std::move(spot));
  for (auto& light : s.lights) combined.lights.push_back(std::move(light));
  for (auto& environment : s.environs)
    combined.environs.push_back(std::move(environment));
  for (auto& environment_anim : s.env_anims)
    combined.env_anims.push_back(std::move(environment_anim));
  for (auto& screen_mask : s.screen_masks)
    combined.screen_masks.push_back(std::move(screen_mask));
  for (auto& placer : s.band_placers)
    combined.band_placers.push_back(std::move(placer));
  for (auto& particle : s.particles)
    combined.particles.push_back(std::move(particle));
  for (auto& crowd : s.world_crowds)
    combined.world_crowds.push_back(std::move(crowd));
  for (auto& name : s.draw_order) combined.draw_order.push_back(std::move(name));
  for (auto& name : s.grouped_meshes)
    combined.grouped_meshes.push_back(std::move(name));

  std::vector<std::string> names(want.begin(), want.end());
  auto imgs = asset::load_milo_textures_from_sources(
      hdr, ark, texture_sources_for_panel(path, want), names);
  for (auto& kv : imgs) textures.emplace(kv.first, std::move(kv.second));
}

Symbol symbol_value(const DataNode& node) {
  if (Object* obj = node.as_object()) return obj->name();
  if (auto sym = node.as_symbol()) return *sym;
  if (auto text = node.as_string()) return Symbol(*text);
  return Symbol();
}

Object* object_value(const DataNode& node) {
  return node.as_object();
}

int int_value(const DataNode& node, int fallback = 0) {
  if (auto i = node.as_int()) return *i;
  if (auto f = node.as_float()) return static_cast<int>(*f);
  return fallback;
}

Symbol indexed_symbol(const char* stem, int player) {
  return Symbol((std::string(stem) + "_" +
                 std::to_string(std::max(0, player))).c_str());
}

std::string source_milo_for_screen_object(ScreenManager& mgr, Object* screen,
                                          Symbol object_name,
                                          Object* object_ptr = nullptr) {
  if (!object_name.valid() && !object_ptr) return {};
  if (!object_name.valid() && object_ptr) object_name = object_ptr->name();
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    auto* dir = dynamic_cast<ObjectDir*>(panel);
    if (!dir) continue;
    Object* child = dir->find(object_name);
    if (!child) continue;
    if (object_ptr && child != object_ptr) continue;
    const std::string path = menu_milo_path_for_file(panel_file(panel));
    if (!path.empty()) return path;
  }
  return {};
}

void append_menu_rig_scene(milo_scene::Scene& combined, milo_scene::Scene& rig) {
  for (auto& g : rig.groups) combined.groups.push_back(std::move(g));
  for (auto& c : rig.cams) combined.cams.push_back(std::move(c));
  for (auto& l : rig.lights) combined.lights.push_back(std::move(l));
  for (auto& e : rig.environs) combined.environs.push_back(std::move(e));
  for (auto& p : rig.band_placers) combined.band_placers.push_back(std::move(p));
}

void append_scene_lighting(milo_scene::Scene& combined, milo_scene::Scene& source) {
  for (auto& light : source.lights) combined.lights.push_back(std::move(light));
  for (auto& env : source.environs) combined.environs.push_back(std::move(env));
  for (auto& spot : source.spotlights) combined.spotlights.push_back(std::move(spot));
}

void apply_menu_meta_camera(const std::string& hdr, const std::string& ark,
                            ghogx::render::MiloSceneRenderer& renderer) {
  // Menu panels and UIProxy-loaded dirs are drawn in the menu camera. GH2 keeps
  // that camera in metacam.milo; do not auto-frame proxy guitar extents.
  ghogx::render::OrbitCamera& cam = renderer.camera();
  cam.authored = false;
  cam.result_frame = {};
  cam.yaw = 0.0f;
  cam.pitch = 0.0f;
  cam.target[0] = 0.0f;
  cam.target[1] = 0.0f;
  cam.target[2] = 0.0f;
  cam.distance = 768.0f;
  cam.fov = 0.602f;
  cam.near_z = 1.0f;
  cam.far_z = 5000.0f;
  milo_scene::Scene cam_scene;
  if (milo_scene::load_scene(hdr, ark, "ui/gen/metacam.milo_ps2", cam_scene)) {
    for (const auto& c : cam_scene.cams) {
      if (!c.decoded || c.name != "meta.cam") continue;
      cam.target[0] = c.local.pos[0];
      cam.target[2] = c.local.pos[2];
      cam.distance = std::max(1.0f, std::fabs(c.local.pos[1]));
      if (c.fov > 0.05f) cam.fov = c.fov;
      std::fprintf(stderr, "[menu] meta.cam eye=(%.1f %.1f %.1f) fov=%.3f\n",
                   c.local.pos[0], c.local.pos[1], c.local.pos[2], cam.fov);
      break;
    }
  }
}

std::unordered_set<std::string> diffuse_texture_names(
    const milo_scene::Scene& scene) {
  std::unordered_set<std::string> want;
  for (const auto& m : scene.mats)
    if (!m.diffuse_tex.empty()) want.insert(m.diffuse_tex);
  return want;
}

void load_scene_textures(const std::string& hdr, const std::string& ark,
                         const std::string& milo_path,
                         const milo_scene::Scene& scene,
                         std::map<std::string, asset::Image>& textures) {
  std::unordered_set<std::string> want = diffuse_texture_names(scene);
  std::vector<std::string> names(want.begin(), want.end());
  auto imgs = asset::load_milo_textures_from_sources(hdr, ark, {milo_path}, names);
  for (auto& kv : imgs) textures.emplace(kv.first, std::move(kv.second));
}

bool panel_showing(Object* panel) {
  if (!panel) return false;
  if (panel->has_property(Symbol("showing")) &&
      !node_bool(panel->get_property(Symbol("showing"))))
    return false;
  // MultiSelectPanel::active is the stock native visibility gate. Outfit
  // panels enter inactive and must not contribute either their MILO drawables
  // or labels until multi_char_selected activates that player's panel.
  if (panel->has_property(Symbol("active")) &&
      !node_bool(panel->get_property(Symbol("active"))))
    return false;
  return true;
}

struct GuitarDisplayAttachTarget {
  std::string view;
  std::string placer;
  std::string parent() const { return placer.empty() ? view : placer; }
};

struct GuitarDisplayRuntimeAnim {
  std::string target;
  ghogx::render::MiloSceneRenderer::MeshTransformAnim anim;
  float frames_per_second = 30.0f;
};

ghogx::render::MiloSceneRenderer::MeshTransformAnim to_renderer_anim(
    const MenuSliderAnim& source);

GuitarDisplayAttachTarget guitar_display_attach_target(Symbol panel_name,
                                                       int player) {
  if (panel_name == Symbol("store_guitar_display_panel"))
    return {"guitar_store.view", "guitar_store.placer"};
  if (panel_name == Symbol("multi_guitar_display_panel")) {
    if (player == 1) return {"guitar_p2.view", "guitar_multi1.placer"};
    return {"guitar_p1.view", "guitar_multi0.placer"};
  }
  if (panel_name == Symbol("unlock_guitar_display_panel"))
    return {"guitar_axe.view", "guitar_axe.placer"};
  return {"guitar_single.view", ""};
}

std::string guitar_display_filter_name(Symbol panel_name, int player) {
  if (panel_name == Symbol("store_guitar_display_panel"))
    return "guitar_store.filt";
  if (panel_name == Symbol("multi_guitar_display_panel"))
    return player == 1 ? "guitar_p2.filt" : "guitar_p1.filt";
  if (panel_name == Symbol("unlock_guitar_display_panel"))
    return "guitar_axe.filt";
  return {};
}

bool guitar_display_uses_live_proxy(Symbol panel_name) {
  // ihatecompvir's UIProxy::SyncDir copies the proxy's WorldXfm into the
  // loaded directory/main transform. Stock menu scripts pass live UIProxy
  // objects to show_guitar when they want screen-authored placement.
  //
  // GH2's single, multiplayer, store, and unlock guitar screens pass
  // screen-local UIProxy/AnimFilter pairs whose transforms are authored beside
  // their cases/award posters. The shared guitar_display.milo placers are only
  // a fallback when a script does not provide live proxy objects.
  return panel_name == Symbol("guitar_display_panel") ||
         panel_name == Symbol("multi_guitar_display_panel") ||
         panel_name == Symbol("store_guitar_display_panel") ||
         panel_name == Symbol("unlock_guitar_display_panel");
}

DataNode guitar_display_panel_value(Object* panel, const char* stem, int player) {
  if (!panel) return DataNode();
  DataNode indexed = panel->get_property(indexed_symbol(stem, player));
  if (!indexed.empty()) return indexed;
  return panel->get_property(Symbol(stem));
}

std::vector<int> guitar_display_active_players(Object* panel) {
  std::vector<int> players;
  if (!panel) return players;
  auto add_indexed = [&](int player) {
    if (player < 0 ||
        std::find(players.begin(), players.end(), player) != players.end()) {
      return;
    }
    if (symbol_value(panel->get_property(indexed_symbol("guitar", player)))
            .valid()) {
      players.push_back(player);
    }
  };
  // GH2's stock local menu scripts address player slots 0 and 1 explicitly.
  add_indexed(0);
  add_indexed(1);
  if (!players.empty()) return players;

  const int active =
      std::clamp(int_value(panel->get_property(Symbol("guitar_display_player")),
                           0),
                 0, 1);
  if (symbol_value(guitar_display_panel_value(panel, "guitar", active)).valid())
    players.push_back(active);
  return players;
}

std::array<float, 4> normalize_quat_xyzw(std::array<float, 4> q) {
  const float len = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] +
                              q[3] * q[3]);
  if (len <= 0.000001f || !std::isfinite(len)) return {0, 0, 0, 1};
  for (float& v : q) v /= len;
  return q;
}

std::array<float, 4> slerp_quat_xyzw(std::array<float, 4> a,
                                     std::array<float, 4> b, float t) {
  a = normalize_quat_xyzw(a);
  b = normalize_quat_xyzw(b);
  float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
  if (dot < 0.0f) {
    for (float& v : b) v = -v;
    dot = -dot;
  }
  if (dot > 0.9995f) {
    return normalize_quat_xyzw({a[0] + (b[0] - a[0]) * t,
                                a[1] + (b[1] - a[1]) * t,
                                a[2] + (b[2] - a[2]) * t,
                                a[3] + (b[3] - a[3]) * t});
  }
  dot = std::clamp(dot, -1.0f, 1.0f);
  const float theta0 = std::acos(dot);
  const float theta = theta0 * t;
  const float sin_theta = std::sin(theta);
  const float sin_theta0 = std::sin(theta0);
  const float s0 = std::cos(theta) - dot * sin_theta / sin_theta0;
  const float s1 = sin_theta / sin_theta0;
  return normalize_quat_xyzw({a[0] * s0 + b[0] * s1,
                              a[1] * s0 + b[1] * s1,
                              a[2] * s0 + b[2] * s1,
                              a[3] * s0 + b[3] * s1});
}

std::array<float, 4> mul_quat_xyzw(const std::array<float, 4>& a,
                                   const std::array<float, 4>& b) {
  const float ax = a[0], ay = a[1], az = a[2], aw = a[3];
  const float bx = b[0], by = b[1], bz = b[2], bw = b[3];
  return normalize_quat_xyzw({
      aw * bx + ax * bw + ay * bz - az * by,
      aw * by - ax * bz + ay * bw + az * bx,
      aw * bz + ax * by - ay * bx + az * bw,
      aw * bw - ax * bx - ay * by - az * bz,
  });
}

std::array<float, 4> conjugate_quat_xyzw(std::array<float, 4> q) {
  q = normalize_quat_xyzw(q);
  q[0] = -q[0];
  q[1] = -q[1];
  q[2] = -q[2];
  return q;
}

std::array<float, 4> sample_menu_quat_value(
    const std::vector<MenuTransQuatKey>& keys, float frame) {
  if (keys.empty()) return {0, 0, 0, 1};
  const auto* a = &keys.front();
  const auto* b = &keys.back();
  for (std::size_t i = 1; i < keys.size(); ++i) {
    if (frame <= keys[i].frame) {
      a = &keys[i - 1];
      b = &keys[i];
      break;
    }
  }
  const float span = std::max(b->frame - a->frame, 0.001f);
  const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  const std::array<float, 4> qa = a->quat_xyzw;
  const std::array<float, 4> qb = b->quat_xyzw;
  return slerp_quat_xyzw(qa, qb, t);
}

std::array<float, 3> sample_menu_vec_value(
    const std::vector<MenuTransVecKey>& keys, float frame) {
  if (keys.empty()) return {0.0f, 0.0f, 0.0f};
  const auto* a = &keys.front();
  const auto* b = &keys.back();
  for (std::size_t i = 1; i < keys.size(); ++i) {
    if (frame <= keys[i].frame) {
      a = &keys[i - 1];
      b = &keys[i];
      break;
    }
  }
  const float span = std::max(b->frame - a->frame, 0.001f);
  const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  std::array<float, 3> cur{};
  for (int i = 0; i < 3; ++i)
    cur[i] = a->value[i] + (b->value[i] - a->value[i]) * t;
  return cur;
}

void quat_xyzw_to_row_rot(const std::array<float, 4>& quat_xyzw,
                          float rot[3][3]) {
  const auto q = normalize_quat_xyzw(quat_xyzw);
  const float x = q[0], y = q[1], z = q[2], w = q[3];
  rot[0][0] = 1.0f - 2.0f * (y * y + z * z);
  rot[0][1] = 2.0f * (x * y + z * w);
  rot[0][2] = 2.0f * (x * z - y * w);
  rot[1][0] = 2.0f * (x * y - z * w);
  rot[1][1] = 1.0f - 2.0f * (x * x + z * z);
  rot[1][2] = 2.0f * (y * z + x * w);
  rot[2][0] = 2.0f * (x * z + y * w);
  rot[2][1] = 2.0f * (y * z - x * w);
  rot[2][2] = 1.0f - 2.0f * (x * x + y * y);
}

std::array<float, 3> xfm_row_scales(const milo_scene::Xfm& xfm);

std::array<float, 4> row_rot_to_quat_xyzw(const milo_scene::Xfm& xfm) {
  const auto scale = xfm_row_scales(xfm);
  float m[3][3]{};
  for (int r = 0; r < 3; ++r) {
    const float divisor = std::fabs(scale[r]) > 0.000001f ? scale[r] : 1.0f;
    for (int c = 0; c < 3; ++c) m[c][r] = xfm.rot[r][c] / divisor;
  }
  std::array<float, 4> q{};
  const float trace = m[0][0] + m[1][1] + m[2][2];
  if (trace > 0.0f) {
    const float s = std::sqrt(trace + 1.0f) * 2.0f;
    q = {(m[2][1] - m[1][2]) / s, (m[0][2] - m[2][0]) / s,
         (m[1][0] - m[0][1]) / s, 0.25f * s};
  } else if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
    const float s = std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
    q = {0.25f * s, (m[0][1] + m[1][0]) / s,
         (m[0][2] + m[2][0]) / s, (m[2][1] - m[1][2]) / s};
  } else if (m[1][1] > m[2][2]) {
    const float s = std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
    q = {(m[0][1] + m[1][0]) / s, 0.25f * s,
         (m[1][2] + m[2][1]) / s, (m[0][2] - m[2][0]) / s};
  } else {
    const float s = std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
    q = {(m[0][2] + m[2][0]) / s, (m[1][2] + m[2][1]) / s,
         0.25f * s, (m[1][0] - m[0][1]) / s};
  }
  return normalize_quat_xyzw(q);
}

std::array<float, 3> xfm_row_scales(const milo_scene::Xfm& xfm) {
  std::array<float, 3> scale = {
      std::sqrt(xfm.rot[0][0] * xfm.rot[0][0] +
                xfm.rot[0][1] * xfm.rot[0][1] +
                xfm.rot[0][2] * xfm.rot[0][2]),
      std::sqrt(xfm.rot[1][0] * xfm.rot[1][0] +
                xfm.rot[1][1] * xfm.rot[1][1] +
                xfm.rot[1][2] * xfm.rot[1][2]),
      std::sqrt(xfm.rot[2][0] * xfm.rot[2][0] +
                xfm.rot[2][1] * xfm.rot[2][1] +
                xfm.rot[2][2] * xfm.rot[2][2]),
  };
  const float cross01[3] = {
      xfm.rot[0][1] * xfm.rot[1][2] -
          xfm.rot[0][2] * xfm.rot[1][1],
      xfm.rot[0][2] * xfm.rot[1][0] -
          xfm.rot[0][0] * xfm.rot[1][2],
      xfm.rot[0][0] * xfm.rot[1][1] -
          xfm.rot[0][1] * xfm.rot[1][0],
  };
  const float det_sign = cross01[0] * xfm.rot[2][0] +
                         cross01[1] * xfm.rot[2][1] +
                         cross01[2] * xfm.rot[2][2];
  if (det_sign < 0.0f) scale[2] = -scale[2];
  return scale;
}

void apply_local_rotation_absolute(milo_scene::Xfm& xfm,
                                   const std::array<float, 4>& quat_xyzw) {
  float rot[3][3];
  quat_xyzw_to_row_rot(quat_xyzw, rot);
  const std::array<float, 3> scale = xfm_row_scales(xfm);
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      xfm.rot[r][c] = rot[r][c] * scale[r];
    }
  }
}

void apply_local_translation_absolute(milo_scene::Xfm& xfm,
                                      const std::array<float, 3>& value) {
  for (int i = 0; i < 3; ++i) xfm.pos[i] = value[i];
}

void apply_local_scale_absolute(milo_scene::Xfm& xfm,
                                const std::array<float, 3>& value) {
  for (int r = 0; r < 3; ++r) {
    float len = std::sqrt(xfm.rot[r][0] * xfm.rot[r][0] +
                          xfm.rot[r][1] * xfm.rot[r][1] +
                          xfm.rot[r][2] * xfm.rot[r][2]);
    if (len <= 0.000001f || !std::isfinite(len)) {
      xfm.rot[r][0] = xfm.rot[r][1] = xfm.rot[r][2] = 0.0f;
      xfm.rot[r][r] = value[r];
      continue;
    }
    for (int c = 0; c < 3; ++c) xfm.rot[r][c] = xfm.rot[r][c] / len * value[r];
  }
}

float anim_filter_source_frame(const MenuAnimFilter& filter) {
  float scale = filter.scale;
  if (filter.end < filter.start) scale = -std::fabs(scale);
  const float frame_offset =
      filter.offset + (filter.end < filter.start ? filter.start - filter.end
                                                  : 0.0f);
  float frame = filter.frame * scale + frame_offset;
  const float lo = std::min(filter.start, filter.end);
  const float hi = std::max(filter.start, filter.end);
  const float span = hi - lo;
  if (span > 0.0001f) {
    if (filter.type == 1) {
      while (frame < lo) frame += span;
      while (frame >= hi) frame -= span;
    } else if (filter.type == 2) {
      const float shuttle_span = span * 2.0f;
      while (frame < lo) frame += shuttle_span;
      while (frame > lo + shuttle_span) frame -= shuttle_span;
      if (frame > hi) frame = hi - (frame - hi);
    } else {
      frame = std::clamp(frame, lo, hi);
    }
  }
  return frame;
}

milo_scene::Xfm xfm_from_menu_matrix(const std::array<float, 12>& m) {
  milo_scene::Xfm out;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) out.rot[r][c] = m[r * 3 + c];
  out.pos[0] = m[9];
  out.pos[1] = m[10];
  out.pos[2] = m[11];
  return out;
}

std::array<float, 16> mat4_from_xfm(const milo_scene::Xfm& x) {
  return {x.rot[0][0], x.rot[0][1], x.rot[0][2], 0.0f,
          x.rot[1][0], x.rot[1][1], x.rot[1][2], 0.0f,
          x.rot[2][0], x.rot[2][1], x.rot[2][2], 0.0f,
          x.pos[0],    x.pos[1],    x.pos[2],    1.0f};
}

milo_scene::Xfm xfm_from_mat4(const std::array<float, 16>& m) {
  milo_scene::Xfm out;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) out.rot[r][c] = m[r * 4 + c];
  out.pos[0] = m[12];
  out.pos[1] = m[13];
  out.pos[2] = m[14];
  return out;
}

std::array<float, 16> mul_mat4(const std::array<float, 16>& a,
                               const std::array<float, 16>& b) {
  std::array<float, 16> out{};
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      float v = 0.0f;
      for (int k = 0; k < 4; ++k) v += a[r * 4 + k] * b[k * 4 + c];
      out[r * 4 + c] = v;
    }
  }
  return out;
}

std::array<float, 16> apply_transform_constraint(
    const std::array<float, 16>& local,
    const std::array<float, 16>& parent_world,
    std::uint32_t constraint) {
  constexpr std::uint32_t kConstraintLocalRotate = 1;
  constexpr std::uint32_t kConstraintParentWorld = 2;
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
  return mul_mat4(local, parent_world);
}

std::array<float, 16> identity_mat4() {
  return {1.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 1.0f, 0.0f,
          0.0f, 0.0f, 0.0f, 1.0f};
}

void transform_menu_point(const std::array<float, 16>& m, float& x, float& y,
                          float& z) {
  const float ox = x;
  const float oy = y;
  const float oz = z;
  x = ox * m[0] + oy * m[4] + oz * m[8] + m[12];
  y = ox * m[1] + oy * m[5] + oz * m[9] + m[13];
  z = ox * m[2] + oy * m[6] + oz * m[10] + m[14];
}

void transform_menu_normal(const std::array<float, 16>& m, float& x, float& y,
                           float& z) {
  const float ox = x;
  const float oy = y;
  const float oz = z;
  x = ox * m[0] + oy * m[4] + oz * m[8];
  y = ox * m[1] + oy * m[5] + oz * m[9];
  z = ox * m[2] + oy * m[6] + oz * m[10];
  const float len = std::sqrt(x * x + y * y + z * z);
  if (len > 0.000001f && std::isfinite(len)) {
    x /= len;
    y /= len;
    z /= len;
  }
}

struct SceneTransformNode {
  milo_scene::Xfm local;
  milo_scene::Xfm world_stored;
  std::string parent;
  std::uint32_t constraint = 0;
};

bool find_scene_transform_node(const milo_scene::Scene& scene,
                               const std::string& name,
                               SceneTransformNode& out) {
  for (const auto& trans : scene.transes) {
    if (trans.name != name) continue;
    out.local = trans.local;
    out.world_stored = trans.world_stored;
    out.parent = trans.parent;
    out.constraint = trans.constraint;
    return true;
  }
  for (const auto& group : scene.groups) {
    if (group.name != name || !group.has_transform) continue;
    out.local = group.local;
    out.world_stored = group.world_stored;
    out.parent = group.parent;
    out.constraint = group.constraint;
    return true;
  }
  for (const auto& mesh : scene.meshes) {
    if (mesh.name != name || !mesh.decoded) continue;
    out.local = mesh.local;
    out.world_stored = mesh.world_stored;
    out.parent = mesh.parent;
    out.constraint = mesh.constraint;
    return true;
  }
  for (const auto& placer : scene.band_placers) {
    if (placer.name != name || !placer.decoded) continue;
    out.local = placer.local;
    out.world_stored = placer.world_stored;
    out.parent = placer.parent;
    out.constraint = 0;
    return true;
  }
  return false;
}

bool scene_world_for_name(const milo_scene::Scene& scene,
                          const std::string& name,
                          std::array<float, 16>& world,
                          int guard = 0) {
  if (guard >= 64) return false;
  SceneTransformNode node;
  if (!find_scene_transform_node(scene, name, node)) return false;
  const std::array<float, 16> local = mat4_from_xfm(node.local);
  if (node.parent.empty()) {
    world = local;
    return true;
  }
  std::array<float, 16> parent_world{};
  if (!scene_world_for_name(scene, node.parent, parent_world, guard + 1)) {
    world = mat4_from_xfm(node.world_stored);
    return true;
  }
  world = apply_transform_constraint(local, parent_world, node.constraint);
  return true;
}

bool compose_proxy_world(const milo_scene::Scene& source_scene,
                         const MenuProxyTransform& proxy,
                         const milo_scene::Xfm& local,
                         milo_scene::Xfm& world) {
  if (proxy.parent.empty()) {
    world = local;
    return true;
  }
  std::array<float, 16> parent_world{};
  if (!scene_world_for_name(source_scene, proxy.parent, parent_world)) {
    world = xfm_from_menu_matrix(proxy.world);
    return false;
  }
  const std::array<float, 16> composed =
      apply_transform_constraint(mat4_from_xfm(local), parent_world,
                                 proxy.constraint);
  world = xfm_from_mat4(composed);
  return true;
}

void append_proxy_group(milo_scene::Scene& scene,
                        const std::string& name,
                        const milo_scene::Xfm& world) {
  for (const auto& group : scene.groups)
    if (group.name == name) return;
  milo_scene::GroupObj group;
  group.name = name;
  group.local = world;
  group.world_stored = world;
  group.parent.clear();
  group.has_transform = true;
  group.decoded = true;
  group.source_order_decoded = true;
  group.showing = true;
  scene.groups.push_back(std::move(group));
}

bool scene_has_group(const milo_scene::Scene& scene, const std::string& name) {
  for (const auto& group : scene.groups)
    if (group.name == name) return true;
  return false;
}

bool scene_has_trans(const milo_scene::Scene& scene, const std::string& name) {
  for (const auto& trans : scene.transes)
    if (trans.name == name) return true;
  return false;
}

void append_proxy_transform_context(milo_scene::Scene& combined,
                                    milo_scene::Scene& source) {
  for (auto& group : source.groups)
    if (!scene_has_group(combined, group.name))
      combined.groups.push_back(std::move(group));
  for (auto& trans : source.transes)
    if (!scene_has_trans(combined, trans.name))
      combined.transes.push_back(std::move(trans));
}

void append_proxy_transform_node(milo_scene::Scene& scene,
                                 const MenuProxyTransform& proxy,
                                 const milo_scene::Xfm& local,
                                 const milo_scene::Xfm& world) {
  if (proxy.name.empty() || scene_has_group(scene, proxy.name)) return;
  milo_scene::GroupObj group;
  group.name = proxy.name;
  group.local = local;
  group.world_stored = world;
  group.parent = proxy.parent;
  group.constraint = proxy.constraint;
  group.has_transform = true;
  group.decoded = true;
  group.source_order_decoded = true;
  group.showing = true;
  scene.groups.push_back(std::move(group));
}

bool apply_guitar_proxy_main_trans(milo_scene::Scene& scene,
                                   const std::string& main_trans,
                                   const milo_scene::Xfm& world) {
  if (main_trans.empty()) return false;
  for (auto& mesh : scene.meshes) {
    if (mesh.name != main_trans || !mesh.decoded) continue;
    // Stock ui_objects.dta declares UIProxy/guitar main_trans "guitar.mesh".
    // UIProxy::SyncDir calls SetWorldXfm on that transform. Harmonix keeps the
    // authored local/parent chain and only updates the cached world matrix.
    mesh.world_stored = world;
    mesh.world_xfm_override = true;
    return true;
  }
  for (auto& trans : scene.transes) {
    if (trans.name != main_trans) continue;
    trans.world_stored = world;
    trans.world_xfm_override = true;
    return true;
  }
  for (auto& group : scene.groups) {
    if (group.name != main_trans || !group.has_transform) continue;
    group.world_stored = world;
    group.world_xfm_override = true;
    return true;
  }
  return false;
}

void log_guitar_scene_hierarchy(const milo_scene::Scene& scene,
                                const char* tag) {
  if (!std::getenv("GHOGX_LOG_GUITAR_HIERARCHY")) return;
  auto log_rows = [](const char* kind, const char* tag,
                     const std::string& name, const milo_scene::Xfm& xfm) {
    std::fprintf(stderr,
                 "[menu] guitar rows %s %s=%s row0=(%.6f %.6f %.6f) "
                 "row1=(%.6f %.6f %.6f) row2=(%.6f %.6f %.6f)\n",
                 tag, kind, name.c_str(), xfm.rot[0][0], xfm.rot[0][1],
                 xfm.rot[0][2], xfm.rot[1][0], xfm.rot[1][1],
                 xfm.rot[1][2], xfm.rot[2][0], xfm.rot[2][1],
                 xfm.rot[2][2]);
  };
  for (const auto& mesh : scene.meshes) {
    std::fprintf(stderr,
                 "[menu] guitar hierarchy %s mesh=%s parent=%s local=(%.3f %.3f %.3f) world=(%.3f %.3f %.3f) override=%d material=%s\n",
                 tag, mesh.name.c_str(),
                 mesh.parent.empty() ? "<none>" : mesh.parent.c_str(),
                 mesh.local.pos[0], mesh.local.pos[1], mesh.local.pos[2],
                 mesh.world_stored.pos[0], mesh.world_stored.pos[1],
                 mesh.world_stored.pos[2], mesh.world_xfm_override ? 1 : 0,
                 mesh.material.c_str());
    if (mesh.name == "guitar.mesh" || mesh.name == "guitar_strings.mesh")
      log_rows("mesh_world", tag, mesh.name, mesh.world_stored);
  }
  for (const auto& trans : scene.transes) {
    std::fprintf(stderr,
                 "[menu] guitar hierarchy %s trans=%s parent=%s local=(%.3f %.3f %.3f) world=(%.3f %.3f %.3f) override=%d\n",
                 tag, trans.name.c_str(),
                 trans.parent.empty() ? "<none>" : trans.parent.c_str(),
                 trans.local.pos[0], trans.local.pos[1], trans.local.pos[2],
                 trans.world_stored.pos[0], trans.world_stored.pos[1],
                 trans.world_stored.pos[2],
                 trans.world_xfm_override ? 1 : 0);
  }
  for (const auto& group : scene.groups) {
    std::fprintf(stderr,
                 "[menu] guitar hierarchy %s group=%s parent=%s local=(%.3f %.3f %.3f) world=(%.3f %.3f %.3f) override=%d has_transform=%d children=%zu\n",
                 tag, group.name.c_str(),
                 group.parent.empty() ? "<none>" : group.parent.c_str(),
                 group.local.pos[0], group.local.pos[1], group.local.pos[2],
                 group.world_stored.pos[0], group.world_stored.pos[1],
                 group.world_stored.pos[2],
                 group.world_xfm_override ? 1 : 0,
                 group.has_transform ? 1 : 0, group.children.size());
  }
}

bool apply_guitar_display_filter_to_target(const std::string& hdr,
                                           const std::string& ark,
                                           const std::string& milo_path,
                                           const std::string& filter_name,
                                           const std::string& fallback_target,
                                           milo_scene::Scene& scene) {
  if (milo_path.empty() || filter_name.empty() || fallback_target.empty())
    return false;
  const MenuAnimFilter filter =
      extract_menu_anim_filter(hdr, ark, milo_path, filter_name);
  if (!filter.valid) return false;
  const MenuSliderAnim anim =
      extract_menu_slider_anim(hdr, ark, milo_path, filter.trans_anim);
  if (!anim.valid ||
      (anim.rotation_keys.empty() && anim.translation_keys.empty() &&
       anim.scale_keys.empty())) {
    return false;
  }
  const std::string target = anim.target.empty() ? fallback_target : anim.target;
  float source_frame = anim_filter_source_frame(filter);
  if (const char* diagnostic_frame =
          std::getenv("GHOGX_GUITAR_FILTER_SOURCE_FRAME")) {
    const float requested = static_cast<float>(std::atof(diagnostic_frame));
    if (std::isfinite(requested)) source_frame = requested;
  }
  const auto rot_value = sample_menu_quat_value(anim.rotation_keys, source_frame);
  const auto pos_value = sample_menu_vec_value(anim.translation_keys, source_frame);
  const auto scale_value = sample_menu_vec_value(anim.scale_keys, source_frame);
  for (auto& placer : scene.band_placers) {
    if (placer.name != target || !placer.decoded) continue;
    if (!anim.rotation_keys.empty())
      apply_local_rotation_absolute(placer.local, rot_value);
    if (!anim.translation_keys.empty())
      apply_local_translation_absolute(placer.local, pos_value);
    if (!anim.scale_keys.empty())
      apply_local_scale_absolute(placer.local, scale_value);
    if (std::getenv("GHOGX_LOG_GUITAR_FILTER")) {
      std::fprintf(stderr,
                   "[menu] guitar filter apply %s/%s -> %s frame=%.2f source_frame=%.2f target=%s\n",
                   milo_path.c_str(), filter_name.c_str(),
                   filter.trans_anim.c_str(), filter.frame, source_frame,
                   target.c_str());
    }
    return true;
  }
  for (auto& group : scene.groups) {
    if (group.name != target || !group.has_transform) continue;
    if (!anim.rotation_keys.empty()) {
      apply_local_rotation_absolute(group.local, rot_value);
      apply_local_rotation_absolute(group.world_stored, rot_value);
    }
    if (!anim.translation_keys.empty()) {
      apply_local_translation_absolute(group.local, pos_value);
      apply_local_translation_absolute(group.world_stored, pos_value);
    }
    if (!anim.scale_keys.empty()) {
      apply_local_scale_absolute(group.local, scale_value);
      apply_local_scale_absolute(group.world_stored, scale_value);
    }
    if (std::getenv("GHOGX_LOG_GUITAR_FILTER")) {
      std::fprintf(stderr,
                   "[menu] guitar filter apply %s/%s -> %s frame=%.2f source_frame=%.2f target=%s\n",
                   milo_path.c_str(), filter_name.c_str(),
                   filter.trans_anim.c_str(), filter.frame, source_frame,
                   target.c_str());
    }
    return true;
  }
  return false;
}

bool apply_guitar_display_filter_to_xfm(const std::string& hdr,
                                        const std::string& ark,
                                        const std::string& milo_path,
                                        const std::string& filter_name,
                                        const std::string& fallback_target,
                                        milo_scene::Xfm& xfm) {
  if (milo_path.empty() || filter_name.empty() || fallback_target.empty())
    return false;
  const MenuAnimFilter filter =
      extract_menu_anim_filter(hdr, ark, milo_path, filter_name);
  if (!filter.valid) return false;
  const MenuSliderAnim anim =
      extract_menu_slider_anim(hdr, ark, milo_path, filter.trans_anim);
  if (!anim.valid ||
      (anim.rotation_keys.empty() && anim.translation_keys.empty() &&
       anim.scale_keys.empty())) {
    return false;
  }
  const std::string target = anim.target.empty() ? fallback_target : anim.target;
  if (target != fallback_target) return false;
  float source_frame = anim_filter_source_frame(filter);
  if (const char* diagnostic_frame =
          std::getenv("GHOGX_GUITAR_FILTER_SOURCE_FRAME")) {
    const float requested = static_cast<float>(std::atof(diagnostic_frame));
    if (std::isfinite(requested)) source_frame = requested;
  }
  if (!anim.rotation_keys.empty())
    apply_local_rotation_absolute(
        xfm, sample_menu_quat_value(anim.rotation_keys, source_frame));
  if (!anim.translation_keys.empty())
    apply_local_translation_absolute(
        xfm, sample_menu_vec_value(anim.translation_keys, source_frame));
  if (!anim.scale_keys.empty())
    apply_local_scale_absolute(
        xfm, sample_menu_vec_value(anim.scale_keys, source_frame));
  if (std::getenv("GHOGX_LOG_GUITAR_FILTER")) {
    std::fprintf(stderr,
                 "[menu] guitar dir filter apply %s/%s -> %s frame=%.2f source_frame=%.2f target=%s\n",
                 milo_path.c_str(), filter_name.c_str(),
                 filter.trans_anim.c_str(), filter.frame, source_frame,
                 target.c_str());
  }
  return true;
}

void parent_root_nodes_to_display(milo_scene::Scene& scene,
                                  const std::string& display_parent) {
  std::string root_parent = display_parent;
  if (!scene.dir_name.empty()) {
    bool has_dir_root = false;
    for (const auto& group : scene.groups)
      if (group.name == scene.dir_name) has_dir_root = true;
    for (const auto& trans : scene.transes)
      if (trans.name == scene.dir_name) has_dir_root = true;
    for (const auto& mesh : scene.meshes)
      if (mesh.name == scene.dir_name) has_dir_root = true;
    if (!has_dir_root) {
      milo_scene::GroupObj dir_root;
      dir_root.name = scene.dir_name;
      dir_root.parent = display_parent;
      dir_root.has_transform = true;
      dir_root.decoded = true;
      dir_root.source_order_decoded = true;
      dir_root.showing = true;
      scene.groups.push_back(std::move(dir_root));
      root_parent = scene.dir_name;
    }
  }
  for (auto& mesh : scene.meshes)
    if (mesh.parent.empty()) mesh.parent = root_parent;
  for (auto& trans : scene.transes)
    if (trans.parent.empty()) trans.parent = root_parent;
  for (auto& group : scene.groups)
    if (group.name != root_parent && group.parent.empty())
      group.parent = root_parent;
}

void dirty_reparented_scene_worlds(milo_scene::Scene& scene) {
  for (auto& mesh : scene.meshes) mesh.world_stored = mesh.local;
  for (auto& trans : scene.transes) trans.world_stored = trans.local;
  for (auto& group : scene.groups)
    if (group.has_transform) group.world_stored = group.local;
  for (auto& placer : scene.band_placers)
    if (placer.decoded) placer.world_stored = placer.local;
}

void apply_guitar_skin_material(milo_scene::Scene& scene, Symbol skin_mat) {
  if (!skin_mat.valid()) return;
  const std::string mat = skin_mat.c_str();
  if (!scene.find_mat(mat)) return;
  for (auto& mesh : scene.meshes) {
    if (mesh.name == "guitar.mesh" || mesh.name == "guitar_fire.mesh")
      mesh.material = mat;
  }
}

bool guitar_display_support_mesh(const std::string& name) {
  return name == "shadow_guitar.mesh";
}

void namespace_guitar_scene_nodes(milo_scene::Scene& scene,
                                  const std::string& suffix) {
  if (suffix.empty()) return;
  std::unordered_map<std::string, std::string> renamed;
  auto rename = [&](std::string& name) {
    if (name.empty()) return;
    auto [it, inserted] = renamed.emplace(name, name + suffix);
    name = it->second;
  };
  auto rewrite = [&](std::string& name) {
    auto it = renamed.find(name);
    if (it != renamed.end()) name = it->second;
  };

  for (auto& mesh : scene.meshes) rename(mesh.name);
  for (auto& trans : scene.transes) rename(trans.name);
  for (auto& group : scene.groups) rename(group.name);
  for (auto& mat : scene.mats) rename(mat.name);

  for (auto& mesh : scene.meshes) {
    rewrite(mesh.parent);
    rewrite(mesh.target);
    rewrite(mesh.material);
    rewrite(mesh.geometry_owner);
    for (auto& bone : mesh.bones) rewrite(bone.name);
  }
  for (auto& trans : scene.transes) {
    rewrite(trans.parent);
    rewrite(trans.target);
  }
  for (auto& group : scene.groups) {
    rewrite(group.parent);
    rewrite(group.target);
    rewrite(group.draw_only);
    rewrite(group.lod);
    for (auto& child : group.children) rewrite(child);
  }
  for (auto& name : scene.draw_order) rewrite(name);
  for (auto& name : scene.grouped_meshes) rewrite(name);
  rewrite(scene.dir_name);
}

void bake_guitar_scene_to_display_world(milo_scene::Scene& scene,
                                        const std::array<float, 16>& display_world) {
  for (auto& mesh : scene.meshes) {
    if (!mesh.decoded) continue;
    const std::array<float, 16> total =
        mul_mat4(scene.world_matrix(mesh), display_world);
    for (auto& v : mesh.verts) {
      transform_menu_point(total, v.px, v.py, v.pz);
      transform_menu_normal(total, v.nx, v.ny, v.nz);
    }
    mesh.bb_min[0] = mesh.bb_min[1] = mesh.bb_min[2] = 1.0e30f;
    mesh.bb_max[0] = mesh.bb_max[1] = mesh.bb_max[2] = -1.0e30f;
    for (const auto& v : mesh.verts) {
      mesh.bb_min[0] = std::min(mesh.bb_min[0], v.px);
      mesh.bb_min[1] = std::min(mesh.bb_min[1], v.py);
      mesh.bb_min[2] = std::min(mesh.bb_min[2], v.pz);
      mesh.bb_max[0] = std::max(mesh.bb_max[0], v.px);
      mesh.bb_max[1] = std::max(mesh.bb_max[1], v.py);
      mesh.bb_max[2] = std::max(mesh.bb_max[2], v.pz);
    }
    mesh.local = {};
    mesh.world_stored = {};
    mesh.world_xfm_override = false;
    mesh.parent.clear();
    mesh.target.clear();
    mesh.constraint = 0;
  }
  for (auto& trans : scene.transes) {
    trans.local = {};
    trans.world_stored = {};
    trans.world_xfm_override = false;
    trans.parent.clear();
    trans.target.clear();
    trans.constraint = 0;
  }
  for (auto& group : scene.groups) {
    if (!group.has_transform) continue;
    group.local = {};
    group.world_stored = {};
    group.world_xfm_override = false;
    group.parent.clear();
    group.target.clear();
    group.constraint = 0;
  }
}

bool build_live_guitar_display_scene(const std::string& hdr, const std::string& ark,
                                     ScreenManager& mgr, Object* screen,
                                     const ConfigDb& db,
                                     milo_scene::Scene& combined,
                                     std::map<std::string, asset::Image>& textures,
                                     std::string& default_environment,
                                     std::array<float, 16>& scene_world_transform,
                                     bool& uses_screen_proxy_camera,
                                     std::vector<GuitarDisplayRuntimeAnim>& runtime_anims) {
  scene_world_transform = identity_mat4();
  uses_screen_proxy_camera = false;
  bool rig_added = false;
  bool added_guitar = false;
  std::unordered_set<std::string> lighting_sources_added;
  std::unordered_set<std::string> proxy_context_sources_added;
  auto ensure_shared_rig = [&]() {
    if (rig_added) return;
    milo_scene::Scene rig;
    if (milo_scene::load_scene(hdr, ark, "ui/gen/guitar_display.milo_ps2", rig)) {
      append_menu_rig_scene(combined, rig);
      rig_added = true;
    }
  };
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel || panel->class_name() != Symbol("GuitarDisplayPanel") ||
        !panel_showing(panel)) {
      continue;
    }
    const std::vector<int> players = guitar_display_active_players(panel);
    for (const int player : players) {

    Symbol guitar = symbol_value(guitar_display_panel_value(panel, "guitar", player));
    Symbol skin = symbol_value(guitar_display_panel_value(panel, "guitar_skin", player));
    if (!guitar.valid()) continue;
    if (!skin.valid()) skin = db.first_guitar_skin(guitar);

    Symbol outfit =
        symbol_value(db.guitar_skin_field(guitar, skin, Symbol("outfit")));
    if (!outfit.valid()) outfit = guitar;
    Symbol skin_mat =
        symbol_value(db.guitar_skin_field(guitar, skin, Symbol("mat")));
    const std::string guitar_path =
        std::string("char/og/guitars/gen/") + outfit.c_str() + ".milo_ps2";

    DataNode proxy_node = guitar_display_panel_value(panel, "guitar_proxy", player);
    DataNode filter_node = guitar_display_panel_value(panel, "guitar_filter", player);
    Symbol proxy_name = symbol_value(proxy_node);
    Object* proxy_obj = object_value(proxy_node);
    Symbol filter_symbol = symbol_value(filter_node);
    Object* filter_obj = object_value(filter_node);
    const std::string proxy_milo =
        source_milo_for_screen_object(mgr, screen, proxy_name, proxy_obj);
    std::string filter_milo =
        source_milo_for_screen_object(mgr, screen, filter_symbol, filter_obj);
    if (filter_milo.empty()) filter_milo = proxy_milo;

    DataNode env_node =
        guitar_display_panel_value(panel, "guitar_display_env", player);
    Symbol env_name = symbol_value(env_node);
    Object* env_obj = object_value(env_node);
    std::string env_milo =
        source_milo_for_screen_object(mgr, screen, env_name, env_obj);
    if (env_milo.empty()) env_milo = proxy_milo;
    if (env_milo.empty()) env_milo = filter_milo;
    if (env_name.valid() && !env_milo.empty() &&
        lighting_sources_added.insert(env_milo).second) {
      milo_scene::Scene env_scene;
      if (milo_scene::load_scene(hdr, ark, env_milo, env_scene)) {
        if (env_scene.find_environ(env_name.c_str()))
          default_environment = env_name.c_str();
        append_scene_lighting(combined, env_scene);
      }
    }

    milo_scene::Scene guitar_scene;
    if (!milo_scene::load_scene(hdr, ark, guitar_path, guitar_scene)) continue;
    apply_guitar_skin_material(guitar_scene, skin_mat);
    load_scene_textures(hdr, ark, guitar_path, guitar_scene, textures);

    std::string display_parent;
    std::string applied_filter_source;
    bool used_live_proxy = false;
    bool used_live_proxy_main_trans = false;
    bool baked_shared_display = false;
    if (guitar_display_uses_live_proxy(pn) && proxy_name.valid() &&
        !proxy_milo.empty()) {
      const MenuProxyTransform proxy =
          extract_menu_proxy_transform(hdr, ark, proxy_milo, proxy_name.c_str());
      if (proxy.valid) {
        milo_scene::Xfm proxy_local = xfm_from_menu_matrix(proxy.local);
        const MenuAnimFilter filter =
            extract_menu_anim_filter(hdr, ark, filter_milo,
                                     filter_symbol.c_str());
        const MenuSliderAnim filter_anim =
            filter.valid ? extract_menu_slider_anim(hdr, ark, filter_milo,
                                                     filter.trans_anim)
                         : MenuSliderAnim{};
        if (filter.valid &&
            apply_guitar_display_filter_to_xfm(hdr, ark, filter_milo,
                                               filter_symbol.c_str(),
                                               proxy.name, proxy_local)) {
          applied_filter_source = filter_milo;
        }
        milo_scene::Scene proxy_scene;
        milo_scene::Xfm proxy_world = xfm_from_menu_matrix(proxy.world);
        bool composed_proxy = false;
        if (milo_scene::load_scene(hdr, ark, proxy_milo, proxy_scene)) {
          composed_proxy =
              compose_proxy_world(proxy_scene, proxy, proxy_local, proxy_world);
          if (proxy_context_sources_added.insert(proxy_milo).second)
            append_proxy_transform_context(combined, proxy_scene);
        }
        append_proxy_transform_node(combined, proxy, proxy_local, proxy_world);
        used_live_proxy = true;
        uses_screen_proxy_camera = true;
        log_guitar_scene_hierarchy(guitar_scene, "before_main_trans");
        std::array<float, 4> guitar_base_q{0.0f, 0.0f, 0.0f, 1.0f};
        for (const auto& mesh : guitar_scene.meshes) {
          if (mesh.name == "guitar.mesh") {
            guitar_base_q = row_rot_to_quat_xyzw(mesh.local);
            break;
          }
        }
        if (apply_guitar_proxy_main_trans(guitar_scene, "guitar.mesh",
                                          proxy_world)) {
          used_live_proxy_main_trans = true;
          if (filter.valid && filter.type == 1 && filter.scale != 0.0f &&
              !filter_anim.rotation_keys.empty() && filter.end > filter.start) {
            const float initial = anim_filter_source_frame(filter);
            const float source_span = filter.end - filter.start;
            const float parent_span = source_span / std::fabs(filter.scale);
            const auto initial_q =
                sample_menu_quat_value(filter_anim.rotation_keys, initial);
            GuitarDisplayRuntimeAnim runtime;
            runtime.target = "guitar.mesh";
            if (players.size() > 1)
              runtime.target +=
                  "__p" + std::to_string(std::max(0, player));
            runtime.anim.rotation_slerp = true;
            const int samples =
                std::max(2, static_cast<int>(std::ceil(parent_span)));
            runtime.anim.rotation_keys.reserve(static_cast<size_t>(samples + 1));
            for (int i = 0; i <= samples; ++i) {
              const float parent_frame = parent_span * float(i) / float(samples);
              float source_frame = initial + parent_frame * filter.scale;
              while (source_frame >= filter.end) source_frame -= source_span;
              while (source_frame < filter.start) source_frame += source_span;
              const auto source_q = sample_menu_quat_value(
                  filter_anim.rotation_keys, source_frame);
              const auto delta_q =
                  mul_quat_xyzw(conjugate_quat_xyzw(initial_q), source_q);
              const auto sampled_q = mul_quat_xyzw(guitar_base_q, delta_q);
              ghogx::render::MiloSceneRenderer::MeshQuatAnimKey key;
              key.frame = parent_frame;
              for (int q = 0; q < 4; ++q) key.quat_xyzw[q] = sampled_q[q];
              runtime.anim.rotation_keys.push_back(key);
            }
            runtime_anims.push_back(std::move(runtime));
          }
        } else {
          append_proxy_group(combined, proxy.name, proxy_world);
          display_parent = proxy.name;
        }
        log_guitar_scene_hierarchy(guitar_scene, "after_main_trans");
        if (std::getenv("GHOGX_LOG_GUITAR_FILTER")) {
          std::fprintf(stderr,
                       "[menu] proxy world %s/%s parent=%s constraint=%u local=(%.3f %.3f %.3f) world=(%.3f %.3f %.3f) composed=%d main_trans=%d\n",
                       proxy_milo.c_str(), proxy.name.c_str(),
                       proxy.parent.empty() ? "<none>" : proxy.parent.c_str(),
                       proxy.constraint, proxy_local.pos[0], proxy_local.pos[1],
                       proxy_local.pos[2], proxy_world.pos[0],
                       proxy_world.pos[1], proxy_world.pos[2],
                       composed_proxy ? 1 : 0,
                       used_live_proxy_main_trans ? 1 : 0);
          std::fprintf(stderr,
                       "[menu] proxy rows local row0=(%.6f %.6f %.6f) "
                       "row1=(%.6f %.6f %.6f) row2=(%.6f %.6f %.6f) "
                       "world row0=(%.6f %.6f %.6f) row1=(%.6f %.6f %.6f) "
                       "row2=(%.6f %.6f %.6f)\n",
                       proxy_local.rot[0][0], proxy_local.rot[0][1],
                       proxy_local.rot[0][2], proxy_local.rot[1][0],
                       proxy_local.rot[1][1], proxy_local.rot[1][2],
                       proxy_local.rot[2][0], proxy_local.rot[2][1],
                       proxy_local.rot[2][2], proxy_world.rot[0][0],
                       proxy_world.rot[0][1], proxy_world.rot[0][2],
                       proxy_world.rot[1][0], proxy_world.rot[1][1],
                       proxy_world.rot[1][2], proxy_world.rot[2][0],
                       proxy_world.rot[2][1], proxy_world.rot[2][2]);
        }
      }
    }

    GuitarDisplayAttachTarget target;
    if (display_parent.empty() && !used_live_proxy_main_trans) {
      ensure_shared_rig();
      target = guitar_display_attach_target(pn, player);
      display_parent = target.parent();
      if (apply_guitar_display_filter_to_target(
              hdr, ark, "ui/gen/guitar_display.milo_ps2",
              guitar_display_filter_name(pn, player), target.placer, combined)) {
        applied_filter_source = "ui/gen/guitar_display.milo_ps2";
      }
      std::array<float, 16> display_world{};
      if (scene_world_for_name(combined, display_parent, display_world)) {
        bake_guitar_scene_to_display_world(guitar_scene, display_world);
        baked_shared_display = true;
      }
    }
    if (default_environment.empty()) {
      ensure_shared_rig();
      default_environment = "guitar_setup.env";
    }
    if (baked_shared_display) {
      namespace_guitar_scene_nodes(
          guitar_scene, players.size() > 1
                            ? ("__p" + std::to_string(std::max(0, player)))
                            : "");
    } else if (!display_parent.empty()) {
      parent_root_nodes_to_display(guitar_scene, display_parent);
      namespace_guitar_scene_nodes(
          guitar_scene, players.size() > 1
                            ? ("__p" + std::to_string(std::max(0, player)))
                            : "");
      // Fallback guitar-display placers are part of the composed menu rig. Once
      // reparented into that rig, dirty the guitar file's old absolute rows so
      // the renderer follows the new local-parent chain.
      dirty_reparented_scene_worlds(guitar_scene);
    } else if (players.size() > 1) {
      namespace_guitar_scene_nodes(
          guitar_scene, "__p" + std::to_string(std::max(0, player)));
    }
    for (auto& mesh : guitar_scene.meshes) {
      if (guitar_display_support_mesh(mesh.name)) continue;
      combined.meshes.push_back(std::move(mesh));
    }
    for (auto& mat : guitar_scene.mats) combined.mats.push_back(std::move(mat));
    for (auto& trans : guitar_scene.transes) combined.transes.push_back(std::move(trans));
    for (auto& group : guitar_scene.groups) combined.groups.push_back(std::move(group));
    for (auto& name : guitar_scene.draw_order) {
      if (guitar_display_support_mesh(name)) continue;
      combined.draw_order.push_back(std::move(name));
    }
    for (auto& name : guitar_scene.grouped_meshes) {
      if (guitar_display_support_mesh(name)) continue;
      combined.grouped_meshes.push_back(std::move(name));
    }
    added_guitar = true;

    std::fprintf(stderr,
                 "[menu] guitar display: panel=%s player=%d guitar=%s skin=%s outfit=%s mat=%s parent=%s proxy=%s:%s filter=%s:%s env=%s:%s live_proxy=%d\n",
                 pn.c_str(), player, guitar.c_str(), skin.c_str(), outfit.c_str(),
                 skin_mat.valid() ? skin_mat.c_str() : "<none>",
                 display_parent.c_str(),
                 proxy_milo.empty() ? "<none>" : proxy_milo.c_str(),
                 proxy_name.valid() ? proxy_name.c_str() : "<none>",
                 applied_filter_source.empty() ? "<none>" : applied_filter_source.c_str(),
                 filter_symbol.valid() ? filter_symbol.c_str() : "<none>",
                 env_milo.empty() ? "<none>" : env_milo.c_str(),
                 env_name.valid() ? env_name.c_str() : "<none>",
                 used_live_proxy ? 1 : 0);
    }
  }
  return added_guitar;
}

// The panel names listed in a screen's (panels ...) property.
std::vector<Symbol> screen_panel_names(Object* screen) {
  std::vector<Symbol> out;
  if (!screen) return out;
  DataNode p = screen->get_property(Symbol("panels"));
  if (auto arr = p.as_array()) {
    for (std::size_t i = 0; i < arr->size(); ++i)
      if (auto s = arr->at(i).as_symbol()) out.push_back(*s);
  } else if (auto s = p.as_symbol()) {
    out.push_back(*s);
  }
  return out;
}

bool screen_has_panel(Object* screen, Symbol panel) {
  for (Symbol name : screen_panel_names(screen)) {
    if (name == panel) return true;
  }
  return false;
}

std::string panel_file(Object* panel) {
  if (!panel) return {};
  DataNode f = panel->get_property(Symbol("file"));
  if (f.empty()) f = panel->handle_property(Symbol("file"), DataArray());
  std::string file;
  if (auto sym = f.as_symbol()) file = sym->c_str();
  if (auto text = f.as_string()) file = std::string(*text);
  return file;
}

std::array<float, 16> menu_xfm_to_mat4(const std::array<float, 12>& xfm) {
  return {xfm[0], xfm[1], xfm[2], 0.0f,
          xfm[3], xfm[4], xfm[5], 0.0f,
          xfm[6], xfm[7], xfm[8], 0.0f,
          xfm[9], xfm[10], xfm[11], 1.0f};
}

std::unordered_set<std::string> hidden_meshes_from_live_views(
    ScreenManager& mgr, const milo_scene::Scene& scene) {
  std::unordered_map<std::string, std::string> parent_by_name;
  auto assign_parent = [&](const std::string& child,
                           const std::string& parent) {
    if (child.empty() || parent.empty()) return;
    if (parent_by_name.find(child) == parent_by_name.end())
      parent_by_name.emplace(child, parent);
  };
  for (const auto& trans : scene.transes) assign_parent(trans.name, trans.parent);
  for (const auto& group : scene.groups) {
    assign_parent(group.name, group.parent);
    for (const auto& child : group.children) assign_parent(child, group.name);
    assign_parent(group.draw_only, group.name);
  }
  for (const auto& mesh : scene.meshes) assign_parent(mesh.name, mesh.parent);

  auto live_hidden = [&](const std::string& name) {
    if (name.empty()) return false;
    if (Object* obj = mgr.resolve_object(Symbol(name.c_str()))) {
      if (obj->has_property(Symbol("showing")))
        return !node_bool(obj->get_property(Symbol("showing")));
      const DataNode handled =
          obj->handle_property(Symbol("showing"), DataArray());
      if (!handled.empty()) return !node_bool(handled);
    }
    return false;
  };

  auto hidden_by_ancestor = [&](const std::string& name) {
    std::string cur = name;
    std::unordered_set<std::string> visited;
    while (!cur.empty() && visited.insert(cur).second) {
      if (live_hidden(cur)) return true;
      auto it = parent_by_name.find(cur);
      if (it == parent_by_name.end()) break;
      cur = it->second;
    }
    return false;
  };

  std::unordered_set<std::string> hidden;
  for (const auto& mesh : scene.meshes) {
    if (hidden_by_ancestor(mesh.name)) hidden.insert(mesh.name);
  }
  return hidden;
}

// Build the renderer's scene from the current screen's panels' MILOs.
void rebuild_scene(const std::string& hdr, const std::string& ark, ScreenManager& mgr,
                   Object* screen,
                   ghogx::render::MiloSceneRenderer& renderer) {
  milo_scene::Scene combined;
  std::map<std::string, asset::Image> textures;
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel_showing(panel)) continue;
    add_panel_milo(hdr, ark, panel_file(panel), combined, textures);
  }
  // PanelDir::CamOverride refers to metacam through a stock subdirectory on
  // ordinary menu panels. Make that authored camera/environment available to
  // the combined renderer just as RndDir would after resolving the subdir.
  if (combined.panel_camera == "meta.cam") {
    milo_scene::Scene metacam;
    if (milo_scene::load_scene(hdr, ark, "ui/gen/metacam.milo_ps2", metacam)) {
      for (auto& camera : metacam.cams)
        combined.cams.push_back(std::move(camera));
      for (auto& light : metacam.lights)
        combined.lights.push_back(std::move(light));
      for (auto& environment : metacam.environs)
        combined.environs.push_back(std::move(environment));
      for (auto& environment_anim : metacam.env_anims)
        combined.env_anims.push_back(std::move(environment_anim));
    }
  }
  if (screen && screen->name() == Symbol("endgame_screen")) {
    std::string outfit;
    if (Object* game = mgr.resolve_object(Symbol("game"))) {
      if (auto value = game->get_property(Symbol("result_character")).as_string())
        outfit = std::string(*value);
    }
    const std::string portrait_path =
        asset::endgame_photo_bitmap_path_for_outfit(outfit);
    asset::Image portrait;
    if (!portrait_path.empty())
      portrait = asset::load_ps2_bitmap_from_ark(hdr, ark, portrait_path);
    if (!portrait.valid()) {
      // ui_objects.dta::UIPicture/endgame authors this exact resource and
      // default texture, so direct-start/debug results still show valid game
      // art when no completed-song outfit has been recorded.
      portrait = asset::load_milo_texture_named(
          hdr, ark, "ui/gen/picture_endgame.milo_ps2", "pic_photo.tex");
    }
    if (portrait.valid()) {
      textures["winner_photo0.tex"] = std::move(portrait);
      std::fprintf(stderr, "[menu] endgame portrait: %s\n",
                   portrait_path.empty() ? "ui/gen/picture_endgame.milo_ps2::pic_photo.tex"
                                         : portrait_path.c_str());
    }
  }
  std::fprintf(stderr, "[menu] %s: %zu meshes, %zu textures\n",
               screen ? screen->name().c_str() : "?", combined.meshes.size(), textures.size());
  auto hidden_meshes = hidden_meshes_from_live_views(mgr, combined);
  if (!hidden_meshes.empty())
    std::fprintf(stderr, "[menu] live hidden meshes: %zu\n", hidden_meshes.size());
  renderer.set_scene(std::move(combined), textures);
  renderer.set_default_environment(renderer.scene_panel_environment());
  renderer.set_hidden_meshes(std::move(hidden_meshes));
  if (!renderer.select_scene_panel_camera())
    apply_menu_meta_camera(hdr, ark, renderer);
}

bool rebuild_guitar_display_scene(const std::string& hdr, const std::string& ark,
                                  ScreenManager& mgr, Object* screen,
                                  const ConfigDb& db,
                                  ghogx::render::MiloSceneRenderer& renderer) {
  milo_scene::Scene scene;
  std::map<std::string, asset::Image> textures;
  std::string default_environment;
  std::array<float, 16> scene_world_transform = identity_mat4();
  bool uses_screen_proxy_camera = false;
  std::vector<GuitarDisplayRuntimeAnim> runtime_anims;
  const bool visible =
      build_live_guitar_display_scene(hdr, ark, mgr, screen, db, scene, textures,
                                      default_environment, scene_world_transform,
                                      uses_screen_proxy_camera, runtime_anims);
  renderer.set_scene(std::move(scene), textures);
  for (auto& runtime : runtime_anims) {
    renderer.trigger_mesh_transform_anim(runtime.target, std::move(runtime.anim),
                                         runtime.frames_per_second, true);
  }
  renderer.set_world_transform(scene_world_transform);
  renderer.set_default_environment(default_environment.empty() ? "guitar_setup.env"
                                                              : default_environment);
  renderer.set_clear_depth_on_overlay(true);
  // UIProxy::DrawShowing uses the owning panel's active camera. Shared lighting
  // setup can contribute guitar_setup.cam to this scene; clear that automatic
  // selection for live proxies so draw_menu_layers supplies the owning screen
  // camera (with the proxy clipping range widened there).
  if (uses_screen_proxy_camera) apply_menu_meta_camera(hdr, ark, renderer);
  return visible;
}

struct MenuCharacterPreview {
  int player = 0;
  std::string panel;
  std::string outfit;
  std::string placer;
  std::string environment;
  std::unique_ptr<ghogx::character::CharClip> ui_enter_clip;
  std::unique_ptr<ghogx::character::CharClip> ui_clip;
  // CharDriver::Transfer keeps the old play nodes alive while the selected
  // outfit's ui_loop blends onto the transferred stack. These owners preserve
  // the clip pointers held by clip_player across the character reload.
  std::vector<std::unique_ptr<ghogx::character::CharClip>> transferred_clips;
  ghogx::character::CharClipPlayer clip_player;
  bool enter_pending_loop = false;
  std::unique_ptr<ghogx::character::CharRenderer> renderer;
};

std::string indexed_runtime_name(const char* stem, int player) {
  return std::string(stem) + "_" + std::to_string(std::max(0, player));
}

std::vector<MenuCharacterPreview> rebuild_character_display_scenes(
    const std::string& hdr, const std::string& ark, ScreenManager& mgr,
    Object* screen, ghogx::render::Window& window,
    std::vector<MenuCharacterPreview> previous = {}) {
  std::vector<MenuCharacterPreview> previews;
  if (!screen) return previews;

  for (Symbol panel_name : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(panel_name);
    if (!panel || panel->class_name() != Symbol("CharsysPanel") ||
        !panel_showing(panel))
      continue;
    const int slots = std::max(
        1, panel->get_property(Symbol("num_placers")).as_int().value_or(1));
    for (int player = 0; player < slots; ++player) {
      const Symbol outfit = symbol_value(panel->get_property(
          Symbol(indexed_runtime_name("char_outfit", player).c_str())));
      if (!outfit.valid()) continue;
      const bool reload_requested =
          !panel->get_property(
                    Symbol(indexed_runtime_name("char_loaded", player).c_str()))
               .as_int()
               .value_or(0);

      const Symbol placer_symbol = symbol_value(panel->get_property(
          Symbol(indexed_runtime_name("char_placer", player).c_str())));
      std::string placer = placer_symbol.c_str();
      if (placer.empty()) {
        if (slots > 1)
          placer = "char_multi" + std::to_string(player) + ".placer";
        else if (panel_name == Symbol("char_store"))
          placer = "char_store.placer";
        else
          placer = "char_single.placer";
      }

      std::string placer_milo = source_milo_for_screen_object(
          mgr, screen, Symbol(placer.c_str()));
      if (placer_milo.empty()) {
        placer_milo = slots > 1 ? "ui/gen/multi_sel_character.milo_ps2"
                                : (panel_name == Symbol("char_store")
                                       ? "ui/gen/store_character.milo_ps2"
                                       : "ui/gen/sel_character.milo_ps2");
      }
      milo_scene::Scene placement_scene;
      std::array<float, 16> placement_world = identity_mat4();
      if (!milo_scene::load_scene(hdr, ark, placer_milo, placement_scene) ||
          !scene_world_for_name(placement_scene, placer, placement_world)) {
        std::fprintf(stderr,
                     "[menu-char] player=%d outfit=%s missing source placer "
                     "%s in %s\n",
                     player, outfit.c_str(), placer.c_str(), placer_milo.c_str());
        panel->set_property(
            Symbol(indexed_runtime_name("char_loaded", player).c_str()),
            DataNode::Int(0));
        continue;
      }

      const std::string environment =
          symbol_value(panel->get_property(
                           Symbol(indexed_runtime_name("char_env", player)
                                      .c_str())))
              .c_str();
      if (!reload_requested) {
        auto old = std::find_if(
            previous.begin(), previous.end(), [&](const auto& candidate) {
              return candidate.player == player &&
                     candidate.panel == panel_name.c_str() &&
                     candidate.outfit == outfit.c_str();
            });
        if (old != previous.end()) {
          old->placer = placer;
          old->environment = environment;
          old->renderer->set_world_transform(placement_world);
          previews.push_back(std::move(*old));
          continue;
        }
      }

      const std::string base = "char/" + std::string(outfit.c_str()) +
                               "/og/gen/" + outfit.c_str();
      std::string char_milo = base + "_ui.milo_ps2";
      ghogx::character::Character character;
      if (!ghogx::character::load_character(hdr, ark, char_milo, character)) {
        char_milo = base + ".milo_ps2";
        if (!ghogx::character::load_character(hdr, ark, char_milo, character)) {
          std::fprintf(stderr,
                       "[menu-char] player=%d failed stock character %s\n",
                       player, outfit.c_str());
          panel->set_property(
              Symbol(indexed_runtime_name("char_loaded", player).c_str()),
              DataNode::Int(0));
          continue;
        }
      }
      const auto texture_names = character.texture_names();
      auto textures =
          asset::load_milo_textures(hdr, ark, char_milo, texture_names);
      MenuCharacterPreview preview;
      preview.player = player;
      preview.panel = panel_name.c_str();
      preview.outfit = outfit.c_str();
      preview.placer = placer;
      preview.environment = environment;
      preview.renderer =
          std::make_unique<ghogx::character::CharRenderer>(window);
      preview.renderer->set_character(std::move(character), textures);
      preview.renderer->set_world_transform(placement_world);
      preview.renderer->set_use_scene_lighting(true);
      std::string ui_anim_owner = outfit.c_str();
      std::string ui_anim_milo =
          "char/" + ui_anim_owner + "/anims/gen/" + ui_anim_owner +
          "_ui.milo_ps2";
      preview.ui_clip = std::make_unique<ghogx::character::CharClip>(
          ghogx::character::load_clip(hdr, ark, ui_anim_milo, "ui_loop"));
      if (!preview.ui_clip->loaded && !ui_anim_owner.empty() &&
          ui_anim_owner.back() >= '2' && ui_anim_owner.back() <= '9') {
        // Alternate outfits own geometry/textures but share the base outfit's
        // authored UI animation bank (for example punk2 -> punk1_ui and the
        // independently audited rock2 -> rock1_ui stock case).
        ui_anim_owner.back() = '1';
        ui_anim_milo = "char/" + ui_anim_owner + "/anims/gen/" +
                       ui_anim_owner + "_ui.milo_ps2";
        preview.ui_clip = std::make_unique<ghogx::character::CharClip>(
            ghogx::character::load_clip(hdr, ark, ui_anim_milo, "ui_loop"));
      }
      if (!preview.ui_clip->loaded) {
        std::fprintf(stderr,
                     "[menu-char] player=%d outfit=%s missing stock ui_loop "
                     "in %s\n",
                     player, outfit.c_str(), ui_anim_milo.c_str());
        panel->set_property(
            Symbol(indexed_runtime_name("char_loaded", player).c_str()),
            DataNode::Int(0));
        continue;
      }
      const Symbol char_event = symbol_value(panel->get_property(
          Symbol(indexed_runtime_name("char_event", player).c_str())));
      const bool transfer_pending =
          panel->get_property(Symbol(indexed_runtime_name(
                                  "char_transfer_pending", player).c_str()))
              .as_int().value_or(0) != 0;
      if (slots == 1 && char_event == Symbol("animate")) {
        preview.ui_enter_clip = std::make_unique<ghogx::character::CharClip>(
            ghogx::character::load_clip(hdr, ark, ui_anim_milo, "ui_enter"));
      }
      auto transfer_source = previous.end();
      if (char_event == Symbol("select") ||
          (transfer_pending && char_event != Symbol("store"))) {
        transfer_source = std::find_if(
            previous.begin(), previous.end(), [&](const auto& candidate) {
              return candidate.player == player &&
                     candidate.panel == panel_name.c_str() &&
                     candidate.clip_player.active();
            });
      }
      if (transfer_source != previous.end()) {
        if (transfer_source->ui_enter_clip)
          preview.transferred_clips.push_back(
              std::move(transfer_source->ui_enter_clip));
        if (transfer_source->ui_clip)
          preview.transferred_clips.push_back(
              std::move(transfer_source->ui_clip));
        for (auto& clip : transfer_source->transferred_clips)
          if (clip) preview.transferred_clips.push_back(std::move(clip));
        preview.clip_player = std::move(transfer_source->clip_player);
        preview.clip_player.play(*preview.ui_clip,
                                 ghogx::character::kCharPlayLoop);
        std::fprintf(stderr,
                     "[menu-char] driver transfer player=%d from=%s to=%s "
                     "event=%s blend=source-default\n",
                     player, transfer_source->outfit.c_str(), outfit.c_str(),
                     char_event.valid() ? char_event.c_str() : "transfer");
      } else if (preview.ui_enter_clip && preview.ui_enter_clip->loaded) {
        preview.clip_player.play(
            *preview.ui_enter_clip,
            ghogx::character::kCharPlayNoLoop);
        preview.enter_pending_loop = true;
      } else {
        preview.clip_player.play(*preview.ui_clip,
                                 ghogx::character::kCharPlayLoop);
      }
      panel->set_property(
          Symbol(indexed_runtime_name("char_loaded", player).c_str()),
          DataNode::Int(1));
      panel->set_property(
          Symbol(indexed_runtime_name("char_object", player).c_str()),
          DataNode::Sym(outfit));
      panel->set_property(
          Symbol(indexed_runtime_name("char_transfer_pending", player).c_str()),
          DataNode::Int(0));
      std::fprintf(stderr,
                   "[menu-char] loaded player=%d outfit=%s source=%s placer=%s "
                   "placement=%s\n",
                   player, outfit.c_str(), char_milo.c_str(), placer.c_str(),
                   placer_milo.c_str());
      previews.push_back(std::move(preview));
    }
  }
  return previews;
}

ghogx::render::MiloSceneRenderer::MeshTransformAnim to_renderer_anim(
    const MenuSliderAnim& source) {
  ghogx::render::MiloSceneRenderer::MeshTransformAnim out;
  out.rotation_keys.reserve(source.rotation_keys.size());
  for (const MenuTransQuatKey& key : source.rotation_keys) {
    ghogx::render::MiloSceneRenderer::MeshQuatAnimKey dst;
    dst.frame = key.frame;
    for (int i = 0; i < 4; ++i) dst.quat_xyzw[i] = key.quat_xyzw[i];
    out.rotation_keys.push_back(dst);
  }
  out.translation_keys.reserve(source.translation_keys.size());
  for (const MenuTransVecKey& key : source.translation_keys) {
    ghogx::render::MiloSceneRenderer::MeshAnimKey dst;
    dst.frame = key.frame;
    for (int i = 0; i < 3; ++i) dst.pos[i] = key.value[i];
    out.translation_keys.push_back(dst);
  }
  out.scale_keys.reserve(source.scale_keys.size());
  for (const MenuTransVecKey& key : source.scale_keys) {
    ghogx::render::MiloSceneRenderer::MeshAnimKey dst;
    dst.frame = key.frame;
    for (int i = 0; i < 3; ++i) dst.pos[i] = key.value[i];
    out.scale_keys.push_back(dst);
  }
  return out;
}

void apply_loading_source_anims(const std::string& hdr, const std::string& ark,
                                ScreenManager& mgr, Object* screen,
                                ghogx::render::MiloSceneRenderer& renderer) {
  bool has_loading_panel = false;
  for (Symbol pn : screen_panel_names(screen)) {
    if (panel_file(mgr.find_object(pn)) == "loading.milo") {
      has_loading_panel = true;
      break;
    }
  }
  if (!has_loading_panel) return;

  for (const char* anim_name :
       {"wing1.tnm", "wing2.tnm", "tape.tnm", "loading_word.tnm"}) {
    const MenuSliderAnim anim = extract_menu_slider_anim(
        hdr, ark, "ui/gen/loading.milo_ps2", anim_name);
    if (!anim.valid) continue;
    renderer.trigger_mesh_transform_anim(anim.target, to_renderer_anim(anim),
                                         30.0f, true);
  }
}

const std::string* sample_material_texture_frame(
    const std::vector<MenuMaterialTextureKey>& keys, float frame) {
  if (keys.empty()) return nullptr;
  if (!std::isfinite(frame)) frame = keys.front().frame;
  size_t key_index = 0;
  constexpr float kFrameEpsilon = 0.0001f;
  while (key_index + 1 < keys.size() &&
         frame + kFrameEpsilon >= keys[key_index + 1].frame) {
    ++key_index;
  }
  return &keys[key_index].texture;
}

void apply_loading_material_source_anim(
    ScreenManager& mgr, Object* screen,
    const MenuMaterialAnim& loading_word_anim,
    ghogx::render::MiloSceneRenderer& renderer) {
  bool has_loading_panel = false;
  if (screen) {
    for (Symbol pn : screen_panel_names(screen)) {
      if (panel_file(mgr.find_object(pn)) == "loading.milo") {
        has_loading_panel = true;
        break;
      }
    }
  }
  if (!has_loading_panel || !loading_word_anim.valid) {
    renderer.set_material_texture_overrides({});
    return;
  }

  float frame = 0.0f;
  if (Object* loading_word = mgr.resolve_object(Symbol("loading_word.grp"))) {
    frame = loading_word->handle_property(Symbol("frame"), DataArray())
                .as_float()
                .value_or(0.0f);
  }
  const float span =
      std::max(0.0f, loading_word_anim.last_frame - loading_word_anim.first_frame);
  if (span > 0.0f) {
    frame = std::fmod(frame - loading_word_anim.first_frame, span);
    if (frame < 0.0f) frame += span;
    frame += loading_word_anim.first_frame;
  }
  const std::string* texture =
      sample_material_texture_frame(loading_word_anim.texture_keys, frame);
  if (texture && !texture->empty())
    renderer.set_material_texture_overrides(
        {{loading_word_anim.material, *texture}});
  else
    renderer.set_material_texture_overrides({});
}

// Fire the focused component's SELECT_START_MSG (Confirm). The screen's (focus)
// names the active panel; that panel's (focus) names the active component.
void fire_button_down(ScreenManager& mgr, Object* screen, Object* panel,
                      Symbol button, int player_num = 0) {
  if (!screen) return;
  mgr.set_global(Symbol("button"), DataNode::Sym(button));
  mgr.set_global(Symbol("player_num"), DataNode::Int(player_num));
  // MultiSelectScreen is the stock controller-to-player event boundary and
  // delegates exactly once to either the active per-player panel or its
  // focused shared panel. Calling the focused panel first would let one X
  // activate an outfit panel and then immediately mark that outfit ready.
  if (panel && screen->class_name() != Symbol("MultiSelectScreen"))
    panel->handle_property(Symbol("BUTTON_DOWN_MSG"), DataArray());
  if (mgr.current_screen() == screen)
    screen->handle_property(Symbol("BUTTON_DOWN_MSG"), DataArray());
}

void do_confirm(ScreenManager& mgr, int player_num = 0) {
  Object* screen = mgr.current_screen();
  if (!screen) return;
  Symbol panel_name = screen->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
  Object* panel = panel_name.valid() ? mgr.find_object(panel_name) : nullptr;
  if (!panel) {
    fire_button_down(mgr, screen, nullptr, Symbol("kPad_X"), player_num);
    return;
  }
  Symbol comp = panel->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
  if (!comp.valid()) {
    fire_button_down(mgr, screen, panel, Symbol("kPad_X"), player_num);
    return;
  }
  Object* comp_obj = comp.valid() ? mgr.resolve_object(comp) : nullptr;
  mgr.set_global(Symbol("component"),
                 comp_obj ? DataNode::Obj(comp_obj) : DataNode::Sym(comp));
  // A disabled component ignores SELECT (the original's disabled BandButton does
  // not fire its handler) — e.g. multiplayer when is_missing_multi_controller.
  if (comp.valid() && compute_disabled(mgr).count(comp.c_str())) {
    mgr.handle_property(Symbol("BAD_SELECT_START_MSG"), DataArray());
    return;
  }
  if (comp_obj) {
    comp_obj->handle_property(Symbol("send_select"), DataArray());
    // BandTextEntry owns this confirm: it emits TEXT_ENTRY_MSG, then the panel
    // poll starts the authored flush. Letting the same kPad_X bubble to the
    // high-score screen would skip that entire one-second/7-second sequence.
    if (comp_obj->class_name() == Symbol("BandTextEntry")) return;
  }
  fire_button_down(mgr, screen, panel, Symbol("kPad_X"), player_num);
  if (mgr.current_screen() != screen) return;
  mgr.handle_property(Symbol("SELECT_START_MSG"), DataArray());
  panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
  if (mgr.current_screen() == screen)
    screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
}

bool screen_allows_generic_back(Object* screen) {
  if (!screen || !screen->has_property(Symbol("allow_back"))) return true;
  return node_bool(screen->get_property(Symbol("allow_back")));
}

// Back (B/circle): retail sends the button event first, so authored
// BUTTON_DOWN_MSG kPad_Tri handlers can animate, play SFX, or route manually.
// Only screens that allow generic backing fall through to GHScreen::go_back.
void do_back(ScreenManager& mgr) {
  Object* screen = mgr.current_screen();
  if (!screen) return;
  Symbol panel_name =
      screen->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
  Object* panel = panel_name.valid() ? mgr.find_object(panel_name) : nullptr;
  if (cancel_focused_slider(mgr, panel)) return;
  fire_button_down(mgr, screen, panel, Symbol("kPad_Tri"));
  if (mgr.current_screen() != screen) return;
  mgr.handle_property(Symbol("SCREEN_BACK_MSG"), DataArray());
  screen->handle_property(Symbol("SCREEN_BACK_MSG"), DataArray());
  if (mgr.current_screen() != screen) return;
  if (!screen_allows_generic_back(screen)) return;
  screen->handle_property(Symbol("go_back"), DataArray());
  if (mgr.current_screen() != screen) return;
  mgr.go_back();
}

// Load ui/eng/gen/locale.dtb into a key->display-string map. The menu's button
// labels are locale keys (e.g. "QUICK_PLAY" -> "QUICK PLAY"); the BandButton
// embeds the key, the locale resolves the shown text. 1:1 with the stock data.
std::map<std::string, std::string> load_locale(const gh::ark::ArkV3Reader& ark,
                                               const std::vector<std::string>& arks) {
  std::map<std::string, std::string> m;
  try {
    auto e = ark.find("ui/eng/gen/locale.dtb");
    if (!e) return m;
    auto bytes = ark.read_entry(*e, arks);
    gh::dtb::Tree tree = gh::dtb::parse(bytes);
    std::shared_ptr<DataArray> root = dtb_bridge::from_tree(tree);
    if (root) {
      for (std::size_t i = 0; i < root->size(); ++i) {
        auto kv = root->at(i).as_array();
        if (!kv || kv->size() < 2) continue;
        auto key = kv->at(0).as_symbol();
        auto val = kv->at(1).as_string();
        if (key && key->valid() && val) m[key->c_str()] = std::string(*val);
      }
    }
  } catch (const std::exception&) {
  }
  std::fprintf(stderr, "[menu] locale: %zu strings\n", m.size());
  return m;
}

struct MetaMusicConfig {
  std::vector<std::string> tracks;
  float volume_db = 0.0f;
  float fade_seconds = 1.0f;
  std::string background_sequence;
  float background_min_delay = 0.0f;
  float background_max_delay = 0.0f;
};

MetaMusicConfig load_meta_music_config(const gh::ark::ArkV3Reader& ark,
                                       const std::vector<std::string>& arks) {
  MetaMusicConfig out;
  try {
    auto entry = ark.find("config/gen/synth.dtb");
    if (!entry) return out;
    const auto bytes = ark.read_entry(*entry, arks);
    const gh::dtb::Tree tree = gh::dtb::parse(bytes);
    const std::shared_ptr<DataArray> root = dtb_bridge::from_tree(tree);
    if (!root) return out;
    std::shared_ptr<DataArray> metamusic;
    for (std::size_t i = 0; i < root->size(); ++i) {
      auto array = root->at(i).as_array();
      if (!array || array->size() == 0) continue;
      if (array->at(0).as_symbol().value_or(Symbol()) == Symbol("metamusic")) {
        metamusic = array;
        break;
      }
    }
    if (!metamusic) return out;
    for (std::size_t i = 1; i < metamusic->size(); ++i) {
      auto field = metamusic->at(i).as_array();
      if (!field || field->size() == 0) continue;
      const Symbol key = field->at(0).as_symbol().value_or(Symbol());
      if (key == Symbol("volume") && field->size() > 1) {
        out.volume_db = field->at(1).as_float().value_or(
            static_cast<float>(field->at(1).as_int().value_or(0)));
      } else if (key == Symbol("fade_rate") && field->size() > 1) {
        out.fade_seconds = field->at(1).as_float().value_or(
            static_cast<float>(field->at(1).as_int().value_or(1)));
      } else if (key == Symbol("music")) {
        for (std::size_t track = 1; track < field->size(); ++track) {
          const Symbol name = field->at(track).as_symbol().value_or(Symbol());
          if (name.valid()) out.tracks.emplace_back(name.c_str());
        }
      } else if (key == Symbol("background_sfx")) {
        for (std::size_t bg = 1; bg < field->size(); ++bg) {
          auto setting = field->at(bg).as_array();
          if (!setting || setting->size() < 2) continue;
          const Symbol setting_key =
              setting->at(0).as_symbol().value_or(Symbol());
          if (setting_key == Symbol("sequence")) {
            const Symbol sequence =
                setting->at(1).as_symbol().value_or(Symbol());
            if (sequence.valid()) out.background_sequence = sequence.c_str();
          } else if (setting_key == Symbol("min_delay_sec")) {
            out.background_min_delay = setting->at(1).as_float().value_or(
                static_cast<float>(setting->at(1).as_int().value_or(0)));
          } else if (setting_key == Symbol("max_delay_sec")) {
            out.background_max_delay = setting->at(1).as_float().value_or(
                static_cast<float>(setting->at(1).as_int().value_or(0)));
          }
        }
      }
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[menu-audio] synth.dtb parse failed: %s\n",
                 ex.what());
  }
  std::fprintf(stderr,
               "[menu-audio] synth.dtb tracks=%zu volume_db=%.2f fade=%.2f "
               "background=%s delay=%.2f..%.2f\n",
               out.tracks.size(), out.volume_db, out.fade_seconds,
               out.background_sequence.c_str(), out.background_min_delay,
               out.background_max_delay);
  return out;
}

// Legacy placeholder constants below are kept only because nearby comments and
// docs still reference the old investigation. Rendering uses kResolvedColNormal
// and kResolvedColFocused, which come from the live ColorResolve trace.

// GH2 main-menu item colours — GROUND TRUTH: the actual retail menu (reference
// frame of the real game) shows NORMAL items RED and the FOCUSED item WHITE
// (CAREER white, QUICK PLAY/MULTIPLAYER/TRAINING/.../OPTIONS red).
//   normal  = RED   (1,0,0)
//   focused = WHITE (1,1,1)
//   disabled= GREY  (held for the multiplayer-disabled case)
// NOTE: the common.milo per-state .font mats (normal.mat white / focused.mat
// yellow / selecting.mat red / disabled.mat grey) are the GENERIC arial UIButton
// widget set — NOT the main-menu BandButtons, which use this red/white scheme. I
// wrongly applied the arial mats earlier; the exact data source for red/white
// (a PanelDir type or per-button colour) is to be re-pinned, but the VALUES are
// fixed by the real menu.
constexpr uint32_t kColNormal    = 0xFFFF0000u;  // RED   — normal items
constexpr uint32_t kColFocused   = 0xFFFFFFFFu;  // WHITE — focused item
constexpr uint32_t kColDisabled  = 0xFF666666u;  // grey  — disabled (multiplayer)
// Live 360 hmx_BandButton_ColorResolve/sub_82122920 outputs for settled
// main-menu buttons: normal state 0 = (0.4471, 0.1686, 0.1373), focused
// state 1 = (0.8196, 0.8196, 0.8196). These are the values to render until
// the PS2-specific resolver path is decoded.
constexpr uint32_t kResolvedColNormal  = 0xFF722B23u;
constexpr uint32_t kResolvedColFocused = 0xFFD1D1D1u;
constexpr uint32_t kResolvedColSelecting = 0xFFFFFFFFu;  // ui_objects.dta GH2 selecting_color
constexpr float kFocusScale      = 1.05f;        // ui_objects_ps2.dta:10 (focus_scale 1.05)
// Base RndText text_size: static main.milo tail and live trace both show 0.5.
// The main-menu overlay still needs the projected RndText fit model decoded, so
// the main-menu BandButtons use a separate visual-fit scalar below.
constexpr float kTextScale = 0.50f;
// Current main-menu fit against the user-provided reference frame. Keep separate
// from kTextScale so this is easy to delete once the RndText fit path is decoded.
constexpr float kMainButtonTextScale = 0.748f;
// Main-menu vertical layout — GROUNDED in the live XEX. The trace-360 BandButton
// struct hook captured all five main-menu buttons (scale 0.555/1.899, tilt -1deg =
// the main-menu template + poster tilt, confirmed by main-menu logic running). Their
// runtime Z is an EXACT affine remap of the static bind-pose Z (world[11]):
//     runtime_Z = 0.875 * bind_Z + 4.0
// fitting all five to < 0.05 (career 16.90->18.79, quickspin -13.46->-7.75,
// multiplayer -43.79->-34.32, tutorial -74.19->-60.92, options -104.63->-87.54). So
// the panel compresses the column to ~87.5% and nudges it up 4.0. (This is the REAL
// main-menu transform; the earlier reverted 0.758/-23.54 was from a boot DIALOG
// mistaken for the menu — different screen, scale 1.05 / tilt +2deg.)
constexpr float kMenuZScale  = 0.965f;
constexpr float kMenuZOffset = -1.65f;
// Shared centre axis for the menu column. The exact RndText alignment transform
// still needs to replace this projected fit constant.
constexpr float kMenuCenterX = 1.2f;

uint32_t pack_rgba_color(const std::array<float, 4>& color) {
  const auto channel = [](float value) -> uint32_t {
    if (!std::isfinite(value)) value = 1.0f;
    return static_cast<uint32_t>(
        std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
  };
  const uint32_t r = channel(color[0]);
  const uint32_t g = channel(color[1]);
  const uint32_t b = channel(color[2]);
  const uint32_t a = channel(color[3]);
  return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t pack_milo_vertex_color(const milo_scene::Vertex& vertex,
                                const milo_scene::MatObj* mat,
                                uint32_t fallback) {
  const auto unpack = [](uint32_t color, int shift) {
    return static_cast<float>((color >> shift) & 0xFFu) / 255.0f;
  };
  const float mr = mat ? mat->color[0] : unpack(fallback, 16);
  const float mg = mat ? mat->color[1] : unpack(fallback, 8);
  const float mb = mat ? mat->color[2] : unpack(fallback, 0);
  const float ma = mat ? mat->color[3] : unpack(fallback, 24);
  return pack_rgba_color({{vertex.r * mr, vertex.g * mg, vertex.b * mb,
                           vertex.a * ma}});
}

bool label_world_on_text_plane(const MenuLabel& label) {
  // Some PS2 BandLabels serialize a parent-composed WorldXfm whose Y is in a
  // panel scene space behind metacam (for example sel_guitar at about -975).
  // Text is authored on the menu draw plane, so use WorldXfm only when it is
  // already on that plane.
  return label.has_world && std::isfinite(label.world[10]) &&
         std::fabs(label.world[10]) < 100.0f;
}

std::string apply_authored_caps(std::string text, const MenuLabel& label) {
  bool all_caps = false;
  if (label.type == "BandButton" && label.button_tail.valid)
    all_caps = label.button_tail.all_caps != 0;
  if (label.type == "BandLabel" && label.text_tail.valid)
    all_caps = label.text_tail.all_caps != 0;
  if (!all_caps) return text;
  for (char& ch : text) {
    if (ch >= 'a' && ch <= 'z')
      ch = static_cast<char>(ch - ('a' - 'A'));
  }
  return text;
}

std::string display_text_from_node(
    const DataNode& node, const std::map<std::string, std::string>& locale) {
  std::string text;
  if (auto s = node.as_string())
    text = std::string(*s);
  else if (auto sym = node.as_symbol())
    text = sym->c_str();
  else if (auto i = node.as_int())
    text = std::to_string(*i);
  if (text.empty()) return {};
  if (auto it = locale.find(text); it != locale.end()) return it->second;
  const std::string ps2_key = text + "_ps2";
  if (auto it = locale.find(ps2_key); it != locale.end()) return it->second;
  return text;
}

Object* resolve_live_label_object(ScreenManager& mgr, const MenuLabel& label,
                                  const std::string& name) {
  if (!label.runtime_owner.empty()) {
    Object* owner = mgr.find_object(Symbol(label.runtime_owner.c_str()));
    if (auto* dir = dynamic_cast<ObjectDir*>(owner)) {
      if (Object* object = dir->find_path(name)) return object;
    }
  }
  return mgr.resolve_object(Symbol(name.c_str()));
}

std::string live_label_text(ScreenManager& mgr, const MenuLabel& label) {
  if (Object* obj = resolve_live_label_object(mgr, label, label.name)) {
    if (obj->has_property(Symbol("text"))) {
      DataNode node = obj->get_property(Symbol("text"));
      if (auto text = node.as_string())
        return std::string(*text);
      if (auto token = node.as_symbol()) return token->c_str();
      if (auto value = node.as_int()) return std::to_string(*value);
      return {};
    }
  }
  return label.text;
}

int live_component_state_code(ScreenManager& mgr, const MenuLabel& label) {
  Object* obj = resolve_live_label_object(mgr, label, label.name);
  if (!obj) return 0;
  DataNode state = obj->get_property(Symbol("state"));
  if (auto i = state.as_int()) return *i;
  if (auto s = state.as_symbol()) {
    if (*s == Symbol("focused") || *s == Symbol("kFocused")) return 1;
    if (*s == Symbol("disabled") || *s == Symbol("kDisabled")) return 2;
    if (*s == Symbol("selecting") || *s == Symbol("kSelecting")) return 3;
    if (*s == Symbol("selected") || *s == Symbol("kSelected")) return 4;
  }
  if (auto text = state.as_string()) {
    if (*text == "focused" || *text == "kFocused") return 1;
    if (*text == "disabled" || *text == "kDisabled") return 2;
    if (*text == "selecting" || *text == "kSelecting") return 3;
    if (*text == "selected" || *text == "kSelected") return 4;
  }
  return 0;
}

int live_component_state_code(ScreenManager& mgr, const std::string& name) {
  MenuLabel unowned;
  unowned.name = name;
  return live_component_state_code(mgr, unowned);
}

std::string normalized_label_font(std::string font) {
  if (font.empty()) return "impact";
  constexpr char suffix[] = ".font";
  constexpr std::size_t suffix_len = sizeof(suffix) - 1;
  if (font.size() > suffix_len &&
      font.compare(font.size() - suffix_len, suffix_len, suffix) == 0) {
    font.resize(font.size() - suffix_len);
  }
  return font;
}

class MenuFontCatalog {
 public:
  MenuFontCatalog(std::string hdr, std::string ark)
      : hdr_(std::move(hdr)), ark_(std::move(ark)) {}

  const MenuFont* get(std::string family) {
    family = normalized_label_font(std::move(family));
    if (family.empty()) family = "impact";
    auto found = fonts_.find(family);
    if (found != fonts_.end())
      return found->second && found->second->valid() ? found->second.get()
                                                       : nullptr;
    auto font = std::make_unique<MenuFont>();
    const bool loaded = font->load(hdr_, ark_,
                                   "ui/gen/" + family + ".milo_ps2");
    const MenuFont* result = loaded && font->valid() ? font.get() : nullptr;
    fonts_.emplace(std::move(family), std::move(font));
    return result;
  }

 private:
  std::string hdr_;
  std::string ark_;
  std::unordered_map<std::string, std::unique_ptr<MenuFont>> fonts_;
};

std::vector<std::string> wrap_text_lines(const MenuFont& font,
                                         const std::string& text,
                                         float max_native_width) {
  if (max_native_width <= 0.0f || font.measure(text) <= max_native_width)
    return {text};

  std::vector<std::string> lines;
  std::string line;
  std::string word;
  auto flush_word = [&]() {
    if (word.empty()) return;
    const std::string candidate = line.empty() ? word : line + " " + word;
    if (!line.empty() && font.measure(candidate) > max_native_width) {
      lines.push_back(line);
      line = word;
    } else {
      line = candidate;
    }
    word.clear();
  };

  for (char ch : text) {
    if (ch == '\n') {
      flush_word();
      lines.push_back(line);
      line.clear();
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch))) {
      flush_word();
    } else {
      word.push_back(ch);
    }
  }
  flush_word();
  if (!line.empty() || lines.empty()) lines.push_back(line);
  return lines;
}

std::vector<std::string> explicit_text_lines(const std::string& text) {
  std::vector<std::string> lines;
  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t newline = text.find('\n', start);
    lines.push_back(text.substr(
        start, newline == std::string::npos ? std::string::npos
                                            : newline - start));
    if (newline == std::string::npos) break;
    start = newline + 1;
  }
  return lines;
}

bool live_element_showing(ScreenManager& mgr, const std::string& name,
                          const std::string& parent,
                          bool authored_showing);

bool live_label_showing(ScreenManager& mgr, const MenuLabel& label,
                        bool authored_showing) {
  auto showing_value = [](Object* object) -> std::optional<bool> {
    if (!object) return std::nullopt;
    if (object->has_property(Symbol("showing")))
      return node_bool(object->get_property(Symbol("showing")));
    const DataNode handled =
        object->handle_property(Symbol("showing"), DataArray());
    if (!handled.empty()) return node_bool(handled);
    return std::nullopt;
  };
  Object* object = resolve_live_label_object(mgr, label, label.name);
  // store.dtb switches the category page as one logical st_screen1 view, but
  // the category BandButtons are serialized under per-category groups rather
  // than as transform descendants of st_screen1.view. Native UIView drawing
  // still gates them with that page. Mirror that stock page ownership here.
  static const std::unordered_set<std::string> kStoreCategoryButtons = {
      "st_guitars.btn", "st_skins.btn", "st_songs.btn",
      "st_characters.btn", "st_outfits.btn", "st_videos.btn"};
  if (kStoreCategoryButtons.find(label.name) != kStoreCategoryButtons.end()) {
    Object* page = mgr.resolve_object(Symbol("st_screen1.view"));
    if (page) {
      DataNode page_showing = page->handle_property(Symbol("showing"), DataArray());
      if (!page_showing.empty() && !node_bool(page_showing)) return false;
    }
  }
  if (const auto showing = showing_value(object)) {
    if (!*showing) return false;
  } else if (!authored_showing) {
    return false;
  }
  std::vector<std::string> ancestors = label.visibility_ancestors;
  if (ancestors.empty() && !label.parent.empty())
    ancestors.push_back(label.parent);
  bool visible = true;
  std::optional<bool> parent_showing;
  Object* parent = nullptr;
  for (const std::string& ancestor : ancestors) {
    parent = resolve_live_label_object(mgr, label, ancestor);
    parent_showing = showing_value(parent);
    if (parent_showing && !*parent_showing) {
      visible = false;
      break;
    }
  }
  if (std::getenv("GHOGX_LOG_MENU_LABEL_VISIBILITY")) {
    std::fprintf(stderr,
                 "[menu-label] name=%s parent=%s object=%s parent_object=%s "
                 "authored=%d parent_showing=%d:%d\n",
                 label.name.c_str(), label.parent.c_str(),
                 object ? object->name().c_str() : "<none>",
                 parent ? parent->name().c_str() : "<none>",
                 authored_showing ? 1 : 0, parent_showing ? 1 : 0,
                 parent_showing && *parent_showing ? 1 : 0);
  }
  return visible;
}

void append_text_quads(const std::vector<MenuLabel>& labels, const MenuFont& font,
                       const std::string& font_family,
                       ScreenManager& mgr,
                       const std::map<std::string, std::string>& locale,
                       const std::string& focused,
                       const std::unordered_set<std::string>& disabled,
                       std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out,
                       std::vector<ghogx::render::MiloSceneRenderer::TextTransformSpan>&
                           transform_spans) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  const float capH = font.cap_height();
  auto emit = [&](const std::vector<MenuFont::Quad>& quads,
                  const std::function<TV(float, float, float, float)>& V) {
    for (const auto& q : quads) {
      TV a = V(q.x0, q.y0, q.u0, q.v0), b = V(q.x1, q.y0, q.u1, q.v0),
         c = V(q.x1, q.y1, q.u1, q.v1), d = V(q.x0, q.y1, q.u0, q.v1);
      out.push_back(a); out.push_back(b); out.push_back(c);
      out.push_back(a); out.push_back(c); out.push_back(d);
    }
  };
  for (const auto& lbl : labels) {
    if (!lbl.has_local) continue;
    if (lbl.font.empty()) continue;
    const bool authored_showing = !lbl.has_showing || lbl.showing;
    if (!live_label_showing(mgr, lbl, authored_showing))
      continue;
    const bool isBtn = (lbl.type == "BandButton");
    const bool isTextEntry = (lbl.type == "BandTextEntry");
    if (!isBtn && !isTextEntry && lbl.type != "Text" &&
        lbl.type != "BandLabel")
      continue;
    if (normalized_label_font(lbl.font) != font_family) continue;

    // Colour: buttons by state; BandLabel uses its authored RGBA tail color.
    bool foc = false;
    uint32_t col = 0xFFFFFFFFu;
    if (isBtn) {
      foc = (lbl.name == focused);
      const int live_state = live_component_state_code(mgr, lbl);
      if (disabled.count(lbl.name) || live_state == 2)
        col = kColDisabled;
      else if (live_state == 3)
        col = kResolvedColSelecting;
      else
        col = foc ? kResolvedColFocused : kResolvedColNormal;
    } else if ((lbl.type == "BandLabel" || lbl.type == "Text") &&
               lbl.text_tail.valid) {
      // BandLabel and RndText both serialize their source RGBA with their text
      // layout. The parser decodes each class from its own source order.
      col = pack_rgba_color(lbl.text_tail.color);
    } else if (isTextEntry) {
      // config.dtb::textentry/styles/high_score::text_color = 0.1 0.1 0.1.
      col = 0xFF1A1A1Au;
    }
    std::string token = live_label_text(mgr, lbl);
    std::string disp = display_text_from_node(DataNode::Str(token), locale);
    disp = apply_authored_caps(std::move(disp), lbl);
    if (disp.empty()) continue;
    const size_t first_vertex = out.size();
    float w = 0.0f;
    auto quads = font.layout(disp, &w);

    // Normalized local-X / local-Z axes (X-Z plane) for the menu's slight tilt;
    // n0/n2 are the original scale magnitudes (used to spot the bind pose).
    float r0x = lbl.world[0], r0z = lbl.world[2], r2x = lbl.world[6], r2z = lbl.world[8];
    float n0 = std::sqrt(r0x*r0x + r0z*r0z), n2 = std::sqrt(r2x*r2x + r2z*r2z);
    if (n0 > 1e-6f) { r0x /= n0; r0z /= n0; }
    if (n2 > 1e-6f) { r2x /= n2; r2z /= n2; }

    if (isBtn) {
      // BandButton glyphs always use the uniform kTextScale (the in-MILO button
      // scale is a TransAnim bind pose; it does NOT give the rendered size). X:
      // the main menu's bind-pose buttons (non-uniform box scale 0.555/1.899) use
      // the runtime-aligned left edge; other screens use the button's translation.
      const bool bindPose = n0 > 1e-3f && n2 > 1e-3f &&
                            (std::min(n0, n2) / std::max(n0, n2) < 0.6f);
      const float ax = bindPose ? kMenuCenterX : lbl.world[9];
      const float ay = lbl.world[10];
      // Main-menu bind-pose buttons: remap the bind-pose Z to the XEX-measured
      // runtime Z (affine). Other screens use their Z.
      const float az = bindPose ? (kMenuZScale * lbl.world[11] + kMenuZOffset) : lbl.world[11];
      if (bindPose || !lbl.button_tail.valid ||
          lbl.button_tail.text_size <= 0.0f) {
        const float scl = (bindPose ? kMainButtonTextScale : kTextScale) *
                          (foc ? kFocusScale : 1.0f);
        float align_x = -w * 0.5f;
        if (!bindPose && lbl.button_tail.valid) {
          const int alignment = lbl.button_tail.alignment;
          if ((alignment & 4) != 0)
            align_x = -w;
          else if ((alignment & 2) != 0)
            align_x = -w * 0.5f;
          else
            align_x = 0.0f;
        }
        emit(quads, [&](float qx, float qy, float u, float v) {
          const float lx = (qx + align_x) * scl;
          const float lz = -(qy - capH * 0.5f) * scl;
          TV tv{ax + lx * r0x + lz * r2x, ay,
                az + lx * r0z + lz * r2z, u, v, col};
          return tv;
        });
      } else {
        const auto& tail = lbl.button_tail;
        const int alignment = tail.alignment;
        // GH2's legacy BandButton tail stores mSize as a multiplier of the
        // source font metrics (main=.5, multi=1.0). Newer UILabel tails store
        // an absolute menu-world text height.
        // GH2's compact BandButton stores a relative size beside its authored
        // component box.  The box height is the source font-height basis; the
        // atlas cap-height is only a UV/layout metric and must not become menu
        // world units (that made multiplayer outfit rows several times too
        // large). Modern UILabel tails already store absolute world height.
        float source_scale =
            tail.legacy_layout
                ? tail.text_size *
                      (tail.height > 0.0f ? tail.height : kTextScale * capH)
                : tail.text_size;
        const float max_native_width =
            tail.width_bound > 0.0f ? tail.width_bound * capH / source_scale
                                    : 0.0f;
        // UILabel::kFitJust (serialized value 2) fits the authored line into
        // its box; it does not word-wrap at RndText's retained wrap width.
        // GH2's Top Rockers title is the decisive source case: width=130,
        // wrap=400, fit=2, and retail renders TOP ROCKERS on one fitted line.
        const auto lines = tail.fit_text == 2
                               ? explicit_text_lines(disp)
                               : wrap_text_lines(font, disp, max_native_width);
        float fit_scale = 1.0f;
        // GH2's legacy BandButton always lays out a single authored line
        // inside the serialized component width, even when its older fit enum
        // is zero. Without this native component-box constraint long localized
        // outfit names such as LIBERTY SPIKES bleed out of the 110-unit panel.
        // Modern UILabel kFitJust uses the same width fit plus its height fit.
        if ((tail.fit_text == 2 || tail.legacy_layout) && !lines.empty()) {
          float max_line_world = 0.0f;
          for (const auto& line : lines) {
            max_line_world =
                std::max(max_line_world,
                         font.measure(line) / capH * source_scale);
          }
          const float leading = tail.leading > 0.0f ? tail.leading : 1.0f;
          const float block_height =
              source_scale *
              (1.0f + static_cast<float>(lines.size() - 1) * leading);
          if (tail.width > 0.0f && max_line_world > tail.width)
            fit_scale = std::min(fit_scale, tail.width / max_line_world);
          if (tail.fit_text == 2 && tail.height > 0.0f &&
              block_height > tail.height)
            fit_scale = std::min(fit_scale, tail.height / block_height);
        }
        const float draw_scale =
            source_scale * fit_scale * (foc ? kFocusScale : 1.0f);
        const float leading = tail.leading > 0.0f ? tail.leading : 1.0f;
        const float block_height =
            draw_scale * (1.0f +
                          static_cast<float>(lines.size() - 1) * leading);
        float first_line_z = block_height * 0.5f - draw_scale * 0.5f;
        if ((alignment & 0x10) != 0)
          first_line_z = -draw_scale * 0.5f;
        else if ((alignment & 0x40) != 0)
          first_line_z = block_height - draw_scale * 0.5f;
        for (std::size_t line_i = 0; line_i < lines.size(); ++line_i) {
          float line_w = 0.0f;
          const auto line_quads = font.layout(lines[line_i], &line_w);
          float x_offset = 0.0f;
          if ((alignment & 4) != 0)
            x_offset = -(line_w / capH) * draw_scale;
          else if ((alignment & 2) != 0)
            x_offset = -(line_w / capH) * draw_scale * 0.5f;
          const float line_z =
              first_line_z - static_cast<float>(line_i) * draw_scale * leading;
          emit(line_quads, [&](float qx, float qy, float u, float v) {
            const float lx = (qx / capH) * draw_scale + x_offset;
            const float lz =
                -((qy - capH * 0.5f) / capH) * draw_scale + line_z;
            TV tv{ax + lx * r0x + lz * r2x, ay,
                  az + lx * r0z + lz * r2z, u, v, col};
            return tv;
          });
        }
      }
    } else {
      // Text / BandLabel: BandLabel's serialized WorldXfm is the authored
      // parent-composed placement from the MILO. Use it when present so
      // parented menu titles (for example mem_card's `op_memcard`) land on the
      // poster instead of at their unparented local offset.
      const auto& xfm =
          ((lbl.type == "BandLabel" || lbl.type == "Text" || isTextEntry) &&
           label_world_on_text_plane(lbl))
              ? lbl.world
              : lbl.local;
      float source_scale = 1.0f;
      float fit_width_world = 0.0f;
      float fit_height_world = 0.0f;
      float wrap_width_world = 0.0f;
      float leading = 1.0f;
      int alignment = 34;
      int fit_text = 0;
      if ((lbl.type == "BandLabel" || lbl.type == "Text" || isTextEntry) &&
          lbl.text_tail.valid &&
          lbl.text_tail.text_size > 0.0f) {
        source_scale = lbl.text_tail.text_size;
        fit_width_world = lbl.text_tail.width;
        fit_height_world = lbl.text_tail.height;
        wrap_width_world = lbl.text_tail.width_bound > 0.0f
                               ? lbl.text_tail.width_bound
                               : fit_width_world;
        leading = lbl.text_tail.leading > 0.0f ? lbl.text_tail.leading : 1.0f;
        alignment = lbl.text_tail.alignment;
        fit_text = lbl.text_tail.fit_text;
      }
      const float max_native_width =
          wrap_width_world > 0.0f ? wrap_width_world * capH / source_scale : 0.0f;
      // Harmonix UILabel fit types are kFitWrap=0, kFitStretch=1 and
      // kFitJust=2. Stretch preserves authored lines and scales them as one
      // block. Justification still lays text out through RndText's wrap width,
      // then fits that wrapped block to the UILabel rectangle. Treating Just
      // as explicit-lines-only collapsed the multiline win-game contract into
      // a single tiny row because each paragraph measured as one huge line.
      const auto lines = fit_text == 1
                             ? explicit_text_lines(disp)
                             : wrap_text_lines(font, disp, max_native_width);
      float fit_scale = 1.0f;
      if ((fit_text == 1 || fit_text == 2) && !lines.empty()) {
        float max_line_world = 0.0f;
        for (const auto& line : lines)
          max_line_world =
              std::max(max_line_world, font.measure(line) / capH * source_scale);
        const float block_height =
            source_scale * (1.0f + static_cast<float>(lines.size() - 1) * leading);
        if (fit_width_world > 0.0f && max_line_world > fit_width_world)
          fit_scale = std::min(fit_scale, fit_width_world / max_line_world);
        if (fit_height_world > 0.0f && block_height > fit_height_world)
          fit_scale = std::min(fit_scale, fit_height_world / block_height);
      }
      const float draw_scale = source_scale * fit_scale;
      const float block_height =
          draw_scale * (1.0f +
                        static_cast<float>(lines.size() - 1) * leading);
      float first_line_z = block_height * 0.5f - draw_scale * 0.5f;
      if ((alignment & 0x10) != 0)       // RndText::kTop*
        first_line_z = -draw_scale * 0.5f;
      else if ((alignment & 0x40) != 0)  // RndText::kBottom*
        first_line_z = block_height - draw_scale * 0.5f;
      for (std::size_t line_i = 0; line_i < lines.size(); ++line_i) {
        float line_w = 0.0f;
        const auto line_quads = font.layout(lines[line_i], &line_w);
        float x_offset = 0.0f;
        if ((alignment & 4) != 0) {
          x_offset = -(line_w / capH) * draw_scale;
        } else if ((alignment & 2) != 0) {
          x_offset = -(line_w / capH) * draw_scale * 0.5f;
        }
        const float line_z =
            first_line_z - static_cast<float>(line_i) * draw_scale * leading;
        if (isTextEntry) {
          // BandTextEntry draws label_hand_pen.txt once per character from the
          // component origin; the resource RndText's center alignment is a
          // character template, not alignment for the completed name string.
          // Revision-3 stores black entered text, a red current character, and
          // a +20% current-character scale in the component's final 44 bytes.
          Object* live_entry = mgr.resolve_object(Symbol(lbl.name.c_str()));
          const bool editing =
              !live_entry ||
              !node_bool(live_entry->handle_property(Symbol("is_done"),
                                                      DataArray()));
          const std::size_t current =
              lines[line_i].empty() ? 0 : lines[line_i].size() - 1;
          std::string prefix;
          for (std::size_t char_i = 0; char_i < lines[line_i].size(); ++char_i) {
            const std::string glyph_text(1, lines[line_i][char_i]);
            float glyph_w = 0.0f;
            const auto glyph_quads = font.layout(glyph_text, &glyph_w);
            const float prefix_w = font.measure(prefix);
            const bool dynamic = editing && char_i == current;
            const float char_scale =
                draw_scale *
                (dynamic && lbl.text_entry_tail.valid
                     ? 1.0f + lbl.text_entry_tail.text_scale
                     : 1.0f);
            const float base_world_w = glyph_w / capH * draw_scale;
            const float scaled_world_w = glyph_w / capH * char_scale;
            const float char_x = prefix_w / capH * draw_scale -
                                 (scaled_world_w - base_world_w) * 0.5f;
            uint32_t char_col = col;
            if (lbl.text_entry_tail.valid) {
              char_col = pack_rgba_color(
                  dynamic ? lbl.text_entry_tail.dynamic_color
                          : lbl.text_entry_tail.entered_color);
            }
            emit(glyph_quads, [&](float qx, float qy, float u, float v) {
              const float ex = char_x + (qx / capH) * char_scale;
              const float ez =
                  -((qy - capH * 0.5f) / capH) * char_scale + line_z;
              const auto world = transform_menu_text_point(xfm, ex, ez);
              return TV{world[0], world[1], world[2], u, v, char_col};
            });
            prefix.push_back(lines[line_i][char_i]);
          }
          continue;
        }
        emit(line_quads, [&](float qx, float qy, float u, float v) {
          const float ex = (qx / capH) * draw_scale + x_offset;
          const float ez =
              -((qy - capH * 0.5f) / capH) * draw_scale + line_z;
          const auto world = transform_menu_text_point(xfm, ex, ez);
          TV tv{world[0], world[1], world[2], u, v, col};
          return tv;
        });
      }
    }
    if (out.size() > first_vertex) {
      ghogx::render::MiloSceneRenderer::TextTransformSpan span;
      span.first_vertex = first_vertex;
      span.vertex_count = out.size() - first_vertex;
      span.name = lbl.name;
      span.parent = lbl.parent;
      span.local = menu_xfm_to_mat4(lbl.local);
      span.bind_world = menu_xfm_to_mat4(lbl.has_world ? lbl.world : lbl.local);
      transform_spans.push_back(std::move(span));
    }
  }
}

void append_song_string(const std::string& text, const MenuFont& font, float x, float y,
                        float z, float scale, uint32_t col,
                        std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out,
                        float native_height = 0.0f) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  float w = 0.0f;
  auto quads = font.layout(text, &w);
  const float h = native_height > 0.0f ? native_height : font.cap_height();
  for (const auto& q : quads) {
    auto V = [&](float qx, float qy, float u, float v) {
      return TV{x + qx * scale, y, z - (qy - h * 0.5f) * scale, u, v, col};
    };
    TV a = V(q.x0, q.y0, q.u0, q.v0), b = V(q.x1, q.y0, q.u1, q.v0),
       c = V(q.x1, q.y1, q.u1, q.v1), d = V(q.x0, q.y1, q.u0, q.v1);
    out.push_back(a); out.push_back(b); out.push_back(c);
    out.push_back(a); out.push_back(c); out.push_back(d);
  }
}

struct LiveMenuAnimationSource {
  ObjectDir* panel = nullptr;
  std::string milo_path;
  std::vector<MenuSliderAnim> transform_anims;
  std::vector<MenuMaterialAnim> material_anims;
  std::vector<milo_scene::EnvAnimObj> environment_anims;
};

std::vector<LiveMenuAnimationSource> collect_live_menu_animation_sources(
    const std::string& hdr, const std::string& ark, ScreenManager& mgr,
    Object* screen) {
  std::vector<LiveMenuAnimationSource> out;
  if (!screen) return out;
  for (Symbol panel_name : screen_panel_names(screen)) {
    auto* panel = dynamic_cast<ObjectDir*>(mgr.find_object(panel_name));
    if (!panel) continue;
    const std::string path = menu_milo_path_for_file(panel_file(panel));
    if (path.empty()) continue;
    LiveMenuAnimationSource source;
    source.panel = panel;
    source.milo_path = path;
    source.transform_anims = extract_menu_transform_anims(hdr, ark, path);
    source.material_anims = extract_menu_material_anims(hdr, ark, path);
    milo_scene::Scene scene;
    if (milo_scene::load_scene(hdr, ark, path, scene))
      source.environment_anims = std::move(scene.env_anims);
    if (!source.transform_anims.empty() || !source.material_anims.empty() ||
        !source.environment_anims.empty())
      out.push_back(std::move(source));
  }
  return out;
}

std::array<float, 4> sample_menu_material_color(
    const std::vector<MenuMaterialColorKey>& keys, float frame) {
  if (keys.empty()) return {1.0f, 1.0f, 1.0f, 1.0f};
  const MenuMaterialColorKey* a = &keys.front();
  const MenuMaterialColorKey* b = &keys.back();
  for (std::size_t i = 1; i < keys.size(); ++i) {
    if (frame <= keys[i].frame) {
      a = &keys[i - 1];
      b = &keys[i];
      break;
    }
  }
  const float span = std::max(b->frame - a->frame, 0.001f);
  const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  std::array<float, 4> out{};
  for (int i = 0; i < 4; ++i)
    out[i] = a->color[i] + (b->color[i] - a->color[i]) * t;
  return out;
}

float sample_menu_material_float(const std::vector<MenuMaterialFloatKey>& keys,
                                 float frame) {
  if (keys.empty()) return 0.0f;
  const MenuMaterialFloatKey* a = &keys.front();
  const MenuMaterialFloatKey* b = &keys.back();
  for (std::size_t i = 1; i < keys.size(); ++i) {
    if (frame <= keys[i].frame) {
      a = &keys[i - 1];
      b = &keys[i];
      break;
    }
  }
  const float span = std::max(b->frame - a->frame, 0.001f);
  const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
  return a->value + (b->value - a->value) * t;
}

void apply_live_menu_animation_frames(
    const std::vector<LiveMenuAnimationSource>& sources,
    ghogx::render::MiloSceneRenderer& renderer) {
  using Renderer = ghogx::render::MiloSceneRenderer;
  std::map<std::string, Renderer::MeshTransformSample> transforms;
  std::map<std::string, std::array<float, 4>> colors;
  std::map<std::string, float> alphas;
  std::map<std::string, std::string> textures;
  std::map<std::string, Renderer::MaterialTexTransformSample> tex_transforms;
  std::map<std::string, std::array<float, 4>> environment_colors;
  std::map<std::string, Renderer::EnvironmentFogOverride> environment_fog;
  for (const LiveMenuAnimationSource& source : sources) {
    if (!source.panel) continue;
    for (const MenuSliderAnim& anim : source.transform_anims) {
      Object* live = source.panel->find(Symbol(anim.name));
      if (!live || anim.target.empty() ||
          !live->has_property(Symbol("frame")))
        continue;
      const float frame = live->get_property(Symbol("frame"))
                              .as_float().value_or(anim.first_frame);
      Renderer::MeshTransformSample sample;
      sample.has_source_frame = true;
      sample.source_frame = frame;
      if (!anim.translation_keys.empty()) {
        sample.has_translation = true;
        sample.translation_is_absolute = true;
        sample.translation = sample_menu_vec_value(anim.translation_keys, frame);
      }
      if (!anim.rotation_keys.empty()) {
        sample.has_rotation = true;
        sample.rotation_is_absolute = true;
        sample.rotation_xyzw = sample_menu_quat_value(anim.rotation_keys, frame);
      }
      if (!anim.scale_keys.empty()) {
        sample.has_scale = true;
        sample.scale_is_absolute = true;
        sample.scale = sample_menu_vec_value(anim.scale_keys, frame);
      }
      transforms[anim.target] = sample;
    }
    for (const MenuMaterialAnim& anim : source.material_anims) {
      Object* live = source.panel->find(Symbol(anim.name));
      if (!live || anim.material.empty() ||
          !live->has_property(Symbol("frame")))
        continue;
      const float frame = live->get_property(Symbol("frame"))
                              .as_float().value_or(anim.first_frame);
      if (!anim.color_keys.empty())
        colors[anim.material] =
            sample_menu_material_color(anim.color_keys, frame);
      if (!anim.alpha_keys.empty())
        alphas[anim.material] =
            sample_menu_material_float(anim.alpha_keys, frame);
      if (const std::string* texture =
              sample_material_texture_frame(anim.texture_keys, frame))
        textures[anim.material] = *texture;
      Renderer::MaterialTexTransformSample tex_sample;
      if (!anim.translation_keys.empty()) {
        const auto value = sample_menu_vec_value(anim.translation_keys, frame);
        tex_sample.has_translation = true;
        tex_sample.translation = {value[0], value[1]};
      }
      if (!anim.scale_keys.empty()) {
        const auto value = sample_menu_vec_value(anim.scale_keys, frame);
        tex_sample.has_scale = true;
        tex_sample.scale = {value[0], value[1]};
      }
      if (!anim.rotation_keys.empty()) {
        const auto value = sample_menu_vec_value(anim.rotation_keys, frame);
        tex_sample.has_rotation = true;
        tex_sample.rotation_radians = value[2];
      }
      if (tex_sample.has_translation || tex_sample.has_scale ||
          tex_sample.has_rotation)
        tex_transforms[anim.material] = tex_sample;
    }
    for (const milo_scene::EnvAnimObj& anim : source.environment_anims) {
      Object* live = source.panel->find(Symbol(anim.name));
      if (!live) continue;
      const float frame =
          live->get_property(Symbol("frame")).as_float().value_or(anim.frame);
      const std::string environment =
          anim.environment.empty() ? renderer.scene_panel_environment()
                                   : anim.environment;
      if (environment.empty()) continue;
      const auto sample_color = [frame](
                                    const std::vector<milo_scene::EnvAnimColorKey>& keys) {
        if (keys.empty()) return std::array<float, 4>{1, 1, 1, 1};
        const auto* a = &keys.front();
        const auto* b = &keys.back();
        for (size_t i = 1; i < keys.size(); ++i) {
          if (frame <= keys[i].frame) {
            a = &keys[i - 1];
            b = &keys[i];
            break;
          }
        }
        const float span = std::max(0.001f, b->frame - a->frame);
        const float t = std::clamp((frame - a->frame) / span, 0.0f, 1.0f);
        std::array<float, 4> value{};
        for (int component = 0; component < 4; ++component)
          value[component] = a->value[component] +
                             (b->value[component] - a->value[component]) * t;
        return value;
      };
      if (!anim.ambient_color_keys.empty())
        environment_colors[environment] = sample_color(anim.ambient_color_keys);
      auto& fog = environment_fog[environment];
      if (!anim.fog_color_keys.empty()) {
        fog.has_color = true;
        fog.color = sample_color(anim.fog_color_keys);
      }
      if (!anim.fog_range_keys.empty()) {
        const auto* a = &anim.fog_range_keys.front();
        const auto* b = &anim.fog_range_keys.back();
        for (size_t i = 1; i < anim.fog_range_keys.size(); ++i) {
          if (frame <= anim.fog_range_keys[i].frame) {
            a = &anim.fog_range_keys[i - 1];
            b = &anim.fog_range_keys[i];
            break;
          }
        }
        const float t = std::clamp(
            (frame - a->frame) / std::max(0.001f, b->frame - a->frame),
            0.0f, 1.0f);
        fog.has_range = true;
        for (int component = 0; component < 2; ++component)
          fog.range[component] = a->value[component] +
                                 (b->value[component] - a->value[component]) * t;
      }
    }
  }
  renderer.set_mesh_transform_offsets(std::move(transforms));
  renderer.set_material_color_overrides(std::move(colors));
  renderer.set_material_alpha_overrides(std::move(alphas));
  renderer.set_material_texture_overrides(std::move(textures));
  renderer.set_material_tex_transform_overrides(std::move(tex_transforms));
  renderer.set_environment_color_overrides(std::move(environment_colors));
  renderer.set_environment_fog_overrides(std::move(environment_fog));
}

void append_song_string_centered_z(
    const std::string& text, const MenuFont& font, float x, float y,
    float target_center_z, float scale, uint32_t col,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  float width = 0.0f;
  const auto quads = font.layout(text, &width);
  if (quads.empty()) return;
  float min_y = quads.front().y0;
  float max_y = quads.front().y1;
  for (const auto& q : quads) {
    min_y = std::min({min_y, q.y0, q.y1});
    max_y = std::max({max_y, q.y0, q.y1});
  }
  const float glyph_center_y = (min_y + max_y) * 0.5f;
  for (const auto& q : quads) {
    auto V = [&](float qx, float qy, float u, float v) {
      return TV{x + qx * scale, y,
                target_center_z - (qy - glyph_center_y) * scale,
                u, v, col};
    };
    TV a = V(q.x0, q.y0, q.u0, q.v0), b = V(q.x1, q.y0, q.u1, q.v0),
       c = V(q.x1, q.y1, q.u1, q.v1), d = V(q.x0, q.y1, q.u0, q.v1);
    out.push_back(a); out.push_back(b); out.push_back(c);
    out.push_back(a); out.push_back(c); out.push_back(d);
  }
}

ghogx::render::Mat4 footer_view_projection(
    const ghogx::render::OrbitCamera& cam) {
  using ghogx::render::Mat4;
  float eye[3] = {};
  cam.eye(eye);
  float result_at[3] = {};
  const float* at = cam.authored ? cam.authored_at : cam.target;
  const float* up = cam.authored ? cam.authored_up : nullptr;
  if (cam.result_frame.valid) {
    for (int axis = 0; axis < 3; ++axis) {
      result_at[axis] = cam.result_frame.position[axis] +
                        cam.result_frame.forward[axis] * 100.0f;
    }
    at = result_at;
    up = cam.result_frame.up;
  }
  Mat4 view = Mat4::look_at_lh(eye[0], eye[1], eye[2], at[0], at[1], at[2],
                               up ? up[0] : 0.0f, up ? up[1] : 0.0f,
                               up ? up[2] : 1.0f);
  // The menu renderer pins authored PS2 cameras to 4:3.  Aspect affects only
  // clip X, but using the same value here keeps this projection identical to
  // MiloSceneRenderer::draw_impl for both pillarboxed and native 4:3 output.
  constexpr float kPs2MenuCameraAspect = 4.0f / 3.0f;
  Mat4 projection = Mat4::perspective_lh(
      cam.fov, kPs2MenuCameraAspect, cam.near_z, cam.far_z);
  if (cam.result_frame.valid && cam.result_frame.has_custom_view) {
    for (int row = 0; row < 4; ++row)
      for (int col = 0; col < 4; ++col)
        view.m[row][col] = cam.result_frame.custom_view[row * 4 + col];
  }
  if (cam.result_frame.valid && cam.result_frame.has_custom_projection) {
    for (int row = 0; row < 4; ++row)
      for (int col = 0; col < 4; ++col)
        projection.m[row][col] =
            cam.result_frame.custom_projection[row * 4 + col];
  } else if ((cam.authored || cam.result_frame.valid) &&
             !(cam.result_frame.valid &&
               cam.result_frame.screen_offset_consumed)) {
    constexpr float kScreenOffsetToClip = 1.0f / 768.0f;
    projection.m[2][0] += cam.screen_offset[0] * kScreenOffsetToClip;
    projection.m[2][1] += cam.screen_offset[1] * kScreenOffsetToClip;
  }
  return view * projection;
}

std::optional<float> projected_vertical_center(
    const std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& verts,
    const ghogx::render::Mat4& view_projection, float z_offset = 0.0f) {
  if (verts.empty()) return std::nullopt;
  float min_y = 0.0f;
  float max_y = 0.0f;
  bool valid = false;
  for (const auto& vertex : verts) {
    const float z = vertex.z + z_offset;
    const float clip_y = vertex.x * view_projection.m[0][1] +
                         vertex.y * view_projection.m[1][1] +
                         z * view_projection.m[2][1] +
                         view_projection.m[3][1];
    const float clip_w = vertex.x * view_projection.m[0][3] +
                         vertex.y * view_projection.m[1][3] +
                         z * view_projection.m[2][3] +
                         view_projection.m[3][3];
    if (!std::isfinite(clip_y) || !std::isfinite(clip_w) ||
        std::abs(clip_w) < 1.0e-6f)
      continue;
    const float ndc_y = clip_y / clip_w;
    if (!std::isfinite(ndc_y)) continue;
    if (!valid) {
      min_y = max_y = ndc_y;
      valid = true;
    } else {
      min_y = std::min(min_y, ndc_y);
      max_y = std::max(max_y, ndc_y);
    }
  }
  if (!valid) return std::nullopt;
  return (min_y + max_y) * 0.5f;
}

void center_vertices_in_projected_footer(
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& verts,
    const ghogx::render::Mat4& view_projection, float target_center) {
  if (verts.empty()) return;
  float z_offset = 0.0f;
  // Perspective makes a world-Z offset slightly nonlinear when source meshes
  // and text sit at different depths.  Newton iteration solves the actual
  // projected midpoint instead of relying on an authored-screen pixel nudge.
  constexpr float kProbe = 0.25f;
  for (int iteration = 0; iteration < 8; ++iteration) {
    const auto center =
        projected_vertical_center(verts, view_projection, z_offset);
    if (!center) return;
    const float error = target_center - *center;
    if (std::abs(error) < 1.0e-6f) break;
    const auto probe =
        projected_vertical_center(verts, view_projection, z_offset + kProbe);
    if (!probe) return;
    const float derivative = (*probe - *center) / kProbe;
    if (!std::isfinite(derivative) || std::abs(derivative) < 1.0e-7f) return;
    z_offset += std::clamp(error / derivative, -20.0f, 20.0f);
  }
  if (!std::isfinite(z_offset)) return;
  for (auto& vertex : verts) vertex.z += z_offset;
}

void append_image_quad(float x, float y, float z, float w, float h, uint32_t col,
                       std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  const float x0 = x - w * 0.5f, x1 = x + w * 0.5f;
  const float z0 = z - h * 0.5f, z1 = z + h * 0.5f;
  TV a{x0, y, z1, 0.0f, 0.0f, col}, b{x1, y, z1, 1.0f, 0.0f, col};
  TV c{x1, y, z0, 1.0f, 1.0f, col}, d{x0, y, z0, 0.0f, 1.0f, col};
  out.push_back(a); out.push_back(b); out.push_back(c);
  out.push_back(a); out.push_back(c); out.push_back(d);
}

void append_image_quad_uv(float x, float y, float z, float w, float h,
                          float u0, float v0, float u1, float v1, uint32_t col,
                          std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  const float x0 = x - w * 0.5f, x1 = x + w * 0.5f;
  const float z0 = z - h * 0.5f, z1 = z + h * 0.5f;
  TV a{x0, y, z1, u0, v0, col}, b{x1, y, z1, u1, v0, col};
  TV c{x1, y, z0, u1, v1, col}, d{x0, y, z0, u0, v1, col};
  out.push_back(a); out.push_back(b); out.push_back(c);
  out.push_back(a); out.push_back(c); out.push_back(d);
}

const milo_scene::MeshObj* find_decoded_mesh(const milo_scene::Scene& scene,
                                             const char* mesh_name) {
  for (const auto& mesh : scene.meshes) {
    if (mesh.name == mesh_name && mesh.decoded) return &mesh;
  }
  return nullptr;
}

float mesh_world_pos_or(const milo_scene::Scene& scene, const char* mesh_name,
                        int axis, float fallback) {
  const milo_scene::MeshObj* mesh = find_decoded_mesh(scene, mesh_name);
  if (!mesh || axis < 0 || axis > 2) return fallback;
  return mesh->world_stored.pos[axis];
}

struct MeshWorldBounds {
  float min_x = 0.0f;
  float max_x = 0.0f;
  float min_z = 0.0f;
  float max_z = 0.0f;
  bool valid = false;
};

MeshWorldBounds mesh_world_bounds(const milo_scene::MeshObj& mesh) {
  MeshWorldBounds out;
  for (const auto& v : mesh.verts) {
    const float x = v.px * mesh.world_stored.rot[0][0] +
                    v.py * mesh.world_stored.rot[1][0] +
                    v.pz * mesh.world_stored.rot[2][0] +
                    mesh.world_stored.pos[0];
    const float z = v.px * mesh.world_stored.rot[0][2] +
                    v.py * mesh.world_stored.rot[1][2] +
                    v.pz * mesh.world_stored.rot[2][2] +
                    mesh.world_stored.pos[2];
    if (!out.valid) {
      out.min_x = out.max_x = x;
      out.min_z = out.max_z = z;
      out.valid = true;
    } else {
      out.min_x = std::min(out.min_x, x);
      out.max_x = std::max(out.max_x, x);
      out.min_z = std::min(out.min_z, z);
      out.max_z = std::max(out.max_z, z);
    }
  }
  return out;
}

uint32_t representative_mesh_color(const milo_scene::MeshObj* mesh,
                                   const milo_scene::MatObj* mat,
                                   uint32_t fallback) {
  if (!mesh || mesh->verts.empty()) return fallback;
  return pack_milo_vertex_color(mesh->verts.front(), mat, fallback);
}

void append_helpbar_mesh_quad_centered_z(
    const milo_scene::Scene& scene, const char* mesh_name,
    float target_x, float target_center_z, uint32_t col,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  const milo_scene::MeshObj* mesh = find_decoded_mesh(scene, mesh_name);
  if (!mesh || mesh->indices.empty()) return;
  const auto& world = mesh->world_stored;
  const MeshWorldBounds bounds = mesh_world_bounds(*mesh);
  if (!bounds.valid) return;
  const float x_offset = target_x - world.pos[0];
  const float source_center_z = (bounds.min_z + bounds.max_z) * 0.5f;
  const float z_offset = target_center_z - source_center_z;
  auto vertex = [&](uint16_t index) {
    const auto& v = mesh->verts[index];
    const float x = v.px * world.rot[0][0] + v.py * world.rot[1][0] +
                    v.pz * world.rot[2][0] + world.pos[0] + x_offset;
    const float y = v.px * world.rot[0][1] + v.py * world.rot[1][1] +
                    v.pz * world.rot[2][1] + world.pos[1];
    const float z = v.px * world.rot[0][2] + v.py * world.rot[1][2] +
                    v.pz * world.rot[2][2] + world.pos[2] + z_offset;
    return TV{x, y, z, v.u, v.v, col};
  };
  for (uint16_t index : mesh->indices) out.push_back(vertex(index));
}

void append_helpbar_mesh_quad_in_bounds(
    const milo_scene::Scene& scene, const char* mesh_name, float target_min_x,
    float target_max_x, float target_min_z, float target_max_z, uint32_t col,
    const milo_scene::MatObj* mat,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  const milo_scene::MeshObj* mesh = find_decoded_mesh(scene, mesh_name);
  if (!mesh || mesh->indices.empty()) return;
  const MeshWorldBounds src = mesh_world_bounds(*mesh);
  const float src_w = src.max_x - src.min_x;
  const float src_h = src.max_z - src.min_z;
  if (!src.valid || src_w == 0.0f || src_h == 0.0f) return;
  auto vertex = [&](uint16_t index) {
    const auto& v = mesh->verts[index];
    const float src_x = v.px * mesh->world_stored.rot[0][0] +
                        v.py * mesh->world_stored.rot[1][0] +
                        v.pz * mesh->world_stored.rot[2][0] +
                        mesh->world_stored.pos[0];
    const float src_z = v.px * mesh->world_stored.rot[0][2] +
                        v.py * mesh->world_stored.rot[1][2] +
                        v.pz * mesh->world_stored.rot[2][2] +
                        mesh->world_stored.pos[2];
    const float tx = (src_x - src.min_x) / src_w;
    const float tz = (src_z - src.min_z) / src_h;
    return TV{target_min_x + (target_max_x - target_min_x) * tx,
              0.0f,
              target_min_z + (target_max_z - target_min_z) * tz,
              v.u,
              v.v,
              pack_milo_vertex_color(v, mat, col)};
  };
  for (uint16_t index : mesh->indices) out.push_back(vertex(index));
}

std::array<float, 16> mat4_from_xfm12(const std::array<float, 12>& x) {
  std::array<float, 16> m{};
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) m[r * 4 + c] = x[r * 3 + c];
  m[12] = x[9];
  m[13] = x[10];
  m[14] = x[11];
  m[15] = 1.0f;
  return m;
}

std::array<float, 16> mat4_mul(const std::array<float, 16>& a,
                               const std::array<float, 16>& b) {
  std::array<float, 16> r{};
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      float s = 0.0f;
      for (int k = 0; k < 4; ++k) s += a[row * 4 + k] * b[k * 4 + col];
      r[row * 4 + col] = s;
    }
  }
  return r;
}

ghogx::render::MiloSceneRenderer::TextVertex checkbox_vertex(
    const milo_scene::Vertex& v, const std::array<float, 16>& world,
    uint32_t col) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  const float x = v.px * world[0] + v.py * world[4] + v.pz * world[8] + world[12];
  const float y = v.px * world[1] + v.py * world[5] + v.pz * world[9] + world[13];
  const float z = v.px * world[2] + v.py * world[6] + v.pz * world[10] + world[14];
  return TV{x, y, z, v.u, v.v, col};
}

ghogx::render::MiloSceneRenderer::TextVertex mesh_overlay_vertex(
    const milo_scene::Vertex& v, const milo_scene::MatObj* mat,
    const std::array<float, 16>& world, uint32_t col) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  const float x = v.px * world[0] + v.py * world[4] + v.pz * world[8] + world[12];
  const float y = v.px * world[1] + v.py * world[5] + v.pz * world[9] + world[13];
  const float z = v.px * world[2] + v.py * world[6] + v.pz * world[10] + world[14];
  float u = v.u;
  float vv = v.v;
  if (mat) {
    u = v.u * mat->tex_xfm[0][0] + v.v * mat->tex_xfm[1][0] +
        mat->tex_xfm[2][0];
    vv = v.u * mat->tex_xfm[0][1] + v.v * mat->tex_xfm[1][1] +
         mat->tex_xfm[2][1];
  }
  return TV{x, y, z, u, vv, col};
}

bool node_bool(const DataNode& node) {
  if (auto i = node.as_int()) return *i != 0;
  if (auto f = node.as_float()) return *f != 0.0f;
  if (auto s = node.as_string())
    return !(*s == "FALSE" || *s == "false" || *s == "0" || s->empty());
  return node.as_object() != nullptr;
}

bool live_showing(ScreenManager& mgr, const std::string& name,
                  bool authored_showing) {
  if (name.empty()) return authored_showing;
  if (Object* obj = mgr.resolve_object(Symbol(name.c_str()))) {
    if (obj->has_property(Symbol("showing")))
      return node_bool(obj->get_property(Symbol("showing")));
  }
  return authored_showing;
}

bool live_element_showing(ScreenManager& mgr, const std::string& name,
                          const std::string& parent,
                          bool authored_showing) {
  if (!live_showing(mgr, name, authored_showing)) return false;
  return live_showing(mgr, parent, true);
}

bool checkbox_checked(ScreenManager& mgr, const MenuCheckbox& checkbox) {
  if (Object* obj = mgr.resolve_object(Symbol(checkbox.name.c_str()))) {
    if (obj->has_property(Symbol("checked")))
      return node_bool(obj->get_property(Symbol("checked")));
  }
  return checkbox.checked;
}

void append_checkbox_widgets(
    ScreenManager& mgr, const std::vector<MenuCheckbox>& checkboxes,
    const milo_scene::Scene& checkbox_scene,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& on_verts,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& off_verts,
    std::vector<ghogx::render::MiloSceneRenderer::TextTransformSpan>& on_spans,
    std::vector<ghogx::render::MiloSceneRenderer::TextTransformSpan>& off_spans) {
  const milo_scene::MeshObj* mesh = nullptr;
  for (const auto& m : checkbox_scene.meshes) {
    if (m.name == "checkbox_toggle.mesh" && m.decoded) {
      mesh = &m;
      break;
    }
  }
  if (!mesh) return;
  for (const MenuCheckbox& checkbox : checkboxes) {
    if (!live_element_showing(mgr, checkbox.name, checkbox.parent,
                              checkbox.showing) ||
        !checkbox.has_world)
      continue;
    const bool checked = checkbox_checked(mgr, checkbox);
    auto& verts = checked ? on_verts : off_verts;
    auto& spans = checked ? on_spans : off_spans;
    const size_t first_vertex = verts.size();
    // GH2's checkbox resource has separate on/off textures sharing one toggle
    // mesh. The component already carries the 0.4 scale and X/Z-plane
    // orientation, so composing the resource mesh's stored world rows again
    // double-rotates it.
    const std::array<float, 16> world = mat4_from_xfm12(checkbox.world);
    for (std::size_t i = 0; i + 2 < mesh->indices.size(); i += 3) {
      verts.push_back(checkbox_vertex(mesh->verts[mesh->indices[i + 0]],
                                      world, 0xFFFFFFFFu));
      verts.push_back(checkbox_vertex(mesh->verts[mesh->indices[i + 1]],
                                      world, 0xFFFFFFFFu));
      verts.push_back(checkbox_vertex(mesh->verts[mesh->indices[i + 2]],
                                      world, 0xFFFFFFFFu));
    }
    ghogx::render::MiloSceneRenderer::TextTransformSpan span;
    span.first_vertex = first_vertex;
    span.vertex_count = verts.size() - first_vertex;
    span.name = checkbox.name;
    span.parent = checkbox.parent;
    span.local = checkbox.has_local ? mat4_from_xfm12(checkbox.local)
                                    : identity_mat4();
    span.bind_world = world;
    spans.push_back(std::move(span));
  }
}

int live_slider_int(ScreenManager& mgr, const MenuSlider& slider,
                    const char* prop, int fallback) {
  if (Object* obj = mgr.resolve_object(Symbol(slider.name.c_str()))) {
    DataNode node = obj->handle_property(Symbol(prop), DataArray());
    if (auto i = node.as_int()) return *i;
    if (obj->has_property(Symbol(prop))) {
      node = obj->get_property(Symbol(prop));
      if (auto i = node.as_int()) return *i;
    }
  }
  return fallback;
}

float live_slider_frame(ScreenManager& mgr, const MenuSlider& slider) {
  int steps = live_slider_int(mgr, slider, "num_steps", slider.num_steps);
  int current = live_slider_int(mgr, slider, "current", slider.current);
  if (steps < 1) steps = 1;
  current = std::clamp(current, 0, steps - 1);
  if (steps == 1) return 0.0f;
  return static_cast<float>(current) / static_cast<float>(steps - 1);
}

std::string slider_material_for(const MenuSlider& slider,
                                const milo_scene::MeshObj& mesh,
                                bool focused) {
  std::string resource = slider.resource.empty() ? "char" : slider.resource;
  if (mesh.name == "char_slider_pod.mesh")
    return resource + (focused ? "_slider_pod_focus.mat" : "_slider_pod.mat");
  if (mesh.name == "char_slider.mesh")
    return resource + (focused ? "_slider_focus.mat" : "_slider_default.mat");
  return mesh.material;
}

std::array<float, 16> slider_resource_world(
    const milo_scene::Scene& scene, const milo_scene::MeshObj& mesh,
    const MenuSliderAnim& anim, float frame) {
  std::array<float, 16> world = scene.world_matrix(mesh);
  if (!anim.valid || mesh.name != anim.target) return world;

  const float denom = anim.last_frame - anim.first_frame;
  float t = denom == 0.0f ? 0.0f : (frame - anim.first_frame) / denom;
  t = std::clamp(t, 0.0f, 1.0f);
  world[12] = anim.first[0] + (anim.last[0] - anim.first[0]) * t;
  world[13] = anim.first[1] + (anim.last[1] - anim.first[1]) * t;
  world[14] = anim.first[2] + (anim.last[2] - anim.first[2]) * t;
  return world;
}

void append_slider_token_labels(
    ScreenManager& mgr, const std::string& focused,
    const std::unordered_set<std::string>& disabled,
    const std::vector<MenuSlider>& sliders, const MenuFont& font,
    const std::map<std::string, std::string>& locale,
    std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out,
    std::vector<ghogx::render::MiloSceneRenderer::TextTransformSpan>& spans) {
  for (const MenuSlider& slider : sliders) {
    if (slider.token.empty() || !slider.has_world)
      continue;
    if (!live_element_showing(mgr, slider.name, slider.parent, slider.showing))
      continue;

    uint32_t color = kResolvedColNormal;
    const int live_state = live_component_state_code(mgr, slider.name);
    if (disabled.count(slider.name) || live_state == 2)
      color = kColDisabled;
    else if (live_state == 3)
      color = kResolvedColSelecting;
    else if (slider.name == focused)
      color = kResolvedColFocused;

    const std::string text =
        display_text_from_node(DataNode::Str(slider.token), locale);
    if (text.empty()) continue;

    // BandSlider serializes the label/config token inside the component rather
    // than as a sibling BandLabel. Anchor the label to the same authored world
    // row used by the slider resource geometry.
    constexpr float kSliderTokenXOffset = -125.0f;
    constexpr float kSliderTokenZOffset = 2.0f;
    constexpr float kSliderTokenScale = kTextScale;
    const size_t first_vertex = out.size();
    append_song_string(text, font, slider.world[9] + kSliderTokenXOffset,
                       slider.world[10],
                       slider.world[11] + kSliderTokenZOffset,
                       kSliderTokenScale, color, out);
    ghogx::render::MiloSceneRenderer::TextTransformSpan span;
    span.first_vertex = first_vertex;
    span.vertex_count = out.size() - first_vertex;
    span.name = slider.name;
    span.parent = slider.parent;
    span.local = slider.has_local ? mat4_from_xfm12(slider.local)
                                  : identity_mat4();
    span.bind_world = mat4_from_xfm12(slider.world);
    spans.push_back(std::move(span));
  }
}

void append_slider_widgets(
    ScreenManager& mgr, const std::string& focused,
    const std::vector<MenuSlider>& sliders,
    const milo_scene::Scene& slider_scene, const MenuSliderAnim& slider_anim,
    const std::map<std::string, asset::Image>& slider_textures,
    std::vector<ghogx::render::MiloSceneRenderer::TextBatch>& batches) {
  for (const MenuSlider& slider : sliders) {
    if (!live_element_showing(mgr, slider.name, slider.parent, slider.showing) ||
        !slider.has_world)
      continue;
    const bool is_focused = slider.name == focused;
    const std::array<float, 16> widget_world = mat4_from_xfm12(slider.world);
    const float frame = live_slider_frame(mgr, slider);

    for (const auto& mesh : slider_scene.meshes) {
      if (!mesh.decoded || mesh.verts.empty() || mesh.indices.empty())
        continue;
      if (mesh.name != "char_slider.mesh" &&
          mesh.name != "char_slider_pod.mesh")
        continue;

      const std::string material_name =
          slider_material_for(slider, mesh, is_focused);
      const milo_scene::MatObj* mat = slider_scene.find_mat(material_name);
      if (!mat || mat->diffuse_tex.empty()) continue;
      auto tex_it = slider_textures.find(mat->diffuse_tex);
      if (tex_it == slider_textures.end() || !tex_it->second.valid())
        continue;

      std::vector<ghogx::render::MiloSceneRenderer::TextVertex> verts;
      const std::array<float, 16> resource_world =
          slider_resource_world(slider_scene, mesh, slider_anim, frame);
      const std::array<float, 16> world =
          mat4_mul(resource_world, widget_world);
      const uint32_t color = pack_rgba_color(
          std::array<float, 4>{{mat->color[0], mat->color[1], mat->color[2],
                                mat->color[3]}});
      for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        verts.push_back(mesh_overlay_vertex(mesh.verts[mesh.indices[i + 0]], mat,
                                            world, color));
        verts.push_back(mesh_overlay_vertex(mesh.verts[mesh.indices[i + 1]], mat,
                                            world, color));
        verts.push_back(mesh_overlay_vertex(mesh.verts[mesh.indices[i + 2]], mat,
                                            world, color));
      }
      ghogx::render::MiloSceneRenderer::TextBatch batch;
      batch.verts = std::move(verts);
      batch.atlas = &tex_it->second;
      ghogx::render::MiloSceneRenderer::TextTransformSpan span;
      span.first_vertex = 0;
      span.vertex_count = batch.verts.size();
      span.name = slider.name;
      span.parent = slider.parent;
      span.local = slider.has_local ? mat4_from_xfm12(slider.local)
                                    : identity_mat4();
      span.bind_world = widget_world;
      batch.transform_spans.push_back(std::move(span));
      batches.push_back(std::move(batch));
    }
  }
}

asset::Image ink_alpha_image(asset::Image img) {
  if (!img.valid()) return img;
  for (std::size_t i = 0; i + 3 < img.rgba.size(); i += 4) {
    const int r = img.rgba[i + 0];
    const int g = img.rgba[i + 1];
    const int b = img.rgba[i + 2];
    const int a = img.rgba[i + 3];
    const int luma = (77 * r + 150 * g + 29 * b) >> 8;
    const int ink = std::clamp((185 - luma) * 4, 0, 255);
    img.rgba[i + 3] = static_cast<std::uint8_t>(std::min(a, ink));
  }
  return img;
}

struct HelpItem {
  std::string control;
  std::string token;
};

struct HelpbarSpacing {
  float button_spacing = 35.0f;
  float strumbar_spacing = 70.0f;
  float text_spacing = 30.0f;
};

float helpbar_prop_float(Object* helpbar, const char* key, float fallback) {
  if (!helpbar) return fallback;
  auto value = helpbar->get_property(Symbol(key)).as_float();
  if (!value || *value <= 0.0f) return fallback;
  return *value;
}

HelpbarSpacing helpbar_spacing_from_panel(Object* helpbar) {
  HelpbarSpacing out;
  out.button_spacing =
      helpbar_prop_float(helpbar, "button_spacing", out.button_spacing);
  out.strumbar_spacing =
      helpbar_prop_float(helpbar, "strumbar_spacing", out.strumbar_spacing);
  out.text_spacing =
      helpbar_prop_float(helpbar, "text_spacing", out.text_spacing);
  return out;
}

void collect_help_tokens(const DataNode& n, std::vector<HelpItem>& out) {
  auto arr = n.as_array();
  if (!arr) return;
  if (arr->size() == 2) {
    auto control = arr->at(0).as_symbol();
    auto token = arr->at(1).as_symbol();
    if (control && token) {
      const char* c = control->c_str();
      if (std::strcmp(c, "fret1") == 0 || std::strcmp(c, "fret2") == 0 ||
          std::strcmp(c, "fret3") == 0 || std::strcmp(c, "strum") == 0 ||
          std::strcmp(c, "start") == 0) {
        out.push_back({control->c_str(), token->c_str()});
        return;
      }
    }
  }
  for (std::size_t i = 0; i < arr->size(); ++i) collect_help_tokens(arr->at(i), out);
}

DataNode focused_help_display(const DataNode& display,
                              const std::string& focused) {
  auto rows = display.as_array();
  if (!rows) return display;
  DataNode fallback;
  DataNode selected;
  bool has_focus_branches = false;
  for (std::size_t i = 0; i < rows->size(); ++i) {
    auto row = rows->at(i).as_array();
    if (!row || row->size() != 2 || !row->at(1).as_array()) continue;
    auto key = row->at(0).as_symbol();
    if (!key) continue;
    const char* name = key->c_str();
    // A direct display is itself a list of (control token) pairs. Only the
    // screen-level (default (...))/(component (...)) form is focus-keyed.
    if (std::strcmp(name, "fret1") == 0 || std::strcmp(name, "fret2") == 0 ||
        std::strcmp(name, "fret3") == 0 || std::strcmp(name, "strum") == 0 ||
        std::strcmp(name, "start") == 0)
      continue;
    has_focus_branches = true;
    if (std::strcmp(name, "default") == 0) fallback = row->at(1);
    if (!focused.empty() && focused == name) selected = row->at(1);
  }
  if (!selected.empty()) return selected;
  if (!fallback.empty()) return fallback;
  return has_focus_branches ? DataNode() : display;
}

std::string help_label(const std::string& token,
                       const std::map<std::string, std::string>& locale) {
  if (auto it = locale.find(token); it != locale.end()) return it->second;
  const std::string ps2_key = token + "_ps2";
  if (auto it = locale.find(ps2_key); it != locale.end()) return it->second;
  return token;
}

void append_help_footer(Object* screen, const MenuFont& font,
                        const MenuTextStyle& source_style,
                        ScreenManager& mgr,
                        const std::string& focused,
                        const ghogx::render::OrbitCamera& camera,
                        const std::map<std::string, std::string>& locale,
                        const std::map<std::string, asset::Image>& icons,
                        const milo_scene::Scene& helpbar_scene,
                        std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out,
                        std::vector<ghogx::render::MiloSceneRenderer::TextBatch>& batches) {
  if (!screen) return;
  if (!screen_has_panel(screen, Symbol("helpbar"))) return;

  std::vector<HelpItem> items;
  DataNode display;
  Object* helpbar_obj = mgr.resolve_object(Symbol("helpbar"));
  if (helpbar_obj) display = helpbar_obj->get_property(Symbol("display"));
  if (display.empty()) display = screen->get_property(Symbol("helpbar"));
  display = focused_help_display(display, focused);
  collect_help_tokens(display, items);
  if (items.empty()) return;

  // `helpbar.milo_ps2` owns the shared panel geometry; `splash.dtb` owns the
  // HelpBarPanel spacing knobs. Keep the dynamic text/token choice per screen,
  // but anchor placement to those global source values.
  constexpr float kFooterY = 0.0f;
  constexpr const char* kFretIconMesh = "help_bar_starting.mesh";
  constexpr const char* kFretTemplateMesh = "help_bar.mesh";
  constexpr const char* kStrumIconMesh = "help_bar_strum.mesh";
  constexpr const char* kStrumbarAnchorMesh = "help_bar_strumbar.mesh";
  const float first_icon_left =
      mesh_world_pos_or(helpbar_scene, kFretIconMesh, 0, -294.915f);
  const float strum_icon_left =
      mesh_world_pos_or(helpbar_scene, kStrumbarAnchorMesh, 0, 90.699f);
  const float kFooterZ =
      mesh_world_pos_or(helpbar_scene, kFretIconMesh, 2, -205.0f);
  const float footer_box_z = kFooterZ + 4.0f;
  const auto help_prop_int = [&](const char* name, int fallback) {
    if (!helpbar_obj) return fallback;
    return helpbar_obj->get_property(Symbol(name)).as_int().value_or(fallback);
  };
  constexpr float kDefaultTextSpacing = 30.0f;
  const HelpbarSpacing spacing = helpbar_spacing_from_panel(helpbar_obj);
  const int max_labels = help_prop_int("max_labels", 4);
  const int max_buttons = help_prop_int("max_buttons", max_labels);
  int max_items = max_labels;
  if (max_buttons > 0)
    max_items = max_items > 0 ? std::min(max_items, max_buttons) : max_buttons;
  if (max_items > 0 && items.size() > static_cast<std::size_t>(max_items))
    items.resize(static_cast<std::size_t>(max_items));
  const float source_size =
      source_style.valid && source_style.text_size > 0.0f
          ? source_style.text_size
          : 18.0f;
  const float kFooterScale = source_size / std::max(1.0f, font.cap_height());
  const uint32_t kFooterCol =
      source_style.valid ? pack_rgba_color(source_style.color)
                         : 0xFFE6E6E6u;
  const ghogx::render::Mat4 footer_projection =
      footer_view_projection(camera);
  constexpr float kPs2FretSlotWorldPerButtonSpacing = 135.0f / 35.0f;
  const float fret_slot_step =
      spacing.button_spacing * kPs2FretSlotWorldPerButtonSpacing;
  const float second_icon_left = first_icon_left + fret_slot_step;
  const float third_icon_left = first_icon_left + fret_slot_step * 2.0f;
  auto icon_left_for_control = [&](const std::string& control) {
    if (control == "fret1" || control == "start") return first_icon_left;
    if (control == "fret2") return second_icon_left;
    if (control == "fret3") return third_icon_left;
    if (control == "strum") return strum_icon_left;
    return first_icon_left;
  };
  auto text_x_for_control = [&](const std::string& control) {
    // HelpBarPanel populates fixed widget slots; the small residuals below are
    // from the settled PS2-populated footer boxes after applying help_bar.txt.
    constexpr float kFirstFretTextResidual = 5.915f;
    constexpr float kMiddleFretTextResidual = 7.582f;
    constexpr float kStrumTextResidual = 4.301f;
    if (control == "fret1" || control == "start")
      return first_icon_left + spacing.text_spacing + kFirstFretTextResidual;
    if (control == "fret2")
      return second_icon_left + spacing.text_spacing + kMiddleFretTextResidual;
    if (control == "fret3")
      return third_icon_left + spacing.text_spacing + kMiddleFretTextResidual;
    if (control == "strum")
      return strum_icon_left + spacing.strumbar_spacing + kStrumTextResidual;
    return first_icon_left + kDefaultTextSpacing;
  };
  auto box_right_padding_for_control = [](const std::string& control) {
    // ihatecompvir's RB2 HelpBarPanel layout exposes fixed mWidgetXPos slots;
    // GH2 PS2 serializes only the public spacing knobs, so the strum/fat-bar
    // widget's wider right edge is trace-derived from the native footer shape.
    return control == "strum" ? 22.0f : 14.0f;
  };
  auto tex_for_control = [](const std::string& control) -> const char* {
    if (control == "fret1") return "hb_fret1.tex";
    if (control == "fret2") return "hb_fret2.tex";
    if (control == "fret3") return "hb_fret3.tex";
    if (control == "strum") return "hb_strum.tex";
    if (control == "start") return "hb_start.tex";
    return "";
  };
  for (const HelpItem& item : items) {
    const bool strum = item.control == "strum";
    const float icon_left = icon_left_for_control(item.control);
    const float icon_w = strum ? 64.0f : 32.0f;
    const float icon_h = 32.0f;
    const float icon_x = icon_left + icon_w * 0.5f;
    const float x = text_x_for_control(item.control);
    const std::string label = help_label(item.token, locale);
    float label_w = 0.0f;
    font.layout(label, &label_w);
    label_w *= kFooterScale;
    const float box_left = std::min(icon_x - icon_w * 0.5f, x) - 2.0f;
    const float box_right =
        std::max(icon_x + icon_w * 0.5f, x + label_w) +
        box_right_padding_for_control(item.control);
    const float box_x = (box_left + box_right) * 0.5f;
    const float box_w = box_right - box_left;
    auto mid_it = icons.find("help_box_mid.tex");
    auto cap_it = icons.find("help_box_corner.tex");
    std::optional<float> projected_box_center;
    if (mid_it != icons.end() && mid_it->second.valid() &&
        cap_it != icons.end() && cap_it->second.valid()) {
      constexpr const char* kBoxLeftMesh = "help_box_left.mesh";
      constexpr const char* kBoxRightMesh = "help_box_right.mesh";
      constexpr const char* kBoxMidMesh = "help_box_mid.mesh";
      const milo_scene::MeshObj* left_mesh =
          find_decoded_mesh(helpbar_scene, kBoxLeftMesh);
      const milo_scene::MeshObj* right_mesh =
          find_decoded_mesh(helpbar_scene, kBoxRightMesh);
      const milo_scene::MeshObj* mid_mesh =
          find_decoded_mesh(helpbar_scene, kBoxMidMesh);
      const milo_scene::MatObj* left_mat =
          left_mesh ? helpbar_scene.find_mat(left_mesh->material) : nullptr;
      const milo_scene::MatObj* right_mat =
          right_mesh ? helpbar_scene.find_mat(right_mesh->material) : nullptr;
      const milo_scene::MatObj* mid_mat =
          mid_mesh ? helpbar_scene.find_mat(mid_mesh->material) : nullptr;
      const MeshWorldBounds left_bounds =
          left_mesh ? mesh_world_bounds(*left_mesh) : MeshWorldBounds{};
      const MeshWorldBounds right_bounds =
          right_mesh ? mesh_world_bounds(*right_mesh) : MeshWorldBounds{};
      const MeshWorldBounds mid_bounds =
          mid_mesh ? mesh_world_bounds(*mid_mesh) : MeshWorldBounds{};
      const float box_z = footer_box_z;
      const float left_cap_w =
          left_bounds.valid ? left_bounds.max_x - left_bounds.min_x : 8.0f;
      const float right_cap_w =
          right_bounds.valid ? right_bounds.max_x - right_bounds.min_x : 8.0f;
      const float box_h =
          mid_bounds.valid ? mid_bounds.max_z - mid_bounds.min_z : 40.0f;
      const float box_min_z = box_z - box_h * 0.5f;
      const float box_max_z = box_z + box_h * 0.5f;
      std::vector<ghogx::render::MiloSceneRenderer::TextVertex> cap_verts;
      append_helpbar_mesh_quad_in_bounds(helpbar_scene, kBoxLeftMesh, box_left,
                                         box_left + left_cap_w, box_min_z,
                                         box_max_z, 0xFFFFFFFFu, left_mat,
                                         cap_verts);
      append_helpbar_mesh_quad_in_bounds(helpbar_scene, kBoxRightMesh,
                                         box_right - right_cap_w, box_right,
                                         box_min_z, box_max_z, 0xFFFFFFFFu,
                                         right_mat, cap_verts);
      std::vector<ghogx::render::MiloSceneRenderer::TextVertex> mid_verts;
      const float mid_w = std::max(1.0f, box_w - left_cap_w - right_cap_w);
      const uint32_t mid_color =
          representative_mesh_color(mid_mesh, mid_mat, 0xFFFFFFFFu);
      append_image_quad(box_x, kFooterY, box_z, mid_w, box_h, mid_color,
                        mid_verts);
      std::vector<ghogx::render::MiloSceneRenderer::TextVertex>
          background_verts = cap_verts;
      background_verts.insert(background_verts.end(), mid_verts.begin(),
                              mid_verts.end());
      projected_box_center =
          projected_vertical_center(background_verts, footer_projection);
      ghogx::render::MiloSceneRenderer::TextBatch cap_batch;
      cap_batch.verts = std::move(cap_verts);
      cap_batch.atlas = &cap_it->second;
      batches.push_back(std::move(cap_batch));
      ghogx::render::MiloSceneRenderer::TextBatch mid_batch;
      mid_batch.verts = std::move(mid_verts);
      mid_batch.atlas = &mid_it->second;
      batches.push_back(std::move(mid_batch));
    }
    const char* tex = tex_for_control(item.control);
    if (auto it = icons.find(tex); it != icons.end() && it->second.valid()) {
      std::vector<ghogx::render::MiloSceneRenderer::TextVertex> icon_verts;
      append_helpbar_mesh_quad_centered_z(
          helpbar_scene, strum ? kStrumIconMesh : kFretTemplateMesh,
          icon_left, footer_box_z, 0xFFFFFFFFu, icon_verts);
      if (icon_verts.empty()) {
        append_image_quad(icon_x, kFooterY, footer_box_z, icon_w, icon_h,
                          0xFFFFFFFFu, icon_verts);
      }
      if (projected_box_center) {
        center_vertices_in_projected_footer(
            icon_verts, footer_projection, *projected_box_center);
      }
      ghogx::render::MiloSceneRenderer::TextBatch icon_batch;
      icon_batch.verts = std::move(icon_verts);
      icon_batch.atlas = &it->second;
      batches.push_back(std::move(icon_batch));
    }
    const std::size_t label_start = out.size();
    append_song_string_centered_z(label, font, x, kFooterY, footer_box_z,
                                  kFooterScale, kFooterCol, out);
    if (projected_box_center) {
      std::vector<ghogx::render::MiloSceneRenderer::TextVertex> label_verts(
          out.begin() + static_cast<std::ptrdiff_t>(label_start), out.end());
      center_vertices_in_projected_footer(
          label_verts, footer_projection, *projected_box_center);
      std::copy(label_verts.begin(), label_verts.end(),
                out.begin() + static_cast<std::ptrdiff_t>(label_start));
    }
  }
}

struct SongListEntry {
  bool header = false;
  std::string text;
  int song_pos = -1;
};

struct CreditRow {
  std::vector<std::string> columns;
};

enum class CreditTextAlign { Left, Center, Right };

using TextVerts = std::vector<ghogx::render::MiloSceneRenderer::TextVertex>;

std::string song_title_by_key(const ConfigDb& db, Symbol key) {
  for (std::size_t i = 0; i < db.song_count(); ++i) {
    if (db.song_key(i) != key) continue;
    std::string title(db.song_field(i, Symbol("name")).as_string().value_or(""));
    return title.empty() ? std::string(key.c_str()) : title;
  }
  return std::string(key.c_str());
}

std::vector<SongListEntry> quickplay_entries(
    const ConfigDb& db, const std::map<std::string, std::string>& locale) {
  std::vector<SongListEntry> out;
  const DataArray* campaign = db.table(Symbol("campaign"));
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  int song_pos = 0;
  if (order) {
    for (std::size_t i = 1; i < order->size(); ++i) {
      auto tier = order->at(i).as_array();
      if (!tier || tier->empty()) continue;
      Symbol tier_name = tier->at(0).as_symbol().value_or(Symbol());
      std::string header_key = std::string("song_header_") + tier_name.c_str();
      std::string header = header_key;
      if (auto it = locale.find(header_key); it != locale.end()) header = it->second;
      out.push_back({true, header, -1});
      for (std::size_t j = 1; j < tier->size(); ++j) {
        Symbol song = tier->at(j).as_symbol().value_or(Symbol());
        if (!song.valid()) continue;
        out.push_back({false, song_title_by_key(db, song), song_pos++});
      }
    }
  }
  if (!out.empty()) return out;

  for (std::size_t i = 0; i < db.song_count(); ++i) {
    std::string title(db.song_field(i, Symbol("name")).as_string().value_or(""));
    if (title.empty()) title = db.song_key(i).c_str();
    out.push_back({false, title, static_cast<int>(i)});
  }
  return out;
}

std::vector<CreditRow> credits_entries(const ConfigDb& db) {
  std::vector<CreditRow> out;
  const DataArray* credits = db.table(Symbol("credits"));
  if (!credits) return out;
  out.reserve(credits->size());
  for (std::size_t i = 0; i < credits->size(); ++i) {
    auto row = credits->at(i).as_array();
    CreditRow entry;
    if (row) {
      entry.columns.reserve(row->size());
      for (std::size_t j = 0; j < row->size(); ++j) {
        if (auto s = row->at(j).as_string())
          entry.columns.emplace_back(*s);
        else
          entry.columns.emplace_back();
      }
    }
    out.push_back(std::move(entry));
  }
  return out;
}

void seed_ui_list_source_fields(Object* list, const UiListLayout& layout,
                                std::size_t provider_count) {
  if (!list || !layout.valid) return;
  list->set_property(Symbol("num_display"), DataNode::Int(layout.num_display));
  list->set_property(Symbol("min_display"), DataNode::Int(layout.min_display));
  list->set_property(Symbol("max_display"), DataNode::Int(layout.max_display));
  list->set_property(Symbol("speed"), DataNode::Float(layout.speed));
  list->set_property(Symbol("num_data"), DataNode::Int(layout.num_data));
  list->set_property(Symbol("provider_num_data"),
                     DataNode::Int(static_cast<int>(provider_count)));
  // UIList's constructor default is 2 seconds; rev < 14 lists do not serialize
  // mAutoScrollPause, so keep that source default when the field is absent.
  const float pause = layout.auto_scroll_pause > 0.0f
                          ? layout.auto_scroll_pause
                          : 2.0f;
  list->set_property(Symbol("auto_scroll_pause"), DataNode::Float(pause));
}

Object* panel_child(ScreenManager& mgr, Symbol panel_name, Symbol child_name) {
  Object* panel = mgr.find_object(panel_name);
  if (auto* dir = dynamic_cast<ObjectDir*>(panel)) return dir->find_path(child_name.c_str());
  return nullptr;
}

void seed_source_list_layouts(const std::string& hdr, const std::string& ark,
                              ScreenManager& mgr, const ConfigDb& db,
                              const std::map<std::string, std::string>& locale) {
  const auto song_entries = quickplay_entries(db, locale);
  seed_ui_list_source_fields(
      panel_child(mgr, Symbol("sel_song_panel"), Symbol("ss_song.lst")),
      extract_ui_list_layout(hdr, ark, "ui/gen/sel_song_quickplay.milo_ps2",
                             "ss_song.lst"),
      song_entries.size());

  const auto credit_entries = credits_entries(db);
  seed_ui_list_source_fields(
      panel_child(mgr, Symbol("credits_panel"), Symbol("credits.lst")),
      extract_ui_list_layout(hdr, ark, "ui/gen/credits.milo_ps2",
                             "credits.lst"),
      credit_entries.size());
}

int display_row_for_song(const std::vector<SongListEntry>& entries, int selected_song) {
  for (std::size_t i = 0; i < entries.size(); ++i)
    if (!entries[i].header && entries[i].song_pos == selected_song) return static_cast<int>(i);
  return 0;
}

float source_text_scale(const UiListLayout& layout, const MenuFont& font,
                        float fallback_scale) {
  const float cell_h = font.line_height();
  if (layout.valid && layout.has_legacy_row_metrics &&
      layout.legacy_text_height > 0.0f && cell_h > 0.0f)
    return layout.legacy_text_height / cell_h;
  return fallback_scale;
}

float source_text_scale(const MenuTextStyle& style, const MenuFont& font,
                        float fallback_scale) {
  const float cap_h = font.cap_height();
  if (style.valid && style.text_size > 0.0f && cap_h > 0.0f)
    return style.text_size / cap_h;
  return fallback_scale;
}

float source_setlist_slot_text_scale(const MenuTextStyle& style,
                                     const MenuFont& font,
                                     float fallback_scale) {
  // UIListLabel::CreateElement clones the UILabel resource, then the provider
  // only swaps text. The row/header RndText::mSize in list_song.milo is the
  // rendered glyph size; ss_song.lst's legacy text_h is list-window metadata.
  // ihatecompvir's RndFont source names the first two font metrics `cellSize`,
  // so RndText::mSize scales against the cell height, not the horizontal cell
  // width. For dyingmarker this is 26 / 28, while the legacy UIList text_h=30
  // only bounds the slot window.
  const float cell_h = font.line_height();
  if (style.valid && style.text_size > 0.0f && cell_h > 0.0f)
    return style.text_size / cell_h;
  return fallback_scale;
}

void append_quickplay_song_list(const std::string& hdr, const std::string& ark,
                                ScreenManager& mgr, const ConfigDb& db,
                                const std::map<std::string, std::string>& locale,
                                const MenuFont& font,
                                std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  Object* list = mgr.resolve_object(Symbol("ss_song.lst"));
  Object* panel = mgr.find_object(Symbol("sel_song_panel"));
  int selected = 0;
  if (list) selected = list->handle_property(Symbol("selected_pos"), DataArray()).as_int().value_or(0);
  else if (panel) selected = panel->get_property(Symbol("ss_song_selected")).as_int().value_or(0);
  if (selected < 0) selected = 0;

  UiListLayout list_layout = extract_ui_list_layout(
      hdr, ark, "ui/gen/sel_song_quickplay.milo_ps2", "ss_song.lst");
  const int source_visible_rows =
      list_layout.valid ? std::clamp(list_layout.num_display, 1, 32) : 7;
  int source_min_display =
      list_layout.valid ? std::clamp(list_layout.min_display, 0,
                                     source_visible_rows - 1)
                        : 0;
  int source_max_display = source_visible_rows - 1;
  if (list_layout.valid && list_layout.max_display >= 0) {
    source_max_display =
        std::clamp(list_layout.max_display, source_min_display,
                   source_visible_rows - 1);
  }
  if (std::getenv("GHOGX_LOG_MENU_LISTS")) {
    std::fprintf(stderr,
                 "[menu-list] ss_song.lst source valid=%d rev=%u "
                 "num_display=%d min=%d max=%d circular=%d speed=%.3f "
                 "row=%.3f text_h=%.3f\n",
                 list_layout.valid ? 1 : 0,
                 static_cast<unsigned>(list_layout.revision),
                 list_layout.num_display, list_layout.min_display,
                 list_layout.max_display, list_layout.circular ? 1 : 0,
                 list_layout.speed,
                 list_layout.has_legacy_row_metrics
                     ? list_layout.legacy_row_height
                     : 0.0f,
                 list_layout.has_legacy_row_metrics
                     ? list_layout.legacy_text_height
                     : 0.0f);
  }

  // ss_song.lst behavior comes from the authored UIList fields decoded above
  // using ihatecompvir's UIList source order and GH2 PS2's compact rev-2 row
  // metrics. The live glyph size comes from the cloned slot label/text objects:
  // UIListLabel::CreateElement ResourceCopy()s list_song.milo's UILabel, whose
  // UILabel/RndText source fields carry the visible text size.
  const int kVisibleRows = source_visible_rows;
  constexpr float kBaseX = 25.0f;
  constexpr float kBaseY = 0.0f;
  constexpr float kBaseZ = 105.0f;
  const float row_h =
      (list_layout.valid && list_layout.has_legacy_row_metrics &&
       list_layout.legacy_row_height > 0.0f)
          ? list_layout.legacy_row_height
          : 40.0f;
  const MenuTextStyle list_style = extract_menu_text_style(
      hdr, ark, "ui/gen/list_song.milo_ps2", "list.txt");
  const MenuTextStyle header_style = extract_menu_text_style(
      hdr, ark, "ui/gen/list_song.milo_ps2", "header.txt");
  const float fallback_text_scale = font.cap_height() > 0.0f ? 1.0f : 0.54f;
  const float list_text_scale =
      source_setlist_slot_text_scale(list_style, font, fallback_text_scale);
  const float header_text_scale =
      source_setlist_slot_text_scale(header_style, font, fallback_text_scale);
  if (std::getenv("GHOGX_LOG_MENU_LISTS")) {
    std::fprintf(stderr,
                 "[menu-list] list_song.milo text styles: list valid=%d "
                 "mSize=%.3f list_scale=%.3f header valid=%d mSize=%.3f "
                 "header_scale=%.3f\n",
                 list_style.valid ? 1 : 0, list_style.text_size,
                 list_text_scale, header_style.valid ? 1 : 0,
                 header_style.text_size, header_text_scale);
  }
  constexpr float kTitleX = -263.0f;
  constexpr float kTitleZ = 0.0f;
  constexpr float kHeaderX = -294.0f;
  constexpr float kHeaderZ = 1.0f;

  std::vector<SongListEntry> entries = quickplay_entries(db, locale);
  int selected_display = display_row_for_song(entries, selected);
  int first_display = 0;
  if (selected_display > source_max_display) {
    first_display = selected_display - source_max_display;
  } else if (selected_display < source_min_display) {
    first_display = selected_display - source_min_display;
  }
  const int max_first =
      std::max(0, static_cast<int>(entries.size()) - kVisibleRows);
  first_display = std::clamp(first_display, 0, max_first);
  for (int row = 0; row < kVisibleRows; ++row) {
    int ei = first_display + row;
    if (ei < 0 || ei >= static_cast<int>(entries.size())) break;
    const SongListEntry& e = entries[ei];
    float rz = kBaseZ - row * row_h;
    if (e.header) {
      append_song_string(e.text, font, kBaseX + kHeaderX, kBaseY, rz + kHeaderZ,
                         header_text_scale, 0xFFB30000u, out);
    } else {
      const bool foc = (e.song_pos == selected);
      uint32_t title_col = foc ? 0xFF003CFFu : 0xFF1A1A1Au;
      append_song_string(e.text, font, kBaseX + kTitleX, kBaseY, rz + kTitleZ,
                         list_text_scale, title_col, out);
    }
  }
}

void append_credit_text_native(
    const MenuLabel& slot, const MenuTextStyle& style, const std::string& text,
    const MenuFont& font, float list_x, float list_y, float row_z,
    float column_unit, CreditTextAlign align,
    TextVerts& out) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  if (text.empty() || !slot.has_world) return;

  float w = 0.0f;
  auto quads = font.layout(text, &w);
  const float fallback_scale =
      font.cap_height() > 0.0f ? 20.0f / font.cap_height() : 1.0f;
  const float scale = source_text_scale(style, font, fallback_scale);
  float Tx = list_x + slot.world[9] * column_unit;
  if (align == CreditTextAlign::Center)
    Tx -= w * scale * 0.5f;
  else if (align == CreditTextAlign::Right)
    Tx -= w * scale;
  const float Ty = list_y + slot.world[10];
  const float Tz = slot.world[11] + row_z;
  const float h = font.cap_height();
  const uint32_t color = style.valid ? pack_rgba_color(style.color) : 0xFFFFFFFFu;
  for (const auto& q : quads) {
    auto V = [&](float qx, float qy, float u, float v) {
      return TV{Tx + qx * scale, Ty, Tz - (qy - h * 0.5f) * scale, u, v,
                color};
    };
    TV a = V(q.x0, q.y0, q.u0, q.v0), b = V(q.x1, q.y0, q.u1, q.v0),
       c = V(q.x1, q.y1, q.u1, q.v1), d = V(q.x0, q.y1, q.u0, q.v1);
    out.push_back(a); out.push_back(b); out.push_back(c);
    out.push_back(a); out.push_back(c); out.push_back(d);
  }
}

const MenuLabel* find_credit_slot(
    const std::map<std::string, MenuLabel>& slots, const char* name) {
  auto it = slots.find(name);
  return it == slots.end() ? nullptr : &it->second;
}

const MenuTextStyle& find_credit_style(
    const std::map<std::string, MenuTextStyle>& styles, const char* name) {
  static const MenuTextStyle empty;
  auto it = styles.find(name);
  return it == styles.end() ? empty : it->second;
}

const MenuFont& credit_font_for_style(const MenuTextStyle& style,
                                      const MenuFont& clarendon_font,
                                      const MenuFont& rockletters_font) {
  if (style.font == "rockletters.font" && rockletters_font.valid())
    return rockletters_font;
  return clarendon_font;
}

TextVerts& credit_output_for_style(const MenuTextStyle& style,
                                   TextVerts& clarendon_out,
                                   TextVerts& rockletters_out) {
  if (style.font == "rockletters.font") return rockletters_out;
  return clarendon_out;
}

void append_credit_text_source_font(
    const MenuLabel& slot, const MenuTextStyle& style, const std::string& text,
    const MenuFont& clarendon_font, const MenuFont& rockletters_font,
    float list_x, float list_y, float row_z, float column_unit,
    CreditTextAlign align, TextVerts& clarendon_out,
    TextVerts& rockletters_out) {
  const MenuFont& font =
      credit_font_for_style(style, clarendon_font, rockletters_font);
  TextVerts& out = credit_output_for_style(style, clarendon_out, rockletters_out);
  append_credit_text_native(slot, style, text, font, list_x, list_y, row_z,
                            column_unit, align, out);
}

void append_credit_row(
    const CreditRow& row, const std::map<std::string, MenuLabel>& slots,
    const std::map<std::string, MenuTextStyle>& styles,
    const MenuFont& clarendon_font, const MenuFont& rockletters_font,
    float list_x, float list_y, float row_z, float column_unit,
    TextVerts& clarendon_out, TextVerts& rockletters_out) {
  if (row.columns.empty()) return;
  if (!row.columns[0].empty() && row.columns[0] == "image") return;

  const MenuLabel* left = find_credit_slot(slots, "title.txt");
  const MenuLabel* right = find_credit_slot(slots, "name.txt");
  const MenuLabel* heading = find_credit_slot(slots, "centername.txt");
  const MenuLabel* centered = find_credit_slot(slots, "center.txt");

  if (row.columns.size() == 1) {
    if (heading)
      append_credit_text_source_font(
          *heading, find_credit_style(styles, "centername.txt"),
          row.columns[0], clarendon_font, rockletters_font, list_x, list_y,
          row_z, column_unit, CreditTextAlign::Center, clarendon_out,
          rockletters_out);
    return;
  }

  if (row.columns.size() >= 3 && row.columns[0].empty() &&
      row.columns[2].empty()) {
    if (centered)
      append_credit_text_source_font(
          *centered, find_credit_style(styles, "center.txt"), row.columns[1],
          clarendon_font, rockletters_font, list_x, list_y, row_z, column_unit,
          CreditTextAlign::Center, clarendon_out, rockletters_out);
    return;
  }

  if (left && !row.columns[0].empty())
    append_credit_text_source_font(*left, find_credit_style(styles, "title.txt"),
                                   row.columns[0], clarendon_font,
                                   rockletters_font, list_x, list_y, row_z,
                                   column_unit, CreditTextAlign::Right,
                                   clarendon_out, rockletters_out);
  if (right && row.columns.size() >= 2 && !row.columns[1].empty())
    append_credit_text_source_font(*right, find_credit_style(styles, "name.txt"),
                                   row.columns[1], clarendon_font,
                                   rockletters_font, list_x, list_y, row_z,
                                   column_unit, CreditTextAlign::Left,
                                   clarendon_out, rockletters_out);
}

void append_credits_list(
    const std::string& hdr, const std::string& ark, ScreenManager& mgr,
    const ConfigDb& db, const MenuFont& clarendon_font,
    const MenuFont& rockletters_font, TextVerts& clarendon_out,
    TextVerts& rockletters_out) {
  std::vector<CreditRow> rows = credits_entries(db);
  if (rows.empty()) return;

  UiListLayout list_layout =
      extract_ui_list_layout(hdr, ark, "ui/gen/credits.milo_ps2", "credits.lst");
  const int visible_rows =
      list_layout.valid ? std::clamp(list_layout.num_display, 1, 64) : 16;
  const float row_h =
      (list_layout.valid && list_layout.has_legacy_row_metrics &&
       list_layout.legacy_row_height > 0.0f)
          ? list_layout.legacy_row_height
          : 25.0f;
  const float list_x = list_layout.has_world ? list_layout.world[9] : 0.0f;
  const float list_y = list_layout.has_world ? list_layout.world[10] : 0.0f;
  const float list_z = list_layout.has_world ? list_layout.world[11] : 186.0f;
  const float text_h =
      (list_layout.valid && list_layout.has_legacy_row_metrics &&
       list_layout.legacy_text_height > 0.0f)
          ? list_layout.legacy_text_height
          : clarendon_font.line_height();
  (void)text_h;  // logged as the UIList text-size source; X slots are world units.
  // list_credits text slots already carry authored WorldXfm translations
  // (-10/0/+10). Keep those values direct; only glyph size comes from mSize.
  const float column_unit = 1.0f;

  int selected = 0;
  Object* list = mgr.resolve_object(Symbol("credits.lst"));
  if (list)
    selected =
        list->handle_property(Symbol("selected_pos"), DataArray()).as_int().value_or(0);
  selected = std::clamp(selected, 0, static_cast<int>(rows.size() - 1));

  int min_display =
      list_layout.valid ? std::clamp(list_layout.min_display, 0, visible_rows - 1)
                        : 0;
  int max_display = visible_rows - 1;
  if (list_layout.valid && list_layout.max_display >= 0) {
    max_display =
        std::clamp(list_layout.max_display, min_display, visible_rows - 1);
  }
  int first = 0;
  if (selected > max_display)
    first = selected - max_display;
  else if (selected < min_display)
    first = selected - min_display;
  first = std::clamp(first, 0, std::max(0, static_cast<int>(rows.size()) - visible_rows));
  int target_first = first;
  float visual_first = static_cast<float>(first);
  if (list) {
    first = list->handle_property(Symbol("first_showing"), DataArray())
                .as_int()
                .value_or(first);
    target_first = list->get_property(Symbol("target_showing"))
                       .as_int()
                       .value_or(first);
    const int max_first =
        std::max(0, static_cast<int>(rows.size()) - visible_rows);
    first = std::clamp(first, 0, max_first);
    target_first = std::clamp(target_first, 0, max_first);
    const float step =
        std::clamp(list->get_property(Symbol("scroll_step_percent"))
                       .as_float()
                       .value_or(target_first == first ? 1.0f : 0.0f),
                   0.0f, 1.0f);
    visual_first =
        static_cast<float>(first) +
        static_cast<float>(target_first - first) * step;
  }
  const int draw_first =
      std::clamp(static_cast<int>(std::floor(visual_first)), 0,
                 std::max(0, static_cast<int>(rows.size()) - visible_rows));
  const float row_phase = visual_first - static_cast<float>(draw_first);

  std::map<std::string, MenuLabel> slots;
  for (auto& label :
       extract_menu_labels(hdr, ark, "ui/gen/list_credits.milo_ps2")) {
    slots[label.name] = std::move(label);
  }
  std::map<std::string, MenuTextStyle> styles;
  for (const char* name :
       {"title.txt", "centername.txt", "center.txt", "name.txt"}) {
    styles.emplace(name, extract_menu_text_style(
                             hdr, ark, "ui/gen/list_credits.milo_ps2", name));
  }

  if (std::getenv("GHOGX_LOG_MENU_LISTS")) {
    std::fprintf(stderr,
                 "[menu-list] credits.lst source valid=%d rev=%u "
                 "num_display=%d selected=%d first=%d target=%d visual=%.3f "
                 "row=%.3f text_h=%.3f column_unit=%.3f slots=%zu rows=%zu\n",
                 list_layout.valid ? 1 : 0,
                 static_cast<unsigned>(list_layout.revision),
                 list_layout.num_display, selected, first, target_first,
                 visual_first, row_h,
                 list_layout.has_legacy_row_metrics
                     ? list_layout.legacy_text_height
                     : 0.0f,
                 column_unit, slots.size(), rows.size());
  }

  for (int row = 0; row < visible_rows + 1; ++row) {
    const int index = draw_first + row;
    if (index < 0 || index >= static_cast<int>(rows.size())) break;
    const float z = list_z - (static_cast<float>(row) - row_phase) * row_h;
    append_credit_row(rows[index], slots, styles, clarendon_font,
                      rockletters_font, list_x, list_y, z, column_unit,
                      clarendon_out, rockletters_out);
  }
}

void append_provider_list(const std::string& hdr, const std::string& ark,
                          ScreenManager& mgr,
                          const std::map<std::string, std::string>& locale,
                          const MenuFont& font,
                          const std::string& panel_milo,
                          const std::string& list_name,
                          const std::string& resource_milo,
                          const std::string& text_name,
                          TextVerts& out) {
  Object* list = mgr.resolve_object(Symbol(list_name.c_str()));
  if (!list) return;
  Object* provider = list->get_property(Symbol("provider")).as_object();
  if (!provider) {
    if (list_name == "sel_section.lst")
      provider = mgr.resolve_object(Symbol("section_provider"));
  }
  if (!provider) return;

  UiListLayout layout =
      extract_ui_list_layout(hdr, ark, panel_milo, list_name);
  const int count = provider->handle_property(Symbol("list_length"), DataArray())
                        .as_int()
                        .value_or(0);
  if (count <= 0) return;
  const int visible_rows =
      layout.valid ? std::clamp(layout.num_display, 1, 64) : 8;
  const int selected = std::clamp(
      list->handle_property(Symbol("selected_pos"), DataArray())
          .as_int()
          .value_or(0),
      0, std::max(0, count - 1));
  const int min_display =
      layout.valid ? std::clamp(layout.min_display, 0, visible_rows - 1) : 0;
  int max_display = visible_rows - 1;
  if (layout.valid && layout.max_display >= 0)
    max_display = std::clamp(layout.max_display, min_display, visible_rows - 1);

  int first = 0;
  if (selected > max_display)
    first = selected - max_display;
  else if (selected < min_display)
    first = selected - min_display;
  first = std::clamp(first, 0, std::max(0, count - visible_rows));

  const float row_h =
      (layout.valid && layout.has_legacy_row_metrics &&
       layout.legacy_row_height > 0.0f)
          ? layout.legacy_row_height
          : 32.0f;
  const float list_x = layout.has_world ? layout.world[9] : 0.0f;
  const float list_y = layout.has_world ? layout.world[10] : 0.0f;
  const float list_z = layout.has_world ? layout.world[11] : 40.0f;
  const MenuTextStyle text_style =
      extract_menu_text_style(hdr, ark, resource_milo, text_name);
  const float text_x =
      list_x + (text_style.has_world ? text_style.world[9] : 0.0f);
  const float text_y =
      list_y + (text_style.has_world ? text_style.world[10] : 0.0f);
  const float text_z =
      list_z + (text_style.has_world ? text_style.world[11] : 0.0f);
  const float scale =
      source_setlist_slot_text_scale(text_style, font,
                                     source_text_scale(layout, font, 0.7f));

  if (std::getenv("GHOGX_LOG_MENU_LISTS")) {
    std::fprintf(stderr,
                 "[menu-list] %s source valid=%d rows=%d selected=%d first=%d "
                 "row=%.3f scale=%.3f origin=(%.1f %.1f %.1f) "
                 "text=%s:%s valid=%d font=%s size=%.3f "
                 "text_world=(%.1f %.1f %.1f)\n",
                 list_name.c_str(), layout.valid ? 1 : 0, visible_rows,
                 selected, first, row_h, scale, list_x, list_y, list_z,
                 resource_milo.c_str(), text_name.c_str(),
                 text_style.valid ? 1 : 0, text_style.font.c_str(),
                 text_style.text_size, text_x, text_y, text_z);
  }

  for (int row = 0; row < visible_rows; ++row) {
    const int index = first + row;
    if (index < 0 || index >= count) break;
    DataArray arg;
    arg.push(DataNode::Int(index));
    std::string text =
        display_text_from_node(provider->handle_property(Symbol("get_text"), arg),
                               locale);
    if (text.empty())
      text = display_text_from_node(
          provider->handle_property(Symbol("get_symbol"), arg), locale);
    if (text.empty()) continue;
    const bool focused = index == selected;
    const uint32_t color = focused ? 0xFF003CFFu : 0xFF1A1A1Au;
    append_song_string(text, font, text_x, text_y,
                       text_z - static_cast<float>(row) * row_h, scale, color,
                       out);
  }
}

void append_text_entry_widgets(
    ScreenManager& mgr, const std::vector<MenuLabel>& entries,
    const MenuFont& font, const std::map<std::string, std::string>& locale,
    const milo_scene::Scene& textentry_scene,
    const std::map<std::string, asset::Image>& textentry_textures,
    std::vector<ghogx::render::MiloSceneRenderer::TextBatch>& batches) {
  if (!font.valid()) return;
  for (const MenuLabel& entry : entries) {
    if (entry.type != "BandTextEntry" || !entry.has_world ||
        !entry.text_tail.valid || !entry.text_entry_tail.valid ||
        !live_element_showing(mgr, entry.name, entry.parent,
                              !entry.has_showing || entry.showing))
      continue;
    Object* live = mgr.resolve_object(Symbol(entry.name.c_str()));
    if (live && node_bool(live->handle_property(Symbol("is_done"), DataArray())))
      continue;
    const std::string text = display_text_from_node(
        DataNode::Str(live_label_text(mgr, entry)), locale);
    if (text.empty()) continue;

    const std::string prefix = text.substr(0, text.size() - 1);
    const std::string current(1, text.back());
    float current_advance = 0.0f;
    const auto current_quads = font.layout(current, &current_advance);
    if (current_quads.empty() || font.cap_height() <= 0.0f) continue;
    const float draw_scale = entry.text_tail.text_size;
    const float current_scale =
        draw_scale * (1.0f + entry.text_entry_tail.text_scale);
    const float base_world_width =
        current_advance / font.cap_height() * draw_scale;
    const float scaled_world_width =
        current_advance / font.cap_height() * current_scale;
    const float current_x = font.measure(prefix) / font.cap_height() * draw_scale -
                            (scaled_world_width - base_world_width) * 0.5f;
    float glyph_min_x = std::numeric_limits<float>::max();
    float glyph_max_x = std::numeric_limits<float>::lowest();
    float glyph_min_z = std::numeric_limits<float>::max();
    float glyph_max_z = std::numeric_limits<float>::lowest();
    for (const auto& quad : current_quads) {
      for (const float qx : {quad.x0, quad.x1}) {
        const float x = current_x + qx / font.cap_height() * current_scale;
        glyph_min_x = std::min(glyph_min_x, x);
        glyph_max_x = std::max(glyph_max_x, x);
      }
      for (const float qy : {quad.y0, quad.y1}) {
        const float z =
            -((qy - font.cap_height() * 0.5f) / font.cap_height()) *
            current_scale;
        glyph_min_z = std::min(glyph_min_z, z);
        glyph_max_z = std::max(glyph_max_z, z);
      }
    }
    const std::array<float, 16> widget_world = mat4_from_xfm12(entry.world);

    for (const auto& mesh : textentry_scene.meshes) {
      if (!mesh.decoded || mesh.verts.empty() || mesh.indices.empty()) continue;
      if (mesh.name != "char_back.mesh" && mesh.name != "char_next.mesh")
        continue;
      const milo_scene::MatObj* mat =
          textentry_scene.find_mat(mesh.material);
      if (!mat || mat->diffuse_tex.empty()) continue;
      auto texture = textentry_textures.find(mat->diffuse_tex);
      if (texture == textentry_textures.end() || !texture->second.valid())
        continue;
      const std::array<float, 16> resource_world =
          textentry_scene.world_matrix(mesh);
      float resource_min_x = std::numeric_limits<float>::max();
      float resource_max_x = std::numeric_limits<float>::lowest();
      float resource_min_z = std::numeric_limits<float>::max();
      float resource_max_z = std::numeric_limits<float>::lowest();
      for (const auto& vertex : mesh.verts) {
        const float x = vertex.px * resource_world[0] +
                        vertex.py * resource_world[4] +
                        vertex.pz * resource_world[8] + resource_world[12];
        const float z = vertex.px * resource_world[2] +
                        vertex.py * resource_world[6] +
                        vertex.pz * resource_world[10] + resource_world[14];
        resource_min_x = std::min(resource_min_x, x);
        resource_max_x = std::max(resource_max_x, x);
        resource_min_z = std::min(resource_min_z, z);
        resource_max_z = std::max(resource_max_z, z);
      }
      std::array<float, 16> cursor_local{};
      cursor_local[0] = cursor_local[5] = cursor_local[10] = cursor_local[15] =
          1.0f;
      // The stock meshes bracket the actual highlighted glyph, not the
      // nominal RndText size.  Using the decoded glyph and mesh bounds also
      // accounts for each character's bearing.  A negative arrow_offset pulls
      // both arrows inward, matching ui_objects_ps2.dta's "up/down" contract.
      cursor_local[12] =
          (glyph_min_x + glyph_max_x - resource_min_x - resource_max_x) * 0.5f;
      if (mesh.name == "char_back.mesh") {
        cursor_local[14] = glyph_max_z + entry.text_entry_tail.arrow_offset -
                           resource_min_z;
      } else {
        cursor_local[14] = glyph_min_z - entry.text_entry_tail.arrow_offset -
                           resource_max_z;
      }
      const std::array<float, 16> world = mat4_mul(
          mat4_mul(resource_world, cursor_local), widget_world);
      const uint32_t color = pack_rgba_color(
          std::array<float, 4>{{mat->color[0], mat->color[1], mat->color[2],
                                mat->color[3]}});
      std::vector<ghogx::render::MiloSceneRenderer::TextVertex> verts;
      for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        verts.push_back(mesh_overlay_vertex(mesh.verts[mesh.indices[i + 0]],
                                            mat, world, color));
        verts.push_back(mesh_overlay_vertex(mesh.verts[mesh.indices[i + 1]],
                                            mat, world, color));
        verts.push_back(mesh_overlay_vertex(mesh.verts[mesh.indices[i + 2]],
                                            mat, world, color));
      }
      ghogx::render::MiloSceneRenderer::TextBatch batch;
      batch.verts = std::move(verts);
      batch.atlas = &texture->second;
      ghogx::render::MiloSceneRenderer::TextTransformSpan span;
      span.first_vertex = 0;
      span.vertex_count = batch.verts.size();
      span.name = entry.name;
      span.parent = entry.parent;
      span.local = entry.has_local ? mat4_from_xfm12(entry.local)
                                   : identity_mat4();
      span.bind_world = widget_world;
      batch.transform_spans.push_back(std::move(span));
      batches.push_back(std::move(batch));
    }
  }
}

void append_endgame_stats_list(
    const std::string& hdr, const std::string& ark, ScreenManager& mgr,
    const std::map<std::string, std::string>& locale,
    const MenuFont& receipt_font, TextVerts& out) {
  Object* list = mgr.resolve_object(Symbol("stats_sections.lst"));
  if (!list || !receipt_font.valid()) return;
  Object* provider = list->get_property(Symbol("provider")).as_object();
  if (!provider) provider = mgr.resolve_object(Symbol("stats_provider"));
  if (!provider) return;

  const UiListLayout layout = extract_ui_list_layout(
      hdr, ark, "ui/gen/endgame_stats.milo_ps2", "stats_sections.lst");
  const int count = provider->handle_property(Symbol("num_data"), DataArray())
                        .as_int()
                        .value_or(0);
  if (count <= 0) return;
  const int visible = layout.valid ? std::clamp(layout.num_display, 1, 64) : 30;
  const float row_h =
      layout.valid && layout.has_legacy_row_metrics &&
              layout.legacy_row_height > 0.0f
          ? layout.legacy_row_height
          : 24.0f;
  const float list_x = layout.has_world ? layout.world[9] : 0.0f;
  const float list_y = layout.has_world ? layout.world[10] : 0.0f;
  const float list_z = layout.has_world ? layout.world[11] : -70.0f;

  std::map<std::string, MenuLabel> slots;
  for (auto& label :
       extract_menu_labels(hdr, ark, "ui/gen/list_stats.milo_ps2"))
    slots[label.name] = std::move(label);
  const MenuTextStyle section_style = extract_menu_text_style(
      hdr, ark, "ui/gen/list_stats.milo_ps2", "section.txt");
  const MenuTextStyle notes_style = extract_menu_text_style(
      hdr, ark, "ui/gen/list_stats.milo_ps2", "notes1.txt");
  const auto section_slot = slots.find("section.txt");
  const auto notes_slot = slots.find("notes1.txt");
  if (section_slot == slots.end() || notes_slot == slots.end()) return;

  for (int row = 0; row < std::min(count, visible); ++row) {
    DataArray index;
    index.push(DataNode::Int(row));
    std::string section = display_text_from_node(
        provider->handle_property(Symbol("get_section"), index), locale);
    std::string notes = display_text_from_node(
        provider->handle_property(Symbol("get_notes1"), index), locale);
    const float row_z = list_z - static_cast<float>(row) * row_h;
    append_credit_text_native(section_slot->second, section_style, section,
                              receipt_font, list_x, list_y, row_z, 1.0f,
                              CreditTextAlign::Left, out);
    append_credit_text_native(notes_slot->second, notes_style, notes,
                              receipt_font, list_x, list_y, row_z, 1.0f,
                              CreditTextAlign::Right, out);
  }
}

void configure_practice_section_selection_mesh(
    const std::string& hdr, const std::string& ark, ScreenManager& mgr,
    ghogx::render::MiloSceneRenderer& renderer) {
  Object* list = mgr.resolve_object(Symbol("sel_section.lst"));
  if (!list) return;
  Object* provider = list->get_property(Symbol("provider")).as_object();
  if (!provider) provider = mgr.resolve_object(Symbol("section_provider"));
  if (!provider) return;

  const UiListLayout layout = extract_ui_list_layout(
      hdr, ark, "ui/gen/practice_sel_section.milo_ps2", "sel_section.lst");
  if (!layout.valid || !layout.has_world) return;
  const int count = provider->handle_property(Symbol("list_length"), DataArray())
                        .as_int()
                        .value_or(0);
  if (count <= 0) return;
  const int visible_rows = std::clamp(layout.num_display, 1, 64);
  const int selected = std::clamp(
      list->handle_property(Symbol("selected_pos"), DataArray())
          .as_int()
          .value_or(0),
      0, std::max(0, count - 1));
  const int min_display = std::clamp(layout.min_display, 0, visible_rows - 1);
  int max_display = visible_rows - 1;
  if (layout.max_display >= 0)
    max_display = std::clamp(layout.max_display, min_display, visible_rows - 1);

  int first = 0;
  if (selected > max_display)
    first = selected - max_display;
  else if (selected < min_display)
    first = selected - min_display;
  first = std::clamp(first, 0, std::max(0, count - visible_rows));
  const int selected_row = std::clamp(selected - first, 0, visible_rows - 1);
  const float row_h =
      layout.has_legacy_row_metrics && layout.legacy_row_height > 0.0f
          ? layout.legacy_row_height
          : 20.0f;

  milo_scene::Scene panel_scene;
  milo_scene::Scene resource_scene;
  if (!milo_scene::load_scene(hdr, ark, "ui/gen/practice_sel_section.milo_ps2",
                              panel_scene) ||
      !milo_scene::load_scene(hdr, ark, "ui/gen/list_section.milo_ps2",
                              resource_scene))
    return;
  const milo_scene::MeshObj* full_selection =
      find_decoded_mesh(panel_scene, "full_selection.mesh");
  const milo_scene::MeshObj* highlight =
      find_decoded_mesh(resource_scene, "highlight.mesh");
  if (!full_selection || !highlight) return;
  const MeshWorldBounds full_bounds = mesh_world_bounds(*full_selection);
  const MeshWorldBounds highlight_bounds = mesh_world_bounds(*highlight);
  if (!full_bounds.valid || !highlight_bounds.valid) return;

  const float current_x = (full_bounds.min_x + full_bounds.max_x) * 0.5f;
  const float current_z = (full_bounds.min_z + full_bounds.max_z) * 0.5f;
  const float target_x =
      layout.world[9] + (highlight_bounds.min_x + highlight_bounds.max_x) * 0.5f;
  const float target_z =
      layout.world[11] +
      (highlight_bounds.min_z + highlight_bounds.max_z) * 0.5f -
      static_cast<float>(selected_row) * row_h;

  std::unordered_set<std::string> meshes{"full_selection.mesh"};
  std::map<std::string, std::array<float, 3>> offsets;
  offsets["full_selection.mesh"] = {target_x - current_x, 0.0f,
                                    target_z - current_z};
  renderer.set_post_text_meshes(std::move(meshes));
  renderer.set_post_text_mesh_world_offsets(std::move(offsets));
  renderer.set_post_text_mesh_text_split(0);

  if (std::getenv("GHOGX_LOG_MENU_LISTS")) {
    std::fprintf(stderr,
                 "[menu-list] practice selection mesh selected=%d first=%d "
                 "row=%d row_h=%.3f current=(%.1f %.1f) target=(%.1f %.1f) "
                 "offset=(%.1f %.1f)\n",
                 selected, first, selected_row, row_h, current_x, current_z,
                 target_x, target_z, target_x - current_x,
                 target_z - current_z);
  }
}

void collect_disabled_objects(Object* obj, std::unordered_set<std::string>& out) {
  if (!obj) return;
  if (obj->has_property(Symbol("disabled")) &&
      node_bool(obj->get_property(Symbol("disabled")))) {
    out.insert(obj->name().c_str());
  }
  if (auto* dir = dynamic_cast<ObjectDir*>(obj)) {
    for (std::size_t i = 0; i < dir->size(); ++i)
      collect_disabled_objects(dir->at(i), out);
  }
}

// Items the original would disable: authored scripts call enable/disable on
// child widgets, and the main panel poll disables multiplayer when the second
// controller is missing. Collect both the live UI disabled flags and that
// game-side condition.
std::unordered_set<std::string> compute_disabled(ScreenManager& mgr,
                                                 Object* screen) {
  std::unordered_set<std::string> d;
  Object* target_screen = screen ? screen : mgr.current_screen();
  if (target_screen) {
    for (Symbol pn : screen_panel_names(target_screen))
      collect_disabled_objects(mgr.find_object(pn), d);
  }
  // `game` is a singleton -> resolve_object (find_object only checks the screen
  // registry, so it would miss the singletons and never disable anything).
  if (Object* g = mgr.resolve_object(Symbol("game"))) {
    DataNode mm = g->handle_property(Symbol("is_missing_multi_controller"), DataArray());
    bool missing = false;
    if (auto s = mm.as_symbol()) missing = (std::strcmp(s->c_str(), "TRUE") == 0);
    if (auto i = mm.as_int()) missing = missing || (*i != 0);
    if (missing) d.insert("main_multiplayer.btn");
  }
  return d;
}

// All text-bearing objects of the current screen's panels (for focus nav).
std::vector<MenuLabel> gather_labels(const std::string& hdr, const std::string& ark,
                                     ScreenManager& mgr, Object* screen) {
  std::vector<MenuLabel> out;
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel_showing(panel)) continue;
    std::string file = panel_file(panel);
    if (file.empty()) continue;
    const std::string milo_path = "ui/gen/" + file + "_ps2";
    auto labels = extract_menu_labels(hdr, ark, milo_path);
    std::unordered_map<std::string, std::string> parent_by_name;
    milo_scene::Scene hierarchy;
    if (milo_scene::load_scene(hdr, ark, milo_path, hierarchy)) {
      auto assign_parent = [&](const std::string& child,
                               const std::string& parent) {
        if (!child.empty() && !parent.empty() &&
            parent_by_name.find(child) == parent_by_name.end())
          parent_by_name.emplace(child, parent);
      };
      for (const auto& group : hierarchy.groups) {
        assign_parent(group.name, group.parent);
        for (const auto& child : group.children)
          assign_parent(child, group.name);
      }
      for (const auto& trans : hierarchy.transes)
        assign_parent(trans.name, trans.parent);
    }
    for (auto& l : labels) {
      l.runtime_owner = pn.c_str();
      std::string ancestor = l.parent;
      std::unordered_set<std::string> visited;
      while (!ancestor.empty() && visited.insert(ancestor).second) {
        l.visibility_ancestors.push_back(ancestor);
        const auto parent = parent_by_name.find(ancestor);
        if (parent == parent_by_name.end()) break;
        ancestor = parent->second;
      }
      if (std::getenv("GHOGX_MENU_LABEL_TRACE")) {
        std::fprintf(stderr,
                     "[menu-label] owner=%s file=%s name=%s type=%s font=%s "
                     "button=(valid=%d legacy=%d fit=%d size=%.4f w=%.4f "
                     "h=%.4f align=%d) text=(valid=%d fit=%d size=%.4f "
                     "w=%.4f h=%.4f align=%d) local=(%.3f %.3f %.3f)\n",
                     pn.c_str(), file.c_str(), l.name.c_str(), l.type.c_str(),
                     l.font.c_str(), l.button_tail.valid ? 1 : 0,
                     l.button_tail.legacy_layout ? 1 : 0,
                     l.button_tail.fit_text, l.button_tail.text_size,
                     l.button_tail.width, l.button_tail.height,
                     l.button_tail.alignment, l.text_tail.valid ? 1 : 0,
                     l.text_tail.fit_text, l.text_tail.text_size,
                     l.text_tail.width, l.text_tail.height,
                     l.text_tail.alignment, l.local[9], l.local[10],
                     l.local[11]);
      }
      out.push_back(std::move(l));
    }
  }
  return out;
}

// Move focus down (dir>0) / up (dir<0) along the BandButton nav links, skipping
// disabled items. Sets the focused panel's (focus) property to the new component.
void set_panel_focus(ScreenManager& mgr, Object* panel, const std::string& next) {
  if (!panel || next.empty()) return;
  std::string cur = panel->get_property(Symbol("focus")).as_symbol().value_or(Symbol()).c_str();
  if (cur == next) return;
  Object* old_focus = cur.empty() ? nullptr : mgr.resolve_object(Symbol(cur.c_str()));
  Object* new_focus = mgr.resolve_object(Symbol(next.c_str()));
  panel->handle_property(Symbol("set_focus"),
                         one_arg(new_focus ? DataNode::Obj(new_focus)
                                           : DataNode::Sym(Symbol(next.c_str()))));
  mgr.set_global(Symbol("old_focus"), DataNode::Obj(old_focus));
  mgr.set_global(Symbol("new_focus"), DataNode::Obj(new_focus));
  mgr.handle_property(Symbol("FOCUS_MSG"), DataArray());
  panel->handle_property(Symbol("FOCUS_MSG"), DataArray());
}

bool is_slider_object(Object* obj) {
  if (!obj) return false;
  Symbol cls = obj->class_name();
  return cls == Symbol("UISlider") || cls == Symbol("BandSlider");
}

Object* focused_slider(ScreenManager& mgr, Object* panel) {
  if (!panel) return nullptr;
  Symbol focus =
      panel->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
  Object* obj = focus.valid() ? mgr.resolve_object(focus) : nullptr;
  return is_slider_object(obj) ? obj : nullptr;
}

bool slider_scroll_selected(Object* slider) {
  return slider &&
         node_bool(slider->handle_property(Symbol("is_scroll_selected"),
                                           DataArray()));
}

void send_slider_panel_msg(ScreenManager& mgr, Object* panel, Object* slider,
                           Symbol msg) {
  if (!panel || !slider) return;
  mgr.set_global(Symbol("component"), DataNode::Obj(slider));
  DataArray args;
  args.push(DataNode::Obj(slider));
  panel->handle_property(msg, args);
}

bool adjust_focused_slider(ScreenManager& mgr, Object* panel, int dir) {
  Object* slider = focused_slider(mgr, panel);
  if (!slider || !slider_scroll_selected(slider)) return false;
  const int steps = std::max(1, slider->handle_property(Symbol("num_steps"),
                                                        DataArray())
                                    .as_int()
                                    .value_or(1));
  int current = slider->handle_property(Symbol("current"), DataArray())
                    .as_int()
                    .value_or(0);
  current = std::clamp(current + dir, 0, steps - 1);
  slider->handle_property(Symbol("set_current"), one_arg(DataNode::Int(current)));
  send_slider_panel_msg(mgr, panel, slider, Symbol("slider_start_msg"));
  return true;
}

bool cancel_focused_slider(ScreenManager& mgr, Object* panel) {
  Object* slider = focused_slider(mgr, panel);
  if (!slider || !slider_scroll_selected(slider)) return false;
  slider->handle_property(Symbol("undo"), DataArray());
  send_slider_panel_msg(mgr, panel, slider, Symbol("slider_select_cancel"));
  return true;
}

void focus_move(ScreenManager& mgr, const std::vector<MenuLabel>& labels,
                const std::unordered_set<std::string>& disabled, int dir,
                std::size_t song_count, std::size_t credits_count) {
  Object* screen = mgr.current_screen();
  if (!screen) return;
  Symbol fpn = screen->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
  Object* panel = fpn.valid() ? mgr.find_object(fpn) : nullptr;
  if (!panel) return;
  std::string cur = panel->get_property(Symbol("focus")).as_symbol().value_or(Symbol()).c_str();
  if (cur == "ss_song.lst" || cur == "credits.lst") {
    const bool song_list = cur == "ss_song.lst";
    const Symbol stored(song_list ? "ss_song_selected" : "credits_selected");
    const std::size_t item_count = song_list ? song_count : credits_count;
    if (Object* list = mgr.resolve_object(Symbol(cur.c_str()))) {
      int pos =
          list->handle_property(Symbol("selected_pos"), DataArray()).as_int().value_or(0);
      pos += dir;
      int max_pos = item_count > 0 ? static_cast<int>(item_count - 1) : 0;
      if (pos < 0) pos = 0;
      if (pos > max_pos) pos = max_pos;
      list->handle_property(Symbol("set_selected"), one_arg(DataNode::Int(pos)));
      panel->set_property(stored, DataNode::Int(pos));
      if (song_list) {
        if (Object* game = mgr.resolve_object(Symbol("game")))
          game->handle_property(Symbol("set_song_index"), one_arg(DataNode::Int(pos)));
      }
      mgr.handle_property(Symbol("SCROLL_MSG"), DataArray());
      panel->handle_property(Symbol("SCROLL_MSG"), DataArray());
    } else {
      int pos = panel->get_property(stored).as_int().value_or(0) + dir;
      int max_pos = item_count > 0 ? static_cast<int>(item_count - 1) : 0;
      if (pos < 0) pos = 0;
      if (pos > max_pos) pos = max_pos;
      panel->set_property(stored, DataNode::Int(pos));
      if (song_list) {
        if (Object* game = mgr.resolve_object(Symbol("game")))
          game->handle_property(Symbol("set_song_index"), one_arg(DataNode::Int(pos)));
      }
    }
    return;
  }
  if (adjust_focused_slider(mgr, panel, dir)) return;
  for (size_t guard = 0; guard <= labels.size(); ++guard) {
    std::string next;
    if (dir > 0) {
      for (const auto& l : labels) if (l.name == cur) { next = l.nav; break; }
    } else {
      for (const auto& l : labels) if (!l.nav.empty() && l.nav == cur) { next = l.name; break; }
    }
    if (next.empty()) return;
    if (!disabled.count(next)) {
      set_panel_focus(mgr, panel, next);
      return;
    }
    cur = next;  // disabled -> keep moving in the same direction
  }
}

// Rebuild the renderer's text overlay from the current screen's panels.
void rebuild_text(const std::string& hdr, const std::string& ark, ScreenManager& mgr,
                  Object* screen, ghogx::render::MiloSceneRenderer& renderer,
                  MenuFontCatalog& fonts, const ConfigDb& db,
                  const std::map<std::string, std::string>& locale) {
  const MenuFont* impact = fonts.get("impact");
  if (!impact) {
    renderer.set_text_batches({});
    return;
  }
  const MenuFont& font = *impact;
  const MenuFont& song_font = *fonts.get("dyingmarker");
  const MenuFont& credits_font = *fonts.get("clarendon");
  const MenuFont& rockletters_font = *fonts.get("rockletters");
  const MenuFont& receipt_font = *fonts.get("receipt");
  const MenuFont& helvetica_font = *fonts.get("helveticablackcondensed");
  const MenuFont& helvetica_black_font = *fonts.get("helveticablack");
  renderer.set_post_text_meshes({});
  renderer.set_post_text_mesh_world_offsets({});
  renderer.set_post_text_mesh_text_split(0);
  // The focused component (screen.focus -> panel; panel.focus -> component) is
  // drawn in the focused colour (yellow).
  std::string focused;
  if (screen) {
    Symbol fpn = screen->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
    if (Object* fp = fpn.valid() ? mgr.find_object(fpn) : nullptr) {
      Symbol fc = fp->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
      if (fc.valid()) focused = fc.c_str();
    }
  }
  std::unordered_set<std::string> disabled = compute_disabled(mgr, screen);
  std::map<std::string,
           std::vector<ghogx::render::MiloSceneRenderer::TextVertex>>
      label_verts;
  std::map<std::string,
           std::vector<ghogx::render::MiloSceneRenderer::TextTransformSpan>>
      label_transform_spans;
  std::vector<ghogx::render::MiloSceneRenderer::TextBatch> batches;
  std::vector<ghogx::render::MiloSceneRenderer::TextBatch> helpbar_batches;
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> helpbar_verts;
  std::vector<MenuCheckbox> checkboxes;
  std::vector<MenuSlider> sliders;
  std::vector<MenuLabel> text_entries;
  std::unordered_set<std::string> foreground_panel_meshes;
  bool foreground_before_text = false;
  auto help_icons = asset::load_milo_textures(
      hdr, ark, "ui/gen/helpbar.milo_ps2",
      {"hb_fret1.tex", "hb_fret2.tex", "hb_fret3.tex", "hb_strum.tex", "hb_start.tex",
       "help_box_mid.tex", "help_box_corner.tex"});
  milo_scene::Scene helpbar_scene;
  milo_scene::load_scene(hdr, ark, "ui/gen/helpbar.milo_ps2",
                         helpbar_scene);
  MenuTextStyle helpbar_text_style =
      extract_menu_text_style(hdr, ark, "ui/gen/helpbar.milo_ps2",
                              "help_bar.txt");
  auto textentry_textures = asset::load_milo_textures(
      hdr, ark, "ui/gen/textentry.milo_ps2",
      {"char_arrow_up.tex", "char_arrow_down.tex"});
  milo_scene::Scene textentry_scene;
  milo_scene::load_scene(hdr, ark, "ui/gen/textentry.milo_ps2",
                         textentry_scene);
  asset::Image setlist_title_ink;
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel_showing(panel)) continue;
    std::string file = panel_file(panel);
    if (file.empty()) continue;
    if (file == "chooseprof.milo" || file == "manage_band.milo")
      foreground_panel_meshes.insert("notebook_cover.mesh");
    if (file == "multi_char_outfit1.milo" ||
        file == "multi_char_outfit2.milo") {
      // The outfit backing and its labels are one foreground panel in stock.
      // Characters are rendered as live scenes after the combined menu scene,
      // so defer every active outfit drawable until the text overlay pass:
      // backing first, then glyphs. Otherwise the character is composited
      // between the backing plate and the outfit names.
      milo_scene::Scene outfit_scene;
      if (milo_scene::load_scene(hdr, ark, "ui/gen/" + file + "_ps2",
                                 outfit_scene)) {
        for (const auto& mesh : outfit_scene.meshes)
        {
          foreground_panel_meshes.insert(mesh.name);
          if (std::getenv("GHOGX_MENU_LABEL_TRACE"))
            std::fprintf(stderr, "[menu-outfit-foreground] owner=%s mesh=%s\n",
                         pn.c_str(), mesh.name.c_str());
        }
        foreground_before_text = true;
      }
    }
    // store.milo::us_gate.tnm targets us_gate.trans.  The shutter begins a
    // contiguous source-authored foreground stack: handle and door trim follow
    // it, then the outer shop frame.  Defer the complete stack together so the
    // shutter remains over the labels without being composited over the frame
    // pieces which canonically follow it.
    if (file == "store.milo") {
      for (const char* mesh : {
               "us_gate.mesh", "us_gate_handle.mesh", "us_gate_side.mesh",
               "us_gate_frame.mesh", "us_right.mesh", "us_left.mesh",
               "us_bottom.mesh", "us_top.mesh"})
        foreground_panel_meshes.insert(mesh);
    }
    auto labels = extract_menu_labels(hdr, ark, "ui/gen/" + file + "_ps2");
    for (const auto& label : labels)
      if (label.type == "BandTextEntry") text_entries.push_back(label);
    std::unordered_set<std::string> panel_fonts;
    // HelpBarPanel expands its display array into per-control resource slots;
    // help_bar.txt is that template, not an additional static footer label.
    // append_help_footer below performs the native panel expansion.
    if (pn != Symbol("helpbar")) {
      for (const MenuLabel& label : labels) {
        if (!label.font.empty())
          panel_fonts.insert(normalized_label_font(label.font));
      }
    }
    for (const std::string& family : panel_fonts) {
      if (const MenuFont* source_font = fonts.get(family)) {
        append_text_quads(labels, *source_font, family, mgr, locale, focused,
                          disabled, label_verts[family],
                          label_transform_spans[family]);
      }
    }
    auto panel_checkboxes =
        extract_menu_checkboxes(hdr, ark, "ui/gen/" + file + "_ps2");
    for (auto& checkbox : panel_checkboxes)
      checkboxes.push_back(std::move(checkbox));
    auto panel_sliders =
        extract_menu_sliders(hdr, ark, "ui/gen/" + file + "_ps2");
    for (auto& slider : panel_sliders)
      sliders.push_back(std::move(slider));
  }
  const MenuFont& helpbar_font =
      helvetica_font.valid() ? helvetica_font : font;
  append_help_footer(screen, helpbar_font, helpbar_text_style, mgr, focused,
                     renderer.camera(), locale, help_icons, helpbar_scene,
                     helpbar_verts, helpbar_batches);
  if (screen && screen->name() == Symbol("qp_selsong_screen")) {
    auto setlist_title = asset::load_milo_textures(
        hdr, ark, "ui/gen/sel_song_quickplay.milo_ps2", {"setlist_top.tex"});
    if (auto it = setlist_title.find("setlist_top.tex");
        it != setlist_title.end() && it->second.valid()) {
      setlist_title_ink = ink_alpha_image(std::move(it->second));
      std::vector<ghogx::render::MiloSceneRenderer::TextVertex> title_verts;
      append_image_quad(15.0f, 0.0f, 205.0f, 350.0f, 160.0f, 0xFFFFFFFFu,
                        title_verts);
      batches.push_back({std::move(title_verts), &setlist_title_ink});
    }
  }
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> song_verts;
  if (screen && screen->name() == Symbol("qp_selsong_screen") && song_font.valid()) {
    append_quickplay_song_list(hdr, ark, mgr, db, locale, song_font, song_verts);
  }
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> credit_verts;
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> credit_rock_verts;
  if (screen && screen->name() == Symbol("credits_screen") && credits_font.valid()) {
    append_credits_list(hdr, ark, mgr, db, credits_font, rockletters_font,
                        credit_verts, credit_rock_verts);
  }
  if (screen && screen->name() == Symbol("practice_sel_section_screen")) {
    append_provider_list(hdr, ark, mgr, locale, song_font,
                         "ui/gen/practice_sel_section.milo_ps2",
                         "sel_section.lst", "ui/gen/list_section.milo_ps2",
                         "list.txt", song_verts);
  }
  if (screen && screen->name() == Symbol("endgame_stats_screen")) {
    append_endgame_stats_list(hdr, ark, mgr, locale, receipt_font,
                              label_verts["receipt"]);
  }
  if (screen && screen->name() == Symbol("practice_sel_section_screen"))
    configure_practice_section_selection_mesh(hdr, ark, mgr, renderer);
  asset::Image checkbox_on;
  asset::Image checkbox_off;
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> checkbox_on_verts;
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> checkbox_off_verts;
  std::vector<ghogx::render::MiloSceneRenderer::TextTransformSpan>
      checkbox_on_spans;
  std::vector<ghogx::render::MiloSceneRenderer::TextTransformSpan>
      checkbox_off_spans;
  if (!checkboxes.empty()) {
    milo_scene::Scene checkbox_scene;
    if (milo_scene::load_scene(hdr, ark, "ui/gen/checkbox.milo_ps2",
                               checkbox_scene)) {
      auto checkbox_textures = asset::load_milo_textures(
          hdr, ark, "ui/gen/checkbox.milo_ps2",
          {"checkbox_on.tex", "checkbox_off.tex"});
      if (auto it = checkbox_textures.find("checkbox_on.tex");
          it != checkbox_textures.end())
        checkbox_on = std::move(it->second);
      if (auto it = checkbox_textures.find("checkbox_off.tex");
          it != checkbox_textures.end())
        checkbox_off = std::move(it->second);
      append_checkbox_widgets(mgr, checkboxes, checkbox_scene,
                              checkbox_on_verts, checkbox_off_verts,
                              checkbox_on_spans, checkbox_off_spans);
    }
  }
  milo_scene::Scene slider_scene;
  MenuSliderAnim slider_anim;
  std::map<std::string, asset::Image> slider_textures;
  if (!sliders.empty() && helvetica_black_font.valid())
    append_slider_token_labels(mgr, focused, disabled, sliders,
                               helvetica_black_font, locale,
                               label_verts["helveticablack"],
                               label_transform_spans["helveticablack"]);
  if (!sliders.empty() &&
      milo_scene::load_scene(hdr, ark, "ui/gen/slider.milo_ps2", slider_scene)) {
    slider_anim = extract_menu_slider_anim(
        hdr, ark, "ui/gen/slider.milo_ps2", "char_slider.tnm");
    slider_textures = asset::load_milo_textures(
        hdr, ark, "ui/gen/slider.milo_ps2",
        {"slider_base.tex", "slider_knob.tex"});
    append_slider_widgets(mgr, focused, sliders, slider_scene, slider_anim,
                          slider_textures, batches);
  }
  append_text_entry_widgets(mgr, text_entries, rockletters_font, locale,
                            textentry_scene, textentry_textures, batches);
  std::size_t label_vertex_count = 0;
  for (const auto& [family, vertices] : label_verts) {
    (void)family;
    label_vertex_count += vertices.size();
  }
  std::fprintf(stderr, "[menu] focused component = '%s'\n", focused.c_str());
  std::fprintf(stderr,
               "[menu] text: %zu glyph-verts, song-list: %zu glyph-verts, "
               "credits: %zu glyph-verts, "
               "checkboxes: %zu on-verts/%zu off-verts, sliders: %zu\n",
               label_vertex_count, song_verts.size(),
               credit_verts.size() + credit_rock_verts.size(),
               checkbox_on_verts.size(), checkbox_off_verts.size(),
               sliders.size());
  {
    ghogx::render::MiloSceneRenderer::TextBatch batch;
    batch.verts = std::move(checkbox_off_verts);
    batch.atlas = &checkbox_off;
    batch.transform_spans = std::move(checkbox_off_spans);
    batches.push_back(std::move(batch));
  }
  {
    ghogx::render::MiloSceneRenderer::TextBatch batch;
    batch.verts = std::move(checkbox_on_verts);
    batch.atlas = &checkbox_on;
    batch.transform_spans = std::move(checkbox_on_spans);
    batches.push_back(std::move(batch));
  }
  for (auto& [family, vertices] : label_verts) {
    if (const MenuFont* source_font = fonts.get(family)) {
      ghogx::render::MiloSceneRenderer::TextBatch batch;
      batch.verts = std::move(vertices);
      batch.atlas = &source_font->atlas();
      batch.transform_spans = std::move(label_transform_spans[family]);
      batches.push_back(std::move(batch));
    }
  }
  {
    ghogx::render::MiloSceneRenderer::TextBatch batch;
    batch.verts = std::move(song_verts);
    batch.atlas = &song_font.atlas();
    batches.push_back(std::move(batch));
  }
  {
    ghogx::render::MiloSceneRenderer::TextBatch batch;
    batch.verts = std::move(credit_verts);
    batch.atlas = &credits_font.atlas();
    batches.push_back(std::move(batch));
  }
  if (rockletters_font.valid()) {
    ghogx::render::MiloSceneRenderer::TextBatch batch;
    batch.verts = std::move(credit_rock_verts);
    batch.atlas = &rockletters_font.atlas();
    batches.push_back(std::move(batch));
  }
  // chooseprof.milo authors notebook_cover.mesh as the foreground leaf driven
  // by notebook_cover.tnm/.filt. RndDrawable ordering places it after the page
  // labels while closed; keep HelpBarPanel screen-front after that leaf.
  if (!foreground_panel_meshes.empty()) {
    renderer.set_post_text_meshes(std::move(foreground_panel_meshes));
    renderer.set_post_text_mesh_text_split(foreground_before_text
                                               ? 0
                                               : batches.size());
  }
  for (auto& batch : helpbar_batches) batches.push_back(std::move(batch));
  if (!helpbar_verts.empty()) {
    ghogx::render::MiloSceneRenderer::TextBatch batch;
    batch.verts = std::move(helpbar_verts);
    batch.atlas = &helpbar_font.atlas();
    batches.push_back(std::move(batch));
  }
  renderer.set_text_batches(std::move(batches));
}

}  // namespace

int run_menu_mode(const std::string& hdr, const std::string& ark,
                  const std::string& screenshot_path, int screenshot_frame,
                  int max_frames, int window_width, int window_height,
                  float fixed_dt, const MenuRunOptions& options) {
  // 1. Boot the menu logic engine: classes, all screens (verbatim), game-side.
  register_ui_classes();
  ScreenManager mgr;
  install_default_singletons(mgr);

  gh::ark::ArkV3Reader arkr = gh::ark::ArkV3Reader::load(hdr);
  std::vector<std::string> arks = {ark};
  int n = load_all_ui_screens(arkr, arks, mgr);
  ConfigDb db;
  db.load(arkr, arks);
  install_meta_singletons(mgr, db);

  // Some stock panel (file) expressions depend on live meta state. In
  // particular, unlock_venue_panel resolves unlockvenue<campaign-status> and
  // has no status-zero asset. Materialize diagnostic reward state before MILO
  // widget ingestion so the referenced RndGroup/animation objects exist when
  // TRANSITION_COMPLETE_MSG starts their task. The same state is restored
  // below after init.dtb, which may reset campaign defaults.
  const bool seed_unlock_venue_widgets =
      std::getenv("GHOGX_MENU_START_SCREEN") &&
      std::strcmp(std::getenv("GHOGX_MENU_START_SCREEN"),
                  "unlock_venue_screen") == 0 &&
      std::getenv("GHOGX_MENU_SEED_UNLOCK_VENUE");
  if (seed_unlock_venue_widgets) {
    if (Object* campaign = mgr.resolve_object(Symbol("campaign")))
      campaign->set_property(Symbol("status"), DataNode::Int(1));
  }
  int widget_n = load_panel_milo_widgets(arkr, arks, mgr);
  std::fprintf(stderr, "[menu] booted: %d DTBs, %d MILO widgets, %zu objects, %zu songs\n",
               n, widget_n, mgr.registry().size(), db.song_count());

  // Font families are authored per Text/BandLabel/BandButton in each panel
  // MILO. Load them lazily from the corresponding stock RndDir instead of
  // maintaining a screen-dependent font whitelist.
  MenuFontCatalog fonts(hdr, ark);
  std::vector<asset::Image> boot_slides;
  for (const char* source : {"ui/gen/pub_splash.milo_ps2",
                             "ui/gen/activision_splash.milo_ps2",
                             "ui/gen/harmonix_splash.milo_ps2"}) {
    asset::Image slide = asset::load_milo_texture(hdr, ark, source);
    if (slide.valid()) boot_slides.push_back(std::move(slide));
  }
  std::map<std::string, std::string> locale = load_locale(arkr, arks);
  seed_source_list_layouts(hdr, ark, mgr, db, locale);
  const MenuMaterialAnim loading_word_material_anim = extract_menu_material_anim(
      hdr, ark, "ui/gen/loading.milo_ps2", "loading_word.mnm");
  const std::size_t credit_count = credits_entries(db).size();
  const MetaMusicConfig meta_music_config =
      load_meta_music_config(arkr, arks);

  ghogx::game::AudioPlayer menu_audio;
  ghogx::game::AudioPlayer meta_music_audio;
  bool meta_music_requested = false;
  bool menu_sfx_ready = false;
  Symbol loaded_menu_audio_venue;
  bool loaded_menu_audio_tutorial = false;
  auto current_menu_audio_venue = [&]() {
    if (Object* game = mgr.resolve_object(Symbol("game"))) {
      const Symbol venue = symbol_value(game->get_property(Symbol("venue")));
      if (venue.valid() && db.is_venue(venue)) return venue;
    }
    return db.default_venue();
  };
  auto ensure_menu_sfx_banks = [&]() {
    const Symbol venue = current_menu_audio_venue();
    bool tutorial_running = false;
    if (Object* game = mgr.resolve_object(Symbol("game"))) {
      tutorial_running =
          node_bool(game->get_property(Symbol("tutorial_running")));
    }
    if (menu_sfx_ready && venue == loaded_menu_audio_venue &&
        tutorial_running == loaded_menu_audio_tutorial)
      return true;
    std::vector<std::string> banks = {
        "sfx/gen/ingame_bank.milo_ps2",
        "sfx/gen/practice_bank.milo_ps2",
        "sfx/gen/metagame_bank.milo_ps2"};
    if (tutorial_running)
      banks.push_back("tutorial/gen/tutorial_bank.milo_ps2");
    if (venue.valid()) {
      const std::string key = venue.c_str();
      banks.push_back("world/" + key + "/gen/" + key +
                      "_bank.milo_ps2");
    }
    menu_sfx_ready = menu_audio.load_sfx_banks(hdr, ark, banks);
    loaded_menu_audio_venue = venue;
    loaded_menu_audio_tutorial = tutorial_running;
    std::fprintf(stderr,
                 "[menu-audio] stock banks ready=%d venue=%s tutorial=%d\n",
                 menu_sfx_ready ? 1 : 0,
                 venue.valid() ? venue.c_str() : "-",
                 tutorial_running ? 1 : 0);
    return menu_sfx_ready;
  };
  ensure_menu_sfx_banks();
  mgr.set_audio_event_handler(
      [&](Symbol action, Symbol cue, bool value) {
        if (action == Symbol("play_sequence") ||
            action == Symbol("play_sfx")) {
          if (cue.valid() && ensure_menu_sfx_banks())
            menu_audio.play_sfx(cue.c_str());
        } else if (action == Symbol("stop_sfx")) {
          if (cue.valid()) menu_audio.stop_sfx(cue.c_str());
        } else if (action == Symbol("stop_all_sfx")) {
          menu_audio.stop_all_sfx();
        } else if (action == Symbol("pause_all_sfx")) {
          menu_audio.pause_all_sfx(value);
        } else if (action == Symbol("meta_music")) {
          meta_music_requested = value;
        }
      });
  gh::dtb::NodeList init_roots =
      load_ui_script_roots_from_ark(arkr, arks, "ui/gen/init.dtb");
  if (!init_roots.empty()) {
    mgr.run_script(init_roots);
    std::fprintf(stderr, "[menu] ran stock init.dtb boot script: %zu roots\n",
                 init_roots.size());
  }
  if (!mgr.current_screen()) {
    // Last-ditch fallback for stripped asset sets. Stock data reaches here via
    // init.dtb: meta defaults, ui my_init, then ui goto_screen $first_screen.
    Symbol first_screen("bootup_load");
    if (!mgr.find_object(first_screen)) first_screen = Symbol("main_screen");
    mgr.set_global(Symbol("first_screen"), DataNode::Sym(first_screen));
    mgr.goto_screen(first_screen);
  }
  if (const char* missing = std::getenv("GHOGX_MENU_MISSING_CONTROLLER")) {
    if (std::strcmp(missing, "0") != 0 &&
        std::strcmp(missing, "FALSE") != 0 &&
        std::strcmp(missing, "false") != 0) {
      if (Object* game = mgr.resolve_object(Symbol("game")))
        game->handle_property(Symbol("set_missing_controller"),
                              one_arg(DataNode::Sym(Symbol("TRUE"))));
    }
  }
  if (const char* start = std::getenv("GHOGX_MENU_START_SCREEN")) {
    Symbol start_screen(start);
    if (mgr.find_object(start_screen)) {
      // unlock_venue_panel chooses unlockvenue<campaign-status>.milo while it
      // enters. The shipped sequence reaches it only after finish_song has
      // advanced status; status zero has no corresponding asset. Keep direct
      // starts honest by seeding the first real earned tier only on request.
      if (start_screen == Symbol("unlock_venue_screen") &&
          std::getenv("GHOGX_MENU_SEED_UNLOCK_VENUE")) {
        if (Object* campaign = mgr.resolve_object(Symbol("campaign")))
          campaign->set_property(Symbol("status"), DataNode::Int(1));
        std::fprintf(stderr,
                     "[menu] seeded stock unlock venue campaign status=1\n");
      }
      mgr.goto_screen(start_screen);
      std::fprintf(stderr, "[menu] capture start screen = %s\n", start);
    }
  }
  if (std::getenv("GHOGX_DUMP_MENU_SCREENS")) {
    for (std::size_t i = 0; i < mgr.registry().size(); ++i) {
      Object* object = mgr.registry().at(i);
      if (!object) continue;
      const Symbol type = object->class_name();
      if (type != Symbol("GHScreen") &&
          type != Symbol("MultiSelectScreen") &&
          type != Symbol("TrackBudgetScreen"))
        continue;
      std::fprintf(stderr, "[menu-audit] screen=%s class=%s panels=",
                   object->name().c_str(), type.c_str());
      bool first = true;
      for (Symbol panel_name : screen_panel_names(object)) {
        Object* panel = mgr.find_object(panel_name);
        std::fprintf(stderr, "%s%s:%s", first ? "" : ",",
                     panel_name.c_str(), panel_file(panel).c_str());
        first = false;
      }
      std::fprintf(stderr, "\n");
    }
  }

  // 2. Window + scene renderer.
  auto win = ghogx::render::Window::create(window_width, window_height,
                                           "Guitar Hero Classic");
  if (!win) { std::fprintf(stderr, "[menu] window/device create failed\n"); return 1; }
  ghogx::render::MiloSceneRenderer renderer(*win);
  ghogx::render::MiloSceneRenderer guitar_renderer(*win);
  ghogx::render::MiloSceneRenderer outgoing_renderer(*win);
  ghogx::render::MiloSceneRenderer outgoing_guitar_renderer(*win);
  std::vector<MenuCharacterPreview> character_previews;
  std::vector<MenuCharacterPreview> outgoing_character_previews;
  std::vector<LiveMenuAnimationSource> outgoing_menu_animation_sources;
  bool outgoing_transition_visible = false;
  bool outgoing_guitar_visible = false;
  ghogx::game::Gameplay gameplay;
  ghogx::hud::HudRenderer gameplay_hud;
  YouRockOverlay you_rock_overlay;
  auto* d3d = static_cast<IDirect3DDevice9*>(win->device_ptr());
  const bool hud_ready = gameplay_hud.load(d3d, hdr, ark);
  const MenuFont* rockletters_font = fonts.get("rockletters");
  const MenuFont* impact_font = fonts.get("impact");
  if (rockletters_font || impact_font)
    you_rock_overlay.load(d3d, rockletters_font ? *rockletters_font
                                                : *impact_font);

  enum class RuntimePhase { BootLogos, IntroVideo, Menus, Gameplay, YouRock };
  const bool explicit_start_screen = std::getenv("GHOGX_MENU_START_SCREEN") != nullptr;
  const bool disable_live_input =
      std::getenv("GHOGX_MENU_DISABLE_LIVE_INPUT") != nullptr;
  RuntimePhase phase =
      options.play_boot_presentation && !explicit_start_screen
          ? RuntimePhase::BootLogos
          : RuntimePhase::Menus;
  meta_music_requested = phase == RuntimePhase::Menus;
  float phase_seconds = 0.0f;
  float screen_seconds = 0.0f;
  PssVideoPlayerWin32 intro_video;
  bool intro_video_started = false;
  bool gameplay_loaded = false;
  bool gameplay_results_committed = false;
  bool auto_loop_completed = false;
  std::string automated_screen;
  int loaded_difficulty = std::clamp(options.preferred_difficulty, 0, 3);
  std::string loaded_song;

  std::mt19937 meta_music_rng{std::random_device{}()};
  std::vector<std::size_t> meta_music_order;
  std::size_t meta_music_order_pos = 0;
  bool meta_music_loaded = false;
  bool meta_music_paused = false;
  float meta_music_gain = 0.0f;
  float meta_background_delay = 0.0f;
  const float meta_music_target_gain =
      std::pow(10.0f, meta_music_config.volume_db / 20.0f);
  auto refill_meta_music_order = [&]() {
    meta_music_order.resize(meta_music_config.tracks.size());
    for (std::size_t i = 0; i < meta_music_order.size(); ++i)
      meta_music_order[i] = i;
    std::shuffle(meta_music_order.begin(), meta_music_order.end(),
                 meta_music_rng);
    meta_music_order_pos = 0;
  };
  auto load_next_meta_music_track = [&]() {
    if (meta_music_config.tracks.empty()) return false;
    if (meta_music_order_pos >= meta_music_order.size())
      refill_meta_music_order();
    const std::string& name =
        meta_music_config.tracks[meta_music_order[meta_music_order_pos++]];
    const std::string path = "sfx/streams/" + name + ".vgs";
    if (!meta_music_audio.load_vgs(hdr, ark, path)) return false;
    meta_music_audio.set_output_volume(meta_music_gain);
    meta_music_audio.play();
    meta_music_loaded = true;
    meta_music_paused = false;
    std::fprintf(stderr, "[menu-audio] meta track=%s source=synth.dtb\n",
                 path.c_str());
    return true;
  };
  auto schedule_meta_background = [&]() {
    const float lo = std::max(0.0f, meta_music_config.background_min_delay);
    const float hi = std::max(lo, meta_music_config.background_max_delay);
    meta_background_delay =
        hi > lo ? std::uniform_real_distribution<float>(lo, hi)(meta_music_rng)
                : lo;
  };
  auto update_meta_music = [&](float dt) {
    if (meta_music_requested) {
      if (!meta_music_loaded) {
        load_next_meta_music_track();
      } else if (meta_music_paused) {
        meta_music_audio.play();
        meta_music_paused = false;
      } else if (!meta_music_audio.is_playing()) {
        load_next_meta_music_track();
      }
    }

    const float target = meta_music_requested ? meta_music_target_gain : 0.0f;
    const float fade_seconds = std::max(0.001f, meta_music_config.fade_seconds);
    const float step = meta_music_target_gain * dt / fade_seconds;
    if (meta_music_gain < target)
      meta_music_gain = std::min(target, meta_music_gain + step);
    else if (meta_music_gain > target)
      meta_music_gain = std::max(target, meta_music_gain - step);
    if (meta_music_loaded) meta_music_audio.set_output_volume(meta_music_gain);
    if (!meta_music_requested && meta_music_loaded && !meta_music_paused &&
        meta_music_gain <= 0.0f) {
      meta_music_audio.stop();
      meta_music_paused = true;
      if (!meta_music_config.background_sequence.empty())
        menu_audio.stop_sfx(meta_music_config.background_sequence);
    }

    if (meta_music_requested &&
        !meta_music_config.background_sequence.empty()) {
      if (meta_background_delay <= 0.0f) schedule_meta_background();
      meta_background_delay -= dt;
      if (meta_background_delay <= 0.0f) {
        menu_audio.play_sfx(meta_music_config.background_sequence);
        schedule_meta_background();
      }
    } else {
      meta_background_delay = 0.0f;
    }
  };

  const std::filesystem::path intro_path =
      std::filesystem::path(hdr).parent_path().parent_path() / "videos" /
      "intro.pss";
  auto route_after_intro = [&]() {
    intro_video.close();
    meta_music_requested = true;
    Object* campaign = mgr.resolve_object(Symbol("campaign"));
    const int profiles = campaign
                             ? campaign->handle_property(Symbol("num_profiles"),
                                                         DataArray())
                                   .as_int()
                                   .value_or(0)
                             : 0;
    Symbol target = profiles > 0 ? Symbol("splash_screen")
                                 : Symbol("guitar_help_screen");
    if (!mgr.find_object(target)) target = Symbol("splash_screen");
    if (!mgr.find_object(target)) target = Symbol("main_screen");
    mgr.goto_screen(target);
    phase = RuntimePhase::Menus;
    phase_seconds = 0.0f;
    screen_seconds = 0.0f;
    std::fprintf(stderr, "[boot] intro complete -> %s\n", target.c_str());
  };

  auto start_intro_video = [&]() {
    phase = RuntimePhase::IntroVideo;
    phase_seconds = 0.0f;
    intro_video_started = intro_video.open(intro_path.string());
    if (!intro_video_started) route_after_intro();
  };
  // GH2 PS2 menu cameras are authored for a 4:3 frame. Keep the renderer-wide
  // GHOGX_CAMERA_ASPECT override available for diagnostics, but do not let a
  // widescreen backbuffer silently stretch source-authored menu geometry.
  constexpr float kPs2MenuCameraAspect = 4.0f / 3.0f;
  renderer.set_default_camera_aspect(kPs2MenuCameraAspect);
  guitar_renderer.set_default_camera_aspect(kPs2MenuCameraAspect);

  Object* shown = mgr.current_screen();
  // Run one poll tick before the first text build so panel `poll` handlers have
  // set their state (e.g. multiplayer disabled via is_missing_multi_controller).
  mgr.update(0.0f);
  // Capture harness for the stateful campaign reward screen. A bare direct
  // start intentionally has no award; seed one stock tour reward and re-enter
  // the shipped panel handler only when explicitly requested.
  if (shown && shown->name() == Symbol("unlock_guitar_screen") &&
      std::getenv("GHOGX_MENU_SEED_UNLOCK_GUITAR")) {
    Symbol award;
    for (Symbol item : db.store_items(Symbol("guitar"))) {
      const DataArray* guitar = db.guitar(item);
      if (!guitar) continue;
      const Symbol require =
          ConfigDb::field(guitar, Symbol("require"))
              .as_symbol().value_or(Symbol());
      if (require == Symbol("tour_passed") ||
          require == Symbol("tour_5_star")) {
        award = item;
        break;
      }
    }
    if (award.valid()) {
      if (Object* campaign = mgr.resolve_object(Symbol("campaign"))) {
        campaign->set_property(
            Symbol((std::string("pending_guitar_award.") + award.c_str()).c_str()),
            DataNode::Int(1));
      }
      if (Object* panel = mgr.find_object(Symbol("unlock_guitar_panel")))
        panel->handle_property(Symbol("enter"), DataArray());
      std::fprintf(stderr, "[menu] seeded stock unlock guitar award=%s\n",
                   award.c_str());
    }
  }
  rebuild_scene(hdr, ark, mgr, shown, renderer);
  bool guitar_visible =
      rebuild_guitar_display_scene(hdr, ark, mgr, shown, db, guitar_renderer);
  character_previews =
      rebuild_character_display_scenes(hdr, ark, mgr, shown, *win);
  if (!character_previews.empty()) mgr.update(0.0f);
  apply_loading_source_anims(hdr, ark, mgr, shown, renderer);
  rebuild_text(hdr, ark, mgr, shown, renderer, fonts, db, locale);
  auto live_menu_animation_sources =
      collect_live_menu_animation_sources(hdr, ark, mgr, shown);
  auto draw_menu_layers =
      [](ghogx::render::MiloSceneRenderer& scene_renderer,
         ghogx::render::MiloSceneRenderer& live_guitar_renderer,
         bool live_guitar_visible,
         std::vector<MenuCharacterPreview>& live_characters,
         bool clear_target) {
        if (live_guitar_visible || !live_characters.empty()) {
          if (clear_target)
            scene_renderer.draw_scene_only();
          else
            scene_renderer.draw_scene_only_over_scene();
          // Live guitar scenes are already parented to source menu proxies or
          // guitar_display placers. Screen-local proxy scenes use metacam;
          // shared guitar_display rigs carry their own authored guitar camera.
          if (live_guitar_visible) {
            if (live_guitar_renderer.camera().authored)
              live_guitar_renderer.draw_scene_only_over_scene_preserving_state();
            else {
              auto proxy_camera = scene_renderer.camera();
              // Panel cameras use their authored clipping range for the panel
              // geometry. UIProxy draws its loaded directory with RndDir's
              // broad proxy range; store's guitar sits only ~120 units from
              // the store camera and would be rejected by its near=400 plane.
              proxy_camera.near_z = 1.0f;
              proxy_camera.far_z = std::max(proxy_camera.far_z, 5000.0f);
              live_guitar_renderer.draw_scene_only_over_scene_preserving_state(
                  proxy_camera);
            }
          }
          // Character and guitar display panels retain their last selections
          // across adjacent flows, but stock guitar screens draw the guitar
          // proxy exclusively. A populated CharsysPanel is state, not another
          // visible preview layer, while a GuitarDisplayPanel is active.
          if (!live_guitar_visible) {
            for (auto& character : live_characters) {
              if (!character.environment.empty())
                scene_renderer.apply_environment_lighting_state(
                    character.environment);
              character.renderer->draw_over_scene(scene_renderer.camera());
            }
          }
          scene_renderer.draw_text_over_scene();
        } else {
          if (clear_target)
            scene_renderer.draw();
          else
            scene_renderer.draw_over_scene(scene_renderer.camera());
        }
      };

  // Audit mode (GHOGX_MENU_DUMP=1): visit every stock screen-class object,
  // + text, and report the mesh/texture/glyph counts. The fastest way to find
  // screens that don't render (no panels, missing MILO, empty text) — no nav
  // needed. Prints a one-line summary per screen, then exits.
  if (std::getenv("GHOGX_MENU_DUMP")) {
    ObjectDir& reg = mgr.registry();
    int ok = 0, failed = 0, empty = 0, redirected = 0, total = 0;
    int ui_resources = 0, runtime_resources = 0, dynamic_resources = 0;
    const auto routes = collect_ui_route_refs(mgr);
    int literal_routes = 0, dynamic_routes = 0, unresolved_routes = 0;
    for (const UiRouteRef& route : routes) {
      if (route.dynamic) {
        ++dynamic_routes;
        std::fprintf(stderr,
                     "[dump-route-dynamic] owner=%s op=%s expr=%s line=%u\n",
                     route.owner.c_str(), route.operation.c_str(),
                     route.target.c_str(), route.source_line);
        continue;
      }
      if (route.target.empty() && route.operation == "pop_screen") continue;
      ++literal_routes;
      if (!mgr.find_object(Symbol(route.target.c_str()))) {
        ++unresolved_routes;
        std::fprintf(stderr,
                     "[dump-route-missing] owner=%s op=%s target=%s line=%u\n",
                     route.owner.c_str(), route.operation.c_str(),
                     route.target.c_str(), route.source_line);
      }
    }
    std::unordered_map<std::string, int> font_usage;
    for (std::size_t i = 0; i < reg.size(); ++i) {
      Object* o = reg.at(i);
      if (!o) continue;
      std::string nm = o->name().c_str();
      const Symbol cls = o->class_name();
      if (cls != Symbol("GHScreen") && cls != Symbol("MultiSelectScreen") &&
          cls != Symbol("TrackBudgetScreen"))
        continue;
      ++total;
      mgr.goto_screen(o->name());
      mgr.update(0.0f);
      Object* s = mgr.current_screen();
      std::vector<Symbol> pn = screen_panel_names(o);
      int missing_panels = 0;
      int resource_panels = 0;
      int missing_resources = 0;
      int labels = 0;
      int lists = 0;
      int checkboxes = 0;
      int sliders = 0;
      for (Symbol panel_name : pn) {
        Object* panel = mgr.find_object(panel_name);
        if (!panel) {
          ++missing_panels;
          continue;
        }
        const std::string file = panel_file(panel);
        if (file.empty()) continue;
        ++resource_panels;
        std::string path = stock_milo_path_for_panel_file(arkr, file);
        if (path.empty()) {
          const auto dynamic_paths = stock_dynamic_ui_milo_paths(arkr, file);
          if (dynamic_paths.empty()) {
            std::fprintf(stderr,
                         "[dump-missing] screen=%s panel=%s class=%s file=%s "
                         "ui_guess=%s\n",
                         nm.c_str(), panel_name.c_str(),
                         panel->class_name().c_str(), file.c_str(),
                         menu_milo_path_for_file(file).c_str());
            ++missing_resources;
            continue;
          }
          ++dynamic_resources;
          // Audit every authored variant. The blind screen walk can evaluate a
          // campaign-status expression to unlockvenue0.milo, but shipped flow
          // only enters it with a valid status and selects variants 1..7.
          for (const std::string& dynamic_path : dynamic_paths) {
            labels += static_cast<int>(
                extract_menu_labels(hdr, ark, dynamic_path).size());
          }
          continue;
        }
        if (path.rfind("ui/gen/", 0) == 0)
          ++ui_resources;
        else
          ++runtime_resources;
        // Runtime-owned track/hud MILOs are valid members of an in-game
        // UIScreen, but they are not menu panels and should not feed the menu
        // label/widget audit.
        if (path.rfind("ui/gen/", 0) != 0) continue;
        const auto source_labels = extract_menu_labels(hdr, ark, path);
        labels += static_cast<int>(source_labels.size());
        for (const MenuLabel& label : source_labels) {
          if (!label.font.empty()) {
            ++font_usage[label.font];
            if (label.font == "helpbar.view" || label.font == "cs_set.grp") {
              std::fprintf(stderr,
                           "[dump-font-source] screen=%s panel=%s milo=%s "
                           "object=%s type=%s font=%s parent=%s text=%s\n",
                           nm.c_str(), panel_name.c_str(), path.c_str(),
                           label.name.c_str(), label.type.c_str(),
                           label.font.c_str(), label.parent.c_str(),
                           label.text.c_str());
            }
          }
        }
        checkboxes += static_cast<int>(
            extract_menu_checkboxes(hdr, ark, path).size());
        sliders +=
            static_cast<int>(extract_menu_sliders(hdr, ark, path).size());
        if (auto* dir = dynamic_cast<ObjectDir*>(panel)) {
          for (std::size_t child_i = 0; child_i < dir->size(); ++child_i) {
            Object* child = dir->at(child_i);
            if (child && (child->class_name() == Symbol("UIList") ||
                          child->class_name() == Symbol("BandList")))
              ++lists;
          }
        }
      }
      const bool was_redirected = s != o;
      if (was_redirected) ++redirected;
      const bool screen_ok = missing_panels == 0 && missing_resources == 0;
      if (screen_ok)
        ++ok;
      else
        ++failed;
      if (resource_panels == 0) ++empty;
      std::fprintf(
          stderr,
          "[dump] %-34s class=%-18s panels=%zu resource=%d missing_panel=%d "
          "missing_milo=%d labels=%d lists=%d checkboxes=%d sliders=%d "
          "route=%s %s\n",
          nm.c_str(), cls.c_str(), pn.size(), resource_panels, missing_panels,
          missing_resources, labels, lists, checkboxes, sliders,
          was_redirected && s ? s->name().c_str() : "self",
          screen_ok ? "OK" : "FAIL");
      rebuild_scene(hdr, ark, mgr, s, renderer);
      guitar_visible =
          rebuild_guitar_display_scene(hdr, ark, mgr, s, db, guitar_renderer);
      apply_loading_source_anims(hdr, ark, mgr, s, renderer);
      rebuild_text(hdr, ark, mgr, s, renderer, fonts, db, locale);
      renderer.draw();
      win->present();
    }
    std::fprintf(stderr,
                 "[dump] %d stock screens audited (ok=%d failed=%d "
                 "logic_only=%d redirected=%d ui_milo=%d runtime_milo=%d "
                 "dynamic_ui=%d)\n",
                 total, ok, failed, empty, redirected, ui_resources,
                 runtime_resources, dynamic_resources);
    std::fprintf(stderr,
                 "[dump] authored routes=%zu literal=%d dynamic=%d "
                 "unresolved=%d\n",
                 routes.size(), literal_routes, dynamic_routes,
                 unresolved_routes);
    std::vector<std::pair<std::string, int>> font_counts(font_usage.begin(),
                                                         font_usage.end());
    std::sort(font_counts.begin(), font_counts.end());
    for (const auto& [name, uses] : font_counts)
      std::fprintf(stderr, "[dump-font] %-32s uses=%d\n", name.c_str(), uses);
    return 0;
  }

  // Per-screen nav state: the focusable components (with nav links) + disabled set.
  std::vector<MenuLabel> cur_labels = gather_labels(hdr, ark, mgr, shown);
  std::unordered_set<std::string> cur_disabled = compute_disabled(mgr);
  auto list_state_key = [&](Symbol list_name, int fallback_pos) -> std::string {
    std::string key = list_name.c_str();
    Object* list = mgr.resolve_object(list_name);
    const int pos =
        list ? list->handle_property(Symbol("selected_pos"), DataArray())
                   .as_int()
                   .value_or(fallback_pos)
             : fallback_pos;
    key += ":" + std::to_string(pos);
    if (list) {
      const int first = list->handle_property(Symbol("first_showing"), DataArray())
                            .as_int()
                            .value_or(0);
      const int target = list->get_property(Symbol("target_showing"))
                             .as_int()
                             .value_or(first);
      const int step =
          static_cast<int>(std::round(
              list->get_property(Symbol("scroll_step_percent"))
                  .as_float()
                  .value_or(1.0f) *
              1000.0f));
      key += ":" + std::to_string(first) + ":" + std::to_string(target) +
             ":" + std::to_string(step);
    }
    return key;
  };
  auto focus_name = [&]() -> std::string {
    Object* s = mgr.current_screen();
    if (!s) return "";
    auto live_widget_state = [&](std::string& key) {
      for (Symbol pn : screen_panel_names(s)) {
        Object* panel = mgr.find_object(pn);
        auto* dir = dynamic_cast<ObjectDir*>(panel);
        if (!dir) continue;
        for (std::size_t i = 0; i < dir->size(); ++i) {
          Object* child = dir->at(i);
          if (!child) continue;
          Symbol cls = child->class_name();
          if (cls == Symbol("CheckBox") || cls == Symbol("CheckboxDisplay")) {
            key += "|";
            key += child->name().c_str();
            key += "=";
            key += std::to_string(child->handle_property(Symbol("get_check"),
                                                         DataArray())
                                      .as_int()
                                      .value_or(0));
          } else if (cls == Symbol("UISlider") || cls == Symbol("BandSlider")) {
            key += "|";
            key += child->name().c_str();
            key += "=";
            key += std::to_string(child->handle_property(Symbol("current"),
                                                         DataArray())
                                      .as_int()
                                      .value_or(0));
            key += "/";
            key += std::to_string(child->handle_property(Symbol("num_steps"),
                                                         DataArray())
                                      .as_int()
                                      .value_or(1));
          }
        }
      }
    };
    if (s->name() == Symbol("credits_screen")) {
      std::string key = list_state_key(Symbol("credits.lst"), 0);
      live_widget_state(key);
      return key;
    }
    Symbol fpn = s->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
    Object* p = fpn.valid() ? mgr.find_object(fpn) : nullptr;
    std::string f = p ? p->get_property(Symbol("focus")).as_symbol().value_or(Symbol()).c_str() : "";
    if (f.size() > 4 && f.compare(f.size() - 4, 4, ".lst") == 0) {
      int fallback_pos = 0;
      if (Object* list = mgr.resolve_object(Symbol(f.c_str())))
        fallback_pos =
            list->handle_property(Symbol("selected_pos"), DataArray())
                .as_int()
                .value_or(0);
      else if (f == "ss_song.lst") {
        if (Object* panel = mgr.find_object(Symbol("sel_song_panel")))
          fallback_pos =
              panel->get_property(Symbol("ss_song_selected")).as_int().value_or(0);
      }
      f = list_state_key(Symbol(f.c_str()), fallback_pos);
    }
    live_widget_state(f);
    return f;
  };
  std::string last_focus = focus_name();

  // Headless auto-nav harness
  // (GHOGX_MENU_NAV="up,confirm,text:CODEX,confirm,...") — one action every
  // kNavStep frames.  text: does not bypass the stock profile flow: it writes
  // through the focused BandTextEntry and its normal send_select path emits
  // TEXT_ENTRY_MSG to the owning screen.
  std::vector<std::string> nav;
  if (const char* env = std::getenv("GHOGX_MENU_NAV")) {
    std::string e(env), tok;
    for (char ch : e + ",") { if (ch == ',') { if (!tok.empty()) nav.push_back(tok); tok.clear(); } else tok += ch; }
  }
  size_t nav_i = 0;
  const uint64_t kNavStep = 5;

  auto current_song_index = [&]() -> std::size_t {
    Object* game = mgr.resolve_object(Symbol("game"));
    const int value =
        game ? game->get_property(Symbol("song_index")).as_int().value_or(0)
             : 0;
    if (db.song_count() == 0) return 0;
    return std::min<std::size_t>(static_cast<std::size_t>(std::max(0, value)),
                                 db.song_count() - 1);
  };
  auto current_difficulty = [&]() -> int {
    Object* player = mgr.resolve_object(Symbol("player0"));
    const Symbol value =
        player ? player->get_property(Symbol("difficulty"))
                     .as_symbol()
                     .value_or(Symbol("kDifficultyMedium"))
               : Symbol("kDifficultyMedium");
    if (value == Symbol("kDifficultyEasy")) return 0;
    if (value == Symbol("kDifficultyHard")) return 2;
    if (value == Symbol("kDifficultyExpert")) return 3;
    return 1;
  };
  auto set_selected_song = [&](std::size_t index) {
    if (db.song_count() == 0) return;
    index = std::min(index, db.song_count() - 1);
    if (Object* list = mgr.resolve_object(Symbol("ss_song.lst")))
      list->handle_property(Symbol("set_selected"),
                            one_arg(DataNode::Int(static_cast<int>(index))));
    if (Object* panel = mgr.find_object(Symbol("sel_song_panel")))
      panel->set_property(Symbol("ss_song_selected"),
                          DataNode::Int(static_cast<int>(index)));
    if (Object* game = mgr.resolve_object(Symbol("game")))
      game->handle_property(Symbol("set_song_index"),
                            one_arg(DataNode::Int(static_cast<int>(index))));
  };
  auto preferred_song_index = [&]() -> std::size_t {
    if (options.preferred_song.empty()) return current_song_index();
    for (std::size_t i = 0; i < db.song_count(); ++i) {
      if (options.preferred_song == db.song_key(i).c_str()) return i;
    }
    return current_song_index();
  };
  auto prepare_gameplay = [&]() -> bool {
    const std::size_t index = current_song_index();
    const Symbol song = db.song_key(index);
    if (!song.valid()) {
      std::fprintf(stderr, "[flow] no selected song at index %zu\n", index);
      return false;
    }
    loaded_song = song.c_str();
    loaded_difficulty = current_difficulty();
    gameplay.set_diagnostic_autoplay(options.gameplay_autoplay);
    gameplay.set_deterministic_clock(fixed_dt > 0.0f);
    if (!gameplay.load_song(hdr, ark, loaded_song, loaded_difficulty)) {
      std::fprintf(stderr, "[flow] gameplay load failed: %s diff=%d\n",
                   loaded_song.c_str(), loaded_difficulty);
      return false;
    }
    // Full-loop proof still enters through the shipped menu/loading route, but
    // may replay the already-loaded chart/world state near the song ending so
    // gameplay exit and the stock post-show chain can be verified without a
    // real-time song-length run. This is opt-in and has no retail-path effect.
    if (const char* seek =
            std::getenv("GHOGX_MENU_DIAGNOSTIC_SONG_START_SEC")) {
      const double seconds = std::max(0.0, std::atof(seek));
      if (seconds > 0.0) {
        gameplay.seek_for_diagnostic_capture(seconds);
        std::fprintf(stderr,
                     "[flow] diagnostic menu-loop song seek: %.3f sec\n",
                     seconds);
      }
    }
    if (Object* game = mgr.resolve_object(Symbol("game"))) {
      game->set_property(
          Symbol("result_character"),
          DataNode::Str(std::string(gameplay.quickplay_character_outfit())));
    }
    gameplay_loaded = true;
    gameplay_results_committed = false;
    std::fprintf(stderr, "[flow] gameplay ready: %s diff=%d autoplay=%d\n",
                 loaded_song.c_str(), loaded_difficulty,
                 options.gameplay_autoplay ? 1 : 0);
    return true;
  };
  auto commit_gameplay_results = [&]() {
    if (gameplay_results_committed) return;
    gameplay_results_committed = true;
    const int hit = std::max(0, gameplay.hit_count());
    const int miss = std::max(0, gameplay.miss_count());
    const int total = hit + miss;
    const int percent = total > 0
                            ? static_cast<int>(std::lround(
                                  100.0 * static_cast<double>(hit) / total))
                            : 0;
    const int stars = percent >= 95 ? 5 : percent >= 80 ? 4 : 3;
    const float average_multiplier = gameplay.average_multiplier();
    if (Object* player = mgr.resolve_object(Symbol("player0"))) {
      player->set_property(Symbol("score"), DataNode::Int(gameplay.score()));
      player->set_property(Symbol("percent_hit"), DataNode::Int(percent));
      player->set_property(Symbol("percent_complete"), DataNode::Int(100));
      player->set_property(Symbol("longest_streak"),
                           DataNode::Int(gameplay.longest_streak()));
      player->set_property(Symbol("gems_hit"), DataNode::Int(hit));
      player->set_property(Symbol("gems_passed"), DataNode::Int(miss));
      player->set_property(Symbol("avg_multiplier"),
                           DataNode::Float(average_multiplier));
      player->set_property(Symbol("num_stars"), DataNode::Int(stars));
      // endgame.milo's endgame_review_data.lbl uses the authored `stars` font;
      // its text is a star-glyph string, while num_stars remains numeric for
      // the headline and campaign scripts.
      player->set_property(Symbol("star_rating"),
                           DataNode::Str(std::string(stars, '*')));
      const int phrases = gameplay.completed_star_phrases();
      const int total_phrases = gameplay.total_star_phrases();
      player->set_property(Symbol("sp_phrases"),
                           DataNode::Str(std::to_string(phrases) + "/" +
                                         std::to_string(total_phrases)));
      player->set_property(Symbol("sp_phrases_captured"),
                           DataNode::Int(phrases));
      player->set_property(Symbol("sp_phrases_total"),
                           DataNode::Int(total_phrases));
      auto section_rows = std::make_shared<DataArray>();
      for (const auto& section : gameplay.section_results()) {
        auto row = std::make_shared<DataArray>();
        row->push(DataNode::Str(section.name));
        row->push(DataNode::Int(section.hit));
        row->push(DataNode::Int(section.total));
        section_rows->push(DataNode::Array(std::move(row)));
      }
      player->set_property(Symbol("section_stats"),
                           DataNode::Array(std::move(section_rows)));
    }
    if (Object* game = mgr.resolve_object(Symbol("game"))) {
      game->set_property(Symbol("song_duration_sec"),
                         DataNode::Float(static_cast<float>(gameplay.song_time())));
    }
    std::fprintf(stderr,
                 "[flow] results committed: score=%d hit=%d miss=%d percent=%d "
                 "streak=%d stars=%d\n",
                 gameplay.score(), hit, miss, percent,
                 gameplay.longest_streak(), stars);
  };
  auto draw_gameplay = [&](bool show_you_rock) {
    gameplay.draw(*win);
    if (hud_ready) {
      ghogx::hud::HudState state;
      state.score = gameplay.score();
      state.streak = gameplay.streak();
      state.multiplier = gameplay.multiplier() *
                         (gameplay.star_power_active() ? 2 : 1);
      state.sp_fill = gameplay.star_power_fill();
      state.sp_active = gameplay.star_power_active();
      state.rock_fill = gameplay.rock_fill();
      state.anim_seconds = static_cast<float>(gameplay.song_time());
      state.track_intro_active = gameplay.track_intro_active();
      gameplay_hud.draw(d3d, state);
    }
    if (show_you_rock)
      you_rock_overlay.draw(d3d, win->bb_width(), win->bb_height(),
                            phase_seconds);
  };

  auto automate_current_screen = [&]() -> bool {
    if (!options.automate_full_loop || auto_loop_completed || !shown)
      return false;
    const std::string screen = shown->name().c_str();
    if (automated_screen == screen) return false;
    const float dwell = (screen == "endgame_screen" ||
                         screen == "endgame_stats_screen" ||
                         screen == "highscore_screen")
                            ? 2.5f
                            : (screen == "complete_screen" ? 2.0f : 0.65f);
    if (screen_seconds < dwell) return false;
    if (screen == "guitar_help_screen" || screen == "splash_screen") {
      automated_screen = screen;
      do_confirm(mgr);
      return true;
    }
    if (screen == "main_screen") {
      Object* panel = mgr.find_object(Symbol("main_panel"));
      set_panel_focus(mgr, panel, "main_quickspin.btn");
      automated_screen = screen;
      do_confirm(mgr);
      return true;
    }
    if (screen == "qp_selsong_screen") {
      set_selected_song(preferred_song_index());
      automated_screen = screen;
      do_confirm(mgr);
      return true;
    }
    if (screen == "qp_diff_screen") {
      Object* panel = mgr.find_object(Symbol("sel_difficulty_panel"));
      set_panel_focus(
          mgr, panel,
          "sd_diff" +
              std::to_string(std::clamp(options.preferred_difficulty, 0, 3) +
                             1) +
              ".btn");
      automated_screen = screen;
      do_confirm(mgr);
      return true;
    }
    if (screen == "endgame_screen") {
      Object* panel = mgr.find_object(Symbol("endgame_panel"));
      set_panel_focus(mgr, panel, "me_morestats.btn");
      automated_screen = screen;
      do_confirm(mgr);
      return true;
    }
    if (screen == "endgame_stats_screen" || screen == "highscore_screen") {
      automated_screen = screen;
      do_confirm(mgr);
      return true;
    }
    if (screen == "complete_screen") {
      Object* panel = mgr.find_object(Symbol("complete_panel"));
      set_panel_focus(mgr, panel, "comp_selsong.btn");
      automated_screen = screen;
      do_confirm(mgr);
      auto_loop_completed = true;
      std::fprintf(stderr, "[flow] automated full loop returned to menus\n");
      return true;
    }
    return false;
  };

  using clock = std::chrono::steady_clock;
  auto last = clock::now();
  uint64_t frame = 0;
  int last_connected_gamepads = -1;
  int diagnostic_gamepad_count = -1;
  if (const char* count = std::getenv("GHOGX_MENU_GAMEPAD_COUNT"))
    diagnostic_gamepad_count = std::clamp(std::atoi(count), 0, 4);
  if (!screenshot_path.empty() && max_frames == 0) max_frames = screenshot_frame + 3;

  while (!win->should_close()) {
    win->pump();
    if (win->should_close()) break;

    const int connected_gamepads =
        diagnostic_gamepad_count >= 0 ? diagnostic_gamepad_count
                                      : win->connected_gamepads();
    if (connected_gamepads != last_connected_gamepads) {
      last_connected_gamepads = connected_gamepads;
      if (Object* game = mgr.resolve_object(Symbol("game"))) {
        game->set_property(Symbol("multiple_controllers"),
                           DataNode::Int(connected_gamepads >= 2 ? 1 : 0));
        game->set_property(Symbol("missing_multi_controller"),
                           DataNode::Int(connected_gamepads >= 2 ? 0 : 1));
      }
      // The first connected-controller sample occurs after the initial menu
      // build. Refresh stock-script disable state immediately so the authored
      // Multiplayer button participates in that same frame's navigation.
      mgr.update(0.0f);
      cur_disabled = compute_disabled(mgr, shown);
      std::fprintf(stderr, "[menu-input] connected gamepads=%d\n",
                   connected_gamepads);
    }

    auto now = clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;
    if (dt > 0.1f) dt = 0.1f;
    if (fixed_dt > 0.0f && std::isfinite(fixed_dt)) dt = fixed_dt;

    phase_seconds += dt;
    update_meta_music(dt);

    if (phase == RuntimePhase::BootLogos) {
      const bool skip = win->action_pressed(Action::Confirm) ||
                        win->action_pressed(Action::Start) ||
                        win->action_pressed(Action::Back);
      constexpr float kFade = 0.45f;
      constexpr float kHold = 1.20f;
      constexpr float kSlide = kFade + kHold + kFade;
      const int slide = static_cast<int>(phase_seconds / kSlide);
      if (skip || boot_slides.empty() ||
          slide >= static_cast<int>(boot_slides.size())) {
        if (skip) std::fprintf(stderr, "[boot] logo sequence skipped\n");
        start_intro_video();
      }
      if (phase == RuntimePhase::BootLogos) {
        const float local = phase_seconds - static_cast<float>(slide) * kSlide;
        float brightness = 1.0f;
        if (local < kFade)
          brightness = local / kFade;
        else if (local > kFade + kHold)
          brightness = (kSlide - local) / kFade;
        const asset::Image& image = boot_slides[static_cast<std::size_t>(slide)];
        win->clear(0.0f, 0.0f, 0.0f);
        win->blit_fullscreen_rgba(image.rgba.data(), image.width, image.height,
                                  std::clamp(brightness, 0.0f, 1.0f));
        if (!screenshot_path.empty() &&
            frame == static_cast<uint64_t>(screenshot_frame))
          win->save_screenshot(screenshot_path.c_str());
        win->present();
        ++frame;
        if (max_frames > 0 && frame >= static_cast<uint64_t>(max_frames)) break;
        continue;
      }
    }

    if (phase == RuntimePhase::IntroVideo) {
      const bool skip = win->action_pressed(Action::Confirm) ||
                        win->action_pressed(Action::Start) ||
                        win->action_pressed(Action::Back);
      if (skip) {
        std::fprintf(stderr, "[boot-video] intro skipped\n");
        route_after_intro();
      } else if (intro_video_started && intro_video.read_next_frame()) {
        win->clear(0.0f, 0.0f, 0.0f);
        win->blit_fullscreen_rgba(intro_video.rgba().data(),
                                  intro_video.width(), intro_video.height());
        if (!screenshot_path.empty() &&
            frame == static_cast<uint64_t>(screenshot_frame))
          win->save_screenshot(screenshot_path.c_str());
        win->present();
        ++frame;
        if (max_frames > 0 && frame >= static_cast<uint64_t>(max_frames)) break;
        continue;
      } else if (phase == RuntimePhase::IntroVideo) {
        route_after_intro();
      }
    }

    if (phase == RuntimePhase::Gameplay) {
      const uint32_t guitar_input =
          win->guitar_input_held() |
          (win->guitar_input_edge() & ((1u << 5) | (1u << 6)));
      gameplay.tick(dt, guitar_input, win->guitar_whammy_axis());
      if (gameplay.failed()) {
        gameplay.stop_audio();
        commit_gameplay_results();
        gameplay_loaded = false;
        mgr.goto_screen(Symbol("lose_screen"));
        phase = RuntimePhase::Menus;
        phase_seconds = 0.0f;
        screen_seconds = 0.0f;
        std::fprintf(stderr, "[flow] song failed -> lose_screen\n");
      } else if (gameplay.is_finished()) {
        gameplay.stop_audio();
        commit_gameplay_results();
        phase = RuntimePhase::YouRock;
        phase_seconds = 0.0f;
        std::fprintf(stderr, "[flow] song complete -> YOU ROCK\n");
      }
      draw_gameplay(phase == RuntimePhase::YouRock);
      if (!screenshot_path.empty() &&
          frame == static_cast<uint64_t>(screenshot_frame))
        win->save_screenshot(screenshot_path.c_str());
      win->present();
      ++frame;
      if (max_frames > 0 && frame >= static_cast<uint64_t>(max_frames)) break;
      continue;
    }

    if (phase == RuntimePhase::YouRock) {
      const bool continue_pressed =
          phase_seconds >= 0.75f &&
          (win->action_pressed(Action::Confirm) ||
           win->action_pressed(Action::Start));
      draw_gameplay(true);
      if (!screenshot_path.empty() &&
          frame == static_cast<uint64_t>(screenshot_frame))
        win->save_screenshot(screenshot_path.c_str());
      win->present();
      if (continue_pressed || phase_seconds >= 4.0f) {
        gameplay_loaded = false;
        mgr.goto_screen(Symbol("post_show_screen"));
        phase = RuntimePhase::Menus;
        phase_seconds = 0.0f;
        screen_seconds = 0.0f;
        automated_screen.clear();
        std::fprintf(stderr, "[flow] YOU ROCK -> stock post-show results\n");
      }
      ++frame;
      if (max_frames > 0 && frame >= static_cast<uint64_t>(max_frames)) break;
      continue;
    }

    screen_seconds += dt;

    // Loading completion is an external engine event in the stock scripts.
    // Load the selected song while their loading screen is visible, then send
    // exactly that event to enter game_screen.
    Object* live_screen = mgr.current_screen();
    if (live_screen && live_screen->name() == Symbol("loading_screen") &&
        !gameplay_loaded && screen_seconds >= 0.15f) {
      if (prepare_gameplay()) {
        live_screen->handle_property(Symbol("TRANSITION_COMPLETE_MSG"),
                                     DataArray());
        phase = RuntimePhase::Gameplay;
        phase_seconds = 0.0f;
      } else {
        mgr.goto_screen(Symbol("qp_selsong_screen"));
        screen_seconds = 0.0f;
      }
    }

    // Input (live controller/keyboard) -> focus nav + the real menu scripts.
    bool visual_dirty = false;
    if (!disable_live_input && win->action_pressed(Action::Down))
      focus_move(mgr, cur_labels, cur_disabled, +1, db.song_count(),
                 credit_count);
    if (!disable_live_input && win->action_pressed(Action::Up))
      focus_move(mgr, cur_labels, cur_disabled, -1, db.song_count(),
                 credit_count);
    if (!disable_live_input && win->action_pressed(Action::Confirm)) {
      do_confirm(mgr);
      visual_dirty = true;
    }
    if (!disable_live_input && win->action_pressed(Action::Back)) {
      do_back(mgr);
      visual_dirty = true;
    }

    // Scripted auto-nav (headless testing): one action per kNavStep frames.
    if (nav_i < nav.size() && frame == (nav_i + 1) * kNavStep) {
      const std::string& a = nav[nav_i++];
      if (a == "down")
        focus_move(mgr, cur_labels, cur_disabled, +1, db.song_count(),
                   credit_count);
      else if (a == "up")
        focus_move(mgr, cur_labels, cur_disabled, -1, db.song_count(),
                   credit_count);
      else if (a == "confirm") {
        do_confirm(mgr);
        visual_dirty = true;
      } else if (a == "confirm2") {
        do_confirm(mgr, 1);
        visual_dirty = true;
      } else if (a == "up1" || a == "down1" || a == "up2" ||
                 a == "down2" || a.rfind("button1:", 0) == 0 ||
                 a.rfind("button2:", 0) == 0) {
        Object* screen = mgr.current_screen();
        const Symbol panel_name =
            screen ? screen->get_property(Symbol("focus"))
                         .as_symbol()
                         .value_or(Symbol())
                   : Symbol();
        Object* panel =
            panel_name.valid() ? mgr.find_object(panel_name) : nullptr;
        const bool raw_button = a.rfind("button", 0) == 0;
        const bool second_player = raw_button ? a[6] == '2' : a.back() == '2';
        const bool up = a.rfind("up", 0) == 0;
        const Symbol button =
            raw_button
                ? Symbol(a.substr(8).c_str())
                : Symbol(up ? "kPad_DU" : "kPad_DD");
        fire_button_down(mgr, screen, panel,
                         button, second_player ? 1 : 0);
        visual_dirty = true;
      } else if (a == "back") {
        do_back(mgr);
        visual_dirty = true;
      }
      else if (a.rfind("text:", 0) == 0) {
        Object* screen = mgr.current_screen();
        const Symbol panel_name =
            screen ? screen->get_property(Symbol("focus"))
                         .as_symbol()
                         .value_or(Symbol())
                   : Symbol();
        Object* panel =
            panel_name.valid() ? mgr.find_object(panel_name) : nullptr;
        const Symbol component_name =
            panel ? panel->get_property(Symbol("focus"))
                        .as_symbol()
                        .value_or(Symbol())
                  : Symbol();
        Object* component = component_name.valid()
                                ? mgr.resolve_object(component_name)
                                : nullptr;
        if (component && component->class_name() == Symbol("BandTextEntry")) {
          component->handle_property(
              Symbol("set_text"),
              one_arg(DataNode::Str(a.substr(std::strlen("text:")))));
          component->handle_property(Symbol("send_select"), DataArray());
          visual_dirty = true;
        } else {
          std::fprintf(stderr,
                       "[menu-nav] text input ignored: focused component is "
                       "not BandTextEntry\n");
        }
      }
      else if (a.rfind("focus:", 0) == 0) {
        Object* s = mgr.current_screen();
        Symbol fpn = s ? s->get_property(Symbol("focus")).as_symbol().value_or(Symbol()) : Symbol();
        if (Object* p = fpn.valid() ? mgr.find_object(fpn) : nullptr)
          set_panel_focus(mgr, p, a.substr(6));
      }
    }

    if (automate_current_screen()) visual_dirty = true;

    mgr.update(dt);

    // Reload the scene + text when the screen changed; re-render text (re-colour)
    // when only the focus moved.
    if (mgr.current_screen() != shown) {
      const auto transition = mgr.transition_snapshot();
      outgoing_transition_visible =
          transition.active && transition.duration > 0.0001f &&
          transition.exiting_screen == shown;
      if (outgoing_transition_visible) {
        rebuild_scene(hdr, ark, mgr, shown, outgoing_renderer);
        outgoing_guitar_visible = rebuild_guitar_display_scene(
            hdr, ark, mgr, shown, db, outgoing_guitar_renderer);
        outgoing_character_previews = std::move(character_previews);
        apply_loading_source_anims(hdr, ark, mgr, shown, outgoing_renderer);
        rebuild_text(hdr, ark, mgr, shown, outgoing_renderer, fonts, db,
                     locale);
        outgoing_menu_animation_sources =
            collect_live_menu_animation_sources(hdr, ark, mgr, shown);
      } else {
        outgoing_guitar_visible = false;
        outgoing_character_previews.clear();
        outgoing_menu_animation_sources.clear();
      }
      shown = mgr.current_screen();
      screen_seconds = 0.0f;
      automated_screen.clear();
      cur_labels = gather_labels(hdr, ark, mgr, shown);
      cur_disabled = compute_disabled(mgr);
      rebuild_scene(hdr, ark, mgr, shown, renderer);
      guitar_visible =
      rebuild_guitar_display_scene(hdr, ark, mgr, shown, db, guitar_renderer);
      character_previews =
          rebuild_character_display_scenes(hdr, ark, mgr, shown, *win);
      if (!character_previews.empty()) mgr.update(0.0f);
      apply_loading_source_anims(hdr, ark, mgr, shown, renderer);
      rebuild_text(hdr, ark, mgr, shown, renderer, fonts, db, locale);
      live_menu_animation_sources =
          collect_live_menu_animation_sources(hdr, ark, mgr, shown);
      last_focus = focus_name();
    } else if (visual_dirty) {
      cur_labels = gather_labels(hdr, ark, mgr, shown);
      cur_disabled = compute_disabled(mgr);
      rebuild_scene(hdr, ark, mgr, shown, renderer);
      guitar_visible =
          rebuild_guitar_display_scene(hdr, ark, mgr, shown, db, guitar_renderer);
      character_previews =
          rebuild_character_display_scenes(hdr, ark, mgr, shown, *win,
                                           std::move(character_previews));
      if (!character_previews.empty()) mgr.update(0.0f);
      apply_loading_source_anims(hdr, ark, mgr, shown, renderer);
      rebuild_text(hdr, ark, mgr, shown, renderer, fonts, db, locale);
      live_menu_animation_sources =
          collect_live_menu_animation_sources(hdr, ark, mgr, shown);
      last_focus = focus_name();
    } else if (focus_name() != last_focus) {
      last_focus = focus_name();
      rebuild_text(hdr, ark, mgr, shown, renderer, fonts, db, locale);
    }

    if (outgoing_transition_visible &&
        !mgr.transition_snapshot().active) {
      outgoing_transition_visible = false;
      outgoing_guitar_visible = false;
      outgoing_character_previews.clear();
      outgoing_menu_animation_sources.clear();
    }

    apply_loading_material_source_anim(mgr, shown, loading_word_material_anim,
                                       renderer);
    apply_live_menu_animation_frames(live_menu_animation_sources, renderer);
    if (outgoing_transition_visible)
      apply_live_menu_animation_frames(outgoing_menu_animation_sources,
                                       outgoing_renderer);
    renderer.update(dt);
    guitar_renderer.update(dt);
    if (outgoing_transition_visible) {
      outgoing_renderer.update(dt);
      outgoing_guitar_renderer.update(dt);
    }
    for (auto& character : character_previews)
    {
      character.renderer->update(dt);
      character.clip_player.advance(dt);
      if (character.enter_pending_loop && character.ui_enter_clip &&
          character.clip_player.current_time_seconds() >=
              character.ui_enter_clip->duration_seconds()) {
        character.clip_player.play(
            *character.ui_clip,
            ghogx::character::kCharPlayLoop);
        character.enter_pending_loop = false;
      }
      ghogx::character::ClipChannelLayerStack pose_stack;
      pose_stack.debug_label = "menu_ui_loop";
      ghogx::character::CharacterPosePlayerLayerBuildSources player_inputs;
      player_inputs.main = &character.clip_player;
      const auto player_layers =
          ghogx::character::make_character_pose_player_layer_sources(
              player_inputs);
      ghogx::character::append_character_pose_player_layers(pose_stack,
                                                             player_layers);
      ghogx::character::CharacterPoseControllerFrameSources controller_sources;
      controller_sources.pose_stack = &pose_stack;
      controller_sources.time_seconds =
          character.clip_player.current_time_seconds();
      controller_sources.controllers_enabled = true;
      ghogx::character::apply_character_pose_controller_frame(
          character.renderer->character(), controller_sources);
    }
    if (outgoing_transition_visible) {
      draw_menu_layers(outgoing_renderer, outgoing_guitar_renderer,
                       outgoing_guitar_visible, outgoing_character_previews,
                       /*clear_target=*/true);
      draw_menu_layers(renderer, guitar_renderer, guitar_visible,
                       character_previews,
                       /*clear_target=*/false);
    } else {
      draw_menu_layers(renderer, guitar_renderer, guitar_visible,
                       character_previews,
                       /*clear_target=*/true);
    }

    if (!screenshot_path.empty() && frame == static_cast<uint64_t>(screenshot_frame))
      win->save_screenshot(screenshot_path.c_str());
    win->present();

    ++frame;
    if (max_frames > 0 && frame >= static_cast<uint64_t>(max_frames)) break;
  }
  return 0;
}

}  // namespace ghogx::ui
