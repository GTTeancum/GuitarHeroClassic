// engine/src/chart/midi_reader.cpp
//
// parse_midi implementation.

#include "chart/midi_reader.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

namespace ghogx::chart {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

// Read a big-endian unsigned integer of `n` bytes (1..4) from `p`.
// Returns 0 if n==0.
inline uint32_t read_be(const uint8_t* p, int n) {
    uint32_t v = 0;
    for (int i = 0; i < n; ++i)
        v = (v << 8) | p[i];
    return v;
}

// Read a MIDI variable-length quantity. Advances *pos past the VLQ.
// Returns 0 and does not advance if pos >= end.
uint32_t read_vlq(const uint8_t* data, size_t end, size_t* pos) {
    uint32_t v = 0;
    for (int i = 0; i < 4 && *pos < end; ++i) {
        const uint8_t b = data[(*pos)++];
        v = (v << 7) | (b & 0x7F);
        if ((b & 0x80) == 0) return v;
    }
    return v;  // malformed but continue
}

// GH2 MIDI note → difficulty index + lane.
// Returns false if this pitch is not a guitar gem note.
//
// Confirmed from PART GUITAR hexdump: GH2 PS2 uses the standard Harmonix gem
// pitch ranges (Easy 60-64, Medium 72-76, Hard 84-88, Expert 96-100), with
// Green=base+0, Red=+1, Yellow=+2, Blue=+3, Orange=+4. Notes appear on various
// MIDI channels (channel is ignored). Note-off is a velocity-0 note-on.
bool decode_gem(int pitch, int& diff_out, int& lane_out) {
    // Easy 60-64, Medium 72-76, Hard 84-88, Expert 96-100
    static const int base[4] = {60, 72, 84, 96};
    for (int d = 0; d < 4; ++d) {
        if (pitch >= base[d] && pitch < base[d] + 5) {
            diff_out = d;
            lane_out = pitch - base[d];
            return true;
        }
    }
    return false;
}


// Internal per-note scratch record during parse.
struct RawNote {
    uint32_t tick_on;
    uint32_t tick_off;
    int diff;
    int lane;
};

struct TrackNoteOn {
    uint32_t tick;
    int pitch;
    int velocity;
};

// Star power region.
struct SPRegion {
    uint32_t tick_on;
    uint32_t tick_off;
};

std::string clean_gh2_text_event(const uint8_t* data, uint32_t len) {
    std::string s(reinterpret_cast<const char*>(data), len);
    const size_t bracket = s.find('[');
    if (bracket != std::string::npos && bracket > 0) s.erase(0, bracket);
    return s;
}

// Parse one MTrk chunk. Fills raw_notes and sp_regions with new events.
// tempo_map receives any meta tempo events.
// name_out is set from the track-name meta event.
// track_idx is for diagnostic logging only.
void parse_track(const uint8_t* data, size_t start, size_t end,
                 std::vector<TempoChange>& tempo_map,
                 std::vector<RawNote>& raw_notes,
                 std::vector<SPRegion>& sp_regions,
                 std::vector<TextEvent>& text_events,
                 std::vector<TrackNoteOn>& note_ons,
                 std::string& name_out) {
    size_t pos = start;
    uint32_t abs_tick = 0;
    uint8_t running_status = 0;

    // Note tracking: on note-on, push a tap note immediately. On note-off,
    // extend the most recent note's tick_off (for sustains). active_gem_idx
    // tracks the raw_notes index of the most recent note-on per diff/lane.
    int      active_gem_idx[4][5];
    bool     active_gem_set[4][5] = {};
    uint32_t active_sp_tick = 0;
    bool     active_sp = false;
    const uint32_t kMinDur = 1;  // minimum tick duration for tap notes

    for (int d = 0; d < 4; ++d)
        for (int l = 0; l < 5; ++l)
            active_gem_idx[d][l] = -1;

    while (pos < end) {
        // Delta-time VLQ
        const uint32_t delta = read_vlq(data, end, &pos);
        abs_tick += delta;

        if (pos >= end) break;

        uint8_t status = data[pos];

        if (status == 0xFF) {
            // --- Meta event ---
            // NOTE: GH2 MIDIs rely on running status PERSISTING across meta
            // events (e.g. a note-on via running status immediately follows a
            // "[play]" text marker). So we do NOT clear running_status here.
            ++pos;
            if (pos + 1 > end) break;
            const uint8_t meta_type = data[pos++];
            const uint32_t meta_len = read_vlq(data, end, &pos);
            if (pos + meta_len > end) break;

            if (meta_type == 0x51 && meta_len == 3) {
                // Tempo change: 3 bytes, big-endian microseconds per beat.
                const uint32_t us = read_be(data + pos, 3);
                tempo_map.push_back({abs_tick, us});
            } else if (meta_type == 0x03) {
                // Track name.
                name_out = clean_gh2_text_event(data + pos, meta_len);
            } else if (meta_type == 0x01 || meta_type == 0x05 ||
                       meta_type == 0x06 || meta_type == 0x07) {
                std::string text = clean_gh2_text_event(data + pos, meta_len);
                if (!text.empty()) text_events.push_back({abs_tick, std::move(text)});
            }
            pos += meta_len;
        } else if (status == 0xF0 || status == 0xF7) {
            // --- SysEx --- (running status also persists, matching meta handling)
            ++pos;
            const uint32_t slen = read_vlq(data, end, &pos);
            pos += slen;
        } else {
            // --- MIDI channel event ---
            uint8_t cmd;
            if (status & 0x80) {
                cmd = status & 0xF0;
                running_status = status;
                ++pos;
            } else {
                // Data byte → running status event.
                cmd = running_status & 0xF0;
            }

            // Determine parameter count for this command type.
            int param_count = 0;
            switch (cmd) {
                case 0x80: case 0x90: case 0xA0: case 0xB0: case 0xE0:
                    param_count = 2; break;
                case 0xC0: case 0xD0:
                    param_count = 1; break;
                default:
                    // No valid running status — desync. Skip 1 byte to recover.
                    if (pos < end) ++pos;
                    continue;
            }
            if (pos + static_cast<size_t>(param_count) > end) break;
            const uint8_t p1 = data[pos];
            const uint8_t p2 = (param_count > 1) ? data[pos + 1] : 0;
            pos += static_cast<size_t>(param_count);

            // We only care about note-on (0x90) and note-off (0x80).
            // Note-on with velocity 0 is treated as note-off.
            bool is_on  = (cmd == 0x90) && (p2 > 0);
            bool is_off = (cmd == 0x80) || (cmd == 0x90 && p2 == 0);

            const int pitch = static_cast<int>(p1);

            if (is_on) {
                note_ons.push_back(
                    {abs_tick, pitch, static_cast<int>(p2)});
            }

            // Star power: MIDI note 116.
            if (pitch == 116) {
                if (is_on) {
                    active_sp_tick = abs_tick;
                    active_sp = true;
                } else if (is_off && active_sp) {
                    sp_regions.push_back({active_sp_tick, abs_tick});
                    active_sp = false;
                }
            }

            // Guitar gem notes.
            int diff = -1, lane = -1;
            if (decode_gem(pitch, diff, lane)) {
                if (is_on) {
                    // Push a tap note immediately (tick_off = tick_on + 1).
                    // If a note-off follows, it will extend tick_off to the real end.
                    raw_notes.push_back({abs_tick, abs_tick + kMinDur, diff, lane});
                    active_gem_idx[diff][lane] = static_cast<int>(raw_notes.size()) - 1;
                    active_gem_set[diff][lane] = true;
                } else if (is_off && active_gem_set[diff][lane]) {
                    // Extend the most recent note to the real end (sustain).
                    int idx = active_gem_idx[diff][lane];
                    if (idx >= 0 && idx < static_cast<int>(raw_notes.size()))
                        raw_notes[static_cast<size_t>(idx)].tick_off = abs_tick;
                    active_gem_set[diff][lane] = false;
                }
            }
        }
    }

    // Sustains still open at end of track: cap tick_off to abs_tick.
    for (int d = 0; d < 4; ++d)
        for (int l = 0; l < 5; ++l)
            if (active_gem_set[d][l] && active_gem_idx[d][l] >= 0) {
                auto idx = static_cast<size_t>(active_gem_idx[d][l]);
                if (idx < raw_notes.size() && raw_notes[idx].tick_off < abs_tick)
                    raw_notes[idx].tick_off = abs_tick;
            }
}

}  // namespace

// ---------------------------------------------------------------------------
// Chart::tick_to_sec
// ---------------------------------------------------------------------------

double Chart::tick_to_sec(uint32_t tick) const {
    if (tempo_map.empty() || ticks_per_beat == 0) return 0.0;
    double sec = 0.0;
    uint32_t cur_tick = 0;
    uint32_t us_per_beat = 500000;  // default 120 BPM
    for (const auto& tc : tempo_map) {
        if (tc.tick >= tick) break;
        const uint32_t end_tick = (tc.tick > tick) ? tick : tc.tick;
        sec += static_cast<double>(end_tick - cur_tick) /
               static_cast<double>(ticks_per_beat) *
               (static_cast<double>(us_per_beat) * 1e-6);
        cur_tick = tc.tick;
        us_per_beat = tc.us_per_beat;
    }
    // Remaining ticks from last tempo event to `tick`.
    sec += static_cast<double>(tick - cur_tick) /
           static_cast<double>(ticks_per_beat) *
           (static_cast<double>(us_per_beat) * 1e-6);
    return sec;
}

uint32_t Chart::sec_to_tick(double sec) const {
    if (tempo_map.empty() || ticks_per_beat == 0 || sec <= 0.0) return 0;
    double cur_sec = 0.0;
    uint32_t cur_tick = 0;
    uint32_t us_per_beat = 500000;
    for (const auto& tc : tempo_map) {
        if (tc.tick <= cur_tick) {
            us_per_beat = tc.us_per_beat;
            continue;
        }
        const double span_sec =
            static_cast<double>(tc.tick - cur_tick) /
            static_cast<double>(ticks_per_beat) *
            (static_cast<double>(us_per_beat) * 1e-6);
        if (cur_sec + span_sec >= sec) {
            const double beat_sec = static_cast<double>(us_per_beat) * 1e-6;
            const double ticks =
                (sec - cur_sec) / beat_sec * static_cast<double>(ticks_per_beat);
            return cur_tick + static_cast<uint32_t>(std::max(0.0, ticks));
        }
        cur_sec += span_sec;
        cur_tick = tc.tick;
        us_per_beat = tc.us_per_beat;
    }
    const double beat_sec = static_cast<double>(us_per_beat) * 1e-6;
    const double ticks =
        (sec - cur_sec) / beat_sec * static_cast<double>(ticks_per_beat);
    return cur_tick + static_cast<uint32_t>(std::max(0.0, ticks));
}

double Chart::duration_sec() const {
    uint32_t last = 0;
    for (int d = 0; d < 4; ++d)
        for (const auto& n : notes[d])
            last = std::max(last, n.tick_off);
    for (int d = 0; d < 4; ++d)
        for (const auto& n : bass_notes[d])
            last = std::max(last, n.tick_off);
    return tick_to_sec(last);
}

// ---------------------------------------------------------------------------
// parse_midi
// ---------------------------------------------------------------------------

Chart parse_midi(const std::vector<uint8_t>& bytes) {
    Chart chart;
    // Seed with the MIDI default 120 BPM; overridden by any 0xFF 0x51 events.
    chart.tempo_map.push_back({0, 500000});

    const size_t total = bytes.size();
    const uint8_t* data = bytes.data();

    if (total < 14) {
        std::fprintf(stderr, "[midi] too small (%zu bytes)\n", total);
        return chart;
    }

    // MThd header.
    if (std::memcmp(data, "MThd", 4) != 0) {
        std::fprintf(stderr, "[midi] missing MThd\n");
        return chart;
    }
    const uint32_t hdr_len = read_be(data + 4, 4);
    if (hdr_len < 6 || 8 + hdr_len > total) {
        std::fprintf(stderr, "[midi] bad MThd length %u\n", hdr_len);
        return chart;
    }
    const int smf_format = static_cast<int>(read_be(data + 8, 2));
    const int n_tracks   = static_cast<int>(read_be(data + 10, 2));
    const uint16_t division = static_cast<uint16_t>(read_be(data + 12, 2));
    if (division & 0x8000) {
        std::fprintf(stderr, "[midi] SMPTE timecode not supported\n");
        return chart;
    }
    chart.ticks_per_beat = division;

    std::fprintf(stderr, "[midi] format=%d tracks=%d tpb=%u file=%zu bytes\n",
                 smf_format, n_tracks, chart.ticks_per_beat, total);

    // Collect all track data.
    struct TrackData {
        size_t start;
        size_t end;
        std::string name;
    };
    std::vector<TrackData> tracks;
    tracks.reserve(static_cast<size_t>(n_tracks));

    size_t pos = 8 + hdr_len;
    while (pos + 8 <= total && static_cast<int>(tracks.size()) < n_tracks) {
        if (std::memcmp(data + pos, "MTrk", 4) != 0) {
            std::fprintf(stderr, "[midi] expected MTrk at offset %zu\n", pos);
            break;
        }
        const uint32_t tlen = read_be(data + pos + 4, 4);
        pos += 8;
        if (pos + tlen > total) {
            std::fprintf(stderr, "[midi] MTrk overflows file at offset %zu\n", pos);
            break;
        }
        tracks.push_back({pos, pos + tlen, {}});
        pos += tlen;
    }

    // First pass: parse all tracks to get names (so we can pick the guitar track).
    // Tempo events from any track (especially track 0) are also collected.
    std::vector<std::vector<RawNote>> all_raw(tracks.size());
    std::vector<std::vector<SPRegion>> all_sp(tracks.size());
    std::vector<std::vector<TextEvent>> all_text(tracks.size());
    std::vector<std::vector<TrackNoteOn>> all_note_ons(tracks.size());

    // Extra tempo events collected across all tracks (merged after).
    std::vector<TempoChange> extra_tempos;

    for (size_t t = 0; t < tracks.size(); ++t) {
        std::vector<TempoChange> local_tempos;
        parse_track(data, tracks[t].start, tracks[t].end,
                    local_tempos, all_raw[t], all_sp[t], all_text[t],
                    all_note_ons[t], tracks[t].name);
        // Merge tempos from all tracks.
        for (auto& tc : local_tempos)
            extra_tempos.push_back(tc);
    }

    // Merge all tempo events, sorted by tick, deduplicated.
    // Start from the default 120-BPM seed already in chart.tempo_map.
    for (auto& tc : extra_tempos)
        chart.tempo_map.push_back(tc);
    std::sort(chart.tempo_map.begin(), chart.tempo_map.end(),
              [](const TempoChange& a, const TempoChange& b) {
                  return a.tick < b.tick;
              });
    // Keep only the last tempo event at each tick.
    {
        std::vector<TempoChange> dedup;
        for (auto& tc : chart.tempo_map) {
            if (!dedup.empty() && dedup.back().tick == tc.tick)
                dedup.back() = tc;
            else
                dedup.push_back(tc);
        }
        chart.tempo_map = std::move(dedup);
    }

    // GH2 venue/world events live on the EVENTS track. Performer tracks also
    // contain text markers, but those drive character state and must not select
    // world lighting presets.
    std::vector<size_t> event_tracks;
    for (size_t t = 0; t < tracks.size(); ++t) {
        if (tracks[t].name == "EVENTS") event_tracks.push_back(t);
    }
    if (event_tracks.empty() && !tracks.empty()) {
        event_tracks.push_back(0);
        std::fprintf(stderr, "[midi] no EVENTS track found; using track 0 text events\n");
    }
    for (size_t t : event_tracks) {
        for (auto& ev : all_text[t]) chart.text_events.push_back(std::move(ev));
        std::fprintf(stderr, "[midi] using event track %zu '%s'\n",
                     t, tracks[t].name.c_str());
    }

    for (size_t t = 0; t < tracks.size(); ++t) {
        if (tracks[t].name == "EVENTS" || tracks[t].name.empty()) continue;
        for (const auto& ev : all_text[t]) {
            chart.performer_events.push_back({ev.tick, tracks[t].name, ev.text});
        }
        if (tracks[t].name == "BAND DRUMS") {
            for (const auto& note : all_note_ons[t]) {
                const char* event = nullptr;
                // config/midi_parsers.dta::drummer_kick_drum maps stock GH2
                // PS2 BAND DRUMS pitch 36 to kick_drum and 37 to crash_symbal.
                // hit_snare/hit_hihat are real drummer messages, but traces
                // show them through script/parser rows rather than these two
                // MIDI pitches.
                if (note.pitch == 36) event = "kick_drum";
                if (note.pitch == 37) event = "crash_symbal";
                if (!event) continue;
                chart.drum_cues.push_back(
                    {note.tick, note.pitch, std::string(event)});
            }
        } else if (tracks[t].name == "BAND BASS") {
            for (const auto& note : all_note_ons[t]) {
                // config/midi_parsers.dta::speaker_pulse maps BAND BASS
                // pitch 36 to {handle (world bass_hit)}.
                if (note.pitch != 36) continue;
                chart.bass_cues.push_back(
                    {note.tick, note.pitch, std::string("bass_hit")});
            }
        }
    }

    // (Diagnostic pitch scan removed after format discovery.)
    std::sort(chart.text_events.begin(), chart.text_events.end(),
              [](const TextEvent& a, const TextEvent& b) {
                  return a.tick < b.tick;
              });
    std::sort(chart.performer_events.begin(), chart.performer_events.end(),
              [](const TrackTextEvent& a, const TrackTextEvent& b) {
                  if (a.tick != b.tick) return a.tick < b.tick;
                  return a.track < b.track;
              });
    std::sort(chart.drum_cues.begin(), chart.drum_cues.end(),
              [](const DrumCue& a, const DrumCue& b) {
                  if (a.tick != b.tick) return a.tick < b.tick;
                  return a.pitch < b.pitch;
              });
    std::sort(chart.bass_cues.begin(), chart.bass_cues.end(),
              [](const DrumCue& a, const DrumCue& b) {
                  if (a.tick != b.tick) return a.tick < b.tick;
                  return a.pitch < b.pitch;
              });
    std::fprintf(stderr, "[midi] text events=%zu\n", chart.text_events.size());
    std::fprintf(stderr, "[midi] performer text events=%zu\n",
                 chart.performer_events.size());
    std::fprintf(stderr, "[midi] drum cues=%zu\n", chart.drum_cues.size());
    std::fprintf(stderr, "[midi] bass cues=%zu\n", chart.bass_cues.size());

    auto append_chart_notes = [&](const std::vector<RawNote>& src_notes,
                                  const std::vector<SPRegion>& src_sp,
                                  std::vector<Note> (&dst)[4]) {
        std::vector<RawNote> sorted_notes = src_notes;
        std::vector<SPRegion> sorted_sp = src_sp;
        std::sort(sorted_notes.begin(), sorted_notes.end(),
                  [](const RawNote& a, const RawNote& b) {
                      if (a.diff != b.diff) return a.diff < b.diff;
                      if (a.tick_on != b.tick_on) return a.tick_on < b.tick_on;
                      return a.lane < b.lane;
                  });
        std::sort(sorted_sp.begin(), sorted_sp.end(),
                  [](const SPRegion& a, const SPRegion& b) {
                      return a.tick_on < b.tick_on;
                  });

        const uint32_t hopo_thresh = chart.ticks_per_beat / 3;
        for (int d = 0; d < 4; ++d) {
            uint32_t prev_tick_on = 0;
            int prev_lane = -1;
            bool first = true;
            for (const auto& rn : sorted_notes) {
                if (rn.diff != d) continue;
                Note n;
                n.tick_on = rn.tick_on;
                n.tick_off = rn.tick_off;
                n.lane = rn.lane;
                n.is_hopo = false;
                n.star_power = false;

                if (!first && rn.lane != prev_lane) {
                    const uint32_t gap = (rn.tick_on >= prev_tick_on)
                                             ? (rn.tick_on - prev_tick_on)
                                             : 0;
                    n.is_hopo = (gap < hopo_thresh && gap > 0);
                }

                for (const auto& sp : sorted_sp) {
                    if (sp.tick_off <= rn.tick_on) continue;
                    if (sp.tick_on >= rn.tick_off) break;
                    n.star_power = true;
                    break;
                }

                dst[d].push_back(n);
                prev_tick_on = rn.tick_on;
                prev_lane = rn.lane;
                first = false;
            }
        }
    };

    // Select the guitar track(s) to use for notes.
    // For SMF type 0: all data is in track 0.
    // For SMF type 1: prefer "PART GUITAR", then "T1 GEMS", then track index 1.
    std::vector<size_t> guitar_tracks;
    if (smf_format == 0) {
        guitar_tracks.push_back(0);
    } else {
        // Search by name.
        for (size_t t = 0; t < tracks.size(); ++t) {
            if (tracks[t].name == "PART GUITAR" ||
                tracks[t].name == "T1 GEMS") {
                guitar_tracks.push_back(t);
            }
        }
        // Fallback to track 1 (track 0 is typically the tempo/sync track in type 1).
        if (guitar_tracks.empty() && tracks.size() > 1) {
            guitar_tracks.push_back(1);
            std::fprintf(stderr, "[midi] no named guitar track found; using track 1\n");
        }
        for (size_t t : guitar_tracks)
            std::fprintf(stderr, "[midi] using guitar track %zu '%s'\n",
                         t, tracks[t].name.c_str());
    }

    // Merge raw notes and SP regions from the selected tracks.
    std::vector<RawNote> raw_notes;
    std::vector<SPRegion> sp_regions;
    for (size_t t : guitar_tracks) {
        for (auto& n : all_raw[t]) raw_notes.push_back(n);
        for (auto& s : all_sp[t])  sp_regions.push_back(s);
    }

    append_chart_notes(raw_notes, sp_regions, chart.notes);

    if (smf_format != 0) {
        std::vector<RawNote> raw_bass_notes;
        std::vector<SPRegion> bass_sp_regions;
        for (size_t t = 0; t < tracks.size(); ++t) {
            if (tracks[t].name != "PART RHYTHM" &&
                tracks[t].name != "PART BASS") {
                continue;
            }
            std::fprintf(stderr, "[midi] using bass track %zu '%s'\n",
                         t, tracks[t].name.c_str());
            for (auto& n : all_raw[t]) raw_bass_notes.push_back(n);
            for (auto& s : all_sp[t]) bass_sp_regions.push_back(s);
        }
        append_chart_notes(raw_bass_notes, bass_sp_regions, chart.bass_notes);
    }

    std::fprintf(stderr, "[midi] parsed: Easy=%zu Med=%zu Hard=%zu Expert=%zu BassMed=%zu dur=%.1fs\n",
                 chart.notes[0].size(), chart.notes[1].size(),
                 chart.notes[2].size(), chart.notes[3].size(),
                 chart.bass_notes[1].size(),
                 chart.duration_sec());
    return chart;
}

}  // namespace ghogx::chart
