// engine/src/character/char_facefx.h
//
// GH2 FaceFX FAC graph and authored CharClipSamples output support.

#pragma once

#include "character/char_clip.h"

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
  // Ordinal among FxBonePoseNode records. GH2's FaceFxLipSyncServo passes this
  // exact value to the "visemes" CharClip as its authored sample frame.
  std::optional<std::size_t> pose_ordinal;
  // Index into the decoded FAC transform table. Those transforms identify the
  // outputs but are not the authored expression shape used during gameplay.
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

struct FaceFxMaterializedFrame {
  std::vector<ClipChannel> channels;
  std::vector<CharClip::OutputBone> output_bones;
  float neutral_residual = 1.0f;
  std::size_t active_pose_count = 0;
  bool valid = false;
};

// Resolve the serialized FaceFxLipSyncServo viseme-MILO references relative
// to their owning character package. The returned order covers authored,
// compiled-suffix, gen-directory, and compiled gen-directory forms without
// synthesizing a character-name-based fallback.
std::vector<std::string> facefx_viseme_milo_candidates(
    const std::string& character_milo,
    const std::vector<FaceFxLipSyncServo>& servos);

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
std::unordered_map<std::string, float> sample_facefx_servo_targets(
    const std::vector<FaceFxLipSyncServo>& servos,
    const Character& character);

// Mirrors GH2 XEX sub_821A7978: FxBonePoseNode scalar values select authored
// "visemes" frames by node ordinal, quaternion records receive the neutral
// residual, and the "neutral" clip is composed as the final source pass.
FaceFxMaterializedFrame materialize_facefx_animation_frame(
    const FaceFxGraph& graph,
    const std::unordered_map<std::string, float>& registers,
    const CharClip& neutral_clip,
    const CharClip& visemes_clip);
bool apply_facefx_typed_animation_frame(
    const FaceFxGraph& graph,
    const std::unordered_map<std::string, float>& registers,
    const CharClip& neutral_clip,
    const CharClip& visemes_clip,
    Character& character);

// Legacy FAC-matrix inspection helpers. Gameplay uses the typed viseme path
// above; the FAC matrices are not expression-shape authority.
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
