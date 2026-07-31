#pragma once

#include "gh1_animation_manifest.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace gh::milo_convert {

enum class Gh1CharacterControllerKind {
    ForeTwist,
    UpperTwist,
    HandIk,
    ServoBone,
    Driver,
    Rig,
    PosConstraint,
};

enum class Gh1CharacterRole {
    Guitarist,
    Singer,
    Bassist,
    Drummer,
    Keyboardist,
};

struct Gh1WeightedBoneSpec {
    std::string bone;
    float weight = 0.0f;
};

struct Gh1CharacterControllerSpec {
    Gh1CharacterControllerKind kind =
        Gh1CharacterControllerKind::UpperTwist;
    std::string class_name;
    std::string name;
    std::vector<std::string> bones;
    std::string source;
    std::string destination;
    std::vector<std::string> sources;
    std::vector<std::string> destinations;
    std::vector<std::string> channels;
    std::vector<Gh1WeightedBoneSpec> rig_bones;
    std::string side_axis;
    std::string connected_driver;
    float offset = 0.0f;
    int32_t bend = 0;
    bool align_quaternion = false;
    bool stretch = false;
    bool vertical = false;
};

struct Gh1CharacterEyesSpec {
    bool present = false;
    std::string parent;
    float constraint = 0.0f;
    float lower_lid = 0.0f;
};

struct Gh1CharacterWalkSpec {
    bool present = false;
    uint32_t turn_flags = 0;
    uint32_t stop_flags = 0;
    uint32_t walk_flags = 0;
};

struct Gh1FaceEventSpec {
    int32_t event = 0;
    std::string pose;
};

struct Gh1FaceExcitementSpec {
    int32_t state = 0;
    std::vector<std::string> poses;
};

struct Gh1CharacterFaceSpec {
    bool present = false;
    std::vector<std::string> poses;
    std::vector<Gh1FaceExcitementSpec> excitement_poses;
    float blend_time = 0.0f;
    bool has_pose_length = false;
    float pose_length = 0.0f;
    std::string event_list;
    bool has_event_offset = false;
    int32_t event_offset = 0;
    std::vector<Gh1FaceEventSpec> events;
};

struct Gh1CharacterSpec {
    std::string authored_name;
    std::string package_name;
    bool band_character = false;
    Gh1CharacterRole role = Gh1CharacterRole::Guitarist;
    std::string source_directory;
    std::string source_skeleton;
    std::string compiled_skeleton;
    std::vector<float> lod_screen_sizes;
    std::array<float, 4> sphere{};
    std::string sphere_base;
    bool use_delta = false;
    std::vector<std::string> servo_channels;
    bool main_driver_realign = false;
    std::vector<Gh1CharacterControllerSpec> controllers;
    Gh1CharacterEyesSpec eyes;
    Gh1CharacterWalkSpec walk;
    Gh1CharacterFaceSpec face;
    std::string face_file;
    std::string shadow_file;
};

struct Gh1CharacterManifest {
    std::vector<Gh1CharacterSpec> characters;
};

struct Gh1CharacterTrackSurfaceSpec {
    std::string authored_name;
    std::string source_surface;
};

// Compiles the authored GH1 character system, including recursive includes,
// merges, and macro expansions, into the facts needed by the native package
// writer. All paths are archive paths supplied by `reader`.
Gh1CharacterManifest compile_gh1_character_manifest(
    const std::string& dtb_path,
    const std::vector<uint8_t>& dtb_bytes,
    const VirtualAssetReader& reader);

// Recovers the per-character highway bitmap references authored in
// config/gen/characters.dtb. Destination naming is intentionally left to the
// GH2 bundle writer so this parser remains a source-format contract.
std::vector<Gh1CharacterTrackSurfaceSpec>
compile_gh1_character_track_surfaces(
    const std::vector<uint8_t>& dtb_bytes);

std::string gh1_character_manifest_tsv(
    const Gh1CharacterManifest& manifest);

}  // namespace gh::milo_convert
