// venue_catalog.h - Enumerate venues from a Harmonix ARK.
//
// Venues in PS2 GH live under world/<shortname>/gen/<shortname>.dtb. Each
// such DTB is a (WorldDir (types (<shortname> WORLD_OBJECT_BASE ...))) tree
// with sound bank assignment, crowd reaction levels, camera shots, etc.
// This module discovers venues by their DTB path pattern and pulls the
// shortname plus the most useful summary bits (sound bank, crowd level
// count).

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace gh::ark { class ArkV3Reader; }

namespace ghogx::catalog {

struct Venue {
    std::string shortname;        // e.g. "small2", "arena", "battle"
    std::string dtb_path;         // path inside ARK (for diagnostics)
    std::optional<std::string> sound_bank;
    int crowd_levels = 0;         // count of (levels ...) entries
};

// Discover and return all venues in the given ARK. Each venue is parsed
// from its world/<x>/gen/<x>.dtb file via our DTB reader. Entries that
// don't decrypt or don't match the expected shape are skipped.
std::vector<Venue> extract_venues(const gh::ark::ArkV3Reader& ark,
                                  const std::string& ark_file_path);

}  // namespace ghogx::catalog
