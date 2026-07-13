#include "character/char_clip.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

bool approx(float a, float b) {
  return std::fabs(a - b) < 1.0e-5f;
}

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::printf("FAIL %s got=%d want=%d\n", label, got ? 1 : 0,
              want ? 1 : 0);
  return false;
}

bool expect_float(float got, float want, const char* label) {
  if (approx(got, want)) return true;
  std::printf("FAIL %s got=%.8f want=%.8f\n", label, got, want);
  return false;
}

bool expect_size(size_t got, size_t want, const char* label) {
  if (got == want) return true;
  std::printf("FAIL %s got=%zu want=%zu\n", label, got, want);
  return false;
}

bool expect_string(const std::string& got, const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::printf("FAIL %s got=%s want=%s\n", label, got.c_str(), want.c_str());
  return false;
}

bool has(const std::vector<std::string>& values, const std::string& value) {
  for (const std::string& candidate : values) {
    if (candidate == value) return true;
  }
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharIKHandMeasure;
  using ghogx::character::SourceCharIKHandElbowCollisionInput;
  using ghogx::character::SourceCharIKHandElbowSwingInput;
  using ghogx::character::SourceCharIKHandPollFlowInput;
  using ghogx::character::SourceCharIKHandSetHandResult;
  using ghogx::character::SourceCharIKHandTargetInput;
  using ghogx::character::SourceCharIKHandWristConstraintInput;
  using ghogx::character::source_char_ik_hand_copy_plan;
  using ghogx::character::source_char_ik_hand_elbow_cosine;
  using ghogx::character::source_char_ik_hand_elbow_collision_gate;
  using ghogx::character::source_char_ik_hand_elbow_swing;
  using ghogx::character::source_char_ik_hand_finger_target;
  using ghogx::character::source_char_ik_hand_handler_plan;
  using ghogx::character::source_char_ik_hand_load_plan;
  using ghogx::character::source_char_ik_hand_measure_lengths;
  using ghogx::character::source_char_ik_hand_multi_target_blend;
  using ghogx::character::source_char_ik_hand_poll_flow;
  using ghogx::character::source_char_ik_hand_prop_sync_plan;
  using ghogx::character::source_char_ik_hand_save_plan;
  using ghogx::character::source_char_ik_hand_set_hand;
  using ghogx::character::source_char_ik_hand_update_measure_lengths;
  using ghogx::character::source_char_ik_hand_wrist_constraint;

  bool ok = true;

  const auto bad_load = source_char_ik_hand_load_plan(13);
  ok &= expect_bool(bad_load.known_revision, false,
                    "IKHand rejects newer revision");
  ok &= expect_size(bad_load.read_order.size(), 0,
                    "IKHand bad load has no rows");

  const auto load_v0 = source_char_ik_hand_load_plan(0);
  ok &= expect_bool(load_v0.known_revision, true,
                    "IKHand accepts revision zero");
  ok &= expect_string(load_v0.read_order[0], "LOAD_REVS",
                      "IKHand load starts with revs");
  ok &= expect_bool(has(load_v0.branches, "mFinger=0"), true,
                    "IKHand v0 clears finger");
  ok &= expect_bool(has(load_v0.read_order, "legacyTarget"), true,
                    "IKHand v0 reads legacy target");
  ok &= expect_bool(has(load_v0.branches, "targets=singleLegacyTargetExtent0"),
                    true, "IKHand v0 expands single target");
  ok &= expect_bool(has(load_v0.branches, "mScalable=false"), true,
                    "IKHand v0 defaults scalable");
  ok &= expect_bool(has(load_v0.branches, "mMoveElbow=true"), true,
                    "IKHand v0 defaults move elbow");
  ok &= expect_bool(has(load_v0.branches, "mElbowSwing=0"), true,
                    "IKHand v0 defaults elbow swing");
  ok &= expect_bool(load_v0.calls_set_hand, true, "IKHand load calls SetHand");

  const auto load_v9 = source_char_ik_hand_load_plan(9);
  ok &= expect_bool(has(load_v9.read_order, "mFinger"), true,
                    "IKHand v9 reads finger");
  ok &= expect_bool(has(load_v9.read_order, "legacyTargetList"), true,
                    "IKHand v9 reads legacy target list");
  ok &= expect_bool(has(load_v9.read_order, "mAlwaysIKElbow"), true,
                    "IKHand v9 reads always IK elbow");
  ok &= expect_bool(has(load_v9.read_order, "mConstrainWrist"), true,
                    "IKHand v9 reads constrain wrist");
  ok &= expect_bool(has(load_v9.read_order, "rev9StringPadding"), true,
                    "IKHand v9 reads string padding");
  ok &= expect_bool(has(load_v9.read_order, "rev9BoolPadding"), true,
                    "IKHand v9 reads bool padding");
  ok &= expect_bool(has(load_v9.read_order, "mElbowCollide"), false,
                    "IKHand v9 does not read elbow collide");

  const auto load_v12 = source_char_ik_hand_load_plan(12);
  ok &= expect_bool(has(load_v12.read_order, "mTargets"), true,
                    "IKHand v12 reads IKTarget vector");
  ok &= expect_bool(has(load_v12.read_order, "mElbowCollide"), true,
                    "IKHand v12 reads elbow collide");
  ok &= expect_bool(has(load_v12.read_order, "mClockwise"), true,
                    "IKHand v12 reads clockwise");

  const auto copy = source_char_ik_hand_copy_plan();
  ok &= expect_size(copy.copied_superclasses.size(), 2,
                    "IKHand copy superclass count");
  ok &= expect_string(copy.copied_superclasses[0], "Hmx::Object",
                      "IKHand copy first superclass");
  ok &= expect_string(copy.copied_superclasses[1], "CharWeightable",
                      "IKHand copy second superclass");
  ok &= expect_string(copy.member_steps[0], "SetHand(c->mHand)",
                      "IKHand copy calls SetHand first");
  ok &= expect_string(copy.member_steps[1], "mHand",
                      "IKHand copy copies hand");
  ok &= expect_string(copy.member_steps[2], "mTargets",
                      "IKHand copy first targets row");
  ok &= expect_string(copy.member_steps[10], "mTargets",
                      "IKHand copy duplicated targets row");
  ok &= expect_bool(has(copy.member_steps, "mFinger"), false,
                    "IKHand visible copy omits finger");

  const auto handlers = source_char_ik_hand_handler_plan();
  ok &= expect_size(handlers.handlers.size(), 1,
                    "IKHand handler count");
  ok &= expect_string(handlers.handlers[0], "measure_lengths",
                      "IKHand handler name");
  ok &= expect_string(handlers.superclasses[0], "CharWeightable",
                      "IKHand handler first superclass");
  ok &= expect_string(handlers.superclasses[1], "Hmx::Object",
                      "IKHand handler second superclass");
  ok &= expect_string(handlers.check, "0x33D", "IKHand handler check");

  const auto props = source_char_ik_hand_prop_sync_plan();
  ok &= expect_string(props.target_properties[0], "target",
                      "IKTarget prop target");
  ok &= expect_string(props.target_properties[1], "extent",
                      "IKTarget prop extent");
  ok &= expect_string(props.set_properties[0], "hand",
                      "IKHand set prop hand");
  ok &= expect_string(props.properties[0], "finger",
                      "IKHand first direct prop");
  ok &= expect_string(props.properties[11], "clockwise",
                      "IKHand last direct prop");
  ok &= expect_string(props.superclass, "CharWeightable",
                      "IKHand prop superclass");
  ok &= expect_bool(source_char_ik_hand_save_plan().save_id == 0x2A8,
                    true, "IKHand save id");
  const SourceCharIKHandSetHandResult set_hand =
      source_char_ik_hand_set_hand("bone_l_hand.mesh");
  ok &= expect_string(set_hand.assigned_hand, "bone_l_hand.mesh",
                      "IKHand SetHand assigns source row");
  ok &= expect_bool(set_hand.hand_changed, true,
                    "IKHand SetHand marks hand changed");
  const SourceCharIKHandSetHandResult clear_hand =
      source_char_ik_hand_set_hand("");
  ok &= expect_string(clear_hand.assigned_hand, "",
                      "IKHand SetHand preserves empty hand row");
  ok &= expect_bool(clear_hand.hand_changed, true,
                    "IKHand SetHand dirties empty hand row");

  const SourceCharIKHandMeasure missing =
      source_char_ik_hand_measure_lengths(false, 4.0f, 3.0f);
  ok &= expect_bool(missing.has_elbow_chain, false, "missing chain flag");
  ok &= expect_float(missing.inv_2ab, 0.0f, "missing chain inv_2ab");
  float cosine = 12.0f;
  ok &= expect_bool(source_char_ik_hand_elbow_cosine(missing, 25.0f, cosine),
                    false, "missing chain cosine rejected");
  ok &= expect_float(cosine, 12.0f, "missing chain leaves cosine");

  const SourceCharIKHandMeasure measure =
      source_char_ik_hand_measure_lengths(true, 4.0f, 3.0f);
  ok &= expect_bool(measure.has_elbow_chain, true, "measure chain flag");
  ok &= expect_float(measure.inv_2ab, 1.0f / 24.0f, "measure inv_2ab");
  ok &= expect_float(measure.a2_plus_b2, 25.0f, "measure a2 plus b2");
  ok &= expect_float(measure.aa_plus_bb, 7.0f, "measure reach sum");

  ok &= expect_bool(source_char_ik_hand_elbow_cosine(measure, 25.0f, cosine),
                    true, "cosine accepted");
  ok &= expect_float(cosine, 0.0f, "right-angle cosine");
  ok &= expect_bool(source_char_ik_hand_elbow_cosine(measure, 49.0f, cosine),
                    true, "max reach cosine accepted");
  ok &= expect_float(cosine, 1.0f, "max reach cosine");
  ok &= expect_bool(source_char_ik_hand_elbow_cosine(measure, 1.0f, cosine),
                    true, "folded cosine accepted");
  ok &= expect_float(cosine, -1.0f, "folded cosine");
  ok &= expect_bool(source_char_ik_hand_elbow_cosine(measure, 1000.0f, cosine),
                    true, "source high clamp accepted");
  ok &= expect_float(cosine, 1.0f, "source high clamp");
  ok &= expect_bool(source_char_ik_hand_elbow_cosine(measure, -1000.0f, cosine),
                    true, "source low clamp accepted");
  ok &= expect_float(cosine, -1.0f, "source low clamp");

  bool hand_changed = true;
  ok &= expect_bool(source_char_ik_hand_update_measure_lengths(
                        false, hand_changed),
                    true, "initial hand change measures");
  ok &= expect_bool(hand_changed, false, "initial hand change clears");
  ok &= expect_bool(source_char_ik_hand_update_measure_lengths(
                        false, hand_changed),
                    false, "non-scalable stable skips measure");
  ok &= expect_bool(hand_changed, false, "stable hand remains clear");
  ok &= expect_bool(source_char_ik_hand_update_measure_lengths(
                        true, hand_changed),
                    true, "scalable hand measures every poll");
  ok &= expect_bool(hand_changed, false, "scalable measure leaves clear");

  const auto target_blend = source_char_ik_hand_multi_target_blend(
      0.75f, {SourceCharIKHandTargetInput{true, {12.0f, 0.0f, 0.0f}, 0.0f, {}},
              SourceCharIKHandTargetInput{true, {0.0f, 6.0f, 0.0f}, 0.0f, {}}});
  ok &= expect_bool(target_blend.entered, true, "multi-target blend entered");
  ok &= expect_size(target_blend.weights.size(), 2,
                    "multi-target stores source weights");
  ok &= expect_float(target_blend.weights[0], 1.0f,
                     "first multi-target inverse-distance weight");
  ok &= expect_float(target_blend.weights[1], 4.0f,
                     "second multi-target inverse-distance weight");
  ok &= expect_float(target_blend.sum, 5.0f, "multi-target weight sum");
  ok &= expect_float(target_blend.adjusted_weight, 0.75f,
                     "multi-target keeps weight when sum high");
  ok &= expect_float(target_blend.blended_pos[0], 2.4f,
                     "multi-target blended x");
  ok &= expect_float(target_blend.blended_pos[1], 4.8f,
                     "multi-target blended y");

  const auto reduced_blend = source_char_ik_hand_multi_target_blend(
      0.8f, {SourceCharIKHandTargetInput{true, {120.0f, 0.0f, 0.0f}, 0.0f, {}}});
  ok &= expect_float(reduced_blend.sum, 0.01f,
                     "low-sum multi-target weight");
  ok &= expect_bool(reduced_blend.reduced_weight_for_low_sum, true,
                    "low-sum multi-target reduces char weight");
  ok &= expect_float(reduced_blend.adjusted_weight, 0.008f,
                     "low-sum multi-target adjusted weight");

  const auto extent_blend = source_char_ik_hand_multi_target_blend(
      1.0f, {SourceCharIKHandTargetInput{true, {0.0f, 4.0f, -10.0f}, 5.0f, {}},
             SourceCharIKHandTargetInput{true, {3.0f, 4.0f, -2.0f}, 5.0f, {}}});
  ok &= expect_float(extent_blend.weights[0], 0.001f,
                     "positive extent behind target uses source floor");
  ok &= expect_float(extent_blend.weights[1], 144.0f / 25.0f,
                     "positive extent projects z before length");
  ok &= expect_bool(extent_blend.blended_pos[0] > 2.99f &&
                        extent_blend.blended_pos[0] < 3.01f,
                    true, "positive extent blended x remains near second target");
  ok &= expect_bool(extent_blend.blended_pos[2] < -2.0f &&
                        extent_blend.blended_pos[2] > -2.01f,
                    true, "positive extent keeps source world z in blend");
  ok &= expect_bool(extent_blend.orientation_blended, false,
                    "orientation stays inert when disabled");

  const auto orientation_blend = source_char_ik_hand_multi_target_blend(
      1.0f,
      {SourceCharIKHandTargetInput{true,
                                   {12.0f, 0.0f, 0.0f},
                                   0.0f,
                                   std::array<float, 4>{0.0f, 0.0f, 0.0f,
                                                        1.0f}},
       SourceCharIKHandTargetInput{true,
                                   {-12.0f, 0.0f, 0.0f},
                                   0.0f,
                                   std::array<float, 4>{0.0f, 0.0f, 1.0f,
                                                        0.0f}}},
      true);
  ok &= expect_bool(orientation_blend.orientation_blended, true,
                    "orientation blend entered");
  ok &= expect_bool(orientation_blend.orientation_normalized, true,
                    "orientation blend normalized");
  ok &= expect_float(orientation_blend.blended_quat[0], 0.0f,
                     "orientation blend quat x");
  ok &= expect_float(orientation_blend.blended_quat[2], 0.70710677f,
                     "orientation blend quat z");
  ok &= expect_float(orientation_blend.blended_quat[3], 0.70710677f,
                     "orientation blend quat w");

  ghogx::milo_scene::Xfm hand_world;
  hand_world.pos[0] = 10.0f;
  ghogx::milo_scene::Xfm finger_world;
  finger_world.pos[0] = 2.0f;
  const auto no_finger_target = source_char_ik_hand_finger_target(
      false, hand_world, finger_world, {5.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f, 1.0f});
  ok &= expect_bool(no_finger_target.applied, false,
                    "finger target no-finger stays inert");
  ok &= expect_float(no_finger_target.adjusted_target.pos[0], 5.0f,
                     "finger target no-finger keeps position");

  const auto finger_target = source_char_ik_hand_finger_target(
      true, hand_world, finger_world, {5.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f, 1.0f});
  ok &= expect_bool(finger_target.applied, true,
                    "finger target applies source branch");
  ok &= expect_float(finger_target.adjusted_target.pos[0], 13.0f,
                     "finger target source transform x");
  ok &= expect_float(finger_target.adjusted_target.rot[0][0], 1.0f,
                     "finger target keeps identity rotation");

  SourceCharIKHandWristConstraintInput wrist_disabled;
  wrist_disabled.constrain_wrist = false;
  wrist_disabled.char_weight = 1.0f;
  wrist_disabled.has_parent = true;
  const auto wrist_gate = source_char_ik_hand_wrist_constraint(wrist_disabled);
  ok &= expect_bool(wrist_gate.entered, false,
                    "wrist constraint disabled skips source branch");

  SourceCharIKHandWristConstraintInput wrist_inside;
  wrist_inside.constrain_wrist = true;
  wrist_inside.char_weight = 1.0f;
  wrist_inside.has_parent = true;
  wrist_inside.wrist_radians = 0.1f;
  const auto wrist_inside_result =
      source_char_ik_hand_wrist_constraint(wrist_inside);
  ok &= expect_bool(wrist_inside_result.entered, true,
                    "wrist constraint enters with parent and weight");
  ok &= expect_bool(wrist_inside_result.angle_exceeded, false,
                    "wrist constraint inside limit does not write");
  ok &= expect_float(wrist_inside_result.raw_angle, 0.0f,
                     "wrist constraint zero raw angle");

  SourceCharIKHandWristConstraintInput wrist_exceeded;
  wrist_exceeded.constrain_wrist = true;
  wrist_exceeded.char_weight = 1.0f;
  wrist_exceeded.has_parent = true;
  wrist_exceeded.wrist_radians = 0.25f;
  wrist_exceeded.parent_x = {1.0f, 0.0f, 0.0f};
  wrist_exceeded.hand_x = {0.0f, 0.0f, -1.0f};
  wrist_exceeded.hand_y = {0.0f, 1.0f, 0.0f};
  wrist_exceeded.hand_z = {1.0f, 0.0f, 0.0f};
  wrist_exceeded.hand_pos = {10.0f, 20.0f, 30.0f};
  wrist_exceeded.finger_before_pos = {1.0f, 2.0f, 3.0f};
  wrist_exceeded.finger_after_first_set_pos = {1.5f, 1.0f, 5.25f};
  const auto wrist_exceeded_result =
      source_char_ik_hand_wrist_constraint(wrist_exceeded);
  ok &= expect_bool(wrist_exceeded_result.angle_exceeded, true,
                    "wrist constraint exceeded writes source branch");
  ok &= expect_float(wrist_exceeded_result.raw_angle, -1.57079637f,
                     "wrist constraint raw limit angle");
  ok &= expect_float(wrist_exceeded_result.correction_angle, -1.32079637f,
                     "wrist constraint corrected angle");
  ok &= expect_float(wrist_exceeded_result.corrected_x[0], 0.96891242f,
                     "wrist constraint corrected x row");
  ok &= expect_float(wrist_exceeded_result.corrected_z[2], 0.96891242f,
                     "wrist constraint rebuilt z row");
  ok &= expect_bool(wrist_exceeded_result.wrote_first_hand_xfm, true,
                    "wrist constraint writes first hand xfm");
  ok &= expect_float(wrist_exceeded_result.finger_delta[0], 0.5f,
                     "wrist constraint finger delta x");
  ok &= expect_float(wrist_exceeded_result.finger_delta[1], -1.0f,
                     "wrist constraint finger delta y");
  ok &= expect_float(wrist_exceeded_result.finger_delta[2], 2.25f,
                     "wrist constraint finger delta z");
  ok &= expect_float(wrist_exceeded_result.final_hand_pos[0], 9.5f,
                     "wrist constraint compensated hand x");
  ok &= expect_float(wrist_exceeded_result.final_hand_pos[1], 21.0f,
                     "wrist constraint compensated hand y");
  ok &= expect_float(wrist_exceeded_result.final_hand_pos[2], 27.75f,
                     "wrist constraint compensated hand z");
  ok &= expect_bool(wrist_exceeded_result.updates_world_dst, true,
                    "wrist constraint updates world dst");
  ok &= expect_bool(wrist_exceeded_result.requests_elbow_resolve, true,
                    "wrist constraint requests elbow solve");
  ok &= expect_bool(wrist_exceeded_result.rewrites_hand_after_elbow, true,
                    "wrist constraint rewrites hand after elbow");

  const auto elbow_swing_disabled = source_char_ik_hand_elbow_swing(
      SourceCharIKHandElbowSwingInput{0.0f, {4.0f, 0.0f}, {0.0f, 4.0f}});
  ok &= expect_bool(elbow_swing_disabled.entered, false,
                    "elbow swing disabled skips source branch");

  const auto elbow_swing_floor = source_char_ik_hand_elbow_swing(
      SourceCharIKHandElbowSwingInput{1.0f, {1.0f, 0.0f}, {0.0f, 1.0f}});
  ok &= expect_bool(elbow_swing_floor.entered, true,
                    "elbow swing enters for positive swing");
  ok &= expect_float(elbow_swing_floor.current_len_sq, 16.0f,
                     "elbow swing floors current length");
  ok &= expect_float(elbow_swing_floor.target_len_sq, 16.0f,
                     "elbow swing floors target length");
  ok &= expect_float(elbow_swing_floor.denom, 16.0f,
                     "elbow swing floor denominator");
  ok &= expect_float(elbow_swing_floor.cross, -1.0f,
                     "elbow swing 2D cross");
  ok &= expect_float(elbow_swing_floor.unclamped, -0.0625f,
                     "elbow swing unclamped floor ratio");
  ok &= expect_float(elbow_swing_floor.clamped, -0.0625f,
                     "elbow swing unclamped within limit");
  ok &= expect_float(elbow_swing_floor.rotate_about_x, 0.0625f,
                     "elbow swing rotates by negative clamped");

  const auto elbow_swing_negative_clamp = source_char_ik_hand_elbow_swing(
      SourceCharIKHandElbowSwingInput{0.5f, {4.0f, 0.0f}, {0.0f, 4.0f}});
  ok &= expect_float(elbow_swing_negative_clamp.unclamped, -1.0f,
                     "elbow swing negative unclamped");
  ok &= expect_float(elbow_swing_negative_clamp.clamped, -0.5f,
                     "elbow swing negative clamp");
  ok &= expect_float(elbow_swing_negative_clamp.rotate_about_x, 0.5f,
                     "elbow swing negative clamp rotate");
  ok &= expect_bool(
      elbow_swing_negative_clamp.recompute_current_after_rotation, true,
      "elbow swing recomputes current vector");

  const auto elbow_swing_positive_clamp = source_char_ik_hand_elbow_swing(
      SourceCharIKHandElbowSwingInput{0.25f, {0.0f, 4.0f}, {4.0f, 0.0f}});
  ok &= expect_float(elbow_swing_positive_clamp.unclamped, 1.0f,
                     "elbow swing positive unclamped");
  ok &= expect_float(elbow_swing_positive_clamp.clamped, 0.25f,
                     "elbow swing positive clamp");
  ok &= expect_float(elbow_swing_positive_clamp.rotate_about_x, -0.25f,
                     "elbow swing positive clamp rotate");

  const auto elbow_collide_none = source_char_ik_hand_elbow_collision_gate(
      SourceCharIKHandElbowCollisionInput{false, true, 0.0f, 1.0f, false});
  ok &= expect_bool(elbow_collide_none.entered, false,
                    "elbow collision skips without object");
  ok &= expect_bool(elbow_collide_none.final_shoulder_repull, true,
                    "elbow collision records final shoulder repull");

  const auto elbow_collide_non_sphere = source_char_ik_hand_elbow_collision_gate(
      SourceCharIKHandElbowCollisionInput{true, false, 0.0f, 1.0f, false});
  ok &= expect_bool(elbow_collide_non_sphere.entered, true,
                    "elbow collision enters with object");
  ok &= expect_bool(elbow_collide_non_sphere.needs_source_shoulder_offset, true,
                    "elbow collision needs source shoulder offset");
  ok &= expect_bool(elbow_collide_non_sphere.warn_non_sphere, true,
                    "elbow collision warns non-sphere");
  ok &= expect_bool(elbow_collide_non_sphere.sphere_branch, false,
                    "elbow collision rejects non-sphere branch");

  const auto elbow_collide_outside = source_char_ik_hand_elbow_collision_gate(
      SourceCharIKHandElbowCollisionInput{true, true, 4.0f, 4.0f, false});
  ok &= expect_bool(elbow_collide_outside.sphere_branch, true,
                    "elbow collision enters sphere branch");
  ok &= expect_bool(elbow_collide_outside.inside_sphere, false,
                    "elbow collision outside radius uses strict less-than");
  ok &= expect_bool(elbow_collide_outside.needs_collision_rotation, false,
                    "elbow collision outside radius skips rotation");

  const auto elbow_collide_ccw = source_char_ik_hand_elbow_collision_gate(
      SourceCharIKHandElbowCollisionInput{true, true, 2.0f, 4.0f, false});
  ok &= expect_bool(elbow_collide_ccw.inside_sphere, true,
                    "elbow collision detects inside sphere");
  ok &= expect_bool(elbow_collide_ccw.needs_collision_rotation, true,
                    "elbow collision inside sphere needs rotation");
  ok &= expect_bool(elbow_collide_ccw.uses_counterclockwise_candidate, true,
                    "elbow collision default candidate");
  ok &= expect_bool(elbow_collide_ccw.uses_clockwise_candidate, false,
                    "elbow collision default skips clockwise candidate");
  ok &= expect_bool(elbow_collide_ccw.updates_upper_arm_matrix, true,
                    "elbow collision updates upper arm matrix");
  ok &= expect_bool(elbow_collide_ccw.updates_forearm_matrix, true,
                    "elbow collision updates forearm matrix");

  const auto elbow_collide_clockwise = source_char_ik_hand_elbow_collision_gate(
      SourceCharIKHandElbowCollisionInput{true, true, 2.0f, 4.0f, true});
  ok &= expect_bool(elbow_collide_clockwise.uses_clockwise_candidate, true,
                    "elbow collision clockwise candidate");
  ok &= expect_bool(elbow_collide_clockwise.uses_counterclockwise_candidate,
                    false, "elbow collision clockwise skips default candidate");

  const auto poll_no_hand = source_char_ik_hand_poll_flow(
      SourceCharIKHandPollFlowInput{false, true, true, true, true, false,
                                    true, true, 1.0f});
  ok &= expect_bool(poll_no_hand.early_out, true,
                    "IKHand poll flow early-outs with no hand");

  const auto poll_no_grandparent = source_char_ik_hand_poll_flow(
      SourceCharIKHandPollFlowInput{true, true, true, false, true, false,
                                    false, false, 1.0f});
  ok &= expect_bool(poll_no_grandparent.calls_ik_elbow, true,
                    "IKHand poll flow still calls IKElbow");
  ok &= expect_bool(poll_no_grandparent.parent1_after_grandparent_gate, false,
                    "IKHand poll flow clears parent without grandparent");
  ok &= expect_bool(poll_no_grandparent.ik_elbow_has_chain, false,
                    "IKHand poll flow marks missing elbow chain");
  ok &= expect_bool(poll_no_grandparent.final_hand_write, true,
                    "IKHand poll flow writes final hand without parent");
  ok &= expect_bool(poll_no_grandparent.final_position_from_world_dst, true,
                    "IKHand poll flow writes mWorldDst without parent");
  ok &= expect_bool(poll_no_grandparent.final_orientation_from_target, false,
                    "IKHand poll flow skips orientation when disabled");

  const auto poll_elbow_only = source_char_ik_hand_poll_flow(
      SourceCharIKHandPollFlowInput{true, true, true, true, true, false,
                                    false, false, 1.0f});
  ok &= expect_bool(poll_elbow_only.calls_ik_elbow, true,
                    "IKHand poll flow calls elbow with chain");
  ok &= expect_bool(poll_elbow_only.ik_elbow_has_chain, true,
                    "IKHand poll flow marks elbow chain");
  ok &= expect_bool(poll_elbow_only.final_hand_write, false,
                    "IKHand poll flow skips final write for parented no orient/stretch");

  const auto poll_orient_stretch = source_char_ik_hand_poll_flow(
      SourceCharIKHandPollFlowInput{true, true, true, true, true, false,
                                    true, true, 0.5f});
  ok &= expect_bool(poll_orient_stretch.final_hand_write, true,
                    "IKHand poll flow writes final hand with orient/stretch");
  ok &= expect_bool(poll_orient_stretch.final_position_from_world_dst, true,
                    "IKHand poll flow stretch writes mWorldDst");
  ok &= expect_bool(poll_orient_stretch.final_orientation_from_target, true,
                    "IKHand poll flow orientation writes target rot");
  ok &= expect_bool(poll_orient_stretch.interpolates_orientation, true,
                    "IKHand poll flow interpolates partial orientation");

  const auto poll_always_elbow = source_char_ik_hand_poll_flow(
      SourceCharIKHandPollFlowInput{true, true, true, true, true, true,
                                    false, false, 0.0f});
  ok &= expect_bool(poll_always_elbow.calls_ik_elbow, true,
                    "IKHand poll flow always IK elbow calls at zero weight");
  ok &= expect_bool(poll_always_elbow.final_hand_write, false,
                    "IKHand poll flow zero weight skips final hand write");

  std::printf("character_ik_hand_source_test %s\n", ok ? "OK" : "FAIL");
  return ok ? 0 : 1;
}
