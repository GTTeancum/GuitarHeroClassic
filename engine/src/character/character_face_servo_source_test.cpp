#include "character/char_mesh.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>

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

bool expect_int(int got, int want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_size(std::size_t got, std::size_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_string(const std::string& got,
                   const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharFaceServoBlinkClips;
  using ghogx::character::SourceCharFaceServoBlinkState;
  using ghogx::character::source_char_face_servo_apply_procedural_weights;
  using ghogx::character::source_char_face_servo_copy_plan;
  using ghogx::character::source_char_face_servo_enter_plan;
  using ghogx::character::source_char_face_servo_handler_plan;
  using ghogx::character::source_char_face_servo_load_plan;
  using ghogx::character::source_char_face_servo_poll_deps_plan;
  using ghogx::character::source_char_face_servo_poll_plan;
  using ghogx::character::source_char_face_servo_procedural_weights_plan;
  using ghogx::character::source_char_face_servo_prop_sync_plan;
  using ghogx::character::source_char_face_servo_save_plan;
  using ghogx::character::source_char_face_servo_scale_add_blink;
  using ghogx::character::source_char_face_servo_set_clip_type_plan;
  using ghogx::character::source_char_face_servo_set_clips_plan;
  using ghogx::character::source_char_face_servo_try_scale_down;

  bool ok = true;
  const auto load_unknown = source_char_face_servo_load_plan(5);
  ok &= expect_bool(load_unknown.known_revision, false,
                    "load rejects high revision");
  ok &= expect_size(load_unknown.read_order.size(), 0,
                    "unknown load has no reads");
  const auto load_legacy = source_char_face_servo_load_plan(0);
  ok &= expect_bool(load_legacy.known_revision, true,
                    "load accepts revision 0");
  ok &= expect_string(load_legacy.read_order[0], "Hmx::Object",
                      "legacy load object");
  ok &= expect_string(load_legacy.read_order[1], "clipObjectDir",
                      "legacy load clip dir");
  ok &= expect_size(load_legacy.branches.size(), 2,
                    "legacy load derives clip type");
  ok &= expect_string(load_legacy.branches[0], "deriveClipTypeFromDirType",
                      "legacy load dir type branch");
  ok &= expect_string(load_legacy.read_order.back(), "SetClipType",
                      "legacy load calls SetClipType");
  const auto load_latest = source_char_face_servo_load_plan(4);
  ok &= expect_bool(load_latest.known_revision, true,
                    "load accepts revision 4");
  ok &= expect_size(load_latest.read_order.size(), 9,
                    "latest load read count");
  ok &= expect_string(load_latest.read_order[2], "clipTypeSymbol",
                      "latest load clip type");
  ok &= expect_string(load_latest.read_order[3], "mBlinkClipLeftName",
                      "latest load left blink");
  ok &= expect_string(load_latest.read_order[7], "SetClips",
                      "latest load SetClips");
  ok &= expect_bool(load_latest.calls_set_clips, true,
                    "latest load calls set clips");
  ok &= expect_bool(load_latest.calls_set_clip_type, true,
                    "latest load calls set clip type");

  const auto copy_plan = source_char_face_servo_copy_plan();
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "copy superclass");
  ok &= expect_size(copy_plan.copied_members.size(), 6,
                    "copy member count");
  ok &= expect_string(copy_plan.copied_members[0], "mBlinkWeightLeft",
                      "copy first member");
  ok &= expect_string(copy_plan.copied_members.back(),
                      "mBlinkClipRightName2", "copy last member");
  ok &= expect_string(copy_plan.post_copy_calls[0], "SetClips(c->mClips)",
                      "copy SetClips call");
  ok &= expect_string(copy_plan.post_copy_calls[1],
                      "SetClipType(c->mClipType)", "copy SetClipType call");

  const auto handler_plan = source_char_face_servo_handler_plan();
  ok &= expect_string(handler_plan.superclasses[0], "Hmx::Object",
                      "handler superclass");
  ok &= expect_int(handler_plan.check, 0x119, "handler check");
  ok &= expect_int(source_char_face_servo_save_plan().save_id, 0xCE,
                   "CharFaceServo save id");

  const auto prop_sync = source_char_face_servo_prop_sync_plan();
  ok &= expect_string(prop_sync.set_properties[0], "clips",
                      "prop clips");
  ok &= expect_string(prop_sync.set_actions[1], "SetClipType",
                      "prop SetClipType action");
  ok &= expect_string(prop_sync.properties[0], "blink_clip_left",
                      "prop left blink");
  ok &= expect_string(prop_sync.properties.back(), "blink_clip_right2",
                      "prop right2 blink");
  ok &= expect_string(prop_sync.superclasses[0], "CharBonesMeshes",
                      "prop superclass");

  const auto enter_plan = source_char_face_servo_enter_plan();
  ok &= expect_string(enter_plan.calls[0], "RndPollable::Enter",
                      "enter superclass");
  ok &= expect_bool(enter_plan.need_scale_down, true,
                    "enter sets scale-down");
  ok &= near(enter_plan.procedural_blink_weight, 0.0f,
             "enter clears procedural weight");

  SourceCharFaceServoBlinkState try_scale_state;
  try_scale_state.left = 0.4f;
  try_scale_state.right = 0.7f;
  auto try_scale_result =
      source_char_face_servo_try_scale_down(try_scale_state, true, true);
  ok &= expect_bool(try_scale_result.consumed_need_scale_down, false,
                    "TryScaleDown skips when not requested");
  ok &= near(try_scale_state.left, 0.4f,
             "TryScaleDown preserves left when skipped");
  try_scale_state.need_scale_down = true;
  try_scale_result =
      source_char_face_servo_try_scale_down(try_scale_state, true, false);
  ok &= expect_bool(try_scale_result.consumed_need_scale_down, true,
                    "TryScaleDown consumes requested reset");
  ok &= expect_bool(try_scale_result.invoked_base_scale_down, false,
                    "TryScaleDown requires clip type for base scale");
  ok &= expect_bool(try_scale_result.reset_blink_weights, true,
                    "TryScaleDown resets blink weights");
  ok &= near(try_scale_state.left, 0.0f, "TryScaleDown clears left weight");
  ok &= near(try_scale_state.right, 0.0f, "TryScaleDown clears right weight");
  ok &= expect_bool(try_scale_state.need_scale_down, false,
                    "TryScaleDown clears flag");
  try_scale_state.left = 0.9f;
  try_scale_state.right = 0.2f;
  try_scale_state.need_scale_down = true;
  try_scale_result =
      source_char_face_servo_try_scale_down(try_scale_state, true, true);
  ok &= expect_bool(try_scale_result.invoked_base_scale_down, true,
                    "TryScaleDown calls base ScaleDown when ready");

  const auto set_clips_plan = source_char_face_servo_set_clips_plan();
  ok &= expect_bool(set_clips_plan.assigns_clips, true,
                    "SetClips assigns dir");
  ok &= expect_size(set_clips_plan.clip_lookups.size(), 5,
                    "SetClips lookup count");
  ok &= expect_string(set_clips_plan.clip_lookups[0], "Base",
                      "SetClips base lookup");
  ok &= expect_string(set_clips_plan.clip_lookups.back(),
                      "mBlinkClipRightName2", "SetClips right2 lookup");

  const auto unchanged_clip_type =
      source_char_face_servo_set_clip_type_plan(false);
  ok &= expect_size(unchanged_clip_type.changed_calls.size(), 0,
                    "unchanged SetClipType no calls");
  const auto changed_clip_type =
      source_char_face_servo_set_clip_type_plan(true);
  ok &= expect_string(changed_clip_type.changed_calls[1], "ClearBones",
                      "changed SetClipType clears bones");
  ok &= expect_string(changed_clip_type.changed_calls[2],
                      "CharBoneDir::StuffBones",
                      "changed SetClipType stuffs bones");

  const auto poll_without_base = source_char_face_servo_poll_plan(false);
  ok &= expect_size(poll_without_base.base_clip_calls.size(), 0,
                    "poll without base skips pose");
  ok &= expect_bool(poll_without_base.sets_need_scale_down, true,
                    "poll sets scale-down after body");
  const auto poll_with_base = source_char_face_servo_poll_plan(true);
  ok &= expect_size(poll_with_base.base_clip_calls.size(), 4,
                    "poll with base call count");
  ok &= expect_string(poll_with_base.base_clip_calls[0], "TryScaleDown",
                      "poll TryScaleDown");
  ok &= expect_string(poll_with_base.base_clip_calls[3], "PoseMeshes",
                      "poll PoseMeshes");
  ok &= expect_bool(poll_with_base.clears_applied_procedural_blink, true,
                    "poll clears applied procedural blink");

  const auto procedural_skip =
      source_char_face_servo_procedural_weights_plan(false, false);
  ok &= expect_size(procedural_skip.calls.size(), 0,
                    "procedural skip no calls");
  const auto procedural_applied =
      source_char_face_servo_procedural_weights_plan(true, false);
  ok &= expect_size(procedural_applied.calls.size(), 4,
                    "procedural call count");
  ok &= expect_string(procedural_applied.calls[1], "left blink ScaleAdd",
                      "procedural left add");
  ok &= expect_bool(procedural_applied.skips_right_when_same_as_left, true,
                    "procedural skips duplicate right clip");
  ok &= expect_bool(source_char_face_servo_poll_deps_plan()
                        .change_list_gets_stuff_meshes,
                    true, "PollDeps stuffs meshes");

  SourceCharFaceServoBlinkState procedural_state;
  procedural_state.left = 0.25f;
  procedural_state.right = 0.5f;
  auto procedural_result = source_char_face_servo_apply_procedural_weights(
      procedural_state, 0.4f, false, true, true, false);
  ok &= expect_bool(procedural_result.accepted, true,
                    "procedural concrete accepted");
  ok &= expect_bool(procedural_result.scale_down, false,
                    "procedural concrete no scale-down");
  ok &= expect_bool(procedural_result.left_applied, true,
                    "procedural concrete left applied");
  ok &= expect_bool(procedural_result.right_applied, true,
                    "procedural concrete right applied");
  ok &= near(procedural_result.left_weight, 0.3f,
             "procedural concrete left weight");
  ok &= near(procedural_result.right_weight, 0.2f,
             "procedural concrete right weight");
  ok &= expect_bool(procedural_result.applied_procedural_blink, true,
                    "procedural concrete marks applied");

  procedural_result = source_char_face_servo_apply_procedural_weights(
      procedural_state, 0.4f, true, true, true, false);
  ok &= expect_bool(procedural_result.accepted, false,
                    "procedural already applied skipped");

  procedural_result = source_char_face_servo_apply_procedural_weights(
      procedural_state, 0.0f, false, true, true, false);
  ok &= expect_bool(procedural_result.accepted, false,
                    "procedural zero weight skipped");

  procedural_state.left = 0.8f;
  procedural_state.right = 0.1f;
  procedural_state.need_scale_down = true;
  procedural_result = source_char_face_servo_apply_procedural_weights(
      procedural_state, 0.5f, false, true, true, true);
  ok &= expect_bool(procedural_result.scale_down, true,
                    "procedural scale-down consumed");
  ok &= expect_bool(procedural_result.left_applied, true,
                    "procedural duplicate left applied");
  ok &= expect_bool(procedural_result.right_applied, false,
                    "procedural duplicate right skipped");
  ok &= near(procedural_result.left_weight, 0.5f,
             "procedural reset left weight");
  ok &= near(procedural_result.right_weight, 0.0f,
             "procedural duplicate right weight");
  ok &= near(procedural_state.left, 0.0f, "procedural reset state left");
  ok &= near(procedural_state.right, 0.0f, "procedural reset state right");
  ok &= expect_bool(procedural_state.need_scale_down, false,
                    "procedural reset state need scale-down");

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
