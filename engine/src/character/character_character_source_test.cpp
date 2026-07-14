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

bool expect_float(float got, float want, const char* label) {
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
  using ghogx::character::SourceCharPollableSorterDep;
  using ghogx::character::source_object_dir_default_state;
  using ghogx::character::source_object_dir_find_object_plan;
  using ghogx::character::source_object_dir_postload_plan;
  using ghogx::character::source_object_dir_preload_plan;
  using ghogx::character::source_object_dir_save_plan;
  using ghogx::character::source_object_dir_subdir_plan;
  using ghogx::character::source_rnddir_copy_plan;
  using ghogx::character::source_rnddir_default_state;
  using ghogx::character::source_rnddir_handler_plan;
  using ghogx::character::source_rnddir_load_plan;
  using ghogx::character::source_rnddir_prop_sync_plan;
  using ghogx::character::source_rnddir_save_plan;
  using ghogx::character::source_rnddir_sync_drawables_plan;
  using ghogx::character::source_rnddir_sync_objects_plan;
  using ghogx::character::source_band_character_deformation_plan;
  using ghogx::character::source_character_added_object;
  using ghogx::character::source_character_add_shadow_bone;
  using ghogx::character::source_character_bone_servo_resolves;
  using ghogx::character::source_character_clear_interest_filter_flags;
  using ghogx::character::source_character_copy_bounding_sphere;
  using ghogx::character::source_character_on_copy_bounding_sphere;
  using ghogx::character::source_character_on_play_clip;
  using ghogx::character::source_character_copy_plan;
  using ghogx::character::source_character_default_state;
  using ghogx::character::source_character_enable_blinks;
  using ghogx::character::source_character_enter;
  using ghogx::character::source_character_exit;
  using ghogx::character::source_character_handler_plan;
  using ghogx::character::source_character_lod_assign;
  using ghogx::character::source_character_lod_copy_plan;
  using ghogx::character::source_character_lod_copy_state;
  using ghogx::character::source_character_lod_default_state;
  using ghogx::character::source_character_lod_prop_sync_plan;
  using ghogx::character::source_character_load_plan;
  using ghogx::character::source_char_lifecycle_plan;
  using ghogx::character::source_character_force_blink;
  using ghogx::character::source_character_on_get_current_interests;
  using ghogx::character::source_character_poll;
  using ghogx::character::source_character_pre_save;
  using ghogx::character::source_character_prop_sync_plan;
  using ghogx::character::source_character_removing_object;
  using ghogx::character::source_character_repoint_sphere_base;
  using ghogx::character::source_character_replace;
  using ghogx::character::source_character_save_plan;
  using ghogx::character::source_character_runtime_dump_evidence;
  using ghogx::character::source_character_set_debug_draw_interest_objects;
  using ghogx::character::source_character_set_sphere_base;
  using ghogx::character::source_character_set_focus_interest;
  using ghogx::character::source_character_set_interest_filter_flags;
  using ghogx::character::source_character_set_interest_objects;
  using ghogx::character::source_character_sync_shadow;
  using ghogx::character::source_character_sync_objects;
  using ghogx::character::source_character_unhook_shadow;
  using ghogx::character::source_char_pollable_sorter_changed_by;

  bool ok = true;

  const auto object_defaults = source_object_dir_default_state();
  ok &= expect_bool(object_defaults.proxy_override, false,
                    "ObjectDir default proxy override");
  ok &= expect_bool(object_defaults.inline_proxy, true,
                    "ObjectDir default inline proxy");
  ok &= expect_bool(object_defaults.loader_null, true,
                    "ObjectDir default loader null");
  ok &= expect_bool(object_defaults.is_subdir, false,
                    "ObjectDir default subdir flag");
  ok &= expect_int(object_defaults.inline_subdir_type, 0,
                   "ObjectDir default inline subdir type");
  ok &= expect_bool(object_defaults.path_name_null, true,
                    "ObjectDir default path name");
  ok &= expect_bool(object_defaults.current_camera_null, true,
                    "ObjectDir default current camera");
  ok &= expect_bool(object_defaults.always_inlined, false,
                    "ObjectDir default always inlined");
  ok &= expect_bool(object_defaults.always_inline_hash_null, true,
                    "ObjectDir default always inline hash");
  ok &= expect_int(source_object_dir_save_plan().save_id, 0x1A2,
                   "ObjectDir SAVE_OBJ id");

  const auto object_pre_v27 =
      source_object_dir_preload_plan(0x1b, false, false);
  ok &= expect_bool(object_pre_v27.known_revision, true,
                    "ObjectDir preload accepts rev27");
  ok &= expect_string(object_pre_v27.read_order[0], "LOAD_REVS",
                      "ObjectDir preload first step");
  ok &= expect_bool(has(object_pre_v27.read_order, "Hmx::Object::LoadType"),
                    true, "ObjectDir rev27 reads LoadType");
  ok &= expect_bool(has(object_pre_v27.read_order, "mAlwaysInlined"), true,
                    "ObjectDir rev27 reads always inlined");
  ok &= expect_bool(has(object_pre_v27.read_order, "mInlineProxy"), true,
                    "ObjectDir rev27 reads inline proxy");
  ok &= expect_bool(has(object_pre_v27.read_order, "mInlineSubDirType"), true,
                    "ObjectDir rev27 reads inline subdir type");
  ok &= expect_bool(object_pre_v27.pushes_revision, true,
                    "ObjectDir preload pushes revision");

  const auto object_pre_v13 =
      source_object_dir_preload_plan(0x0d, true, true);
  ok &= expect_bool(has(object_pre_v13.read_order, "proxyFilePath"), true,
                    "ObjectDir rev13 reads proxy path");
  ok &= expect_bool(has(object_pre_v13.read_order, "OldLoadProxies"), true,
                    "ObjectDir rev13 reads old proxies");
  ok &= expect_bool(has(object_pre_v13.branches, "proxyOverridePath"), true,
                    "ObjectDir proxy override branch");

  const auto object_post =
      source_object_dir_postload_plan(0x1b, 2, false, true, false, false,
                                      false, false);
  ok &= expect_bool(has(object_post.steps, "postloadInlinedDirsReverse"), true,
                    "ObjectDir postload inlined dirs");
  ok &= expect_bool(has(object_post.steps, "postloadOffsetSubDirs"), true,
                    "ObjectDir postload new subdir branch");
  ok &= expect_bool(has(object_post.steps, "LoadRest"), true,
                    "ObjectDir postload LoadRest branch");
  ok &= expect_bool(has(object_post.branches, "createDirLoaderForProxyFile"),
                    true, "ObjectDir proxy file reload branch");

  const auto object_find_entry = source_object_dir_find_object_plan(
      true, false, false, true, true, false, false);
  ok &= expect_string(object_find_entry.result, "entry",
                      "ObjectDir FindObject local hit");
  const auto object_find_parent = source_object_dir_find_object_plan(
      false, false, false, true, true, false, false);
  ok &= expect_string(object_find_parent.search_order.back(), "parentDir",
                      "ObjectDir FindObject parent search");
  ok &= expect_string(object_find_parent.result, "parentDir",
                      "ObjectDir FindObject parent result");
  const auto object_find_main = source_object_dir_find_object_plan(
      false, false, false, true, false, false, false);
  ok &= expect_string(object_find_main.result, "mainDir",
                      "ObjectDir FindObject main fallback");

  const auto object_add_subdir = source_object_dir_subdir_plan(true);
  ok &= expect_bool(object_add_subdir.clears_name_and_type, true,
                    "ObjectDir SetSubDir clears name/type");
  ok &= expect_bool(object_add_subdir.added_publishes_nested_objects, true,
                    "ObjectDir AddedSubDir publishes nested objects");
  const auto object_remove_subdir = source_object_dir_subdir_plan(false);
  ok &= expect_bool(object_remove_subdir.removing_sets_subdir_false, true,
                    "ObjectDir RemovingSubDir clears subdir flag");

  const auto rnd_defaults = source_rnddir_default_state();
  ok &= expect_bool(rnd_defaults.env_null, true, "RndDir default env null");
  ok &= expect_int(rnd_defaults.draw_count, 0, "RndDir default draw count");
  ok &= expect_int(rnd_defaults.anim_count, 0, "RndDir default anim count");
  ok &= expect_int(rnd_defaults.poll_count, 0, "RndDir default poll count");
  ok &= expect_int(source_rnddir_save_plan().save_id, 0x1C1,
                   "RndDir SAVE_OBJ id");

  const auto rnd_load_v10 = source_rnddir_load_plan(0x0a, false);
  ok &= expect_bool(rnd_load_v10.known_revision, true,
                    "RndDir load accepts rev10");
  ok &= expect_bool(has(rnd_load_v10.preload_steps, "ObjectDir::PreLoad"),
                    true, "RndDir PreLoad delegates ObjectDir");
  ok &= expect_bool(has(rnd_load_v10.postload_steps, "RndAnimatable::Load"),
                    true, "RndDir PostLoad loads animatable");
  ok &= expect_bool(has(rnd_load_v10.postload_steps, "RndDrawable::Load"),
                    true, "RndDir PostLoad loads drawable");
  ok &= expect_bool(has(rnd_load_v10.postload_steps, "RndTransformable::Load"),
                    true, "RndDir PostLoad loads transformable");
  ok &= expect_bool(has(rnd_load_v10.postload_reads, "mEnv"), true,
                    "RndDir rev10 reads env");
  ok &= expect_bool(has(rnd_load_v10.postload_reads, "mTestEvent"), true,
                    "RndDir rev10 reads test event");

  const auto rnd_load_v6 = source_rnddir_load_plan(6, true);
  ok &= expect_bool(has(rnd_load_v6.postload_reads, "mEnvProxyDummy"), true,
                    "RndDir proxy load reads env dummy");
  ok &= expect_bool(has(rnd_load_v6.postload_reads, "legacyRndPostProc"),
                    true, "RndDir legacy post proc");
  ok &= expect_bool(has(rnd_load_v6.branches, "loadAndDeleteRndPostProc"),
                    true, "RndDir legacy post proc branch");

  const auto sync_objects =
      source_rnddir_sync_objects_plan(false, true);
  ok &= expect_bool(sync_objects.calls_sync_drawables, true,
                    "RndDir SyncObjects calls SyncDrawables");
  ok &= expect_bool(sync_objects.collects_animatables, true,
                    "RndDir SyncObjects collects animatables");
  ok &= expect_bool(sync_objects.removes_anim_children, true,
                    "RndDir SyncObjects removes anim children");
  ok &= expect_bool(sync_objects.collects_pollables, true,
                    "RndDir SyncObjects collects pollables");
  ok &= expect_bool(sync_objects.sorts_polls, true,
                    "RndDir SyncObjects sorts polls");
  ok &= expect_bool(sync_objects.chains_source_subdir, true,
                    "RndDir SyncObjects chains source subdir");
  ok &= expect_bool(sync_objects.calls_object_dir_sync, true,
                    "RndDir SyncObjects delegates ObjectDir");
  ok &= expect_bool(source_rnddir_sync_objects_plan(true, true)
                        .calls_object_dir_sync,
                    false, "RndDir subdir SyncObjects is fenced");

  const auto sync_draws = source_rnddir_sync_drawables_plan(false);
  ok &= expect_bool(sync_draws.collects_drawables, true,
                    "RndDir SyncDrawables collects drawables");
  ok &= expect_bool(sync_draws.updates_preclear_state, true,
                    "RndDir SyncDrawables updates preclear state");
  ok &= expect_bool(sync_draws.removes_draw_children, true,
                    "RndDir SyncDrawables removes draw children");
  ok &= expect_bool(sync_draws.sorts_draws, true,
                    "RndDir SyncDrawables sorts draws");

  const auto rnd_copy = source_rnddir_copy_plan();
  ok &= expect_size(rnd_copy.copied_superclasses.size(), 4,
                    "RndDir copy superclass count");
  ok &= expect_string(rnd_copy.copied_superclasses[0], "ObjectDir",
                      "RndDir copy object dir first");
  ok &= expect_string(rnd_copy.copied_superclasses[3], "RndTransformable",
                      "RndDir copy transform last");
  ok &= expect_string(rnd_copy.copied_members[0], "mEnv",
                      "RndDir copy env");
  ok &= expect_string(rnd_copy.copied_members[1], "mTestEvent",
                      "RndDir copy test event");

  const auto rnd_handlers = source_rnddir_handler_plan();
  ok &= expect_string(rnd_handlers.handlers[0], "show_objects",
                      "RndDir handler show objects");
  ok &= expect_string(rnd_handlers.handlers[1], "supported_events",
                      "RndDir handler supported events");
  ok &= expect_size(rnd_handlers.superclasses.size(), 6,
                    "RndDir handler superclass count");
  ok &= expect_int(rnd_handlers.check, 609, "RndDir handler check");

  const auto rnd_props = source_rnddir_prop_sync_plan();
  ok &= expect_size(rnd_props.properties.size(), 4,
                    "RndDir prop count");
  ok &= expect_string(rnd_props.properties[0], "environ",
                      "RndDir prop env");
  ok &= expect_string(rnd_props.properties[3], "test_event",
                      "RndDir prop test event");
  ok &= expect_string(rnd_props.superclasses[0], "ObjectDir",
                      "RndDir prop superclass ObjectDir");

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

  auto copy_plan = source_character_copy_plan();
  ok &= expect_size(copy_plan.copied_superclasses.size(), 1,
                    "Character copy superclass count");
  ok &= expect_string(copy_plan.copied_superclasses[0], "RndDir",
                      "Character copy superclass");
  ok &= expect_bool(copy_plan.creates_copy, true,
                    "Character copy creates destination");
  ok &= expect_string(copy_plan.member_gate, "ty != kCopyFromMax",
                      "Character copy member gate");
  ok &= expect_size(copy_plan.copied_members.size(), 10,
                    "Character copy member count");
  ok &= expect_string(copy_plan.copied_members[0], "mLods",
                      "Character copy lods first");
  ok &= expect_string(copy_plan.copied_members[2], "mMinLod",
                      "Character copy first min lod");
  ok &= expect_string(copy_plan.copied_members[7], "mFrozen",
                      "Character copy frozen member");
  ok &= expect_string(copy_plan.copied_members[8], "mMinLod",
                      "Character copy duplicated min lod");
  ok &= expect_string(copy_plan.copied_members[9], "mTransGroup",
                      "Character copy trans group last");

  const auto handlers = source_character_handler_plan();
  ok &= expect_size(handlers.handlers.size(), 8,
                    "Character handler source row count");
  ok &= expect_string(handlers.handlers[0], "teleport",
                      "Character handler teleport first");
  ok &= expect_string(handlers.handlers[1], "play_clip",
                      "Character handler play_clip second");
  ok &= expect_string(handlers.handlers[3], "copy_bounding_sphere",
                      "Character handler copy bounding sphere");
  ok &= expect_string(handlers.handlers[7], "enable_blink",
                      "Character handler enable_blink last");
  ok &= expect_size(handlers.debug_handlers.size(), 2,
                    "Character debug handler row count");
  ok &= expect_string(handlers.superclass, "RndDir",
                      "Character handler superclass");
  ok &= expect_string(handlers.check, "0x57B", "Character handler check");
  ok &= expect_int(source_character_save_plan().save_id, 0x495,
                   "Character save id");

  const auto props = source_character_prop_sync_plan();
  ok &= expect_size(props.set_properties.size(), 3,
                    "Character set property count");
  ok &= expect_string(props.set_properties[0], "sphere_base",
                      "Character set prop sphere base first");
  ok &= expect_string(props.set_properties[1], "shadow",
                      "Character set prop shadow");
  ok &= expect_string(props.set_properties[2], "driver",
                      "Character set prop driver");
  ok &= expect_size(props.properties.size(), 6,
                    "Character direct property count");
  ok &= expect_string(props.properties[0], "lods",
                      "Character prop lods first");
  ok &= expect_string(props.properties[5], "frozen",
                      "Character prop frozen last");
  ok &= expect_size(props.modify_properties.size(), 1,
                    "Character modify property count");
  ok &= expect_string(props.modify_properties[0], "interest_to_force",
                      "Character modify prop interest");
  ok &= expect_string(props.superclass, "RndDir",
                      "Character prop-sync superclass");

  auto play = source_character_on_play_clip(false, 3, 9, true);
  ok &= expect_bool(play.called_driver_play, false,
                    "OnPlayClip skips without driver");
  ok &= expect_bool(play.returns_true, false,
                    "OnPlayClip without driver returns false");

  play = source_character_on_play_clip(true, 3, 9, true);
  ok &= expect_bool(play.called_driver_play, true,
                    "OnPlayClip calls driver");
  ok &= expect_int(play.play_flags, 4, "OnPlayClip default play flags");
  ok &= expect_float(play.blend_width, -1.0f, "OnPlayClip blend width");
  ok &= expect_float(play.end_beat, 1.0e30f, "OnPlayClip end beat");
  ok &= expect_float(play.start_beat, 0.0f, "OnPlayClip start beat");
  ok &= expect_bool(play.returns_true, true,
                    "OnPlayClip returns driver success");

  play = source_character_on_play_clip(true, 4, 6, false);
  ok &= expect_int(play.play_flags, 6, "OnPlayClip supplied play flags");
  ok &= expect_bool(play.returns_true, false,
                    "OnPlayClip returns driver failure");

  play = source_character_on_play_clip(true, 5, 7, true);
  ok &= expect_bool(play.would_assert_size, true,
                    "OnPlayClip records source size assert");
  ok &= expect_bool(play.called_driver_play, false,
                    "OnPlayClip assert gates driver play");

  auto copy_handler = source_character_on_copy_bounding_sphere(false);
  ok &= expect_bool(copy_handler.copied, false,
                    "OnCopyBoundingSphere skips missing source");
  ok &= expect_bool(copy_handler.returns_zero, true,
                    "OnCopyBoundingSphere returns zero");
  copy_handler = source_character_on_copy_bounding_sphere(true);
  ok &= expect_bool(copy_handler.copied, true,
                    "OnCopyBoundingSphere copies present source");

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

  const auto runtime_dump = source_character_runtime_dump_evidence();
  ok &= expect_string(runtime_dump.poll_range, "0x8030D360 -> 0x8030D434",
                      "runtime dump Poll range");
  ok &= expect_string(runtime_dump.bone_servo_range,
                      "0x8030E15C -> 0x8030E190",
                      "runtime dump BoneServo range");
  ok &= expect_string(runtime_dump.convert_bones_to_transes_range,
                      "0x8030EE9C -> 0x8030F7CC",
                      "runtime dump ConvertBonesToTranses range");
  ok &= expect_string(runtime_dump.sync_objects_range,
                      "0x8030F7CC -> 0x8030FD5C",
                      "runtime dump SyncObjects range");
  ok &= expect_bool(has(runtime_dump.poll_locals, "AutoTimer _at"), true,
                    "runtime dump Poll local");
  ok &= expect_bool(has(runtime_dump.bone_servo_references,
                        "CharServoBone RTTI"),
                    true, "runtime dump BoneServo RTTI");
  ok &= expect_bool(has(runtime_dump.convert_bones_to_transes_locals,
                        "ObjDirItr mesh"),
                    true, "runtime dump ConvertBonesToTranses mesh local");
  ok &= expect_bool(has(runtime_dump.sync_objects_locals,
                        "CharPollableSorter sorter"),
                    true, "runtime dump SyncObjects sorter local");
  ok &= expect_bool(runtime_dump.has_statement_bodies, false,
                    "runtime dump has no statement bodies");
  ok &= expect_bool(runtime_dump.safe_to_publish_pose, false,
                    "runtime dump does not publish pose");
  ok &= expect_bool(runtime_dump.safe_to_replace_pose_publisher, false,
                    "runtime dump does not replace pose publisher");

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

  auto current_interests =
      source_character_on_get_current_interests(false, {"ignored.look"});
  ok &= expect_bool(current_interests.found_eyes, false,
                    "OnGetCurrentInterests records missing eyes");
  ok &= expect_int(current_interests.interest_count, 0,
                   "OnGetCurrentInterests missing eyes count");
  ok &= expect_bool(current_interests.first_node_empty_symbol, true,
                    "OnGetCurrentInterests first node empty symbol");
  ok &= expect_size(current_interests.data_array_symbols.size(), 1,
                    "OnGetCurrentInterests missing eyes array size");
  ok &= expect_string(current_interests.data_array_symbols[0], "",
                      "OnGetCurrentInterests missing eyes empty symbol");

  current_interests = source_character_on_get_current_interests(
      true, {"singer.look", "guitarist.look", "crowd.look"});
  ok &= expect_bool(current_interests.found_eyes, true,
                    "OnGetCurrentInterests records eyes");
  ok &= expect_int(current_interests.interest_count, 3,
                   "OnGetCurrentInterests source interest count");
  ok &= expect_size(current_interests.data_array_symbols.size(), 4,
                    "OnGetCurrentInterests source array size");
  ok &= expect_string(current_interests.data_array_symbols[0], "",
                      "OnGetCurrentInterests source leading empty symbol");
  ok &= expect_string(current_interests.data_array_symbols[1],
                      "singer.look",
                      "OnGetCurrentInterests first interest name");
  ok &= expect_string(current_interests.data_array_symbols[3],
                      "crowd.look",
                      "OnGetCurrentInterests preserves source order");

  const auto debug_interest_on =
      source_character_set_debug_draw_interest_objects(true);
  ok &= expect_bool(debug_interest_on.assigned, true,
                    "SetDebugDrawInterestObjects assigns flag");
  ok &= expect_bool(debug_interest_on.debug_draw_interest_objects, true,
                    "SetDebugDrawInterestObjects stores true");
  const auto debug_interest_off =
      source_character_set_debug_draw_interest_objects(false);
  ok &= expect_bool(debug_interest_off.debug_draw_interest_objects, false,
                    "SetDebugDrawInterestObjects stores false");

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

  const auto no_deform = source_band_character_deformation_plan(false, true, true);
  ok &= expect_bool(no_deform.has_deform_clip, false,
                    "BandCharacter SetDeformation skips missing clip");
  ok &= expect_size(no_deform.steps.size(), 0,
                    "BandCharacter missing deform clip has no steps");

  const auto deform =
      source_band_character_deformation_plan(true, true, false);
  ok &= expect_bool(deform.has_deform_clip, true,
                    "BandCharacter SetDeformation records deform clip");
  ok &= expect_int(deform.deform_weight_count, 18,
                   "BandCharacter deform weight count");
  ok &= expect_int(deform.sync_mesh_mask, 0xBF,
                   "BandCharacter source SyncMesh mask");
  ok &= expect_bool(deform.poses_neutral_before_cache, true,
                    "BandCharacter poses neutral before mesh cache");
  ok &= expect_bool(deform.poses_weighted_after_cache, true,
                    "BandCharacter poses weighted after deform weights");
  ok &= expect_bool(deform.captures_ik_scale_before, true,
                    "BandCharacter captures IK scale before deformation");
  ok &= expect_bool(deform.captures_ik_scale_after, true,
                    "BandCharacter captures IK scale after deformation");
  ok &= expect_bool(deform.measures_ik_hand_lengths_after_deform, true,
                    "BandCharacter measures IK hand lengths after deformation");
  ok &= expect_bool(deform.clears_dirty_bit, true,
                    "BandCharacter clears deformation dirty bit");
  ok &= expect_string(deform.steps[0],
                      "BandCharDesc::GetDeformClip(mGender)",
                      "BandCharacter first deform step");
  ok &= expect_string(deform.steps[6], "meshes.PoseMeshes(neutral)",
                      "BandCharacter neutral PoseMeshes step");
  ok &= expect_string(deform.steps[7], "BoneServo.AcquirePose",
                      "BandCharacter edit bone-servo acquire step");
  ok &= expect_string(deform.steps[9], "CharMeshCacheMgr.Disable(true)",
                      "BandCharacter disables cache outside closet");
  ok &= expect_string(deform.steps[16], "ComputeDeformWeights(weights[18])",
                      "BandCharacter computes source deform weights");
  ok &= expect_string(deform.steps[18], "meshes.PoseMeshes(weighted)",
                      "BandCharacter weighted PoseMeshes step");
  ok &= expect_string(deform.steps[21], "CharIKScale.CaptureAfter",
                      "BandCharacter IK scale capture-after step");
  ok &= expect_string(deform.steps[22], "CharIKHand.MeasureLengths",
                      "BandCharacter IK hand measure step");
  ok &= expect_string(deform.steps[26], "clear unk224 dirty bit 0x2",
                      "BandCharacter dirty-bit clear step");

  const auto closet_deform =
      source_band_character_deformation_plan(true, false, true);
  ok &= expect_string(closet_deform.steps[7], "skip BoneServo.AcquirePose",
                      "BandCharacter non-edit skips bone-servo acquire");
  ok &= expect_string(closet_deform.steps[9],
                      "CharMeshCacheMgr.Disable(false)",
                      "BandCharacter keeps mesh cache enabled in closet");

  std::vector<SourceCharPollableSorterDep> deps = {
      {"target.driver", {}, 0},
      {"lookat.poll", {0}, 0},
      {"hair.poll", {1}, 0},
  };
  auto changed_by = source_char_pollable_sorter_changed_by(deps, 0, 2, 7);
  ok &= expect_bool(changed_by.changed_by, true,
                    "PollableSorter finds transitive dependency");
  ok &= expect_int(changed_by.search_id, 8,
                   "PollableSorter increments search id");
  ok &= expect_size(changed_by.visited_indices.size(), 2,
                    "PollableSorter visits before target");
  ok &= expect_int(changed_by.visited_indices[0], 2,
                   "PollableSorter visits query first");
  ok &= expect_int(changed_by.visited_indices[1], 1,
                   "PollableSorter visits changedBy edge");
  ok &= expect_int(deps[2].search_id, 8,
                   "PollableSorter marks query search id");
  ok &= expect_int(deps[1].search_id, 8,
                   "PollableSorter marks intermediate search id");
  ok &= expect_int(deps[0].search_id, 0,
                   "PollableSorter returns before marking target");

  std::vector<SourceCharPollableSorterDep> cycle_deps = {
      {"cycle.a", {1}, 0},
      {"cycle.b", {0}, 0},
      {"unreached.target", {}, 0},
  };
  changed_by = source_char_pollable_sorter_changed_by(cycle_deps, 2, 0, 41);
  ok &= expect_bool(changed_by.changed_by, false,
                    "PollableSorter cycle does not fabricate reachability");
  ok &= expect_int(changed_by.search_id, 42,
                   "PollableSorter cycle search id");
  ok &= expect_size(changed_by.visited_indices.size(), 2,
                    "PollableSorter cycle visits once per dep");
  ok &= expect_int(cycle_deps[0].search_id, 42,
                   "PollableSorter cycle marks first dep");
  ok &= expect_int(cycle_deps[1].search_id, 42,
                   "PollableSorter cycle marks second dep");

  changed_by = source_char_pollable_sorter_changed_by(cycle_deps, 1, 1, 42);
  ok &= expect_bool(changed_by.changed_by, false,
                    "PollableSorter same dep returns false");
  ok &= expect_bool(changed_by.same_dep_short_circuit, true,
                    "PollableSorter records same dep short-circuit");
  ok &= expect_int(changed_by.search_id, 42,
                   "PollableSorter same dep does not increment search id");
  ok &= expect_size(changed_by.visited_indices.size(), 0,
                    "PollableSorter same dep skips recursion");

  changed_by = source_char_pollable_sorter_changed_by(cycle_deps, 0, -1, 5);
  ok &= expect_bool(changed_by.changed_by, false,
                    "PollableSorter null query returns false");
  ok &= expect_int(changed_by.search_id, 6,
                   "PollableSorter null query still increments search id");
  ok &= expect_size(changed_by.visited_indices.size(), 0,
                    "PollableSorter null query visits no deps");

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
