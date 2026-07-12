#include "character/char_mesh.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_int(int got, int want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_size(size_t got, size_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_string(const std::string& got, const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::source_char_eye_dart_ruleset_copy;
  using ghogx::character::source_char_eye_dart_ruleset_copy_plan;
  using ghogx::character::source_char_eye_dart_ruleset_defaults;
  using ghogx::character::source_char_eye_dart_ruleset_handler_plan;
  using ghogx::character::source_char_eye_dart_ruleset_load_plan;
  using ghogx::character::source_char_eye_dart_ruleset_load_revision_known;
  using ghogx::character::source_char_eye_dart_ruleset_prop_sync_plan;

  bool ok = true;

  auto defaults = source_char_eye_dart_ruleset_defaults();
  ok &= near(defaults.min_radius, 0.5f, "default min radius");
  ok &= near(defaults.max_radius, 3.0f, "default max radius");
  ok &= near(defaults.on_target_angle_thresh, 5.0f,
             "default on target angle");
  ok &= expect_int(defaults.min_darts_per_sequence, 2,
                   "default min darts");
  ok &= expect_int(defaults.max_darts_per_sequence, 5,
                   "default max darts");
  ok &= near(defaults.min_secs_between_darts, 0.25f,
             "default min dart seconds");
  ok &= near(defaults.max_secs_between_darts, 0.65f,
             "default max dart seconds");
  ok &= near(defaults.min_secs_between_sequences, 1.0f,
             "default min sequence seconds");
  ok &= near(defaults.max_secs_between_sequences, 2.0f,
             "default max sequence seconds");
  ok &= expect_bool(defaults.scale_with_distance, true,
                    "default scale with distance");
  ok &= near(defaults.reference_distance, 70.0f,
             "default reference distance");

  ok &= expect_bool(source_char_eye_dart_ruleset_load_revision_known(-1),
                    false, "revision -1 rejected");
  ok &= expect_bool(source_char_eye_dart_ruleset_load_revision_known(0),
                    true, "revision 0 accepted");
  ok &= expect_bool(source_char_eye_dart_ruleset_load_revision_known(1),
                    true, "revision 1 accepted");
  ok &= expect_bool(source_char_eye_dart_ruleset_load_revision_known(2),
                    false, "revision 2 rejected");

  const auto rejected_load = source_char_eye_dart_ruleset_load_plan(2);
  ok &= expect_bool(rejected_load.known_revision, false,
                    "load plan rejects high revision");
  ok &= expect_size(rejected_load.read_order.size(), 0,
                    "rejected load has no rows");
  const auto load = source_char_eye_dart_ruleset_load_plan(1);
  ok &= expect_bool(load.known_revision, true, "load plan accepts rev 1");
  ok &= expect_size(load.read_order.size(), 12, "load row count");
  ok &= expect_string(load.read_order[0], "Hmx::Object",
                      "load object superclass");
  ok &= expect_string(load.read_order[1], "mData.mMinRadius",
                      "load min radius first");
  ok &= expect_string(load.read_order[10], "mData.mScaleWithDistance",
                      "load scale with distance");
  ok &= expect_string(load.read_order[11], "mData.mReferenceDistance",
                      "load reference distance");

  defaults.min_radius = 1.25f;
  defaults.max_radius = 9.5f;
  defaults.on_target_angle_thresh = 11.0f;
  defaults.min_darts_per_sequence = 3;
  defaults.max_darts_per_sequence = 7;
  defaults.min_secs_between_darts = 0.15f;
  defaults.max_secs_between_darts = 0.85f;
  defaults.min_secs_between_sequences = 1.5f;
  defaults.max_secs_between_sequences = 3.5f;
  defaults.scale_with_distance = false;
  defaults.reference_distance = 42.0f;

  const auto copied = source_char_eye_dart_ruleset_copy(defaults);
  ok &= near(copied.min_radius, 1.25f, "copy min radius");
  ok &= near(copied.max_radius, 1.25f,
             "copy max radius follows source min-radius assignment");
  ok &= near(copied.on_target_angle_thresh, 11.0f,
             "copy angle threshold");
  ok &= expect_int(copied.min_darts_per_sequence, 3,
                   "copy min darts");
  ok &= expect_int(copied.max_darts_per_sequence, 7,
                   "copy max darts");
  ok &= near(copied.min_secs_between_darts, 0.15f,
             "copy min dart seconds");
  ok &= near(copied.max_secs_between_darts, 0.85f,
             "copy max dart seconds");
  ok &= near(copied.min_secs_between_sequences, 1.5f,
             "copy min sequence seconds");
  ok &= near(copied.max_secs_between_sequences, 3.5f,
             "copy max sequence seconds");
  ok &= expect_bool(copied.scale_with_distance, false,
                    "copy scale with distance");
  ok &= near(copied.reference_distance, 42.0f,
             "copy reference distance");

  const auto copy_plan = source_char_eye_dart_ruleset_copy_plan();
  ok &= expect_size(copy_plan.copied_superclasses.size(), 1,
                    "copy superclass count");
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "copy object superclass");
  ok &= expect_bool(copy_plan.max_radius_from_min_radius, true,
                    "copy plan max-radius quirk");
  ok &= expect_size(copy_plan.copied_members.size(), 10,
                    "copy member count");
  ok &= expect_string(copy_plan.copied_members[0], "mData.mMinRadius",
                      "copy min radius first");
  ok &= expect_string(copy_plan.copied_members[1],
                      "mData.mOnTargetAngleThresh",
                      "copy skips direct max radius");

  const auto props = source_char_eye_dart_ruleset_prop_sync_plan();
  ok &= expect_size(props.properties.size(), 11, "prop row count");
  ok &= expect_string(props.properties[0], "min_radius",
                      "prop min radius");
  ok &= expect_string(props.properties[1], "max_radius",
                      "prop max radius");
  ok &= expect_string(props.properties[10], "reference_distance",
                      "prop reference distance");

  const auto handlers = source_char_eye_dart_ruleset_handler_plan();
  ok &= expect_size(handlers.superclasses.size(), 1,
                    "handler superclass count");
  ok &= expect_string(handlers.superclasses[0], "Hmx::Object",
                      "handler object superclass");
  ok &= expect_int(handlers.check, 0xd4, "handler check");

  return ok ? 0 : 1;
}
