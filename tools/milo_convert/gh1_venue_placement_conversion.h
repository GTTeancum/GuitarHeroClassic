#pragma once

#include "milo.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace gh::milo_convert {

struct Gh2VenuePlacementRecord {
    std::string role;
    std::string source_helper;
    std::string target_waypoint;
    uint32_t flags = 0;
    std::array<float, 12> source_world{};
    std::array<float, 12> target_transform{};
};

struct Gh2VenuePlacementConversion {
    gh::milo::Directory characters_directory;
    size_t waypoints = 0;
    std::vector<Gh2VenuePlacementRecord> records;
};

// GH1 Arena builds its stage/walk spot records from the numbered helper
// meshes loaded across the venue sections. Translate those authored records
// into the native GH2 Waypoint start contract used by every character layout.
Gh2VenuePlacementConversion
convert_gh1_venue_spots_to_gh2_waypoints(
    const std::string& target_venue,
    const std::vector<gh::milo::Directory>& converted_sections);

void link_gh2_venue_characters_directory(
    gh::milo::Directory& main_directory,
    const std::string& target_venue);

// Promote the converted GH1 main RndDir to the retail GH2 WorldDir11 root
// contract and link each authored Arena load_section as a native ObjectDir
// subdirectory. Section filenames remain independent native RndDirs.
void finalize_gh2_venue_world_directory(
    gh::milo::Directory& main_directory,
    const std::string& target_venue,
    const std::vector<std::pair<std::string, std::string>>&
        loaded_sections,
    bool has_start_handler);

}  // namespace gh::milo_convert
