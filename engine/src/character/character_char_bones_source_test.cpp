#include "character/char_clip.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
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

bool expect_near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << ": got " << got << " want near " << want << "\n";
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

bool expect_zeroed_prefix(const std::vector<uint8_t>& bytes, size_t zero_count,
                          uint8_t tail_value, const char* label) {
  bool ok = true;
  for (size_t i = 0; i < zero_count; ++i) {
    if (bytes[i] != 0) {
      std::cerr << label << " byte[" << i << "]: got "
                << static_cast<int>(bytes[i]) << " want 0\n";
      ok = false;
      break;
    }
  }
  for (size_t i = zero_count; i < bytes.size(); ++i) {
    if (bytes[i] != tail_value) {
      std::cerr << label << " tail[" << i << "]: got "
                << static_cast<int>(bytes[i]) << " want "
                << static_cast<int>(tail_value) << "\n";
      ok = false;
      break;
    }
  }
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
  const SourceCharBonesAddBonesSteps add_steps =
      source_char_bones_add_bones_steps(listed);
  ok &= expect_size(add_steps.add_bone_internal_calls.size(), 3,
                    "AddBones call count");
  ok &= expect_string(add_steps.add_bone_internal_calls[1].name,
                      "bone_head.quat", "AddBones first source row");
  ok &= expect_float(add_steps.add_bone_internal_calls[2].weight, 0.5f,
                     "AddBones second source weight");
  ok &= expect_int(add_steps.reallocate_internal ? 1 : 0, 1,
                   "AddBones reallocates");
  const SourceCharBonesAddBonesSteps empty_add_steps =
      source_char_bones_add_bones_steps({});
  ok &= expect_size(empty_add_steps.add_bone_internal_calls.size(), 0,
                    "AddBones empty call count");
  ok &= expect_int(empty_add_steps.reallocate_internal ? 1 : 0, 1,
                   "AddBones empty reallocates");

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
  const SourceCharBonesFindPtrResult find_ptr_hit =
      source_char_bones_find_ptr(lookup_state, "bone_b.quat");
  ok &= expect_int(find_ptr_hit.found ? 1 : 0, 1, "FindPtr hit found");
  ok &= expect_int(find_ptr_hit.offset, 52, "FindPtr hit offset");
  const SourceCharBonesFindPtrResult find_ptr_missing =
      source_char_bones_find_ptr(lookup_state, "bone_missing.quat");
  ok &= expect_int(find_ptr_missing.found ? 1 : 0, 0,
                   "FindPtr missing found");
  ok &= expect_int(find_ptr_missing.offset, -1, "FindPtr missing offset");
  std::vector<uint8_t> raw_bytes(
      static_cast<size_t>(lookup_state.layout.total_size + 4), uint8_t{0xAB});
  source_char_bones_zero(raw_bytes, lookup_state.layout.total_size);
  ok &= expect_zeroed_prefix(raw_bytes,
                             static_cast<size_t>(lookup_state.layout.total_size),
                             uint8_t{0xAB}, "Zero byte span");
  const SourceCharBonesScaleAddClipStep scale_add_clip =
      source_char_bones_scale_add_clip_step(0.25f, 12.5f, 0.75f);
  ok &= expect_int(scale_add_clip.call_clip_scale_add ? 1 : 0, 1,
                   "ScaleAdd clip delegation");
  ok &= expect_float(scale_add_clip.f1, 0.25f, "ScaleAdd f1");
  ok &= expect_float(scale_add_clip.f2, 12.5f, "ScaleAdd f2");
  ok &= expect_float(scale_add_clip.f3, 0.75f, "ScaleAdd f3");
  const SourceCharBonesAllocReallocateStep realloc_step =
      source_char_bones_alloc_reallocate_step(lookup_state.layout.total_size);
  ok &= expect_int(realloc_step.free_m_start ? 1 : 0, 1,
                   "ReallocateInternal frees mStart");
  ok &= expect_int(realloc_step.mem_alloc_size, 96,
                   "ReallocateInternal alloc size");
  ok &= expect_int(realloc_step.assign_m_start ? 1 : 0, 1,
                   "ReallocateInternal assigns mStart");
  const SourceCharBonesEnterStep enter_step = source_char_bones_enter_step();
  ok &= expect_int(enter_step.zero ? 1 : 0, 1, "CharBones Enter zeros");
  ok &= expect_int(enter_step.set_weights ? 1 : 0, 1,
                   "CharBones Enter sets weights");
  ok &= expect_float(enter_step.set_weights_value, 0.0f,
                     "CharBones Enter weight value");
  const SourceCharBonesBlenderPollStep poll_empty =
      source_char_bones_blender_poll_step(true, true);
  ok &= expect_int(poll_empty.early_out ? 1 : 0, 1,
                   "CharBonesBlender Poll empty out");
  ok &= expect_int(poll_empty.blend_dest ? 1 : 0, 0,
                   "CharBonesBlender Poll empty no blend");
  const SourceCharBonesBlenderPollStep poll_no_dest =
      source_char_bones_blender_poll_step(false, false);
  ok &= expect_int(poll_no_dest.early_out ? 1 : 0, 1,
                   "CharBonesBlender Poll no dest out");
  const SourceCharBonesBlenderPollStep poll_active =
      source_char_bones_blender_poll_step(false, true);
  ok &= expect_int(poll_active.early_out ? 1 : 0, 0,
                   "CharBonesBlender Poll active no out");
  ok &= expect_int(poll_active.blend_dest ? 1 : 0, 1,
                   "CharBonesBlender Poll blends dest");
  ok &= expect_int(poll_active.enter ? 1 : 0, 1,
                   "CharBonesBlender Poll enters after blend");
  const SourceCharBonesBlenderSetDestStep same_dest =
      source_char_bones_blender_set_dest_step(false, true);
  ok &= expect_int(same_dest.changed ? 1 : 0, 0,
                   "CharBonesBlender SetDest unchanged");
  const SourceCharBonesBlenderSetDestStep null_dest =
      source_char_bones_blender_set_dest_step(true, false);
  ok &= expect_int(null_dest.changed ? 1 : 0, 1,
                   "CharBonesBlender SetDest null changed");
  ok &= expect_int(null_dest.assign_dest ? 1 : 0, 1,
                   "CharBonesBlender SetDest null assigns");
  ok &= expect_int(null_dest.add_bones_to_dest ? 1 : 0, 0,
                   "CharBonesBlender SetDest null no add");
  const SourceCharBonesBlenderSetDestStep new_dest =
      source_char_bones_blender_set_dest_step(true, true);
  ok &= expect_int(new_dest.add_bones_to_dest ? 1 : 0, 1,
                   "CharBonesBlender SetDest adds bones");
  const SourceCharBonesBlenderSetClipTypeStep same_clip_type =
      source_char_bones_blender_set_clip_type_step(false);
  ok &= expect_int(same_clip_type.changed ? 1 : 0, 0,
                   "CharBonesBlender SetClipType unchanged");
  const SourceCharBonesBlenderSetClipTypeStep new_clip_type =
      source_char_bones_blender_set_clip_type_step(true);
  ok &= expect_int(new_clip_type.assign_clip_type ? 1 : 0, 1,
                   "CharBonesBlender SetClipType assigns");
  ok &= expect_int(new_clip_type.clear_bones ? 1 : 0, 1,
                   "CharBonesBlender SetClipType clears bones");
  ok &= expect_int(new_clip_type.stuff_bones_from_dir ? 1 : 0, 1,
                   "CharBonesBlender SetClipType stuffs bones");
  const SourceCharBonesBlenderReallocateStep blender_realloc_no_dest =
      source_char_bones_blender_reallocate_step(false);
  ok &= expect_int(blender_realloc_no_dest.char_bones_alloc_reallocate_internal
                       ? 1
                       : 0,
                   1, "CharBonesBlender realloc calls base alloc");
  ok &= expect_int(blender_realloc_no_dest.add_bones_to_dest ? 1 : 0, 0,
                   "CharBonesBlender realloc no dest no add");
  ok &= expect_int(blender_realloc_no_dest.enter ? 1 : 0, 1,
                   "CharBonesBlender realloc enters no dest");
  const SourceCharBonesBlenderReallocateStep blender_realloc_dest =
      source_char_bones_blender_reallocate_step(true);
  ok &= expect_int(blender_realloc_dest.add_bones_to_dest ? 1 : 0, 1,
                   "CharBonesBlender realloc dest add");

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
  output.parent = "decoder.parent";
  output.char_bone_version = 10;
  output.trans_version = 7;
  output.trans_constraint = 3;
  output.trans_target = "decoder.target";
  output.preserve_scale = true;
  output.legacy_pre_rev5_int = 11;
  output.has_legacy_pre_rev5_int = true;
  output.legacy_rev3_to_7_int = 12;
  output.has_legacy_rev3_to_7_int = true;
  output.target = "target.trans";
  output.trans = "bone_hand.trans";
  output.bake_out_as_top_level = true;
  output.unread_bytes = 99;
  const auto copied_output = source_char_bone_copy_members(output);
  ok &= expect_int(copied_output.rotation_context, 0x4,
                   "CharBone copy rotation context");
  ok &= expect_int(copied_output.scale_context, 0x2,
                   "CharBone copy scale context");
  ok &= expect_int(copied_output.position_context, 0x7,
                   "CharBone copy position context");
  ok &= expect_int(copied_output.rotation_type, kSourceCharBonesTypeRotZ,
                   "CharBone copy rotation type");
  ok &= expect_string(copied_output.target, "target.trans",
                      "CharBone copy target");
  ok &= expect_size(copied_output.weights.size(), 3,
                    "CharBone copy weight count");
  ok &= expect_int(copied_output.weights[1].context, 0x4,
                   "CharBone copy weight context");
  ok &= expect_float(copied_output.weights[1].weight, 0.75f,
                     "CharBone copy weight value");
  ok &= expect_string(copied_output.trans, "bone_hand.trans",
                      "CharBone copy trans");
  ok &= expect_int(copied_output.bake_out_as_top_level ? 1 : 0, 1,
                   "CharBone copy bake flag");
  ok &= expect_string(copied_output.parent, "",
                      "CharBone copy resets decoder parent");
  ok &= expect_int(static_cast<int>(copied_output.char_bone_version), 0,
                   "CharBone copy resets decoder revision");
  ok &= expect_int(copied_output.trans_constraint, 0,
                   "CharBone copy resets transform constraint");
  ok &= expect_string(copied_output.trans_target, "",
                      "CharBone copy resets transform target");
  ok &= expect_int(copied_output.preserve_scale ? 1 : 0, 0,
                   "CharBone copy resets preserve scale");
  ok &= expect_int(copied_output.has_legacy_pre_rev5_int ? 1 : 0, 0,
                   "CharBone copy resets legacy pre-rev5 marker");
  ok &= expect_int(copied_output.has_legacy_rev3_to_7_int ? 1 : 0, 0,
                   "CharBone copy resets legacy rev3-7 marker");
  ok &= expect_int(static_cast<int>(copied_output.unread_bytes), 0,
                   "CharBone copy resets unread bytes");
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

  constexpr float kHalfPi = 1.57079632679489661923f;
  std::array<float, 3> facing_pos_delta = {4.0f, 5.0f, 6.0f};
  float facing_rot_delta = 0.25f;
  source_char_servo_bone_zero_deltas(facing_pos_delta, facing_rot_delta);
  ok &= expect_near(facing_pos_delta[0], 0.0f,
                    "CharServoBone ZeroDeltas pos x");
  ok &= expect_near(facing_pos_delta[1], 0.0f,
                    "CharServoBone ZeroDeltas pos y");
  ok &= expect_near(facing_pos_delta[2], 0.0f,
                    "CharServoBone ZeroDeltas pos z");
  ok &= expect_near(facing_rot_delta, 0.0f,
                    "CharServoBone ZeroDeltas rot");

  ghogx::milo_scene::Xfm facing_xfm;
  facing_xfm.pos[0] = 1.0f;
  facing_xfm.pos[1] = 0.0f;
  facing_xfm.pos[2] = 2.0f;
  source_char_servo_bone_move_to_facing(
      facing_xfm, {10.0f, 20.0f, 30.0f}, kHalfPi);
  ok &= expect_near(facing_xfm.pos[0], 10.0f,
                    "CharServoBone MoveToFacing pos x");
  ok &= expect_near(facing_xfm.pos[1], 21.0f,
                    "CharServoBone MoveToFacing pos y");
  ok &= expect_near(facing_xfm.pos[2], 32.0f,
                    "CharServoBone MoveToFacing pos z");
  ok &= expect_near(facing_xfm.rot[0][0], 0.0f,
                    "CharServoBone MoveToFacing rot00");
  ok &= expect_near(facing_xfm.rot[0][1], 1.0f,
                    "CharServoBone MoveToFacing rot01");
  ok &= expect_near(facing_xfm.rot[1][0], -1.0f,
                    "CharServoBone MoveToFacing rot10");
  ok &= expect_near(facing_xfm.rot[1][1], 0.0f,
                    "CharServoBone MoveToFacing rot11");

  ghogx::milo_scene::Xfm delta_xfm;
  delta_xfm.pos[0] = 1.0f;
  delta_xfm.pos[1] = 2.0f;
  delta_xfm.pos[2] = 3.0f;
  source_char_servo_bone_move_to_delta_facing(
      delta_xfm, {4.0f, 5.0f, 6.0f}, kHalfPi);
  ok &= expect_near(delta_xfm.pos[0], 5.0f,
                    "CharServoBone MoveToDeltaFacing pos x");
  ok &= expect_near(delta_xfm.pos[1], 7.0f,
                    "CharServoBone MoveToDeltaFacing pos y");
  ok &= expect_near(delta_xfm.pos[2], 9.0f,
                    "CharServoBone MoveToDeltaFacing pos z");
  ok &= expect_near(delta_xfm.rot[0][0], 0.0f,
                    "CharServoBone MoveToDeltaFacing rot00");
  ok &= expect_near(delta_xfm.rot[0][1], 1.0f,
                    "CharServoBone MoveToDeltaFacing rot01");
  ok &= expect_near(delta_xfm.rot[1][0], -1.0f,
                    "CharServoBone MoveToDeltaFacing rot10");
  ok &= expect_near(delta_xfm.rot[1][1], 0.0f,
                    "CharServoBone MoveToDeltaFacing rot11");

  SourceCharBonesSamplesState samples =
      source_char_bones_samples_empty_state();
  ok &= expect_int(samples.num_samples, 0, "samples default num");
  ok &= expect_int(samples.preview_sample, 0, "samples default preview");
  ok &= expect_int(samples.start_offset, 0, "samples default start");
  ok &= expect_int(samples.raw_data_size, 0, "samples default raw data size");
  ok &= expect_int(source_char_bones_samples_allocate_size(samples), 0,
                   "samples default allocation");
  ok &= expect_int(source_char_bones_samples_set_preview(samples, 2) ? 1 : 0, 0,
                   "samples empty preview rejected");
  SourceCharBonesState prepared_bones = source_char_bones_empty_state();
  prepared_bones.bones.push_back({"bone_head.pos", 0.75f});
  prepared_bones.bones.push_back({"bone_head.rotz", 0.25f});
  prepared_bones.layout.counts = {0, 1, 1, 1, 1, 1, 2};
  prepared_bones.layout = source_char_bones_recompute_layout(
      prepared_bones.layout.counts, 0);
  constexpr int kCompressRots = 1;
  source_char_bones_samples_set(samples, prepared_bones, 3, kCompressRots);
  ok &= expect_int(samples.num_samples, 3, "samples Set num");
  ok &= expect_int(samples.bones.compression, kCompressRots,
                   "samples Set compression");
  ok &= expect_size(samples.bones.bones.size(), 2, "samples Set bone count");
  ok &= expect_string(samples.bones.bones[0].name, "bone_head.pos",
                      "samples Set first bone");
  ok &= expect_float(samples.bones.bones[0].weight, 0.75f,
                     "samples Set first weight");
  ok &= expect_int(samples.bones.layout.total_size, 16,
                   "samples Set recomputed layout");
  ok &= expect_int(samples.raw_data_size, 48, "samples Set raw data size");
  ok &= expect_size(samples.frames.size(), 0, "samples Set clears frames");
  samples.frames = {1.0f, 2.0f, 3.0f};
  const SourceCharBonesSamplesState cloned =
      source_char_bones_samples_clone(samples);
  ok &= expect_int(cloned.num_samples, 3, "samples Clone num");
  ok &= expect_int(cloned.bones.compression, kCompressRots,
                   "samples Clone compression");
  ok &= expect_size(cloned.bones.bones.size(), 2, "samples Clone bone count");
  ok &= expect_int(cloned.raw_data_size, 48, "samples Clone raw data size");
  ok &= expect_size(cloned.frames.size(), 3, "samples Clone frames count");
  ok &= expect_float(cloned.frames[1], 2.0f, "samples Clone frame value");
  samples.bones.layout.total_size = 32;
  samples.num_samples = 4;
  samples.raw_data_size = source_char_bones_samples_allocate_size(samples);
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
  ok &= expect_int(source_char_bones_samples_rotate_by_offset(samples, 3), 96,
                   "samples RotateBy offset");
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
