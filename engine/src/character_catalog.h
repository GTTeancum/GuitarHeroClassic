// character_catalog.h - Derive character outfits + guitar models from songs.
//
// PS2 GH titles reference character outfits and guitars by symbol in each
// song's (quickplay (character_outfit X) (guitar Y) (venue Z)) block. The
// full character system is more complex (skeletal data, body parts, anim
// sets live in milos), but for a catalog projection the set of referenced
// outfits + guitars is the right surface.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ghogx::catalog {

struct Song;  // catalog.h

struct CharacterAggregate {
    std::vector<std::pair<std::string, int>> outfits;   // (outfit_sym, song_count)
    std::vector<std::pair<std::string, int>> guitars;   // (guitar_sym, song_count)
    std::vector<std::pair<std::string, int>> venues;    // (venue_sym,  song_count)
};

// Reduce a list of songs to their distinct quickplay slot values, with a
// usage count per value (how many songs default to that outfit / guitar /
// venue). Sorted by descending count.
CharacterAggregate aggregate_from_songs(const std::vector<Song>& songs);

}  // namespace ghogx::catalog
