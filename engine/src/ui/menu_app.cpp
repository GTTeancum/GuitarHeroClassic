// engine/src/ui/menu_app.cpp -- see menu_app.h.

#include "ui/menu_app.h"

#include "ui/config_db.h"
#include "ui/meta_objects.h"
#include "ui/screen_loader.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include "asset/milo_image.h"
#include "milo_scene/milo_scene.h"
#include "render/milo_scene_renderer.h"
#include "render/window_d3d9.h"

#include "ark_v3.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

namespace ghogx::ui {

namespace {

using Action = ghogx::render::Window::Action;

// Append a panel's MILO (its (file) value, e.g. "main.milo" -> ui/gen/main.milo_ps2)
// into the combined scene + texture set the renderer draws.
void add_panel_milo(const std::string& hdr, const std::string& ark,
                    const std::string& file, milo_scene::Scene& combined,
                    std::map<std::string, asset::Image>& textures) {
  if (file.empty()) return;
  const std::string path = "ui/gen/" + file + "_ps2";  // "main.milo" -> ".milo_ps2"
  milo_scene::Scene s;
  if (!milo_scene::load_scene(hdr, ark, path, s)) return;

  // Collect the diffuse-texture names BEFORE moving the mats out (otherwise the
  // moved-from strings are empty and nothing loads).
  std::unordered_set<std::string> want;
  for (const auto& m : s.mats)
    if (!m.diffuse_tex.empty()) want.insert(m.diffuse_tex);

  for (auto& m : s.meshes) {
    if (m.name.rfind("light", 0) == 0) continue;  // DIAGNOSTIC: skip the glow overlay
    combined.meshes.push_back(std::move(m));
  }
  for (auto& mt : s.mats) combined.mats.push_back(std::move(mt));
  for (auto& tr : s.transes) combined.transes.push_back(std::move(tr));
  for (auto& c : s.cams) combined.cams.push_back(std::move(c));

  std::vector<std::string> names(want.begin(), want.end());
  auto imgs = asset::load_milo_textures(hdr, ark, path, names);
  for (auto& kv : imgs) textures.emplace(kv.first, std::move(kv.second));
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

// Build the renderer's scene from the current screen's panels' MILOs.
void rebuild_scene(const std::string& hdr, const std::string& ark, ScreenManager& mgr,
                   Object* screen, ghogx::render::MiloSceneRenderer& renderer) {
  milo_scene::Scene combined;
  std::map<std::string, asset::Image> textures;
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel) continue;
    DataNode f = panel->get_property(Symbol("file"));
    if (auto sym = f.as_symbol()) add_panel_milo(hdr, ark, std::string(sym->c_str()), combined, textures);
  }
  std::fprintf(stderr, "[menu] %s: %zu meshes, %zu textures\n",
               screen ? screen->name().c_str() : "?", combined.meshes.size(), textures.size());
  renderer.set_scene(std::move(combined), textures);  // auto-frames target = content center

  // GH2 menu panels are a thin slab in the X-Z plane (Y ~ 0; extent X[-1000,1000]
  // Z[-785,655], Y[-2,2]) -- a 2-D layout like the HUD. View it FACE-ON down the Y
  // (depth) axis: yaw=0/pitch=0 places the eye along -Y looking +Y at the X-Z face.
  // (decode_cam's meta.cam fields read garbage -- fov 1060, eye in-plane -- so we
  // frame from the decoded geometry extent, not the broken camera.) Pull the eye
  // back so the panel fits the vertical fov.
  ghogx::render::OrbitCamera& cam = renderer.camera();
  cam.yaw = 0.0f;
  cam.pitch = 0.0f;
  cam.distance *= 1.5f;
  cam.near_z = std::max(cam.distance * 0.01f, 0.5f);
}

// Fire the focused component's SELECT_START_MSG (Confirm). The screen's (focus)
// names the active panel; that panel's (focus) names the active component.
void do_confirm(ScreenManager& mgr) {
  Object* screen = mgr.current_screen();
  if (!screen) return;
  Symbol panel_name = screen->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
  Object* panel = panel_name.valid() ? mgr.find_object(panel_name) : nullptr;
  if (!panel) return;
  Symbol comp = panel->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
  mgr.set_global(Symbol("component"), DataNode::Sym(comp));
  panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
}

}  // namespace

int run_menu_mode(const std::string& hdr, const std::string& ark,
                  const std::string& screenshot_path, int screenshot_frame,
                  int max_frames) {
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
  std::fprintf(stderr, "[menu] booted: %d DTBs, %zu objects, %zu songs\n", n,
               mgr.registry().size(), db.song_count());

  // Boot to the main menu (the bootup_load memcard chain is wired later).
  mgr.goto_screen(Symbol("main_screen"));

  // 2. Window + scene renderer.
  auto win = ghogx::render::Window::create(1280, 720, "GuitarHeroOGX — menu");
  if (!win) { std::fprintf(stderr, "[menu] window/device create failed\n"); return 1; }
  ghogx::render::MiloSceneRenderer renderer(*win);

  Object* shown = mgr.current_screen();
  rebuild_scene(hdr, ark, mgr, shown, renderer);

  using clock = std::chrono::steady_clock;
  auto last = clock::now();
  uint64_t frame = 0;
  if (!screenshot_path.empty() && max_frames == 0) max_frames = screenshot_frame + 3;

  while (!win->should_close()) {
    win->pump();
    if (win->should_close()) break;

    auto now = clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;
    if (dt > 0.1f) dt = 0.1f;

    // Input -> the real menu scripts.
    if (win->action_pressed(Action::Confirm)) do_confirm(mgr);
    if (win->action_pressed(Action::Back)) mgr.pop_screen();

    mgr.update(dt);

    // Reload the rendered scene when the screen changed.
    if (mgr.current_screen() != shown) {
      shown = mgr.current_screen();
      rebuild_scene(hdr, ark, mgr, shown, renderer);
    }

    renderer.draw();

    if (!screenshot_path.empty() && frame == static_cast<uint64_t>(screenshot_frame))
      win->save_screenshot(screenshot_path.c_str());
    win->present();

    ++frame;
    if (max_frames > 0 && frame >= static_cast<uint64_t>(max_frames)) break;
  }
  return 0;
}

}  // namespace ghogx::ui
