// engine/src/ui/menu_app.cpp -- see menu_app.h.

#include "ui/menu_app.h"

#include "ui/config_db.h"
#include "ui/menu_font.h"
#include "ui/menu_labels.h"
#include "ui/meta_objects.h"
#include "ui/screen_loader.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include "asset/milo_image.h"
#include "milo_scene/milo_scene.h"
#include "render/milo_scene_renderer.h"
#include "render/window_d3d9.h"

#include "dtb.h"
#include "dtb_bridge/dtb_bridge.h"
#include "core/data_node.h"
#include "core/symbol.h"

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
  renderer.set_scene(std::move(combined), textures);

  // Use the menu's REAL camera (meta.cam in ui/gen/metacam.milo_ps2), now that
  // decode_cam reads it correctly: eye (0,-768,0) along -Y, looking +Y at the X-Z
  // menu plane, fov ~0.602. This is GH2's exact framing -- the poster fills the
  // screen -- grounded in the decoded camera, no multipliers.
  ghogx::render::OrbitCamera& cam = renderer.camera();
  cam.yaw = 0.0f;
  cam.pitch = 0.0f;
  cam.target[0] = 0.0f; cam.target[1] = 0.0f; cam.target[2] = 0.0f;
  cam.distance = 768.0f;
  cam.fov = 0.602f;
  cam.near_z = 1.0f;
  cam.far_z = 5000.0f;
  milo_scene::Scene cam_scene;
  if (milo_scene::load_scene(hdr, ark, "ui/gen/metacam.milo_ps2", cam_scene)) {
    for (const auto& c : cam_scene.cams) {
      if (!c.decoded || std::strcmp(c.name.c_str(), "meta.cam") != 0) continue;
      cam.target[0] = c.local.pos[0];           // look straight ahead (+Y) at the plane
      cam.target[2] = c.local.pos[2];
      cam.distance = std::max(1.0f, std::fabs(c.local.pos[1]));
      if (c.fov > 0.05f) cam.fov = c.fov;
      std::fprintf(stderr, "[menu] meta.cam eye=(%.1f %.1f %.1f) fov=%.3f\n",
                   c.local.pos[0], c.local.pos[1], c.local.pos[2], cam.fov);
      break;
    }
  }
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

// GH2 menu item colours, read 1:1 from the PanelDir "GH2" type in ui_objects.dtb
// ((normal_color {pack_color 1 0 0}) etc.). Normal items are RED; the focused
// item renders WHITE (selecting_color (1,1,1); retail does not use the green
// focus_color for the resting highlight — verified against a real frame).
constexpr uint32_t kColNormal  = 0xFFFF0000u;  // normal_color   (1,0,0) red
constexpr uint32_t kColFocused = 0xFFFFFFFFu;  // selecting_color (1,1,1) white
constexpr float kFocusScale    = 1.05f;        // PanelDir (focus_scale 1.05)
// Live-XEX capture (trace-360 BandButton_ColorResolve struct dump) showed the
// RENDERED button transform — not the static .btn bind pose: near-uniform scale,
// X ~3.5–4.4 consistent (aligned left edges), tilt ~2°, runtime line pitch ~23
// world units. Cap fits the pitch with a gap → ~17 world units (kTextScale 0.5).
// (Exact glyph size awaits the child-RndText world-xfm capture; the box-vs-native
// font-unit ambiguity is ±, so 0.5 is the best grounded value for now.)
constexpr float kTextScale = 0.50f;
// Common left edge for the menu column (the runtime X the buttons align to;
// the static per-.btn X is the pre-TransAnim bind pose and is NOT used).
constexpr float kMenuLeftX = 3.9f;

void append_text_quads(const std::vector<MenuLabel>& labels, const MenuFont& font,
                       const std::map<std::string, std::string>& locale,
                       const std::string& focused,
                       std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
  using TV = ghogx::render::MiloSceneRenderer::TextVertex;
  for (const auto& lbl : labels) {
    if (!lbl.has_world || lbl.text.empty()) continue;
    // For now only the BandButtons (impact font). The Text objects (SONG/VENUE/
    // DIFFICULTY) and the cutout BandLabel carry a parent-group offset not yet
    // composed, so their world translation alone lands them on the button column
    // — defer until that composition is RE'd.
    if (lbl.type != "BandButton") continue;
    std::string disp = lbl.text;
    auto it = locale.find(lbl.text);
    if (it != locale.end()) disp = it->second;

    float w = 0.0f;
    auto quads = font.layout(disp, &w);
    if (quads.empty())
      std::fprintf(stderr, "[menu]   WARN label '%s' key='%s' disp='%s' -> no glyphs\n",
                   lbl.name.c_str(), lbl.text.c_str(), disp.c_str());
    const bool foc = (lbl.name == focused);
    // X from the runtime aligned left edge (not the static bind-pose X, which
    // varies); Y/Z from the object's translation (vertical column position).
    const float ax = kMenuLeftX, ay = lbl.world[10], az = lbl.world[11];
    const float capH = font.cap_height();
    const uint32_t argb = foc ? kColFocused : kColNormal;
    const float scl = kTextScale * (foc ? kFocusScale : 1.0f);

    // Use the object's REAL world-Trans orientation, not an axis-aligned billboard:
    // the local-X / local-Z axes (rows 0 and 2 of the world matrix, X-Z plane),
    // normalized so only the ~1° rotation (the menu's slight rightward tilt) is
    // applied — the matrix's non-uniform scale is the button BOX, not the glyph
    // size, so the glyphs keep their uniform text size (kTextScale).
    float r0x = lbl.world[0], r0z = lbl.world[2];   // local +X axis in world (X,Z)
    float r2x = lbl.world[6], r2z = lbl.world[8];   // local +Z axis in world (X,Z)
    float n0 = std::sqrt(r0x * r0x + r0z * r0z);
    float n2 = std::sqrt(r2x * r2x + r2z * r2z);
    if (n0 > 1e-6f) { r0x /= n0; r0z /= n0; }
    if (n2 > 1e-6f) { r2x /= n2; r2z /= n2; }

    // The menu items are LEFT-aligned: the text's left edge sits at the box origin
    // (world translation); +font-x runs along the local +X axis, +font-y (down)
    // along local -Z. The whole line is tilted by the matrix's rotation.
    auto V = [&](float qx, float qy, float u, float v) {
      const float lx = qx * scl;                     // left-aligned, along local +X
      const float lz = -(qy - capH * 0.5f) * scl;    // font-y down -> local +Z up
      TV tv;
      tv.x = ax + lx * r0x + lz * r2x;
      tv.z = az + lx * r0z + lz * r2z;
      tv.y = ay;
      tv.u = u; tv.v = v; tv.argb = argb;
      return tv;
    };
    for (const auto& q : quads) {
      TV a = V(q.x0, q.y0, q.u0, q.v0);
      TV b = V(q.x1, q.y0, q.u1, q.v0);
      TV c = V(q.x1, q.y1, q.u1, q.v1);
      TV d = V(q.x0, q.y1, q.u0, q.v1);
      out.push_back(a); out.push_back(b); out.push_back(c);
      out.push_back(a); out.push_back(c); out.push_back(d);
    }
  }
}

// Rebuild the renderer's text overlay from the current screen's panels.
void rebuild_text(const std::string& hdr, const std::string& ark, ScreenManager& mgr,
                  Object* screen, ghogx::render::MiloSceneRenderer& renderer,
                  const MenuFont& font,
                  const std::map<std::string, std::string>& locale) {
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
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> verts;
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel) continue;
    DataNode f = panel->get_property(Symbol("file"));
    auto sym = f.as_symbol();
    if (!sym) continue;
    auto labels = extract_menu_labels(hdr, ark, "ui/gen/" + std::string(sym->c_str()) + "_ps2");
    append_text_quads(labels, font, locale, focused, verts);
  }
  std::fprintf(stderr, "[menu] focused component = '%s'\n", focused.c_str());
  std::fprintf(stderr, "[menu] text: %zu glyph-verts\n", verts.size());
  renderer.set_text(std::move(verts), font.atlas());
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

  // The menu bitmap font ("impact") + the locale (button labels are loc keys).
  MenuFont impact_font;
  impact_font.load(hdr, ark, "ui/gen/impact.milo_ps2");
  std::map<std::string, std::string> locale = load_locale(arkr, arks);

  // Boot to the main menu (the bootup_load memcard chain is wired later).
  mgr.goto_screen(Symbol("main_screen"));

  // 2. Window + scene renderer.
  auto win = ghogx::render::Window::create(1280, 720, "GuitarHeroOGX — menu");
  if (!win) { std::fprintf(stderr, "[menu] window/device create failed\n"); return 1; }
  ghogx::render::MiloSceneRenderer renderer(*win);

  Object* shown = mgr.current_screen();
  rebuild_scene(hdr, ark, mgr, shown, renderer);
  rebuild_text(hdr, ark, mgr, shown, renderer, impact_font, locale);

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
      rebuild_text(hdr, ark, mgr, shown, renderer, impact_font, locale);
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
