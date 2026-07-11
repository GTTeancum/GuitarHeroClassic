#include "character/char_mesh.h"

#include <cmath>
#include <iostream>

namespace {

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharFaceServoBlinkClips;
  using ghogx::character::SourceCharFaceServoBlinkState;
  using ghogx::character::source_char_face_servo_scale_add_blink;

  bool ok = true;
  const SourceCharFaceServoBlinkClips clips{
      "blink_L", "blink_L2", "blink_R", "blink_R2"};

  SourceCharFaceServoBlinkState state;
  state.left = 0.3f;
  state.right = 0.4f;
  state.need_scale_down = true;
  auto result =
      source_char_face_servo_scale_add_blink(state, clips, "blink_L2", true,
                                             0.25f);
  ok &= expect_bool(result.accepted, true, "left accepted");
  ok &= expect_bool(result.scale_down, true, "left scale-down reset");
  ok &= expect_bool(result.matched_left, true, "left matched");
  ok &= expect_bool(result.matched_right, false, "left not right");
  ok &= expect_bool(state.need_scale_down, false, "scale-down consumed");
  ok &= near(state.left, 0.25f, "left after reset/add");
  ok &= near(state.right, 0.0f, "right reset");

  result = source_char_face_servo_scale_add_blink(state, clips, "blink_R",
                                                  true, 0.8f);
  ok &= expect_bool(result.accepted, true, "right accepted");
  ok &= expect_bool(result.scale_down, false, "right no scale-down");
  ok &= expect_bool(result.matched_left, false, "right not left");
  ok &= expect_bool(result.matched_right, true, "right matched");
  ok &= near(state.left, 0.25f, "left preserved");
  ok &= near(state.right, 0.8f, "right add");

  result = source_char_face_servo_scale_add_blink(state, clips, "blink_R2",
                                                  true, 0.5f);
  ok &= expect_bool(result.matched_right, true, "right2 matched");
  ok &= near(state.right, 1.0f, "right clamps");

  const SourceCharFaceServoBlinkClips overlapping{
      "same", "", "same", ""};
  result = source_char_face_servo_scale_add_blink(state, overlapping, "same",
                                                  true, 0.25f);
  ok &= expect_bool(result.matched_left, true, "overlap left wins");
  ok &= expect_bool(result.matched_right, false, "overlap skips right");
  ok &= near(state.left, 0.5f, "overlap left add");
  ok &= near(state.right, 1.0f, "overlap right unchanged");

  SourceCharFaceServoBlinkState rejected = state;
  result = source_char_face_servo_scale_add_blink(rejected, clips, "blink_L",
                                                  false, 0.5f);
  ok &= expect_bool(result.accepted, false, "non-relative rejected");
  ok &= near(rejected.left, state.left, "non-relative left unchanged");
  ok &= near(rejected.right, state.right, "non-relative right unchanged");

  result = source_char_face_servo_scale_add_blink(rejected, clips, "blink_L",
                                                  true, -0.1f);
  ok &= expect_bool(result.accepted, false, "negative weight rejected");
  ok &= near(rejected.left, state.left, "negative left unchanged");
  ok &= near(rejected.right, state.right, "negative right unchanged");

  return ok ? 0 : 1;
}
