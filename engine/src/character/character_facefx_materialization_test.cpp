#include "character/char_facefx.h"

#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using ghogx::character::CharClip;
using ghogx::character::ClipChannel;
using ghogx::character::FaceFxGraph;
using ghogx::character::FaceFxGraphInput;
using ghogx::character::FaceFxGraphNode;

bool near(float actual, float expected, const std::string& label) {
  if (std::fabs(actual - expected) <= 1.0e-5f) return true;
  std::cerr << label << ": expected " << expected << ", got " << actual
            << "\n";
  return false;
}

ClipChannel pos_channel(const char* name, float x, float y, float z) {
  ClipChannel channel;
  channel.type = ClipChannel::kPos;
  channel.bone_name = name;
  channel.pos[0] = x;
  channel.pos[1] = y;
  channel.pos[2] = z;
  return channel;
}

ClipChannel quat_channel(const char* name, float x, float y, float z,
                         float w) {
  ClipChannel channel;
  channel.type = ClipChannel::kQuat;
  channel.bone_name = name;
  channel.quat[0] = x;
  channel.quat[1] = y;
  channel.quat[2] = z;
  channel.quat[3] = w;
  return channel;
}

CharClip::OutputBone output_bone(const char* name) {
  CharClip::OutputBone bone;
  bone.name = name;
  return bone;
}

FaceFxGraphNode register_node(const char* name, float min_value = 0.0f,
                              float max_value = 1.0f) {
  FaceFxGraphNode node;
  node.name = name;
  node.class_name = "FxCombinerNode";
  node.min_value = min_value;
  node.max_value = max_value;
  return node;
}

FaceFxGraphNode pose_node(const char* name, std::size_t ordinal,
                          const char* input) {
  FaceFxGraphNode node;
  node.name = name;
  node.class_name = "FxBonePoseNode";
  node.min_value = 0.0f;
  node.max_value = 1.0f;
  node.pose_ordinal = ordinal;
  if (input != nullptr) {
    node.inputs.push_back(FaceFxGraphInput{input, "linear", 1.0f});
  }
  return node;
}

const ClipChannel* find_channel(const std::vector<ClipChannel>& channels,
                                ClipChannel::Type type, const char* name) {
  for (const ClipChannel& channel : channels) {
    if (channel.type == type && channel.bone_name == name) return &channel;
  }
  return nullptr;
}

}  // namespace

int main() {
  constexpr float kSqrtHalf = 0.70710678118654752440f;
  bool ok = true;

  FaceFxGraph graph;
  graph.nodes.push_back(pose_node("Neutral", 0, nullptr));
  graph.nodes.push_back(pose_node("Good1", 1, "expressionGood1"));
  graph.nodes.push_back(pose_node("EyesClosed", 2, "Blink"));
  graph.nodes.push_back(register_node("expressionGood1"));
  graph.nodes.push_back(register_node("Blink"));

  CharClip neutral;
  neutral.loaded = true;
  neutral.frames = {{pos_channel("bone_lip-L-corner.mesh", 10.0f, 20.0f,
                                 30.0f),
                     quat_channel("bone_L-upperlid.mesh", 0.0f, 0.0f, 0.0f,
                                  1.0f)}};
  neutral.output_bones = {output_bone("bone_lip-L-corner.trans"),
                          output_bone("bone_L-upperlid.trans")};

  CharClip visemes;
  visemes.loaded = true;
  visemes.frames = {
      {pos_channel("bone_lip-L-corner.mesh", 0.0f, 0.0f, 0.0f),
       quat_channel("bone_L-upperlid.mesh", 0.0f, 0.0f, 0.0f, 1.0f)},
      {pos_channel("bone_lip-L-corner.mesh", 4.0f, -8.0f, 2.0f),
       quat_channel("bone_L-upperlid.mesh", 0.0f, 0.0f, 0.0f, 1.0f)},
      {pos_channel("bone_lip-L-corner.mesh", 0.0f, 0.0f, 0.0f),
       quat_channel("bone_L-upperlid.mesh", 0.0f, 0.0f, 1.0f, 0.0f)}};
  visemes.output_bones = neutral.output_bones;

  const std::unordered_map<std::string, float> registers = {
      {"expressionGood1", 0.25f}, {"Blink", 0.5f}};
  const auto frame = ghogx::character::materialize_facefx_animation_frame(
      graph, registers, neutral, visemes);
  ok &= frame.valid;
  ok &= frame.active_pose_count == 2;
  ok &= near(frame.neutral_residual, 0.25f, "neutral residual");
  const ClipChannel* lip =
      find_channel(frame.channels, ClipChannel::kPos,
                   "bone_lip-L-corner.mesh");
  const ClipChannel* lid =
      find_channel(frame.channels, ClipChannel::kQuat,
                   "bone_L-upperlid.mesh");
  ok &= lip != nullptr;
  ok &= lid != nullptr;
  if (lip != nullptr) {
    ok &= near(lip->pos[0], 11.0f, "lip x weighted viseme");
    ok &= near(lip->pos[1], 18.0f, "lip y weighted viseme");
    ok &= near(lip->pos[2], 30.5f, "lip z weighted viseme");
  }
  if (lid != nullptr) {
    ok &= near(lid->quat[0], 0.0f, "lid quat x");
    ok &= near(lid->quat[1], 0.0f, "lid quat y");
    ok &= near(lid->quat[2], 0.5f, "lid quat z");
    ok &= near(lid->quat[3], 0.5f, "lid quat residual w");
  }

  const auto idle = ghogx::character::materialize_facefx_animation_frame(
      graph, {}, neutral, visemes);
  const ClipChannel* idle_lip =
      find_channel(idle.channels, ClipChannel::kPos,
                   "bone_lip-L-corner.mesh");
  const ClipChannel* idle_lid =
      find_channel(idle.channels, ClipChannel::kQuat,
                   "bone_L-upperlid.mesh");
  ok &= near(idle.neutral_residual, 1.0f, "idle neutral residual");
  ok &= idle_lip != nullptr && idle_lid != nullptr;
  if (idle_lip != nullptr) {
    ok &= near(idle_lip->pos[0], 10.0f, "idle lip neutral x");
    ok &= near(idle_lip->pos[1], 20.0f, "idle lip neutral y");
    ok &= near(idle_lip->pos[2], 30.0f, "idle lip neutral z");
  }
  if (idle_lid != nullptr) {
    ok &= near(idle_lid->quat[0], 0.0f, "idle lid x");
    ok &= near(idle_lid->quat[1], 0.0f, "idle lid y");
    ok &= near(idle_lid->quat[2], 0.0f, "idle lid z");
    ok &= near(idle_lid->quat[3], 1.0f, "idle lid w");
  }

  FaceFxGraph compose_graph;
  compose_graph.nodes.push_back(pose_node("Good1", 1, "expressionGood1"));
  compose_graph.nodes.push_back(register_node("expressionGood1"));
  CharClip compose_neutral = neutral;
  compose_neutral.frames[0][1] = quat_channel(
      "bone_L-upperlid.mesh", kSqrtHalf, 0.0f, 0.0f, kSqrtHalf);
  CharClip compose_visemes = visemes;
  compose_visemes.frames[1][1] = quat_channel(
      "bone_L-upperlid.mesh", 0.0f, 0.0f, kSqrtHalf, kSqrtHalf);
  const auto composed = ghogx::character::materialize_facefx_animation_frame(
      compose_graph, {{"expressionGood1", 1.0f}}, compose_neutral,
      compose_visemes);
  const ClipChannel* composed_lid =
      find_channel(composed.channels, ClipChannel::kQuat,
                   "bone_L-upperlid.mesh");
  ok &= composed_lid != nullptr;
  if (composed_lid != nullptr) {
    ok &= near(composed_lid->quat[0], 0.5f, "neutral times active x");
    ok &= near(composed_lid->quat[1], -0.5f, "neutral times active y");
    ok &= near(composed_lid->quat[2], 0.5f, "neutral times active z");
    ok &= near(composed_lid->quat[3], 0.5f, "neutral times active w");
  }

  ghogx::character::Character eye_character;
  auto eye_mesh = [](const char* name) {
    ghogx::character::SkinnedMesh mesh;
    mesh.name = name;
    for (int row = 0; row < 3; ++row) {
      for (int col = 0; col < 3; ++col) {
        mesh.local.rot[row][col] = row == col ? 1.0f : 0.0f;
      }
    }
    return mesh;
  };
  eye_character.meshes.push_back(eye_mesh("eye-x.mesh"));
  eye_character.meshes.back().local.rot[1][2] = std::sin(0.3f);
  eye_character.meshes.back().local.rot[1][1] = std::cos(0.3f);
  eye_character.meshes.push_back(eye_mesh("eye-y.mesh"));
  eye_character.meshes.back().local.rot[0][2] = -std::sin(0.4f);
  eye_character.meshes.back().local.rot[2][2] = std::cos(0.4f);
  eye_character.meshes.push_back(eye_mesh("eye-z.mesh"));
  eye_character.meshes.back().local.rot[1][0] = -std::sin(0.5f);
  eye_character.meshes.back().local.rot[1][1] = std::cos(0.5f);

  ghogx::character::FaceFxLipSyncServo eye_servo;
  eye_servo.targets = {
      {"eye-x.mesh", 0, "L-eyeX"},
      {"eye-y.mesh", 1, "L-eyeY"},
      {"eye-z.mesh", 2, "L-eyeZ"},
      {"EYE-X.MESH", 0, "fuzzy-must-not-resolve"},
      {"eye-x.mesh", 7, "invalid-op-must-not-resolve"},
  };
  const auto eye_registers =
      ghogx::character::sample_facefx_servo_targets({eye_servo},
                                                     eye_character);
  ok &= eye_registers.size() == 3;
  ok &= near(eye_registers.at("L-eyeX"), 0.3f, "servo RotX radians");
  ok &= near(eye_registers.at("L-eyeY"), 0.4f, "servo RotY radians");
  ok &= near(eye_registers.at("L-eyeZ"), 0.5f, "servo RotZ radians");
  ok &= eye_registers.find("fuzzy-must-not-resolve") == eye_registers.end();
  ok &= eye_registers.find("invalid-op-must-not-resolve") ==
        eye_registers.end();

  if (!ok) {
    std::cerr << "FaceFX materialization must preserve GH2 node-ordinal, "
                 "neutral-residual, and final source-pass behavior.\n";
    return 1;
  }
  return 0;
}
