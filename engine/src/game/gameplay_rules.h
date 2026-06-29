#pragma once

#include <cstdint>

namespace ghogx::game {

struct FoFiXHitWindow {
  double early_sec = 0.0;
  double late_sec = 0.0;
};

struct FoFiXScoreState {
  int score = 0;
  int streak = 0;
  int multiplier = 1;
};

struct FoFiXScoreAward {
  int points = 0;
  int multiplier = 1;
};

FoFiXHitWindow fofix_hit_window_for_bpm(double bpm);

bool fofix_note_in_window(double song_time,
                          double note_time,
                          const FoFiXHitWindow& window);

bool fofix_note_missed(double song_time,
                       double note_time,
                       const FoFiXHitWindow& window);

bool fofix_match_frets(uint32_t held_frets, uint32_t required_frets);

int fofix_multiplier_for_streak(int streak);

FoFiXScoreAward fofix_apply_hit(FoFiXScoreState& state, int gem_count);

void fofix_apply_miss(FoFiXScoreState& state);

}  // namespace ghogx::game
