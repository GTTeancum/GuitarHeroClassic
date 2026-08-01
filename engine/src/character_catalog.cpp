// character_catalog.cpp - see character_catalog.h.

#include "character_catalog.h"
#include "catalog.h"

#include <algorithm>
#include <map>

namespace ghogx::catalog {

namespace {

std::vector<std::pair<std::string, int>>
to_sorted(const std::map<std::string, int>& m) {
    std::vector<std::pair<std::string, int>> v(m.begin(), m.end());
    std::sort(v.begin(), v.end(),
              [](const auto& a, const auto& b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;
              });
    return v;
}

}  // anonymous namespace

CharacterAggregate aggregate_from_songs(const std::vector<Song>& songs) {
    std::map<std::string, int> outfit_count, guitar_count, venue_count;
    for (const auto& s : songs) {
        if (!s.quickplay) continue;
        if (!s.quickplay->character_outfit.empty())
            outfit_count[s.quickplay->character_outfit]++;
        if (!s.quickplay->guitar.empty())
            guitar_count[s.quickplay->guitar]++;
        if (!s.quickplay->venue.empty())
            venue_count[s.quickplay->venue]++;
    }
    CharacterAggregate agg;
    agg.outfits = to_sorted(outfit_count);
    agg.guitars = to_sorted(guitar_count);
    agg.venues  = to_sorted(venue_count);
    return agg;
}

}  // namespace ghogx::catalog
