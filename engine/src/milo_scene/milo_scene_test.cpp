// engine/src/milo_scene/milo_scene_test.cpp
//
// Hermetic unit tests for the MILO render-object decoders. We hand-build the
// exact GH2 PS2 byte layouts (no ARK / no I/O) so the decoder's field offsets
// are pinned by an in-repo oracle. Byte layouts mirror real entries decoded
// from track/gen/track.milo_ps2 (green_gem.mesh, gem.mat, track_fade.trans).

#include "milo_scene/milo_scene.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

using namespace ghogx::milo_scene;

// Assert that works regardless of NDEBUG (unit tests must check unconditionally).
#define CHECK(cond)                                                          \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::printf("  [FAIL] %s:%d  %s\n", __FILE__, __LINE__, #cond);        \
      std::exit(1);                                                          \
    }                                                                        \
  } while (0)

void put_u32(std::vector<uint8_t>& b, uint32_t v) {
  for (int i = 0; i < 4; ++i) b.push_back(static_cast<uint8_t>(v >> (i * 8)));
}
void put_f32(std::vector<uint8_t>& b, float f) {
  uint32_t v;
  std::memcpy(&v, &f, 4);
  put_u32(b, v);
}
void put_u16(std::vector<uint8_t>& b, uint16_t v) {
  b.push_back(static_cast<uint8_t>(v & 0xff));
  b.push_back(static_cast<uint8_t>(v >> 8));
}
void put_str(std::vector<uint8_t>& b, const std::string& s) {
  put_u32(b, static_cast<uint32_t>(s.size()));
  for (char c : s) b.push_back(static_cast<uint8_t>(c));
}
void put_zeros(std::vector<uint8_t>& b, size_t n) {
  for (size_t i = 0; i < n; ++i) b.push_back(0);
}
// Identity rotation + given translation, as a Harmonix 3x4 matrix.
void put_matrix(std::vector<uint8_t>& b, float tx, float ty, float tz) {
  put_f32(b, 1); put_f32(b, 0); put_f32(b, 0);
  put_f32(b, 0); put_f32(b, 1); put_f32(b, 0);
  put_f32(b, 0); put_f32(b, 0); put_f32(b, 1);
  put_f32(b, tx); put_f32(b, ty); put_f32(b, tz);
}

bool approx(float a, float b) { return std::fabs(a - b) < 1e-4f; }

void test_trans() {
  const SourceRndTransLoadPlan rev9_standalone =
      source_rndtrans_load_plan(9, 24, true);
  CHECK(rev9_standalone.standalone);
  CHECK(rev9_standalone.reads_object_fields);
  CHECK(rev9_standalone.reads_local_xfm);
  CHECK(rev9_standalone.reads_world_xfm);
  CHECK(!rev9_standalone.reads_old_child_list);
  CHECK(rev9_standalone.reads_constraint);
  CHECK(rev9_standalone.reads_target);
  CHECK(rev9_standalone.reads_preserve_scale);
  CHECK(rev9_standalone.reads_parent);

  const SourceRndTransLoadPlan rev9_embedded =
      source_rndtrans_load_plan(9, 24, false);
  CHECK(!rev9_embedded.standalone);
  CHECK(!rev9_embedded.reads_object_fields);
  CHECK(rev9_embedded.reads_constraint);

  const SourceRndTransLoadPlan rev8_old_parent =
      source_rndtrans_load_plan(8, 6, false);
  CHECK(rev8_old_parent.reads_old_child_list);
  CHECK(rev8_old_parent.old_child_list_is_null_terminated_strings);
  CHECK(!rev8_old_parent.old_child_list_is_symbols);
  CHECK(rev8_old_parent.reads_constraint);
  CHECK(rev8_old_parent.reads_target);
  CHECK(rev8_old_parent.reads_preserve_scale);

  const SourceRndTransLoadPlan rev8_new_parent =
      source_rndtrans_load_plan(8, 24, false);
  CHECK(rev8_new_parent.old_child_list_is_symbols);
  CHECK(!rev8_new_parent.old_child_list_is_null_terminated_strings);

  const SourceRndTransLoadPlan rev6 = source_rndtrans_load_plan(6, 24, false);
  CHECK(rev6.reads_target);
  CHECK(!rev6.reads_constraint);
  CHECK(!rev6.reads_preserve_scale);

  const SourceRndTransLoadPlan rev5 = source_rndtrans_load_plan(5, 24, false);
  CHECK(!rev5.reads_target);
  CHECK(!rev5.reads_constraint);
  CHECK(rev5.reads_parent);

  std::vector<uint8_t> b;
  put_u32(b, 9);                 // version
  put_zeros(b, 9);               // base metadata
  put_matrix(b, 0, 120.0f, 0);   // local matrix (ty=120, like track_fade.trans)
  put_matrix(b, 0, 120.0f, 0);   // world matrix (identical)
  put_zeros(b, 9);               // constraint/flags
  put_str(b, "track_surface5.view");

  TransObj t = decode_trans("track_fade.trans", b);
  CHECK(t.name == "track_fade.trans");
  CHECK(t.parent == "track_surface5.view");
  CHECK(approx(t.local.pos[1], 120.0f));
  CHECK(approx(t.local.rot[0][0], 1.0f) && approx(t.local.rot[2][2], 1.0f));
  std::printf("  [ok] Trans: parent=%s pos.y=%.1f\n", t.parent.c_str(),
              t.local.pos[1]);
}

void test_trans_proxy() {
  const SourceRndTransProxyDefaultState defaults =
      source_rndtrans_proxy_default_state();
  CHECK(defaults.proxy_null);
  CHECK(defaults.part_null);

  const SourceRndTransProxyLoadPlan load_v0 =
      source_rndtrans_proxy_load_plan(0);
  CHECK(load_v0.accepted_revision);
  CHECK(load_v0.reads_object_fields);
  CHECK(!load_v0.reads_transformable);
  CHECK(load_v0.reads_proxy);
  CHECK(load_v0.reads_part);
  CHECK(load_v0.calls_sync);

  const SourceRndTransProxyLoadPlan load_v1 =
      source_rndtrans_proxy_load_plan(1);
  CHECK(load_v1.accepted_revision);
  CHECK(load_v1.reads_transformable);
  CHECK(!source_rndtrans_proxy_load_plan(2).accepted_revision);

  const SourceRndTransProxySyncPlan direct_proxy =
      source_rndtrans_proxy_sync_plan(true, true, true, false);
  CHECK(direct_proxy.clears_parent_first);
  CHECK(direct_proxy.attempts_direct_proxy_parent);
  CHECK(direct_proxy.uses_direct_proxy_parent);
  CHECK(!direct_proxy.attempts_part_lookup);
  CHECK(!direct_proxy.clears_parent_final);
  CHECK(direct_proxy.resolved_parent_source == "proxy");

  const SourceRndTransProxySyncPlan part_lookup =
      source_rndtrans_proxy_sync_plan(true, false, false, true);
  CHECK(!part_lookup.attempts_direct_proxy_parent);
  CHECK(part_lookup.attempts_part_lookup);
  CHECK(part_lookup.uses_part_lookup_parent);
  CHECK(!part_lookup.clears_parent_final);
  CHECK(part_lookup.resolved_parent_source == "part");

  const SourceRndTransProxySyncPlan null_part_fallback =
      source_rndtrans_proxy_sync_plan(true, true, false, true);
  CHECK(null_part_fallback.attempts_direct_proxy_parent);
  CHECK(!null_part_fallback.uses_direct_proxy_parent);
  CHECK(null_part_fallback.attempts_part_lookup);
  CHECK(null_part_fallback.uses_part_lookup_parent);
  CHECK(null_part_fallback.resolved_parent_source == "part");

  const SourceRndTransProxySyncPlan no_match =
      source_rndtrans_proxy_sync_plan(true, false, false, false);
  CHECK(no_match.attempts_part_lookup);
  CHECK(!no_match.uses_part_lookup_parent);
  CHECK(no_match.clears_parent_final);
  CHECK(no_match.resolved_parent_source == "none");

  const SourceRndTransProxySyncPlan no_proxy =
      source_rndtrans_proxy_sync_plan(false, false, false, true);
  CHECK(!no_proxy.attempts_direct_proxy_parent);
  CHECK(!no_proxy.attempts_part_lookup);
  CHECK(no_proxy.clears_parent_final);

  const SourceRndTransProxySetterPlan changed =
      source_rndtrans_proxy_setter_plan(true);
  CHECK(changed.value_changed);
  CHECK(changed.assigns_value);
  CHECK(changed.calls_sync);
  const SourceRndTransProxySetterPlan unchanged =
      source_rndtrans_proxy_setter_plan(false);
  CHECK(!unchanged.assigns_value);
  CHECK(!unchanged.calls_sync);

  const SourceRndTransProxySavePlan save_plan =
      source_rndtrans_proxy_save_plan();
  CHECK(save_plan.presave_clears_parent);
  CHECK(save_plan.postsave_calls_sync);

  const SourceRndTransProxyCopyPlan copy_plan =
      source_rndtrans_proxy_copy_plan();
  CHECK(copy_plan.superclasses.size() == 2);
  CHECK(copy_plan.superclasses[0] == "Hmx::Object");
  CHECK(copy_plan.superclasses[1] == "RndTransformable");
  CHECK(copy_plan.member_order.size() == 2);
  CHECK(copy_plan.member_order[0] == "mProxy");
  CHECK(copy_plan.member_order[1] == "mPart");
  CHECK(copy_plan.calls_sync);

  const SourceRndTransProxyHandlerPlan handlers =
      source_rndtrans_proxy_handler_plan();
  CHECK(handlers.superclasses.size() == 2);
  CHECK(handlers.superclasses[0] == "RndTransformable");
  CHECK(handlers.superclasses[1] == "Hmx::Object");
  CHECK(handlers.check == 0x6A);

  const SourceRndTransProxyPropSyncPlan props =
      source_rndtrans_proxy_prop_sync_plan();
  CHECK(props.props.size() == 2);
  CHECK(props.props[0] == "proxy:Sync");
  CHECK(props.props[1] == "part:Sync");
  CHECK(props.superclasses.size() == 1);
  CHECK(props.superclasses[0] == "RndTransformable");

  std::printf("  [ok] TransProxy: sync=%s/%s\n",
              direct_proxy.resolved_parent_source.c_str(),
              part_lookup.resolved_parent_source.c_str());
}

void test_trans_anim() {
  const SourceRndTransAnimDefaultState defaults =
      source_rndtrans_anim_default_state();
  CHECK(defaults.trans_null);
  CHECK(!defaults.trans_spline);
  CHECK(!defaults.scale_spline);
  CHECK(!defaults.rot_slerp);
  CHECK(!defaults.rot_spline);
  CHECK(defaults.keys_owner_self);
  CHECK(!defaults.repeat_trans);
  CHECK(!defaults.follow_path);

  const SourceRndTransAnimLoadPlan load_v7 =
      source_rndtrans_anim_load_plan(7);
  CHECK(load_v7.accepted_revision);
  CHECK(load_v7.reads_object_fields);
  CHECK(load_v7.reads_animatable);
  CHECK(!load_v7.dumps_drawable);
  CHECK(load_v7.reads_trans);
  CHECK(load_v7.reads_rot_and_trans_keys);
  CHECK(!load_v7.reads_scale_keys);
  CHECK(load_v7.reads_keys_owner);
  CHECK(load_v7.null_keys_owner_defaults_to_self);
  CHECK(!load_v7.reads_legacy_int);
  CHECK(load_v7.reads_follow_path);
  CHECK(load_v7.reads_rot_slerp);
  CHECK(load_v7.reads_rot_spline);

  const SourceRndTransAnimLoadPlan load_v2 =
      source_rndtrans_anim_load_plan(2);
  CHECK(load_v2.accepted_revision);
  CHECK(!load_v2.reads_object_fields);
  CHECK(load_v2.dumps_drawable);
  CHECK(!load_v2.reads_rot_and_trans_keys);
  CHECK(load_v2.reads_legacy_int);
  CHECK(load_v2.reads_follow_path);
  CHECK(!load_v2.follow_path_from_keys_owner);
  CHECK(!load_v2.reads_rot_slerp);

  const SourceRndTransAnimLoadPlan load_v1 =
      source_rndtrans_anim_load_plan(1);
  CHECK(load_v1.reads_rot_and_trans_keys);
  CHECK(load_v1.reads_legacy_int);
  CHECK(!load_v1.reads_follow_path);
  CHECK(load_v1.follow_path_from_keys_owner);
  CHECK(!source_rndtrans_anim_load_plan(8).accepted_revision);

  const SourceRndTransAnimSetKeysOwnerPlan set_owner =
      source_rndtrans_anim_set_keys_owner_plan();
  CHECK(set_owner.asserts_non_null);
  CHECK(set_owner.assigns_keys_owner);

  const SourceRndTransAnimReplacePlan replace_null =
      source_rndtrans_anim_replace_plan(true, true);
  CHECK(replace_null.calls_object_replace);
  CHECK(replace_null.keys_owner_matches_from);
  CHECK(replace_null.replacement_null);
  CHECK(replace_null.assigns_self);
  CHECK(!replace_null.copies_replacement_keys_owner);

  const SourceRndTransAnimReplacePlan replace_to =
      source_rndtrans_anim_replace_plan(true, false);
  CHECK(!replace_to.assigns_self);
  CHECK(replace_to.copies_replacement_keys_owner);

  const SourceRndTransAnimReplacePlan replace_miss =
      source_rndtrans_anim_replace_plan(false, false);
  CHECK(replace_miss.calls_object_replace);
  CHECK(!replace_miss.assigns_self);
  CHECK(!replace_miss.copies_replacement_keys_owner);

  const SourceRndTransAnimCopyPlan copy_shallow =
      source_rndtrans_anim_copy_plan(true, false, true);
  CHECK(copy_shallow.superclasses.size() == 2);
  CHECK(copy_shallow.superclasses[0] == "Hmx::Object");
  CHECK(copy_shallow.superclasses[1] == "RndAnimatable");
  CHECK(copy_shallow.copies_trans);
  CHECK(copy_shallow.copies_keys_owner_ref);
  CHECK(!copy_shallow.assigns_self_as_keys_owner);
  CHECK(copy_shallow.copied_owned_members.empty());

  const SourceRndTransAnimCopyPlan copy_owned =
      source_rndtrans_anim_copy_plan(false, false, true);
  CHECK(!copy_owned.copies_keys_owner_ref);
  CHECK(copy_owned.assigns_self_as_keys_owner);
  CHECK(copy_owned.copied_owned_members.size() == 9);
  CHECK(copy_owned.copied_owned_members[0] == "mTransKeys");
  CHECK(copy_owned.copied_owned_members[8] == "mRotSpline");

  const SourceRndTransAnimCopyPlan copy_from_max_external =
      source_rndtrans_anim_copy_plan(false, true, false);
  CHECK(copy_from_max_external.copies_keys_owner_ref);

  const SourceRndTransAnimFramePlan frame_with_trans =
      source_rndtrans_anim_set_frame_plan(true);
  CHECK(frame_with_trans.calls_animatable_set_frame);
  CHECK(frame_with_trans.copies_local_transform);
  CHECK(frame_with_trans.calls_make_transform);
  CHECK(frame_with_trans.writes_local_transform);
  CHECK(frame_with_trans.make_transform_assert_body_only);

  const SourceRndTransAnimFramePlan frame_without_trans =
      source_rndtrans_anim_set_frame_plan(false);
  CHECK(frame_without_trans.calls_animatable_set_frame);
  CHECK(!frame_without_trans.calls_make_transform);
  CHECK(frame_without_trans.make_transform_assert_body_only);

  const SourceRndTransAnimSetKeyPlan set_key =
      source_rndtrans_anim_set_key_plan(true);
  CHECK(set_key.operations.size() == 4);
  CHECK(set_key.operations[0] == "add_trans_key_from_local_translation");
  CHECK(set_key.operations[3] == "add_scale_key_from_local_matrix");
  CHECK(source_rndtrans_anim_set_key_plan(false).operations.empty());

  const SourceRndTransAnimHandlerPlan handlers =
      source_rndtrans_anim_handler_plan();
  CHECK(handlers.handlers.size() == 15);
  CHECK(handlers.handlers[0] == "trans");
  CHECK(handlers.handlers[14] == "set_rot_slerp");
  CHECK(handlers.superclasses.size() == 2);
  CHECK(handlers.superclasses[0] == "RndAnimatable");
  CHECK(handlers.superclasses[1] == "Hmx::Object");
  CHECK(handlers.check == 489);

  const SourceRndTransAnimPropSyncPlan props =
      source_rndtrans_anim_prop_sync_plan();
  CHECK(props.props.size() == 1);
  CHECK(props.props[0] == "keys_owner:SetKeysOwner");
  CHECK(props.superclasses.size() == 1);
  CHECK(props.superclasses[0] == "RndAnimatable");

  std::printf("  [ok] TransAnim: load_v7=%d handlers=%zu\n",
              load_v7.accepted_revision ? 1 : 0, handlers.handlers.size());
}

void test_mesh_anim() {
  const SourceRndMeshAnimDefaultState defaults =
      source_rndmeshanim_default_state();
  CHECK(defaults.mesh_null);
  CHECK(defaults.keys_owner_self);

  const SourceRndMeshAnimNumVertsPlan num_verts =
      source_rndmeshanim_num_verts_plan(0, 6, 4, 9);
  CHECK(num_verts.result == 9);
  CHECK(num_verts.nonempty_sources.size() == 3);
  CHECK(num_verts.nonempty_sources[0] == "normals");
  CHECK(num_verts.nonempty_sources[2] == "colors");

  const SourceRndMeshAnimReplacePlan replace_null =
      source_rndmeshanim_replace_plan(true, true);
  CHECK(replace_null.calls_object_replace);
  CHECK(replace_null.assigns_self);
  CHECK(!replace_null.copies_replacement_keys_owner);
  const SourceRndMeshAnimReplacePlan replace_other =
      source_rndmeshanim_replace_plan(true, false);
  CHECK(!replace_other.assigns_self);
  CHECK(replace_other.copies_replacement_keys_owner);
  const SourceRndMeshAnimReplacePlan replace_miss =
      source_rndmeshanim_replace_plan(false, false);
  CHECK(!replace_miss.assigns_self);
  CHECK(!replace_miss.copies_replacement_keys_owner);

  const SourceRndMeshAnimLoadPlan rev0 = source_rndmeshanim_load_plan(0);
  CHECK(rev0.accepted_revision);
  CHECK(!rev0.reads_object_fields);
  CHECK(rev0.reads_animatable);
  CHECK(rev0.reads_mesh);
  CHECK(rev0.reads_vert_points_keys);
  CHECK(!rev0.reads_vert_normals_keys);
  CHECK(rev0.reads_vert_texs_keys);
  CHECK(rev0.reads_vert_colors_keys);
  CHECK(rev0.reads_keys_owner);
  CHECK(rev0.null_keys_owner_defaults_to_self);

  const SourceRndMeshAnimLoadPlan rev2 = source_rndmeshanim_load_plan(2);
  CHECK(rev2.accepted_revision);
  CHECK(rev2.reads_object_fields);
  CHECK(rev2.reads_vert_normals_keys);
  CHECK(!source_rndmeshanim_load_plan(3).accepted_revision);

  const SourceRndMeshAnimCopyPlan shallow_copy =
      source_rndmeshanim_copy_plan(true, false, true);
  CHECK(shallow_copy.superclasses.size() == 2);
  CHECK(shallow_copy.superclasses[0] == "Hmx::Object");
  CHECK(shallow_copy.superclasses[1] == "RndAnimatable");
  CHECK(shallow_copy.copies_mesh);
  CHECK(shallow_copy.copies_keys_owner_ref);
  CHECK(!shallow_copy.assigns_self_as_keys_owner);

  const SourceRndMeshAnimCopyPlan max_external =
      source_rndmeshanim_copy_plan(false, true, false);
  CHECK(max_external.copies_keys_owner_ref);
  const SourceRndMeshAnimCopyPlan owned_copy =
      source_rndmeshanim_copy_plan(false, false, true);
  CHECK(!owned_copy.copies_keys_owner_ref);
  CHECK(owned_copy.assigns_self_as_keys_owner);
  CHECK(owned_copy.copied_owned_members.size() == 4);
  CHECK(owned_copy.copied_owned_members[0] == "mVertPointsKeys");
  CHECK(owned_copy.copied_owned_members[3] == "mVertColorsKeys");

  const SourceRndMeshAnimEndFramePlan end_frame =
      source_rndmeshanim_end_frame_plan(2.0f, 6.0f, 4.0f, 3.0f);
  CHECK(approx(end_frame.result, 6.0f));

  const SourceRndMeshAnimInterpPlan interp_prev =
      source_rndmeshanim_interp_plan(0.0f, 1.0f, 8, 5);
  CHECK(interp_prev.affected_verts == 5);
  CHECK(interp_prev.uses_first_key);
  CHECK(!interp_prev.uses_second_key);
  CHECK(!interp_prev.interpolates_between_keys);
  CHECK(!interp_prev.blends_with_existing_vert);
  const SourceRndMeshAnimInterpPlan interp_mid =
      source_rndmeshanim_interp_plan(0.25f, 0.5f, 3, 9);
  CHECK(interp_mid.affected_verts == 3);
  CHECK(interp_mid.uses_first_key);
  CHECK(interp_mid.uses_second_key);
  CHECK(interp_mid.interpolates_between_keys);
  CHECK(interp_mid.blends_with_existing_vert);

  const SourceRndMeshAnimSetFramePlan no_mesh =
      source_rndmeshanim_set_frame_plan(false, 0x1F, true, true, true, true);
  CHECK(no_mesh.calls_animatable_set_frame);
  CHECK(!no_mesh.evaluates_points);
  CHECK(!no_mesh.calls_mesh_sync);
  const SourceRndMeshAnimSetFramePlan immutable =
      source_rndmeshanim_set_frame_plan(true, 0, true, true, true, true);
  CHECK(immutable.notifies_not_mutable);
  CHECK(!immutable.evaluates_points);
  CHECK(!immutable.calls_mesh_sync);
  const SourceRndMeshAnimSetFramePlan mutable_mesh =
      source_rndmeshanim_set_frame_plan(true, 0x04, true, false, true, false);
  CHECK(mutable_mesh.mesh_mutable);
  CHECK(mutable_mesh.evaluates_points);
  CHECK(!mutable_mesh.evaluates_normals);
  CHECK(mutable_mesh.evaluates_texs);
  CHECK(mutable_mesh.sync_mask == 0x1F);
  CHECK(mutable_mesh.calls_mesh_sync);

  const SourceRndMeshAnimSetKeyPlan set_key =
      source_rndmeshanim_set_key_plan();
  CHECK(set_key.body_empty);

  const SourceRndMeshAnimShrinkPlan shrink_verts =
      source_rndmeshanim_shrink_verts_plan(12, true, false, true, true);
  CHECK(shrink_verts.requested_count == 12);
  CHECK(shrink_verts.resized_streams.size() == 3);
  CHECK(shrink_verts.resized_streams[0] == "points_values");
  CHECK(shrink_verts.resized_streams[2] == "colors_values");
  const SourceRndMeshAnimShrinkPlan shrink_keys =
      source_rndmeshanim_shrink_keys_plan(2, false, true, false, true);
  CHECK(shrink_keys.resized_streams.size() == 2);
  CHECK(shrink_keys.resized_streams[0] == "normals_keys");
  CHECK(shrink_keys.resized_streams[1] == "colors_keys");

  const SourceRndMeshAnimHandlerPlan handlers =
      source_rndmeshanim_handler_plan();
  CHECK(handlers.expressions.size() == 1);
  CHECK(handlers.expressions[0] == "num_verts");
  CHECK(handlers.actions.size() == 2);
  CHECK(handlers.actions[0] == "shrink_verts");
  CHECK(handlers.actions[1] == "shrink_keys");
  CHECK(handlers.superclasses.size() == 2);
  CHECK(handlers.check == 0x207);

  const SourceRndMeshAnimPropSyncPlan props =
      source_rndmeshanim_prop_sync_plan();
  CHECK(props.props.size() == 1);
  CHECK(props.props[0] == "mesh");
  CHECK(props.superclasses.size() == 1);
  CHECK(props.superclasses[0] == "RndAnimatable");

  std::printf("  [ok] MeshAnim: numVerts=%d sync=0x%x actions=%zu\n",
              num_verts.result, mutable_mesh.sync_mask,
              handlers.actions.size());
}

void test_mat() {
  const SourceRndMatLoadPlan v27_plan = source_rndmat_load_plan(27);
  CHECK(v27_plan.reads_blend);
  CHECK(v27_plan.reads_color);
  CHECK(v27_plan.reads_modern_render_state);
  CHECK(v27_plan.reads_use_environ);
  CHECK(v27_plan.reads_prelit);
  CHECK(v27_plan.reads_z_mode);
  CHECK(v27_plan.reads_alpha_cut);
  CHECK(!v27_plan.reads_alpha_threshold);
  CHECK(v27_plan.reads_alpha_write);
  CHECK(v27_plan.reads_tex_gen);
  CHECK(v27_plan.reads_tex_wrap);
  CHECK(v27_plan.reads_tex_xfm);
  CHECK(v27_plan.reads_diffuse_tex);
  CHECK(v27_plan.reads_next_pass);
  CHECK(v27_plan.reads_intensify);
  CHECK(v27_plan.reads_cull);
  CHECK(v27_plan.reads_emissive_multiplier);
  CHECK(v27_plan.gh2_v27_has_no_alpha_threshold);
  CHECK(v27_plan.modern_order.size() == 15);
  CHECK(v27_plan.modern_order[0] == "blend");
  CHECK(v27_plan.modern_order[5] == "alpha_cut");
  CHECK(v27_plan.modern_order[6] == "alpha_write");
  CHECK(v27_plan.modern_order[10] == "diffuse_tex");
  CHECK(v27_plan.modern_order[13] == "cull");

  const SourceRndMatLoadPlan v38_plan = source_rndmat_load_plan(38);
  CHECK(v38_plan.reads_alpha_threshold);
  CHECK(!v38_plan.gh2_v27_has_no_alpha_threshold);
  CHECK(v38_plan.modern_order.size() == 16);
  CHECK(v38_plan.modern_order[6] == "alpha_threshold");
  CHECK(v38_plan.modern_order[7] == "alpha_write");

  const SourceRndMatLoadPlan v21_plan = source_rndmat_load_plan(21);
  CHECK(!v21_plan.reads_modern_render_state);
  CHECK(v21_plan.modern_order.empty());

  std::vector<uint8_t> b;
  put_u32(b, 27);                // version 0x1b
  put_zeros(b, 9);               // base metadata
  put_u32(b, 4);                 // kBlendSrcAlphaAdd
  put_f32(b, 1); put_f32(b, 1); put_f32(b, 1); put_f32(b, 1);  // colour RGBA
  b.push_back(1);                // use_environ
  b.push_back(0);                // prelit
  put_u32(b, 2);                 // z_mode = kZModeTransparent
  b.push_back(1);                // alpha_cut
  b.push_back(0);                // alpha_write (GH2 v27 has no threshold field)
  put_u32(b, 0);                 // tex_gen = kTexGenNone
  put_u32(b, 4);                 // tex_wrap = kTexWrapMirror
  put_f32(b, 2.0f); put_f32(b, 0.0f); put_f32(b, 0.0f);
  put_f32(b, 0.0f); put_f32(b, 3.0f); put_f32(b, 0.0f);
  put_f32(b, 0.25f); put_f32(b, 0.5f); put_f32(b, 1.0f);
  put_str(b, "gem.tex");         // diffuse texture
  put_u32(b, 0);                 // empty next_pass ref
  b.push_back(0);                // trailing state byte before ng.cull
  b.push_back(0);                // Mat.ng.cull = false / two-sided
  put_f32(b, 1.0f);              // emissive_multiplier at observed +6
  put_zeros(b, 38);

  MatObj m = decode_mat("gem.mat", b);
  CHECK(m.decoded);
  CHECK(m.diffuse_tex == "gem.tex");
  CHECK(approx(m.color[0], 1.0f));
  CHECK(m.blend == 4);
  CHECK(m.use_environ);
  CHECK(!m.prelit);
  CHECK(m.has_render_state);
  CHECK(m.z_mode == 2);
  CHECK(m.alpha_cut);
  CHECK(m.alpha_threshold == 0);
  CHECK(!m.alpha_write);
  CHECK(m.tex_gen == 0);
  CHECK(m.tex_wrap == 4);
  CHECK(approx(m.tex_scale[0], 2.0f));
  CHECK(approx(m.tex_scale[1], 3.0f));
  CHECK(approx(m.tex_offset[0], 0.25f));
  CHECK(approx(m.tex_offset[1], 0.5f));
  CHECK(m.has_cull);
  CHECK(!m.cull);
  std::printf("  [ok] Mat: tex=%s blend=%u alphaCut=%d zMode=%u texWrap=%u cull=%d color=(%.0f,%.0f,%.0f,%.0f)\n",
              m.diffuse_tex.c_str(), static_cast<unsigned>(m.blend),
              m.alpha_cut ? 1 : 0, static_cast<unsigned>(m.z_mode),
              static_cast<unsigned>(m.tex_wrap), m.cull ? 1 : 0,
              m.color[0], m.color[1], m.color[2], m.color[3]);
}

void test_group() {
  const SourceRndAnimatableLoadPlan anim_v4 =
      source_rndanimatable_load_plan(4);
  CHECK(anim_v4.accepted_revision);
  CHECK(anim_v4.default_frame_zero);
  CHECK(anim_v4.default_rate_30_fps);
  CHECK(anim_v4.reads_frame);
  CHECK(anim_v4.reads_int_rate);
  CHECK(!anim_v4.reads_legacy_byte_rate);
  CHECK(!anim_v4.reads_legacy_rev0_filter_rows);

  const SourceRndAnimatableLoadPlan anim_v3 =
      source_rndanimatable_load_plan(3);
  CHECK(anim_v3.reads_frame);
  CHECK(!anim_v3.reads_int_rate);
  CHECK(anim_v3.reads_legacy_byte_rate);

  const SourceRndAnimatableLoadPlan anim_v1 =
      source_rndanimatable_load_plan(1);
  CHECK(!anim_v1.reads_frame);
  CHECK(!anim_v1.reads_int_rate);
  CHECK(!anim_v1.reads_legacy_byte_rate);
  CHECK(!anim_v1.reads_legacy_rev0_filter_rows);

  const SourceRndAnimatableLoadPlan anim_v0 =
      source_rndanimatable_load_plan(0);
  CHECK(anim_v0.accepted_revision);
  CHECK(anim_v0.reads_legacy_rev0_filter_rows);
  CHECK(anim_v0.reads_legacy_rev0_anim_list);

  const SourceRndAnimatableLoadPlan anim_v5 =
      source_rndanimatable_load_plan(5);
  CHECK(!anim_v5.accepted_revision);

  const SourceRndDrawableLoadPlan drawable_v3 =
      source_rnddrawable_load_plan(3, 24);
  CHECK(drawable_v3.reads_showing);
  CHECK(!drawable_v3.reads_old_drawable_list);
  CHECK(drawable_v3.reads_sphere);
  CHECK(drawable_v3.reads_draw_order);
  CHECK(!drawable_v3.reads_clip_planes);

  const SourceRndDrawableLoadPlan drawable_v1_old_parent =
      source_rnddrawable_load_plan(1, 6);
  CHECK(drawable_v1_old_parent.reads_old_drawable_list);
  CHECK(drawable_v1_old_parent.old_list_is_null_terminated_strings);
  CHECK(!drawable_v1_old_parent.old_list_is_symbols);
  CHECK(drawable_v1_old_parent.reads_sphere);
  CHECK(!drawable_v1_old_parent.reads_draw_order);

  const SourceRndDrawableLoadPlan drawable_v1_new_parent =
      source_rnddrawable_load_plan(1, 24);
  CHECK(drawable_v1_new_parent.old_list_is_symbols);
  CHECK(!drawable_v1_new_parent.old_list_is_null_terminated_strings);

  const SourceRndDrawableLoadPlan drawable_v4 =
      source_rnddrawable_load_plan(4, 24);
  CHECK(drawable_v4.reads_clip_planes);

  const SourceRndGroupLoadPlan group_v15 = source_rndgroup_load_plan(15);
  CHECK(group_v15.reads_object_fields);
  CHECK(group_v15.reads_animatable);
  CHECK(group_v15.reads_trans);
  CHECK(group_v15.reads_drawable);
  CHECK(group_v15.reads_objects);
  CHECK(group_v15.reads_environ);
  CHECK(group_v15.reads_draw_only);
  CHECK(group_v15.reads_lod);
  CHECK(group_v15.reads_sort_in_world);

  const SourceRndGroupLoadPlan group_v16 = source_rndgroup_load_plan(16);
  CHECK(group_v16.reads_objects);
  CHECK(!group_v16.reads_environ);
  CHECK(group_v16.reads_draw_only);
  CHECK(!group_v16.reads_lod);

  const SourceRndGroupLoadPlan group_v4 = source_rndgroup_load_plan(4);
  CHECK(!group_v4.reads_object_fields);
  CHECK(!group_v4.reads_objects);
  CHECK(group_v4.reads_legacy_rev4_objects);
  CHECK(!group_v4.reads_sort_in_world);

  const SourceRndGroupLoadPlan group_v7 = source_rndgroup_load_plan(7);
  CHECK(group_v7.reads_rev7_lod_dimensions);

  const SourceRndGroupDefaultState group_defaults =
      source_rndgroup_default_state();
  CHECK(group_defaults.objects_owner_control);
  CHECK(group_defaults.env_null);
  CHECK(group_defaults.draw_only_null);
  CHECK(group_defaults.lod_null);
  CHECK(approx(group_defaults.lod_screen_size, 0.0f));
  CHECK(!group_defaults.sort_in_world);
  CHECK(!group_defaults.unkf8);

  const SourceRndGroupCopyPlan group_copy = source_rndgroup_copy_plan();
  CHECK(group_copy.superclasses.size() == 4);
  CHECK(group_copy.superclasses[0] == "Hmx::Object");
  CHECK(group_copy.superclasses[3] == "RndTransformable");
  CHECK(group_copy.member_order.size() == 6);
  CHECK(group_copy.member_order[0] == "mEnv");
  CHECK(group_copy.member_order[5] == "mObjects");
  CHECK(group_copy.deep_copies_objects);
  CHECK(group_copy.from_max_merges_objects);
  CHECK(group_copy.calls_update);

  const SourceRndGroupReplacePlan group_replace_found =
      source_rndgroup_replace_plan(true);
  CHECK(group_replace_found.calls_transformable_replace);
  CHECK(group_replace_found.scans_objects);
  CHECK(group_replace_found.add_object_when_found);
  CHECK(group_replace_found.sets_in_replace_around_remove);
  CHECK(group_replace_found.remove_object_when_found);
  CHECK(!group_replace_found.no_object_no_membership_change);

  const SourceRndGroupReplacePlan group_replace_missing =
      source_rndgroup_replace_plan(false);
  CHECK(group_replace_missing.calls_transformable_replace);
  CHECK(group_replace_missing.scans_objects);
  CHECK(!group_replace_missing.add_object_when_found);
  CHECK(!group_replace_missing.sets_in_replace_around_remove);
  CHECK(!group_replace_missing.remove_object_when_found);
  CHECK(group_replace_missing.no_object_no_membership_change);

  const SourceRndGroupHandlerPlan group_handlers =
      source_rndgroup_handler_plan();
  CHECK(group_handlers.actions.size() == 4);
  CHECK(group_handlers.actions[0] == "sort_draws");
  CHECK(group_handlers.actions[3] == "clear_objects");
  CHECK(group_handlers.queries.size() == 2);
  CHECK(group_handlers.queries[0] == "get_draws");
  CHECK(group_handlers.queries[1] == "has_object");
  CHECK(group_handlers.superclasses.size() == 4);
  CHECK(group_handlers.superclasses[0] == "RndAnimatable");
  CHECK(group_handlers.superclasses[3] == "Hmx::Object");
  CHECK(group_handlers.check == 0x29B);

  const SourceRndGroupPropSyncPlan group_props =
      source_rndgroup_prop_sync_plan();
  CHECK(group_props.props.size() == 6);
  CHECK(group_props.props[0] == "objects");
  CHECK(group_props.props[5] == "sort_in_world");
  CHECK(group_props.side_effects.size() == 3);
  CHECK(group_props.side_effects[0] == "objects:Update");
  CHECK(group_props.side_effects[2] == "lod_screen_size:UpdateLODState");
  CHECK(group_props.superclasses.size() == 3);
  CHECK(group_props.superclasses[0] == "RndDrawable");
  CHECK(group_props.superclasses[2] == "RndAnimatable");

  std::vector<uint8_t> b;
  put_u32(b, 15);                // RndGroup revision
  put_zeros(b, 9);               // Hmx::Object fields
  put_u32(b, 4);                 // RndAnimatable revision
  put_f32(b, 0.0f);              // frame
  put_u32(b, 0);                 // rate
  put_u32(b, 9);                 // RndTrans revision
  put_matrix(b, 0, 0, 0);
  put_matrix(b, 0, 0, 0);
  put_u32(b, 0);                 // constraint
  put_str(b, "");                // target
  b.push_back(0);                // preserve_scale
  put_str(b, "");                // parent
  put_u32(b, 3);                 // RndDrawable revision
  b.push_back(1);                // showing
  put_zeros(b, 16);              // sphere
  put_f32(b, 0.25f);             // draw_order
  put_u32(b, 2);                 // objects
  put_str(b, "hair-front1.mesh");
  put_str(b, "hair-front2.mesh");
  put_str(b, "stage.env");       // environ for rev < 16
  put_str(b, "hair-front1.mesh"); // drawOnly for rev > 12
  put_str(b, "lod1.grp");        // legacy lod group
  put_f32(b, 0.5f);              // lod screen size
  b.push_back(1);                // sortInWorld

  GroupObj g = decode_group("lod0.grp", b);
  CHECK(g.decoded);
  CHECK(g.children.size() == 2);
  CHECK(g.children[0] == "hair-front1.mesh");
  CHECK(g.children[1] == "hair-front2.mesh");
  CHECK(g.environment_ref == "stage.env");
  CHECK(g.draw_only == "hair-front1.mesh");
  CHECK(g.sort_in_world);
  std::printf("  [ok] Group: children=%zu env=%s drawOnly=%s sort=%d\n",
              g.children.size(), g.environment_ref.c_str(),
              g.draw_only.c_str(), g.sort_in_world ? 1 : 0);
}

void test_mesh_deform() {
  const SourceRndMeshDeformVertArrayState vert_array_parented =
      source_rndmesh_deform_vert_array_default_state(true);
  CHECK(vert_array_parented.size == 0);
  CHECK(vert_array_parented.data_null);
  CHECK(vert_array_parented.parent_set);

  const SourceRndMeshDeformVertArrayState vert_array_unparented =
      source_rndmesh_deform_vert_array_default_state(false);
  CHECK(!vert_array_unparented.parent_set);

  const SourceRndMeshDeformVertArraySetSizePlan resize =
      source_rndmesh_deform_vert_array_set_size_plan(4, 12);
  CHECK(resize.old_size == 4);
  CHECK(resize.new_size == 12);
  CHECK(resize.changes_size);
  CHECK(resize.frees_existing_data);
  CHECK(resize.allocates_requested_size);

  const SourceRndMeshDeformVertArraySetSizePlan same_size =
      source_rndmesh_deform_vert_array_set_size_plan(4, 4);
  CHECK(!same_size.changes_size);
  CHECK(!same_size.frees_existing_data);
  CHECK(!same_size.allocates_requested_size);

  const SourceRndMeshDeformClearPlan clear_nonempty =
      source_rndmesh_deform_clear_plan(3);
  CHECK(clear_nonempty.calls_set_size_zero);
  CHECK(clear_nonempty.changes_size);
  const SourceRndMeshDeformClearPlan clear_empty =
      source_rndmesh_deform_clear_plan(0);
  CHECK(clear_empty.calls_set_size_zero);
  CHECK(!clear_empty.changes_size);

  const SourceRndMeshDeformDefaultState defaults =
      source_rndmesh_deform_default_state();
  CHECK(defaults.mesh_null);
  CHECK(defaults.bones_parent_set);
  CHECK(defaults.verts_parent_set);
  CHECK(!defaults.skip_inverse);
  CHECK(!defaults.deformed);

  const SourceRndMeshDeformSetMeshPlan set_mesh =
      source_rndmesh_deform_set_mesh_plan();
  CHECK(set_mesh.assigns_mesh);
  CHECK(set_mesh.clears_verts);

  const SourceRndMeshDeformHandlerPlan handlers =
      source_rndmesh_deform_handler_plan();
  CHECK(handlers.superclasses.size() == 1);
  CHECK(handlers.superclasses[0] == "Hmx::Object");
  CHECK(handlers.check == 0x2A1);

  const SourceRndMeshDeformBodyAvailability bodies =
      source_rndmesh_deform_body_availability();
  CHECK(!bodies.load_body_visible);
  CHECK(!bodies.copy_body_visible);
  CHECK(!bodies.reskin_body_visible);
  CHECK(!bodies.copy_weights_body_visible);
  CHECK(!bodies.find_deform_body_visible);

  std::printf("  [ok] MeshDeform: resize=%d handler=0x%x\n",
              resize.new_size, handlers.check);
}

void test_multimesh() {
  const SourceRndMultiMeshDefaultState defaults =
      source_rndmultimesh_default_state();
  CHECK(defaults.mesh_null);
  CHECK(defaults.unk9p4_zero);

  const SourceRndMultiMeshInstanceDefaultState instance_defaults =
      source_rndmultimesh_instance_default_state();
  CHECK(instance_defaults.resets_transform);

  const SourceRndMultiMeshLoadPlan rev0 = source_rndmultimesh_load_plan(0);
  CHECK(rev0.accepted_revision);
  CHECK(!rev0.reads_object_fields);
  CHECK(rev0.reads_drawable);
  CHECK(rev0.reads_mesh);
  CHECK(rev0.reads_legacy_transform_dump_and_returns);
  CHECK(!rev0.reads_instances);

  const SourceRndMultiMeshLoadPlan rev3 = source_rndmultimesh_load_plan(3);
  CHECK(rev3.accepted_revision);
  CHECK(rev3.reads_object_fields);
  CHECK(!rev3.reads_legacy_transform_dump_and_returns);
  CHECK(rev3.reads_instances);
  CHECK(rev3.reads_legacy_tail_byte);

  const SourceRndMultiMeshLoadPlan rev4 = source_rndmultimesh_load_plan(4);
  CHECK(rev4.accepted_revision);
  CHECK(rev4.reads_instances);
  CHECK(!rev4.reads_legacy_tail_byte);

  const SourceRndMultiMeshLoadPlan rev5 = source_rndmultimesh_load_plan(5);
  CHECK(!rev5.accepted_revision);

  const SourceRndMultiMeshCopyPlan regular_copy =
      source_rndmultimesh_copy_plan(false);
  CHECK(regular_copy.superclasses.size() == 2);
  CHECK(regular_copy.superclasses[0] == "Hmx::Object");
  CHECK(regular_copy.superclasses[1] == "RndDrawable");
  CHECK(regular_copy.copies_mesh);
  CHECK(regular_copy.copies_instances);
  CHECK(regular_copy.calls_update_mesh);
  const SourceRndMultiMeshCopyPlan max_copy =
      source_rndmultimesh_copy_plan(true);
  CHECK(!max_copy.copies_mesh);
  CHECK(max_copy.copies_instances);

  const SourceRndMultiMeshSetMeshPlan set_mesh =
      source_rndmultimesh_set_mesh_plan();
  CHECK(set_mesh.assigns_mesh);
  CHECK(set_mesh.calls_update_mesh);

  const SourceRndMultiMeshHandlerPlan handlers =
      source_rndmultimesh_handler_plan();
  CHECK(handlers.handlers.size() == 17);
  CHECK(handlers.handlers.front() == "move_xfms");
  CHECK(handlers.handlers.back() == "num_xfms");
  CHECK(handlers.actions.size() == 1);
  CHECK(handlers.actions[0] == "set_mesh");
  CHECK(handlers.superclasses.size() == 2);
  CHECK(handlers.warns_unhandled);

  const SourceRndMultiMeshSetPosPlan set_pos =
      source_rndmultimesh_set_pos_plan(4);
  CHECK(set_pos.requested_index == 4);
  CHECK(set_pos.advances_iterator_by_index);
  CHECK(set_pos.assignment_order.size() == 6);
  CHECK(set_pos.assignment_order[0] == "read_z");
  CHECK(set_pos.assignment_order[2] == "read_x");
  CHECK(set_pos.assignment_order[5] == "write_z");

  const SourceRndMultiMeshPropSyncPlan prop_sync =
      source_rndmultimesh_prop_sync_plan();
  CHECK(prop_sync.superclasses.size() == 1);
  CHECK(prop_sync.superclasses[0] == "RndDrawable");

  const SourceRndMultiMeshProxyDefaultState proxy_defaults =
      source_rndmultimesh_proxy_default_state();
  CHECK(proxy_defaults.multimesh_null);
  CHECK(proxy_defaults.index_zero);

  const SourceRndMultiMeshProxySetPlan proxy_set =
      source_rndmultimesh_proxy_set_plan(true);
  CHECK(proxy_set.clears_multimesh_first);
  CHECK(proxy_set.has_mesh);
  CHECK(proxy_set.copies_instance_local_transform);
  CHECK(proxy_set.assigns_multimesh);
  CHECK(proxy_set.assigns_index);
  const SourceRndMultiMeshProxySetPlan proxy_set_null =
      source_rndmultimesh_proxy_set_plan(false);
  CHECK(!proxy_set_null.copies_instance_local_transform);

  const SourceRndMultiMeshProxyDrawPlan draw_full =
      source_rndmultimesh_proxy_draw_plan(true, true);
  CHECK(draw_full.reads_multimesh_mesh);
  CHECK(draw_full.sets_mesh_world_from_instance);
  CHECK(draw_full.draws_mesh);
  const SourceRndMultiMeshProxyDrawPlan draw_no_mesh =
      source_rndmultimesh_proxy_draw_plan(true, false);
  CHECK(draw_no_mesh.reads_multimesh_mesh);
  CHECK(!draw_no_mesh.sets_mesh_world_from_instance);
  CHECK(!draw_no_mesh.draws_mesh);

  const SourceRndMultiMeshProxyUpdatedWorldPlan updated =
      source_rndmultimesh_proxy_updated_world_plan(true);
  CHECK(updated.writes_instance_from_world);
  CHECK(updated.szbe69_variant_visible);
  const SourceRndMultiMeshProxyUpdatedWorldPlan updated_null =
      source_rndmultimesh_proxy_updated_world_plan(false);
  CHECK(!updated_null.writes_instance_from_world);

  const SourceRndMultiMeshProxyFailurePlan failures =
      source_rndmultimesh_proxy_failure_plan();
  CHECK(failures.load_fails);
  CHECK(failures.save_fails);
  CHECK(failures.copy_fails);

  const SourceRndMultiMeshProxyHandlerPlan proxy_handlers =
      source_rndmultimesh_proxy_handler_plan();
  CHECK(proxy_handlers.check == 0x3F);
  const SourceRndMultiMeshProxyPropSyncPlan proxy_props =
      source_rndmultimesh_proxy_prop_sync_plan();
  CHECK(!proxy_props.has_rows);

  std::printf("  [ok] MultiMesh: handlers=%zu proxy_check=0x%x\n",
              handlers.handlers.size(), proxy_handlers.check);
}

void test_light() {
  std::vector<uint8_t> b;
  put_u32(b, 6);                 // Light version
  put_zeros(b, 13);              // object/base header
  put_matrix(b, 10.0f, 20.0f, 30.0f);
  put_matrix(b, 40.0f, 50.0f, 60.0f);
  put_zeros(b, 13);              // tail before color block
  put_f32(b, 0.1f); put_f32(b, 0.2f); put_f32(b, 0.3f); put_f32(b, 1.0f);
  put_f32(b, 500.0f);
  put_u32(b, 1);                 // kLightDirectional
  b.push_back(1);                // animate_color_from_preset
  b.push_back(0);                // animate_position_from_preset

  LightObj light = decode_light("stage_light_02.lit", b);
  CHECK(light.decoded);
  CHECK(approx(light.world_stored.pos[0], 40.0f));
  CHECK(approx(light.color[2], 0.3f));
  CHECK(approx(light.range, 500.0f));
  CHECK(light.type == 1);
  CHECK(light.animate_color_from_preset);
  CHECK(!light.animate_position_from_preset);
  std::printf("  [ok] Light: type=%d color=(%.1f,%.1f,%.1f) range=%.1f\n",
              light.type, light.color[0], light.color[1], light.color[2],
              light.range);
}

void test_environ_with_lights() {
  std::vector<uint8_t> b;
  put_u32(b, 5);                 // Environ version
  put_zeros(b, 9);               // base metadata
  put_u32(b, 2);                 // dynamic light ref count
  put_str(b, "stage_light_02.lit");
  put_str(b, "stage_light_03.lit");
  const size_t base = b.size();
  put_f32(b, 0.25f); put_f32(b, 0.5f); put_f32(b, 0.75f); put_f32(b, 1.0f);
  put_f32(b, 0.0f);
  put_f32(b, 1.0f);
  put_f32(b, 1.0f); put_f32(b, 1.0f); put_f32(b, 1.0f); put_f32(b, 1.0f);
  while (b.size() < base + 0x2f) b.push_back(0);
  b[base + 0x29] = 1;            // animate_from_preset
  put_f32(b, 1000.0f);

  EnvironObj env = decode_environ("stage.env", b);
  CHECK(env.decoded);
  CHECK(env.lights.size() == 2);
  CHECK(env.lights[0] == "stage_light_02.lit");
  CHECK(approx(env.color_a[0], 0.25f));
  CHECK(approx(env.color_a[2], 0.75f));
  CHECK(!env.fog_enabled);
  CHECK(env.animate_from_preset);
  CHECK(approx(env.range, 1000.0f));
  std::printf("  [ok] Environ: lights=%zu ambient=(%.2f,%.2f,%.2f)\n",
              env.lights.size(), env.color_a[0], env.color_a[1],
              env.color_a[2]);
}

void test_environ_with_extensionless_light() {
  std::vector<uint8_t> b;
  put_u32(b, 5);                 // Environ version
  put_zeros(b, 9);               // base metadata
  put_u32(b, 1);                 // dynamic light ref count
  put_str(b, "curtain");         // GH2 PS2 Big uses Light__curtain
  const size_t base = b.size();
  put_f32(b, 0.30f); put_f32(b, 0.30f); put_f32(b, 0.30f); put_f32(b, 1.0f);
  put_f32(b, 250.0f);
  put_f32(b, 1.0f);
  put_f32(b, 1.0f); put_f32(b, 1.0f); put_f32(b, 1.0f); put_f32(b, 1.0f);
  while (b.size() < base + 0x2f) b.push_back(0);
  b[base + 0x29] = 1;            // animate_from_preset
  put_f32(b, 1000.0f);

  EnvironObj env = decode_environ("curtain_light", b);
  CHECK(env.decoded);
  CHECK(env.lights.size() == 1);
  CHECK(env.lights[0] == "curtain");
  CHECK(approx(env.color_a[0], 0.30f));
  CHECK(approx(env.range_a, 250.0f));
  CHECK(approx(env.fog_start, 250.0f));
  CHECK(env.animate_from_preset);
  CHECK(approx(env.range, 1000.0f));
  std::printf("  [ok] Environ extensionless light: %s -> %s\n",
              env.name.c_str(), env.lights[0].c_str());
}

void test_environ_with_fog() {
  std::vector<uint8_t> b;
  put_u32(b, 5);                 // Environ version
  put_zeros(b, 9);               // base metadata
  put_u32(b, 0);                 // dynamic light ref count
  const size_t base = b.size();
  put_f32(b, 0.07f); put_f32(b, 0.04f); put_f32(b, 0.14f); put_f32(b, 1.0f);
  put_f32(b, 0.0f);              // fog_start
  put_f32(b, 3000.0f);           // fog_end
  put_f32(b, 0.5f); put_f32(b, 0.0f); put_f32(b, 0.5f); put_f32(b, 1.0f);
  while (b.size() < base + 0x2f) b.push_back(0);
  b[base + 0x28] = 1;            // fog_enable
  b[base + 0x29] = 0;            // animate_from_preset
  put_f32(b, 1000.0f);

  EnvironObj env = decode_environ("op_Art_projection.env", b);
  CHECK(env.decoded);
  CHECK(env.fog_enabled);
  CHECK(!env.animate_from_preset);
  CHECK(approx(env.fog_start, 0.0f));
  CHECK(approx(env.fog_end, 3000.0f));
  CHECK(approx(env.fog_color[0], 0.5f));
  CHECK(approx(env.fog_color[2], 0.5f));
  std::printf("  [ok] Environ fog: %s start=%.0f end=%.0f\n",
              env.name.c_str(), env.fog_start, env.fog_end);
}

void test_mesh() {
  std::vector<uint8_t> b;
  put_u32(b, 28);                // mesh version 0x1c
  put_zeros(b, 9);               // Mesh object metadata
  // Embedded Trans base. MiloLib reads no object metadata here.
  put_u32(b, 9);                 // trans version
  put_matrix(b, 1.0f, 2.0f, 3.0f);  // local (translation 1,2,3)
  put_matrix(b, 1.0f, 2.0f, 3.0f);  // world
  put_zeros(b, 9);
  put_str(b, "track.view");      // trans parent
  // Draw base.
  put_u32(b, 3);                 // draw version
  const size_t draw_showing_offset = b.size();
  b.push_back(1);                // showing
  put_zeros(b, 20);              // sphere + draw-order
  // Mesh fields.
  put_str(b, "gem.mat");         // material
  put_str(b, "tri.mesh");        // geometry owner
  put_zeros(b, 9);
  put_u32(b, 3);                 // vertex_count = 3
  // 3 vertices (pos / normal / colour / uv), forming a unit triangle.
  const float P[3][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  for (int i = 0; i < 3; ++i) {
    put_f32(b, P[i][0]); put_f32(b, P[i][1]); put_f32(b, P[i][2]);  // pos
    put_f32(b, 0); put_f32(b, 0); put_f32(b, 1);                    // normal
    put_f32(b, 1); put_f32(b, 1); put_f32(b, 1); put_f32(b, 1);     // colour
    put_f32(b, P[i][0]); put_f32(b, P[i][1]);                       // uv
  }
  put_u32(b, 1);                 // face_count = 1
  put_u16(b, 0); put_u16(b, 1); put_u16(b, 2);  // the triangle
  put_zeros(b, 8);               // trailing group data

  MeshObj m = decode_mesh("tri.mesh", b);
  if (!m.decoded) std::printf("  [FAIL] mesh error: %s\n", m.error.c_str());
  CHECK(m.decoded);
  CHECK(m.vertex_count == 3);
  CHECK(m.face_count == 1);
  CHECK(m.indices.size() == 3 && m.indices[2] == 2);
  CHECK(m.material == "gem.mat");
  CHECK(m.parent == "track.view");
  CHECK(m.showing);
  CHECK(approx(m.local.pos[0], 1.0f) && approx(m.local.pos[2], 3.0f));
  // bbox of the unit triangle.
  CHECK(approx(m.bb_min[0], 0.0f) && approx(m.bb_max[0], 1.0f));
  CHECK(approx(m.bb_max[1], 1.0f));
  std::printf("  [ok] Mesh: vtx=%u face=%u mat=%s parent=%s bbox=[%.0f,%.0f,%.0f]-[%.0f,%.0f,%.0f]\n",
              m.vertex_count, m.face_count, m.material.c_str(),
              m.parent.c_str(), m.bb_min[0], m.bb_min[1], m.bb_min[2],
              m.bb_max[0], m.bb_max[1], m.bb_max[2]);

  // World-matrix composition: a mesh under a Trans that translates by (10,0,0).
  Scene sc;
  TransObj parent;
  parent.name = "track.view";
  parent.local.pos[0] = 10.0f;
  sc.transes.push_back(parent);
  sc.meshes.push_back(m);
  auto w = sc.world_matrix(sc.meshes[0]);
  // local pos (1,2,3) composed with parent translate (10,0,0) -> (11,2,3).
  CHECK(approx(w[12], 11.0f));
  CHECK(approx(w[13], 2.0f));
  CHECK(approx(w[14], 3.0f));
  std::printf("  [ok] world compose: translation=(%.0f,%.0f,%.0f)\n", w[12],
              w[13], w[14]);

  std::vector<uint8_t> hidden = b;
  hidden[draw_showing_offset] = 0;
  MeshObj h = decode_mesh("hidden.mesh", hidden);
  CHECK(h.decoded);
  CHECK(!h.showing);
}

}  // namespace

int main() {
  std::printf("milo_scene_test\n");
  test_trans();
  test_trans_proxy();
  test_trans_anim();
  test_mesh_anim();
  test_mat();
  test_group();
  test_mesh_deform();
  test_multimesh();
  test_light();
  test_environ_with_lights();
  test_environ_with_extensionless_light();
  test_environ_with_fog();
  test_mesh();
  std::printf("ALL PASS\n");
  return 0;
}
