#include "character/char_mesh.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_int(int32_t got, int32_t want, const char* label) {
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
  using ghogx::character::SourceCharacterTestBones;
  using ghogx::character::SourceCharacterTestExisting;
  using ghogx::character::SourceCharacterTestPollInput;
  using ghogx::character::source_character_test_add_defaults;
  using ghogx::character::source_character_test_default_state;
  using ghogx::character::source_character_test_destroy;
  using ghogx::character::source_character_test_draw;
  using ghogx::character::source_character_test_load;
  using ghogx::character::source_character_test_poll;
  using ghogx::character::source_character_test_set_move_self;
  using ghogx::character::source_character_test_set_start_end_beat;
  using ghogx::character::source_character_test_teleport_to;
  using ghogx::character::source_character_test_walk;

  bool ok = true;

  auto state = source_character_test_default_state();
  ok &= expect_string(state.show_dist_map, "none", "constructor dist map");
  ok &= expect_int(state.transition, 0, "constructor transition");
  ok &= expect_bool(state.cycle_transition, true,
                    "constructor cycle transition");
  ok &= expect_bool(state.metronome, false, "constructor metronome");
  ok &= expect_bool(state.zero_travel, false, "constructor zero travel");
  ok &= expect_bool(state.show_screen_size, false,
                    "constructor screen size");
  ok &= expect_bool(state.show_foot_extents, false,
                    "constructor foot extents");
  ok &= expect_bool(state.overlay_requested, true,
                    "constructor overlay request");

  auto destroy = source_character_test_destroy(true, true);
  ok &= expect_bool(destroy.looked_up_overlay, true, "destroy lookup");
  ok &= expect_bool(destroy.cleared_callback, true, "destroy clears callback");
  ok &= expect_bool(destroy.hid_overlay, true, "destroy hides overlay");
  ok &= expect_bool(destroy.restarted_timer, true, "destroy restarts timer");
  destroy = source_character_test_destroy(true, false);
  ok &= expect_bool(destroy.cleared_callback, false,
                    "destroy ignores other callback");

  auto draw = source_character_test_draw(true, false, true, true, true);
  ok &= expect_bool(draw.highlighted_driver, true, "draw highlights driver");
  ok &= expect_string(draw.draw_transform, "bone_head", "draw head transform");
  ok &= expect_bool(draw.drew_screen_size, true, "draw screen size");
  draw = source_character_test_draw(false, true, false, false, false);
  ok &= expect_bool(draw.highlighted_driver, false,
                    "draw needs driver to highlight");
  ok &= expect_string(draw.draw_transform, "self", "draw self fallback");

  SourceCharacterTestPollInput poll_input;
  auto poll = source_character_test_poll(poll_input);
  ok &= expect_bool(poll.entered_clip_branch, false,
                    "poll skips without driver/clip");

  poll_input.has_driver = true;
  poll_input.has_clip_dir = true;
  poll_input.has_clip1 = true;
  poll_input.metronome = true;
  poll_input.static_click_present = true;
  poll_input.beat = 8.0f;
  poll_input.delta_beat = 0.25f;
  poll_input.zero_travel = true;
  poll_input.has_bone_servo = true;
  poll = source_character_test_poll(poll_input);
  ok &= expect_bool(poll.entered_clip_branch, true, "poll enters clip branch");
  ok &= expect_bool(poll.loaded_click_cue, false, "poll keeps cached click");
  ok &= expect_bool(poll.restored_click_static, true, "poll restores click");
  ok &= expect_bool(poll.metronome_edge, true, "poll metronome edge");
  ok &= expect_bool(poll.would_play_click, true, "poll plays cached click");
  ok &= expect_bool(poll.play_new, true, "poll no first driver plays new");
  ok &= expect_bool(poll.reset_bone_servo_regulate, true,
                    "poll zero travel regulates servo");
  ok &= expect_bool(poll.recenter, true, "poll zero travel recenters");

  poll_input.has_first_driver = true;
  poll_input.first_clip_is_clip1 = true;
  poll_input.zero_travel = false;
  poll = source_character_test_poll(poll_input);
  ok &= expect_bool(poll.play_new, false,
                    "poll keeps existing clip1 without clip2");

  poll_input.has_clip2 = true;
  poll_input.first_clip_is_clip1 = false;
  poll_input.first_clip_is_clip2 = true;
  poll_input.transition_beat = 2.0f;
  poll_input.first_driver_beat = 3.0f;
  poll = source_character_test_poll(poll_input);
  ok &= expect_bool(poll.play_new, true,
                    "poll clip2 after transition plays new");

  SourceCharacterTestExisting existing;
  SourceCharacterTestBones bones;
  bones.bone_l_hand = true;
  bones.bone_l_fore_twist2 = true;
  bones.bone_r_hand = true;
  bones.bone_r_fore_twist2 = true;
  bones.bone_l_upper_twist1 = true;
  bones.bone_l_upper_twist2 = true;
  bones.bone_l_upper_arm = true;
  bones.bone_r_upper_twist1 = true;
  bones.bone_r_upper_twist2 = true;
  bones.bone_r_upper_arm = true;
  auto add = source_character_test_add_defaults(existing, bones);
  ok &= expect_bool(add.created_main_driver, true,
                    "defaults creates main driver");
  ok &= expect_bool(add.created_bone_servo, true,
                    "defaults creates bone servo");
  ok &= expect_bool(add.set_driver_bones_to_bone_servo, true,
                    "defaults sets driver bones");
  ok &= expect_size(add.controllers.size(), 4, "defaults controller count");
  ok &= expect_string(add.controllers[0].name, "foreTwist_L.ik",
                      "defaults left foretwist name");
  ok &= expect_string(add.controllers[0].hand, "bone_L-hand",
                      "defaults left foretwist hand");
  ok &= expect_string(add.controllers[0].twist2, "bone_L-foreTwist2",
                      "defaults left foretwist twist2");
  ok &= expect_bool(add.controllers[0].has_offset, true,
                    "defaults left foretwist offset flag");
  ok &= near(add.controllers[0].offset, 90.0f,
             "defaults left foretwist offset");
  ok &= expect_string(add.controllers[1].name, "foreTwist_R.ik",
                      "defaults right foretwist name");
  ok &= near(add.controllers[1].offset, -90.0f,
             "defaults right foretwist offset");
  ok &= expect_string(add.controllers[2].upper_arm, "bone_L-upperArm",
                      "defaults left upper arm");
  ok &= expect_string(add.controllers[3].twist1, "bone_R-upperTwist1",
                      "defaults right upper twist1");

  existing.has_main_driver = true;
  existing.has_bone_servo = true;
  existing.has_fore_twist_l = true;
  existing.has_fore_twist_r = true;
  existing.has_upper_twist_l = true;
  existing.has_upper_twist_r = true;
  add = source_character_test_add_defaults(existing, bones);
  ok &= expect_bool(add.created_main_driver, false,
                    "defaults preserves main driver");
  ok &= expect_bool(add.set_driver_bones_to_bone_servo, false,
                    "defaults skips existing bone servo");
  ok &= expect_size(add.controllers.size(), 0,
                    "defaults skips existing controllers");

  auto walked = source_character_test_walk({"a.wp", "b.wp"});
  ok &= expect_size(walked.size(), 2, "walk copies waypoints");
  ok &= expect_string(walked[1], "b.wp", "walk waypoint order");
  ok &= expect_string(source_character_test_teleport_to("stage.wp"),
                      "stage.wp", "teleport waypoint");
  ok &= expect_string(source_character_test_teleport_to(""), "",
                      "teleport missing waypoint");

  auto start_end = source_character_test_set_start_end_beat(
      true, true, true, 4.0f, 8.0f, 120);
  ok &= expect_bool(start_end.unfroze_character, true,
                    "start end unfreezes");
  ok &= expect_bool(start_end.set_bpm, true, "start end sets bpm");
  ok &= expect_bool(start_end.sent_set_anim_frame, true,
                    "start end sends frame message");
  ok &= near(start_end.start_frame, 60.0f, "start frame");
  ok &= near(start_end.end_frame, 120.0f, "end frame");
  start_end = source_character_test_set_start_end_beat(
      true, true, false, 4.0f, 8.0f, 120);
  ok &= expect_bool(start_end.sent_set_anim_frame, false,
                    "start end skips other anim");

  ok &= expect_bool(source_character_test_set_move_self(false), false,
                    "move self skips without servo");
  ok &= expect_bool(source_character_test_set_move_self(true), true,
                    "move self calls servo");

  auto load = source_character_test_load(0xD, 0);
  ok &= expect_bool(load.loaded_driver, false, "load rev d skips driver");
  load = source_character_test_load(0xE, 0);
  ok &= expect_bool(load.loaded_driver, true, "load other rev loads driver");
  load = source_character_test_load(0x10, 1);
  ok &= expect_bool(load.fail_new_revision, true, "load new rev fails");
  ok &= expect_bool(load.fail_new_alt_revision, true, "load alt rev fails");

  return ok ? 0 : 1;
}
