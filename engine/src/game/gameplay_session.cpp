#include "game/gameplay_session.h"

#include <algorithm>
#include <utility>

namespace ghogx::game {

namespace {

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

FoFiXGameplaySession::FoFiXGameplaySession(std::vector<FoFiXSessionNote> notes,
                                           double bpm)
    : notes_(std::move(notes)), hit_window_(fofix_hit_window_for_bpm(bpm)) {
  std::sort(notes_.begin(), notes_.end(),
            [](const FoFiXSessionNote& a, const FoFiXSessionNote& b) {
              if (a.time != b.time) return a.time < b.time;
              return a.mask < b.mask;
            });
  consumed_.assign(notes_.size(), 0);
  beat_seconds_ = bpm > 0.0 ? 60.0 / bpm : 0.5;
}

size_t FoFiXGameplaySession::group_end(size_t start) const {
  const double t = notes_[start].time;
  size_t end = start + 1;
  while (end < notes_.size() && notes_[end].time == t) ++end;
  return end;
}

uint32_t FoFiXGameplaySession::group_mask(size_t start, size_t end) const {
  uint32_t mask = 0;
  for (size_t i = start; i < end; ++i) {
    if (i < consumed_.size() && consumed_[i]) continue;
    mask |= notes_[i].mask & 0x1fu;
  }
  return mask;
}

int FoFiXGameplaySession::group_gem_count(size_t start, size_t end) const {
  int count = 0;
  for (size_t i = start; i < end; ++i) {
    if (i < consumed_.size() && consumed_[i]) continue;
    count += std::max(1, popcount5(notes_[i].mask));
  }
  return count;
}

bool FoFiXGameplaySession::group_star_power(size_t start, size_t end) const {
  for (size_t i = start; i < end; ++i) {
    if (i < consumed_.size() && consumed_[i]) continue;
    if (notes_[i].star_power) return true;
  }
  return false;
}

void FoFiXGameplaySession::finish_star_phrase() {
  if (!star_phrase_active_) return;
  if (!star_phrase_missed_) fofix_award_star_phrase(star_power_);
  star_phrase_active_ = false;
  star_phrase_missed_ = false;
}

void FoFiXGameplaySession::observe_star_phrase(size_t start,
                                               size_t end,
                                               bool hit) {
  if (!group_star_power(start, end)) {
    finish_star_phrase();
    return;
  }
  if (!star_phrase_active_) {
    star_phrase_active_ = true;
    star_phrase_missed_ = false;
  }
  if (!hit) star_phrase_missed_ = true;
}

void FoFiXGameplaySession::award_sustain(const ActiveSustain& sustain,
                                         double held_until) {
  const double held_seconds =
      std::max(0.0, std::min(held_until, sustain.end_time) -
                        sustain.start_time);
  score_.score += fofix_sustain_score(
      held_seconds, sustain.gem_count, beat_seconds_,
      score_.multiplier * fofix_star_power_score_multiplier(star_power_));
}

void FoFiXGameplaySession::update_sustains(double song_time,
                                           uint32_t held_frets) {
  if (active_sustains_.empty()) return;
  std::vector<ActiveSustain> keep;
  keep.reserve(active_sustains_.size());
  for (const ActiveSustain& sustain : active_sustains_) {
    if (song_time >= sustain.end_time) {
      award_sustain(sustain, sustain.end_time);
      continue;
    }
    if (!fofix_match_frets(held_frets, sustain.mask)) {
      award_sustain(sustain, song_time);
      continue;
    }
    keep.push_back(sustain);
  }
  active_sustains_ = std::move(keep);
}

void FoFiXGameplaySession::start_sustain(size_t start,
                                         size_t end,
                                         double song_time) {
  uint32_t mask = 0;
  int gems = 0;
  double end_time = 1.0e30;
  for (size_t i = start; i < end; ++i) {
    if (notes_[i].end_time <= notes_[i].time + beat_seconds_ / 4.0) continue;
    mask |= notes_[i].mask & 0x1fu;
    gems += std::max(1, popcount5(notes_[i].mask));
    end_time = std::min(end_time, notes_[i].end_time);
  }
  if (mask == 0 || gems <= 0 || end_time == 1.0e30) return;
  active_sustains_.push_back(
      ActiveSustain{mask, gems, std::max(song_time, notes_[start].time),
                    end_time});
}

void FoFiXGameplaySession::apply_hit(size_t start,
                                     size_t end,
                                     double song_time) {
  const int gem_count = group_gem_count(start, end);
  if (gem_count <= 0) return;
  for (const ActiveSustain& sustain : active_sustains_)
    award_sustain(sustain, song_time);
  active_sustains_.clear();
  observe_star_phrase(start, end, true);
  fofix_apply_hit(score_, gem_count,
                  fofix_star_power_score_multiplier(star_power_));
  fofix_apply_rock_hit(rock_);
  for (size_t i = start; i < end; ++i) {
    if (i < consumed_.size()) consumed_[i] = 1;
  }
  start_sustain(start, end, song_time);
  ++hits_;
}

void FoFiXGameplaySession::apply_miss(size_t start, size_t end) {
  observe_star_phrase(start, end, false);
  fofix_apply_miss(score_);
  fofix_apply_rock_miss(rock_);
  for (size_t i = start; i < end; ++i) {
    if (i < consumed_.size()) consumed_[i] = 1;
  }
  ++misses_;
}

void FoFiXGameplaySession::apply_overstrum() {
  fofix_apply_miss(score_);
  fofix_apply_rock_overstrum(rock_);
  ++overstrums_;
}

void FoFiXGameplaySession::tick(double song_time, uint32_t fret_mask) {
  const double dt = std::max(0.0, song_time - last_time_);
  last_time_ = std::max(last_time_, song_time);
  if ((fret_mask & (1u << 6)) != 0) {
    fofix_activate_star_power(star_power_);
  }
  fofix_update_star_power(star_power_, dt);

  const uint32_t held_frets = fret_mask & 0x1fu;
  update_sustains(song_time, held_frets);

  const bool strummed = (fret_mask & (1u << 5)) != 0;
  bool hit_this_frame = false;
  bool missed_this_frame = false;

  while (next_note_ < notes_.size()) {
    if (next_note_ < consumed_.size() && consumed_[next_note_]) {
      ++next_note_;
      continue;
    }
    const size_t end = group_end(next_note_);
    if (!fofix_note_missed(song_time, notes_[next_note_].time, hit_window_))
      break;
    apply_miss(next_note_, end);
    missed_this_frame = true;
    next_note_ = end;
  }

  for (size_t i = next_note_; i < notes_.size(); ++i) {
    if (i < consumed_.size() && consumed_[i]) continue;
    const size_t end = group_end(i);
    const double note_time = notes_[i].time;
    if (note_time > song_time + hit_window_.early_sec) break;
    if (!fofix_note_in_window(song_time, note_time, hit_window_)) {
      i = end - 1;
      continue;
    }

    const uint32_t required = group_mask(i, end);
    const int gem_count = group_gem_count(i, end);
    if (gem_count <= 0 || required == 0) {
      i = end - 1;
      continue;
    }

    const bool is_hopo = gem_count == 1 && notes_[i].hopo && score_.streak > 0;
    const int lane = first_lane(required);
    bool can_hit = false;
    if (is_hopo && (held_frets & (1u << lane)) != 0 &&
        (prev_fret_mask_ & (1u << lane)) == 0) {
      can_hit = fofix_match_frets(held_frets, required);
    }
    if (!can_hit && strummed) {
      can_hit = fofix_match_frets(held_frets, required);
    }
    if (can_hit) {
      apply_hit(i, end, song_time);
      hit_this_frame = true;
    }
    i = end - 1;
  }

  while (next_note_ < notes_.size() && next_note_ < consumed_.size() &&
         consumed_[next_note_]) {
    ++next_note_;
  }
  if (next_note_ >= notes_.size()) finish_star_phrase();

  if (strummed && !hit_this_frame && !missed_this_frame && held_frets != 0)
    apply_overstrum();
  prev_fret_mask_ = fret_mask;
}

}  // namespace ghogx::game
