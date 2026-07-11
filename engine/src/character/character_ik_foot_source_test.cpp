#include "character/char_clip.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace {

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::printf("FAIL %s got=%.8f want=%.8f\n", label, got, want);
  return false;
}

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::printf("FAIL %s got=%d want=%d\n", label, got ? 1 : 0,
              want ? 1 : 0);
  return false;
}

bool expect_int(int got, int want, const char* label) {
  if (got == want) return true;
  std::printf("FAIL %s got=%d want=%d\n", label, got, want);
  return false;
}

bool expect_string(const std::string& got, const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::printf("FAIL %s got=%s want=%s\n", label, got.c_str(), want.c_str());
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharIKFootState;
  using ghogx::character::source_char_ik_foot_copy;
  using ghogx::character::source_char_ik_foot_default_state;
  using ghogx::character::source_char_ik_foot_do_fsm;
  using ghogx::character::source_char_ik_foot_enter;
  using ghogx::character::source_char_ik_foot_load_steps;
  using ghogx::character::source_char_ik_foot_poll_deps_plan;
  using ghogx::character::source_char_ik_foot_poll_plan;
  using ghogx::character::source_char_ik_foot_set_name;

  bool ok = true;

  SourceCharIKFootState foot = source_char_ik_foot_default_state();
  ok &= expect_bool(foot.helper_target_created, true,
                    "constructor creates helper target");
  ok &= expect_bool(foot.helper_target_local_reset, true,
                    "constructor resets helper target local xfm");
  ok &= expect_int(foot.fsm_state, 0, "constructor fsm state");
  ok &= expect_string(foot.data, "", "constructor data ref");
  ok &= expect_int(foot.data_index, 0, "constructor data index");

  foot.fsm_state = 2;
  foot.release_distance = 3.0f;
  auto enter = source_char_ik_foot_enter(foot);
  ok &= expect_bool(enter.reset_fsm_state, true, "Enter resets FSM flag");
  ok &= expect_bool(enter.reset_release_distance, true,
                    "Enter resets release distance flag");
  ok &= expect_int(foot.fsm_state, 0, "Enter state value");
  ok &= near(foot.release_distance, 0.0f, "Enter release distance value");

  auto set_name = source_char_ik_foot_set_name(foot, "rock2", true);
  ok &= expect_bool(set_name.call_hmx_set_name, true,
                    "SetName delegates Hmx object");
  ok &= expect_bool(set_name.assigned_character, true,
                    "SetName assigns character");
  ok &= expect_string(foot.character_dir, "rock2", "SetName character dir");
  set_name = source_char_ik_foot_set_name(foot, "world", false);
  ok &= expect_bool(set_name.assigned_character, false,
                    "SetName rejects non-character dir");
  ok &= expect_string(foot.character_dir, "", "SetName clears non-character");

  auto plan = source_char_ik_foot_poll_plan(true, true, false);
  ok &= expect_bool(plan.should_poll, false, "Poll returns without data");
  plan = source_char_ik_foot_poll_plan(true, true, true);
  ok &= expect_bool(plan.clear_targets_before, true,
                    "Poll clears targets before helper target");
  ok &= expect_bool(plan.push_helper_target, true, "Poll pushes helper target");
  ok &= expect_bool(plan.run_do_fsm, true, "Poll runs DoFSM");
  ok &= expect_bool(plan.call_char_ik_hand_poll, true,
                    "Poll delegates CharIKHand Poll");
  ok &= expect_bool(plan.clear_targets_after, true,
                    "Poll clears targets after hand poll");
  ok &= expect_bool(source_char_ik_foot_poll_deps_plan()
                        .call_char_ik_hand_poll_deps,
                    true, "PollDeps delegates CharIKHand PollDeps");

  foot = source_char_ik_foot_default_state();
  source_char_ik_foot_enter(foot);
  auto fsm = source_char_ik_foot_do_fsm(
      foot, {0.0f, 0.0f, 0.0f}, {1.0f, 2.0f, 0.25f}, 0.0f, 0.016f, false);
  ok &= expect_bool(fsm.copied_finger_matrix, true,
                    "DoFSM copies finger matrix row");
  ok &= expect_bool(fsm.planted, true, "DoFSM data below one plants foot");
  ok &= expect_bool(fsm.returned_from_planted_state, true,
                    "DoFSM planted state returns early");
  ok &= expect_int(fsm.fsm_state, 1, "DoFSM enters planted state");
  ok &= near(fsm.target_pos[0], 1.0f, "DoFSM planted target x");
  ok &= near(fsm.target_pos[1], 2.0f, "DoFSM planted target y");
  ok &= near(fsm.target_pos[2], 0.25f, "DoFSM planted target z");

  fsm = source_char_ik_foot_do_fsm(
      foot, fsm.target_pos, {2.0f, 2.0f, 0.25f}, 0.0f, 0.016f, false);
  ok &= expect_int(fsm.fsm_state, 1, "DoFSM remains planted");
  ok &= near(fsm.target_pos[0], 1.125f, "DoFSM clamps planted travel");

  fsm = source_char_ik_foot_do_fsm(
      foot, {1.125f, 2.0f, 1.0f}, {2.0f, 2.0f, 1.0f}, 2.0f, 0.01f, false);
  ok &= expect_bool(fsm.planted, false, "DoFSM data threshold releases foot");
  ok &= expect_int(fsm.fsm_state, 2, "DoFSM enters release state");
  ok &= near(fsm.release_distance, 0.625f, "DoFSM release distance decays");
  ok &= near(fsm.target_pos[0], 1.375f, "DoFSM release target advances");

  foot.fsm_state = 2;
  foot.release_distance = 0.5f;
  fsm = source_char_ik_foot_do_fsm(
      foot, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, 2.0f, -0.5f, false);
  ok &= expect_bool(fsm.clamped_negative_delta, true,
                    "DoFSM clamps negative delta seconds");
  ok &= near(fsm.release_distance, 0.5f,
             "DoFSM clamped delta preserves release distance");
  ok &= near(fsm.target_pos[0], 0.5f, "DoFSM release target uses distance");

  foot.fsm_state = 2;
  foot.release_distance = 0.25f;
  fsm = source_char_ik_foot_do_fsm(
      foot, {0.0f, 0.0f, 1.0f}, {4.0f, 0.0f, 1.0f}, 2.0f, 0.01f, true);
  ok &= expect_int(fsm.fsm_state, 0, "DoFSM teleport resets state");
  ok &= near(fsm.target_pos[0], 4.0f, "DoFSM teleport uses finger target");

  auto load = source_char_ik_foot_load_steps(4);
  ok &= expect_bool(load.known_revision, true, "Load rev4 known");
  ok &= expect_bool(load.load_char_ik_hand, true, "Load CharIKHand superclass");
  ok &= expect_bool(load.read_legacy_symbol, true, "Load rev4 legacy symbol");
  ok &= expect_int(load.legacy_int_reads, 3, "Load rev4 legacy int count");
  ok &= expect_bool(load.load_data, false, "Load rev4 skips data ref");
  load = source_char_ik_foot_load_steps(5);
  ok &= expect_bool(load.read_legacy_symbol, true, "Load rev5 legacy symbol");
  ok &= expect_bool(load.load_data, true, "Load rev5 data ref");
  ok &= expect_bool(load.load_data_index, true, "Load rev5 data index");
  load = source_char_ik_foot_load_steps(6);
  ok &= expect_bool(load.read_legacy_symbol, false, "Load rev6 no symbol");
  ok &= expect_bool(load.load_data, true, "Load rev6 data ref");
  load = source_char_ik_foot_load_steps(7);
  ok &= expect_bool(load.known_revision, false, "Load rev7 rejected");

  SourceCharIKFootState source = source_char_ik_foot_default_state();
  source.data = "foot_data";
  source.data_index = 2;
  SourceCharIKFootState dest = source_char_ik_foot_default_state();
  auto copy = source_char_ik_foot_copy(dest, source);
  ok &= expect_bool(copy.copy_char_ik_hand, true,
                    "Copy delegates CharIKHand superclass");
  ok &= expect_bool(copy.copy_data, true, "Copy data ref");
  ok &= expect_bool(copy.copy_data_index, true, "Copy data index");
  ok &= expect_string(dest.data, "foot_data", "Copy data value");
  ok &= expect_int(dest.data_index, 2, "Copy data index value");

  std::printf("character_ik_foot_source_test %s\n", ok ? "OK" : "FAIL");
  return ok ? 0 : 1;
}
