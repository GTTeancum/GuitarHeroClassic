#include "game/gameplay_session.h"

#include "chart/midi_reader.h"

#include <algorithm>
#include <cmath>
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

double tempo_bpm_at_tick(const ghogx::chart::Chart& chart, uint32_t tick) {
  uint32_t us_per_beat = 500000;
  for (const auto& tempo : chart.tempo_map) {
    if (tempo.tick > tick) break;
    if (tempo.us_per_beat != 0) us_per_beat = tempo.us_per_beat;
  }
  return 60000000.0 / static_cast<double>(us_per_beat);
}

double beat_seconds_at_tick(const ghogx::chart::Chart& chart, uint32_t tick) {
  const double bpm = tempo_bpm_at_tick(chart, tick);
  return bpm > 0.0 ? 60.0 / bpm : 0.5;
}

}  // namespace

FoFiXGameplaySession::FoFiXGameplaySession(std::vector<FoFiXSessionNote> notes,
                                           double bpm)
    : notes_(std::move(notes)), hit_window_(fofix_hit_window_for_bpm(bpm)) {
  beat_seconds_ = bpm > 0.0 ? 60.0 / bpm : 0.5;
  std::sort(notes_.begin(), notes_.end(),
            [](const FoFiXSessionNote& a, const FoFiXSessionNote& b) {
              if (a.time != b.time) return a.time < b.time;
              return a.mask < b.mask;
            });
  for (FoFiXSessionNote& note : notes_) {
    if (note.end_time < note.time) note.end_time = note.time;
    if (note.beat_seconds <= 0.0 || !std::isfinite(note.beat_seconds)) {
      note.beat_seconds = beat_seconds_;
    }
    if (note.hit_early_sec <= 0.0 || !std::isfinite(note.hit_early_sec)) {
      note.hit_early_sec = hit_window_.early_sec;
    }
    if (note.hit_late_sec <= 0.0 || !std::isfinite(note.hit_late_sec)) {
      note.hit_late_sec = hit_window_.late_sec;
    }
  }
  consumed_.assign(notes_.size(), 0);
}

FoFiXGameplaySession FoFiXGameplaySession::FromChart(
    const ghogx::chart::Chart& chart, int difficulty) {
  const int diff = std::clamp(difficulty, 0, 3);
  std::vector<FoFiXSessionNote> notes;
  notes.reserve(chart.notes[diff].size());
  for (const auto& note : chart.notes[diff]) {
    const double bpm = tempo_bpm_at_tick(chart, note.tick_on);
    const FoFiXHitWindow window = fofix_hit_window_for_bpm(bpm);
    notes.push_back(FoFiXSessionNote{
        chart.tick_to_sec(note.tick_on),
        std::max(chart.tick_to_sec(note.tick_on),
                 chart.tick_to_sec(note.tick_off)),
        1u << std::clamp(note.lane, 0, 4),
        note.is_hopo,
        note.star_power,
        beat_seconds_at_tick(chart, note.tick_on),
        window.early_sec,
        window.late_sec,
        notes.size(),
        note.tick_on,
    });
  }
  return FoFiXGameplaySession(std::move(notes));
}

FoFiXHitWindow FoFiXGameplaySession::window_for_note(size_t index) const {
  if (index >= notes_.size()) return hit_window_;
  return FoFiXHitWindow{notes_[index].hit_early_sec,
                        notes_[index].hit_late_sec};
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

FoFiXSessionEvent FoFiXGameplaySession::make_event(
    FoFiXSessionEventType type,
    double time,
    uint32_t mask,
    int gem_count,
    int score_delta,
    size_t source_index,
    uint32_t source_tick) const {
  FoFiXSessionEvent event;
  event.type = type;
  event.time = time;
  event.mask = mask;
  event.gem_count = gem_count;
  event.score_delta = score_delta;
  event.score = score_.score;
  event.streak = score_.streak;
  event.multiplier = score_.multiplier;
  event.source_index = source_index;
  event.source_tick = source_tick;
  event.rock_fill = fofix_rock_fill(rock_);
  event.star_power_fill = fofix_star_power_fill(star_power_);
  event.failed = fofix_rock_failed(rock_);
  return event;
}

void FoFiXGameplaySession::finish_star_phrase() {
  if (!star_phrase_active_) return;
  if (!star_phrase_missed_) {
    fofix_award_star_phrase(star_power_);
    last_events_.push_back(make_event(FoFiXSessionEventType::StarPhraseComplete,
                                      last_time_, 0, 0, 0,
                                      star_phrase_source_index_,
                                      star_phrase_source_tick_));
  } else {
    last_events_.push_back(make_event(FoFiXSessionEventType::StarPhraseMiss,
                                      last_time_, 0, 0, 0,
                                      star_phrase_source_index_,
                                      star_phrase_source_tick_));
  }
  star_phrase_active_ = false;
  star_phrase_missed_ = false;
  star_phrase_source_index_ = static_cast<size_t>(-1);
  star_phrase_source_tick_ = UINT32_MAX;
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
    star_phrase_source_index_ = notes_[start].source_index;
    star_phrase_source_tick_ = notes_[start].source_tick;
  }
  if (!hit) star_phrase_missed_ = true;
}

void FoFiXGameplaySession::award_sustain(const ActiveSustain& sustain,
                                         double held_until) {
  const double held_seconds =
      std::max(0.0, std::min(held_until, sustain.end_time) -
                        sustain.start_time);
  const int points = fofix_sustain_score(
      held_seconds, sustain.gem_count, sustain.beat_seconds,
      score_.multiplier * fofix_star_power_score_multiplier(star_power_));
  score_.score += points;
  if (points > 0) {
    last_events_.push_back(make_event(FoFiXSessionEventType::Sustain,
                                      held_until, sustain.mask,
                                      sustain.gem_count, points,
                                      sustain.source_index,
                                      sustain.source_tick));
  }
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
    if (notes_[i].end_time <= notes_[i].time + notes_[i].beat_seconds / 4.0)
      continue;
    mask |= notes_[i].mask & 0x1fu;
    gems += std::max(1, popcount5(notes_[i].mask));
    end_time = std::min(end_time, notes_[i].end_time);
  }
  if (mask == 0 || gems <= 0 || end_time == 1.0e30) return;
  active_sustains_.push_back(
      ActiveSustain{mask, gems, std::max(song_time, notes_[start].time),
                    end_time, notes_[start].beat_seconds,
                    notes_[start].source_index,
                    notes_[start].source_tick});
}

void FoFiXGameplaySession::apply_hit(size_t start,
                                     size_t end,
                                     double song_time) {
  const int gem_count = group_gem_count(start, end);
  if (gem_count <= 0) return;
  const uint32_t mask = group_mask(start, end);
  active_sustains_.clear();
  observe_star_phrase(start, end, true);
  const FoFiXScoreAward award = fofix_apply_hit(
      score_, gem_count, fofix_star_power_score_multiplier(star_power_));
  fofix_apply_rock_hit(
      rock_,
      static_cast<double>(fofix_star_power_score_multiplier(star_power_)));
  for (size_t i = start; i < end; ++i) {
    if (i < consumed_.size()) consumed_[i] = 1;
  }
  start_sustain(start, end, song_time);
  ++hits_;
  last_events_.push_back(make_event(FoFiXSessionEventType::Hit, song_time,
                                    mask, gem_count, award.points,
                                    notes_[start].source_index,
                                    notes_[start].source_tick));
}

void FoFiXGameplaySession::apply_miss(size_t start, size_t end) {
  const uint32_t mask = group_mask(start, end);
  const int gem_count = group_gem_count(start, end);
  observe_star_phrase(start, end, false);
  fofix_apply_miss(score_);
  fofix_apply_rock_miss(
      rock_,
      static_cast<double>(fofix_star_power_score_multiplier(star_power_)));
  for (size_t i = start; i < end; ++i) {
    if (i < consumed_.size()) consumed_[i] = 1;
  }
  ++misses_;
  last_events_.push_back(make_event(FoFiXSessionEventType::Miss,
                                    notes_[start].time, mask, gem_count, 0,
                                    notes_[start].source_index,
                                    notes_[start].source_tick));
}

void FoFiXGameplaySession::apply_skip(size_t start, size_t end) {
  observe_star_phrase(start, end, false);
  for (size_t i = start; i < end; ++i) {
    if (i < consumed_.size()) consumed_[i] = 1;
  }
}

void FoFiXGameplaySession::apply_overstrum(uint32_t held_frets) {
  fofix_apply_miss(score_);
  fofix_apply_rock_overstrum(
      rock_,
      static_cast<double>(fofix_star_power_score_multiplier(star_power_)));
  ++overstrums_;
  last_events_.push_back(make_event(FoFiXSessionEventType::Overstrum,
                                    last_time_, held_frets & 0x1fu, 0, 0,
                                    static_cast<size_t>(-1), UINT32_MAX));
}

void FoFiXGameplaySession::seek_without_scoring(double song_time) {
  last_events_.clear();
  last_time_ = std::max(0.0, song_time);
  prev_fret_mask_ = 0;
  active_sustains_.clear();
  star_phrase_active_ = false;
  star_phrase_missed_ = false;
  star_phrase_source_index_ = static_cast<size_t>(-1);
  star_phrase_source_tick_ = UINT32_MAX;
  while (next_note_ < notes_.size()) {
    if (next_note_ < consumed_.size() && consumed_[next_note_]) {
      ++next_note_;
      continue;
    }
    if (!fofix_note_missed(last_time_, notes_[next_note_].time,
                           window_for_note(next_note_))) {
      break;
    }
    const size_t end = group_end(next_note_);
    for (size_t i = next_note_; i < end; ++i) {
      if (i < consumed_.size()) consumed_[i] = 1;
    }
    next_note_ = end;
  }
}

void FoFiXGameplaySession::tick(double song_time, uint32_t fret_mask) {
  last_events_.clear();
  const double dt = std::max(0.0, song_time - last_time_);
  last_time_ = std::max(last_time_, song_time);
  if ((fret_mask & (1u << 6)) != 0) {
    if (fofix_activate_star_power(star_power_)) {
      last_events_.push_back(make_event(FoFiXSessionEventType::StarPowerActivate,
                                        song_time, 1u << 6, 0, 0,
                                        static_cast<size_t>(-1), UINT32_MAX));
    }
  }
  fofix_update_star_power(star_power_, dt);
  if (fofix_rock_failed(rock_)) {
    active_sustains_.clear();
    prev_fret_mask_ = fret_mask;
    return;
  }

  const bool strummed =
      (fret_mask & (1u << 5)) != 0 && (prev_fret_mask_ & (1u << 5)) == 0;
  const uint32_t held_frets = fret_mask & 0x1fu;
  if (strummed)
    active_sustains_.clear();
  else
    update_sustains(song_time, held_frets);
  bool hit_this_frame = false;
  bool overstrum_candidate_seen = false;
  size_t overstrum_candidate_start = 0;
  size_t overstrum_candidate_end = 0;

  while (next_note_ < notes_.size()) {
    if (next_note_ < consumed_.size() && consumed_[next_note_]) {
      ++next_note_;
      continue;
    }
    const size_t end = group_end(next_note_);
    if (!fofix_note_missed(song_time, notes_[next_note_].time,
                           window_for_note(next_note_)))
      break;
    apply_miss(next_note_, end);
    next_note_ = end;
  }

  for (size_t i = next_note_; i < notes_.size(); ++i) {
    if (i < consumed_.size() && consumed_[i]) continue;
    const size_t end = group_end(i);
    const double note_time = notes_[i].time;
    const FoFiXHitWindow window = window_for_note(i);
    if (note_time > song_time + window.early_sec) break;
    if (!fofix_note_in_window(song_time, note_time, window)) {
      i = end - 1;
      continue;
    }

    const uint32_t required = group_mask(i, end);
    const int gem_count = group_gem_count(i, end);
    if (gem_count <= 0 || required == 0) {
      i = end - 1;
      continue;
    }
    if (strummed && !overstrum_candidate_seen) {
      overstrum_candidate_seen = true;
      overstrum_candidate_start = i;
      overstrum_candidate_end = end;
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
    if (!strummed && !can_hit) {
      break;
    }
    if (can_hit) {
      if (strummed) {
        for (size_t skipped = next_note_; skipped < i;) {
          if (skipped < consumed_.size() && consumed_[skipped]) {
            ++skipped;
            continue;
          }
          const size_t skipped_end = group_end(skipped);
          apply_skip(skipped, skipped_end);
          skipped = skipped_end;
        }
      }
      apply_hit(i, end, song_time);
      hit_this_frame = true;
      break;
    }
    i = end - 1;
  }

  while (next_note_ < notes_.size() && next_note_ < consumed_.size() &&
         consumed_[next_note_]) {
    ++next_note_;
  }
  if (next_note_ >= notes_.size()) finish_star_phrase();

  if (strummed && !hit_this_frame) {
    if (overstrum_candidate_seen &&
        group_star_power(overstrum_candidate_start, overstrum_candidate_end)) {
      observe_star_phrase(overstrum_candidate_start, overstrum_candidate_end,
                          false);
    }
    apply_overstrum(held_frets);
  }
  prev_fret_mask_ = fret_mask;
}

}  // namespace ghogx::game
