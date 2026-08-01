#include "character/char_facefx.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <unordered_map>

int main(int argc, char** argv) {
  if (argc != 6) {
    std::fprintf(stderr,
                 "usage: ghogx_character_facefx_audit <main.hdr> "
                 "<main_0.ark> <character.milo_ps2> <viseme.milo_ps2> "
                 "<register-name>\n");
    return 2;
  }

  const std::string hdr = argv[1];
  const std::string ark = argv[2];
  const std::string character_milo = argv[3];
  const std::string viseme_milo = argv[4];
  const std::string register_name = argv[5];

  ghogx::character::Character character;
  if (!ghogx::character::load_character(hdr, ark, character_milo, character)) {
    std::fprintf(stderr, "[facefx-audit] character load failed: %s\n",
                 character_milo.c_str());
    return 1;
  }
  auto graph = ghogx::character::load_facefx_graph(
      hdr, ark, character_milo, character);
  if (!graph) {
    std::fprintf(stderr, "[facefx-audit] graph load failed: %s\n",
                 character_milo.c_str());
    return 1;
  }

  ghogx::character::CharClip neutral = ghogx::character::load_clip(
      hdr, ark, viseme_milo, "neutral");
  ghogx::character::CharClip visemes = ghogx::character::load_clip(
      hdr, ark, viseme_milo, "visemes");
  if (neutral.frames.empty() || visemes.frames.empty()) {
    std::fprintf(stderr, "[facefx-audit] neutral/visemes load failed: %s\n",
                 viseme_milo.c_str());
    return 1;
  }

  std::size_t pose_nodes = 0;
  std::size_t max_ordinal = 0;
  for (const auto& node : graph->nodes) {
    if (!node.pose_ordinal) continue;
    ++pose_nodes;
    max_ordinal = std::max(max_ordinal, *node.pose_ordinal);
    std::printf("[facefx-audit-node] ordinal=%zu name=%s poseTable=%s\n",
                *node.pose_ordinal, node.name.c_str(),
                node.pose_index ? "decoded" : "missing");
  }

  std::size_t servo_targets = 0;
  for (const auto& servo : character.lip_sync_servos) {
    for (const auto& target : servo.targets) {
      ++servo_targets;
      std::printf(
          "[facefx-audit-target] servo=%s object=%s op=%d register=%s\n",
          servo.name.c_str(), target.object.c_str(), target.prop_type,
          target.property.c_str());
    }
  }

  const std::unordered_map<std::string, float> registers = {
      {register_name, 1.0f}};
  const auto frame = ghogx::character::materialize_facefx_animation_frame(
      *graph, registers, neutral, visemes);
  std::printf(
      "[facefx-audit] character=%s graphNodes=%zu poseNodes=%zu "
      "visemeFrames=%zu neutralFrames=%zu maxOrdinal=%zu servoTargets=%zu "
      "testRegister=%s activePoses=%zu residual=%.6f channels=%zu "
      "outputBones=%zu valid=%d\n",
      character_milo.c_str(), graph->nodes.size(), pose_nodes,
      visemes.frames.size(), neutral.frames.size(), max_ordinal, servo_targets,
      register_name.c_str(), frame.active_pose_count, frame.neutral_residual,
      frame.channels.size(), frame.output_bones.size(), frame.valid ? 1 : 0);

  const bool ordinals_fit = pose_nodes > 0 && max_ordinal < visemes.frames.size();
  if (!frame.valid || !ordinals_fit) {
    std::fprintf(stderr,
                 "[facefx-audit] rejected: valid=%d ordinalsFit=%d\n",
                 frame.valid ? 1 : 0, ordinals_fit ? 1 : 0);
    return 1;
  }
  return 0;
}
