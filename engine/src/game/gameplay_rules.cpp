#include "game/gameplay_rules.h"

#include <algorithm>

namespace ghogx::game {

namespace {

constexpr double kFoFiXStandardHitScale = 1.2;
constexpr double kMillisecondsPerSecond = 1000.0;
constexpr double kRockMax = 30000.0;
constexpr double kMinBase = 400.0;
constexpr double kPlusBase = 15.0;
constexpr double kMinGain = 2.0;
constexpr double kPlusGain = 7.0;
constexpr double kStarPhraseAward = 25.0;
constexpr double kStarActivationThreshold = 50.0;
constexpr double kStarDrainDivisorMs = 200.0;
constexpr double kBaseSustainScore = 0.1;

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

FoFiXScoreAward fofix_apply_hit(FoFiXScoreState& state,
                                int gem_count,
                                int power_multiplier) {
  ++state.streak;
  state.multiplier = fofix_multiplier_for_streak(state.streak);
  const int gems = std::max(1, gem_count);
  const int points = gems * 50 * state.multiplier *
                     std::max(1, power_multiplier);
  state.score += points;
  return FoFiXScoreAward{points, state.multiplier};
}

void fofix_apply_miss(FoFiXScoreState& state) {
  state.streak = 0;
  state.multiplier = 1;
}

void fofix_apply_rock_hit(FoFiXRockState& state, double power_multiplier) {
  power_multiplier = std::max(1.0, power_multiplier);
  if (state.value < kRockMax) {
    state.plus_amount += kPlusGain * power_multiplier;
    state.value += state.plus_amount * power_multiplier;
  }
  state.value = std::clamp(state.value, 0.0, kRockMax);
  if (state.minus_amount > kMinBase) {
    state.minus_amount -= (kMinGain / 2.0) * power_multiplier;
  }
  state.minus_amount = std::max(state.minus_amount, kMinBase);
  state.plus_amount = std::max(state.plus_amount, kPlusBase);
}

void fofix_apply_rock_miss(FoFiXRockState& state, double power_multiplier) {
  power_multiplier = std::max(1.0, power_multiplier);
  state.minus_amount += kMinGain / power_multiplier;
  const double rock_minus = state.minus_amount / power_multiplier;
  state.value -= rock_minus;
  if (state.plus_amount > kPlusBase) {
    state.plus_amount -= (kPlusGain * 2.0) / power_multiplier;
  }
  state.value = std::clamp(state.value, 0.0, kRockMax);
  state.minus_amount = std::max(state.minus_amount, kMinBase);
  state.plus_amount = std::max(state.plus_amount, kPlusBase);
}

void fofix_apply_rock_overstrum(FoFiXRockState& state,
                                double power_multiplier) {
  power_multiplier = std::max(1.0, power_multiplier);
  state.minus_amount += kMinGain / 5.0 / power_multiplier;
  const double rock_minus = state.minus_amount / 5.0 / power_multiplier;
  state.value -= rock_minus;
  if (state.plus_amount > kPlusBase) {
    state.plus_amount -= kPlusGain / 2.5 / power_multiplier;
  }
  state.value = std::clamp(state.value, 0.0, kRockMax);
  state.minus_amount = std::max(state.minus_amount, kMinBase);
  state.plus_amount = std::max(state.plus_amount, kPlusBase);
}

double fofix_rock_fill(const FoFiXRockState& state) {
  return std::clamp(state.value / kRockMax, 0.0, 1.0);
}

bool fofix_rock_failed(const FoFiXRockState& state) {
  return state.value <= 0.0;
}

void fofix_award_star_phrase(FoFiXStarPowerState& state) {
  state.value = std::clamp(state.value + kStarPhraseAward, 0.0, 100.0);
}

bool fofix_activate_star_power(FoFiXStarPowerState& state) {
  if (state.active || state.value < kStarActivationThreshold) return false;
  state.active = true;
  return true;
}

void fofix_update_star_power(FoFiXStarPowerState& state, double dt_seconds) {
  if (!state.active || dt_seconds <= 0.0) return;
  state.value -= (dt_seconds * kMillisecondsPerSecond) / kStarDrainDivisorMs;
  if (state.value <= 0.0) {
    state.value = 0.0;
    state.active = false;
  }
}

double fofix_star_power_fill(const FoFiXStarPowerState& state) {
  return std::clamp(state.value / 100.0, 0.0, 1.0);
}

int fofix_star_power_score_multiplier(const FoFiXStarPowerState& state) {
  return state.active ? 2 : 1;
}

int fofix_sustain_score(double held_seconds,
                        int note_count,
                        double beat_seconds,
                        int multiplier) {
  if (held_seconds <= 0.0 || note_count <= 0 || beat_seconds <= 0.0)
    return 0;
  if (held_seconds <= 1.1 * beat_seconds / 4.0)
    return 0;
  const double held_ms = held_seconds * kMillisecondsPerSecond;
  const int base_score =
      static_cast<int>(kBaseSustainScore * held_ms *
                       static_cast<double>(note_count));
  return base_score * std::max(1, multiplier);
}

}  // namespace ghogx::game
