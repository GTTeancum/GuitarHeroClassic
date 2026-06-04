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
#include <cstdlib>
#include <functional>
#include <cstring>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

namespace ghogx::ui {

namespace {

using Action = ghogx::render::Window::Action;

std::unordered_set<std::string> compute_disabled(ScreenManager& mgr);  // fwd

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
  // A disabled component ignores SELECT (the original's disabled BandButton does
  // not fire its handler) — e.g. multiplayer when is_missing_multi_controller.
  if (comp.valid() && compute_disabled(mgr).count(comp.c_str())) return;
  mgr.set_global(Symbol("component"), DataNode::Sym(comp));
  panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
}

// Back (B/circle): go to the screen's (back_screen) if set (screens that navigate
// forward set it on their target, e.g. main.dta `{nameprof_screen set back_screen
// main_screen}`), else fall back to the main menu. (goto_screen-based nav has no
// push stack, so pop_screen doesn't apply.)
void do_back(ScreenManager& mgr) {
  Object* s = mgr.current_screen();
  if (!s) return;
  Symbol back = s->get_property(Symbol("back_screen")).as_symbol().value_or(Symbol());
  if (back.valid()) { mgr.goto_screen(back); return; }
  if (s->name() != Symbol("main_screen")) mgr.goto_screen(Symbol("main_screen"));
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

// GH2 per-state menu-text colours — byte-exact from common.milo_ps2, NOT
// interpretation. UIButton/UILabel map each UI state to a per-state .font
// (ui_objects_ps2.dta lines 45-56: normal/focused/disabled/selecting.font). In
// common.milo those four Font objects share ONE glyph atlas (each Font body
// references "normal.font" glyph data) and differ ONLY by the Mat they bind:
// normal.font->normal.mat, focused.font->focused.mat, etc. So the per-state
// colour IS that Mat's diffuse RGBA (the 4 floats after the Mat's flag byte):
//   normal.mat    (1, 1, 1, 1)        -> white
//   focused.mat   (1, 1, 0, 1)        -> yellow
//   selecting.mat (1, 0, 0, 1)        -> red   (transient: a button being pressed)
//   disabled.mat  (0.4, 0.4, 0.4, 1)  -> grey
// (The earlier red-normal/white-focused scheme was the Deluxe MOD's GHPanel
// recolour, not stock GH2. Stock = white normal, yellow focus.)
constexpr uint32_t kColNormal    = 0xFFFFFFFFu;  // normal.mat    (1,1,1)
constexpr uint32_t kColFocused   = 0xFFFFFF00u;  // focused.mat   (1,1,0) yellow
[[maybe_unused]] constexpr uint32_t kColSelecting = 0xFFFF0000u;  // selecting.mat (1,0,0) red
                                                 // (wired when the press/confirm flash anim lands)
constexpr uint32_t kColDisabled  = 0xFF666666u;  // disabled.mat  (0.4,0.4,0.4)
constexpr float kFocusScale      = 1.05f;        // ui_objects_ps2.dta:10 (focus_scale 1.05)
// Glyph size = the RndText text_size, GROUNDED 0.5 from BOTH sides:
//   - static: all five main-menu buttons carry the shared run [15.0, 1.0, 0.5, 1.0,
//     -0.05, 30.0, 280.0] after their locale token in main.milo (-0.05 = italic
//     slant, 280 = box width, 15 = box height).
//   - dynamic: the live XEX BandButton struct (trace-360 hook on sub_82122920)
//     carries text_size at word 74 = 0.5000, byte-identical on every button dumped.
// The earlier "field-identity unconfirmed" caveat is discharged: word 74 IS the
// resolved text_size and equals 0.5. Applied as cap-units->world it renders to the
// grounded ~30.4 row pitch (button Trans Z spacing). The runtime world matrix also
// carries a ~1.05 uniform button scale + 2.00deg tilt (tagged 3x4 in the struct);
// that refinement is logged in FIDELITY (the captured buttons' screen identity is
// ambiguous, so the global kTextScale stays the confirmed text_size 0.5).
constexpr float kTextScale = 0.50f;
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
constexpr float kMenuZScale  = 0.875f;
constexpr float kMenuZOffset = 4.0f;

void append_text_quads(const std::vector<MenuLabel>& labels, const MenuFont& font,
                       const std::map<std::string, std::string>& locale,
                       const std::string& focused,
                       const std::unordered_set<std::string>& disabled,
                       std::vector<ghogx::render::MiloSceneRenderer::TextVertex>& out) {
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
    if (!lbl.has_world || lbl.text.empty()) continue;
    const bool isBtn = (lbl.type == "BandButton");
    if (!isBtn && lbl.type != "Text" && lbl.type != "BandLabel") continue;

    // Colour: buttons by state (white normal / yellow focused / grey disabled,
    // from common.milo per-state mats); Text & BandLabel are white.
    bool foc = false;
    uint32_t col = 0xFFFFFFFFu;
    if (isBtn) {
      foc = (lbl.name == focused);
      col = disabled.count(lbl.name) ? kColDisabled : (foc ? kColFocused : kColNormal);
    }
    std::string disp = lbl.text;
    if (auto it = locale.find(lbl.text); it != locale.end()) disp = it->second;
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
      // X: each button keeps its own per-.btn X (the poster design staggers them;
      // the XEX confirms the runtime is NOT left-aligned — settled X varies 4.5..-1.9,
      // within ~0.8 of the byte-exact bind world[9], far closer than a fixed column).
      const float ax = lbl.world[9];
      const float ay = lbl.world[10];
      // Main-menu bind-pose buttons: remap the bind-pose Z to the XEX-measured
      // runtime Z (affine, fits all 5 buttons exactly). Other screens use their Z.
      const float az = bindPose ? (kMenuZScale * lbl.world[11] + kMenuZOffset) : lbl.world[11];
      const float scl = kTextScale * (foc ? kFocusScale : 1.0f);
      emit(quads, [&](float qx, float qy, float u, float v) {
        const float lx = qx * scl, lz = -(qy - capH * 0.5f) * scl;
        TV tv{ax + lx * r0x + lz * r2x, ay, az + lx * r0z + lz * r2z, u, v, col};
        return tv;
      });
    } else {
      // Text / BandLabel: their in-MILO Trans IS the render transform (not anim-
      // driven). Full matrix, em-normalised layout, centred on the translation.
      const float Tx = lbl.world[9], Ty = lbl.world[10], Tz = lbl.world[11];
      const float m0 = lbl.world[0], m2 = lbl.world[2], m6 = lbl.world[6], m8 = lbl.world[8];
      const float halfW = (w / capH) * 0.5f;
      emit(quads, [&](float qx, float qy, float u, float v) {
        const float ex = qx / capH - halfW, ez = -((qy - capH * 0.5f) / capH);
        TV tv{Tx + m0 * ex + m2 * ez, Ty, Tz + m6 * ex + m8 * ez, u, v, col};
        return tv;
      });
    }
  }
}

// Items the original would disable: the main panel poll disables multiplayer when
// `game is_missing_multi_controller` (main.dta). We evaluate that game condition.
std::unordered_set<std::string> compute_disabled(ScreenManager& mgr) {
  std::unordered_set<std::string> d;
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
    if (!panel) continue;
    auto sym = panel->get_property(Symbol("file")).as_symbol();
    if (!sym) continue;
    auto labels = extract_menu_labels(hdr, ark, "ui/gen/" + std::string(sym->c_str()) + "_ps2");
    for (auto& l : labels) out.push_back(std::move(l));
  }
  return out;
}

// Move focus down (dir>0) / up (dir<0) along the BandButton nav links, skipping
// disabled items. Sets the focused panel's (focus) property to the new component.
void focus_move(ScreenManager& mgr, const std::vector<MenuLabel>& labels,
                const std::unordered_set<std::string>& disabled, int dir) {
  Object* screen = mgr.current_screen();
  if (!screen) return;
  Symbol fpn = screen->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
  Object* panel = fpn.valid() ? mgr.find_object(fpn) : nullptr;
  if (!panel) return;
  std::string cur = panel->get_property(Symbol("focus")).as_symbol().value_or(Symbol()).c_str();
  for (size_t guard = 0; guard <= labels.size(); ++guard) {
    std::string next;
    if (dir > 0) {
      for (const auto& l : labels) if (l.name == cur) { next = l.nav; break; }
    } else {
      for (const auto& l : labels) if (!l.nav.empty() && l.nav == cur) { next = l.name; break; }
    }
    if (next.empty()) return;
    if (!disabled.count(next)) {
      panel->set_property(Symbol("focus"), DataNode::Sym(Symbol(next.c_str())));
      return;
    }
    cur = next;  // disabled -> keep moving in the same direction
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
  std::unordered_set<std::string> disabled = compute_disabled(mgr);
  std::vector<ghogx::render::MiloSceneRenderer::TextVertex> verts;
  for (Symbol pn : screen_panel_names(screen)) {
    Object* panel = mgr.find_object(pn);
    if (!panel) continue;
    DataNode f = panel->get_property(Symbol("file"));
    auto sym = f.as_symbol();
    if (!sym) continue;
    auto labels = extract_menu_labels(hdr, ark, "ui/gen/" + std::string(sym->c_str()) + "_ps2");
    append_text_quads(labels, font, locale, focused, disabled, verts);
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
  // Run one poll tick before the first text build so panel `poll` handlers have
  // set their state (e.g. multiplayer disabled via is_missing_multi_controller).
  mgr.update(0.0f);
  rebuild_scene(hdr, ark, mgr, shown, renderer);
  rebuild_text(hdr, ark, mgr, shown, renderer, impact_font, locale);

  // Audit mode (GHOGX_MENU_DUMP=1): goto every *_screen object, rebuild its scene
  // + text, and report the mesh/texture/glyph counts. The fastest way to find
  // screens that don't render (no panels, missing MILO, empty text) — no nav
  // needed. Prints a one-line summary per screen, then exits.
  if (std::getenv("GHOGX_MENU_DUMP")) {
    ObjectDir& reg = mgr.registry();
    int ok = 0, empty = 0, total = 0;
    for (std::size_t i = 0; i < reg.size(); ++i) {
      Object* o = reg.at(i);
      if (!o) continue;
      std::string nm = o->name().c_str();
      if (nm.size() <= 7 || nm.compare(nm.size() - 7, 7, "_screen") != 0) continue;
      ++total;
      mgr.goto_screen(o->name());
      mgr.update(0.0f);
      Object* s = mgr.current_screen();
      std::vector<Symbol> pn = screen_panel_names(s);
      std::fprintf(stderr, "[dump] %-34s panels=%zu\n", nm.c_str(), pn.size());
      rebuild_scene(hdr, ark, mgr, s, renderer);
      rebuild_text(hdr, ark, mgr, s, renderer, impact_font, locale);
      renderer.draw();
      win->present();
    }
    std::fprintf(stderr, "[dump] %d screens audited (ok=%d empty=%d)\n", total, ok, empty);
    return 0;
  }

  // Per-screen nav state: the focusable components (with nav links) + disabled set.
  std::vector<MenuLabel> cur_labels = gather_labels(hdr, ark, mgr, shown);
  std::unordered_set<std::string> cur_disabled = compute_disabled(mgr);
  auto focus_name = [&]() -> std::string {
    Object* s = mgr.current_screen();
    if (!s) return "";
    Symbol fpn = s->get_property(Symbol("focus")).as_symbol().value_or(Symbol());
    Object* p = fpn.valid() ? mgr.find_object(fpn) : nullptr;
    return p ? p->get_property(Symbol("focus")).as_symbol().value_or(Symbol()).c_str() : "";
  };
  std::string last_focus = focus_name();

  // Headless auto-nav harness (GHOGX_MENU_NAV="down,confirm,back,focus:main_career.btn"
  // ...) — one action every kNavStep frames, so screenshots can reach any screen.
  std::vector<std::string> nav;
  if (const char* env = std::getenv("GHOGX_MENU_NAV")) {
    std::string e(env), tok;
    for (char ch : e + ",") { if (ch == ',') { if (!tok.empty()) nav.push_back(tok); tok.clear(); } else tok += ch; }
  }
  size_t nav_i = 0;
  const uint64_t kNavStep = 5;

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

    // Input (live controller/keyboard) -> focus nav + the real menu scripts.
    if (win->action_pressed(Action::Down))    focus_move(mgr, cur_labels, cur_disabled, +1);
    if (win->action_pressed(Action::Up))      focus_move(mgr, cur_labels, cur_disabled, -1);
    if (win->action_pressed(Action::Confirm)) do_confirm(mgr);
    if (win->action_pressed(Action::Back))    do_back(mgr);

    // Scripted auto-nav (headless testing): one action per kNavStep frames.
    if (nav_i < nav.size() && frame == (nav_i + 1) * kNavStep) {
      const std::string& a = nav[nav_i++];
      if (a == "down") focus_move(mgr, cur_labels, cur_disabled, +1);
      else if (a == "up") focus_move(mgr, cur_labels, cur_disabled, -1);
      else if (a == "confirm") do_confirm(mgr);
      else if (a == "back") do_back(mgr);
      else if (a.rfind("focus:", 0) == 0) {
        Object* s = mgr.current_screen();
        Symbol fpn = s ? s->get_property(Symbol("focus")).as_symbol().value_or(Symbol()) : Symbol();
        if (Object* p = fpn.valid() ? mgr.find_object(fpn) : nullptr)
          p->set_property(Symbol("focus"), DataNode::Sym(Symbol(a.substr(6).c_str())));
      }
    }

    mgr.update(dt);

    // Reload the scene + text when the screen changed; re-render text (re-colour)
    // when only the focus moved.
    if (mgr.current_screen() != shown) {
      shown = mgr.current_screen();
      cur_labels = gather_labels(hdr, ark, mgr, shown);
      cur_disabled = compute_disabled(mgr);
      rebuild_scene(hdr, ark, mgr, shown, renderer);
      rebuild_text(hdr, ark, mgr, shown, renderer, impact_font, locale);
      last_focus = focus_name();
    } else if (focus_name() != last_focus) {
      last_focus = focus_name();
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
