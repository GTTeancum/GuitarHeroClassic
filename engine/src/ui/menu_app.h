// engine/src/ui/menu_app.h
//
// Windowed menu mode: boots the screen manager (all 40 stock screens loaded
// verbatim + the config-DTB-backed game-side), renders the CURRENT screen's
// panel MILOs as the real 3-D scene (GH2 menus are 3-D scenes through a camera,
// not 2-D quads -- so this reuses the MiloSceneRenderer), and drives navigation
// from controller/keyboard input through the real SELECT_START_MSG scripts.
//
// This is the presentation layer over the (committed) menu logic engine.

#pragma once

#include <string>

namespace ghogx::ui {

struct MenuModeOptions {
  std::string screenshot_path;
  int screenshot_frame = 30;
  int max_frames = 0;
  std::string tune_file;
};

// hdr/ark = the PS2 ARK; screenshot_path (optional) captures one frame at
// screenshot_frame; max_frames>0 auto-exits (0 = run until the window closes).
int run_menu_mode(const std::string& hdr, const std::string& ark,
                  const MenuModeOptions& options);

}  // namespace ghogx::ui
