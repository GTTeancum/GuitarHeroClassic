#pragma once

#include "game/gameplay_rules.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ghogx::chart {
struct Chart;
}

namespace ghogx::game {

struct FoFiXSessionNote {
  double time = 0.0;
  double end_time = 0.0;
  uint32_t mask = 0;
  bool hopo = false;
  bool star_power = false;
  double beat_seconds = 0.0;
  double hit_early_sec = 0.0;
  double hit_late_sec = 0.0;
};

class FoFiXGameplaySession {
 public:
  explicit FoFiXGameplaySession(std::vector<FoFiXSessionNote> notes,
                                double bpm = 120.0);
  static FoFiXGameplaySession FromChart(const ghogx::chart::Chart& chart,
                                        int difficulty);

  void tick(double song_time, uint32_t fret_mask);
  void seek_without_scoring(double song_time);

  int score() const { return score_.score; }
  int streak() const { return score_.streak; }
  int multiplier() const { return score_.multiplier; }
  double rock_fill() const { return fofix_rock_fill(rock_); }
  double star_power_fill() const { return fofix_star_power_fill(star_power_); }
  bool star_power_active() const { return star_power_.active; }
  bool failed() const { return fofix_rock_failed(rock_); }
  int hits() const { return hits_; }
  int misses() const { return misses_; }
  int overstrums() const { return overstrums_; }

 private:
  struct ActiveSustain {
    uint32_t mask = 0;
    int gem_count = 0;
    double start_time = 0.0;
    double end_time = 0.0;
    double beat_seconds = 0.5;
  };

  FoFiXHitWindow window_for_note(size_t index) const;
  size_t group_end(size_t start) const;
  uint32_t group_mask(size_t start, size_t end) const;
  int group_gem_count(size_t start, size_t end) const;
  bool group_star_power(size_t start, size_t end) const;
  void finish_star_phrase();
  void observe_star_phrase(size_t start, size_t end, bool hit);
  void award_sustain(const ActiveSustain& sustain, double held_until);
  void update_sustains(double song_time, uint32_t held_frets);
  void start_sustain(size_t start, size_t end, double song_time);
  void apply_hit(size_t start, size_t end, double song_time);
  void apply_miss(size_t start, size_t end);
  void apply_overstrum();

  std::vector<FoFiXSessionNote> notes_;
  std::vector<uint8_t> consumed_;
  std::vector<ActiveSustain> active_sustains_;
  FoFiXHitWindow hit_window_;
  double beat_seconds_ = 0.5;
  double last_time_ = 0.0;
  uint32_t prev_fret_mask_ = 0;
  size_t next_note_ = 0;
  FoFiXScoreState score_;
  FoFiXRockState rock_;
  FoFiXStarPowerState star_power_;
  bool star_phrase_active_ = false;
  bool star_phrase_missed_ = false;
  int hits_ = 0;
  int misses_ = 0;
  int overstrums_ = 0;
};

}  // namespace ghogx::game
