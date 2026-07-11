#include "character/char_clip.h"

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
  using ghogx::character::source_char_clip_set_copy;
  using ghogx::character::source_char_clip_set_default_state;
  using ghogx::character::source_char_clip_set_draw_showing;
  using ghogx::character::source_char_clip_set_end_frame;
  using ghogx::character::source_char_clip_set_load_character;
  using ghogx::character::source_char_clip_set_post_load_plan;
  using ghogx::character::source_char_clip_set_post_save;
  using ghogx::character::source_char_clip_set_pre_load_plan;
  using ghogx::character::source_char_clip_set_pre_save;
  using ghogx::character::source_char_clip_set_randomize_groups;
  using ghogx::character::source_char_clip_set_recenter_all_warning;
  using ghogx::character::source_char_clip_set_reset_editor_state;
  using ghogx::character::source_char_clip_set_reset_preview_state;
  using ghogx::character::source_char_clip_set_set_bpm;
  using ghogx::character::source_char_clip_set_sort_groups;
  using ghogx::character::source_char_clip_set_start_frame;

  bool ok = true;

  auto state = source_char_clip_set_default_state();
  ok &= expect_string(state.char_file_root, "", "default char file root");
  ok &= expect_bool(state.has_preview_char, false, "default preview char");
  ok &= expect_bool(state.has_preview_clip, false, "default preview clip");
  ok &= expect_bool(state.has_still_clip, false, "default still clip");
  ok &= expect_int(state.filter_flags, 0, "default filter flags");
  ok &= expect_int(state.bpm, 90, "default bpm");
  ok &= expect_bool(state.preview_walk, false, "default preview walk");
  ok &= expect_bool(state.rate_is_1_fpb, true, "constructor rate");

  state.char_file_root = "characters/rock1";
  state.has_preview_char = true;
  state.has_preview_clip = true;
  state.has_still_clip = true;
  state.filter_flags = 7;
  state.bpm = 140;
  state.preview_walk = true;
  source_char_clip_set_reset_preview_state(state);
  ok &= expect_string(state.char_file_root, "", "ResetPreviewState root");
  ok &= expect_bool(state.has_preview_char, false,
                    "ResetPreviewState deletes preview char");
  ok &= expect_bool(state.has_preview_clip, false,
                    "ResetPreviewState clears preview clip");
  ok &= expect_bool(state.has_still_clip, false,
                    "ResetPreviewState clears still clip");
  ok &= expect_int(state.filter_flags, 0, "ResetPreviewState filter flags");
  ok &= expect_int(state.bpm, 90, "ResetPreviewState bpm");
  ok &= expect_bool(state.preview_walk, false, "ResetPreviewState walk");

  state.has_preview_char = true;
  auto reset_editor = source_char_clip_set_reset_editor_state(state);
  ok &= expect_bool(reset_editor.reset_preview_state, true,
                    "ResetEditorState resets preview");
  ok &= expect_bool(reset_editor.object_dir_reset_editor_state, true,
                    "ResetEditorState delegates ObjectDir");
  ok &= expect_bool(state.has_preview_char, false,
                    "ResetEditorState clears preview char");

  const std::vector<std::string> groups = {"idle.grp", "solo.grp"};
  auto group_steps = source_char_clip_set_randomize_groups(groups);
  ok &= expect_size(group_steps.size(), 2, "RandomizeGroups count");
  ok &= expect_string(group_steps[0].group, "idle.grp",
                      "RandomizeGroups preserves order");
  ok &= expect_bool(group_steps[1].randomize, true,
                    "RandomizeGroups calls Randomize");
  ok &= expect_bool(group_steps[1].sort, false,
                    "RandomizeGroups does not sort");

  group_steps = source_char_clip_set_sort_groups(groups);
  ok &= expect_bool(group_steps[0].randomize, false,
                    "SortGroups does not randomize");
  ok &= expect_bool(group_steps[0].sort, true, "SortGroups calls Sort");

  state.has_preview_char = true;
  auto pre_save = source_char_clip_set_pre_save(state, false);
  ok &= expect_bool(pre_save.preview_char_name_cleared, true,
                    "PreSave clears preview name");
  ok &= expect_bool(pre_save.reset_preview_state, false,
                    "PreSave noncached keeps preview state");
  ok &= expect_bool(state.has_preview_char, true,
                    "PreSave noncached keeps preview char");

  pre_save = source_char_clip_set_pre_save(state, true);
  ok &= expect_bool(pre_save.reset_preview_state, true,
                    "PreSave cached resets preview");
  ok &= expect_bool(pre_save.reset_editor_state, true,
                    "PreSave cached resets editor");
  ok &= expect_bool(state.has_preview_char, false,
                    "PreSave cached clears preview char");

  state.has_preview_char = true;
  auto post_save = source_char_clip_set_post_save(state, true);
  ok &= expect_bool(post_save.object_dir_post_save, true,
                    "PostSave delegates ObjectDir");
  ok &= expect_bool(post_save.preview_char_name_restored, true,
                    "PostSave restores preview name");
  ok &= expect_bool(post_save.preview_char_entered, true,
                    "PostSave enters preview character");
  ok &= expect_bool(post_save.sent_update_objects, true,
                    "PostSave sends update_objects when milo found");

  post_save = source_char_clip_set_post_save(state, false);
  ok &= expect_bool(post_save.sent_update_objects, false,
                    "PostSave skips update without milo");

  auto pre_load = source_char_clip_set_pre_load_plan();
  ok &= expect_int(pre_load.max_revision, 0x18, "PreLoad max revision");
  ok &= expect_bool(pre_load.require_revision_gt_3, true,
                    "PreLoad revision lower bound");
  ok &= expect_bool(pre_load.push_packed_revision, true,
                    "PreLoad pushes revision");
  ok &= expect_bool(pre_load.object_dir_pre_load, true,
                    "PreLoad delegates ObjectDir");

  auto post_load = source_char_clip_set_post_load_plan(4, false, 3, false);
  ok &= expect_bool(post_load.object_dir_post_load, true,
                    "PostLoad delegates ObjectDir");
  ok &= expect_bool(post_load.read_two_legacy_ints, true,
                    "PostLoad rev4 two legacy ints");
  ok &= expect_bool(post_load.read_legacy_graph_path, true,
                    "PostLoad rev4 graph path");
  ok &= expect_bool(post_load.read_legacy_reexport_string, true,
                    "PostLoad rev4 reexport string");
  ok &= expect_bool(post_load.read_rev_lt7_int, true,
                    "PostLoad rev4 extra int");
  ok &= expect_int(post_load.read_legacy_clip_triplets, 3,
                   "PostLoad rev4 clip triplets");
  ok &= expect_bool(post_load.read_symbol_count, true,
                    "PostLoad rev4 symbol count");
  ok &= expect_bool(post_load.warn_transition_bug, true,
                    "PostLoad rev4 bug warning");
  ok &= expect_bool(post_load.handle_filter_clips, true,
                    "PostLoad rev4 filter clips");

  post_load = source_char_clip_set_post_load_plan(0x17, false, 2, true);
  ok &= expect_bool(post_load.read_old_flag_bool, true,
                    "PostLoad rev23 old flag bool");
  ok &= expect_bool(post_load.read_old_flag_second_bool, true,
                    "PostLoad rev23 second old flag bool");
  ok &= expect_bool(post_load.read_legacy_string_lists, true,
                    "PostLoad rev23 legacy string lists");
  ok &= expect_bool(post_load.read_legacy_symbol_and_int, true,
                    "PostLoad rev23 symbol and int");
  ok &= expect_bool(post_load.read_char_file_path, true,
                    "PostLoad rev23 char file path");
  ok &= expect_bool(post_load.read_preview_clip, true,
                    "PostLoad rev23 preview clip");
  ok &= expect_bool(post_load.read_filter_flags, true,
                    "PostLoad rev23 filter flags");
  ok &= expect_bool(post_load.read_bpm, true, "PostLoad rev23 bpm");
  ok &= expect_bool(post_load.read_preview_walk, true,
                    "PostLoad rev23 preview walk");
  ok &= expect_bool(post_load.read_still_clip, true,
                    "PostLoad rev23 still clip");

  post_load = source_char_clip_set_post_load_plan(0x18, true, 5, false);
  ok &= expect_bool(post_load.returned_for_proxy, true,
                    "PostLoad proxy returns early");
  ok &= expect_int(post_load.read_legacy_clip_triplets, 0,
                   "PostLoad proxy skips clip triplets");

  state = source_char_clip_set_default_state();
  auto load_character =
      source_char_clip_set_load_character(state, false, true, true, false, true);
  ok &= expect_bool(load_character.asserted_edit_mode, false,
                    "LoadCharacter records failed edit assertion");
  ok &= expect_bool(load_character.loaded_objects, false,
                    "LoadCharacter skips load outside edit mode");

  load_character =
      source_char_clip_set_load_character(state, true, true, false, true, true);
  ok &= expect_bool(load_character.deleted_preview_char, true,
                    "LoadCharacter deletes old preview");
  ok &= expect_bool(load_character.loaded_objects, true,
                    "LoadCharacter loads objects");
  ok &= expect_bool(load_character.loaded_rnd_dir, true,
                    "LoadCharacter accepts RndDir");
  ok &= expect_bool(load_character.selected_nested_character, true,
                    "LoadCharacter selects nested character");
  ok &= expect_bool(load_character.preview_char_entered, true,
                    "LoadCharacter enters preview");
  ok &= expect_bool(load_character.preview_char_named, true,
                    "LoadCharacter names preview");
  ok &= expect_bool(load_character.sent_update_objects, true,
                    "LoadCharacter sends update_objects");
  ok &= expect_bool(state.has_preview_char, true,
                    "LoadCharacter stores preview char");

  ok &= expect_bool(source_char_clip_set_draw_showing(false), false,
                    "DrawShowing skips without preview char");
  ok &= expect_bool(source_char_clip_set_draw_showing(true), true,
                    "DrawShowing draws preview char");
  ok &= near(source_char_clip_set_start_frame(false, 10.0f), 0.0f,
             "StartFrame default");
  ok &= near(source_char_clip_set_start_frame(true, 10.0f), 10.0f,
             "StartFrame preview clip");
  ok &= near(source_char_clip_set_end_frame(false, 20.0f), 0.0f,
             "EndFrame default");
  ok &= near(source_char_clip_set_end_frame(true, 20.0f), 20.0f,
             "EndFrame preview clip");

  auto bpm = source_char_clip_set_set_bpm(state, 128, true);
  ok &= expect_int(state.bpm, 128, "SetBpm stores bpm");
  ok &= expect_bool(bpm.set_milo_property, true,
                    "SetBpm updates milo property when found");
  ok &= expect_int(bpm.bpm, 128, "SetBpm result bpm");
  ok &= expect_string(source_char_clip_set_recenter_all_warning(),
                      "You can only recenter clips from PC",
                      "RecenterAll warning");

  ghogx::character::SourceCharClipSetState source;
  source.char_file_root = "char/path";
  source.has_preview_clip = true;
  source.has_still_clip = true;
  source.filter_flags = 5;
  source.bpm = 111;
  source.preview_walk = true;
  ghogx::character::SourceCharClipSetState dest =
      source_char_clip_set_default_state();
  auto copy = source_char_clip_set_copy(dest, source);
  ok &= expect_bool(copy.copy_object_dir, true, "Copy ObjectDir");
  ok &= expect_bool(copy.copy_char_file_path, true,
                    "Copy char file path");
  ok &= expect_bool(copy.copy_preview_clip, true, "Copy preview clip");
  ok &= expect_bool(copy.copy_filter_flags, true, "Copy filter flags");
  ok &= expect_bool(copy.copy_bpm, true, "Copy bpm");
  ok &= expect_bool(copy.copy_preview_walk, true, "Copy preview walk");
  ok &= expect_bool(copy.copy_still_clip, true, "Copy still clip");
  ok &= expect_string(dest.char_file_root, "char/path",
                      "Copy char file path value");
  ok &= expect_bool(dest.has_preview_clip, true, "Copy preview clip value");
  ok &= expect_bool(dest.has_still_clip, true, "Copy still clip value");
  ok &= expect_int(dest.filter_flags, 5, "Copy filter flags value");
  ok &= expect_int(dest.bpm, 111, "Copy bpm value");
  ok &= expect_bool(dest.preview_walk, true, "Copy preview walk value");

  return ok ? 0 : 1;
}
