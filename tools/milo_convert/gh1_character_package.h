#pragma once

#include "acg.h"
#include "acp.h"
#include "gh1_animation_manifest.h"
#include "milo.h"

#include <cstdint>
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

// GH1 stores guitarist animation-family membership in the same source flag
// word as tempo, walk direction/style, and IK facts. GH2 stores family
// membership in CharClipGroup objects instead. Remove only the recovered GH1
// family bits when emitting a GH2 guitarist CharClip; all shared semantic bits
// remain intact.
uint32_t convert_gh1_guitar_clip_flags_to_gh2(uint32_t source_flags);

// GH1 band sets have no separate venue-intro animation family. GH2's stock
// BAND_COMMON startup requests kBandIntro followed by kBandIntroIdle before
// normal play/idle messages begin. GH2 also adds kBandNosnare to the drummer
// mode domain, whereas a GH1 drummer domain has only normal/allbeat/double/
// half. Promote source idle clips into the two target intro domains and, only
// for a structurally identified drummer domain, let its normal active motion
// satisfy GH2's otherwise-unrepresented no-snare mode.
uint32_t convert_gh1_band_clip_flags_to_gh2(
    uint32_t source_flags, bool source_has_drum_modes);

// GH1's packed hand_events/hand_strum_mapping contract names one source clip
// for each open, short, long, and downward-pluck semantic. GH2's
// midi_parsers.dtb addresses repeated children in three target families.
// Return the target-family aliases that must share the converted source clip.
std::vector<std::string> gh2_strum_clip_aliases_for_gh1_source(
    const std::string& source_clip);

// Compiles one GH1 clip-set manifest record plus its exact packed ACP/ACG and
// archetype data into one or more self-contained GH2 revision-24 CharClipSet
// directories. A GH1 hand bundle becomes separate fret and strum packages.
std::vector<Gh2ClipSetPackage>
convert_gh1_clip_set_to_gh2_packages(
    const Gh1ClipSetBuildInput& input);

}  // namespace gh::milo_convert
