// engine/src/chart/midi_reader.h
//
// parse_midi: read a Standard MIDI File (SMF type 0 or 1) and extract the
// Guitar Hero II note chart.
//
// GH2 MIDI note layout:
//   Easy   60-64  (60=Green 61=Red 62=Yellow 63=Blue 64=Orange)
//   Medium 72-76
//   Hard   84-88
//   Expert 96-100
//   Star power phrase: note 116 on/off on any track
//
// HOPO threshold: gap from previous note < ticks_per_beat / 3.
//
// The guitar part lives on the track named "PART GUITAR"; "T1 GEMS" is the
// GH1 / older Harmonix fallback. For SMF type 0 every event is in track 0.
// For SMF type 1 we search tracks by name, then fall back to track index 1.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ghogx::chart {

struct TempoChange {
    uint32_t tick;           // absolute MIDI tick (0-based)
    uint32_t us_per_beat;    // microseconds per beat (default = 500000 = 120 BPM)
};

struct Note {
    uint32_t tick_on;
    uint32_t tick_off;
    int      lane;           // 0=Green 1=Red 2=Yellow 3=Blue 4=Orange
    bool     is_hopo;        // true if gap from previous note < ticks_per_beat/3
    bool     star_power;     // true if this note falls inside a star power phrase
};

struct TextEvent {
    uint32_t tick;
    std::string text;
};

struct TrackTextEvent {
    uint32_t tick;
    std::string track;
    std::string text;
};

struct DrumCue {
    uint32_t tick;
    int pitch = 0;
    std::string event;
};

struct Chart {
    uint32_t ticks_per_beat = 480;
    std::vector<TempoChange> tempo_map;     // sorted by tick ascending
    std::vector<Note>        notes[4];      // [0]=Easy [1]=Medium [2]=Hard [3]=Expert
    std::vector<Note>        bass_notes[4]; // PART RHYTHM/PART BASS gems by difficulty
    std::vector<TextEvent>   text_events;   // GH2 world EVENTS track text
    std::vector<TrackTextEvent> performer_events;  // role-specific text tracks
    std::vector<DrumCue>     drum_cues;      // BAND DRUMS parser cue notes
    std::vector<DrumCue>     bass_cues;      // BAND BASS speaker_pulse cues

    // Convert a MIDI tick to wall-clock seconds using the tempo map.
    double tick_to_sec(uint32_t tick) const;
    uint32_t sec_to_tick(double sec) const;

    // Total song duration: time of the last note-off across all difficulties.
    double duration_sec() const;
};

// Parse a Standard MIDI File byte buffer extracted from the ARK.
// Returns an empty Chart (tempo_map has at least the default 120-BPM entry)
// and logs to stderr on any format error rather than throwing.
Chart parse_midi(const std::vector<uint8_t>& bytes);

}  // namespace ghogx::chart
