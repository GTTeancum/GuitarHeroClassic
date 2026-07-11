#include "character/char_mesh.h"

#include <cmath>
#include <iostream>

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

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::source_char_eye_dart_ruleset_copy;
  using ghogx::character::source_char_eye_dart_ruleset_defaults;
  using ghogx::character::source_char_eye_dart_ruleset_load_revision_known;

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

  return ok ? 0 : 1;
}
