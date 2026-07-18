// engine/src/milo_scene/milo_scene.h
//
// milo_scene — decode the 3-D render objects (Trans / Mat / Mesh) out of a GH2
// PS2 MILO container's object directory, so the engine can draw the real scene
// geometry (a venue stage, the note highway board + gems, etc.) instead of
// stand-in quads.
//
// All formats were decoded from the ACTUAL GH2 PS2 bytes (see
// memory/subsystems/milo_format.md + gem_hit.md), cross-checked against the
// entry size. The MILO binary format is platform-independent, so these decoders
// are correct for the runtime-native path (read .milo_ps2 from the ARK, decode
// in memory — never extract intermediate files).
//
// Confirmed byte layouts (GH2 PS2, milo dir version 24):
//
//   Trans  (standalone .trans = version 9; also embedded as the base of Mesh):
//     i32   version (= 9)
//     9     bytes (Hmx::Object base metadata; all zero in practice)
//     48    local  matrix : 12 f32 = 9 rotation (row-major 3x3) + 3 translation
//     48    world  matrix : 12 f32 (identical to local for static props)
//     u32   constraint (RndTransformable::Constraint)
//     str   target (usually empty for GH2 venue props)
//     u8    preserve_scale
//     str   parent/target name (length-prefixed UTF-8; "" if unparented)
//
//   Mat  (material, version 0x1b = 27):
//     i32   version (= 27)
//     9     bytes
//     i32   blend (BLEND_ENUM from macros.dta)
//     4×f32 diffuse colour RGBA
//     u8    use_environ (schema: modulate with environment ambient/lights)
//     u8    prelit      (schema: vertex color/alpha feeds base or ambient)
//     i32   z_mode      (RndMat::ZMode)
//     u8    alpha_cut
//     u8    alpha_write
//     i32   tex_gen
//     i32   tex_wrap
//     48    texture transform matrix
//     str   diffuse texture name (".tex")
//     str   next-pass material name
//     u8    intensify
//     u8    cull
//     u8    alpha_write      (GH2 v27; later revs insert alpha_threshold first)
//     ...   trailing source-schema render-state / next-pass bytes
//
//   Mesh  (version 0x1c = 28):
//     9 bytes      Hmx::Object base metadata
//     Trans base   (version 9 + 48 + 48 + constraint + target +
//                  preserve_scale + parent string, as above, without another
//                  Object metadata block)
//                  The second matrix is preserved as the runtime world matrix;
//                  venue/prop rendering uses it when it carries resolved
//                  hierarchy state, falling back to local-parent composition
//                  when it is still identical to local.
//     Draw  base   : i32 version (= 3) + 21 bytes (showing flag + bounding
//                    sphere [cx,cy,cz,r] + draw-order byte)
//     str   material name (the Mat entry this mesh draws with)
//     str   geometry-owner name (usually the mesh's own name)
//     9     bytes
//     i32   vertex_count
//     verts : vertex_count × 48 bytes, each =
//                position (3×f32) + normal (3×f32) +
//                weight/bone scalars (4×f32, not color in GH2 rev 28) +
//                uv (2×f32)
//     i32   face_count
//     faces : face_count × (3 × u16) triangle indices
//     ...   per-material / bone-group trailing data (not needed to draw)

//   Light (version 6, observed in theatre_lighting.milo_ps2):
//     i32 version (= 6)
//     9 bytes Hmx::Object metadata
//     Trans base (version 9 + 48 + 48 + constraint + target +
//                 preserve_scale + parent string)
//     4xf32 color
//     f32 range
//     i32 type (0 point, 1 directional, 2 fake spot, 3 floor spot)
//     u8 animate_color_from_preset
//     u8 animate_position_from_preset
//
//   Group (version 15 in venue geometry; version 12 observed in UI views):
//     ...   Draw/Anim fields
//     48    local matrix (UI groups store this without the standalone Trans
//           object's 9-byte metadata immediately before it)
//     48    world matrix
//     u32   constraint
//     str   target
//     u8    preserve_scale
//     str   parent name
//     ...   child object refs
//     str   environ ref at the tail when the group draws under an Environ
//
//   Environ (version 5):
//     i32 version (= 5)
//     9 bytes object/base metadata
//     u32 light ref count
//     str[] .lit refs
//     4 RGBA-ish floats at payload base + 0x00 (ambient_color)
//     f32 fog_start at payload base + 0x10
//     f32 fog_end at payload base + 0x14
//     4 RGBA-ish floats at payload base + 0x18 (fog_color when fog is enabled)
//     u8 fog_enable at payload base + 0x28
//     u8 animate_from_preset at payload base + 0x29
//     u8 fade_out at payload base + 0x2a
//     f32 fade_start at payload base + 0x2b
//     f32 fade_end at payload base + 0x2f
//
//   Cam (version 12, observed in ui/gen/metacam.milo_ps2):
//     i32 version (= 12)
//     9 bytes object/base metadata
//     i32 embedded Trans version (= 9)
//     48 local matrix
//     48 world matrix
//     u32 constraint
//     str target
//     u8 preserve_scale
//     str parent
//     f32 near_plane
//     f32 far_plane
//     f32 vertical fov (radians in GH2 PS2 data)
//     4xf32 screen rect x/y/width/height
//     2xf32 z range
//     str target texture

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ghogx::milo_scene {

// 4×3 affine transform stored as a row-major 3x3 rotation + a translation.
// Matches the Harmonix Trans matrix layout exactly.
struct Xfm {
  // rot[row][col], row-major. rot[*][0..2] are the 3 basis rows.
  float rot[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  float pos[3] = {0, 0, 0};
};

struct SourceMiloEditorDtbNodePayloadPlan {
  int32_t node_type = 0;
  std::string node_type_name;
  bool known_node_type = false;
  bool reads_uint32 = false;
  bool reads_float = false;
  bool reads_symbol = false;
  bool reads_array_parent = false;
  bool consumes_no_payload = false;
};

SourceMiloEditorDtbNodePayloadPlan
source_milo_editor_dtb_node_payload_plan(int32_t node_type);

struct SourceRndTransLoadPlan {
  int32_t revision = 0;
  int32_t parent_revision = 0;
  bool standalone = false;
  bool reads_object_fields = false;
  bool reads_local_xfm = true;
  bool reads_world_xfm = true;
  bool reads_old_child_list = false;
  bool old_child_list_is_null_terminated_strings = false;
  bool old_child_list_is_symbols = false;
  bool reads_constraint = false;
  bool reads_target = false;
  bool reads_preserve_scale = false;
  bool reads_parent = true;
};

struct SourceMiloEditorRndTransNewPlan {
  int32_t revision = 0;
  int32_t alt_revision = 0;
  bool sets_revision = true;
  bool sets_alt_revision = true;
  bool local_xfm_identity = true;
  bool world_xfm_identity = true;
  Xfm local_xfm;
  Xfm world_xfm;
};

SourceMiloEditorRndTransNewPlan source_milo_editor_rndtrans_new_plan(
    int32_t revision,
    int32_t alt_revision);

struct SourceRndTransformableCppLoadPlan {
  int32_t revision = 0;
  bool loading_proxy_from_disk = false;
  bool class_is_static = false;
  bool accepted_revision = false;
  bool reads_object_fields_for_static_class = false;
  bool reads_stored_local_world = true;
  bool reads_proxy_temp_transforms = false;
  bool reads_old_child_list = false;
  bool old_child_list_sets_parent = false;
  bool rev6_reads_constraint = false;
  bool rev6_preserve_scale_from_target_world = false;
  bool reads_legacy_assert_vector = false;
  bool reads_legacy_bool = false;
  bool reads_sphere = false;
  bool may_set_drawable_sphere = false;
  bool reads_target = false;
  bool proxy_loads_target_ref = false;
  bool reads_preserve_scale = false;
  bool reads_parent = false;
  bool proxy_loads_parent_ref = false;
  bool parent_sets_trans_parent = false;
  bool rev7_8_parent_sets_constraint_parent_world = false;
  bool rev6_parent_from_target_when_constraint_parent_world = false;
};

struct SourceRndTransformableDefaultState {
  bool parent_null = true;
  bool target_null = true;
  int32_t constraint = 0;
  bool preserve_scale = false;
  bool local_xfm_reset = true;
  bool world_xfm_reset = true;
  bool cache_allocated = true;
  bool cache_set_to_self = true;
};

struct SourceRndTransformableSavePlan {
  int32_t save_id = 586;
};

struct SourceRndTransformableDirtyPlan {
  bool cache_already_dirty = false;
  bool set_dirty_force = false;
  bool sets_last_bit = false;
  bool propagates_to_children = false;
};

struct SourceRndTransformableParentPlan {
  bool same_parent = false;
  bool preserve_world = false;
  bool had_old_parent = false;
  bool has_new_parent = false;
  bool asserts_new_parent_not_self = true;
  bool same_parent_sets_dirty = false;
  bool computes_reparent_delta = false;
  bool transforms_local_xfm = false;
  bool transforms_trans_anims = false;
  bool removes_from_old_parent = false;
  bool assigns_parent = false;
  bool cache_set_to_new_parent_or_zero = false;
  bool adds_to_new_parent_children = false;
  bool calls_set_dirty = false;
};

struct SourceRndTransformableWorldWritePlan {
  std::string setter;
  bool writes_world_xfm = false;
  bool writes_world_position_only = false;
  bool clears_cache_dirty_bit = false;
  bool calls_updated_world_xfm = false;
  bool dirties_children = false;
};

struct SourceRndTransformableLocalWritePlan {
  std::string setter;
  bool resets_local_xfm = false;
  bool writes_local_xfm = false;
  bool writes_local_rotation = false;
  bool writes_local_position = false;
  bool calls_set_dirty = false;
  bool returns_dirty_local_ref = false;
};

struct SourceRndTransformableConstraintPlan {
  int32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  bool asserts_target_not_self = true;
  bool writes_constraint = true;
  bool writes_preserve_scale = true;
  bool writes_target = true;
  bool calls_set_dirty = true;
};

struct SourceRndTransformableCopyPlan {
  bool object_superclass_only_for_static_class = true;
  bool creates_copy = true;
  std::vector<std::string> member_steps;
};

struct SourceRndTransformableHandlerPlan {
  std::vector<std::string> handlers;
  std::vector<std::string> actions;
  std::vector<std::string> exprs;
  std::vector<std::string> superclasses;
  bool object_superclass_only_for_static_class = true;
  int32_t check = 0x357;
};

struct SourceRndTransformablePropSyncPlan {
  std::vector<std::string> set_properties;
};

struct SourceRndTransformableChildRow {
  std::string name;
  float local_x = 0.0f;
  float local_z = 0.0f;
};

struct SourceRndTransformableDistributedChild {
  std::string name;
  int32_t source_index = 0;
  float original_axis_value = 0.0f;
  float assigned_axis_value = 0.0f;
  bool calls_set_local_xfm = true;
};

struct SourceRndTransformableDistributeChildrenPlan {
  bool horizontal = false;
  float spacing = 0.0f;
  int32_t axis = 2;
  bool entered = false;
  float base_axis_value = 0.0f;
  std::vector<std::string> sorted_children;
  std::vector<SourceRndTransformableDistributedChild> writes;
};

struct SourceRndTransformableCopyLocalToPlan {
  bool iterates_reverse = true;
  bool calls_set_local_xfm = true;
  std::vector<std::string> write_order;
};

SourceRndTransLoadPlan source_rndtrans_load_plan(
    int32_t revision,
    int32_t parent_revision,
    bool standalone);
SourceRndTransformableCppLoadPlan source_rndtransformable_cpp_load_plan(
    int32_t revision,
    bool loading_proxy_from_disk,
    bool class_is_static);
SourceRndTransformableDefaultState source_rndtransformable_default_state();
SourceRndTransformableSavePlan source_rndtransformable_save_plan();
SourceRndTransformableDirtyPlan source_rndtransformable_set_dirty_plan(
    bool cache_already_dirty,
    bool has_children);
SourceRndTransformableParentPlan source_rndtransformable_set_parent_plan(
    bool same_parent,
    bool preserve_world,
    bool had_old_parent,
    bool has_new_parent);
SourceRndTransformableWorldWritePlan source_rndtransformable_world_write_plan(
    const std::string& setter,
    bool has_children);
SourceRndTransformableLocalWritePlan source_rndtransformable_local_write_plan(
    const std::string& setter);
SourceRndTransformableConstraintPlan
source_rndtransformable_set_constraint_plan(
    int32_t constraint,
    const std::string& target,
    bool preserve_scale);
SourceRndTransformableCopyPlan source_rndtransformable_copy_plan();
SourceRndTransformableHandlerPlan source_rndtransformable_handler_plan();
SourceRndTransformablePropSyncPlan source_rndtransformable_prop_sync_plan();
SourceRndTransformableDistributeChildrenPlan
source_rndtransformable_distribute_children_plan(
    bool horizontal,
    float spacing,
    const std::vector<SourceRndTransformableChildRow>& children);
SourceRndTransformableCopyLocalToPlan
source_rndtransformable_copy_local_to_plan(
    const std::vector<std::string>& targets);

struct SourceRndTransProxyDefaultState {
  bool proxy_null = true;
  bool part_null = true;
};

SourceRndTransProxyDefaultState source_rndtrans_proxy_default_state();

struct SourceRndTransProxyLoadPlan {
  int32_t revision = 0;
  bool accepted_revision = false;
  bool reads_object_fields = true;
  bool reads_transformable = false;
  bool reads_proxy = true;
  bool reads_part = true;
  bool calls_sync = true;
};

SourceRndTransProxyLoadPlan source_rndtrans_proxy_load_plan(int32_t revision);

struct SourceRndTransProxySyncPlan {
  bool clears_parent_first = true;
  bool has_proxy = false;
  bool part_null = false;
  bool attempts_direct_proxy_parent = false;
  bool uses_direct_proxy_parent = false;
  bool attempts_part_lookup = false;
  bool uses_part_lookup_parent = false;
  bool clears_parent_final = true;
  std::string resolved_parent_source = "none";
};

SourceRndTransProxySyncPlan source_rndtrans_proxy_sync_plan(
    bool has_proxy,
    bool part_null,
    bool proxy_is_transformable,
    bool part_lookup_found_transformable);

struct SourceRndTransProxySetterPlan {
  bool value_changed = false;
  bool assigns_value = false;
  bool calls_sync = false;
};

SourceRndTransProxySetterPlan source_rndtrans_proxy_setter_plan(
    bool value_changed);

struct SourceRndTransProxySavePlan {
  bool presave_clears_parent = true;
  bool postsave_calls_sync = true;
};

SourceRndTransProxySavePlan source_rndtrans_proxy_save_plan();

struct SourceRndTransProxyCopyPlan {
  std::vector<std::string> superclasses;
  std::vector<std::string> member_order;
  bool calls_sync = true;
};

SourceRndTransProxyCopyPlan source_rndtrans_proxy_copy_plan();

struct SourceRndTransProxyHandlerPlan {
  std::vector<std::string> superclasses;
  int32_t check = 0x6A;
};

SourceRndTransProxyHandlerPlan source_rndtrans_proxy_handler_plan();

struct SourceRndTransProxyPropSyncPlan {
  std::vector<std::string> props;
  std::vector<std::string> superclasses;
};

SourceRndTransProxyPropSyncPlan source_rndtrans_proxy_prop_sync_plan();

struct SourceRndTransAnimDefaultState {
  bool trans_null = true;
  bool trans_spline = false;
  bool scale_spline = false;
  bool rot_slerp = false;
  bool rot_spline = false;
  bool keys_owner_self = true;
  bool repeat_trans = false;
  bool follow_path = false;
};

SourceRndTransAnimDefaultState source_rndtrans_anim_default_state();

struct SourceRndTransAnimLoadPlan {
  int32_t revision = 0;
  bool accepted_revision = false;
  bool reads_object_fields = false;
  bool reads_animatable = true;
  bool dumps_drawable = false;
  bool reads_trans = true;
  bool reads_rot_and_trans_keys = true;
  bool reads_scale_keys = false;
  bool reads_keys_owner = true;
  bool null_keys_owner_defaults_to_self = true;
  bool reads_legacy_int = false;
  bool reads_follow_path = false;
  bool follow_path_from_keys_owner = false;
  bool reads_rot_slerp = false;
  bool reads_rot_spline = false;
};

SourceRndTransAnimLoadPlan source_rndtrans_anim_load_plan(int32_t revision);

struct SourceRndTransAnimSetKeysOwnerPlan {
  bool asserts_non_null = true;
  bool assigns_keys_owner = true;
};

SourceRndTransAnimSetKeysOwnerPlan source_rndtrans_anim_set_keys_owner_plan();

struct SourceRndTransAnimReplacePlan {
  bool calls_object_replace = true;
  bool keys_owner_matches_from = false;
  bool replacement_null = false;
  bool assigns_self = false;
  bool copies_replacement_keys_owner = false;
};

SourceRndTransAnimReplacePlan source_rndtrans_anim_replace_plan(
    bool keys_owner_matches_from,
    bool replacement_null);

struct SourceRndTransAnimCopyPlan {
  std::vector<std::string> superclasses;
  bool copies_trans = true;
  bool copies_keys_owner_ref = false;
  bool assigns_self_as_keys_owner = false;
  std::vector<std::string> copied_owned_members;
};

SourceRndTransAnimCopyPlan source_rndtrans_anim_copy_plan(
    bool copy_shallow,
    bool copy_from_max,
    bool source_keys_owner_is_self);

struct SourceRndTransAnimFramePlan {
  bool calls_animatable_set_frame = true;
  bool has_trans = false;
  bool copies_local_transform = false;
  bool calls_make_transform = false;
  bool writes_local_transform = false;
  bool make_transform_assert_body_only = true;
};

SourceRndTransAnimFramePlan source_rndtrans_anim_set_frame_plan(bool has_trans);

struct SourceRndTransAnimSetKeyPlan {
  bool has_trans = false;
  std::vector<std::string> operations;
};

SourceRndTransAnimSetKeyPlan source_rndtrans_anim_set_key_plan(bool has_trans);

struct SourceRndTransAnimHandlerPlan {
  std::vector<std::string> handlers;
  std::vector<std::string> superclasses;
  int32_t check = 489;
};

SourceRndTransAnimHandlerPlan source_rndtrans_anim_handler_plan();

struct SourceRndTransAnimPropSyncPlan {
  std::vector<std::string> props;
  std::vector<std::string> superclasses;
};

SourceRndTransAnimPropSyncPlan source_rndtrans_anim_prop_sync_plan();

enum SourceRndAnimRate {
  kSourceRndAnimRate30Fps = 0,
  kSourceRndAnimRate480Fpb = 1,
  kSourceRndAnimRate30FpsUi = 2,
  kSourceRndAnimRate1Fpb = 3,
  kSourceRndAnimRate30FpsTutorial = 4,
  kSourceRndAnimRateUnknown = 5,
};

struct SourceRndAnimatableDefaultState {
  float frame = 0.0f;
  SourceRndAnimRate rate = kSourceRndAnimRate30Fps;
};

SourceRndAnimatableDefaultState source_rndanimatable_default_state();

struct SourceRndAnimatableLoadPlan {
  int32_t revision = 0;
  bool accepted_revision = false;
  bool default_frame_zero = true;
  bool default_rate_30_fps = true;
  bool reads_frame = false;
  bool reads_int_rate = false;
  bool reads_legacy_byte_rate = false;
  bool reads_legacy_rev0_filter_rows = false;
  bool reads_legacy_rev0_anim_list = false;
};

SourceRndAnimatableLoadPlan source_rndanimatable_load_plan(
    int32_t revision);

struct SourceRndAnimatableRatePlan {
  SourceRndAnimRate rate = kSourceRndAnimRateUnknown;
  bool valid_rate = false;
  std::string task_units;
  float frames_per_unit = 0.0f;
};

SourceRndAnimatableRatePlan source_rndanimatable_rate_plan(
    SourceRndAnimRate rate);

struct SourceRndAnimatableConvertFramesPlan {
  SourceRndAnimRate rate = kSourceRndAnimRateUnknown;
  float input_frames = 0.0f;
  float output_units = 0.0f;
  bool returns_converted = false;
  std::string task_units;
};

SourceRndAnimatableConvertFramesPlan source_rndanimatable_convert_frames_plan(
    SourceRndAnimRate rate,
    float input_frames);

struct SourceRndAnimatableCopyPlan {
  bool requires_animatable_source = true;
  bool copies_frame = true;
  bool copies_rate = true;
  bool ignores_non_animatable_source = true;
};

SourceRndAnimatableCopyPlan source_rndanimatable_copy_plan();

struct SourceRndAnimatableHandlerPlan {
  std::vector<std::string> handlers;
  int32_t check = 0x16C;
};

SourceRndAnimatableHandlerPlan source_rndanimatable_handler_plan();

struct SourceRndAnimatablePropSyncPlan {
  std::vector<std::string> props;
};

SourceRndAnimatablePropSyncPlan source_rndanimatable_prop_sync_plan();

struct SourceRndAnimatableAnimatePlan {
  std::vector<std::string> defaults;
  std::vector<std::string> data_keys;
  std::vector<std::string> mode_rows;
  bool creates_anim_task = true;
  bool named_task_requires_data_this = true;
  bool wait_requires_same_rate = true;
  bool starts_task_manager = true;
};

SourceRndAnimatableAnimatePlan source_rndanimatable_on_animate_plan();

struct SourceAnimTaskInitPlan {
  float start = 0.0f;
  float end = 0.0f;
  float frames_per_unit = 0.0f;
  bool loop = false;
  float blend_period = 0.0f;
  float min_frame = 0.0f;
  float max_frame = 0.0f;
  float scale = 0.0f;
  float offset = 0.0f;
  bool scans_anim_target_refs_for_blend_task = true;
  bool marks_blend_task_when_blending = false;
  bool calls_start_anim = true;
};

SourceAnimTaskInitPlan source_anim_task_init_plan(
    float start,
    float end,
    float frames_per_unit,
    bool loop,
    float blend_period,
    bool has_blend_task);

struct SourceAnimTaskTimePlan {
  float min_frame = 0.0f;
  float max_frame = 0.0f;
  float current_frame = 0.0f;
  float frames_per_unit = 0.0f;
  float scale = 0.0f;
  float time_until_end = 0.0f;
};

SourceAnimTaskTimePlan source_anim_task_time_until_end_plan(
    float min_frame,
    float max_frame,
    float current_frame,
    float frames_per_unit,
    float scale);

struct SourceRndPollableHandlerPlan {
  std::vector<std::string> actions;
  std::vector<std::string> static_actions;
  int32_t check = 0x1A;
};

SourceRndPollableHandlerPlan source_rndpollable_handler_plan();

struct SourceRndPollableBasePlan {
  bool poll_body_empty = true;
  bool enter_handles_enter_msg = true;
  bool exit_handles_exit_msg = true;
  bool list_poll_children_empty = true;
};

SourceRndPollableBasePlan source_rndpollable_base_plan();

struct SourceRndPollAnimDefaultState {
  bool anims_no_null = true;
};

SourceRndPollAnimDefaultState source_rndpollanim_default_state();

struct SourceRndPollAnimEndFramePlan {
  std::vector<float> child_end_frames;
  float result = 0.0f;
};

SourceRndPollAnimEndFramePlan source_rndpollanim_end_frame_plan(
    const std::vector<float>& child_end_frames);

struct SourceRndPollAnimChildListPlan {
  int32_t child_count = 0;
  int32_t published_children = 0;
};

SourceRndPollAnimChildListPlan source_rndpollanim_child_list_plan(
    int32_t child_count);

struct SourceRndPollAnimLifecyclePlan {
  int32_t child_count = 0;
  int32_t start_anim_calls = 0;
  int32_t end_anim_calls = 0;
};

SourceRndPollAnimLifecyclePlan source_rndpollanim_enter_plan(
    int32_t child_count);
SourceRndPollAnimLifecyclePlan source_rndpollanim_exit_plan(
    int32_t child_count);

struct SourceRndPollAnimRateFramePlan {
  SourceRndAnimRate rate = kSourceRndAnimRateUnknown;
  bool recognized = false;
  bool uses_seconds = false;
  bool uses_ui_seconds = false;
  bool uses_tutorial_seconds = false;
  bool uses_beat = false;
  float multiplier = 0.0f;
  float frame = 0.0f;
};

SourceRndPollAnimRateFramePlan source_rndpollanim_rate_frame_plan(
    SourceRndAnimRate rate,
    float seconds,
    float ui_seconds,
    float tutorial_seconds,
    float beat);

struct SourceRndPollAnimPollPlan {
  int32_t child_count = 0;
  bool calls_set_frame = false;
  float blend = 1.0f;
};

SourceRndPollAnimPollPlan source_rndpollanim_poll_plan(int32_t child_count);

struct SourceRndPollAnimLoadPlan {
  int32_t revision = 0;
  bool accepted_revision = false;
  std::vector<std::string> superclasses;
  bool reads_anims = true;
};

SourceRndPollAnimLoadPlan source_rndpollanim_load_plan(int32_t revision);

struct SourceRndPollAnimCopyPlan {
  std::vector<std::string> superclasses;
  bool copies_anims = true;
};

SourceRndPollAnimCopyPlan source_rndpollanim_copy_plan();

struct SourceRndPollAnimEmptyBodyPlan {
  bool start_anim_empty = true;
  bool end_anim_empty = true;
  bool set_frame_empty = true;
};

SourceRndPollAnimEmptyBodyPlan source_rndpollanim_empty_body_plan();

struct SourceRndPollAnimHandlerPlan {
  std::vector<std::string> superclasses;
  int32_t check = 0x8B;
};

SourceRndPollAnimHandlerPlan source_rndpollanim_handler_plan();

struct SourceRndPollAnimPropSyncPlan {
  std::vector<std::string> props;
  std::vector<std::string> superclass_order;
  bool returns_animatable_when_handled = true;
  bool falls_back_to_pollable = true;
};

SourceRndPollAnimPropSyncPlan source_rndpollanim_prop_sync_plan();

enum SourcePropKeysAnimKeysType {
  kSourcePropKeysFloat = 0,
  kSourcePropKeysColor = 1,
  kSourcePropKeysObject = 2,
  kSourcePropKeysBool = 3,
  kSourcePropKeysQuat = 4,
  kSourcePropKeysVector3 = 5,
  kSourcePropKeysSymbol = 6,
};

enum SourcePropKeysInterpolation {
  kSourcePropKeysStep = 0,
  kSourcePropKeysLinear = 1,
  kSourcePropKeysSpline = 2,
  kSourcePropKeysSlerp = 3,
  kSourcePropKeysHermite = 4,
  kSourcePropKeysInterp5 = 5,
  kSourcePropKeysInterp6 = 6,
};

enum SourcePropKeysExceptionId {
  kSourcePropKeysNoException = 0,
  kSourcePropKeysTransQuat = 1,
  kSourcePropKeysTransScale = 2,
  kSourcePropKeysTransPos = 3,
  kSourcePropKeysDirEvent = 4,
  kSourcePropKeysHandleInterp = 5,
  kSourcePropKeysMacro = 6,
};

struct SourceRndPropAnimDefaultState {
  float last_frame = 0.0f;
  bool in_set_frame = false;
  bool loop = false;
};

SourceRndPropAnimDefaultState source_rndpropanim_default_state();

struct SourceRndPropAnimLoadPlan {
  int32_t revision = 0;
  bool accepted_revision = false;
  bool sets_prop_keys_revision = true;
  std::vector<std::string> superclasses;
  bool captures_last_frame_from_anim_frame = true;
  bool removes_existing_keys = true;
  bool uses_pre7_loader = false;
  bool reads_key_count = false;
  bool reads_key_type_per_entry = false;
  bool loads_prop_keys_per_entry = false;
  bool reads_loop = false;
};

SourceRndPropAnimLoadPlan source_rndpropanim_load_plan(int32_t revision);

struct SourceRndPropAnimPre7LoadPlan {
  int32_t revision = 0;
  bool reads_legacy_owner_before_count = false;
  bool reads_count = true;
  bool reads_owner_per_entry = false;
  bool reads_symbol_property = false;
  bool reads_dataarray_property = false;
  bool reads_float_keys_only = false;
  bool reads_anim_type = false;
  bool reads_float_keys = true;
  bool reads_color_keys = false;
  bool reads_object_keys_with_owner_stage = false;
  bool reads_bool_keys = false;
  bool reads_quat_keys = false;
};

SourceRndPropAnimPre7LoadPlan source_rndpropanim_pre7_load_plan(
    int32_t revision);

struct SourceRndPropAnimCopyPlan {
  std::vector<std::string> superclasses;
  bool captures_last_frame_from_get_frame = true;
  bool removes_existing_keys = true;
  bool copies_prop_keys = true;
  bool copies_loop = true;
};

SourceRndPropAnimCopyPlan source_rndpropanim_copy_plan();

struct SourceRndPropAnimFrameBoundsPlan {
  std::vector<float> key_frames;
  float result = 0.0f;
};

SourceRndPropAnimFrameBoundsPlan source_rndpropanim_start_frame_plan(
    const std::vector<float>& key_start_frames);
SourceRndPropAnimFrameBoundsPlan source_rndpropanim_end_frame_plan(
    const std::vector<float>& key_end_frames);

struct SourceRndPropAnimAdvanceFramePlan {
  bool loop = false;
  bool applies_mod_range = false;
  bool calls_animatable_set_frame = true;
  float blend = 1.0f;
};

SourceRndPropAnimAdvanceFramePlan source_rndpropanim_advance_frame_plan(
    bool loop);

struct SourceRndPropAnimSetFramePlan {
  bool already_in_set_frame = false;
  int32_t key_count = 0;
  int32_t dir_event_key_count = 0;
  bool enters_set_frame_guard = false;
  bool calls_advance_frame = false;
  bool scans_dir_event_keys = false;
  bool sets_each_key_frame = false;
  bool updates_last_frame = false;
  bool clears_set_frame_guard = false;
};

SourceRndPropAnimSetFramePlan source_rndpropanim_set_frame_plan(
    bool already_in_set_frame,
    int32_t key_count,
    int32_t dir_event_key_count);

struct SourceRndPropAnimKeyListPlan {
  int32_t key_count = 0;
  int32_t calls = 0;
};

SourceRndPropAnimKeyListPlan source_rndpropanim_set_key_plan(
    int32_t key_count);
SourceRndPropAnimKeyListPlan source_rndpropanim_start_anim_plan(
    int32_t key_count);
SourceRndPropAnimKeyListPlan source_rndpropanim_remove_all_keys_plan(
    int32_t key_count);

struct SourceRndPropAnimFindKeysPlan {
  bool property_null = false;
  bool target_matches = false;
  bool property_matches = false;
  bool matches_null_property_row = false;
  bool found = false;
};

SourceRndPropAnimFindKeysPlan source_rndpropanim_find_keys_plan(
    bool property_null,
    bool target_matches,
    bool property_matches,
    bool row_property_null);

struct SourceRndPropAnimChangePropPathPlan {
  bool new_path_empty = false;
  bool calls_remove_keys = false;
  bool found_existing_keys = false;
  bool sets_new_prop = false;
  bool result = false;
};

SourceRndPropAnimChangePropPathPlan source_rndpropanim_change_prop_path_plan(
    bool new_path_empty,
    bool found_existing_keys);

struct SourceRndPropAnimValuePlan {
  SourcePropKeysAnimKeysType type = kSourcePropKeysFloat;
  bool has_keys = false;
  bool valid_index = false;
  bool result = false;
  std::string output_kind;
};

SourceRndPropAnimValuePlan source_rndpropanim_value_from_index_plan(
    SourcePropKeysAnimKeysType type,
    bool has_keys,
    bool valid_index);
SourceRndPropAnimValuePlan source_rndpropanim_value_from_frame_plan(
    SourcePropKeysAnimKeysType type,
    bool has_keys);

struct SourceRndPropAnimHandlerPlan {
  std::vector<std::string> expressions;
  std::vector<std::string> actions;
  std::vector<std::string> handlers;
  std::vector<std::string> superclasses;
  int32_t check = 0x43C;
};

SourceRndPropAnimHandlerPlan source_rndpropanim_handler_plan();

struct SourceRndPropAnimPropSyncPlan {
  std::vector<std::string> props;
  std::vector<std::string> superclasses;
};

SourceRndPropAnimPropSyncPlan source_rndpropanim_prop_sync_plan();

struct SourcePropKeysDefaultState {
  bool prop_null = true;
  bool trans_null = true;
  int32_t last_key_frame_index = -2;
  SourcePropKeysAnimKeysType keys_type = kSourcePropKeysFloat;
  SourcePropKeysInterpolation interpolation = kSourcePropKeysLinear;
  SourcePropKeysExceptionId exception_id = kSourcePropKeysNoException;
  bool last_bit = false;
};

SourcePropKeysDefaultState source_propkeys_default_state();

struct SourcePropKeysLoadPlan {
  int32_t revision = 0;
  bool accepted_revision = false;
  bool fails_pre7 = false;
  bool reads_keys_type = false;
  bool reads_target = false;
  bool reads_prop = false;
  bool reads_interpolation = false;
  bool derives_legacy_interpolation = false;
  bool legacy_macro_exception_branch = false;
  bool reads_interp_handler = false;
  bool reads_exception_id = false;
  bool reads_last_bit = false;
  bool calls_set_prop_exception_id = false;
};

SourcePropKeysLoadPlan source_propkeys_load_plan(int32_t revision,
                                                 int32_t interpolation_row);

struct SourcePropKeysExceptionPlan {
  std::string property;
  bool target_is_trans = false;
  bool target_is_object_dir = false;
  SourcePropKeysExceptionId exception_id = kSourcePropKeysNoException;
};

SourcePropKeysExceptionPlan source_propkeys_exception_plan(
    const std::string& property,
    bool target_is_trans,
    bool target_is_object_dir);

struct SourcePropKeysSetPropExceptionPlan {
  bool interp_handler_null = true;
  SourcePropKeysExceptionId current_exception = kSourcePropKeysNoException;
  SourcePropKeysExceptionId property_exception = kSourcePropKeysNoException;
  SourcePropKeysExceptionId result_exception = kSourcePropKeysNoException;
  bool updates_transform_cache = false;
};

SourcePropKeysSetPropExceptionPlan source_propkeys_set_prop_exception_plan(
    bool interp_handler_null,
    SourcePropKeysExceptionId current_exception,
    SourcePropKeysExceptionId property_exception);

struct SourceRndMeshAnimDefaultState {
  bool mesh_null = true;
  bool keys_owner_self = true;
};

SourceRndMeshAnimDefaultState source_rndmeshanim_default_state();

struct SourceRndMeshAnimNumVertsPlan {
  int32_t points_keys = 0;
  int32_t normals_keys = 0;
  int32_t texs_keys = 0;
  int32_t colors_keys = 0;
  int32_t result = 0;
  std::vector<std::string> nonempty_sources;
};

SourceRndMeshAnimNumVertsPlan source_rndmeshanim_num_verts_plan(
    int32_t points_keys,
    int32_t normals_keys,
    int32_t texs_keys,
    int32_t colors_keys);

struct SourceRndMeshAnimReplacePlan {
  bool calls_object_replace = true;
  bool keys_owner_matches_from = false;
  bool replacement_null = false;
  bool assigns_self = false;
  bool copies_replacement_keys_owner = false;
};

SourceRndMeshAnimReplacePlan source_rndmeshanim_replace_plan(
    bool keys_owner_matches_from,
    bool replacement_null);

struct SourceRndMeshAnimLoadPlan {
  int32_t revision = 0;
  bool accepted_revision = false;
  bool reads_object_fields = false;
  bool reads_animatable = true;
  bool reads_mesh = true;
  bool reads_vert_points_keys = true;
  bool reads_vert_normals_keys = false;
  bool reads_vert_texs_keys = true;
  bool reads_vert_colors_keys = true;
  bool reads_keys_owner = true;
  bool null_keys_owner_defaults_to_self = true;
};

SourceRndMeshAnimLoadPlan source_rndmeshanim_load_plan(int32_t revision);

struct SourceRndMeshAnimCopyPlan {
  std::vector<std::string> superclasses;
  bool copies_mesh = true;
  bool copies_keys_owner_ref = false;
  bool assigns_self_as_keys_owner = false;
  std::vector<std::string> copied_owned_members;
};

SourceRndMeshAnimCopyPlan source_rndmeshanim_copy_plan(
    bool copy_shallow,
    bool copy_from_max,
    bool source_keys_owner_is_self);

struct SourceRndMeshAnimEndFramePlan {
  float points_last = 0.0f;
  float normals_last = 0.0f;
  float texs_last = 0.0f;
  float colors_last = 0.0f;
  float result = 0.0f;
};

SourceRndMeshAnimEndFramePlan source_rndmeshanim_end_frame_plan(
    float points_last,
    float normals_last,
    float texs_last,
    float colors_last);

struct SourceRndMeshAnimInterpPlan {
  float ref = 0.0f;
  float blend = 1.0f;
  int32_t source_values = 0;
  int32_t mesh_verts = 0;
  int32_t affected_verts = 0;
  bool uses_first_key = false;
  bool uses_second_key = false;
  bool interpolates_between_keys = false;
  bool blends_with_existing_vert = false;
};

SourceRndMeshAnimInterpPlan source_rndmeshanim_interp_plan(
    float ref,
    float blend,
    int32_t source_values,
    int32_t mesh_verts);

struct SourceRndMeshAnimSetFramePlan {
  bool calls_animatable_set_frame = true;
  bool has_mesh = false;
  uint32_t mesh_mutable_mask = 0;
  bool mesh_mutable = false;
  bool notifies_not_mutable = false;
  bool evaluates_points = false;
  bool evaluates_normals = false;
  bool evaluates_texs = false;
  bool evaluates_colors = false;
  uint32_t sync_mask = 0;
  bool calls_mesh_sync = false;
};

SourceRndMeshAnimSetFramePlan source_rndmeshanim_set_frame_plan(
    bool has_mesh,
    uint32_t mesh_mutable_mask,
    bool has_points_keys,
    bool has_normals_keys,
    bool has_texs_keys,
    bool has_colors_keys);

struct SourceRndMeshAnimSetKeyPlan {
  bool body_empty = true;
};

SourceRndMeshAnimSetKeyPlan source_rndmeshanim_set_key_plan();

struct SourceRndMeshAnimShrinkPlan {
  int32_t requested_count = 0;
  bool points_nonempty = false;
  bool normals_nonempty = false;
  bool texs_nonempty = false;
  bool colors_nonempty = false;
  std::vector<std::string> resized_streams;
};

SourceRndMeshAnimShrinkPlan source_rndmeshanim_shrink_verts_plan(
    int32_t requested_count,
    bool points_nonempty,
    bool normals_nonempty,
    bool texs_nonempty,
    bool colors_nonempty);

SourceRndMeshAnimShrinkPlan source_rndmeshanim_shrink_keys_plan(
    int32_t requested_count,
    bool points_nonempty,
    bool normals_nonempty,
    bool texs_nonempty,
    bool colors_nonempty);

struct SourceRndMeshAnimHandlerPlan {
  std::vector<std::string> expressions;
  std::vector<std::string> actions;
  std::vector<std::string> superclasses;
  int32_t check = 0x207;
};

SourceRndMeshAnimHandlerPlan source_rndmeshanim_handler_plan();

struct SourceRndMeshAnimPropSyncPlan {
  std::vector<std::string> props;
  std::vector<std::string> superclasses;
};

SourceRndMeshAnimPropSyncPlan source_rndmeshanim_prop_sync_plan();

struct TransObj {
  std::string name;          // the entry name
  Xfm local;                 // local matrix (matrix 1)
  Xfm world_stored;          // world matrix as stored (matrix 2)
  uint32_t constraint = 0;    // RndTransformable::Constraint
  std::string target;         // optional target name used by constrained transforms
  bool preserve_scale = false;
  std::string parent;        // parent/target name ("" if none)
};

// A camera (Cam) — Trans base + projection params. Decoded so the scene viewer
// can optionally frame the venue from its authored camera.
struct CamObj {
  std::string name;
  uint16_t revision = 0;
  uint16_t alt_revision = 0;
  uint16_t trans_revision = 0;
  Xfm local;                 // camera transform (pos in local.pos)
  Xfm world_stored;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  std::string parent;
  float near_plane = 1.0f;
  float far_plane = 1000.0f;
  float fov = 0.5f;          // vertical fov, radians
  float screen_rect[4] = {0.0f, 0.0f, 1.0f, 1.0f};
  float z_range[2] = {0.0f, 1.0f};
  std::string target_tex;
  bool source_order_decoded = false;
  bool decoded = false;
};

struct WaypointObj {
  std::string name;
  Xfm local;
  Xfm world_stored;
  uint32_t flags = 0;
  bool decoded = false;
};

struct SpotlightObj {
  std::string name;
  uint16_t revision = 0;
  uint16_t draw_revision = 0;
  uint16_t trans_revision = 0;
  std::string parent;
  Xfm local;
  Xfm world_stored;
  uint32_t constraint = 0;
  std::string trans_target;
  bool preserve_scale = false;
  bool has_transform = false;
  bool showing = true;
  float draw_order = 0.0f;
  float default_color[3] = {1.0f, 1.0f, 1.0f};
  float default_intensity = 1.0f;
  bool has_default_state = false;
  bool beam_is_cone = false;
  float beam_length = 0.0f;
  float beam_bottom_radius = 0.0f;
  float beam_top_radius = 0.0f;
  float beam_top_side_border = 0.0f;
  float beam_bottom_side_border = 0.0f;
  float beam_bottom_border = 0.0f;
  float beam_offset = 0.0f;
  float beam_target_offset[2] = {0.0f, 0.0f};
  float spot_scale = 30.0f;
  float spot_height = 0.25f;
  float light_can_offset = 0.0f;
  float damping_constant = 1.0f;
  float flare_size[2] = {0.0f, 0.0f};
  float flare_range[2] = {0.0f, 0.0f};
  int32_t flare_steps = 0;
  float flare_offset = 0.0f;
  bool flare_enabled = true;
  bool flare_visibility_test = true;
  float lens_size = 0.0f;
  float lens_offset = 0.0f;
  bool target_shadow = false;
  bool animate_color_from_preset = true;
  bool animate_orientation_from_preset = true;
  std::string material;
  std::string group;
  std::string light_can_group;
  std::string target;
  std::string disc_material;
  std::string flare_material;
  std::string circle_mesh;
  std::vector<std::string> instance_meshes;
  std::string circle_material;
  std::string lens_material;
  bool source_order_decoded = false;
  bool decoded = false;
  std::string error;
};

struct LightObj {
  std::string name;
  Xfm local;
  Xfm world_stored;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  std::string parent;
  float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float range = 0.0f;
  float falloff_start = 0.0f;
  int type = 0;
  bool animate_color_from_preset = false;
  bool animate_position_from_preset = false;
  bool animate_range_from_preset = false;
  bool source_order_decoded = false;
  bool decoded = false;
  std::string error;
};

struct EnvironObj {
  std::string name;
  uint16_t revision = 0;
  std::vector<std::string> lights;
  float color_a[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float fog_start = 0.0f;
  float fog_end = 0.0f;
  float fog_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  bool fog_enabled = false;
  bool animate_from_preset = false;
  bool fade_out = false;
  float fade_start = 0.0f;
  float fade_end = 1000.0f;
  float range_a = 0.0f;
  float range_b = 0.0f;
  float color_b[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float range = 0.0f;
  bool source_order_decoded = false;
  bool decoded = false;
  std::string error;
};

struct GroupObj {
  std::string name;
  std::string parent;
  Xfm local;
  Xfm world_stored;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  bool has_transform = false;
  bool decoded = false;
  bool source_order_decoded = false;
  bool showing = true;
  float draw_order = 0.0f;
  size_t dir_index = 0;
  std::vector<std::string> children;
  std::string environment_ref;
  std::string draw_only;
  std::string lod;
  float lod_screen_size = 0.0f;
  bool sort_in_world = false;
  std::string error;
};

struct BandPlacerObj {
  std::string name;
  std::string kind;
  std::string parent;
  Xfm local;
  Xfm world_stored;
  bool decoded = false;
  std::string error;
};

struct SourceRndDrawableLoadPlan {
  int32_t revision = 0;
  int32_t parent_revision = 0;
  bool accepted_revision = false;
  bool reads_showing = true;
  bool reads_old_drawable_list = false;
  bool old_list_is_null_terminated_strings = false;
  bool old_list_is_symbols = false;
  bool reads_sphere = false;
  bool reads_draw_order = false;
  bool reads_clip_planes = false;
};

SourceRndDrawableLoadPlan source_rnddrawable_load_plan(
    int32_t revision,
    int32_t parent_revision);

struct SourceRndDrawableDefaultState {
  bool showing = true;
  bool sphere_zeroed = true;
  float order = 0.0f;
  int32_t draw_revision = 3;
  int32_t highlight_style_count = 5;
  float normal_display_length = 1.0f;
};

SourceRndDrawableDefaultState source_rnddrawable_default_state();

struct SourceMiloEditorRndDrawableNewPlan {
  int32_t revision = 0;
  int32_t alt_revision = 0;
  bool sets_revision = true;
  bool sets_alt_revision = true;
  bool relies_on_constructor_defaults = true;
  bool does_not_initialize_sphere_or_draw_order = true;
};

SourceMiloEditorRndDrawableNewPlan source_milo_editor_rnddrawable_new_plan(
    int32_t revision,
    int32_t alt_revision);

struct SourceRndDrawableSavePlan {
  int32_t save_id = 0xAE;
};

SourceRndDrawableSavePlan source_rnddrawable_save_plan();

struct SourceRndDrawableDrawPlan {
  bool showing = false;
  bool has_world_sphere = false;
  bool sphere_culled = false;
  bool calls_make_world_sphere = false;
  bool calls_draw_showing = false;
};

SourceRndDrawableDrawPlan source_rnddrawable_draw_plan(
    bool showing,
    bool has_world_sphere,
    bool sphere_culled);

struct SourceRndDrawableBudgetPlan {
  bool returns_true = true;
  bool calls_make_world_sphere = false;
  bool calls_draw_showing_budget = false;
};

SourceRndDrawableBudgetPlan source_rnddrawable_budget_plan(
    bool showing,
    bool has_world_sphere,
    bool sphere_culled);

struct SourceRndDrawableCopyPlan {
  std::vector<std::string> normal_members;
  std::vector<std::string> from_max_members;
  bool from_max_copies_sphere_only_when_both_radii_nonzero = true;
};

SourceRndDrawableCopyPlan source_rnddrawable_copy_plan(
    bool copy_from_max,
    bool dest_sphere_nonzero,
    bool source_sphere_nonzero);

struct SourceRndDrawableCollidePlan {
  bool showing = false;
  bool has_world_sphere = false;
  bool sphere_intersects = false;
  bool collide_sphere_result = false;
  int32_t collide_plane_result = -1;
  bool collide_calls_showing = false;
};

SourceRndDrawableCollidePlan source_rnddrawable_collide_plan(
    bool showing,
    bool has_world_sphere,
    bool sphere_intersects,
    float plane_dot,
    float sphere_radius);

struct SourceRndDrawableHandlerPlan {
  std::vector<std::string> handlers;
  int32_t check = 0;
};

SourceRndDrawableHandlerPlan source_rnddrawable_handler_plan();

struct SourceRndDrawablePropSyncPlan {
  std::vector<std::string> properties;
  std::vector<std::string> showing_ops;
};

SourceRndDrawablePropSyncPlan source_rnddrawable_prop_sync_plan();

struct SourceRndGroupLoadPlan {
  int32_t revision = 0;
  bool reads_object_fields = false;
  bool reads_animatable = true;
  bool reads_trans = true;
  bool reads_drawable = true;
  bool reads_objects = false;
  bool reads_environ = false;
  bool reads_draw_only = false;
  bool reads_lod = false;
  bool reads_legacy_rev4_objects = false;
  bool reads_rev7_lod_dimensions = false;
  bool reads_sort_in_world = false;
};

SourceRndGroupLoadPlan source_rndgroup_load_plan(int32_t revision);

struct SourceRndGroupDefaultState {
  bool objects_owner_control = true;
  bool env_null = true;
  bool draw_only_null = true;
  bool lod_null = true;
  float lod_screen_size = 0.0f;
  bool sort_in_world = false;
  bool unkf8 = false;
};

SourceRndGroupDefaultState source_rndgroup_default_state();

struct SourceMiloEditorRndGroupNewPlan {
  int32_t revision = 0;
  int32_t alt_revision = 0;
  bool sets_revision = true;
  bool sets_alt_revision = true;
  bool relies_on_constructor_defaults = true;
  bool does_not_initialize_membership_or_lod = true;
};

SourceMiloEditorRndGroupNewPlan source_milo_editor_rndgroup_new_plan(
    int32_t revision,
    int32_t alt_revision);

struct SourceRndGroupSavePlan {
  int32_t save_id = 0x30;
};

SourceRndGroupSavePlan source_rndgroup_save_plan();

struct SourceRndGroupCopyPlan {
  std::vector<std::string> superclasses;
  std::vector<std::string> member_order;
  bool deep_copies_objects = true;
  bool from_max_merges_objects = true;
  bool calls_update = true;
};

SourceRndGroupCopyPlan source_rndgroup_copy_plan();

struct SourceRndGroupReplacePlan {
  bool calls_transformable_replace = true;
  bool scans_objects = true;
  bool add_object_when_found = true;
  bool sets_in_replace_around_remove = true;
  bool remove_object_when_found = true;
  bool no_object_no_membership_change = true;
};

SourceRndGroupReplacePlan source_rndgroup_replace_plan(bool object_found);

struct SourceRndGroupHandlerPlan {
  std::vector<std::string> actions;
  std::vector<std::string> queries;
  std::vector<std::string> superclasses;
  int32_t check = 0x29B;
};

SourceRndGroupHandlerPlan source_rndgroup_handler_plan();

struct SourceRndGroupPropSyncPlan {
  std::vector<std::string> props;
  std::vector<std::string> side_effects;
  std::vector<std::string> superclasses;
};

SourceRndGroupPropSyncPlan source_rndgroup_prop_sync_plan();

struct SourceRndMeshDeformVertArrayState {
  int32_t size = 0;
  bool data_null = true;
  bool parent_set = false;
};

SourceRndMeshDeformVertArrayState
source_rndmesh_deform_vert_array_default_state(bool parent_provided);

struct SourceRndMeshDeformVertArraySetSizePlan {
  int32_t old_size = 0;
  int32_t new_size = 0;
  bool changes_size = false;
  bool frees_existing_data = false;
  bool allocates_requested_size = false;
};

SourceRndMeshDeformVertArraySetSizePlan
source_rndmesh_deform_vert_array_set_size_plan(int32_t old_size,
                                               int32_t new_size);

struct SourceRndMeshDeformClearPlan {
  bool calls_set_size_zero = true;
  bool changes_size = false;
};

SourceRndMeshDeformClearPlan source_rndmesh_deform_clear_plan(
    int32_t old_size);

struct SourceRndMeshDeformDefaultState {
  bool mesh_null = true;
  bool bones_parent_set = true;
  bool verts_parent_set = true;
  bool skip_inverse = false;
  bool deformed = false;
};

SourceRndMeshDeformDefaultState source_rndmesh_deform_default_state();

struct SourceRndMeshDeformSetMeshPlan {
  bool assigns_mesh = true;
  bool clears_verts = true;
};

SourceRndMeshDeformSetMeshPlan source_rndmesh_deform_set_mesh_plan();

struct SourceRndMeshDeformHandlerPlan {
  std::vector<std::string> superclasses;
  int32_t check = 0x2A1;
};

SourceRndMeshDeformHandlerPlan source_rndmesh_deform_handler_plan();

struct SourceRndMeshDeformBodyAvailability {
  bool load_body_visible = false;
  bool copy_body_visible = false;
  bool reskin_body_visible = false;
  bool copy_weights_body_visible = false;
  bool find_deform_body_visible = false;
};

SourceRndMeshDeformBodyAvailability
source_rndmesh_deform_body_availability();

struct SourceRndMultiMeshDefaultState {
  bool mesh_null = true;
  bool unk9p4_zero = true;
};

SourceRndMultiMeshDefaultState source_rndmultimesh_default_state();

struct SourceRndMultiMeshInstanceDefaultState {
  bool resets_transform = true;
};

SourceRndMultiMeshInstanceDefaultState
source_rndmultimesh_instance_default_state();

struct SourceRndMultiMeshLoadPlan {
  int32_t revision = 0;
  bool accepted_revision = false;
  bool reads_object_fields = false;
  bool reads_drawable = true;
  bool reads_mesh = true;
  bool reads_legacy_transform_dump_and_returns = false;
  bool reads_instances = false;
  bool reads_legacy_tail_byte = false;
};

SourceRndMultiMeshLoadPlan source_rndmultimesh_load_plan(int32_t revision);

struct SourceRndMultiMeshCopyPlan {
  std::vector<std::string> superclasses;
  bool copies_mesh = true;
  bool copies_instances = true;
  bool calls_update_mesh = true;
};

SourceRndMultiMeshCopyPlan source_rndmultimesh_copy_plan(bool copy_from_max);

struct SourceRndMultiMeshSetMeshPlan {
  bool assigns_mesh = true;
  bool calls_update_mesh = true;
};

SourceRndMultiMeshSetMeshPlan source_rndmultimesh_set_mesh_plan();

struct SourceRndMultiMeshHandlerPlan {
  std::vector<std::string> handlers;
  std::vector<std::string> actions;
  std::vector<std::string> superclasses;
  bool warns_unhandled = true;
};

SourceRndMultiMeshHandlerPlan source_rndmultimesh_handler_plan();

struct SourceRndMultiMeshSetPosPlan {
  int32_t requested_index = 0;
  bool advances_iterator_by_index = true;
  std::vector<std::string> assignment_order;
};

SourceRndMultiMeshSetPosPlan source_rndmultimesh_set_pos_plan(
    int32_t requested_index);

struct SourceRndMultiMeshPropSyncPlan {
  std::vector<std::string> superclasses;
};

SourceRndMultiMeshPropSyncPlan source_rndmultimesh_prop_sync_plan();

struct SourceRndMultiMeshProxyDefaultState {
  bool multimesh_null = true;
  bool index_zero = true;
};

SourceRndMultiMeshProxyDefaultState
source_rndmultimesh_proxy_default_state();

struct SourceRndMultiMeshProxySetPlan {
  bool clears_multimesh_first = true;
  bool has_mesh = false;
  bool copies_instance_local_transform = false;
  bool assigns_multimesh = true;
  bool assigns_index = true;
};

SourceRndMultiMeshProxySetPlan source_rndmultimesh_proxy_set_plan(
    bool has_mesh);

struct SourceRndMultiMeshProxyDrawPlan {
  bool has_multimesh = false;
  bool has_mesh = false;
  bool reads_multimesh_mesh = false;
  bool sets_mesh_world_from_instance = false;
  bool draws_mesh = false;
};

SourceRndMultiMeshProxyDrawPlan source_rndmultimesh_proxy_draw_plan(
    bool has_multimesh,
    bool has_mesh);

struct SourceRndMultiMeshProxyUpdatedWorldPlan {
  bool has_multimesh = false;
  bool writes_instance_from_world = false;
  bool szbe69_variant_visible = true;
};

SourceRndMultiMeshProxyUpdatedWorldPlan
source_rndmultimesh_proxy_updated_world_plan(bool has_multimesh);

struct SourceRndMultiMeshProxyFailurePlan {
  bool load_fails = true;
  bool save_fails = true;
  bool copy_fails = true;
};

SourceRndMultiMeshProxyFailurePlan
source_rndmultimesh_proxy_failure_plan();

struct SourceRndMultiMeshProxyHandlerPlan {
  int32_t check = 0x3F;
};

SourceRndMultiMeshProxyHandlerPlan
source_rndmultimesh_proxy_handler_plan();

struct SourceRndMultiMeshProxyPropSyncPlan {
  bool has_rows = false;
};

SourceRndMultiMeshProxyPropSyncPlan
source_rndmultimesh_proxy_prop_sync_plan();

struct SourceRndWindDefaultState {
  float prevailing[3] = {0.0f, 0.0f, 0.0f};
  float random[3] = {0.0f, 0.0f, 0.0f};
  float time_loop = 100.0f;
  float space_loop = 100.0f;
  bool wind_owner_self = true;
  bool calls_sync_loops = true;
};

struct SourceRndWindSetDefaultsPlan {
  float prevailing[3] = {0.0f, 0.0f, 0.0f};
  float random[3] = {17.0f, 17.0f, 0.0f};
  float time_loop = 100.0f;
  float space_loop = 100.0f;
  bool calls_sync_loops = false;
};

struct SourceRndWindZeroPlan {
  bool zeroes_prevailing = true;
  bool zeroes_random = true;
  bool leaves_time_loop = true;
  bool leaves_space_loop = true;
  bool leaves_wind_owner = true;
  bool calls_sync_loops = false;
};

struct SourceRndWindLoopRatePlan {
  float time_loop = 0.0f;
  float space_loop = 0.0f;
  float time_rate[3] = {0.0f, 0.0f, 0.0f};
  float space_rate[3] = {0.0f, 0.0f, 0.0f};
  bool time_loop_zero = false;
  bool space_loop_zero = false;
};

struct SourceRndWindSavePlan {
  int32_t save_id = 0x96;
};

struct SourceRndWindLoadPlan {
  int32_t revision = 0;
  bool accepted_revision = false;
  bool reads_object_fields = true;
  bool reads_prevailing = true;
  bool reads_random = true;
  bool reads_time_loop = true;
  bool reads_space_loop = true;
  bool reads_wind_owner = false;
  bool calls_set_wind_owner = false;
  bool calls_sync_loops = true;
};

struct SourceRndWindSetOwnerPlan {
  bool input_owner_present = false;
  bool assigns_input_owner = false;
  bool assigns_self = true;
};

struct SourceRndWindCopyPlan {
  bool copy_shallow = false;
  bool copies_object_superclass = true;
  bool shallow_copies_wind_owner = false;
  bool resets_wind_owner_to_self = true;
  bool copies_wind_owner = true;
  bool copies_prevailing = true;
  bool copies_random = true;
  bool copies_time_loop = true;
  bool copies_space_loop = true;
  bool calls_sync_loops = true;
};

struct SourceRndWindReplacePlan {
  bool wind_owner_matches_from = false;
  bool replacement_is_wind = false;
  bool calls_object_replace = true;
  bool calls_set_wind_owner = false;
  bool assigns_replacement_wind = false;
  bool assigns_self = false;
};

struct SourceRndWindRuntimeBoundary {
  bool vector_get_wind_delegates_to_owner = true;
  bool scalar_get_wind_declared = true;
  bool scalar_get_wind_body_visible = false;
  bool self_get_wind_declared = true;
  bool self_get_wind_body_visible = false;
  bool native_generates_wind_force = false;
};

struct SourceRndWindHandlerPlan {
  std::vector<std::string> actions;
  std::vector<std::string> superclasses;
  int32_t check = 0xda;
};

struct SourceRndWindPropSyncPlan {
  std::vector<std::string> direct_rows;
  std::vector<std::string> set_rows;
  std::vector<std::string> modify_rows;
  bool loop_rows_call_sync_loops = true;
};

SourceRndWindDefaultState source_rndwind_default_state();
SourceRndWindSetDefaultsPlan source_rndwind_set_defaults_plan();
SourceRndWindZeroPlan source_rndwind_zero_plan();
SourceRndWindLoopRatePlan source_rndwind_sync_loops(float time_loop,
                                                    float space_loop);
SourceRndWindSavePlan source_rndwind_save_plan();
SourceRndWindLoadPlan source_rndwind_load_plan(int32_t revision);
SourceRndWindSetOwnerPlan source_rndwind_set_owner_plan(
    bool input_owner_present);
SourceRndWindCopyPlan source_rndwind_copy_plan(bool copy_shallow);
SourceRndWindReplacePlan source_rndwind_replace_plan(
    bool wind_owner_matches_from,
    bool replacement_is_wind);
SourceRndWindRuntimeBoundary source_rndwind_runtime_boundary();
SourceRndWindHandlerPlan source_rndwind_handler_plan();
SourceRndWindPropSyncPlan source_rndwind_prop_sync_plan();

struct MatObj {
  std::string name;          // entry name (e.g. "gem.mat")
  std::string diffuse_tex;   // diffuse .tex reference ("" if none)
  float color[4] = {1, 1, 1, 1};  // diffuse RGBA
  uint8_t blend = 0;         // BLEND_ENUM from macros.dta: Src/Add/SrcAlpha/...
  // Diffuse texcoord transform, mapped from the Mat's 12-float source texture
  // matrix and applied as [u v 1] * tex_xfm by 2-D UV renderers.
  // Row 2 carries offset; off-diagonal and negative scale are used by mirrored
  // UI tiles such as the pause-card border corners.
  float tex_xfm[3][3] = {{1.0f, 0.0f, 0.0f},
                         {0.0f, 1.0f, 0.0f},
                         {0.0f, 0.0f, 1.0f}};
  // Compatibility fields for older render paths and diagnostics.
  float tex_scale[2] = {1.0f, 1.0f};
  float tex_offset[2] = {0.0f, 0.0f};
  bool use_environ = false;
  bool prelit = false;
  // Source schema render state immediately after color. GH2 PS2 v27 does not
  // serialize alpha_threshold; later revisions do. The default threshold is 0,
  // matching MiloLib's default RndMat field when the serialized value is absent.
  bool has_render_state = false;
  bool point_lights = false;
  uint8_t z_mode = 1;
  bool alpha_cut = false;
  int32_t alpha_threshold = 0;
  bool alpha_write = false;
  uint8_t tex_gen = 0;
  uint8_t tex_wrap = 1;
  std::string next_pass;
  bool intensify = false;
  float emissive_multiplier = 1.0f;
  // Source schema: Mat.ng.cull. GH2 PS2 v27 stores this in the
  // post-diffuse render-state block; absent/undecoded means keep renderer
  // default culling.
  bool has_cull = false;
  bool cull = true;
  // Raw bytes preserved around the diffuse texture object ref so Mat.ng.cull
  // and neighboring render-state bytes can be audited against the in-repo
  // MiloEditor RndMat source order and observed GH2 PS2 material bodies.
  uint32_t diffuse_tex_offset = 0;
  std::vector<uint8_t> pre_diffuse_tex_bytes;
  std::vector<uint8_t> post_diffuse_tex_bytes;
  bool decoded = false;
};

struct SourceRndMatLoadPlan {
  int32_t revision = 0;
  bool reads_blend = true;
  bool reads_color = true;
  bool reads_modern_render_state = false;
  bool reads_use_environ = false;
  bool reads_prelit = false;
  bool reads_z_mode = false;
  bool reads_alpha_cut = false;
  bool reads_alpha_threshold = false;
  bool reads_alpha_write = false;
  bool reads_tex_gen = false;
  bool reads_tex_wrap = false;
  bool reads_tex_xfm = false;
  bool reads_diffuse_tex = false;
  bool reads_next_pass = false;
  bool reads_intensify = false;
  bool reads_cull = false;
  bool reads_emissive_multiplier = false;
  bool gh2_v27_has_no_alpha_threshold = false;
  std::vector<std::string> modern_order;
};

struct SourceRndMatDefaultState {
  float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  bool diffuse_tex_null = true;
  int32_t alpha_threshold = 0;
  bool next_pass_null = true;
  bool emissive_map_null = true;
  float refract_strength = 0.0f;
  bool refract_normal_map_null = true;
  bool intensify = false;
  bool use_environ = true;
  bool prelit = false;
  bool alpha_cut = false;
  bool alpha_write = false;
  bool cull = true;
  bool per_pixel_lit = false;
  bool screen_aligned = false;
  bool refract_enabled = false;
  bool point_lights = false;
  bool fog = false;
  bool fadeout = false;
  bool color_adjust = false;
  int32_t blend = 1;
  int32_t tex_gen = 0;
  int32_t tex_wrap = 1;
  int32_t z_mode = 1;
  int32_t stencil_mode = 0;
  int32_t shader_variation = 0;
  int32_t dirty = 3;
  float emissive_multiplier = 1.0f;
  bool tex_xfm_reset = true;
  int32_t color_mod_count = 3;
};

struct SourceRndMatSavePlan {
  int32_t save_id = 159;
};

struct SourceMiloEditorRndMatNewPlan {
  int32_t revision = 0;
  int32_t alt_revision = 0;
  bool sets_revision = true;
  bool sets_alt_revision = true;
  bool relies_on_constructor_defaults = true;
  bool does_not_initialize_render_state_or_textures = true;
};

SourceMiloEditorRndMatNewPlan source_milo_editor_rndmat_new_plan(
    int32_t revision,
    int32_t alt_revision);

struct SourceMatShaderOptionsDefaultState {
  bool temp_mat = false;
  uint32_t pack = 0x12;
};

struct SourceMatPerfSettingsDefaultState {
  bool recv_proj_lights = false;
  bool recv_point_cube_tex = false;
  bool ps3_force_trilinear = false;
};

struct SourceMatPerfSettingsLoadPlan {
  int32_t revision = 0;
  bool reads_recv_proj_lights = true;
  bool reads_ps3_force_trilinear = true;
  bool reads_recv_point_cube_tex = false;
  std::vector<std::string> read_order;
};

struct SourceRndMatAccessorResult {
  uint8_t blend = 1;
  uint8_t z_mode = 1;
  uint8_t tex_wrap = 1;
  std::string diffuse_tex;
  std::string next_pass;
  float alpha = 1.0f;
};

struct SourceRndMatSetterPlan {
  std::string setter;
  bool writes_member = false;
  bool writes_rgb_only = false;
  bool writes_alpha_only = false;
  int32_t dirty_or_mask = 0;
};

struct SourceRndMatColorModPlan {
  int32_t index = 0;
  bool assertion_would_fail = false;
  bool writes_color_mod = false;
  int32_t dirty_or_mask = 0;
};

struct SourceRndMatRefractEnabledPlan {
  bool refract_enabled = false;
  float refract_strength = 0.0f;
  bool has_refract_normal_map = false;
  bool allow_without_current_frame_tex = false;
  bool has_current_frame_tex = false;
  bool base_gate = false;
  bool frame_gate = false;
  bool result = false;
};

struct SourceRndMatRefractAccessorPlan {
  bool returns_normal_map = true;
  bool returns_strength = true;
};

struct SourceRndMatIsNextPassPlan {
  std::string candidate;
  std::vector<std::string> chain;
  bool walks_next_pass_chain = true;
  bool found = false;
};

struct SourceRndMatAllowedNextPassPlan {
  int32_t mat_count = 0;
  int32_t allocated_node_count = 0;
  bool node0_is_null = true;
  bool preserves_current_next_pass = false;
  bool excludes_recursive_next_passes = true;
  int32_t resized_node_count = 0;
  std::vector<std::string> allowed_order;
};

struct SourceRndMatAllowedNormalMapPlan {
  bool uses_directory = true;
  bool calls_get_normal_map_textures = true;
  bool returns_data_node = true;
};

struct SourceRndMatHandlerPlan {
  std::vector<std::string> handlers;
  std::vector<std::string> superclasses;
  int32_t check = 0x305;
};

struct SourceRndMatCopyPlan {
  bool copy_from_max = false;
  bool asserts_source_mat = true;
  bool copies_object_superclass = true;
  bool copies_diffuse_tex = false;
  bool copies_other_material_members = false;
  int32_t dirty_value = 3;
};

struct SourceRndMatPropSyncPlan {
  std::vector<std::string> dirty_color_rows;
  std::vector<std::string> dirty_render_rows;
  std::vector<std::string> direct_no_dirty_rows;
  std::vector<std::string> perf_setting_rows;
  int32_t color_dirty_or_mask = 1;
  int32_t render_dirty_or_mask = 2;
  bool custom_bit_rows_skip_size_or_get_dirty = true;
};

SourceRndMatLoadPlan source_rndmat_load_plan(int32_t revision);
SourceRndMatDefaultState source_rndmat_default_state();
SourceRndMatSavePlan source_rndmat_save_plan();
SourceMatShaderOptionsDefaultState source_mat_shader_options_default_state();
SourceMatPerfSettingsDefaultState source_mat_perf_settings_default_state();
SourceMatPerfSettingsLoadPlan source_mat_perf_settings_load_plan(
    int32_t revision);
SourceRndMatAccessorResult source_rndmat_accessors(const MatObj& mat);
SourceRndMatSetterPlan source_rndmat_setter_plan(const std::string& setter);
SourceRndMatColorModPlan source_rndmat_set_color_mod_plan(int32_t index);
SourceRndMatRefractEnabledPlan source_rndmat_get_refract_enabled_plan(
    bool refract_enabled,
    float refract_strength,
    bool has_refract_normal_map,
    bool allow_without_current_frame_tex,
    bool has_current_frame_tex);
SourceRndMatRefractAccessorPlan source_rndmat_refract_accessor_plan();
SourceRndMatIsNextPassPlan source_rndmat_is_next_pass_plan(
    const std::vector<std::string>& next_pass_chain,
    const std::string& candidate);
SourceRndMatAllowedNextPassPlan source_rndmat_allowed_next_pass_plan(
    const std::vector<std::string>& directory_mats,
    const std::string& current_next_pass,
    const std::vector<std::string>& recursive_next_passes);
SourceRndMatAllowedNormalMapPlan source_rndmat_allowed_normal_map_plan();
SourceRndMatHandlerPlan source_rndmat_handler_plan();
SourceRndMatCopyPlan source_rndmat_copy_plan(bool copy_from_max);
SourceRndMatPropSyncPlan source_rndmat_prop_sync_plan();

struct Vertex {
  float px, py, pz;          // position
  float nx, ny, nz;          // normal
  float r, g, b, a;          // runtime diffuse tint; source slot is weights
  float u, v;                // texture coords
};
static_assert(sizeof(Vertex) == 48, "GH2 PS2 mesh vertex stride must be 48 bytes");

struct MeshObj {
  std::string name;          // entry name (e.g. "green_gem.mesh")
  std::string parent;        // Trans parent name (links into the parent chain)
  std::string material;      // Mat entry name this mesh draws with
  std::string geometry_owner;// Mesh entry that owns reusable geometry.
  Xfm local;                 // the mesh's own Trans local matrix
  Xfm world_stored;          // the stored Trans world matrix from the MILO
  uint32_t constraint = 0;    // RndTransformable::Constraint
  std::string target;
  bool preserve_scale = false;
  uint32_t mutable_flags = 0;  // RndMesh::mMutable; MeshAnim needs low bits.
  uint32_t vertex_count = 0;
  uint32_t face_count = 0;
  std::vector<Vertex> verts;
  std::vector<uint16_t> indices;  // face_count*3 indices
  // Bounding box (object-space), filled after decode.
  float bb_min[3] = {0, 0, 0};
  float bb_max[3] = {0, 0, 0};
  bool showing = true;
  float draw_order = 0.0f;
  size_t dir_index = 0;
  bool decoded = false;
  std::string error;         // non-empty if decode failed (mesh still listed)
};

struct ParticleSysObj {
  std::string name;
  uint16_t revision = 0;
  uint16_t anim_revision = 0;
  uint16_t trans_revision = 0;
  uint16_t draw_revision = 0;
  std::string parent;
  std::string material;
  Xfm local;
  Xfm world_stored;
  uint32_t constraint = 0;
  std::string target;
  bool preserve_scale = false;
  bool showing = true;
  float draw_order = 0.0f;
  size_t dir_index = 0;
  uint32_t max_particles = 0;
  float velocity_min[3] = {0.0f, 0.0f, 0.0f};
  float velocity_max[3] = {0.0f, 0.0f, 0.0f};
  float life_min_frames = 30.0f;
  float life_max_frames = 30.0f;
  float box_extent_min[3] = {0.0f, 0.0f, 0.0f};
  float box_extent_max[3] = {0.0f, 0.0f, 0.0f};
  float speed_min = 0.0f;
  float speed_max = 0.0f;
  float pitch_min = 0.0f;
  float pitch_max = 0.0f;
  float yaw_min = 0.0f;
  float yaw_max = 0.0f;
  float emit_rate_min = 1.0f;
  float emit_rate_max = 1.0f;
  float start_size_min = 1.0f;
  float start_size_max = 1.0f;
  float delta_size_min = 0.0f;
  float delta_size_max = 0.0f;
  float lifetime_min = 1.0f;
  float lifetime_max = 1.0f;
  float size_start = 1.0f;
  float size_end = 1.0f;
  std::array<float, 4> start_color_low = {1.0f, 1.0f, 1.0f, 1.0f};
  std::array<float, 4> start_color_high = {1.0f, 1.0f, 1.0f, 1.0f};
  std::array<float, 4> end_color_low = {1.0f, 1.0f, 1.0f, 1.0f};
  std::array<float, 4> end_color_high = {1.0f, 1.0f, 1.0f, 1.0f};
  std::string bounce;
  float force_dir[3] = {0.0f, 0.0f, 0.0f};
  uint32_t particle_flags = 0;
  float grow_ratio = 0.0f;
  float shrink_ratio = 1.0f;
  float mid_color_ratio = 0.0f;
  std::array<float, 4> mid_color_low = {1.0f, 1.0f, 1.0f, 1.0f};
  std::array<float, 4> mid_color_high = {1.0f, 1.0f, 1.0f, 1.0f};
  float bubble_period_min = 10.0f;
  float bubble_period_max = 10.0f;
  float bubble_size_min = 1.0f;
  float bubble_size_max = 1.0f;
  bool bubble = false;
  float relative_motion = 0.0f;
  std::string relative_parent;
  std::string emitter_mesh;
  bool preserve_particles = false;
  uint32_t preserved_particle_count = 0;
  uint32_t preserved_particle_stride_bytes = 0;
  bool source_order_decoded = false;
  bool decoded = false;
  std::string error;
};

struct WorldCrowdActor {
  std::string name;
  float params[3] = {0.0f, 0.0f, 0.0f};
};

struct WorldCrowdPlacementSet {
  std::string actor_name;
  std::vector<Xfm> placements;
};

struct WorldCrowdObj {
  std::string name;
  std::string area_mesh;
  uint32_t total_placements = 0;  // ihatecompvir WorldCrowd::mNum.
  uint32_t decoded_placement_count = 0;
  std::vector<WorldCrowdActor> actors;
  std::vector<WorldCrowdPlacementSet> placement_sets;
  bool decoded = false;
  std::string error;
};

// Decode one entry body (raw bytes = payload.data()+entry.offset, entry.size).
// `entry_name` is the MILO entry name. Throws std::runtime_error on malformed
// input; the scene loader catches per-entry so one bad object never aborts.
TransObj decode_trans(const std::string& entry_name,
                      const std::vector<uint8_t>& body,
                      int32_t parent_dir_revision = 24);
CamObj decode_cam(const std::string& entry_name,
                  const std::vector<uint8_t>& body);
WaypointObj decode_waypoint(const std::string& entry_name,
                             const std::vector<uint8_t>& body);
SpotlightObj decode_spotlight(const std::string& entry_name,
                              const std::vector<uint8_t>& body);
LightObj decode_light(const std::string& entry_name,
                      const std::vector<uint8_t>& body);
EnvironObj decode_environ(const std::string& entry_name,
                          const std::vector<uint8_t>& body);
MatObj decode_mat(const std::string& entry_name,
                  const std::vector<uint8_t>& body);
GroupObj decode_group(const std::string& entry_name,
                      const std::vector<uint8_t>& body,
                      int32_t parent_dir_revision = 24);
BandPlacerObj decode_band_placer(const std::string& entry_name,
                                 const std::vector<uint8_t>& body);
// Mesh decode never throws — on failure it returns a MeshObj with decoded=false
// and a populated .error, so the `mesh` subcommand can report it.
MeshObj decode_mesh(const std::string& entry_name,
                    const std::vector<uint8_t>& body,
                    int32_t parent_dir_revision = 24);
ParticleSysObj decode_particle_sys(const std::string& entry_name,
                                   const std::vector<uint8_t>& body);
WorldCrowdObj decode_world_crowd(const std::string& entry_name,
                                 const std::vector<uint8_t>& body);

// A whole decoded scene: every Trans/Mat/Mesh in one MILO, plus the texture
// names referenced by materials (so the caller can batch-load them).
struct Scene {
  std::vector<MeshObj> meshes;
  std::vector<TransObj> transes;
  std::vector<MatObj> mats;
  std::vector<CamObj> cams;
  std::vector<WaypointObj> waypoints;
  std::vector<SpotlightObj> spotlights;
  std::vector<LightObj> lights;
  std::vector<EnvironObj> environs;
  std::vector<GroupObj> groups;
  std::vector<BandPlacerObj> band_placers;
  std::vector<ParticleSysObj> particles;
  std::vector<WorldCrowdObj> world_crowds;
  std::vector<std::string> draw_order;  // Group-authored Mesh child order.
  std::vector<std::string> grouped_meshes;  // Meshes referenced by any Group.
  std::string dir_name;
  std::string dir_type;

  // Resolve a mesh's full world matrix by composing its local matrix up the
  // parent chain (parents resolved by name among transes + meshes in this
  // scene). Returns a 4x4 row-major matrix flattened to 16 floats, in the
  // same convention as render::Mat4 (row vectors; translation in row 3).
  std::array<float, 16> world_matrix(const MeshObj& mesh) const;
  std::array<float, 16> world_matrix(const ParticleSysObj& particle) const;

  // Find a material by name (nullptr if absent).
  const MatObj* find_mat(const std::string& name) const;
  // Find a dynamic light by name (nullptr if absent or decode failed).
  const LightObj* find_light(const std::string& name) const;
  // Find an environment by name (nullptr if absent or decode failed).
  const EnvironObj* find_environ(const std::string& name) const;
  // Find an authored menu display placer by name (nullptr if absent or failed).
  const BandPlacerObj* find_band_placer(const std::string& name) const;
};

// Load + decode a MILO straight from a PS2 ARK (hdr/ark). Runtime-native: reads
// the .milo_ps2 bytes from the ARK and decodes in memory. Returns false (with a
// logged reason) if the MILO can't be read; partial decodes (some objects fail)
// still return true with those objects flagged.
bool load_scene(const std::string& hdr_path, const std::string& ark_path,
                const std::string& milo_path, Scene& out);

// Rebuild the RndDir-style group draw metadata after tests or diagnostics
// mutate a Scene by hand. load_scene calls this automatically.
void rebuild_group_authored_draw_order_for_test(Scene& scene);

}  // namespace ghogx::milo_scene
