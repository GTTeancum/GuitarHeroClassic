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

struct FoFiXRockState {
  double value = 15000.0;
  double minus_amount = 400.0;
  double plus_amount = 15.0;
};

struct FoFiXStarPowerState {
  double value = 0.0;
  bool active = false;
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

void fofix_apply_rock_hit(FoFiXRockState& state,
                          double power_multiplier = 1.0);

void fofix_apply_rock_miss(FoFiXRockState& state,
                           double power_multiplier = 1.0);

void fofix_apply_rock_overstrum(FoFiXRockState& state,
                                double power_multiplier = 1.0);

double fofix_rock_fill(const FoFiXRockState& state);

bool fofix_rock_failed(const FoFiXRockState& state);

void fofix_award_star_phrase(FoFiXStarPowerState& state);

double fofix_star_power_fill(const FoFiXStarPowerState& state);

}  // namespace ghogx::game
