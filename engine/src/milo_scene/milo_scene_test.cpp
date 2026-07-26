// engine/src/milo_scene/milo_scene_test.cpp
//
// Hermetic unit tests for the MILO render-object decoders. We hand-build the
// exact GH2 PS2 byte layouts (no ARK / no I/O) so the decoder's field offsets
// are pinned by an in-repo oracle. Byte layouts mirror real entries decoded
// from track/gen/track.milo_ps2 (green_gem.mesh, gem.mat, track_fade.trans).

#include "milo_scene/milo_scene.h"

#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>

namespace {

using namespace ghogx::milo_scene;
namespace fs = std::filesystem;

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
void put_utf8_z(std::vector<uint8_t>& b, const std::string& s) {
  for (char c : s) b.push_back(static_cast<uint8_t>(c));
  b.push_back(0);
}
void put_zeros(std::vector<uint8_t>& b, size_t n) {
  for (size_t i = 0; i < n; ++i) b.push_back(0);
}
std::vector<uint8_t> bytes_from_hex(const char* text) {
  std::vector<uint8_t> bytes;
  int high = -1;
  for (const unsigned char* p =
           reinterpret_cast<const unsigned char*>(text);
       *p != 0; ++p) {
    if (!std::isxdigit(*p)) continue;
    const int value = std::isdigit(*p)
                          ? static_cast<int>(*p - '0')
                          : static_cast<int>(std::tolower(*p) - 'a' + 10);
    if (high < 0) {
      high = value;
    } else {
      bytes.push_back(static_cast<uint8_t>((high << 4) | value));
      high = -1;
    }
  }
  CHECK(high < 0);
  return bytes;
}
// Identity rotation + given translation, as a Harmonix 3x4 matrix.
void put_matrix(std::vector<uint8_t>& b, float tx, float ty, float tz) {
  put_f32(b, 1); put_f32(b, 0); put_f32(b, 0);
  put_f32(b, 0); put_f32(b, 1); put_f32(b, 0);
  put_f32(b, 0); put_f32(b, 0); put_f32(b, 1);
  put_f32(b, tx); put_f32(b, ty); put_f32(b, tz);
}

bool approx(float a, float b) { return std::fabs(a - b) < 1e-4f; }
bool approx_eps(float a, float b, float eps) { return std::fabs(a - b) < eps; }

std::string first_existing(const std::string& dir,
                           std::initializer_list<const char*> names) {
  for (const char* n : names) {
    fs::path p = fs::path(dir) / n;
    if (fs::exists(p)) return p.string();
  }
  return {};
}

void test_milo_editor_dtb_node_payload_plan() {
  const SourceMiloEditorDtbNodePayloadPlan dtb_int =
      source_milo_editor_dtb_node_payload_plan(0x00);
  CHECK(dtb_int.known_node_type);
  CHECK(dtb_int.node_type_name == "Int");
  CHECK(dtb_int.reads_uint32);
  CHECK(!dtb_int.reads_float);
  CHECK(!dtb_int.reads_symbol);
  CHECK(!dtb_int.reads_array_parent);

  const SourceMiloEditorDtbNodePayloadPlan dtb_float =
      source_milo_editor_dtb_node_payload_plan(0x01);
  CHECK(dtb_float.known_node_type);
  CHECK(dtb_float.node_type_name == "Float");
  CHECK(dtb_float.reads_float);

  const SourceMiloEditorDtbNodePayloadPlan dtb_symbol =
      source_milo_editor_dtb_node_payload_plan(0x05);
  CHECK(dtb_symbol.known_node_type);
  CHECK(dtb_symbol.node_type_name == "Symbol");
  CHECK(dtb_symbol.reads_symbol);

  const SourceMiloEditorDtbNodePayloadPlan dtb_array =
      source_milo_editor_dtb_node_payload_plan(0x10);
  CHECK(dtb_array.known_node_type);
  CHECK(dtb_array.node_type_name == "Array");
  CHECK(dtb_array.reads_array_parent);

  const SourceMiloEditorDtbNodePayloadPlan dtb_func =
      source_milo_editor_dtb_node_payload_plan(0x03);
  CHECK(dtb_func.known_node_type);
  CHECK(dtb_func.node_type_name == "Func");
  CHECK(dtb_func.consumes_no_payload);
  CHECK(!dtb_func.reads_uint32);
  CHECK(!dtb_func.reads_float);
  CHECK(!dtb_func.reads_symbol);
  CHECK(!dtb_func.reads_array_parent);

  const SourceMiloEditorDtbNodePayloadPlan dtb_unknown =
      source_milo_editor_dtb_node_payload_plan(0x7f);
  CHECK(!dtb_unknown.known_node_type);
  CHECK(dtb_unknown.node_type_name == "Unknown");
  CHECK(dtb_unknown.consumes_no_payload);
}

void test_trans() {
  const SourceRndTransformableDefaultState trans_defaults =
      source_rndtransformable_default_state();
  CHECK(trans_defaults.parent_null);
  CHECK(trans_defaults.target_null);
  CHECK(trans_defaults.constraint == 0);
  CHECK(!trans_defaults.preserve_scale);
  CHECK(trans_defaults.local_xfm_reset);
  CHECK(trans_defaults.world_xfm_reset);
  CHECK(trans_defaults.cache_allocated);
  CHECK(trans_defaults.cache_set_to_self);
  CHECK(source_rndtransformable_save_plan().save_id == 586);

  const SourceRndTransformableDirtyPlan dirty_clean =
      source_rndtransformable_set_dirty_plan(false, true);
  CHECK(!dirty_clean.cache_already_dirty);
  CHECK(dirty_clean.set_dirty_force);
  CHECK(dirty_clean.sets_last_bit);
  CHECK(dirty_clean.propagates_to_children);
  const SourceRndTransformableDirtyPlan dirty_already =
      source_rndtransformable_set_dirty_plan(true, true);
  CHECK(dirty_already.cache_already_dirty);
  CHECK(!dirty_already.set_dirty_force);
  CHECK(!dirty_already.propagates_to_children);

  const SourceRndTransformableParentPlan same_parent =
      source_rndtransformable_set_parent_plan(true, true, true, true);
  CHECK(same_parent.same_parent);
  CHECK(same_parent.same_parent_sets_dirty);
  CHECK(same_parent.calls_set_dirty);
  CHECK(!same_parent.assigns_parent);

  const SourceRndTransformableParentPlan reparent_preserve =
      source_rndtransformable_set_parent_plan(false, true, true, true);
  CHECK(reparent_preserve.preserve_world);
  CHECK(reparent_preserve.computes_reparent_delta);
  CHECK(reparent_preserve.transforms_local_xfm);
  CHECK(reparent_preserve.transforms_trans_anims);
  CHECK(reparent_preserve.removes_from_old_parent);
  CHECK(reparent_preserve.assigns_parent);
  CHECK(reparent_preserve.cache_set_to_new_parent_or_zero);
  CHECK(reparent_preserve.adds_to_new_parent_children);
  CHECK(reparent_preserve.calls_set_dirty);

  const SourceRndTransformableWorldWritePlan set_world =
      source_rndtransformable_world_write_plan("SetWorldXfm", true);
  CHECK(set_world.writes_world_xfm);
  CHECK(set_world.clears_cache_dirty_bit);
  CHECK(set_world.calls_updated_world_xfm);
  CHECK(set_world.dirties_children);
  const SourceRndTransformableWorldWritePlan set_world_pos =
      source_rndtransformable_world_write_plan("SetWorldPos", true);
  CHECK(set_world_pos.writes_world_position_only);
  CHECK(!set_world_pos.clears_cache_dirty_bit);
  CHECK(set_world_pos.calls_updated_world_xfm);
  CHECK(set_world_pos.dirties_children);

  const SourceRndTransformableLocalWritePlan set_local_pos =
      source_rndtransformable_local_write_plan("SetLocalPos");
  CHECK(set_local_pos.writes_local_position);
  CHECK(set_local_pos.calls_set_dirty);
  const SourceRndTransformableLocalWritePlan dirty_local =
      source_rndtransformable_local_write_plan("DirtyLocalXfm");
  CHECK(dirty_local.calls_set_dirty);
  CHECK(dirty_local.returns_dirty_local_ref);

  const SourceRndTransformableConstraintPlan constraint =
      source_rndtransformable_set_constraint_plan(9, "bone_head.mesh", true);
  CHECK(constraint.asserts_target_not_self);
  CHECK(constraint.constraint == 9);
  CHECK(constraint.target == "bone_head.mesh");
  CHECK(constraint.preserve_scale);
  CHECK(constraint.writes_constraint);
  CHECK(constraint.writes_preserve_scale);
  CHECK(constraint.writes_target);
  CHECK(constraint.calls_set_dirty);

  const SourceRndTransformableCopyPlan copy =
      source_rndtransformable_copy_plan();
  CHECK(copy.object_superclass_only_for_static_class);
  CHECK(copy.creates_copy);
  CHECK(copy.member_steps.size() == 7);
  CHECK(copy.member_steps[0] == "COPY_MEMBER(mWorldXfm)");
  CHECK(copy.member_steps[4] ==
        "if(ty != kCopyFromMax) COPY_MEMBER(mTarget)");
  CHECK(copy.member_steps[5] ==
        "else if(mConstraint == c->mConstraint) COPY_MEMBER(mTarget)");
  CHECK(copy.member_steps[6] == "SetTransParent(c->mParent, false)");

  const SourceRndTransformableHandlerPlan handlers =
      source_rndtransformable_handler_plan();
  CHECK(handlers.handlers.size() == 19);
  CHECK(handlers.handlers[0] == "copy_local_to:OnCopyLocalTo");
  CHECK(handlers.handlers[1] == "set_constraint:OnSetTransConstraint");
  CHECK(handlers.handlers[18] == "get_children:OnGetChildren");
  CHECK(handlers.actions.size() == 4);
  CHECK(handlers.actions[1] == "set_trans_parent:SetTransParent");
  CHECK(handlers.exprs.size() == 1);
  CHECK(handlers.exprs[0] == "trans_parent:mParent");
  CHECK(handlers.superclasses.size() == 1);
  CHECK(handlers.superclasses[0] == "Hmx::Object");
  CHECK(handlers.object_superclass_only_for_static_class);
  CHECK(handlers.check == 0x357);

  const SourceRndTransformablePropSyncPlan prop_sync =
      source_rndtransformable_prop_sync_plan();
  CHECK(prop_sync.set_properties.size() == 4);
  CHECK(prop_sync.set_properties[0] ==
        "trans_parent:SetTransParent(_val.Obj<RndTransformable>(0), true)");
  CHECK(prop_sync.set_properties[1] ==
        "trans_constraint:SetTransConstraint((Constraint)_val.Int(0), mTarget, mPreserveScale)");
  CHECK(prop_sync.set_properties[2] ==
        "trans_target:SetTransConstraint((Constraint)mConstraint, _val.Obj<RndTransformable>(0), mPreserveScale)");
  CHECK(prop_sync.set_properties[3] ==
        "preserve_scale:SetTransConstraint((Constraint)mConstraint, mTarget, _val.Int(0))");

  const SourceRndTransformableDistributeChildrenPlan distribute_horizontal =
      source_rndtransformable_distribute_children_plan(
          true, 2.5f, {{"right", 4.0f, 0.0f},
                       {"left", 1.0f, 3.0f},
                       {"middle", 2.0f, 8.0f}});
  CHECK(distribute_horizontal.entered);
  CHECK(distribute_horizontal.horizontal);
  CHECK(distribute_horizontal.axis == 0);
  CHECK(approx(distribute_horizontal.base_axis_value, 1.0f));
  CHECK(distribute_horizontal.sorted_children.size() == 3);
  CHECK(distribute_horizontal.sorted_children[0] == "left");
  CHECK(distribute_horizontal.sorted_children[1] == "middle");
  CHECK(distribute_horizontal.sorted_children[2] == "right");
  CHECK(distribute_horizontal.writes.size() == 2);
  CHECK(distribute_horizontal.writes[0].name == "middle");
  CHECK(distribute_horizontal.writes[0].source_index == 2);
  CHECK(approx(distribute_horizontal.writes[0].assigned_axis_value, 3.5f));
  CHECK(distribute_horizontal.writes[0].calls_set_local_xfm);
  CHECK(distribute_horizontal.writes[1].name == "right");
  CHECK(approx(distribute_horizontal.writes[1].assigned_axis_value, 6.0f));

  const SourceRndTransformableDistributeChildrenPlan distribute_vertical =
      source_rndtransformable_distribute_children_plan(
          false, 1.25f, {{"low", 9.0f, -1.0f},
                         {"high", 0.0f, 5.0f},
                         {"mid", 4.0f, 2.0f}});
  CHECK(distribute_vertical.entered);
  CHECK(!distribute_vertical.horizontal);
  CHECK(distribute_vertical.axis == 2);
  CHECK(approx(distribute_vertical.base_axis_value, 5.0f));
  CHECK(distribute_vertical.sorted_children[0] == "high");
  CHECK(distribute_vertical.sorted_children[1] == "mid");
  CHECK(distribute_vertical.sorted_children[2] == "low");
  CHECK(distribute_vertical.writes.size() == 2);
  CHECK(distribute_vertical.writes[0].name == "mid");
  CHECK(approx(distribute_vertical.writes[0].original_axis_value, 2.0f));
  CHECK(approx(distribute_vertical.writes[0].assigned_axis_value, 6.25f));
  CHECK(distribute_vertical.writes[1].name == "low");
  CHECK(approx(distribute_vertical.writes[1].assigned_axis_value, 7.5f));

  const SourceRndTransformableDistributeChildrenPlan distribute_single =
      source_rndtransformable_distribute_children_plan(
          true, 3.0f, {{"only", 10.0f, 20.0f}});
  CHECK(!distribute_single.entered);
  CHECK(distribute_single.axis == 0);
  CHECK(distribute_single.sorted_children.empty());
  CHECK(distribute_single.writes.empty());

  const SourceRndTransformableCopyLocalToPlan copy_local =
      source_rndtransformable_copy_local_to_plan(
          {"first", "second", "third"});
  CHECK(copy_local.iterates_reverse);
  CHECK(copy_local.calls_set_local_xfm);
  CHECK(copy_local.write_order.size() == 3);
  CHECK(copy_local.write_order[0] == "third");
  CHECK(copy_local.write_order[1] == "second");
  CHECK(copy_local.write_order[2] == "first");

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

  const SourceMiloEditorRndTransNewPlan trans_new =
      source_milo_editor_rndtrans_new_plan(9, 2);
  CHECK(trans_new.revision == 9);
  CHECK(trans_new.alt_revision == 2);
  CHECK(trans_new.sets_revision);
  CHECK(trans_new.sets_alt_revision);
  CHECK(trans_new.local_xfm_identity);
  CHECK(trans_new.world_xfm_identity);
  CHECK(approx(trans_new.local_xfm.rot[0][0], 1.0f));
  CHECK(approx(trans_new.local_xfm.rot[1][1], 1.0f));
  CHECK(approx(trans_new.local_xfm.rot[2][2], 1.0f));
  CHECK(approx(trans_new.local_xfm.pos[0], 0.0f));
  CHECK(approx(trans_new.world_xfm.rot[0][0], 1.0f));
  CHECK(approx(trans_new.world_xfm.rot[1][1], 1.0f));
  CHECK(approx(trans_new.world_xfm.rot[2][2], 1.0f));
  CHECK(approx(trans_new.world_xfm.pos[2], 0.0f));

  const SourceRndTransformableCppLoadPlan cpp_rev9 =
      source_rndtransformable_cpp_load_plan(9, false, true);
  CHECK(cpp_rev9.accepted_revision);
  CHECK(cpp_rev9.reads_object_fields_for_static_class);
  CHECK(cpp_rev9.reads_stored_local_world);
  CHECK(!cpp_rev9.reads_proxy_temp_transforms);
  CHECK(!cpp_rev9.reads_old_child_list);
  CHECK(!cpp_rev9.rev6_reads_constraint);
  CHECK(cpp_rev9.reads_target);
  CHECK(cpp_rev9.reads_preserve_scale);
  CHECK(cpp_rev9.reads_parent);
  CHECK(cpp_rev9.parent_sets_trans_parent);
  CHECK(!cpp_rev9.proxy_loads_parent_ref);

  const SourceRndTransformableCppLoadPlan cpp_rev9_proxy =
      source_rndtransformable_cpp_load_plan(9, true, false);
  CHECK(cpp_rev9_proxy.accepted_revision);
  CHECK(!cpp_rev9_proxy.reads_stored_local_world);
  CHECK(cpp_rev9_proxy.reads_proxy_temp_transforms);
  CHECK(cpp_rev9_proxy.proxy_loads_target_ref);
  CHECK(cpp_rev9_proxy.proxy_loads_parent_ref);
  CHECK(!cpp_rev9_proxy.parent_sets_trans_parent);

  const SourceRndTransformableCppLoadPlan cpp_rev8 =
      source_rndtransformable_cpp_load_plan(8, false, false);
  CHECK(cpp_rev8.reads_old_child_list);
  CHECK(cpp_rev8.old_child_list_sets_parent);
  CHECK(cpp_rev8.reads_target);
  CHECK(cpp_rev8.reads_preserve_scale);
  CHECK(cpp_rev8.reads_parent);
  CHECK(cpp_rev8.parent_sets_trans_parent);
  CHECK(cpp_rev8.rev7_8_parent_sets_constraint_parent_world);
  CHECK(!cpp_rev8.reads_sphere);

  const SourceRndTransformableCppLoadPlan cpp_rev6 =
      source_rndtransformable_cpp_load_plan(6, false, false);
  CHECK(cpp_rev6.rev6_reads_constraint);
  CHECK(cpp_rev6.rev6_preserve_scale_from_target_world);
  CHECK(cpp_rev6.reads_legacy_assert_vector);
  CHECK(!cpp_rev6.reads_legacy_bool);
  CHECK(cpp_rev6.reads_sphere);
  CHECK(cpp_rev6.may_set_drawable_sphere);
  CHECK(cpp_rev6.reads_target);
  CHECK(!cpp_rev6.reads_preserve_scale);
  CHECK(!cpp_rev6.reads_parent);
  CHECK(cpp_rev6.rev6_parent_from_target_when_constraint_parent_world);

  const SourceRndTransformableCppLoadPlan cpp_rev4 =
      source_rndtransformable_cpp_load_plan(4, false, false);
  CHECK(cpp_rev4.reads_legacy_assert_vector);
  CHECK(cpp_rev4.reads_legacy_bool);
  CHECK(!cpp_rev4.reads_target);
  CHECK(!cpp_rev4.reads_sphere);

  const SourceRndTransformableCppLoadPlan cpp_bad =
      source_rndtransformable_cpp_load_plan(10, false, false);
  CHECK(!cpp_bad.accepted_revision);

  std::vector<uint8_t> b;
  put_u32(b, 9);                 // version
  put_zeros(b, 9);               // base metadata
  put_matrix(b, 0, 120.0f, 0);   // local matrix (ty=120, like track_fade.trans)
  put_matrix(b, 0, 120.0f, 0);   // world matrix (identical)
  put_zeros(b, 9);               // constraint/flags
  put_str(b, "track_surface5.view");

  TransObj t = decode_trans("track_fade.trans", b, 24);
  CHECK(t.name == "track_fade.trans");
  CHECK(t.parent == "track_surface5.view");
  CHECK(approx(t.local.pos[1], 120.0f));
  CHECK(approx(t.local.rot[0][0], 1.0f) && approx(t.local.rot[2][2], 1.0f));

  std::vector<uint8_t> legacy;
  put_u32(legacy, 8);                  // RndTrans revision
  put_zeros(legacy, 9);                // standalone Object fields
  put_matrix(legacy, 1, 2, 3);
  put_matrix(legacy, 4, 5, 6);
  put_u32(legacy, 1);                  // old child list count
  put_utf8_z(legacy, "legacy_child");  // parent dir revision <= 6 branch
  put_u32(legacy, 3);                  // constraint
  put_str(legacy, "legacy_target");
  legacy.push_back(1);                 // preserve scale
  put_str(legacy, "legacy_parent");
  TransObj legacy_t = decode_trans("legacy.trans", legacy, 6);
  CHECK(legacy_t.parent == "legacy_parent");
  CHECK(legacy_t.constraint == 3);
  CHECK(legacy_t.target == "legacy_target");
  CHECK(legacy_t.preserve_scale);
  CHECK(approx(legacy_t.world_stored.pos[1], 5.0f));
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

  // Byte-for-byte body of stock GH2 PS2
  // track/gen/track.milo_ps2::extend_track_normal.tnm.  This pins the exact
  // MiloEditor field order and, importantly, the authored frame values above
  // the old highway scanner's fabricated 1000-frame ceiling.
  const std::vector<uint8_t> track_intro = bytes_from_hex(
      "06 00 00 00 00 00 00 00 00 00 00 00 00 04 00 00 "
      "00 00 00 00 00 01 00 00 00 09 00 00 00 74 72 61 "
      "63 6b 2e 63 61 6d 09 00 00 00 e3 3c 56 bd 00 00 "
      "00 00 00 00 00 00 07 a4 7f 3f 00 00 00 00 e3 3c "
      "56 bd 00 00 00 00 00 00 00 00 07 a4 7f 3f 00 00 "
      "fa 44 79 66 32 bd 50 19 88 3b be 41 88 3b f9 96 "
      "7f 3f 00 40 fa 44 ea 55 44 bd 6a bf 47 bb 17 6b "
      "19 39 0c a0 7f 3f 00 80 fa 44 27 f7 55 bd 08 98 "
      "45 ba 07 4b 0b bb c5 7c 7f 3f 00 e0 fa 44 94 09 "
      "4d bd 29 20 00 3b 6f 4c 91 bb 7a 85 7f 3f 00 40 "
      "fb 44 92 69 5a bd 63 f0 44 ba 25 34 0b bb 65 78 "
      "7f 3f 00 c0 fb 44 93 34 54 bd 00 00 00 00 00 00 "
      "00 00 07 a6 7f 3f 00 60 fc 44 79 47 56 bd 00 00 "
      "00 00 00 00 00 00 fd a3 7f 3f 00 20 fd 44 02 00 "
      "00 00 42 5c 49 3e 00 00 80 42 d6 88 8f 41 00 00 "
      "00 00 42 5c 49 3e c5 de 7c c2 d6 88 8f 41 00 00 "
      "b4 44 17 00 00 00 65 78 74 65 6e 64 5f 74 72 61 "
      "63 6b 5f 6e 6f 72 6d 61 6c 2e 74 6e 6d 01 00 01 "
      "00 00 00 00 00 80 3f 00 00 80 3f 00 00 80 3f 00 "
      "00 00 00 00 00 00");
  CHECK(track_intro.size() == 310);
  const DecodedRndTransAnimBody decoded =
      decode_rndtrans_anim_body_source_order(track_intro.data(),
                                             track_intro.size());
  CHECK(decoded.decoded);
  CHECK(decoded.exact_eof);
  CHECK(decoded.bytes_consumed == track_intro.size());
  CHECK(decoded.revision == 6);
  CHECK(decoded.anim_revision == 4);
  CHECK(decoded.anim_rate == 1);
  CHECK(decoded.target == "track.cam");
  CHECK(decoded.keys_owner == "extend_track_normal.tnm");
  CHECK(decoded.rotation_keys.size() == 9);
  CHECK(decoded.translation_keys.size() == 2);
  CHECK(decoded.scale_keys.size() == 1);
  CHECK(decoded.trans_spline);
  CHECK(!decoded.repeat_trans);
  CHECK(!decoded.scale_spline);
  CHECK(!decoded.follow_path);
  CHECK(!decoded.rot_slerp);
  CHECK(!decoded.rot_spline);
  CHECK(approx(decoded.translation_keys.front().value[0], 0.19664f));
  CHECK(approx(decoded.translation_keys.front().value[1], 64.0f));
  CHECK(approx(decoded.translation_keys.front().value[2], 17.94181f));
  CHECK(approx(decoded.translation_keys.front().frame, 0.0f));
  CHECK(approx(decoded.translation_keys.back().value[0], 0.19664f));
  CHECK(approx(decoded.translation_keys.back().value[1], -63.21755f));
  CHECK(approx(decoded.translation_keys.back().value[2], 17.94181f));
  CHECK(approx(decoded.translation_keys.back().frame, 1440.0f));
  CHECK(approx(rndtrans_anim_start_frame(decoded), 0.0f));
  CHECK(approx(rndtrans_anim_end_frame(decoded), 2025.0f));

  const auto frame_0 = sample_rndtrans_anim_translation(decoded, 0.0f);
  const auto frame_720 = sample_rndtrans_anim_translation(decoded, 720.0f);
  const auto frame_1440 = sample_rndtrans_anim_translation(decoded, 1440.0f);
  const auto frame_1920 = sample_rndtrans_anim_translation(decoded, 1920.0f);
  CHECK(approx(frame_0[1], 64.0f));
  CHECK(approx_eps(frame_720[1], 0.391225f, 0.0002f));
  CHECK(approx(frame_1440[1], -63.21755f));
  CHECK(approx(frame_1920[1], -63.21755f));

  std::printf(
      "  [ok] TransAnim: load_v7=%d handlers=%zu track.cam y=%.3f->%.3f\n",
      load_v7.accepted_revision ? 1 : 0, handlers.handlers.size(),
      frame_0[1], frame_1920[1]);
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

void test_poll_anim() {
  const SourceRndPollableHandlerPlan pollable_handlers =
      source_rndpollable_handler_plan();
  CHECK(pollable_handlers.actions.size() == 2);
  CHECK(pollable_handlers.actions[0] == "enter");
  CHECK(pollable_handlers.actions[1] == "poll");
  CHECK(pollable_handlers.static_actions.size() == 1);
  CHECK(pollable_handlers.static_actions[0] == "exit");
  CHECK(pollable_handlers.check == 0x1A);

  const SourceRndPollableBasePlan pollable_base =
      source_rndpollable_base_plan();
  CHECK(pollable_base.poll_body_empty);
  CHECK(pollable_base.enter_handles_enter_msg);
  CHECK(pollable_base.exit_handles_exit_msg);
  CHECK(pollable_base.list_poll_children_empty);

  const SourceRndPollAnimDefaultState defaults =
      source_rndpollanim_default_state();
  CHECK(defaults.anims_no_null);

  const SourceRndPollAnimEndFramePlan end_frame =
      source_rndpollanim_end_frame_plan({4.0f, 12.5f, 9.0f});
  CHECK(end_frame.child_end_frames.size() == 3);
  CHECK(approx(end_frame.result, 12.5f));
  const SourceRndPollAnimEndFramePlan empty_end =
      source_rndpollanim_end_frame_plan({});
  CHECK(approx(empty_end.result, 0.0f));

  const SourceRndPollAnimChildListPlan child_list =
      source_rndpollanim_child_list_plan(3);
  CHECK(child_list.child_count == 3);
  CHECK(child_list.published_children == 3);

  const SourceRndPollAnimLifecyclePlan enter =
      source_rndpollanim_enter_plan(2);
  CHECK(enter.child_count == 2);
  CHECK(enter.start_anim_calls == 2);
  CHECK(enter.end_anim_calls == 0);
  const SourceRndPollAnimLifecyclePlan exit =
      source_rndpollanim_exit_plan(2);
  CHECK(exit.child_count == 2);
  CHECK(exit.start_anim_calls == 0);
  CHECK(exit.end_anim_calls == 2);

  const SourceRndPollAnimRateFramePlan fps =
      source_rndpollanim_rate_frame_plan(kSourceRndAnimRate30Fps, 1.5f,
                                         2.0f, 3.0f, 4.0f);
  CHECK(fps.recognized);
  CHECK(fps.uses_seconds);
  CHECK(!fps.uses_beat);
  CHECK(approx(fps.multiplier, 30.0f));
  CHECK(approx(fps.frame, 45.0f));

  const SourceRndPollAnimRateFramePlan fpb =
      source_rndpollanim_rate_frame_plan(kSourceRndAnimRate480Fpb, 1.5f,
                                         2.0f, 3.0f, 0.25f);
  CHECK(fpb.recognized);
  CHECK(fpb.uses_beat);
  CHECK(approx(fpb.multiplier, 480.0f));
  CHECK(approx(fpb.frame, 120.0f));

  const SourceRndPollAnimRateFramePlan ui =
      source_rndpollanim_rate_frame_plan(kSourceRndAnimRate30FpsUi, 1.5f,
                                         2.0f, 3.0f, 4.0f);
  CHECK(ui.recognized);
  CHECK(ui.uses_ui_seconds);
  CHECK(approx(ui.frame, 60.0f));

  const SourceRndPollAnimRateFramePlan one_fpb =
      source_rndpollanim_rate_frame_plan(kSourceRndAnimRate1Fpb, 1.5f,
                                         2.0f, 3.0f, 4.0f);
  CHECK(one_fpb.recognized);
  CHECK(one_fpb.uses_beat);
  CHECK(approx(one_fpb.multiplier, 1.0f));
  CHECK(approx(one_fpb.frame, 4.0f));

  const SourceRndPollAnimRateFramePlan tutorial =
      source_rndpollanim_rate_frame_plan(kSourceRndAnimRate30FpsTutorial,
                                         1.5f, 2.0f, 3.0f, 4.0f);
  CHECK(tutorial.recognized);
  CHECK(tutorial.uses_tutorial_seconds);
  CHECK(approx(tutorial.frame, 90.0f));

  const SourceRndPollAnimRateFramePlan unknown =
      source_rndpollanim_rate_frame_plan(kSourceRndAnimRateUnknown, 1.5f,
                                         2.0f, 3.0f, 4.0f);
  CHECK(!unknown.recognized);
  CHECK(approx(unknown.frame, 0.0f));

  const SourceRndPollAnimPollPlan poll =
      source_rndpollanim_poll_plan(3);
  CHECK(poll.child_count == 3);
  CHECK(poll.calls_set_frame);
  CHECK(approx(poll.blend, 1.0f));
  CHECK(!source_rndpollanim_poll_plan(0).calls_set_frame);

  const SourceRndPollAnimLoadPlan load0 =
      source_rndpollanim_load_plan(0);
  CHECK(load0.accepted_revision);
  CHECK(load0.superclasses.size() == 3);
  CHECK(load0.superclasses[0] == "Hmx::Object");
  CHECK(load0.superclasses[1] == "RndAnimatable");
  CHECK(load0.superclasses[2] == "RndPollable");
  CHECK(load0.reads_anims);
  CHECK(!source_rndpollanim_load_plan(1).accepted_revision);

  const SourceRndPollAnimCopyPlan copy = source_rndpollanim_copy_plan();
  CHECK(copy.superclasses.size() == 3);
  CHECK(copy.copies_anims);

  const SourceRndPollAnimEmptyBodyPlan empty =
      source_rndpollanim_empty_body_plan();
  CHECK(empty.start_anim_empty);
  CHECK(empty.end_anim_empty);
  CHECK(empty.set_frame_empty);

  const SourceRndPollAnimHandlerPlan handlers =
      source_rndpollanim_handler_plan();
  CHECK(handlers.superclasses.size() == 3);
  CHECK(handlers.superclasses[0] == "RndAnimatable");
  CHECK(handlers.superclasses[1] == "RndPollable");
  CHECK(handlers.superclasses[2] == "Hmx::Object");
  CHECK(handlers.check == 0x8B);

  const SourceRndPollAnimPropSyncPlan props =
      source_rndpollanim_prop_sync_plan();
  CHECK(props.props.size() == 1);
  CHECK(props.props[0] == "anims");
  CHECK(props.superclass_order.size() == 2);
  CHECK(props.superclass_order[0] == "RndAnimatable");
  CHECK(props.superclass_order[1] == "RndPollable");
  CHECK(props.returns_animatable_when_handled);
  CHECK(props.falls_back_to_pollable);

  std::printf("  [ok] PollAnim: end=%.1f beat480=%.1f handlers=0x%x\n",
              end_frame.result, fpb.frame, handlers.check);
}

void test_prop_anim() {
  const SourceRndPropAnimDefaultState defaults =
      source_rndpropanim_default_state();
  CHECK(approx(defaults.last_frame, 0.0f));
  CHECK(!defaults.in_set_frame);
  CHECK(!defaults.loop);

  const SourceRndPropAnimLoadPlan rev13 =
      source_rndpropanim_load_plan(13);
  CHECK(rev13.accepted_revision);
  CHECK(rev13.sets_prop_keys_revision);
  CHECK(rev13.superclasses.size() == 2);
  CHECK(rev13.superclasses[0] == "Hmx::Object");
  CHECK(rev13.superclasses[1] == "RndAnimatable");
  CHECK(rev13.captures_last_frame_from_anim_frame);
  CHECK(rev13.removes_existing_keys);
  CHECK(!rev13.uses_pre7_loader);
  CHECK(rev13.reads_key_count);
  CHECK(rev13.reads_key_type_per_entry);
  CHECK(rev13.loads_prop_keys_per_entry);
  CHECK(rev13.reads_loop);

  const SourceRndPropAnimLoadPlan rev6 = source_rndpropanim_load_plan(6);
  CHECK(rev6.accepted_revision);
  CHECK(rev6.uses_pre7_loader);
  CHECK(!rev6.reads_key_count);
  CHECK(!rev6.reads_loop);
  CHECK(!source_rndpropanim_load_plan(14).accepted_revision);

  const SourceRndPropAnimPre7LoadPlan pre0 =
      source_rndpropanim_pre7_load_plan(0);
  CHECK(pre0.reads_legacy_owner_before_count);
  CHECK(pre0.reads_symbol_property);
  CHECK(pre0.reads_float_keys_only);
  CHECK(!pre0.reads_anim_type);
  const SourceRndPropAnimPre7LoadPlan pre3 =
      source_rndpropanim_pre7_load_plan(3);
  CHECK(!pre3.reads_legacy_owner_before_count);
  CHECK(pre3.reads_owner_per_entry);
  CHECK(pre3.reads_dataarray_property);
  CHECK(pre3.reads_anim_type);
  CHECK(pre3.reads_color_keys);
  CHECK(!pre3.reads_object_keys_with_owner_stage);
  const SourceRndPropAnimPre7LoadPlan pre6 =
      source_rndpropanim_pre7_load_plan(6);
  CHECK(pre6.reads_object_keys_with_owner_stage);
  CHECK(pre6.reads_bool_keys);
  CHECK(pre6.reads_quat_keys);

  const SourceRndPropAnimCopyPlan copy = source_rndpropanim_copy_plan();
  CHECK(copy.superclasses.size() == 2);
  CHECK(copy.superclasses[0] == "Hmx::Object");
  CHECK(copy.superclasses[1] == "RndAnimatable");
  CHECK(copy.captures_last_frame_from_get_frame);
  CHECK(copy.removes_existing_keys);
  CHECK(copy.copies_prop_keys);
  CHECK(copy.copies_loop);

  const SourceRndPropAnimFrameBoundsPlan start =
      source_rndpropanim_start_frame_plan({4.0f, -2.0f, 6.0f});
  CHECK(approx(start.result, -2.0f));
  const SourceRndPropAnimFrameBoundsPlan positive_start =
      source_rndpropanim_start_frame_plan({2.0f, 5.0f});
  CHECK(approx(positive_start.result, 0.0f));
  const SourceRndPropAnimFrameBoundsPlan end =
      source_rndpropanim_end_frame_plan({4.0f, -2.0f, 6.0f});
  CHECK(approx(end.result, 6.0f));

  const SourceRndPropAnimAdvanceFramePlan advance_loop =
      source_rndpropanim_advance_frame_plan(true);
  CHECK(advance_loop.loop);
  CHECK(advance_loop.applies_mod_range);
  CHECK(advance_loop.calls_animatable_set_frame);
  CHECK(approx(advance_loop.blend, 1.0f));
  CHECK(!source_rndpropanim_advance_frame_plan(false).applies_mod_range);

  const SourceRndPropAnimSetFramePlan set_frame =
      source_rndpropanim_set_frame_plan(false, 3, 1);
  CHECK(set_frame.enters_set_frame_guard);
  CHECK(set_frame.calls_advance_frame);
  CHECK(set_frame.scans_dir_event_keys);
  CHECK(set_frame.sets_each_key_frame);
  CHECK(set_frame.updates_last_frame);
  CHECK(set_frame.clears_set_frame_guard);
  const SourceRndPropAnimSetFramePlan reentrant =
      source_rndpropanim_set_frame_plan(true, 3, 1);
  CHECK(reentrant.already_in_set_frame);
  CHECK(!reentrant.calls_advance_frame);
  CHECK(!reentrant.sets_each_key_frame);

  CHECK(source_rndpropanim_set_key_plan(4).calls == 4);
  CHECK(source_rndpropanim_start_anim_plan(2).calls == 2);
  CHECK(source_rndpropanim_remove_all_keys_plan(5).calls == 5);

  const SourceRndPropAnimFindKeysPlan null_prop =
      source_rndpropanim_find_keys_plan(true, false, false, true);
  CHECK(null_prop.matches_null_property_row);
  CHECK(null_prop.found);
  const SourceRndPropAnimFindKeysPlan target_match =
      source_rndpropanim_find_keys_plan(false, true, true, false);
  CHECK(target_match.found);
  CHECK(!source_rndpropanim_find_keys_plan(false, true, false, false).found);

  const SourceRndPropAnimChangePropPathPlan remove_hit =
      source_rndpropanim_change_prop_path_plan(true, true);
  CHECK(remove_hit.calls_remove_keys);
  CHECK(remove_hit.result);
  const SourceRndPropAnimChangePropPathPlan remove_miss =
      source_rndpropanim_change_prop_path_plan(true, false);
  CHECK(remove_miss.calls_remove_keys);
  CHECK(!remove_miss.result);
  const SourceRndPropAnimChangePropPathPlan change_hit =
      source_rndpropanim_change_prop_path_plan(false, true);
  CHECK(!change_hit.calls_remove_keys);
  CHECK(change_hit.sets_new_prop);
  CHECK(change_hit.result);

  const SourceRndPropAnimValuePlan quat_index =
      source_rndpropanim_value_from_index_plan(kSourcePropKeysQuat, true, true);
  CHECK(quat_index.result);
  CHECK(quat_index.output_kind == "quat_array");
  const SourceRndPropAnimValuePlan invalid_index =
      source_rndpropanim_value_from_index_plan(kSourcePropKeysFloat, true,
                                               false);
  CHECK(!invalid_index.result);
  CHECK(invalid_index.output_kind == "zero");
  const SourceRndPropAnimValuePlan vec_frame =
      source_rndpropanim_value_from_frame_plan(kSourcePropKeysVector3, true);
  CHECK(vec_frame.result);
  CHECK(vec_frame.output_kind == "vector3_array");
  CHECK(source_rndpropanim_value_from_frame_plan(kSourcePropKeysBool, false)
            .output_kind == "index_-1");

  const SourceRndPropAnimHandlerPlan handlers =
      source_rndpropanim_handler_plan();
  CHECK(handlers.expressions.size() == 6);
  CHECK(handlers.expressions[0] == "remove_keys");
  CHECK(handlers.actions.size() == 6);
  CHECK(handlers.actions[0] == "add_keys");
  CHECK(handlers.handlers.size() == 10);
  CHECK(handlers.handlers[0] == "foreach_target");
  CHECK(handlers.handlers[9] == "value_from_frame");
  CHECK(handlers.superclasses.size() == 2);
  CHECK(handlers.superclasses[0] == "RndAnimatable");
  CHECK(handlers.superclasses[1] == "Hmx::Object");
  CHECK(handlers.check == 0x43C);

  const SourceRndPropAnimPropSyncPlan props =
      source_rndpropanim_prop_sync_plan();
  CHECK(props.props.size() == 1);
  CHECK(props.props[0] == "loop");
  CHECK(props.superclasses.size() == 1);
  CHECK(props.superclasses[0] == "RndAnimatable");

  const SourcePropKeysDefaultState keys_defaults =
      source_propkeys_default_state();
  CHECK(keys_defaults.prop_null);
  CHECK(keys_defaults.trans_null);
  CHECK(keys_defaults.last_key_frame_index == -2);
  CHECK(keys_defaults.keys_type == kSourcePropKeysFloat);
  CHECK(keys_defaults.interpolation == kSourcePropKeysLinear);
  CHECK(keys_defaults.exception_id == kSourcePropKeysNoException);
  CHECK(!keys_defaults.last_bit);

  const SourcePropKeysLoadPlan keys_rev6 =
      source_propkeys_load_plan(6, 1);
  CHECK(!keys_rev6.accepted_revision);
  CHECK(keys_rev6.fails_pre7);
  const SourcePropKeysLoadPlan keys_rev7 =
      source_propkeys_load_plan(7, 1);
  CHECK(keys_rev7.accepted_revision);
  CHECK(keys_rev7.reads_keys_type);
  CHECK(keys_rev7.reads_target);
  CHECK(keys_rev7.reads_prop);
  CHECK(!keys_rev7.reads_interpolation);
  CHECK(keys_rev7.derives_legacy_interpolation);
  const SourcePropKeysLoadPlan keys_rev8 =
      source_propkeys_load_plan(8, 1);
  CHECK(keys_rev8.reads_interpolation);
  CHECK(!keys_rev8.derives_legacy_interpolation);
  const SourcePropKeysLoadPlan keys_rev10 =
      source_propkeys_load_plan(10, 4);
  CHECK(keys_rev10.reads_interp_handler);
  CHECK(keys_rev10.legacy_macro_exception_branch);
  const SourcePropKeysLoadPlan keys_rev11 =
      source_propkeys_load_plan(11, 1);
  CHECK(keys_rev11.reads_exception_id);
  const SourcePropKeysLoadPlan keys_rev13 =
      source_propkeys_load_plan(13, 1);
  CHECK(keys_rev13.reads_last_bit);
  CHECK(keys_rev13.calls_set_prop_exception_id);

  CHECK(source_propkeys_exception_plan("rotation", true, false).exception_id ==
        kSourcePropKeysTransQuat);
  CHECK(source_propkeys_exception_plan("scale", true, false).exception_id ==
        kSourcePropKeysTransScale);
  CHECK(source_propkeys_exception_plan("position", true, false).exception_id ==
        kSourcePropKeysTransPos);
  CHECK(source_propkeys_exception_plan("event", false, true).exception_id ==
        kSourcePropKeysDirEvent);
  CHECK(source_propkeys_exception_plan("event", false, false).exception_id ==
        kSourcePropKeysNoException);

  const SourcePropKeysSetPropExceptionPlan interp_handler =
      source_propkeys_set_prop_exception_plan(
          false, kSourcePropKeysNoException, kSourcePropKeysTransPos);
  CHECK(interp_handler.result_exception == kSourcePropKeysHandleInterp);
  CHECK(!interp_handler.updates_transform_cache);
  const SourcePropKeysSetPropExceptionPlan macro =
      source_propkeys_set_prop_exception_plan(
          true, kSourcePropKeysMacro, kSourcePropKeysTransPos);
  CHECK(macro.result_exception == kSourcePropKeysMacro);
  CHECK(!macro.updates_transform_cache);
  const SourcePropKeysSetPropExceptionPlan trans_pos =
      source_propkeys_set_prop_exception_plan(
          true, kSourcePropKeysNoException, kSourcePropKeysTransPos);
  CHECK(trans_pos.result_exception == kSourcePropKeysTransPos);
  CHECK(trans_pos.updates_transform_cache);

  std::printf("  [ok] PropAnim: load_v13=%d handlers=0x%x value=%s\n",
              rev13.accepted_revision ? 1 : 0, handlers.check,
              quat_index.output_kind.c_str());
}

void test_mat() {
  const SourceRndMatDefaultState defaults = source_rndmat_default_state();
  CHECK(approx(defaults.color[0], 1.0f));
  CHECK(approx(defaults.color[3], 1.0f));
  CHECK(defaults.diffuse_tex_null);
  CHECK(defaults.alpha_threshold == 0);
  CHECK(defaults.next_pass_null);
  CHECK(defaults.emissive_map_null);
  CHECK(approx(defaults.refract_strength, 0.0f));
  CHECK(defaults.refract_normal_map_null);
  CHECK(!defaults.intensify);
  CHECK(defaults.use_environ);
  CHECK(!defaults.prelit);
  CHECK(!defaults.alpha_cut);
  CHECK(!defaults.alpha_write);
  CHECK(defaults.cull);
  CHECK(!defaults.per_pixel_lit);
  CHECK(!defaults.screen_aligned);
  CHECK(!defaults.refract_enabled);
  CHECK(!defaults.point_lights);
  CHECK(!defaults.fog);
  CHECK(!defaults.fadeout);
  CHECK(!defaults.color_adjust);
  CHECK(defaults.blend == 1);
  CHECK(defaults.tex_gen == 0);
  CHECK(defaults.tex_wrap == 1);
  CHECK(defaults.z_mode == 1);
  CHECK(defaults.stencil_mode == 0);
  CHECK(defaults.shader_variation == 0);
  CHECK(defaults.dirty == 3);
  CHECK(approx(defaults.emissive_multiplier, 1.0f));
  CHECK(defaults.tex_xfm_reset);
  CHECK(defaults.color_mod_count == 3);
  CHECK(source_rndmat_save_plan().save_id == 159);

  const SourceMiloEditorRndMatNewPlan mat_new =
      source_milo_editor_rndmat_new_plan(27, 0);
  CHECK(mat_new.revision == 27);
  CHECK(mat_new.alt_revision == 0);
  CHECK(mat_new.sets_revision);
  CHECK(mat_new.sets_alt_revision);
  CHECK(mat_new.relies_on_constructor_defaults);
  CHECK(mat_new.does_not_initialize_render_state_or_textures);

  const SourceMatShaderOptionsDefaultState shader_options =
      source_mat_shader_options_default_state();
  CHECK(!shader_options.temp_mat);
  CHECK(shader_options.pack == 0x12);
  const SourceMatPerfSettingsDefaultState perf_defaults =
      source_mat_perf_settings_default_state();
  CHECK(!perf_defaults.recv_proj_lights);
  CHECK(!perf_defaults.recv_point_cube_tex);
  CHECK(!perf_defaults.ps3_force_trilinear);
  const SourceMatPerfSettingsLoadPlan perf_rev65 =
      source_mat_perf_settings_load_plan(0x41);
  CHECK(perf_rev65.reads_recv_proj_lights);
  CHECK(perf_rev65.reads_ps3_force_trilinear);
  CHECK(!perf_rev65.reads_recv_point_cube_tex);
  CHECK(perf_rev65.read_order.size() == 2);
  CHECK(perf_rev65.read_order[0] == "recv_proj_lights");
  CHECK(perf_rev65.read_order[1] == "ps3_force_trilinear");
  const SourceMatPerfSettingsLoadPlan perf_rev66 =
      source_mat_perf_settings_load_plan(0x42);
  CHECK(perf_rev66.reads_recv_point_cube_tex);
  CHECK(perf_rev66.read_order.size() == 3);
  CHECK(perf_rev66.read_order[2] == "recv_point_cube_tex");

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

  std::vector<uint8_t> legacy;
  put_u32(legacy, 21);           // GH1 RndMat revision
  put_u32(legacy, 1);            // one legacy texture entry
  put_u32(legacy, 2);            // texture slot
  put_u32(legacy, 0);            // diffuse map type
  put_f32(legacy, 1); put_f32(legacy, 0); put_f32(legacy, 0);
  put_f32(legacy, 0); put_f32(legacy, 1); put_f32(legacy, 0);
  put_f32(legacy, 0); put_f32(legacy, 0); put_f32(legacy, 1);
  put_f32(legacy, 0); put_f32(legacy, 0); put_f32(legacy, 0);
  put_u32(legacy, 1);            // repeat
  put_str(legacy, "legacy.tex");
  put_u32(legacy, 2);            // legacy default-texture selector
  put_f32(legacy, 1); put_f32(legacy, 1);
  put_f32(legacy, 1); put_f32(legacy, 1);
  legacy.push_back(0);           // GH1 use-environ
  put_u16(legacy, 0x0101);       // prelit=1, z-mode=normal
  put_u32(legacy, 1);
  put_u16(legacy, 0);
  put_u32(legacy, 3);            // actual kBlendSrcAlpha
  put_u16(legacy, 0);
  const MatObj legacy_mat = decode_mat("legacy.mat", legacy);
  CHECK(legacy_mat.decoded);
  CHECK(legacy_mat.diffuse_tex == "legacy.tex");
  CHECK(!legacy_mat.use_environ);
  CHECK(legacy_mat.prelit);
  CHECK(legacy_mat.z_mode == 1);
  CHECK(legacy_mat.blend == 3);
  CHECK(!legacy_mat.has_render_state);

  auto legacy_selector1 = legacy;
  legacy_selector1[12] = 1;
  const MatObj legacy_selector1_mat =
      decode_mat("legacy_selector1.mat", legacy_selector1);
  CHECK(legacy_selector1_mat.diffuse_tex == "legacy.tex");

  auto legacy_selector5 = legacy;
  legacy_selector5[12] = 5;
  const MatObj legacy_selector5_mat =
      decode_mat("legacy_selector5.mat", legacy_selector5);
  CHECK(legacy_selector5_mat.diffuse_tex.empty());
  CHECK(legacy_selector5_mat.use_environ);

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
  put_f32(b, 0.0f); put_f32(b, 0.0f); put_f32(b, 1.0f);
  put_f32(b, 0.25f); put_f32(b, 0.5f); put_f32(b, 0.0f);
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
  CHECK(approx(m.tex_xfm[0][0], 2.0f));
  CHECK(approx(m.tex_xfm[1][1], 3.0f));
  CHECK(approx(m.tex_xfm[2][0], 0.25f));
  CHECK(approx(m.tex_xfm[2][1], 0.5f));
  CHECK(approx(m.tex_xfm[2][2], 1.0f));
  CHECK(m.diffuse_tex_offset == 0x61);
  CHECK(m.next_pass.empty());
  CHECK(!m.intensify);
  CHECK(m.has_cull);
  CHECK(!m.cull);
  CHECK(approx(m.emissive_multiplier, 1.0f));
  const SourceRndMatAccessorResult accessors = source_rndmat_accessors(m);
  CHECK(accessors.blend == 4);
  CHECK(accessors.z_mode == 2);
  CHECK(accessors.tex_wrap == 4);
  CHECK(accessors.diffuse_tex == "gem.tex");
  CHECK(accessors.next_pass.empty());
  CHECK(approx(accessors.alpha, 1.0f));

  const SourceRndMatSetterPlan set_alpha =
      source_rndmat_setter_plan("SetAlpha");
  CHECK(set_alpha.writes_member);
  CHECK(set_alpha.writes_alpha_only);
  CHECK(set_alpha.dirty_or_mask == 1);
  const SourceRndMatSetterPlan set_blend =
      source_rndmat_setter_plan("SetBlend");
  CHECK(set_blend.writes_member);
  CHECK(!set_blend.writes_alpha_only);
  CHECK(set_blend.dirty_or_mask == 2);
  const SourceRndMatSetterPlan set_threshold =
      source_rndmat_setter_plan("SetAlphaThreshold");
  CHECK(set_threshold.writes_member);
  CHECK(set_threshold.dirty_or_mask == 0);
  const SourceRndMatSetterPlan set_color =
      source_rndmat_setter_plan("SetColor");
  CHECK(set_color.writes_member);
  CHECK(set_color.writes_rgb_only);
  CHECK(set_color.dirty_or_mask == 1);
  const SourceRndMatColorModPlan color_mod_valid =
      source_rndmat_set_color_mod_plan(2);
  CHECK(!color_mod_valid.assertion_would_fail);
  CHECK(color_mod_valid.writes_color_mod);
  CHECK(color_mod_valid.dirty_or_mask == 2);
  const SourceRndMatColorModPlan color_mod_invalid =
      source_rndmat_set_color_mod_plan(3);
  CHECK(color_mod_invalid.assertion_would_fail);
  CHECK(!color_mod_invalid.writes_color_mod);
  const SourceRndMatRefractEnabledPlan refract_forced =
      source_rndmat_get_refract_enabled_plan(true, 0.5f, true, true, false);
  CHECK(refract_forced.base_gate);
  CHECK(refract_forced.frame_gate);
  CHECK(refract_forced.result);
  const SourceRndMatRefractEnabledPlan refract_frame =
      source_rndmat_get_refract_enabled_plan(true, 0.5f, true, false, true);
  CHECK(refract_frame.result);
  const SourceRndMatRefractEnabledPlan refract_no_frame =
      source_rndmat_get_refract_enabled_plan(true, 0.5f, true, false, false);
  CHECK(refract_no_frame.base_gate);
  CHECK(!refract_no_frame.frame_gate);
  CHECK(!refract_no_frame.result);
  const SourceRndMatRefractEnabledPlan refract_no_map =
      source_rndmat_get_refract_enabled_plan(true, 0.5f, false, true, true);
  CHECK(!refract_no_map.base_gate);
  CHECK(!refract_no_map.result);
  const SourceRndMatRefractAccessorPlan refract_accessors =
      source_rndmat_refract_accessor_plan();
  CHECK(refract_accessors.returns_normal_map);
  CHECK(refract_accessors.returns_strength);
  const SourceRndMatIsNextPassPlan is_next_pass =
      source_rndmat_is_next_pass_plan({"hair_pass1.mat", "hair_pass2.mat"},
                                      "hair_pass2.mat");
  CHECK(is_next_pass.walks_next_pass_chain);
  CHECK(is_next_pass.found);
  const SourceRndMatIsNextPassPlan is_not_next_pass =
      source_rndmat_is_next_pass_plan({"hair_pass1.mat", "hair_pass2.mat"},
                                      "body.mat");
  CHECK(!is_not_next_pass.found);
  const SourceRndMatAllowedNextPassPlan allowed_next_pass =
      source_rndmat_allowed_next_pass_plan(
          {"hair_pass1.mat", "hair_pass2.mat", "body.mat"},
          "hair_pass1.mat", {"hair_pass1.mat", "hair_pass2.mat"});
  CHECK(allowed_next_pass.mat_count == 3);
  CHECK(allowed_next_pass.allocated_node_count == 5);
  CHECK(allowed_next_pass.node0_is_null);
  CHECK(allowed_next_pass.preserves_current_next_pass);
  CHECK(allowed_next_pass.excludes_recursive_next_passes);
  CHECK(allowed_next_pass.resized_node_count == 3);
  CHECK(allowed_next_pass.allowed_order[0] == "<null>");
  CHECK(allowed_next_pass.allowed_order[1] == "hair_pass1.mat");
  CHECK(allowed_next_pass.allowed_order[2] == "body.mat");
  const SourceRndMatAllowedNormalMapPlan allowed_normal_map =
      source_rndmat_allowed_normal_map_plan();
  CHECK(allowed_normal_map.uses_directory);
  CHECK(allowed_normal_map.calls_get_normal_map_textures);
  CHECK(allowed_normal_map.returns_data_node);
  const SourceRndMatHandlerPlan mat_handlers = source_rndmat_handler_plan();
  CHECK(mat_handlers.handlers.size() == 2);
  CHECK(mat_handlers.handlers[0] == "allowed_next_pass");
  CHECK(mat_handlers.handlers[1] == "allowed_normal_map");
  CHECK(mat_handlers.superclasses.size() == 1);
  CHECK(mat_handlers.superclasses[0] == "Hmx::Object");
  CHECK(mat_handlers.check == 0x305);
  const SourceRndMatCopyPlan copy_from_max =
      source_rndmat_copy_plan(true);
  CHECK(copy_from_max.asserts_source_mat);
  CHECK(copy_from_max.copies_object_superclass);
  CHECK(copy_from_max.copies_diffuse_tex);
  CHECK(!copy_from_max.copies_other_material_members);
  CHECK(copy_from_max.dirty_value == 3);
  const SourceRndMatCopyPlan normal_copy =
      source_rndmat_copy_plan(false);
  CHECK(!normal_copy.copies_diffuse_tex);
  CHECK(!normal_copy.copies_other_material_members);
  CHECK(normal_copy.dirty_value == 3);
  const SourceRndMatPropSyncPlan prop_sync =
      source_rndmat_prop_sync_plan();
  CHECK(prop_sync.dirty_color_rows.size() == 2);
  CHECK(prop_sync.dirty_color_rows[0] == "color");
  CHECK(prop_sync.dirty_color_rows[1] == "alpha");
  CHECK(prop_sync.color_dirty_or_mask == 1);
  CHECK(prop_sync.dirty_render_rows.size() == 22);
  CHECK(prop_sync.dirty_render_rows[0] == "intensify");
  CHECK(prop_sync.dirty_render_rows[10] == "prelit");
  CHECK(prop_sync.dirty_render_rows[16] == "emissive_multiplier");
  CHECK(prop_sync.dirty_render_rows[21] == "screen_aligned");
  CHECK(prop_sync.render_dirty_or_mask == 2);
  CHECK(prop_sync.direct_no_dirty_rows.size() == 5);
  CHECK(prop_sync.direct_no_dirty_rows[0] == "next_pass");
  CHECK(prop_sync.direct_no_dirty_rows[4] == "color_adjust");
  CHECK(prop_sync.perf_setting_rows.size() == 3);
  CHECK(prop_sync.perf_setting_rows[0] == "recv_proj_lights");
  CHECK(prop_sync.perf_setting_rows[2] == "ps3_force_trilinear");
  CHECK(prop_sync.custom_bit_rows_skip_size_or_get_dirty);
  std::printf("  [ok] Mat: tex=%s blend=%u alphaCut=%d zMode=%u texWrap=%u cull=%d color=(%.0f,%.0f,%.0f,%.0f)\n",
              m.diffuse_tex.c_str(), static_cast<unsigned>(m.blend),
              m.alpha_cut ? 1 : 0, static_cast<unsigned>(m.z_mode),
              static_cast<unsigned>(m.tex_wrap), m.cull ? 1 : 0,
              m.color[0], m.color[1], m.color[2], m.color[3]);
}

void test_wind() {
  const SourceRndWindDefaultState defaults = source_rndwind_default_state();
  CHECK(approx(defaults.prevailing[0], 0.0f));
  CHECK(approx(defaults.random[0], 0.0f));
  CHECK(approx(defaults.time_loop, 100.0f));
  CHECK(approx(defaults.space_loop, 100.0f));
  CHECK(defaults.wind_owner_self);
  CHECK(defaults.calls_sync_loops);
  CHECK(source_rndwind_save_plan().save_id == 0x96);

  const SourceRndWindSetDefaultsPlan set_defaults =
      source_rndwind_set_defaults_plan();
  CHECK(approx(set_defaults.prevailing[0], 0.0f));
  CHECK(approx(set_defaults.random[0], 17.0f));
  CHECK(approx(set_defaults.random[1], 17.0f));
  CHECK(approx(set_defaults.random[2], 0.0f));
  CHECK(approx(set_defaults.time_loop, 100.0f));
  CHECK(!set_defaults.calls_sync_loops);

  const SourceRndWindZeroPlan zero = source_rndwind_zero_plan();
  CHECK(zero.zeroes_prevailing);
  CHECK(zero.zeroes_random);
  CHECK(zero.leaves_time_loop);
  CHECK(zero.leaves_space_loop);
  CHECK(zero.leaves_wind_owner);
  CHECK(!zero.calls_sync_loops);

  const SourceRndWindLoopRatePlan rates =
      source_rndwind_sync_loops(100.0f, 50.0f);
  CHECK(!rates.time_loop_zero);
  CHECK(!rates.space_loop_zero);
  CHECK(approx(rates.time_rate[0], 0.01f));
  CHECK(approx(rates.time_rate[1], 0.01f * 0.773437f));
  CHECK(approx(rates.time_rate[2], 0.01f * 1.38484f));
  CHECK(approx(rates.space_rate[0], 0.02f));
  CHECK(approx(rates.space_rate[1], 0.02f * 0.773437f));
  CHECK(approx(rates.space_rate[2], 0.02f * 1.38484f));
  const SourceRndWindLoopRatePlan zero_rates =
      source_rndwind_sync_loops(0.0f, 0.0f);
  CHECK(zero_rates.time_loop_zero);
  CHECK(zero_rates.space_loop_zero);
  CHECK(approx(zero_rates.time_rate[0], 0.0f));
  CHECK(approx(zero_rates.space_rate[2], 0.0f));

  const SourceRndWindLoadPlan load_v1 = source_rndwind_load_plan(1);
  CHECK(load_v1.accepted_revision);
  CHECK(load_v1.reads_object_fields);
  CHECK(load_v1.reads_prevailing);
  CHECK(load_v1.reads_random);
  CHECK(load_v1.reads_time_loop);
  CHECK(load_v1.reads_space_loop);
  CHECK(!load_v1.reads_wind_owner);
  CHECK(!load_v1.calls_set_wind_owner);
  CHECK(load_v1.calls_sync_loops);
  const SourceRndWindLoadPlan load_v2 = source_rndwind_load_plan(2);
  CHECK(load_v2.reads_wind_owner);
  CHECK(load_v2.calls_set_wind_owner);
  const SourceRndWindLoadPlan load_v3 = source_rndwind_load_plan(3);
  CHECK(!load_v3.accepted_revision);

  const SourceRndWindSetOwnerPlan owner_null =
      source_rndwind_set_owner_plan(false);
  CHECK(owner_null.assigns_self);
  CHECK(!owner_null.assigns_input_owner);
  const SourceRndWindSetOwnerPlan owner_external =
      source_rndwind_set_owner_plan(true);
  CHECK(owner_external.assigns_input_owner);
  CHECK(!owner_external.assigns_self);

  const SourceRndWindCopyPlan shallow = source_rndwind_copy_plan(true);
  CHECK(shallow.copy_shallow);
  CHECK(shallow.copies_object_superclass);
  CHECK(shallow.shallow_copies_wind_owner);
  CHECK(!shallow.copies_prevailing);
  CHECK(!shallow.calls_sync_loops);
  const SourceRndWindCopyPlan deep = source_rndwind_copy_plan(false);
  CHECK(!deep.copy_shallow);
  CHECK(deep.resets_wind_owner_to_self);
  CHECK(deep.copies_wind_owner);
  CHECK(deep.copies_prevailing);
  CHECK(deep.copies_random);
  CHECK(deep.copies_time_loop);
  CHECK(deep.copies_space_loop);
  CHECK(deep.calls_sync_loops);

  const SourceRndWindReplacePlan replace_unrelated =
      source_rndwind_replace_plan(false, true);
  CHECK(replace_unrelated.calls_object_replace);
  CHECK(!replace_unrelated.calls_set_wind_owner);
  const SourceRndWindReplacePlan replace_with_wind =
      source_rndwind_replace_plan(true, true);
  CHECK(replace_with_wind.calls_set_wind_owner);
  CHECK(replace_with_wind.assigns_replacement_wind);
  CHECK(!replace_with_wind.assigns_self);
  const SourceRndWindReplacePlan replace_with_nonwind =
      source_rndwind_replace_plan(true, false);
  CHECK(replace_with_nonwind.calls_set_wind_owner);
  CHECK(!replace_with_nonwind.assigns_replacement_wind);
  CHECK(replace_with_nonwind.assigns_self);

  const SourceRndWindRuntimeBoundary runtime =
      source_rndwind_runtime_boundary();
  CHECK(runtime.vector_get_wind_delegates_to_owner);
  CHECK(runtime.scalar_get_wind_declared);
  CHECK(!runtime.scalar_get_wind_body_visible);
  CHECK(runtime.self_get_wind_declared);
  CHECK(!runtime.self_get_wind_body_visible);
  CHECK(!runtime.native_generates_wind_force);

  const SourceRndWindHandlerPlan handlers = source_rndwind_handler_plan();
  CHECK(handlers.actions.size() == 2);
  CHECK(handlers.actions[0] == "set_defaults");
  CHECK(handlers.actions[1] == "set_zero");
  CHECK(handlers.superclasses.size() == 1);
  CHECK(handlers.superclasses[0] == "Hmx::Object");
  CHECK(handlers.check == 0xda);
  const SourceRndWindPropSyncPlan props = source_rndwind_prop_sync_plan();
  CHECK(props.direct_rows.size() == 2);
  CHECK(props.direct_rows[0] == "prevailing");
  CHECK(props.direct_rows[1] == "random");
  CHECK(props.set_rows.size() == 1);
  CHECK(props.set_rows[0] == "wind_owner");
  CHECK(props.modify_rows.size() == 2);
  CHECK(props.modify_rows[0] == "time_loop");
  CHECK(props.modify_rows[1] == "space_loop");
  CHECK(props.loop_rows_call_sync_loops);

  std::printf("  [ok] Wind: save=0x%x timeRate=(%.4f,%.4f,%.4f) ownerSelf=%d\n",
              source_rndwind_save_plan().save_id, rates.time_rate[0],
              rates.time_rate[1], rates.time_rate[2],
              defaults.wind_owner_self ? 1 : 0);
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

  const SourceRndAnimatableDefaultState anim_defaults =
      source_rndanimatable_default_state();
  CHECK(approx(anim_defaults.frame, 0.0f));
  CHECK(anim_defaults.rate == kSourceRndAnimRate30Fps);

  const SourceRndAnimatableRatePlan anim_30fps =
      source_rndanimatable_rate_plan(kSourceRndAnimRate30Fps);
  CHECK(anim_30fps.valid_rate);
  CHECK(anim_30fps.task_units == "seconds");
  CHECK(approx(anim_30fps.frames_per_unit, 30.0f));
  const SourceRndAnimatableRatePlan anim_480fpb =
      source_rndanimatable_rate_plan(kSourceRndAnimRate480Fpb);
  CHECK(anim_480fpb.valid_rate);
  CHECK(anim_480fpb.task_units == "beats");
  CHECK(approx(anim_480fpb.frames_per_unit, 480.0f));
  const SourceRndAnimatableRatePlan anim_1fpb =
      source_rndanimatable_rate_plan(kSourceRndAnimRate1Fpb);
  CHECK(anim_1fpb.valid_rate);
  CHECK(anim_1fpb.task_units == "beats");
  CHECK(approx(anim_1fpb.frames_per_unit, 1.0f));

  const SourceRndAnimatableConvertFramesPlan convert_seconds =
      source_rndanimatable_convert_frames_plan(kSourceRndAnimRate30Fps, 60.0f);
  CHECK(approx(convert_seconds.output_units, 2.0f));
  CHECK(convert_seconds.returns_converted);
  const SourceRndAnimatableConvertFramesPlan convert_beats =
      source_rndanimatable_convert_frames_plan(kSourceRndAnimRate480Fpb,
                                               960.0f);
  CHECK(approx(convert_beats.output_units, 2.0f));
  CHECK(!convert_beats.returns_converted);

  const SourceRndAnimatableCopyPlan anim_copy =
      source_rndanimatable_copy_plan();
  CHECK(anim_copy.requires_animatable_source);
  CHECK(anim_copy.copies_frame);
  CHECK(anim_copy.copies_rate);
  CHECK(anim_copy.ignores_non_animatable_source);

  const SourceRndAnimatableHandlerPlan anim_handlers =
      source_rndanimatable_handler_plan();
  CHECK(anim_handlers.handlers.size() == 9);
  CHECK(anim_handlers.handlers[0] == "set_frame");
  CHECK(anim_handlers.handlers[5] == "animate");
  CHECK(anim_handlers.handlers[8] == "convert_frames");
  CHECK(anim_handlers.check == 0x16C);
  const SourceRndAnimatablePropSyncPlan anim_props =
      source_rndanimatable_prop_sync_plan();
  CHECK(anim_props.props.size() == 2);
  CHECK(anim_props.props[0] == "rate");
  CHECK(anim_props.props[1] == "frame:SetFrame");

  const SourceRndAnimatableAnimatePlan on_animate =
      source_rndanimatable_on_animate_plan();
  CHECK(on_animate.defaults.size() == 9);
  CHECK(on_animate.defaults[0] == "blend=0");
  CHECK(on_animate.defaults[5] == "period=FramesPerUnit");
  CHECK(on_animate.data_keys.size() == 5);
  CHECK(on_animate.mode_rows.size() == 4);
  CHECK(on_animate.mode_rows[2] == "dest:current-frame-to-dest/no-loop");
  CHECK(on_animate.creates_anim_task);
  CHECK(on_animate.named_task_requires_data_this);
  CHECK(on_animate.wait_requires_same_rate);
  CHECK(on_animate.starts_task_manager);

  const SourceAnimTaskInitPlan forward_task =
      source_anim_task_init_plan(2.0f, 8.0f, 30.0f, false, 0.0f, false);
  CHECK(approx(forward_task.min_frame, 2.0f));
  CHECK(approx(forward_task.max_frame, 8.0f));
  CHECK(approx(forward_task.scale, 30.0f));
  CHECK(approx(forward_task.offset, 2.0f));
  CHECK(forward_task.calls_start_anim);
  const SourceAnimTaskInitPlan reverse_blend_task =
      source_anim_task_init_plan(8.0f, 2.0f, 30.0f, true, 0.25f, true);
  CHECK(reverse_blend_task.loop);
  CHECK(approx(reverse_blend_task.scale, -30.0f));
  CHECK(approx(reverse_blend_task.offset, 8.0f));
  CHECK(reverse_blend_task.marks_blend_task_when_blending);
  const SourceAnimTaskTimePlan forward_time =
      source_anim_task_time_until_end_plan(2.0f, 8.0f, 5.0f, 30.0f,
                                           30.0f);
  CHECK(approx(forward_time.time_until_end, 0.1f));
  const SourceAnimTaskTimePlan reverse_time =
      source_anim_task_time_until_end_plan(2.0f, 8.0f, 5.0f, 30.0f,
                                           -30.0f);
  CHECK(approx(reverse_time.time_until_end, 0.1f));

  const SourceRndDrawableLoadPlan drawable_v3 =
      source_rnddrawable_load_plan(3, 24);
  CHECK(drawable_v3.accepted_revision);
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
  CHECK(!drawable_v4.accepted_revision);
  CHECK(!drawable_v4.reads_showing);
  CHECK(!drawable_v4.reads_clip_planes);

  const SourceRndDrawableDefaultState drawable_defaults =
      source_rnddrawable_default_state();
  CHECK(drawable_defaults.showing);
  CHECK(drawable_defaults.sphere_zeroed);
  CHECK(approx(drawable_defaults.order, 0.0f));
  CHECK(drawable_defaults.draw_revision == 3);
  CHECK(drawable_defaults.highlight_style_count == 5);
  CHECK(approx(drawable_defaults.normal_display_length, 1.0f));

  const SourceMiloEditorRndDrawableNewPlan drawable_new =
      source_milo_editor_rnddrawable_new_plan(3, 1);
  CHECK(drawable_new.revision == 3);
  CHECK(drawable_new.alt_revision == 1);
  CHECK(drawable_new.sets_revision);
  CHECK(drawable_new.sets_alt_revision);
  CHECK(drawable_new.relies_on_constructor_defaults);
  CHECK(drawable_new.does_not_initialize_sphere_or_draw_order);

  CHECK(source_rnddrawable_save_plan().save_id == 0xAE);

  const SourceRndDrawableDrawPlan draw_visible =
      source_rnddrawable_draw_plan(true, true, false);
  CHECK(draw_visible.calls_make_world_sphere);
  CHECK(draw_visible.calls_draw_showing);
  CHECK(!source_rnddrawable_draw_plan(true, true, true).calls_draw_showing);
  CHECK(!source_rnddrawable_draw_plan(false, true, false)
             .calls_make_world_sphere);

  const SourceRndDrawableBudgetPlan budget_culled =
      source_rnddrawable_budget_plan(true, true, true);
  CHECK(budget_culled.calls_make_world_sphere);
  CHECK(!budget_culled.calls_draw_showing_budget);
  CHECK(source_rnddrawable_budget_plan(true, false, false)
            .calls_draw_showing_budget);

  const SourceRndDrawableCopyPlan drawable_copy =
      source_rnddrawable_copy_plan(false, false, false);
  CHECK(drawable_copy.normal_members.size() == 3);
  CHECK(drawable_copy.normal_members[0] == "mShowing");
  CHECK(drawable_copy.normal_members[2] == "mSphere");
  CHECK(source_rnddrawable_copy_plan(true, true, true)
            .from_max_members.size() == 1);
  CHECK(source_rnddrawable_copy_plan(true, true, false)
            .from_max_members.empty());

  const SourceRndDrawableCollidePlan collide_inside =
      source_rnddrawable_collide_plan(true, true, true, 0.25f, 1.0f);
  CHECK(collide_inside.collide_sphere_result);
  CHECK(collide_inside.collide_calls_showing);
  CHECK(collide_inside.collide_plane_result == 0);
  CHECK(source_rnddrawable_collide_plan(true, true, false, 2.0f, 1.0f)
            .collide_plane_result == 1);
  CHECK(source_rnddrawable_collide_plan(true, true, true, -2.0f, 1.0f)
            .collide_plane_result == -1);

  const SourceRndDrawableHandlerPlan drawable_handlers =
      source_rnddrawable_handler_plan();
  CHECK(drawable_handlers.handlers.size() == 6);
  CHECK(drawable_handlers.handlers[0] == "set_showing");
  CHECK(drawable_handlers.handlers[5] == "copy_sphere");
  CHECK(drawable_handlers.check == 0x168);
  const SourceRndDrawablePropSyncPlan drawable_props =
      source_rnddrawable_prop_sync_plan();
  CHECK(drawable_props.properties.size() == 3);
  CHECK(drawable_props.properties[1] == "showing");
  CHECK(drawable_props.showing_ops.size() == 2);

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
  CHECK(source_rndgroup_save_plan().save_id == 0x30);

  const SourceMiloEditorRndGroupNewPlan group_new =
      source_milo_editor_rndgroup_new_plan(15, 0);
  CHECK(group_new.revision == 15);
  CHECK(group_new.alt_revision == 0);
  CHECK(group_new.sets_revision);
  CHECK(group_new.sets_alt_revision);
  CHECK(group_new.relies_on_constructor_defaults);
  CHECK(group_new.does_not_initialize_membership_or_lod);

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

  std::vector<uint8_t> legacy_drawable;
  put_u32(legacy_drawable, 15);              // RndGroup revision
  put_zeros(legacy_drawable, 9);             // Hmx::Object fields
  put_u32(legacy_drawable, 4);               // RndAnimatable revision
  put_f32(legacy_drawable, 0.0f);            // frame
  put_u32(legacy_drawable, 0);               // rate
  put_u32(legacy_drawable, 9);               // RndTrans revision
  put_matrix(legacy_drawable, 0, 0, 0);
  put_matrix(legacy_drawable, 0, 0, 0);
  put_u32(legacy_drawable, 0);               // constraint
  put_str(legacy_drawable, "");              // target
  legacy_drawable.push_back(0);              // preserve_scale
  put_str(legacy_drawable, "");              // parent
  put_u32(legacy_drawable, 1);               // RndDrawable revision
  legacy_drawable.push_back(1);              // showing
  put_u32(legacy_drawable, 1);               // old drawable list count
  put_utf8_z(legacy_drawable, "legacy_draw_child");
  put_zeros(legacy_drawable, 16);            // sphere
  put_u32(legacy_drawable, 0);               // objects
  put_str(legacy_drawable, "");              // environ for rev < 16
  put_str(legacy_drawable, "");              // drawOnly for rev > 12
  put_str(legacy_drawable, "");              // legacy lod group
  put_f32(legacy_drawable, 0.0f);            // lod screen size
  legacy_drawable.push_back(0);              // sortInWorld

  GroupObj old_draw = decode_group("legacy_draw.grp", legacy_drawable, 6);
  CHECK(old_draw.decoded);
  CHECK(old_draw.children.empty());
  CHECK(!old_draw.sort_in_world);
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
  put_zeros(b, 9);               // Hmx::Object metadata
  put_u32(b, 9);                 // RndTransformable revision
  put_matrix(b, 10.0f, 20.0f, 30.0f);
  put_matrix(b, 40.0f, 50.0f, 60.0f);
  put_u32(b, 0);                 // constraint
  put_str(b, "");                // target
  b.push_back(0);                // preserve_scale
  put_str(b, "stage_root.grp");  // parent
  put_f32(b, 0.1f); put_f32(b, 0.2f); put_f32(b, 0.3f); put_f32(b, 1.0f);
  put_f32(b, 500.0f);
  put_u32(b, 1);                 // kLightDirectional
  b.push_back(1);                // animate_color_from_preset
  b.push_back(0);                // animate_position_from_preset

  LightObj light = decode_light("stage_light_02.lit", b);
  CHECK(light.decoded);
  CHECK(light.source_order_decoded);
  CHECK(light.parent == "stage_root.grp");
  CHECK(approx(light.world_stored.pos[0], 40.0f));
  CHECK(approx(light.color[2], 0.3f));
  CHECK(approx(light.range, 500.0f));
  CHECK(light.type == 1);
  CHECK(light.animate_color_from_preset);
  CHECK(!light.animate_position_from_preset);
  CHECK(light.animate_range_from_preset);
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
  put_f32(b, 0.25f); put_f32(b, 0.5f); put_f32(b, 0.75f); put_f32(b, 1.0f);
  put_f32(b, 0.0f);
  put_f32(b, 1.0f);
  put_f32(b, 1.0f); put_f32(b, 1.0f); put_f32(b, 1.0f); put_f32(b, 1.0f);
  b.push_back(0);                 // fog_enable
  b.push_back(1);                 // animate_from_preset
  b.push_back(1);                 // fade_out
  put_f32(b, 120.0f);             // fade_start
  put_f32(b, 1000.0f);            // fade_end

  EnvironObj env = decode_environ("stage.env", b);
  CHECK(env.decoded);
  CHECK(env.source_order_decoded);
  CHECK(env.revision == 5);
  CHECK(env.lights.size() == 2);
  CHECK(env.lights[0] == "stage_light_02.lit");
  CHECK(approx(env.color_a[0], 0.25f));
  CHECK(approx(env.color_a[2], 0.75f));
  CHECK(!env.fog_enabled);
  CHECK(env.animate_from_preset);
  CHECK(env.fade_out);
  CHECK(approx(env.fade_start, 120.0f));
  CHECK(approx(env.fade_end, 1000.0f));
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
  put_f32(b, 0.30f); put_f32(b, 0.30f); put_f32(b, 0.30f); put_f32(b, 1.0f);
  put_f32(b, 250.0f);
  put_f32(b, 1.0f);
  put_f32(b, 1.0f); put_f32(b, 1.0f); put_f32(b, 1.0f); put_f32(b, 1.0f);
  b.push_back(0);                 // fog_enable
  b.push_back(1);                 // animate_from_preset
  b.push_back(0);                 // fade_out
  put_f32(b, 300.0f);             // fade_start
  put_f32(b, 1000.0f);            // fade_end

  EnvironObj env = decode_environ("curtain_light", b);
  CHECK(env.decoded);
  CHECK(env.source_order_decoded);
  CHECK(env.lights.size() == 1);
  CHECK(env.lights[0] == "curtain");
  CHECK(approx(env.color_a[0], 0.30f));
  CHECK(approx(env.range_a, 250.0f));
  CHECK(approx(env.fog_start, 250.0f));
  CHECK(env.animate_from_preset);
  CHECK(!env.fade_out);
  CHECK(approx(env.fade_start, 300.0f));
  CHECK(approx(env.fade_end, 1000.0f));
  CHECK(approx(env.range, 1000.0f));
  std::printf("  [ok] Environ extensionless light: %s -> %s\n",
              env.name.c_str(), env.lights[0].c_str());
}

void test_environ_with_fog() {
  std::vector<uint8_t> b;
  put_u32(b, 5);                 // Environ version
  put_zeros(b, 9);               // base metadata
  put_u32(b, 0);                 // dynamic light ref count
  put_f32(b, 0.07f); put_f32(b, 0.04f); put_f32(b, 0.14f); put_f32(b, 1.0f);
  put_f32(b, 0.0f);              // fog_start
  put_f32(b, 3000.0f);           // fog_end
  put_f32(b, 0.5f); put_f32(b, 0.0f); put_f32(b, 0.5f); put_f32(b, 1.0f);
  b.push_back(1);                 // fog_enable
  b.push_back(0);                 // animate_from_preset
  b.push_back(1);                 // fade_out
  put_f32(b, 10.0f);              // fade_start
  put_f32(b, 1000.0f);            // fade_end

  EnvironObj env = decode_environ("op_Art_projection.env", b);
  CHECK(env.decoded);
  CHECK(env.source_order_decoded);
  CHECK(env.fog_enabled);
  CHECK(!env.animate_from_preset);
  CHECK(approx(env.fog_start, 0.0f));
  CHECK(approx(env.fog_end, 3000.0f));
  CHECK(approx(env.fog_color[0], 0.5f));
  CHECK(approx(env.fog_color[2], 0.5f));
  CHECK(env.fade_out);
  CHECK(approx(env.fade_start, 10.0f));
  std::printf("  [ok] Environ fog: %s start=%.0f end=%.0f\n",
              env.name.c_str(), env.fog_start, env.fog_end);
}

void test_spotlight_source_order_rev20() {
  std::vector<uint8_t> b;
  put_u32(b, 20);                // GH2 PS2 Spotlight revision.
  put_zeros(b, 9);               // Hmx::Object metadata.
  put_u32(b, 3);                 // RndDrawable revision.
  b.push_back(1);                // showing.
  put_f32(b, 0.0f); put_f32(b, 0.0f); put_f32(b, 0.0f); put_f32(b, 1.0f);
  put_f32(b, 2.0f);              // draw order.
  put_u32(b, 9);                 // RndTransformable revision.
  put_matrix(b, 10.0f, 20.0f, 30.0f);
  put_matrix(b, 40.0f, 50.0f, 60.0f);
  put_u32(b, 0);                 // constraint.
  put_str(b, "");                // trans target.
  b.push_back(0);                // preserve_scale.
  put_str(b, "lighting_root.grp");
  put_f32(b, 1.5f);              // spot_scale.
  put_f32(b, 2.5f);              // spot_height.
  put_u32(b, 3);                 // pre-rev23 ObjVector<BeamDef> count.
  b.push_back(1);                // BeamDef::mIsCone.
  put_f32(b, 100.0f);            // length.
  put_f32(b, 10.0f);             // bottom_radius.
  put_f32(b, 4.0f);              // top_radius.
  put_f32(b, 0.2f);              // top_side_border.
  put_f32(b, 0.3f);              // bottom_side_border.
  put_f32(b, 0.4f);              // bottom_border.
  put_str(b, "beam.mat");
  put_f32(b, 0.75f);             // beam offset.
  put_f32(b, 1.0f); put_f32(b, 2.0f);  // target offset.
  b.push_back(0);                // extra beam consumed, but not selected.
  put_f32(b, 200.0f);
  put_f32(b, 20.0f);
  put_f32(b, 8.0f);
  put_f32(b, 0.2f);
  put_f32(b, 0.3f);
  put_f32(b, 0.4f);
  put_str(b, "beam_second.mat");
  put_f32(b, 0.80f);
  put_f32(b, 3.0f); put_f32(b, 4.0f);
  b.push_back(0);                // extra beam consumed, but not selected.
  put_f32(b, 300.0f);
  put_f32(b, 30.0f);
  put_f32(b, 12.0f);
  put_f32(b, 0.2f);
  put_f32(b, 0.3f);
  put_f32(b, 0.4f);
  put_str(b, "beam_third.mat");
  put_f32(b, 0.85f);
  put_f32(b, 5.0f); put_f32(b, 6.0f);
  put_str(b, "lightcan.grp");
  put_str(b, "bone_pelvis.mesh");
  put_f32(b, 3.25f);             // light_can_offset.
  put_f32(b, 0.1f); put_f32(b, 0.2f); put_f32(b, 0.3f); put_f32(b, 0.4f);
  put_f32(b, 0.8f);              // intensity.
  put_str(b, "spot_circle.mat"); // disc material.
  put_f32(b, 0.6f);              // damping_constant.
  put_str(b, "legacy_symbol");
  put_str(b, "flare.mat");
  put_f32(b, 5.0f); put_f32(b, 6.0f);  // flare sizes.
  put_f32(b, 7.0f); put_f32(b, 8.0f);  // flare range.
  put_u32(b, 9);                 // flare steps.
  put_f32(b, 1.25f);             // flare offset.
  b.push_back(1);                // flare enabled.
  b.push_back(0);                // flare visibility test.
  put_f32(b, 11.0f);             // lens size.
  put_f32(b, 12.0f);             // lens offset.
  put_str(b, "lens.mat");
  put_u32(b, 2);                 // additional objects.
  put_str(b, "SPOT_circle.mesh");
  put_str(b, "beam_instance.mesh");
  b.push_back(1);                // target shadow.
  b.push_back(0);                // animate_color_from_preset.

  SpotlightObj spot = decode_spotlight("SHADOW_solo.spot", b);
  CHECK(spot.decoded);
  CHECK(spot.source_order_decoded);
  CHECK(spot.revision == 20);
  CHECK(spot.draw_revision == 3);
  CHECK(spot.trans_revision == 9);
  CHECK(spot.parent == "lighting_root.grp");
  CHECK(spot.group == "lightcan.grp");
  CHECK(spot.target == "bone_pelvis.mesh");
  CHECK(spot.material == "beam.mat");
  CHECK(spot.disc_material == "spot_circle.mat");
  CHECK(spot.circle_material == "spot_circle.mat");
  CHECK(spot.flare_material == "flare.mat");
  CHECK(spot.lens_material == "lens.mat");
  CHECK(spot.circle_mesh == "SPOT_circle.mesh");
  CHECK(spot.instance_meshes.size() == 2);
  CHECK(spot.instance_meshes[1] == "beam_instance.mesh");
  CHECK(spot.has_default_state);
  CHECK(approx(spot.default_color[1], 0.2f));
  CHECK(approx(spot.default_intensity, 0.8f));
  CHECK(approx(spot.world_stored.pos[0], 40.0f));
  CHECK(approx(spot.beam_length, 100.0f));
  CHECK(approx(spot.spot_scale, 1.5f));
  CHECK(approx(spot.light_can_offset, 3.25f));
  CHECK(spot.flare_steps == 9);
  CHECK(spot.flare_enabled);
  CHECK(!spot.flare_visibility_test);
  CHECK(spot.target_shadow);
  CHECK(!spot.animate_color_from_preset);
  CHECK(!spot.animate_orientation_from_preset);
  std::printf("  [ok] Spotlight: source rev=%u target=%s group=%s\n",
              spot.revision, spot.target.c_str(), spot.group.c_str());
}

void test_cam_projection_fields() {
  std::vector<uint8_t> b;
  put_u32(b, 12);                // Cam revision from GH2 PS2 metacam.
  put_zeros(b, 9);               // object/base metadata.
  put_u32(b, 9);                 // embedded Trans revision.
  put_matrix(b, 0.0f, -768.0f, 0.0f);
  put_matrix(b, 0.0f, -768.0f, 0.0f);
  put_u32(b, 0);                 // constraint.
  put_str(b, "");                // target.
  b.push_back(0);                // preserve_scale.
  put_str(b, "meta.cam");        // parent, as in meta_proxy.cam.
  put_f32(b, 50.0f);             // near.
  put_f32(b, 1000.0f);           // far.
  put_f32(b, 0.6024157f);        // vertical fov, stored as radians.
  put_f32(b, 0.0f); put_f32(b, 0.0f); put_f32(b, 1.0f); put_f32(b, 1.0f);
  put_f32(b, 0.0f); put_f32(b, 1.0f);
  put_str(b, "venue_cut.rt");

  CamObj cam = decode_cam("meta_proxy.cam", b);
  CHECK(cam.decoded);
  CHECK(cam.parent == "meta.cam");
  CHECK(approx(cam.local.pos[1], -768.0f));
  CHECK(approx(cam.world_stored.pos[1], -768.0f));
  CHECK(approx(cam.near_plane, 50.0f));
  CHECK(approx(cam.far_plane, 1000.0f));
  CHECK(approx(cam.fov, 0.6024157f));
  CHECK(approx(cam.screen_rect[2], 1.0f));
  CHECK(approx(cam.screen_rect[3], 1.0f));
  CHECK(approx(cam.z_range[0], 0.0f));
  CHECK(approx(cam.z_range[1], 1.0f));
  CHECK(cam.target_tex == "venue_cut.rt");

  b.clear();
  put_u32(b, 11);                // RndCam::Load rev<12 converts stored Y-FOV.
  put_zeros(b, 9);               // object/base metadata.
  put_u32(b, 9);                 // embedded Trans revision.
  put_matrix(b, 0.0f, -768.0f, 0.0f);
  put_matrix(b, 0.0f, -768.0f, 0.0f);
  put_u32(b, 0);
  put_str(b, "");
  b.push_back(0);
  put_str(b, "meta.cam");
  put_f32(b, 50.0f);
  put_f32(b, 1000.0f);
  put_f32(b, 0.8f);
  put_f32(b, 0.0f); put_f32(b, 0.0f); put_f32(b, 1.0f); put_f32(b, 1.0f);
  put_f32(b, 0.0f); put_f32(b, 1.0f);
  put_str(b, "venue_cut.rt");

  CamObj old_cam = decode_cam("old_proxy.cam", b);
  CHECK(old_cam.decoded);
  CHECK(approx(old_cam.fov, std::atan(0.75f * std::tan(0.4f)) * 2.0f));
  std::printf("  [ok] Cam rev11 ConvertFov: stored=0.800000 decoded=%.6f\n",
              old_cam.fov);
  std::printf(
      "  [ok] Cam: parent=%s near=%.0f far=%.0f fov=%.6f z=(%.0f,%.0f) target=%s\n",
      cam.parent.c_str(), cam.near_plane, cam.far_plane, cam.fov,
      cam.z_range[0], cam.z_range[1], cam.target_tex.c_str());
}

void test_group_transform() {
  std::vector<uint8_t> b;
  put_u32(b, 13);                // GH2-era Group version with drawOnly.
  put_zeros(b, 9);               // Hmx::Object metadata.
  put_u32(b, 4);                 // RndAnimatable revision.
  put_f32(b, 0.0f);              // frame.
  put_u32(b, 0);                 // rate.
  put_u32(b, 9);                 // RndTransformable revision.
  put_matrix(b, 25.0f, 0.0f, -40.0f);
  put_matrix(b, 25.0f, 0.0f, 940.0f);
  put_zeros(b, 9);               // constraint, empty target, preserve_scale.
  put_str(b, "ss_setlist.view");
  put_u32(b, 3);                  // RndDrawable revision.
  b.push_back(1);                 // showing.
  put_zeros(b, 16);               // sphere.
  put_f32(b, 2.0f);               // draw order.
  put_u32(b, 3);                  // RndGroup objects.
  put_str(b, "paper.mesh");
  put_str(b, "title.lbl");
  put_str(b, "child.view");
  put_str(b, "lighting.env");
  put_str(b, "paper.mesh");       // drawOnly.
  put_str(b, "");                 // LOD.
  put_f32(b, 0.0f);               // LOD screen size.

  GroupObj group = decode_group("ss_songlist.view", b);
  CHECK(group.name == "ss_songlist.view");
  CHECK(group.decoded);
  CHECK(group.source_order_decoded);
  CHECK(group.has_transform);
  CHECK(group.parent == "ss_setlist.view");
  CHECK(group.showing);
  CHECK(approx(group.draw_order, 2.0f));
  CHECK(approx(group.local.pos[0], 25.0f));
  CHECK(approx(group.local.pos[2], -40.0f));
  CHECK(approx(group.world_stored.pos[2], 940.0f));
  CHECK(group.children.size() == 3);
  CHECK(group.children[0] == "paper.mesh");
  CHECK(group.children[1] == "title.lbl");
  CHECK(group.children[2] == "child.view");
  CHECK(group.environment_ref == "lighting.env");
  CHECK(group.draw_only == "paper.mesh");
  std::printf("  [ok] Group: parent=%s local.z=%.0f world.z=%.0f\n",
              group.parent.c_str(), group.local.pos[2],
              group.world_stored.pos[2]);
}

void test_group_draw_order_matches_rnddir_roots() {
  Scene scene;
  auto mesh = [](const char* name) {
    MeshObj out;
    out.name = name;
    out.decoded = true;
    out.showing = true;
    return out;
  };
  MeshObj root_mesh = mesh("root.mesh");
  root_mesh.draw_order = 0.5f;
  root_mesh.dir_index = 10;
  scene.meshes.push_back(root_mesh);
  scene.meshes.push_back(mesh("translucent.mesh"));
  scene.meshes.push_back(mesh("opaque.mesh"));
  scene.meshes.push_back(mesh("hidden_child.mesh"));

  GroupObj translucent;
  translucent.name = "translucent.grp";
  translucent.showing = true;
  translucent.draw_order = 1.0f;
  translucent.children.push_back("translucent.mesh");
  scene.groups.push_back(translucent);

  GroupObj hidden_parent;
  hidden_parent.name = "hidden_parent.grp";
  hidden_parent.showing = false;
  hidden_parent.draw_order = -1.0f;
  hidden_parent.children.push_back("hidden_child.grp");
  scene.groups.push_back(hidden_parent);

  GroupObj opaque;
  opaque.name = "opaque.grp";
  opaque.showing = true;
  opaque.draw_order = 0.0f;
  opaque.dir_index = 20;
  opaque.children.push_back("opaque.mesh");
  scene.groups.push_back(opaque);

  GroupObj hidden_child;
  hidden_child.name = "hidden_child.grp";
  hidden_child.showing = true;
  hidden_child.draw_order = -1.0f;
  hidden_child.children.push_back("hidden_child.mesh");
  scene.groups.push_back(hidden_child);

  rebuild_group_authored_draw_order_for_test(scene);
  CHECK(scene.draw_order.size() == 3);
  CHECK(scene.draw_order[0] == "opaque.mesh");
  CHECK(scene.draw_order[1] == "root.mesh");
  CHECK(scene.draw_order[2] == "translucent.mesh");
  CHECK(std::find(scene.draw_order.begin(), scene.draw_order.end(),
                  "hidden_child.mesh") == scene.draw_order.end());
  CHECK(std::find(scene.grouped_meshes.begin(), scene.grouped_meshes.end(),
                  "hidden_child.mesh") != scene.grouped_meshes.end());
  std::printf(
      "  [ok] Group draw roots: opaque before translucent, hidden child suppressed\n");
}

void test_band_placer() {
  std::vector<uint8_t> b;
  put_u32(b, 2);                 // BandPlacer version.
  put_zeros(b, 8);               // object/base header before the kind string.
  put_str(b, "char");
  b.push_back(0);                // PS2 BandPlacer kind strings are nul-padded.
  put_u32(b, 3);
  put_u32(b, 1);
  put_zeros(b, 0x2a - b.size());
  put_u32(b, 9);                 // embedded transform marker.
  put_matrix(b, -35.0f, -30.0f, -47.5f);
  put_matrix(b, -35.0f, -636.5f, -47.5f);
  put_zeros(b, 9);
  put_str(b, "mgs_camerafix.grp");
  put_str(b, "spot_ui.mesh");

  BandPlacerObj placer = decode_band_placer("char_multi0.placer", b);
  CHECK(placer.decoded);
  CHECK(placer.kind == "char");
  CHECK(placer.parent == "mgs_camerafix.grp");
  CHECK(approx(placer.local.pos[0], -35.0f));
  CHECK(approx(placer.world_stored.pos[1], -636.5f));
  CHECK(approx(placer.world_stored.pos[2], -47.5f));

  Scene sc;
  sc.band_placers.push_back(placer);
  CHECK(sc.find_band_placer("char_multi0.placer") != nullptr);
  CHECK(sc.find_band_placer("missing.placer") == nullptr);
  std::printf("  [ok] BandPlacer: kind=%s parent=%s world=(%.1f %.1f %.1f)\n",
              placer.kind.c_str(), placer.parent.c_str(),
              placer.world_stored.pos[0], placer.world_stored.pos[1],
              placer.world_stored.pos[2]);
}

void test_real_menu_band_placers() {
  const std::string ark_dir =
      "C:/Programming/GitHub/Guitar Hero II/gh2_ps2_hybrid_assets/gen";
  const std::string hdr = first_existing(ark_dir, {"main.hdr", "MAIN.HDR"});
  const std::string ark = first_existing(ark_dir, {"main_0.ark", "MAIN_0.ARK"});
  if (hdr.empty() || ark.empty()) {
    std::printf("  [skip] real menu BandPlacers (no PS2 archive)\n");
    return;
  }

  Scene single;
  CHECK(load_scene(hdr, ark, "ui/gen/sel_character.milo_ps2", single));
  const BandPlacerObj* single_placer =
      single.find_band_placer("char_single.placer");
  CHECK(single_placer != nullptr);
  CHECK(single_placer && single_placer->parent == "sel_character.view");

  Scene multi;
  CHECK(load_scene(hdr, ark, "ui/gen/multi_sel_character.milo_ps2", multi));
  CHECK(multi.find_band_placer("char_multi0.placer") != nullptr);
  CHECK(multi.find_band_placer("char_multi1.placer") != nullptr);

  Scene store;
  CHECK(load_scene(hdr, ark, "ui/gen/store.milo_ps2", store));
  CHECK(store.find_band_placer("char_store.placer") != nullptr);
  std::printf("  [ok] real menu BandPlacers: char_single, char_multi0/1, char_store\n");
}

void test_real_pause_tile_source_transforms() {
  const std::string ark_dir =
      "C:/Programming/GitHub/Guitar Hero II/gh2_ps2_hybrid_assets/gen";
  const std::string hdr = first_existing(ark_dir, {"main.hdr", "MAIN.HDR"});
  const std::string ark = first_existing(ark_dir, {"main_0.ark", "MAIN_0.ARK"});
  if (hdr.empty() || ark.empty()) {
    std::printf("  [skip] real pause tile transforms (no PS2 archive)\n");
    return;
  }

  struct ExpectedTile {
    const char* mesh_name;
    const char* mat_name;
    float x;
    float z;
    float u_scale;
    float v_scale;
  };
  const ExpectedTile pause_expected[] = {
      {"pause_tile1.mesh", "pause_tile1.mat", -110.0f, 100.0f, 1.0f, 1.0f},
      {"pause_tile2.mesh", "pause_tile2.mat", 110.0f, 100.0f, -1.0f, 1.0f},
      {"pause_tile3.mesh", "pause_tile3.mat", -110.0f, -100.0f, 1.0f, -1.0f},
      {"pause_tile4.mesh", "pause_tile4.mat", 110.0f, -100.0f, -1.0f, -1.0f},
  };
  const ExpectedTile pause_controller_expected[] = {
      {"tile1.mesh", "tile1.mat", -110.0f, 100.0f, 1.0f, 1.0f},
      {"tile2.mesh", "tile2.mat", 110.0f, 100.0f, -1.0f, 1.0f},
      {"tile3.mesh", "tile3.mat", -110.0f, -100.0f, 1.0f, -1.0f},
      {"tile4.mesh", "tile4.mat", 110.0f, -100.0f, -1.0f, -1.0f},
  };
  const ExpectedTile pause_audio_expected[] = {
      {"gs_tile1.mesh", "gs_tile1.mat", -110.0f, 100.0f, 1.0f, 1.0f},
      {"gs_tile2.mesh", "gs_tile2.mat", 110.0f, 100.0f, -1.0f, 1.0f},
      {"gs_tile3.mesh", "gs_tile3.mat", -110.0f, -100.0f, 1.0f, -1.0f},
      {"gs_tile4.mesh", "gs_tile4.mat", 110.0f, -100.0f, -1.0f, -1.0f},
  };
  const ExpectedTile pause_video_expected[] = {
      {"gs_tile1.mesh", "gs_tile1.mat", -110.0f, 100.0f, 1.0f, 1.0f},
      {"gs_tile2.mesh", "gs_tile2.mat", 110.0f, 100.0f, -1.0f, 1.0f},
      {"gs_tile3.mesh", "gs_tile3.mat", -110.0f, -100.0f, 1.0f, -1.0f},
      {"gs_tile4.mesh", "gs_tile4.mat", 110.0f, -100.0f, -1.0f, -1.0f},
  };
  struct PauseTileCase {
    const char* milo_path;
    const char* parent;
    const ExpectedTile* tiles;
    std::size_t count;
  };
  const PauseTileCase cases[] = {
      {"ui/gen/pause.milo_ps2", "pause_background.view", pause_expected,
       std::size(pause_expected)},
      {"ui/gen/pause_controller.milo_ps2",
       "pause_controller_background.view", pause_controller_expected,
       std::size(pause_controller_expected)},
      {"ui/gen/pause_audio_settings.milo_ps2", "gs_background.view",
       pause_audio_expected, std::size(pause_audio_expected)},
      {"ui/gen/pause_video_settings.milo_ps2", "gs_background.view",
       pause_video_expected, std::size(pause_video_expected)},
  };
  for (const PauseTileCase& source : cases) {
    Scene scene;
    CHECK(load_scene(hdr, ark, source.milo_path, scene));
    for (std::size_t i = 0; i < source.count; ++i) {
      const ExpectedTile& tile = source.tiles[i];
      const MeshObj* mesh = nullptr;
      for (const MeshObj& candidate : scene.meshes) {
        if (candidate.name == tile.mesh_name) {
          mesh = &candidate;
          break;
        }
      }
      CHECK(mesh != nullptr);
      CHECK(mesh && mesh->decoded);
      if (!mesh) continue;
      const MatObj* mat = scene.find_mat(mesh->material);
      CHECK(mat != nullptr);
      const auto world = scene.world_matrix(*mesh);
      CHECK(mesh->parent == source.parent);
      CHECK(mesh->material == tile.mat_name);
      CHECK(approx(world[12], tile.x));
      CHECK(approx(world[13], 0.0f));
      CHECK(approx(world[14], tile.z));
      CHECK(approx(mesh->local.rot[0][0], 1.0f));
      CHECK(approx(mesh->local.rot[1][2], 1.0f));
      CHECK(approx(mesh->local.rot[2][1], -1.0f));
      CHECK(approx_eps(mat->tex_xfm[0][0], tile.u_scale, 0.002f));
      CHECK(approx(mat->tex_xfm[0][1], 0.0f));
      CHECK(approx(mat->tex_xfm[1][0], 0.0f));
      CHECK(approx_eps(mat->tex_xfm[1][1], tile.v_scale, 0.002f));
      CHECK(approx(mat->tex_xfm[2][0], 0.0f));
      CHECK(approx(mat->tex_xfm[2][1], 0.0f));
      CHECK(approx(mat->tex_xfm[2][2], 1.0f));
    }
  }
  std::printf("  [ok] real pause border tiles: pause, controller-loss, audio settings, and video settings source UV flips form all four corners\n");
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
  put_u32(b, 0x1F);              // RndMesh::mMutable
  put_u32(b, 1);                 // volume
  b.push_back(0);                // null BSP-tree owner
  put_u32(b, 3);                 // vertex_count = 3
  // 3 vertices (pos / normal / GH2 rev28 weight slot / uv), forming a unit triangle.
  const float P[3][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  for (int i = 0; i < 3; ++i) {
    put_f32(b, P[i][0]); put_f32(b, P[i][1]); put_f32(b, P[i][2]);  // pos
    put_f32(b, 0); put_f32(b, 0); put_f32(b, 1);                    // normal
    put_f32(b, 1); put_f32(b, 0); put_f32(b, 0); put_f32(b, 0);     // weights
    put_f32(b, P[i][0]); put_f32(b, P[i][1]);                       // uv
  }
  put_u32(b, 1);                 // face_count = 1
  put_u16(b, 0); put_u16(b, 1); put_u16(b, 2);  // the triangle
  put_u32(b, 1);                 // groupSizesCount
  b.push_back(1);                // one material/face group
  put_str(b, "bone_a.mesh");     // old pre-rev33 fixed four-entry bone table
  put_str(b, "bone_b.mesh");
  put_str(b, "");
  put_str(b, "");
  put_matrix(b, 7.0f, 8.0f, 9.0f);
  put_matrix(b, 10.0f, 11.0f, 12.0f);
  put_matrix(b, 0.0f, 0.0f, 0.0f);
  put_matrix(b, 0.0f, 0.0f, 0.0f);

  MeshObj m = decode_mesh("tri.mesh", b);
  if (!m.decoded) std::printf("  [FAIL] mesh error: %s\n", m.error.c_str());
  CHECK(m.decoded);
  CHECK(m.vertex_count == 3);
  CHECK(m.face_count == 1);
  CHECK(m.indices.size() == 3 && m.indices[2] == 2);
  CHECK(m.material == "gem.mat");
  CHECK(m.mutable_flags == 0x1F);
  CHECK(m.parent == "track.view");
  CHECK(m.showing);
  CHECK(approx(m.verts[0].w[0], 1.0f) && approx(m.verts[0].w[1], 0.0f));
  CHECK(m.bones.size() == 2);
  CHECK(m.bones[0].name == "bone_a.mesh");
  CHECK(approx(m.bones[0].offset.pos[0], 7.0f));
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

  Scene group_sc;
  GroupObj group;
  group.name = "track.view";
  group.has_transform = true;
  group.local.pos[0] = 10.0f;
  group.world_stored = group.local;
  group_sc.groups.push_back(group);
  group_sc.meshes.push_back(m);
  auto gw = group_sc.world_matrix(group_sc.meshes[0]);
  CHECK(approx(gw[12], 11.0f));
  CHECK(approx(gw[13], 2.0f));
  CHECK(approx(gw[14], 3.0f));
  std::printf("  [ok] group world compose: translation=(%.0f,%.0f,%.0f)\n",
              gw[12], gw[13], gw[14]);

  Scene placer_sc;
  GroupObj display_group;
  display_group.name = "guitar_store.view";
  display_group.has_transform = true;
  display_group.local.pos[0] = -33.0f;
  display_group.local.pos[2] = -30.0f;
  display_group.world_stored = display_group.local;
  placer_sc.groups.push_back(display_group);
  BandPlacerObj guitar_placer;
  guitar_placer.name = "guitar_store.placer";
  guitar_placer.kind = "guitar";
  guitar_placer.parent = "guitar_store.view";
  guitar_placer.local.pos[2] = 55.0f;
  guitar_placer.world_stored.pos[0] = -33.0f;
  guitar_placer.world_stored.pos[2] = 25.0f;
  guitar_placer.decoded = true;
  placer_sc.band_placers.push_back(guitar_placer);
  MeshObj placed = m;
  placed.parent = "guitar_store.placer";
  placer_sc.meshes.push_back(placed);
  auto pw = placer_sc.world_matrix(placer_sc.meshes[0]);
  CHECK(approx(pw[12], -32.0f));
  CHECK(approx(pw[13], 2.0f));
  CHECK(approx(pw[14], 28.0f));
  std::printf("  [ok] BandPlacer world compose: translation=(%.0f,%.0f,%.0f)\n",
              pw[12], pw[13], pw[14]);

  MeshObj authored = m;
  authored.world_stored.pos[0] = 100.0f;
  authored.world_stored.pos[1] = 200.0f;
  authored.world_stored.pos[2] = 300.0f;
  Scene authored_sc;
  authored_sc.groups.push_back(group);
  authored_sc.meshes.push_back(authored);
  auto aw = authored_sc.world_matrix(authored_sc.meshes[0]);
  CHECK(approx(aw[12], 100.0f));
  CHECK(approx(aw[13], 200.0f));
  CHECK(approx(aw[14], 300.0f));
  std::printf("  [ok] authored world wins: translation=(%.0f,%.0f,%.0f)\n",
              aw[12], aw[13], aw[14]);

  std::vector<uint8_t> hidden = b;
  hidden[draw_showing_offset] = 0;
  MeshObj h = decode_mesh("hidden.mesh", hidden);
  CHECK(h.decoded);
  CHECK(!h.showing);
}

void test_particle_sys_source_order() {
  std::vector<uint8_t> b;
  put_u32(b, 27);                // GH2 PS2 RndParticleSys revision.
  put_zeros(b, 9);               // Hmx::Object metadata.
  put_u32(b, 4);                 // RndAnimatable revision.
  put_f32(b, 30.0f);             // anim rate.
  put_u32(b, 0);                 // anim flags.
  put_u32(b, 9);                 // RndTransformable revision.
  put_matrix(b, 1.0f, 2.0f, 3.0f);
  put_matrix(b, 4.0f, 5.0f, 6.0f);
  put_u32(b, 2);                 // constraint.
  put_str(b, "spark_target.trans");
  b.push_back(1);                // preserve scale.
  put_str(b, "stage_root.grp");
  put_u32(b, 3);                 // RndDrawable revision.
  b.push_back(1);                // showing.
  put_zeros(b, 16);              // bounding sphere.
  put_f32(b, 12.0f);             // draw order.

  put_f32(b, 45.0f); put_f32(b, 90.0f);       // life.
  put_f32(b, -1.0f); put_f32(b, -2.0f); put_f32(b, -3.0f);
  put_f32(b, 4.0f); put_f32(b, 5.0f); put_f32(b, 6.0f);
  put_f32(b, 7.0f); put_f32(b, 9.0f);         // speed.
  put_f32(b, 0.1f); put_f32(b, 0.2f);         // pitch.
  put_f32(b, 0.3f); put_f32(b, 0.4f);         // yaw.
  put_f32(b, 10.0f); put_f32(b, 20.0f);       // emit rate.
  put_f32(b, 2.0f); put_f32(b, 3.0f);         // start size.
  put_f32(b, -0.5f); put_f32(b, 1.0f);        // delta size.
  put_f32(b, 0.1f); put_f32(b, 0.2f); put_f32(b, 0.3f); put_f32(b, 0.4f);
  put_f32(b, 0.5f); put_f32(b, 0.6f); put_f32(b, 0.7f); put_f32(b, 0.8f);
  put_f32(b, 0.9f); put_f32(b, 0.8f); put_f32(b, 0.7f); put_f32(b, 0.6f);
  put_f32(b, 0.5f); put_f32(b, 0.4f); put_f32(b, 0.3f); put_f32(b, 0.2f);
  put_str(b, "spark_bounce.trans");
  put_f32(b, 0.0f); put_f32(b, 1.0f); put_f32(b, 2.0f);
  put_str(b, "spark.mat");
  put_u32(b, 0xA5A5u);
  put_f32(b, 0.2f); put_f32(b, 0.8f); put_f32(b, 0.4f);
  put_f32(b, 0.2f); put_f32(b, 0.3f); put_f32(b, 0.4f); put_f32(b, 0.5f);
  put_f32(b, 0.6f); put_f32(b, 0.7f); put_f32(b, 0.8f); put_f32(b, 0.9f);
  put_u32(b, 128);
  put_f32(b, 11.0f); put_f32(b, 13.0f);       // bubble period.
  put_f32(b, 0.7f); put_f32(b, 1.3f);         // bubble size.
  b.push_back(1);                             // bubble.
  put_f32(b, 0.25f);                          // relative motion.
  put_str(b, "spark_parent.trans");
  put_str(b, "spark_emitter.mesh");
  b.push_back(1);                             // preserve particles.
  put_u32(b, 2);
  for (int row = 0; row < 2; ++row) {
    put_f32(b, 10.0f + row);                  // position x
    put_f32(b, 20.0f + row);                  // position y
    put_f32(b, 30.0f + row);                  // position z
    put_f32(b, 0.1f);                         // color r
    put_f32(b, 0.2f);                         // color g
    put_f32(b, 0.3f);                         // color b
    put_f32(b, 0.4f);                         // color a
    put_f32(b, 5.0f + row);                   // size
  }

  ParticleSysObj p = decode_particle_sys("sparks.part", b);
  if (!p.decoded) std::printf("  [FAIL] ParticleSys error: %s\n", p.error.c_str());
  CHECK(p.decoded);
  CHECK(p.source_order_decoded);
  CHECK(p.revision == 27);
  CHECK(p.anim_revision == 4);
  CHECK(p.trans_revision == 9);
  CHECK(p.draw_revision == 3);
  CHECK(p.parent == "stage_root.grp");
  CHECK(p.target == "spark_target.trans");
  CHECK(p.preserve_scale);
  CHECK(p.material == "spark.mat");
  CHECK(p.bounce == "spark_bounce.trans");
  CHECK(p.relative_parent == "spark_parent.trans");
  CHECK(p.emitter_mesh == "spark_emitter.mesh");
  CHECK(p.max_particles == 128);
  CHECK(approx(p.life_min_frames, 45.0f));
  CHECK(approx(p.life_max_frames, 90.0f));
  CHECK(approx(p.box_extent_min[1], -2.0f));
  CHECK(approx(p.box_extent_max[2], 6.0f));
  CHECK(approx(p.speed_min, 7.0f));
  CHECK(approx(p.speed_max, 9.0f));
  CHECK(approx(p.emit_rate_max, 20.0f));
  CHECK(approx(p.delta_size_min, -0.5f));
  CHECK(approx(p.start_color_low[2], 0.3f));
  CHECK(approx(p.end_color_high[3], 0.2f));
  CHECK(approx(p.force_dir[2], 2.0f));
  CHECK(p.particle_flags == 0xA5A5u);
  CHECK(approx(p.grow_ratio, 0.2f));
  CHECK(approx(p.shrink_ratio, 0.8f));
  CHECK(approx(p.mid_color_ratio, 0.4f));
  CHECK(approx(p.mid_color_high[2], 0.8f));
  CHECK(p.bubble);
  CHECK(approx(p.bubble_period_max, 13.0f));
  CHECK(approx(p.bubble_size_min, 0.7f));
  CHECK(approx(p.relative_motion, 0.25f));
  CHECK(p.preserve_particles);
  CHECK(p.preserved_particle_count == 2);
  CHECK(p.preserved_particle_stride_bytes == 8u * sizeof(float));
  std::printf("  [ok] ParticleSys: source rev=%u mat=%s max=%u parent=%s preserved_stride=%u\n",
              p.revision, p.material.c_str(), p.max_particles,
              p.parent.c_str(), p.preserved_particle_stride_bytes);
}

void test_world_crowd_gh2_matrix_stride() {
  std::vector<uint8_t> b;
  put_u32(b, 6);                 // WorldCrowd revision used by GH2 PS2 chars.
  put_u32(b, 3);                 // RndDrawable revision.
  b.push_back(1);                // showing
  put_f32(b, 0); put_f32(b, 0); put_f32(b, 0); put_f32(b, 0);
  put_f32(b, 0);                 // draw order
  put_str(b, "Crowd_area.mesh"); // placement mesh
  put_u32(b, 2);                 // total placements
  b.push_back(0);                // pre-rev8 flag
  put_u32(b, 1);                 // actor count
  put_str(b, "crowd_male01");
  put_f32(b, 95.0f);             // height
  put_f32(b, 1.0f);              // density
  put_f32(b, 10.0f);             // radius
  put_u32(b, 2);                 // GH2 matrix-only placement count
  put_matrix(b, 10.0f, 20.0f, 30.0f);
  put_matrix(b, 40.0f, 50.0f, 60.0f);
  put_u32(b, 1234);              // modifyStamp
  b.push_back(0);                // show3DOnly

  WorldCrowdObj crowd = decode_world_crowd("crowd", b);
  CHECK(crowd.decoded);
  CHECK(crowd.area_mesh == "Crowd_area.mesh");
  CHECK(crowd.total_placements == 2);
  CHECK(crowd.decoded_placement_count == 2);
  CHECK(crowd.actors.size() == 1);
  CHECK(crowd.placement_sets.size() == 1);
  CHECK(crowd.placement_sets[0].placements.size() == 2);
  CHECK(approx(crowd.placement_sets[0].placements[0].pos[0], 10.0f));
  CHECK(approx(crowd.placement_sets[0].placements[0].pos[1], 20.0f));
  CHECK(approx(crowd.placement_sets[0].placements[0].pos[2], 30.0f));
  CHECK(approx(crowd.placement_sets[0].placements[1].pos[0], 40.0f));
  CHECK(approx(crowd.placement_sets[0].placements[1].pos[1], 50.0f));
  CHECK(approx(crowd.placement_sets[0].placements[1].pos[2], 60.0f));
  std::printf("  [ok] WorldCrowd rev6: placements=%u matrix stride\n",
              crowd.total_placements);
}

void test_world_crowd_mnum_separate_from_source_rows() {
  std::vector<uint8_t> b;
  put_u32(b, 6);                 // GH2 PS2 WorldCrowd revision.
  put_u32(b, 3);                 // RndDrawable revision.
  b.push_back(1);                // showing.
  put_f32(b, 0); put_f32(b, 0); put_f32(b, 0); put_f32(b, 0);
  put_f32(b, 0);                 // draw order.
  put_str(b, "crowd_area_left.mesh");
  put_u32(b, 2);                 // ihatecompvir mNum, not row sum.
  b.push_back(0);                // pre-rev8 flag.
  put_u32(b, 2);                 // character rows.
  put_str(b, "crowd_female05");
  put_f32(b, 75.0f);
  put_f32(b, 1.0f);
  put_f32(b, 10.0f);
  put_str(b, "crowd_male05");
  put_f32(b, 75.0f);
  put_f32(b, 1.0f);
  put_f32(b, 10.0f);
  put_u32(b, 2);
  put_matrix(b, 10.0f, 20.0f, 30.0f);
  put_matrix(b, 40.0f, 50.0f, 60.0f);
  put_u32(b, 2);
  put_matrix(b, 70.0f, 80.0f, 90.0f);
  put_matrix(b, 100.0f, 110.0f, 120.0f);
  put_u32(b, 1234);              // modifyStamp.
  b.push_back(0);                // show3DOnly.

  WorldCrowdObj crowd = decode_world_crowd("crowd_left", b);
  if (!crowd.decoded) std::printf("  [FAIL] WorldCrowd error: %s\n", crowd.error.c_str());
  CHECK(crowd.decoded);
  CHECK(crowd.total_placements == 2);
  CHECK(crowd.decoded_placement_count == 4);
  CHECK(crowd.actors.size() == 2);
  CHECK(crowd.placement_sets.size() == 2);
  CHECK(crowd.placement_sets[0].actor_name == "crowd_female05");
  CHECK(crowd.placement_sets[1].actor_name == "crowd_male05");
  CHECK(crowd.placement_sets[0].placements.size() == 2);
  CHECK(crowd.placement_sets[1].placements.size() == 2);
  CHECK(approx(crowd.placement_sets[1].placements[1].pos[2], 120.0f));
  std::printf("  [ok] WorldCrowd: mNum=%u decodedRows=%u actors=%zu\n",
              crowd.total_placements, crowd.decoded_placement_count,
              crowd.actors.size());
}

}  // namespace

int main() {
  std::printf("milo_scene_test\n");
  test_milo_editor_dtb_node_payload_plan();
  test_trans();
  test_trans_proxy();
  test_trans_anim();
  test_mesh_anim();
  test_poll_anim();
  test_prop_anim();
  test_mat();
  test_wind();
  test_group();
  test_mesh_deform();
  test_multimesh();
  test_light();
  test_environ_with_lights();
  test_environ_with_extensionless_light();
  test_environ_with_fog();
  test_spotlight_source_order_rev20();
  test_cam_projection_fields();
  test_group_transform();
  test_group_draw_order_matches_rnddir_roots();
  test_band_placer();
  test_real_menu_band_placers();
  test_real_pause_tile_source_transforms();
  test_mesh();
  test_particle_sys_source_order();
  test_world_crowd_gh2_matrix_stride();
  test_world_crowd_mnum_separate_from_source_rows();
  std::printf("ALL PASS\n");
  return 0;
}
