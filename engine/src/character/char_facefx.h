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
#include <unordered_map>
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

struct FaceFxGraphInput {
  std::string node;
  std::string link_function;
  float weight = 1.0f;
};

struct FaceFxGraphNode {
  std::string name;
  std::string class_name;
  float min_value = 0.0f;
  float max_value = 1.0f;
  float default_value = 0.0f;
  std::vector<FaceFxGraphInput> inputs;
  std::optional<std::size_t> pose_index;
};

struct FaceFxGraph {
  std::vector<FaceFxPose> poses;
  std::vector<FaceFxGraphNode> nodes;
};

struct FaceFxCurveKey {
  float time = 0.0f;
  float value = 0.0f;
};

struct FaceFxCurve {
  std::string name;
  std::vector<FaceFxCurveKey> keys;
};

struct FaceFxAnimation {
  std::string name;
  std::vector<FaceFxCurve> curves;
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
std::optional<FaceFxGraph> load_facefx_graph(const std::string& hdr_path,
                                             const std::string& ark_path,
                                             const std::string& character_milo,
                                             const Character& character);
std::optional<FaceFxAnimation> load_facefx_animation(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& voc_path);

float evaluate_facefx_node(
    const FaceFxGraph& graph, const std::string& node_name,
    const std::unordered_map<std::string, float>& registers);
std::unordered_map<std::string, float> sample_facefx_animation(
    const FaceFxAnimation& animation, float time);
bool apply_facefx_animation_frame(
    const FaceFxGraph& graph,
    const std::unordered_map<std::string, float>& registers,
    Character& character);
bool apply_facefx_pose_node_delta(
    const FaceFxGraph& graph, const std::string& base_pose_name,
    const std::string& pose_node_name,
    const std::unordered_map<std::string, float>& registers,
    Character& character);

void apply_facefx_pose(const FaceFxPose& pose, float weight,
                       Character& character);
void apply_facefx_pose_delta(const FaceFxPose& base, const FaceFxPose& pose,
                             float weight, Character& character);

bool apply_facefx_neutral_pose(const std::string& hdr_path,
                               const std::string& ark_path,
                               const std::string& character_milo,
                               Character& character);

}  // namespace ghogx::character
