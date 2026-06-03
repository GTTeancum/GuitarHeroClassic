// engine/src/ui/ui_test.cpp
//
// Headless boot test for the UI object layer + screen manager, run against the
// REAL stock GH2 DTBs from the PS2 ARK. Loads ui/gen/{main,splash,quickplay}.dtb,
// asserts the {new GHPanel/GHScreen ...} objects + their parsed handler blocks
// exist, then drives the manager: goto_screen(main_screen) runs the authored
// (enter)/reset_player_settings scripts, and a simulated SELECT_START on
// main_quickspin.btn runs the real SELECT_START_MSG switch -> {ui goto_screen
// qp_selsong_screen}. Skips (exit 0) when the ARK is absent.

#include "core/data_node.h"
#include "core/symbol.h"
#include "ui/screen_loader.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include "ark_v3.h"

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

using namespace ghogx;
namespace fs = std::filesystem;

static int g_failures = 0;
#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      std::fprintf(stderr, "FAIL %s:%d  CHECK(%s)\n", __FILE__, __LINE__, \
                   #cond);                                                \
      ++g_failures;                                                       \
    }                                                                     \
  } while (0)

static std::string first_existing(const std::string& dir, std::vector<std::string> names) {
  for (auto& n : names) {
    std::string p = dir + "/" + n;
    if (fs::exists(p)) return p;
  }
  return {};
}

static bool has_unhandled(const ui::ScreenManager& mgr, const std::string& key) {
  for (auto& u : mgr.unhandled())
    if (u == key) return true;
  return false;
}

int main(int argc, char** argv) {
  std::string ark_dir =
      argc > 1 ? argv[1]
               : "C:/Programming/GitHub/Guitar Hero II/Guitar Hero II PS2 (USA)/GEN";
  std::string hdr = first_existing(ark_dir, {"MAIN.HDR", "main.hdr"});
  std::string ark0 = first_existing(ark_dir, {"MAIN_0.ARK", "main_0.ark"});
  if (hdr.empty() || ark0.empty()) {
    std::printf("ghogx_ui_test: SKIP (no stock ARK at %s)\n", ark_dir.c_str());
    return 0;
  }

  ui::register_ui_classes();
  ui::ScreenManager mgr;
  ui::install_default_singletons(mgr);

  gh::ark::ArkV3Reader ark = gh::ark::ArkV3Reader::load(hdr);
  std::vector<std::string> arks = {ark0};
  bool ok = true;
  ok &= ui::load_ui_dtb_from_ark(ark, arks, "ui/gen/main.dtb", mgr);
  ok &= ui::load_ui_dtb_from_ark(ark, arks, "ui/gen/splash.dtb", mgr);
  ok &= ui::load_ui_dtb_from_ark(ark, arks, "ui/gen/quickplay.dtb", mgr);
  CHECK(ok);

  // 1. The {new ...} objects exist.
  Object* main_panel = mgr.find_object(Symbol("main_panel"));
  Object* main_screen = mgr.find_object(Symbol("main_screen"));
  CHECK(main_panel != nullptr);
  CHECK(main_screen != nullptr);
  CHECK(main_panel && main_panel->class_name() == Symbol("GHPanel"));
  CHECK(main_screen && main_screen->class_name() == Symbol("GHScreen"));

  // 2. main_panel's authored handler blocks parsed (enter/poll/SELECT_START_MSG
  //    + the custom reset_player_settings).
  if (auto* mp = dynamic_cast<ui::UiObject*>(main_panel)) {
    CHECK(mp->has_handler(Symbol("enter")));
    CHECK(mp->has_handler(Symbol("poll")));
    CHECK(mp->has_handler(Symbol("SELECT_START_MSG")));
    CHECK(mp->has_handler(Symbol("reset_player_settings")));
    // ...but config entries are properties, not handlers.
    CHECK(!mp->has_handler(Symbol("file")));
    CHECK(main_panel->get_property(Symbol("file")).as_symbol().has_value());
  }

  // 3. goto_screen(main_screen) runs the real (enter) scripts. reset_player_
  //    settings hits the game stub -> proves the authored script executed.
  mgr.goto_screen(Symbol("main_screen"));
  CHECK(mgr.current_screen() == main_screen);
  CHECK(has_unhandled(mgr, "game::set_venue"));        // {game set_venue small2}
  CHECK(has_unhandled(mgr, "game::set_character"));    // {game set_character punk1 TRUE}

  // 4. Simulate a SELECT_START on main_quickspin.btn: the real SELECT_START_MSG
  //    switch must route to {ui goto_screen qp_selsong_screen}.
  CHECK(mgr.find_object(Symbol("qp_selsong_screen")) != nullptr);  // quickplay.dtb loaded it
  mgr.set_global(Symbol("component"), DataNode::Sym(Symbol("main_quickspin.btn")));
  main_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
  CHECK(mgr.current_screen() != nullptr &&
        mgr.current_screen()->name() == Symbol("qp_selsong_screen"));

  if (g_failures == 0) {
    std::printf("ghogx_ui_test: stock main.dtb boots -> object tree + handlers + "
                "goto_screen + SELECT_START all run -- passed\n");
    return 0;
  }
  std::fprintf(stderr, "ghogx_ui_test: %d check(s) failed\n", g_failures);
  return 1;
}
