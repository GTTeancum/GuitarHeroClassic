// engine/src/character/char_facefx.h
//
// Minimal FaceFX FAC support for GH2 character viewing. This is intentionally
// narrow: it decodes authored FxBonePoseNode data enough to apply the neutral
// face pose referenced by FaceFxLipSyncServo::facefx_path.

#pragma once

#include "character/char_mesh.h"

#include <optional>
#include <cstddef>
#include <string>
#include <vector>

namespace ghogx::character {

struct FaceFxPoseBone {
  std::string name;
  float pos[3] = {};
  float quat_wxyz[4] = {1.0f, 0.0f, 0.0f, 0.0f};
};

struct FaceFxPose {
  std::string name;
  std::vector<FaceFxPoseBone> bones;
};

std::optional<FaceFxPose> load_facefx_pose(const std::string& hdr_path,
                                           const std::string& ark_path,
                                           const std::string& character_milo,
                                           const Character& character,
                                           const std::string& pose_name);
std::optional<std::size_t> load_facefx_pose_index(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& character_milo, const Character& character,
    const std::string& pose_name);

void apply_facefx_pose(const FaceFxPose& pose, float weight,
                       Character& character);
void apply_facefx_pose_delta(const FaceFxPose& base, const FaceFxPose& pose,
                             float weight, Character& character);

bool apply_facefx_neutral_pose(const std::string& hdr_path,
                               const std::string& ark_path,
                               const std::string& character_milo,
                               Character& character);

}  // namespace ghogx::character
