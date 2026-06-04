// engine/src/game/gameplay.h
//
// Gameplay — song gameplay state machine.
//
// Loads a song from the PS2 ARK, drives the note-hit detection loop,
// and renders the 2-D note highway.

#pragma once

#include "chart/midi_reader.h"
#include "game/audio_player.h"
#include "game/highway_renderer.h"

#include <cstdint>
#include <memory>
#include <string>

namespace ghogx::render { class Window; }

namespace ghogx::game {

struct HitResult {
    bool hit;
    bool was_hopo;
};

class Gameplay {
 public:
  Gameplay() = default;
  ~Gameplay() = default;

  Gameplay(const Gameplay&) = delete;
  Gameplay& operator=(const Gameplay&) = delete;

  // Load a song from the ARK by short name (e.g. "shoutatthedevil").
  // hdr_path/ark_path are the paths to MAIN.HDR / MAIN_0.ARK.
  // difficulty: 0=Easy 1=Medium 2=Hard 3=Expert.
  bool load_song(const std::string& hdr_path, const std::string& ark_path,
                 const std::string& shortname, int difficulty = 3);

  // Called each frame.
  // dt        — frame delta seconds.
  // fret_mask — current button bitmask:
  //             bit0=Green  bit1=Red  bit2=Yellow  bit3=Blue  bit4=Orange  bit5=Strum.
  void tick(float dt, uint32_t fret_mask);

  // Draw the highway for this frame. Creates the HighwayRenderer on first call.
  void draw(ghogx::render::Window& win);

  bool is_loaded()   const { return chart_loaded_; }
  // Song is finished when the audio clock passes the chart duration.
  bool is_finished() const;
  double song_time() const { return song_time_; }
  int    score()     const { return score_; }
  int    streak()    const { return streak_; }
  int    difficulty()const { return difficulty_; }

 private:
  // Detect a strum-triggered or HOPO note hit in the given lane.
  HitResult try_hit(int lane, bool strummed, bool is_hopo_candidate);

  ghogx::chart::Chart chart_;
  bool chart_loaded_ = false;

  AudioPlayer audio_;
  std::unique_ptr<HighwayRenderer> highway_;

  double   song_time_      = 0.0;
  int      difficulty_     = 3;
  // Index of the next unprocessed note in chart_.notes[difficulty_].
  size_t   next_note_idx_  = 0;

  int      score_          = 0;
  int      streak_         = 0;
  int      multiplier_     = 1;

  // Per-frame hit/miss feedback for the renderer (cleared each tick).
  uint32_t hit_flash_mask_  = 0;
  uint32_t miss_flash_mask_ = 0;

  // Per-lane hit-flame intensity (1.0 on hit, decays to 0). Drives the
  // strikeline flames in the renderer.
  float lane_flash_[5] = {};

  // Previous-frame fret mask for edge detection.
  uint32_t prev_fret_mask_  = 0;

  // Per-lane: has this lane's gem been hit this pass (so we don't double-hit)?
  bool lane_hit_[5] = {};

  // Hit window: ±70 ms around the note's ideal time.
  static constexpr double kHitWindowSec = 0.070;

  std::string hdr_path_;
  std::string ark_path_;
};

}  // namespace ghogx::game
