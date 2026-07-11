#include "character/char_clip.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << ": got " << got << " want " << want << "\n";
  return false;
}

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got " << got << " want " << want << "\n";
  return false;
}

bool expect_size(size_t got, size_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got " << got << " want " << want << "\n";
  return false;
}

bool expect_string(const std::string& got, const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got '" << got << "' want '" << want << "'\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharLookAtPollDeps;
  using ghogx::character::source_char_lookat_copy_plan;
  using ghogx::character::source_char_lookat_enter;
  using ghogx::character::source_char_lookat_load_plan;
  using ghogx::character::source_char_lookat_poll_deps;
  using ghogx::character::source_char_lookat_poll_plan;
  using ghogx::character::source_char_lookat_sync_limits;

  bool ok = true;

  const auto defaults = source_char_lookat_sync_limits(-80.0f, 80.0f,
                                                       -80.0f, 80.0f);
  ok &= near(defaults.min[1], 0.17364818f, "default min y");
  ok &= near(defaults.max[1], 1.0e29f, "default max y");
  ok &= near(defaults.min[2], -0.98480779f, "default min yaw z");
  ok &= near(defaults.max[2], 0.98480779f, "default max yaw z");
  ok &= near(defaults.min[0], -0.98480779f, "default min pitch x");
  ok &= near(defaults.max[0], 0.98480779f, "default max pitch x");

  const auto clamped = source_char_lookat_sync_limits(-120.0f, 120.0f,
                                                      -90.0f, 90.0f);
  ok &= near(clamped.min[1], defaults.min[1], "clamped min y");
  ok &= near(clamped.min[2], defaults.min[2], "clamped min yaw z");
  ok &= near(clamped.max[2], defaults.max[2], "clamped max yaw z");
  ok &= near(clamped.min[0], defaults.min[0], "clamped min pitch x");
  ok &= near(clamped.max[0], defaults.max[0], "clamped max pitch x");

  const auto asymmetric = source_char_lookat_sync_limits(-30.0f, 45.0f,
                                                         -10.0f, 20.0f);
  ok &= near(asymmetric.min[1], 0.70710677f, "asymmetric min y");
  ok &= near(asymmetric.min[2], -0.40824831f, "asymmetric min yaw z");
  ok &= near(asymmetric.max[2], 0.70710677f, "asymmetric max yaw z");
  ok &= near(asymmetric.min[0], -0.12468200f, "asymmetric min pitch x");
  ok &= near(asymmetric.max[0], 0.25735635f, "asymmetric max pitch x");

  const auto load_v0 = source_char_lookat_load_plan(0);
  ok &= expect_bool(load_v0.revision_supported, true,
                    "load v0 supported");
  ok &= expect_size(load_v0.read_order.size(), 10, "load v0 read count");
  ok &= expect_string(load_v0.read_order[0], "Hmx::Object",
                      "load v0 object first");
  ok &= expect_string(load_v0.read_order[1], "CharWeightable",
                      "load v0 weightable");
  ok &= expect_string(load_v0.read_order[9], "mMaxPitch",
                      "load v0 max pitch last");
  ok &= expect_string(load_v0.branches[0], "mAllowRoll=true",
                      "load v0 allow-roll default");
  ok &= expect_string(load_v0.branches[1], "mEnableJitter=false",
                      "load v0 jitter default");
  ok &= expect_bool(load_v0.sync_limits, true, "load v0 sync limits");

  const auto load_v2 = source_char_lookat_load_plan(2);
  ok &= expect_size(load_v2.read_order.size(), 13, "load v2 read count");
  ok &= expect_string(load_v2.read_order[10], "mMinWeightYaw",
                      "load v2 min weight yaw");
  ok &= expect_string(load_v2.read_order[12], "mWeightYawSpeed",
                      "load v2 yaw speed");
  ok &= expect_string(load_v2.branches[0], "mAllowRoll=true",
                      "load v2 default allow roll");

  const auto load_v4 = source_char_lookat_load_plan(4);
  ok &= expect_string(load_v4.read_order[13], "mAllowRoll",
                      "load v4 reads allow roll");
  ok &= expect_string(load_v4.read_order[14], "mEnableJitter",
                      "load v4 reads jitter flag");
  ok &= expect_string(load_v4.read_order[16], "mYawJitterLimit",
                      "load v4 reads yaw jitter");

  const auto load_v5 = source_char_lookat_load_plan(5);
  ok &= expect_string(load_v5.read_order.back(), "mSourceRadius",
                      "load v5 source radius last");
  ok &= expect_bool(source_char_lookat_load_plan(6).revision_supported, false,
                    "load rejects high revision");

  const auto copy_plan = source_char_lookat_copy_plan();
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "copy object superclass");
  ok &= expect_string(copy_plan.copied_superclasses[1], "CharWeightable",
                      "copy weightable superclass");
  ok &= expect_string(copy_plan.copied_members[0], "mSource",
                      "copy source first");
  ok &= expect_string(copy_plan.copied_members[12], "mSourceRadius",
                      "copy source radius");
  ok &= expect_string(copy_plan.copied_members.back(), "mPitchJitterLimit",
                      "copy pitch jitter last");
  ok &= expect_bool(copy_plan.sync_limits, true, "copy sync limits");

  const auto entered = source_char_lookat_enter(true);
  ok &= near(entered.smoothed_dir[0], 1.0e29f, "enter smoothed dir x");
  ok &= near(entered.smoothed_dir[1], 0.0f, "enter smoothed dir y");
  ok &= near(entered.smoothed_dir[2], 0.0f, "enter smoothed dir z");
  if (!entered.reset_pivot_local) {
    std::cerr << "enter should request pivot local identity\n";
    ok = false;
  }
  const auto no_pivot_enter = source_char_lookat_enter(false);
  if (no_pivot_enter.reset_pivot_local) {
    std::cerr << "enter should not reset missing pivot\n";
    ok = false;
  }

  SourceCharLookAtPollDeps deps;
  source_char_lookat_poll_deps(deps, "explicit.source", "pivot.lookat",
                               "target.lookat");
  if (deps.changed_by.size() != 2 || deps.changed_by[0] != "explicit.source" ||
      deps.changed_by[1] != "target.lookat" || deps.change.size() != 1 ||
      deps.change[0] != "pivot.lookat") {
    std::cerr << "poll deps explicit source mismatch\n";
    ok = false;
  }

  SourceCharLookAtPollDeps fallback_deps;
  source_char_lookat_poll_deps(fallback_deps, "", "pivot.lookat",
                               "target.lookat");
  if (fallback_deps.changed_by.size() != 2 ||
      fallback_deps.changed_by[0] != "pivot.lookat" ||
      fallback_deps.changed_by[1] != "target.lookat" ||
      fallback_deps.change.size() != 1 ||
      fallback_deps.change[0] != "pivot.lookat") {
    std::cerr << "poll deps pivot fallback mismatch\n";
    ok = false;
  }

  const auto inert_plan = source_char_lookat_poll_plan(
      true, true, false, true, 1.0f, 1.0f, -1.0f, 0.0f, false, false,
      0.0f, false, false, false, false, false, true);
  if (inert_plan.poll_gate_open || inert_plan.compute_dest_vector ||
      inert_plan.write_roll_local_rotation) {
    std::cerr << "poll plan should stay inert without a destination\n";
    ok = false;
  }

  const auto roll_plan = source_char_lookat_poll_plan(
      true, true, true, true, 1.0f, 1.0f, -1.0f, 0.0f, false, false,
      0.0f, false, false, false, false, false, true);
  if (!roll_plan.poll_gate_open || !roll_plan.compute_dest_vector ||
      roll_plan.apply_weight_yaw || roll_plan.skip_zero_weight ||
      !roll_plan.write_pivot_world_to_source ||
      roll_plan.normalize_dest_vector || !roll_plan.transform_to_parent_space ||
      !roll_plan.clamp_bounds || roll_plan.smooth_half_time ||
      roll_plan.use_test_range || roll_plan.use_show_range ||
      roll_plan.apply_jitter || roll_plan.subtract_source_radius_offset ||
      !roll_plan.write_roll_local_rotation || roll_plan.write_no_roll_axes) {
    std::cerr << "poll plan roll branch mismatch\n";
    ok = false;
  }

  const auto no_roll_plan = source_char_lookat_poll_plan(
      true, true, true, true, 0.25f, 0.75f, 0.0f, 5.0f, true, true,
      0.5f, false, true, true, false, false, false);
  if (!no_roll_plan.poll_gate_open || !no_roll_plan.apply_weight_yaw ||
      !no_roll_plan.update_source_radius_history ||
      !no_roll_plan.clamp_source_radius_offset ||
      no_roll_plan.write_pivot_world_to_source ||
      !no_roll_plan.normalize_dest_vector ||
      !no_roll_plan.smooth_half_time || no_roll_plan.use_test_range ||
      !no_roll_plan.use_show_range || !no_roll_plan.apply_jitter ||
      !no_roll_plan.subtract_source_radius_offset ||
      no_roll_plan.write_roll_local_rotation ||
      !no_roll_plan.write_no_roll_axes) {
    std::cerr << "poll plan no-roll/radius branch mismatch\n";
    ok = false;
  }

  const auto zero_weight_plan = source_char_lookat_poll_plan(
      true, true, true, true, 1.0f, 0.0f, 0.0f, 5.0f, false, true,
      0.5f, true, false, true, false, false, true);
  if (!zero_weight_plan.poll_gate_open || !zero_weight_plan.apply_weight_yaw ||
      !zero_weight_plan.skip_zero_weight ||
      zero_weight_plan.clamp_source_radius_offset ||
      zero_weight_plan.write_roll_local_rotation) {
    std::cerr << "poll plan zero-weight branch mismatch\n";
    ok = false;
  }

  return ok ? 0 : 1;
}
