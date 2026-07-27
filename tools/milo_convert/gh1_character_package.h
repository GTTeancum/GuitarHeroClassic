#pragma once

#include "acg.h"
#include "acp.h"
#include "gh1_animation_manifest.h"
#include "milo.h"

#include <optional>
#include <string>
#include <vector>

namespace gh::milo_convert {

enum class Gh2ClipSetRole {
    GuitarMain,
    GuitarUi,
    GuitarFret,
    GuitarStrum,
    Band,
    Crowd,
    Generic,
};

struct Gh1ClipSetBuildInput {
    Gh1ClipSetSpec spec;
    gh::milo::Directory archetype;
    // Same authored order as spec.animations.
    std::vector<gh::acp::File> clips;
    std::optional<gh::acg::Graph> graph;
};

struct Gh2ClipSetPackage {
    Gh2ClipSetRole role = Gh2ClipSetRole::Generic;
    std::string directory_name;
    gh::milo::Directory directory;
    std::vector<std::string> source_clips;
};

const char* gh2_clip_set_role_name(Gh2ClipSetRole role);

// Compiles one GH1 clip-set manifest record plus its exact packed ACP/ACG and
// archetype data into one or more self-contained GH2 revision-24 CharClipSet
// directories. A GH1 hand bundle becomes separate fret and strum packages.
std::vector<Gh2ClipSetPackage>
convert_gh1_clip_set_to_gh2_packages(
    const Gh1ClipSetBuildInput& input);

}  // namespace gh::milo_convert
