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

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace ghogx::ui {

struct MenuRunOptions {
  // Menu navigation stays human-driven.  This flag affects only the in-song
  // controller so the normal front-end flow can be exercised hands-free.
  bool gameplay_autoplay = false;
  // Optional state-aware proof harness.  It presses the real menu components;
  // it does not bypass the stock screen scripts.
  bool automate_full_loop = false;
  std::string preferred_song;
  int preferred_difficulty = 1;
  bool play_boot_presentation = true;
  // Optional independent gameplay/content archive. The primary archive still
  // owns the GH2 retail front end; this mount owns songs, venues, and band.
  std::string content_hdr;
  std::string content_ark;
  // Read-only sidecar archives used for independently selected foreign
  // characters. They never replace the primary GH2 presentation archive.
  std::vector<std::pair<std::string, std::string>> auxiliary_asset_archives;
  std::map<uint64_t, std::string> screenshot_sequence;
};

// hdr/ark = the PS2 ARK; screenshot_path (optional) captures one frame at
// screenshot_frame; max_frames>0 auto-exits (0 = run until the window closes).
int run_menu_mode(const std::string& hdr, const std::string& ark,
                  const std::string& screenshot_path, int screenshot_frame,
                  int max_frames, int window_width = 960,
                  int window_height = 720, float fixed_dt = 0.0f,
                  const MenuRunOptions& options = {});

}  // namespace ghogx::ui
