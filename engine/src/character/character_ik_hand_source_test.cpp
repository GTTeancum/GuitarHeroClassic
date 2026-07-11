#include "character/char_clip.h"

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
  using ghogx::character::source_char_ik_hand_copy_plan;
  using ghogx::character::source_char_ik_hand_elbow_cosine;
  using ghogx::character::source_char_ik_hand_handler_plan;
  using ghogx::character::source_char_ik_hand_load_plan;
  using ghogx::character::source_char_ik_hand_measure_lengths;
  using ghogx::character::source_char_ik_hand_prop_sync_plan;
  using ghogx::character::source_char_ik_hand_update_measure_lengths;

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

  std::printf("character_ik_hand_source_test %s\n", ok ? "OK" : "FAIL");
  return ok ? 0 : 1;
}
