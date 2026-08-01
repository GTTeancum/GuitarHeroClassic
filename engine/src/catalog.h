// catalog.h - Song catalog extraction from a Harmonix DTB tree.
//
// The catalog DTB (e.g. GH80s' config/gen/songs.dtb) is a tree of top-level
// arrays, one per song. Each song array starts with a bareword shortname
// (e.g. `18andlife`) and has keyed sub-arrays for its display info, audio
// stem layout, MIDI chart path, default quickplay rig, and per-difficulty
// practice mixes.
//
// This module walks that tree and projects it into plain C++ structs the
// engine can consume without touching the DTB types directly.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace gh::dtb { struct Tree; }

namespace ghogx::catalog {

struct Quickplay {
    std::string character_outfit;   // e.g. "glam1"
    std::string guitar;             // e.g. "flying_v"
    std::string venue;              // e.g. "arena"
};

struct Song {
    std::string shortname;          // first sym of the top-level array
    std::string display_name;       // (name "...")
    std::string artist;             // (artist "...")
    std::string midi_path;          // (song (midi_file ...))
    std::string master_audio_path;  // (song (name songs/<x>/<x>))  (no ext)
    std::string anim_tempo;         // (anim_tempo kTempoMedium/kTempoFast)
    std::optional<int32_t> preview_start_ms;
    std::optional<int32_t> preview_end_ms;
    std::optional<Quickplay> quickplay;
    std::vector<std::string> band;  // optional song-level singer/bass/drummer override
};

// Walk a parsed DTB tree and return one Song per top-level song array.
// Entries that don't match the expected shape are skipped silently; the
// engine's responsibility is to surface useful catalog data, not enforce
// shape.
std::vector<Song> extract_songs(const gh::dtb::Tree& tree);

}  // namespace ghogx::catalog
