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

const ghogx::character::SourceCharPoseRuntimeSymbolEvidence* find_symbol(
    const std::vector<ghogx::character::SourceCharPoseRuntimeSymbolEvidence>&
        symbols,
    const std::string& symbol) {
  for (const auto& entry : symbols) {
    if (entry.symbol == symbol) return &entry;
  }
  return nullptr;
}

bool expect_symbol(
    const std::vector<ghogx::character::SourceCharPoseRuntimeSymbolEvidence>&
        symbols,
    const std::string& symbol,
    const std::string& address,
    uint32_t size,
    const char* label) {
  const auto* entry = find_symbol(symbols, symbol);
  if (!entry) {
    std::cerr << label << ": missing symbol " << symbol << "\n";
    return false;
  }
  bool ok = true;
  ok &= expect_string(entry->source, "rb3/config/SZBE69_B8/symbols.txt",
                      label);
  ok &= expect_string(entry->address, address, label);
  ok &= expect_int(static_cast<int>(entry->size), static_cast<int>(size),
                   label);
  ok &= expect_int(entry->has_statement_body ? 1 : 0, 0, label);
  ok &= expect_int(entry->safe_to_import_runtime ? 1 : 0, 0, label);
  return ok;
}

bool expect_vec3_near(const std::array<float, 3>& got,
                      const std::array<float, 3>& want,
                      const char* label) {
  bool ok = true;
  ok &= expect_near(got[0], want[0], label);
  ok &= expect_near(got[1], want[1], label);
  ok &= expect_near(got[2], want[2], label);
  return ok;
}

bool expect_vec4_near(const std::array<float, 4>& got,
                      const std::array<float, 4>& want,
                      const char* label) {
  bool ok = true;
  ok &= expect_near(got[0], want[0], label);
  ok &= expect_near(got[1], want[1], label);
  ok &= expect_near(got[2], want[2], label);
  ok &= expect_near(got[3], want[3], label);
  return ok;
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
  ok &= expect_int(source_char_bones_type_of("bone.head.pos"),
                   kSourceCharBonesTypePos, "type scans later dot");
  ok &= expect_int(source_char_bones_type_of("bone.head.rotz"),
                   kSourceCharBonesTypeRotZ, "type scans later rot suffix");
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
  const SourceCharBonesPoseBodyBoundary pose_boundary =
      source_char_bones_pose_body_boundary();
  ok &= expect_int(pose_boundary.rb3_latest_declares_scale_add ? 1 : 0, 1,
                   "CharBones pose rb3 declares ScaleAdd");
  ok &= expect_int(pose_boundary.rb3_latest_exposes_scale_add_body ? 1 : 0,
                   0, "CharBones pose rb3 lacks ScaleAdd body");
  ok &= expect_int(pose_boundary.rb2_dump_maps_rotate_to ? 1 : 0, 1,
                   "CharBones pose rb2 maps RotateTo");
  ok &= expect_int(pose_boundary.rb2_dump_exposes_statement_body ? 1 : 0, 0,
                   "CharBones pose rb2 lacks statement body");
  ok &= expect_int(pose_boundary.safe_to_use_layout_helpers ? 1 : 0, 1,
                   "CharBones pose boundary allows layout helpers");
  ok &= expect_int(pose_boundary.safe_to_apply_pose_math ? 1 : 0, 0,
                   "CharBones pose boundary fences pose math");
  ok &= expect_size(pose_boundary.fenced_bodies.size(), 6,
                    "CharBones pose fenced count");
  ok &= expect_string(pose_boundary.fenced_bodies[0],
                      "CharBones::ScaleAdd(CharBones&, float)",
                      "CharBones pose first fenced body");
  ok &= expect_string(pose_boundary.fenced_bodies[3],
                      "CharBones::Blend",
                      "CharBones pose blend fenced");
  const SourceCharBonesRuntimeDumpEvidence bones_dump =
      source_char_bones_runtime_dump_evidence();
  ok &= expect_string(bones_dump.scale_down_range,
                      "0x8031C058 -> 0x8031C33C",
                      "CharBones dump ScaleDown range");
  ok &= expect_string(bones_dump.scale_add_range,
                      "0x8031C33C -> 0x8031CB00",
                      "CharBones dump ScaleAdd range");
  ok &= expect_string(bones_dump.rotate_by_range,
                      "0x8031CB00 -> 0x8031D118",
                      "CharBones dump RotateBy range");
  ok &= expect_string(bones_dump.rotate_to_range,
                      "0x8031D118 -> 0x8031D864",
                      "CharBones dump RotateTo range");
  ok &= expect_string(bones_dump.scale_add_identity_range,
                      "0x8031D864 -> 0x8031D8B0",
                      "CharBones dump ScaleAddIdentity range");
  ok &= expect_string(bones_dump.blend_range,
                      "0x8031F2C0 -> 0x8031F670",
                      "CharBones dump Blend range");
  ok &= expect_size(bones_dump.scale_down_locals.size(), 9,
                    "CharBones dump ScaleDown locals");
  ok &= expect_string(bones_dump.scale_down_locals[2],
                      "const Bone* boneName",
                      "CharBones dump ScaleDown boneName local");
  ok &= expect_size(bones_dump.scale_add_locals.size(), 23,
                    "CharBones dump ScaleAdd locals");
  ok &= expect_string(bones_dump.scale_add_locals[4],
                      "const ShortVector3* sp",
                      "CharBones dump ScaleAdd short vector local");
  ok &= expect_string(bones_dump.scale_add_locals[20], "float aweight",
                      "CharBones dump ScaleAdd angle weight local");
  ok &= expect_size(bones_dump.rotate_by_locals.size(), 16,
                    "CharBones dump RotateBy locals");
  ok &= expect_string(bones_dump.rotate_by_locals[8],
                      "const ByteQuat* bq",
                      "CharBones dump RotateBy byte quat local");
  ok &= expect_size(bones_dump.rotate_to_locals.size(), 18,
                    "CharBones dump RotateTo locals");
  ok &= expect_string(bones_dump.rotate_to_locals[15], "float shortWeight",
                      "CharBones dump RotateTo short weight local");
  ok &= expect_size(bones_dump.scale_add_identity_locals.size(), 2,
                    "CharBones dump ScaleAddIdentity locals");
  ok &= expect_int(bones_dump.rb2_dump_maps_blend ? 1 : 0, 1,
                   "CharBones dump maps Blend");
  ok &= expect_size(bones_dump.blend_locals.size(), 12,
                    "CharBones dump Blend locals");
  ok &= expect_string(bones_dump.blend_locals[7], "float ds",
                      "CharBones dump Blend delta-scale local");
  ok &= expect_int(bones_dump.has_scale_down_statement_body ? 1 : 0, 0,
                   "CharBones dump lacks ScaleDown statement body");
  ok &= expect_int(bones_dump.has_scale_add_statement_body ? 1 : 0, 0,
                   "CharBones dump lacks ScaleAdd statement body");
  ok &= expect_int(bones_dump.has_rotate_by_statement_body ? 1 : 0, 0,
                   "CharBones dump lacks RotateBy statement body");
  ok &= expect_int(bones_dump.has_rotate_to_statement_body ? 1 : 0, 0,
                   "CharBones dump lacks RotateTo statement body");
  ok &= expect_int(bones_dump.safe_to_apply_pose_math ? 1 : 0, 0,
                   "CharBones dump fences pose math");
  const auto runtime_symbols = source_char_pose_runtime_symbol_evidence();
  ok &= expect_size(runtime_symbols.size(), 20,
                    "pose runtime symbol inventory count");
  ok &= expect_symbol(runtime_symbols,
                      "ScaleAdd__9CharBonesCFR9CharBonesf",
                      "0x80689780", 0x8E8u,
                      "pose symbol CharBones ScaleAdd");
  ok &= expect_symbol(runtime_symbols,
                      "PoseMeshes__15CharBonesMeshesFv",
                      "0x8068E700", 0x564u,
                      "pose symbol CharBonesMeshes PoseMeshes");
  ok &= expect_symbol(runtime_symbols,
                      "EvaluateChannel__16CharBonesSamplesFPviif",
                      "0x80690180", 0x75Cu,
                      "pose symbol CharBonesSamples EvaluateChannel");
  ok &= expect_symbol(runtime_symbols,
                      "Relativize__16CharBonesSamplesFP8CharClip",
                      "0x80690AA0", 0x105Cu,
                      "pose symbol CharBonesSamples Relativize");
  ok &= expect_symbol(runtime_symbols,
                      "Evaluate__14CharClipDriverFfff",
                      "0x806A02F0", 0x560u,
                      "pose symbol CharClipDriver Evaluate");
  ok &= expect_symbol(runtime_symbols,
                      "EvaluateFlags__10CharDriverFi",
                      "0x806B3960", 0x1C8u,
                      "pose symbol CharDriver EvaluateFlags");
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
  const SourceCharBonesBlenderLoadPlan blender_load_v1 =
      source_char_bones_blender_load_plan(1);
  ok &= expect_int(blender_load_v1.known_revision ? 1 : 0, 1,
                   "CharBonesBlender Load v1 known");
  ok &= expect_size(blender_load_v1.read_order.size(), 2,
                    "CharBonesBlender Load v1 row count");
  ok &= expect_string(blender_load_v1.read_order[0], "Hmx::Object",
                      "CharBonesBlender Load v1 object");
  ok &= expect_string(blender_load_v1.read_order[1], "boneObjPtr",
                      "CharBonesBlender Load v1 dest ptr");
  ok &= expect_string(blender_load_v1.branches[0],
                      "mClipType defaults empty",
                      "CharBonesBlender Load v1 clip type default");
  ok &= expect_string(blender_load_v1.call_order[0], "SetClipType",
                      "CharBonesBlender Load call order first");
  ok &= expect_string(blender_load_v1.call_order[1], "SetDest",
                      "CharBonesBlender Load call order second");
  const SourceCharBonesBlenderLoadPlan blender_load_v2 =
      source_char_bones_blender_load_plan(2);
  ok &= expect_int(blender_load_v2.known_revision ? 1 : 0, 1,
                   "CharBonesBlender Load v2 known");
  ok &= expect_size(blender_load_v2.read_order.size(), 3,
                    "CharBonesBlender Load v2 row count");
  ok &= expect_string(blender_load_v2.read_order[2], "mClipType",
                      "CharBonesBlender Load v2 clip type row");
  ok &= expect_int(source_char_bones_blender_load_plan(3).known_revision ? 1
                                                                         : 0,
                   0, "CharBonesBlender Load rejects high revision");
  const SourceCharBonesBlenderSavePlan blender_save =
      source_char_bones_blender_save_plan();
  ok &= expect_int(blender_save.save_id, 0x58,
                   "CharBonesBlender SAVE_OBJ id");
  const SourceCharBonesBlenderCopyPlan blender_copy =
      source_char_bones_blender_copy_plan();
  ok &= expect_size(blender_copy.copied_superclasses.size(), 1,
                    "CharBonesBlender Copy superclass count");
  ok &= expect_string(blender_copy.copied_superclasses[0], "Hmx::Object",
                      "CharBonesBlender Copy object superclass");
  ok &= expect_string(blender_copy.member_calls[0], "SetClipType",
                      "CharBonesBlender Copy first setter");
  ok &= expect_string(blender_copy.member_calls[1], "SetDest",
                      "CharBonesBlender Copy second setter");
  const SourceCharBonesBlenderHandlerPlan blender_handlers =
      source_char_bones_blender_handler_plan();
  ok &= expect_size(blender_handlers.superclasses.size(), 2,
                    "CharBonesBlender handler superclass count");
  ok &= expect_string(blender_handlers.superclasses[0], "CharPollable",
                      "CharBonesBlender handler pollable superclass");
  ok &= expect_string(blender_handlers.superclasses[1], "Hmx::Object",
                      "CharBonesBlender handler object superclass");
  ok &= expect_int(blender_handlers.check, 0x81,
                   "CharBonesBlender handler check");
  const SourceCharBonesBlenderPropSyncPlan blender_props =
      source_char_bones_blender_prop_sync_plan();
  ok &= expect_size(blender_props.set_properties.size(), 2,
                    "CharBonesBlender prop sync set count");
  ok &= expect_string(blender_props.set_properties[0], "dest",
                      "CharBonesBlender prop sync dest");
  ok &= expect_string(blender_props.set_properties[1], "clip_type",
                      "CharBonesBlender prop sync clip type");
  ok &= expect_string(blender_props.superclasses[0], "CharBonesObject",
                      "CharBonesBlender prop sync superclass");
  SourceCharBonesBlenderPollDeps blender_deps;
  source_char_bones_blender_poll_deps(blender_deps, "");
  ok &= expect_size(blender_deps.change.size(), 1,
                    "CharBonesBlender PollDeps always publishes dest row");
  ok &= expect_string(blender_deps.change[0], "",
                      "CharBonesBlender PollDeps preserves empty dest row");
  ok &= expect_size(blender_deps.changed_by.size(), 0,
                    "CharBonesBlender PollDeps has no changed-by rows");
  blender_deps = SourceCharBonesBlenderPollDeps{};
  source_char_bones_blender_poll_deps(blender_deps, "bones.dest");
  ok &= expect_size(blender_deps.change.size(), 1,
                    "CharBonesBlender PollDeps resolved change count");
  ok &= expect_string(blender_deps.change[0], "bones.dest",
                      "CharBonesBlender PollDeps resolved dest row");

  const SourceCharBoneLoadPlan char_bone_v1 = source_char_bone_load_plan(1);
  ok &= expect_int(char_bone_v1.known_revision ? 1 : 0, 1,
                   "CharBone Load v1 known");
  ok &= expect_string(char_bone_v1.read_order[0], "Hmx::Object",
                      "CharBone Load v1 object row");
  ok &= expect_string(char_bone_v1.read_order[1],
                      "RndTransformableRemover",
                      "CharBone Load v1 transform remover");
  ok &= expect_string(char_bone_v1.read_order[2],
                      "mPositionContextBool",
                      "CharBone Load v1 position bool");
  ok &= expect_string(char_bone_v1.read_order[4], "legacyPreRev5Int",
                      "CharBone Load v1 legacy int");
  ok &= expect_string(char_bone_v1.branches[0], "mScaleContext=0",
                      "CharBone Load v1 scale default");
  ok &= expect_string(char_bone_v1.branches[1], "mRotation=mRotation+1",
                      "CharBone Load v1 rotation bump");

  const SourceCharBoneLoadPlan char_bone_v6 = source_char_bone_load_plan(6);
  ok &= expect_string(char_bone_v6.read_order[3], "mScaleContextBool",
                      "CharBone Load v6 scale bool");
  ok &= expect_string(char_bone_v6.read_order[5], "legacyRev3To7Int",
                      "CharBone Load v6 legacy int");
  ok &= expect_string(char_bone_v6.read_order[6], "mTarget",
                      "CharBone Load v6 target");
  ok &= expect_string(char_bone_v6.read_order[7], "sharedContext",
                      "CharBone Load v6 shared context");
  ok &= expect_string(char_bone_v6.branches.back(),
                      "nonzeroContextsUseSharedContext",
                      "CharBone Load v6 shared branch");

  const SourceCharBoneLoadPlan char_bone_v10 = source_char_bone_load_plan(10);
  ok &= expect_size(char_bone_v10.read_order.size(), 9,
                    "CharBone Load v10 row count");
  ok &= expect_string(char_bone_v10.read_order[1], "mPositionContext",
                      "CharBone Load v10 position context");
  ok &= expect_string(char_bone_v10.read_order[4], "mRotationContext",
                      "CharBone Load v10 rotation context");
  ok &= expect_string(char_bone_v10.read_order.back(),
                      "mBakeOutAsTopLevel",
                      "CharBone Load v10 bake flag");
  ok &= expect_int(source_char_bone_load_plan(11).known_revision ? 1 : 0, 0,
                   "CharBone Load rejects high revision");
  ok &= expect_int(source_char_bone_save_plan().save_id, 0xBF,
                   "CharBone save id");

  const SourceCharBoneCopyPlan char_bone_copy_plan =
      source_char_bone_copy_plan();
  ok &= expect_string(char_bone_copy_plan.copied_superclasses[0],
                      "Hmx::Object",
                      "CharBone Copy object superclass");
  ok &= expect_string(char_bone_copy_plan.copied_members[0],
                      "mRotationContext",
                      "CharBone Copy first member");
  ok &= expect_string(char_bone_copy_plan.copied_members.back(),
                      "mBakeOutAsTopLevel",
                      "CharBone Copy last member");
  const SourceCharBoneHandlerPlan char_bone_handlers =
      source_char_bone_handler_plan();
  ok &= expect_size(char_bone_handlers.action_handlers.size(), 1,
                    "CharBone handler action count");
  ok &= expect_string(char_bone_handlers.action_handlers[0], "clear_context",
                      "CharBone handler clear_context");
  ok &= expect_size(char_bone_handlers.handlers.size(), 1,
                    "CharBone handler count");
  ok &= expect_string(char_bone_handlers.handlers[0], "get_context_flags",
                      "CharBone handler get_context_flags");
  ok &= expect_size(char_bone_handlers.superclasses.size(), 1,
                    "CharBone handler superclass count");
  ok &= expect_string(char_bone_handlers.superclasses[0], "Hmx::Object",
                      "CharBone handler superclass");
  ok &= expect_int(char_bone_handlers.check, 0x152,
                   "CharBone handler check value");
  const SourceCharBoneWeightContextPropSyncPlan weight_prop_sync =
      source_char_bone_weight_context_prop_sync_plan();
  ok &= expect_size(weight_prop_sync.properties.size(), 2,
                    "CharBone WeightContext prop count");
  ok &= expect_string(weight_prop_sync.properties[0], "context",
                      "CharBone WeightContext context");
  ok &= expect_string(weight_prop_sync.properties[1], "weight",
                      "CharBone WeightContext weight");
  const auto weight_default =
      source_char_bone_weight_context_default_state();
  ok &= expect_int(weight_default.context, 0,
                   "CharBone WeightContext default context");
  ok &= expect_float(weight_default.weight, 0.0f,
                     "CharBone WeightContext default weight");
  const auto weight_load = source_char_bone_weight_context_load_plan();
  ok &= expect_size(weight_load.read_order.size(), 2,
                    "CharBone WeightContext load row count");
  ok &= expect_string(weight_load.read_order[0], "mContext",
                      "CharBone WeightContext load context first");
  ok &= expect_string(weight_load.read_order[1], "mWeight",
                      "CharBone WeightContext load weight second");

  const auto context_flags_dir =
      source_char_bone_get_context_flags_step(true);
  ok &= expect_int(context_flags_dir.returns_dir_context_flags ? 1 : 0, 1,
                   "CharBone context flags returns parent dir flags");
  ok &= expect_int(context_flags_dir.warns_no_char_bone_dir ? 1 : 0, 0,
                   "CharBone context flags dir no warning");
  const auto context_flags_no_dir =
      source_char_bone_get_context_flags_step(false);
  ok &= expect_int(context_flags_no_dir.returns_dir_context_flags ? 1 : 0, 0,
                   "CharBone context flags no dir skips dir flags");
  ok &= expect_int(context_flags_no_dir.warns_no_char_bone_dir ? 1 : 0, 1,
                   "CharBone context flags no dir warns");
  ok &= expect_int(context_flags_no_dir.returns_empty_array ? 1 : 0, 1,
                   "CharBone context flags no dir empty array");
  ok &= expect_string(context_flags_no_dir.warning,
                      "CharBone: No CharBoneDir for context flags.",
                      "CharBone context flags warning");
  const SourceCharBonePropSyncPlan char_bone_prop_sync =
      source_char_bone_prop_sync_plan();
  ok &= expect_size(char_bone_prop_sync.properties.size(), 8,
                    "CharBone prop sync count");
  ok &= expect_string(char_bone_prop_sync.properties[0], "position_context",
                      "CharBone prop sync first");
  ok &= expect_string(char_bone_prop_sync.properties[3], "rotation_context",
                      "CharBone prop sync rotation context");
  ok &= expect_string(char_bone_prop_sync.properties.back(),
                      "bake_out_as_top_level", "CharBone prop sync last");
  ok &= expect_size(char_bone_prop_sync.superclasses.size(), 1,
                    "CharBone prop sync superclass count");
  ok &= expect_string(char_bone_prop_sync.superclasses[0], "Hmx::Object",
                      "CharBone prop sync superclass");
  const SourceCharBonesBonePropSyncPlan bones_bone_prop_sync =
      source_char_bones_bone_prop_sync_plan();
  ok &= expect_size(bones_bone_prop_sync.properties.size(), 2,
                    "CharBones::Bone prop count");
  ok &= expect_string(bones_bone_prop_sync.properties[0], "name",
                      "CharBones::Bone prop name");
  ok &= expect_string(bones_bone_prop_sync.properties[1], "weight",
                      "CharBones::Bone prop weight");
  ok &= expect_size(bones_bone_prop_sync.set_properties.size(), 1,
                    "CharBones::Bone set prop count");
  ok &= expect_string(bones_bone_prop_sync.set_properties[0], "preview_val",
                      "CharBones::Bone preview_val");
  ok &= expect_int(bones_bone_prop_sync.preview_uses_prop_bones_string_val ? 1
                                                                          : 0,
                   1, "CharBones::Bone preview string lookup");
  const SourceCharBonesObjectPropSyncPlan bones_object_prop_sync =
      source_char_bones_object_prop_sync_plan();
  ok &= expect_int(bones_object_prop_sync.assigns_prop_bones ? 1 : 0, 1,
                   "CharBonesObject prop sync assigns gPropBones");
  ok &= expect_size(bones_object_prop_sync.custom_branches.size(), 1,
                    "CharBonesObject prop branch count");
  ok &= expect_string(bones_object_prop_sync.custom_branches[0], "bones",
                      "CharBonesObject prop bones branch");

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

  const SourceCharBoneDirDefaultState dir_default =
      source_char_bone_dir_default_state();
  ok &= expect_int(dir_default.recenter_targets_no_null ? 1 : 0, 1,
                   "CharBoneDir default recenter targets no-null");
  ok &= expect_int(dir_default.recenter_average_no_null ? 1 : 0, 1,
                   "CharBoneDir default recenter average no-null");
  ok &= expect_int(dir_default.recenter_slide ? 1 : 0, 0,
                   "CharBoneDir default recenter slide");
  ok &= expect_int(dir_default.move_context, 0,
                   "CharBoneDir default move context");
  ok &= expect_int(dir_default.bake_out_facing ? 1 : 0, 1,
                   "CharBoneDir default bake out facing");
  ok &= expect_int(dir_default.context_flags_is_int ? 1 : 0, 1,
                   "CharBoneDir default context flags type");
  ok &= expect_int(dir_default.context_flags_int, 0,
                   "CharBoneDir default context flags int");
  ok &= expect_int(dir_default.filter_context, 0,
                   "CharBoneDir default filter context");
  ok &= expect_int(dir_default.filter_bones_no_null ? 1 : 0, 1,
                   "CharBoneDir default filter bones no-null");
  ok &= expect_int(dir_default.filter_names_empty ? 1 : 0, 1,
                   "CharBoneDir default filter names empty");
  const SourceCharBoneDirLoadPlan dir_load_unknown =
      source_char_bone_dir_load_plan(5);
  ok &= expect_int(dir_load_unknown.known_revision ? 1 : 0, 0,
                   "CharBoneDir load rejects unknown revision");
  ok &= expect_size(dir_load_unknown.postload_order.size(), 0,
                    "CharBoneDir unknown revision has no reads");
  const SourceCharBoneDirLoadPlan dir_load_legacy =
      source_char_bone_dir_load_plan(1);
  ok &= expect_int(dir_load_legacy.known_revision ? 1 : 0, 1,
                   "CharBoneDir load accepts legacy revision");
  ok &= expect_size(dir_load_legacy.preload_order.size(), 3,
                    "CharBoneDir preload order count");
  ok &= expect_string(dir_load_legacy.preload_order[2], "ObjectDir::PreLoad",
                      "CharBoneDir preload object dir");
  ok &= expect_string(dir_load_legacy.load_order[0], "ObjectDir::Load",
                      "CharBoneDir load object dir");
  ok &= expect_string(dir_load_legacy.postload_order[3],
                      "legacyMoveContextBool",
                      "CharBoneDir legacy move bool");
  ok &= expect_string(dir_load_legacy.postload_order[4],
                      "legacyPreRev3Bool",
                      "CharBoneDir legacy rev3 bool");
  ok &= expect_string(dir_load_legacy.postload_order[5], "mRecenter",
                      "CharBoneDir recenter read");
  const SourceCharBoneDirLoadPlan dir_load_latest =
      source_char_bone_dir_load_plan(4);
  ok &= expect_int(dir_load_latest.known_revision ? 1 : 0, 1,
                   "CharBoneDir load accepts latest source revision");
  ok &= expect_size(dir_load_latest.postload_order.size(), 6,
                    "CharBoneDir latest postload count");
  ok &= expect_string(dir_load_latest.postload_order[3], "mMoveContext",
                      "CharBoneDir latest move context");
  ok &= expect_string(dir_load_latest.postload_order[4], "mRecenter",
                      "CharBoneDir latest recenter");
  ok &= expect_string(dir_load_latest.postload_order[5], "mBakeOutFacing",
                      "CharBoneDir latest bake out facing");
  const SourceCharBoneDirSavePlan dir_save = source_char_bone_dir_save_plan();
  ok &= expect_int(dir_save.save_id, 0x18c, "CharBoneDir SAVE_OBJ id");
  const SourceCharBoneDirCopyPlan dir_copy =
      source_char_bone_dir_copy_plan();
  ok &= expect_size(dir_copy.copied_superclasses.size(), 1,
                    "CharBoneDir copy superclass count");
  ok &= expect_string(dir_copy.copied_superclasses[0], "ObjectDir",
                      "CharBoneDir copy superclass");
  ok &= expect_size(dir_copy.copied_members.size(), 3,
                    "CharBoneDir copy member count");
  ok &= expect_string(dir_copy.copied_members[0], "mMoveContext",
                      "CharBoneDir copy move context");
  ok &= expect_string(dir_copy.copied_members[1], "mRecenter",
                      "CharBoneDir copy recenter");
  ok &= expect_string(dir_copy.copied_members[2], "mBakeOutFacing",
                      "CharBoneDir copy bake out facing");
  const SourceCharBoneDirHandlerPlan dir_handler =
      source_char_bone_dir_handler_plan();
  ok &= expect_size(dir_handler.handlers.size(), 1,
                    "CharBoneDir handler count");
  ok &= expect_string(dir_handler.handlers[0], "get_context_flags",
                      "CharBoneDir handler get_context_flags");
  ok &= expect_size(dir_handler.superclasses.size(), 1,
                    "CharBoneDir handler superclass count");
  ok &= expect_string(dir_handler.superclasses[0], "ObjectDir",
                      "CharBoneDir handler superclass");
  ok &= expect_int(dir_handler.check, 0x1D1,
                   "CharBoneDir handler check");
  const SourceCharBoneDirRecenterPropSyncPlan recenter_prop_sync =
      source_char_bone_dir_recenter_prop_sync_plan();
  ok &= expect_size(recenter_prop_sync.properties.size(), 3,
                    "CharBoneDir Recenter prop count");
  ok &= expect_string(recenter_prop_sync.properties[0], "targets",
                      "CharBoneDir Recenter targets");
  ok &= expect_string(recenter_prop_sync.properties[1], "average",
                      "CharBoneDir Recenter average");
  ok &= expect_string(recenter_prop_sync.properties[2], "slide",
                      "CharBoneDir Recenter slide");
  const SourceCharBoneDirRecenterLoadPlan recenter_load =
      source_char_bone_dir_recenter_load_plan();
  ok &= expect_size(recenter_load.read_order.size(), 3,
                    "CharBoneDir Recenter load row count");
  ok &= expect_string(recenter_load.read_order[0], "mTargets",
                      "CharBoneDir Recenter load targets");
  ok &= expect_string(recenter_load.read_order[1], "mAverage",
                      "CharBoneDir Recenter load average");
  ok &= expect_string(recenter_load.read_order[2], "mSlide",
                      "CharBoneDir Recenter load slide");
  const SourceCharBoneDirPropSyncPlan dir_prop_sync =
      source_char_bone_dir_prop_sync_plan();
  ok &= expect_size(dir_prop_sync.properties.size(), 5,
                    "CharBoneDir prop sync direct count");
  ok &= expect_string(dir_prop_sync.properties[0], "recenter",
                      "CharBoneDir prop sync recenter");
  ok &= expect_string(dir_prop_sync.properties[2], "bake_out_facing",
                      "CharBoneDir prop sync bake flag");
  ok &= expect_string(dir_prop_sync.properties.back(), "filter_names",
                      "CharBoneDir prop sync filter_names");
  ok &= expect_size(dir_prop_sync.set_properties.size(), 1,
                    "CharBoneDir prop sync set count");
  ok &= expect_string(dir_prop_sync.set_properties[0], "merge_character",
                      "CharBoneDir prop sync merge_character");
  ok &= expect_size(dir_prop_sync.modify_properties.size(), 1,
                    "CharBoneDir prop sync modify count");
  ok &= expect_string(dir_prop_sync.modify_properties[0], "filter_context",
                      "CharBoneDir prop sync filter_context");
  ok &= expect_size(dir_prop_sync.modify_actions.size(), 1,
                    "CharBoneDir prop sync modify action count");
  ok &= expect_string(dir_prop_sync.modify_actions[0], "SyncFilter",
                      "CharBoneDir prop sync SyncFilter");
  ok &= expect_size(dir_prop_sync.superclasses.size(), 1,
                    "CharBoneDir prop sync superclass count");
  ok &= expect_string(dir_prop_sync.superclasses[0], "ObjectDir",
                      "CharBoneDir prop sync superclass");

  const SourceCharBoneDirInitPlan dir_init_missing_types =
      source_char_bone_dir_init_plan("char/resources", false, {});
  ok &= expect_int(dir_init_missing_types.creates_char_resources ? 1 : 0, 1,
                   "CharBoneDir Init creates resource dir");
  ok &= expect_int(
      dir_init_missing_types.skipped_missing_clip_types ? 1 : 0, 1,
      "CharBoneDir Init skips missing clip types");
  ok &= expect_size(dir_init_missing_types.load_requests.size(), 0,
                    "CharBoneDir Init missing types no loads");
  const SourceCharBoneDirInitPlan dir_init_empty_path =
      source_char_bone_dir_init_plan("", true,
                                     {{"row0", true, "ignored"}});
  ok &= expect_int(dir_init_empty_path.skipped_empty_resource_path ? 1 : 0, 1,
                   "CharBoneDir Init skips empty resource path");
  ok &= expect_size(dir_init_empty_path.load_requests.size(), 0,
                    "CharBoneDir Init empty path no loads");
  const std::vector<SourceCharBoneDirInitClipTypeRow> init_rows = {
      {"row0", true, "row0_resource"},
      {"no_resource", false, ""},
      {"already", true, "shared_resource", true},
      {"lead", true, "lead_resource", false, true},
      {"failed", true, "failed_resource", false, false}};
  const SourceCharBoneDirInitPlan dir_init =
      source_char_bone_dir_init_plan("char/resources", true, init_rows);
  ok &= expect_int(dir_init.reads_resource_path ? 1 : 0, 1,
                   "CharBoneDir Init reads resource path");
  ok &= expect_int(dir_init.reads_char_clip_types ? 1 : 0, 1,
                   "CharBoneDir Init reads clip types");
  ok &= expect_int(dir_init.registers_get_clip_types ? 1 : 0, 0,
                   "CharBoneDir Init get_clip_types remains unregistered");
  ok &= expect_size(dir_init.scanned_rows, 4,
                    "CharBoneDir Init skips source row zero");
  ok &= expect_size(dir_init.skipped_existing_resources.size(), 1,
                    "CharBoneDir Init existing resource skip count");
  ok &= expect_string(dir_init.skipped_existing_resources[0],
                      "shared_resource",
                      "CharBoneDir Init existing resource name");
  ok &= expect_size(dir_init.load_requests.size(), 2,
                    "CharBoneDir Init load request count");
  ok &= expect_string(dir_init.load_requests[0],
                      "char/resources/lead_resource.milo",
                      "CharBoneDir Init load request path");
  ok &= expect_string(dir_init.load_requests[1],
                      "char/resources/failed_resource.milo",
                      "CharBoneDir Init failed request path");
  ok &= expect_size(dir_init.named_loaded_resources.size(), 1,
                    "CharBoneDir Init names successful loads");
  ok &= expect_string(dir_init.named_loaded_resources[0], "lead_resource",
                      "CharBoneDir Init loaded resource name");
  ok &= expect_size(dir_init.failed_load_resources.size(), 1,
                    "CharBoneDir Init records failed load");
  ok &= expect_string(dir_init.failed_load_resources[0], "failed_resource",
                      "CharBoneDir Init failed resource name");
  const SourceCharBoneDirTerminatePlan dir_terminate =
      source_char_bone_dir_terminate_plan();
  ok &= expect_int(dir_terminate.deletes_resources ? 1 : 0, 1,
                   "CharBoneDir Terminate deletes resources");
  ok &= expect_int(dir_terminate.clears_resources_pointer ? 1 : 0, 0,
                   "CharBoneDir Terminate does not clear pointer in source");
  const SourceCharBoneDirFindResourceResult direct_resource =
      source_char_bone_dir_find_resource({"lead_resource", "band_resource"},
                                         "band_resource");
  ok &= expect_int(direct_resource.found ? 1 : 0, 1,
                   "CharBoneDir FindResource exact hit");
  ok &= expect_string(direct_resource.resource_name, "band_resource",
                      "CharBoneDir FindResource queried name");
  const SourceCharBoneDirFindResourceResult direct_missing_resource =
      source_char_bone_dir_find_resource({"lead_resource"}, "band_resource");
  ok &= expect_int(direct_missing_resource.found ? 1 : 0, 0,
                   "CharBoneDir FindResource exact miss");

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
  const std::vector<SourceCharBoneDirClipTypeResource> clip_resources = {
      {"solo", true, "lead_resource", 0x4, true, "solo_ctx"},
      {"rhythm", true, "band_resource", 0x2, false, "rhythm_ctx"},
      {"broken", false, "", 0x0, false, ""}};
  const std::vector<std::string> clip_types =
      source_char_bone_dir_get_clip_types(clip_resources);
  ok &= expect_size(clip_types.size(), 4, "CharBoneDir clip type count");
  ok &= expect_string(clip_types[0], "", "CharBoneDir clip type empty symbol");
  ok &= expect_string(clip_types[1], "broken",
                      "CharBoneDir clip type first sorted");
  ok &= expect_string(clip_types[3], "solo",
                      "CharBoneDir clip type last sorted");
  const SourceCharBoneDirResourceLookupResult missing_type =
      source_char_bone_dir_find_resource_from_clip_type(clip_resources,
                                                       "missing");
  ok &= expect_int(missing_type.clip_type_found ? 1 : 0, 0,
                   "CharBoneDir missing type");
  ok &= expect_string(missing_type.warning, "no_type",
                      "CharBoneDir missing type warning");
  const SourceCharBoneDirResourceLookupResult missing_resource_field =
      source_char_bone_dir_find_resource_from_clip_type(clip_resources,
                                                       "broken");
  ok &= expect_int(missing_resource_field.clip_type_found ? 1 : 0, 1,
                   "CharBoneDir missing resource type found");
  ok &= expect_int(missing_resource_field.resource_field_found ? 1 : 0, 0,
                   "CharBoneDir missing resource field");
  ok &= expect_string(missing_resource_field.warning, "no_resource_field",
                      "CharBoneDir missing resource field warning");
  const SourceCharBoneDirResourceLookupResult missing_resource =
      source_char_bone_dir_find_resource_from_clip_type(clip_resources,
                                                       "rhythm");
  ok &= expect_int(missing_resource.resource_field_found ? 1 : 0, 1,
                   "CharBoneDir missing resource field found");
  ok &= expect_int(missing_resource.resource_found ? 1 : 0, 0,
                   "CharBoneDir resource missing");
  ok &= expect_string(missing_resource.resource_name, "band_resource",
                      "CharBoneDir missing resource name");
  ok &= expect_string(missing_resource.warning, "no_resource",
                      "CharBoneDir missing resource warning");
  const SourceCharBoneDirResourceLookupResult found_resource =
      source_char_bone_dir_find_resource_from_clip_type(clip_resources, "solo");
  ok &= expect_int(found_resource.resource_found ? 1 : 0, 1,
                   "CharBoneDir resource found");
  ok &= expect_string(found_resource.resource_name, "lead_resource",
                      "CharBoneDir resource name");
  ok &= expect_int(found_resource.context_mask, 0x4,
                   "CharBoneDir resource context");
  ok &= expect_string(found_resource.warning, "",
                      "CharBoneDir resource no warning");
  const SourceCharBoneDirStuffBonesSymbolStep stuff_missing =
      source_char_bone_dir_stuff_bones_symbol_step(clip_resources, "rhythm");
  ok &= expect_int(stuff_missing.call_stuff_bones ? 1 : 0, 0,
                   "CharBoneDir StuffBones missing resource");
  const SourceCharBoneDirStuffBonesSymbolStep stuff_found =
      source_char_bone_dir_stuff_bones_symbol_step(clip_resources, "solo");
  ok &= expect_int(stuff_found.call_stuff_bones ? 1 : 0, 1,
                   "CharBoneDir StuffBones resource found");
  ok &= expect_int(stuff_found.context_mask, 0x4,
                   "CharBoneDir StuffBones context");
  const std::vector<SourceCharBoneDirClipTypeResource> context_resources = {
      {"dummy0", true, "lead_resource", 0x4, true, "row0_skipped"},
      {"rhythm", true, "other_resource", 0x2, true, "ignored_context"},
      {"harmony", true, "lead_resource", 0x1, true, "alpha_context"},
      {"dup", true, "lead_resource", 0x8, true, "alpha_context"},
      {"tail", true, "lead_resource", 0x10, true, "tail_skipped"}};
  const SourceCharBoneDirContextFlagsStep cached_context =
      source_char_bone_dir_get_context_flags_step(
          context_resources, "lead_resource", {"cached_context"}, false);
  ok &= expect_int(cached_context.rebuilt ? 1 : 0, 0,
                   "CharBoneDir GetContextFlags cached gate");
  ok &= expect_size(cached_context.context_flags.size(), 1,
                    "CharBoneDir GetContextFlags cached count");
  ok &= expect_string(cached_context.context_flags[0], "cached_context",
                      "CharBoneDir GetContextFlags cached value");
  const SourceCharBoneDirContextFlagsStep rebuilt_context =
      source_char_bone_dir_get_context_flags_step(context_resources,
                                                  "lead_resource", {}, true);
  ok &= expect_int(rebuilt_context.rebuilt ? 1 : 0, 1,
                   "CharBoneDir GetContextFlags rebuilds int cache");
  ok &= expect_size(rebuilt_context.scanned_rows, 3,
                    "CharBoneDir GetContextFlags source scan count");
  ok &= expect_size(rebuilt_context.context_flags.size(), 1,
                    "CharBoneDir GetContextFlags unique count");
  ok &= expect_string(rebuilt_context.context_flags[0], "alpha_context",
                      "CharBoneDir GetContextFlags sorts first");
  std::vector<CharClip::OutputBone> filter_inputs;
  CharClip::OutputBone filter_pos;
  filter_pos.name = "bone_pos";
  filter_pos.position_context = 0x2;
  filter_inputs.push_back(filter_pos);
  CharClip::OutputBone filter_scale;
  filter_scale.name = "bone_scale";
  filter_scale.scale_context = 0x4;
  filter_inputs.push_back(filter_scale);
  CharClip::OutputBone filter_rot;
  filter_rot.name = "bone_rot";
  filter_rot.rotation_type = kSourceCharBonesTypeRotY;
  filter_rot.rotation_context = 0x8;
  filter_inputs.push_back(filter_rot);
  CharClip::OutputBone filter_rot_end;
  filter_rot_end.name = "bone_rot_end";
  filter_rot_end.rotation_type = kSourceCharBonesTypeEnd;
  filter_rot_end.rotation_context = 0x8;
  filter_inputs.push_back(filter_rot_end);
  const std::vector<std::string> filter_context_six =
      source_char_bone_dir_sync_filter(filter_inputs, 0x6);
  ok &= expect_size(filter_context_six.size(), 2,
                    "CharBoneDir SyncFilter pos scale count");
  ok &= expect_string(filter_context_six[0], "bone_pos",
                      "CharBoneDir SyncFilter position row");
  ok &= expect_string(filter_context_six[1], "bone_scale",
                      "CharBoneDir SyncFilter scale row");
  const std::vector<std::string> filter_context_eight =
      source_char_bone_dir_sync_filter(filter_inputs, 0x8);
  ok &= expect_size(filter_context_eight.size(), 1,
                    "CharBoneDir SyncFilter rotation count");
  ok &= expect_string(filter_context_eight[0], "bone_rot",
                      "CharBoneDir SyncFilter skips TYPE_END rotation");
  const SourceCharBoneDirMergeCharacterPlan merge_failed =
      source_char_bone_dir_merge_character_plan(false, {});
  ok &= expect_int(merge_failed.load_attempted ? 1 : 0, 1,
                   "CharBoneDir MergeCharacter attempts load");
  ok &= expect_int(merge_failed.loaded ? 1 : 0, 0,
                   "CharBoneDir MergeCharacter failed not loaded");
  ok &= expect_int(merge_failed.warned_failed_load ? 1 : 0, 1,
                   "CharBoneDir MergeCharacter failed warns");
  ok &= expect_size(merge_failed.selected_transforms.size(), 0,
                    "CharBoneDir MergeCharacter failed selects none");
  const std::vector<SourceCharBoneDirMergeTransform> merge_transforms = {
      {"character.dir", true, true},
      {"bone_root.mesh", false, true},
      {"exo_spine.mesh", false, true},
      {"spot_anchor.trans", false, true},
      {"bone_static.mesh", false, false},
      {"bone_hand.mesh", false, true}};
  const SourceCharBoneDirMergeCharacterPlan merge_plan =
      source_char_bone_dir_merge_character_plan(true, merge_transforms);
  ok &= expect_int(merge_plan.loaded ? 1 : 0, 1,
                   "CharBoneDir MergeCharacter loaded");
  ok &= expect_size(merge_plan.scanned_transforms, 6,
                    "CharBoneDir MergeCharacter scan count");
  ok &= expect_size(merge_plan.selected_transforms.size(), 3,
                    "CharBoneDir MergeCharacter selected count");
  ok &= expect_string(merge_plan.selected_transforms[0], "bone_root.mesh",
                      "CharBoneDir MergeCharacter selects bone prefix");
  ok &= expect_string(merge_plan.selected_transforms[1], "exo_spine.mesh",
                      "CharBoneDir MergeCharacter selects exo prefix");
  ok &= expect_string(merge_plan.selected_transforms[2], "bone_hand.mesh",
                      "CharBoneDir MergeCharacter preserves source order");
  ok &= expect_int(merge_plan.merge_body_fenced ? 1 : 0, 1,
                   "CharBoneDir MergeCharacter body remains fenced");
  const SourceCharBonesMeshesLifetimePlan meshes_lifetime =
      source_char_bones_meshes_lifetime_plan();
  ok &= expect_int(meshes_lifetime.constructs_mesh_vector_with_owner ? 1 : 0, 1,
                   "CharBonesMeshes constructor owns mesh vector");
  ok &= expect_int(meshes_lifetime.creates_dummy_mesh_transform ? 1 : 0, 1,
                   "CharBonesMeshes constructor creates dummy mesh");
  ok &= expect_int(meshes_lifetime.destructor_clears_mesh_vector ? 1 : 0, 1,
                   "CharBonesMeshes destructor clears meshes");
  ok &= expect_int(meshes_lifetime.destructor_deletes_dummy_mesh ? 1 : 0, 1,
                   "CharBonesMeshes destructor deletes dummy");
  ok &= expect_int(meshes_lifetime.dummy_mesh_is_fallback_target ? 1 : 0, 1,
                   "CharBonesMeshes dummy is fallback target");
  const SourceCharBonesMeshesReplaceStep replace_dummy_from =
      source_char_bones_meshes_replace_step({"mesh_a", "mesh_b"}, "dummy_mesh",
                                            "mesh_c", true, "dummy_mesh");
  ok &= expect_int(replace_dummy_from.object_replace ? 1 : 0, 1,
                   "CharBonesMeshes Replace calls object replace");
  ok &= expect_int(replace_dummy_from.scan_meshes ? 1 : 0, 0,
                   "CharBonesMeshes Replace dummy from skips scan");
  ok &= expect_string(replace_dummy_from.meshes[1], "mesh_b",
                      "CharBonesMeshes Replace dummy preserves meshes");
  const SourceCharBonesMeshesReplaceStep replace_hit =
      source_char_bones_meshes_replace_step({"mesh_a", "mesh_b", "mesh_b"},
                                            "mesh_b", "mesh_c", true,
                                            "dummy_mesh");
  ok &= expect_int(replace_hit.scan_meshes ? 1 : 0, 1,
                   "CharBonesMeshes Replace scans meshes");
  ok &= expect_int(replace_hit.replaced_index, 1,
                   "CharBonesMeshes Replace first match");
  ok &= expect_string(replace_hit.meshes[1], "mesh_c",
                      "CharBonesMeshes Replace transformable target");
  ok &= expect_string(replace_hit.meshes[2], "mesh_b",
                      "CharBonesMeshes Replace returns after first match");
  const SourceCharBonesMeshesReplaceStep replace_non_transform =
      source_char_bones_meshes_replace_step({"mesh_a", "mesh_b"}, "mesh_b",
                                            "not_trans", false, "dummy_mesh");
  ok &= expect_int(replace_non_transform.assigned_dummy ? 1 : 0, 1,
                   "CharBonesMeshes Replace non-transform dummy");
  ok &= expect_string(replace_non_transform.meshes[1], "dummy_mesh",
                      "CharBonesMeshes Replace dummy target");
  const std::vector<SourceCharBonesBone> mesh_bones = {
      {"bone_hand.pos", 1.0f},
      {"bone_facing.pos", 1.0f},
      {"bone_missing.scale", 1.0f}};
  const SourceCharBonesMeshesReallocateStep mesh_realloc =
      source_char_bones_meshes_reallocate_step(
          mesh_bones, {{"bone_hand.pos", "hand_xfm"}}, "dummy_mesh");
  ok &= expect_int(mesh_realloc.char_bones_alloc_reallocate_internal ? 1 : 0, 1,
                   "CharBonesMeshes Reallocate calls base alloc");
  ok &= expect_size(mesh_realloc.meshes.size(), 3,
                    "CharBonesMeshes Reallocate mesh count");
  ok &= expect_string(mesh_realloc.meshes[0], "hand_xfm",
                      "CharBonesMeshes Reallocate resolved target");
  ok &= expect_string(mesh_realloc.meshes[1], "dummy_mesh",
                      "CharBonesMeshes Reallocate facing dummy");
  ok &= expect_string(mesh_realloc.meshes[2], "dummy_mesh",
                      "CharBonesMeshes Reallocate missing dummy");
  ok &= expect_size(mesh_realloc.missing_non_facing_bones.size(), 1,
                    "CharBonesMeshes Reallocate missing log count");
  ok &= expect_string(mesh_realloc.missing_non_facing_bones[0],
                      "bone_missing.scale",
                      "CharBonesMeshes Reallocate missing log row");
  ok &= expect_int(mesh_realloc.acquire_pose ? 1 : 0, 1,
                   "CharBonesMeshes Reallocate acquires pose");
  const SourceCharBonesMeshesReallocateStep mesh_realloc_empty =
      source_char_bones_meshes_reallocate_step({}, {}, "dummy_mesh");
  ok &= expect_size(mesh_realloc_empty.meshes.size(), 0,
                    "CharBonesMeshes Reallocate empty mesh count");
  ok &= expect_int(mesh_realloc_empty.acquire_pose ? 1 : 0, 0,
                   "CharBonesMeshes Reallocate empty no pose");
  const std::vector<std::string> stuffed_meshes =
      source_char_bones_meshes_stuff_meshes({"existing"}, {"mesh_a", "mesh_b"});
  ok &= expect_size(stuffed_meshes.size(), 3,
                    "CharBonesMeshes StuffMeshes count");
  ok &= expect_string(stuffed_meshes[0], "existing",
                      "CharBonesMeshes StuffMeshes preserves caller object");
  ok &= expect_string(stuffed_meshes[1], "mesh_a",
                      "CharBonesMeshes StuffMeshes first mesh");
  ok &= expect_string(stuffed_meshes[2], "mesh_b",
                      "CharBonesMeshes StuffMeshes second mesh");

  const SourceCharBonesMeshesPoseDumpEvidence pose_meshes_dump =
      source_char_bones_meshes_pose_dump_evidence();
  ok &= expect_string(pose_meshes_dump.pose_meshes_range,
                      "0x80321520->0x80321A64",
                      "CharBonesMeshes PoseMeshes range");
  ok &= expect_string(pose_meshes_dump.prop_sync_range,
                      "0x80321B48->0x80321C20",
                      "CharBonesMeshes prop sync range");
  ok &= expect_size(pose_meshes_dump.pose_meshes_locals.size(), 12,
                    "CharBonesMeshes PoseMeshes locals");
  ok &= expect_string(pose_meshes_dump.pose_meshes_locals[0], "bone",
                      "CharBonesMeshes PoseMeshes first local");
  ok &= expect_string(pose_meshes_dump.pose_meshes_locals[11], "blendScale",
                      "CharBonesMeshes PoseMeshes blend scale local");
  ok &= expect_string(
      pose_meshes_dump.latest_source_file,
      "rb3/src/system/char/CharBonesMeshes.cpp",
      "CharBonesMeshes latest source file");
  ok &= expect_string(pose_meshes_dump.latest_source_comment,
                      "fn_804B0C60 - pose meshes",
                      "CharBonesMeshes latest source stub comment");
  ok &= expect_size(pose_meshes_dump.latest_source_stub_steps.size(), 4,
                    "CharBonesMeshes latest source stub step count");
  ok &= expect_string(pose_meshes_dump.latest_source_stub_steps[2],
                      "m.RotateAboutY(angle)",
                      "CharBonesMeshes latest source Y rotate stub");
  ok &= expect_int(pose_meshes_dump.latest_source_mesh_loop_present ? 1 : 0, 0,
                   "CharBonesMeshes latest source lacks mesh loop");
  ok &= expect_int(
      pose_meshes_dump.latest_source_uses_uninitialized_angle ? 1 : 0, 1,
      "CharBonesMeshes latest source uses uninitialized angle");
  ok &= expect_int(
      pose_meshes_dump.latest_source_publishes_transform_rows ? 1 : 0, 0,
      "CharBonesMeshes latest source does not publish transforms");
  ok &= expect_int(pose_meshes_dump.latest_source_body_incomplete ? 1 : 0, 1,
                   "CharBonesMeshes latest PoseMeshes incomplete");
  ok &= expect_int(pose_meshes_dump.rb2_dump_has_statement_body ? 1 : 0, 0,
                   "CharBonesMeshes dump is not statement body");
  ok &= expect_int(pose_meshes_dump.safe_to_publish_mesh_transforms ? 1 : 0, 0,
                   "CharBonesMeshes live transform publishing remains fenced");
  const SourceReleasePosePublisherBoundary shoulder_boundary =
      source_release_pose_publisher_boundary();
  ok &= expect_int(
      shoulder_boundary.zero_weight_hand_ik_does_not_explain_pose ? 1 : 0,
      1, "Rockabill2 zero-weight hand IK does not explain shoulder issue");
  ok &= expect_int(shoulder_boundary.full_output_graph_changes_pose ? 1 : 0, 1,
                   "Rockabill2 full output diagnostic changes body pose");
  ok &= expect_int(shoulder_boundary.safe_to_blame_ik_or_twist ? 1 : 0, 0,
                   "Rockabill2 shoulder not blamed on IK/twist");
  ok &= expect_int(
      shoulder_boundary.safe_to_promote_full_output_graph ? 1 : 0, 0,
      "Rockabill2 full output graph remains fenced");
  ok &= expect_string(
      shoulder_boundary.remaining_source_gap,
      "CharClipSamples / CharBonesSamples / CharBones / PoseMeshes publisher",
      "Rockabill2 remaining source publisher gap");
  ok &= expect_size(shoulder_boundary.source_evidence.size(), 5,
                    "Rockabill2 shoulder evidence count");
  ok &= expect_string(
      shoulder_boundary.source_evidence[1],
      "CharClip::PoseMeshes calls ScaleDown then ScaleAdd before PoseMeshes",
      "Rockabill2 shoulder source call flow evidence");
  ok &= expect_string(
      shoulder_boundary.source_evidence[3],
      "release-pose frame logs hand IK solveWeight zero",
      "Rockabill2 shoulder zero-weight IK evidence");
  ok &= expect_size(shoulder_boundary.rejected_shortcuts.size(), 3,
                    "Rockabill2 shoulder rejected shortcut count");
  ok &= expect_string(
      shoulder_boundary.rejected_shortcuts[2],
      "default-on broad CharBone output without the source publisher body",
      "Rockabill2 shoulder rejects default full-output shortcut");

  const SourceCharPosePublisherSourceRefresh publisher_refresh =
      source_char_pose_publisher_source_refresh_20260714();
  ok &= expect_string(publisher_refresh.rb3_commit, "41719f2",
                      "pose publisher rb3 mirror commit");
  ok &= expect_string(publisher_refresh.grim_commit, "1c05ca3",
                      "pose publisher grim mirror commit");
  ok &= expect_string(publisher_refresh.re_notes_commit, "5c486fd",
                      "pose publisher re-notes mirror commit");
  ok &= expect_int(publisher_refresh.rb3_after_fetch ? 1 : 0, 1,
                   "pose publisher rb3 mirror refreshed");
  ok &= expect_int(publisher_refresh.grim_after_fetch ? 1 : 0, 1,
                   "pose publisher grim mirror refreshed");
  ok &= expect_int(publisher_refresh.re_notes_after_fetch ? 1 : 0, 1,
                   "pose publisher re-notes mirror refreshed");
  ok &= expect_int(publisher_refresh.char_clip_pose_meshes_body ? 1 : 0, 1,
                   "pose publisher CharClip PoseMeshes body present");
  ok &= expect_int(
      publisher_refresh.char_bones_samples_scale_add_sample_body ? 1 : 0, 1,
      "pose publisher CharBonesSamples ScaleAddSample body present");
  ok &= expect_int(publisher_refresh.char_bones_scale_add_body ? 1 : 0, 0,
                   "pose publisher CharBones ScaleAdd body still fenced");
  ok &= expect_int(
      publisher_refresh.char_bones_samples_evaluate_channel_body ? 1 : 0, 0,
      "pose publisher CharBonesSamples EvaluateChannel still fenced");
  ok &= expect_int(
      publisher_refresh.char_bones_meshes_pose_meshes_statement_body ? 1 : 0,
      0, "pose publisher CharBonesMeshes PoseMeshes statement body fenced");
  ok &= expect_int(
      publisher_refresh.char_bones_meshes_latest_pose_meshes_stub_only ? 1 : 0,
      1, "pose publisher latest PoseMeshes body is stub only");
  ok &= expect_int(publisher_refresh.rb2_dump_is_range_local_map ? 1 : 0, 1,
                   "pose publisher RB2 dump is range/local map");
  ok &= expect_size(publisher_refresh.still_fenced.size(), 5,
                    "pose publisher still-fenced count");
  ok &= expect_string(publisher_refresh.still_fenced.front(),
                      "CharBones::ScaleAdd",
                      "pose publisher first fenced body");
  ok &= expect_string(publisher_refresh.still_fenced.back(),
                      "CharClipDriver::Evaluate",
                      "pose publisher final fenced body");
  const SourceCharClipPoseMeshesSteps pose_meshes_steps =
      source_char_clip_pose_meshes_steps(18.5f);
  ok &= expect_string(pose_meshes_steps.temp_meshes_name, "tmp_viseme_bones",
                      "CharClip PoseMeshes temp bones name");
  ok &= expect_size(pose_meshes_steps.call_order.size(), 6,
                    "CharClip PoseMeshes call order count");
  ok &= expect_string(pose_meshes_steps.call_order[0],
                      "CharBonesMeshes meshes",
                      "CharClip PoseMeshes constructs meshes first");
  ok &= expect_string(pose_meshes_steps.call_order[3], "ScaleDown",
                      "CharClip PoseMeshes calls ScaleDown before ScaleAdd");
  ok &= expect_string(pose_meshes_steps.call_order[5], "meshes.PoseMeshes",
                      "CharClip PoseMeshes final call");
  ok &= expect_string(pose_meshes_steps.scale_down_target, "meshes",
                      "CharClip PoseMeshes ScaleDown target");
  ok &= expect_float(pose_meshes_steps.scale_down_weight, 0.0f,
                     "CharClip PoseMeshes ScaleDown weight");
  ok &= expect_string(pose_meshes_steps.scale_add_target, "meshes",
                      "CharClip PoseMeshes ScaleAdd target");
  ok &= expect_float(pose_meshes_steps.scale_add_weight, 1.0f,
                     "CharClip PoseMeshes ScaleAdd weight");
  ok &= expect_float(pose_meshes_steps.scale_add_frame, 18.5f,
                     "CharClip PoseMeshes ScaleAdd frame");
  ok &= expect_float(pose_meshes_steps.scale_add_blend, 0.0f,
                     "CharClip PoseMeshes ScaleAdd blend");
  ok &= expect_string(pose_meshes_steps.pose_meshes_target, "meshes",
                      "CharClip PoseMeshes final target");

  const SourceCharServoBoneDefaultState servo_defaults =
      source_char_servo_bone_default_state();
  ok &= expect_int(servo_defaults.pelvis_null ? 1 : 0, 1,
                   "CharServoBone default pelvis null");
  ok &= expect_int(servo_defaults.facing_rot_delta_null ? 1 : 0, 1,
                   "CharServoBone default rot delta null");
  ok &= expect_int(servo_defaults.facing_pos_delta_null ? 1 : 0, 1,
                   "CharServoBone default pos delta null");
  ok &= expect_int(servo_defaults.facing_rot_null ? 1 : 0, 1,
                   "CharServoBone default rot null");
  ok &= expect_int(servo_defaults.facing_pos_null ? 1 : 0, 1,
                   "CharServoBone default pos null");
  ok &= expect_int(servo_defaults.move_self ? 1 : 0, 0,
                   "CharServoBone default move self");
  ok &= expect_int(servo_defaults.delta_changed ? 1 : 0, 0,
                   "CharServoBone default delta changed");
  ok &= expect_int(servo_defaults.regulate_empty ? 1 : 0, 1,
                   "CharServoBone default regulate empty");

  const SourceCharServoBoneSetNameStep servo_set_name_non_character =
      source_char_servo_bone_set_name(false);
  ok &= expect_int(
      servo_set_name_non_character.calls_hmx_object_set_name ? 1 : 0, 1,
      "CharServoBone SetName calls object SetName");
  ok &= expect_int(
      servo_set_name_non_character.reads_current_dir_after_set_name ? 1 : 0, 1,
      "CharServoBone SetName reads Dir after SetName");
  ok &= expect_int(
      servo_set_name_non_character.assigns_character_owner ? 1 : 0, 0,
      "CharServoBone SetName non-character owner");
  const SourceCharServoBoneSetNameStep servo_set_name_character =
      source_char_servo_bone_set_name(true);
  ok &= expect_int(servo_set_name_character.assigns_character_owner ? 1 : 0, 1,
                   "CharServoBone SetName character owner");

  const SourceCharServoBoneSetClipTypeStep same_servo_clip =
      source_char_servo_bone_set_clip_type_step(false);
  ok &= expect_int(same_servo_clip.changed ? 1 : 0, 0,
                   "CharServoBone SetClipType unchanged");
  ok &= expect_int(same_servo_clip.clear_bones ? 1 : 0, 0,
                   "CharServoBone SetClipType unchanged no clear");
  const SourceCharServoBoneSetClipTypeStep new_servo_clip =
      source_char_servo_bone_set_clip_type_step(true);
  ok &= expect_int(new_servo_clip.assign_clip_type ? 1 : 0, 1,
                   "CharServoBone SetClipType assigns");
  ok &= expect_int(new_servo_clip.clear_bones ? 1 : 0, 1,
                   "CharServoBone SetClipType clears");
  ok &= expect_int(new_servo_clip.stuff_bones_from_dir ? 1 : 0, 1,
                   "CharServoBone SetClipType stuffs bones");

  const SourceCharServoBoneEnterStep servo_enter_without_delta =
      source_char_servo_bone_enter(false);
  ok &= expect_int(servo_enter_without_delta.zero_deltas ? 1 : 0, 1,
                   "CharServoBone Enter zeros");
  ok &= expect_int(servo_enter_without_delta.clear_regulate ? 1 : 0, 1,
                   "CharServoBone Enter clears regulate");
  ok &= expect_int(servo_enter_without_delta.delta_changed ? 1 : 0, 0,
                   "CharServoBone Enter delta not changed");
  ok &= expect_int(servo_enter_without_delta.move_self ? 1 : 0, 0,
                   "CharServoBone Enter no delta move self");
  const SourceCharServoBoneEnterStep servo_enter_with_delta =
      source_char_servo_bone_enter(true);
  ok &= expect_int(servo_enter_with_delta.move_self ? 1 : 0, 1,
                   "CharServoBone Enter delta move self");

  const SourceCharServoBoneSetMoveSelfStep servo_same_move_self =
      source_char_servo_bone_set_move_self(true, true);
  ok &= expect_int(servo_same_move_self.changed ? 1 : 0, 0,
                   "CharServoBone SetMoveSelf same unchanged");
  ok &= expect_int(servo_same_move_self.move_self ? 1 : 0, 1,
                   "CharServoBone SetMoveSelf same preserves");
  ok &= expect_int(servo_same_move_self.delta_changed ? 1 : 0, 0,
                   "CharServoBone SetMoveSelf same no delta change");
  const SourceCharServoBoneSetMoveSelfStep servo_new_move_self =
      source_char_servo_bone_set_move_self(false, true);
  ok &= expect_int(servo_new_move_self.changed ? 1 : 0, 1,
                   "CharServoBone SetMoveSelf changed");
  ok &= expect_int(servo_new_move_self.move_self ? 1 : 0, 1,
                   "CharServoBone SetMoveSelf assigns");
  ok &= expect_int(servo_new_move_self.delta_changed ? 1 : 0, 1,
                   "CharServoBone SetMoveSelf marks delta");

  const SourceCharServoBoneReallocatePlan servo_realloc_no_delta =
      source_char_servo_bone_reallocate_plan(false);
  ok &= expect_int(servo_realloc_no_delta.calls_char_bones_meshes_reallocate
                       ? 1
                       : 0,
                   1, "CharServoBone Reallocate calls base");
  ok &= expect_int(servo_realloc_no_delta.resets_facing_rot_delta ? 1 : 0, 1,
                   "CharServoBone Reallocate resets rot delta");
  ok &= expect_size(servo_realloc_no_delta.lookup_order.size(), 1,
                    "CharServoBone Reallocate no delta lookup count");
  ok &= expect_string(servo_realloc_no_delta.lookup_order[0],
                      "bone_facing_delta.pos",
                      "CharServoBone Reallocate first lookup");
  ok &= expect_int(servo_realloc_no_delta.lookup_pelvis ? 1 : 0, 0,
                   "CharServoBone Reallocate no delta skips pelvis");

  const SourceCharServoBoneReallocatePlan servo_realloc_delta =
      source_char_servo_bone_reallocate_plan(true);
  ok &= expect_int(servo_realloc_delta.lookup_facing_pos ? 1 : 0, 1,
                   "CharServoBone Reallocate finds facing pos");
  ok &= expect_int(servo_realloc_delta.lookup_pelvis ? 1 : 0, 1,
                   "CharServoBone Reallocate finds pelvis");
  ok &= expect_int(servo_realloc_delta.assert_facing_pos_and_pelvis ? 1 : 0, 1,
                   "CharServoBone Reallocate asserts facing and pelvis");
  ok &= expect_int(servo_realloc_delta.lookup_facing_rot ? 1 : 0, 1,
                   "CharServoBone Reallocate finds facing rot");
  ok &= expect_int(servo_realloc_delta.lookup_facing_rot_delta ? 1 : 0, 1,
                   "CharServoBone Reallocate finds facing rot delta");
  ok &= expect_size(servo_realloc_delta.lookup_order.size(), 5,
                    "CharServoBone Reallocate delta lookup count");
  ok &= expect_string(servo_realloc_delta.lookup_order[1], "bone_facing.pos",
                      "CharServoBone Reallocate facing pos lookup");
  ok &= expect_string(servo_realloc_delta.lookup_order[2], "bone_pelvis",
                      "CharServoBone Reallocate pelvis lookup");
  ok &= expect_string(servo_realloc_delta.lookup_order[4],
                      "bone_facing_delta.rotz",
                      "CharServoBone Reallocate facing rot delta lookup");

  const SourceCharServoBoneCopyPlan servo_copy =
      source_char_servo_bone_copy_plan();
  ok &= expect_size(servo_copy.copied_superclasses.size(), 1,
                    "CharServoBone Copy superclass count");
  ok &= expect_string(servo_copy.copied_superclasses[0], "Hmx::Object",
                      "CharServoBone Copy superclass");
  ok &= expect_size(servo_copy.copied_members.size(), 1,
                    "CharServoBone Copy member count");
  ok &= expect_string(servo_copy.copied_members[0], "mMoveSelf",
                      "CharServoBone Copy member");
  ok &= expect_int(servo_copy.calls_set_clip_type ? 1 : 0, 1,
                   "CharServoBone Copy calls SetClipType");
  const SourceCharServoBoneLoadPlan servo_load_v0 =
      source_char_servo_bone_load_plan(0);
  ok &= expect_int(servo_load_v0.known_revision ? 1 : 0, 1,
                   "CharServoBone Load v0 known");
  ok &= expect_size(servo_load_v0.read_order.size(), 1,
                    "CharServoBone Load v0 row count");
  ok &= expect_string(servo_load_v0.read_order[0], "Hmx::Object",
                      "CharServoBone Load object row");
  ok &= expect_string(servo_load_v0.branches[0],
                      "mClipType defaults empty",
                      "CharServoBone Load v0 clip type default");
  ok &= expect_string(servo_load_v0.call_order[0], "SetClipType",
                      "CharServoBone Load calls SetClipType");
  const SourceCharServoBoneLoadPlan servo_load_v2 =
      source_char_servo_bone_load_plan(2);
  ok &= expect_size(servo_load_v2.read_order.size(), 2,
                    "CharServoBone Load v2 row count");
  ok &= expect_string(servo_load_v2.read_order[1], "mClipType",
                      "CharServoBone Load v2 clip type");
  ok &= expect_int(source_char_servo_bone_load_plan(3).known_revision ? 1 : 0,
                   0, "CharServoBone Load rejects high revision");
  const SourceCharServoBoneSavePlan servo_save =
      source_char_servo_bone_save_plan();
  ok &= expect_int(servo_save.save_id, 0x14a,
                   "CharServoBone SAVE_OBJ id");
  const SourceCharServoBoneHandlerPlan servo_handlers =
      source_char_servo_bone_handler_plan();
  ok &= expect_size(servo_handlers.superclasses.size(), 2,
                    "CharServoBone handler superclass count");
  ok &= expect_string(servo_handlers.superclasses[0], "CharPollable",
                      "CharServoBone handler pollable superclass");
  ok &= expect_string(servo_handlers.superclasses[1], "Hmx::Object",
                      "CharServoBone handler object superclass");
  ok &= expect_int(servo_handlers.check, 0x16E,
                   "CharServoBone handler check");
  const SourceCharServoBonePropSyncPlan servo_props =
      source_char_servo_bone_prop_sync_plan();
  ok &= expect_size(servo_props.set_properties.size(), 2,
                    "CharServoBone prop set count");
  ok &= expect_string(servo_props.set_properties[0], "clip_type",
                      "CharServoBone prop set clip type");
  ok &= expect_string(servo_props.set_properties[1], "move_self",
                      "CharServoBone prop set move self");
  ok &= expect_size(servo_props.properties.size(), 2,
                    "CharServoBone prop direct count");
  ok &= expect_string(servo_props.properties[0], "delta_changed",
                      "CharServoBone prop delta changed");
  ok &= expect_string(servo_props.properties[1], "regulate",
                      "CharServoBone prop regulate");
  ok &= expect_string(servo_props.superclasses[0], "CharBonesMeshes",
                      "CharServoBone prop sync superclass");

  const SourceCharServoBoneRuntimeDumpEvidence servo_runtime =
      source_char_servo_bone_runtime_dump_evidence();
  ok &= expect_string(servo_runtime.poll_range, "0x8038F4A0->0x8038F820",
                      "CharServoBone rb2 Poll range");
  ok &= expect_string(servo_runtime.regulate_override_range,
                      "0x8038FD74->0x8038FF30",
                      "CharServoBone rb2 RegulateOverride range");
  ok &= expect_string(servo_runtime.regulate_range,
                      "0x8038FF30->0x803901BC",
                      "CharServoBone rb2 Regulate range");
  ok &= expect_size(servo_runtime.poll_locals.size(), 5,
                    "CharServoBone rb2 Poll locals");
  ok &= expect_string(servo_runtime.regulate_locals[0], "before",
                      "CharServoBone rb2 Regulate before local");
  ok &= expect_int(servo_runtime.rb2_dump_has_statement_body ? 1 : 0, 0,
                   "CharServoBone dump is not statement body");
  ok &= expect_int(servo_runtime.safe_to_run_poll ? 1 : 0, 0,
                   "CharServoBone Poll remains fenced");
  ok &= expect_int(servo_runtime.safe_to_publish_servo_motion ? 1 : 0, 0,
                   "CharServoBone live motion remains fenced");

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
  ok &= expect_size(samples.raw_data.size(), 0, "samples default raw data");
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
  ok &= expect_size(samples.raw_data.size(), 48, "samples Set raw data bytes");
  ok &= expect_size(samples.frames.size(), 0, "samples Set clears frames");
  samples.raw_data[7] = 0x5Au;
  samples.raw_data[47] = 0xA5u;
  samples.frames = {1.0f, 2.0f, 3.0f};
  const SourceCharBonesSamplesState cloned =
      source_char_bones_samples_clone(samples);
  ok &= expect_int(cloned.num_samples, 3, "samples Clone num");
  ok &= expect_int(cloned.bones.compression, kCompressRots,
                   "samples Clone compression");
  ok &= expect_size(cloned.bones.bones.size(), 2, "samples Clone bone count");
  ok &= expect_int(cloned.raw_data_size, 48, "samples Clone raw data size");
  ok &= expect_size(cloned.raw_data.size(), 48, "samples Clone raw data bytes");
  ok &= expect_int(cloned.raw_data[7], 0x5A, "samples Clone raw data byte 7");
  ok &= expect_int(cloned.raw_data[47], 0xA5, "samples Clone raw data byte 47");
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
  ok &= expect_string(one_step[0].downstream_call, "",
                      "samples split generic call");
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
  const SourceCharBonesSampleStep rotate_by_step =
      source_char_bones_samples_rotate_by_step(samples, 3);
  ok &= expect_int(rotate_by_step.start_offset, 96,
                   "samples RotateBy step offset");
  ok &= expect_float(rotate_by_step.weight, 0.0f,
                     "samples RotateBy step unweighted");
  ok &= expect_string(rotate_by_step.downstream_call, "CharBones::RotateBy",
                      "samples RotateBy downstream call");
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
  ok &= expect_string(rotate_steps[0].downstream_call, "CharBones::RotateTo",
                      "samples RotateTo first downstream call");
  ok &= expect_string(rotate_steps[1].downstream_call, "CharBones::RotateTo",
                      "samples RotateTo second downstream call");
  const std::vector<SourceCharBonesSampleStep> scale_steps =
      source_char_bones_samples_scale_add_steps(samples, 0, 0.5f, 0.0f);
  ok &= expect_size(scale_steps.size(), 1, "samples ScaleAddSample count");
  ok &= expect_int(scale_steps[0].start_offset, 0,
                   "samples ScaleAddSample first offset");
  ok &= expect_float(scale_steps[0].weight, 0.5f,
                     "samples ScaleAddSample first weight");
  ok &= expect_string(scale_steps[0].downstream_call, "CharBones::ScaleAdd",
                      "samples ScaleAddSample downstream call");
  const std::vector<SourceCharBonesSampleStep> scale_blend_steps =
      source_char_bones_samples_scale_add_steps(samples, 1, 0.8f, 0.25f);
  ok &= expect_size(scale_blend_steps.size(), 2,
                    "samples ScaleAddSample blended count");
  ok &= expect_int(scale_blend_steps[0].start_offset, 32,
                   "samples ScaleAddSample blended first offset");
  ok &= expect_float(scale_blend_steps[0].weight, 0.6f,
                     "samples ScaleAddSample blended first weight");
  ok &= expect_int(scale_blend_steps[1].start_offset, 64,
                   "samples ScaleAddSample blended second offset");
  ok &= expect_float(scale_blend_steps[1].weight, 0.2f,
                     "samples ScaleAddSample blended second weight");
  ok &= expect_string(scale_blend_steps[1].downstream_call,
                      "CharBones::ScaleAdd",
                      "samples ScaleAddSample blended downstream call");
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
  const SourceCharBonesSamplesLoadPlan samples_load_plan =
      source_char_bones_samples_load_plan(16);
  ok &= expect_int(samples_load_plan.known_version ? 1 : 0, 1,
                   "samples load plan accepts source version");
  ok &= expect_size(samples_load_plan.read_order.size(), 3,
                    "samples load plan read count");
  ok &= expect_string(samples_load_plan.read_order[0], "gVer",
                      "samples load plan reads version");
  ok &= expect_string(samples_load_plan.read_order[1], "LoadHeader",
                      "samples load plan delegates header");
  ok &= expect_string(samples_load_plan.read_order[2], "LoadData",
                      "samples load plan delegates data");
  const SourceCharBonesSamplesLoadPlan samples_bad_load =
      source_char_bones_samples_load_plan(12);
  ok &= expect_int(samples_bad_load.known_version ? 1 : 0, 0,
                   "samples load plan rejects low version");
  ok &= expect_size(samples_bad_load.read_order.size(), 0,
                    "samples bad load plan has no reads");

  ok &= expect_int(source_grim_char_clip_samples_version_known(10) ? 1 : 0, 1,
                   "grim CharClipSamples GH2 version 10 accepted");
  ok &= expect_int(source_grim_char_clip_samples_version_known(11) ? 1 : 0, 1,
                   "grim CharClipSamples GH2 360 version 11 accepted");
  ok &= expect_int(source_grim_char_clip_samples_version_known(16) ? 1 : 0, 1,
                   "grim CharClipSamples TBRB version 16 accepted");
  ok &= expect_int(source_grim_char_clip_samples_version_known(13) ? 1 : 0, 0,
                   "grim CharClipSamples version 13 rejected");
  ok &= expect_int(
      source_grim_char_bones_samples_standalone_version_known(16) ? 1 : 0, 1,
      "grim standalone CharBonesSamples version 16 accepted");
  ok &= expect_int(
      source_grim_char_bones_samples_standalone_version_known(10) ? 1 : 0, 0,
      "grim standalone CharBonesSamples version 10 rejected");
  ok &= expect_int(source_grim_char_clip_version_known(5) ? 1 : 0, 1,
                   "grim CharClip version 5 accepted");
  ok &= expect_int(source_grim_char_clip_version_known(12) ? 1 : 0, 1,
                   "grim CharClip version 12 accepted");

  ok &= expect_int(source_grim_char_bones_samples_get_type_of("bone.pos"),
                   kSourceCharBonesTypePos, "grim get_type_of pos");
  ok &= expect_int(source_grim_char_bones_samples_get_type_of("bone.scale"),
                   kSourceCharBonesTypeScale, "grim get_type_of scale");
  ok &= expect_int(source_grim_char_bones_samples_get_type_of("bone.quat"),
                   kSourceCharBonesTypeQuat, "grim get_type_of quat");
  ok &= expect_int(source_grim_char_bones_samples_get_type_of("bone.rotx"),
                   kSourceCharBonesTypeRotX, "grim get_type_of rotx");
  ok &= expect_int(source_grim_char_bones_samples_get_type_of("bone.roty"),
                   kSourceCharBonesTypeRotY, "grim get_type_of roty");
  ok &= expect_int(source_grim_char_bones_samples_get_type_of("bone.rotz"),
                   kSourceCharBonesTypeRotZ, "grim get_type_of rotz");
  ok &= expect_int(source_grim_char_bones_samples_get_type_of("bone.head.pos"),
                   kSourceCharBonesTypeEnd, "grim get_type_of first-dot rule");
  ok &= expect_string(
      source_grim_char_bones_samples_channel_mesh_name("bone_head.pos"),
      "bone_head.mesh", "grim channel pos maps to mesh");
  ok &= expect_string(
      source_grim_char_bones_samples_channel_mesh_name("bone_head.quat"),
      "bone_head.mesh", "grim channel quat maps to mesh");
  ok &= expect_string(
      source_grim_char_bones_samples_channel_mesh_name("bone_head.rotz"),
      "bone_head.mesh", "grim channel rotz maps to mesh");
  ok &= expect_string(
      source_grim_char_bones_samples_channel_mesh_name("bone_head.rotx"),
      "bone_head.rotx", "grim channel rotx stays fenced");
  ok &= expect_string(
      source_grim_char_bones_samples_channel_mesh_name("bone_head.scale"),
      "bone_head.scale", "grim channel scale stays fenced");
  const SourceGrimCharBonesSamplesDecodePlan grim_decode =
      source_grim_char_bones_samples_decode_plan();
  ok &= expect_int(grim_decode.walks_serialized_bones ? 1 : 0, 1,
                   "grim decode walks serialized bones");
  ok &= expect_int(grim_decode.groups_by_mesh_name ? 1 : 0, 1,
                   "grim decode groups by mesh name");
  ok &= expect_int(grim_decode.stores_channel_weights ? 1 : 0, 1,
                   "grim decode stores channel weights");
  ok &= expect_int(grim_decode.sorts_bone_samples_by_symbol ? 1 : 0, 1,
                   "grim decode sorts samples by symbol");
  ok &= expect_size(grim_decode.decoded_types.size(), 3,
                    "grim decode supported type count");
  ok &= expect_int(grim_decode.decoded_types[0], kSourceCharBonesTypePos,
                   "grim decode supports pos first");
  ok &= expect_int(grim_decode.decoded_types[1], kSourceCharBonesTypeQuat,
                   "grim decode supports quat second");
  ok &= expect_int(grim_decode.decoded_types[2], kSourceCharBonesTypeRotZ,
                   "grim decode supports rotz third");
  ok &= expect_size(grim_decode.unsupported_types.size(), 4,
                    "grim decode unsupported type count");
  ok &= expect_int(source_grim_char_bones_samples_decodes_channel_type(
                       kSourceCharBonesTypePos)
                       ? 1
                       : 0,
                   1, "grim decode accepts pos");
  ok &= expect_int(source_grim_char_bones_samples_decodes_channel_type(
                       kSourceCharBonesTypeScale)
                       ? 1
                       : 0,
                   0, "grim decode rejects scale");
  ok &= expect_int(source_grim_char_bones_samples_panics_channel_type(
                       kSourceCharBonesTypeRotX)
                       ? 1
                       : 0,
                   1, "grim decode panics rotx");
  ok &= expect_float(source_grim_char_bones_samples_channel_weight(
                         {0.25f, 0.75f}, 0),
                     0.25f, "grim channel weight first");
  ok &= expect_float(source_grim_char_bones_samples_channel_weight(
                         {0.25f, 0.75f}, 1),
                     0.75f, "grim channel weight second");
  ok &= expect_float(source_grim_char_bones_samples_channel_weight(
                         {0.25f, 0.75f}, 2),
                     1.0f, "grim channel weight fallback");
  SourceGrimCharBonesSamplesExportTranslationInput grim_export_pos;
  grim_export_pos.base_translation = {10.0f, 20.0f, 30.0f};
  grim_export_pos.weight = 0.5f;
  grim_export_pos.pos_samples = {
      std::array<float, 3>{2.0f, -4.0f, 6.0f},
      std::array<float, 3>{-2.0f, 4.0f, -6.0f},
  };
  const SourceGrimCharBonesSamplesExportTranslationPlan grim_export_plan =
      source_grim_char_bones_samples_export_translation_plan(grim_export_pos);
  ok &= expect_int(grim_export_plan.has_pos_samples ? 1 : 0, 1,
                   "grim export has pos samples");
  ok &= expect_int(grim_export_plan.uses_default_translation_sample ? 1 : 0,
                   0, "grim export skips default when pos exists");
  ok &= expect_int(grim_export_plan.uses_sample_index_times ? 1 : 0, 1,
                   "grim export uses sample index times");
  ok &= expect_int(grim_export_plan.multiplies_sample_index_by_fps ? 1 : 0, 1,
                   "grim export scales sample index by fps");
  ok &= expect_int(grim_export_plan.uses_frame_values ? 1 : 0, 0,
                   "grim export does not use frames");
  ok &= expect_int(grim_export_plan.adds_base_translation_to_pos_samples ? 1
                                                                         : 0,
                   0, "grim export pos omits base translation");
  ok &= expect_near(grim_export_plan.sample_time_step, 1.0f / 30.0f,
                    "grim export sample time step");
  ok &= expect_size(grim_export_plan.input_times.size(), 2,
                    "grim export input count");
  ok &= expect_near(grim_export_plan.input_times[0], 0.0f,
                    "grim export input time 0");
  ok &= expect_near(grim_export_plan.input_times[1], 1.0f / 30.0f,
                    "grim export input time 1");
  ok &= expect_size(grim_export_plan.output_translations.size(), 2,
                    "grim export output count");
  ok &= expect_vec3_near(grim_export_plan.output_translations[0],
                         {1.0f, -2.0f, 3.0f},
                         "grim export weighted translation 0");
  ok &= expect_vec3_near(grim_export_plan.output_translations[1],
                         {-1.0f, 2.0f, -3.0f},
                         "grim export weighted translation 1");
  SourceGrimCharBonesSamplesExportTranslationInput grim_export_default;
  grim_export_default.base_translation = {1.0f, 2.0f, 3.0f};
  const SourceGrimCharBonesSamplesExportTranslationPlan grim_default_plan =
      source_grim_char_bones_samples_export_translation_plan(
          grim_export_default);
  ok &= expect_int(grim_default_plan.has_pos_samples ? 1 : 0, 0,
                   "grim default export has no pos");
  ok &= expect_int(grim_default_plan.uses_default_translation_sample ? 1 : 0,
                   1, "grim default export emits base sample");
  ok &= expect_size(grim_default_plan.input_times.size(), 1,
                    "grim default export input count");
  ok &= expect_near(grim_default_plan.input_times[0], 0.0f,
                    "grim default export input time");
  ok &= expect_size(grim_default_plan.output_translations.size(), 1,
                    "grim default export output count");
  ok &= expect_vec3_near(grim_default_plan.output_translations[0],
                         {1.0f, 2.0f, 3.0f},
                         "grim default export base translation");
  SourceGrimCharBonesSamplesExportRotationInput grim_export_rot;
  grim_export_rot.pos_sample_count = 3;
  grim_export_rot.quat_weight = 0.5f;
  grim_export_rot.rotz_weight = 0.5f;
  grim_export_rot.quat_samples_xyzw = {
      std::array<float, 4>{0.0f, 0.0f, 0.0f, 2.0f},
      std::array<float, 4>{0.0f, 2.0f, 0.0f, 0.0f},
  };
  grim_export_rot.rotz_samples = {0.5f};
  const SourceGrimCharBonesSamplesExportRotationPlan grim_rot_plan =
      source_grim_char_bones_samples_export_rotation_plan(grim_export_rot);
  ok &= expect_int(grim_rot_plan.has_rotation_samples ? 1 : 0, 1,
                   "grim rotation export has samples");
  ok &= expect_int(grim_rot_plan.includes_pos_sample_count ? 1 : 0, 1,
                   "grim rotation export includes pos count");
  ok &= expect_int(grim_rot_plan.initializes_from_node_rotation ? 1 : 0, 1,
                   "grim rotation export starts at node rotation");
  ok &= expect_int(grim_rot_plan.quat_replaces_node_rotation ? 1 : 0, 1,
                   "grim rotation export quat replaces node rotation");
  ok &= expect_int(grim_rot_plan.rotz_post_multiplies ? 1 : 0, 1,
                   "grim rotation export rotz post-multiplies");
  ok &= expect_int(grim_rot_plan.rotz_uses_z_axis ? 1 : 0, 1,
                   "grim rotation export rotz uses z axis");
  ok &= expect_int(grim_rot_plan.rotz_angle_is_pi_scaled ? 1 : 0, 1,
                   "grim rotation export rotz uses pi scale");
  ok &= expect_int(grim_rot_plan.uses_frame_values ? 1 : 0, 0,
                   "grim rotation export does not use frames");
  ok &= expect_size(grim_rot_plan.sample_count, 3,
                    "grim rotation export sample count");
  ok &= expect_size(grim_rot_plan.input_times.size(), 3,
                    "grim rotation export input count");
  ok &= expect_near(grim_rot_plan.input_times[2], 2.0f / 30.0f,
                    "grim rotation export input time 2");
  ok &= expect_size(grim_rot_plan.output_rotations_xyzw.size(), 3,
                    "grim rotation export output count");
  ok &= expect_vec4_near(
      grim_rot_plan.output_rotations_xyzw[0],
      {0.0f, 0.0f, 0.38268343f, 0.9238795f},
      "grim rotation export weighted quat plus rotz");
  ok &= expect_vec4_near(grim_rot_plan.output_rotations_xyzw[1],
                         {0.0f, 1.0f, 0.0f, 0.0f},
                         "grim rotation export quat replacement");
  ok &= expect_vec4_near(grim_rot_plan.output_rotations_xyzw[2],
                         {0.0f, 0.0f, 0.0f, 1.0f},
                         "grim rotation export pos-only base rotation");
  const SourceGrimCharBonesSamplesExportRotationPlan grim_rot_empty =
      source_grim_char_bones_samples_export_rotation_plan({});
  ok &= expect_int(grim_rot_empty.has_rotation_samples ? 1 : 0, 0,
                   "grim empty rotation export has no samples");
  ok &= expect_size(grim_rot_empty.output_rotations_xyzw.size(), 0,
                    "grim empty rotation export emits no rotations");
  std::vector<ClipChannel> grim_channels(4);
  grim_channels[0].bone_name = "bone_z.mesh";
  grim_channels[0].type = ClipChannel::kPos;
  grim_channels[1].bone_name = "bone_a.mesh";
  grim_channels[1].type = ClipChannel::kQuat;
  grim_channels[2].bone_name = "bone_a.mesh";
  grim_channels[2].type = ClipChannel::kRotZ;
  grim_channels[3].bone_name = "bone_m.mesh";
  grim_channels[3].type = ClipChannel::kPos;
  source_grim_char_bones_samples_sort_decoded_channels(grim_channels);
  ok &= expect_string(grim_channels[0].bone_name, "bone_a.mesh",
                      "grim sort first target");
  ok &= expect_int(grim_channels[0].type, ClipChannel::kQuat,
                   "grim sort preserves first same-target channel");
  ok &= expect_int(grim_channels[1].type, ClipChannel::kRotZ,
                   "grim sort preserves second same-target channel");
  ok &= expect_string(grim_channels[2].bone_name, "bone_m.mesh",
                      "grim sort middle target");
  ok &= expect_string(grim_channels[3].bone_name, "bone_z.mesh",
                      "grim sort final target");
  ok &= expect_near(source_grim_char_bones_samples_decode_snorm16(0), 0.0f,
                    "grim snorm16 zero");
  ok &= expect_near(source_grim_char_bones_samples_decode_snorm16(32767), 1.0f,
                    "grim snorm16 positive max");
  ok &= expect_near(source_grim_char_bones_samples_decode_snorm16(-32768),
                    -1.0f, "grim snorm16 negative clamp");
  ok &= expect_near(source_grim_char_bones_samples_decode_snorm16(16384),
                    16384.0f / 32767.0f, "grim snorm16 mid value");
  const auto short_quat = source_grim_char_bones_samples_decode_short_quat(
      -32768, 0, 16384, 32767);
  ok &= expect_near(short_quat[0], -1.0f,
                    "grim short quat x negative clamp");
  ok &= expect_near(short_quat[1], 0.0f, "grim short quat y zero");
  ok &= expect_near(short_quat[2], 16384.0f / 32767.0f,
                    "grim short quat z mid value");
  ok &= expect_near(short_quat[3], 1.0f, "grim short quat w positive max");

  ok &= expect_size(
      source_grim_char_bones_samples_get_type_size(kSourceCharBonesTypePos, 0),
      16, "grim get_type_size pos uncompressed");
  ok &= expect_size(
      source_grim_char_bones_samples_get_type_size(kSourceCharBonesTypeScale,
                                                   0),
      16, "grim get_type_size scale uncompressed");
  ok &= expect_size(
      source_grim_char_bones_samples_get_type_size(kSourceCharBonesTypePos, 2),
      6, "grim get_type_size pos compressed vectors");
  ok &= expect_size(
      source_grim_char_bones_samples_get_type_size(kSourceCharBonesTypeQuat, 3),
      4, "grim get_type_size quat compressed quats");
  ok &= expect_size(
      source_grim_char_bones_samples_get_type_size(kSourceCharBonesTypeRotZ, 1),
      2, "grim get_type_size rotz compressed rots");
  ok &= expect_size(
      source_grim_char_bones_samples_get_type_size2(kSourceCharBonesTypePos, 0),
      12, "grim get_type_size2 pos uncompressed");
  ok &= expect_size(
      source_grim_char_bones_samples_get_type_size2(kSourceCharBonesTypeScale,
                                                    0),
      4, "grim get_type_size2 scale uncompressed");
  ok &= expect_size(
      source_grim_char_bones_samples_get_type_size2(kSourceCharBonesTypeQuat,
                                                    0),
      16, "grim get_type_size2 quat uncompressed");
  ok &= expect_size(
      source_grim_char_bones_samples_get_type_size2(kSourceCharBonesTypeQuat,
                                                    3),
      4, "grim get_type_size2 quat compressed quats");
  ok &= expect_size(
      source_grim_char_bones_samples_get_type_size2(kSourceCharBonesTypeRotX,
                                                    2),
      2, "grim get_type_size2 rotx compressed vectors");
  ok &= expect_size(
      source_grim_char_bones_samples_get_type_size2(kSourceCharBonesTypePos, 4),
      0, "grim get_type_size2 rejects unsupported compression");

  const SourceGrimCharBonesSamplesComputedSizes grim_sizes1 =
      source_grim_char_bones_samples_recompute_sizes(
          1, {0, 1, 1, 22, 22, 22, 32});
  ok &= expect_int(grim_sizes1.valid ? 1 : 0, 1,
                   "grim recompute compression 1 valid");
  ok &= expect_int(static_cast<int>(grim_sizes1.computed_sizes[0]), 0,
                   "grim recompute c1 size 0");
  ok &= expect_int(static_cast<int>(grim_sizes1.computed_sizes[1]), 16,
                   "grim recompute c1 size 1");
  ok &= expect_int(static_cast<int>(grim_sizes1.computed_sizes[3]), 184,
                   "grim recompute c1 quat boundary");
  ok &= expect_int(static_cast<int>(grim_sizes1.computed_sizes[6]), 204,
                   "grim recompute c1 final size");
  ok &= expect_int(static_cast<int>(grim_sizes1.computed_flags), 208,
                   "grim recompute c1 flags");

  const SourceGrimCharBonesSamplesComputedSizes grim_sizes2 =
      source_grim_char_bones_samples_recompute_sizes(
          2, {0, 36, 36, 53, 53, 53, 53});
  ok &= expect_int(grim_sizes2.valid ? 1 : 0, 1,
                   "grim recompute compression 2 valid");
  ok &= expect_int(static_cast<int>(grim_sizes2.computed_sizes[1]), 216,
                   "grim recompute c2 pos size");
  ok &= expect_int(static_cast<int>(grim_sizes2.computed_sizes[3]), 352,
                   "grim recompute c2 quat boundary");
  ok &= expect_int(static_cast<int>(grim_sizes2.computed_flags), 352,
                   "grim recompute c2 flags");
  ok &= expect_int(source_grim_char_bones_samples_recompute_sizes(
                       4, {0, 1, 1, 1, 1, 1, 1})
                       .valid
                       ? 1
                       : 0,
                   0, "grim recompute rejects unsupported compression");
  ok &= expect_int(source_grim_char_bones_samples_recompute_sizes(
                       1, {0, 2, 1, 1, 1, 1, 1})
                       .valid
                       ? 1
                       : 0,
                   0, "grim recompute rejects decreasing counts");

  const SourceGrimCharBonesSamplesDataPlan grim_data10 =
      source_grim_char_bones_samples_data_plan(
          10, kCompressRots,
          {"bone_root.pos", "bone_root.quat", "bone_root.rotz"}, 3);
  ok &= expect_int(grim_data10.known_version ? 1 : 0, 1,
                   "grim data v10 known");
  ok &= expect_size(grim_data10.kept_channels.size(), 3,
                    "grim data v10 kept channels");
  ok &= expect_size(grim_data10.channel_sizes[0], 12,
                    "grim data v10 pos size");
  ok &= expect_size(grim_data10.channel_sizes[1], 8,
                    "grim data v10 quat size");
  ok &= expect_size(grim_data10.channel_sizes[2], 2,
                    "grim data v10 rotz size");
  ok &= expect_size(grim_data10.unaligned_sample_size, 22,
                    "grim data v10 unaligned sample size");
  ok &= expect_size(grim_data10.sample_size, 22,
                    "grim data v10 sample size");
  ok &= expect_size(grim_data10.total_sample_bytes, 66,
                    "grim data v10 total bytes");
  ok &= expect_int(grim_data10.aligns_sample_data_to_4 ? 1 : 0, 0,
                   "grim data v10 no alignment");

  const SourceGrimCharBonesSamplesDataPlan grim_data16 =
      source_grim_char_bones_samples_data_plan(
          16, kCompressRots,
          {"bone_root.pos", "bone_root.quat", "bone_root.rotz"}, 3);
  ok &= expect_int(grim_data16.aligns_sample_data_to_4 ? 1 : 0, 1,
                   "grim data v16 aligns");
  ok &= expect_size(grim_data16.unaligned_sample_size, 22,
                    "grim data v16 unaligned sample size");
  ok &= expect_size(grim_data16.sample_size, 24,
                    "grim data v16 aligned sample size");
  ok &= expect_size(grim_data16.total_sample_bytes, 72,
                    "grim data v16 total bytes");

  const SourceGrimCharBonesSamplesDataPlan grim_data_invalid =
      source_grim_char_bones_samples_data_plan(
          10, kCompressRots, {"bone.root.pos", "bone_root.pos"}, 1);
  ok &= expect_size(grim_data_invalid.ignored_channels.size(), 1,
                    "grim data invalid channel ignored");
  ok &= expect_string(grim_data_invalid.ignored_channels[0],
                      "bone.root.pos",
                      "grim data invalid first-dot channel");
  ok &= expect_size(grim_data_invalid.sample_size, 12,
                    "grim data invalid channel skipped size");
  ok &= expect_int(source_grim_char_bones_samples_data_plan(
                       13, kCompressRots, {"bone_root.pos"}, 1)
                       .known_version
                       ? 1
                       : 0,
                   0, "grim data rejects unsupported version");

  const SourceGrimCharBonesSamplesHeaderPlan grim_header10 =
      source_grim_char_bones_samples_header_plan(10);
  ok &= expect_int(grim_header10.known_version ? 1 : 0, 1,
                   "grim header v10 known");
  ok &= expect_int(grim_header10.count_size, 10,
                   "grim header v10 count size");
  ok &= expect_int(grim_header10.defaults_weight ? 1 : 0, 1,
                   "grim header v10 defaults weights");
  ok &= expect_int(grim_header10.reads_weight ? 1 : 0, 0,
                   "grim header v10 skips weights");
  ok &= expect_int(grim_header10.reads_frame_table ? 1 : 0, 0,
                   "grim header v10 skips frames");
  ok &= expect_int(grim_header10.aligns_sample_data_to_4 ? 1 : 0, 0,
                   "grim header v10 skips alignment");
  ok &= expect_string(grim_header10.read_order[2], "default_weight_1.0",
                      "grim header v10 default weight row");

  const SourceGrimCharBonesSamplesHeaderPlan grim_header11 =
      source_grim_char_bones_samples_header_plan(11);
  ok &= expect_int(grim_header11.reads_weight ? 1 : 0, 1,
                   "grim header v11 reads weights");
  ok &= expect_string(grim_header11.read_order[2], "weights",
                      "grim header v11 weight row");

  const SourceGrimCharBonesSamplesHeaderPlan grim_header16 =
      source_grim_char_bones_samples_header_plan(16);
  ok &= expect_int(grim_header16.count_size, 7,
                   "grim header v16 count size");
  ok &= expect_int(grim_header16.reads_frame_table ? 1 : 0, 1,
                   "grim header v16 reads frames");
  ok &= expect_int(grim_header16.aligns_sample_data_to_4 ? 1 : 0, 1,
                   "grim header v16 aligns data");

  const SourceGrimCharClipLoadPlan grim_clip5 =
      source_grim_char_clip_load_plan(5, true);
  ok &= expect_int(grim_clip5.known_version ? 1 : 0, 1,
                   "grim CharClip v5 known");
  ok &= expect_int(grim_clip5.reads_object_meta ? 1 : 0, 1,
                   "grim CharClip reads meta");
  ok &= expect_int(grim_clip5.skips_v5_unknown_bool ? 1 : 0, 1,
                   "grim CharClip v5 skips unknown bool");
  ok &= expect_int(grim_clip5.reads_deprecated_events ? 1 : 0, 1,
                   "grim CharClip v5 reads deprecated events");
  ok &= expect_int(grim_clip5.reads_node_size ? 1 : 0, 0,
                   "grim CharClip v5 skips node size");
  ok &= expect_string(grim_clip5.read_order[1], "Object::Load",
                      "grim CharClip meta read order");

  const SourceGrimCharClipLoadPlan grim_clip12 =
      source_grim_char_clip_load_plan(12, true);
  ok &= expect_int(grim_clip12.reads_relative ? 1 : 0, 1,
                   "grim CharClip v12 reads relative");
  ok &= expect_int(grim_clip12.reads_unknown_1 ? 1 : 0, 1,
                   "grim CharClip v12 reads unknown_1");
  ok &= expect_int(grim_clip12.reads_do_not_decompress ? 1 : 0, 1,
                   "grim CharClip v12 reads do_not_decompress");
  ok &= expect_int(grim_clip12.reads_node_size ? 1 : 0, 1,
                   "grim CharClip v12 reads node size");

  const SourceGrimCharClipSamplesLoadPlan grim_clip_samples10 =
      source_grim_char_clip_samples_load_plan(10);
  ok &= expect_int(grim_clip_samples10.known_version ? 1 : 0, 1,
                   "grim CharClipSamples v10 known");
  ok &= expect_int(grim_clip_samples10.calls_char_clip_with_meta ? 1 : 0, 1,
                   "grim CharClipSamples calls CharClip metadata loader");
  ok &= expect_int(grim_clip_samples10.legacy_split_headers_and_data ? 1 : 0,
                   1, "grim CharClipSamples v10 split headers/data");
  ok &= expect_int(grim_clip_samples10.reads_duplicate_legacy_header ? 1 : 0,
                   1, "grim CharClipSamples v10 duplicate header");
  ok &= expect_int(grim_clip_samples10.runtime_data_lists, 2,
                   "grim CharClipSamples v10 runtime data list count");
  ok &= expect_string(grim_clip_samples10.read_order[2], "full.header",
                      "grim CharClipSamples v10 first header");
  ok &= expect_string(grim_clip_samples10.read_order[4], "duplicate.header",
                      "grim CharClipSamples v10 duplicate header order");
  ok &= expect_string(grim_clip_samples10.read_order[5], "full.data",
                      "grim CharClipSamples v10 full data order");

  const SourceGrimCharClipSamplesLoadPlan grim_clip_samples16 =
      source_grim_char_clip_samples_load_plan(16);
  ok &= expect_int(grim_clip_samples16.reads_some_bool ? 1 : 0, 1,
                   "grim CharClipSamples v16 reads some_bool");
  ok &= expect_int(grim_clip_samples16.legacy_split_headers_and_data ? 1 : 0,
                   0, "grim CharClipSamples v16 uses standalone samples");
  ok &= expect_int(grim_clip_samples16.reads_extra_bones ? 1 : 0, 1,
                   "grim CharClipSamples v16 reads extra bones");

  const SourceGrimCharClipSamplesExtraBonesPlan extra_bones10 =
      source_grim_char_clip_samples_extra_bones_plan(10);
  ok &= expect_int(extra_bones10.active ? 1 : 0, 0,
                   "grim CharClipSamples v10 has no extra bones block");
  ok &= expect_size(extra_bones10.read_order.size(), 0,
                    "grim CharClipSamples v10 extra bones order empty");

  const SourceGrimCharClipSamplesExtraBonesPlan extra_bones16 =
      source_grim_char_clip_samples_extra_bones_plan(16);
  ok &= expect_int(extra_bones16.active ? 1 : 0, 1,
                   "grim CharClipSamples v16 extra bones block active");
  ok &= expect_int(extra_bones16.reads_count ? 1 : 0, 1,
                   "grim CharClipSamples v16 extra bones reads count");
  ok &= expect_int(extra_bones16.reads_name ? 1 : 0, 1,
                   "grim CharClipSamples v16 extra bones reads names");
  ok &= expect_int(extra_bones16.reads_weight ? 1 : 0, 1,
                   "grim CharClipSamples v16 extra bones reads weights");
  ok &= expect_int(extra_bones16.stores_runtime_rows ? 1 : 0, 0,
                   "grim CharClipSamples v16 extra bones rows are discarded");
  ok &= expect_size(extra_bones16.read_order.size(), 3,
                    "grim CharClipSamples v16 extra bones order size");
  ok &= expect_string(extra_bones16.read_order[0], "bone_count",
                      "grim CharClipSamples v16 extra bones count order");
  ok &= expect_string(extra_bones16.read_order[1], "name",
                      "grim CharClipSamples v16 extra bones name order");
  ok &= expect_string(extra_bones16.read_order[2], "weight",
                      "grim CharClipSamples v16 extra bones weight order");

  const SourceReNotesCharBonesSamplesDecodePlan re_notes_decode =
      source_re_notes_char_bones_samples_decode_plan();
  ok &= expect_int(re_notes_decode.sample_data_grouped_by_time ? 1 : 0, 1,
                   "re-notes samples grouped by time");
  ok &= expect_int(re_notes_decode.has_generic_rot_sample ? 1 : 0, 1,
                   "re-notes exposes generic RotSample");
  ok &= expect_int(re_notes_decode.active_reader_counts_pos ? 1 : 0, 1,
                   "re-notes active reader counts pos");
  ok &= expect_int(re_notes_decode.active_reader_counts_quat ? 1 : 0, 1,
                   "re-notes active reader counts quat");
  ok &= expect_int(re_notes_decode.active_reader_counts_rotz ? 1 : 0, 1,
                   "re-notes active reader counts rotz");
  ok &= expect_int(re_notes_decode.active_reader_counts_rotx ? 1 : 0, 0,
                   "re-notes active reader does not count rotx");
  ok &= expect_int(re_notes_decode.active_reader_counts_roty ? 1 : 0, 0,
                   "re-notes active reader does not count roty");
  ok &= expect_int(re_notes_decode.active_reader_counts_scale ? 1 : 0, 0,
                   "re-notes active reader does not count scale");
  ok &= expect_size(re_notes_decode.active_sample_order.size(), 3,
                    "re-notes active sample order size");
  ok &= expect_string(re_notes_decode.active_sample_order[0], ".pos",
                      "re-notes active sample order pos");
  ok &= expect_string(re_notes_decode.active_sample_order[1], ".quat",
                      "re-notes active sample order quat");
  ok &= expect_string(re_notes_decode.active_sample_order[2], ".rotz",
                      "re-notes active sample order rotz");
  ok &= expect_size(re_notes_decode.fenced_channels.size(), 3,
                    "re-notes fenced channel count");
  ok &= expect_string(re_notes_decode.fenced_channels[1], ".rotx",
                      "re-notes fences rotx");

  const SourceProblemCharacterClipRawAxisAudit problem_raw_axis =
      source_problem_character_clip_raw_axis_audit_20260714();
  ok &= expect_string(
      problem_raw_axis.artifact,
      "engine/out/source_clip_audit_20260714/problem_character_clip_audit.log",
      "problem character raw-axis audit artifact");
  ok &= expect_size(problem_raw_axis.rows.size(), 7,
                    "problem character raw-axis row count");
  ok &= expect_string(problem_raw_axis.rows[0].milo,
                      "char/rock1/anims/gen/rock1_fret.milo_ps2",
                      "problem character raw-axis first row");
  ok &= expect_int(problem_raw_axis.rows[1].clips, 113,
                   "problem character rock1 main clip count");
  ok &= expect_int(problem_raw_axis.rows[1].accepted, 113,
                   "problem character rock1 main accepted count");
  ok &= expect_int(problem_raw_axis.rows[1].frames, 16247,
                   "problem character rock1 main frame total");
  ok &= expect_string(
      problem_raw_axis.rows[4].milo,
      "char/rockabill1/anims/gen/rockabill1_main.milo_ps2",
      "problem character rockabill shared main row");
  ok &= expect_int(problem_raw_axis.rows[4].clips, 116,
                   "problem character rockabill main clip count");
  ok &= expect_int(problem_raw_axis.rows[4].accepted, 116,
                   "problem character rockabill main accepted count");
  int total_problem_clips = 0;
  int total_fenced_raw = 0;
  int total_raw_scale = 0;
  int total_raw_rotx = 0;
  int total_raw_roty = 0;
  for (const auto& row : problem_raw_axis.rows) {
    total_problem_clips += row.clips;
    total_fenced_raw += row.fenced_clips;
    total_raw_scale += row.raw_scale;
    total_raw_rotx += row.raw_rotx;
    total_raw_roty += row.raw_roty;
  }
  ok &= expect_int(total_problem_clips, 338,
                   "problem character raw-axis total clip count");
  ok &= expect_int(total_fenced_raw, 0,
                   "problem character raw-axis total fenced clips");
  ok &= expect_int(total_raw_scale + total_raw_rotx + total_raw_roty, 0,
                   "problem character raw-axis total fenced channel rows");
  ok &= expect_int(problem_raw_axis.all_rows_accepted ? 1 : 0, 1,
                   "problem character raw-axis all rows accepted");
  ok &= expect_int(
      problem_raw_axis.all_problem_rows_have_zero_fenced_raw ? 1 : 0, 1,
      "problem character raw-axis all rows have zero fenced raw");
  ok &= expect_int(
      problem_raw_axis.supports_publisher_gap_not_raw_axis_gap ? 1 : 0, 1,
      "problem character raw-axis points back to publisher gap");
  ok &= expect_size(problem_raw_axis.shared_animation_notes.size(), 2,
                    "problem character shared animation note count");
  ok &= expect_string(
      problem_raw_axis.shared_animation_notes[0],
      "rock2 has no private CharClipSamples rows under char/rock2 in the "
      "stock GH2 ARK",
      "problem character rock2 shared animation note");

  const SourceCharBonesSamplesPropSyncPlan samples_prop_sync =
      source_char_bones_samples_prop_sync_plan();
  ok &= expect_size(samples_prop_sync.properties.size(), 2,
                    "samples prop sync direct count");
  ok &= expect_string(samples_prop_sync.properties[0], "num_samples",
                      "samples prop sync num_samples");
  ok &= expect_string(samples_prop_sync.properties[1], "frames",
                      "samples prop sync frames");
  ok &= expect_size(samples_prop_sync.set_properties.size(), 2,
                    "samples prop sync set count");
  ok &= expect_string(samples_prop_sync.set_properties[0], "preview_sample",
                      "samples prop sync preview_sample");
  ok &= expect_string(samples_prop_sync.set_properties[1], "compression",
                      "samples prop sync compression");
  ok &= expect_size(samples_prop_sync.custom_branches.size(), 1,
                    "samples prop sync custom branch count");
  ok &= expect_string(samples_prop_sync.custom_branches[0], "bones",
                      "samples prop sync bones branch");

  const SourceCharBonesSamplesBodyBoundary samples_boundary =
      source_char_bones_samples_body_boundary();
  ok &= expect_int(samples_boundary.rb3_latest_load_delegates_header ? 1 : 0,
                   1, "samples rb3 load delegates header");
  ok &= expect_int(samples_boundary.rb3_latest_declares_load_header ? 1 : 0,
                   1, "samples rb3 declares LoadHeader");
  ok &= expect_int(
      samples_boundary.rb3_latest_exposes_load_header_body ? 1 : 0, 0,
      "samples rb3 lacks LoadHeader body");
  ok &= expect_int(samples_boundary.rb2_dump_maps_load_header ? 1 : 0, 1,
                   "samples rb2 maps LoadHeader");
  ok &= expect_int(samples_boundary.rb2_dump_exposes_statement_body ? 1 : 0,
                   0, "samples rb2 dump lacks statement body");
  ok &= expect_int(samples_boundary.safe_to_decode_logged_rows ? 1 : 0, 1,
                   "samples boundary allows row decode");
  ok &= expect_int(samples_boundary.safe_to_publish_pose ? 1 : 0, 0,
                   "samples boundary fences pose publish");
  ok &= expect_size(samples_boundary.fenced_bodies.size(), 4,
                    "samples fenced body count");
  ok &= expect_string(samples_boundary.fenced_bodies[0],
                      "CharBonesSamples::LoadHeader",
                      "samples first fenced body");
  ok &= expect_string(samples_boundary.fenced_bodies[2],
                      "CharBonesSamples::EvaluateChannel",
                      "samples evaluate channel fenced");

  const SourceCharBonesSamplesRuntimeDumpEvidence samples_dump =
      source_char_bones_samples_runtime_dump_evidence();
  ok &= expect_string(samples_dump.frac_to_sample_range,
                      "0x80323420 -> 0x80323654",
                      "samples dump FracToSample range");
  ok &= expect_string(samples_dump.evaluate_channel_range,
                      "0x80323654 -> 0x80323E64",
                      "samples dump EvaluateChannel range");
  ok &= expect_string(samples_dump.rotate_by_range,
                      "0x80323E64 -> 0x80323E7C",
                      "samples dump RotateBy range");
  ok &= expect_string(samples_dump.rotate_to_range,
                      "0x80323E7C -> 0x80323F3C",
                      "samples dump RotateTo range");
  ok &= expect_string(samples_dump.scale_add_sample_range,
                      "0x80323F3C -> 0x80323FFC",
                      "samples dump ScaleAddSample range");
  ok &= expect_string(samples_dump.relativize_range,
                      "0x80323FFC -> 0x803250DC",
                      "samples dump Relativize range");
  ok &= expect_string(samples_dump.load_header_range,
                      "0x80325C9C -> 0x80326054",
                      "samples dump LoadHeader range");
  ok &= expect_string(samples_dump.load_data_range,
                      "0x80326054 -> 0x80326370",
                      "samples dump LoadData range");
  ok &= expect_string(samples_dump.sync_property_range,
                      "0x803263A8 -> 0x803266E8",
                      "samples dump SyncProperty range");
  ok &= expect_size(samples_dump.frac_to_sample_locals.size(), 3,
                    "samples dump FracToSample locals");
  ok &= expect_string(samples_dump.frac_to_sample_locals[2], "float w",
                      "samples dump FracToSample weight local");
  ok &= expect_int(samples_dump.rb3_latest_declares_frac_to_sample ? 1 : 0, 1,
                   "samples latest declares FracToSample");
  ok &= expect_int(samples_dump.rb2_dump_maps_frac_to_sample ? 1 : 0, 1,
                   "samples dump maps FracToSample");
  ok &= expect_int(samples_dump.has_frac_to_sample_statement_body ? 1 : 0, 0,
                   "samples dump lacks FracToSample body");
  ok &= expect_int(samples_dump.safe_to_use_source_frac_to_sample ? 1 : 0, 0,
                   "samples dump fences source FracToSample");
  ok &= expect_size(samples_dump.evaluate_channel_locals.size(), 9,
                    "samples dump EvaluateChannel locals");
  ok &= expect_string(samples_dump.evaluate_channel_locals[0],
                      "const char* src",
                      "samples dump EvaluateChannel first local");
  ok &= expect_string(samples_dump.evaluate_channel_locals[8], "Vector3 b",
                      "samples dump EvaluateChannel last local");
  ok &= expect_size(samples_dump.relativize_locals.size(), 29,
                    "samples dump Relativize locals");
  ok &= expect_string(samples_dump.relativize_locals[1], "const Bone* bone",
                      "samples dump Relativize bone local");
  ok &= expect_string(samples_dump.relativize_locals[24], "Quat first",
                      "samples dump Relativize quat first local");
  ok &= expect_size(samples_dump.load_header_locals.size(), 4,
                    "samples dump LoadHeader locals");
  ok &= expect_string(samples_dump.load_header_locals[2], "int count",
                      "samples dump LoadHeader count local");
  ok &= expect_size(samples_dump.load_data_locals.size(), 15,
                    "samples dump LoadData locals");
  ok &= expect_string(samples_dump.load_data_locals[1],
                      "const ShortVector3* send",
                      "samples dump LoadData first vector local");
  ok &= expect_string(samples_dump.load_data_locals[13],
                      "const float* rend",
                      "samples dump LoadData float end local");
  ok &= expect_int(samples_dump.has_load_header_statement_body ? 1 : 0, 0,
                   "samples dump lacks LoadHeader body");
  ok &= expect_int(samples_dump.has_load_data_statement_body ? 1 : 0, 0,
                   "samples dump lacks LoadData body");
  ok &= expect_int(
      samples_dump.has_evaluate_channel_statement_body ? 1 : 0, 0,
      "samples dump lacks EvaluateChannel body");
  ok &= expect_int(samples_dump.has_relativize_statement_body ? 1 : 0, 0,
                   "samples dump lacks Relativize body");
  ok &= expect_int(samples_dump.safe_to_decode_logged_rows ? 1 : 0, 1,
                   "samples dump allows row decode");
  ok &= expect_int(samples_dump.safe_to_publish_pose ? 1 : 0, 0,
                   "samples dump fences pose publish");

  const SourceCharClipSamplesRuntimeDumpEvidence clip_samples_dump =
      source_char_clip_samples_runtime_dump_evidence();
  ok &= expect_string(clip_samples_dump.facing_bones_set_range,
                      "0x803331CC -> 0x80333344",
                      "clip samples dump FacingBones::Set range");
  ok &= expect_string(clip_samples_dump.facing_set_scale_add_range,
                      "0x80333414 -> 0x80333600",
                      "clip samples dump FacingSet::ScaleAdd range");
  ok &= expect_string(clip_samples_dump.frame_to_sample_range,
                      "0x8033373C -> 0x8033376C",
                      "clip samples dump FrameToSample range");
  ok &= expect_string(clip_samples_dump.evaluate_channel_range,
                      "0x80333A24 -> 0x80333AB8",
                      "clip samples dump EvaluateChannel range");
  ok &= expect_string(clip_samples_dump.evaluate_channel_sample_range,
                      "0x80333AB8 -> 0x80333B18",
                      "clip samples dump EvaluateChannel sample range");
  ok &= expect_string(clip_samples_dump.rotate_by_range,
                      "0x80333BCC -> 0x80333C70",
                      "clip samples dump RotateBy range");
  ok &= expect_string(clip_samples_dump.rotate_to_range,
                      "0x80333C70 -> 0x80333CF4",
                      "clip samples dump RotateTo range");
  ok &= expect_string(clip_samples_dump.scale_add_frame_range,
                      "0x80333CF4 -> 0x80333DAC",
                      "clip samples dump ScaleAdd frame range");
  ok &= expect_string(clip_samples_dump.scale_add_sample_range,
                      "0x80333DAC -> 0x80333E78",
                      "clip samples dump ScaleAdd sample range");
  ok &= expect_string(clip_samples_dump.relativize_range,
                      "0x80333ED4 -> 0x80333F94",
                      "clip samples dump Relativize range");
  ok &= expect_string(clip_samples_dump.load_range,
                      "0x80334274 -> 0x80334470",
                      "clip samples dump Load range");
  ok &= expect_size(clip_samples_dump.facing_set_scale_add_locals.size(), 3,
                    "clip samples dump FacingSet locals");
  ok &= expect_string(clip_samples_dump.facing_set_scale_add_locals[0],
                      "Vector3 curPos",
                      "clip samples dump first FacingSet local");
  ok &= expect_size(clip_samples_dump.evaluate_channel_sample_locals.size(), 2,
                    "clip samples dump sample evaluate locals");
  ok &= expect_string(clip_samples_dump.scale_add_frame_locals[3],
                      "int lastSample",
                      "clip samples dump ScaleAdd last sample local");
  ok &= expect_string(clip_samples_dump.load_locals[0],
                      "CharBonesSamples delta",
                      "clip samples dump Load local");
  ok &= expect_int(
      clip_samples_dump.has_evaluate_channel_statement_body ? 1 : 0, 0,
      "clip samples dump lacks EvaluateChannel body");
  ok &= expect_int(clip_samples_dump.has_rotate_by_statement_body ? 1 : 0, 0,
                   "clip samples dump lacks RotateBy body");
  ok &= expect_int(clip_samples_dump.has_scale_add_statement_body ? 1 : 0, 0,
                   "clip samples dump lacks ScaleAdd body");
  ok &= expect_int(clip_samples_dump.has_load_statement_body ? 1 : 0, 0,
                   "clip samples dump lacks Load body");
  ok &= expect_int(clip_samples_dump.safe_to_publish_pose ? 1 : 0, 0,
                   "clip samples dump fences pose publish");

  return ok ? 0 : 1;
}
