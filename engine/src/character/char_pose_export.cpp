// Native character-pose sampler for source-model interchange tools.

#include "character/char_clip.h"
#include "character/char_mesh.h"
#include "character/char_renderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using ghogx::character::Character;
using ghogx::character::CharClip;
using ghogx::milo_scene::Xfm;

struct FixedPoseLayer {
  std::string milo_path;
  CharClip clip;
  int frame = 0;
};

void write_json_string(std::ostream& out, const std::string& value) {
  out << '"';
  for (const unsigned char ch : value) {
    switch (ch) {
      case '"': out << "\\\""; break;
      case '\\': out << "\\\\"; break;
      case '\b': out << "\\b"; break;
      case '\f': out << "\\f"; break;
      case '\n': out << "\\n"; break;
      case '\r': out << "\\r"; break;
      case '\t': out << "\\t"; break;
      default:
        if (ch < 0x20) {
          const auto flags = out.flags();
          const auto fill = out.fill();
          out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<int>(ch);
          out.flags(flags);
          out.fill(fill);
        } else {
          out << static_cast<char>(ch);
        }
        break;
    }
  }
  out << '"';
}

void write_xfm_matrix12(std::ostream& out, const Xfm& xfm) {
  out << '[';
  bool first = true;
  for (int row = 0; row < 3; ++row) {
    for (int column = 0; column < 3; ++column) {
      if (!first) out << ',';
      out << xfm.rot[row][column];
      first = false;
    }
  }
  for (int axis = 0; axis < 3; ++axis) out << ',' << xfm.pos[axis];
  out << ']';
}

void write_world_matrix12(std::ostream& out,
                          const std::array<float, 16>& matrix) {
  out << '[' << matrix[0] << ',' << matrix[1] << ',' << matrix[2] << ','
      << matrix[4] << ',' << matrix[5] << ',' << matrix[6] << ','
      << matrix[8] << ',' << matrix[9] << ',' << matrix[10] << ','
      << matrix[12] << ',' << matrix[13] << ',' << matrix[14] << ']';
}

void reset_character_pose(Character& character) {
  for (size_t index = 0;
       index < character.bones.size() &&
       index < character.bind_bone_local.size();
       ++index) {
    character.bones[index].local = character.bind_bone_local[index];
  }
  for (size_t index = 0;
       index < character.meshes.size() &&
       index < character.bind_mesh_local.size();
       ++index) {
    character.meshes[index].local = character.bind_mesh_local[index];
  }
  for (auto& [name, proxy] : character.attached_prop_transform_proxies) {
    (void)name;
    proxy.local = proxy.bind_local;
  }
  ghogx::character::clear_runtime_trans_worlds(character);
}

void write_transform(std::ostream& out, const std::string& kind,
                     const std::string& name, const std::string& parent,
                     const Xfm& local,
                     const std::array<float, 16>& world, bool& first) {
  if (!first) out << ',';
  first = false;
  out << "{\"kind\":";
  write_json_string(out, kind);
  out << ",\"name\":";
  write_json_string(out, name);
  out << ",\"parent\":";
  write_json_string(out, parent);
  out << ",\"local_matrix12\":";
  write_xfm_matrix12(out, local);
  out << ",\"world_matrix12\":";
  write_world_matrix12(out, world);
  out << '}';
}

void append_pose_layers(
    ghogx::character::ClipChannelLayerStack& pose_stack,
    const CharClip& clip, int frame,
    const std::vector<FixedPoseLayer>& fixed_layers) {
  ghogx::character::append_clip_frame_layer(
      pose_stack, clip, frame, 1.0f, false);
  for (const auto& layer : fixed_layers) {
    ghogx::character::append_clip_frame_layer(
        pose_stack, layer.clip, layer.frame, 1.0f, false);
  }
}

void write_frame(std::ostream& out, Character& character,
                 const CharClip& clip, int frame,
                 const std::vector<FixedPoseLayer>& fixed_layers,
                 bool controllers, float left_ik_weight,
                 float right_ik_weight) {
  reset_character_pose(character);

  ghogx::character::ClipChannelLayerStack pose_stack;
  pose_stack.debug_label = "pose-export";
  append_pose_layers(pose_stack, clip, frame, fixed_layers);

  ghogx::character::CharacterPoseControllerFrameSources sources;
  sources.pose_stack = &pose_stack;
  sources.controllers_enabled = controllers;
  ghogx::character::SourceCharMainDriverHandWeights clip_driver_weights;
  if (left_ik_weight < 0.0f && right_ik_weight < 0.0f) {
    clip_driver_weights =
        ghogx::character::source_char_main_driver_hand_weights_from_clip_flags(
            character, clip.flags, 0.0f, 0.0f);
    sources.driver_weights = &clip_driver_weights;
  } else {
    if (left_ik_weight >= 0.0f) {
      sources.fallback_ik_weights.push_back({"left.weight", left_ik_weight});
    }
    if (right_ik_weight >= 0.0f) {
      sources.fallback_ik_weights.push_back({"right.weight", right_ik_weight});
    }
  }
  sources.time_seconds =
      clip.fps > 0 ? static_cast<float>(frame) / clip.fps : 0.0f;
  ghogx::character::apply_character_pose_controller_frame(character, sources);

  out << "{\"frame\":" << frame << ",\"transforms\":[";
  bool first = true;
  for (const auto& bone : character.bones) {
    write_transform(out, "Trans", bone.name, bone.parent, bone.local,
                    character.bone_world_local_chain(bone.name), first);
  }
  for (const auto& mesh : character.meshes) {
    write_transform(out, "Mesh", mesh.name, mesh.parent, mesh.local,
                    character.mesh_world(mesh), first);
  }
  out << "]}\n";
}

void write_u32(std::ostream& out, uint32_t value) {
  out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void write_f32(std::ostream& out, float value) {
  out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void write_binary_string(std::ostream& out, const std::string& value) {
  write_u32(out, static_cast<uint32_t>(value.size()));
  out.write(value.data(), static_cast<std::streamsize>(value.size()));
}

void write_deformed_pose(const Character& character, int frame,
                         const std::string& path) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) throw std::runtime_error("cannot open deformed output " + path);
  out.write("GH2POSED", 8);
  write_u32(out, 1);
  write_u32(out, static_cast<uint32_t>(frame));
  uint32_t mesh_count = 0;
  for (const auto& mesh : character.meshes) {
    if (mesh.decoded && !mesh.verts.empty()) ++mesh_count;
  }
  write_u32(out, mesh_count);

  for (const auto& mesh : character.meshes) {
    if (!mesh.decoded || mesh.verts.empty()) continue;
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 3>> normals;
    ghogx::character::skin_to_pose(mesh, character, positions, normals);
    const auto submission =
        ghogx::character::source_character_mesh_submission_world(mesh,
                                                                 character);
    const bool transform_submission = mesh.bone_palette.empty();
    write_binary_string(out, mesh.name);
    write_u32(out, static_cast<uint32_t>(positions.size()));
    for (size_t index = 0; index < positions.size(); ++index) {
      auto position = positions[index];
      auto normal = normals[index];
      if (transform_submission) {
        const auto source_position = position;
        position = {
            source_position[0] * submission[0] +
                source_position[1] * submission[4] +
                source_position[2] * submission[8] + submission[12],
            source_position[0] * submission[1] +
                source_position[1] * submission[5] +
                source_position[2] * submission[9] + submission[13],
            source_position[0] * submission[2] +
                source_position[1] * submission[6] +
                source_position[2] * submission[10] + submission[14]};
        const auto source_normal = normal;
        normal = {
            source_normal[0] * submission[0] +
                source_normal[1] * submission[4] +
                source_normal[2] * submission[8],
            source_normal[0] * submission[1] +
                source_normal[1] * submission[5] +
                source_normal[2] * submission[9],
            source_normal[0] * submission[2] +
                source_normal[1] * submission[6] +
                source_normal[2] * submission[10]};
        const float length = std::sqrt(normal[0] * normal[0] +
                                       normal[1] * normal[1] +
                                       normal[2] * normal[2]);
        if (length > 1.0e-8f) {
          normal[0] /= length;
          normal[1] /= length;
          normal[2] /= length;
        }
      }
      for (float value : position) write_f32(out, value);
      for (float value : normal) write_f32(out, value);
    }
  }
  out.close();
  if (!out) throw std::runtime_error("failed while writing " + path);
}

void usage() {
  std::fprintf(
      stderr,
      "usage: ghogx_character_pose_export <main.hdr> <main_0.ark> "
      "<character.milo_ps2> <clip.milo_ps2> <clip-name> <output.jsonl> "
      "[options]\n"
      "   or: ghogx_character_pose_export --loose <character.milo_ps2> "
      "<clip.milo_ps2> <clip-name> <output.jsonl> "
      "[--frame N] [--no-controllers] "
      "[--left-ik-weight 0..1] [--right-ik-weight 0..1] "
      "[--layer <clip.milo_ps2> <clip-name> <frame>] "
      "[--deformed-frame N --deformed-output FILE]\n");
}

}  // namespace

int main(int argc, char** argv) {
  const bool loose = argc > 1 && std::string(argv[1]) == "--loose";
  const int required = loose ? 6 : 7;
  if (argc < required) {
    usage();
    return 2;
  }

  const int base = loose ? 2 : 1;
  const std::string hdr_path = loose ? std::string() : argv[base];
  const std::string ark_path = loose ? std::string() : argv[base + 1];
  const std::string character_milo = argv[base + (loose ? 0 : 2)];
  const std::string clip_milo = argv[base + (loose ? 1 : 3)];
  const std::string clip_name = argv[base + (loose ? 2 : 4)];
  const std::string output_path = argv[base + (loose ? 3 : 5)];
  int selected_frame = -1;
  int deformed_frame = -1;
  std::string deformed_output;
  bool controllers = true;
  float left_ik_weight = -1.0f;
  float right_ik_weight = -1.0f;
  struct LayerArgument {
    std::string milo_path;
    std::string clip_name;
    int frame = 0;
  };
  std::vector<LayerArgument> layer_arguments;
  for (int index = required; index < argc; ++index) {
    const std::string option = argv[index];
    if (option == "--frame" && index + 1 < argc) {
      selected_frame = std::stoi(argv[++index]);
    } else if (option == "--no-controllers") {
      controllers = false;
    } else if (option == "--left-ik-weight" && index + 1 < argc) {
      left_ik_weight = std::stof(argv[++index]);
    } else if (option == "--right-ik-weight" && index + 1 < argc) {
      right_ik_weight = std::stof(argv[++index]);
    } else if (option == "--layer" && index + 3 < argc) {
      LayerArgument layer;
      layer.milo_path = argv[++index];
      layer.clip_name = argv[++index];
      layer.frame = std::stoi(argv[++index]);
      layer_arguments.push_back(std::move(layer));
    } else if (option == "--deformed-frame" && index + 1 < argc) {
      deformed_frame = std::stoi(argv[++index]);
    } else if (option == "--deformed-output" && index + 1 < argc) {
      deformed_output = argv[++index];
    } else {
      usage();
      return 2;
    }
  }

  try {
    for (const float weight : {left_ik_weight, right_ik_weight}) {
      if (weight >= 0.0f && (!std::isfinite(weight) || weight > 1.0f)) {
        throw std::runtime_error("IK weights must be finite values in [0, 1]");
      }
    }
    Character character;
    if (!ghogx::character::load_character(
            hdr_path, ark_path, character_milo, character)) {
      throw std::runtime_error("failed to load character " + character_milo);
    }
    const auto clip = ghogx::character::load_clip(
        hdr_path, ark_path, clip_milo, clip_name);
    if (!clip.loaded || clip.frames.empty()) {
      throw std::runtime_error("failed to load clip " + clip_name);
    }
    std::vector<FixedPoseLayer> fixed_layers;
    fixed_layers.reserve(layer_arguments.size());
    for (const auto& argument : layer_arguments) {
      FixedPoseLayer layer;
      layer.milo_path = argument.milo_path;
      layer.clip = ghogx::character::load_clip(
          hdr_path, ark_path, argument.milo_path, argument.clip_name);
      layer.frame = argument.frame;
      if (!layer.clip.loaded || layer.clip.frames.empty()) {
        throw std::runtime_error("failed to load layer clip " +
                                 argument.clip_name);
      }
      if (layer.frame < 0 ||
          layer.frame >= static_cast<int>(layer.clip.frames.size())) {
        throw std::runtime_error("layer frame is outside clip range for " +
                                 argument.clip_name);
      }
      fixed_layers.push_back(std::move(layer));
    }
    if (selected_frame >= static_cast<int>(clip.frames.size())) {
      throw std::runtime_error("selected frame is outside clip range");
    }
    if ((deformed_frame >= 0) != !deformed_output.empty()) {
      throw std::runtime_error(
          "--deformed-frame and --deformed-output must be supplied together");
    }
    if (deformed_frame >= static_cast<int>(clip.frames.size())) {
      throw std::runtime_error("deformed frame is outside clip range");
    }

    std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("cannot open output " + output_path);
    output << std::setprecision(9);
    output << "{\"format\":\"ghogx-character-pose-jsonl\",\"version\":2,"
              "\"character_milo\":";
    write_json_string(output, character_milo);
    output << ",\"clip_milo\":";
    write_json_string(output, clip_milo);
    output << ",\"clip\":";
    write_json_string(output, clip.name);
    output << ",\"fps\":" << clip.fps
           << ",\"frame_count\":" << clip.frames.size()
           << ",\"controllers\":" << (controllers ? "true" : "false")
           << ",\"left_ik_weight\":";
    if (left_ik_weight >= 0.0f) output << left_ik_weight;
    else output << "null";
    output << ",\"right_ik_weight\":";
    if (right_ik_weight >= 0.0f) output << right_ik_weight;
    else output << "null";
    output << ",\"fixed_layers\":[";
    for (size_t index = 0; index < fixed_layers.size(); ++index) {
      if (index != 0) output << ',';
      output << "{\"clip_milo\":";
      write_json_string(output, fixed_layers[index].milo_path);
      output << ",\"clip\":";
      write_json_string(output, fixed_layers[index].clip.name);
      output << ",\"frame\":" << fixed_layers[index].frame << '}';
    }
    output << "]}\n";

    const int begin = selected_frame >= 0 ? selected_frame : 0;
    const int end = selected_frame >= 0
                        ? selected_frame + 1
                        : static_cast<int>(clip.frames.size());
    for (int frame = begin; frame < end; ++frame) {
      write_frame(output, character, clip, frame, fixed_layers, controllers,
                  left_ik_weight, right_ik_weight);
      if (frame == deformed_frame) {
        write_deformed_pose(character, frame, deformed_output);
      }
    }
    if (deformed_frame >= 0 && (deformed_frame < begin || deformed_frame >= end)) {
      reset_character_pose(character);
      ghogx::character::ClipChannelLayerStack pose_stack;
      append_pose_layers(pose_stack, clip, deformed_frame, fixed_layers);
      ghogx::character::CharacterPoseControllerFrameSources sources;
      sources.pose_stack = &pose_stack;
      sources.controllers_enabled = controllers;
      ghogx::character::SourceCharMainDriverHandWeights clip_driver_weights;
      if (left_ik_weight < 0.0f && right_ik_weight < 0.0f) {
        clip_driver_weights = ghogx::character::
            source_char_main_driver_hand_weights_from_clip_flags(
                character, clip.flags, 0.0f, 0.0f);
        sources.driver_weights = &clip_driver_weights;
      } else {
        if (left_ik_weight >= 0.0f) {
          sources.fallback_ik_weights.push_back(
              {"left.weight", left_ik_weight});
        }
        if (right_ik_weight >= 0.0f) {
          sources.fallback_ik_weights.push_back(
              {"right.weight", right_ik_weight});
        }
      }
      sources.time_seconds =
          clip.fps > 0 ? static_cast<float>(deformed_frame) / clip.fps : 0.0f;
      ghogx::character::apply_character_pose_controller_frame(character,
                                                               sources);
      write_deformed_pose(character, deformed_frame, deformed_output);
    }
    output.close();
    if (!output) throw std::runtime_error("failed while writing " + output_path);

    std::fprintf(stderr,
                 "[pose-export] clip=%s frames=%d..%d transforms=%zu "
                 "controllers=%d output=%s\n",
                 clip.name.c_str(), begin, end - 1,
                 character.bones.size() + character.meshes.size(),
                 controllers ? 1 : 0, output_path.c_str());
    return 0;
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[pose-export] %s\n", ex.what());
    return 1;
  }
}
