// gh2test - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include "ps2_ark_hook.h"

#include <rex/rex_app.h>

class Gh2testApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Gh2testApp>(new Gh2testApp(ctx, "gh2test",
        PPCImageConfig));
  }

  // OnConfigurePaths runs after CLI/cvar parsing has populated the path
  // config but before the guest XEX starts touching files. That's the
  // window where we hand the PS2 ARK hook the game data root so its
  // lazy load can resolve gen/main.hdr cleanly.
  void OnConfigurePaths(rex::PathConfig& paths) override {
    ps2_ark::set_game_data_root(paths.game_data_root.string());
  }
};
