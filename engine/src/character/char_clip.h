// engine/src/character/char_clip.h
//
// CharClipSamples decoder: loads all frames from a GH2 PS2 animation clip.

#pragma once

#include "character/char_mesh.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ghogx::character {

enum SourceCharBonesType {
  kSourceCharBonesTypePos = 0,
  kSourceCharBonesTypeScale = 1,
  kSourceCharBonesTypeQuat = 2,
  kSourceCharBonesTypeRotX = 3,
  kSourceCharBonesTypeRotY = 4,
  kSourceCharBonesTypeRotZ = 5,
  kSourceCharBonesTypeEnd = 6,
};

struct SourceCharBonesLayout {
  std::array<int, kSourceCharBonesTypeEnd + 1> counts = {};
  std::array<int, kSourceCharBonesTypeEnd + 1> offsets = {};
  int total_size = 0;
};

struct SourceCharBonesCompressionUpdate {
  int compression = 0;
  SourceCharBonesLayout layout;
  bool changed = false;
};

struct SourceCharBonesBone {
  std::string name;
  float weight = 1.0f;
};

struct SourceCharBonesState {
  int compression = 0;
  SourceCharBonesLayout layout;
  std::vector<SourceCharBonesBone> bones;
};

struct SourceCharBonesFindPtrResult {
  bool found = false;
  int offset = -1;
};

struct SourceCharBonesScaleAddClipStep {
  bool call_clip_scale_add = true;
  float f1 = 0.0f;
  float f2 = 0.0f;
  float f3 = 0.0f;
};

struct SourceCharBonesPoseBodyBoundary {
  bool rb3_latest_declares_scale_add = true;
  bool rb3_latest_declares_rotate_by = true;
  bool rb3_latest_declares_rotate_to = true;
  bool rb3_latest_declares_blend = true;
  bool rb3_latest_declares_scale_down = true;
  bool rb3_latest_exposes_scale_add_body = false;
  bool rb3_latest_exposes_rotate_by_body = false;
  bool rb3_latest_exposes_rotate_to_body = false;
  bool rb3_latest_exposes_blend_body = false;
  bool rb3_latest_exposes_scale_down_body = false;
  bool rb2_dump_maps_scale_add = true;
  bool rb2_dump_maps_rotate_by = true;
  bool rb2_dump_maps_rotate_to = true;
  bool rb2_dump_maps_scale_down = true;
  bool rb2_dump_maps_scale_add_identity = true;
  bool rb2_dump_exposes_statement_body = false;
  bool safe_to_use_layout_helpers = true;
  bool safe_to_apply_pose_math = false;
  std::vector<std::string> fenced_bodies;
};

struct SourceCharBonesRuntimeDumpEvidence {
  std::string scale_down_range;
  std::string scale_add_range;
  std::string rotate_by_range;
  std::string rotate_to_range;
  std::string scale_add_identity_range;
  std::vector<std::string> scale_down_locals;
  std::vector<std::string> scale_add_locals;
  std::vector<std::string> rotate_by_locals;
  std::vector<std::string> rotate_to_locals;
  std::vector<std::string> scale_add_identity_locals;
  bool rb2_dump_maps_blend = false;
  bool has_scale_down_statement_body = false;
  bool has_scale_add_statement_body = false;
  bool has_rotate_by_statement_body = false;
  bool has_rotate_to_statement_body = false;
  bool has_scale_add_identity_statement_body = false;
  bool safe_to_apply_pose_math = false;
};

struct SourceCharBonesAddBonesSteps {
  std::vector<SourceCharBonesBone> add_bone_internal_calls;
  bool reallocate_internal = false;
};

struct SourceCharBonesAllocReallocateStep {
  bool free_m_start = true;
  int mem_alloc_size = 0;
  bool assign_m_start = true;
};

struct SourceCharBonesEnterStep {
  bool zero = true;
  bool set_weights = true;
  float set_weights_value = 0.0f;
};

struct SourceCharBonesBlenderPollStep {
  bool early_out = false;
  bool blend_dest = false;
  bool enter = false;
};

struct SourceCharBonesBlenderSetDestStep {
  bool changed = false;
  bool assign_dest = false;
  bool add_bones_to_dest = false;
};

struct SourceCharBonesBlenderSetClipTypeStep {
  bool changed = false;
  bool assign_clip_type = false;
  bool clear_bones = false;
  bool stuff_bones_from_dir = false;
};

struct SourceCharBonesBlenderReallocateStep {
  bool char_bones_alloc_reallocate_internal = true;
  bool add_bones_to_dest = false;
  bool enter = true;
};

struct SourceCharBonesBlenderLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  std::vector<std::string> call_order;
  std::vector<std::string> branches;
};

struct SourceCharBonesBlenderCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> member_calls;
};

struct SourceCharBonesBlenderHandlerPlan {
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharBonesBlenderPropSyncPlan {
  std::vector<std::string> set_properties;
  std::vector<std::string> superclasses;
};

struct SourceCharBoneLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
};

struct SourceCharBoneCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

struct SourceCharBoneHandlerPlan {
  std::vector<std::string> action_handlers;
  std::vector<std::string> handlers;
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharBoneWeightContextPropSyncPlan {
  std::vector<std::string> properties;
};

struct SourceCharBonePropSyncPlan {
  std::vector<std::string> properties;
  std::vector<std::string> superclasses;
};

struct SourceCharBonesBonePropSyncPlan {
  std::vector<std::string> properties;
  std::vector<std::string> set_properties;
  bool preview_uses_prop_bones_string_val = true;
};

struct SourceCharBonesObjectPropSyncPlan {
  bool assigns_prop_bones = true;
  std::vector<std::string> custom_branches;
};

struct SourceCharBoneDirDefaultState {
  bool recenter_targets_no_null = true;
  bool recenter_average_no_null = true;
  bool recenter_slide = false;
  int move_context = 0;
  bool bake_out_facing = true;
  bool context_flags_is_int = true;
  int context_flags_int = 0;
  int filter_context = 0;
  bool filter_bones_no_null = true;
  bool filter_names_empty = true;
};

struct SourceCharBoneDirLoadPlan {
  bool known_revision = false;
  std::vector<std::string> preload_order;
  std::vector<std::string> load_order;
  std::vector<std::string> postload_order;
  std::vector<std::string> branches;
};

struct SourceCharBoneDirCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

struct SourceCharBoneDirHandlerPlan {
  std::vector<std::string> handlers;
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharBoneDirRecenterPropSyncPlan {
  std::vector<std::string> properties;
};

struct SourceCharBoneDirPropSyncPlan {
  std::vector<std::string> properties;
  std::vector<std::string> set_properties;
  std::vector<std::string> modify_properties;
  std::vector<std::string> modify_actions;
  std::vector<std::string> superclasses;
};

struct SourceCharBoneDirClipTypeResource {
  std::string clip_type;
  bool has_resource = false;
  std::string resource_name;
  int context_mask = 0;
  bool resource_found = false;
  std::string context_symbol;
};

struct SourceCharBoneDirInitClipTypeRow {
  std::string clip_type;
  bool has_resource = false;
  std::string resource_name;
  bool already_loaded = false;
  bool load_succeeds = true;
};

struct SourceCharBoneDirInitPlan {
  bool creates_char_resources = true;
  bool reads_resource_path = true;
  bool reads_char_clip_types = true;
  bool skipped_missing_clip_types = false;
  bool skipped_empty_resource_path = false;
  bool registers_get_clip_types = false;
  size_t scanned_rows = 0;
  std::vector<std::string> skipped_existing_resources;
  std::vector<std::string> load_requests;
  std::vector<std::string> named_loaded_resources;
  std::vector<std::string> failed_load_resources;
};

struct SourceCharBoneDirTerminatePlan {
  bool deletes_resources = true;
  bool clears_resources_pointer = false;
};

struct SourceCharBoneDirFindResourceResult {
  bool found = false;
  std::string resource_name;
};

struct SourceCharBoneDirResourceLookupResult {
  bool clip_type_found = false;
  bool resource_field_found = false;
  bool resource_found = false;
  std::string resource_name;
  int context_mask = 0;
  std::string warning;
};

struct SourceCharBoneDirStuffBonesSymbolStep {
  SourceCharBoneDirResourceLookupResult lookup;
  bool call_stuff_bones = false;
  int context_mask = 0;
};

struct SourceCharBoneDirContextFlagsStep {
  bool rebuilt = false;
  size_t scanned_rows = 0;
  std::vector<std::string> context_flags;
};

struct SourceCharBoneDirMergeTransform {
  std::string name;
  bool is_loaded_dir = false;
  bool animatable = false;
};

struct SourceCharBoneDirMergeCharacterPlan {
  bool load_attempted = true;
  bool loaded = false;
  bool warned_failed_load = false;
  size_t scanned_transforms = 0;
  std::vector<std::string> selected_transforms;
  bool merge_body_fenced = true;
};

struct SourceCharBonesMeshesReplaceStep {
  bool object_replace = true;
  bool scan_meshes = false;
  int replaced_index = -1;
  bool assigned_dummy = false;
  std::vector<std::string> meshes;
};

struct SourceCharBonesMeshesReallocateStep {
  bool char_bones_alloc_reallocate_internal = true;
  std::vector<std::string> meshes;
  std::vector<std::string> missing_non_facing_bones;
  bool acquire_pose = false;
};

struct SourceCharBonesMeshesPoseDumpEvidence {
  std::string pose_meshes_range;
  std::string prop_sync_range;
  std::vector<std::string> pose_meshes_locals;
  bool latest_source_body_incomplete = true;
  bool rb2_dump_has_statement_body = false;
  bool safe_to_pose_meshes = false;
  bool safe_to_publish_mesh_transforms = false;
};

struct SourceCharServoBoneDefaultState {
  bool pelvis_null = true;
  bool facing_rot_delta_null = true;
  bool facing_pos_delta_null = true;
  bool facing_rot_null = true;
  bool facing_pos_null = true;
  bool move_self = false;
  bool delta_changed = false;
  bool regulate_empty = true;
};

struct SourceCharServoBoneSetClipTypeStep {
  bool changed = false;
  bool assign_clip_type = false;
  bool clear_bones = false;
  bool stuff_bones_from_dir = false;
};

struct SourceCharServoBoneEnterStep {
  bool zero_deltas = true;
  bool clear_regulate = true;
  bool delta_changed = false;
  bool move_self = false;
};

struct SourceCharServoBoneSetMoveSelfStep {
  bool changed = false;
  bool move_self = false;
  bool delta_changed = false;
};

struct SourceCharServoBoneCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
  bool calls_set_clip_type = true;
};

struct SourceCharServoBoneLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  std::vector<std::string> call_order;
  std::vector<std::string> branches;
};

struct SourceCharServoBoneHandlerPlan {
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharServoBonePropSyncPlan {
  std::vector<std::string> set_properties;
  std::vector<std::string> properties;
  std::vector<std::string> superclasses;
};

struct SourceCharServoBoneRuntimeDumpEvidence {
  std::string poll_range;
  std::string regulate_override_range;
  std::string regulate_range;
  std::string poll_deps_range;
  std::vector<std::string> poll_locals;
  std::vector<std::string> regulate_override_locals;
  std::vector<std::string> regulate_locals;
  bool rb2_dump_has_statement_body = false;
  bool latest_source_has_poll_body = false;
  bool safe_to_run_poll = false;
  bool safe_to_run_regulate = false;
  bool safe_to_publish_servo_motion = false;
};

struct SourceCharBonesSamplesState {
  SourceCharBonesState bones;
  int num_samples = 0;
  int preview_sample = 0;
  int start_offset = 0;
  int raw_data_size = 0;
  std::vector<float> frames;
};

struct SourceCharBonesSampleStep {
  int start_offset = 0;
  float weight = 0.0f;
};

struct SourceCharBonesSamplesLoadPlan {
  bool known_version = false;
  std::vector<std::string> read_order;
};

struct SourceGrimCharBonesSamplesHeaderPlan {
  bool known_version = false;
  int count_size = 0;
  bool defaults_weight = false;
  bool reads_weight = false;
  bool reads_frame_table = false;
  bool aligns_sample_data_to_4 = false;
  std::vector<std::string> read_order;
};

struct SourceGrimCharClipLoadPlan {
  bool known_version = false;
  bool reads_object_meta = false;
  bool reads_range = false;
  bool skips_v5_unknown_bool = false;
  bool reads_relative = false;
  bool reads_unknown_1 = false;
  bool reads_do_not_decompress = false;
  bool reads_node_size = false;
  bool reads_deprecated_events = false;
  bool reads_events = false;
  std::vector<std::string> read_order;
};

struct SourceGrimCharClipSamplesLoadPlan {
  bool known_version = false;
  bool calls_char_clip_with_meta = false;
  bool reads_some_bool = false;
  bool legacy_split_headers_and_data = false;
  bool reads_duplicate_legacy_header = false;
  bool reads_extra_bones = false;
  int runtime_data_lists = 0;
  std::vector<std::string> read_order;
};

struct SourceReNotesCharBonesSamplesDecodePlan {
  bool sample_data_grouped_by_time = true;
  bool has_generic_rot_sample = true;
  bool active_reader_counts_pos = true;
  bool active_reader_counts_quat = true;
  bool active_reader_counts_rotz = true;
  bool active_reader_counts_rotx = false;
  bool active_reader_counts_roty = false;
  bool active_reader_counts_scale = false;
  std::vector<std::string> active_sample_order;
  std::vector<std::string> fenced_channels;
};

struct SourceCharBonesSamplesPropSyncPlan {
  std::vector<std::string> properties;
  std::vector<std::string> set_properties;
  std::vector<std::string> custom_branches;
};

struct SourceCharBonesSamplesBodyBoundary {
  bool rb3_latest_load_delegates_header = true;
  bool rb3_latest_load_delegates_data = true;
  bool rb3_latest_declares_load_header = true;
  bool rb3_latest_declares_load_data = true;
  bool rb3_latest_declares_evaluate_channel = true;
  bool rb3_latest_exposes_load_header_body = false;
  bool rb3_latest_exposes_load_data_body = false;
  bool rb3_latest_exposes_evaluate_channel_body = false;
  bool rb2_dump_maps_load_header = true;
  bool rb2_dump_maps_load_data = true;
  bool rb2_dump_maps_evaluate_channel = true;
  bool rb2_dump_exposes_statement_body = false;
  bool safe_to_decode_logged_rows = true;
  bool safe_to_publish_pose = false;
  std::vector<std::string> fenced_bodies;
};

struct SourceCharBonesSamplesRuntimeDumpEvidence {
  std::string frac_to_sample_range;
  std::string evaluate_channel_range;
  std::string rotate_by_range;
  std::string rotate_to_range;
  std::string scale_add_sample_range;
  std::string relativize_range;
  std::string load_range;
  std::string read_counts_range;
  std::string load_header_range;
  std::string load_data_range;
  std::string sync_property_range;
  std::vector<std::string> frac_to_sample_locals;
  std::vector<std::string> evaluate_channel_locals;
  std::vector<std::string> relativize_locals;
  std::vector<std::string> load_header_locals;
  std::vector<std::string> load_data_locals;
  bool has_load_header_statement_body = false;
  bool has_load_data_statement_body = false;
  bool has_evaluate_channel_statement_body = false;
  bool has_relativize_statement_body = false;
  bool safe_to_decode_logged_rows = true;
  bool safe_to_publish_pose = false;
};

struct SourceCharClipSamplesRuntimeDumpEvidence {
  std::string facing_bones_set_range;
  std::string facing_set_scale_add_range;
  std::string frame_to_sample_range;
  std::string get_channel_range;
  std::string evaluate_channel_range;
  std::string evaluate_channel_sample_range;
  std::string rotate_by_range;
  std::string rotate_to_range;
  std::string scale_add_frame_range;
  std::string scale_add_sample_range;
  std::string relativize_range;
  std::string set_relative_range;
  std::string load_range;
  std::vector<std::string> facing_set_scale_add_locals;
  std::vector<std::string> evaluate_channel_locals;
  std::vector<std::string> evaluate_channel_sample_locals;
  std::vector<std::string> rotate_by_locals;
  std::vector<std::string> rotate_to_locals;
  std::vector<std::string> scale_add_frame_locals;
  std::vector<std::string> load_locals;
  bool has_evaluate_channel_statement_body = false;
  bool has_rotate_by_statement_body = false;
  bool has_scale_add_statement_body = false;
  bool has_load_statement_body = false;
  bool safe_to_publish_pose = false;
};

enum class SourceCharUtlObjectKind {
  kTransformable,
  kMesh,
  kCamera,
  kDirectory,
  kCharBone,
  kCharCollide,
  kCharCuff,
};

struct SourceCharUtlObject {
  std::string name;
  SourceCharUtlObjectKind kind = SourceCharUtlObjectKind::kTransformable;
  int mesh_bone_count = 0;
  std::string char_bone_transform;
};

struct SourceCharUtlBoneTransResult {
  std::string lookup_name;
  std::string resolved_name;
  bool via_char_bone = false;
};

struct SourceCharUtlMergeBone {
  std::string name;
  std::string target;
  int32_t position_context = 0;
  int32_t scale_context = 0;
  int32_t rotation_type = kSourceCharBonesTypeEnd;
  int32_t rotation_context = 0;
};

struct SourceCharUtlMergeWarning {
  std::string code;
  std::string bone_name;
  std::string source_name;
  std::string dest_name;
};

struct SourceCharUtlMergeResult {
  std::vector<SourceCharUtlMergeBone> dest_bones;
  std::vector<SourceCharUtlMergeWarning> warnings;
};

struct SourceCharUtlTransformRow {
  std::string name;
  bool has_parent = false;
};

struct SourceCharUtlClipPredictFrame {
  std::array<float, 3> facing_pos = {0.0f, 0.0f, 0.0f};
  float facing_rot = 0.0f;
};

struct SourceCharUtlClipPredictState {
  std::array<float, 3> pos = {0.0f, 0.0f, 0.0f};
  float ang = 0.0f;
  std::array<float, 3> last_pos = {0.0f, 0.0f, 0.0f};
  float last_ang = 0.0f;
};

struct SourceCharUtlInitPlan {
  std::vector<std::string> registered_functions;
  std::vector<std::string> reset_hair_handler_steps;
  std::vector<std::string> char_merge_bones_handler_steps;
  bool char_merge_bones_deletes_loaded_dir = true;
};

struct SourceCharLookAtBounds {
  std::array<float, 3> min = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> max = {0.0f, 0.0f, 0.0f};
};

struct SourceCharLookAtLimitState {
  float min_yaw = -80.0f;
  float max_yaw = 80.0f;
  float min_pitch = -80.0f;
  float max_pitch = 80.0f;
  SourceCharLookAtBounds bounds;
};

struct SourceCharLookAtEnterState {
  std::array<float, 3> smoothed_dir = {1.0e29f, 0.0f, 0.0f};
  bool reset_pivot_local = false;
};

struct SourceCharLookAtPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharLookAtPollPlan {
  bool poll_gate_open = false;
  bool compute_dest_vector = false;
  bool apply_weight_yaw = false;
  bool skip_zero_weight = false;
  bool update_source_radius_history = false;
  bool clamp_source_radius_offset = false;
  bool write_pivot_world_to_source = false;
  bool normalize_dest_vector = false;
  bool transform_to_parent_space = false;
  bool clamp_bounds = false;
  bool smooth_half_time = false;
  bool use_test_range = false;
  bool use_show_range = false;
  bool apply_jitter = false;
  bool subtract_source_radius_offset = false;
  bool write_roll_local_rotation = false;
  bool write_no_roll_axes = false;
};

struct SourceCharLookAtYawWeightResult {
  bool applied = false;
  bool speed_limited = false;
  float dot_clamped = 0.0f;
  float target_yaw_weight = 1.0f;
  float updated_yaw_weight = 1.0f;
  float final_weight = 1.0f;
};

struct SourceCharLookAtNoRollAxesResult {
  std::array<float, 3> x = {1.0f, 0.0f, 0.0f};
  std::array<float, 3> y = {0.0f, 1.0f, 0.0f};
  std::array<float, 3> z = {0.0f, 0.0f, 1.0f};
  bool invalid_xx = false;
};

struct SourceCharLookAtSmoothResult {
  bool applied = false;
  float factor = 0.0f;
  std::array<float, 3> dir = {0.0f, 1.0f, 0.0f};
};

struct SourceCharLookAtRangeResult {
  bool applied = false;
  bool used_test_range = false;
  bool used_show_range = false;
  bool force_weight_one = false;
  int show_range_case = -1;
  std::array<float, 3> dir = {0.0f, 1.0f, 0.0f};
};

struct SourceCharLookAtSourceRadiusResult {
  bool active = false;
  bool updated_history = false;
  bool clamped_to_radius = false;
  float radius_radians = 0.0f;
  float pre_clamp_length_sq = 0.0f;
  std::array<float, 3> history = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> offset = {0.0f, 0.0f, 0.0f};
};

struct SourceCharLookAtLoadPlan {
  bool revision_supported = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
  bool sync_limits = false;
};

struct SourceCharLookAtCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
  bool sync_limits = false;
};

// One channel value for one frame.
struct ClipChannel {
  enum Type { kPos, kScale, kQuat, kRotX, kRotY, kRotZ } type = kPos;
  std::string bone_name;  // name without suffix (e.g. "bone_R-clavicle")
  float pos[3] = {};      // kPos: X,Y,Z
  float scale[3] = {1.0f, 1.0f, 1.0f};  // kScale: local X,Y,Z scale
  float quat[4] = {};     // kQuat: X,Y,Z,W
  float angle = 0.0f;     // kRotX/kRotY/kRotZ: radians
};

// All frames of one clip, indexed [frame][channel].
struct CharClip {
  std::string name;
  std::vector<std::vector<ClipChannel>> frames;  // frames[f][ch]
  struct RawChannelCounts {
    int pos = 0;
    int scale = 0;
    int quat = 0;
    int rotx = 0;
    int roty = 0;
    int rotz = 0;
  };
  struct OutputBone {
    std::string name;    // CharBone entry name, normally bone_*.trans
    std::string parent;  // CharBone parent, normally another *.trans
    milo_scene::Xfm local;
    milo_scene::Xfm world_stored;
    uint32_t char_bone_version = 0;
    uint32_t trans_version = 0;
    uint32_t trans_constraint = 0;
    std::string trans_target;
    bool preserve_scale = false;
    int32_t position_context = 0;
    int32_t scale_context = 0;
    int32_t rotation_type = 6;  // ihatecompvir CharBones::TYPE_END.
    int32_t rotation_context = 0;
    int32_t legacy_pre_rev5_int = 0;
    bool has_legacy_pre_rev5_int = false;
    int32_t legacy_rev3_to_7_int = 0;
    bool has_legacy_rev3_to_7_int = false;
    std::string target;
    struct WeightContext {
      int32_t context = 0;
      float weight = 0.0f;
    };
    std::vector<WeightContext> weights;
    std::string trans;
    bool bake_out_as_top_level = false;
    size_t unread_bytes = 0;
  };
  // Animation MILOs carry CharBone output records beside CharClipSamples.
  // The public ihatecompvir snapshot used by this worktree does not include
  // the runtime pose publisher, so broad output publishing remains diagnostic.
  // Full, face, and lower-body output bridges must stay behind explicit
  // diagnostic enable switches until a source CharBones publisher is ported.
  std::vector<OutputBone> output_bones;
  // Raw header channel counts are diagnostic evidence for the source-backed
  // decode boundary. `.scale`, `.rotx`, and `.roty` are consumed but not
  // published until the missing EvaluateChannel/pose body is sourced.
  RawChannelCounts raw_channel_counts;
  int fps = 30;        // authored clip playback rate
  float start_frame = 0.0f;
  float end_frame = 0.0f;
  uint32_t flags = 0;
  uint32_t default_play_flags = 0;
  float blend_width = 0.0f;
  float range = 0.0f;
  bool relative = false;
  bool loaded = false;

  float duration_seconds() const;
};

// Source-backed CharClipGroup load state. The public ihatecompvir source stores
// mClips, mWhich, and mFlags, and GetClip() advances mWhich in-place.
struct CharClipGroup {
  std::string name;
  std::string milo_path;
  std::vector<std::string> clips;
  uint32_t version = 0;
  int32_t which = 0;
  int32_t flags = 0;
  bool loaded = false;
};

struct SourceCharClipGroupLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  bool read_flags = false;
  int32_t default_flags = 0;
};

struct SourceCharClipGroupHandlerPlan {
  std::vector<std::string> handlers;
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharClipGroupPropSyncPlan {
  std::vector<std::string> properties;
};

struct SourceCharClipRefOwner {
  bool is_clip_group = false;
  std::vector<std::string> group_clips;
};

struct ClipChannelLayer {
  std::vector<ClipChannel> channels;
  float weight = 1.0f;
  const std::vector<CharClip::OutputBone>* output_bones = nullptr;
  std::string debug_name;
  bool relative = false;
  bool overlay_override = false;
};

enum CharPlayFlags : uint32_t {
  kCharPlayNoDefault = 0x00000000u,
  kCharPlayNow       = 0x00000001u,
  kCharPlayNoBlend   = 0x00000002u,
  kCharPlayFirst     = 0x00000003u,
  kCharPlayLast      = 0x00000004u,
  kCharPlayDirty     = 0x00000008u,
  kCharPlayNoLoop    = 0x00000010u,
  kCharPlayLoop      = 0x00000020u,
  kCharPlayGraphLoop = 0x00000030u,
  kCharPlayNodeLoop  = 0x00000040u,
  kCharPlayRealTime  = 0x00000200u,
  kCharPlayUserTime  = 0x00000400u,
};

// Lightweight viewer-side CharDriver play-node emulation. It owns clip time,
// loop/clamp behavior, and the previous-node blend that the game runtime uses
// when a new clip is started without kCharPlayNoBlend.
class CharClipPlayer {
 public:
  void clear();
  void play(const CharClip& clip, uint32_t flags = kCharPlayLoop,
            float blend_width = -1.0f, float speed = 1.0f);
  void set_source_driver_blend_width(float blend_width);
  void set_source_play_multiple_clips(bool play_multiple_clips);
  void set_speed(float speed);
  void advance(float dt_seconds);
  void apply(Character& character, float weight = 1.0f) const;
  std::vector<ClipChannel> sampled_pose() const;
  bool sampled_pose_relative() const;
  float current_blend_weight() const;
  bool source_starved() const;
  bool active() const { return !layers_.empty(); }
  const CharClip* current_clip() const;

 private:
  struct Layer {
    const CharClip* clip = nullptr;
    uint32_t flags = 0;
    float time_seconds = 0.0f;
    float blend_width = 0.0f;
    float blend_progress = 0.0f;
    float speed = 1.0f;
  };

  float source_driver_blend_width_ = 1.0f;
  bool source_play_multiple_clips_ = false;
  std::vector<Layer> layers_;
};

// Load all frames of a named CharClipSamples entry from the PS2 ARK.
// Returns a CharClip with frames.empty() on failure.
CharClip load_clip(const std::string& hdr_path,
                   const std::string& ark_path,
                   const std::string& milo_path,
                   const std::string& clip_name);

// Source-backed CharClipGroup::Load reader. Returns the group's serialized
// ObjPtr clip names and source mWhich/mFlags state from the first matching
// animation MILO.
CharClipGroup load_clip_group(
    const std::string& hdr_path, const std::string& ark_path,
    const std::vector<std::string>& milo_paths,
    const std::string& group_name);

// Source-backed CharClipGroup::GetClip index step. Mutates group.which.
std::optional<size_t> char_clip_group_get_clip_index(CharClipGroup& group);

// Source-backed CharClipGroup::NumFlagDuplicates helper. `clip_index` selects
// the source clip row whose flags are compared against every other row.
int source_char_clip_group_num_flag_duplicates(
    const std::vector<uint32_t>& clip_flags,
    size_t clip_index,
    uint32_t mask);
std::vector<std::string> source_char_clip_group_sorted_names(
    std::vector<std::string> clip_names);
std::vector<std::string> source_char_clip_group_add_clip(
    std::vector<std::string> clip_names,
    const std::string& clip_name);
std::vector<std::string> source_char_clip_group_remove_clip(
    std::vector<std::string> clip_names,
    const std::string& clip_name);
SourceCharClipGroupLoadPlan source_char_clip_group_load_plan(int revision);
SourceCharClipGroupHandlerPlan source_char_clip_group_handler_plan();
SourceCharClipGroupPropSyncPlan source_char_clip_group_prop_sync_plan();

struct SourceCharClipDriverState {
  uint32_t play_flags = 0;
  float blend_width = 0.0f;
  float time_scale = 1.0f;
  float d_beat = 0.0f;
  float advance_beat = 0.0f;
  bool has_clip = false;
  bool has_next = false;
  int next_event = -1;
  bool play_multiple_clips = false;
};

struct SourceCharClipDriverExitDecision {
  bool recurse_next = false;
  bool execute_exit_event = false;
  bool end_sync_anim = false;
  bool delete_self = false;
  std::optional<size_t> returned_stack_head;
  std::vector<size_t> deleted_indices;
};

struct SourceCharClipDriverDeleteClipResult {
  std::optional<size_t> deleted_index;
  std::vector<size_t> remaining_indices;
};

struct SourceCharClipDriverRuntimeDumpEvidence {
  std::string copy_ctor_range;
  std::string evaluate_range;
  std::string scale_add_range;
  std::string rotate_to_range;
  std::string align_to_frame_range;
  std::string play_events_range;
  std::vector<std::string> evaluate_locals;
  std::vector<std::string> scale_add_locals;
  std::vector<std::string> rotate_to_locals;
  std::vector<std::string> align_to_frame_locals;
  std::vector<std::string> play_events_locals;
  bool has_evaluate_statement_body = false;
  bool has_scale_add_statement_body = false;
  bool has_rotate_to_statement_body = false;
  bool safe_to_import_runtime = false;
};

// Source-backed CharClipDriver constructor play-flag masking.
uint32_t source_char_clip_driver_masked_play_flags(uint32_t clip_play_flags,
                                                   uint32_t mask);
uint32_t char_clip_driver_masked_play_flags(const CharClip& clip,
                                            uint32_t mask);
SourceCharClipDriverState source_char_clip_driver_construct(
    uint32_t clip_play_flags,
    bool has_clip,
    bool has_next,
    uint32_t mask,
    float blend_width,
    bool play_multiple_clips);
std::vector<size_t> source_char_clip_driver_delete_stack_order(
    size_t stack_size);
SourceCharClipDriverExitDecision source_char_clip_driver_exit_decision(
    size_t stack_size,
    bool exit_next,
    bool has_sync_anim);
SourceCharClipDriverDeleteClipResult source_char_clip_driver_delete_clip_result(
    const std::vector<bool>& clip_matches_source_order);
bool source_char_clip_driver_should_execute_event(bool symbol_null,
                                                  bool clip_has_type_def);
SourceCharClipDriverRuntimeDumpEvidence
source_char_clip_driver_runtime_dump_evidence();

// Source-backed CharClip::BeatAlignString helper.
const char* source_char_clip_beat_align_string(uint32_t mask);

struct SourceCharClipFlagUpdate {
  uint32_t value = 0;
  bool dirty = false;
  bool changed = false;
};

struct SourceCharClipDefaultState {
  float frames_per_sec = 30.0f;
  uint32_t flags = 0;
  uint32_t play_flags = 0;
  float range = 0.0f;
  bool dirty = true;
  bool do_not_compress = false;
  int unk42 = -1;
  size_t beat_track_count = 1;
  float first_beat_frame = 0.0f;
  float first_beat_value = 0.0f;
};

struct SourceCharClipBeatEvent {
  std::string event;
  float beat = 0.0f;
};

struct SourceCharClipResourceLookup {
  bool has_type_def = false;
  bool has_resource_array = false;
  std::string resource_name;
  bool found_resource = false;
  bool warn_no_resource = false;
};

struct SourceCharClipTransitionsState {
  bool has_owner = false;
  std::vector<int> node_sizes;
};

struct SourceCharClipTransitionsClearResult {
  size_t released_clips = 0;
  bool resized_zero = false;
};

struct SourceCharClipTransitionsDumpEvidence {
  std::string remove_nodes_range;
  std::string resize_nodes_range;
  std::string add_node_range;
  bool has_remove_nodes_locals = true;
  bool has_resize_nodes_locals = true;
  bool has_add_node_locals = true;
  bool has_statement_bodies = false;
};

struct SourceCharClipRuntimeDumpEvidence {
  std::string find_nodes_range;
  std::string find_first_node_range;
  std::string find_last_node_range;
  std::string find_node_range;
  std::string replace_range;
  std::string clear_all_nodes_range;
  std::string load_range;
  std::string check_stick_range;
  std::string sync_property_range;
  std::vector<std::string> find_nodes_locals;
  std::vector<std::string> find_first_node_locals;
  std::vector<std::string> find_last_node_locals;
  std::vector<std::string> find_node_locals;
  std::vector<std::string> load_locals;
  std::vector<std::string> check_stick_locals;
  bool has_load_statement_body = false;
  bool has_check_stick_statement_body = false;
  bool has_sync_property_statement_body = false;
  bool safe_to_import_load = false;
  bool safe_to_import_check_stick = false;
  bool safe_to_import_sync_property = false;
};

struct SourceCharClipPoseMeshesSteps {
  std::string temp_meshes_name;
  bool stuff_bones = false;
  bool scale_down = false;
  float scale_down_weight = 0.0f;
  bool scale_add = false;
  float scale_add_weight = 0.0f;
  float scale_add_frame = 0.0f;
  float scale_add_blend = 0.0f;
  bool pose_meshes = false;
};

struct SourceCharClipPropSyncPlan {
  std::vector<std::string> graph_node_properties;
  bool node_vector_size_query = true;
  std::vector<std::string> node_vector_properties;
  std::vector<std::string> beat_event_set_properties;
  std::vector<std::string> clip_set_properties;
  std::vector<std::string> clip_properties;
  std::vector<std::string> sample_subobjects;
};

enum SourceCharDriverApplyMode {
  kSourceCharDriverApplyBlend = 0,
  kSourceCharDriverApplyAdd = 1,
  kSourceCharDriverApplyRotateTo = 2,
  kSourceCharDriverApplyBlendWeights = 3,
};

struct SourceCharDriverState {
  bool has_bones = false;
  bool has_clips = false;
  bool has_first = false;
  bool has_test_clip = false;
  bool has_default_clip = false;
  bool default_play_starved = false;
  std::string starved_handler;
  bool last_node_valid = false;
  float old_beat = 1.0e30f;
  bool realign = false;
  float beat_scale = 1.0f;
  float blend_width = 1.0f;
  std::string clip_type;
  SourceCharDriverApplyMode apply = kSourceCharDriverApplyBlend;
  bool has_internal_bones = false;
  bool play_multiple_clips = false;
};

struct SourceCharDriverTransferPlan {
  bool clear_stack = true;
  bool create_first_driver_copy = false;
  std::vector<std::string> copied_members;
  std::vector<std::string> preserved_members;
};

struct SourceCharDriverSyncDecision {
  bool changed = false;
  bool clear_stack = false;
  bool reset_last_node = false;
  bool delete_internal_bones = false;
  bool allocate_internal_bones = false;
  bool clear_internal_bones = false;
  bool stuff_internal_bones = false;
  bool has_internal_bones = false;
};

struct SourceCharDriverEnterDecision {
  bool changed = false;
  bool clear_stack = false;
  bool reset_last_node = false;
  bool reset_old_beat = false;
  bool reset_beat_scale = false;
  bool play_default_clip = false;
  int default_play_flags = 1;
  float default_requested_blend_width = -1.0f;
  float default_old_beat = 1.0e30f;
  float default_start = 0.0f;
};

struct SourceCharDriverPlayDecision {
  bool found_clip = false;
  bool notify_missing_clip = false;
  bool set_last_node = false;
  bool duplicate_clip = false;
  bool create_clip_driver = false;
  bool new_stack_head = false;
  int play_flags = 0;
  float resolved_blend_width = 0.0f;
  float old_beat = 0.0f;
  float start = 0.0f;
  bool play_multiple_clips = false;
};

struct SourceCharDriverPlayNodeDecision {
  bool copied_requested_node = true;
  bool find_clip_warn = true;
  SourceCharDriverPlayDecision clip_play;
  bool final_last_node_from_request = false;
  bool returned_driver = false;
};

struct SourceCharDriverPlayGroupDecision {
  bool has_clip_dir = false;
  bool found_group = false;
  bool warn_no_clips = false;
  bool warn_missing_group = false;
  bool call_group_get_clip = false;
  bool request_play = false;
};

struct SourceCharDriverRuntimeDumpEvidence {
  std::string play_if_safe_range;
  std::string set_beat_scale_range;
  std::string evaluate_flags_range;
  std::string last_range;
  std::string before_range;
  std::string most_playing_range;
  std::string pre_load_range;
  std::string post_load_range;
  std::vector<std::string> play_if_safe_locals;
  std::vector<std::string> set_beat_scale_locals;
  std::vector<std::string> evaluate_flags_locals;
  std::vector<std::string> most_playing_locals;
  bool rb3_latest_has_poll_body = false;
  bool rb2_dump_has_poll_range = false;
  bool has_evaluate_flags_statement_body = false;
  bool has_set_beat_scale_statement_body = false;
  bool safe_to_evaluate_flags = false;
  bool safe_to_import_poll = false;
};

struct SourceCharDriverPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharDriverMidiState {
  bool unk89 = false;
  std::string parser;
  std::string flag_parser;
  int clip_flags = 0;
  float blend_override_pct = 1.0f;
  bool has_default_clip = false;
};

struct SourceCharDriverMidiEnterDecision {
  SourceCharDriverEnterDecision driver_enter;
  bool set_unk89 = false;
  bool add_parser_sink = false;
  bool add_flag_parser_sink = false;
};

struct SourceCharDriverMidiExitDecision {
  bool call_driver_exit = false;
  bool remove_parser_sink = false;
  bool remove_flag_parser_sink = false;
};

struct SourceCharDriverMidiPollPlan {
  bool call_driver_poll = false;
  bool call_driver_poll_deps = false;
};

struct SourceCharDriverMidiParserDecision {
  bool used_default_clip = false;
  bool call_group_get_clip = false;
  int group_clip_flags = 0;
  bool request_play = false;
  int play_flags = 0;
  float requested_blend_width = 0.0f;
  float old_beat = 0.0f;
  float start = 0.0f;
  float assigned_blend_width = 0.0f;
};

struct SourceCharDriverMidiLoadPlan {
  bool known_revision = false;
  int32_t max_revision = 7;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
};

struct SourceCharDriverMidiHandlerPlan {
  std::vector<std::string> handlers;
  std::vector<std::string> superclasses;
  int32_t check = 0x99;
};

struct SourceCharDriverMidiPropSyncPlan {
  std::vector<std::string> properties;
  std::vector<std::string> superclasses;
};

struct SourceCharDriverMidiCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
  std::vector<std::string> not_in_source_copy_members;
};

struct SourceCharClipSetState {
  std::string char_file_root;
  bool has_preview_char = false;
  bool has_preview_clip = false;
  bool has_still_clip = false;
  int filter_flags = 0;
  int bpm = 90;
  bool preview_walk = false;
  bool rate_is_1_fpb = true;
};

struct SourceCharClipSetGroupStep {
  std::string group;
  bool randomize = false;
  bool sort = false;
};

struct SourceCharClipSetResetEditorResult {
  bool reset_preview_state = false;
  bool object_dir_reset_editor_state = false;
};

struct SourceCharClipSetPreSaveResult {
  bool preview_char_name_cleared = false;
  bool reset_preview_state = false;
  bool reset_editor_state = false;
};

struct SourceCharClipSetPostSaveResult {
  bool object_dir_post_save = false;
  bool preview_char_name_restored = false;
  bool preview_char_entered = false;
  bool sent_update_objects = false;
};

struct SourceCharClipSetPreLoadPlan {
  int32_t max_revision = 0x18;
  bool require_revision_gt_3 = true;
  bool push_packed_revision = true;
  bool object_dir_pre_load = true;
};

struct SourceCharClipSetPostLoadPlan {
  bool object_dir_post_load = true;
  bool returned_for_proxy = false;
  bool read_two_legacy_ints = false;
  bool read_rev_15_16_int = false;
  bool read_legacy_graph_path = false;
  bool read_legacy_reexport_string = false;
  bool read_rev_lt7_int = false;
  int32_t read_legacy_clip_triplets = 0;
  bool read_old_flag_bool = false;
  bool read_old_flag_second_bool = false;
  bool read_symbol_count = false;
  bool read_legacy_string_lists = false;
  bool read_legacy_symbol_and_int = false;
  bool read_rev_11_bool = false;
  bool warn_transition_bug = false;
  bool handle_filter_clips = false;
  bool read_char_file_path = false;
  bool read_preview_clip = false;
  bool read_filter_flags = false;
  bool read_bpm = false;
  bool read_preview_walk = false;
  bool read_still_clip = false;
};

struct SourceCharClipSetLoadCharacterResult {
  bool asserted_edit_mode = false;
  bool deleted_preview_char = false;
  bool loaded_objects = false;
  bool loaded_rnd_dir = false;
  bool selected_nested_character = false;
  bool preview_char_entered = false;
  bool preview_char_named = false;
  bool sent_update_objects = false;
};

struct SourceCharClipSetSetBpmResult {
  bool set_milo_property = false;
  int bpm = 90;
};

struct SourceCharClipSetCopyResult {
  bool copy_object_dir = false;
  bool copy_char_file_path = false;
  bool copy_preview_clip = false;
  bool copy_filter_flags = false;
  bool copy_bpm = false;
  bool copy_preview_walk = false;
  bool copy_still_clip = false;
};

struct SourceCharClipDisplayGlobals {
  std::string dir;
  float em = 0.0f;
};

struct SourceCharClipDisplayState {
  std::string clip;
  std::string text;
  float text_width_plus_em = 0.0f;
  float start_beat = 0.0f;
  float end_beat = 0.0f;
  bool start_end_called = false;
  bool start_end_flag = false;
};

struct SourceCharClipDisplayMsgSource {
  std::string source;
  std::vector<std::string> sinks;
};

struct SourceCharClipDisplayFindSourceResult {
  bool found = false;
  std::string source;
};

struct SourceCharTaskMgrState {
  bool show_graph = false;
  bool registered_toggle_char_task_graph = false;
};

struct SourceClipGraphGeneratePairStep {
  bool remove_existing_nodes = true;
  bool captures_type_def = true;
  bool return_null_before_script = false;
  bool execute_on_transition = false;
  bool set_data_variables = false;
  bool stores_clip_pair = false;
  bool clears_dmap_before_script = false;
  bool clears_dmap_after_script = false;
  bool returns_dmap = false;
  bool set_nodes = false;
  std::string reason;
};

struct SourceClipGraphTransitionInputs {
  uint32_t clip_a_play_flags = 0;
  uint32_t clip_b_play_flags = 0;
  float max_error = 1.0e30f;
  float beat_align = 0.0f;
  float blend_width = 1.0f;
  float max_facing_degrees = 0.0f;
  float max_dist = 0.0f;
  float end_dist = 0.0f;
  bool has_restrict = false;
  bool has_bone_weights = false;
};

struct SourceClipGraphTransitionPlan {
  int clip_a_flag = 0;
  int clip_b_flag = 0;
  int min_flag = 0;
  float beat_align = 0.0f;
  float blend_width = 1.0f;
  int dist_map_sample_stride = 3;
  bool has_restrict = false;
  bool has_bone_weights = false;
  float find_dists_max_facing_radians = 0.0f;
  float find_nodes_max_error = 0.0f;
  float find_nodes_max_dist = 0.0f;
  float find_nodes_end_dist = 0.0f;
};

struct SourceClipCollideState {
  std::string char_path;
  std::string position = "front";
  bool clip_null = true;
  bool world_lines = false;
  bool move_camera = true;
  std::string mode;
};

struct SourceClipCollideLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
  bool clears_clip = false;
};

struct SourceClipCollideSyncCharStep {
  bool set_proxy_file = false;
  bool sync_waypoint = true;
};

struct SourceClipCollideSetTypeDefStep {
  bool call_object_set_type_def = false;
  bool update_mode = false;
  bool assert_modes_array = false;
};

struct SourceClipCollideValidationStep {
  bool send_message = false;
  bool valid = true;
  std::string message;
};

struct SourceClipCollideDemonstrateStep {
  bool sync_waypoint = false;
  bool play_clip = false;
  int play_mode = 2;
  float play_start = -1.0f;
  float play_end = 1.0e30f;
  float play_blend = 0.0f;
};

struct SourceClipCollideListPlan {
  size_t source_array_size = 0;
  bool writes_null_first = true;
  size_t first_item_index = 1;
  std::vector<std::string> items;
};

struct SourceClipCollideTestClipsPlan {
  std::vector<std::string> directions;
  size_t collide_calls = 0;
};

struct SourceFileMergerMergerState {
  bool proxy = false;
  bool pre_clear = false;
  int subdirs = 4;
  bool dir_null = true;
  bool loaded_objects_no_null = true;
  bool loaded_subdirs_no_null = true;
};

struct SourceFileMergerState {
  bool async_load = false;
  bool loading_load = false;
  int unk44 = 0;
  int unk50 = 0;
  bool callback_self = true;
  bool asserts_heap_when_heaps_exist = true;
};

struct SourceFileMergerCopyPlan {
  std::vector<std::string> copied_members;
};

struct SourceClipCompressorEvidence {
  bool has_runtime_class = false;
  std::string observed_function;
  std::string format_string;
};

// Source-backed CharClip constructor state.
SourceCharClipDefaultState source_char_clip_default_state();
SourceCharClipBeatEvent source_char_clip_beat_event_default();
SourceCharClipBeatEvent source_char_clip_beat_event_copy(
    const SourceCharClipBeatEvent& source);
void source_char_clip_beat_event_assign(SourceCharClipBeatEvent& dest,
                                        const SourceCharClipBeatEvent& source);
SourceCharClipBeatEvent source_char_clip_beat_event_loaded(
    const std::string& event,
    float beat);
SourceCharClipPropSyncPlan source_char_clip_prop_sync_plan();
SourceCharClipResourceLookup source_char_clip_get_resource(
    bool has_type_def,
    bool has_resource_array,
    const std::string& resource_name,
    bool resource_found);
int source_char_clip_get_context(bool has_type_def,
                                 bool has_resource_array,
                                 int resource_context);
SourceCharClipTransitionsState source_char_clip_transitions_construct(
    bool has_owner);
size_t source_char_clip_transitions_size(
    const SourceCharClipTransitionsState& transitions);
SourceCharClipTransitionsClearResult source_char_clip_transitions_clear(
    SourceCharClipTransitionsState& transitions);
SourceCharClipTransitionsDumpEvidence
source_char_clip_transitions_dump_evidence();
SourceCharClipRuntimeDumpEvidence source_char_clip_runtime_dump_evidence();
std::vector<SourceCharBonesBone> source_char_clip_stuff_bones(
    const std::vector<SourceCharBonesBone>& existing_bones,
    const std::vector<SourceCharBonesBone>& listed_bones);
SourceCharClipPoseMeshesSteps source_char_clip_pose_meshes_steps(float frame);

// Source-backed CharDriver constructor, Clear, Transfer, setter, and
// SyncInternalBones state helpers.
SourceCharDriverState source_char_driver_default_state();
void source_char_driver_clear(SourceCharDriverState& state);
SourceCharDriverEnterDecision source_char_driver_enter(
    SourceCharDriverState& state);
SourceCharDriverTransferPlan source_char_driver_transfer_plan(
    bool source_has_first);
void source_char_driver_transfer(SourceCharDriverState& state,
                                 const SourceCharDriverState& driver);
void source_char_driver_set_clips(SourceCharDriverState& state,
                                  bool has_clips);
void source_char_driver_set_bones(SourceCharDriverState& state,
                                  bool has_bones);
void source_char_driver_set_starved(SourceCharDriverState& state,
                                    const std::string& starved_handler);
void source_char_driver_set_blend_width(SourceCharDriverState& state,
                                        float blend_width);
SourceCharDriverSyncDecision source_char_driver_sync_internal_bones(
    SourceCharDriverState& state);
SourceCharDriverSyncDecision source_char_driver_set_apply(
    SourceCharDriverState& state,
    SourceCharDriverApplyMode apply);
SourceCharDriverSyncDecision source_char_driver_set_clip_type(
    SourceCharDriverState& state,
    const std::string& clip_type);
SourceCharDriverPlayGroupDecision source_char_driver_play_group_decision(
    bool has_clip_dir,
    bool found_group);
SourceCharDriverRuntimeDumpEvidence
source_char_driver_runtime_dump_evidence();
void source_char_driver_poll_deps(SourceCharDriverPollDeps& deps,
                                  const std::string& bones);
SourceCharDriverMidiState source_char_driver_midi_default_state();
SourceCharDriverMidiEnterDecision source_char_driver_midi_enter(
    SourceCharDriverState& driver_state,
    SourceCharDriverMidiState& midi_state,
    bool parser_found,
    bool flag_parser_found);
SourceCharDriverMidiExitDecision source_char_driver_midi_exit(
    bool parser_found,
    bool flag_parser_found);
SourceCharDriverMidiPollPlan source_char_driver_midi_poll_plan();
void source_char_driver_midi_poll_deps(SourceCharDriverPollDeps& deps,
                                       const std::string& bones);
void source_char_driver_midi_on_parser_flags(
    SourceCharDriverMidiState& midi_state,
    int clip_flags);
SourceCharDriverMidiParserDecision source_char_driver_midi_on_parser(
    const SourceCharDriverMidiState& midi_state,
    bool found_clip,
    bool clip_uses_real_time,
    float message_float,
    float beat_to_seconds_message_plus_current,
    float task_seconds,
    float average_beats_per_second);
SourceCharDriverMidiParserDecision source_char_driver_midi_on_parser_group(
    const SourceCharDriverMidiState& midi_state,
    bool found_group,
    bool found_group_clip,
    bool clip_uses_real_time,
    float message_float,
    float average_beats_per_second);
SourceCharDriverMidiLoadPlan source_char_driver_midi_load_plan(int revision);
SourceCharDriverMidiHandlerPlan source_char_driver_midi_handler_plan();
SourceCharDriverMidiPropSyncPlan source_char_driver_midi_prop_sync_plan();
SourceCharDriverMidiCopyPlan source_char_driver_midi_copy_plan();
SourceCharClipSetState source_char_clip_set_default_state();
void source_char_clip_set_reset_preview_state(
    SourceCharClipSetState& state);
SourceCharClipSetResetEditorResult source_char_clip_set_reset_editor_state(
    SourceCharClipSetState& state);
std::vector<SourceCharClipSetGroupStep> source_char_clip_set_randomize_groups(
    const std::vector<std::string>& groups);
std::vector<SourceCharClipSetGroupStep> source_char_clip_set_sort_groups(
    const std::vector<std::string>& groups);
SourceCharClipSetPreSaveResult source_char_clip_set_pre_save(
    SourceCharClipSetState& state,
    bool cached_stream);
SourceCharClipSetPostSaveResult source_char_clip_set_post_save(
    const SourceCharClipSetState& state,
    bool milo_found);
SourceCharClipSetPreLoadPlan source_char_clip_set_pre_load_plan();
SourceCharClipSetPostLoadPlan source_char_clip_set_post_load_plan(
    int32_t revision,
    bool is_proxy,
    int32_t clip_count,
    bool type_null);
SourceCharClipSetCopyResult source_char_clip_set_copy(
    SourceCharClipSetState& dest,
    const SourceCharClipSetState& source);
SourceCharClipSetLoadCharacterResult source_char_clip_set_load_character(
    SourceCharClipSetState& state,
    bool edit_mode,
    bool loaded_is_rnd_dir,
    bool loaded_is_character,
    bool nested_character_found,
    bool milo_found);
bool source_char_clip_set_draw_showing(bool has_preview_char);
float source_char_clip_set_start_frame(bool has_preview_clip,
                                       float preview_clip_start_beat);
float source_char_clip_set_end_frame(bool has_preview_clip,
                                     float preview_clip_end_beat);
SourceCharClipSetSetBpmResult source_char_clip_set_set_bpm(
    SourceCharClipSetState& state,
    int bpm,
    bool milo_found);
const char* source_char_clip_set_recenter_all_warning();
void source_char_clip_display_init(SourceCharClipDisplayGlobals& globals,
                                   const std::string& dir,
                                   float draw_empty_y);
SourceCharClipDisplayFindSourceResult source_char_clip_display_find_source(
    const std::vector<SourceCharClipDisplayMsgSource>& sources,
    const std::string& object);
void source_char_clip_display_set_text(
    SourceCharClipDisplayState& state,
    const SourceCharClipDisplayGlobals& globals,
    const std::string& text,
    float draw_text_x);
void source_char_clip_display_set_start_end(
    SourceCharClipDisplayState& state,
    float start_beat,
    float end_beat,
    bool flag);
void source_char_clip_display_set_clip(
    SourceCharClipDisplayState& state,
    const SourceCharClipDisplayGlobals& globals,
    const std::string& clip_name,
    float start_beat,
    float end_beat,
    bool flag,
    float draw_text_x);
float source_char_clip_display_line_spacing(
    const SourceCharClipDisplayGlobals& globals);
SourceCharTaskMgrState source_char_task_mgr_default_state();
void source_char_task_mgr_init(SourceCharTaskMgrState& state);
bool source_char_task_mgr_toggle_graph(SourceCharTaskMgrState& state);
SourceClipGraphGeneratePairStep source_clip_graph_generate_pair_step(
    bool has_type_def,
    bool same_type,
    uint32_t clip_a_play_flags,
    bool has_on_transition,
    bool script_creates_dmap);
SourceClipGraphTransitionPlan source_clip_graph_on_generate_transitions(
    const SourceClipGraphTransitionInputs& inputs);
SourceClipCollideState source_clip_collide_default_state();
bool source_clip_collide_load_revision_known(int revision);
SourceClipCollideLoadPlan source_clip_collide_load_plan(int revision);
SourceClipCollideSyncCharStep source_clip_collide_sync_char_step(
    bool has_character,
    bool char_path_empty,
    bool path_matches_proxy);
SourceClipCollideSetTypeDefStep source_clip_collide_set_type_def_step(
    bool type_def_changed,
    bool has_type_def);
SourceClipCollideValidationStep source_clip_collide_valid_waypoint(
    bool handler_unhandled,
    bool handler_value);
SourceClipCollideValidationStep source_clip_collide_valid_clip(
    bool has_waypoint,
    bool handler_unhandled,
    bool handler_value);
SourceClipCollideDemonstrateStep source_clip_collide_demonstrate_step(
    bool has_character,
    bool has_waypoint,
    bool has_clip);
SourceClipCollideListPlan source_clip_collide_list_objects_plan(
    const std::vector<std::string>& valid_objects);
SourceClipCollideListPlan source_clip_collide_list_report_plan(
    const std::vector<std::string>& reports);
SourceClipCollideTestClipsPlan source_clip_collide_test_clips_plan(
    size_t valid_clip_count);
SourceFileMergerState source_file_merger_default_state();
SourceFileMergerMergerState source_file_merger_merger_default_state();
SourceFileMergerCopyPlan source_file_merger_merger_copy_plan();
SourceClipCompressorEvidence source_clip_compressor_evidence();

// Source-backed CharClip::SetFlags / SetPlayFlags dirty-state helpers.
SourceCharClipFlagUpdate source_char_clip_set_flags(uint32_t current_flags,
                                                    bool current_dirty,
                                                    uint32_t requested_flags);
SourceCharClipFlagUpdate source_char_clip_set_play_flags(
    uint32_t current_play_flags,
    bool current_dirty,
    uint32_t requested_play_flags);
bool source_char_clip_shares_groups(
    const std::vector<SourceCharClipRefOwner>& ref_owners,
    const std::string& candidate_clip_name);

// Source-backed CharDriver::Starved helper for the visible play stack state.
bool source_char_driver_starved(bool has_first, bool first_has_next,
                                uint32_t first_play_flags);

// Source-backed CharDriver::Play blend-width fallback.
float source_char_driver_resolve_blend_width(float requested_blend_width,
                                             float driver_blend_width);

// Source-backed CharDriver::Play duplicate-clip gate.
bool source_char_driver_should_start_clip(bool play_multiple_clips,
                                          bool clip_already_playing);

// Source-backed CharDriver::Play(CharClip*) state decision. This records the
// branch order without allocating a CharClipDriver node.
SourceCharDriverPlayDecision source_char_driver_play_decision(
    SourceCharDriverState& state,
    bool found_clip,
    bool clip_already_playing,
    int play_flags,
    float requested_blend_width,
    float old_beat,
    float start);

// Source-backed CharDriver::Play(DataNode) decision. The checked source copies
// the requested node, resolves a clip, calls Play(CharClip*), then restores
// mLastNode to the requested node even when no clip/driver was created.
SourceCharDriverPlayNodeDecision source_char_driver_play_node_decision(
    SourceCharDriverState& state,
    bool find_clip_succeeds,
    bool clip_already_playing,
    int play_flags,
    float requested_blend_width,
    float old_beat,
    float start);

// Source-backed CharDriver::FirstPlaying helper. The input is in source stack
// order: mFirst, then each mNext.
std::optional<size_t> source_char_driver_first_playing_index(
    const std::vector<float>& source_stack_blend_fracs);

// Source-backed CharBones channel helpers.
int source_char_bones_type_of(const std::string& channel);
const char* source_char_bones_suffix_of(int type);
std::string source_char_bones_channel_name(const std::string& name, int type);
size_t source_char_bones_type_size(int type, int compression);
SourceCharBonesLayout source_char_bones_recompute_layout(
    const std::array<int, kSourceCharBonesTypeEnd + 1>& counts,
    int compression);
SourceCharBonesCompressionUpdate source_char_bones_set_compression(
    int current_compression,
    const SourceCharBonesLayout& current_layout,
    int requested_compression);
SourceCharBonesState source_char_bones_empty_state();
void source_char_bones_clear(SourceCharBonesState& state);
void source_char_bones_set_weights(std::vector<SourceCharBonesBone>& bones,
                                   float weight);
void source_char_bones_set_weights(SourceCharBonesState& state, float weight);
void source_char_bones_list_bones(const SourceCharBonesState& state,
                                  std::vector<SourceCharBonesBone>& bones);
int source_char_bones_find_offset(const SourceCharBonesState& state,
                                  const std::string& channel);
SourceCharBonesFindPtrResult source_char_bones_find_ptr(
    const SourceCharBonesState& state,
    const std::string& channel);
void source_char_bones_zero(std::vector<uint8_t>& start, int total_size);
SourceCharBonesScaleAddClipStep source_char_bones_scale_add_clip_step(
    float f1, float f2, float f3);
SourceCharBonesPoseBodyBoundary source_char_bones_pose_body_boundary();
SourceCharBonesRuntimeDumpEvidence source_char_bones_runtime_dump_evidence();
SourceCharBonesAddBonesSteps source_char_bones_add_bones_steps(
    const std::vector<SourceCharBonesBone>& bones);
SourceCharBonesAllocReallocateStep source_char_bones_alloc_reallocate_step(
    int total_size);
SourceCharBonesEnterStep source_char_bones_enter_step();
SourceCharBonesBlenderPollStep source_char_bones_blender_poll_step(
    bool bones_empty,
    bool has_dest);
SourceCharBonesBlenderSetDestStep source_char_bones_blender_set_dest_step(
    bool dest_changed,
    bool new_dest_exists);
SourceCharBonesBlenderSetClipTypeStep
source_char_bones_blender_set_clip_type_step(bool clip_type_changed);
SourceCharBonesBlenderReallocateStep
source_char_bones_blender_reallocate_step(bool has_dest);
SourceCharBonesBlenderLoadPlan source_char_bones_blender_load_plan(
    int32_t revision);
SourceCharBonesBlenderCopyPlan source_char_bones_blender_copy_plan();
SourceCharBonesBlenderHandlerPlan source_char_bones_blender_handler_plan();
SourceCharBonesBlenderPropSyncPlan source_char_bones_blender_prop_sync_plan();

// Source-backed CharBone helpers for decoded CharClip output rows.
SourceCharBoneLoadPlan source_char_bone_load_plan(int32_t revision);
SourceCharBoneCopyPlan source_char_bone_copy_plan();
SourceCharBoneHandlerPlan source_char_bone_handler_plan();
SourceCharBoneWeightContextPropSyncPlan
source_char_bone_weight_context_prop_sync_plan();
SourceCharBonePropSyncPlan source_char_bone_prop_sync_plan();
SourceCharBonesBonePropSyncPlan source_char_bones_bone_prop_sync_plan();
SourceCharBonesObjectPropSyncPlan source_char_bones_object_prop_sync_plan();
CharClip::OutputBone source_char_bone_copy_members(
    const CharClip::OutputBone& source);
std::optional<size_t> source_char_bone_find_weight_index(
    const CharClip::OutputBone& bone, int context_mask);
float source_char_bone_get_weight(const CharClip::OutputBone& bone,
                                  int context_mask);
void source_char_bone_clear_context(CharClip::OutputBone& bone,
                                    int context_mask);
void source_char_bone_stuff_bones(const CharClip::OutputBone& bone,
                                  int context_mask,
                                  std::vector<SourceCharBonesBone>& bones);
SourceCharBoneDirDefaultState source_char_bone_dir_default_state();
SourceCharBoneDirLoadPlan source_char_bone_dir_load_plan(int32_t revision);
SourceCharBoneDirCopyPlan source_char_bone_dir_copy_plan();
SourceCharBoneDirHandlerPlan source_char_bone_dir_handler_plan();
SourceCharBoneDirRecenterPropSyncPlan
source_char_bone_dir_recenter_prop_sync_plan();
SourceCharBoneDirPropSyncPlan source_char_bone_dir_prop_sync_plan();
SourceCharBoneDirInitPlan source_char_bone_dir_init_plan(
    const std::string& resource_path,
    bool has_clip_types,
    const std::vector<SourceCharBoneDirInitClipTypeRow>& clip_types);
SourceCharBoneDirTerminatePlan source_char_bone_dir_terminate_plan();
SourceCharBoneDirFindResourceResult source_char_bone_dir_find_resource(
    const std::vector<std::string>& loaded_resources,
    const std::string& resource_name);
void source_char_bone_dir_list_bones(
    const std::vector<CharClip::OutputBone>& output_bones,
    int move_context,
    int context_mask,
    bool include_delta_facing,
    std::vector<SourceCharBonesBone>& bones);
std::vector<std::string> source_char_bone_dir_get_clip_types(
    const std::vector<SourceCharBoneDirClipTypeResource>& clip_types);
SourceCharBoneDirResourceLookupResult
source_char_bone_dir_find_resource_from_clip_type(
    const std::vector<SourceCharBoneDirClipTypeResource>& clip_types,
    const std::string& clip_type);
SourceCharBoneDirStuffBonesSymbolStep
source_char_bone_dir_stuff_bones_symbol_step(
    const std::vector<SourceCharBoneDirClipTypeResource>& clip_types,
    const std::string& clip_type);
SourceCharBoneDirContextFlagsStep
source_char_bone_dir_get_context_flags_step(
    const std::vector<SourceCharBoneDirClipTypeResource>& clip_types,
    const std::string& resource_name,
    const std::vector<std::string>& cached_context_flags,
    bool context_flags_is_int);
std::vector<std::string> source_char_bone_dir_sync_filter(
    const std::vector<CharClip::OutputBone>& output_bones,
    int filter_context);
SourceCharBoneDirMergeCharacterPlan source_char_bone_dir_merge_character_plan(
    bool load_succeeds,
    const std::vector<SourceCharBoneDirMergeTransform>& transforms);
SourceCharBonesMeshesReplaceStep source_char_bones_meshes_replace_step(
    const std::vector<std::string>& meshes,
    const std::string& from,
    const std::string& to,
    bool to_is_transformable,
    const std::string& dummy_mesh);
SourceCharBonesMeshesReallocateStep source_char_bones_meshes_reallocate_step(
    const std::vector<SourceCharBonesBone>& bones,
    const std::unordered_map<std::string, std::string>& transform_lookup,
    const std::string& dummy_mesh);
std::vector<std::string> source_char_bones_meshes_stuff_meshes(
    const std::vector<std::string>& existing_objects,
    const std::vector<std::string>& meshes);
SourceCharBonesMeshesPoseDumpEvidence
source_char_bones_meshes_pose_dump_evidence();

// Source-backed CharServoBone movement helpers. These port the isolated math
// bodies only; broad CharBonesMeshes movement stays fenced to the clip stack.
SourceCharServoBoneDefaultState source_char_servo_bone_default_state();
SourceCharServoBoneSetClipTypeStep source_char_servo_bone_set_clip_type_step(
    bool clip_type_changed);
SourceCharServoBoneEnterStep source_char_servo_bone_enter(
    bool facing_pos_delta_present);
SourceCharServoBoneSetMoveSelfStep source_char_servo_bone_set_move_self(
    bool current_move_self,
    bool requested_move_self);
SourceCharServoBoneCopyPlan source_char_servo_bone_copy_plan();
SourceCharServoBoneLoadPlan source_char_servo_bone_load_plan(int32_t revision);
SourceCharServoBoneHandlerPlan source_char_servo_bone_handler_plan();
SourceCharServoBonePropSyncPlan source_char_servo_bone_prop_sync_plan();
SourceCharServoBoneRuntimeDumpEvidence
source_char_servo_bone_runtime_dump_evidence();
void source_char_servo_bone_zero_deltas(
    std::array<float, 3>& facing_pos_delta,
    float& facing_rot_delta_radians);
void source_char_servo_bone_move_to_facing(
    milo_scene::Xfm& xfm,
    const std::array<float, 3>& facing_pos,
    float facing_rot_radians);
void source_char_servo_bone_move_to_delta_facing(
    milo_scene::Xfm& xfm,
    const std::array<float, 3>& facing_pos_delta,
    float facing_rot_delta_radians);

// Source-backed CharBonesSamples state helpers.
SourceCharBonesSamplesState source_char_bones_samples_empty_state();
void source_char_bones_samples_set(SourceCharBonesSamplesState& samples,
                                   const SourceCharBonesState& bones,
                                   int num_samples,
                                   int compression);
SourceCharBonesSamplesState source_char_bones_samples_clone(
    const SourceCharBonesSamplesState& source);
int source_char_bones_samples_allocate_size(
    const SourceCharBonesSamplesState& samples);
bool source_char_bones_samples_set_preview(
    SourceCharBonesSamplesState& samples, int requested_sample);
std::vector<SourceCharBonesSampleStep> source_char_bones_samples_split_steps(
    const SourceCharBonesSamplesState& samples,
    int sample,
    float weight,
    float frac);
int source_char_bones_samples_rotate_by_offset(
    const SourceCharBonesSamplesState& samples,
    int sample);
std::vector<SourceCharBonesSampleStep> source_char_bones_samples_rotate_to_steps(
    const SourceCharBonesSamplesState& samples,
    int sample,
    float angle,
    float frac);
std::vector<SourceCharBonesSampleStep>
source_char_bones_samples_scale_add_steps(
    const SourceCharBonesSamplesState& samples,
    int sample,
    float weight,
    float frac);
bool source_char_bones_samples_set_ver_known(int version);
bool source_char_bones_samples_load_version_known(int version);
SourceCharBonesSamplesLoadPlan source_char_bones_samples_load_plan(int version);
bool source_grim_char_bones_samples_standalone_version_known(int version);
bool source_grim_char_clip_samples_version_known(int version);
bool source_grim_char_clip_version_known(int version);
int source_grim_char_bones_samples_get_type_of(const std::string& channel);
float source_grim_char_bones_samples_decode_snorm16(int16_t value);
size_t source_grim_char_bones_samples_get_type_size(int type,
                                                    int compression);
size_t source_grim_char_bones_samples_get_type_size2(int type,
                                                     int compression);
SourceGrimCharBonesSamplesHeaderPlan
source_grim_char_bones_samples_header_plan(int version);
SourceGrimCharClipLoadPlan source_grim_char_clip_load_plan(int version,
                                                           bool read_meta);
SourceGrimCharClipSamplesLoadPlan
source_grim_char_clip_samples_load_plan(int version);
SourceReNotesCharBonesSamplesDecodePlan
source_re_notes_char_bones_samples_decode_plan();
SourceCharBonesSamplesPropSyncPlan source_char_bones_samples_prop_sync_plan();
SourceCharBonesSamplesBodyBoundary
source_char_bones_samples_body_boundary();
SourceCharBonesSamplesRuntimeDumpEvidence
source_char_bones_samples_runtime_dump_evidence();
SourceCharClipSamplesRuntimeDumpEvidence
source_char_clip_samples_runtime_dump_evidence();

// Source-backed CharUtl name/object helpers. CharUtlFindBone rewrites the
// incoming name to .cb. CharUtlFindBoneTrans checks .cb first and returns that
// CharBone row's transform before falling back to .trans, then .mesh.
std::string source_char_utl_name_with_suffix(const std::string& name,
                                             const std::string& suffix);
std::optional<SourceCharUtlObject> source_char_utl_find_bone(
    const std::string& name,
    const std::vector<SourceCharUtlObject>& objects);
std::optional<SourceCharUtlBoneTransResult> source_char_utl_find_bone_trans(
    const std::string& name,
    const std::vector<SourceCharUtlObject>& objects);
bool source_char_utl_is_animatable(const SourceCharUtlObject& object);
SourceCharUtlMergeResult source_char_utl_merge_bones(
    const std::vector<SourceCharUtlMergeBone>& source_bones,
    const std::vector<SourceCharUtlMergeBone>& dest_bones,
    int context_mask);
std::vector<std::string> source_char_utl_bone_saver_capture_names(
    const std::vector<SourceCharUtlTransformRow>& transforms);
std::vector<std::string> source_char_utl_reset_transform_names(
    const std::vector<SourceCharUtlTransformRow>& transforms);
std::vector<std::string> source_char_utl_reset_hair_names(
    const std::vector<std::string>& hair_names);
void source_char_utl_clip_predict(SourceCharUtlClipPredictState& state,
                                  const SourceCharUtlClipPredictFrame& first,
                                  const SourceCharUtlClipPredictFrame& second);
SourceCharUtlInitPlan source_char_utl_init_plan();

// Source-backed CharLookAt::SyncLimits helper. Angles are serialized in degrees.
SourceCharLookAtBounds source_char_lookat_sync_limits(
    float min_yaw, float max_yaw, float min_pitch, float max_pitch);
SourceCharLookAtLimitState source_char_lookat_default_limit_state();
void source_char_lookat_set_min_yaw(SourceCharLookAtLimitState& state,
                                    float yaw);
void source_char_lookat_set_max_yaw(SourceCharLookAtLimitState& state,
                                    float yaw);
void source_char_lookat_set_min_pitch(SourceCharLookAtLimitState& state,
                                      float pitch);
void source_char_lookat_set_max_pitch(SourceCharLookAtLimitState& state,
                                      float pitch);
SourceCharLookAtLoadPlan source_char_lookat_load_plan(int32_t revision);
SourceCharLookAtCopyPlan source_char_lookat_copy_plan();
SourceCharLookAtEnterState source_char_lookat_enter(bool has_pivot);
void source_char_lookat_poll_deps(SourceCharLookAtPollDeps& deps,
                                  const std::string& source,
                                  const std::string& pivot,
                                  const std::string& dest);
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
    bool allow_roll);
SourceCharLookAtYawWeightResult source_char_lookat_yaw_weight_step(
    float row_weight,
    float previous_yaw_weight,
    float min_weight_yaw,
    float max_weight_yaw,
    float weight_yaw_speed,
    float delta_seconds,
    std::array<float, 3> source_world_y,
    std::array<float, 3> dest_delta);
SourceCharLookAtNoRollAxesResult source_char_lookat_no_roll_axes(
    std::array<float, 3> current_local_y,
    std::array<float, 3> desired_parent_space_dir,
    float weight);
SourceCharLookAtSmoothResult source_char_lookat_smooth_dir(
    bool has_previous,
    std::array<float, 3> previous_dir,
    std::array<float, 3> current_dir,
    float delta_seconds,
    float half_time);
SourceCharLookAtRangeResult source_char_lookat_range_dir(
    const SourceCharLookAtBounds& bounds,
    bool test_range,
    float test_range_pitch,
    float test_range_yaw,
    bool show_range,
    int seconds);
SourceCharLookAtSourceRadiusResult source_char_lookat_source_radius_offset(
    float source_radius_degrees,
    float delta_seconds,
    std::array<float, 3> previous_history,
    std::array<float, 3> source_world_y);

// Source-backed CharWeightable::Weight helper. The owner row is used when it
// resolves; otherwise this falls back to the row's own serialized weight.
struct SourceCharWeightableState {
  std::string name;
  float weight = 1.0f;
  std::string weight_owner;
};

struct SourceCharWeightableLoadPlan {
  bool revision_supported = false;
  std::vector<std::string> read_order;
};

struct SourceCharWeightableCopyPlan {
  std::vector<std::string> shallow_actions;
  std::vector<std::string> deep_actions;
};

struct SourceCharWeightableHandlerPlan {
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharWeightablePropSyncPlan {
  std::vector<std::string> properties;
  std::vector<std::string> set_actions;
  std::vector<std::string> get_actions;
  std::vector<std::string> blocked_ops;
};

SourceCharWeightableLoadPlan source_char_weightable_load_plan(
    int32_t revision);
SourceCharWeightableCopyPlan source_char_weightable_copy_plan();
SourceCharWeightableHandlerPlan source_char_weightable_handler_plan();
SourceCharWeightablePropSyncPlan source_char_weightable_prop_sync_plan();
SourceCharWeightableState source_char_weightable_default_state(
    const std::string& name);
void source_char_weightable_set_weight(SourceCharWeightableState& state,
                                       float weight);
void source_char_weightable_set_weight_owner(SourceCharWeightableState& state,
                                             const std::string& weight_owner);
void source_char_weightable_replace(SourceCharWeightableState& state,
                                    const std::string& old_owner,
                                    const std::string& new_owner,
                                    bool new_owner_is_weightable);
void source_char_weightable_copy(SourceCharWeightableState& dest,
                                 const SourceCharWeightableState& source,
                                 bool shallow_copy,
                                 float source_owner_weight);
float source_char_weightable_weight(
    const CharWeightSetter& setter,
    const std::unordered_map<std::string, float>& weights_by_name);

struct SourceCharMirrorState {
  SourceCharWeightableState weightable;
  std::string servo;
  std::string mirror_servo;
  size_t bones_total_size = 0;
  size_t ops_count = 0;
};

struct SourceCharMirrorPollResult {
  float weight = 0.0f;
  bool weight_zero = false;
  bool bones_empty = false;
  bool scale_down = false;
  float scale_down_weight = 0.0f;
  std::string servo;
};

struct SourceCharMirrorSetServoResult {
  bool changed = false;
  bool synced_bones = false;
};

struct SourceCharMirrorPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharMirrorLoadSteps {
  int32_t max_revision = 1;
  bool load_hmx_object = false;
  bool load_weightable = false;
  bool load_mirror_servo = false;
  bool load_servo = false;
  bool sync_bones = false;
};

struct SourceCharMirrorCopyResult {
  bool copy_hmx_object = false;
  bool copy_weightable = false;
  SourceCharMirrorSetServoResult set_mirror_servo;
  SourceCharMirrorSetServoResult set_servo;
};

SourceCharMirrorState source_char_mirror_default_state(
    const std::string& name);
SourceCharMirrorPollResult source_char_mirror_poll(
    const SourceCharMirrorState& state,
    const std::unordered_map<std::string, float>& weights_by_name);
SourceCharMirrorSetServoResult source_char_mirror_set_servo(
    SourceCharMirrorState& state,
    const std::string& servo);
SourceCharMirrorSetServoResult source_char_mirror_set_mirror_servo(
    SourceCharMirrorState& state,
    const std::string& mirror_servo);
void source_char_mirror_poll_deps(SourceCharMirrorPollDeps& deps,
                                  const SourceCharMirrorState& state);
SourceCharMirrorLoadSteps source_char_mirror_load_steps();
SourceCharMirrorCopyResult source_char_mirror_copy(
    SourceCharMirrorState& dest,
    const SourceCharMirrorState& source,
    bool shallow_copy,
    float source_owner_weight);

// Source-backed CharWeightSetter::Poll helper for rows that do not require the
// unavailable CharDriver::EvaluateFlags body. Returns false when the row is
// driver-backed and no source evaluator is present.
bool source_char_weight_setter_poll(
    const CharWeightSetter& setter,
    const std::unordered_map<std::string, float>& weights_by_name,
    float delta_beats,
    float& out_weight);

struct SourceCharWeightSetterRefOwner {
  std::string name;
  bool weight_owner_is_setter = false;
};

struct SourceCharWeightSetterPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharWeightSetterState {
  SourceCharWeightableState weightable;
  bool has_base = false;
  bool has_driver = false;
  size_t min_weight_count = 0;
  size_t max_weight_count = 0;
  uint32_t flags = 0;
  float offset = 0.0f;
  float scale = 1.0f;
  float base_weight = 0.0f;
  float beats_per_weight = 0.0f;
};

struct SourceCharWeightSetterLoadPlan {
  bool revision_supported = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
};

struct SourceCharWeightSetterCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

struct SourceCharWeightSetterHandlerPlan {
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharWeightSetterPropSyncPlan {
  std::vector<std::string> properties;
  std::vector<std::string> superclasses;
};

struct SourceCharWeightSetterRuntimeDumpEvidence {
  std::string poll_range;
  std::string poll_deps_range;
  std::string load_range;
  std::string copy_range;
  std::vector<std::string> poll_locals;
  std::vector<std::string> poll_deps_locals;
  std::vector<std::string> load_locals;
  bool rb2_dump_has_statement_body = false;
  bool safe_to_run_driver_branch = false;
  bool safe_to_publish_driver_weight = false;
};

struct SourceCharIKHeadPoint {
  std::string transform;
  float local_length = 0.0f;
  float normalized_remaining = 0.0f;
};

struct SourceCharIKHeadState {
  SourceCharWeightableState weightable;
  std::string head;
  std::string spine;
  std::string mouth;
  std::string target;
  std::array<float, 3> head_filter = {0.0f, 0.0f, 0.0f};
  float target_radius = 0.75f;
  float head_mat = 0.5f;
  std::string offset;
  std::array<float, 3> offset_scale = {1.0f, 1.0f, 1.0f};
  float spine_length = 0.0f;
  bool update_points = true;
  std::string character_dir;
  std::vector<SourceCharIKHeadPoint> points;
};

struct SourceCharIKHeadSetNameResult {
  bool call_hmx_set_name = false;
  bool assigned_character = false;
};

struct SourceCharIKHeadPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharIKHeadUpdatePointsResult {
  bool entered_body = false;
  bool rebuilt_points = false;
  size_t point_count = 0;
  float spine_length = 0.0f;
};

struct SourceCharIKHeadLoadSteps {
  int32_t max_revision = 3;
  bool load_hmx_object = false;
  bool load_weightable = false;
  bool load_head = false;
  bool load_spine = false;
  bool load_mouth = false;
  bool load_target = false;
  bool load_target_radius = false;
  bool load_head_mat = false;
  bool load_offset = false;
  bool load_offset_scale = false;
  bool set_update_points = false;
};

struct SourceCharIKHeadCopyResult {
  bool copy_hmx_object = false;
  bool copy_weightable = false;
  bool copy_head = false;
  bool copy_spine = false;
  bool copy_mouth = false;
  bool copy_target = false;
  bool copy_target_radius = false;
  bool copy_head_mat = false;
  bool copy_offset = false;
  bool copy_offset_scale = false;
  bool set_update_points = false;
};

struct SourceCharIKSliderMidiState {
  SourceCharWeightableState weightable;
  std::string target;
  std::string first_spot;
  std::string second_spot;
  std::array<float, 3> dest_pos = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> old_pos = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> cur_pos = {0.0f, 0.0f, 0.0f};
  float target_percentage = 1.0f;
  float old_percentage = 0.0f;
  float frac = 0.0f;
  float frac_per_beat = 0.0f;
  bool percentage_changed = false;
  bool reset_all = true;
  std::string character_dir;
  float tolerance = 0.0f;
};

struct SourceCharIKSliderMidiEnterResult {
  bool cleared_percentage_changed = false;
  bool reset_frac = false;
  bool reset_frac_per_beat = false;
  bool call_rnd_pollable_enter = false;
};

struct SourceCharIKSliderMidiSetNameResult {
  bool call_hmx_set_name = false;
  bool assigned_character = false;
};

struct SourceCharIKSliderMidiSetupResult {
  bool reset_all = false;
};

struct SourceCharIKSliderMidiPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharIKSliderMidiLoadSteps {
  int32_t max_revision = 2;
  bool known_revision = false;
  bool load_hmx_object = false;
  bool load_weightable = false;
  bool load_target = false;
  bool load_first_spot = false;
  bool load_second_spot = false;
  bool load_tolerance = false;
};

struct SourceCharIKSliderMidiCopyResult {
  bool copy_hmx_object = false;
  bool copy_weightable = false;
  bool copy_target = false;
  bool copy_first_spot = false;
  bool copy_second_spot = false;
  bool copy_tolerance = false;
};

struct SourceCharIKMidiState {
  std::string bone;
  std::string cur_spot;
  std::string new_spot;
  bool spot_changed = false;
  bool local_xfm_reset = true;
  bool old_local_xfm_reset = true;
  float frac = 0.0f;
  float frac_per_beat = 0.0f;
  std::string anim_blender;
  float max_anim_blend = 1.0f;
  float anim_frac_per_beat = 0.0f;
  float anim_frac = 0.0f;
};

struct SourceCharIKMidiEnterResult {
  bool clear_cur_spot = true;
  bool clear_new_spot = true;
  bool clear_spot_changed = true;
  bool reset_frac = true;
  bool reset_frac_per_beat = true;
  bool reset_local_xfm = true;
  bool reset_old_local_xfm = true;
  bool call_rnd_pollable_enter = true;
};

struct SourceCharIKMidiPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

struct SourceCharIKMidiLoadSteps {
  bool known_revision = false;
  bool load_hmx_object = false;
  bool load_bone = false;
  bool load_legacy_spots = false;
  bool load_legacy_string = false;
  bool load_anim_blend = false;
};

struct SourceCharIKMidiCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

struct SourceCharLipSyncGeneratorState {
  bool lip_sync_null = true;
  int last_count = 0;
  std::vector<int> weights;
};

struct SourceCharLipSyncState {
  std::string prop_anim;
  std::vector<std::string> visemes;
  int frames = 0;
  std::vector<uint8_t> data;
};

struct SourceCharLipSyncLoadSteps {
  int32_t max_revision = 1;
  bool known_revision = false;
  bool load_hmx_object = false;
  bool load_visemes = false;
  bool load_frames = false;
  bool load_data = false;
  bool load_prop_anim = false;
};

struct SourceCharLipSyncDriverState {
  SourceCharWeightableState weightable;
  std::string lip_sync;
  std::string clips;
  std::string blink_clip;
  std::string song_owner;
  float song_offset = 0.0f;
  bool loop = false;
  bool song_player_null = true;
  std::string bones;
  std::string test_clip;
  float test_weight = 1.0f;
  std::string override_clip;
  float override_weight = 0.0f;
  std::string override_options;
  bool apply_override_additively = false;
  std::string alternate_driver;
};

struct SourceCharLipSyncDriverPollDeps {
  std::vector<std::string> changed_by;
  std::vector<std::string> change;
};

SourceCharWeightSetterState source_char_weight_setter_default_state(
    const std::string& name);
void source_char_weight_setter_set_weight(SourceCharWeightSetterState& state,
                                          float weight);
SourceCharWeightSetterLoadPlan source_char_weight_setter_load_plan(
    int32_t revision);
SourceCharWeightSetterCopyPlan source_char_weight_setter_copy_plan();
SourceCharWeightSetterHandlerPlan source_char_weight_setter_handler_plan();
SourceCharWeightSetterPropSyncPlan source_char_weight_setter_prop_sync_plan();
SourceCharWeightSetterRuntimeDumpEvidence
source_char_weight_setter_runtime_dump_evidence();

// Source-backed CharWeightSetter::PollDeps helper. Ref owners are supplied in
// source Refs() order; the helper scans them in reverse like the source body.
void source_char_weight_setter_poll_deps(
    SourceCharWeightSetterPollDeps& deps,
    const CharWeightSetter& setter,
    const std::vector<SourceCharWeightSetterRefOwner>& ref_owners);

SourceCharIKHeadState source_char_ik_head_default_state(
    const std::string& name);
SourceCharIKHeadSetNameResult source_char_ik_head_set_name(
    SourceCharIKHeadState& state,
    const std::string& dir_name,
    bool dir_is_character);
void source_char_ik_head_poll_deps(
    SourceCharIKHeadPollDeps& deps,
    const SourceCharIKHeadState& state,
    const std::vector<std::string>& head_to_spine_parent_chain,
    bool generation_count_nonzero);
SourceCharIKHeadUpdatePointsResult source_char_ik_head_update_points(
    SourceCharIKHeadState& state,
    bool force,
    const std::vector<std::string>& head_to_spine_chain,
    const std::vector<float>& local_lengths);
SourceCharIKHeadLoadSteps source_char_ik_head_load_steps(int32_t revision);
SourceCharIKHeadCopyResult source_char_ik_head_copy(
    SourceCharIKHeadState& dest,
    const SourceCharIKHeadState& source,
    bool shallow_copy,
    float source_owner_weight);
SourceCharIKSliderMidiState source_char_ik_slider_midi_default_state(
    const std::string& name);
SourceCharIKSliderMidiEnterResult source_char_ik_slider_midi_enter(
    SourceCharIKSliderMidiState& state);
SourceCharIKSliderMidiSetNameResult source_char_ik_slider_midi_set_name(
    SourceCharIKSliderMidiState& state,
    const std::string& dir_name,
    bool dir_is_character);
SourceCharIKSliderMidiSetupResult source_char_ik_slider_midi_setup_transforms(
    SourceCharIKSliderMidiState& state);
void source_char_ik_slider_midi_poll_deps(
    SourceCharIKSliderMidiPollDeps& deps,
    const SourceCharIKSliderMidiState& state);
SourceCharIKSliderMidiLoadSteps source_char_ik_slider_midi_load_steps(
    int32_t revision);
SourceCharIKSliderMidiCopyResult source_char_ik_slider_midi_copy(
    SourceCharIKSliderMidiState& dest,
    const SourceCharIKSliderMidiState& source,
    bool shallow_copy,
    float source_owner_weight);
SourceCharIKMidiState source_char_ik_midi_default_state();
SourceCharIKMidiEnterResult source_char_ik_midi_enter(
    SourceCharIKMidiState& state);
void source_char_ik_midi_poll_deps(SourceCharIKMidiPollDeps& deps,
                                   const SourceCharIKMidiState& state);
SourceCharIKMidiLoadSteps source_char_ik_midi_load_steps(int32_t revision);
SourceCharIKMidiCopyPlan source_char_ik_midi_copy_plan();
SourceCharLipSyncGeneratorState source_char_lip_sync_generator_default_state();
SourceCharLipSyncState source_char_lip_sync_default_state();
SourceCharLipSyncLoadSteps source_char_lip_sync_load_steps(int32_t revision);
SourceCharLipSyncDriverState source_char_lip_sync_driver_default_state(
    const std::string& name);
void source_char_lip_sync_driver_poll_deps(
    SourceCharLipSyncDriverPollDeps& deps,
    const SourceCharLipSyncDriverState& state);

struct SourceCharIKRodDefaultState {
  bool left_end_empty = true;
  bool right_end_empty = true;
  float dest_pos = 0.5f;
  bool side_axis_empty = true;
  bool vertical = false;
  bool dest_empty = true;
  bool xfm_identity = true;
};

struct SourceCharIKRodLoadPlan {
  bool known_revision = false;
  std::vector<std::string> read_order;
};

struct SourceCharIKRodCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> copied_members;
};

struct SourceCharIKRodHandlerPlan {
  std::vector<std::string> superclasses;
  int check = 0;
};

struct SourceCharIKRodPropSyncPlan {
  std::vector<std::string> modify_alt_properties;
  std::vector<std::string> modify_actions;
};

struct SourceCharIKRodPollDeps {
  std::vector<std::string> change;
  std::vector<std::string> changed_by;
};

// Source-backed CharIKRod::ComputeRod/Poll helper. Returns false when any
// source-required endpoint or destination transform is unresolved.
SourceCharIKRodDefaultState source_char_ik_rod_default_state();
SourceCharIKRodLoadPlan source_char_ik_rod_load_plan(int32_t revision);
SourceCharIKRodCopyPlan source_char_ik_rod_copy_plan();
SourceCharIKRodHandlerPlan source_char_ik_rod_handler_plan();
SourceCharIKRodPropSyncPlan source_char_ik_rod_prop_sync_plan();
void source_char_ik_rod_poll_deps(SourceCharIKRodPollDeps& deps,
                                  const CharIKRod& rod);
bool source_char_ik_rod_compute_world(const CharIKRod& rod,
                                      const Character& character,
                                      std::array<float, 16>& dest_world);

struct SourceCharIKHandMeasure {
  bool has_elbow_chain = false;
  float inv_2ab = 0.0f;
  float a2_plus_b2 = 0.0f;
  float aa_plus_bb = 0.0f;
};

struct SourceCharIKHandLoadPlan {
  int32_t max_revision = 0x0c;
  bool known_revision = false;
  std::vector<std::string> read_order;
  std::vector<std::string> branches;
  bool calls_set_hand = false;
};

struct SourceCharIKHandCopyPlan {
  std::vector<std::string> copied_superclasses;
  std::vector<std::string> member_steps;
  bool creates_copy = true;
};

struct SourceCharIKHandHandlerPlan {
  std::vector<std::string> handlers;
  std::vector<std::string> superclasses;
  std::string check;
};

struct SourceCharIKHandPropSyncPlan {
  std::vector<std::string> target_properties;
  std::vector<std::string> set_properties;
  std::vector<std::string> properties;
  std::string superclass;
};

struct SourceCharIKHandTargetInput {
  bool present = true;
  std::array<float, 3> world_pos = {0.0f, 0.0f, 0.0f};
  float extent = 0.0f;
  std::optional<std::array<float, 4>> world_quat;
};

struct SourceCharIKHandTargetBlendResult {
  bool entered = false;
  float sum = 0.0f;
  float adjusted_weight = 0.0f;
  bool reduced_weight_for_low_sum = false;
  std::array<float, 3> blended_pos = {0.0f, 0.0f, 0.0f};
  bool orientation_blended = false;
  bool orientation_normalized = false;
  std::array<float, 4> blended_quat = {0.0f, 0.0f, 0.0f, 0.0f};
  std::vector<float> weights;
};

struct SourceCharIKHandFingerTargetResult {
  bool applied = false;
  milo_scene::Xfm adjusted_target;
};

struct SourceCharIKFootState {
  bool helper_target_created = true;
  bool helper_target_local_reset = true;
  int fsm_state = 0;
  std::string data;
  int data_index = 0;
  std::array<float, 3> planted_pos = {0.0f, 0.0f, 0.0f};
  float release_distance = 0.0f;
  std::string character_dir;
};

struct SourceCharIKFootEnterResult {
  bool reset_fsm_state = false;
  bool reset_release_distance = false;
};

struct SourceCharIKFootSetNameResult {
  bool call_hmx_set_name = false;
  bool assigned_character = false;
};

struct SourceCharIKFootPollPlan {
  bool should_poll = false;
  bool clear_targets_before = false;
  bool push_helper_target = false;
  bool run_do_fsm = false;
  bool call_char_ik_hand_poll = false;
  bool clear_targets_after = false;
};

struct SourceCharIKFootPollDepsPlan {
  bool call_char_ik_hand_poll_deps = false;
};

struct SourceCharIKFootFsmResult {
  bool copied_finger_matrix = false;
  bool clamped_negative_delta = false;
  bool planted = false;
  bool returned_from_planted_state = false;
  std::array<float, 3> target_pos = {0.0f, 0.0f, 0.0f};
  int fsm_state = 0;
  float release_distance = 0.0f;
};

struct SourceCharIKFootLoadSteps {
  int32_t max_revision = 6;
  bool known_revision = false;
  bool load_char_ik_hand = false;
  bool read_legacy_symbol = false;
  int legacy_int_reads = 0;
  bool load_data = false;
  bool load_data_index = false;
};

struct SourceCharIKFootCopyResult {
  bool copy_char_ik_hand = false;
  bool copy_data = false;
  bool copy_data_index = false;
};

// Source-backed CharIKHand::MeasureLengths / IKElbow scalar helper. The length
// inputs correspond to mHand->mLocalXfm.v and mHand->TransParent()->mLocalXfm.v.
SourceCharIKHandLoadPlan source_char_ik_hand_load_plan(int32_t revision);
SourceCharIKHandCopyPlan source_char_ik_hand_copy_plan();
SourceCharIKHandHandlerPlan source_char_ik_hand_handler_plan();
SourceCharIKHandPropSyncPlan source_char_ik_hand_prop_sync_plan();
SourceCharIKHandMeasure source_char_ik_hand_measure_lengths(
    bool has_elbow_chain,
    float hand_local_len,
    float parent_local_len);
bool source_char_ik_hand_update_measure_lengths(bool scalable,
                                                bool& hand_changed);
bool source_char_ik_hand_elbow_cosine(
    const SourceCharIKHandMeasure& measure,
    float distance_squared,
    float& out_cosine);
SourceCharIKHandTargetBlendResult source_char_ik_hand_multi_target_blend(
    float char_weight,
    const std::vector<SourceCharIKHandTargetInput>& targets,
    bool orientation = false);
SourceCharIKHandFingerTargetResult source_char_ik_hand_finger_target(
    bool has_finger,
    const milo_scene::Xfm& hand_world,
    const milo_scene::Xfm& finger_world,
    std::array<float, 3> target_pos,
    std::array<float, 4> target_quat);
SourceCharIKFootState source_char_ik_foot_default_state();
SourceCharIKFootEnterResult source_char_ik_foot_enter(
    SourceCharIKFootState& state);
SourceCharIKFootSetNameResult source_char_ik_foot_set_name(
    SourceCharIKFootState& state,
    const std::string& dir_name,
    bool dir_is_character);
SourceCharIKFootPollPlan source_char_ik_foot_poll_plan(
    bool has_finger,
    bool has_hand,
    bool has_data);
SourceCharIKFootPollDepsPlan source_char_ik_foot_poll_deps_plan();
SourceCharIKFootFsmResult source_char_ik_foot_do_fsm(
    SourceCharIKFootState& state,
    const std::array<float, 3>& current_target_pos,
    const std::array<float, 3>& finger_world_pos,
    float data_value,
    float delta_seconds,
    bool character_teleported);
SourceCharIKFootLoadSteps source_char_ik_foot_load_steps(int32_t revision);
SourceCharIKFootCopyResult source_char_ik_foot_copy(
    SourceCharIKFootState& dest,
    const SourceCharIKFootState& source);

// Source-backed CharBoneOffset::Poll helper. Returns false when the source
// object pointer or its parent transform would be missing.
bool source_char_bone_offset_poll_world(
    const CharBoneOffset& offset,
    bool has_dest,
    bool has_parent,
    const milo_scene::Xfm& dest_local,
    const std::array<float, 16>& parent_world,
    std::array<float, 16>& dest_world);
void source_char_bone_offset_apply_to_local(const CharBoneOffset& offset,
                                            milo_scene::Xfm& dest_local);

// Source-backed CharBoneTwist::Poll helpers. Returns false when the source
// bone or target list would be missing.
float source_char_bone_twist_weight(
    const CharBoneTwist& twist,
    const std::unordered_map<std::string, float>& weights_by_name);
bool source_char_bone_twist_poll_world(
    const CharBoneTwist& twist,
    bool has_bone,
    const std::array<float, 16>& bone_world,
    const std::vector<std::array<float, 16>>& target_worlds,
    const std::unordered_map<std::string, float>& weights_by_name,
    std::array<float, 16>& out_world);

struct SourceCharForeTwistPollWorldResult {
  bool applied = false;
  float source_angle_radians = 0.0f;
  float applied_rotation_radians = 0.0f;
  float twist2_position_ratio = 0.0f;
  std::array<float, 16> twist_parent_world = {};
  std::array<float, 16> twist2_world = {};
};

struct SourceCharUpperTwistPollWorldResult {
  bool applied = false;
  std::array<float, 16> twist1_world = {};
  std::array<float, 16> twist2_world = {};
};

// Source-backed CharForeTwist::Poll and CharUpperTwist::Poll world-row
// helpers. These are pure translations of the ihatecompvir routines; callers
// remain responsible for resolving object pointers and converting SetWorldXfm
// results back into local rows.
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
    SourceCharForeTwistPollWorldResult& out);
bool source_char_upper_twist_poll_world(
    bool has_source,
    bool has_twist1,
    bool has_twist2,
    bool has_source_parent,
    const std::array<float, 16>& source_parent_world,
    const std::array<float, 16>& source_world,
    const std::array<float, 16>& twist1_current_world,
    const std::array<float, 16>& twist2_current_world,
    SourceCharUpperTwistPollWorldResult& out);

// Source-backed CharHair::FreezePoseRaw helper. Writes current runtime point
// positions back into point.unk5c in the strand root-parent local basis.
int source_char_hair_freeze_pose_raw(Character& character, CharHair& hair,
                                     SourceCharHairRuntime& state);

// Compatibility helper for callers that only need the stored clip names.
std::vector<std::string> load_clip_group_names(
    const std::string& hdr_path, const std::string& ark_path,
    const std::vector<std::string>& milo_paths,
    const std::string& group_name);

// Apply one frame of a clip to the character's bone local matrices.
// frame_idx is clamped to [0, frames.size()-1].
void apply_clip_frame(const CharClip& clip, int frame_idx, Character& character);
void apply_clip_frame_weighted(const CharClip& clip, int frame_idx,
                               float weight, Character& character);

// Apply decoded character-level controllers that sit outside CharClipSamples.
// Call after clip poses for the frame.
void apply_character_controllers(Character& character, float time_seconds);

void clear_runtime_ik_weights(Character& character);
void set_runtime_ik_weight(Character& character, const std::string& weight_prop,
                           float weight);
void clear_runtime_trans_worlds(Character& character);

// CharIKMidi bridge. GHDX/PS2 player*_fret_pos maps MIDI pitches 40..59 to
// spot_neck_fret01..20 and feeds the character's fret.ik object.
void apply_ik_midi_fret_target(Character& character,
                               const std::string& spot_name,
                               float time_seconds);

// Legacy single-frame helpers kept for --clip screenshot mode.
std::vector<ClipChannel> load_clip_pose(const std::string& hdr_path,
                                        const std::string& ark_path,
                                        const std::string& milo_path,
                                        const std::string& clip_name);
void apply_clip_pose(const std::vector<ClipChannel>& channels, Character& character);
void apply_clip_pose_weighted(const std::vector<ClipChannel>& channels,
                              float weight, Character& character,
                              bool relative = false);
void apply_clip_pose_sampled(const std::vector<ClipChannel>& channels,
                             float weight, Character& character,
                             bool relative = false);
std::vector<ClipChannel> blend_channel_layers(
    const std::vector<ClipChannelLayer>& layers);
void apply_clip_channel_layers(const std::vector<ClipChannelLayer>& layers,
                               Character& character, bool relative = false);

}  // namespace ghogx::character
