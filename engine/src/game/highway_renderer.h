// engine/src/game/highway_renderer.h
//
// HighwayRenderer — the Guitar Hero II 3-D note highway.
//
// Renders the classic GH perspective fretboard: a textured board receding into
// the distance, colored gems scrolling toward the strikeline, fret-target
// "smashers", sustain tails, beat lines and hit flames. All art is the
// authentic GH2 PS2 track texture set, loaded natively at runtime from
// track/gen/track.milo_ps2 in the ARK (ARK -> MILO -> PS2 Tex decode -> D3D9
// texture). Nothing is pre-extracted.

#pragma once

#include "chart/midi_reader.h"

#include <cstdint>
#include <map>
#include <string>

struct IDirect3DDevice9;
struct IDirect3DTexture9;

namespace ghogx::render { class Window; }

namespace ghogx::game {

class HighwayRenderer {
 public:
  explicit HighwayRenderer(ghogx::render::Window& win);
  ~HighwayRenderer();

  HighwayRenderer(const HighwayRenderer&) = delete;
  HighwayRenderer& operator=(const HighwayRenderer&) = delete;

  // Load the GH2 track texture set natively from track/gen/track.milo_ps2.
  // Returns false if the MILO/textures can't be loaded.
  bool load_textures(const std::string& hdr_path, const std::string& ark_path);
  bool textures_loaded() const { return loaded_; }

  // Draw one frame of the 3-D note highway.
  //   song_time      — playback position in seconds (drives note scroll).
  //   chart          — the parsed chart.
  //   difficulty     — 0=Easy 1=Medium 2=Hard 3=Expert.
  //   fret_held_mask — bits 0-4 = lanes currently held (smasher glow).
  //   hit_flash      — per-lane flame intensity 0..1 (recent hits, decaying).
  //   lookahead_sec  — seconds of chart visible ahead of the strikeline.
  void draw(double song_time, const ghogx::chart::Chart& chart, int difficulty,
            uint32_t fret_held_mask, const float hit_flash[5],
            float lookahead_sec = 1.5f);

 private:
  IDirect3DTexture9* tex(const std::string& name) const;

  ghogx::render::Window* win_;
  IDirect3DDevice9* dev_ = nullptr;
  std::map<std::string, IDirect3DTexture9*> textures_;
  bool loaded_ = false;
};

}  // namespace ghogx::game
