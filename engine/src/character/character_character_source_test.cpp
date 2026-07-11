#include "character/char_mesh.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool has(const std::vector<std::string>& values, const std::string& value) {
  return std::find(values.begin(), values.end(), value) != values.end();
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

bool expect_size(size_t got, size_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_string(const std::string& got, const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::cerr << label << " got '" << got << "' want '" << want << "'\n";
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
  using ghogx::character::source_character_add_shadow_bone;
  using ghogx::character::source_character_bone_servo_resolves;
  using ghogx::character::source_character_clear_interest_filter_flags;
  using ghogx::character::source_character_copy_bounding_sphere;
  using ghogx::character::source_character_default_state;
  using ghogx::character::source_character_enable_blinks;
  using ghogx::character::source_character_enter;
  using ghogx::character::source_character_exit;
  using ghogx::character::source_character_lod_assign;
  using ghogx::character::source_character_lod_copy_plan;
  using ghogx::character::source_character_lod_copy_state;
  using ghogx::character::source_character_lod_default_state;
  using ghogx::character::source_character_lod_prop_sync_plan;
  using ghogx::character::source_character_load_plan;
  using ghogx::character::source_char_lifecycle_plan;
  using ghogx::character::source_character_force_blink;
  using ghogx::character::source_character_poll;
  using ghogx::character::source_character_pre_save;
  using ghogx::character::source_character_removing_object;
  using ghogx::character::source_character_repoint_sphere_base;
  using ghogx::character::source_character_replace;
  using ghogx::character::source_character_set_sphere_base;
  using ghogx::character::source_character_set_focus_interest;
  using ghogx::character::source_character_set_interest_filter_flags;
  using ghogx::character::source_character_set_interest_objects;
  using ghogx::character::source_character_sync_shadow;
  using ghogx::character::source_character_sync_objects;
  using ghogx::character::source_character_unhook_shadow;

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
  ok &= expect_bool(state.sphere_base_is_null, false,
                    "constructor sphere base is not null");
  ok &= expect_bool(state.has_driver, false, "constructor no driver");

  auto lod = source_character_lod_default_state();
  ok &= expect_int(static_cast<int>(lod.screen_size), 0,
                   "LOD constructor screen size");
  ok &= expect_bool(lod.group.empty(), true, "LOD constructor group null");
  ok &= expect_bool(lod.trans_group.empty(), true,
                    "LOD constructor trans group null");

  lod.screen_size = 17.0f;
  lod.group = "lod0.grp";
  lod.trans_group = "lod0_trans.grp";
  auto lod_copy = source_character_lod_copy_state(lod);
  ok &= expect_int(static_cast<int>(lod_copy.screen_size), 17,
                   "LOD copy screen size");
  ok &= expect_string(lod_copy.group, "lod0.grp", "LOD copy group");
  ok &= expect_string(lod_copy.trans_group, "lod0_trans.grp",
                      "LOD copy trans group");

  auto lod_dest = source_character_lod_default_state();
  source_character_lod_assign(lod_dest, lod);
  ok &= expect_string(lod_dest.group, lod.group, "LOD assign group");
  ok &= expect_string(lod_dest.trans_group, lod.trans_group,
                      "LOD assign trans group");

  auto lod_copy_plan = source_character_lod_copy_plan();
  ok &= expect_size(lod_copy_plan.copied_members.size(), 3,
                    "LOD copy member count");
  ok &= expect_string(lod_copy_plan.copied_members[0], "mScreenSize",
                      "LOD copy screen size member");
  ok &= expect_string(lod_copy_plan.copied_members[1], "mGroup",
                      "LOD copy group member");
  ok &= expect_string(lod_copy_plan.copied_members[2], "mTransGroup",
                      "LOD copy trans group member");
  ok &= expect_bool(lod_copy_plan.assignment_returns_self, true,
                    "LOD assignment returns self");

  auto lod_props = source_character_lod_prop_sync_plan();
  ok &= expect_size(lod_props.properties.size(), 3,
                    "LOD prop-sync property count");
  ok &= expect_string(lod_props.properties[0], "screen_size",
                      "LOD prop-sync screen size");
  ok &= expect_string(lod_props.properties[1], "group",
                      "LOD prop-sync group");
  ok &= expect_string(lod_props.properties[2], "trans_group",
                      "LOD prop-sync trans group");

  auto bad_load = source_character_load_plan(0x12, false, 0);
  ok &= expect_bool(bad_load.known_revision, false,
                    "Character load rejects new revision");
  ok &= expect_size(bad_load.preload_steps.size(), 0,
                    "invalid Character load has no preload steps");

  auto load_v17 = source_character_load_plan(0x11, false, 0);
  ok &= expect_bool(load_v17.known_revision, true,
                    "Character load accepts rev17");
  ok &= expect_string(load_v17.preload_steps[0], "LOAD_REVS",
                      "Character preload first step");
  ok &= expect_bool(has(load_v17.preload_steps, "RndDir::PreLoad"), true,
                    "Character rev17 PreLoad delegates to RndDir");
  ok &= expect_bool(has(load_v17.preload_steps, "mRate=k1_fpb"), false,
                    "Character rev17 keeps rate");
  ok &= expect_bool(has(load_v17.postload_steps, "RndDir::PostLoad"), true,
                    "Character rev17 PostLoad delegates to RndDir");
  ok &= expect_bool(has(load_v17.postload_reads, "mLods"), true,
                    "Character rev17 reads lods");
  ok &= expect_bool(has(load_v17.postload_reads, "mTransGroup"), true,
                    "Character rev17 reads trans group");
  ok &= expect_bool(has(load_v17.postload_reads, "mTest"), true,
                    "Character rev17 reads test");
  ok &= expect_bool(
      has(load_v17.branches, "scaleLodScreenSizeBySphereRadius"), false,
      "Character rev17 does not scale lod screen size");

  auto load_v17_proxy = source_character_load_plan(0x11, true, 0);
  ok &= expect_bool(has(load_v17_proxy.postload_reads, "mLods"), false,
                    "Character proxy rev17 skips lod rows");
  ok &= expect_bool(has(load_v17_proxy.postload_reads, "mTest"), true,
                    "Character proxy rev17 reads test only");
  ok &= expect_bool(has(load_v17_proxy.branches, "proxyTestOnly"), true,
                    "Character proxy rev17 branch recorded");

  auto load_v6 = source_character_load_plan(6, false, 0);
  ok &= expect_bool(has(load_v6.preload_steps, "mRate=k1_fpb"), true,
                    "Character rev6 sets legacy rate");
  ok &= expect_bool(has(load_v6.postload_reads, "legacyNestedLods"), true,
                    "Character rev6 reads nested lods");
  ok &= expect_bool(has(load_v6.branches, "mBounding.Zero"), true,
                    "Character rev6 zeroes bounding");
  ok &= expect_bool(
      has(load_v6.branches, "legacyBoundingFromSphereWhenSelf"), true,
      "Character rev6 legacy bounding branch");
  ok &= expect_bool(
      has(load_v6.branches, "scaleLodScreenSizeBySphereRadius"), true,
      "Character rev6 scales lod screen size");

  auto load_v1_legacy5 = source_character_load_plan(1, false, 5);
  ok &= expect_bool(has(load_v1_legacy5.preload_steps, "somerev"), true,
                    "Character legacy PreLoad reads somerev");
  ok &= expect_bool(
      has(load_v1_legacy5.preload_steps, "RndTransformable::Load"), true,
      "Character legacy PreLoad reads transformable");
  ok &= expect_bool(has(load_v1_legacy5.postload_steps, "ObjectDir::PostLoad"),
                    true, "Character legacy PostLoad delegates ObjectDir");
  ok &= expect_bool(has(load_v1_legacy5.postload_reads, "mEnv"), true,
                    "Character legacy rev5 reads env");
  ok &= expect_bool(has(load_v1_legacy5.postload_reads, "legacyNestedLods"),
                    true, "Character legacy rev5 reads lods");
  ok &= expect_bool(has(load_v1_legacy5.branches, "legacyRenameLods"), true,
                    "Character legacy rev5 renames lod groups");
  ok &= expect_bool(has(load_v1_legacy5.postload_reads, "mShadow"), false,
                    "Character legacy rev5 does not read shadow");

  auto load_v1_legacy7 = source_character_load_plan(1, false, 7);
  ok &= expect_bool(has(load_v1_legacy7.branches, "legacyRenameLods"), false,
                    "Character legacy rev7 keeps lod names");
  ok &= expect_bool(has(load_v1_legacy7.postload_reads, "mShadow"), true,
                    "Character legacy rev7 reads shadow");

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

  auto sphere = source_character_set_sphere_base(state, false);
  ok &= expect_bool(sphere.defaulted_to_self, true,
                    "SetSphereBase null defaults to self");
  ok &= expect_bool(sphere.made_world_sphere, true,
                    "SetSphereBase makes world sphere");
  ok &= expect_bool(sphere.multiplied_by_trans_world, true,
                    "SetSphereBase multiplies by transform world");
  ok &= expect_bool(sphere.set_sphere, true, "SetSphereBase sets sphere");
  ok &= expect_bool(state.sphere_base_is_self, true,
                    "SetSphereBase stores self fallback");
  ok &= expect_bool(state.sphere_base_is_null, false,
                    "SetSphereBase keeps non-null base");

  sphere = source_character_set_sphere_base(state, true);
  ok &= expect_bool(sphere.defaulted_to_self, false,
                    "SetSphereBase accepts transform");
  ok &= expect_bool(state.sphere_base_is_self, false,
                    "SetSphereBase stores transform");

  auto interests =
      source_character_set_interest_objects(false, {true, false}, true);
  ok &= expect_bool(interests.found_eyes, false,
                    "SetInterestObjects skips without eyes");
  ok &= expect_int(interests.validated_count, 0,
                   "SetInterestObjects does not validate without eyes");

  interests = source_character_set_interest_objects(true, {true, false, true},
                                                    false);
  ok &= expect_bool(interests.cleared_all, true,
                    "SetInterestObjects clears existing eyes interests");
  ok &= expect_int(interests.validated_count, 3,
                   "SetInterestObjects validates each row");
  ok &= expect_int(interests.add_count, 2,
                   "SetInterestObjects adds validated rows");
  ok &= expect_int(interests.used_interest_dir_count, 3,
                   "SetInterestObjects uses interest dir without override");

  interests = source_character_set_interest_objects(true, {true}, true);
  ok &= expect_int(interests.used_override_dir_count, 1,
                   "SetInterestObjects uses override dir");

  auto shadow = source_character_add_shadow_bone(2, false, false);
  ok &= expect_bool(shadow.returned_null, true,
                    "AddShadowBone returns null without transform");
  ok &= expect_int(shadow.final_shadow_bones, 2,
                   "AddShadowBone null keeps count");

  shadow = source_character_add_shadow_bone(2, true, true);
  ok &= expect_bool(shadow.returned_existing, true,
                    "AddShadowBone returns existing parent");
  ok &= expect_int(shadow.final_shadow_bones, 2,
                   "AddShadowBone existing keeps count");

  shadow = source_character_add_shadow_bone(2, true, false);
  ok &= expect_bool(shadow.created, true, "AddShadowBone creates new row");
  ok &= expect_int(shadow.final_shadow_bones, 3,
                   "AddShadowBone increments count");

  auto unhook = source_character_unhook_shadow(4);
  ok &= expect_bool(unhook.deleted_all, true, "UnhookShadow deletes all");
  ok &= expect_int(unhook.deleted_shadow_bones, 4,
                   "UnhookShadow reports deleted count");

  auto sync_shadow = source_character_sync_shadow(false, true, {2});
  ok &= expect_bool(sync_shadow.unhooked_shadow, true,
                    "SyncShadow always unhooks first");
  ok &= expect_bool(sync_shadow.removed_shadow_draw, false,
                    "SyncShadow skips draw remove without shadow");

  sync_shadow = source_character_sync_shadow(true, false, {2, 0});
  ok &= expect_int(sync_shadow.hooked_bone_count, 0,
                   "SyncShadow skips hookups outside old gfx");
  ok &= expect_bool(sync_shadow.removed_shadow_draw, true,
                    "SyncShadow removes shadow drawable");

  sync_shadow = source_character_sync_shadow(true, true, {2, 0, 3});
  ok &= expect_int(sync_shadow.hooked_bone_count, 5,
                   "SyncShadow hooks mesh bones");
  ok &= expect_int(sync_shadow.hooked_mesh_parent_count, 1,
                   "SyncShadow hooks mesh parent without bones");

  auto copied = source_character_copy_bounding_sphere(state, true);
  ok &= expect_bool(copied.set_sphere, true,
                    "CopyBoundingSphere copies sphere");
  ok &= expect_bool(copied.copied_bounding, true,
                    "CopyBoundingSphere copies bounding");
  ok &= expect_bool(copied.copied_sphere_base, true,
                    "CopyBoundingSphere copies source sphere base");
  ok &= expect_bool(state.sphere_base_is_null, false,
                    "CopyBoundingSphere keeps non-null base");

  copied = source_character_copy_bounding_sphere(state, false);
  ok &= expect_bool(copied.cleared_sphere_base, true,
                    "CopyBoundingSphere clears missing source base");
  ok &= expect_bool(state.sphere_base_is_null, true,
                    "CopyBoundingSphere stores null base");

  auto repoint = source_character_repoint_sphere_base(state, true);
  ok &= expect_bool(repoint.had_sphere_base, false,
                    "RepointSphereBase skips null base");
  ok &= expect_bool(repoint.looked_up_by_name, false,
                    "RepointSphereBase does not lookup null base");

  state.sphere_base_is_null = false;
  repoint = source_character_repoint_sphere_base(state, false);
  ok &= expect_bool(repoint.looked_up_by_name, true,
                    "RepointSphereBase looks up existing base");
  ok &= expect_bool(repoint.repointed, false,
                    "RepointSphereBase keeps old base when missing");

  repoint = source_character_repoint_sphere_base(state, true);
  ok &= expect_bool(repoint.repointed, true,
                    "RepointSphereBase stores matching transform");
  ok &= expect_bool(state.sphere_base_is_self, false,
                    "RepointSphereBase target is transform");

  ok &= expect_bool(source_character_pre_save().unhooked_shadow, true,
                    "PreSave unhooks shadow");

  const auto lifecycle = source_char_lifecycle_plan();
  ok &= expect_size(lifecycle.init_steps.size(), 7, "CharInit step count");
  ok &= expect_string(lifecycle.init_steps[0], "Character::Init",
                      "CharInit first step");
  ok &= expect_string(lifecycle.init_steps[1], "CharBonesObject::Init",
                      "CharInit second step");
  ok &= expect_string(lifecycle.init_steps[2], "CharBoneOffset::Init",
                      "CharInit third step");
  ok &= expect_string(lifecycle.init_steps[3], "PreloadSharedSubdirs(char)",
                      "CharInit preload step");
  ok &= expect_string(lifecycle.init_steps[4], "CharBoneDir::Init",
                      "CharInit bone dir step");
  ok &= expect_string(lifecycle.init_steps[5], "CharUtlInit",
                      "CharInit utility step");
  ok &= expect_string(lifecycle.init_steps[6],
                      "AddExitCallback(CharTerminate)",
                      "CharInit callback step");
  ok &= expect_size(lifecycle.terminate_steps.size(), 3,
                    "CharTerminate step count");
  ok &= expect_string(lifecycle.terminate_steps[0],
                      "RemoveExitCallback(CharTerminate)",
                      "CharTerminate callback step");
  ok &= expect_string(lifecycle.terminate_steps[1], "Character::Terminate",
                      "CharTerminate character step");
  ok &= expect_string(lifecycle.terminate_steps[2], "CharBoneDir::Terminate",
                      "CharTerminate bone dir step");

  return ok ? 0 : 1;
}
