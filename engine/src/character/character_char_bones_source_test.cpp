#include "character/char_clip.h"

#include <array>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool expect_int(int got, int want, const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got " << got << " want " << want << "\n";
  return false;
}

bool expect_size(size_t got, size_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got " << got << " want " << want << "\n";
  return false;
}

bool expect_float(float got, float want, const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got " << got << " want " << want << "\n";
  return false;
}

bool expect_layout(const ghogx::character::SourceCharBonesLayout& got,
                   const std::array<int, 7>& offsets, int total_size,
                   const char* label) {
  bool ok = true;
  for (size_t i = 0; i < offsets.size(); ++i) {
    if (got.offsets[i] != offsets[i]) {
      std::cerr << label << " offset[" << i << "]: got " << got.offsets[i]
                << " want " << offsets[i] << "\n";
      ok = false;
    }
  }
  if (got.total_size != total_size) {
    std::cerr << label << " total: got " << got.total_size << " want "
              << total_size << "\n";
    ok = false;
  }
  return ok;
}

bool expect_empty_state(const ghogx::character::SourceCharBonesState& got,
                        const char* label) {
  bool ok = true;
  ok &= expect_int(got.compression, 0, label);
  ok &= expect_size(got.bones.size(), 0, label);
  for (size_t i = 0; i < got.layout.counts.size(); ++i) {
    ok &= expect_int(got.layout.counts[i], 0, label);
    ok &= expect_int(got.layout.offsets[i], 0, label);
  }
  ok &= expect_int(got.layout.total_size, 0, label);
  return ok;
}

bool expect_compression_update(
    const ghogx::character::SourceCharBonesCompressionUpdate& got,
    int compression, bool changed, const std::array<int, 7>& offsets,
    int total_size, const char* label) {
  bool ok = true;
  if (got.compression != compression) {
    std::cerr << label << " compression: got " << got.compression
              << " want " << compression << "\n";
    ok = false;
  }
  if (got.changed != changed) {
    std::cerr << label << " changed: got " << got.changed << " want "
              << changed << "\n";
    ok = false;
  }
  ok &= expect_layout(got.layout, offsets, total_size, label);
  return ok;
}

bool expect_string(const std::string& got, const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got '" << got << "' want '" << want << "'\n";
  return false;
}

}  // namespace

int main() {
  using namespace ghogx::character;

  bool ok = true;
  ok &= expect_int(source_char_bones_type_of("bone_head.pos"),
                   kSourceCharBonesTypePos, "type pos");
  ok &= expect_int(source_char_bones_type_of("bone_head.scale"),
                   kSourceCharBonesTypeScale, "type scale");
  ok &= expect_int(source_char_bones_type_of("bone_head.quat"),
                   kSourceCharBonesTypeQuat, "type quat");
  ok &= expect_int(source_char_bones_type_of("bone_head.rotx"),
                   kSourceCharBonesTypeRotX, "type rotx");
  ok &= expect_int(source_char_bones_type_of("bone_head.roty"),
                   kSourceCharBonesTypeRotY, "type roty");
  ok &= expect_int(source_char_bones_type_of("bone_head.rotz"),
                   kSourceCharBonesTypeRotZ, "type rotz");
  ok &= expect_int(source_char_bones_type_of("bone_head"),
                   kSourceCharBonesTypeEnd, "type missing suffix");

  ok &= expect_string(source_char_bones_suffix_of(kSourceCharBonesTypePos),
                      "pos", "suffix pos");
  ok &= expect_string(source_char_bones_suffix_of(kSourceCharBonesTypeRotZ),
                      "rotz", "suffix rotz");
  ok &= expect_string(source_char_bones_suffix_of(kSourceCharBonesTypeEnd),
                      "", "suffix invalid");

  ok &= expect_string(source_char_bones_channel_name("bone_head",
                                                    kSourceCharBonesTypeQuat),
                      "bone_head.quat", "channel append suffix");
  ok &= expect_string(source_char_bones_channel_name("bone_head.pos",
                                                    kSourceCharBonesTypeRotY),
                      "bone_head.roty", "channel replace suffix");
  ok &= expect_string(source_char_bones_channel_name("bone.head.pos",
                                                    kSourceCharBonesTypeRotX),
                      "bone.rotx", "channel first-dot rule");

  for (int compression = 0; compression <= 4; ++compression) {
    const size_t vec_size = compression < 2 ? 12u : 6u;
    const size_t quat_size =
        compression > 2 ? 4u : (compression == 0 ? 16u : 8u);
    const size_t angle_size = compression == 0 ? 4u : 2u;
    ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypePos,
                                                  compression),
                      vec_size, "type size pos");
    ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypeScale,
                                                  compression),
                      vec_size, "type size scale");
    ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypeQuat,
                                                  compression),
                      quat_size, "type size quat");
    ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypeRotX,
                                                  compression),
                      angle_size, "type size rotx");
    ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypeRotY,
                                                  compression),
                      angle_size, "type size roty");
    ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypeRotZ,
                                                  compression),
                      angle_size, "type size rotz");
  }

  ok &= expect_size(source_char_bones_type_size(kSourceCharBonesTypeEnd, 0), 0,
                    "type size invalid");

  const std::array<int, kSourceCharBonesTypeEnd + 1> counts = {
      0, 2, 3, 5, 7, 8, 9};
  ok &= expect_layout(source_char_bones_recompute_layout(counts, 0),
                      {0, 24, 36, 68, 76, 80, 84}, 96,
                      "layout uncompressed");
  ok &= expect_layout(source_char_bones_recompute_layout(counts, 2),
                      {0, 12, 18, 34, 38, 40, 42}, 48,
                      "layout compressed vectors");
  ok &= expect_layout(source_char_bones_recompute_layout(counts, 4),
                      {0, 12, 18, 26, 30, 32, 34}, 48,
                      "layout compressed all");
  const SourceCharBonesLayout layout_none =
      source_char_bones_recompute_layout(counts, 0);
  ok &= expect_compression_update(
      source_char_bones_set_compression(0, layout_none, 0),
      0, false, {0, 24, 36, 68, 76, 80, 84}, 96,
      "compression unchanged");
  ok &= expect_compression_update(
      source_char_bones_set_compression(0, layout_none, 4),
      4, true, {0, 12, 18, 26, 30, 32, 34}, 48,
      "compression changed");

  ok &= expect_empty_state(source_char_bones_empty_state(),
                           "default CharBones state");
  SourceCharBonesState state = source_char_bones_empty_state();
  state.compression = 4;
  state.layout = source_char_bones_recompute_layout(counts, 4);
  state.bones.push_back({"bone_head.quat", 0.25f});
  state.bones.push_back({"bone_hand.pos", 0.75f});
  source_char_bones_set_weights(state, 0.5f);
  ok &= expect_size(state.bones.size(), 2, "state SetWeights count");
  ok &= expect_string(state.bones[0].name, "bone_head.quat",
                      "state SetWeights preserves first name");
  ok &= expect_float(state.bones[0].weight, 0.5f,
                     "state SetWeights first weight");
  ok &= expect_string(state.bones[1].name, "bone_hand.pos",
                      "state SetWeights preserves second name");
  ok &= expect_float(state.bones[1].weight, 0.5f,
                     "state SetWeights second weight");
  std::vector<SourceCharBonesBone> listed;
  listed.push_back({"preexisting.scale", 1.0f});
  source_char_bones_list_bones(state, listed);
  ok &= expect_size(listed.size(), 3, "ListBones append count");
  ok &= expect_string(listed[0].name, "preexisting.scale",
                      "ListBones preserves caller rows");
  ok &= expect_float(listed[0].weight, 1.0f,
                     "ListBones preserves caller weight");
  ok &= expect_string(listed[1].name, "bone_head.quat",
                      "ListBones first appended name");
  ok &= expect_float(listed[1].weight, 0.5f,
                     "ListBones first appended weight");
  ok &= expect_string(listed[2].name, "bone_hand.pos",
                      "ListBones second appended name");
  ok &= expect_float(listed[2].weight, 0.5f,
                     "ListBones second appended weight");

  SourceCharBonesState lookup_state = source_char_bones_empty_state();
  lookup_state.compression = 0;
  lookup_state.layout.counts = {0, 2, 3, 5, 7, 8, 9};
  lookup_state.layout =
      source_char_bones_recompute_layout(lookup_state.layout.counts,
                                         lookup_state.compression);
  lookup_state.bones = {{"bone_a.pos", 1.0f},
                        {"bone_b.pos", 1.0f},
                        {"bone_a.scale", 1.0f},
                        {"bone_a.quat", 1.0f},
                        {"bone_b.quat", 1.0f},
                        {"bone_a.rotx", 1.0f},
                        {"bone_b.rotx", 1.0f},
                        {"bone_a.roty", 1.0f},
                        {"bone_a.rotz", 1.0f}};
  ok &= expect_int(source_char_bones_find_offset(lookup_state, "bone_a.pos"),
                   0, "FindOffset first pos");
  ok &= expect_int(source_char_bones_find_offset(lookup_state, "bone_b.pos"),
                   12, "FindOffset second pos");
  ok &= expect_int(source_char_bones_find_offset(lookup_state, "bone_b.quat"),
                   52, "FindOffset second quat");
  ok &= expect_int(source_char_bones_find_offset(lookup_state, "bone_a.rotz"),
                   80, "FindOffset rotz");
  ok &= expect_int(source_char_bones_find_offset(lookup_state, "bone_missing.pos"),
                   -1, "FindOffset missing same type");
  ok &= expect_int(source_char_bones_find_offset(lookup_state, "bone_a"),
                   -1, "FindOffset invalid suffix");

  source_char_bones_clear(state);
  ok &= expect_empty_state(state, "ClearBones state reset");

  std::vector<SourceCharBonesBone> bones;
  bones.push_back({"bone_a.rotx", 1.0f});
  bones.push_back({"bone_b.roty", 2.0f});
  source_char_bones_set_weights(bones, 0.0f);
  ok &= expect_size(bones.size(), 2, "static SetWeights count");
  ok &= expect_float(bones[0].weight, 0.0f, "static SetWeights first");
  ok &= expect_float(bones[1].weight, 0.0f, "static SetWeights second");

  CharClip::OutputBone output;
  output.name = "bone_hand";
  output.position_context = 0x7;
  output.scale_context = 0x2;
  output.rotation_type = kSourceCharBonesTypeRotZ;
  output.rotation_context = 0x4;
  output.weights.push_back({0x2, 0.25f});
  output.weights.push_back({0x4, 0.75f});
  output.weights.push_back({0x6, 0.50f});
  const auto weight_scale = source_char_bone_find_weight_index(output, 0x2);
  ok &= expect_int(weight_scale ? static_cast<int>(*weight_scale) : -1, 0,
                   "FindWeight first matching context");
  const auto weight_rot = source_char_bone_find_weight_index(output, 0x4);
  ok &= expect_int(weight_rot ? static_cast<int>(*weight_rot) : -1, 1,
                   "FindWeight second matching context");
  const auto weight_missing = source_char_bone_find_weight_index(output, 0x8);
  ok &= expect_int(weight_missing ? 1 : 0, 0, "FindWeight missing context");
  ok &= expect_float(source_char_bone_get_weight(output, 0x2), 0.25f,
                     "GetWeight explicit");
  ok &= expect_float(source_char_bone_get_weight(output, 0x8), 1.0f,
                     "GetWeight default");
  std::vector<SourceCharBonesBone> stuffed;
  stuffed.push_back({"preexisting.quat", 0.125f});
  source_char_bone_stuff_bones(output, 0x4, stuffed);
  ok &= expect_size(stuffed.size(), 3, "StuffBones append count");
  ok &= expect_string(stuffed[0].name, "preexisting.quat",
                      "StuffBones preserves caller row");
  ok &= expect_string(stuffed[1].name, "bone_hand.pos",
                      "StuffBones position channel");
  ok &= expect_float(stuffed[1].weight, 0.75f,
                     "StuffBones position weight");
  ok &= expect_string(stuffed[2].name, "bone_hand.rotz",
                      "StuffBones rotation channel");
  ok &= expect_float(stuffed[2].weight, 0.75f,
                     "StuffBones rotation weight");
  source_char_bone_clear_context(output, 0x2);
  ok &= expect_int(output.position_context, 0x5, "ClearContext position");
  ok &= expect_int(output.scale_context, 0x0, "ClearContext scale");
  ok &= expect_int(output.rotation_context, 0x4, "ClearContext rotation");
  stuffed.clear();
  source_char_bone_stuff_bones(output, 0x2, stuffed);
  ok &= expect_size(stuffed.size(), 0, "StuffBones after clear");

  std::vector<CharClip::OutputBone> dir_output_bones;
  dir_output_bones.push_back(output);
  std::vector<SourceCharBonesBone> dir_bones;
  dir_bones.push_back({"preexisting.pos", 2.0f});
  source_char_bone_dir_list_bones(dir_output_bones, 0x1, 0x1, true,
                                  dir_bones);
  ok &= expect_size(dir_bones.size(), 6, "CharBoneDir ListBones count");
  ok &= expect_string(dir_bones[0].name, "preexisting.pos",
                      "CharBoneDir preserves caller row");
  ok &= expect_string(dir_bones[1].name, "bone_facing.pos",
                      "CharBoneDir facing pos");
  ok &= expect_float(dir_bones[1].weight, 1.0f,
                     "CharBoneDir facing pos weight");
  ok &= expect_string(dir_bones[2].name, "bone_facing.rotz",
                      "CharBoneDir facing rot");
  ok &= expect_string(dir_bones[3].name, "bone_facing_delta.pos",
                      "CharBoneDir delta pos");
  ok &= expect_string(dir_bones[4].name, "bone_facing_delta.rotz",
                      "CharBoneDir delta rot");
  ok &= expect_string(dir_bones[5].name, "bone_hand.pos",
                      "CharBoneDir delegated CharBone row");
  ok &= expect_float(dir_bones[5].weight, 1.0f,
                     "CharBoneDir delegated default weight");
  dir_bones.clear();
  source_char_bone_dir_list_bones(dir_output_bones, 0x1, 0x4, false,
                                  dir_bones);
  ok &= expect_size(dir_bones.size(), 2, "CharBoneDir no facing count");
  ok &= expect_string(dir_bones[0].name, "bone_hand.pos",
                      "CharBoneDir delegated position with no facing");
  ok &= expect_string(dir_bones[1].name, "bone_hand.rotz",
                      "CharBoneDir delegated rotation with no facing");

  SourceCharBonesSamplesState samples =
      source_char_bones_samples_empty_state();
  ok &= expect_int(samples.num_samples, 0, "samples default num");
  ok &= expect_int(samples.preview_sample, 0, "samples default preview");
  ok &= expect_int(samples.start_offset, 0, "samples default start");
  ok &= expect_int(source_char_bones_samples_allocate_size(samples), 0,
                   "samples default allocation");
  ok &= expect_int(source_char_bones_samples_set_preview(samples, 2) ? 1 : 0, 0,
                   "samples empty preview rejected");
  samples.bones.layout.total_size = 32;
  samples.num_samples = 4;
  ok &= expect_int(source_char_bones_samples_allocate_size(samples), 128,
                   "samples allocation");
  ok &= expect_int(source_char_bones_samples_set_preview(samples, 2) ? 1 : 0, 1,
                   "samples preview accepted");
  ok &= expect_int(samples.preview_sample, 2, "samples preview middle");
  ok &= expect_int(samples.start_offset, 64, "samples preview offset");
  ok &= expect_int(source_char_bones_samples_set_preview(samples, -3) ? 1 : 0,
                   1, "samples preview low accepted");
  ok &= expect_int(samples.preview_sample, 0, "samples preview low clamp");
  ok &= expect_int(samples.start_offset, 0, "samples preview low offset");
  ok &= expect_int(source_char_bones_samples_set_preview(samples, 99) ? 1 : 0,
                   1, "samples preview high accepted");
  ok &= expect_int(samples.preview_sample, 3, "samples preview high clamp");
  ok &= expect_int(samples.start_offset, 96, "samples preview high offset");
  const std::vector<SourceCharBonesSampleStep> one_step =
      source_char_bones_samples_split_steps(samples, 1, 0.8f, 0.0f);
  ok &= expect_size(one_step.size(), 1, "samples split single count");
  ok &= expect_int(one_step[0].start_offset, 32, "samples split single offset");
  ok &= expect_float(one_step[0].weight, 0.8f, "samples split single weight");
  const std::vector<SourceCharBonesSampleStep> two_steps =
      source_char_bones_samples_split_steps(samples, 1, 1.0f, 0.25f);
  ok &= expect_size(two_steps.size(), 2, "samples split blended count");
  ok &= expect_int(two_steps[0].start_offset, 32,
                   "samples split blended first offset");
  ok &= expect_float(two_steps[0].weight, 0.75f,
                     "samples split blended first weight");
  ok &= expect_int(two_steps[1].start_offset, 64,
                   "samples split blended second offset");
  ok &= expect_float(two_steps[1].weight, 0.25f,
                     "samples split blended second weight");
  const std::vector<SourceCharBonesSampleStep> rotate_steps =
      source_char_bones_samples_rotate_to_steps(samples, 2, 2.0f, 0.25f);
  ok &= expect_size(rotate_steps.size(), 2, "samples RotateTo count");
  ok &= expect_int(rotate_steps[0].start_offset, 64,
                   "samples RotateTo first offset");
  ok &= expect_float(rotate_steps[0].weight, 1.5f,
                     "samples RotateTo first angle");
  ok &= expect_int(rotate_steps[1].start_offset, 96,
                   "samples RotateTo second offset");
  ok &= expect_float(rotate_steps[1].weight, 0.5f,
                     "samples RotateTo second angle");
  const std::vector<SourceCharBonesSampleStep> scale_steps =
      source_char_bones_samples_scale_add_steps(samples, 0, 0.5f, 0.0f);
  ok &= expect_size(scale_steps.size(), 1, "samples ScaleAddSample count");
  ok &= expect_int(scale_steps[0].start_offset, 0,
                   "samples ScaleAddSample first offset");
  ok &= expect_float(scale_steps[0].weight, 0.5f,
                     "samples ScaleAddSample first weight");
  ok &= expect_int(source_char_bones_samples_load_version_known(12) ? 1 : 0, 0,
                   "samples load version low rejected");
  ok &= expect_int(source_char_bones_samples_load_version_known(13) ? 1 : 0, 1,
                   "samples load version 13 accepted");
  ok &= expect_int(source_char_bones_samples_load_version_known(16) ? 1 : 0, 1,
                   "samples load version 16 accepted");
  ok &= expect_int(source_char_bones_samples_load_version_known(17) ? 1 : 0, 0,
                   "samples load version high rejected");
  ok &= expect_int(source_char_bones_samples_set_ver_known(12) ? 1 : 0, 1,
                   "samples SetVer legacy 12 accepted");
  ok &= expect_int(source_char_bones_samples_set_ver_known(13) ? 1 : 0, 0,
                   "samples SetVer source 13 rejected");

  return ok ? 0 : 1;
}
