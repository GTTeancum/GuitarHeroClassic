#include "character/char_mesh.h"

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

bool expect_poll_state(ghogx::character::SourceCharacterPollState got,
                       ghogx::character::SourceCharacterPollState want,
                       const char* label) {
  return expect_int(static_cast<int>(got), static_cast<int>(want), label);
}

bool expect_draw_mode(ghogx::character::SourceCharacterDrawMode got,
                      ghogx::character::SourceCharacterDrawMode want,
                      const char* label) {
  return expect_int(static_cast<int>(got), static_cast<int>(want), label);
}

}  // namespace

int main() {
  using ghogx::character::SourceCharacterDrawMode;
  using ghogx::character::SourceCharacterPollState;
  using ghogx::character::source_character_added_object;
  using ghogx::character::source_character_bone_servo_resolves;
  using ghogx::character::source_character_clear_interest_filter_flags;
  using ghogx::character::source_character_default_state;
  using ghogx::character::source_character_enable_blinks;
  using ghogx::character::source_character_enter;
  using ghogx::character::source_character_exit;
  using ghogx::character::source_character_force_blink;
  using ghogx::character::source_character_poll;
  using ghogx::character::source_character_removing_object;
  using ghogx::character::source_character_replace;
  using ghogx::character::source_character_set_focus_interest;
  using ghogx::character::source_character_set_interest_filter_flags;
  using ghogx::character::source_character_sync_objects;

  bool ok = true;

  auto state = source_character_default_state();
  ok &= expect_int(state.min_lod, 0, "constructor min LOD");
  ok &= expect_int(state.last_lod, 0, "constructor last LOD");
  ok &= expect_poll_state(state.poll_state, SourceCharacterPollState::kCreated,
                          "constructor poll state");
  ok &= expect_draw_mode(state.draw_mode, SourceCharacterDrawMode::kAll,
                         "constructor draw mode");
  ok &= expect_bool(state.frozen, false, "constructor frozen");
  ok &= expect_bool(state.teleported, true, "constructor teleported");
  ok &= expect_bool(state.sphere_base_is_self, true,
                    "constructor sphere base is self");
  ok &= expect_bool(state.has_driver, false, "constructor no driver");

  state.min_lod = 4;
  state.last_lod = 9;
  state.frozen = true;
  state.teleported = false;
  state.interest_to_force = "old_interest";
  source_character_enter(state);
  ok &= expect_poll_state(state.poll_state, SourceCharacterPollState::kEntered,
                          "Enter poll state");
  ok &= expect_int(state.min_lod, -1, "Enter min LOD");
  ok &= expect_int(state.last_lod, 0, "Enter last LOD");
  ok &= expect_bool(state.frozen, false, "Enter unfreezes");
  ok &= expect_bool(state.teleported, true, "Enter teleported");
  ok &= expect_bool(state.interest_to_force.empty(), true,
                    "Enter clears forced interest");

  auto poll = source_character_poll(state);
  ok &= expect_bool(poll.called_rnd_dir_poll, true, "Poll calls RndDir");
  ok &= expect_bool(poll.skipped_for_frozen, false, "Poll not skipped");
  ok &= expect_bool(state.teleported, false, "Poll clears teleported");
  ok &= expect_poll_state(state.poll_state, SourceCharacterPollState::kPolled,
                          "Poll state");

  state.frozen = true;
  poll = source_character_poll(state);
  ok &= expect_bool(poll.called_rnd_dir_poll, false,
                    "frozen Poll skips RndDir");
  ok &= expect_bool(poll.skipped_for_frozen, true, "frozen Poll reports skip");
  ok &= expect_poll_state(state.poll_state, SourceCharacterPollState::kPolled,
                          "frozen Poll keeps prior state");

  source_character_exit(state);
  ok &= expect_poll_state(state.poll_state, SourceCharacterPollState::kExited,
                          "Exit poll state");

  ok &= expect_bool(source_character_bone_servo_resolves(false, true), false,
                    "BoneServo no driver");
  ok &= expect_bool(source_character_bone_servo_resolves(true, false), false,
                    "BoneServo wrong bones type");
  ok &= expect_bool(source_character_bone_servo_resolves(true, true), true,
                    "BoneServo returns servo bones");

  auto added = source_character_added_object(state, true, true, "not_main.drv");
  ok &= expect_bool(added.accepted_pollable, true,
                    "AddedObject accepts pollable");
  ok &= expect_bool(added.assigned_main_driver, false,
                    "AddedObject ignores other drivers");
  ok &= expect_bool(state.has_driver, false,
                    "AddedObject leaves driver unset");

  added = source_character_added_object(state, true, true, "main.drv");
  ok &= expect_bool(added.assigned_main_driver, true,
                    "AddedObject assigns main driver");
  ok &= expect_bool(state.has_driver, true,
                    "AddedObject stores driver pointer");

  auto removed = source_character_removing_object(state, false);
  ok &= expect_bool(removed.cleared_driver, false,
                    "RemovingObject keeps unrelated driver");
  ok &= expect_bool(removed.called_rnd_dir_removing_object, true,
                    "RemovingObject delegates to RndDir");
  ok &= expect_bool(state.has_driver, true, "driver remains after unrelated");

  removed = source_character_removing_object(state, true);
  ok &= expect_bool(removed.cleared_driver, true,
                    "RemovingObject clears current driver");
  ok &= expect_bool(state.has_driver, false, "driver cleared");

  state.sphere_base_is_self = false;
  auto replace = source_character_replace(state, false, false);
  ok &= expect_bool(replace.called_rnd_dir_replace, true,
                    "Replace delegates to RndDir");
  ok &= expect_bool(replace.repointed_sphere_base, false,
                    "Replace ignores unrelated object");
  ok &= expect_bool(state.sphere_base_is_self, false,
                    "Replace leaves unrelated sphere base");

  replace = source_character_replace(state, true, true);
  ok &= expect_bool(replace.repointed_sphere_base, true,
                    "Replace repoints sphere base");
  ok &= expect_bool(replace.fell_back_to_self, false,
                    "Replace accepts transform target");
  ok &= expect_bool(state.sphere_base_is_self, false,
                    "Replace stores transform target");

  replace = source_character_replace(state, true, false);
  ok &= expect_bool(replace.fell_back_to_self, true,
                    "Replace falls back to self on non-transform");
  ok &= expect_bool(state.sphere_base_is_self, true,
                    "Replace self fallback stored");

  auto sync = source_character_sync_objects(state, true, 3);
  ok &= expect_poll_state(state.poll_state, SourceCharacterPollState::kSyncObject,
                          "SyncObjects poll state");
  ok &= expect_bool(sync.converted_bones_to_transes, true,
                    "SyncObjects converts pelvis mesh");
  ok &= expect_bool(sync.called_rnd_dir_sync_objects, true,
                    "SyncObjects delegates to RndDir");
  ok &= expect_bool(sync.removed_trans_group, true,
                    "SyncObjects removes trans group");
  ok &= expect_int(sync.removed_lod_draws, 6,
                   "SyncObjects removes LOD and trans LOD groups");
  ok &= expect_bool(sync.synced_shadow, true, "SyncObjects syncs shadow");
  ok &= expect_bool(sync.sorted_polls, true, "SyncObjects sorts polls");

  sync = source_character_sync_objects(state, false, -2);
  ok &= expect_bool(sync.converted_bones_to_transes, false,
                    "SyncObjects skips missing pelvis mesh");
  ok &= expect_int(sync.removed_lod_draws, 0,
                   "SyncObjects clamps impossible negative test count");

  ok &= expect_bool(source_character_force_blink(false).invoked_eyes, false,
                    "ForceBlink skips without eyes");
  ok &= expect_bool(source_character_force_blink(true).invoked_eyes, true,
                    "ForceBlink invokes eyes");
  ok &= expect_bool(source_character_enable_blinks(true).invoked_eyes, true,
                    "EnableBlinks invokes eyes");
  ok &= expect_bool(source_character_set_focus_interest(false).invoked_eyes,
                    false, "SetFocusInterest skips without eyes");
  ok &= expect_bool(
      source_character_set_interest_filter_flags(true).invoked_eyes, true,
      "SetInterestFilterFlags invokes eyes");
  ok &= expect_bool(
      source_character_clear_interest_filter_flags(true).invoked_eyes, true,
      "ClearInterestFilterFlags invokes eyes");

  return ok ? 0 : 1;
}
