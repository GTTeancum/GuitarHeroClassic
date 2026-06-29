#include "game/gameplay_rules.h"

#include <algorithm>

namespace ghogx::game {

namespace {

constexpr double kFoFiXStandardHitScale = 1.2;
constexpr double kMillisecondsPerSecond = 1000.0;

int popcount5(uint32_t mask) {
  int count = 0;
  mask &= 0x1fu;
  while (mask != 0) {
    count += static_cast<int>(mask & 1u);
    mask >>= 1;
  }
  return count;
}

int first_lane(uint32_t mask) {
  for (int lane = 0; lane < 5; ++lane) {
    if ((mask & (1u << lane)) != 0) return lane;
  }
  return 0;
}

}  // namespace

FoFiXHitWindow fofix_hit_window_for_bpm(double bpm) {
  const double clamped_bpm = std::clamp(bpm, 1.0, 200.0);
  const double margin_ms =
      std::max(0.0, 250.0 - clamped_bpm / 5.0 -
                        70.0 * kFoFiXStandardHitScale);
  const double margin_sec = margin_ms / kMillisecondsPerSecond;
  return FoFiXHitWindow{margin_sec, margin_sec};
}

bool fofix_note_in_window(double song_time,
                          double note_time,
                          const FoFiXHitWindow& window) {
  return note_time >= song_time - window.late_sec &&
         note_time <= song_time + window.early_sec;
}

bool fofix_note_missed(double song_time,
                       double note_time,
                       const FoFiXHitWindow& window) {
  return note_time < song_time - window.late_sec;
}

bool fofix_match_frets(uint32_t held_frets, uint32_t required_frets) {
  held_frets &= 0x1fu;
  required_frets &= 0x1fu;
  const int required_count = popcount5(required_frets);
  if (required_count == 0) return held_frets == 0;

  if (required_count > 1) {
    return held_frets == required_frets;
  }

  const int lane = first_lane(required_frets);
  const uint32_t required_bit = 1u << lane;
  if ((held_frets & required_bit) == 0) return false;

  const uint32_t higher_frets = 0x1fu & ~((1u << (lane + 1)) - 1u);
  return (held_frets & higher_frets) == 0;
}

int fofix_multiplier_for_streak(int streak) {
  if (streak >= 30) return 4;
  if (streak >= 20) return 3;
  if (streak >= 10) return 2;
  return 1;
}

FoFiXScoreAward fofix_apply_hit(FoFiXScoreState& state, int gem_count) {
  ++state.streak;
  state.multiplier = fofix_multiplier_for_streak(state.streak);
  const int gems = std::max(1, gem_count);
  const int points = gems * 50 * state.multiplier;
  state.score += points;
  return FoFiXScoreAward{points, state.multiplier};
}

void fofix_apply_miss(FoFiXScoreState& state) {
  state.streak = 0;
  state.multiplier = 1;
}

}  // namespace ghogx::game
