#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace gh::milo_convert {

using VirtualAssetReader =
    std::function<std::vector<uint8_t>(const std::string& path)>;

struct Gh1AnimationSpec {
    std::string name;
    uint32_t flags = 0;
    uint32_t play_flags = 0;
    float blend_width = 0.0f;
    std::vector<std::string> channels;
    std::vector<std::string> excluded_venues;
};

struct Gh1ClipSetSpec {
    std::string invocation;
    std::string qualified_name;
    std::string target_name;
    std::string source_directory;
    std::string archetype_rnd;
    std::string dependency_acg;
    uint32_t play_flags = 0;
    float blend_width = 0.0f;
    bool move_self = false;
    std::vector<std::string> channels;
    std::vector<std::string> recenter_channels;
    std::vector<std::string> recenter_bones;
    bool recenter_slide = false;
    std::vector<Gh1AnimationSpec> animations;
};

struct Gh1AnimationManifest {
    std::vector<Gh1ClipSetSpec> clip_sets;
};

// Compile a GH1 .acs plus its recursively included compiled DTBs. All paths
// are virtual archive paths and all bytes are supplied by `reader`.
Gh1AnimationManifest compile_gh1_animation_manifest(
    const std::string& acs_path,
    const std::vector<uint8_t>& acs_bytes,
    const VirtualAssetReader& reader);

std::string gh1_animation_manifest_tsv(
    const Gh1AnimationManifest& manifest);

std::string gh1_compiled_rnd_path(
    const std::string& authored_path);

}  // namespace gh::milo_convert
