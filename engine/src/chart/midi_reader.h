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
//   Fret-position animation: 40-59 (spot_neck_fret01..20)
//   Star power phrase: note 103 on/off on the guitar track
//
// HOPO threshold: FoFiX/GH2 default cutoff is 170 ticks at 480 PPQ.
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
    bool     is_hopo;        // true if rendered/playable as a tappable HOPO
    bool     star_power;     // true if this note falls inside a star power phrase
    int      hopo_tappable = 0; // FoFiX class: 0=strum, 1=start, 2=middle, 3=end
    bool     final_star = false; // final note/chord in a star power phrase
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

struct FretPositionCue {
    // Original MIDI note interval. The mapped spot is due at `tick`; the
    // inverted MidiParser event is dispatched earlier at `event_beat`.
    uint32_t tick;
    int pitch = 0;       // 40..59 from config/midi_parsers.dta::player*_fret_pos
    int spot_index = 0;  // 1..20, maps to spot_neck_fretNN.mesh
    uint32_t tick_off = 0;
    double event_beat = 0.0;
};

struct HandGemCue {
    uint32_t tick;
    uint32_t tick_off;
    uint32_t mask = 0;  // bits 0..4, from config/midi_parsers.dta::player*_fret
    double length = 0.0;
};

struct LightingCue {
    uint32_t tick;
    int pitch = 0;
    std::string event;
};

struct VenueCue {
    uint32_t tick;
    int pitch = 0;
    std::string event;
};

struct SingerFaceCue {
    uint32_t tick_on = 0;
    uint32_t tick_off = 0;
    int pitch = 0;
};

struct HandMapCue {
    uint32_t tick = 0;
    std::string map;
};

struct HandAnimationCue {
    uint32_t tick_on = 0;
    uint32_t tick_off = 0;
    int pitch = 0;
};

struct Chart {
    uint32_t ticks_per_beat = 480;
    bool gh1_anim_track = false;
    std::vector<TempoChange> tempo_map;     // sorted by tick ascending
    std::vector<Note>        notes[4];      // [0]=Easy [1]=Medium [2]=Hard [3]=Expert
    std::vector<Note>        bass_notes[4]; // PART RHYTHM/PART BASS gems by difficulty
    std::vector<FretPositionCue> fret_positions;      // PART GUITAR player*_fret_pos
    std::vector<FretPositionCue> bass_fret_positions; // PART BASS/RHYTHM player*_fret_pos
    std::vector<HandGemCue>  fret_hand_cues[4];       // PART GUITAR player*_fret
    std::vector<HandGemCue>  bass_fret_hand_cues[4];  // PART BASS/RHYTHM player*_fret
    std::vector<TextEvent>   text_events;   // GH2 world EVENTS track text
    std::vector<TrackTextEvent> performer_events;  // role-specific text tracks
    std::vector<DrumCue>     drum_cues;      // BAND DRUMS parser cue notes
    std::vector<DrumCue>     bass_cues;      // BAND BASS speaker_pulse cues
    std::vector<LightingCue> lighting_cues;  // TRIGGERS lighting_parser cues
    std::vector<VenueCue>    venue_cues;     // TRIGGERS effect_parser cues
    // GH1 singer EventList rows. Retail midi_parsers.dtb maps guitar-track
    // pitch 108 note spans to the "singer" list with payload "open".
    std::vector<SingerFaceCue> singer_face_cues;
    // GH1's ANIM parser owns these independently of the gameplay-gem track.
    // charsys.dtb maps ANIM text beginning HandMap_/StrumMap_ to persistent
    // mapper selection lists and maps pitches 40..59 to fret positions.
    std::vector<HandMapCue> hand_map_cues;
    std::vector<HandMapCue> strum_map_cues;
    // Other ANIM notes are looked up through charsys.dtb::hand_events. Retail
    // songs normally derive fingers/strums from gems, but Get Ready 2 Rokk
    // contains explicit pitch-60 finger_powerchord_1 events.
    std::vector<HandAnimationCue> hand_animation_cues;

    // Convert a MIDI tick to wall-clock seconds using the tempo map.
    double tick_to_sec(uint32_t tick) const;
    uint32_t sec_to_tick(double sec) const;
    double sec_to_beat(double sec) const;

    // Total song duration: time of the last note-off across all difficulties.
    double duration_sec() const;
};

// Parse a Standard MIDI File byte buffer extracted from the ARK.
// Returns an empty Chart (tempo_map has at least the default 120-BPM entry)
// and logs to stderr on any format error rather than throwing.
Chart parse_midi(const std::vector<uint8_t>& bytes);

}  // namespace ghogx::chart
