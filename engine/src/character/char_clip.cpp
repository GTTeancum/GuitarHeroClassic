// engine/src/character/char_clip.cpp
//
// CharClipSamples / CharBonesSamples decoder.
//
// Decoder evidence is bounded by ihatecompvir source. rb3-latest exposes
// CharClip/CharBones/CharBonesSamples layouts and call flow, while the
// rb3-retail-old RB2 dump maps CharClipSamples runtime functions. Some exact
// sample math bodies are still absent from the checked public C++ source. Keep
// broad output writes diagnostic until the corresponding runtime path is ported
// from source or proved by direct trace.
//
// Grim-backed GH2-era format:
//  A CharClipSamples entry contains a CharClipSamples version, the CharClip
//  base (metadata, beat/play flags, nodes/events), then full/one
//  CharBonesSamples headers plus a duplicate legacy header. Runtime sample data
//  follows only for the full and one headers. Each bone list header is:
//      uint32 bone_count
//      bone_count x { length-prefixed name (ends .pos/.scale/.quat/.rotx/.roty/.rotz),
//                     float32 weight }     (weight present for gRev>10)
//      uint32 cum_counts[10]   cumulative bone count per category (0..9)
//      uint32 compression      source CharBones::CompressionType:
//                              0 none, 1 rotations, 2 vectors,
//                              3 quats, 4 all
//      uint32 numSamples       number of frames
//  Then, AFTER every bone-list header, the full/one sample data blocks follow
//  in list order (two-pass: all defs, then data for the live lists). Each
//  list's block is:
//      numSamples x frame, where each frame is walked in serialized bone order:
//         vectors (.pos + .scale):  float32x3 (12B) or int16x3 (6B)
//         quats   (.quat):          float32x4 (16B), int16x4 (8B), or
//                                   source ByteQuat (4B)
//         scalar axes (.rotx/.roty/.rotz): float32 (4B) or int16 (2B)
//
// Source-backed bone classification: .pos=0 .scale=1 .quat=2
// .rotx=3 .roty=4 .rotz=5, matching ihatecompvir CharBones::Type.

#include "character/char_clip.h"

#include "ark_v3.h"
#include "milo.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ghogx::character {

// Grim decode_samples rewrites channel suffixes to .mesh names before grouping.
// Defined below; forward-declared so the anonymous-namespace parser can use it.
std::string source_grim_char_bones_samples_channel_mesh_name(
    const std::string& channel);
static void source_rotate_about_z_vec(float v[3], float angle);
static float source_limit_ang(float radians);

int source_char_bones_type_of(const std::string& channel) {
  for (size_t dot = channel.find('.'); dot != std::string::npos;
       dot = channel.find('.', dot + 1)) {
    if (dot + 1 >= channel.size()) break;
    switch (channel[dot + 1]) {
      case 'p':
        return kSourceCharBonesTypePos;
      case 's':
        return kSourceCharBonesTypeScale;
      case 'q':
        return kSourceCharBonesTypeQuat;
      case 'r':
        if (dot + 4 < channel.size()) {
          const char axis = channel[dot + 4];
          if (axis >= 'x' && axis <= 'z') {
            return kSourceCharBonesTypeRotX + (axis - 'x');
          }
        }
        break;
      default:
        break;
    }
  }
  return kSourceCharBonesTypeEnd;
}

const char* source_char_bones_suffix_of(int type) {
  static const char* suffixes[kSourceCharBonesTypeEnd] = {
      "pos", "scale", "quat", "rotx", "roty", "rotz"};
  if (type < 0 || type >= kSourceCharBonesTypeEnd) return "";
  return suffixes[type];
}

std::string source_char_bones_channel_name(const std::string& name, int type) {
  const char* suffix = source_char_bones_suffix_of(type);
  if (!suffix[0]) return name;
  std::string out = name;
  size_t dot = out.find('.');
  if (dot == std::string::npos) {
    out.push_back('.');
    dot = out.size() - 1;
  }
  out.resize(dot + 1);
  out += suffix;
  return out;
}

size_t source_char_bones_type_size(int type, int compression) {
  if (type < 0 || type >= kSourceCharBonesTypeEnd) return 0u;
  if (type < kSourceCharBonesTypeQuat) {
    return compression < 2 ? 12u : 6u;
  }
  if (type != kSourceCharBonesTypeQuat) {
    return compression == 0 ? 4u : 2u;
  }
  if (compression > 2) return 4u;
  if (compression == 0) return 16u;
  return 8u;
}

SourceCharBonesLayout source_char_bones_recompute_layout(
    const std::array<int, kSourceCharBonesTypeEnd + 1>& counts,
    int compression) {
  SourceCharBonesLayout layout;
  layout.counts = counts;
  layout.offsets[0] = 0;
  for (int type = 0; type < kSourceCharBonesTypeEnd; ++type) {
    const int diff = counts[type + 1] - counts[type];
    const int size = static_cast<int>(
        source_char_bones_type_size(type, compression));
    layout.offsets[type + 1] = layout.offsets[type] + diff * size;
  }
  layout.total_size = (layout.offsets[kSourceCharBonesTypeEnd] + 0xF) &
                      ~0xF;
  return layout;
}

SourceCharBonesCompressionUpdate source_char_bones_set_compression(
    int current_compression,
    const SourceCharBonesLayout& current_layout,
    int requested_compression) {
  SourceCharBonesCompressionUpdate update;
  update.compression = current_compression;
  update.layout = current_layout;
  if (requested_compression != current_compression) {
    update.compression = requested_compression;
    update.layout = source_char_bones_recompute_layout(current_layout.counts,
                                                       requested_compression);
    update.changed = true;
  }
  return update;
}

SourceCharBonesState source_char_bones_empty_state() {
  return SourceCharBonesState{};
}

void source_char_bones_clear(SourceCharBonesState& state) {
  state.bones.clear();
  state.layout = SourceCharBonesLayout{};
  state.compression = 0;
}

void source_char_bones_set_weights(std::vector<SourceCharBonesBone>& bones,
                                   float weight) {
  for (SourceCharBonesBone& bone : bones) {
    bone.weight = weight;
  }
}

void source_char_bones_set_weights(SourceCharBonesState& state, float weight) {
  source_char_bones_set_weights(state.bones, weight);
}

void source_char_bones_list_bones(const SourceCharBonesState& state,
                                  std::vector<SourceCharBonesBone>& bones) {
  for (const SourceCharBonesBone& bone : state.bones) {
    bones.push_back(bone);
  }
}

int source_char_bones_find_offset(const SourceCharBonesState& state,
                                  const std::string& channel) {
  const int type = source_char_bones_type_of(channel);
  if (type < 0 || type >= kSourceCharBonesTypeEnd) return -1;
  const int next_count = state.layout.counts[type + 1];
  const int count = state.layout.counts[type];
  if (count < 0 || next_count < count) return -1;
  const int type_size =
      static_cast<int>(source_char_bones_type_size(type, state.compression));
  int offset = state.layout.offsets[type];
  for (int i = 0; i < next_count - count; ++i) {
    const int bone_index = count + i;
    if (bone_index >= 0 &&
        static_cast<size_t>(bone_index) < state.bones.size() &&
        state.bones[static_cast<size_t>(bone_index)].name == channel) {
      return offset;
    }
    offset += type_size;
  }
  return -1;
}

SourceCharBonesFindPtrResult source_char_bones_find_ptr(
    const SourceCharBonesState& state,
    const std::string& channel) {
  SourceCharBonesFindPtrResult result;
  result.offset = source_char_bones_find_offset(state, channel);
  result.found = result.offset != -1;
  return result;
}

void source_char_bones_zero(std::vector<uint8_t>& start, int total_size) {
  std::fill(start.begin(), start.begin() + total_size, uint8_t{0});
}

SourceCharBonesScaleAddClipStep source_char_bones_scale_add_clip_step(
    float f1, float f2, float f3) {
  SourceCharBonesScaleAddClipStep step;
  step.f1 = f1;
  step.f2 = f2;
  step.f3 = f3;
  return step;
}

SourceCharBonesPoseBodyBoundary source_char_bones_pose_body_boundary() {
  SourceCharBonesPoseBodyBoundary boundary;
  boundary.fenced_bodies = {
      "CharBones::ScaleAdd(CharBones&, float)",
      "CharBones::RotateBy",
      "CharBones::RotateTo",
      "CharBones::Blend",
      "CharBones::ScaleDown",
      "CharBones::ScaleAddIdentity",
  };
  return boundary;
}

SourceCharBonesRuntimeDumpEvidence source_char_bones_runtime_dump_evidence() {
  SourceCharBonesRuntimeDumpEvidence evidence;
  evidence.scale_down_range = "0x8031C058 -> 0x8031C33C";
  evidence.scale_add_range = "0x8031C33C -> 0x8031CB00";
  evidence.rotate_by_range = "0x8031CB00 -> 0x8031D118";
  evidence.rotate_to_range = "0x8031D118 -> 0x8031D864";
  evidence.scale_add_identity_range = "0x8031D864 -> 0x8031D8B0";
  evidence.blend_range = "0x8031F2C0 -> 0x8031F670";
  evidence.scale_down_locals = {"const Bone* name",
                                "const Bone* endName",
                                "const Bone* boneName",
                                "Vector3* pos",
                                "Quat* quat",
                                "float* ang",
                                "Vector3* pos",
                                "Quat* quat",
                                "float* ang"};
  evidence.scale_add_locals = {"const Bone* name",
                               "const Bone* endName",
                               "Bone* boneName",
                               "Vector3* pos",
                               "const ShortVector3* sp",
                               "Vector3 v",
                               "const Vector3* p",
                               "float fweight",
                               "Quat* quat",
                               "float fqweight",
                               "float qweight",
                               "const ByteQuat* bq",
                               "Quat s",
                               "float fqweight",
                               "float qweight",
                               "const ShortQuat* qs",
                               "Quat s",
                               "const Quat* q",
                               "Quat s",
                               "float* ang",
                               "float aweight",
                               "const signed short* as",
                               "const float* a"};
  evidence.rotate_by_locals = {"const Bone* name",
                               "const Bone* endName",
                               "const Bone* boneName",
                               "Vector3* pos",
                               "const ShortVector3* sp",
                               "Vector3 v",
                               "const Vector3* p",
                               "Quat* quat",
                               "const ByteQuat* bq",
                               "Quat s",
                               "const ShortQuat* qs",
                               "Quat s",
                               "const Quat* q",
                               "float* ang",
                               "const signed short* as",
                               "const float* a"};
  evidence.rotate_to_locals = {"const Bone* name",
                               "const Bone* endName",
                               "const Bone* boneName",
                               "Vector3* pos",
                               "const ShortVector3* sp",
                               "Vector3 v",
                               "const Vector3* p",
                               "Quat* quat",
                               "const ByteQuat* bq",
                               "Quat s",
                               "const ShortQuat* qs",
                               "Quat s",
                               "const Quat* q",
                               "Quat s",
                               "float* ang",
                               "float shortWeight",
                               "const signed short* as",
                               "const float* a"};
  evidence.scale_add_identity_locals = {"const Quat* end", "Quat* q"};
  evidence.blend_locals = {"const Bone* name",
                           "const Bone* endName",
                           "const Bone* boneName",
                           "const Vector3* p",
                           "Vector3* pos",
                           "Quat* quat",
                           "const Quat* q",
                           "float ds",
                           "float fweight",
                           "Quat s",
                           "float* ang",
                           "const float* a"};
  evidence.rb2_dump_maps_blend = true;
  evidence.has_scale_down_statement_body = false;
  evidence.has_scale_add_statement_body = false;
  evidence.has_rotate_by_statement_body = false;
  evidence.has_rotate_to_statement_body = false;
  evidence.has_scale_add_identity_statement_body = false;
  evidence.safe_to_apply_pose_math = false;
  return evidence;
}

std::vector<SourceCharPoseRuntimeSymbolEvidence>
source_char_pose_runtime_symbol_evidence() {
  const char* source =
      "rb3/config/SZBE69_B8/symbols.txt";
  return {
      {source, "CharBones::ScaleAdd(CharBones&, float)",
       "ScaleAdd__9CharBonesCFR9CharBonesf", "0x80689780", 0x8E8u},
      {source, "CharBones::Blend(CharBones&)",
       "Blend__9CharBonesCFR9CharBones", "0x8068A070", 0x440u},
      {source, "CharBones::RotateBy(CharBones&)",
       "RotateBy__9CharBonesCFR9CharBones", "0x8068A4B0", 0x7B8u},
      {source, "CharBones::RotateTo(CharBones&, float)",
       "RotateTo__9CharBonesCFR9CharBonesf", "0x8068AC70", 0x890u},
      {source, "CharBones::ScaleAddIdentity()",
       "ScaleAddIdentity__9CharBonesFv", "0x8068B500", 0x74u},
      {source, "CharBonesMeshes::AcquirePose()",
       "AcquirePose__15CharBonesMeshesFv", "0x8068E420", 0x2E0u},
      {source, "CharBonesMeshes::PoseMeshes()",
       "PoseMeshes__15CharBonesMeshesFv", "0x8068E700", 0x564u},
      {source, "CharBonesSamples::EvaluateChannel(void*, int, int, float)",
       "EvaluateChannel__16CharBonesSamplesFPviif", "0x80690180",
       0x75Cu},
      {source, "CharBonesSamples::ScaleAddSample(CharBones&, float, int, float)",
       "ScaleAddSample__16CharBonesSamplesFR9CharBonesfif", "0x806909D0",
       0xC8u},
      {source, "CharBonesSamples::Relativize(CharClip*)",
       "Relativize__16CharBonesSamplesFP8CharClip", "0x80690AA0",
       0x105Cu},
      {source, "CharClip::EvaluateChannel(void*, const void*, int, float)",
       "EvaluateChannel__8CharClipFPvPCvif", "0x80697CE0", 0x120u},
      {source, "CharClip::EvaluateChannel(void*, const void*, float)",
       "EvaluateChannel__8CharClipFPvPCvf", "0x80697E00", 0x60u},
      {source, "CharClipDriver::PreEvaluate(float, float, float, float)",
       "PreEvaluate__14CharClipDriverFfff", "0x8069FE50", 0x4A0u},
      {source, "CharClipDriver::Evaluate(float, float, float, float)",
       "Evaluate__14CharClipDriverFfff", "0x806A02F0", 0x560u},
      {source, "CharClipDriver::ScaleAdd(CharBones&, float)",
       "ScaleAdd__14CharClipDriverFR9CharBonesf", "0x806A0850",
       0x18Cu},
      {source, "CharClipDriver::RotateTo(CharBones&, float)",
       "RotateTo__14CharClipDriverFR9CharBonesf", "0x806A09E0",
       0x194u},
      {source, "CharDriver::SetBeatScale(float, bool)",
       "SetBeatScale__10CharDriverFfb", "0x806B32A0", 0x9Cu},
      {source, "CharDriver::EvaluateFlags(int)",
       "EvaluateFlags__10CharDriverFi", "0x806B3960", 0x1C8u},
      {source, "CharDriver::FirstPlaying()",
       "FirstPlaying__10CharDriverFv", "0x806B3B90", 0x2Cu},
      {source, "CharDriver::FirstPlayingClip()",
       "FirstPlayingClip__10CharDriverFv", "0x806B3BE0", 0x34u},
  };
}

SourceCharBonesAddBonesSteps source_char_bones_add_bones_steps(
    const std::vector<SourceCharBonesBone>& bones) {
  SourceCharBonesAddBonesSteps steps;
  steps.add_bone_internal_calls = bones;
  steps.reallocate_internal = true;
  return steps;
}

SourceCharBonesAllocReallocateStep source_char_bones_alloc_reallocate_step(
    int total_size) {
  SourceCharBonesAllocReallocateStep step;
  step.mem_alloc_size = total_size;
  return step;
}

SourceCharBonesEnterStep source_char_bones_enter_step() {
  return SourceCharBonesEnterStep{};
}

SourceCharBonesBlenderPollStep source_char_bones_blender_poll_step(
    bool bones_empty,
    bool has_dest) {
  SourceCharBonesBlenderPollStep step;
  if (bones_empty || !has_dest) {
    step.early_out = true;
    return step;
  }
  step.blend_dest = true;
  step.enter = true;
  return step;
}

SourceCharBonesBlenderSetDestStep source_char_bones_blender_set_dest_step(
    bool dest_changed,
    bool new_dest_exists) {
  SourceCharBonesBlenderSetDestStep step;
  if (!dest_changed) return step;
  step.changed = true;
  step.assign_dest = true;
  step.add_bones_to_dest = new_dest_exists;
  return step;
}

SourceCharBonesBlenderSetClipTypeStep
source_char_bones_blender_set_clip_type_step(bool clip_type_changed) {
  SourceCharBonesBlenderSetClipTypeStep step;
  if (!clip_type_changed) return step;
  step.changed = true;
  step.assign_clip_type = true;
  step.clear_bones = true;
  step.stuff_bones_from_dir = true;
  return step;
}

SourceCharBonesBlenderReallocateStep
source_char_bones_blender_reallocate_step(bool has_dest) {
  SourceCharBonesBlenderReallocateStep step;
  step.add_bones_to_dest = has_dest;
  return step;
}

SourceCharBonesBlenderLoadPlan source_char_bones_blender_load_plan(
    int32_t revision) {
  SourceCharBonesBlenderLoadPlan plan;
  plan.known_revision = revision >= 0 && revision <= 2;
  if (!plan.known_revision) return plan;

  plan.read_order = {"Hmx::Object", "boneObjPtr"};
  if (revision > 1) {
    plan.read_order.push_back("mClipType");
  } else {
    plan.branches.push_back("mClipType defaults empty");
  }
  plan.call_order = {"SetClipType", "SetDest"};
  return plan;
}

SourceCharBonesBlenderSavePlan source_char_bones_blender_save_plan() {
  return SourceCharBonesBlenderSavePlan{};
}

SourceCharBonesBlenderCopyPlan source_char_bones_blender_copy_plan() {
  SourceCharBonesBlenderCopyPlan plan;
  plan.copied_superclasses = {"Hmx::Object"};
  plan.member_calls = {"SetClipType", "SetDest"};
  return plan;
}

SourceCharBonesBlenderHandlerPlan source_char_bones_blender_handler_plan() {
  SourceCharBonesBlenderHandlerPlan plan;
  plan.superclasses = {"CharPollable", "Hmx::Object"};
  plan.check = 0x81;
  return plan;
}

SourceCharBonesBlenderPropSyncPlan
source_char_bones_blender_prop_sync_plan() {
  SourceCharBonesBlenderPropSyncPlan plan;
  plan.set_properties = {"dest", "clip_type"};
  plan.superclasses = {"CharBonesObject"};
  return plan;
}

void source_char_bones_blender_poll_deps(
    SourceCharBonesBlenderPollDeps& deps,
    const std::string& dest) {
  deps.change.push_back(dest);
}

SourceCharBoneLoadPlan source_char_bone_load_plan(int32_t revision) {
  SourceCharBoneLoadPlan plan;
  plan.known_revision = revision >= 0 && revision <= 10;
  if (!plan.known_revision) return plan;

  plan.read_order.push_back("Hmx::Object");
  if (revision < 9) plan.read_order.push_back("RndTransformableRemover");

  plan.read_order.push_back(revision > 6 ? "mPositionContext"
                                          : "mPositionContextBool");
  if (revision > 6) {
    plan.read_order.push_back("mScaleContext");
  } else if (revision > 1) {
    plan.read_order.push_back("mScaleContextBool");
  }

  plan.read_order.push_back("mRotation");
  if (revision < 5) plan.read_order.push_back("legacyPreRev5Int");
  if (revision < 2) {
    plan.branches.push_back("mScaleContext=0");
    plan.branches.push_back("mRotation=mRotation+1");
  }
  if (revision < 5) plan.branches.push_back("clampRotationToTypeEnd");

  if (revision > 6) {
    plan.read_order.push_back("mRotationContext");
  } else {
    plan.branches.push_back("mRotationContext=mRotation!=TYPE_END");
  }

  if (revision >= 3 && revision <= 7) {
    plan.read_order.push_back("legacyRev3To7Int");
  }
  if (revision > 3) plan.read_order.push_back("mTarget");
  if (revision == 6) {
    plan.read_order.push_back("sharedContext");
    plan.branches.push_back("nonzeroContextsUseSharedContext");
  }
  if (revision > 7) plan.read_order.push_back("mWeights");
  if (revision > 8) plan.read_order.push_back("mTrans");
  if (revision > 9) plan.read_order.push_back("mBakeOutAsTopLevel");
  return plan;
}

SourceCharBoneSavePlan source_char_bone_save_plan() {
  return SourceCharBoneSavePlan{};
}

SourceCharBoneCopyPlan source_char_bone_copy_plan() {
  SourceCharBoneCopyPlan plan;
  plan.copied_superclasses = {"Hmx::Object"};
  plan.copied_members = {"mRotationContext", "mScaleContext",
                         "mPositionContext", "mRotation",
                         "mTarget",          "mWeights",
                         "mTrans",           "mBakeOutAsTopLevel"};
  return plan;
}

SourceCharBoneHandlerPlan source_char_bone_handler_plan() {
  SourceCharBoneHandlerPlan plan;
  plan.action_handlers = {"clear_context"};
  plan.handlers = {"get_context_flags"};
  plan.superclasses = {"Hmx::Object"};
  plan.check = 0x152;
  return plan;
}

SourceCharBoneWeightContextPropSyncPlan
source_char_bone_weight_context_prop_sync_plan() {
  SourceCharBoneWeightContextPropSyncPlan plan;
  plan.properties = {"context", "weight"};
  return plan;
}

SourceCharBoneWeightContextDefaultState
source_char_bone_weight_context_default_state() {
  return SourceCharBoneWeightContextDefaultState{};
}

SourceCharBoneWeightContextLoadPlan source_char_bone_weight_context_load_plan() {
  SourceCharBoneWeightContextLoadPlan plan;
  plan.read_order = {"mContext", "mWeight"};
  return plan;
}

SourceCharBoneContextFlagsStep source_char_bone_get_context_flags_step(
    bool parent_is_char_bone_dir) {
  SourceCharBoneContextFlagsStep step;
  step.parent_is_char_bone_dir = parent_is_char_bone_dir;
  if (parent_is_char_bone_dir) {
    step.returns_dir_context_flags = true;
  } else {
    step.warns_no_char_bone_dir = true;
    step.returns_empty_array = true;
    step.warning = "CharBone: No CharBoneDir for context flags.";
  }
  return step;
}

SourceCharBonePropSyncPlan source_char_bone_prop_sync_plan() {
  SourceCharBonePropSyncPlan plan;
  plan.properties = {"position_context", "scale_context",
                     "rotation",         "rotation_context",
                     "target",           "weights",
                     "trans",            "bake_out_as_top_level"};
  plan.superclasses = {"Hmx::Object"};
  return plan;
}

SourceCharBonesBonePropSyncPlan source_char_bones_bone_prop_sync_plan() {
  SourceCharBonesBonePropSyncPlan plan;
  plan.properties = {"name", "weight"};
  plan.set_properties = {"preview_val"};
  return plan;
}

SourceCharBonesObjectPropSyncPlan source_char_bones_object_prop_sync_plan() {
  SourceCharBonesObjectPropSyncPlan plan;
  plan.custom_branches = {"bones"};
  return plan;
}

CharClip::OutputBone source_char_bone_copy_members(
    const CharClip::OutputBone& source) {
  CharClip::OutputBone dest;
  dest.rotation_context = source.rotation_context;
  dest.scale_context = source.scale_context;
  dest.position_context = source.position_context;
  dest.rotation_type = source.rotation_type;
  dest.target = source.target;
  dest.weights = source.weights;
  dest.trans = source.trans;
  dest.bake_out_as_top_level = source.bake_out_as_top_level;
  return dest;
}

std::optional<size_t> source_char_bone_find_weight_index(
    const CharClip::OutputBone& bone, int context_mask) {
  for (size_t i = 0; i < bone.weights.size(); ++i) {
    if ((bone.weights[i].context & context_mask) != 0) return i;
  }
  return std::nullopt;
}

float source_char_bone_get_weight(const CharClip::OutputBone& bone,
                                  int context_mask) {
  const std::optional<size_t> index =
      source_char_bone_find_weight_index(bone, context_mask);
  if (index) return bone.weights[*index].weight;
  return 1.0f;
}

void source_char_bone_clear_context(CharClip::OutputBone& bone,
                                    int context_mask) {
  const int mask = ~context_mask;
  bone.position_context &= mask;
  bone.scale_context &= mask;
  bone.rotation_context &= mask;
}

void source_char_bone_stuff_bones(const CharClip::OutputBone& bone,
                                  int context_mask,
                                  std::vector<SourceCharBonesBone>& bones) {
  if ((bone.position_context & context_mask) != 0) {
    bones.push_back({source_char_bones_channel_name(
                         bone.name, kSourceCharBonesTypePos),
                     source_char_bone_get_weight(bone, context_mask)});
  }
  if ((bone.scale_context & context_mask) != 0) {
    bones.push_back({source_char_bones_channel_name(
                         bone.name, kSourceCharBonesTypeScale),
                     source_char_bone_get_weight(bone, context_mask)});
  }
  if (bone.rotation_type != kSourceCharBonesTypeEnd &&
      (bone.rotation_context & context_mask) != 0) {
    bones.push_back({source_char_bones_channel_name(bone.name,
                                                   bone.rotation_type),
                     source_char_bone_get_weight(bone, context_mask)});
  }
}

SourceCharBoneDirDefaultState source_char_bone_dir_default_state() {
  return SourceCharBoneDirDefaultState{};
}

SourceCharBoneDirLoadPlan source_char_bone_dir_load_plan(int32_t revision) {
  SourceCharBoneDirLoadPlan plan;
  plan.known_revision = revision >= 0 && revision <= 4;
  if (!plan.known_revision) return plan;

  plan.preload_order = {"LOAD_REVS", "PushRev(packRevs(gAltRev,gRev))",
                        "ObjectDir::PreLoad"};
  plan.load_order = {"ObjectDir::Load"};
  plan.postload_order = {"ObjectDir::PostLoad", "PopRev",
                         "restore gRev/gAltRev"};
  if (revision < 2) {
    plan.postload_order.push_back("legacyMoveContextBool");
  } else {
    plan.postload_order.push_back("mMoveContext");
  }
  if (revision < 3) plan.postload_order.push_back("legacyPreRev3Bool");
  plan.postload_order.push_back("mRecenter");
  if (revision > 3) plan.postload_order.push_back("mBakeOutFacing");
  return plan;
}

SourceCharBoneDirSavePlan source_char_bone_dir_save_plan() {
  return SourceCharBoneDirSavePlan{};
}

SourceCharBoneDirCopyPlan source_char_bone_dir_copy_plan() {
  SourceCharBoneDirCopyPlan plan;
  plan.copied_superclasses = {"ObjectDir"};
  plan.copied_members = {"mMoveContext", "mRecenter", "mBakeOutFacing"};
  return plan;
}

SourceCharBoneDirHandlerPlan source_char_bone_dir_handler_plan() {
  SourceCharBoneDirHandlerPlan plan;
  plan.handlers = {"get_context_flags"};
  plan.superclasses = {"ObjectDir"};
  plan.check = 0x1D1;
  return plan;
}

SourceCharBoneDirRecenterPropSyncPlan
source_char_bone_dir_recenter_prop_sync_plan() {
  SourceCharBoneDirRecenterPropSyncPlan plan;
  plan.properties = {"targets", "average", "slide"};
  return plan;
}

SourceCharBoneDirRecenterLoadPlan source_char_bone_dir_recenter_load_plan() {
  SourceCharBoneDirRecenterLoadPlan plan;
  plan.read_order = {"mTargets", "mAverage", "mSlide"};
  return plan;
}

SourceCharBoneDirPropSyncPlan source_char_bone_dir_prop_sync_plan() {
  SourceCharBoneDirPropSyncPlan plan;
  plan.properties = {"recenter", "move_context", "bake_out_facing",
                     "filter_bones", "filter_names"};
  plan.set_properties = {"merge_character"};
  plan.modify_properties = {"filter_context"};
  plan.modify_actions = {"SyncFilter"};
  plan.superclasses = {"ObjectDir"};
  return plan;
}

SourceCharBoneDirInitPlan source_char_bone_dir_init_plan(
    const std::string& resource_path,
    bool has_clip_types,
    const std::vector<SourceCharBoneDirInitClipTypeRow>& clip_types) {
  SourceCharBoneDirInitPlan plan;
  if (!has_clip_types) {
    plan.skipped_missing_clip_types = true;
    return plan;
  }
  if (resource_path.empty()) {
    plan.skipped_empty_resource_path = true;
    return plan;
  }

  for (size_t source_index = 1; source_index < clip_types.size();
       ++source_index) {
    ++plan.scanned_rows;
    const SourceCharBoneDirInitClipTypeRow& row = clip_types[source_index];
    if (!row.has_resource) continue;
    if (row.already_loaded) {
      plan.skipped_existing_resources.push_back(row.resource_name);
      continue;
    }
    plan.load_requests.push_back(resource_path + "/" + row.resource_name +
                                 ".milo");
    if (row.load_succeeds) {
      plan.named_loaded_resources.push_back(row.resource_name);
    } else {
      plan.failed_load_resources.push_back(row.resource_name);
    }
  }
  return plan;
}

SourceCharBoneDirTerminatePlan source_char_bone_dir_terminate_plan() {
  return SourceCharBoneDirTerminatePlan{};
}

SourceCharBoneDirFindResourceResult source_char_bone_dir_find_resource(
    const std::vector<std::string>& loaded_resources,
    const std::string& resource_name) {
  SourceCharBoneDirFindResourceResult result;
  result.resource_name = resource_name;
  result.found = std::find(loaded_resources.begin(), loaded_resources.end(),
                           resource_name) != loaded_resources.end();
  return result;
}

void source_char_bone_dir_list_bones(
    const std::vector<CharClip::OutputBone>& output_bones,
    int move_context,
    int context_mask,
    bool include_delta_facing,
    std::vector<SourceCharBonesBone>& bones) {
  if ((move_context & context_mask) != 0) {
    bones.push_back({"bone_facing.pos", 1.0f});
    bones.push_back({"bone_facing.rotz", 1.0f});
    if (include_delta_facing) {
      bones.push_back({"bone_facing_delta.pos", 1.0f});
      bones.push_back({"bone_facing_delta.rotz", 1.0f});
    }
  }
  for (const CharClip::OutputBone& output_bone : output_bones) {
    source_char_bone_stuff_bones(output_bone, context_mask, bones);
  }
}

std::vector<std::string> source_char_bone_dir_get_clip_types(
    const std::vector<SourceCharBoneDirClipTypeResource>& clip_types) {
  std::vector<std::string> result;
  result.push_back("");
  for (const SourceCharBoneDirClipTypeResource& clip_type : clip_types) {
    result.push_back(clip_type.clip_type);
  }
  std::sort(result.begin(), result.end());
  return result;
}

SourceCharBoneDirResourceLookupResult
source_char_bone_dir_find_resource_from_clip_type(
    const std::vector<SourceCharBoneDirClipTypeResource>& clip_types,
    const std::string& clip_type) {
  SourceCharBoneDirResourceLookupResult result;
  const auto it = std::find_if(
      clip_types.begin(), clip_types.end(),
      [&](const SourceCharBoneDirClipTypeResource& row) {
        return row.clip_type == clip_type;
      });
  if (it == clip_types.end()) {
    result.warning = "no_type";
    return result;
  }
  result.clip_type_found = true;
  if (!it->has_resource) {
    result.warning = "no_resource_field";
    return result;
  }
  result.resource_field_found = true;
  result.resource_name = it->resource_name;
  result.context_mask = it->context_mask;
  if (!it->resource_found) {
    result.warning = "no_resource";
    return result;
  }
  result.resource_found = true;
  return result;
}

SourceCharBoneDirStuffBonesSymbolStep
source_char_bone_dir_stuff_bones_symbol_step(
    const std::vector<SourceCharBoneDirClipTypeResource>& clip_types,
    const std::string& clip_type) {
  SourceCharBoneDirStuffBonesSymbolStep step;
  step.lookup =
      source_char_bone_dir_find_resource_from_clip_type(clip_types, clip_type);
  if (step.lookup.resource_found) {
    step.call_stuff_bones = true;
    step.context_mask = step.lookup.context_mask;
  }
  return step;
}

SourceCharBoneDirContextFlagsStep
source_char_bone_dir_get_context_flags_step(
    const std::vector<SourceCharBoneDirClipTypeResource>& clip_types,
    const std::string& resource_name,
    const std::vector<std::string>& cached_context_flags,
    bool context_flags_is_int) {
  SourceCharBoneDirContextFlagsStep step;
  step.context_flags = cached_context_flags;
  if (!context_flags_is_int) return step;

  step.rebuilt = true;
  step.context_flags.clear();
  for (size_t source_index = 1; source_index + 1 < clip_types.size();
       ++source_index) {
    ++step.scanned_rows;
    const SourceCharBoneDirClipTypeResource& row = clip_types[source_index];
    if (!row.has_resource || row.resource_name != resource_name) continue;
    if (std::find(step.context_flags.begin(), step.context_flags.end(),
                  row.context_symbol) == step.context_flags.end()) {
      step.context_flags.push_back(row.context_symbol);
    }
  }
  std::sort(step.context_flags.begin(), step.context_flags.end());
  return step;
}

std::vector<std::string> source_char_bone_dir_sync_filter(
    const std::vector<CharClip::OutputBone>& output_bones,
    int filter_context) {
  std::vector<std::string> filter_bones;
  for (const CharClip::OutputBone& bone : output_bones) {
    if ((filter_context & bone.position_context) != 0 ||
        (filter_context & bone.scale_context) != 0 ||
        (bone.rotation_type != kSourceCharBonesTypeEnd &&
         (filter_context & bone.rotation_context) != 0)) {
      filter_bones.push_back(bone.name);
    }
  }
  return filter_bones;
}

SourceCharBoneDirMergeCharacterPlan source_char_bone_dir_merge_character_plan(
    bool load_succeeds,
    const std::vector<SourceCharBoneDirMergeTransform>& transforms) {
  SourceCharBoneDirMergeCharacterPlan plan;
  if (!load_succeeds) {
    plan.warned_failed_load = true;
    return plan;
  }

  plan.loaded = true;
  for (const SourceCharBoneDirMergeTransform& transform : transforms) {
    ++plan.scanned_transforms;
    if (transform.is_loaded_dir || !transform.animatable) continue;
    if (transform.name.rfind("bone_", 0) == 0 ||
        transform.name.rfind("exo_", 0) == 0) {
      plan.selected_transforms.push_back(transform.name);
    }
  }
  return plan;
}

SourceCharBonesMeshesReplaceStep source_char_bones_meshes_replace_step(
    const std::vector<std::string>& meshes,
    const std::string& from,
    const std::string& to,
    bool to_is_transformable,
    const std::string& dummy_mesh) {
  SourceCharBonesMeshesReplaceStep step;
  step.meshes = meshes;
  if (from == dummy_mesh) return step;
  step.scan_meshes = true;
  for (size_t i = 0; i < step.meshes.size(); ++i) {
    if (step.meshes[i] != from) continue;
    step.replaced_index = static_cast<int>(i);
    if (to_is_transformable) {
      step.meshes[i] = to;
    } else {
      step.meshes[i] = dummy_mesh;
      step.assigned_dummy = true;
    }
    return step;
  }
  return step;
}

SourceCharBonesMeshesReallocateStep source_char_bones_meshes_reallocate_step(
    const std::vector<SourceCharBonesBone>& bones,
    const std::unordered_map<std::string, std::string>& transform_lookup,
    const std::string& dummy_mesh) {
  SourceCharBonesMeshesReallocateStep step;
  step.meshes.resize(bones.size());
  for (size_t i = 0; i < bones.size(); ++i) {
    const auto it = transform_lookup.find(bones[i].name);
    if (it != transform_lookup.end()) {
      step.meshes[i] = it->second;
      continue;
    }
    if (bones[i].name.rfind("bone_facing", 0) != 0) {
      step.missing_non_facing_bones.push_back(bones[i].name);
    }
    step.meshes[i] = dummy_mesh;
  }
  step.acquire_pose = !step.meshes.empty();
  return step;
}

SourceCharBonesMeshesLifetimePlan source_char_bones_meshes_lifetime_plan() {
  return SourceCharBonesMeshesLifetimePlan{};
}

std::vector<std::string> source_char_bones_meshes_stuff_meshes(
    const std::vector<std::string>& existing_objects,
    const std::vector<std::string>& meshes) {
  std::vector<std::string> objects = existing_objects;
  objects.insert(objects.end(), meshes.begin(), meshes.end());
  return objects;
}

SourceCharBonesMeshesPoseDumpEvidence
source_char_bones_meshes_pose_dump_evidence() {
  SourceCharBonesMeshesPoseDumpEvidence evidence;
  evidence.pose_meshes_range = "0x80321520->0x80321A64";
  evidence.prop_sync_range = "0x80321B48->0x80321C20";
  evidence.pose_meshes_locals = {"bone", "pend", "p",    "qend", "q",
                                 "a",    "xend", "yend", "end",  "send",
                                 "s",    "blendScale"};
  evidence.latest_source_file =
      "rb3/src/system/char/CharBonesMeshes.cpp";
  evidence.latest_source_comment = "fn_804B0C60 - pose meshes";
  evidence.latest_source_stub_steps = {
      "declare float angle",
      "declare Hmx::Matrix3 m",
      "m.RotateAboutY(angle)",
      "m.RotateAboutX(angle)",
  };
  evidence.latest_source_body_incomplete = true;
  evidence.latest_source_mesh_loop_present = false;
  evidence.latest_source_uses_uninitialized_angle = true;
  evidence.latest_source_publishes_transform_rows = false;
  evidence.rb2_dump_has_statement_body = false;
  evidence.safe_to_pose_meshes = false;
  evidence.safe_to_publish_mesh_transforms = false;
  return evidence;
}

SourceCharBonesSamplesState source_char_bones_samples_empty_state() {
  return SourceCharBonesSamplesState{};
}

void source_char_bones_samples_set(SourceCharBonesSamplesState& samples,
                                   const SourceCharBonesState& bones,
                                   int num_samples,
                                   int compression) {
  SourceCharBonesSamplesState next;
  next.bones = bones;
  SourceCharBonesCompressionUpdate update =
      source_char_bones_set_compression(next.bones.compression,
                                        next.bones.layout, compression);
  next.bones.compression = update.compression;
  next.bones.layout = update.layout;
  next.num_samples = num_samples;
  next.raw_data_size = source_char_bones_samples_allocate_size(next);
  samples = next;
}

SourceCharBonesSamplesState source_char_bones_samples_clone(
    const SourceCharBonesSamplesState& source) {
  SourceCharBonesSamplesState clone;
  source_char_bones_samples_set(clone, source.bones, source.num_samples,
                                source.bones.compression);
  clone.frames = source.frames;
  return clone;
}

int source_char_bones_samples_allocate_size(
    const SourceCharBonesSamplesState& samples) {
  return samples.bones.layout.total_size * samples.num_samples;
}

bool source_char_bones_samples_set_preview(
    SourceCharBonesSamplesState& samples, int requested_sample) {
  if (samples.num_samples <= 0) return false;
  const int last = samples.num_samples - 1;
  const int clamped = std::max(0, std::min(last, requested_sample));
  samples.preview_sample = clamped;
  samples.start_offset = samples.bones.layout.total_size * clamped;
  return true;
}

std::vector<SourceCharBonesSampleStep> source_char_bones_samples_split_steps(
    const SourceCharBonesSamplesState& samples,
    int sample,
    float weight,
    float frac) {
  std::vector<SourceCharBonesSampleStep> steps;
  steps.push_back(
      {samples.bones.layout.total_size * sample, (1.0f - frac) * weight});
  if (frac > 0.0f) {
    steps.push_back(
        {samples.bones.layout.total_size * (sample + 1), frac * weight});
  }
  return steps;
}

int source_char_bones_samples_rotate_by_offset(
    const SourceCharBonesSamplesState& samples,
    int sample) {
  return samples.bones.layout.total_size * sample;
}

std::vector<SourceCharBonesSampleStep> source_char_bones_samples_rotate_to_steps(
    const SourceCharBonesSamplesState& samples,
    int sample,
    float angle,
    float frac) {
  return source_char_bones_samples_split_steps(samples, sample, angle, frac);
}

std::vector<SourceCharBonesSampleStep>
source_char_bones_samples_scale_add_steps(
    const SourceCharBonesSamplesState& samples,
    int sample,
    float weight,
    float frac) {
  return source_char_bones_samples_split_steps(samples, sample, weight, frac);
}

bool source_char_bones_samples_set_ver_known(int version) {
  return version < 13;
}

bool source_char_bones_samples_load_version_known(int version) {
  return version > 12 && version <= 16;
}

SourceCharBonesSamplesLoadPlan source_char_bones_samples_load_plan(
    int version) {
  SourceCharBonesSamplesLoadPlan plan;
  if (!source_char_bones_samples_load_version_known(version)) return plan;
  plan.known_version = true;
  plan.read_order = {"gVer", "LoadHeader", "LoadData"};
  return plan;
}

bool source_grim_char_bones_samples_standalone_version_known(int version) {
  return version == 16;
}

bool source_grim_char_clip_samples_version_known(int version) {
  return version == 10 || version == 11 || version == 16;
}

bool source_grim_char_clip_version_known(int version) {
  return version == 5 || version == 12;
}

int source_grim_char_bones_samples_get_type_of(const std::string& channel) {
  const size_t dot = channel.find('.');
  if (dot == std::string::npos) return kSourceCharBonesTypeEnd;
  const std::string ext = channel.substr(dot);
  if (ext == ".pos") return kSourceCharBonesTypePos;
  if (ext == ".scale") return kSourceCharBonesTypeScale;
  if (ext == ".quat") return kSourceCharBonesTypeQuat;
  if (ext == ".rotx") return kSourceCharBonesTypeRotX;
  if (ext == ".roty") return kSourceCharBonesTypeRotY;
  if (ext == ".rotz") return kSourceCharBonesTypeRotZ;
  return kSourceCharBonesTypeEnd;
}

float source_grim_char_bones_samples_decode_snorm16(int16_t value) {
  return std::max(static_cast<float>(value) / 32767.0f, -1.0f);
}

std::array<float, 4> source_grim_char_bones_samples_decode_short_quat(
    int16_t x,
    int16_t y,
    int16_t z,
    int16_t w) {
  return {source_grim_char_bones_samples_decode_snorm16(x),
          source_grim_char_bones_samples_decode_snorm16(y),
          source_grim_char_bones_samples_decode_snorm16(z),
          source_grim_char_bones_samples_decode_snorm16(w)};
}

float source_grim_char_bones_samples_pose_axis_angle(
    ClipChannel::Type axis,
    float sample) {
  if (axis == ClipChannel::kRotZ) {
    return 3.14159265358979323846f * sample;
  }
  return sample;
}

size_t source_grim_char_bones_samples_get_type_size(int type,
                                                    int compression) {
  if (type < 0 || type >= kSourceCharBonesTypeEnd) return 0u;
  if (compression < 0 || compression > 3) return 0u;
  if (type < kSourceCharBonesTypeQuat) {
    return compression < 2 ? 16u : 6u;
  }
  if (type != kSourceCharBonesTypeQuat) {
    return compression == 0 ? 4u : 2u;
  }
  if (compression > 2) return 4u;
  return compression == 0 ? 16u : 8u;
}

size_t source_grim_char_bones_samples_get_type_size2(int type,
                                                     int compression) {
  if (type < 0 || type >= kSourceCharBonesTypeEnd) return 0u;
  static const size_t kSizes[4][kSourceCharBonesTypeEnd] = {
      {12u, 4u, 16u, 4u, 4u, 4u},
      {12u, 4u, 8u, 2u, 2u, 2u},
      {6u, 4u, 8u, 2u, 2u, 2u},
      {6u, 4u, 4u, 2u, 2u, 2u},
  };
  if (compression < 0 || compression >= 4) return 0u;
  return kSizes[compression][type];
}

SourceGrimCharBonesSamplesComputedSizes
source_grim_char_bones_samples_recompute_sizes(
    int compression,
    const std::array<uint32_t, kSourceCharBonesTypeEnd + 1>& counts) {
  SourceGrimCharBonesSamplesComputedSizes plan;
  plan.compression = compression;
  plan.counts = counts;
  if (compression < 0 || compression >= 4) return plan;

  plan.valid = true;
  plan.computed_sizes[0] = 0;
  for (int type = 0; type < kSourceCharBonesTypeEnd; ++type) {
    const uint32_t curr_count = counts[type];
    const uint32_t next_count = counts[type + 1];
    if (next_count < curr_count) {
      plan.valid = false;
      return plan;
    }
    const uint32_t type_size = static_cast<uint32_t>(
        source_grim_char_bones_samples_get_type_size(type, compression));
    plan.computed_sizes[type + 1] =
        plan.computed_sizes[type] + (next_count - curr_count) * type_size;
  }
  plan.computed_flags =
      (plan.computed_sizes[kSourceCharBonesTypeEnd] + 0xFu) & 0xFFFFFFF0u;
  return plan;
}

SourceGrimCharBonesSamplesHeaderPlan
source_grim_char_bones_samples_header_plan(int version) {
  SourceGrimCharBonesSamplesHeaderPlan plan;
  if (!(version == 10 || version == 11 || version == 16)) return plan;
  plan.known_version = true;
  plan.count_size = version > 15 ? 7 : 10;
  plan.defaults_weight = version <= 10;
  plan.reads_weight = version > 10;
  plan.reads_frame_table = version > 11;
  plan.aligns_sample_data_to_4 = version > 11;
  plan.read_order = {
      "bone_count",
      "bone_symbols",
      plan.reads_weight ? "weights" : "default_weight_1.0",
      plan.count_size == 7 ? "counts[7]" : "counts[10]",
      "compression",
      "sample_count",
  };
  if (plan.reads_frame_table) plan.read_order.push_back("frames");
  return plan;
}

SourceGrimCharBonesSamplesDataPlan source_grim_char_bones_samples_data_plan(
    int version,
    int compression,
    const std::vector<std::string>& channels,
    int sample_count) {
  SourceGrimCharBonesSamplesDataPlan plan;
  plan.compression = compression;
  plan.sample_count = sample_count;

  const SourceGrimCharBonesSamplesHeaderPlan header_plan =
      source_grim_char_bones_samples_header_plan(version);
  if (!header_plan.known_version || sample_count < 0) return plan;

  plan.known_version = true;
  for (const std::string& channel : channels) {
    const int type = source_grim_char_bones_samples_get_type_of(channel);
    if (type < 0 || type >= kSourceCharBonesTypeEnd) {
      plan.ignored_channels.push_back(channel);
      continue;
    }
    const size_t type_size =
        source_grim_char_bones_samples_get_type_size2(type, compression);
    if (type_size == 0u) {
      plan.ignored_channels.push_back(channel);
      continue;
    }
    plan.kept_channels.push_back(channel);
    plan.channel_sizes.push_back(type_size);
    plan.unaligned_sample_size += type_size;
  }

  plan.sample_size = plan.unaligned_sample_size;
  plan.aligns_sample_data_to_4 = version > 11;
  if (plan.aligns_sample_data_to_4) {
    plan.sample_size = (plan.sample_size + 3u) & ~static_cast<size_t>(3u);
  }
  plan.total_sample_bytes =
      plan.sample_size * static_cast<size_t>(sample_count);
  return plan;
}

bool source_grim_char_bones_samples_decodes_channel_type(int type) {
  return type == kSourceCharBonesTypePos ||
         type == kSourceCharBonesTypeQuat ||
         type == kSourceCharBonesTypeRotZ;
}

float source_grim_char_bones_samples_channel_weight(
    const std::vector<float>& weights,
    size_t index) {
  return index < weights.size() ? weights[index] : 1.0f;
}

void source_grim_char_bones_samples_sort_decoded_channels(
    std::vector<ClipChannel>& channels) {
  std::stable_sort(channels.begin(), channels.end(),
                   [](const ClipChannel& a, const ClipChannel& b) {
                     return a.bone_name < b.bone_name;
                   });
}

bool source_grim_char_bones_samples_panics_channel_type(int type) {
  return !source_grim_char_bones_samples_decodes_channel_type(type);
}

SourceGrimCharBonesSamplesDecodePlan
source_grim_char_bones_samples_decode_plan() {
  SourceGrimCharBonesSamplesDecodePlan plan;
  plan.decoded_types = {kSourceCharBonesTypePos,
                        kSourceCharBonesTypeQuat,
                        kSourceCharBonesTypeRotZ};
  plan.unsupported_types = {kSourceCharBonesTypeScale,
                            kSourceCharBonesTypeRotX,
                            kSourceCharBonesTypeRotY,
                            kSourceCharBonesTypeEnd};
  plan.target_name_replacements = {
      ".pos=>.mesh",
      ".quat=>.mesh",
      ".rotz=>.mesh",
  };
  return plan;
}

SourceGrimCharBonesSamplesExportTranslationPlan
source_grim_char_bones_samples_export_translation_plan(
    const SourceGrimCharBonesSamplesExportTranslationInput& input) {
  SourceGrimCharBonesSamplesExportTranslationPlan plan;
  plan.uses_sample_index_times = true;
  plan.multiplies_sample_index_by_fps = true;
  plan.uses_frame_values = false;
  plan.adds_base_translation_to_pos_samples = false;
  plan.sample_time_step = 1.0f / 30.0f;
  plan.has_pos_samples = !input.pos_samples.empty();
  if (!plan.has_pos_samples) {
    plan.uses_default_translation_sample = true;
    plan.input_times.push_back(0.0f);
    plan.output_translations.push_back(input.base_translation);
    return plan;
  }

  for (size_t i = 0; i < input.pos_samples.size(); ++i) {
    const std::array<float, 3>& sample = input.pos_samples[i];
    plan.input_times.push_back(static_cast<float>(i) * plan.sample_time_step);
    plan.output_translations.push_back({
        sample[0] * input.weight,
        sample[1] * input.weight,
        sample[2] * input.weight,
    });
  }
  return plan;
}

SourceGrimCharBonesSamplesExportRotationPlan
source_grim_char_bones_samples_export_rotation_plan(
    const SourceGrimCharBonesSamplesExportRotationInput& input) {
  auto normalize = [](std::array<float, 4> q) {
    const float len_sq =
        q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
    if (len_sq <= 1.0e-8f) return std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
    const float inv_len = 1.0f / std::sqrt(len_sq);
    return std::array<float, 4>{q[0] * inv_len, q[1] * inv_len,
                                q[2] * inv_len, q[3] * inv_len};
  };
  auto multiply = [](const std::array<float, 4>& a,
                     const std::array<float, 4>& b) {
    const float ax = a[0], ay = a[1], az = a[2], aw = a[3];
    const float bx = b[0], by = b[1], bz = b[2], bw = b[3];
    return std::array<float, 4>{
        aw * bx + ax * bw + ay * bz - az * by,
        aw * by - ax * bz + ay * bw + az * bx,
        aw * bz + ax * by - ay * bx + az * bw,
        aw * bw - ax * bx - ay * by - az * bz,
    };
  };

  SourceGrimCharBonesSamplesExportRotationPlan plan;
  plan.uses_sample_index_times = true;
  plan.multiplies_sample_index_by_fps = true;
  plan.uses_frame_values = false;
  plan.sample_time_step = 1.0f / 30.0f;
  plan.sample_count =
      std::max({input.pos_sample_count, input.quat_samples_xyzw.size(),
                input.rotz_samples.size()});
  plan.has_rotation_samples = plan.sample_count > 0;
  if (!plan.has_rotation_samples) return plan;

  plan.output_rotations_xyzw.assign(
      plan.sample_count, normalize(input.base_rotation_xyzw));

  for (size_t i = 0; i < input.quat_samples_xyzw.size(); ++i) {
    const std::array<float, 4>& q = input.quat_samples_xyzw[i];
    plan.output_rotations_xyzw[i] =
        normalize({q[0] * input.quat_weight, q[1] * input.quat_weight,
                   q[2] * input.quat_weight, q[3] * input.quat_weight});
  }

  constexpr float kPi = 3.14159265358979323846f;
  for (size_t i = 0; i < input.rotz_samples.size(); ++i) {
    const float half_angle =
        0.5f * kPi * (input.rotz_samples[i] * input.rotz_weight);
    const std::array<float, 4> qz = {0.0f, 0.0f, std::sin(half_angle),
                                    std::cos(half_angle)};
    plan.output_rotations_xyzw[i] =
        normalize(multiply(plan.output_rotations_xyzw[i], qz));
  }

  for (size_t i = 0; i < plan.sample_count; ++i) {
    plan.input_times.push_back(static_cast<float>(i) * plan.sample_time_step);
  }
  return plan;
}

SourceGrimCharClipLoadPlan source_grim_char_clip_load_plan(int version,
                                                           bool read_meta) {
  SourceGrimCharClipLoadPlan plan;
  if (!source_grim_char_clip_version_known(version)) return plan;
  plan.known_version = true;
  plan.reads_object_meta = read_meta;
  plan.reads_range = version > 3;
  plan.skips_v5_unknown_bool = version == 5;
  plan.reads_relative = version > 5;
  plan.reads_unknown_1 = version > 9;
  plan.reads_do_not_decompress = version > 11;
  plan.reads_node_size = version >= 8;
  plan.reads_deprecated_events = version < 7;
  plan.reads_events = true;
  plan.read_order = {"version"};
  if (read_meta) plan.read_order.push_back("Object::Load");
  plan.read_order.insert(plan.read_order.end(),
                         {"start_beat", "end_beat", "beats_per_sec",
                          "flags", "play_flags", "blend_width"});
  if (plan.reads_range) plan.read_order.push_back("range");
  if (plan.skips_v5_unknown_bool) plan.read_order.push_back("v5_unknown_bool");
  if (plan.reads_relative) plan.read_order.push_back("relative");
  if (plan.reads_unknown_1) plan.read_order.push_back("unknown_1");
  if (plan.reads_do_not_decompress) {
    plan.read_order.push_back("do_not_decompress");
  }
  if (plan.reads_node_size) plan.read_order.push_back("node_size");
  plan.read_order.push_back("nodes");
  if (plan.reads_deprecated_events) plan.read_order.push_back("enter_exit");
  plan.read_order.push_back("events");
  return plan;
}

SourceGrimCharClipSamplesLoadPlan
source_grim_char_clip_samples_load_plan(int version) {
  SourceGrimCharClipSamplesLoadPlan plan;
  if (!source_grim_char_clip_samples_version_known(version)) return plan;
  plan.known_version = true;
  plan.calls_char_clip_with_meta = true;
  plan.reads_some_bool = version >= 16;
  plan.legacy_split_headers_and_data = version < 13;
  plan.reads_duplicate_legacy_header = version < 13 && version > 7;
  plan.reads_extra_bones = version > 14;
  plan.runtime_data_lists = version < 13 ? 2 : 2;
  plan.read_order = {"version", "CharClip(read_meta=true)"};
  if (plan.reads_some_bool) plan.read_order.push_back("some_bool");
  if (plan.legacy_split_headers_and_data) {
    plan.read_order.insert(plan.read_order.end(),
                           {"full.header", "one.header"});
    if (plan.reads_duplicate_legacy_header) {
      plan.read_order.push_back("duplicate.header");
    }
    plan.read_order.insert(plan.read_order.end(), {"full.data", "one.data"});
  } else {
    plan.read_order.insert(plan.read_order.end(),
                           {"full.CharBonesSamples", "one.CharBonesSamples"});
  }
  if (plan.reads_extra_bones) plan.read_order.push_back("extra_bones");
  return plan;
}

SourceGrimCharClipSamplesExtraBonesPlan
source_grim_char_clip_samples_extra_bones_plan(int version) {
  SourceGrimCharClipSamplesExtraBonesPlan plan;
  if (!source_grim_char_clip_samples_version_known(version) || version <= 14)
    return plan;
  plan.active = true;
  plan.reads_count = true;
  plan.reads_name = true;
  plan.reads_weight = true;
  plan.stores_runtime_rows = false;
  plan.read_order = {"bone_count", "name", "weight"};
  return plan;
}

SourceReNotesCharBonesSamplesDecodePlan
source_re_notes_char_bones_samples_decode_plan() {
  SourceReNotesCharBonesSamplesDecodePlan plan;
  plan.active_sample_order = {".pos", ".quat", ".rotz"};
  plan.fenced_channels = {".scale", ".rotx", ".roty"};
  return plan;
}

SourceProblemCharacterClipRawAxisAudit
source_problem_character_clip_raw_axis_audit_20260714() {
  SourceProblemCharacterClipRawAxisAudit audit;
  audit.artifact =
      "engine/out/source_clip_audit_20260714/"
      "problem_character_clip_audit.log";
  audit.rows = {
      {"char/rock1/anims/gen/rock1_fret.milo_ps2", 25, 25, 182, 0, 0, 0, 0},
      {"char/rock1/anims/gen/rock1_main.milo_ps2", 113, 113, 16247, 0, 0, 0,
       0},
      {"char/rock1/anims/gen/rock1_strum.milo_ps2", 17, 17, 247, 0, 0, 0, 0},
      {"char/rockabill1/anims/gen/rockabill1_fret.milo_ps2", 25, 25, 186, 0,
       0, 0, 0},
      {"char/rockabill1/anims/gen/rockabill1_main.milo_ps2", 116, 116, 17349,
       0, 0, 0, 0},
      {"char/rockabill1/anims/gen/rockabill1_strum.milo_ps2", 17, 17, 247, 0,
       0, 0, 0},
      {"char/rockabill2/anims/gen/rockabill2_fret.milo_ps2", 25, 25, 186, 0,
       0, 0, 0},
  };
  audit.shared_animation_notes = {
      "rock2 has no private CharClipSamples rows under char/rock2 in the "
      "stock GH2 ARK",
      "rockabill2 has a local fret row set; main and strum are not private "
      "rockabill2 animation row sets in this audit",
  };
  return audit;
}

SourceCharBonesSamplesPropSyncPlan
source_char_bones_samples_prop_sync_plan() {
  SourceCharBonesSamplesPropSyncPlan plan;
  plan.properties = {"num_samples", "frames"};
  plan.set_properties = {"preview_sample", "compression"};
  plan.custom_branches = {"bones"};
  return plan;
}

SourceCharBonesSamplesBodyBoundary
source_char_bones_samples_body_boundary() {
  SourceCharBonesSamplesBodyBoundary boundary;
  boundary.fenced_bodies = {
      "CharBonesSamples::LoadHeader",
      "CharBonesSamples::LoadData",
      "CharBonesSamples::EvaluateChannel",
      "CharBonesSamples::Relativize",
  };
  return boundary;
}

SourceCharBonesSamplesRuntimeDumpEvidence
source_char_bones_samples_runtime_dump_evidence() {
  SourceCharBonesSamplesRuntimeDumpEvidence evidence;
  evidence.frac_to_sample_range = "0x80323420 -> 0x80323654";
  evidence.evaluate_channel_range = "0x80323654 -> 0x80323E64";
  evidence.rotate_by_range = "0x80323E64 -> 0x80323E7C";
  evidence.rotate_to_range = "0x80323E7C -> 0x80323F3C";
  evidence.scale_add_sample_range = "0x80323F3C -> 0x80323FFC";
  evidence.relativize_range = "0x80323FFC -> 0x803250DC";
  evidence.load_range = "0x80325AD8 -> 0x80325B8C";
  evidence.read_counts_range = "0x80325B8C -> 0x80325C9C";
  evidence.load_header_range = "0x80325C9C -> 0x80326054";
  evidence.load_data_range = "0x80326054 -> 0x80326370";
  evidence.sync_property_range = "0x803263A8 -> 0x803266E8";
  evidence.frac_to_sample_locals = {"int lastSample", "int sample",
                                    "float w"};
  evidence.evaluate_channel_locals = {
      "const char* src", "const char* src2", "float& d",
      "Quat a",         "Quat b",           "Quat a",
      "Quat b",         "Vector3 a",        "Vector3 b"};
  evidence.relativize_locals = {
      "int i",
      "const Bone* bone",
      "ShortVector3* sp",
      "Vector3 first",
      "Vector3 v",
      "Vector3* p",
      "Vector3 first",
      "ByteQuat* bq",
      "Quat first",
      "Matrix3 firstMat",
      "Quat q",
      "Matrix3 m",
      "signed short* sa",
      "float first",
      "float a",
      "ShortQuat* sq",
      "Quat first",
      "Matrix3 firstMat",
      "Quat q",
      "Matrix3 m",
      "signed short* sa",
      "float first",
      "float a",
      "Quat* q",
      "Quat first",
      "Matrix3 firstMat",
      "Matrix3 m",
      "float* a",
      "float first"};
  evidence.load_header_locals = {"int size", "int i", "int count",
                                 "int tmp"};
  evidence.load_data_locals = {"int i",
                               "const ShortVector3* send",
                               "ShortVector3* sp",
                               "const Vector3* send",
                               "Vector3* p",
                               "const ByteQuat* qend",
                               "ByteQuat* bq",
                               "const ShortQuat* qend",
                               "ShortQuat* q",
                               "const Quat* qend",
                               "Quat* q",
                               "const signed short* rend",
                               "signed short* a",
                               "const float* rend",
                               "float* a"};
  evidence.has_load_header_statement_body = false;
  evidence.has_load_data_statement_body = false;
  evidence.has_evaluate_channel_statement_body = false;
  evidence.has_relativize_statement_body = false;
  evidence.safe_to_decode_logged_rows = true;
  evidence.safe_to_publish_pose = false;
  return evidence;
}

SourceCharClipSamplesRuntimeDumpEvidence
source_char_clip_samples_runtime_dump_evidence() {
  SourceCharClipSamplesRuntimeDumpEvidence evidence;
  evidence.facing_bones_set_range = "0x803331CC -> 0x80333344";
  evidence.facing_set_scale_add_range = "0x80333414 -> 0x80333600";
  evidence.frame_to_sample_range = "0x8033373C -> 0x8033376C";
  evidence.get_channel_range = "0x8033399C -> 0x80333A24";
  evidence.evaluate_channel_range = "0x80333A24 -> 0x80333AB8";
  evidence.evaluate_channel_sample_range = "0x80333AB8 -> 0x80333B18";
  evidence.rotate_by_range = "0x80333BCC -> 0x80333C70";
  evidence.rotate_to_range = "0x80333C70 -> 0x80333CF4";
  evidence.scale_add_frame_range = "0x80333CF4 -> 0x80333DAC";
  evidence.scale_add_sample_range = "0x80333DAC -> 0x80333E78";
  evidence.relativize_range = "0x80333ED4 -> 0x80333F94";
  evidence.set_relative_range = "0x80333F94 -> 0x80334058";
  evidence.load_range = "0x80334274 -> 0x80334470";
  evidence.facing_set_scale_add_locals = {"Vector3 curPos", "float curAng",
                                          "float lastAng"};
  evidence.evaluate_channel_locals = {"int offset"};
  evidence.evaluate_channel_sample_locals = {"float frac", "int sample"};
  evidence.rotate_by_locals = {"float frac", "int sample"};
  evidence.rotate_to_locals = {"float frac", "int sample"};
  evidence.scale_add_frame_locals = {"float frac", "float lastFrac",
                                     "int sample", "int lastSample"};
  evidence.load_locals = {"CharBonesSamples delta"};
  evidence.has_evaluate_channel_statement_body = false;
  evidence.has_rotate_by_statement_body = false;
  evidence.has_scale_add_statement_body = false;
  evidence.has_load_statement_body = false;
  evidence.safe_to_publish_pose = false;
  return evidence;
}

std::string source_char_utl_name_with_suffix(const std::string& name,
                                             const std::string& suffix) {
  const size_t dot = name.rfind('.');
  const size_t end = dot == std::string::npos ? name.size() : dot;
  std::string out = name.substr(0, end);
  out.push_back('.');
  out += suffix;
  return out;
}

namespace {

bool source_char_utl_is_transformable_kind(SourceCharUtlObjectKind kind) {
  return kind != SourceCharUtlObjectKind::kCharBone;
}

std::optional<SourceCharUtlObject> source_char_utl_find_named_transformable(
    const std::string& name,
    const std::vector<SourceCharUtlObject>& objects) {
  for (const SourceCharUtlObject& object : objects) {
    if (object.name == name &&
        source_char_utl_is_transformable_kind(object.kind)) {
      return object;
    }
  }
  return std::nullopt;
}

std::optional<size_t> source_char_utl_find_merge_bone_index(
    const std::string& name,
    const std::vector<SourceCharUtlMergeBone>& bones) {
  const std::string lookup = source_char_utl_name_with_suffix(name, "cb");
  for (size_t i = 0; i < bones.size(); ++i) {
    if (bones[i].name == lookup) return i;
  }
  return std::nullopt;
}

std::optional<size_t> source_char_utl_grab_merge_bone(
    const SourceCharUtlMergeBone& source_bone,
    const std::vector<SourceCharUtlMergeBone>& dest_bones,
    std::vector<SourceCharUtlMergeWarning>& warnings) {
  const std::optional<size_t> index =
      source_char_utl_find_merge_bone_index(source_bone.name, dest_bones);
  if (!index) {
    warnings.push_back(
        {"missing_bone", source_bone.name, std::string(), std::string()});
  }
  return index;
}

}  // namespace

std::optional<SourceCharUtlObject> source_char_utl_find_bone(
    const std::string& name,
    const std::vector<SourceCharUtlObject>& objects) {
  const std::string lookup = source_char_utl_name_with_suffix(name, "cb");
  for (const SourceCharUtlObject& object : objects) {
    if (object.name == lookup &&
        object.kind == SourceCharUtlObjectKind::kCharBone) {
      return object;
    }
  }
  return std::nullopt;
}

std::optional<SourceCharUtlBoneTransResult> source_char_utl_find_bone_trans(
    const std::string& name,
    const std::vector<SourceCharUtlObject>& objects) {
  const std::string cb_lookup = source_char_utl_name_with_suffix(name, "cb");
  for (const SourceCharUtlObject& object : objects) {
    if (object.name == cb_lookup &&
        object.kind == SourceCharUtlObjectKind::kCharBone) {
      if (object.char_bone_transform.empty()) return std::nullopt;
      return SourceCharUtlBoneTransResult{cb_lookup, object.char_bone_transform,
                                          true};
    }
  }

  const std::string trans_lookup =
      source_char_utl_name_with_suffix(name, "trans");
  if (source_char_utl_find_named_transformable(trans_lookup, objects)) {
    return SourceCharUtlBoneTransResult{trans_lookup, trans_lookup, false};
  }

  const std::string mesh_lookup =
      source_char_utl_name_with_suffix(name, "mesh");
  if (source_char_utl_find_named_transformable(mesh_lookup, objects)) {
    return SourceCharUtlBoneTransResult{mesh_lookup, mesh_lookup, false};
  }

  return std::nullopt;
}

bool source_char_utl_is_animatable(const SourceCharUtlObject& object) {
  if (object.kind == SourceCharUtlObjectKind::kMesh &&
      object.mesh_bone_count != 0) {
    return false;
  }
  if (object.kind == SourceCharUtlObjectKind::kCamera ||
      object.kind == SourceCharUtlObjectKind::kCharCollide ||
      object.kind == SourceCharUtlObjectKind::kCharCuff ||
      object.kind == SourceCharUtlObjectKind::kDirectory ||
      object.kind == SourceCharUtlObjectKind::kCharBone) {
    return false;
  }
  return object.name.rfind("spot_", 0) != 0;
}

SourceCharUtlMergeResult source_char_utl_merge_bones(
    const std::vector<SourceCharUtlMergeBone>& source_bones,
    const std::vector<SourceCharUtlMergeBone>& dest_bones,
    int context_mask) {
  SourceCharUtlMergeResult result;
  result.dest_bones = dest_bones;

  for (const SourceCharUtlMergeBone& source_bone : source_bones) {
    if (!source_bone.target.empty()) {
      const std::optional<size_t> dest_index =
          source_char_utl_grab_merge_bone(source_bone, result.dest_bones,
                                          result.warnings);
      if (dest_index) {
        SourceCharUtlMergeBone& dest = result.dest_bones[*dest_index];
        if (dest.target.empty()) {
          const std::optional<size_t> target_index =
              source_char_utl_find_merge_bone_index(source_bone.target,
                                                    result.dest_bones);
          if (!target_index) {
            result.warnings.push_back({"missing_target", source_bone.name,
                                       source_bone.target, std::string()});
          }
          dest.target =
              target_index ? result.dest_bones[*target_index].name : "";
        } else if (dest.target != source_bone.target) {
          result.warnings.push_back({"different_target", source_bone.name,
                                     source_bone.target, dest.target});
        }
      }
    }

    if (source_bone.position_context != 0) {
      const std::optional<size_t> dest_index =
          source_char_utl_grab_merge_bone(source_bone, result.dest_bones,
                                          result.warnings);
      if (dest_index) {
        result.dest_bones[*dest_index].position_context |= context_mask;
      }
    }

    if (source_bone.scale_context != 0) {
      const std::optional<size_t> dest_index =
          source_char_utl_grab_merge_bone(source_bone, result.dest_bones,
                                          result.warnings);
      if (dest_index) {
        SourceCharUtlMergeBone& dest = result.dest_bones[*dest_index];
        dest.position_context = dest.scale_context | context_mask;
      }
    }

    if (source_bone.rotation_context != 0 &&
        source_bone.rotation_type != kSourceCharBonesTypeEnd) {
      const std::optional<size_t> dest_index =
          source_char_utl_grab_merge_bone(source_bone, result.dest_bones,
                                          result.warnings);
      if (dest_index) {
        SourceCharUtlMergeBone& dest = result.dest_bones[*dest_index];
        if (dest.rotation_type != kSourceCharBonesTypeEnd &&
            dest.rotation_type != source_bone.rotation_type) {
          result.warnings.push_back(
              {"different_rotation", source_bone.name,
               std::to_string(source_bone.rotation_type),
               std::to_string(dest.rotation_type)});
        } else {
          dest.rotation_type = source_bone.rotation_type;
          dest.rotation_context |= context_mask;
        }
      }
    }
  }

  return result;
}

std::vector<std::string> source_char_utl_bone_saver_capture_names(
    const std::vector<SourceCharUtlTransformRow>& transforms) {
  std::vector<std::string> names;
  for (const SourceCharUtlTransformRow& transform : transforms) {
    if (transform.name.rfind("bone_", 0) == 0) names.push_back(transform.name);
  }
  return names;
}

std::vector<std::string> source_char_utl_reset_transform_names(
    const std::vector<SourceCharUtlTransformRow>& transforms) {
  std::vector<std::string> names;
  for (const SourceCharUtlTransformRow& transform : transforms) {
    if (!transform.has_parent) names.push_back(transform.name);
  }
  return names;
}

std::vector<std::string> source_char_utl_reset_hair_names(
    const std::vector<std::string>& hair_names) {
  return hair_names;
}

SourceCharUtlInitPlan source_char_utl_init_plan() {
  SourceCharUtlInitPlan plan;
  plan.registered_functions = {"reset_hair", "char_merge_bones"};
  plan.reset_hair_handler_steps = {"CharUtlResetHair(Obj<Character>(1))"};
  plan.char_merge_bones_handler_steps = {
      "FilePath(Str(1))", "DirLoader::LoadObjects",
      "Obj<ObjectDir>(2)", "Int(3)", "CharUtlMergeBones",
      "delete loaded dir"};
  return plan;
}

void source_char_utl_clip_predict(SourceCharUtlClipPredictState& state,
                                  const SourceCharUtlClipPredictFrame& first,
                                  const SourceCharUtlClipPredictFrame& second) {
  float delta[3] = {second.facing_pos[0] - first.facing_pos[0],
                    second.facing_pos[1] - first.facing_pos[1],
                    second.facing_pos[2] - first.facing_pos[2]};
  source_rotate_about_z_vec(delta,
                            source_limit_ang(state.ang - first.facing_rot));
  state.pos[0] += delta[0];
  state.pos[1] += delta[1];
  state.pos[2] += delta[2];
  state.last_pos = second.facing_pos;
  state.last_ang = second.facing_rot;
  state.ang = source_limit_ang(
      state.ang + source_limit_ang(second.facing_rot - first.facing_rot));
}

SourceCharLookAtBounds source_char_lookat_sync_limits(
    float min_yaw, float max_yaw, float min_pitch, float max_pitch) {
  constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
  min_yaw = std::clamp(min_yaw, -80.0f, 80.0f);
  max_yaw = std::clamp(max_yaw, -80.0f, 80.0f);
  min_pitch = std::clamp(min_pitch, -80.0f, 80.0f);
  max_pitch = std::clamp(max_pitch, -80.0f, 80.0f);

  const float max_yaw_abs = std::max(std::fabs(min_yaw), std::fabs(max_yaw));
  const float max_pitch_abs =
      std::max(std::fabs(min_pitch), std::fabs(max_pitch));
  const float max_overall = std::max(max_yaw_abs, max_pitch_abs);
  const float min_y = std::cos(max_overall * kDegToRad);

  SourceCharLookAtBounds bounds;
  bounds.min[1] = min_y;
  bounds.max[1] = 1.0e29f;
  bounds.min[2] = min_y * std::tan(min_yaw * kDegToRad);
  bounds.max[2] = min_y * std::tan(max_yaw * kDegToRad);
  bounds.min[0] = min_y * std::tan(min_pitch * kDegToRad);
  bounds.max[0] = min_y * std::tan(max_pitch * kDegToRad);
  return bounds;
}

static void source_char_lookat_sync_limit_state(
    SourceCharLookAtLimitState& state) {
  state.min_yaw = std::clamp(state.min_yaw, -80.0f, 80.0f);
  state.max_yaw = std::clamp(state.max_yaw, -80.0f, 80.0f);
  state.min_pitch = std::clamp(state.min_pitch, -80.0f, 80.0f);
  state.max_pitch = std::clamp(state.max_pitch, -80.0f, 80.0f);
  state.bounds = source_char_lookat_sync_limits(
      state.min_yaw, state.max_yaw, state.min_pitch, state.max_pitch);
}

SourceCharLookAtLimitState source_char_lookat_default_limit_state() {
  SourceCharLookAtLimitState state;
  source_char_lookat_sync_limit_state(state);
  return state;
}

void source_char_lookat_set_min_yaw(SourceCharLookAtLimitState& state,
                                    float yaw) {
  state.min_yaw = yaw;
  source_char_lookat_sync_limit_state(state);
}

void source_char_lookat_set_max_yaw(SourceCharLookAtLimitState& state,
                                    float yaw) {
  state.max_yaw = yaw;
  source_char_lookat_sync_limit_state(state);
}

void source_char_lookat_set_min_pitch(SourceCharLookAtLimitState& state,
                                      float pitch) {
  state.min_pitch = pitch;
  source_char_lookat_sync_limit_state(state);
}

void source_char_lookat_set_max_pitch(SourceCharLookAtLimitState& state,
                                      float pitch) {
  state.max_pitch = pitch;
  source_char_lookat_sync_limit_state(state);
}

SourceCharLookAtLoadPlan source_char_lookat_load_plan(int32_t revision) {
  SourceCharLookAtLoadPlan plan;
  plan.revision_supported = revision >= 0 && revision <= 5;
  if (!plan.revision_supported) return plan;

  plan.read_order = {"Hmx::Object", "CharWeightable", "mSource",
                     "mPivot",      "mDest",          "mHalfTime",
                     "mMinYaw",     "mMaxYaw",        "mMinPitch",
                     "mMaxPitch"};
  if (revision > 1) {
    plan.read_order.push_back("mMinWeightYaw");
    plan.read_order.push_back("mMaxWeightYaw");
    plan.read_order.push_back("mWeightYawSpeed");
  }
  if (revision < 3) {
    plan.branches.push_back("mAllowRoll=true");
  } else {
    plan.read_order.push_back("mAllowRoll");
  }
  if (revision < 4) {
    plan.branches.push_back("mEnableJitter=false");
    plan.branches.push_back("mPitchJitterLimit=0");
    plan.branches.push_back("mYawJitterLimit=0");
  } else {
    plan.read_order.push_back("mEnableJitter");
    plan.read_order.push_back("mPitchJitterLimit");
    plan.read_order.push_back("mYawJitterLimit");
  }
  if (revision > 4) plan.read_order.push_back("mSourceRadius");
  plan.sync_limits = true;
  return plan;
}

SourceCharLookAtCopyPlan source_char_lookat_copy_plan() {
  SourceCharLookAtCopyPlan plan;
  plan.copied_superclasses = {"Hmx::Object", "CharWeightable"};
  plan.copied_members = {"mSource",         "mPivot",
                         "mDest",           "mHalfTime",
                         "mMinYaw",         "mMaxYaw",
                         "mMinPitch",       "mMaxPitch",
                         "mMinWeightYaw",   "mMaxWeightYaw",
                         "mWeightYawSpeed", "mAllowRoll",
                         "mSourceRadius",   "mEnableJitter",
                         "mYawJitterLimit", "mPitchJitterLimit"};
  plan.sync_limits = true;
  return plan;
}

SourceCharLookAtHandlerPlan source_char_lookat_handler_plan() {
  SourceCharLookAtHandlerPlan plan;
  plan.superclasses = {"CharPollable", "Hmx::Object"};
  plan.check = 0x1DF;
  return plan;
}

SourceCharLookAtPropSyncPlan source_char_lookat_prop_sync_plan() {
  SourceCharLookAtPropSyncPlan plan;
  plan.properties = {"source",           "pivot",          "target",
                     "half_time",        "min_weight_yaw", "max_weight_yaw",
                     "weight_yaw_speed", "allow_roll",     "show_range",
                     "source_radius",    "enable_jitter",  "yaw_jitter_limit",
                     "pitch_jitter_limit", "test_range",
                     "test_range_pitch",  "test_range_yaw"};
  plan.set_properties = {"min_yaw", "max_yaw", "min_pitch", "max_pitch"};
  plan.set_actions = {"SetMinYaw", "SetMaxYaw", "SetMinPitch", "SetMaxPitch"};
  plan.superclasses = {"CharWeightable"};
  return plan;
}

SourceCharLookAtSavePlan source_char_lookat_save_plan() {
  return SourceCharLookAtSavePlan{};
}

SourceCharLookAtEnterState source_char_lookat_enter(bool has_pivot) {
  SourceCharLookAtEnterState state;
  state.smoothed_dir = {1.0e29f, 0.0f, 0.0f};
  state.reset_pivot_local = has_pivot;
  return state;
}

void source_char_lookat_poll_deps(SourceCharLookAtPollDeps& deps,
                                  const std::string& source,
                                  const std::string& pivot,
                                  const std::string& dest) {
  deps.changed_by.push_back(source.empty() ? pivot : source);
  deps.changed_by.push_back(dest);
  deps.change.push_back(pivot);
}

SourceCharLookAtPollPlan source_char_lookat_poll_plan(
    bool has_resolved_source,
    bool has_pivot,
    bool has_dest,
    bool has_pivot_parent,
    float delta_seconds,
    float weight,
    float min_weight_yaw,
    float source_radius,
    bool source_is_pivot,
    bool has_smoothed_dir,
    float half_time,
    bool test_range,
    bool show_range,
    bool enable_jitter,
    bool static_disable_jitter,
    bool cheat_disable_eye_jitter,
    bool allow_roll) {
  SourceCharLookAtPollPlan plan;
  plan.poll_gate_open =
      has_dest && has_pivot && has_pivot_parent && has_resolved_source &&
      delta_seconds >= 0.0f;
  if (!plan.poll_gate_open) return plan;

  plan.compute_dest_vector = true;
  plan.apply_weight_yaw = min_weight_yaw >= 0.0f;
  if (weight == 0.0f) {
    plan.skip_zero_weight = true;
    return plan;
  }

  const bool has_source_radius = source_radius > 0.0f;
  plan.update_source_radius_history =
      has_source_radius && delta_seconds > 0.0f;
  plan.clamp_source_radius_offset = has_source_radius;
  plan.write_pivot_world_to_source = !source_is_pivot;
  plan.normalize_dest_vector = source_is_pivot;
  plan.transform_to_parent_space = true;
  plan.clamp_bounds = true;
  plan.smooth_half_time = has_smoothed_dir && half_time != 0.0f;
  plan.use_test_range = test_range;
  plan.use_show_range = !test_range && show_range;
  plan.apply_jitter = enable_jitter && !static_disable_jitter &&
                      !cheat_disable_eye_jitter && delta_seconds > 0.0f;
  plan.subtract_source_radius_offset = has_source_radius;
  plan.write_roll_local_rotation = allow_roll;
  plan.write_no_roll_axes = !allow_roll;
  return plan;
}

SourceCharLookAtYawWeightResult source_char_lookat_yaw_weight_step(
    float row_weight,
    float previous_yaw_weight,
    float min_weight_yaw,
    float max_weight_yaw,
    float weight_yaw_speed,
    float delta_seconds,
    std::array<float, 3> source_world_y,
    std::array<float, 3> dest_delta) {
  SourceCharLookAtYawWeightResult result;
  result.updated_yaw_weight = previous_yaw_weight;
  result.final_weight = row_weight;
  if (min_weight_yaw < 0.0f) return result;

  result.applied = true;

  const float src_len =
      std::sqrt(source_world_y[0] * source_world_y[0] +
                source_world_y[1] * source_world_y[1] +
                source_world_y[2] * source_world_y[2]);
  source_world_y[0] /= src_len;
  source_world_y[1] /= src_len;
  source_world_y[2] /= src_len;

  dest_delta[2] = 0.0f;
  source_world_y[2] = 0.0f;
  const float times = source_world_y[0] * dest_delta[0] +
                      source_world_y[1] * dest_delta[1] +
                      source_world_y[2] * dest_delta[2];
  const float source_flat_len =
      std::sqrt(source_world_y[0] * source_world_y[0] +
                source_world_y[1] * source_world_y[1] +
                source_world_y[2] * source_world_y[2]);
  const float dest_flat_len =
      std::sqrt(dest_delta[0] * dest_delta[0] +
                dest_delta[1] * dest_delta[1] +
                dest_delta[2] * dest_delta[2]);
  result.dot_clamped =
      std::clamp(times / (source_flat_len * dest_flat_len), -1.0f, 1.0f);

  float clamped2 = std::clamp(
      max_weight_yaw -
          (std::acos(result.dot_clamped) / (max_weight_yaw - min_weight_yaw)),
      0.0f,
      1.0f);
  result.target_yaw_weight = clamped2;

  float loc13c = (clamped2 - previous_yaw_weight) / delta_seconds;
  if (loc13c > weight_yaw_speed) {
    loc13c = weight_yaw_speed;
    clamped2 = loc13c * delta_seconds + previous_yaw_weight;
    result.speed_limited = true;
  }

  result.updated_yaw_weight = clamped2;
  result.final_weight = row_weight * clamped2;
  return result;
}

SourceCharLookAtNoRollAxesResult source_char_lookat_no_roll_axes(
    std::array<float, 3> current_local_y,
    std::array<float, 3> desired_parent_space_dir,
    float weight) {
  SourceCharLookAtNoRollAxesResult result;
  auto normalize = [](std::array<float, 3> v) {
    const float len_sq = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (len_sq > 0.0f) {
      const float inv_len = 1.0f / std::sqrt(len_sq);
      for (float& value : v) value *= inv_len;
    }
    return v;
  };
  auto cross = [](const std::array<float, 3>& a,
                  const std::array<float, 3>& b) {
    return std::array<float, 3>{
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    };
  };

  for (int axis = 0; axis < 3; ++axis) {
    result.y[axis] = current_local_y[axis] +
                     (desired_parent_space_dir[axis] -
                      current_local_y[axis]) *
                         weight;
  }
  result.z = {-1.0f, 0.0f, 0.0f};
  result.y = normalize(result.y);
  result.x = normalize(cross(result.y, result.z));
  result.z = cross(result.x, result.y);
  result.invalid_xx = result.x[0] < -2.0f || result.x[0] > 2.0f;
  if (result.invalid_xx) {
    result.x = {1.0f, 0.0f, 0.0f};
    result.y = {0.0f, 1.0f, 0.0f};
    result.z = {0.0f, 0.0f, 1.0f};
  }
  return result;
}

SourceCharLookAtSmoothResult source_char_lookat_smooth_dir(
    bool has_previous,
    std::array<float, 3> previous_dir,
    std::array<float, 3> current_dir,
    float delta_seconds,
    float half_time) {
  SourceCharLookAtSmoothResult result;
  result.dir = current_dir;
  if (!has_previous || half_time == 0.0f) return result;
  result.applied = true;
  result.factor = delta_seconds / (delta_seconds + half_time);
  for (int axis = 0; axis < 3; ++axis) {
    result.dir[axis] =
        previous_dir[axis] +
        (current_dir[axis] - previous_dir[axis]) * result.factor;
  }
  return result;
}

SourceCharLookAtRangeResult source_char_lookat_range_dir(
    const SourceCharLookAtBounds& bounds,
    bool test_range,
    float test_range_pitch,
    float test_range_yaw,
    bool show_range,
    int seconds) {
  SourceCharLookAtRangeResult result;
  if (test_range) {
    result.applied = true;
    result.used_test_range = true;
    result.dir = {
        bounds.min[0] + (bounds.max[0] - bounds.min[0]) * test_range_pitch,
        bounds.min[1],
        bounds.min[2] + (bounds.max[2] - bounds.min[2]) * test_range_yaw};
    return result;
  }
  if (!show_range) return result;

  result.applied = true;
  result.used_show_range = true;
  result.force_weight_one = true;
  result.show_range_case = seconds & 7;
  switch (result.show_range_case) {
    case 0:
      result.dir = {bounds.min[0], bounds.min[1], bounds.min[2]};
      break;
    case 1:
      result.dir = {0.0f, bounds.min[2], bounds.max[0]};
      break;
    case 2:
      result.dir = {bounds.max[0], bounds.min[1], bounds.min[2]};
      break;
    case 3:
      result.dir = {bounds.max[0], bounds.min[1], 0.0f};
      break;
    case 4:
      result.dir = {bounds.max[0], bounds.min[1], bounds.max[2]};
      break;
    case 5:
      result.dir = {0.0f, bounds.min[1], bounds.max[2]};
      break;
    case 6:
      result.dir = {bounds.min[0], bounds.min[1], bounds.max[2]};
      break;
    case 7:
      result.dir = {bounds.min[0], bounds.min[1], 0.0f};
      break;
    default:
      break;
  }
  return result;
}

SourceCharLookAtSourceRadiusResult source_char_lookat_source_radius_offset(
    float source_radius_degrees,
    float delta_seconds,
    std::array<float, 3> previous_history,
    std::array<float, 3> source_world_y) {
  SourceCharLookAtSourceRadiusResult result;
  result.history = previous_history;
  if (source_radius_degrees <= 0.0f) return result;

  result.active = true;
  if (delta_seconds > 0.0f) {
    result.updated_history = true;
    for (int axis = 0; axis < 3; ++axis) {
      result.history[axis] =
          previous_history[axis] +
          (source_world_y[axis] - previous_history[axis]) * 0.1f;
    }
  }

  for (int axis = 0; axis < 3; ++axis) {
    result.offset[axis] = source_world_y[axis] - result.history[axis];
    result.pre_clamp_length_sq += result.offset[axis] * result.offset[axis];
  }

  constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
  result.radius_radians = source_radius_degrees * kDegToRad;
  const float radius_sq = result.radius_radians * result.radius_radians;
  if (radius_sq < result.pre_clamp_length_sq &&
      result.pre_clamp_length_sq > 0.0f) {
    const float scale =
        result.radius_radians / std::sqrt(result.pre_clamp_length_sq);
    for (float& value : result.offset) value *= scale;
    result.clamped_to_radius = true;
  }
  return result;
}

namespace {

// ---- little-endian cursor over the entry body ----------------------------
struct Cur {
  const uint8_t* p;
  size_t n;
  size_t pos = 0;
  Cur(const uint8_t* d, size_t l) : p(d), n(l) {}

  bool can(size_t k) const { return pos + k <= n; }
  void need(size_t k) const {
    if (pos + k > n) throw std::runtime_error("char_clip: read past end");
  }
  void skip(size_t k) { need(k); pos += k; }
  uint8_t  u8()  { need(1); uint8_t v = p[pos]; pos += 1; return v; }
  uint16_t u16() { need(2); uint16_t v; std::memcpy(&v, p + pos, 2); pos += 2; return v; }
  int16_t  i16() { return (int16_t)u16(); }
  uint32_t u32() { need(4); uint32_t v; std::memcpy(&v, p + pos, 4); pos += 4; return v; }
  float    f32() { need(4); float v; std::memcpy(&v, p + pos, 4); pos += 4; return v; }
  std::string str() {
    const uint32_t len = u32();
    if (len > n - pos || len > (1u << 20)) {
      throw std::runtime_error("char_clip: implausible string length");
    }
    std::string s(reinterpret_cast<const char*>(p + pos), len);
    pos += len;
    return s;
  }
  uint32_t peek_u32(size_t at) const {
    uint32_t v; std::memcpy(&v, p + at, 4); return v;
  }
};

void read_clip_dtb_node(Cur& c);

void read_clip_dtb_array_parent(Cur& c) {
  const uint16_t child_count = c.u16();
  (void)c.u32();
  for (uint16_t i = 0; i < child_count; ++i) read_clip_dtb_node(c);
}

void read_clip_dtb_parent(Cur& c) {
  if (c.u8() == 0) return;
  read_clip_dtb_array_parent(c);
}

void read_clip_dtb_node(Cur& c) {
  const uint32_t type = c.u32();
  const milo_scene::SourceMiloEditorDtbNodePayloadPlan plan =
      milo_scene::source_milo_editor_dtb_node_payload_plan(
          static_cast<int32_t>(type));
  if (plan.reads_uint32) {
    (void)c.u32();
  } else if (plan.reads_float) {
    (void)c.f32();
  } else if (plan.reads_symbol) {
    (void)c.str();
  } else if (plan.reads_array_parent) {
    read_clip_dtb_array_parent(c);
  }
}

void read_clip_object_fields(Cur& c) {
  const uint32_t combined_revision = c.u32();
  const uint16_t revision = static_cast<uint16_t>(combined_revision & 0xffffu);
  (void)c.str();
  read_clip_dtb_parent(c);
  if (revision > 0) (void)c.str();
}

struct CharClipMetadata {
  bool valid = false;
  uint32_t samples_version = 0;
  uint32_t clip_version = 0;
  float start_beat = 0.0f;
  float end_beat = 0.0f;
  float beats_per_sec = 0.0f;
  uint32_t flags = 0;
  uint32_t play_flags = 0;
  float blend_width = 0.0f;
  float range = 0.0f;
  bool gh2_unknown_bool = false;
  std::string relative;
  size_t samples_offset = SIZE_MAX;
};

CharClipMetadata read_char_clip_metadata(const uint8_t* d, size_t n) {
  CharClipMetadata out;
  try {
    Cur c(d, n);
    if (!c.can(8)) return out;
    out.samples_version = c.u32();
    if (!source_grim_char_clip_samples_version_known(
            static_cast<int>(out.samples_version))) {
      return out;
    }
    out.clip_version = c.u32();
    if (!source_grim_char_clip_version_known(
            static_cast<int>(out.clip_version))) {
      return out;
    }

    read_clip_object_fields(c);
    out.start_beat = c.f32();
    out.end_beat = c.f32();
    out.beats_per_sec = c.f32();
    out.flags = c.u32();
    out.play_flags = c.u32();
    out.blend_width = c.f32();
    if (out.clip_version > 3) out.range = c.f32();
    if (out.clip_version == 5) {
      out.gh2_unknown_bool = c.u8() != 0;
    } else if (out.clip_version > 5) {
      out.relative = c.str();
    }
    if (out.clip_version > 9) (void)c.u32();
    if (out.clip_version > 11) (void)c.u8();

    uint32_t node_count = 0;
    if (out.clip_version < 8) {
      node_count = c.u32();
    } else {
      c.skip(4);
      node_count = c.u32();
    }
    if (node_count > 10000u) return CharClipMetadata{};
    for (uint32_t i = 0; i < node_count; ++i) {
      (void)c.str();
      const uint32_t value_count = c.u32();
      if (value_count > 100000u) return CharClipMetadata{};
      const size_t bytes = static_cast<size_t>(value_count) * 8u;
      c.skip(bytes);
    }

    if (out.clip_version < 7) {
      (void)c.str();
      (void)c.str();
    }

    const uint32_t event_count = c.u32();
    if (event_count > 10000u) return CharClipMetadata{};
    for (uint32_t i = 0; i < event_count; ++i) {
      (void)c.f32();
      (void)c.str();
    }

    out.samples_offset = c.pos;
    out.valid = true;
    return out;
  } catch (...) {
    return CharClipMetadata{};
  }
}

// Is there a valid length-prefixed bone name at byte offset `at`?
bool is_bone_name_at(const uint8_t* d, size_t n, size_t at) {
  if (at + 4 > n) return false;
  uint32_t len; std::memcpy(&len, d + at, 4);
  if (len < 6 || len > 48 || at + 4 + len > n) return false;
  const char* s = reinterpret_cast<const char*>(d + at + 4);
  for (uint32_t i = 0; i < len; ++i)
    if (s[i] < 0x20 || s[i] >= 0x7f) return false;
  std::string cand(s, len);
  auto ends = [&](const char* x) {
    size_t sl = std::strlen(x);
    return cand.size() >= sl && cand.compare(cand.size() - sl, sl, x) == 0;
  };
  const bool suffix = ends(".pos") || ends(".scale") || ends(".quat") ||
                      ends(".rotx") || ends(".roty") || ends(".rotz");
  bool bone = cand.rfind("bone_", 0) == 0 || cand.rfind("spot_", 0) == 0;
  return suffix && bone;
}

// One decoded bone list (CharBonesSamples header).
struct BoneList {
  std::vector<std::string> names;   // full names, file order
  std::vector<int>         cats;    // category per bone
  std::vector<float>       weights;
  uint32_t cum[10] = {};
  int      compression = 1;
  int      num_samples = 0;
  int      n_vec = 0, n_quat = 0, n_angle = 0;
  size_t   frame_bytes = 0;
};

enum SourceCharBonesCompression {
  kSourceCompressNone = 0,
  kSourceCompressRots = 1,
  kSourceCompressVects = 2,
  kSourceCompressQuats = 3,
  kSourceCompressAll = 4,
};

bool is_valid_category_name(const std::string& name) {
  int c = source_grim_char_bones_samples_get_type_of(name);
  return c >= 0 && c < kSourceCharBonesTypeEnd;
}

float env_float_or(const char* name, float fallback) {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  if (_dupenv_s(&value, &len, name) != 0 || !value || !value[0]) {
    std::free(value);
    return fallback;
  }
  char* end = nullptr;
  const float parsed = std::strtof(value, &end);
  std::free(value);
#else
  const char* value = std::getenv(name);
  if (!value || !value[0]) return fallback;
  char* end = nullptr;
  const float parsed = std::strtof(value, &end);
#endif
  return end && end != value && std::isfinite(parsed) ? parsed : fallback;
}

bool source_char_bones_compression_known(int compression) {
  return compression >= kSourceCompressNone &&
         compression <= kSourceCompressAll;
}

const char* source_char_bones_compression_name(int compression) {
  switch (compression) {
    case kSourceCompressNone: return "kCompressNone";
    case kSourceCompressRots: return "kCompressRots";
    case kSourceCompressVects: return "kCompressVects";
    case kSourceCompressQuats: return "kCompressQuats";
    case kSourceCompressAll: return "kCompressAll";
    default: return "unknown";
  }
}

std::array<uint32_t, kSourceCharBonesTypeEnd + 1>
source_grim_char_bones_samples_first_counts(const uint32_t cum[10]) {
  std::array<uint32_t, kSourceCharBonesTypeEnd + 1> counts = {};
  for (size_t i = 0; i < counts.size(); ++i) counts[i] = cum[i];
  return counts;
}

bool uses_source_byte_quat(const BoneList& list) {
  if (list.compression < kSourceCompressQuats) return false;
  return std::find(list.cats.begin(), list.cats.end(), 2) != list.cats.end();
}

bool debug_clip_parse_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_CLIP") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_CLIP");
  return value && value[0];
#endif
}

bool read_zero_bone_list(const uint8_t* d, size_t n, size_t& at,
                         int samples_version,
                         BoneList& out) {
  const SourceGrimCharBonesSamplesHeaderPlan header_plan =
      source_grim_char_bones_samples_header_plan(samples_version);
  if (!header_plan.known_version) return false;
  const size_t header_size =
      4u + static_cast<size_t>(header_plan.count_size) * 4u + 8u;
  if (at + header_size > n) return false;
  Cur c(d, n);
  c.pos = at;
  uint32_t count = c.u32();
  if (count != 0) return false;

  out = BoneList{};
  for (int i = 0; i < header_plan.count_size; ++i) out.cum[i] = c.u32();
  for (int i = 1; i < header_plan.count_size; ++i) {
    if (out.cum[i] < out.cum[i - 1]) return false;
  }
  if (out.cum[header_plan.count_size - 1] != 0) return false;

  out.compression = (int)c.u32();
  out.num_samples = (int)c.u32();
  if (!source_char_bones_compression_known(out.compression)) return false;
  if (!source_grim_char_bones_samples_recompute_sizes(
           out.compression,
           source_grim_char_bones_samples_first_counts(out.cum))
           .valid) {
    return false;
  }
  if (out.num_samples < 0 || out.num_samples > 100000) return false;
  at = c.pos;
  return true;
}

// Try to read a bone-list header starting at byte `at`. On success advances
// `at` past the header and fills `out`. Returns false if not a valid list.
bool read_bone_list(const uint8_t* d, size_t n, size_t& at,
                    int samples_version, BoneList& out) {
  if (at + 4 > n) return false;
  const SourceGrimCharBonesSamplesHeaderPlan header_plan =
      source_grim_char_bones_samples_header_plan(samples_version);
  if (!header_plan.known_version) return false;
  Cur c(d, n);
  c.pos = at;
  uint32_t count = c.u32();
  if (count == 0) return read_zero_bone_list(d, n, at, samples_version, out);
  if (count < 1 || count > 300) return false;
  if (!is_bone_name_at(d, n, c.pos)) return false;  // first entry must be a bone

  out = BoneList{};
  out.names.reserve(count);
  out.weights.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    if (!is_bone_name_at(d, n, c.pos)) return false;
    uint32_t len = c.u32();
    std::string name(reinterpret_cast<const char*>(d + c.pos), len);
    c.pos += len;
    if (!is_valid_category_name(name)) return false;
    out.names.push_back(name);
    out.cats.push_back(source_grim_char_bones_samples_get_type_of(name));
    if (header_plan.reads_weight) {
      if (c.pos + 4 > n) return false;
      out.weights.push_back(c.f32());
    } else {
      out.weights.push_back(1.0f);
    }
  }

  if (c.pos + static_cast<size_t>(header_plan.count_size) * 4u > n) {
    return false;
  }
  for (int i = 0; i < header_plan.count_size; ++i) out.cum[i] = c.u32();
  // Validate: cum[0]==0, non-decreasing, cum[9] <= count.
  if (out.cum[0] != 0) return false;
  for (int i = 1; i < header_plan.count_size; ++i) {
    if (out.cum[i] < out.cum[i - 1]) return false;
  }
  if (out.cum[header_plan.count_size - 1] > count) return false;

  if (c.pos + 8 > n) return false;
  out.compression = (int)c.u32();
  out.num_samples = (int)c.u32();
  if (!source_char_bones_compression_known(out.compression)) return false;
  if (!source_grim_char_bones_samples_recompute_sizes(
           out.compression,
           source_grim_char_bones_samples_first_counts(out.cum))
           .valid) {
    return false;
  }
  if (out.num_samples < 0 || out.num_samples > 100000) return false;

  // Category breakdown from cumulative counts.
  auto cat_n = [&](int cat) -> int {
    uint32_t lo = out.cum[cat];
    uint32_t hi = (cat + 1 < 10) ? out.cum[cat + 1] : count;
    return (int)(hi - lo);
  };
  out.n_vec   = cat_n(0) + cat_n(1);                       // pos + scale
  out.n_quat  = cat_n(2);                                  // quat
  out.n_angle = cat_n(3) + cat_n(4) + cat_n(5);            // rot*
  const SourceGrimCharBonesSamplesDataPlan data_plan =
      source_grim_char_bones_samples_data_plan(samples_version,
                                               out.compression,
                                               out.names,
                                               out.num_samples);
  if (!data_plan.known_version ||
      data_plan.kept_channels.size() != out.names.size()) {
    return false;
  }
  out.frame_bytes = data_plan.sample_size;

  // Source TypeSize proves the 4-byte ByteQuat row for kCompressQuats and
  // kCompressAll, but the checked source snapshot does not expose the exact
  // ByteQuat-to-Quat conversion body. Refuse those lists rather than silently
  // reading four bytes as a ShortQuat.
  if (uses_source_byte_quat(out)) return false;

  if (header_plan.reads_frame_table) return false;

  at = c.pos;
  return true;
}

// Decode one bone's quaternion from the data cursor (advances it). Grim and
// re-notes decode short-packed sample components through the same clamped
// signed-normalized helper used by scalar sample rows; Hmx::Quat stores x,y,z,w.
void read_quat(Cur& c, bool comp, ClipChannel& ch) {
  ch.type = ClipChannel::kQuat;
  if (comp) {
    const int16_t x = c.i16();
    const int16_t y = c.i16();
    const int16_t z = c.i16();
    const int16_t w = c.i16();
    const auto quat = source_grim_char_bones_samples_decode_short_quat(
        x, y, z, w);
    ch.quat[0] = quat[0];  // x
    ch.quat[1] = quat[1];  // y
    ch.quat[2] = quat[2];  // z
    ch.quat[3] = quat[3];  // w
  } else {
    ch.quat[0] = c.f32();      // x
    ch.quat[1] = c.f32();      // y
    ch.quat[2] = c.f32();      // z
    ch.quat[3] = c.f32();      // w
  }
}

float read_snorm16(Cur& c) {
  return source_grim_char_bones_samples_decode_snorm16(c.i16());
}

void read_vec(Cur& c, int cat, int compression, ClipChannel& ch) {
  float x, y, z;
  if (compression < 2) {
    x = c.f32();
    y = c.f32();
    z = c.f32();
  } else {
    x = read_snorm16(c) * 1345.0f;
    y = read_snorm16(c) * 1345.0f;
    z = read_snorm16(c) * 1345.0f;
  }
  if (cat == 1) {
    ch.type = ClipChannel::kScale;
    ch.scale[0] = x;
    ch.scale[1] = y;
    ch.scale[2] = z;
  } else {
    ch.type = ClipChannel::kPos;
    ch.pos[0] = x;
    ch.pos[1] = y;
    ch.pos[2] = z;
  }
}

void skip_grim_scale(Cur& c) {
  (void)c.u32();
}

void skip_grim_angle(Cur& c, bool comp) {
  if (comp) {
    (void)c.u16();
  } else {
    (void)c.u32();
  }
}

void read_angle(Cur& c, bool comp, int cat, ClipChannel& ch) {
  ch.type = cat == kSourceCharBonesTypeRotX ? ClipChannel::kRotX
          : cat == kSourceCharBonesTypeRotY ? ClipChannel::kRotY
                                            : ClipChannel::kRotZ;
  ch.angle = comp ? read_snorm16(c) : c.f32();
}

void add_raw_channel_count(CharClip::RawChannelCounts& counts, int type) {
  switch (type) {
    case kSourceCharBonesTypePos: ++counts.pos; break;
    case kSourceCharBonesTypeScale: ++counts.scale; break;
    case kSourceCharBonesTypeQuat: ++counts.quat; break;
    case kSourceCharBonesTypeRotX: ++counts.rotx; break;
    case kSourceCharBonesTypeRotY: ++counts.roty; break;
    case kSourceCharBonesTypeRotZ: ++counts.rotz; break;
    default: break;
  }
}

// Parse the whole clip entry into frames.
std::vector<std::vector<ClipChannel>> parse_all(
    const uint8_t* d, size_t n, int& num_samples_out,
    CharClip::RawChannelCounts* raw_channel_counts,
    size_t* sample_header_offset_out = nullptr) {
  num_samples_out = 0;
  if (sample_header_offset_out) *sample_header_offset_out = SIZE_MAX;
  if (n < 4) return {};
  uint32_t samples_version = 0;
  std::memcpy(&samples_version, d, 4);
  if (!source_grim_char_clip_samples_version_known(
          static_cast<int>(samples_version))) {
    return {};
  }
  if (samples_version >= 13) return {};

  std::vector<BoneList> lists;
  size_t p = SIZE_MAX;

  // GH2 CharClipSamples entries begin with the samples version, then a CharClip
  // base payload, then full/one CharBonesSamples headers plus a duplicate
  // serialized header for version 8+. Grim then reads data only for full/one.
  // The CharClip base contains arbitrary transition names, so finding the first
  // "bone_*.pos" is not reliable; `neutral` even has an empty list first.
  // Instead, accept only a candidate whose two declared sample blocks consume
  // the remaining entry bytes exactly.
  for (size_t at = 4; at + 52 <= n; ++at) {
    std::vector<BoneList> candidate;
    size_t q = at;
    bool ok = true;
    for (int i = 0; i < 3; ++i) {
      BoneList bl;
      if (!read_bone_list(d, n, q, static_cast<int>(samples_version), bl)) {
        ok = false;
        break;
      }
      candidate.push_back(std::move(bl));
    }
    if (!ok || candidate.empty()) continue;
    bool has_channels = false;
    uint64_t sample_bytes = 0;
    for (size_t i = 0; i < candidate.size() && i < 2; ++i) {
      const auto& bl = candidate[i];
      if (!bl.names.empty()) has_channels = true;
      sample_bytes += static_cast<uint64_t>(bl.frame_bytes) *
                      static_cast<uint64_t>(bl.num_samples);
    }
    if (!has_channels) continue;
    if (sample_bytes == n - q) {
      candidate.resize(2);
      lists = std::move(candidate);
      if (sample_header_offset_out) *sample_header_offset_out = at;
      p = q;
      break;
    }
  }
  if (lists.empty() || p == SIZE_MAX) return {};

  if (raw_channel_counts) {
    *raw_channel_counts = CharClip::RawChannelCounts{};
    for (const BoneList& bl : lists) {
      for (int cat : bl.cats) {
        add_raw_channel_count(*raw_channel_counts, cat);
      }
    }
  }

  if (debug_clip_parse_enabled()) {
    for (size_t i = 0; i < lists.size(); ++i) {
      const BoneList& bl = lists[i];
      int type_counts[kSourceCharBonesTypeEnd] = {};
      for (int cat : bl.cats) {
        if (cat >= 0 && cat < kSourceCharBonesTypeEnd) ++type_counts[cat];
      }
      std::fprintf(stderr,
                   "[clip-source-bones] list=%zu comp=%d(%s) samples=%d "
                   "channels=%zu bytes=%zu byteQuat=%d\n",
                   i, bl.compression,
                   source_char_bones_compression_name(bl.compression),
                   bl.num_samples, bl.names.size(), bl.frame_bytes,
                   uses_source_byte_quat(bl) ? 1 : 0);
      std::fprintf(stderr,
                   "[clip-source-bones-counts] list=%zu vec=%d quat=%d "
                   "angle=%d\n",
                   i, bl.n_vec, bl.n_quat, bl.n_angle);
      std::fprintf(stderr,
                   "[clip-source-bones-types] list=%zu pos=%d scale=%d "
                   "quat=%d rotx=%d roty=%d rotz=%d\n",
                   i, type_counts[kSourceCharBonesTypePos],
                   type_counts[kSourceCharBonesTypeScale],
                   type_counts[kSourceCharBonesTypeQuat],
                   type_counts[kSourceCharBonesTypeRotX],
                   type_counts[kSourceCharBonesTypeRotY],
                   type_counts[kSourceCharBonesTypeRotZ]);
    }
  }

  // Sample data begins at p (after the third header). Use the max declared
  // frame count; one-sample lists are constant channels repeated across frames.
  int num_samples = 0;
  for (auto& bl : lists) num_samples = std::max(num_samples, bl.num_samples);
  if (num_samples <= 0) return {};
  num_samples_out = num_samples;

  std::vector<std::vector<ClipChannel>> frames(num_samples);

  // For each list, its block is frames_here frames walked in serialized bone
  // order. Grim's decode_samples iterates `for bone in bones`, consuming each
  // channel's get_type_size2 bytes before moving to the next bone.
  // GH2 clips commonly include a full-rate list plus a one-sample list for
  // constant channels; repeat that single sample so every frame is a complete
  // pose instead of silently losing those channels after frame 0.
  size_t data = p;
  for (auto& bl : lists) {
    int frames_here = bl.num_samples > 0 ? bl.num_samples : num_samples;
    bool comp = bl.compression != 0;
    for (int f = 0; f < num_samples; ++f) {
      if (frames_here != 1 && f >= frames_here) break;
      int sample_idx = (frames_here == 1) ? 0 : f;
      size_t frame_off = data + (size_t)sample_idx * bl.frame_bytes;
      if (frame_off + bl.frame_bytes > n) break;
      Cur c(d, n);
      c.pos = frame_off;

      for (size_t bi = 0; bi < bl.names.size(); ++bi) {
        ClipChannel ch;
        ch.bone_name =
            source_grim_char_bones_samples_channel_mesh_name(bl.names[bi]);
        ch.source_weight =
            source_grim_char_bones_samples_channel_weight(bl.weights, bi);
        const int cat = bl.cats[bi];
        if (!source_grim_char_bones_samples_decodes_channel_type(cat)) {
          switch (cat) {
            case kSourceCharBonesTypeScale:
              skip_grim_scale(c);
              break;
            case kSourceCharBonesTypeRotX:
            case kSourceCharBonesTypeRotY:
              skip_grim_angle(c, comp);
              break;
            default:
              break;
          }
          continue;
        }
        switch (cat) {
          case kSourceCharBonesTypePos:
            read_vec(c, cat, bl.compression, ch);
            frames[f].push_back(ch);
            break;
          case kSourceCharBonesTypeRotZ:
            read_angle(c, comp, cat, ch);
            frames[f].push_back(ch);
            break;
          case kSourceCharBonesTypeQuat:
            read_quat(c, comp, ch);
            frames[f].push_back(ch);
            break;
          default:
            break;
        }
      }
    }
    data += (size_t)frames_here * bl.frame_bytes;
  }

  for (std::vector<ClipChannel>& frame : frames) {
    source_grim_char_bones_samples_sort_decoded_channels(frame);
  }

  return frames;
}

}  // namespace

std::string source_grim_char_bones_samples_channel_mesh_name(
    const std::string& channel) {
  std::string name = channel;
  auto replace_all = [](std::string& s, const char* from, const char* to) {
    const size_t from_len = std::strlen(from);
    const size_t to_len = std::strlen(to);
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
      s.replace(pos, from_len, to);
      pos += to_len;
    }
  };
  replace_all(name, ".pos", ".mesh");
  replace_all(name, ".quat", ".mesh");
  replace_all(name, ".rotz", ".mesh");
  return name;
}

bool debug_clip_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_CLIP") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_CLIP");
  return value && value[0];
#endif
}

bool debug_clip_hair_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_CLIP_HAIR") == 0 && value &&
      value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_CLIP_HAIR");
  return value && value[0];
#endif
}

bool clip_hair_debug_name(const std::string& name) {
  std::string lower = name;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return lower.find("hair") != std::string::npos ||
         lower.find("bang") != std::string::npos ||
         lower.find("pony") != std::string::npos ||
         lower.find("coat") != std::string::npos ||
         lower.find("chain") != std::string::npos ||
         lower.find("wing") != std::string::npos ||
         lower.find("lantern") != std::string::npos;
}

bool debug_lane_mixer_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_LANE_MIXER") == 0 && value &&
      value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_LANE_MIXER");
  return value && value[0];
#endif
}

bool debug_ik_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_IK") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_IK");
  return value && value[0];
#endif
}

bool debug_leg_pose_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_LEG_POSE") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_LEG_POSE");
  return value && value[0];
#endif
}

bool debug_arm_pose_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_ARM_POSE") == 0 && value &&
      value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_ARM_POSE");
  return value && value[0];
#endif
}

bool controller_audit_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_AUDIT_CHARACTER_GRAPH") == 0 && value &&
      value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_AUDIT_CHARACTER_GRAPH");
  return value && value[0];
#endif
}

bool debug_char_hair_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_CHAR_HAIR") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_CHAR_HAIR");
  return value && value[0];
#endif
}

void log_char_hair_source_once(const Character& character,
                               const CharHair& hair) {
  if (!debug_char_hair_enabled()) return;
  static std::unordered_set<std::string> logged;
  const std::string key = character.dir_name + "|" + hair.name;
  if (!logged.insert(key).second) return;

  size_t point_count = 0;
  for (const auto& strand : hair.strands) {
    point_count += strand.points.size();
  }

  std::fprintf(stderr,
               "[charhair-source] character=%s hair=%s "
               "source=ihatecompvir-CharHair version=%d simulate=%d "
               "strands=%zu points=%zu "
               "globals=(stiff=%.4f torsion=%.4f inertia=%.4f "
               "gravity=%.4f weight=%.4f friction=%.4f "
               "minSlack=%.4f maxSlack=%.4f)\n",
               character.dir_name.c_str(), hair.name.c_str(), hair.version,
               hair.simulate ? 1 : 0, hair.strands.size(), point_count,
               hair.stiffness, hair.torsion, hair.inertia, hair.gravity,
               hair.weight, hair.friction, hair.min_slack, hair.max_slack);

  for (size_t strand_i = 0; strand_i < hair.strands.size(); ++strand_i) {
    const auto& strand = hair.strands[strand_i];
    std::fprintf(stderr,
                 "[charhair-source-strand] hair=%s strand=%zu root=%s "
                 "points=%zu angle=%.4f hookup=0x%08x "
                 "baseRow0=(%.4f %.4f %.4f) rootRow0=(%.4f %.4f %.4f)\n",
                 hair.name.c_str(), strand_i, strand.root.c_str(),
                 strand.points.size(), strand.angle,
                 static_cast<unsigned>(strand.hookup_flags),
                 strand.base_mat[0], strand.base_mat[1], strand.base_mat[2],
                 strand.root_mat[0], strand.root_mat[1], strand.root_mat[2]);
    for (size_t point_i = 0; point_i < strand.points.size(); ++point_i) {
      const auto& point = strand.points[point_i];
      std::fprintf(stderr,
                   "[charhair-source-point] hair=%s strand=%zu point=%zu "
                   "bone=%s collision=%s collideType=%u legacyInline=loggedOnly "
                   "pos=(%.4f %.4f %.4f) len=%.4f "
                   "radius=%.4f outer=%.4f side=%.4f "
                   "unk5c=(%.4f %.4f %.4f)\n",
                   hair.name.c_str(), strand_i, point_i, point.bone.c_str(),
                   point.collision.c_str(),
                   static_cast<unsigned>(point.collide_type), point.pos[0],
                   point.pos[1], point.pos[2], point.length, point.radius,
                   point.outer_radius, point.side_length, point.unk5c[0],
                   point.unk5c[1], point.unk5c[2]);
    }
  }
}

void log_character_controller_graph_once(const Character& character) {
  if (!controller_audit_enabled()) return;
  static std::unordered_set<std::string> logged;
  const std::string key = character.dir_name.empty()
                              ? ("<unnamed>@" + std::to_string(
                                                 reinterpret_cast<uintptr_t>(
                                                     &character)))
                              : character.dir_name;
  if (!logged.insert(key).second) return;

  std::fprintf(stderr,
               "[chargraph] %s bones=%zu meshes=%zu drivers=%zu "
               "weightSetters=%zu ik=%zu ikMidi=%zu foreTwist=%zu upperTwist=%zu "
               "hair=%zu collide=%zu posConstraint=%zu animFilter=%zu "
               "lookAt=%zu eyes=%zu sourceIK=%s hairPoll=%s "
               "lookAt=%s\n",
               character.dir_name.c_str(), character.bones.size(),
               character.meshes.size(), character.drivers.size(),
               character.weight_setters.size(), character.ik_hands.size(),
               character.ik_midis.size(), character.fore_twists.size(),
               character.upper_twists.size(), character.hairs.size(),
               character.collides.size(), character.pos_constraints.size(),
               character.anim_filters.size(), character.lookats.size(),
               character.eyes.size(),
               "CharIKHand", "CharHair::Poll-missingHookup", "decode-only");
  for (const auto& ik : character.ik_hands) {
    std::fprintf(stderr,
                 "[chargraph]   ik %s version=%d hand=%s finger=%s "
                 "target=%s targets=%zu weight=%.3f weightProp=%s "
                 "orientation=%d stretch=%d scalable=%d moveElbow=%d "
                 "elbowSwing=%.3f alwaysElbow=%d constrainWrist=%d "
                 "wristRadians=%.3f elbowCollide=%s clockwise=%d "
                 "unreadBytes=%zu\n",
                 ik.name.c_str(), ik.version, ik.hand.c_str(),
                 ik.finger.empty() ? "<none>" : ik.finger.c_str(),
                 ik.target.c_str(), ik.targets.size(), ik.weight,
                 ik.weight_prop.c_str(),
                 ik.orientation ? 1 : 0, ik.stretch ? 1 : 0,
                 ik.scalable ? 1 : 0, ik.move_elbow ? 1 : 0,
                 ik.elbow_swing, ik.always_ik_elbow ? 1 : 0,
                 ik.constrain_wrist ? 1 : 0, ik.wrist_radians,
                 ik.elbow_collide.empty() ? "<none>"
                                           : ik.elbow_collide.c_str(),
                 ik.clockwise ? 1 : 0, ik.unread_bytes);
  }
  for (const auto& driver : character.drivers) {
    std::fprintf(stderr,
                 "[chargraph]   driver %s version=%d "
                 "weightableVersion=%d target=%s clipMilo=%s "
                 "weight=%.3f weightOwner=%s weightProp=%s enabled=%d "
                 "midi=%d midiVersion=%d midiUnreadBytes=%zu "
                 "midiParser=%s midiFlagParser=%s "
                 "midiBlendOverridePct=%.3f\n",
                 driver.name.c_str(), driver.version,
                 driver.weightable_version, driver.target.c_str(),
                 driver.clip_milo.c_str(), driver.weight,
                 driver.weight_owner.c_str(), driver.weight_prop.c_str(),
                 driver.enabled ? 1 : 0, driver.midi ? 1 : 0,
                 driver.midi_version, driver.midi_unread_bytes,
                 driver.midi_parser.empty() ? "<none>"
                                            : driver.midi_parser.c_str(),
                 driver.midi_flag_parser.empty()
                     ? "<none>"
                     : driver.midi_flag_parser.c_str(),
                 driver.midi_blend_override_pct);
  }
  for (const auto& ik : character.ik_midis) {
    std::fprintf(stderr,
                 "[chargraph]   ikMidi %s version=%d bone=%s "
                 "legacySpots=%zu legacyString=%s animBlender=%s "
                 "maxAnimBlend=%.3f unreadBytes=%zu\n",
                 ik.name.c_str(), ik.version,
                 ik.bone.empty() ? "<none>" : ik.bone.c_str(),
                 ik.legacy_spots.size(),
                 ik.legacy_string.empty() ? "<none>"
                                          : ik.legacy_string.c_str(),
                 ik.anim_blender.empty() ? "<none>"
                                         : ik.anim_blender.c_str(),
                 ik.max_anim_blend, ik.unread_bytes);
  }
  for (const auto& setter : character.weight_setters) {
    std::fprintf(stderr,
                 "[chargraph]   weightSetter %s version=%d "
                 "weightableVersion=%d weight=%.3f weightOwner=%s "
                 "driver=%s flags=0x%08x offset=%.3f scale=%.3f "
                 "baseWeight=%.3f beatsPerWeight=%.3f unreadBytes=%zu\n",
                 setter.name.c_str(), setter.version,
                 setter.weightable_version, setter.weight,
                 setter.weight_owner.c_str(), setter.driver.c_str(),
                 setter.flags, setter.offset, setter.scale,
                 setter.base_weight, setter.beats_per_weight,
                 setter.unread_bytes);
  }
  for (const auto& filter : character.anim_filters) {
    std::fprintf(stderr,
                 "[chargraph]   animFilter %s version=%d "
                 "animatableVersion=%d anim=%s frame=%.3f rate=%d "
                 "scale=%.3f offset=%.3f start=%.3f end=%.3f "
                 "type=%d period=%.3f snap=%.3f jitter=%.3f "
                 "unreadBytes=%zu\n",
                 filter.name.c_str(), filter.version,
                 filter.animatable_version,
                 filter.anim.empty() ? "<none>" : filter.anim.c_str(),
                 filter.frame, filter.rate, filter.scale, filter.offset,
                 filter.start, filter.end, filter.type, filter.period,
                 filter.snap, filter.jitter, filter.unread_bytes);
  }
  for (const auto& bone : character.bones) {
    if (bone.name.rfind("spot_", 0) == 0 ||
        bone.name.find("fret") != std::string::npos ||
        bone.name.find("strum") != std::string::npos) {
      std::fprintf(stderr,
                   "[chargraph]   trans %s parent=%s local=[%.2f %.2f %.2f]\n",
                   bone.name.c_str(), bone.parent.c_str(), bone.local.pos[0],
                   bone.local.pos[1], bone.local.pos[2]);
    }
  }
  for (const auto& ft : character.fore_twists) {
    std::fprintf(stderr,
                 "[chargraph]   foreTwist %s hand=%s twist2=%s offset=%.3f\n",
                 ft.name.c_str(), ft.hand.c_str(), ft.twist2.c_str(),
                 ft.offset_degrees);
  }
  for (const auto& ut : character.upper_twists) {
    std::fprintf(stderr,
                 "[chargraph]   upperTwist %s upper=%s twist1=%s twist2=%s\n",
                 ut.name.c_str(), ut.upper_arm.c_str(), ut.twist1.c_str(),
                 ut.twist2.c_str());
  }
  for (const auto& hair : character.hairs) {
    size_t point_count = 0;
    for (const auto& strand : hair.strands) point_count += strand.points.size();
    std::fprintf(stderr,
                 "[chargraph]   hair %s strands=%zu points=%zu simulate=%d "
                 "globals=[%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f]\n",
                 hair.name.c_str(), hair.strands.size(), point_count,
                 hair.simulate ? 1 : 0, hair.stiffness, hair.torsion,
                 hair.inertia, hair.gravity, hair.weight, hair.friction,
                 hair.min_slack, hair.max_slack);
    for (const auto& strand : hair.strands) {
      std::fprintf(stderr,
                   "[chargraph]     hairStrand root=%s angle=%.3f "
                   "points=%zu rows=[%.3f %.3f %.3f | %.3f %.3f %.3f] "
                   "all=[%.3f %.3f %.3f %.3f %.3f %.3f "
                   "%.3f %.3f %.3f %.3f %.3f %.3f "
                   "%.3f %.3f %.3f %.3f %.3f %.3f]\n",
                   strand.root.c_str(), strand.angle,
                   strand.points.size(), strand.base_mat[0],
                   strand.base_mat[1], strand.base_mat[2],
                   strand.root_mat[0], strand.root_mat[1],
                   strand.root_mat[2], strand.base_mat[0],
                   strand.base_mat[1], strand.base_mat[2],
                   strand.base_mat[3], strand.base_mat[4],
                   strand.base_mat[5], strand.base_mat[6],
                   strand.base_mat[7], strand.base_mat[8],
                   strand.root_mat[0], strand.root_mat[1],
                   strand.root_mat[2], strand.root_mat[3],
                   strand.root_mat[4], strand.root_mat[5],
                   strand.root_mat[6], strand.root_mat[7],
                   strand.root_mat[8]);
      for (const auto& point : strand.points) {
        std::fprintf(stderr,
                     "[chargraph]       hairPoint bone=%s "
                     "collision=%s collideType=%u "
                     "pos=[%.3f %.3f %.3f] length=%.3f radius=%.3f "
                     "outer=%.3f side=%.3f unk5c=[%.3f %.3f %.3f]\n",
                     point.bone.c_str(), point.collision.c_str(),
                     static_cast<unsigned>(point.collide_type), point.pos[0],
                     point.pos[1], point.pos[2], point.length, point.radius,
                     point.outer_radius, point.side_length, point.unk5c[0],
                     point.unk5c[1], point.unk5c[2]);
      }
    }
  }
  for (const auto& collide : character.collides) {
    std::fprintf(stderr,
                 "[chargraph]   collide %s version=%d shape=%d flags=0x%08x "
                 "mesh=%s parent=%s radius=(%.3f %.3f) "
                 "length=(%.3f %.3f) curRadius=(%.3f %.3f) "
                 "curLength=(%.3f %.3f) meshYBias=%d\n",
                 collide.name.c_str(), collide.version, collide.shape,
                 static_cast<unsigned>(collide.flags), collide.mesh.c_str(),
                 collide.parent.c_str(), collide.orig_radius[0],
                 collide.orig_radius[1], collide.orig_length[0],
                 collide.orig_length[1], collide.cur_radius[0],
                 collide.cur_radius[1], collide.cur_length[0],
                 collide.cur_length[1], collide.mesh_y_bias ? 1 : 0);
  }
  for (const auto& constraint : character.pos_constraints) {
    std::fprintf(stderr,
                 "[chargraph]   posConstraint %s version=%d source=%s "
                 "targets=%zu boxMin=[%.3f %.3f %.3f] "
                 "boxMax=[%.3f %.3f %.3f]\n",
                 constraint.name.c_str(), constraint.version,
                 constraint.source.empty() ? "<none>"
                                           : constraint.source.c_str(),
                 constraint.targets.size(), constraint.box_min[0],
                 constraint.box_min[1], constraint.box_min[2],
                 constraint.box_max[0], constraint.box_max[1],
                 constraint.box_max[2]);
    for (const auto& target : constraint.targets) {
      std::fprintf(stderr, "[chargraph]     posTarget %s\n",
                   target.empty() ? "<none>" : target.c_str());
    }
  }
  for (const auto& offset : character.bone_offsets) {
    std::fprintf(stderr,
                 "[chargraph]   boneOffset %s version=%d dest=%s "
                 "offset=[%.3f %.3f %.3f] unreadBytes=%zu\n",
                 offset.name.c_str(), offset.version,
                 offset.dest.empty() ? "<none>" : offset.dest.c_str(),
                 offset.offset[0], offset.offset[1], offset.offset[2],
                 offset.unread_bytes);
  }
  for (const auto& twist : character.bone_twists) {
    std::fprintf(stderr,
                 "[chargraph]   boneTwist %s version=%d bone=%s targets=%zu "
                 "weightableVersion=%d weight=%.3f weightOwner=%s "
                 "unreadBytes=%zu\n",
                 twist.name.c_str(), twist.version,
                 twist.bone.empty() ? "<none>" : twist.bone.c_str(),
                 twist.targets.size(), twist.weightable_version, twist.weight,
                 twist.weight_owner.empty() ? "<none>"
                                            : twist.weight_owner.c_str(),
                 twist.unread_bytes);
  }
  for (const auto& look : character.lookats) {
    std::fprintf(stderr,
                 "[chargraph]   lookAt %s version=%d "
                 "weightableVersion=%d weight=%.3f weightOwner=%s "
                 "source=%s pivot=%s dest=%s halfTime=%.3f "
                 "yaw=(%.3f %.3f) pitch=(%.3f %.3f) "
                 "weightYaw=(%.3f %.3f speed=%.3f) allowRoll=%d "
                 "jitter=%d sourceRadius=%.3f unreadBytes=%zu\n",
                 look.name.c_str(), look.version, look.weightable_version,
                 look.weight,
                 look.weight_owner.empty() ? "<none>" : look.weight_owner.c_str(),
                 look.source.empty() ? "<none>" : look.source.c_str(),
                 look.pivot.empty() ? "<none>" : look.pivot.c_str(),
                 look.dest.empty() ? "<none>" : look.dest.c_str(),
                 look.half_time, look.min_yaw, look.max_yaw,
                 look.min_pitch, look.max_pitch, look.min_weight_yaw,
                 look.max_weight_yaw, look.weight_yaw_speed,
                 look.allow_roll ? 1 : 0, look.enable_jitter ? 1 : 0,
                 look.source_radius, look.unread_bytes);
  }
  for (const auto& eyes : character.eyes) {
    std::fprintf(stderr,
                 "[chargraph]   eyes %s version=%d lookats=%zu "
                 "legacyTransform=%s unreadBytes=%zu\n",
                 eyes.name.c_str(), eyes.version, eyes.lookats.size(),
                 eyes.legacy_transform.empty() ? "<none>"
                                               : eyes.legacy_transform.c_str(),
                 eyes.unread_bytes);
    for (const auto& lookat : eyes.lookats) {
      std::fprintf(stderr, "[chargraph]     eyesLookAt %s\n",
                   lookat.c_str());
    }
  }
}

bool debug_face_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_FACE") == 0 && value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_FACE");
  return value && value[0];
#endif
}

bool is_face_quat_bone(const std::string& name) {
  return name.find("upperlid") != std::string::npos ||
         name.find("brow") != std::string::npos ||
         name.find("cheek") != std::string::npos ||
         name.find("jaw") != std::string::npos;
}

static std::string read_len_string(const uint8_t* data, size_t size,
                                   size_t& pos) {
  if (pos + 4 > size) throw std::runtime_error("short string length");
  uint32_t len = 0;
  std::memcpy(&len, data + pos, 4);
  pos += 4;
  if (len > size - pos || len > (1u << 20))
    throw std::runtime_error("implausible string length");
  std::string s(reinterpret_cast<const char*>(data + pos), len);
  pos += len;
  return s;
}

static uint8_t read_u8_at(const uint8_t* data, size_t size, size_t& pos,
                          const char* label) {
  if (pos + 1 > size) {
    throw std::runtime_error(std::string("short ") + label);
  }
  return data[pos++];
}

static void skip_bytes_at(const uint8_t* data, size_t size, size_t& pos,
                          size_t count, const char* label) {
  (void)data;
  if (pos + count > size) {
    throw std::runtime_error(std::string("short ") + label);
  }
  pos += count;
}

static uint32_t read_u32_at(const uint8_t* data, size_t size, size_t& pos,
                            const char* label) {
  if (pos + 4 > size) {
    throw std::runtime_error(std::string("short ") + label);
  }
  uint32_t value = 0;
  std::memcpy(&value, data + pos, 4);
  pos += 4;
  return value;
}

static int32_t read_i32_at(const uint8_t* data, size_t size, size_t& pos,
                           const char* label) {
  return static_cast<int32_t>(read_u32_at(data, size, pos, label));
}

static float read_f32_at(const uint8_t* data, size_t size, size_t& pos,
                         const char* label) {
  const uint32_t raw = read_u32_at(data, size, pos, label);
  float value = 0.0f;
  std::memcpy(&value, &raw, 4);
  return value;
}

static milo_scene::Xfm read_xfm_at(const uint8_t* data, size_t size,
                                   size_t& pos) {
  if (pos + 48 > size) throw std::runtime_error("short matrix");
  milo_scene::Xfm x;
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      std::memcpy(&x.rot[r][c], data + pos, 4);
      pos += 4;
    }
  }
  for (int c = 0; c < 3; ++c) {
    std::memcpy(&x.pos[c], data + pos, 4);
    pos += 4;
  }
  return x;
}

static CharClip::OutputBone decode_output_bone(
    const std::string& entry_name, const uint8_t* body, size_t size) {
  size_t pos = 0;
  CharClip::OutputBone out;
  out.name = entry_name;
  out.char_bone_version = read_u32_at(body, size, pos, "CharBone version");
  skip_bytes_at(body, size, pos, 9, "CharBone object fields");
  if (out.char_bone_version < 9) {
    out.trans_version = read_u32_at(body, size, pos, "RndTransformable version");
    out.local = read_xfm_at(body, size, pos);
    out.world_stored = read_xfm_at(body, size, pos);
    if (out.trans_version < 9) {
      const uint32_t child_count =
          read_u32_at(body, size, pos, "legacy trans child count");
      for (uint32_t i = 0; i < child_count; ++i) {
        (void)read_len_string(body, size, pos);
      }
    }
    if (out.trans_version > 6) {
      out.trans_constraint =
          read_u32_at(body, size, pos, "RndTransformable constraint");
    }
    if (out.trans_version > 5) {
      out.trans_target = read_len_string(body, size, pos);
    }
    if (out.trans_version > 6) {
      out.preserve_scale =
          read_u8_at(body, size, pos, "RndTransformable preserve scale") != 0;
    }
    out.parent = read_len_string(body, size, pos);
  }

  if (out.char_bone_version > 6) {
    out.position_context =
        read_i32_at(body, size, pos, "CharBone position context");
  } else {
    out.position_context =
        read_u8_at(body, size, pos, "CharBone legacy position context") ? 1 : 0;
  }
  if (out.char_bone_version > 6) {
    out.scale_context =
        read_i32_at(body, size, pos, "CharBone scale context");
  } else if (out.char_bone_version > 1) {
    out.scale_context =
        read_u8_at(body, size, pos, "CharBone legacy scale context") ? 1 : 0;
  }
  out.rotation_type = read_i32_at(body, size, pos, "CharBone rotation type");
  if (out.char_bone_version < 5) {
    out.legacy_pre_rev5_int =
        read_i32_at(body, size, pos, "CharBone pre-rev5 legacy int");
    out.has_legacy_pre_rev5_int = true;
  }
  if (out.char_bone_version < 2) {
    out.scale_context = 0;
    ++out.rotation_type;
  }
  constexpr int32_t kSourceTypeEnd = 6;
  if (out.char_bone_version < 5 && out.rotation_type > kSourceTypeEnd) {
    out.rotation_type = kSourceTypeEnd;
  }
  if (out.char_bone_version > 6) {
    out.rotation_context =
        read_i32_at(body, size, pos, "CharBone rotation context");
  } else {
    out.rotation_context = out.rotation_type != kSourceTypeEnd ? 1 : 0;
  }
  if (out.char_bone_version == 3 || out.char_bone_version == 4 ||
      out.char_bone_version == 5 || out.char_bone_version == 6 ||
      out.char_bone_version == 7) {
    out.legacy_rev3_to_7_int =
        read_i32_at(body, size, pos, "CharBone rev3-7 legacy int");
    out.has_legacy_rev3_to_7_int = true;
  }
  if (out.char_bone_version > 3) {
    out.target = read_len_string(body, size, pos);
  }
  if (out.char_bone_version == 6) {
    const int32_t ctx =
        read_i32_at(body, size, pos, "CharBone rev6 shared context");
    if (out.position_context != 0) out.position_context = ctx;
    if (out.scale_context != 0) out.scale_context = ctx;
    if (out.rotation_context != 0) out.rotation_context = ctx;
  }
  if (out.char_bone_version > 7) {
    const uint32_t count =
        read_u32_at(body, size, pos, "CharBone weight count");
    out.weights.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
      CharClip::OutputBone::WeightContext weight;
      weight.context = read_i32_at(body, size, pos, "CharBone weight context");
      weight.weight = read_f32_at(body, size, pos, "CharBone weight value");
      out.weights.push_back(weight);
    }
  }
  if (out.char_bone_version > 8) {
    out.trans = read_len_string(body, size, pos);
  }
  if (out.char_bone_version > 9) {
    out.bake_out_as_top_level =
        read_u8_at(body, size, pos, "CharBone bake out flag") != 0;
  }
  out.unread_bytes = size - pos;
  return out;
}

CharClip load_clip(const std::string& hdr_path, const std::string& ark_path,
                   const std::string& milo_path, const std::string& clip_name) {
  CharClip result;
  result.name = clip_name;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find(milo_path);
    if (!entry) entry = ark.find("../../system/run/" + milo_path);
    if (!entry) { std::fprintf(stderr, "[clip] milo not in ARK: %s\n", milo_path.c_str()); return result; }
    auto bytes = ark.read_entry(*entry, {ark_path});
    auto hdr = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, hdr);
    auto dir = gh::milo::parse_directory(payload);

    for (const auto& de : dir.entries) {
      if (de.type != "CharBone") continue;
      try {
        const uint8_t* body = payload.data() + de.offset;
        result.output_bones.push_back(
            decode_output_bone(de.name, body, static_cast<size_t>(de.size)));
        if (debug_clip_enabled()) {
          const auto& out = result.output_bones.back();
          std::fprintf(stderr,
                       "[clip-output] %-28s sourceCharBone version=%u "
                       "transVersion=%u constraint=%u target=%s preserve=%d "
                       "parent=%-28s posCtx=%d scaleCtx=%d rotType=%d "
                       "rotCtx=%d charTarget=%s weights=%zu trans=%s "
                       "bakeOut=%d unreadBytes=%zu "
                       "localPos=(%.3f %.3f %.3f)\n",
                       out.name.c_str(), out.char_bone_version,
                       out.trans_version, out.trans_constraint,
                       out.trans_target.empty() ? "<none>"
                                                : out.trans_target.c_str(),
                       out.preserve_scale ? 1 : 0, out.parent.c_str(),
                       out.position_context, out.scale_context,
                       out.rotation_type, out.rotation_context,
                       out.target.empty() ? "<none>" : out.target.c_str(),
                       out.weights.size(),
                       out.trans.empty() ? "<none>" : out.trans.c_str(),
                       out.bake_out_as_top_level ? 1 : 0, out.unread_bytes,
                       out.local.pos[0], out.local.pos[1], out.local.pos[2]);
        }
      } catch (const std::exception& ex) {
        if (debug_clip_enabled()) {
          std::fprintf(stderr, "[clip] CharBone '%s' decode: %s\n",
                       de.name.c_str(), ex.what());
        }
      }
    }

    for (const auto& de : dir.entries) {
      if (de.type != "CharClipSamples" || de.name != clip_name) continue;
      const uint8_t* body = payload.data() + de.offset;
      size_t sz = (size_t)de.size;
      int ns = 0;
      const CharClipMetadata metadata = read_char_clip_metadata(body, sz);
      size_t sample_header_offset = SIZE_MAX;
      result.frames = parse_all(body, sz, ns, &result.raw_channel_counts,
                                &sample_header_offset);
      result.fps = 30;  // CharClipSamples are authored at 30 fps; refine if needed.
      result.start_frame = 0.0f;
      result.end_frame = result.frames.empty()
                             ? 0.0f
                             : static_cast<float>(result.frames.size() - 1);
      if (metadata.valid) {
        result.flags = metadata.flags;
        result.default_play_flags = metadata.play_flags;
        result.blend_width = metadata.blend_width;
        result.range = metadata.range;
        if (!metadata.relative.empty()) result.relative = true;
      } else {
        result.default_play_flags = kCharPlayLoop;
      }
      result.relative = result.relative || clip_name == "visemes";
      result.loaded = !result.frames.empty();
      if (result.loaded) {
        std::fprintf(stderr,
                     "[clip] '%s': %zu frames, %zu channels/frame, %zu output bones "
                     "flags=0x%08x playFlags=0x%08x blend=%.3f range=%.3f\n",
                     clip_name.c_str(), result.frames.size(),
                     result.frames.empty() ? 0 : result.frames[0].size(),
                     result.output_bones.size(), result.flags,
                     result.default_play_flags, result.blend_width,
                     result.range);
        if (debug_clip_enabled() && metadata.valid &&
            metadata.samples_offset != sample_header_offset) {
          std::fprintf(stderr,
                       "[clip] '%s': metadata sample header 0x%zx, scanner 0x%zx\n",
                       clip_name.c_str(), metadata.samples_offset,
                       sample_header_offset);
        }
        if (debug_clip_enabled() && !result.frames.empty()) {
          const auto& frame0 = result.frames[0];
          const size_t limit = std::min<size_t>(frame0.size(), 128);
          for (size_t i = 0; i < limit; ++i) {
            const auto& ch = frame0[i];
            const char* type = ch.type == ClipChannel::kPos ? "pos" :
                               ch.type == ClipChannel::kScale ? "scale" :
                               ch.type == ClipChannel::kQuat ? "quat" :
                               ch.type == ClipChannel::kRotX ? "rotx" :
                               ch.type == ClipChannel::kRotY ? "roty" : "rotz";
            if (ch.type == ClipChannel::kQuat) {
              std::fprintf(stderr,
                           "[clip]   %03zu %-5s %-28s [%.5f %.5f %.5f %.5f]\n",
                           i, type, ch.bone_name.c_str(), ch.quat[0],
                           ch.quat[1], ch.quat[2], ch.quat[3]);
            } else if (ch.type == ClipChannel::kPos) {
              std::fprintf(stderr,
                           "[clip]   %03zu %-5s %-28s [%.5f %.5f %.5f]\n",
                           i, type, ch.bone_name.c_str(), ch.pos[0],
                           ch.pos[1], ch.pos[2]);
            } else {
              std::fprintf(stderr, "[clip]   %03zu %-5s %-28s %.5f\n",
                           i, type, ch.bone_name.c_str(), ch.angle);
            }
          }
        }
        if (debug_clip_hair_enabled()) {
          for (size_t i = 0; i < result.output_bones.size(); ++i) {
            const auto& out = result.output_bones[i];
            if (!clip_hair_debug_name(out.name) &&
                !clip_hair_debug_name(out.parent)) {
              continue;
            }
            std::fprintf(stderr,
                         "[clip-hair-output] clip=%s index=%zu name=%s parent=%s "
                         "local=(%.4f %.4f %.4f)\n",
                         clip_name.c_str(), i, out.name.c_str(),
                         out.parent.c_str(), out.local.pos[0],
                         out.local.pos[1], out.local.pos[2]);
          }
          if (!result.frames.empty()) {
            const auto& frame0 = result.frames[0];
            for (size_t i = 0; i < frame0.size(); ++i) {
              const auto& ch = frame0[i];
              if (!clip_hair_debug_name(ch.bone_name)) continue;
              const char* type = ch.type == ClipChannel::kPos ? "pos" :
                                 ch.type == ClipChannel::kScale ? "scale" :
                                 ch.type == ClipChannel::kQuat ? "quat" :
                                 ch.type == ClipChannel::kRotX ? "rotx" :
                                 ch.type == ClipChannel::kRotY ? "roty" : "rotz";
              if (ch.type == ClipChannel::kQuat) {
                std::fprintf(stderr,
                             "[clip-hair-channel] clip=%s index=%zu type=%s "
                             "name=%s value=(%.5f %.5f %.5f %.5f)\n",
                             clip_name.c_str(), i, type, ch.bone_name.c_str(),
                             ch.quat[0], ch.quat[1], ch.quat[2], ch.quat[3]);
              } else if (ch.type == ClipChannel::kPos) {
                std::fprintf(stderr,
                             "[clip-hair-channel] clip=%s index=%zu type=%s "
                             "name=%s value=(%.5f %.5f %.5f)\n",
                             clip_name.c_str(), i, type, ch.bone_name.c_str(),
                             ch.pos[0], ch.pos[1], ch.pos[2]);
              } else {
                std::fprintf(stderr,
                             "[clip-hair-channel] clip=%s index=%zu type=%s "
                             "name=%s value=%.5f\n",
                             clip_name.c_str(), i, type, ch.bone_name.c_str(),
                             ch.angle);
              }
            }
          }
        }
      } else {
        std::fprintf(stderr, "[clip] '%s': parse failed\n", clip_name.c_str());
      }
      return result;
    }
    std::fprintf(stderr, "[clip] '%s' not found in %s\n", clip_name.c_str(), milo_path.c_str());
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[clip] load_clip: %s\n", ex.what());
  }
  return result;
}

CharClipGroup load_clip_group(
    const std::string& hdr_path, const std::string& ark_path,
    const std::vector<std::string>& milo_paths,
    const std::string& group_name) {
  CharClipGroup group;
  group.name = group_name;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    for (const auto& milo_path : milo_paths) {
      auto entry = ark.find(milo_path);
      if (!entry) entry = ark.find("../../system/run/" + milo_path);
      if (!entry) continue;
      auto bytes = ark.read_entry(*entry, {ark_path});
      auto hdr = gh::milo::parse_header(bytes);
      auto payload = gh::milo::inflate_payload(bytes, hdr);
      auto dir = gh::milo::parse_directory(payload);
      for (const auto& de : dir.entries) {
        if (de.type != "CharClipGroup" || de.name != group_name ||
            de.offset + de.size > payload.size()) {
          continue;
        }

        const uint8_t* body = payload.data() + de.offset;
        const size_t size = static_cast<size_t>(de.size);
        size_t pos = 0;
        if (size < 4) throw std::runtime_error("short CharClipGroup");
        uint32_t version = 0;
        std::memcpy(&version, body + pos, 4);
        pos += 4;
        if (version > 2) {
          throw std::runtime_error("unexpected CharClipGroup version");
        }

        // Source CharClipGroup::Load calls Hmx::Object::Load before mClips.
        if (pos + 4 > size) throw std::runtime_error("short object fields");
        pos += 4;  // Hmx::Object revision.
        (void)read_len_string(body, size, pos);  // Hmx::Object subtype symbol.
        if (pos >= size) throw std::runtime_error("short object tree terminator");
        ++pos;  // Hmx::Object empty property-tree terminator for stock rows.

        if (pos + 4 > size) throw std::runtime_error("short clip vector count");
        uint32_t count = 0;
        std::memcpy(&count, body + pos, 4);
        pos += 4;
        group.clips.clear();
        group.clips.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
          group.clips.push_back(read_len_string(body, size, pos));
        }

        if (pos + 4 > size) throw std::runtime_error("short CharClipGroup which");
        std::memcpy(&group.which, body + pos, 4);
        pos += 4;
        group.flags = 0;
        if (version > 1) {
          if (pos + 4 > size) throw std::runtime_error("short CharClipGroup flags");
          std::memcpy(&group.flags, body + pos, 4);
          pos += 4;
        }

        group.version = version;
        group.milo_path = milo_path;
        group.loaded = true;
        std::fprintf(stderr,
                     "[clip-group-source] group=%s milo=%s version=%u "
                     "clips=%zu which=%d flags=0x%08x\n",
                     group_name.c_str(), milo_path.c_str(), version,
                     group.clips.size(), group.which,
                     static_cast<unsigned>(group.flags));
        return group;
      }
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[clip-group-source] group=%s error=%s\n",
                 group_name.c_str(), ex.what());
  }
  return group;
}

std::optional<size_t> char_clip_group_get_clip_index(CharClipGroup& group) {
  if (group.clips.empty()) return std::nullopt;
  const int32_t before = group.which;
  ++group.which;
  if (group.which >= static_cast<int32_t>(group.clips.size())) {
    group.which = 0;
  }
  if (group.which < 0) group.which = 0;
  const size_t index = static_cast<size_t>(group.which);
  std::fprintf(stderr,
               "[clip-group-source-select] group=%s before=%d after=%d "
               "index=%zu clip=%s\n",
               group.name.c_str(), before, group.which, index,
               group.clips[index].c_str());
  return index;
}

std::vector<std::string> load_clip_group_names(
    const std::string& hdr_path, const std::string& ark_path,
    const std::vector<std::string>& milo_paths,
    const std::string& group_name) {
  return load_clip_group(hdr_path, ark_path, milo_paths, group_name).clips;
}

int source_char_clip_group_num_flag_duplicates(
    const std::vector<uint32_t>& clip_flags,
    size_t clip_index,
    uint32_t mask) {
  if (clip_index >= clip_flags.size()) return 0;
  const uint32_t flags = clip_flags[clip_index];
  int count = 0;
  for (size_t i = 0; i < clip_flags.size(); ++i) {
    if (i != clip_index && (mask & flags) == (mask & clip_flags[i])) {
      ++count;
    }
  }
  return count;
}

std::vector<std::string> source_char_clip_group_sorted_names(
    std::vector<std::string> clip_names) {
  std::sort(clip_names.begin(), clip_names.end());
  return clip_names;
}

std::vector<std::string> source_char_clip_group_add_clip(
    std::vector<std::string> clip_names,
    const std::string& clip_name) {
  if (std::find(clip_names.begin(), clip_names.end(), clip_name) ==
      clip_names.end()) {
    clip_names.push_back(clip_name);
  }
  return clip_names;
}

std::vector<std::string> source_char_clip_group_remove_clip(
    std::vector<std::string> clip_names,
    const std::string& clip_name) {
  for (size_t i = 0; i < clip_names.size(); ++i) {
    if (clip_names[i] == clip_name) {
      clip_names.erase(clip_names.begin() + static_cast<std::ptrdiff_t>(i));
    } else {
      ++i;
    }
  }
  return clip_names;
}

SourceCharClipGroupLoadPlan source_char_clip_group_load_plan(int revision) {
  SourceCharClipGroupLoadPlan plan;
  if (revision < 0 || revision > 2) return plan;
  plan.known_revision = true;
  plan.read_order = {"LOAD_REVS", "Hmx::Object", "mClips", "mWhich"};
  if (revision > 1) {
    plan.read_order.push_back("mFlags");
    plan.read_flags = true;
  } else {
    plan.default_flags = 0;
  }
  return plan;
}

SourceCharClipGroupHandlerPlan source_char_clip_group_handler_plan() {
  SourceCharClipGroupHandlerPlan plan;
  plan.handlers = {"get_clip:GetClip",
                   "delete_remaining:DeleteRemaining",
                   "get_size:mClips.size",
                   "has_clip:HasClip",
                   "find_clip:GetClip(int)",
                   "add_clip:AddClip",
                   "set_clip_flags:SetClipFlags",
                   "randomize_index:RandomizeIndex"};
  plan.superclasses = {"Hmx::Object"};
  plan.check = 0x179;
  return plan;
}

SourceCharClipGroupPropSyncPlan source_char_clip_group_prop_sync_plan() {
  SourceCharClipGroupPropSyncPlan plan;
  plan.properties = {"clips", "flags"};
  return plan;
}

SourceCharClipGroupSavePlan source_char_clip_group_save_plan() {
  return SourceCharClipGroupSavePlan{};
}

SourceCharClipGroupDeleteRemainingPlan
source_char_clip_group_delete_remaining_plan(size_t clip_count,
                                             int requested_remaining) {
  SourceCharClipGroupDeleteRemainingPlan plan;
  plan.requested_remaining = requested_remaining;
  plan.visited_clip_count = clip_count;
  return plan;
}

uint32_t source_char_clip_driver_masked_play_flags(uint32_t clip_play_flags,
                                                   uint32_t mask) {
  uint32_t play_flags = clip_play_flags;
  if (mask & 0xF0u) play_flags = (play_flags & 0xffffff0fu) | (mask & 0xF0u);
  if (mask & 0x0Fu) play_flags = (play_flags & 0xfffffff0u) | (mask & 0x0Fu);
  if (mask & 0xF600u) {
    play_flags = (play_flags & 0xffff09ffu) | (mask & 0xF600u);
  }
  return play_flags;
}

uint32_t char_clip_driver_masked_play_flags(const CharClip& clip,
                                            uint32_t mask) {
  return source_char_clip_driver_masked_play_flags(clip.default_play_flags,
                                                   mask);
}

float source_char_driver_evaluate_flags_from_clip_flags(uint32_t clip_flags,
                                                        uint32_t flags) {
  if (flags == 0) return 0.0f;
  return (clip_flags & flags) == flags ? 1.0f : 0.0f;
}

SourceCharClipDriverState source_char_clip_driver_construct(
    uint32_t clip_play_flags,
    bool has_clip,
    bool has_next,
    uint32_t mask,
    float blend_width,
    bool play_multiple_clips) {
  SourceCharClipDriverState state;
  state.play_flags =
      source_char_clip_driver_masked_play_flags(clip_play_flags, mask);
  state.blend_width = blend_width;
  state.time_scale = 1.0f;
  state.d_beat = 0.0f;
  state.advance_beat = 0.0f;
  state.has_clip = has_clip;
  state.has_next = has_next;
  state.next_event = -1;
  state.play_multiple_clips = play_multiple_clips;
  return state;
}

std::vector<size_t> source_char_clip_driver_delete_stack_order(
    size_t stack_size) {
  std::vector<size_t> deleted;
  for (size_t i = stack_size; i > 0; --i) {
    deleted.push_back(i - 1);
  }
  return deleted;
}

SourceCharClipDriverExitDecision source_char_clip_driver_exit_decision(
    size_t stack_size,
    bool exit_next,
    bool has_sync_anim) {
  SourceCharClipDriverExitDecision decision;
  if (stack_size == 0) return decision;
  decision.execute_exit_event = true;
  decision.end_sync_anim = has_sync_anim;
  decision.delete_self = true;
  if (exit_next && stack_size > 1) {
    decision.recurse_next = true;
    decision.deleted_indices =
        source_char_clip_driver_delete_stack_order(stack_size);
  } else {
    decision.deleted_indices.push_back(0);
    if (stack_size > 1) decision.returned_stack_head = 1;
  }
  return decision;
}

SourceCharClipDriverDeleteClipResult source_char_clip_driver_delete_clip_result(
    const std::vector<bool>& clip_matches_source_order) {
  SourceCharClipDriverDeleteClipResult result;
  for (size_t i = 0; i < clip_matches_source_order.size(); ++i) {
    if (clip_matches_source_order[i]) {
      result.deleted_index = i;
      break;
    }
  }
  for (size_t i = 0; i < clip_matches_source_order.size(); ++i) {
    if (!result.deleted_index || i != *result.deleted_index) {
      result.remaining_indices.push_back(i);
    }
  }
  return result;
}

bool source_char_clip_driver_should_execute_event(bool symbol_null,
                                                  bool clip_has_type_def) {
  return !symbol_null && clip_has_type_def;
}

SourceCharClipDriverRuntimeDumpEvidence
source_char_clip_driver_runtime_dump_evidence() {
  SourceCharClipDriverRuntimeDumpEvidence evidence;
  evidence.copy_ctor_range = "0x8032D060 -> 0x8032D168";
  evidence.destructor_range = "0x8032D168 -> 0x8032D1E8";
  evidence.exit_range = "0x8032D1E8 -> 0x8032D28C";
  evidence.delete_stack_range = "0x8032D28C -> 0x8032D2D4";
  evidence.delete_clip_range = "0x8032D2D4 -> 0x8032D33C";
  evidence.evaluate_range = "0x8032D33C -> 0x8032DA1C";
  evidence.scale_add_range = "0x8032DA1C -> 0x8032DB3C";
  evidence.rotate_to_range = "0x8032DB3C -> 0x8032DC90";
  evidence.align_to_frame_range = "0x8032DC90 -> 0x8032DDD0";
  evidence.play_events_range = "0x8032DDD0 -> 0x8032DFB4";
  evidence.execute_event_range = "0x8032DFB4 -> 0x8032E290";
  evidence.copy_ctor_references = {
      "__vt__33ObjOwnerPtr<8CharClip,9ObjectDir>"};
  evidence.destructor_references = {
      "__vt__33ObjOwnerPtr<8CharClip,9ObjectDir>"};
  evidence.exit_locals = {"CharClipDriver* next r31"};
  evidence.exit_references = {"static Symbol exit"};
  evidence.evaluate_locals = {"nextWeight", "rt",       "ut",
                              "rampDelta",  "oldFrame", "delta",
                              "dfrac",      "length",   "w"};
  evidence.evaluate_references = {"Debug TheDebug", "const char * kAssertStr"};
  evidence.scale_add_locals = {"w"};
  evidence.scale_add_references = {"Debug TheDebug", "const char * kAssertStr"};
  evidence.rotate_to_locals = {"w"};
  evidence.rotate_to_references = {"Debug TheDebug", "const char * kAssertStr"};
  evidence.align_to_frame_locals = {"alignBeat", "delta"};
  evidence.align_to_frame_references = {"Debug TheDebug",
                                        "const char * kAssertStr"};
  evidence.play_events_locals = {"frame"};
  evidence.play_events_references = {"Debug TheDebug",
                                     "const char * kAssertStr",
                                     "static DataNode& instant",
                                     "static Symbol enter"};
  evidence.execute_event_references = {"static Message h",
                                       "__vt__7Message",
                                       "static DataNode& dude",
                                       "Debug TheDebug",
                                       "const char * kAssertStr",
                                       "const char * gNullStr"};
  return evidence;
}

const char* source_char_clip_beat_align_string(uint32_t mask) {
  switch (mask & 0xF600u) {
    case kCharPlayRealTime:
      return "RealTime";
    case kCharPlayUserTime:
      return "UserTime";
    case 0x1000u:
      return "BeatAlign1";
    case 0x2000u:
      return "BeatAlign2";
    case 0x4000u:
      return "BeatAlign4";
    case 0x8000u:
      return "BeatAlign8";
    default:
      return "NoAlign";
  }
}

SourceCharClipFlagUpdate source_char_clip_set_flags(uint32_t current_flags,
                                                    bool current_dirty,
                                                    uint32_t requested_flags) {
  SourceCharClipFlagUpdate update;
  update.value = current_flags;
  update.dirty = current_dirty;
  if (requested_flags != current_flags) {
    update.value = requested_flags;
    update.dirty = true;
    update.changed = true;
  }
  return update;
}

SourceCharClipDefaultState source_char_clip_default_state() {
  return SourceCharClipDefaultState{};
}

SourceCharClipNumFramesPlan source_char_clip_num_frames_plan(
    int full_num_samples,
    int full_frame_count,
    int one_num_samples) {
  SourceCharClipNumFramesPlan plan;
  plan.full_num_samples = full_num_samples;
  plan.full_frame_count = full_frame_count;
  plan.one_num_samples = one_num_samples;
  plan.num_frames = std::max(std::max(1, full_num_samples), full_frame_count);
  return plan;
}

SourceCharClipBeatEvent source_char_clip_beat_event_default() {
  return SourceCharClipBeatEvent{};
}

SourceCharClipBeatEvent source_char_clip_beat_event_copy(
    const SourceCharClipBeatEvent& source) {
  return SourceCharClipBeatEvent{source.event, source.beat};
}

void source_char_clip_beat_event_assign(SourceCharClipBeatEvent& dest,
                                        const SourceCharClipBeatEvent& source) {
  dest.event = source.event;
  dest.beat = source.beat;
}

SourceCharClipBeatEvent source_char_clip_beat_event_loaded(
    const std::string& event,
    float beat) {
  SourceCharClipBeatEvent loaded;
  loaded.event = event;
  loaded.beat = beat;
  return loaded;
}

SourceCharClipPropSyncPlan source_char_clip_prop_sync_plan() {
  SourceCharClipPropSyncPlan plan;
  plan.graph_node_properties = {"cur_beat", "next_beat"};
  plan.node_vector_size_query = true;
  plan.node_vector_properties = {"clip", "nodes"};
  plan.beat_event_set_properties = {"beat", "event"};
  plan.clip_set_properties = {"start_beat",       "end_beat",
                              "length_beats",     "frames_per_sec",
                              "length_seconds",   "average_beats_per_sec",
                              "flags",            "default_blend",
                              "default_loop",     "beat_align",
                              "relative",         "dirty",
                              "size",             "compression",
                              "num_frames"};
  plan.clip_properties = {"range", "events", "do_not_compress",
                          "transitions", "sync_anim"};
  plan.sample_subobjects = {"full", "one"};
  return plan;
}

SourceCharClipResourceLookup source_char_clip_get_resource(
    bool has_type_def,
    bool has_resource_array,
    const std::string& resource_name,
    bool resource_found) {
  SourceCharClipResourceLookup lookup;
  lookup.has_type_def = has_type_def;
  lookup.has_resource_array = has_type_def && has_resource_array;
  if (lookup.has_resource_array) {
    lookup.resource_name = resource_name;
    lookup.found_resource = resource_found;
  }
  lookup.warn_no_resource = !lookup.found_resource;
  return lookup;
}

SourceCharClipContextLookup source_char_clip_get_context_lookup(
    bool has_type_def,
    bool has_resource_array,
    const std::string& macro_name,
    int resource_context) {
  SourceCharClipContextLookup lookup;
  lookup.has_type_def = has_type_def;
  lookup.has_resource_array = has_type_def && has_resource_array;
  if (lookup.has_resource_array) {
    lookup.macro_name = macro_name;
    lookup.context = resource_context;
    lookup.reads_macro = true;
  }
  return lookup;
}

int source_char_clip_get_context(bool has_type_def,
                                 bool has_resource_array,
                                 int resource_context) {
  return source_char_clip_get_context_lookup(has_type_def, has_resource_array,
                                             "", resource_context)
      .context;
}

SourceCharClipTransitionsState source_char_clip_transitions_construct(
    bool has_owner) {
  SourceCharClipTransitionsState state;
  state.has_owner = has_owner;
  return state;
}

size_t source_char_clip_transitions_size(
    const SourceCharClipTransitionsState& transitions) {
  return transitions.node_sizes.size();
}

SourceCharClipTransitionsClearResult source_char_clip_transitions_clear(
    SourceCharClipTransitionsState& transitions) {
  SourceCharClipTransitionsClearResult result;
  result.released_clips = source_char_clip_transitions_size(transitions);
  transitions.node_sizes.clear();
  result.resized_zero = true;
  return result;
}

SourceCharClipTransitionsDumpEvidence
source_char_clip_transitions_dump_evidence() {
  SourceCharClipTransitionsDumpEvidence evidence;
  evidence.remove_nodes_range = "0x803286D0 -> 0x80328774";
  evidence.resize_nodes_range = "0x80328774 -> 0x803288A4";
  evidence.add_node_range = "0x803288A4 -> 0x80328A1C";
  return evidence;
}

SourceCharClipRuntimeDumpEvidence source_char_clip_runtime_dump_evidence() {
  SourceCharClipRuntimeDumpEvidence evidence;
  evidence.find_nodes_range = "0x80328218 -> 0x80328258";
  evidence.find_first_node_range = "0x80328258 -> 0x803282D0";
  evidence.find_last_node_range = "0x803282D0 -> 0x80328348";
  evidence.find_node_range = "0x80328348 -> 0x80328564";
  evidence.replace_range = "0x80328564 -> 0x80328650";
  evidence.clear_all_nodes_range = "0x80328650 -> 0x803286D0";
  evidence.load_range = "0x80328D70 -> 0x803296AC";
  evidence.set_default_blend_range = "0x803298AC -> 0x803298DC";
  evidence.set_default_loop_range = "0x803298DC -> 0x8032990C";
  evidence.set_beat_align_mode_range = "0x8032990C -> 0x80329944";
  evidence.in_groups_range = "0x80329944 -> 0x803299FC";
  evidence.make_mru_range = "0x803299FC -> 0x80329B54";
  evidence.lock_and_delete_range = "0x80329B54 -> 0x80329C78";
  evidence.handle_range = "0x80329C78 -> 0x8032A470";
  evidence.on_groups_range = "0x8032A470 -> 0x8032A5DC";
  evidence.check_stick_range = "0x8032A5DC -> 0x8032A8B8";
  evidence.sync_property_range = "0x8032AA84 -> 0x8032B76C";
  evidence.find_nodes_locals = {"NodeVector* n"};
  evidence.find_first_node_locals = {"NodeVector* n", "int i"};
  evidence.find_last_node_locals = {"NodeVector* n", "int i"};
  evidence.find_node_locals = {"CharGraphNode* n", "float beatAlign",
                               "float endBorder", "float f"};
  evidence.load_locals = {"int num",
                          "int i",
                          "char name[256]",
                          "int numNodes",
                          "int j",
                          "CharGraphNode n",
                          "int maxSize",
                          "int num",
                          "NodeVector* start",
                          "NodeVector* n",
                          "int i",
                          "char name[256]",
                          "int j",
                          "int size",
                          "int j",
                          "CharGraphNode n",
                          "int num",
                          "String tmp",
                          "int i",
                          "int num",
                          "int i",
                          "String s",
                          "float lastFrame",
                          "int num",
                          "int i",
                          "float frame"};
  evidence.default_flag_setter_locals = {"int f"};
  evidence.in_groups_locals = {"int count", "_List_iterator i", "Object* o"};
  evidence.make_mru_locals = {"CharClipGroup* groups[256]", "int num",
                              "_List_iterator i", "Object* o",
                              "CharClipGroup* g"};
  evidence.lock_and_delete_locals = {"int i", "CharClip* c",
                                     "CharClip* c"};
  evidence.on_groups_locals = {"_List_iterator i", "Object* o",
                               "CharClipGroup* group"};
  evidence.check_stick_locals = {"RndTransformable* stick",
                                 "RndTransformable* arm",
                                 "CharBonesMeshes bones",
                                 "Vector3 stickDown",
                                 "Vector3 armDown",
                                 "float angle"};
  evidence.has_load_statement_body = false;
  evidence.has_default_flag_setter_statement_bodies = false;
  evidence.has_group_helper_statement_bodies = false;
  evidence.has_check_stick_statement_body = false;
  evidence.has_sync_property_statement_body = false;
  evidence.safe_to_import_load = false;
  evidence.safe_to_import_default_flag_setters = false;
  evidence.safe_to_import_group_helpers = false;
  evidence.safe_to_import_check_stick = false;
  evidence.safe_to_import_sync_property = false;
  return evidence;
}

std::vector<SourceCharBonesBone> source_char_clip_stuff_bones(
    const std::vector<SourceCharBonesBone>& existing_bones,
    const std::vector<SourceCharBonesBone>& listed_bones) {
  std::vector<SourceCharBonesBone> bones = existing_bones;
  bones.insert(bones.end(), listed_bones.begin(), listed_bones.end());
  return bones;
}

SourceCharClipPoseMeshesSteps source_char_clip_pose_meshes_steps(float frame) {
  SourceCharClipPoseMeshesSteps steps;
  steps.temp_meshes_name = "tmp_viseme_bones";
  steps.stuff_bones = true;
  steps.scale_down = true;
  steps.scale_down_weight = 0.0f;
  steps.scale_add = true;
  steps.scale_add_weight = 1.0f;
  steps.scale_add_frame = frame;
  steps.scale_add_blend = 0.0f;
  steps.pose_meshes = true;
  return steps;
}

SourceReleasePosePublisherBoundary
source_release_pose_publisher_boundary() {
  SourceReleasePosePublisherBoundary boundary;
  boundary.remaining_source_gap =
      "CharClipSamples / CharBonesSamples / CharBones / PoseMeshes publisher";
  boundary.source_evidence = {
      "CharClip::PoseMeshes builds tmp_viseme_bones from StuffBones",
      "CharClip::PoseMeshes calls ScaleDown then ScaleAdd before PoseMeshes",
      "CharBonesSamples::ScaleAddSample selects adjacent samples by mStart",
      "release-pose frame logs hand IK solveWeight zero",
      "frame 70 full CharBone output diagnostic changes the body pose",
  };
  boundary.rejected_shortcuts = {
      "character-specific shoulder, neck, or arm offsets",
      "IK or twist controller rewrites for zero-weight release frames",
      "default-on broad CharBone output without the source publisher body",
  };
  return boundary;
}

SourceCharPosePublisherSourceRefresh
source_char_pose_publisher_source_refresh_20260714() {
  SourceCharPosePublisherSourceRefresh refresh;
  refresh.rb3_commit = "41719f2";
  refresh.grim_commit = "1c05ca3";
  refresh.re_notes_commit = "5c486fd";
  refresh.rb3_after_fetch = true;
  refresh.grim_after_fetch = true;
  refresh.re_notes_after_fetch = true;
  refresh.char_clip_pose_meshes_body = true;
  refresh.char_bones_samples_scale_add_sample_body = true;
  refresh.char_bones_scale_add_body = false;
  refresh.char_bones_samples_evaluate_channel_body = false;
  refresh.char_bones_meshes_pose_meshes_statement_body = false;
  refresh.char_bones_meshes_latest_pose_meshes_stub_only = true;
  refresh.rb2_dump_is_range_local_map = true;
  refresh.still_fenced = {
      "CharBones::ScaleAdd",
      "CharBonesSamples::EvaluateChannel",
      "CharBonesMeshes::PoseMeshes statement body",
      "CharClipSamples::ScaleAdd",
      "CharClipDriver::Evaluate",
  };
  return refresh;
}

SourceCharDriverState source_char_driver_default_state() {
  return SourceCharDriverState{};
}

SourceCharDriverDestructorPlan source_char_driver_destructor_plan() {
  return SourceCharDriverDestructorPlan{};
}

SourceCharDriverExitPlan source_char_driver_exit_plan() {
  return SourceCharDriverExitPlan{};
}

SourceCharDriverHighlightDecision source_char_driver_highlight_decision(
    float char_highlight_y) {
  SourceCharDriverHighlightDecision decision;
  decision.global_y_is_sentinel = char_highlight_y == -1.0f;
  if (decision.global_y_is_sentinel) {
    decision.defer_highlight = true;
  } else {
    decision.call_display = true;
    decision.write_global_y_from_display = true;
    decision.display_input = char_highlight_y;
  }
  return decision;
}

void source_char_driver_clear(SourceCharDriverState& state) {
  state.has_first = false;
}

SourceCharDriverEnterDecision source_char_driver_enter(
    SourceCharDriverState& state) {
  SourceCharDriverEnterDecision decision;
  decision.changed = true;
  decision.clear_stack = true;
  decision.reset_last_node = true;
  decision.reset_old_beat = true;
  decision.reset_beat_scale = true;
  source_char_driver_clear(state);
  state.last_node_valid = false;
  state.old_beat = 1.0e30f;
  state.beat_scale = 1.0f;
  if (state.has_default_clip) {
    decision.play_default_clip = true;
    state.last_node_valid = true;
  }
  return decision;
}

SourceCharDriverTransferPlan source_char_driver_transfer_plan(
    bool source_has_first) {
  SourceCharDriverTransferPlan plan;
  plan.create_first_driver_copy = source_has_first;
  plan.copied_members = {"mClips", "mLastNode", "mRealign",
                         "mBeatScale", "mBlendWidth"};
  if (source_has_first) {
    plan.copied_members.push_back(
        "mFirst:new CharClipDriver(this,*driver.mFirst)");
  }
  plan.preserved_members = {"mBones",        "mTestClip",
                            "mDefaultClip",  "mDefaultPlayStarved",
                            "mStarvedHandler", "mOldBeat",
                            "mClipType",     "mApply",
                            "mInternalBones", "mPlayMultipleClips",
                            "unk89"};
  return plan;
}

void source_char_driver_transfer(SourceCharDriverState& state,
                                 const SourceCharDriverState& driver) {
  source_char_driver_clear(state);
  state.has_clips = driver.has_clips;
  state.last_node_valid = driver.last_node_valid;
  state.realign = driver.realign;
  state.beat_scale = driver.beat_scale;
  state.blend_width = driver.blend_width;
  state.has_first = driver.has_first;
}

void source_char_driver_set_clips(SourceCharDriverState& state,
                                  bool has_clips) {
  if (has_clips != state.has_clips) {
    state.last_node_valid = false;
    state.has_clips = has_clips;
  }
}

void source_char_driver_set_bones(SourceCharDriverState& state,
                                  bool has_bones) {
  state.has_bones = has_bones;
}

void source_char_driver_set_starved(SourceCharDriverState& state,
                                    const std::string& starved_handler) {
  state.starved_handler = starved_handler;
}

void source_char_driver_set_blend_width(SourceCharDriverState& state,
                                        float blend_width) {
  state.blend_width = blend_width;
}

SourceCharDriverSyncDecision source_char_driver_sync_internal_bones(
    SourceCharDriverState& state) {
  SourceCharDriverSyncDecision decision;
  decision.changed = true;
  decision.clear_stack = true;
  decision.reset_last_node = true;
  source_char_driver_clear(state);
  state.last_node_valid = false;
  if (state.has_internal_bones && state.clip_type.empty()) {
    decision.delete_internal_bones = true;
    state.has_internal_bones = false;
  } else if (!state.has_internal_bones &&
             state.apply == kSourceCharDriverApplyBlendWeights &&
             !state.clip_type.empty()) {
    decision.allocate_internal_bones = true;
    state.has_internal_bones = true;
  }
  if (state.has_internal_bones) {
    decision.clear_internal_bones = true;
    decision.stuff_internal_bones = true;
  }
  decision.has_internal_bones = state.has_internal_bones;
  return decision;
}

SourceCharDriverSyncDecision source_char_driver_set_apply(
    SourceCharDriverState& state,
    SourceCharDriverApplyMode apply) {
  if (apply == state.apply) return SourceCharDriverSyncDecision{};
  state.apply = apply;
  return source_char_driver_sync_internal_bones(state);
}

SourceCharDriverSyncDecision source_char_driver_set_clip_type(
    SourceCharDriverState& state,
    const std::string& clip_type) {
  if (clip_type == state.clip_type) return SourceCharDriverSyncDecision{};
  state.clip_type = clip_type;
  return source_char_driver_sync_internal_bones(state);
}

SourceCharDriverPlayGroupDecision source_char_driver_play_group_decision(
    bool has_clip_dir,
    bool found_group) {
  SourceCharDriverPlayGroupDecision decision;
  decision.has_clip_dir = has_clip_dir;
  decision.found_group = found_group;
  if (!has_clip_dir) {
    decision.warn_no_clips = true;
    return decision;
  }
  if (!found_group) {
    decision.warn_missing_group = true;
    return decision;
  }
  decision.call_group_get_clip = true;
  decision.request_play = true;
  return decision;
}

SourceCharDriverRuntimeDumpEvidence
source_char_driver_runtime_dump_evidence() {
  SourceCharDriverRuntimeDumpEvidence evidence;
  evidence.play_if_safe_range = "0x8034D8A4 -> 0x8034DB54";
  evidence.set_beat_scale_range = "0x8034DBB4 -> 0x8034DC4C";
  evidence.evaluate_flags_range = "0x8034DC4C -> 0x8034DD64";
  evidence.last_range = "0x8034DD64 -> 0x8034DD88";
  evidence.before_range = "0x8034DD88 -> 0x8034DDAC";
  evidence.most_playing_range = "0x8034DDD4 -> 0x8034DF00";
  evidence.pre_load_range = "0x8034E0E0 -> 0x8034ED68";
  evidence.post_load_range = "0x8034ED68 -> 0x8034F008";
  evidence.play_if_safe_locals = {"d", "FindRestrictLength", "s"};
  evidence.play_if_safe_references = {"TheDebug", "kAssertStr"};
  evidence.set_beat_scale_locals = {"fp", "invScale", "cd"};
  evidence.set_beat_scale_references = {};
  evidence.evaluate_flags_locals = {"weight", "flagWeight", "cd", "w"};
  evidence.evaluate_flags_references = {"TheDebug", "kAssertStr"};
  evidence.last_locals = {"cd"};
  evidence.before_locals = {"cd"};
  evidence.most_playing_locals = {"maxWeight", "best", "weight", "cd", "w"};
  evidence.most_playing_references = {"TheDebug", "kAssertStr"};
  evidence.pre_load_locals = {"tmp", "p"};
  evidence.pre_load_references = {
      "__vt__8FilePath",
      "__RTTI__6Loader",
      "__RTTI__9DirLoader",
      "msg",
      "__vt__7Message",
      "__RTTI__Q23Hmx6Object",
      "__vt__32ObjPtr<11CharClipSet,9ObjectDir>",
      "__RTTI__9ObjectDir",
      "__RTTI__11CharClipSet",
      "TheLoadMgr",
      "sRoot",
      "sClipsPath",
      "TheDebug",
      "gRev"};
  evidence.post_load_references = {"__RTTI__Q23Hmx6Object",
                                   "__RTTI__8CharClip",
                                   "gRev",
                                   "__RTTI__9ObjectDir",
                                   "__RTTI__11CharClipSet",
                                   "TheLoadMgr"};
  evidence.header_declarations_without_checked_bodies = {
      "Handle",       "SyncProperty", "Save",      "Copy",
      "Load",         "Poll",         "Replace",   "EvaluateFlags",
      "Display",      "FindClip",     "FirstClip", "FirstPlayingClip"};
  return evidence;
}

void source_char_driver_poll_deps(SourceCharDriverPollDeps& deps,
                                  const std::string& bones) {
  deps.change.push_back(bones);
}

SourceCharDriverMidiState source_char_driver_midi_default_state() {
  return SourceCharDriverMidiState{};
}

SourceCharDriverMidiEnterDecision source_char_driver_midi_enter(
    SourceCharDriverState& driver_state,
    SourceCharDriverMidiState& midi_state,
    bool parser_found,
    bool flag_parser_found) {
  SourceCharDriverMidiEnterDecision decision;
  midi_state.unk89 = true;
  decision.set_unk89 = true;
  decision.driver_enter = source_char_driver_enter(driver_state);
  decision.add_parser_sink = parser_found;
  decision.add_flag_parser_sink = flag_parser_found;
  return decision;
}

SourceCharDriverMidiExitDecision source_char_driver_midi_exit(
    bool parser_found,
    bool flag_parser_found) {
  SourceCharDriverMidiExitDecision decision;
  decision.call_driver_exit = true;
  decision.remove_parser_sink = parser_found;
  decision.remove_flag_parser_sink = flag_parser_found;
  return decision;
}

SourceCharDriverMidiPollPlan source_char_driver_midi_poll_plan() {
  SourceCharDriverMidiPollPlan plan;
  plan.call_driver_poll = true;
  plan.call_driver_poll_deps = true;
  return plan;
}

void source_char_driver_midi_poll_deps(SourceCharDriverPollDeps& deps,
                                       const std::string& bones) {
  source_char_driver_poll_deps(deps, bones);
}

void source_char_driver_midi_on_parser_flags(
    SourceCharDriverMidiState& midi_state,
    int clip_flags) {
  midi_state.clip_flags = clip_flags;
}

SourceCharDriverMidiParserDecision source_char_driver_midi_on_parser(
    const SourceCharDriverMidiState& midi_state,
    bool found_clip,
    bool clip_uses_real_time,
    float message_float,
    float beat_to_seconds_message_plus_current,
    float task_seconds,
    float average_beats_per_second) {
  SourceCharDriverMidiParserDecision decision;
  decision.used_default_clip = !midi_state.unk89 && midi_state.has_default_clip;
  if (!decision.used_default_clip && !found_clip) return decision;
  float blend = message_float;
  if (clip_uses_real_time) {
    blend = (beat_to_seconds_message_plus_current - task_seconds) *
            average_beats_per_second;
  }
  blend = std::max(blend, 0.0f);
  decision.request_play = true;
  decision.play_flags = 0;
  decision.requested_blend_width = blend * midi_state.blend_override_pct;
  decision.old_beat = -blend;
  decision.start = 0.0f;
  return decision;
}

SourceCharDriverMidiParserDecision source_char_driver_midi_on_parser_group(
    const SourceCharDriverMidiState& midi_state,
    bool found_group,
    bool found_group_clip,
    bool clip_uses_real_time,
    float message_float,
    float average_beats_per_second) {
  SourceCharDriverMidiParserDecision decision;
  if (!found_group) return decision;
  decision.used_default_clip = !midi_state.unk89 && midi_state.has_default_clip;
  if (!decision.used_default_clip) {
    decision.call_group_get_clip = true;
    decision.group_clip_flags = midi_state.clip_flags;
  }
  if (!decision.used_default_clip && !found_group_clip) return decision;
  float blend = message_float;
  if (clip_uses_real_time) blend *= average_beats_per_second;
  blend = std::max(blend, 0.0f);
  decision.request_play = true;
  decision.play_flags = 0;
  decision.requested_blend_width = -blend;
  decision.old_beat = 1.0e30f;
  decision.start = 0.0f;
  decision.assigned_blend_width = blend * midi_state.blend_override_pct;
  return decision;
}

SourceCharDriverMidiLoadPlan source_char_driver_midi_load_plan(int revision) {
  SourceCharDriverMidiLoadPlan plan;
  plan.known_revision = revision >= 0 && revision <= 7;
  if (!plan.known_revision) return plan;

  plan.read_order = {"LOAD_REVS", "CharDriver"};
  if (revision < 7) {
    plan.read_order.push_back("mDefaultClip.Load(false,mClips)");
  }
  if (revision == 2) {
    plan.read_order.push_back("legacyString");
  } else if (revision > 3) {
    plan.read_order.push_back("mParser");
  }
  if (revision > 4) plan.read_order.push_back("mFlagParser");
  if (revision > 5) plan.read_order.push_back("mBlendOverridePct");
  return plan;
}

SourceCharDriverMidiHandlerPlan source_char_driver_midi_handler_plan() {
  SourceCharDriverMidiHandlerPlan plan;
  plan.handlers = {"midi_parser:OnMidiParser",
                   "midi_parser_group:OnMidiParserGroup",
                   "midi_parser_flags:OnMidiParserFlags"};
  plan.superclasses = {"CharDriver"};
  plan.check = 0x99;
  return plan;
}

SourceCharDriverMidiPropSyncPlan source_char_driver_midi_prop_sync_plan() {
  SourceCharDriverMidiPropSyncPlan plan;
  plan.properties = {"parser", "flag_parser", "blend_override_pct"};
  plan.superclasses = {"CharDriver"};
  return plan;
}

SourceCharDriverMidiCopyPlan source_char_driver_midi_copy_plan() {
  SourceCharDriverMidiCopyPlan plan;
  plan.copied_superclasses = {"CharDriver"};
  plan.copied_members = {"unk89", "mParser", "mFlagParser",
                         "mBlendOverridePct"};
  plan.not_in_source_copy_members = {"mClipFlags"};
  return plan;
}

SourceCharDriverMidiSavePlan source_char_driver_midi_save_plan() {
  return SourceCharDriverMidiSavePlan{};
}

SourceCharClipSetState source_char_clip_set_default_state() {
  SourceCharClipSetState state;
  source_char_clip_set_reset_preview_state(state);
  state.rate_is_1_fpb = true;
  return state;
}

void source_char_clip_set_reset_preview_state(
    SourceCharClipSetState& state) {
  state.char_file_root.clear();
  state.has_preview_char = false;
  state.has_preview_clip = false;
  state.has_still_clip = false;
  state.filter_flags = 0;
  state.bpm = 90;
  state.preview_walk = false;
}

SourceCharClipSetResetEditorResult source_char_clip_set_reset_editor_state(
    SourceCharClipSetState& state) {
  SourceCharClipSetResetEditorResult result;
  source_char_clip_set_reset_preview_state(state);
  result.reset_preview_state = true;
  result.object_dir_reset_editor_state = true;
  return result;
}

std::vector<SourceCharClipSetGroupStep> source_char_clip_set_randomize_groups(
    const std::vector<std::string>& groups) {
  std::vector<SourceCharClipSetGroupStep> steps;
  steps.reserve(groups.size());
  for (const std::string& group : groups) {
    steps.push_back({group, true, false});
  }
  return steps;
}

std::vector<SourceCharClipSetGroupStep> source_char_clip_set_sort_groups(
    const std::vector<std::string>& groups) {
  std::vector<SourceCharClipSetGroupStep> steps;
  steps.reserve(groups.size());
  for (const std::string& group : groups) {
    steps.push_back({group, false, true});
  }
  return steps;
}

SourceCharClipSetPreSaveResult source_char_clip_set_pre_save(
    SourceCharClipSetState& state,
    bool cached_stream) {
  SourceCharClipSetPreSaveResult result;
  result.preview_char_name_cleared = state.has_preview_char;
  if (cached_stream) {
    source_char_clip_set_reset_preview_state(state);
    result.reset_preview_state = true;
    const SourceCharClipSetResetEditorResult editor =
        source_char_clip_set_reset_editor_state(state);
    result.reset_editor_state = editor.object_dir_reset_editor_state;
  }
  return result;
}

SourceCharClipSetPostSaveResult source_char_clip_set_post_save(
    const SourceCharClipSetState& state,
    bool milo_found) {
  SourceCharClipSetPostSaveResult result;
  result.object_dir_post_save = true;
  if (state.has_preview_char) {
    result.preview_char_name_restored = true;
    result.preview_char_entered = true;
    result.sent_update_objects = milo_found;
  }
  return result;
}

SourceCharClipSetPreLoadPlan source_char_clip_set_pre_load_plan() {
  return SourceCharClipSetPreLoadPlan{};
}

SourceCharClipSetLoadPlan source_char_clip_set_load_plan() {
  return SourceCharClipSetLoadPlan{};
}

SourceCharClipSetPostLoadPlan source_char_clip_set_post_load_plan(
    int32_t revision,
    bool is_proxy,
    int32_t clip_count,
    bool type_null) {
  SourceCharClipSetPostLoadPlan plan;
  if (is_proxy) {
    plan.returned_for_proxy = true;
    return plan;
  }
  plan.read_two_legacy_ints = revision < 0x11;
  plan.read_rev_15_16_int = revision == 0x0F || revision == 0x10;
  plan.read_legacy_graph_path = revision < 9;
  plan.read_legacy_reexport_string = revision < 6;
  plan.read_rev_lt7_int = revision < 7;
  plan.read_legacy_clip_triplets =
      revision < 0x18 ? std::max(clip_count, 0) : 0;
  if (revision > 0x0D) {
    if (revision < 0x18) {
      plan.read_old_flag_bool = true;
      plan.read_old_flag_second_bool = revision > 0x12;
    }
  } else {
    plan.read_symbol_count = true;
  }
  plan.read_legacy_string_lists = revision >= 5 && revision <= 0x17;
  plan.read_legacy_symbol_and_int = revision >= 10 && revision <= 23;
  plan.read_rev_11_bool = revision == 0x0B;
  plan.warn_transition_bug = revision < 0x0C && !type_null;
  plan.handle_filter_clips = revision < 0x0D;
  plan.read_char_file_path = revision > 0x11;
  plan.read_preview_clip = revision > 0x11;
  plan.read_filter_flags = revision > 0x13;
  plan.read_bpm = revision > 0x14;
  plan.read_preview_walk = revision > 0x15;
  plan.read_still_clip = revision > 0x16;
  return plan;
}

SourceCharClipSetCopyResult source_char_clip_set_copy(
    SourceCharClipSetState& dest,
    const SourceCharClipSetState& source) {
  SourceCharClipSetCopyResult result;
  result.copy_object_dir = true;
  dest.char_file_root = source.char_file_root;
  result.copy_char_file_path = true;
  dest.has_preview_clip = source.has_preview_clip;
  result.copy_preview_clip = true;
  dest.filter_flags = source.filter_flags;
  result.copy_filter_flags = true;
  dest.bpm = source.bpm;
  result.copy_bpm = true;
  dest.preview_walk = source.preview_walk;
  result.copy_preview_walk = true;
  dest.has_still_clip = source.has_still_clip;
  result.copy_still_clip = true;
  return result;
}

SourceCharClipSetLoadCharacterResult source_char_clip_set_load_character(
    SourceCharClipSetState& state,
    bool edit_mode,
    bool loaded_is_rnd_dir,
    bool loaded_is_character,
    bool nested_character_found,
    bool milo_found) {
  SourceCharClipSetLoadCharacterResult result;
  result.asserted_edit_mode = edit_mode;
  result.deleted_preview_char = true;
  state.has_preview_char = false;
  if (!edit_mode) return result;

  result.loaded_objects = true;
  result.loaded_rnd_dir = loaded_is_rnd_dir;
  if (loaded_is_rnd_dir) {
    state.has_preview_char = true;
    if (!loaded_is_character && nested_character_found) {
      result.selected_nested_character = true;
    }
  }
  if (state.has_preview_char) {
    result.preview_char_entered = true;
    result.preview_char_named = true;
    result.sent_update_objects = milo_found;
  }
  return result;
}

bool source_char_clip_set_draw_showing(bool has_preview_char) {
  return has_preview_char;
}

float source_char_clip_set_start_frame(bool has_preview_clip,
                                       float preview_clip_start_beat) {
  return has_preview_clip ? preview_clip_start_beat : 0.0f;
}

float source_char_clip_set_end_frame(bool has_preview_clip,
                                     float preview_clip_end_beat) {
  return has_preview_clip ? preview_clip_end_beat : 0.0f;
}

SourceCharClipSetSetBpmResult source_char_clip_set_set_bpm(
    SourceCharClipSetState& state,
    int bpm,
    bool milo_found) {
  state.bpm = bpm;
  return {milo_found, bpm};
}

const char* source_char_clip_set_recenter_all_warning() {
  return "You can only recenter clips from PC";
}

SourceCharClipSetHandlerPlan source_char_clip_set_handler_plan() {
  SourceCharClipSetHandlerPlan plan;
  plan.action_handlers = {"randomize_groups", "sort_groups", "recenter_all",
                          "load_character"};
  plan.handlers = {"list_clips"};
  plan.superclasses = {"ObjectDir"};
  plan.check = "0x2F0";
  return plan;
}

SourceCharClipSetSavePlan source_char_clip_set_save_plan() {
  return SourceCharClipSetSavePlan{};
}

void source_char_clip_display_init(SourceCharClipDisplayGlobals& globals,
                                   const std::string& dir,
                                   float draw_empty_y) {
  globals.dir = dir;
  globals.em = draw_empty_y;
}

SourceCharClipDisplayFindSourceResult source_char_clip_display_find_source(
    const std::vector<SourceCharClipDisplayMsgSource>& sources,
    const std::string& object) {
  SourceCharClipDisplayFindSourceResult result;
  for (const SourceCharClipDisplayMsgSource& source : sources) {
    for (const std::string& sink : source.sinks) {
      if (sink == object) {
        result.found = true;
        result.source = source.source;
        return result;
      }
    }
  }
  return result;
}

void source_char_clip_display_set_text(
    SourceCharClipDisplayState& state,
    const SourceCharClipDisplayGlobals& globals,
    const std::string& text,
    float draw_text_x) {
  state.text = text;
  state.text_width_plus_em = draw_text_x + globals.em;
}

void source_char_clip_display_set_start_end(
    SourceCharClipDisplayState& state,
    float start_beat,
    float end_beat,
    bool flag) {
  state.start_beat = start_beat;
  state.end_beat = end_beat;
  state.start_end_called = true;
  state.start_end_flag = flag;
}

void source_char_clip_display_set_clip(
    SourceCharClipDisplayState& state,
    const SourceCharClipDisplayGlobals& globals,
    const std::string& clip_name,
    float start_beat,
    float end_beat,
    bool flag,
    float draw_text_x) {
  state.clip = clip_name;
  source_char_clip_display_set_text(state, globals, clip_name, draw_text_x);
  source_char_clip_display_set_start_end(state, start_beat, end_beat, flag);
}

float source_char_clip_display_line_spacing(
    const SourceCharClipDisplayGlobals& globals) {
  return globals.em * 2.0f;
}

SourceCharTaskMgrState source_char_task_mgr_default_state() {
  return SourceCharTaskMgrState{};
}

void source_char_task_mgr_init(SourceCharTaskMgrState& state) {
  state.registered_toggle_char_task_graph = true;
}

bool source_char_task_mgr_toggle_graph(SourceCharTaskMgrState& state) {
  state.show_graph = !state.show_graph;
  return state.show_graph;
}

SourceClipGraphGeneratePairStep source_clip_graph_generate_pair_step(
    bool has_type_def,
    bool same_type,
    uint32_t clip_a_play_flags,
    bool has_on_transition,
    bool script_creates_dmap) {
  SourceClipGraphGeneratePairStep step;
  bool skip_transition_generation = true;
  bool type_pair_allowed = true;
  if (has_type_def && same_type) {
    type_pair_allowed = false;
  }
  if (!type_pair_allowed && ((clip_a_play_flags & 0xF0u) != 0x10u)) {
    skip_transition_generation = false;
  }
  if (skip_transition_generation) {
    step.return_null_before_script = true;
    step.reason = "source-return-null-before-script";
    return step;
  }
  if (!has_on_transition) {
    step.return_null_before_script = true;
    step.reason = "source-missing-on-transition";
    return step;
  }
  step.execute_on_transition = true;
  step.set_data_variables = true;
  step.stores_clip_pair = true;
  step.clears_dmap_before_script = true;
  step.clears_dmap_after_script = true;
  step.returns_dmap = script_creates_dmap;
  step.set_nodes = script_creates_dmap;
  step.reason = script_creates_dmap ? "source-script-created-dmap"
                                    : "source-script-left-dmap-null";
  return step;
}

SourceClipGraphTransitionPlan source_clip_graph_on_generate_transitions(
    const SourceClipGraphTransitionInputs& inputs) {
  constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
  SourceClipGraphTransitionPlan plan;
  plan.clip_a_flag = static_cast<int>((inputs.clip_a_play_flags >> 12) & 15u);
  plan.clip_b_flag = static_cast<int>((inputs.clip_b_play_flags >> 12) & 15u);
  plan.min_flag = std::min(plan.clip_a_flag, plan.clip_b_flag);
  plan.beat_align = std::max(inputs.beat_align,
                             static_cast<float>(plan.min_flag));
  plan.blend_width = inputs.blend_width;
  plan.has_restrict = inputs.has_restrict;
  plan.has_bone_weights = inputs.has_bone_weights;
  plan.find_dists_max_facing_radians = inputs.max_facing_degrees * kDegToRad;
  plan.find_nodes_max_error = inputs.max_error;
  plan.find_nodes_max_dist = inputs.max_dist;
  plan.find_nodes_end_dist = inputs.end_dist;
  return plan;
}

SourceClipCollideState source_clip_collide_default_state() {
  return SourceClipCollideState{};
}

bool source_clip_collide_load_revision_known(int revision) {
  return revision >= 0 && revision <= 1;
}

SourceClipCollideLoadPlan source_clip_collide_load_plan(int revision) {
  SourceClipCollideLoadPlan plan;
  plan.known_revision = source_clip_collide_load_revision_known(revision);
  if (!plan.known_revision) return plan;
  plan.read_order = {"Hmx::Object", "mChar", "mCharPath", "mWaypoint",
                     "mPosition"};
  plan.clears_clip = true;
  return plan;
}

SourceClipCollideSyncCharStep source_clip_collide_sync_char_step(
    bool has_character,
    bool char_path_empty,
    bool path_matches_proxy) {
  SourceClipCollideSyncCharStep step;
  step.set_proxy_file = has_character && !char_path_empty && !path_matches_proxy;
  return step;
}

SourceClipCollideSetTypeDefStep source_clip_collide_set_type_def_step(
    bool type_def_changed,
    bool has_type_def) {
  SourceClipCollideSetTypeDefStep step;
  if (!type_def_changed) return step;
  step.call_object_set_type_def = true;
  step.update_mode = has_type_def;
  step.assert_modes_array = has_type_def;
  return step;
}

SourceClipCollideValidationStep source_clip_collide_valid_waypoint(
    bool handler_unhandled,
    bool handler_value) {
  SourceClipCollideValidationStep step;
  step.send_message = true;
  step.message = "valid_waypoint";
  step.valid = handler_unhandled ? true : handler_value;
  return step;
}

SourceClipCollideValidationStep source_clip_collide_valid_clip(
    bool has_waypoint,
    bool handler_unhandled,
    bool handler_value) {
  SourceClipCollideValidationStep step;
  if (!has_waypoint) return step;
  step.send_message = true;
  step.message = "valid_clip";
  step.valid = handler_unhandled ? true : handler_value;
  return step;
}

SourceClipCollideDemonstrateStep source_clip_collide_demonstrate_step(
    bool has_character,
    bool has_waypoint,
    bool has_clip) {
  SourceClipCollideDemonstrateStep step;
  if (has_character && has_waypoint && has_clip) {
    step.sync_waypoint = true;
    step.play_clip = true;
  }
  return step;
}

SourceClipCollideClearReportStep source_clip_collide_clear_report_step() {
  return SourceClipCollideClearReportStep{};
}

SourceClipCollideSyncModeStep source_clip_collide_sync_mode_step(
    bool mode_null) {
  SourceClipCollideSyncModeStep step;
  step.send_set_mode = !mode_null;
  return step;
}

SourceClipCollideListPlan source_clip_collide_list_objects_plan(
    const std::vector<std::string>& valid_objects) {
  SourceClipCollideListPlan plan;
  plan.source_array_size = valid_objects.size();
  plan.items = valid_objects;
  return plan;
}

SourceClipCollideListPlan source_clip_collide_list_report_plan(
    const std::vector<std::string>& reports) {
  SourceClipCollideListPlan plan;
  plan.source_array_size = reports.size() + 1u;
  plan.items = reports;
  return plan;
}

SourceClipCollideTestClipsPlan source_clip_collide_test_clips_plan(
    size_t valid_clip_count) {
  SourceClipCollideTestClipsPlan plan;
  plan.directions = {"front", "back", "left", "right"};
  plan.collide_calls = valid_clip_count * plan.directions.size();
  return plan;
}

SourceClipCollideTestWaypointsPlan source_clip_collide_test_waypoints_plan(
    bool has_character,
    size_t valid_waypoint_count) {
  SourceClipCollideTestWaypointsPlan plan;
  if (!has_character) return plan;
  plan.valid_waypoint_count = valid_waypoint_count;
  plan.waypoint_assignments = valid_waypoint_count;
  plan.test_clips_calls = valid_waypoint_count;
  return plan;
}

SourceClipCollideTestCharsPlan source_clip_collide_test_chars_plan(
    bool has_character,
    bool has_type_def,
    bool has_chars_array,
    const std::vector<std::string>& char_paths) {
  SourceClipCollideTestCharsPlan plan;
  if (!has_character || !has_type_def || !has_chars_array) return plan;
  for (const std::string& char_path : char_paths) {
    if (char_path.empty()) continue;
    plan.tested_char_paths.push_back(char_path);
  }
  plan.sync_char_calls = plan.tested_char_paths.size();
  plan.test_waypoints_calls = plan.tested_char_paths.size();
  return plan;
}

SourceClipCollideHandlerPlan source_clip_collide_handler_plan() {
  SourceClipCollideHandlerPlan plan;
  plan.handlers = {"list_clips", "list_waypoints", "list_report",
                   "venue_name"};
  plan.action_handlers = {"demonstrate",    "collide",       "test_clips",
                          "test_waypoints", "test_chars",    "clear_report"};
  plan.superclasses = {"Hmx::Object"};
  return plan;
}

SourceClipCollidePropSyncPlan source_clip_collide_prop_sync_plan() {
  SourceClipCollidePropSyncPlan plan;
  plan.rows = {{"character", "mChar", "SyncChar", false},
               {"pick_character", "mCharPath", "SyncChar", false},
               {"waypoint", "mWaypoint", "SyncWaypoint", false},
               {"position", "mPosition", "SyncWaypoint", false},
               {"mode", "mMode", "SyncMode", false},
               {"clip", "mClip", "", false},
               {"clips", "Clips()", "", true},
               {"pick_report", "mReportString", "PickReport", true},
               {"world_lines", "mWorldLines", "", false},
               {"move_camera", "mMoveCamera", "", false}};
  return plan;
}

SourceClipCollideSavePlan source_clip_collide_save_plan() {
  return SourceClipCollideSavePlan{};
}

SourceFileMergerState source_file_merger_default_state() {
  return SourceFileMergerState{};
}

SourceFileMergerMergerState source_file_merger_merger_default_state() {
  return SourceFileMergerMergerState{};
}

SourceFileMergerCopyPlan source_file_merger_merger_copy_plan() {
  SourceFileMergerCopyPlan plan;
  plan.copied_members = {"mName",        "mSelected",      "unk10",
                         "mLoaded",      "mDir",           "mProxy",
                         "mSubdirs",     "mLoadedObjects", "mLoadedSubdirs",
                         "mPreClear"};
  return plan;
}

SourceClipCompressorEvidence source_clip_compressor_evidence() {
  SourceClipCompressorEvidence evidence;
  evidence.observed_function = "unusedclipcompressor";
  evidence.format_string = "%s %f %f";
  return evidence;
}

SourceCharClipFlagUpdate source_char_clip_set_play_flags(
    uint32_t current_play_flags,
    bool current_dirty,
    uint32_t requested_play_flags) {
  SourceCharClipFlagUpdate update;
  update.value = current_play_flags;
  update.dirty = current_dirty;
  if (requested_play_flags != current_play_flags) {
    update.value = requested_play_flags;
    update.dirty = true;
    update.changed = true;
  }
  return update;
}

bool source_char_clip_shares_groups(
    const std::vector<SourceCharClipRefOwner>& ref_owners,
    const std::string& candidate_clip_name) {
  for (auto it = ref_owners.rbegin(); it != ref_owners.rend(); ++it) {
    if (!it->is_clip_group) continue;
    if (std::find(it->group_clips.begin(), it->group_clips.end(),
                  candidate_clip_name) != it->group_clips.end()) {
      return true;
    }
  }
  return false;
}

bool source_char_driver_starved(bool has_first, bool first_has_next,
                                uint32_t first_play_flags) {
  if (has_first) {
    if (first_has_next) return false;
    if ((first_play_flags & 0xF0u) == kCharPlayNoLoop) return false;
  }
  return true;
}

float source_char_driver_resolve_blend_width(float requested_blend_width,
                                             float driver_blend_width) {
  return requested_blend_width == -1.0f ? driver_blend_width
                                        : requested_blend_width;
}

bool source_char_driver_should_start_clip(bool play_multiple_clips,
                                          bool clip_already_playing) {
  if (play_multiple_clips && clip_already_playing) return false;
  return true;
}

SourceCharDriverPlayDecision source_char_driver_play_decision(
    SourceCharDriverState& state,
    bool found_clip,
    bool clip_already_playing,
    int play_flags,
    float requested_blend_width,
    float old_beat,
    float start) {
  SourceCharDriverPlayDecision decision;
  decision.found_clip = found_clip;
  decision.play_flags = play_flags;
  decision.old_beat = old_beat;
  decision.start = start;
  decision.play_multiple_clips = state.play_multiple_clips;
  if (!found_clip) {
    decision.notify_missing_clip = true;
    return decision;
  }

  state.last_node_valid = true;
  decision.set_last_node = true;
  decision.resolved_blend_width =
      source_char_driver_resolve_blend_width(requested_blend_width,
                                             state.blend_width);
  if (!source_char_driver_should_start_clip(state.play_multiple_clips,
                                            clip_already_playing)) {
    decision.duplicate_clip = true;
    return decision;
  }

  state.has_first = true;
  decision.create_clip_driver = true;
  decision.new_stack_head = true;
  return decision;
}

SourceCharDriverPlayNodeDecision source_char_driver_play_node_decision(
    SourceCharDriverState& state,
    bool find_clip_succeeds,
    bool clip_already_playing,
    int play_flags,
    float requested_blend_width,
    float old_beat,
    float start) {
  SourceCharDriverPlayNodeDecision decision;
  decision.clip_play = source_char_driver_play_decision(
      state, find_clip_succeeds, clip_already_playing, play_flags,
      requested_blend_width, old_beat, start);
  state.last_node_valid = true;
  decision.final_last_node_from_request = true;
  decision.returned_driver = decision.clip_play.create_clip_driver;
  return decision;
}

std::optional<size_t> source_char_driver_first_playing_index(
    const std::vector<float>& source_stack_blend_fracs) {
  for (size_t i = 0; i < source_stack_blend_fracs.size(); ++i) {
    if (source_stack_blend_fracs[i] != 0.0f) return i;
  }
  return std::nullopt;
}

// ---- pose application ----------------------------------------------------

static void quat_to_rot(const float q[4], float rot[3][3]) {
  float x = q[0], y = q[1], z = q[2], w = q[3];
  float len2 = x*x + y*y + z*z + w*w;
  if (len2 > 1e-8f) { float inv = 1.0f / std::sqrt(len2); x*=inv; y*=inv; z*=inv; w*=inv; }
  float m[3][3];
  m[0][0] = 1 - 2*(y*y + z*z);  m[0][1] = 2*(x*y + z*w);      m[0][2] = 2*(x*z - y*w);
  m[1][0] = 2*(x*y - z*w);      m[1][1] = 1 - 2*(x*x + z*z);  m[1][2] = 2*(y*z + x*w);
  m[2][0] = 2*(x*z + y*w);      m[2][1] = 2*(y*z - x*w);      m[2][2] = 1 - 2*(x*x + y*y);
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      rot[r][c] = m[r][c];
}

static bool channel_matches_bone(const std::string& bone_name,
                                 const std::string& channel_bone_name) {
  auto equivalent = [](std::string a, std::string b) {
    auto strip_mesh = [](std::string& s) {
      constexpr std::string_view suffix = ".mesh";
      if (s.size() >= suffix.size() &&
          s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0) {
        s.resize(s.size() - suffix.size());
      }
    };
    strip_mesh(a);
    strip_mesh(b);
    auto replace_once = [](std::string& s, std::string_view from,
                           std::string_view to) {
      const size_t p = s.find(from);
      if (p != std::string::npos) s.replace(p, from.size(), to);
    };
    std::string a_alias = a;
    std::string b_alias = b;
    replace_once(a_alias, "-toe0", "-toe");
    replace_once(b_alias, "-toe0", "-toe");
    return a_alias == b_alias;
  };
  if (equivalent(bone_name, channel_bone_name)) return true;
  return (bone_name.size() > channel_bone_name.size() &&
          bone_name.compare(0, channel_bone_name.size(), channel_bone_name) == 0 &&
          bone_name[channel_bone_name.size()] == '.') ||
         bone_name == channel_bone_name;
}

static bool is_hand_bone(const std::string& name) {
  return name.find("-hand") != std::string::npos;
}

static bool is_ik_hand_target_bone(const std::string& name) {
  return name.find("_hand") != std::string::npos;
}

static int find_bone_index(const Character& character, const std::string& name) {
  for (size_t i = 0; i < character.bones.size(); ++i)
    if (character.bones[i].name == name ||
        channel_matches_bone(character.bones[i].name, name))
      return (int)i;
  return -1;
}

[[maybe_unused]] static bool is_eye_mesh_name(const std::string& name) {
  std::string lower = name;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return lower.rfind("eye-", 0) == 0 ||
         lower.rfind("l-eye.", 0) == 0 ||
         lower.rfind("r-eye.", 0) == 0 ||
         lower.find("_eyel") != std::string::npos ||
         lower.find("_eyer") != std::string::npos ||
         lower.find("eye_l") != std::string::npos ||
         lower.find("eye_r") != std::string::npos ||
         lower.find("eyel.") != std::string::npos ||
         lower.find("eyer.") != std::string::npos;
}

static std::array<float, 16> mat4_mul(const std::array<float, 16>& a,
                                      const std::array<float, 16>& b);

static void dump_leg_pose(const Character& character) {
  auto dump = [&](const char* name) {
    const int i = find_bone_index(character, name);
    if (i < 0 || static_cast<size_t>(i) >= character.bones.size()) return;
    const auto& local = character.bones[static_cast<size_t>(i)].local;
    const auto& exact_name = character.bones[static_cast<size_t>(i)].name;
    const auto cur = character.bone_world_local_chain(exact_name);
    const auto bind = character.bone_world_bind_local_chain(exact_name);
    std::fprintf(stderr,
                 "[legpose] %-18s exact=%-24s localPos=(%.3f %.3f %.3f) "
                 "world=(%.3f %.3f %.3f) bind=(%.3f %.3f %.3f) "
                 "rows=[%.3f %.3f %.3f|%.3f %.3f %.3f|%.3f %.3f %.3f]\n",
                 name, exact_name.c_str(), local.pos[0], local.pos[1], local.pos[2],
                 cur[12], cur[13], cur[14], bind[12], bind[13], bind[14],
                 local.rot[0][0], local.rot[0][1], local.rot[0][2],
                 local.rot[1][0], local.rot[1][1], local.rot[1][2],
                 local.rot[2][0], local.rot[2][1], local.rot[2][2]);
  };
  dump("bone_facing");
  dump("bone_pelvis");
  dump("bone_L-thigh");
  dump("bone_L-knee");
  dump("bone_L-foot");
  dump("bone_L-toe");
  dump("bone_R-thigh");
  dump("bone_R-knee");
  dump("bone_R-foot");
  dump("bone_R-toe");
}

static void dump_arm_pose(const Character& character, const char* tag) {
  if (!debug_arm_pose_enabled()) return;
  auto dump = [&](const char* name) {
    const int i = find_bone_index(character, name);
    if (i < 0 || static_cast<size_t>(i) >= character.bones.size()) {
      std::fprintf(stderr, "[armpose] %s %-22s missing\n", tag, name);
      return;
    }
    const auto& bone = character.bones[static_cast<size_t>(i)];
    const auto cur = character.bone_world_local_chain(bone.name);
    const auto bind = character.bone_world_bind_local_chain(bone.name);
    std::fprintf(
        stderr,
        "[armpose] %s %-22s exact=%-28s parent=%-24s "
        "localPos=(%.4f %.4f %.4f) world=(%.4f %.4f %.4f) "
        "bind=(%.4f %.4f %.4f) "
        "rows=[%.5f %.5f %.5f|%.5f %.5f %.5f|%.5f %.5f %.5f]\n",
        tag, name, bone.name.c_str(), bone.parent.c_str(), bone.local.pos[0],
        bone.local.pos[1], bone.local.pos[2], cur[12], cur[13], cur[14],
        bind[12], bind[13], bind[14], bone.local.rot[0][0],
        bone.local.rot[0][1], bone.local.rot[0][2], bone.local.rot[1][0],
        bone.local.rot[1][1], bone.local.rot[1][2], bone.local.rot[2][0],
        bone.local.rot[2][1], bone.local.rot[2][2]);
  };
  dump("bone_pelvis");
  dump("bone_spine1");
  dump("bone_spine2");
  dump("bone_spine3");
  dump("bone_neck");
  dump("bone_head");
  dump("bone_L-clavicle");
  dump("bone_L-upperArm");
  dump("bone_L-upperTwist1");
  dump("bone_L-upperTwist2");
  dump("bone_L-foreArm");
  dump("bone_L-foreTwist1");
  dump("bone_L-foreTwist2");
  dump("bone_L-hand");
  dump("bone_R-clavicle");
  dump("bone_R-upperArm");
  dump("bone_R-upperTwist1");
  dump("bone_R-upperTwist2");
  dump("bone_R-foreArm");
  dump("bone_R-foreTwist1");
  dump("bone_R-foreTwist2");
  dump("bone_R-hand");
}

static bool transform_local_chain_world(const Character& character,
                                        const std::string& name,
                                        std::array<float, 16>& out) {
  const auto runtime_it = character.runtime_world_overrides.find(name);
  if (runtime_it != character.runtime_world_overrides.end()) {
    out = runtime_it->second;
    return true;
  }
  for (const auto& b : character.bones) {
    if (b.name == name || channel_matches_bone(b.name, name)) {
      out = character.bone_world_local_chain(b.name);
      return true;
    }
  }
  for (const auto& m : character.meshes) {
    if (m.name == name || channel_matches_bone(m.name, name)) {
      out = character.mesh_world(m);
      return true;
    }
  }
  return false;
}

static std::array<float, 16> mat4_mul(const std::array<float, 16>& a,
                                      const std::array<float, 16>& b) {
  std::array<float, 16> r{};
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      float s = 0.0f;
      for (int k = 0; k < 4; ++k) s += a[i * 4 + k] * b[k * 4 + j];
      r[i * 4 + j] = s;
    }
  return r;
}

static std::array<float, 16> affine_inverse(const std::array<float, 16>& m) {
  const float a=m[0], b=m[1], c=m[2];
  const float d=m[4], e=m[5], f=m[6];
  const float g=m[8], h=m[9], i=m[10];
  const float det = a*(e*i - f*h) - b*(d*i - f*g) + c*(d*h - e*g);
  if (std::fabs(det) < 1e-8f)
    return {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  const float inv = 1.0f / det;
  std::array<float, 16> r{};
  r[0] =  (e*i - f*h) * inv; r[1] = -(b*i - c*h) * inv; r[2] =  (b*f - c*e) * inv;
  r[4] = -(d*i - f*g) * inv; r[5] =  (a*i - c*g) * inv; r[6] = -(a*f - c*d) * inv;
  r[8] =  (d*h - e*g) * inv; r[9] = -(a*h - b*g) * inv; r[10]=  (a*e - b*d) * inv;
  r[15] = 1.0f;
  const float tx=m[12], ty=m[13], tz=m[14];
  r[12] = -(tx*r[0] + ty*r[4] + tz*r[8]);
  r[13] = -(tx*r[1] + ty*r[5] + tz*r[9]);
  r[14] = -(tx*r[2] + ty*r[6] + tz*r[10]);
  return r;
}

static void mat4_to_xfm(const std::array<float, 16>& m, milo_scene::Xfm& x) {
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c)
      x.rot[r][c] = m[r * 4 + c];
  x.pos[0] = m[12];
  x.pos[1] = m[13];
  x.pos[2] = m[14];
}

struct Vec3 {
  float x = 0, y = 0, z = 0;
};

static Vec3 vsub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static Vec3 vadd(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static Vec3 vscale(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
static float vdot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static Vec3 vcross(Vec3 a, Vec3 b) {
  return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
static float vlen(Vec3 a) { return std::sqrt(vdot(a, a)); }
static Vec3 vnorm(Vec3 a, Vec3 fallback = {1, 0, 0}) {
  const float len = vlen(a);
  return len > 1e-6f ? vscale(a, 1.0f / len) : fallback;
}
static Vec3 mat_pos(const std::array<float, 16>& m) {
  return {m[12], m[13], m[14]};
}
static Vec3 mat_row(const std::array<float, 16>& m, int r) {
  return {m[r * 4 + 0], m[r * 4 + 1], m[r * 4 + 2]};
}

static Vec3 vec_from_array3(const float v[3]) {
  return {v[0], v[1], v[2]};
}

static Vec3 vec_from_array3(const std::array<float, 3>& v) {
  return {v[0], v[1], v[2]};
}

static std::array<float, 3> array3_from_vec(Vec3 v) {
  return {v.x, v.y, v.z};
}

static void array3_from_vec(std::array<float, 3>& out, Vec3 v) {
  out[0] = v.x;
  out[1] = v.y;
  out[2] = v.z;
}

static void set_mat_row(std::array<float, 16>& m, int r, Vec3 v) {
  m[r * 4 + 0] = v.x;
  m[r * 4 + 1] = v.y;
  m[r * 4 + 2] = v.z;
}

static Vec3 local_vec_from_world_rows(const std::array<float, 16>& basis_world,
                                      Vec3 world_vec) {
  return {vdot(mat_row(basis_world, 0), world_vec),
          vdot(mat_row(basis_world, 1), world_vec),
          vdot(mat_row(basis_world, 2), world_vec)};
}

static void quat_from_vec_to_vec(Vec3 from, Vec3 to, float q[4]) {
  from = vnorm(from);
  to = vnorm(to, from);
  Vec3 axis = vcross(from, to);
  float dot = std::clamp(vdot(from, to), -1.0f, 1.0f);
  if (dot < -0.9999f) {
    axis = vcross(from, {1.0f, 0.0f, 0.0f});
    if (vlen(axis) <= 1e-5f) axis = vcross(from, {0.0f, 1.0f, 0.0f});
    axis = vnorm(axis, {0.0f, 0.0f, 1.0f});
    q[0] = axis.x;
    q[1] = axis.y;
    q[2] = axis.z;
    q[3] = 0.0f;
    return;
  }
  const float s = std::sqrt((1.0f + dot) * 2.0f);
  if (s <= 1e-6f) {
    q[0] = q[1] = 0.0f;
    q[2] = 0.0f;
    q[3] = 1.0f;
    return;
  }
  const float inv = 1.0f / s;
  q[0] = axis.x * inv;
  q[1] = axis.y * inv;
  q[2] = axis.z * inv;
  q[3] = 0.5f * s;
}

static Vec3 rotate_vec_by_quat(Vec3 v, const float q_in[4]) {
  float rot[3][3] = {};
  quat_to_rot(q_in, rot);
  return {
      v.x * rot[0][0] + v.y * rot[1][0] + v.z * rot[2][0],
      v.x * rot[0][1] + v.y * rot[1][1] + v.z * rot[2][1],
      v.x * rot[0][2] + v.y * rot[1][2] + v.z * rot[2][2],
  };
}

static void source_look_at_rows(std::array<float, 16>& m) {
  const Vec3 x = mat_row(m, 0);
  const Vec3 z = vnorm(vcross(x, mat_row(m, 1)), {0.0f, 0.0f, 1.0f});
  const Vec3 y = vcross(z, x);
  set_mat_row(m, 1, y);
  set_mat_row(m, 2, z);
}

static void normalize_mat3_rows(std::array<float, 16>& m);

static std::array<float, 16> source_transform_row_mat4(
    const float xfm[4][3]) {
  return {xfm[0][0], xfm[0][1], xfm[0][2], 0.0f,
          xfm[1][0], xfm[1][1], xfm[1][2], 0.0f,
          xfm[2][0], xfm[2][1], xfm[2][2], 0.0f,
          xfm[3][0], xfm[3][1], xfm[3][2], 1.0f};
}

SourceCharIKRodDefaultState source_char_ik_rod_default_state() {
  return SourceCharIKRodDefaultState{};
}

SourceCharIKRodLoadPlan source_char_ik_rod_load_plan(int32_t revision) {
  SourceCharIKRodLoadPlan plan;
  plan.known_revision = revision >= 0 && revision <= 2;
  if (!plan.known_revision) return plan;
  plan.read_order = {"Hmx::Object", "mLeftEnd", "mRightEnd", "mDestPos",
                     "mSideAxis",    "mVertical", "mDest",     "mXfm"};
  return plan;
}

SourceCharIKRodCopyPlan source_char_ik_rod_copy_plan() {
  SourceCharIKRodCopyPlan plan;
  plan.copied_superclasses = {"Hmx::Object"};
  plan.copied_members = {"mLeftEnd",  "mRightEnd", "mDestPos", "mSideAxis",
                         "mVertical", "mDest",     "mXfm"};
  return plan;
}

SourceCharIKRodHandlerPlan source_char_ik_rod_handler_plan() {
  SourceCharIKRodHandlerPlan plan;
  plan.superclasses = {"Hmx::Object"};
  plan.check = 0xAF;
  return plan;
}

SourceCharIKRodPropSyncPlan source_char_ik_rod_prop_sync_plan() {
  SourceCharIKRodPropSyncPlan plan;
  plan.modify_alt_properties = {"left_end", "right_end", "dest_pos",
                                "side_axis", "vertical", "dest"};
  plan.modify_actions.assign(plan.modify_alt_properties.size(), "SyncBones");
  return plan;
}

SourceCharIKRodSavePlan source_char_ik_rod_save_plan() {
  return SourceCharIKRodSavePlan{};
}

void source_char_ik_rod_poll_deps(SourceCharIKRodPollDeps& deps,
                                  const CharIKRod& rod) {
  deps.change.push_back(rod.dest);
  deps.changed_by.push_back(rod.left_end);
  deps.changed_by.push_back(rod.right_end);
  deps.changed_by.push_back(rod.side_axis);
}

bool source_char_ik_rod_compute_world(const CharIKRod& rod,
                                      const Character& character,
                                      std::array<float, 16>& dest_world) {
  if (rod.dest.empty() || rod.left_end.empty() || rod.right_end.empty()) {
    return false;
  }
  if (!character.has_transform(rod.dest)) return false;

  std::array<float, 16> left_world{};
  std::array<float, 16> right_world{};
  if (!transform_local_chain_world(character, rod.left_end, left_world) ||
      !transform_local_chain_world(character, rod.right_end, right_world)) {
    return false;
  }

  std::array<float, 16> rod_world =
      {1, 0, 0, 0, 0, 1, 0, 0,
       0, 0, 1, 0, 0, 0, 0, 1};
  const float t = rod.dest_pos;
  const Vec3 left_pos = mat_pos(left_world);
  const Vec3 right_pos = mat_pos(right_world);
  const Vec3 pos =
      vadd(vscale(left_pos, 1.0f - t), vscale(right_pos, t));
  rod_world[12] = pos.x;
  rod_world[13] = pos.y;
  rod_world[14] = pos.z;

  const Vec3 x = rod.vertical
                     ? Vec3{0.0f, 0.0f, -1.0f}
                     : vnorm(vadd(vscale(mat_row(left_world, 0), 1.0f - t),
                                  vscale(mat_row(right_world, 0), t)));
  Vec3 z{};
  if (!rod.side_axis.empty()) {
    std::array<float, 16> side_world{};
    if (transform_local_chain_world(character, rod.side_axis, side_world)) {
      z = mat_row(side_world, 2);
    } else {
      z = vsub(left_pos, right_pos);
    }
  } else {
    z = vsub(left_pos, right_pos);
  }
  Vec3 y = vnorm(vcross(z, x));
  z = vcross(x, y);
  set_mat_row(rod_world, 0, x);
  set_mat_row(rod_world, 1, y);
  set_mat_row(rod_world, 2, z);
  normalize_mat3_rows(rod_world);

  dest_world = mat4_mul(source_transform_row_mat4(rod.xfm), rod_world);
  normalize_mat3_rows(dest_world);
  return true;
}

SourceCharIKHandMeasure source_char_ik_hand_measure_lengths(
    bool has_elbow_chain,
    float hand_local_len,
    float parent_local_len) {
  SourceCharIKHandMeasure out;
  if (!has_elbow_chain) return out;
  out.has_elbow_chain = true;
  out.inv_2ab = hand_local_len * 2.0f * parent_local_len;
  out.a2_plus_b2 =
      parent_local_len * parent_local_len + hand_local_len * hand_local_len;
  if (out.inv_2ab != 0.0f) out.inv_2ab = 1.0f / out.inv_2ab;
  out.aa_plus_bb = hand_local_len + parent_local_len;
  return out;
}

SourceCharIKHandLoadPlan source_char_ik_hand_load_plan(int32_t revision) {
  SourceCharIKHandLoadPlan plan;
  plan.known_revision = revision >= 0 && revision <= plan.max_revision;
  if (!plan.known_revision) return plan;

  plan.read_order = {"LOAD_REVS", "ASSERT_REVS(0xC,0)", "Hmx::Object",
                     "CharWeightable", "mHand"};
  if (revision > 4) {
    plan.read_order.push_back("mFinger");
  } else {
    plan.branches.push_back("mFinger=0");
  }

  if (revision < 3) {
    plan.read_order.push_back("legacyTarget");
    plan.branches.push_back("targets=singleLegacyTargetExtent0");
  } else if (revision < 0x0b) {
    plan.read_order.push_back("legacyTargetList");
    plan.branches.push_back("targets=legacyListExtent0");
  } else {
    plan.read_order.push_back("mTargets");
  }

  plan.read_order.push_back("mOrientation");
  plan.read_order.push_back("mStretch");
  if (revision > 1) {
    plan.read_order.push_back("mScalable");
  } else {
    plan.branches.push_back("mScalable=false");
  }
  if (revision > 3) {
    plan.read_order.push_back("mMoveElbow");
  } else {
    plan.branches.push_back("mMoveElbow=true");
  }
  if (revision > 5) {
    plan.read_order.push_back("mElbowSwing");
  } else {
    plan.branches.push_back("mElbowSwing=0");
  }
  if (revision > 6) plan.read_order.push_back("mAlwaysIKElbow");
  if (revision > 7) {
    plan.read_order.push_back("mConstrainWrist");
    plan.read_order.push_back("mWristRadians");
  }
  if (revision == 9) {
    plan.read_order.push_back("rev9StringPadding");
    plan.read_order.push_back("rev9BoolPadding");
  }
  if (revision > 0x0b) {
    plan.read_order.push_back("mElbowCollide");
    plan.read_order.push_back("mClockwise");
  }
  plan.calls_set_hand = true;
  return plan;
}

SourceCharIKHandCopyPlan source_char_ik_hand_copy_plan() {
  SourceCharIKHandCopyPlan plan;
  plan.copied_superclasses = {"Hmx::Object", "CharWeightable"};
  plan.member_steps = {"SetHand(c->mHand)", "mHand", "mTargets",
                       "mOrientation",     "mStretch",
                       "mScalable",        "mMoveElbow",
                       "mElbowSwing",      "mAlwaysIKElbow",
                       "mConstrainWrist",  "mTargets",
                       "mElbowCollide",    "mClockwise"};
  return plan;
}

SourceCharIKHandHandlerPlan source_char_ik_hand_handler_plan() {
  SourceCharIKHandHandlerPlan plan;
  plan.handlers = {"measure_lengths"};
  plan.superclasses = {"CharWeightable", "Hmx::Object"};
  plan.check = "0x33D";
  return plan;
}

SourceCharIKHandPropSyncPlan source_char_ik_hand_prop_sync_plan() {
  SourceCharIKHandPropSyncPlan plan;
  plan.target_properties = {"target", "extent"};
  plan.set_properties = {"hand"};
  plan.properties = {"finger",        "targets",         "orientation",
                     "stretch",       "scalable",        "move_elbow",
                     "elbow_swing",   "always_ik_elbow", "constrain_wrist",
                     "wrist_radians", "elbow_collide",   "clockwise"};
  plan.superclass = "CharWeightable";
  return plan;
}

SourceCharIKHandSavePlan source_char_ik_hand_save_plan() {
  return SourceCharIKHandSavePlan{};
}

SourceCharIKHandSetHandResult source_char_ik_hand_set_hand(
    const std::string& hand) {
  SourceCharIKHandSetHandResult result;
  result.assigned_hand = hand;
  result.hand_changed = true;
  return result;
}

bool source_char_ik_hand_update_measure_lengths(bool scalable,
                                                bool& hand_changed) {
  if (scalable || hand_changed) {
    hand_changed = false;
    return true;
  }
  return false;
}

bool source_char_ik_hand_elbow_cosine(
    const SourceCharIKHandMeasure& measure,
    float distance_squared,
    float& out_cosine) {
  if (!measure.has_elbow_chain) return false;
  out_cosine =
      measure.inv_2ab * (distance_squared - measure.a2_plus_b2);
  out_cosine = std::clamp(out_cosine, -1.0f, 1.0f);
  return true;
}

SourceCharIKHandElbowBendRows source_char_ik_hand_elbow_bend_rows(
    float cosine, float sine) {
  SourceCharIKHandElbowBendRows out;
  out.applied = true;
  out.rows[0] = {cosine, -sine, 0.0f};
  out.rows[1] = {sine, cosine, 0.0f};
  out.rows[2] = {0.0f, 0.0f, 1.0f};
  return out;
}

static std::array<float, 16> source_xfm_to_mat4(
    const milo_scene::Xfm& xfm);

SourceCharIKHandTargetBlendResult source_char_ik_hand_multi_target_blend(
    float char_weight,
    const std::vector<SourceCharIKHandTargetInput>& targets,
    bool orientation) {
  SourceCharIKHandTargetBlendResult result;
  if (targets.empty()) return result;
  result.entered = true;
  result.adjusted_weight = char_weight;
  result.weights.assign(targets.size(), 0.0f);

  for (size_t i = 0; i < targets.size(); ++i) {
    const SourceCharIKHandTargetInput& target = targets[i];
    if (!target.present) continue;
    float x = target.world_pos[0];
    float y = target.world_pos[1];
    float z = target.world_pos[2];
    if (target.extent > 0.0f) {
      if (target.extent < -z) {
        result.weights[i] = 0.001f;
        result.sum += result.weights[i];
        continue;
      }
      z = 0.0f;
    }
    const float length_sq = std::max(0.001f, x * x + y * y + z * z);
    result.weights[i] = 144.0f / length_sq;
    result.sum += result.weights[i];
  }

  if (result.sum <= 0.0f) {
    result.adjusted_weight = 0.0f;
    return result;
  }

  if (result.sum < 1.0f) {
    result.adjusted_weight =
        char_weight - (char_weight * (1.0f - result.sum));
    result.reduced_weight_for_low_sum = true;
  }

  for (size_t i = 0; i < targets.size(); ++i) {
    const SourceCharIKHandTargetInput& target = targets[i];
    if (!target.present || result.weights[i] == 0.0f) continue;
    const float scale = result.weights[i] / result.sum;
    for (int axis = 0; axis < 3; ++axis) {
      result.blended_pos[axis] += target.world_pos[axis] * scale;
    }
    if (orientation && target.world_quat.has_value()) {
      result.orientation_blended = true;
      for (int axis = 0; axis < 4; ++axis) {
        result.blended_quat[axis] += (*target.world_quat)[axis] * scale;
      }
    }
  }

  if (result.orientation_blended) {
    float len_sq = 0.0f;
    for (float value : result.blended_quat) len_sq += value * value;
    if (len_sq > 0.0f) {
      const float inv_len = 1.0f / std::sqrt(len_sq);
      for (float& value : result.blended_quat) value *= inv_len;
      result.orientation_normalized = true;
    }
  }
  return result;
}

SourceCharIKHandFingerTargetResult source_char_ik_hand_finger_target(
    bool has_finger,
    const milo_scene::Xfm& hand_world,
    const milo_scene::Xfm& finger_world,
    std::array<float, 3> target_pos,
    std::array<float, 4> target_quat) {
  SourceCharIKHandFingerTargetResult result;
  result.adjusted_target.pos[0] = target_pos[0];
  result.adjusted_target.pos[1] = target_pos[1];
  result.adjusted_target.pos[2] = target_pos[2];
  quat_to_rot(target_quat.data(), result.adjusted_target.rot);
  if (!has_finger) return result;

  const std::array<float, 16> hand_to_finger = mat4_mul(
      source_xfm_to_mat4(hand_world), affine_inverse(source_xfm_to_mat4(finger_world)));
  mat4_to_xfm(mat4_mul(hand_to_finger, source_xfm_to_mat4(result.adjusted_target)),
              result.adjusted_target);
  result.applied = true;
  return result;
}

SourceCharIKHandWristConstraintResult source_char_ik_hand_wrist_constraint(
    const SourceCharIKHandWristConstraintInput& input) {
  SourceCharIKHandWristConstraintResult result;
  result.corrected_x = input.hand_x;
  result.corrected_y = input.hand_y;
  result.corrected_z = input.hand_z;
  result.final_hand_pos = input.hand_pos;

  if (!input.constrain_wrist || input.char_weight <= 0.0f ||
      !input.has_parent) {
    return result;
  }

  result.entered = true;
  const Vec3 parent_x = vec_from_array3(input.parent_x);
  const Vec3 hand_z = vec_from_array3(input.hand_z);
  result.raw_angle =
      std::acos(vdot(parent_x, hand_z)) - 1.570796370506287f;
  if (std::fabs(result.raw_angle) <= input.wrist_radians) {
    return result;
  }

  result.angle_exceeded = true;
  result.correction_angle = result.raw_angle;
  if (result.correction_angle > 0.0f) {
    result.correction_angle -= input.wrist_radians;
  } else {
    result.correction_angle += input.wrist_radians;
  }

  const Vec3 hand_y = vec_from_array3(input.hand_y);
  float q[4] = {hand_y.x * std::sin(result.correction_angle * 0.5f),
                hand_y.y * std::sin(result.correction_angle * 0.5f),
                hand_y.z * std::sin(result.correction_angle * 0.5f),
                std::cos(result.correction_angle * 0.5f)};
  const Vec3 corrected_x = rotate_vec_by_quat(vec_from_array3(input.hand_x), q);
  const Vec3 corrected_z = vcross(corrected_x, hand_y);
  result.corrected_x = {corrected_x.x, corrected_x.y, corrected_x.z};
  result.corrected_y = input.hand_y;
  result.corrected_z = {corrected_z.x, corrected_z.y, corrected_z.z};
  result.wrote_first_hand_xfm = true;

  for (int axis = 0; axis < 3; ++axis) {
    result.finger_delta[axis] =
        input.finger_after_first_set_pos[axis] - input.finger_before_pos[axis];
    result.final_hand_pos[axis] = input.hand_pos[axis] - result.finger_delta[axis];
  }
  result.compensated_finger_delta = true;
  result.updates_world_dst = true;
  result.requests_elbow_resolve = true;
  result.rewrites_hand_after_elbow = true;
  return result;
}

SourceCharIKHandElbowSwingResult source_char_ik_hand_elbow_swing(
    const SourceCharIKHandElbowSwingInput& input) {
  SourceCharIKHandElbowSwingResult result;
  if (input.elbow_swing <= 0.0f) return result;

  result.entered = true;
  const float current_len_sq =
      input.current_yz[0] * input.current_yz[0] +
      input.current_yz[1] * input.current_yz[1];
  const float target_len_sq =
      input.target_yz[0] * input.target_yz[0] +
      input.target_yz[1] * input.target_yz[1];
  result.current_len_sq = std::max(current_len_sq, 16.0f);
  result.target_len_sq = std::max(target_len_sq, 16.0f);
  result.denom = std::sqrt(result.current_len_sq * result.target_len_sq);
  result.cross = input.target_yz[0] * input.current_yz[1] -
                 input.target_yz[1] * input.current_yz[0];
  result.unclamped = result.cross / result.denom;
  result.clamped =
      std::clamp(result.unclamped, -input.elbow_swing, input.elbow_swing);
  result.rotate_about_x = -result.clamped;
  result.recompute_current_after_rotation = true;
  return result;
}

SourceCharIKHandElbowCollisionResult source_char_ik_hand_elbow_collision_gate(
    const SourceCharIKHandElbowCollisionInput& input) {
  SourceCharIKHandElbowCollisionResult result;
  if (!input.has_elbow_collide) return result;

  result.entered = true;
  result.needs_source_shoulder_offset = true;
  if (!input.collide_shape_is_sphere) {
    result.warn_non_sphere = true;
    return result;
  }

  result.sphere_branch = true;
  result.inside_sphere = input.distance_to_elbow < input.sphere_radius;
  if (!result.inside_sphere) return result;

  result.needs_collision_rotation = true;
  result.updates_upper_arm_matrix = true;
  result.updates_forearm_matrix = true;
  if (input.clockwise) {
    result.uses_clockwise_candidate = true;
  } else {
    result.uses_counterclockwise_candidate = true;
  }
  return result;
}

SourceCharIKHandPollFlowResult source_char_ik_hand_poll_flow(
    const SourceCharIKHandPollFlowInput& input) {
  SourceCharIKHandPollFlowResult result;
  if (!input.has_hand || !input.has_targets) {
    result.early_out = true;
    return result;
  }

  result.parent1_initial = input.has_parent;
  result.parent1_after_move_elbow = input.move_elbow && input.has_parent;
  result.parent1_after_grandparent_gate = result.parent1_after_move_elbow;
  if (input.char_weight != 0.0f || input.always_ik_elbow) {
    if (result.parent1_after_move_elbow) {
      result.parent2_resolved = input.has_grandparent;
      if (!result.parent2_resolved) {
        result.parent1_after_grandparent_gate = false;
      }
    }
    result.calls_ik_elbow = true;
    result.ik_elbow_has_chain =
        result.parent1_after_grandparent_gate && result.parent2_resolved;
  }

  result.final_hand_write =
      input.char_weight != 0.0f &&
      (!result.parent1_after_grandparent_gate || input.orientation ||
       input.stretch);
  if (result.final_hand_write) {
    result.final_position_from_world_dst =
        !result.parent1_after_grandparent_gate || input.stretch;
    result.final_orientation_from_target = input.orientation;
    result.interpolates_orientation =
        input.orientation && input.char_weight < 1.0f;
  }
  return result;
}

static float source_distance3(const std::array<float, 3>& a,
                              const std::array<float, 3>& b) {
  const float dx = a[0] - b[0];
  const float dy = a[1] - b[1];
  const float dz = a[2] - b[2];
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

SourceCharIKFootState source_char_ik_foot_default_state() {
  return SourceCharIKFootState{};
}

SourceCharIKFootEnterResult source_char_ik_foot_enter(
    SourceCharIKFootState& state) {
  SourceCharIKFootEnterResult result;
  state.fsm_state = 0;
  result.reset_fsm_state = true;
  state.release_distance = 0.0f;
  result.reset_release_distance = true;
  return result;
}

SourceCharIKFootSetNameResult source_char_ik_foot_set_name(
    SourceCharIKFootState& state,
    const std::string& dir_name,
    bool dir_is_character) {
  SourceCharIKFootSetNameResult result;
  result.call_hmx_set_name = true;
  result.assigned_character = dir_is_character;
  state.character_dir = dir_is_character ? dir_name : std::string{};
  return result;
}

SourceCharIKFootPollPlan source_char_ik_foot_poll_plan(
    bool has_finger,
    bool has_hand,
    bool has_data) {
  SourceCharIKFootPollPlan plan;
  if (!has_finger || !has_hand || !has_data) return plan;
  plan.should_poll = true;
  plan.clear_targets_before = true;
  plan.push_helper_target = true;
  plan.run_do_fsm = true;
  plan.call_char_ik_hand_poll = true;
  plan.clear_targets_after = true;
  return plan;
}

SourceCharIKFootPollDepsPlan source_char_ik_foot_poll_deps_plan() {
  SourceCharIKFootPollDepsPlan plan;
  plan.call_char_ik_hand_poll_deps = true;
  return plan;
}

SourceCharIKFootFsmResult source_char_ik_foot_do_fsm(
    SourceCharIKFootState& state,
    const std::array<float, 3>& current_target_pos,
    const std::array<float, 3>& finger_world_pos,
    float data_value,
    float delta_seconds,
    bool character_teleported) {
  SourceCharIKFootFsmResult result;
  if (character_teleported) state.fsm_state = 0;
  if (delta_seconds < 0.0f) {
    delta_seconds = 0.0f;
    result.clamped_negative_delta = true;
  }

  result.copied_finger_matrix = true;
  std::array<float, 3> target = current_target_pos;
  target[2] = finger_world_pos[2];
  state.planted_pos[2] = target[2];

  bool planted = false;
  if (data_value >= 1.0f) {
    const float threshold = state.fsm_state == 1 ? 0.6f : 0.5f;
    if (threshold > target[2]) planted = true;
  } else {
    planted = true;
  }
  result.planted = planted;

  if (state.fsm_state == 0) {
    target = finger_world_pos;
    if (planted) {
      state.planted_pos = target;
      state.fsm_state = 1;
    }
  }
  if (state.fsm_state == 1) {
    if (!planted) {
      state.fsm_state = 2;
      state.release_distance = source_distance3(finger_world_pos, target);
    } else {
      std::array<float, 3> delta = {
          finger_world_pos[0] - state.planted_pos[0],
          finger_world_pos[1] - state.planted_pos[1],
          finger_world_pos[2] - state.planted_pos[2]};
      const float len = std::sqrt(delta[0] * delta[0] + delta[1] * delta[1] +
                                  delta[2] * delta[2]);
      if (len > 0.125f) {
        const float scale = 0.125f / len;
        delta[0] *= scale;
        delta[1] *= scale;
        delta[2] *= scale;
      }
      target = {state.planted_pos[0] + delta[0],
                state.planted_pos[1] + delta[1],
                state.planted_pos[2] + delta[2]};
      result.returned_from_planted_state = true;
      result.target_pos = target;
      result.fsm_state = state.fsm_state;
      result.release_distance = state.release_distance;
      return result;
    }
  }
  if (state.fsm_state == 2) {
    std::array<float, 3> delta = {finger_world_pos[0] - target[0],
                                  finger_world_pos[1] - target[1],
                                  finger_world_pos[2] - target[2]};
    const float len = std::sqrt(delta[0] * delta[0] + delta[1] * delta[1] +
                                delta[2] * delta[2]);
    state.release_distance =
        std::min(state.release_distance - delta_seconds * 25.0f, len);
    if (state.release_distance <= 0.0f) {
      state.fsm_state = 0;
    } else if (len != 0.0f) {
      const float scale = (len - state.release_distance) / len;
      target[0] += delta[0] * scale;
      target[1] += delta[1] * scale;
      target[2] += delta[2] * scale;
    }
    if (planted) {
      state.planted_pos = target;
      state.fsm_state = 1;
    }
  }

  result.target_pos = target;
  result.fsm_state = state.fsm_state;
  result.release_distance = state.release_distance;
  return result;
}

SourceCharIKFootLoadSteps source_char_ik_foot_load_steps(int32_t revision) {
  SourceCharIKFootLoadSteps steps;
  steps.known_revision = revision >= 0 && revision <= steps.max_revision;
  steps.load_char_ik_hand = true;
  steps.read_legacy_symbol = revision < 6;
  if (revision < 5) {
    if (revision > 1) ++steps.legacy_int_reads;
    if (revision > 2) ++steps.legacy_int_reads;
    if (revision > 3) ++steps.legacy_int_reads;
  } else {
    steps.load_data = true;
    steps.load_data_index = true;
  }
  return steps;
}

SourceCharIKFootCopyResult source_char_ik_foot_copy(
    SourceCharIKFootState& dest,
    const SourceCharIKFootState& source) {
  SourceCharIKFootCopyResult result;
  result.copy_char_ik_hand = true;
  dest.data = source.data;
  result.copy_data = true;
  dest.data_index = source.data_index;
  result.copy_data_index = true;
  return result;
}

SourceCharIKFootHandlerPlan source_char_ik_foot_handler_plan() {
  SourceCharIKFootHandlerPlan plan;
  plan.superclasses = {"CharIKHand"};
  plan.check = 0x16E;
  return plan;
}

SourceCharIKFootPropSyncPlan source_char_ik_foot_prop_sync_plan() {
  SourceCharIKFootPropSyncPlan plan;
  plan.properties = {"data", "data_index"};
  plan.superclasses = {"CharIKHand"};
  return plan;
}

SourceCharIKFootSavePlan source_char_ik_foot_save_plan() {
  return SourceCharIKFootSavePlan{};
}

static std::array<float, 16> source_xfm_to_mat4(
    const milo_scene::Xfm& xfm) {
  return {xfm.rot[0][0], xfm.rot[0][1], xfm.rot[0][2], 0.0f,
          xfm.rot[1][0], xfm.rot[1][1], xfm.rot[1][2], 0.0f,
          xfm.rot[2][0], xfm.rot[2][1], xfm.rot[2][2], 0.0f,
          xfm.pos[0], xfm.pos[1], xfm.pos[2], 1.0f};
}

SourceCharBoneOffsetSavePlan source_char_bone_offset_save_plan() {
  return SourceCharBoneOffsetSavePlan{};
}

void source_char_bone_offset_poll_deps(
    SourceCharBoneOffsetPollDeps& deps,
    const std::string& dest,
    const std::string& dest_parent) {
  deps.change.push_back(dest);
  if (!dest.empty() && !dest_parent.empty()) {
    deps.changed_by.push_back(dest_parent);
  }
}

bool source_char_bone_offset_poll_world(
    const CharBoneOffset& offset,
    bool has_dest,
    bool has_parent,
    const milo_scene::Xfm& dest_local,
    const std::array<float, 16>& parent_world,
    std::array<float, 16>& dest_world) {
  if (!has_dest || !has_parent) return false;
  milo_scene::Xfm local = dest_local;
  local.pos[0] += offset.offset[0];
  local.pos[1] += offset.offset[1];
  local.pos[2] += offset.offset[2];
  dest_world = mat4_mul(source_xfm_to_mat4(local), parent_world);
  return true;
}

void source_char_bone_offset_apply_to_local(const CharBoneOffset& offset,
                                            milo_scene::Xfm& dest_local) {
  dest_local.pos[0] += offset.offset[0];
  dest_local.pos[1] += offset.offset[1];
  dest_local.pos[2] += offset.offset[2];
}

SourceCharBoneTwistSavePlan source_char_bone_twist_save_plan() {
  return SourceCharBoneTwistSavePlan{};
}

void source_char_bone_twist_poll_deps(
    SourceCharBoneTwistPollDeps& deps,
    const std::string& bone,
    const std::vector<std::string>& targets) {
  deps.change.push_back(bone);
  for (const std::string& target : targets) {
    deps.changed_by.push_back(target);
  }
}

float source_char_bone_twist_weight(
    const CharBoneTwist& twist,
    const std::unordered_map<std::string, float>& weights_by_name) {
  if (!twist.weight_owner.empty()) {
    const auto owner = weights_by_name.find(twist.weight_owner);
    if (owner != weights_by_name.end()) return owner->second;
  }
  return twist.weight;
}

bool source_char_bone_twist_poll_world(
    const CharBoneTwist& twist,
    bool has_bone,
    const std::array<float, 16>& bone_world,
    const std::vector<std::array<float, 16>>& target_worlds,
    const std::unordered_map<std::string, float>& weights_by_name,
    std::array<float, 16>& out_world) {
  if (!has_bone || target_worlds.empty()) return false;

  Vec3 avg{};
  for (const auto& target_world : target_worlds) {
    avg = vadd(avg, mat_pos(target_world));
  }
  avg = vscale(avg, 1.0f / static_cast<float>(target_worlds.size()));

  out_world = bone_world;
  const Vec3 x = mat_row(bone_world, 0);
  const Vec3 old_y = mat_row(bone_world, 1);
  const Vec3 old_z = mat_row(bone_world, 2);
  const Vec3 to_targets = vsub(avg, mat_pos(bone_world));
  const Vec3 projected_x = vscale(x, vdot(x, to_targets));
  const Vec3 target_y = vnorm(vsub(to_targets, projected_x), old_y);
  const float weight = source_char_bone_twist_weight(twist, weights_by_name);
  Vec3 y = vnorm(vadd(vscale(old_y, 1.0f - weight),
                      vscale(target_y, weight)),
                 old_y);
  Vec3 z = vscale(vnorm(vcross(x, y), old_z), vlen(x));
  set_mat_row(out_world, 1, y);
  set_mat_row(out_world, 2, z);
  return true;
}

static void normalize_xfm_rows(milo_scene::Xfm& xfm);

static void pre_multiply_local_rot(milo_scene::Xfm& dst,
                                   const milo_scene::Xfm& source,
                                   const float rot[3][3], float weight) {
  weight = std::clamp(weight, 0.0f, 1.0f);
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      const float solved = rot[r][0] * source.rot[0][c] +
                           rot[r][1] * source.rot[1][c] +
                           rot[r][2] * source.rot[2][c];
      dst.rot[r][c] = source.rot[r][c] * (1.0f - weight) +
                      solved * weight;
    }
  }
  normalize_xfm_rows(dst);
}

static void set_local_from_world(milo_scene::Xfm& local,
                                 const std::array<float, 16>& desired_world,
                                 const std::array<float, 16>& parent_world) {
  mat4_to_xfm(mat4_mul(desired_world, affine_inverse(parent_world)), local);
}

static std::array<float, 16> rotation_about_x_world(float angle) {
  const float ca = std::cos(angle);
  const float sa = std::sin(angle);
  return {1, 0, 0, 0, 0, ca, -sa, 0, 0, sa, ca, 0, 0, 0, 0, 1};
}

static std::array<float, 16> source_matrix_multiply_rotation(
    const std::array<float, 16>& rot,
    const std::array<float, 16>& world) {
  std::array<float, 16> out = mat4_mul(rot, world);
  out[12] = world[12];
  out[13] = world[13];
  out[14] = world[14];
  out[15] = 1.0f;
  return out;
}

static void post_rotate_axis(milo_scene::Xfm& xfm, ClipChannel::Type axis,
                             float angle) {
  const float ca = std::cos(angle);
  const float sa = std::sin(angle);
  for (int r = 0; r < 3; ++r) {
    const float x = xfm.rot[r][0];
    const float y = xfm.rot[r][1];
    const float z = xfm.rot[r][2];
    switch (axis) {
      case ClipChannel::kRotX:
        xfm.rot[r][1] = ca * y - sa * z;
        xfm.rot[r][2] = sa * y + ca * z;
        break;
      case ClipChannel::kRotY:
        xfm.rot[r][0] = ca * x + sa * z;
        xfm.rot[r][2] = -sa * x + ca * z;
        break;
      case ClipChannel::kRotZ:
        xfm.rot[r][0] = ca * x - sa * y;
        xfm.rot[r][1] = sa * x + ca * y;
        break;
      default:
        break;
    }
  }
}

static void source_rotate_about_z_vec(float v[3], float angle) {
  const float ca = std::cos(angle);
  const float sa = std::sin(angle);
  const float x = v[0];
  const float y = v[1];
  v[0] = ca * x - sa * y;
  v[1] = sa * x + ca * y;
}

SourceCharServoBoneDefaultState source_char_servo_bone_default_state() {
  return {};
}

SourceCharServoBoneSetNameStep source_char_servo_bone_set_name(
    bool dir_is_character) {
  SourceCharServoBoneSetNameStep step;
  step.assigns_character_owner = dir_is_character;
  return step;
}

SourceCharServoBoneSetClipTypeStep source_char_servo_bone_set_clip_type_step(
    bool clip_type_changed) {
  SourceCharServoBoneSetClipTypeStep step;
  step.changed = clip_type_changed;
  if (clip_type_changed) {
    step.assign_clip_type = true;
    step.clear_bones = true;
    step.stuff_bones_from_dir = true;
  }
  return step;
}

SourceCharServoBoneEnterStep source_char_servo_bone_enter(
    bool facing_pos_delta_present) {
  SourceCharServoBoneEnterStep step;
  step.move_self = facing_pos_delta_present;
  return step;
}

SourceCharServoBoneSetMoveSelfStep source_char_servo_bone_set_move_self(
    bool current_move_self,
    bool requested_move_self) {
  SourceCharServoBoneSetMoveSelfStep step;
  if (current_move_self == requested_move_self) {
    step.move_self = current_move_self;
    return step;
  }
  step.changed = true;
  step.move_self = requested_move_self;
  step.delta_changed = true;
  return step;
}

SourceCharServoBoneReallocatePlan source_char_servo_bone_reallocate_plan(
    bool found_facing_pos_delta) {
  SourceCharServoBoneReallocatePlan plan;
  plan.found_facing_pos_delta = found_facing_pos_delta;
  plan.lookup_order = {"bone_facing_delta.pos"};
  if (!found_facing_pos_delta) return plan;

  plan.lookup_facing_pos = true;
  plan.lookup_pelvis = true;
  plan.assert_facing_pos_and_pelvis = true;
  plan.lookup_facing_rot = true;
  plan.lookup_facing_rot_delta = true;
  plan.lookup_order.push_back("bone_facing.pos");
  plan.lookup_order.push_back("bone_pelvis");
  plan.lookup_order.push_back("bone_facing.rotz");
  plan.lookup_order.push_back("bone_facing_delta.rotz");
  return plan;
}

SourceCharServoBoneCopyPlan source_char_servo_bone_copy_plan() {
  SourceCharServoBoneCopyPlan plan;
  plan.copied_superclasses = {"Hmx::Object"};
  plan.copied_members = {"mMoveSelf"};
  plan.calls_set_clip_type = true;
  return plan;
}

SourceCharServoBoneLoadPlan source_char_servo_bone_load_plan(
    int32_t revision) {
  SourceCharServoBoneLoadPlan plan;
  plan.known_revision = revision >= 0 && revision <= 2;
  if (!plan.known_revision) return plan;

  plan.read_order = {"Hmx::Object"};
  if (revision > 1) {
    plan.read_order.push_back("mClipType");
  } else {
    plan.branches.push_back("mClipType defaults empty");
  }
  plan.call_order = {"SetClipType"};
  return plan;
}

SourceCharServoBoneSavePlan source_char_servo_bone_save_plan() {
  return SourceCharServoBoneSavePlan{};
}

SourceCharServoBoneHandlerPlan source_char_servo_bone_handler_plan() {
  SourceCharServoBoneHandlerPlan plan;
  plan.superclasses = {"CharPollable", "Hmx::Object"};
  plan.check = 0x16E;
  return plan;
}

SourceCharServoBonePropSyncPlan source_char_servo_bone_prop_sync_plan() {
  SourceCharServoBonePropSyncPlan plan;
  plan.set_properties = {"clip_type", "move_self"};
  plan.properties = {"delta_changed", "regulate"};
  plan.superclasses = {"CharBonesMeshes"};
  return plan;
}

SourceCharServoBoneRuntimeDumpEvidence
source_char_servo_bone_runtime_dump_evidence() {
  SourceCharServoBoneRuntimeDumpEvidence evidence;
  evidence.poll_range = "0x8038F4A0->0x8038F820";
  evidence.regulate_override_range = "0x8038FD74->0x8038FF30";
  evidence.regulate_range = "0x8038FF30->0x803901BC";
  evidence.poll_deps_range = "0x803901BC->0x803901C8";
  evidence.poll_locals = {"world", "worldPelv", "invPelv", "worldPelv",
                          "invPelv"};
  evidence.regulate_override_locals = {"names", "pred"};
  evidence.regulate_locals = {"before", "w",     "pred",  "rawDf", "df",
                              "dt",     "pos",   "delta", "dang"};
  evidence.rb2_dump_has_statement_body = false;
  evidence.latest_source_has_poll_body = false;
  evidence.safe_to_run_poll = false;
  evidence.safe_to_run_regulate = false;
  evidence.safe_to_publish_servo_motion = false;
  return evidence;
}

void source_char_servo_bone_zero_deltas(
    std::array<float, 3>& facing_pos_delta,
    float& facing_rot_delta_radians) {
  facing_pos_delta = {0.0f, 0.0f, 0.0f};
  facing_rot_delta_radians = 0.0f;
}

void source_char_servo_bone_move_to_facing(
    milo_scene::Xfm& xfm,
    const std::array<float, 3>& facing_pos,
    float facing_rot_radians) {
  if (facing_rot_radians != 0.0f) {
    post_rotate_axis(xfm, ClipChannel::kRotZ, facing_rot_radians);
    source_rotate_about_z_vec(xfm.pos, facing_rot_radians);
    normalize_xfm_rows(xfm);
  }
  xfm.pos[0] += facing_pos[0];
  xfm.pos[1] += facing_pos[1];
  xfm.pos[2] += facing_pos[2];
}

void source_char_servo_bone_move_to_delta_facing(
    milo_scene::Xfm& xfm,
    const std::array<float, 3>& facing_pos_delta,
    float facing_rot_delta_radians) {
  const float dx = facing_pos_delta[0] * xfm.rot[0][0] +
                   facing_pos_delta[1] * xfm.rot[1][0] +
                   facing_pos_delta[2] * xfm.rot[2][0];
  const float dy = facing_pos_delta[0] * xfm.rot[0][1] +
                   facing_pos_delta[1] * xfm.rot[1][1] +
                   facing_pos_delta[2] * xfm.rot[2][1];
  const float dz = facing_pos_delta[0] * xfm.rot[0][2] +
                   facing_pos_delta[1] * xfm.rot[1][2] +
                   facing_pos_delta[2] * xfm.rot[2][2];
  xfm.pos[0] += dx;
  xfm.pos[1] += dy;
  xfm.pos[2] += dz;
  if (facing_rot_delta_radians != 0.0f) {
    post_rotate_axis(xfm, ClipChannel::kRotZ, facing_rot_delta_radians);
    normalize_xfm_rows(xfm);
  }
}

static float wrap_ps2_angle(float radians) {
  constexpr float kPi = 3.1415927410125732f;
  constexpr float kTwoPi = 6.2831854820251465f;
  float wrapped = std::fmod(radians + kPi, kTwoPi);
  if (wrapped < 0.0f) wrapped += kTwoPi;
  wrapped -= kPi;
  return wrapped;
}

static float source_limit_ang(float radians) {
  constexpr float kPi = 3.14159265358979323846f;
  constexpr float kTwoPi = 6.28318530717958647692f;
  while (radians > kPi) radians -= kTwoPi;
  while (radians < -kPi) radians += kTwoPi;
  return radians;
}

SourceCharForeTwistSavePlan source_char_fore_twist_save_plan() {
  return SourceCharForeTwistSavePlan{};
}

void source_char_fore_twist_poll_deps(
    SourceCharForeTwistPollDeps& deps,
    const std::string& hand,
    const std::string& twist2,
    bool has_twist2,
    const std::string& twist2_parent) {
  deps.changed_by.push_back(hand);
  deps.change.push_back(twist2);
  if (has_twist2) deps.change.push_back(twist2_parent);
}

bool source_char_fore_twist_poll_world(
    const CharForeTwist& twist,
    bool has_hand,
    bool has_twist2,
    bool has_hand_parent,
    bool has_twist2_parent,
    const std::array<float, 16>& hand_parent_world,
    const std::array<float, 16>& hand_world,
    float hand_local_x,
    float twist2_local_x,
    SourceCharForeTwistPollWorldResult& out) {
  out = {};
  if (!has_hand || !has_twist2 || !has_hand_parent || !has_twist2_parent) {
    return false;
  }

  const float clamped =
      std::clamp(vdot(mat_row(hand_world, 2), mat_row(hand_parent_world, 1)),
                 -1.0f, 1.0f);
  const Vec3 cross =
      vcross(mat_row(hand_parent_world, 1), mat_row(hand_world, 2));
  const float clamped2 =
      std::clamp(vdot(mat_row(hand_parent_world, 0), cross), -1.0f, 1.0f);
  constexpr float kDegToRad = 0.01745329238474369049f;
  const float bias = twist.bias_degrees * kDegToRad;
  const float angle = source_limit_ang(
      twist.offset_degrees * kDegToRad + std::atan2(clamped2, clamped) + bias);
  const float final_angle = angle - bias;
  const auto rot = rotation_about_x_world(final_angle * 0.33333f);

  out.twist_parent_world =
      source_matrix_multiply_rotation(rot, hand_parent_world);
  const float ratio = twist2_local_x / hand_local_x;
  out.twist2_world =
      source_matrix_multiply_rotation(rot, out.twist_parent_world);
  const Vec3 twist_parent_pos = mat_pos(out.twist_parent_world);
  const Vec3 hand_pos = mat_pos(hand_world);
  const Vec3 interp_pos =
      vadd(vscale(twist_parent_pos, 1.0f - ratio), vscale(hand_pos, ratio));
  out.twist2_world[12] = interp_pos.x;
  out.twist2_world[13] = interp_pos.y;
  out.twist2_world[14] = interp_pos.z;
  out.twist2_world[15] = 1.0f;
  out.applied = true;
  out.source_angle_radians = angle;
  out.applied_rotation_radians = final_angle * 0.33333f;
  out.twist2_position_ratio = ratio;
  return true;
}

SourceCharUpperTwistSavePlan source_char_upper_twist_save_plan() {
  return SourceCharUpperTwistSavePlan{};
}

void source_char_upper_twist_poll_deps(
    SourceCharUpperTwistPollDeps& deps,
    const std::string& upper_arm,
    const std::string& twist1,
    const std::string& twist2) {
  deps.changed_by.push_back(upper_arm);
  deps.change.push_back(twist1);
  deps.change.push_back(twist2);
}

bool source_char_upper_twist_poll_world(
    bool has_source,
    bool has_twist1,
    bool has_twist2,
    bool has_source_parent,
    const std::array<float, 16>& source_parent_world,
    const std::array<float, 16>& source_world,
    const std::array<float, 16>& twist1_current_world,
    const std::array<float, 16>& twist2_current_world,
    SourceCharUpperTwistPollWorldResult& out) {
  out = {};
  if (!has_source || !has_twist1 || !has_twist2 || !has_source_parent) {
    return false;
  }

  float q[4] = {};
  quat_from_vec_to_vec(mat_row(source_parent_world, 0),
                       mat_row(source_world, 0), q);
  const Vec3 rotated_parent_y =
      rotate_vec_by_quat(mat_row(source_parent_world, 1), q);

  auto make_output = [&](const std::array<float, 16>& current_world,
                         float weight) {
    std::array<float, 16> output = source_world;
    set_mat_row(output, 0, mat_row(source_world, 0));
    set_mat_row(output, 1,
                vadd(vscale(rotated_parent_y, 1.0f - weight),
                     vscale(mat_row(source_world, 1), weight)));
    output[12] = current_world[12];
    output[13] = current_world[13];
    output[14] = current_world[14];
    output[15] = 1.0f;
    source_look_at_rows(output);
    return output;
  };

  out.twist1_world = make_output(twist1_current_world, 0.333f);
  out.twist2_world = make_output(twist2_current_world, 0.666f);
  out.applied = true;
  return true;
}

float source_gh2_trace_local_twist_angle(const milo_scene::Xfm& source) {
  float r0[3] = {source.rot[0][0], source.rot[0][1], source.rot[0][2]};
  float r1[3] = {source.rot[1][0], source.rot[1][1], source.rot[1][2]};
  const float r0_len =
      std::sqrt(r0[0] * r0[0] + r0[1] * r0[1] + r0[2] * r0[2]);
  const float r1_len =
      std::sqrt(r1[0] * r1[0] + r1[1] * r1[1] + r1[2] * r1[2]);
  if (r0_len > 1e-6f) {
    r0[0] /= r0_len;
    r0[1] /= r0_len;
    r0[2] /= r0_len;
  }
  if (r1_len > 1e-6f) {
    r1[0] /= r1_len;
    r1[1] /= r1_len;
    r1[2] /= r1_len;
  }

  // Accepted GH2 traces build a swing-removal quaternion from local row 0,
  // rotate local row 1 through it, then read the residual local-X twist.
  constexpr float kHalf = 0.5f;
  float w = std::sqrt(std::max((r0[0] + 1.0f) * kHalf, 0.0f));
  float qx = 0.0f;
  float qy = 0.0f;
  float qz = 0.0f;
  if (w > 1e-6f) {
    const float inv = kHalf / w;
    qy = r0[2] * inv;
    qz = -r0[1] * inv;
  } else {
    qy = 1.0f;
    w = 0.0f;
  }

  const float q_len = std::sqrt(qx * qx + qy * qy + qz * qz + w * w);
  if (q_len > 1e-6f) {
    qx /= q_len;
    qy /= q_len;
    qz /= q_len;
    w /= q_len;
  }

  const Vec3 v{r1[0], r1[1], r1[2]};
  const Vec3 qv{qx, qy, qz};
  const Vec3 t = vscale(vcross(qv, v), 2.0f);
  const Vec3 rotated = vadd(vadd(v, vscale(t, w)), vcross(qv, t));
  return std::atan2(rotated.z, rotated.y);
}

void source_gh2_trace_write_x_twist(milo_scene::Xfm& dst,
                                    const milo_scene::Xfm& basis,
                                    float angle) {
  dst = basis;
  const float ca = std::cos(angle);
  const float sa = std::sin(angle);
  for (int c = 0; c < 3; ++c) {
    dst.rot[0][c] = basis.rot[0][c];
    dst.rot[1][c] = ca * basis.rot[1][c] - sa * basis.rot[2][c];
    dst.rot[2][c] = sa * basis.rot[1][c] + ca * basis.rot[2][c];
  }
}

bool source_gh2_trace_fore_twist_poll_local(
    const CharForeTwist& twist,
    bool has_hand,
    bool has_forearm,
    bool has_twist1,
    bool has_twist2,
    const milo_scene::Xfm& hand_local,
    const milo_scene::Xfm& forearm_live_local,
    const milo_scene::Xfm& twist2_bind_local,
    SourceGh2TraceForeTwistLocalResult& out) {
  out = {};
  if (!has_hand || !has_forearm || !has_twist1 || !has_twist2) return false;

  constexpr float kDegToRad = 0.01745329238474369049f;
  float roll = source_gh2_trace_local_twist_angle(hand_local);
  roll = -wrap_ps2_angle(roll + twist.offset_degrees * kDegToRad) *
         0.3333333134651184f;
  source_gh2_trace_write_x_twist(out.twist1_local, forearm_live_local, roll);
  source_gh2_trace_write_x_twist(out.twist2_local, twist2_bind_local, roll);
  out.roll_radians = roll;
  out.applied = true;
  return true;
}

bool source_gh2_trace_upper_twist_poll_local(
    bool has_source,
    bool has_twist1,
    bool has_twist2,
    const milo_scene::Xfm& upper_live_local,
    const milo_scene::Xfm& twist2_bind_local,
    SourceGh2TraceUpperTwistLocalResult& out) {
  out = {};
  if (!has_source || !has_twist1 || !has_twist2) return false;

  const float roll = source_gh2_trace_local_twist_angle(upper_live_local);
  out.roll_radians = roll;
  source_gh2_trace_write_x_twist(
      out.twist1_local, upper_live_local, roll * out.twist1_factor);
  source_gh2_trace_write_x_twist(
      out.twist2_local, twist2_bind_local, roll * out.twist2_factor);
  out.applied = true;
  return true;
}

static void write_source_elbow_z_bend(milo_scene::Xfm& dst,
                                      const milo_scene::Xfm& base,
                                      float cos_angle,
                                      float sin_angle) {
  dst = base;
  // Handwritten C++ evidence:
  // DirtyLocalXfm().m.Set(0,0,0,-sqrted,0,0,sqrted,0,1).
  // ihatecompvir's dump names the branch locals `cosc` and `sinc`; publishing
  // the handwritten zero-X row directly collapses the native skin basis.
  const SourceCharIKHandElbowBendRows source_rows =
      source_char_ik_hand_elbow_bend_rows(cos_angle, sin_angle);
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) dst.rot[r][c] = source_rows.rows[r][c];
}

static void normalize_xfm_rows(milo_scene::Xfm& xfm) {
  for (int r = 0; r < 3; ++r) {
    float len = std::sqrt(xfm.rot[r][0] * xfm.rot[r][0] +
                          xfm.rot[r][1] * xfm.rot[r][1] +
                          xfm.rot[r][2] * xfm.rot[r][2]);
    if (len <= 1e-6f) continue;
    for (int c = 0; c < 3; ++c) xfm.rot[r][c] /= len;
  }
}

static void normalize_mat3_rows(std::array<float, 16>& m) {
  for (int r = 0; r < 3; ++r) {
    const float len = std::sqrt(m[r * 4 + 0] * m[r * 4 + 0] +
                                m[r * 4 + 1] * m[r * 4 + 1] +
                                m[r * 4 + 2] * m[r * 4 + 2]);
    if (len <= 1e-6f) continue;
    m[r * 4 + 0] /= len;
    m[r * 4 + 1] /= len;
    m[r * 4 + 2] /= len;
  }
}

static Vec3 source_transform_point(Vec3 local,
                                   const std::array<float, 16>& world) {
  return {local.x * world[0] + local.y * world[4] + local.z * world[8] +
              world[12],
          local.x * world[1] + local.y * world[5] + local.z * world[9] +
              world[13],
          local.x * world[2] + local.y * world[6] + local.z * world[10] +
              world[14]};
}

static std::array<float, 16> source_matrix3_to_mat4(const float m[9]) {
  return {m[0], m[1], m[2], 0.0f,
          m[3], m[4], m[5], 0.0f,
          m[6], m[7], m[8], 0.0f,
          0.0f, 0.0f, 0.0f, 1.0f};
}

static std::array<float, 16> source_char_hair_root_world(
    const CharHairStrand& strand, const std::array<float, 16>& root_world,
    const std::array<float, 16>& parent_world) {
  auto root_mat = source_matrix3_to_mat4(strand.root_mat);
  auto parent_rot = parent_world;
  parent_rot[12] = parent_rot[13] = parent_rot[14] = 0.0f;
  auto out = mat4_mul(root_mat, parent_rot);
  out[12] = root_world[12];
  out[13] = root_world[13];
  out[14] = root_world[14];
  out[15] = 1.0f;
  normalize_mat3_rows(out);
  return out;
}

static const std::string* source_transform_parent(
    const Character& character, const std::string& name) {
  for (const auto& bone : character.bones) {
    if (bone.name == name || channel_matches_bone(bone.name, name))
      return &bone.parent;
  }
  for (const auto& mesh : character.meshes) {
    if (mesh.name == name || channel_matches_bone(mesh.name, name))
      return &mesh.parent;
  }
  return nullptr;
}

static SourceCharHairRuntime& ensure_source_char_hair_runtime(
    Character& character, const CharHair& hair) {
  SourceCharHairRuntime& state = character.source_char_hair_runtime[hair.name];
  state.use_post_proc =
      source_char_hair_set_name_use_post_proc(true, false);
  std::vector<std::string> hair_rows;
  hair_rows.reserve(character.hairs.size());
  for (const auto& character_hair : character.hairs) {
    if (!character_hair.name.empty()) hair_rows.push_back(character_hair.name);
  }
  std::vector<std::string> dir_collides;
  dir_collides.reserve(character.collides.size());
  for (const auto& collide : character.collides) {
    if (!collide.name.empty()) dir_collides.push_back(collide.name);
  }
  const SourceBandCharacterHairHookupPlan character_hookup =
      source_band_character_hair_hookup_plan(hair_rows, dir_collides, false);
  const SourceCharHairHookupPlan default_hookup =
      source_char_hair_hookup_plan(
          character_hookup.sets_managed_hookup,
          character_hookup.collide_rows);
  state.managed_hookup = character_hookup.sets_managed_hookup;
  state.band_character_hookup =
      character_hookup.calls_overloaded_hookup_before_character_sync;
  state.default_hookup_returned_for_managed =
      default_hookup.returned_for_managed_hookup;
  state.hookup_collides = character_hookup.collide_rows;
  state.hookup_collected_from_object_dir =
      character_hookup.collects_collide_rows;
  state.hookup_overload_body_statement_visible =
      default_hookup.overload_body_statement_visible;
  state.legacy_inline_point_count = 0;
  for (const auto& strand : hair.strands) {
    for (const auto& point : strand.points) {
      if (source_char_hair_point_collide_resolution(point)
              .has_legacy_inline_rows) {
        ++state.legacy_inline_point_count;
      }
    }
  }
  if (state.strands.size() != hair.strands.size()) {
    state.strands.clear();
    state.strands.resize(hair.strands.size());
    state.initialized = false;
    state.reset = 1;
  }
  for (size_t si = 0; si < hair.strands.size(); ++si) {
    auto& runtime_strand = state.strands[si];
    if (runtime_strand.points.size() != hair.strands[si].points.size()) {
      runtime_strand.points.clear();
      runtime_strand.points.resize(hair.strands[si].points.size());
      state.initialized = false;
      state.reset = 1;
    }
    for (size_t pi = 0; pi < hair.strands[si].points.size(); ++pi) {
      auto& runtime_point = runtime_strand.points[pi];
      if (runtime_point.initialized) continue;
      array3_from_vec(runtime_point.pos,
                      vec_from_array3(hair.strands[si].points[pi].pos));
      runtime_point.force = {0.0f, 0.0f, 0.0f};
      runtime_point.last_friction = {0.0f, 0.0f, 0.0f};
      runtime_point.last_z = {0.0f, 0.0f, 0.0f};
      runtime_point.initialized = true;
    }
  }
  if (!state.initialized) {
    state.initialized = true;
    state.reset = 1;
  }
  return state;
}

static void source_char_hair_do_reset(Character& character, const CharHair& hair,
                                      SourceCharHairRuntime& state,
                                      int reset_count);

static int source_char_hair_resolved_collide_count(
    const CharHairPoint& point, const SourceCharHairRuntime& state) {
  if (!state.hookup_overload_body_statement_visible) return 0;
  return source_char_hair_point_collide_resolution(point)
             .resolved_runtime_collides
             ? 1
             : 0;
}

static int source_char_hair_simulate_internal(Character& character,
                                              const CharHair& hair,
                                              SourceCharHairRuntime& state,
                                              float fps, float inertia,
                                              float friction,
                                              bool force_simulate = false) {
  if ((!force_simulate && !hair.simulate) || hair.strands.empty()) return 0;
  const float safe_fps = fps > 0.0f ? fps : 60.0f;
  const float sixty_over = 60.0f / safe_fps;
  const float f19 = (1.0f / safe_fps) * sixty_over;
  const float powed =
      std::pow(1.0f - hair.stiffness, sixty_over * sixty_over);
  Vec3 wind_gravity{0.0f, 0.0f, hair.gravity * f19 * -3.858268f};
  int write_count = 0;

  for (size_t si = 0; si < hair.strands.size(); ++si) {
    const auto& strand = hair.strands[si];
    if (strand.root.empty()) continue;
    const std::string* parent_name =
        source_transform_parent(character, strand.root);
    if (!parent_name || parent_name->empty()) continue;

    std::array<float, 16> root_world{};
    std::array<float, 16> parent_world{};
    if (!transform_local_chain_world(character, strand.root, root_world) ||
        !transform_local_chain_world(character, *parent_name, parent_world)) {
      continue;
    }

    std::array<float, 16> t100 =
        source_char_hair_root_world(strand, root_world, parent_world);
    auto& runtime_strand = state.strands[si];
    if (runtime_strand.points.size() != strand.points.size()) continue;
    auto& next_runtime_strand =
        state.strands[(si + 1) % std::max<size_t>(state.strands.size(), 1)];
    const auto& next_strand =
        hair.strands[(si + 1) % std::max<size_t>(hair.strands.size(), 1)];

    for (size_t pi = 0; pi < strand.points.size(); ++pi) {
      const auto& point = strand.points[pi];
      auto& runtime_point = runtime_strand.points[pi];
      Vec3 point_pos = vec_from_array3(runtime_point.pos);
      const Vec3 v140 = point_pos;
      point_pos = vadd(point_pos, vec_from_array3(runtime_point.force));
      point_pos = vadd(point_pos, wind_gravity);

      if (point.side_length >= 0.0f && pi < next_strand.points.size() &&
          pi < next_runtime_strand.points.size()) {
        auto& next_runtime_point = next_runtime_strand.points[pi];
        Vec3 next_pos = vec_from_array3(next_runtime_point.pos);
        Vec3 v_res = vsub(point_pos, next_pos);
        const float len_sq = vdot(v_res, v_res);
        const float min_len = point.side_length - hair.min_slack;
        const float min_len_sq = min_len * min_len;
        if (len_sq < min_len_sq) {
          v_res = vscale(v_res, min_len_sq / (min_len_sq + len_sq) - 0.5f);
          point_pos = vadd(point_pos, v_res);
          next_pos = vsub(next_pos, v_res);
          array3_from_vec(next_runtime_point.pos, next_pos);
        } else {
          const float max_len = point.side_length + hair.max_slack;
          const float max_len_sq = max_len * max_len;
          if (max_len > max_len_sq) {
            v_res =
                vscale(v_res, max_len_sq / (max_len_sq + len_sq) - 0.5f);
            point_pos = vadd(point_pos, v_res);
            next_pos = vsub(next_pos, v_res);
            array3_from_vec(next_runtime_point.pos, next_pos);
          }
        }
      }

      Vec3 m128_y = vsub(point_pos, mat_pos(t100));
      const float len_sq = std::max(vdot(m128_y, m128_y), 1.0e-8f);
      const float rsa = 1.0f / std::sqrt(len_sq);
      const float rsalen = point.length * rsa - 1.0f;
      if (pi > 0) {
        auto& prev_runtime_point = runtime_strand.points[pi - 1];
        const Vec3 prev_force = vec_from_array3(prev_runtime_point.force);
        array3_from_vec(prev_runtime_point.force,
                        vadd(prev_force,
                             vscale(m128_y, -sixty_over * 0.5f * rsalen)));
      }
      point_pos = vadd(point_pos, vscale(m128_y, rsalen));
      array3_from_vec(runtime_point.pos, point_pos);

      const Vec3 v158 =
          vadd(mat_pos(t100), vscale(mat_row(t100, 1), point.length));
      Vec3 m128_z =
          vadd(vscale(vec_from_array3(runtime_point.last_z),
                      1.0f - hair.torsion),
               vscale(mat_row(t100, 2), hair.torsion));

      const SourceCharHairWritebackGate writeback_gate =
          source_char_hair_writeback_gate(
              !point.bone.empty(),
              source_char_hair_resolved_collide_count(point, state));
      if (writeback_gate.enters_collision_branch) {
        Vec3 y = vscale(m128_y, rsa);
        Vec3 x = vnorm(vcross(y, m128_z), mat_row(t100, 0));
        Vec3 z = vcross(x, y);
        set_mat_row(t100, 0, x);
        set_mat_row(t100, 1, y);
        set_mat_row(t100, 2, z);
        t100[12] = point_pos.x;
        t100[13] = point_pos.y;
        t100[14] = point_pos.z;
        normalize_mat3_rows(t100);
        array3_from_vec(runtime_point.last_z, mat_row(t100, 2));
        if (writeback_gate.may_set_world_xfm) {
          character.runtime_world_overrides[point.bone] = t100;
          ++write_count;
        }
        Vec3 force = vsub(v158, point_pos);
        Vec3 v170 = vsub(vec_from_array3(runtime_point.last_friction), force);
        array3_from_vec(runtime_point.last_friction, force);
        force = vscale(force, 1.0f - powed);
        force = vadd(force, vscale(v170, -friction));
        Vec3 v17c = vsub(point_pos, v140);
        force = vadd(force, vscale(v17c, inertia));
        array3_from_vec(runtime_point.force, force);
      }
    }
  }

  return write_count;
}

static int source_char_hair_simulate_loops(Character& character,
                                           const CharHair& hair,
                                           SourceCharHairRuntime& state,
                                           int count, float fps,
                                           float inertia, float friction,
                                           bool force_simulate = false) {
  if ((!force_simulate && !hair.simulate) || hair.strands.empty()) return 0;
  int write_count = 0;
  for (int i = 0; i < count; ++i) {
    write_count += source_char_hair_simulate_internal(
        character, hair, state, fps, inertia, friction, force_simulate);
  }
  return write_count;
}

int source_char_hair_freeze_pose_raw(Character& character, CharHair& hair,
                                     SourceCharHairRuntime& state) {
  int write_count = 0;
  for (size_t si = 0; si < hair.strands.size(); ++si) {
    auto& strand = hair.strands[si];
    if (strand.root.empty()) continue;
    const std::string* parent_name =
        source_transform_parent(character, strand.root);
    if (!parent_name || parent_name->empty()) continue;
    if (si >= state.strands.size()) continue;

    std::array<float, 16> parent_world{};
    if (!transform_local_chain_world(character, *parent_name, parent_world)) {
      continue;
    }
    const std::array<float, 16> parent_inverse = affine_inverse(parent_world);
    auto& runtime_strand = state.strands[si];
    const size_t point_count =
        std::min(strand.points.size(), runtime_strand.points.size());
    for (size_t pi = 0; pi < point_count; ++pi) {
      const Vec3 local = source_transform_point(
          vec_from_array3(runtime_strand.points[pi].pos), parent_inverse);
      strand.points[pi].unk5c[0] = local.x;
      strand.points[pi].unk5c[1] = local.y;
      strand.points[pi].unk5c[2] = local.z;
      ++write_count;
    }
  }
  return write_count;
}

static void source_char_hair_do_reset(Character& character, const CharHair& hair,
                                      SourceCharHairRuntime& state,
                                      int reset_count) {
  for (size_t si = 0; si < hair.strands.size(); ++si) {
    const auto& strand = hair.strands[si];
    if (strand.root.empty()) continue;
    const std::string* parent_name =
        source_transform_parent(character, strand.root);
    if (!parent_name || parent_name->empty()) continue;
    if (si >= state.strands.size()) continue;
    auto& runtime_strand = state.strands[si];

    std::array<float, 16> root_world{};
    std::array<float, 16> parent_world{};
    if (!transform_local_chain_world(character, strand.root, root_world) ||
        !transform_local_chain_world(character, *parent_name, parent_world)) {
      continue;
    }

    Vec3 v80 = mat_pos(root_world);
    Vec3 v8c = mat_row(root_world, 0);
    for (size_t pi = 0; pi < strand.points.size() &&
                        pi < runtime_strand.points.size();
         ++pi) {
      const auto& point = strand.points[pi];
      auto& runtime_point = runtime_strand.points[pi];
      const Vec3 pos = source_transform_point(vec_from_array3(point.unk5c),
                                              parent_world);
      array3_from_vec(runtime_point.pos, pos);
      const Vec3 v98 = vsub(pos, v80);
      v80 = pos;
      const Vec3 last_z = vnorm(vcross(v8c, v98), {0.0f, 0.0f, 1.0f});
      array3_from_vec(runtime_point.last_z, last_z);
      v8c = vcross(v98, last_z);
      runtime_point.force = {0.0f, 0.0f, 0.0f};
      runtime_point.last_friction = {0.0f, 0.0f, 0.0f};
    }
  }

  source_char_hair_simulate_loops(
      character, hair, state, std::max(reset_count, 0),
      source_char_hair_get_fps(state.use_post_proc, 0.0f), 0.0f, 0.0f,
      true);
  state.reset = 0;
}

static void log_debug_xfm_row(const char* tag, const char* name,
                              const milo_scene::Xfm& local,
                              const std::array<float, 16>& world) {
  const Vec3 wp = mat_pos(world);
  std::fprintf(stderr,
               "[%s] %s local_pos=[%.3f %.3f %.3f] "
               "local_r0=[%.4f %.4f %.4f] local_r1=[%.4f %.4f %.4f] "
               "local_r2=[%.4f %.4f %.4f] world_pos=[%.3f %.3f %.3f] "
               "world_r0=[%.4f %.4f %.4f] world_r1=[%.4f %.4f %.4f] "
               "world_r2=[%.4f %.4f %.4f]\n",
               tag, name, local.pos[0], local.pos[1], local.pos[2],
               local.rot[0][0], local.rot[0][1], local.rot[0][2],
               local.rot[1][0], local.rot[1][1], local.rot[1][2],
               local.rot[2][0], local.rot[2][1], local.rot[2][2],
               wp.x, wp.y, wp.z, world[0], world[1], world[2],
               world[4], world[5], world[6], world[8], world[9], world[10]);
}

static void log_debug_xfm_row_short(const char* tag, const char* name,
                                    const milo_scene::Xfm& local) {
  std::fprintf(stderr, "[%s-pos] %s pos=[%.5f %.5f %.5f]\n", tag, name,
               local.pos[0], local.pos[1], local.pos[2]);
  std::fprintf(stderr, "[%s-r0] %s r0=[%.5f %.5f %.5f]\n", tag, name,
               local.rot[0][0], local.rot[0][1], local.rot[0][2]);
  std::fprintf(stderr, "[%s-r1] %s r1=[%.5f %.5f %.5f]\n", tag, name,
               local.rot[1][0], local.rot[1][1], local.rot[1][2]);
  std::fprintf(stderr, "[%s-r2] %s r2=[%.5f %.5f %.5f]\n", tag, name,
               local.rot[2][0], local.rot[2][1], local.rot[2][2]);
}

static void log_debug_world_row(const char* tag, const char* name,
                                const std::array<float, 16>& world) {
  const Vec3 wp = mat_pos(world);
  std::fprintf(stderr,
               "[%s] %s world_pos=[%.3f %.3f %.3f] "
               "world_r0=[%.4f %.4f %.4f] world_r1=[%.4f %.4f %.4f] "
               "world_r2=[%.4f %.4f %.4f]\n",
               tag, name, wp.x, wp.y, wp.z, world[0], world[1], world[2],
               world[4], world[5], world[6], world[8], world[9], world[10]);
}

static bool apply_source_fore_twist(Character& character,
                                    const std::vector<milo_scene::Xfm>& bind_bones,
                                    const CharForeTwist& ft) {
  (void)bind_bones;
  const int hand_i = find_bone_index(character, ft.hand);
  const int twist2_i = find_bone_index(character, ft.twist2);
  if (hand_i < 0 || twist2_i < 0) return false;
  auto& hand = character.bones[static_cast<size_t>(hand_i)];
  auto& twist2 = character.bones[static_cast<size_t>(twist2_i)];
  if (hand.parent.empty() || twist2.parent.empty()) return false;
  const int parent_i = find_bone_index(character, hand.parent);
  const int twist1_i = find_bone_index(character, twist2.parent);
  if (parent_i < 0 || twist1_i < 0) return false;
  auto& forearm = character.bones[static_cast<size_t>(parent_i)];
  auto& twist1 = character.bones[static_cast<size_t>(twist1_i)];
  if (twist1.parent.empty()) return false;
  const int twist1_parent_i = find_bone_index(character, twist1.parent);
  if (twist1_parent_i < 0) return false;
  const float hand_local_x = hand.local.pos[0];
  if (std::fabs(hand_local_x) <= 1.0e-6f) return false;

  std::array<float, 16> hand_parent_world{};
  std::array<float, 16> hand_world{};
  std::array<float, 16> twist1_parent_world{};
  if (!transform_local_chain_world(character, hand.parent, hand_parent_world) ||
      !transform_local_chain_world(character, hand.name, hand_world) ||
      !transform_local_chain_world(character, twist1.parent,
                                   twist1_parent_world)) {
    return false;
  }

  SourceCharForeTwistPollWorldResult twist_result;
  if (!source_char_fore_twist_poll_world(
          ft, true, true, true, true, hand_parent_world, hand_world,
          hand_local_x, twist2.local.pos[0], twist_result)) {
    return false;
  }
  set_local_from_world(twist1.local, twist_result.twist_parent_world,
                       twist1_parent_world);
  set_local_from_world(twist2.local, twist_result.twist2_world,
                       twist_result.twist_parent_world);

  if (debug_ik_enabled()) {
    std::fprintf(stderr,
                 "[twist-fore-source] %s hand=%s fore=%s twist1=%s "
                 "twist2=%s offset=%.3f bias=%.3f angle=%.5f applied=%.5f "
                 "ratio=%.5f\n",
                 ft.name.c_str(), ft.hand.c_str(), forearm.name.c_str(),
                 twist1.name.c_str(), ft.twist2.c_str(), ft.offset_degrees,
                 ft.bias_degrees, twist_result.source_angle_radians,
                 twist_result.applied_rotation_radians,
                 twist_result.twist2_position_ratio);
    log_debug_xfm_row_short("twist-fore-source", twist1.name.c_str(),
                            twist1.local);
    log_debug_xfm_row_short("twist-fore-source", twist2.name.c_str(),
                            twist2.local);
  }
  return true;
}

static void apply_source_upper_twists(
    Character& character, const std::vector<milo_scene::Xfm>& bind_bones) {
  (void)bind_bones;
  for (const auto& ut : character.upper_twists) {
    const int upper_i = find_bone_index(character, ut.upper_arm);
    const int twist1_i = find_bone_index(character, ut.twist1);
    const int twist2_i = find_bone_index(character, ut.twist2);
    if (upper_i < 0 || twist1_i < 0 || twist2_i < 0) continue;
    auto& upper = character.bones[static_cast<size_t>(upper_i)];
    auto& twist1 = character.bones[static_cast<size_t>(twist1_i)];
    auto& twist2 = character.bones[static_cast<size_t>(twist2_i)];
    if (upper.parent.empty()) continue;
    if (twist1.parent.empty() || twist2.parent.empty()) continue;

    std::array<float, 16> source_parent_world{};
    std::array<float, 16> source_world{};
    std::array<float, 16> twist1_world{};
    std::array<float, 16> twist2_world_before_twist1{};
    std::array<float, 16> twist1_parent_world{};
    if (!transform_local_chain_world(character, upper.parent,
                                     source_parent_world) ||
        !transform_local_chain_world(character, upper.name, source_world) ||
        !transform_local_chain_world(character, twist1.name, twist1_world) ||
        !transform_local_chain_world(character, twist2.name,
                                     twist2_world_before_twist1) ||
        !transform_local_chain_world(character, twist1.parent,
                                     twist1_parent_world)) {
      continue;
    }

    SourceCharUpperTwistPollWorldResult twist_result;
    if (!source_char_upper_twist_poll_world(
            true, true, true, true, source_parent_world, source_world,
            twist1_world, twist2_world_before_twist1, twist_result)) {
      continue;
    }

    set_local_from_world(twist1.local, twist_result.twist1_world,
                         twist1_parent_world);
    std::array<float, 16> twist2_world_after_twist1{};
    if (!transform_local_chain_world(character, twist2.name,
                                     twist2_world_after_twist1)) {
      continue;
    }
    SourceCharUpperTwistPollWorldResult sequenced_twist_result;
    if (!source_char_upper_twist_poll_world(
            true, true, true, true, source_parent_world, source_world,
            twist1_world, twist2_world_after_twist1,
            sequenced_twist_result)) {
      continue;
    }
    std::array<float, 16> twist2_parent_world{};
    if (!transform_local_chain_world(character, twist2.parent,
                                     twist2_parent_world)) {
      continue;
    }
    set_local_from_world(twist2.local, sequenced_twist_result.twist2_world,
                         twist2_parent_world);
    if (debug_ik_enabled()) {
      std::fprintf(stderr,
                   "[twist-upper-source] %s upper=%s twist1=%s twist2=%s\n",
                   ut.name.c_str(), ut.upper_arm.c_str(), ut.twist1.c_str(),
                   ut.twist2.c_str());
      log_debug_xfm_row_short("twist-upper-source", twist1.name.c_str(),
                              twist1.local);
      log_debug_xfm_row_short("twist-upper-source", twist2.name.c_str(),
                              twist2.local);
    }
  }
}

static void apply_source_pos_constraints(Character& character) {
  for (const auto& constraint : character.pos_constraints) {
    if (constraint.source.empty() || constraint.targets.empty()) continue;
    std::array<float, 16> source_world{};
    if (!transform_local_chain_world(character, constraint.source,
                                     source_world)) {
      continue;
    }
    const Vec3 source_pos = mat_pos(source_world);
    for (const auto& target : constraint.targets) {
      if (target.empty()) continue;
      std::array<float, 16> target_world{};
      if (!transform_local_chain_world(character, target, target_world)) {
        continue;
      }
      const std::array<float, 3> box_min = {constraint.box_min[0],
                                            constraint.box_min[1],
                                            constraint.box_min[2]};
      const std::array<float, 3> box_max = {constraint.box_max[0],
                                            constraint.box_max[1],
                                            constraint.box_max[2]};
      const Vec3 target_pos = vec_from_array3(
          source_char_pos_constraint_target_position(
              array3_from_vec(source_pos), array3_from_vec(mat_pos(target_world)),
              box_min, box_max));
      const Vec3 delta = vsub(target_pos, source_pos);
      target_world[12] = target_pos.x;
      target_world[13] = target_pos.y;
      target_world[14] = target_pos.z;
      character.runtime_world_overrides[target] = target_world;
      if (controller_audit_enabled()) {
        std::fprintf(stderr,
                     "[posconstraint-source] %s source=%s target=%s "
                     "world=(%.3f %.3f %.3f) delta=(%.3f %.3f %.3f) "
                     "boxMin=(%.3f %.3f %.3f) boxMax=(%.3f %.3f %.3f)\n",
                     constraint.name.c_str(), constraint.source.c_str(),
                     target.c_str(), target_pos.x, target_pos.y, target_pos.z,
                     delta.x, delta.y, delta.z, constraint.box_min[0],
                     constraint.box_min[1], constraint.box_min[2],
                     constraint.box_max[0], constraint.box_max[1],
                     constraint.box_max[2]);
      }
    }
  }
}

SourceCharWeightableLoadPlan source_char_weightable_load_plan(
    int32_t revision) {
  SourceCharWeightableLoadPlan plan;
  plan.revision_supported = revision >= 0 && revision <= 2;
  if (!plan.revision_supported) return plan;
  plan.read_order.push_back("mWeight");
  if (revision > 1) plan.read_order.push_back("mWeightOwner");
  return plan;
}

SourceCharWeightableCopyPlan source_char_weightable_copy_plan() {
  SourceCharWeightableCopyPlan plan;
  plan.shallow_actions = {"SetWeightOwner(source.mWeightOwner)"};
  plan.deep_actions = {"SetWeightOwner(this)",
                       "mWeight=source.mWeightOwner->mWeight"};
  return plan;
}

SourceCharWeightableHandlerPlan source_char_weightable_handler_plan() {
  SourceCharWeightableHandlerPlan plan;
  plan.superclasses = {"Hmx::Object"};
  plan.check = 0x43;
  return plan;
}

SourceCharWeightablePropSyncPlan source_char_weightable_prop_sync_plan() {
  SourceCharWeightablePropSyncPlan plan;
  plan.properties = {"weight", "weight_owner"};
  plan.set_actions = {"weight:SetWeight(_val.Float(0))",
                      "weight_owner:SetWeightOwner(_val.Obj<CharWeightable>(0))"};
  plan.get_actions = {"weight:DataNode(mWeight)",
                      "weight_owner:DataNode(mWeightOwner)"};
  plan.blocked_ops = {"weight:op0x40 returns false",
                      "weight_owner:op0x40 returns false"};
  return plan;
}

SourceCharWeightableSavePlan source_char_weightable_save_plan() {
  return SourceCharWeightableSavePlan{};
}

SourceCharWeightableState source_char_weightable_default_state(
    const std::string& name) {
  SourceCharWeightableState state;
  state.name = name;
  state.weight = 1.0f;
  state.weight_owner = name;
  return state;
}

void source_char_weightable_set_weight(SourceCharWeightableState& state,
                                       float weight) {
  state.weight = weight;
}

void source_char_weightable_set_weight_owner(SourceCharWeightableState& state,
                                             const std::string& weight_owner) {
  state.weight_owner = weight_owner.empty() ? state.name : weight_owner;
}

void source_char_weightable_replace(SourceCharWeightableState& state,
                                    const std::string& old_owner,
                                    const std::string& new_owner,
                                    bool new_owner_is_weightable) {
  if (state.weight_owner == old_owner) {
    state.weight_owner = new_owner_is_weightable ? new_owner : std::string{};
  }
  if (state.weight_owner.empty()) state.weight_owner = state.name;
}

void source_char_weightable_copy(SourceCharWeightableState& dest,
                                 const SourceCharWeightableState& source,
                                 bool shallow_copy,
                                 float source_owner_weight) {
  if (shallow_copy) {
    source_char_weightable_set_weight_owner(dest, source.weight_owner);
  } else {
    source_char_weightable_set_weight_owner(dest, dest.name);
    dest.weight = source_owner_weight;
  }
}

float source_char_weightable_weight(
    const CharWeightSetter& setter,
    const std::unordered_map<std::string, float>& weights_by_name) {
  if (!setter.weight_owner.empty()) {
    const auto owner = weights_by_name.find(setter.weight_owner);
    if (owner != weights_by_name.end()) return owner->second;
  }
  return setter.weight;
}

namespace {

float source_weightable_state_weight(
    const SourceCharWeightableState& state,
    const std::unordered_map<std::string, float>& weights_by_name) {
  if (!state.weight_owner.empty()) {
    const auto owner = weights_by_name.find(state.weight_owner);
    if (owner != weights_by_name.end()) return owner->second;
  }
  return state.weight;
}

}  // namespace

SourceCharMirrorState source_char_mirror_default_state(
    const std::string& name) {
  SourceCharMirrorState state;
  state.weightable = source_char_weightable_default_state(name);
  return state;
}

SourceCharMirrorPollResult source_char_mirror_poll(
    const SourceCharMirrorState& state,
    const std::unordered_map<std::string, float>& weights_by_name) {
  SourceCharMirrorPollResult result;
  result.weight = source_weightable_state_weight(state.weightable,
                                                 weights_by_name);
  result.weight_zero = result.weight == 0.0f;
  result.bones_empty = state.bones_total_size == 0;
  if (!result.weight_zero && !result.bones_empty) {
    result.scale_down = true;
    result.scale_down_weight = 1.0f - result.weight;
    result.servo = state.servo;
  }
  return result;
}

SourceCharMirrorSetServoResult source_char_mirror_set_servo(
    SourceCharMirrorState& state,
    const std::string& servo) {
  SourceCharMirrorSetServoResult result;
  if (servo != state.servo) {
    state.servo = servo;
    result.changed = true;
    result.synced_bones = true;
  }
  return result;
}

SourceCharMirrorSetServoResult source_char_mirror_set_mirror_servo(
    SourceCharMirrorState& state,
    const std::string& mirror_servo) {
  SourceCharMirrorSetServoResult result;
  if (mirror_servo != state.mirror_servo) {
    state.mirror_servo = mirror_servo;
    result.changed = true;
    result.synced_bones = true;
  }
  return result;
}

void source_char_mirror_poll_deps(SourceCharMirrorPollDeps& deps,
                                  const SourceCharMirrorState& state) {
  deps.change.push_back(state.servo);
}

SourceCharMirrorLoadSteps source_char_mirror_load_steps() {
  SourceCharMirrorLoadSteps steps;
  steps.load_hmx_object = true;
  steps.load_weightable = true;
  steps.load_mirror_servo = true;
  steps.load_servo = true;
  steps.sync_bones = true;
  return steps;
}

SourceCharMirrorCopyResult source_char_mirror_copy(
    SourceCharMirrorState& dest,
    const SourceCharMirrorState& source,
    bool shallow_copy,
    float source_owner_weight) {
  SourceCharMirrorCopyResult result;
  result.copy_hmx_object = true;
  result.copy_weightable = true;
  source_char_weightable_copy(dest.weightable, source.weightable, shallow_copy,
                              source_owner_weight);
  result.set_mirror_servo =
      source_char_mirror_set_mirror_servo(dest, source.mirror_servo);
  result.set_servo = source_char_mirror_set_servo(dest, source.servo);
  return result;
}

SourceCharMirrorSavePlan source_char_mirror_save_plan() {
  return SourceCharMirrorSavePlan{};
}

bool source_char_weight_setter_poll(
    const CharWeightSetter& setter,
    const std::unordered_map<std::string, float>& weights_by_name,
    float delta_beats,
    float& out_weight) {
  if (!setter.driver.empty()) {
    return false;
  }
  return source_char_weight_setter_poll_with_driver_result(
      setter, weights_by_name, delta_beats, std::nullopt, out_weight);
}

bool source_char_weight_setter_poll_with_driver_result(
    const CharWeightSetter& setter,
    const std::unordered_map<std::string, float>& weights_by_name,
    float delta_beats,
    std::optional<float> driver_evaluate_flags,
    float& out_weight) {
  float base_weight = setter.base_weight;
  if (!setter.driver.empty()) {
    if (!driver_evaluate_flags) return false;
    base_weight = setter.scale * *driver_evaluate_flags + setter.offset;
  } else if (!setter.base.empty()) {
    const auto base = weights_by_name.find(setter.base);
    if (base == weights_by_name.end()) return false;
    base_weight = setter.scale * base->second + setter.offset;
  }

  for (const auto& min_name : setter.min_weights) {
    const auto min_weight = weights_by_name.find(min_name);
    if (min_weight == weights_by_name.end()) return false;
    base_weight = std::min(base_weight, min_weight->second);
  }
  for (const auto& max_name : setter.max_weights) {
    const auto max_weight = weights_by_name.find(max_name);
    if (max_weight == weights_by_name.end()) return false;
    base_weight = std::max(base_weight, max_weight->second);
  }

  const float current = source_char_weightable_weight(setter, weights_by_name);
  if (base_weight == current) {
    out_weight = current;
    return true;
  }
  if (setter.beats_per_weight <= 0.0f) {
    out_weight = base_weight;
    return true;
  }

  const float step = delta_beats / setter.beats_per_weight;
  if (step > 0.0f) {
    const float delta = std::clamp(base_weight - current, -step, step);
    out_weight = current + delta;
  } else {
    out_weight = current;
  }
  return true;
}

namespace {

template <typename EvaluateFlags>
SourceCharMainDriverHandWeights source_char_main_driver_hand_weights_impl(
    const Character& character, float fallback_left, float fallback_right,
    EvaluateFlags evaluate_flags) {
  SourceCharMainDriverHandWeights result;
  result.left = fallback_left;
  result.right = fallback_right;
  std::unordered_map<std::string, float> source_weight_inputs;
  std::unordered_set<uint32_t> main_driver_flags_seen;
  for (const auto& setter : character.weight_setters) {
    if (setter.driver != "main.drv" || setter.flags == 0) continue;
    const float flag_weight = evaluate_flags(setter.flags);
    if (main_driver_flags_seen.insert(setter.flags).second) {
      result.driver_flags.push_back({setter.driver, setter.flags, flag_weight});
    }

    float owner_weight = setter.weight;
    if (!source_char_weight_setter_poll_with_driver_result(
            setter, source_weight_inputs, 0.0f, flag_weight, owner_weight)) {
      continue;
    }
    const bool left_owner =
        setter.name == "left.weight" || setter.weight_owner == "left.weight";
    const bool right_owner =
        setter.name == "right.weight" || setter.weight_owner == "right.weight";
    if (left_owner) {
      result.left = std::clamp(owner_weight, 0.0f, 1.0f);
      result.left_source = true;
      source_weight_inputs["left.weight"] = result.left;
    }
    if (right_owner) {
      result.right = std::clamp(owner_weight, 0.0f, 1.0f);
      result.right_source = true;
      source_weight_inputs["right.weight"] = result.right;
    }
  }
  return result;
}

}  // namespace

SourceCharMainDriverHandWeights
source_char_main_driver_hand_weights_from_clip_flags(
    const Character& character, uint32_t clip_flags, float fallback_left,
    float fallback_right) {
  return source_char_main_driver_hand_weights_impl(
      character, fallback_left, fallback_right,
      [clip_flags](uint32_t flags) {
        return source_char_driver_evaluate_flags_from_clip_flags(clip_flags,
                                                                 flags);
      });
}

SourceCharMainDriverHandWeights
source_char_main_driver_hand_weights_from_player(
    const Character& character, const CharClipPlayer* player,
    float fallback_left, float fallback_right) {
  if (player == nullptr || !player->active()) {
    SourceCharMainDriverHandWeights result;
    result.left = fallback_left;
    result.right = fallback_right;
    return result;
  }
  return source_char_main_driver_hand_weights_impl(
      character, fallback_left, fallback_right,
      [player](uint32_t flags) { return player->evaluate_flags(flags); });
}

SourceCharWeightSetterState source_char_weight_setter_default_state(
    const std::string& name) {
  SourceCharWeightSetterState state;
  state.weightable = source_char_weightable_default_state(name);
  state.has_base = false;
  state.has_driver = false;
  state.min_weight_count = 0;
  state.max_weight_count = 0;
  state.flags = 0;
  state.offset = 0.0f;
  state.scale = 1.0f;
  state.base_weight = 0.0f;
  state.beats_per_weight = 0.0f;
  return state;
}

void source_char_weight_setter_set_weight(SourceCharWeightSetterState& state,
                                          float weight) {
  state.base_weight = weight;
  state.weightable.weight = weight;
}

SourceCharWeightSetterLoadPlan source_char_weight_setter_load_plan(
    int32_t revision) {
  SourceCharWeightSetterLoadPlan plan;
  plan.revision_supported = revision >= 0 && revision <= 9;
  if (!plan.revision_supported) return plan;

  plan.read_order.push_back("Hmx::Object");
  if (revision > 1) plan.read_order.push_back("CharWeightable");
  plan.read_order.push_back("mDriver");
  plan.read_order.push_back("mFlags");

  if (revision < 3) {
    plan.branches.push_back("mScale=1.0");
    plan.branches.push_back("mOffset=0.0");
  } else if (revision < 4) {
    plan.read_order.push_back("legacyInvertBool");
    plan.branches.push_back("legacy bool true -> mScale=-1.0,mOffset=1.0");
    plan.branches.push_back("legacy bool false -> mScale=1.0,mOffset=0.0");
  } else {
    plan.read_order.push_back("mOffset");
    plan.read_order.push_back("mScale");
  }

  if (revision < 2) {
    plan.read_order.push_back("legacyWeightableOwnerList");
    plan.branches.push_back("legacy owner list assigns SetWeightOwner(this)");
  }

  if (revision > 4) {
    plan.read_order.push_back("mBaseWeight");
    plan.read_order.push_back("mBeatsPerWeight");
  } else {
    plan.branches.push_back("mBaseWeight=mWeight");
    plan.branches.push_back("mBeatsPerWeight=0.0");
  }

  if (revision > 5) plan.read_order.push_back("mBase");
  if (revision > 8) {
    plan.read_order.push_back("mMinWeights");
    plan.read_order.push_back("mMaxWeights");
  } else {
    if (revision > 6) plan.read_order.push_back("legacyMinWeight");
    if (revision > 7) plan.read_order.push_back("legacyMaxWeight");
  }

  return plan;
}

SourceCharWeightSetterCopyPlan source_char_weight_setter_copy_plan() {
  SourceCharWeightSetterCopyPlan plan;
  plan.copied_superclasses = {"Hmx::Object", "CharWeightable"};
  plan.copied_members = {"mDriver",      "mFlags",         "mBase",
                         "mOffset",      "mScale",         "mBaseWeight",
                         "mBeatsPerWeight", "mMinWeights", "mMaxWeights"};
  return plan;
}

SourceCharWeightSetterHandlerPlan source_char_weight_setter_handler_plan() {
  SourceCharWeightSetterHandlerPlan plan;
  plan.superclasses = {"Hmx::Object"};
  plan.check = 0xF4;
  return plan;
}

SourceCharWeightSetterPropSyncPlan
source_char_weight_setter_prop_sync_plan() {
  SourceCharWeightSetterPropSyncPlan plan;
  plan.properties = {"driver",          "flags",       "base",
                     "offset",          "scale",       "base_weight",
                     "beats_per_weight", "min_weights", "max_weights"};
  plan.superclasses = {"CharWeightable"};
  return plan;
}

SourceCharWeightSetterSavePlan source_char_weight_setter_save_plan() {
  return SourceCharWeightSetterSavePlan{};
}

SourceCharWeightSetterRuntimeDumpEvidence
source_char_weight_setter_runtime_dump_evidence() {
  SourceCharWeightSetterRuntimeDumpEvidence evidence;
  evidence.poll_range = "0x8039D368->0x8039D500";
  evidence.poll_deps_range = "0x8039D500->0x8039D73C";
  evidence.load_range = "0x8039D83C->0x8039DC40";
  evidence.copy_range = "0x8039DC40->0x8039DDA0";
  evidence.poll_locals = {"delta"};
  evidence.poll_deps_locals = {"it", "w"};
  evidence.load_locals = {"w", "it"};
  evidence.rb2_dump_has_statement_body = false;
  evidence.safe_to_run_driver_branch = false;
  evidence.safe_to_run_driver_branch_with_supplied_evaluate_flags = true;
  evidence.requires_external_evaluate_flags = true;
  evidence.safe_to_publish_driver_weight = false;
  return evidence;
}

void source_char_weight_setter_poll_deps(
    SourceCharWeightSetterPollDeps& deps,
    const CharWeightSetter& setter,
    const std::vector<SourceCharWeightSetterRefOwner>& ref_owners) {
  deps.changed_by.push_back(setter.driver);
  deps.changed_by.push_back(setter.base);
  for (const auto& min_name : setter.min_weights) {
    deps.changed_by.push_back(min_name);
  }
  for (const auto& max_name : setter.max_weights) {
    deps.changed_by.push_back(max_name);
  }
  for (auto it = ref_owners.rbegin(); it != ref_owners.rend(); ++it) {
    if (it->weight_owner_is_setter) deps.change.push_back(it->name);
  }
}

SourceCharIKHeadState source_char_ik_head_default_state(
    const std::string& name) {
  SourceCharIKHeadState state;
  state.weightable = source_char_weightable_default_state(name);
  return state;
}

SourceCharIKHeadSetNameResult source_char_ik_head_set_name(
    SourceCharIKHeadState& state,
    const std::string& dir_name,
    bool dir_is_character) {
  SourceCharIKHeadSetNameResult result;
  result.call_hmx_set_name = true;
  result.assigned_character = dir_is_character;
  state.character_dir = dir_is_character ? dir_name : std::string{};
  return result;
}

void source_char_ik_head_poll_deps(
    SourceCharIKHeadPollDeps& deps,
    const SourceCharIKHeadState& state,
    const std::vector<std::string>& head_to_spine_parent_chain,
    bool generation_count_nonzero) {
  deps.changed_by.push_back(state.mouth);
  deps.changed_by.push_back(state.head);
  deps.changed_by.push_back(state.target);
  if (generation_count_nonzero) {
    for (const std::string& transform : head_to_spine_parent_chain) {
      deps.change.push_back(transform);
    }
  }
  deps.change.push_back(state.offset);
}

SourceCharIKHeadUpdatePointsResult source_char_ik_head_update_points(
    SourceCharIKHeadState& state,
    bool force,
    const std::vector<std::string>& head_to_spine_chain,
    const std::vector<float>& local_lengths) {
  SourceCharIKHeadUpdatePointsResult result;
  if (!force && !state.update_points) return result;
  result.entered_body = true;
  state.update_points = false;
  state.points.clear();
  const size_t point_count =
      std::min(head_to_spine_chain.size(), local_lengths.size());
  if (point_count <= 1) return result;

  result.rebuilt_points = true;
  state.points.resize(point_count);
  float total = 0.0f;
  for (size_t i = 0; i < point_count; ++i) {
    state.points[i].transform = head_to_spine_chain[i];
    state.points[i].local_length = local_lengths[i];
    total += local_lengths[i];
  }
  state.spine_length = total;
  result.spine_length = total;
  const float inv_total = 1.0f / total;
  float remaining = total;
  for (size_t i = 0; i < point_count; ++i) {
    state.points[i].normalized_remaining = remaining * inv_total;
    remaining -= state.points[i].local_length;
  }
  result.point_count = point_count;
  return result;
}

SourceCharIKHeadLoadSteps source_char_ik_head_load_steps(int32_t revision) {
  SourceCharIKHeadLoadSteps steps;
  steps.load_hmx_object = true;
  steps.load_weightable = true;
  steps.load_head = true;
  steps.load_spine = true;
  steps.load_mouth = true;
  steps.load_target = true;
  steps.load_target_radius = revision > 1;
  steps.load_head_mat = revision > 1;
  steps.load_offset = revision > 2;
  steps.load_offset_scale = revision > 2;
  steps.set_update_points = true;
  return steps;
}

SourceCharIKHeadCopyResult source_char_ik_head_copy(
    SourceCharIKHeadState& dest,
    const SourceCharIKHeadState& source,
    bool shallow_copy,
    float source_owner_weight) {
  SourceCharIKHeadCopyResult result;
  result.copy_hmx_object = true;
  result.copy_weightable = true;
  source_char_weightable_copy(dest.weightable, source.weightable, shallow_copy,
                              source_owner_weight);
  dest.head = source.head;
  result.copy_head = true;
  dest.spine = source.spine;
  result.copy_spine = true;
  dest.mouth = source.mouth;
  result.copy_mouth = true;
  dest.target = source.target;
  result.copy_target = true;
  dest.target_radius = source.target_radius;
  result.copy_target_radius = true;
  dest.head_mat = source.head_mat;
  result.copy_head_mat = true;
  dest.offset = source.offset;
  result.copy_offset = true;
  dest.offset_scale = source.offset_scale;
  result.copy_offset_scale = true;
  dest.update_points = true;
  result.set_update_points = true;
  return result;
}

SourceCharIKHeadHandlerPlan source_char_ik_head_handler_plan() {
  SourceCharIKHeadHandlerPlan plan;
  plan.superclasses = {"CharWeightable", "Hmx::Object"};
  plan.check = 0x138;
  return plan;
}

SourceCharIKHeadPropSyncPlan source_char_ik_head_prop_sync_plan() {
  SourceCharIKHeadPropSyncPlan plan;
  plan.modify_alt_properties = {"head", "spine"};
  plan.modify_alt_actions = {"UpdatePoints(true)", "UpdatePoints(true)"};
  plan.properties = {"mouth",         "target", "target_radius",
                     "head_mat",      "offset", "offset_scale"};
  plan.superclasses = {"CharWeightable"};
  return plan;
}

SourceCharIKHeadSavePlan source_char_ik_head_save_plan() {
  return SourceCharIKHeadSavePlan{};
}

SourceCharIKSliderMidiState source_char_ik_slider_midi_default_state(
    const std::string& name) {
  SourceCharIKSliderMidiState state;
  state.weightable = source_char_weightable_default_state(name);
  source_char_ik_slider_midi_enter(state);
  return state;
}

SourceCharIKSliderMidiEnterResult source_char_ik_slider_midi_enter(
    SourceCharIKSliderMidiState& state) {
  SourceCharIKSliderMidiEnterResult result;
  state.percentage_changed = false;
  result.cleared_percentage_changed = true;
  state.frac = 0.0f;
  result.reset_frac = true;
  state.frac_per_beat = 0.0f;
  result.reset_frac_per_beat = true;
  result.call_rnd_pollable_enter = true;
  return result;
}

SourceCharIKSliderMidiSetNameResult source_char_ik_slider_midi_set_name(
    SourceCharIKSliderMidiState& state,
    const std::string& dir_name,
    bool dir_is_character) {
  SourceCharIKSliderMidiSetNameResult result;
  result.call_hmx_set_name = true;
  result.assigned_character = dir_is_character;
  state.character_dir = dir_is_character ? dir_name : std::string{};
  return result;
}

SourceCharIKSliderMidiSetupResult source_char_ik_slider_midi_setup_transforms(
    SourceCharIKSliderMidiState& state) {
  SourceCharIKSliderMidiSetupResult result;
  state.reset_all = true;
  result.reset_all = true;
  return result;
}

void source_char_ik_slider_midi_poll_deps(
    SourceCharIKSliderMidiPollDeps& deps,
    const SourceCharIKSliderMidiState& state) {
  deps.change.push_back(state.target);
  deps.changed_by.push_back(state.target);
  deps.changed_by.push_back(state.first_spot);
  deps.changed_by.push_back(state.second_spot);
}

SourceCharIKSliderMidiLoadSteps source_char_ik_slider_midi_load_steps(
    int32_t revision) {
  SourceCharIKSliderMidiLoadSteps steps;
  steps.known_revision = revision >= 0 && revision <= steps.max_revision;
  steps.load_hmx_object = true;
  steps.load_weightable = revision > 1;
  steps.load_target = true;
  steps.load_first_spot = true;
  steps.load_second_spot = true;
  steps.load_tolerance = true;
  return steps;
}

SourceCharIKSliderMidiCopyResult source_char_ik_slider_midi_copy(
    SourceCharIKSliderMidiState& dest,
    const SourceCharIKSliderMidiState& source,
    bool shallow_copy,
    float source_owner_weight) {
  SourceCharIKSliderMidiCopyResult result;
  result.copy_hmx_object = true;
  result.copy_weightable = true;
  source_char_weightable_copy(dest.weightable, source.weightable, shallow_copy,
                              source_owner_weight);
  dest.target = source.target;
  result.copy_target = true;
  dest.first_spot = source.first_spot;
  result.copy_first_spot = true;
  dest.second_spot = source.second_spot;
  result.copy_second_spot = true;
  dest.tolerance = source.tolerance;
  result.copy_tolerance = true;
  return result;
}

SourceCharIKSliderMidiHandlerPlan
source_char_ik_slider_midi_handler_plan() {
  SourceCharIKSliderMidiHandlerPlan plan;
  plan.actions = {"set_fraction", "reset"};
  plan.superclasses = {"CharWeightable", "Hmx::Object"};
  plan.check = 0xF8;
  return plan;
}

SourceCharIKSliderMidiPropSyncPlan
source_char_ik_slider_midi_prop_sync_plan() {
  SourceCharIKSliderMidiPropSyncPlan plan;
  plan.modify_properties = {"target", "first_spot", "second_spot"};
  plan.modify_actions = {"SetupTransforms", "SetupTransforms",
                         "SetupTransforms"};
  plan.properties = {"tolerance"};
  plan.superclasses = {"CharWeightable"};
  return plan;
}

SourceCharIKSliderMidiSavePlan
source_char_ik_slider_midi_save_plan() {
  return SourceCharIKSliderMidiSavePlan{};
}

SourceCharIKMidiState source_char_ik_midi_default_state() {
  SourceCharIKMidiState state;
  source_char_ik_midi_enter(state);
  return state;
}

SourceCharIKMidiEnterResult source_char_ik_midi_enter(
    SourceCharIKMidiState& state) {
  SourceCharIKMidiEnterResult result;
  state.cur_spot.clear();
  state.new_spot.clear();
  state.spot_changed = false;
  state.frac = 0.0f;
  state.frac_per_beat = 0.0f;
  state.local_xfm_reset = true;
  state.old_local_xfm_reset = true;
  return result;
}

void source_char_ik_midi_poll_deps(SourceCharIKMidiPollDeps& deps,
                                   const SourceCharIKMidiState& state) {
  deps.change.push_back(state.bone);
  deps.changed_by.push_back(state.bone);
  deps.changed_by.push_back(state.cur_spot);
}

SourceCharIKMidiLoadSteps source_char_ik_midi_load_steps(int32_t revision) {
  SourceCharIKMidiLoadSteps steps;
  steps.known_revision = revision >= 0 && revision <= 5;
  steps.load_hmx_object = true;
  steps.load_bone = true;
  steps.load_legacy_spots = revision < 3;
  steps.load_legacy_string = revision == 2 || revision == 3;
  steps.load_anim_blend = revision > 4;
  return steps;
}

SourceCharIKMidiCopyPlan source_char_ik_midi_copy_plan() {
  SourceCharIKMidiCopyPlan plan;
  plan.copied_superclasses = {"Hmx::Object"};
  plan.copied_members = {"mBone", "mAnimBlender", "mMaxAnimBlend"};
  return plan;
}

SourceCharIKMidiHandlerPlan source_char_ik_midi_handler_plan() {
  SourceCharIKMidiHandlerPlan plan;
  plan.handlers = {"new_spot"};
  plan.superclasses = {"Hmx::Object"};
  plan.check = "0x11C";
  return plan;
}

SourceCharIKMidiPropSyncPlan source_char_ik_midi_prop_sync_plan() {
  SourceCharIKMidiPropSyncPlan plan;
  plan.properties = {"bone", "anim_blend_weightable", "anim_blend_max"};
  plan.set_properties = {"cur_spot"};
  return plan;
}

SourceCharIKMidiSavePlan source_char_ik_midi_save_plan() {
  return SourceCharIKMidiSavePlan{};
}

SourceCharLipSyncGeneratorState source_char_lip_sync_generator_default_state() {
  return SourceCharLipSyncGeneratorState{};
}

SourceCharLipSyncState source_char_lip_sync_default_state() {
  return SourceCharLipSyncState{};
}

SourceCharLipSyncLoadSteps source_char_lip_sync_load_steps(int32_t revision) {
  SourceCharLipSyncLoadSteps steps;
  steps.known_revision = revision >= 0 && revision <= steps.max_revision;
  steps.load_hmx_object = true;
  steps.load_visemes = true;
  steps.load_frames = true;
  steps.load_data = true;
  steps.load_prop_anim = revision != 0;
  return steps;
}

SourceCharLipSyncSavePlan source_char_lip_sync_save_plan() {
  return SourceCharLipSyncSavePlan{};
}

SourceCharLipSyncDriverState source_char_lip_sync_driver_default_state(
    const std::string& name) {
  SourceCharLipSyncDriverState state;
  state.weightable = source_char_weightable_default_state(name);
  return state;
}

SourceCharLipSyncDriverSavePlan source_char_lip_sync_driver_save_plan() {
  return SourceCharLipSyncDriverSavePlan{};
}

void source_char_lip_sync_driver_poll_deps(
    SourceCharLipSyncDriverPollDeps& deps,
    const SourceCharLipSyncDriverState& state) {
  deps.change.push_back(state.bones);
}

std::string source_char_lip_sync_driver_clip_dir(
    const SourceCharLipSyncDriverState& state) {
  return state.clips;
}

std::string source_char_lip_sync_driver_override_dir(
    const SourceCharLipSyncDriverState& state) {
  if (!state.override_options.empty()) return state.override_options;
  return source_char_lip_sync_driver_clip_dir(state);
}

static void apply_source_weight_setters(Character& character,
                                        float delta_beats) {
  std::unordered_map<std::string, float> weights_by_name;
  for (const auto& driver : character.drivers) {
    weights_by_name[driver.name] = driver.weight;
    if (!driver.weight_owner.empty()) {
      weights_by_name[driver.weight_owner] = driver.weight;
    }
  }
  for (const auto& setter : character.weight_setters) {
    weights_by_name[setter.name] = setter.weight;
    if (!setter.weight_owner.empty()) {
      weights_by_name[setter.weight_owner] = setter.weight;
    }
  }

  for (const auto& setter : character.weight_setters) {
    float weight = setter.weight;
    std::optional<float> driver_evaluate_flags;
    if (!setter.driver.empty()) {
      const auto driver = character.runtime_driver_flag_weights.find(setter.driver);
      if (driver != character.runtime_driver_flag_weights.end()) {
        const auto flag = driver->second.find(setter.flags);
        if (flag != driver->second.end()) {
          driver_evaluate_flags = flag->second;
        }
      }
    }
    if (!source_char_weight_setter_poll_with_driver_result(
            setter, weights_by_name, delta_beats, driver_evaluate_flags,
            weight)) {
      if (controller_audit_enabled()) {
        if (!setter.driver.empty()) {
          std::fprintf(stderr,
                       "[weightsetter-source-skip] %s driver=%s base=%s "
                       "reason=missing-source-CharDriver-EvaluateFlags\n",
                       setter.name.c_str(), setter.driver.c_str(),
                       setter.base.empty() ? "<none>" : setter.base.c_str());
        } else {
          std::fprintf(stderr,
                       "[weightsetter-source-skip] %s driver=%s base=%s "
                       "reason=missing-source-base-or-clamp-weight\n",
                       setter.name.c_str(), "<none>",
                       setter.base.empty() ? "<none>" : setter.base.c_str());
        }
      }
      continue;
    }
    weights_by_name[setter.name] = weight;
    if (!setter.weight_owner.empty()) weights_by_name[setter.weight_owner] = weight;
    character.runtime_weight_props[setter.name] = weight;
    if (!setter.weight_owner.empty()) {
      character.runtime_weight_props[setter.weight_owner] = weight;
    }
    if (controller_audit_enabled()) {
      const std::string driver_eval_text =
          driver_evaluate_flags ? std::to_string(*driver_evaluate_flags)
                                : std::string("<none>");
      std::fprintf(stderr,
                   "[weightsetter-source] %s weight=%.5f driver=%s flags=0x%08x "
                   "driverEval=%s base=%s mins=%zu maxs=%zu beatsPerWeight=%.5f\n",
                   setter.name.c_str(), weight,
                   setter.driver.empty() ? "<none>" : setter.driver.c_str(),
                   setter.flags, driver_eval_text.c_str(),
                   setter.base.empty() ? "<none>" : setter.base.c_str(),
                   setter.min_weights.size(), setter.max_weights.size(),
                   setter.beats_per_weight);
    }
  }
}

static void apply_source_ik_rods(Character& character) {
  for (const auto& rod : character.ik_rods) {
    std::array<float, 16> dest_world{};
    if (!source_char_ik_rod_compute_world(rod, character, dest_world)) {
      if (controller_audit_enabled()) {
        std::fprintf(stderr,
                     "[ikrod-source-skip] %s left=%s right=%s dest=%s "
                     "reason=missing-source-required-transform\n",
                     rod.name.c_str(),
                     rod.left_end.empty() ? "<none>" : rod.left_end.c_str(),
                     rod.right_end.empty() ? "<none>" : rod.right_end.c_str(),
                     rod.dest.empty() ? "<none>" : rod.dest.c_str());
      }
      continue;
    }
    character.runtime_world_overrides[rod.dest] = dest_world;
    if (controller_audit_enabled()) {
      std::fprintf(stderr,
                   "[ikrod-source] %s left=%s right=%s side=%s dest=%s "
                   "destPos=%.4f vertical=%d world=(%.3f %.3f %.3f)\n",
                   rod.name.c_str(), rod.left_end.c_str(),
                   rod.right_end.c_str(),
                   rod.side_axis.empty() ? "<none>" : rod.side_axis.c_str(),
                   rod.dest.c_str(), rod.dest_pos, rod.vertical ? 1 : 0,
                   dest_world[12], dest_world[13], dest_world[14]);
    }
  }
}

static float effective_ik_hand_solver_weight(const Character& character,
                                             const CharIKHand& ik) {
  if (!ik.weight_prop.empty()) {
    const auto runtime = character.runtime_weight_props.find(ik.weight_prop);
    if (runtime != character.runtime_weight_props.end()) {
      // MIDI hand-driver code writes the live left/right scalar each tick.
      // That live row overrides the decoded CharIKHand weight.
      return std::clamp(runtime->second, 0.0f, 1.0f);
    }
  }
  return std::clamp(ik.weight, 0.0f, 1.0f);
}

static float effective_ik_hand_target_blend_weight(const Character& character,
                                                   const CharIKHand& ik) {
  if (!ik.weight_prop.empty()) {
    const auto runtime = character.runtime_weight_props.find(ik.weight_prop);
    if (runtime != character.runtime_weight_props.end()) {
      // CharIKHand::Poll blends the hand world position toward the target using
      // the live CharWeightable scalar before the elbow and final hand writes.
      return std::clamp(runtime->second, 0.0f, 1.0f);
    }
  }
  return effective_ik_hand_solver_weight(character, ik);
}

static bool apply_source_ik_hand(Character& character, const CharIKHand& ik) {
  // Bounded single-target slice of ihatecompvir's CharIKHand::Poll/IKElbow
  // dataflow. PullShoulder is a real source function but its body is not in the
  // available ihatecompvir C++, so the remaining branches stay fenced.
    const int hand_i = find_bone_index(character, ik.hand);
    const float solver_weight =
        effective_ik_hand_solver_weight(character, ik);
    const float target_blend_weight =
        effective_ik_hand_target_blend_weight(character, ik);
    if (hand_i < 0 || solver_weight <= 0.0f) {
      if (debug_ik_enabled()) {
        std::fprintf(stderr,
                     "[ik-source] %s skipped hand=%s solveWeight=%.3f "
                     "targetBlend=%.5f hand_found=%d\n",
                     ik.name.c_str(), ik.hand.c_str(), solver_weight,
                     target_blend_weight, hand_i >= 0 ? 1 : 0);
      }
      return false;
    }
    auto& hand = character.bones[(size_t)hand_i];
    if (hand.parent.empty()) return false;
    const int fore_i = find_bone_index(character, hand.parent);
    if (fore_i < 0) return false;
    auto& fore = character.bones[(size_t)fore_i];
    if (fore.parent.empty()) return false;
    const int upper_i = find_bone_index(character, fore.parent);
    if (upper_i < 0) return false;
    auto& upper = character.bones[(size_t)upper_i];
    if (upper.parent.empty()) return false;

    std::array<float, 16> target_world{};
    if (!transform_local_chain_world(character, ik.target, target_world))
      return false;

    const milo_scene::Xfm upper_local0 = upper.local;
    const milo_scene::Xfm fore_local0 = fore.local;
    const auto upper_world0 = character.bone_world_local_chain(upper.name);
    const auto hand_world = character.bone_world_local_chain(hand.name);
    const Vec3 shoulder = mat_pos(upper_world0);
    const Vec3 raw_target = mat_pos(target_world);
    const std::string live_key =
        !ik.name.empty() ? ik.name : (ik.hand + "->" + ik.target);
    Vec3 previous_live = mat_pos(hand_world);
    if (const auto it = character.runtime_ik_hand_targets.find(live_key);
        it != character.runtime_ik_hand_targets.end()) {
      previous_live = {it->second[0], it->second[1], it->second[2]};
    }
    Vec3 target = raw_target;
    if (target_blend_weight < 0.999f) {
      target = vadd(vscale(previous_live, 1.0f - target_blend_weight),
                    vscale(raw_target, target_blend_weight));
    }
    character.runtime_ik_hand_targets[live_key] =
        {target.x, target.y, target.z};
    target_world[12] = target.x;
    target_world[13] = target.y;
    target_world[14] = target.z;
    RuntimeIKHandMeasureState& measure_state =
        character.runtime_ik_hand_measures[live_key];
    if (source_char_ik_hand_update_measure_lengths(
            ik.scalable, measure_state.hand_changed)) {
      const float hand_local_len =
          vlen({hand.local.pos[0], hand.local.pos[1], hand.local.pos[2]});
      const float parent_local_len =
          vlen({fore.local.pos[0], fore.local.pos[1], fore.local.pos[2]});
      const SourceCharIKHandMeasure measured =
          source_char_ik_hand_measure_lengths(true, hand_local_len,
                                              parent_local_len);
      measure_state.has_elbow_chain = measured.has_elbow_chain;
      measure_state.inv_2ab = measured.inv_2ab;
      measure_state.a2_plus_b2 = measured.a2_plus_b2;
      measure_state.aa_plus_bb = measured.aa_plus_bb;
    }
    SourceCharIKHandMeasure source_measure;
    source_measure.has_elbow_chain = measure_state.has_elbow_chain;
    source_measure.inv_2ab = measure_state.inv_2ab;
    source_measure.a2_plus_b2 = measure_state.a2_plus_b2;
    source_measure.aa_plus_bb = measure_state.aa_plus_bb;
    const float reach_sum = std::max(0.001f, source_measure.aa_plus_bb);
    const float upper_len = std::max(
        0.001f, vlen({fore.local.pos[0], fore.local.pos[1],
                      fore.local.pos[2]}));
    const float fore_len = std::max(0.001f, reach_sum - upper_len);
    const Vec3 to_target = vsub(target, shoulder);
    const float raw_dist = vlen(to_target);
    // `stretch` is the final CharIKHand hand-world write. It does not rewrite
    // the hand child local length used by IKElbow or later foretwist rows.
    const float dist2 = raw_dist * raw_dist;
    float cos_elbow = 0.0f;
    if (!source_char_ik_hand_elbow_cosine(source_measure, dist2,
                                          cos_elbow)) {
      return false;
    }
    const float sin_elbow =
        std::sqrt(std::max(0.0f, 1.0f - cos_elbow * cos_elbow));

    milo_scene::Xfm solved_fore = fore.local;
    write_source_elbow_z_bend(solved_fore, fore_local0, cos_elbow, sin_elbow);
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c)
        fore.local.rot[r][c] =
            fore_local0.rot[r][c] * (1.0f - solver_weight) +
            solved_fore.rot[r][c] * solver_weight;
    normalize_xfm_rows(fore.local);

    const auto upper_world_after_bend =
        character.bone_world_local_chain(upper.name);
    const auto hand_world_after_bend =
        character.bone_world_local_chain(hand.name);
    const Vec3 current_local = local_vec_from_world_rows(
        upper_world_after_bend,
        vsub(mat_pos(hand_world_after_bend), mat_pos(upper_world_after_bend)));
    const Vec3 target_local = local_vec_from_world_rows(
        upper_world_after_bend,
        vsub(target, mat_pos(upper_world_after_bend)));
    float swing_quat[4] = {};
    quat_from_vec_to_vec(current_local, target_local, swing_quat);
    float swing_rot[3][3] = {};
    quat_to_rot(swing_quat, swing_rot);
    // CharIKHand::IKElbow calls MakeRotQuat/MakeRotMatrix and writes
    // Multiply(ma0, trans2->LocalXfm().m, trans2->DirtyLocalXfm().m).
    pre_multiply_local_rot(upper.local, upper_local0, swing_rot,
                           solver_weight);

    const auto hand_world_before_final =
        character.bone_world_local_chain(hand.name);
    const float pre_final_error =
        vlen(vsub(mat_pos(hand_world_before_final), target));

    const bool write_final = ik.stretch || ik.orientation;
    if (write_final) {
      std::array<float, 16> solved_world = character.bone_world_local_chain(hand.name);
      if (ik.orientation) {
        std::array<float, 16> desired_orientation = target_world;
        for (int r = 0; r < 3; ++r) {
          for (int c = 0; c < 3; ++c) {
            solved_world[r * 4 + c] =
                solved_world[r * 4 + c] * (1.0f - solver_weight) +
                desired_orientation[r * 4 + c] * solver_weight;
          }
        }
      }
      if (ik.stretch) {
        solved_world[12] = solved_world[12] * (1.0f - solver_weight) +
                           target_world[12] * solver_weight;
        solved_world[13] = solved_world[13] * (1.0f - solver_weight) +
                           target_world[13] * solver_weight;
        solved_world[14] = solved_world[14] * (1.0f - solver_weight) +
                           target_world[14] * solver_weight;
      }
      normalize_mat3_rows(solved_world);
      // CharIKHand::Poll closes with SetWorldXfm(tf). Native keeps the
      // authored local row available to later source controllers and exposes
      // the live hand world row through the transient Trans bridge.
      character.runtime_world_overrides[hand.name] = solved_world;
    }

    if (debug_ik_enabled()) {
      const Vec3 hp = mat_pos(hand_world);
      const Vec3 tp = mat_pos(target_world);
      const auto upper_world_post = character.bone_world_local_chain(upper.name);
      const auto fore_world_post = character.bone_world_local_chain(fore.name);
      const auto hand_world_post = character.bone_world_local_chain(hand.name);
      std::fprintf(stderr,
                   "[ik-source-swing] %s hand=%s target=%s currentLocal=[%.5f %.5f %.5f] targetLocal=[%.5f %.5f %.5f] quat=[%.5f %.5f %.5f %.5f]\n",
                   ik.name.c_str(), ik.hand.c_str(), ik.target.c_str(),
                   current_local.x, current_local.y, current_local.z,
                   target_local.x, target_local.y, target_local.z,
                   swing_quat[0], swing_quat[1], swing_quat[2],
                   swing_quat[3]);
      std::fprintf(stderr,
                   "[ik-swing-cur] %s current=[%.5f %.5f %.5f]\n",
                   ik.name.c_str(), current_local.x, current_local.y,
                   current_local.z);
      std::fprintf(stderr,
                   "[ik-swing-target] %s target=[%.5f %.5f %.5f]\n",
                   ik.name.c_str(), target_local.x, target_local.y,
                   target_local.z);
      std::fprintf(stderr,
                   "[ik-swing-quat] %s quat=[%.5f %.5f %.5f %.5f]\n",
                   ik.name.c_str(), swing_quat[0], swing_quat[1],
                   swing_quat[2], swing_quat[3]);
      std::fprintf(stderr,
                   "[ik-solve-len] %s upper=%.5f reachSum=%.5f "
                   "fore=%.5f scalable=%d handChanged=%d cached=%d\n",
                   ik.name.c_str(), upper_len, source_measure.aa_plus_bb,
                   fore_len, ik.scalable ? 1 : 0,
                   measure_state.hand_changed ? 1 : 0,
                   source_measure.has_elbow_chain ? 1 : 0);
      std::fprintf(stderr,
                   "[ik-solve-dist] %s raw=%.5f dist2=%.5f cos=%.5f\n",
                   ik.name.c_str(), raw_dist, dist2, cos_elbow);
      std::fprintf(stderr,
                   "[ik-solve-flags] %s stretch=%d orient=%d final=%d\n",
                   ik.name.c_str(), ik.stretch ? 1 : 0,
                   ik.orientation ? 1 : 0, write_final ? 1 : 0);
      std::fprintf(stderr,
                   "[ik-live-target] %s raw=[%.5f %.5f %.5f] live=[%.5f %.5f %.5f] prev=[%.5f %.5f %.5f] weight=%.5f\n",
                   ik.name.c_str(), raw_target.x, raw_target.y,
                   raw_target.z, target.x, target.y, target.z,
                   previous_live.x, previous_live.y, previous_live.z,
                   target_blend_weight);
      log_debug_world_row("ik-source-preswing-upper", upper.name.c_str(),
                          upper_world_after_bend);
      log_debug_world_row("ik-source-preswing-hand", hand.name.c_str(),
                          hand_world_after_bend);
      std::fprintf(stderr,
                   "[ik-source] %s hand=%s target=%s solveWeight=%.3f targetBlend=%.5f hand=[%.2f %.2f %.2f] target=[%.2f %.2f %.2f] len=(%.2f %.2f) dist=%.2f cos=%.3f swing=source-pre final=%d orient=%d stretch=%d bendParent=%s upper=%s\n",
                   ik.name.c_str(), ik.hand.c_str(), ik.target.c_str(),
                   solver_weight, target_blend_weight, hp.x, hp.y, hp.z,
                   tp.x, tp.y, tp.z, upper_len, fore_len, raw_dist, cos_elbow,
                   write_final ? 1 : 0, ik.orientation ? 1 : 0,
                   ik.stretch ? 1 : 0,
                   fore.name.c_str(), upper.name.c_str());
      std::fprintf(stderr,
                   "[ik-source-error] %s preFinalError=%.4f "
                   "preFinal=[%.2f %.2f %.2f]\n",
                   ik.name.c_str(), pre_final_error,
                   hand_world_before_final[12], hand_world_before_final[13],
                   hand_world_before_final[14]);
      log_debug_xfm_row("ik-source-row", upper.name.c_str(), upper.local,
                        upper_world_post);
      log_debug_xfm_row_short("ik-source-row", upper.name.c_str(),
                              upper.local);
      log_debug_xfm_row("ik-source-row", fore.name.c_str(), fore.local,
                        fore_world_post);
      log_debug_xfm_row_short("ik-source-row", fore.name.c_str(),
                              fore.local);
      log_debug_xfm_row("ik-source-row", hand.name.c_str(), hand.local,
                        hand_world_post);
      log_debug_xfm_row_short("ik-source-row", hand.name.c_str(),
                              hand.local);
      log_debug_world_row("ik-source-target", ik.target.c_str(), target_world);
    }

  return true;
}

static bool source_hand_matches_fore_twist(const CharIKHand& ik,
                                           const CharForeTwist& ft) {
  if (ik.hand.empty() || ft.hand.empty()) return false;
  return ik.hand == ft.hand || channel_matches_bone(ik.hand, ft.hand) ||
         channel_matches_bone(ft.hand, ik.hand);
}

static int source_ik_hand_role_rank(const CharIKHand& ik) {
  const auto contains = [](const std::string& value, const char* needle) {
    return value.find(needle) != std::string::npos;
  };
  if (ik.name == "left_hand.ik" || ik.weight_prop == "left.weight" ||
      contains(ik.hand, "bone_L-hand") || contains(ik.target, "bone_fret")) {
    return 0;
  }
  if (ik.name == "right_hand.ik" || ik.weight_prop == "right.weight" ||
      contains(ik.hand, "bone_R-hand") || contains(ik.target, "bone_strum")) {
    return 1;
  }
  return 2;
}

static void apply_source_ik_hands_and_fore_twists(
    Character& character, const std::vector<milo_scene::Xfm>& bind_bones) {
  // CharForeTwist::PollDeps says it reads mHand and writes mTwist2 plus the
  // twist parent. Accepted active-song PS2 traces poll instrument performers as
  // fret/left first and strum/right second, with unknown rows preserving the
  // decoded MILO order.
  std::vector<size_t> ik_indices(character.ik_hands.size());
  for (size_t i = 0; i < ik_indices.size(); ++i) ik_indices[i] = i;
  std::stable_sort(ik_indices.begin(), ik_indices.end(),
                   [&](size_t a, size_t b) {
                     return source_ik_hand_role_rank(character.ik_hands[a]) <
                            source_ik_hand_role_rank(character.ik_hands[b]);
                   });

  std::vector<bool> fore_applied(character.fore_twists.size(), false);
  for (const size_t ik_index : ik_indices) {
    const CharIKHand& ik = character.ik_hands[ik_index];
    apply_source_ik_hand(character, ik);
    for (size_t ft_index = 0; ft_index < character.fore_twists.size();
         ++ft_index) {
      if (fore_applied[ft_index]) continue;
      const CharForeTwist& ft = character.fore_twists[ft_index];
      if (!source_hand_matches_fore_twist(ik, ft)) continue;
      apply_source_fore_twist(character, bind_bones, ft);
      fore_applied[ft_index] = true;
    }
  }

  for (size_t ft_index = 0; ft_index < character.fore_twists.size();
       ++ft_index) {
    if (fore_applied[ft_index]) continue;
    apply_source_fore_twist(character, bind_bones,
                            character.fore_twists[ft_index]);
  }
}

struct PendingPose {
  const ClipChannel* pos = nullptr;
  const ClipChannel* scale = nullptr;
  const ClipChannel* quat = nullptr;
  const ClipChannel* rotx = nullptr;
  const ClipChannel* roty = nullptr;
  const ClipChannel* rotz = nullptr;
};

static void apply_clip_pose_sampled_direct(
    const std::vector<ClipChannel>& channels, float weight, Character& character,
    bool relative);

static void apply_pending_pose(const PendingPose& pose, milo_scene::Xfm& local,
                               bool relative = false) {
  if (pose.quat) {
    float scale[3] = {};
    for (int r = 0; r < 3; ++r) {
      scale[r] = std::sqrt(local.rot[r][0] * local.rot[r][0] +
                           local.rot[r][1] * local.rot[r][1] +
                           local.rot[r][2] * local.rot[r][2]);
      if (scale[r] <= 1e-8f) scale[r] = 1.0f;
    }
    float rot[3][3];
    quat_to_rot(pose.quat->quat, rot);
    if (relative) {
      float out[3][3] = {};
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          for (int k = 0; k < 3; ++k) {
            out[r][c] += local.rot[r][k] * rot[k][c];
          }
        }
      }
      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
          local.rot[r][c] = out[r][c];
    } else {
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          const float target = rot[r][c] * scale[r];
          local.rot[r][c] = target;
        }
      }
    }
  }
  if (pose.rotx) {
    post_rotate_axis(
        local, ClipChannel::kRotX,
        source_grim_char_bones_samples_pose_axis_angle(ClipChannel::kRotX,
                                                       pose.rotx->angle));
  }
  if (pose.roty) {
    post_rotate_axis(
        local, ClipChannel::kRotY,
        source_grim_char_bones_samples_pose_axis_angle(ClipChannel::kRotY,
                                                       pose.roty->angle));
  }
  if (pose.rotz) {
    post_rotate_axis(
        local, ClipChannel::kRotZ,
        source_grim_char_bones_samples_pose_axis_angle(ClipChannel::kRotZ,
                                                       pose.rotz->angle));
  }
  if (pose.scale) {
    for (int r = 0; r < 3; ++r) {
      local.rot[r][0] *= pose.scale->scale[0];
      local.rot[r][1] *= pose.scale->scale[1];
      local.rot[r][2] *= pose.scale->scale[2];
    }
  }
  // Hand .pos channels are authored as IK targets; applying them as local FK
  // offsets tears the forearm/hand chain. CharIKHand publishes the live hand
  // world row after the clip pass.
  if (pose.pos &&
      (!is_hand_bone(pose.pos->bone_name) ||
       is_ik_hand_target_bone(pose.pos->bone_name))) {
    if (relative) {
      local.pos[0] += pose.pos->pos[0];
      local.pos[1] += pose.pos->pos[1];
      local.pos[2] += pose.pos->pos[2];
    } else {
      local.pos[0] = pose.pos->pos[0];
      local.pos[1] = pose.pos->pos[1];
      local.pos[2] = pose.pos->pos[2];
    }
  }
}

static void renormalize_rows(milo_scene::Xfm& local) {
  for (int r = 0; r < 3; ++r) {
    float len = std::sqrt(local.rot[r][0] * local.rot[r][0] +
                          local.rot[r][1] * local.rot[r][1] +
                          local.rot[r][2] * local.rot[r][2]);
    if (len <= 1e-6f) continue;
    for (int c = 0; c < 3; ++c) local.rot[r][c] /= len;
  }
}

static void apply_pending_pose_weighted(const PendingPose& pose,
                                        milo_scene::Xfm& local,
                                        float weight,
                                        bool relative = false) {
  weight = std::clamp(weight, 0.0f, 1.0f);
  if (weight <= 0.0f) return;
  if (weight >= 0.999f) {
    apply_pending_pose(pose, local, relative);
    return;
  }

  if (pose.quat) {
    float scale[3] = {};
    for (int r = 0; r < 3; ++r) {
      scale[r] = std::sqrt(local.rot[r][0] * local.rot[r][0] +
                           local.rot[r][1] * local.rot[r][1] +
                           local.rot[r][2] * local.rot[r][2]);
      if (scale[r] <= 1e-8f) scale[r] = 1.0f;
    }
    float rot[3][3];
    quat_to_rot(pose.quat->quat, rot);
    if (relative) {
      float out[3][3] = {};
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          for (int k = 0; k < 3; ++k) {
            out[r][c] += local.rot[r][k] * rot[k][c];
          }
        }
      }
      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
          local.rot[r][c] =
              local.rot[r][c] * (1.0f - weight) + out[r][c] * weight;
    } else {
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          const float target = rot[r][c] * scale[r];
          local.rot[r][c] = local.rot[r][c] * (1.0f - weight) + target * weight;
        }
      }
    }
    renormalize_rows(local);
  }
  if (pose.rotx) {
    post_rotate_axis(
        local, ClipChannel::kRotX,
        source_grim_char_bones_samples_pose_axis_angle(ClipChannel::kRotX,
                                                       pose.rotx->angle) *
            weight);
  }
  if (pose.roty) {
    post_rotate_axis(
        local, ClipChannel::kRotY,
        source_grim_char_bones_samples_pose_axis_angle(ClipChannel::kRotY,
                                                       pose.roty->angle) *
            weight);
  }
  if (pose.rotz) {
    post_rotate_axis(
        local, ClipChannel::kRotZ,
        source_grim_char_bones_samples_pose_axis_angle(ClipChannel::kRotZ,
                                                       pose.rotz->angle) *
            weight);
  }
  if (pose.scale) {
    const float sx = 1.0f + (pose.scale->scale[0] - 1.0f) * weight;
    const float sy = 1.0f + (pose.scale->scale[1] - 1.0f) * weight;
    const float sz = 1.0f + (pose.scale->scale[2] - 1.0f) * weight;
    for (int r = 0; r < 3; ++r) {
      local.rot[r][0] *= sx;
      local.rot[r][1] *= sy;
      local.rot[r][2] *= sz;
    }
  }
  if (pose.pos &&
      (!is_hand_bone(pose.pos->bone_name) ||
       is_ik_hand_target_bone(pose.pos->bone_name))) {
    if (relative) {
      local.pos[0] += pose.pos->pos[0] * weight;
      local.pos[1] += pose.pos->pos[1] * weight;
      local.pos[2] += pose.pos->pos[2] * weight;
    } else {
      local.pos[0] = local.pos[0] * (1.0f - weight) + pose.pos->pos[0] * weight;
      local.pos[1] = local.pos[1] * (1.0f - weight) + pose.pos->pos[1] * weight;
      local.pos[2] = local.pos[2] * (1.0f - weight) + pose.pos->pos[2] * weight;
    }
  }
}

static std::string strip_transform_suffix(std::string s) {
  auto strip = [&](const char* suffix) {
    const size_t n = std::strlen(suffix);
    if (s.size() >= n && s.compare(s.size() - n, n, suffix) == 0) {
      s.resize(s.size() - n);
      return true;
    }
    return false;
  };
  strip(".mesh") || strip(".trans");
  return s;
}

struct OutputPoseNode {
  std::string name;
  std::string key;
  std::string parent_key;
  milo_scene::Xfm bind_local;
  milo_scene::Xfm current_local;
  milo_scene::Xfm world_stored;
};

static std::array<float, 16> xfm_to_mat4_local(const milo_scene::Xfm& x) {
  return {x.rot[0][0], x.rot[0][1], x.rot[0][2], 0.0f,
          x.rot[1][0], x.rot[1][1], x.rot[1][2], 0.0f,
          x.rot[2][0], x.rot[2][1], x.rot[2][2], 0.0f,
          x.pos[0],    x.pos[1],    x.pos[2],    1.0f};
}

static std::array<float, 16> output_node_local_chain(
    const std::vector<OutputPoseNode>& nodes,
    const std::unordered_map<std::string, size_t>& by_key,
    size_t index, bool bind_pose) {
  std::array<float, 16> world =
      xfm_to_mat4_local(bind_pose ? nodes[index].bind_local
                                  : nodes[index].current_local);
  std::string parent = nodes[index].parent_key;
  int guard = 0;
  while (!parent.empty() && guard++ < 128) {
    const auto it = by_key.find(parent);
    if (it == by_key.end()) break;
    const auto& node = nodes[it->second];
    world = mat4_mul(
        world, xfm_to_mat4_local(bind_pose ? node.bind_local
                                           : node.current_local));
    parent = node.parent_key;
  }
  return world;
}

static bool hand_output_layer_disabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool disabled =
      _dupenv_s(&value, &len, "GHOGX_DISABLE_HAND_OUTPUT_LAYER") == 0 &&
      value && value[0];
  std::free(value);
  return disabled;
#else
  const char* value = std::getenv("GHOGX_DISABLE_HAND_OUTPUT_LAYER");
  return value && value[0];
#endif
}

static bool is_hand_driver_root_key(const std::string& key) {
  return key == "bone_strum" || key == "bone_strum_hand" ||
         key == "bone_fret" || key == "bone_fret_hand";
}

static bool is_constant_fret_hand_target_key(const std::string& key) {
  return key == "bone_fret_hand";
}

static bool is_hand_driver_output_key(const std::string& key) {
  if (is_hand_driver_root_key(key)) return true;
  const bool left_or_right =
      key.rfind("bone_L-", 0) == 0 || key.rfind("bone_R-", 0) == 0;
  if (!left_or_right) return false;
  return key.find("-hand") != std::string::npos ||
         key.find("-index") != std::string::npos ||
         key.find("-middlefinger") != std::string::npos ||
         key.find("-ringfinger") != std::string::npos ||
         key.find("-pinky") != std::string::npos ||
         key.find("-thumb") != std::string::npos;
}

static bool output_bones_have_hand_driver_root(
    const std::vector<CharClip::OutputBone>& output_bones) {
  for (const auto& out : output_bones) {
    if (is_hand_driver_root_key(strip_transform_suffix(out.name))) {
      return true;
    }
  }
  return false;
}

enum class HandDriverOutputGroup {
  Fret,
  Strum,
};

static bool hand_driver_key_matches_group(const std::string& key,
                                          HandDriverOutputGroup group) {
  if (group == HandDriverOutputGroup::Fret) {
    return key == "bone_fret" || key == "bone_fret_hand" ||
           key.rfind("bone_L-", 0) == 0;
  }
  return key == "bone_strum" || key == "bone_strum_hand" ||
         key.rfind("bone_R-", 0) == 0;
}

static bool output_bones_have_hand_driver_group_root(
    const std::vector<CharClip::OutputBone>& output_bones,
    HandDriverOutputGroup group) {
  for (const auto& out : output_bones) {
    const std::string key = strip_transform_suffix(out.name);
    if (group == HandDriverOutputGroup::Fret &&
        (key == "bone_fret" || key == "bone_fret_hand")) {
      return true;
    }
    if (group == HandDriverOutputGroup::Strum &&
        (key == "bone_strum" || key == "bone_strum_hand")) {
      return true;
    }
  }
  return false;
}

static int output_depth(const std::vector<OutputPoseNode>& nodes,
                        const std::unordered_map<std::string, size_t>& by_key,
                        size_t index) {
  int depth = 0;
  std::string parent = nodes[index].parent_key;
  while (!parent.empty() && depth < 128) {
    const auto it = by_key.find(parent);
    if (it == by_key.end()) break;
    ++depth;
    parent = nodes[it->second].parent_key;
  }
  return depth;
}

static bool charbone_output_compare_enabled() {
#ifdef _MSC_VER
  char* value = nullptr;
  size_t len = 0;
  const bool enabled =
      _dupenv_s(&value, &len, "GHOGX_DEBUG_CHARBONE_OUTPUT_MAP") == 0 &&
      value && value[0];
  std::free(value);
  return enabled;
#else
  const char* value = std::getenv("GHOGX_DEBUG_CHARBONE_OUTPUT_MAP");
  return value && value[0];
#endif
}

static bool output_map_interesting_bone(const std::string& key) {
  return is_hand_driver_root_key(key) ||
         key == "bone_facing" || key == "bone_pelvis" ||
         key.find("-thigh") != std::string::npos ||
         key.find("-knee") != std::string::npos ||
         key.find("-ankle") != std::string::npos ||
         key.find("-foot") != std::string::npos ||
         key.find("-toe") != std::string::npos ||
         key.find("-arm") != std::string::npos ||
         key.find("-Arm") != std::string::npos ||
         key.find("-forearm") != std::string::npos ||
         key.find("-foreArm") != std::string::npos ||
         key.find("-clavicle") != std::string::npos ||
         key.find("-hand") != std::string::npos ||
         key.find("-thumb") != std::string::npos ||
         key.find("-index") != std::string::npos ||
         key.find("-middlefinger") != std::string::npos ||
         key.find("-ringfinger") != std::string::npos ||
         key.find("-pinky") != std::string::npos ||
         key.find("face") != std::string::npos ||
         key.find("mouth") != std::string::npos ||
         key.find("lip") != std::string::npos ||
         key.find("jaw") != std::string::npos ||
         key.find("brow") != std::string::npos ||
         key.find("lid") != std::string::npos ||
         key.find("eye") != std::string::npos;
}

static void dump_charbone_output_map(
    const Character& character, const std::vector<OutputPoseNode>& nodes,
    const std::unordered_map<std::string, size_t>& by_key,
    const std::vector<bool>& node_driven, bool live_writes_enabled) {
  if (!charbone_output_compare_enabled()) return;
  for (size_t i = 0; i < nodes.size(); ++i) {
    const auto& node = nodes[i];
    if (!output_map_interesting_bone(node.key)) continue;
    const int target_i = find_bone_index(character, node.key);
    if (target_i < 0 ||
        static_cast<size_t>(target_i) >= character.bones.size()) {
      std::fprintf(stderr,
                   "[out-map] %-18s output=%-24s parent=%-18s driven=%d "
                   "live=%d target=<none> outLocal=(%.3f %.3f %.3f)\n",
                   node.key.c_str(), node.name.c_str(),
                   node.parent_key.c_str(), node_driven[i] ? 1 : 0,
                   live_writes_enabled ? 1 : 0,
                   node.current_local.pos[0], node.current_local.pos[1],
                   node.current_local.pos[2]);
      continue;
    }
    const auto& target = character.bones[static_cast<size_t>(target_i)];
    const auto& target_bind =
        character.bind_bone_local.size() > static_cast<size_t>(target_i)
            ? character.bind_bone_local[static_cast<size_t>(target_i)]
            : target.local;
    const auto out_bind_world = output_node_local_chain(nodes, by_key, i, true);
    const auto out_pose_world = output_node_local_chain(nodes, by_key, i, false);
    const auto target_bind_world =
        character.bone_world_bind_local_chain(target.name);
    const auto target_pose_world =
        character.bone_world_local_chain(target.name);
    std::fprintf(
        stderr,
        "[out-map] %-18s output=%-24s parent=%-18s driven=%d "
        "live=%d target=%-24s tParent=%-24s "
        "outLocal=(%.3f %.3f %.3f) meshLocal=(%.3f %.3f %.3f) "
        "outBindW=(%.3f %.3f %.3f) meshBindW=(%.3f %.3f %.3f) "
        "outPoseW=(%.3f %.3f %.3f) meshPoseW=(%.3f %.3f %.3f) "
        "bindLocal=(%.3f %.3f %.3f)\n",
        node.key.c_str(), node.name.c_str(), node.parent_key.c_str(),
        node_driven[i] ? 1 : 0, live_writes_enabled ? 1 : 0,
        target.name.c_str(), target.parent.c_str(),
        node.current_local.pos[0], node.current_local.pos[1],
        node.current_local.pos[2], target.local.pos[0], target.local.pos[1],
        target.local.pos[2], out_bind_world[12], out_bind_world[13],
        out_bind_world[14], target_bind_world[12], target_bind_world[13],
        target_bind_world[14], out_pose_world[12], out_pose_world[13],
        out_pose_world[14], target_pose_world[12], target_pose_world[13],
        target_pose_world[14], target_bind.pos[0], target_bind.pos[1],
        target_bind.pos[2]);
  }
}

static bool apply_clip_pose_output_layer(
    const std::vector<ClipChannel>& channels, float weight, Character& character,
    bool relative, const std::vector<CharClip::OutputBone>& output_bones,
    bool force_selected_output = false) {
  if (output_bones.empty()) {
    return false;
  }

  std::vector<OutputPoseNode> nodes;
  nodes.reserve(output_bones.size());
  std::unordered_map<std::string, size_t> by_key;
  for (const auto& out : output_bones) {
    OutputPoseNode node;
    node.name = out.name;
    node.key = strip_transform_suffix(out.name);
    node.parent_key = strip_transform_suffix(out.parent);
    node.bind_local = out.local;
    node.current_local = out.local;
    node.world_stored = out.world_stored;
    by_key[node.key] = nodes.size();
    nodes.push_back(std::move(node));
  }

  std::vector<PendingPose> poses(nodes.size());
  std::vector<bool> node_driven(nodes.size(), false);
  std::vector<ClipChannel> direct_channels;
  direct_channels.reserve(channels.size());
  const bool compare_output = charbone_output_compare_enabled();
  for (const auto& ch : channels) {
    const auto it = by_key.find(strip_transform_suffix(ch.bone_name));
    if (it == by_key.end()) {
      direct_channels.push_back(ch);
      continue;
    }
    const bool selected_for_live_output = force_selected_output;
    if (!selected_for_live_output && !compare_output) {
      direct_channels.push_back(ch);
      continue;
    }
    PendingPose& pose = poses[it->second];
    node_driven[it->second] = true;
    switch (ch.type) {
      case ClipChannel::kPos: pose.pos = &ch; break;
      case ClipChannel::kScale: pose.scale = &ch; break;
      case ClipChannel::kQuat: pose.quat = &ch; break;
      case ClipChannel::kRotX: pose.rotx = &ch; break;
      case ClipChannel::kRotY: pose.roty = &ch; break;
      case ClipChannel::kRotZ: pose.rotz = &ch; break;
    }
  }

  for (size_t i = 0; i < nodes.size(); ++i) {
    if (!node_driven[i]) continue;
    apply_pending_pose_weighted(poses[i], nodes[i].current_local, weight,
                                relative);
  }

  if (force_selected_output) {
    for (size_t i = 0; i < nodes.size(); ++i) {
      if (node_driven[i]) continue;
      if (is_constant_fret_hand_target_key(nodes[i].key)) {
        node_driven[i] = true;
      }
    }
  }

  dump_charbone_output_map(character, nodes, by_key, node_driven,
                           force_selected_output);

  if (!force_selected_output) {
    return false;
  }

  struct TargetUpdate {
    int depth = 0;
    int target_index = -1;
    milo_scene::Xfm desired_local;
  };
  std::vector<TargetUpdate> updates;
  updates.reserve(nodes.size());

  for (size_t i = 0; i < nodes.size(); ++i) {
    if (!node_driven[i]) continue;
    const int target_i = find_bone_index(character, nodes[i].key);
    if (target_i < 0 ||
        static_cast<size_t>(target_i) >= character.bones.size()) {
      continue;
    }
    TargetUpdate update;
    update.depth = output_depth(nodes, by_key, i);
    update.target_index = target_i;
    update.desired_local = nodes[i].current_local;
    updates.push_back(update);
  }

  std::sort(updates.begin(), updates.end(),
            [](const TargetUpdate& a, const TargetUpdate& b) {
              return a.depth < b.depth;
            });
  for (const auto& update : updates) {
    auto& target = character.bones[static_cast<size_t>(update.target_index)];
    // Only explicitly selected hand output writes this reconstructed CharBone
    // output graph until the source runtime publisher is known.
    target.local = update.desired_local;
  }

  if (!direct_channels.empty()) {
    apply_clip_pose_sampled_direct(direct_channels, weight, character,
                                   relative);
  }
  return true;
}

static void apply_hand_driver_output_layer(
    const std::vector<ClipChannel>& frame, Character& character, bool relative,
    const std::vector<CharClip::OutputBone>& source_output_bones) {
  if (hand_output_layer_disabled() || frame.empty() ||
      !output_bones_have_hand_driver_root(source_output_bones)) {
    return;
  }

  std::vector<CharClip::OutputBone> hand_output_bones;
  std::unordered_set<std::string> hand_keys;
  for (const auto& out : source_output_bones) {
    const std::string key = strip_transform_suffix(out.name);
    if (!is_hand_driver_output_key(key)) continue;
    if (!hand_keys.insert(key).second) continue;
    hand_output_bones.push_back(out);
  }
  if (hand_output_bones.empty()) return;

  std::vector<ClipChannel> hand_channels;
  hand_channels.reserve(frame.size());
  for (const auto& ch : frame) {
    if (hand_keys.find(strip_transform_suffix(ch.bone_name)) ==
        hand_keys.end()) {
      continue;
    }
    hand_channels.push_back(ch);
  }
  if (hand_channels.empty()) return;

  // Hand-driver clips carry their own CharBone output graph. The first-level
  // fingers are authored under bone_strum_hand/bone_fret_hand, while the live
  // mesh skeleton keeps them under bone_R-hand/bone_L-hand. CharIKHand mounts
  // the live hand onto that target after this clip pass, so the child rows must
  // stay in hand-local space here; bridging through the pre-IK parent applies
  // the offset a second time once the hand reaches the target.
  apply_clip_pose_output_layer(hand_channels, 1.0f, character, relative,
                               hand_output_bones, true);
}

static void apply_hand_driver_output_layers(
    const std::vector<ClipChannel>& frame, Character& character, bool relative,
    const std::vector<ClipChannelLayer>& layers) {
  if (hand_output_layer_disabled()) return;
  (void)frame;
  (void)relative;

  auto apply_group = [&](HandDriverOutputGroup group) {
    bool has_hand_driver_overlay = false;
    bool hand_relative = false;
    bool hand_relative_set = false;
    std::vector<ClipChannelLayer> hand_source_layers;
    for (const auto& layer : layers) {
      if (!layer.overlay_override || !layer.output_bones) continue;
      if (!output_bones_have_hand_driver_group_root(*layer.output_bones,
                                                    group)) {
        continue;
      }

      std::vector<ClipChannel> group_channels;
      group_channels.reserve(layer.channels.size());
      for (const auto& ch : layer.channels) {
        const std::string key = strip_transform_suffix(ch.bone_name);
        if (!is_hand_driver_output_key(key) ||
            !hand_driver_key_matches_group(key, group)) {
          continue;
        }
        group_channels.push_back(ch);
      }
      if (group_channels.empty()) continue;

      has_hand_driver_overlay = true;
      ClipChannelLayer group_layer = layer;
      group_layer.channels = std::move(group_channels);
      hand_source_layers.push_back(std::move(group_layer));
      if (!hand_relative_set) {
        hand_relative = layer.relative;
        hand_relative_set = true;
      } else if (hand_relative != layer.relative) {
        hand_relative = false;
      }
    }
    if (!has_hand_driver_overlay) return;

    std::vector<CharClip::OutputBone> hand_output_bones;
    std::unordered_set<std::string> hand_keys;
    for (const auto& layer : hand_source_layers) {
      if (!layer.output_bones) continue;
      for (const auto& out : *layer.output_bones) {
        const std::string key = strip_transform_suffix(out.name);
        if (!is_hand_driver_output_key(key) ||
            !hand_driver_key_matches_group(key, group)) {
          continue;
        }
        if (!hand_keys.insert(key).second) continue;
        hand_output_bones.push_back(out);
      }
    }
    if (hand_output_bones.empty()) return;

    const auto hand_frame = blend_channel_layers(hand_source_layers);
    if (hand_frame.empty()) return;

    std::vector<ClipChannel> hand_channels;
    hand_channels.reserve(hand_frame.size());
    for (const auto& ch : hand_frame) {
      if (hand_keys.find(strip_transform_suffix(ch.bone_name)) ==
          hand_keys.end()) {
        continue;
      }
      hand_channels.push_back(ch);
    }
    if (hand_channels.empty()) return;

    apply_clip_pose_output_layer(hand_channels, 1.0f, character, hand_relative,
                                 hand_output_bones, true);
  };

  apply_group(HandDriverOutputGroup::Strum);
  apply_group(HandDriverOutputGroup::Fret);
}

static void apply_char_hair(Character& character, float time_seconds) {
  if (character.hairs.empty()) return;
  for (const auto& hair : character.hairs) {
    log_char_hair_source_once(character, hair);
    SourceCharHairRuntime& state =
        ensure_source_char_hair_runtime(character, hair);
    const bool first_poll = state.last_time_seconds < 0.0f;
    const bool nonzero_delta =
        first_poll || time_seconds != state.last_time_seconds;
    const SourceCharHairPollDecision poll_decision =
        source_char_hair_poll_decision(true, false, false, 0, state.reset,
                                       nonzero_delta ? 1.0f : 0.0f);
    if (poll_decision.do_reset) {
      source_char_hair_do_reset(character, hair, state,
                                poll_decision.reset_count);
      state.reset = poll_decision.next_reset;
      if (poll_decision.return_after_reset) {
        state.last_time_seconds = time_seconds;
        continue;
      }
    }

    int write_count = 0;
    if (poll_decision.simulate_loops) {
      write_count = source_char_hair_simulate_loops(character, hair, state, 1,
                                                   source_char_hair_get_fps(
                                                       state.use_post_proc,
                                                       0.0f),
                                                   hair.inertia, hair.friction);
    }
    state.last_time_seconds = time_seconds;

    if (debug_char_hair_enabled()) {
      std::fprintf(
          stderr,
          "[charhair-source-sim] character=%s hair=%s "
          "source=ihatecompvir-CharHair::Poll/DoReset/SimulateInternal "
          "runtimeWriteback=%d resolvedPointCollides=0 "
          "managedHookup=%d bandCharacterHookup=%d "
          "defaultHookupWouldReturn=%d dirCollides=%zu "
          "legacyInlinePoints=%d "
          "hookupOverloadBody=%d missingHookupOverloadBody=%d "
          "zeroTimeBodyAvailable=0 "
          "usePostProc=%d nonzeroDelta=%d firstPoll=%d pollHookup=%d "
          "pollReset=%d pollZeroTime=%d time=%.4f\n",
          character.dir_name.c_str(), hair.name.c_str(), write_count,
          state.managed_hookup ? 1 : 0,
          state.band_character_hookup ? 1 : 0,
          state.default_hookup_returned_for_managed ? 1 : 0,
          state.hookup_collides.size(), state.legacy_inline_point_count,
          state.hookup_overload_body_statement_visible ? 1 : 0,
          state.hookup_overload_body_statement_visible ? 0 : 1,
          state.use_post_proc ? 1 : 0, nonzero_delta ? 1 : 0,
          first_poll ? 1 : 0, poll_decision.hookup ? 1 : 0,
          poll_decision.do_reset ? poll_decision.reset_count : -1,
          poll_decision.simulate_zero_time ? 1 : 0, time_seconds);
    }
  }
}

static std::array<float, 16> blend_world_rows(
    const std::array<float, 16>& a, const std::array<float, 16>& b,
    float weight) {
  weight = std::clamp(weight, 0.0f, 1.0f);
  std::array<float, 16> out{};
  for (size_t i = 0; i < out.size(); ++i) {
    out[i] = a[i] * (1.0f - weight) + b[i] * weight;
  }
  out[15] = 1.0f;
  normalize_mat3_rows(out);
  return out;
}

void apply_ik_midi_fret_target(Character& character,
                               const std::string& spot_name,
                               float time_seconds) {
  if (spot_name.empty()) return;
  std::array<float, 16> spot_world{};
  if (!transform_local_chain_world(character, spot_name, spot_world)) return;

  const float blend_seconds =
      std::clamp(env_float_or("GHOGX_IKMIDI_BLEND_SECONDS", 0.08f), 0.0f,
                 0.22f);
  for (const auto& ik : character.ik_midis) {
    if (ik.bone.empty()) continue;
    const int bone_i = find_bone_index(character, ik.bone);
    if (bone_i < 0 ||
        static_cast<size_t>(bone_i) >= character.bones.size()) {
      continue;
    }
    auto& bone = character.bones[static_cast<size_t>(bone_i)];
    RuntimeIKMidiState& state = character.runtime_ik_midi_states[ik.name];
    if (!state.initialized || state.active_spot != spot_name) {
      state.initialized = true;
      state.active_spot = spot_name;
      state.spot_start_time_seconds = time_seconds;
      state.start_world = character.bone_world_local_chain(bone.name);
    }

    const float age = std::max(0.0f, time_seconds - state.spot_start_time_seconds);
    const float weight = blend_seconds > 0.0f ? age / blend_seconds : 1.0f;
    const auto desired_world =
        blend_world_rows(state.start_world, spot_world, weight);
    std::array<float, 16> parent_world =
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    if (!bone.parent.empty()) {
      parent_world = character.bone_world_local_chain(bone.parent);
    }
    set_local_from_world(bone.local, desired_world, parent_world);
    if (debug_ik_enabled()) {
      std::fprintf(stderr,
                   "[ikmidi] %s bone=%s spot=%s age=%.3f blend=%.3f "
                   "weight=%.3f "
                   "target=[%.3f %.3f %.3f]\n",
                   ik.name.c_str(), bone.name.c_str(), spot_name.c_str(),
                   age, blend_seconds, std::clamp(weight, 0.0f, 1.0f),
                   desired_world[12], desired_world[13], desired_world[14]);
    }
  }
}

void clear_runtime_ik_weights(Character& character) {
  character.runtime_weight_props.clear();
  character.runtime_driver_flag_weights.clear();
  character.runtime_world_overrides.clear();
}

void set_runtime_ik_weight(Character& character, const std::string& weight_prop,
                           float weight) {
  if (weight_prop.empty()) return;
  character.runtime_weight_props[weight_prop] = std::clamp(weight, 0.0f, 1.0f);
}

void set_runtime_driver_evaluate_flags(Character& character,
                                       const std::string& driver_name,
                                       uint32_t flags,
                                       float weight) {
  if (driver_name.empty() || flags == 0) return;
  character.runtime_driver_flag_weights[driver_name][flags] =
      std::clamp(weight, 0.0f, 1.0f);
}

void clear_runtime_trans_worlds(Character& character) {
  character.runtime_world_overrides.clear();
}

void apply_character_controllers(Character& character, float time_seconds) {
  (void)time_seconds;
  character.runtime_world_overrides.clear();
  log_character_controller_graph_once(character);
  dump_arm_pose(character, "controllers-pre");
  std::vector<milo_scene::Xfm> bind_bones = character.bind_bone_local;
  if (bind_bones.size() != character.bones.size()) {
    bind_bones.clear();
    bind_bones.reserve(character.bones.size());
    for (const auto& b : character.bones) bind_bones.push_back(b.local);
  }
  apply_source_weight_setters(character, 0.0f);
  apply_source_ik_hands_and_fore_twists(character, bind_bones);
  apply_char_hair(character, time_seconds);
  apply_source_upper_twists(character, bind_bones);
  apply_source_pos_constraints(character);
  apply_source_ik_rods(character);
  dump_arm_pose(character, "controllers-post");

  if (debug_face_enabled()) {
    for (const auto& b : character.bones) {
      if (b.name != "bone_L-upperlid.mesh" &&
          b.name != "bone_R-upperlid.mesh") {
        continue;
      }
      const auto world = character.bone_world_local_chain(b.name);
      std::fprintf(stderr,
                   "[face] upperlid %s parent=%s world=(%.3f %.3f %.3f) "
                   "local=(%.3f %.3f %.3f) "
                   "rows=[%.3f %.3f %.3f|%.3f %.3f %.3f|%.3f %.3f %.3f]\n",
                   b.name.c_str(), b.parent.c_str(), world[12], world[13],
                   world[14], b.local.pos[0], b.local.pos[1], b.local.pos[2],
                   b.local.rot[0][0], b.local.rot[0][1], b.local.rot[0][2],
                   b.local.rot[1][0], b.local.rot[1][1], b.local.rot[1][2],
                   b.local.rot[2][0], b.local.rot[2][1], b.local.rot[2][2]);
    }
    for (const auto& m : character.meshes) {
      if (m.name != "eye-L.mesh" && m.name != "eye-R.mesh") continue;
      const auto parent_world = character.bone_world_local_chain(m.parent);
      const auto eye_world = character.mesh_world(m);
      const Vec3 head_pos = mat_pos(parent_world);
      const Vec3 eye_pos = mat_pos(eye_world);
      std::fprintf(stderr,
                   "[face] eye %s parent=%s head=(%.3f %.3f %.3f) eye=(%.3f %.3f %.3f) local=(%.3f %.3f %.3f) rows=[%.3f %.3f %.3f|%.3f %.3f %.3f|%.3f %.3f %.3f]\n",
                   m.name.c_str(), m.parent.c_str(), head_pos.x, head_pos.y,
                   head_pos.z, eye_pos.x, eye_pos.y, eye_pos.z, m.local.pos[0],
                   m.local.pos[1], m.local.pos[2], m.local.rot[0][0],
                   m.local.rot[0][1], m.local.rot[0][2], m.local.rot[1][0],
                   m.local.rot[1][1], m.local.rot[1][2], m.local.rot[2][0],
                   m.local.rot[2][1], m.local.rot[2][2]);
    }
  }
}

void apply_clip_pose(const std::vector<ClipChannel>& channels, Character& character) {
  apply_clip_pose_weighted(channels, 1.0f, character);
}

static void apply_clip_pose_sampled_direct(
    const std::vector<ClipChannel>& channels, float weight, Character& character,
    bool relative) {
  std::vector<PendingPose> poses(character.bones.size());
  std::vector<PendingPose> mesh_poses(character.meshes.size());
  for (const auto& ch : channels) {
    bool matched = false;
    for (size_t i = 0; i < character.bones.size(); ++i) {
      if (!channel_matches_bone(character.bones[i].name, ch.bone_name)) continue;
      switch (ch.type) {
        case ClipChannel::kPos: poses[i].pos = &ch; break;
        case ClipChannel::kScale: poses[i].scale = &ch; break;
        case ClipChannel::kQuat: poses[i].quat = &ch; break;
        case ClipChannel::kRotX: poses[i].rotx = &ch; break;
        case ClipChannel::kRotY: poses[i].roty = &ch; break;
        case ClipChannel::kRotZ: poses[i].rotz = &ch; break;
      }
      matched = true;
      break;
    }
    if (matched) continue;
    for (size_t i = 0; i < character.meshes.size(); ++i) {
      if (!channel_matches_bone(character.meshes[i].name, ch.bone_name)) continue;
      switch (ch.type) {
        case ClipChannel::kPos: mesh_poses[i].pos = &ch; break;
        case ClipChannel::kScale: mesh_poses[i].scale = &ch; break;
        case ClipChannel::kQuat: mesh_poses[i].quat = &ch; break;
        case ClipChannel::kRotX: mesh_poses[i].rotx = &ch; break;
        case ClipChannel::kRotY: mesh_poses[i].roty = &ch; break;
        case ClipChannel::kRotZ: mesh_poses[i].rotz = &ch; break;
      }
      break;
    }
  }

  for (size_t i = 0; i < character.bones.size(); ++i) {
    apply_pending_pose_weighted(poses[i], character.bones[i].local, weight,
                                relative);
  }
  for (size_t i = 0; i < character.meshes.size(); ++i) {
    apply_pending_pose_weighted(mesh_poses[i], character.meshes[i].local, weight,
                                relative);
  }
  if (debug_leg_pose_enabled()) dump_leg_pose(character);
}

void apply_clip_pose_sampled(const std::vector<ClipChannel>& channels,
                             float weight, Character& character,
                             bool relative) {
  apply_clip_pose_sampled_direct(channels, weight, character, relative);
}

void apply_clip_pose_weighted(const std::vector<ClipChannel>& channels,
                              float weight, Character& character,
                              bool relative) {
  apply_clip_pose_sampled(channels, weight, character, relative);
}

void apply_clip_frame(const CharClip& clip, int frame_idx, Character& character) {
  if (clip.frames.empty()) return;
  int fi = std::clamp(frame_idx, 0, (int)clip.frames.size() - 1);
  if (!apply_clip_pose_output_layer(clip.frames[(size_t)fi], 1.0f, character,
                                    clip.relative, clip.output_bones)) {
    apply_clip_pose_sampled_direct(clip.frames[(size_t)fi], 1.0f, character,
                                   clip.relative);
  }
  dump_arm_pose(character, "clip-frame-post");
}

void apply_clip_frame_weighted(const CharClip& clip, int frame_idx,
                               float weight, Character& character) {
  if (clip.frames.empty()) return;
  int fi = std::clamp(frame_idx, 0, (int)clip.frames.size() - 1);
  if (!apply_clip_pose_output_layer(clip.frames[(size_t)fi], weight, character,
                                    clip.relative, clip.output_bones)) {
    apply_clip_pose_sampled_direct(clip.frames[(size_t)fi], weight, character,
                                   clip.relative);
  }
  dump_arm_pose(character, "clip-frame-weighted-post");
}

float CharClip::duration_seconds() const {
  if (frames.empty()) return 0.0f;
  const float rate = fps > 0 ? static_cast<float>(fps) : 30.0f;
  float first = start_frame;
  float last = end_frame;
  if (last < first || last <= 0.0f) {
    first = 0.0f;
    last = static_cast<float>(frames.size() - 1);
  }
  return std::max(0.0f, (last - first + 1.0f) / rate);
}

namespace {

bool play_flags_loop(const CharClip& clip, uint32_t flags) {
  uint32_t loop_mode = flags & 0x70u;
  if (loop_mode == 0) loop_mode = clip.default_play_flags & 0x70u;
  return loop_mode != kCharPlayNoLoop;
}

uint32_t play_mode(const CharClip& clip, uint32_t flags) {
  uint32_t mode = flags & 0x0fu;
  if (mode == kCharPlayNoDefault) mode = clip.default_play_flags & 0x0fu;
  return mode;
}

int clip_frame_at_time(const CharClip& clip, float seconds, uint32_t flags) {
  if (clip.frames.empty()) return 0;
  const float rate = clip.fps > 0 ? static_cast<float>(clip.fps) : 30.0f;
  float first = clip.start_frame;
  float last = clip.end_frame;
  if (last < first || last <= 0.0f) {
    first = 0.0f;
    last = static_cast<float>(clip.frames.size() - 1);
  }

  const float frame_count = std::max(1.0f, last - first + 1.0f);
  float frame = first + seconds * rate;
  if (play_flags_loop(clip, flags)) {
    frame = std::fmod(frame - first, frame_count);
    if (frame < 0.0f) frame += frame_count;
    frame += first;
  } else {
    frame = std::clamp(frame, first, last);
  }

  return std::clamp(static_cast<int>(std::floor(frame)), 0,
                    static_cast<int>(clip.frames.size()) - 1);
}

float clip_frame_float_at_time(const CharClip& clip, float seconds,
                               uint32_t flags) {
  if (clip.frames.empty()) return 0.0f;
  const float rate = clip.fps > 0 ? static_cast<float>(clip.fps) : 30.0f;
  float first = clip.start_frame;
  float last = clip.end_frame;
  if (last < first || last <= 0.0f) {
    first = 0.0f;
    last = static_cast<float>(clip.frames.size() - 1);
  }

  const float frame_count = std::max(1.0f, last - first + 1.0f);
  float frame = first + seconds * rate;
  if (play_flags_loop(clip, flags)) {
    frame = std::fmod(frame - first, frame_count);
    if (frame < 0.0f) frame += frame_count;
    frame += first;
  } else {
    frame = std::clamp(frame, first, last);
  }
  return std::clamp(frame, 0.0f,
                    static_cast<float>(clip.frames.size() - 1));
}

const ClipChannel* matching_channel(const std::vector<ClipChannel>& frame,
                                    const ClipChannel& needle) {
  for (const auto& ch : frame) {
    if (ch.type == needle.type && ch.bone_name == needle.bone_name) return &ch;
  }
  return nullptr;
}

float blend_axis_angle(float a, float b, float t) {
  return wrap_ps2_angle(a + wrap_ps2_angle(b - a) * t);
}

void blend_channel_into(ClipChannel& out, const ClipChannel& rhs, float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  switch (out.type) {
    case ClipChannel::kPos:
      for (int i = 0; i < 3; ++i)
        out.pos[i] = out.pos[i] * (1.0f - t) + rhs.pos[i] * t;
      break;
    case ClipChannel::kScale:
      for (int i = 0; i < 3; ++i)
        out.scale[i] = out.scale[i] * (1.0f - t) + rhs.scale[i] * t;
      break;
    case ClipChannel::kQuat: {
      float dot = 0.0f;
      for (int i = 0; i < 4; ++i) dot += out.quat[i] * rhs.quat[i];
      float len = 0.0f;
      for (int i = 0; i < 4; ++i) {
        const float b = dot < 0.0f ? -rhs.quat[i] : rhs.quat[i];
        out.quat[i] = out.quat[i] * (1.0f - t) + b * t;
        len += out.quat[i] * out.quat[i];
      }
      len = std::sqrt(std::max(len, 1e-8f));
      for (float& q : out.quat) q /= len;
      break;
    }
    case ClipChannel::kRotX:
    case ClipChannel::kRotY:
    case ClipChannel::kRotZ:
      out.angle = blend_axis_angle(out.angle, rhs.angle, t);
      break;
  }
}

ClipChannel identity_channel_like(const ClipChannel& ch) {
  ClipChannel identity = ch;
  switch (identity.type) {
    case ClipChannel::kPos:
      identity.pos[0] = 0.0f;
      identity.pos[1] = 0.0f;
      identity.pos[2] = 0.0f;
      break;
    case ClipChannel::kScale:
      identity.scale[0] = 1.0f;
      identity.scale[1] = 1.0f;
      identity.scale[2] = 1.0f;
      break;
    case ClipChannel::kQuat:
      identity.quat[0] = 0.0f;
      identity.quat[1] = 0.0f;
      identity.quat[2] = 0.0f;
      identity.quat[3] = 1.0f;
      break;
    case ClipChannel::kRotX:
    case ClipChannel::kRotY:
    case ClipChannel::kRotZ:
      identity.angle = 0.0f;
      break;
  }
  return identity;
}

std::vector<ClipChannel> interpolate_frame(const CharClip& clip, float frame) {
  if (clip.frames.empty()) return {};
  const int f0 = std::clamp(static_cast<int>(std::floor(frame)), 0,
                            static_cast<int>(clip.frames.size()) - 1);
  const int f1 = std::min(f0 + 1, static_cast<int>(clip.frames.size()) - 1);
  const float t = std::clamp(frame - static_cast<float>(f0), 0.0f, 1.0f);
  if (f0 == f1 || t <= 1e-4f) return clip.frames[(size_t)f0];

  std::vector<ClipChannel> out = clip.frames[(size_t)f0];
  const auto& next = clip.frames[(size_t)f1];
  for (auto& ch : out) {
    const ClipChannel* b = matching_channel(next, ch);
    if (!b) continue;
    blend_channel_into(ch, *b, t);
  }
  return out;
}

std::vector<ClipChannel> blend_channel_sets(std::vector<ClipChannel> previous,
                                            const std::vector<ClipChannel>& current,
                                            float current_weight) {
  current_weight = std::clamp(current_weight, 0.0f, 1.0f);
  if (previous.empty() || current_weight >= 0.999f) return current;
  if (current.empty() || current_weight <= 0.001f) return previous;

  const size_t previous_count = previous.size();
  std::vector<bool> previous_matched(previous_count, false);
  for (const auto& ch : current) {
    bool matched = false;
    for (size_t i = 0; i < previous_count; ++i) {
      auto& out = previous[i];
      if (out.type != ch.type || out.bone_name != ch.bone_name) continue;
      blend_channel_into(out, ch, current_weight);
      previous_matched[i] = true;
      matched = true;
      break;
    }
    if (!matched) {
      ClipChannel out = identity_channel_like(ch);
      blend_channel_into(out, ch, current_weight);
      previous.push_back(out);
    }
  }
  for (size_t i = 0; i < previous_count; ++i) {
    if (previous_matched[i]) continue;
    const ClipChannel identity = identity_channel_like(previous[i]);
    blend_channel_into(previous[i], identity, current_weight);
  }
  return previous;
}

const char* channel_type_name(ClipChannel::Type type) {
  switch (type) {
    case ClipChannel::kPos: return "pos";
    case ClipChannel::kScale: return "scale";
    case ClipChannel::kQuat: return "quat";
    case ClipChannel::kRotX: return "rotx";
    case ClipChannel::kRotY: return "roty";
    case ClipChannel::kRotZ: return "rotz";
  }
  return "?";
}

bool lane_mixer_interesting_channel(const std::string& bone_name) {
  const bool body_channel =
      bone_name.find("hand") != std::string::npos ||
      bone_name.find("finger") != std::string::npos ||
      bone_name.find("thumb") != std::string::npos ||
      bone_name.find("fret") != std::string::npos ||
      bone_name.find("strum") != std::string::npos ||
      bone_name.find("clavicle") != std::string::npos ||
      bone_name.find("upperArm") != std::string::npos ||
      bone_name.find("foreArm") != std::string::npos ||
      bone_name.find("foreTwist") != std::string::npos ||
      bone_name.find("upperTwist") != std::string::npos;
  if (body_channel) return true;
  return debug_face_enabled() && is_face_quat_bone(bone_name);
}

void dump_lane_channel_value(const ClipChannel& ch) {
  switch (ch.type) {
    case ClipChannel::kPos:
      std::fprintf(stderr, " pos=(%.4f %.4f %.4f)", ch.pos[0], ch.pos[1],
                   ch.pos[2]);
      break;
    case ClipChannel::kScale:
      std::fprintf(stderr, " scale=(%.4f %.4f %.4f)", ch.scale[0],
                   ch.scale[1], ch.scale[2]);
      break;
    case ClipChannel::kQuat:
      std::fprintf(stderr, " quat=(%.4f %.4f %.4f %.4f)", ch.quat[0],
                   ch.quat[1], ch.quat[2], ch.quat[3]);
      break;
    case ClipChannel::kRotX:
    case ClipChannel::kRotY:
    case ClipChannel::kRotZ:
      std::fprintf(stderr, " angle=%.4f", ch.angle);
      break;
  }
}

bool is_axis_rot_channel(const ClipChannel& ch) {
  return ch.type == ClipChannel::kRotX || ch.type == ClipChannel::kRotY ||
         ch.type == ClipChannel::kRotZ;
}

bool is_quat_channel(const ClipChannel& ch) {
  return ch.type == ClipChannel::kQuat;
}

ClipChannel weighted_first_layer_channel(const ClipChannel& ch, float weight) {
  ClipChannel out = ch;
  if (is_axis_rot_channel(out)) {
    out.angle = wrap_ps2_angle(out.angle * weight);
  } else if (is_quat_channel(out)) {
    for (float& q : out.quat) q *= weight;
  }
  return out;
}

void accumulate_quat_channel(ClipChannel& out, const ClipChannel& rhs,
                             float weight) {
  float dot = 0.0f;
  for (int i = 0; i < 4; ++i) dot += out.quat[i] * rhs.quat[i];
  const float sign = dot < 0.0f ? -1.0f : 1.0f;
  for (int i = 0; i < 4; ++i) out.quat[i] += rhs.quat[i] * weight * sign;
}

void dump_lane_mixer_layers(const std::vector<ClipChannelLayer>& layers) {
  if (!debug_lane_mixer_enabled() || layers.empty()) return;

  std::string signature;
  for (size_t i = 0; i < layers.size(); ++i) {
    const auto& layer = layers[i];
    const std::string name = layer.debug_name.empty()
                                 ? ("layer" + std::to_string(i))
                                 : layer.debug_name;
    signature += name + ":" + std::to_string(layer.channels.size()) + ":" +
                 std::to_string(static_cast<int>(layer.weight * 1000.0f)) +
                 ";";
  }

  static std::unordered_set<std::string> seen_signatures;
  if (!seen_signatures.insert(signature).second) return;

  std::fprintf(stderr, "[lane-mix] layers=%zu signature=%s\n", layers.size(),
               signature.c_str());
  std::unordered_map<std::string, std::vector<std::string>> owners;
  for (size_t i = 0; i < layers.size(); ++i) {
    const auto& layer = layers[i];
    const std::string name = layer.debug_name.empty()
                                 ? ("layer" + std::to_string(i))
                                 : layer.debug_name;
    std::fprintf(stderr,
                 "[lane-mix]   layer %zu name=%s weight=%.3f channels=%zu "
                 "outputBones=%zu relative=%d overlay=%d\n",
                 i, name.c_str(), layer.weight, layer.channels.size(),
                 layer.output_bones ? layer.output_bones->size() : 0,
                 layer.relative ? 1 : 0,
                 layer.overlay_override ? 1 : 0);
    for (const auto& ch : layer.channels) {
      if (!lane_mixer_interesting_channel(ch.bone_name)) continue;
      const std::string key =
          std::string(channel_type_name(ch.type)) + ":" + ch.bone_name;
      owners[key].push_back(name);
      if (debug_face_enabled() && is_face_quat_bone(ch.bone_name)) {
        std::fprintf(stderr, "[lane-mix]     face %s %s",
                     name.c_str(), key.c_str());
        dump_lane_channel_value(ch);
        std::fprintf(stderr, "\n");
      }
    }
  }

  int printed = 0;
  for (const auto& [key, names] : owners) {
    if (names.size() < 2) continue;
    if (printed++ >= 96) {
      std::fprintf(stderr, "[lane-mix]   collision <truncated>\n");
      break;
    }
    std::fprintf(stderr, "[lane-mix]   collision %s <-", key.c_str());
    for (const auto& name : names) std::fprintf(stderr, " %s", name.c_str());
    std::fprintf(stderr, "\n");
  }
}

}  // namespace

std::vector<ClipChannel> blend_channel_layers(
    const std::vector<ClipChannelLayer>& layers) {
  dump_lane_mixer_layers(layers);
  struct AccumRef {
    size_t index = 0;
    float weight = 0.0f;
  };
  auto key_for = [](const ClipChannel& ch) {
    return std::to_string(static_cast<int>(ch.type)) + "\n" + ch.bone_name;
  };

  std::vector<ClipChannel> out;
  std::unordered_map<std::string, AccumRef> by_key;
  for (const auto& layer : layers) {
    const float layer_weight = std::max(0.0f, layer.weight);
    if (layer_weight <= 0.0f) continue;
    for (const auto& ch : layer.channels) {
      const std::string key = key_for(ch);
      const auto it = by_key.find(key);
      if (it == by_key.end()) {
        by_key.emplace(key, AccumRef{out.size(), layer_weight});
        out.push_back(weighted_first_layer_channel(ch, layer_weight));
        continue;
      }

      AccumRef& acc = it->second;
      (void)layer.overlay_override;
      if (is_quat_channel(ch)) {
        // SLUS 0x00168320 accumulates quaternion rows into the shared
        // destination block with sign correction. quat_to_rot() normalizes the
        // accumulated row later when it becomes a transform.
        accumulate_quat_channel(out[acc.index], ch, layer_weight);
        acc.weight += layer_weight;
        continue;
      }
      if (is_axis_rot_channel(ch)) {
        // SLUS 0x00168320 accumulates scalar output rows into the shared
        // destination block. Duplicated forearm axis rows from body + hand
        // lanes must therefore add, not normalize by total source count.
        out[acc.index].angle =
            wrap_ps2_angle(out[acc.index].angle + ch.angle * layer_weight);
        acc.weight += layer_weight;
        continue;
      }
      const float total = acc.weight + layer_weight;
      if (total <= 0.0f) continue;
      blend_channel_into(out[acc.index], ch, layer_weight / total);
      acc.weight = total;
    }
  }
  return out;
}

void apply_clip_channel_layers(const std::vector<ClipChannelLayer>& layers,
                               Character& character, bool relative) {
  std::vector<ClipChannelLayer> body_layers;
  body_layers.reserve(layers.size());
  for (const auto& layer : layers) {
    if (!layer.overlay_override) body_layers.push_back(layer);
  }

  const auto frame = blend_channel_layers(body_layers);
  if (frame.empty()) {
    apply_hand_driver_output_layers({}, character, relative, layers);
    return;
  }

  std::vector<CharClip::OutputBone> output_bones;
  std::unordered_set<std::string> output_keys;
  auto collect_output_bones = [&](bool overlay_sources) {
    for (const auto& layer : body_layers) {
      if (layer.overlay_override != overlay_sources || !layer.output_bones) {
        continue;
      }
      for (const auto& out : *layer.output_bones) {
        const std::string key = strip_transform_suffix(out.name);
        if (!output_keys.insert(key).second) continue;
        output_bones.push_back(out);
      }
    }
  };
  collect_output_bones(false);
  if (output_bones.empty()) {
    collect_output_bones(true);
  }

  if (apply_clip_pose_output_layer(frame, 1.0f, character, relative,
                                   output_bones)) {
    apply_hand_driver_output_layers(frame, character, relative, layers);
    return;
  }
  apply_clip_pose_sampled_direct(frame, 1.0f, character, relative);
  apply_hand_driver_output_layers(frame, character, relative, layers);
}

namespace {

void update_layer_stack_relative(ClipChannelLayerStack& stack, bool relative) {
  if (!stack.relative_set) {
    stack.relative = relative;
    stack.relative_set = true;
  } else if (stack.relative != relative) {
    stack.relative = false;
  }
}

bool is_overlay_lower_body_channel(const ClipChannel& channel) {
  const std::string key = strip_transform_suffix(channel.bone_name);
  return key == "bone_facing" || key == "bone_pelvis" ||
         key.find("-thigh") != std::string::npos ||
         key.find("-knee") != std::string::npos ||
         key.find("-ankle") != std::string::npos ||
         key.find("-foot") != std::string::npos ||
         key.find("-toe") != std::string::npos;
}

void strip_overlay_lower_body_channels(std::vector<ClipChannel>& channels) {
  channels.erase(std::remove_if(channels.begin(), channels.end(),
                                is_overlay_lower_body_channel),
                 channels.end());
}

}  // namespace

bool append_clip_player_layer(ClipChannelLayerStack& stack,
                              const CharClipPlayer& player, float weight,
                              bool overlay_override) {
  if (!player.active()) return false;
  std::vector<ClipChannelLayer> layers =
      player.sampled_pose_layers(weight, overlay_override);
  if (layers.empty()) return false;
  for (auto& layer : layers) {
    update_layer_stack_relative(stack, layer.relative);
    stack.layers.push_back(std::move(layer));
  }
  return true;
}

bool append_clip_player_layers(
    ClipChannelLayerStack& stack,
    const std::vector<ClipPlayerLayerSource>& sources) {
  bool appended = false;
  for (const auto& source : sources) {
    if (source.player == nullptr) continue;
    appended = append_clip_player_layer(stack, *source.player, source.weight,
                                        source.overlay_override) ||
               appended;
  }
  return appended;
}

bool append_clip_frame_layer(ClipChannelLayerStack& stack, const CharClip& clip,
                             int frame_idx, float weight,
                             bool overlay_override) {
  if (clip.frames.empty()) return false;
  const int fi =
      std::clamp(frame_idx, 0, static_cast<int>(clip.frames.size()) - 1);
  std::vector<ClipChannel> channels = clip.frames[static_cast<size_t>(fi)];
  if (overlay_override) strip_overlay_lower_body_channels(channels);
  if (channels.empty()) return false;
  update_layer_stack_relative(stack, clip.relative);
  stack.layers.push_back(ClipChannelLayer{
      std::move(channels), weight, &clip.output_bones, clip.name,
      clip.relative, overlay_override});
  return true;
}

bool append_clip_frame_layers(
    ClipChannelLayerStack& stack,
    const std::vector<ClipFrameLayerSource>& sources) {
  bool appended = false;
  for (const auto& source : sources) {
    if (source.clip == nullptr) continue;
    appended = append_clip_frame_layer(stack, *source.clip, source.frame_idx,
                                       source.weight,
                                       source.overlay_override) ||
               appended;
  }
  return appended;
}

CharacterPosePlayerLayerSources make_character_pose_player_layer_sources(
    const CharacterPosePlayerLayerBuildSources& sources) {
  CharacterPosePlayerLayerSources result;
  result.main = sources.main;
  result.face_base = sources.face_base;
  result.face = sources.face;
  if (!sources.hand_driver_active) return result;

  result.strum = sources.strum;
  result.fret = sources.fret;
  result.fret_extras = sources.fret_extras;
  if (sources.hand_weights != nullptr) {
    result.strum_weight = sources.hand_weights->right;
    result.fret_weight = sources.hand_weights->left;
  }
  return result;
}

bool append_character_pose_player_layers(
    ClipChannelLayerStack& stack,
    const CharacterPosePlayerLayerSources& sources) {
  std::vector<ClipPlayerLayerSource> layers = {
      {sources.main, 1.0f, false},
      {sources.face_base, 1.0f, false},
      {sources.strum, sources.strum_weight, true},
      {sources.fret, sources.fret_weight, true}};
  layers.reserve(layers.size() + sources.fret_extras.size() + 1);
  for (const CharClipPlayer* player : sources.fret_extras) {
    layers.push_back({player, sources.fret_weight, true});
  }
  layers.push_back({sources.face, 1.0f, false});
  return append_clip_player_layers(stack, layers);
}

CharacterPoseFrameLayerSources make_character_pose_frame_layer_sources(
    const CharacterPoseFrameLayerBuildSources& sources) {
  CharacterPoseFrameLayerSources result;
  result.main = sources.main;
  result.face_base = sources.face_base;
  result.face = sources.face;
  result.frame_idx = sources.frame_idx;
  if (!sources.hand_driver_active) return result;

  result.strum = sources.strum;
  result.fret = sources.fret;
  if (sources.hand_weights != nullptr) {
    result.strum_weight = sources.hand_weights->right;
    result.fret_weight = sources.hand_weights->left;
  }
  return result;
}

bool append_character_pose_frame_layers(
    ClipChannelLayerStack& stack,
    const CharacterPoseFrameLayerSources& sources) {
  const std::vector<ClipFrameLayerSource> layers = {
      {sources.main, sources.frame_idx, 1.0f, false},
      {sources.face_base, sources.frame_idx, 1.0f, false},
      {sources.strum, sources.frame_idx, sources.strum_weight, true},
      {sources.fret, sources.frame_idx, sources.fret_weight, true},
      {sources.face, sources.frame_idx, 1.0f, false}};
  return append_clip_frame_layers(stack, layers);
}

void apply_clip_layer_stack(const ClipChannelLayerStack& stack,
                            Character& character) {
  if (stack.layers.empty()) return;
  apply_clip_channel_layers(stack.layers, character, stack.relative);
}

CharacterPoseStackFrameResult apply_character_pose_stack_frame(
    Character& character,
    const ClipChannelLayerStack* stack) {
  CharacterPoseStackFrameResult result;
  clear_runtime_trans_worlds(character);
  if (stack != nullptr && !stack->layers.empty()) {
    apply_clip_layer_stack(*stack, character);
    result.applied_clip_layers = true;
  }
  return result;
}

CharacterPoseControllerFrameResult apply_character_pose_controller_frame(
    Character& character,
    const CharacterPoseControllerFrameSources& sources) {
  CharacterPoseControllerFrameResult result;

  const CharacterPoseStackFrameResult pose_result =
      apply_character_pose_stack_frame(character, sources.pose_stack);
  result.applied_clip_layers = pose_result.applied_clip_layers;

  if (!sources.controllers_enabled) return result;

  clear_runtime_ik_weights(character);
  if (sources.driver_weights != nullptr) {
    for (const auto& flag : sources.driver_weights->driver_flags) {
      set_runtime_driver_evaluate_flags(character, flag.driver, flag.flags,
                                        flag.weight);
      result.fed_driver_flags = result.fed_driver_flags ||
                                (!flag.driver.empty() && flag.flags != 0);
    }
  }

  for (const auto& fallback : sources.fallback_ik_weights) {
    if (fallback.weight_prop.empty()) continue;
    set_runtime_ik_weight(character, fallback.weight_prop, fallback.weight);
    ++result.fallback_ik_weights;
  }

  if (sources.midi_fret_target_enabled && !sources.midi_fret_target.empty()) {
    apply_ik_midi_fret_target(character, sources.midi_fret_target,
                              sources.time_seconds);
    result.applied_midi_fret_target = true;
  }

  apply_character_controllers(character, sources.time_seconds);
  result.applied_controllers = true;
  return result;
}

void CharClipPlayer::clear() {
  layers_.clear();
}

const CharClip* CharClipPlayer::current_clip() const {
  return layers_.empty() ? nullptr : layers_.back().clip;
}

void CharClipPlayer::play(const CharClip& clip, uint32_t flags,
                          float blend_width, float speed) {
  if (!clip.loaded || clip.frames.empty()) return;
  const uint32_t play_flags = char_clip_driver_masked_play_flags(clip, flags);
  const float resolved_blend =
      source_char_driver_resolve_blend_width(blend_width,
                                             source_driver_blend_width_);
  bool clip_already_playing = false;
  if (source_play_multiple_clips_) {
    for (const Layer& layer : layers_) {
      if (layer.clip == &clip) {
        clip_already_playing = true;
        break;
      }
    }
  }
  if (!source_char_driver_should_start_clip(source_play_multiple_clips_,
                                            clip_already_playing)) {
    return;
  }
  const bool no_blend =
      play_mode(clip, play_flags) == kCharPlayNoBlend ||
      resolved_blend <= 0.0f || layers_.empty();

  Layer next;
  next.clip = &clip;
  next.flags = play_flags;
  next.blend_width = no_blend ? 0.0f : resolved_blend;
  next.speed = speed;

  if (no_blend) {
    layers_.clear();
  } else if (layers_.size() > 1) {
    Layer prev = layers_.back();
    layers_.clear();
    layers_.push_back(prev);
  }
  layers_.push_back(next);
}

void CharClipPlayer::set_source_driver_blend_width(float blend_width) {
  if (!std::isfinite(blend_width)) blend_width = 1.0f;
  source_driver_blend_width_ = std::max(0.0f, blend_width);
}

void CharClipPlayer::set_source_play_multiple_clips(bool play_multiple_clips) {
  source_play_multiple_clips_ = play_multiple_clips;
}

void CharClipPlayer::set_speed(float speed) {
  if (!std::isfinite(speed) || speed <= 0.0f) speed = 1.0f;
  for (auto& layer : layers_) {
    layer.speed = speed;
  }
}

void CharClipPlayer::advance(float dt_seconds) {
  if (layers_.empty()) return;
  for (auto& layer : layers_) {
    if (!layer.clip) continue;
    layer.time_seconds += dt_seconds * layer.speed;
    const float duration = layer.clip->duration_seconds();
    if (duration > 0.0f) {
      if (play_flags_loop(*layer.clip, layer.flags)) {
        layer.time_seconds = std::fmod(layer.time_seconds, duration);
        if (layer.time_seconds < 0.0f) layer.time_seconds += duration;
      } else {
        layer.time_seconds = std::clamp(layer.time_seconds, 0.0f, duration);
      }
    }
  }
  Layer& current = layers_.back();
  if (current.blend_width > 0.0f) {
    current.blend_progress += std::max(0.0f, dt_seconds);
    if (current.blend_progress >= current.blend_width && layers_.size() > 1) {
      if (current.clip && !play_flags_loop(*current.clip, current.flags)) {
        current.blend_width = 0.0f;
        current.blend_progress = 0.0f;
      } else {
        Layer keep = current;
        keep.blend_width = 0.0f;
        keep.blend_progress = 0.0f;
        layers_.clear();
        layers_.push_back(keep);
      }
    }
  }
  if (layers_.size() > 1) {
    const Layer& exit_current = layers_.back();
    if (exit_current.clip &&
        !play_flags_loop(*exit_current.clip, exit_current.flags)) {
      const float duration = exit_current.clip->duration_seconds();
      if (duration > 0.0f && exit_current.time_seconds >= duration) {
        layers_.pop_back();
      }
    }
  }
}

void CharClipPlayer::apply(Character& character, float weight) const {
  if (layers_.empty() || weight <= 0.0f) return;
  weight = std::clamp(weight, 0.0f, 1.0f);
  const auto frame = sampled_pose();
  if (frame.empty()) return;
  const bool relative = sampled_pose_relative();
  const CharClip* current = current_clip();
  if (current && apply_clip_pose_output_layer(frame, weight, character, relative,
                                              current->output_bones)) {
    apply_hand_driver_output_layer(frame, character, relative,
                                   current->output_bones);
    return;
  }
  apply_clip_pose_sampled_direct(frame, weight, character, relative);
  if (current) {
    apply_hand_driver_output_layer(frame, character, relative,
                                   current->output_bones);
  }
}

std::vector<ClipChannelLayer> CharClipPlayer::sampled_pose_layers(
    float weight, bool overlay_override) const {
  std::vector<ClipChannelLayer> out;
  if (layers_.empty()) return out;

  auto append_sample = [&](const CharClip& clip, int frame_idx,
                           float sample_weight) {
    if (clip.frames.empty()) return;
    const int fi =
        std::clamp(frame_idx, 0, static_cast<int>(clip.frames.size()) - 1);
    std::vector<ClipChannel> channels = clip.frames[static_cast<size_t>(fi)];
    if (overlay_override) strip_overlay_lower_body_channels(channels);
    if (channels.empty()) return;
    out.push_back(ClipChannelLayer{std::move(channels),
                                   sample_weight,
                                   &clip.output_bones,
                                   clip.name,
                                   clip.relative,
                                   overlay_override});
  };

  if (layers_.size() == 1) {
    const Layer& layer = layers_.back();
    if (!layer.clip) return out;
    const float frame =
        clip_frame_float_at_time(*layer.clip, layer.time_seconds, layer.flags);
    const int f0 = std::clamp(static_cast<int>(std::floor(frame)), 0,
                              static_cast<int>(layer.clip->frames.size()) - 1);
    const int f1 = std::min(f0 + 1,
                            static_cast<int>(layer.clip->frames.size()) - 1);
    const float frac = std::clamp(frame - static_cast<float>(f0), 0.0f, 1.0f);
    append_sample(*layer.clip, f0, (1.0f - frac) * weight);
    if (frac > 0.0f && f1 != f0) {
      append_sample(*layer.clip, f1, frac * weight);
    }
    return out;
  }

  // CharBonesSamples::ScaleAddSample gives the adjacent-sample split inside a
  // clip. CharClipDriver::ScaleAdd still lacks a reviewable statement body, so
  // multi-node driver blends keep the existing collapsed diagnostic layer.
  auto channels = sampled_pose();
  if (overlay_override) strip_overlay_lower_body_channels(channels);
  if (channels.empty()) return out;
  const bool relative = sampled_pose_relative();
  const CharClip* clip = current_clip();
  out.push_back(ClipChannelLayer{
      std::move(channels), weight, clip ? &clip->output_bones : nullptr,
      clip ? clip->name : std::string{}, relative, overlay_override});
  return out;
}

std::vector<ClipChannel> CharClipPlayer::sampled_pose() const {
  if (layers_.empty()) return {};
  const Layer& current = layers_.back();
  const float current_weight = current_blend_weight();

  if (layers_.size() > 1) {
    const Layer& previous = layers_[layers_.size() - 2];
    if (previous.clip) {
      const float ff = clip_frame_float_at_time(*previous.clip,
                                                previous.time_seconds,
                                                previous.flags);
      const auto previous_frame = interpolate_frame(*previous.clip, ff);
      if (current.clip) {
        const float current_ff = clip_frame_float_at_time(*current.clip,
                                                          current.time_seconds,
                                                          current.flags);
        const auto current_frame = interpolate_frame(*current.clip, current_ff);
        return blend_channel_sets(previous_frame, current_frame,
                                  current_weight);
      }
      return previous_frame;
    }
  }

  if (current.clip) {
    const float ff = clip_frame_float_at_time(*current.clip,
                                              current.time_seconds,
                                              current.flags);
    return interpolate_frame(*current.clip, ff);
  }
  return {};
}

bool CharClipPlayer::sampled_pose_relative() const {
  const CharClip* clip = current_clip();
  return clip && clip->relative;
}

float CharClipPlayer::current_blend_weight() const {
  if (layers_.empty()) return 0.0f;
  const Layer& current = layers_.back();
  if (current.blend_width <= 0.0f) return 1.0f;
  return std::clamp(current.blend_progress / current.blend_width, 0.0f,
                    1.0f);
}

float CharClipPlayer::evaluate_flags(uint32_t flags) const {
  if (layers_.empty()) return 0.0f;
  const Layer& current = layers_.back();
  const float current_flag =
      current.clip ? source_char_driver_evaluate_flags_from_clip_flags(
                         current.clip->flags, flags)
                   : 0.0f;
  if (layers_.size() <= 1) return current_flag;

  const Layer& previous = layers_[layers_.size() - 2];
  const float previous_flag =
      previous.clip ? source_char_driver_evaluate_flags_from_clip_flags(
                          previous.clip->flags, flags)
                    : 0.0f;
  const float current_weight = current_blend_weight();
  return std::clamp(previous_flag * (1.0f - current_weight) +
                        current_flag * current_weight,
                    0.0f, 1.0f);
}

bool CharClipPlayer::source_starved() const {
  if (layers_.empty()) return source_char_driver_starved(false, false, 0);
  return source_char_driver_starved(true, layers_.size() > 1,
                                    layers_.back().flags);
}

// Legacy single-frame entry point (frame 0).
std::vector<ClipChannel> load_clip_pose(const std::string& hdr_path,
                                        const std::string& ark_path,
                                        const std::string& milo_path,
                                        const std::string& clip_name) {
  CharClip c = load_clip(hdr_path, ark_path, milo_path, clip_name);
  if (c.frames.empty()) return {};
  return c.frames[0];
}

}  // namespace ghogx::character
