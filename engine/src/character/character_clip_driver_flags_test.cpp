#include "character/char_clip.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

uint32_t source_mask(uint32_t base, uint32_t mask) {
  uint32_t out = base;
  if (mask & 0xF0u) out = (out & 0xffffff0fu) | (mask & 0xF0u);
  if (mask & 0x0Fu) out = (out & 0xfffffff0u) | (mask & 0x0Fu);
  if (mask & 0xF600u) out = (out & 0xffff09ffu) | (mask & 0xF600u);
  return out;
}

bool expect_flags(uint32_t base, uint32_t mask, const char* label) {
  ghogx::character::CharClip clip;
  clip.default_play_flags = base;
  const uint32_t got =
      ghogx::character::char_clip_driver_masked_play_flags(clip, mask);
  const uint32_t want = source_mask(base, mask);
  if (got == want) return true;
  std::cerr << "masked play flags mismatch for " << label << ": got 0x"
            << std::hex << got << " want 0x" << want << std::dec << "\n";
  return false;
}

bool expect_group_duplicates(const std::vector<uint32_t>& clip_flags,
                             size_t clip_index, uint32_t mask, int want,
                             const char* label) {
  const int got = ghogx::character::source_char_clip_group_num_flag_duplicates(
      clip_flags, clip_index, mask);
  if (got == want) return true;
  std::cerr << "group duplicate mismatch for " << label << ": got " << got
            << " want " << want << "\n";
  return false;
}

bool expect_group_sort(const std::vector<std::string>& input,
                       const std::vector<std::string>& want,
                       const char* label) {
  const std::vector<std::string> got =
      ghogx::character::source_char_clip_group_sorted_names(input);
  if (got == want) return true;
  std::cerr << "group sort mismatch for " << label << "\n";
  return false;
}

bool expect_group_add(const std::vector<std::string>& input,
                      const std::string& clip_name,
                      const std::vector<std::string>& want,
                      const char* label) {
  const std::vector<std::string> got =
      ghogx::character::source_char_clip_group_add_clip(input, clip_name);
  if (got == want) return true;
  std::cerr << "group add mismatch for " << label << ": got";
  for (const auto& clip : got) std::cerr << " " << clip;
  std::cerr << " want";
  for (const auto& clip : want) std::cerr << " " << clip;
  std::cerr << "\n";
  return false;
}

bool expect_group_remove(const std::vector<std::string>& input,
                         const std::string& clip_name,
                         const std::vector<std::string>& want,
                         const char* label) {
  const std::vector<std::string> got =
      ghogx::character::source_char_clip_group_remove_clip(input, clip_name);
  if (got == want) return true;
  std::cerr << "group remove mismatch for " << label << ": got";
  for (const auto& clip : got) std::cerr << " " << clip;
  std::cerr << " want";
  for (const auto& clip : want) std::cerr << " " << clip;
  std::cerr << "\n";
  return false;
}

bool expect_indices(const std::vector<size_t>& got,
                    const std::vector<size_t>& want,
                    const char* label) {
  if (got == want) return true;
  std::cerr << "index vector mismatch for " << label << ": got";
  for (size_t index : got) std::cerr << " " << index;
  std::cerr << " want";
  for (size_t index : want) std::cerr << " " << index;
  std::cerr << "\n";
  return false;
}

bool nearf(float got, float want) {
  return std::fabs(got - want) <= 0.0001f;
}

const ghogx::character::ClipChannel* find_pose_channel(
    const std::vector<ghogx::character::ClipChannel>& pose,
    ghogx::character::ClipChannel::Type type, const std::string& bone_name) {
  for (const auto& channel : pose) {
    if (channel.type == type && channel.bone_name == bone_name) {
      return &channel;
    }
  }
  return nullptr;
}

bool has_any_pose_channel(
    const std::vector<ghogx::character::ClipChannel>& pose,
    ghogx::character::ClipChannel::Type type,
    const std::vector<std::string>& bone_names) {
  for (const std::string& bone_name : bone_names) {
    if (find_pose_channel(pose, type, bone_name)) return true;
  }
  return false;
}

bool expect_starved(bool has_first, bool first_has_next,
                    uint32_t first_play_flags, bool want,
                    const char* label) {
  const bool got = ghogx::character::source_char_driver_starved(
      has_first, first_has_next, first_play_flags);
  if (got == want) return true;
  std::cerr << "starved mismatch for " << label << ": got " << got
            << " want " << want << "\n";
  return false;
}

bool expect_beat_align(uint32_t mask, const char* want, const char* label) {
  const char* got = ghogx::character::source_char_clip_beat_align_string(mask);
  if (std::string(got) == want) return true;
  std::cerr << "beat-align string mismatch for " << label << ": got '"
            << got << "' want '" << want << "'\n";
  return false;
}

bool expect_flag_update(const ghogx::character::SourceCharClipFlagUpdate& got,
                        uint32_t value, bool dirty, bool changed,
                        const char* label) {
  bool ok = true;
  if (got.value != value) {
    std::cerr << "flag update value mismatch for " << label << ": got 0x"
              << std::hex << got.value << " want 0x" << value << std::dec
              << "\n";
    ok = false;
  }
  if (got.dirty != dirty) {
    std::cerr << "flag update dirty mismatch for " << label << ": got "
              << got.dirty << " want " << dirty << "\n";
    ok = false;
  }
  if (got.changed != changed) {
    std::cerr << "flag update changed mismatch for " << label << ": got "
              << got.changed << " want " << changed << "\n";
    ok = false;
  }
  return ok;
}

bool expect_shares_groups(
    const std::vector<ghogx::character::SourceCharClipRefOwner>& ref_owners,
    const std::string& candidate_clip_name,
    bool want,
    const char* label) {
  const bool got =
      ghogx::character::source_char_clip_shares_groups(ref_owners,
                                                       candidate_clip_name);
  if (got == want) return true;
  std::cerr << "SharesGroups mismatch for " << label << ": got " << got
            << " want " << want << "\n";
  return false;
}

bool expect_default_state(const ghogx::character::SourceCharClipDefaultState& got) {
  bool ok = true;
  if (got.frames_per_sec != 30.0f) {
    std::cerr << "default frames_per_sec mismatch: got "
              << got.frames_per_sec << "\n";
    ok = false;
  }
  if (got.flags != 0) {
    std::cerr << "default flags mismatch: got " << got.flags << "\n";
    ok = false;
  }
  if (got.play_flags != 0) {
    std::cerr << "default play_flags mismatch: got " << got.play_flags << "\n";
    ok = false;
  }
  if (got.range != 0.0f) {
    std::cerr << "default range mismatch: got " << got.range << "\n";
    ok = false;
  }
  if (!got.dirty) {
    std::cerr << "default dirty mismatch\n";
    ok = false;
  }
  if (got.do_not_compress) {
    std::cerr << "default do_not_compress mismatch\n";
    ok = false;
  }
  if (got.unk42 != -1) {
    std::cerr << "default unk42 mismatch: got " << got.unk42 << "\n";
    ok = false;
  }
  if (got.beat_track_count != 1) {
    std::cerr << "default beat_track_count mismatch: got "
              << got.beat_track_count << "\n";
    ok = false;
  }
  if (got.first_beat_frame != 0.0f || got.first_beat_value != 0.0f) {
    std::cerr << "default first beat mismatch: got frame "
              << got.first_beat_frame << " value " << got.first_beat_value
              << "\n";
    ok = false;
  }
  return ok;
}

bool expect_beat_event(const ghogx::character::SourceCharClipBeatEvent& got,
                       const std::string& event, float beat,
                       const char* label) {
  bool ok = true;
  if (got.event != event) {
    std::cerr << "beat event name mismatch for " << label << ": got '"
              << got.event << "' want '" << event << "'\n";
    ok = false;
  }
  if (!nearf(got.beat, beat)) {
    std::cerr << "beat event beat mismatch for " << label << ": got "
              << got.beat << " want " << beat << "\n";
    ok = false;
  }
  return ok;
}

bool expect_num_frames(const ghogx::character::SourceCharClipNumFramesPlan& got,
                       int want,
                       const char* label) {
  bool ok = true;
  if (got.num_frames != want) {
    std::cerr << "NumFrames mismatch for " << label << ": got "
              << got.num_frames << " want " << want << "\n";
    ok = false;
  }
  if (!got.clamps_minimum_to_one || !got.uses_full_num_samples ||
      !got.uses_full_frame_count || !got.ignores_one_num_samples) {
    std::cerr << "NumFrames source flags mismatch for " << label << "\n";
    ok = false;
  }
  return ok;
}

bool expect_driver_default_state(
    const ghogx::character::SourceCharDriverState& got) {
  bool ok = true;
  if (got.has_bones || got.has_clips || got.has_first || got.has_test_clip ||
      got.has_default_clip || got.default_play_starved ||
      got.last_node_valid || got.realign || got.has_internal_bones ||
      got.play_multiple_clips) {
    std::cerr << "driver default bool state mismatch\n";
    ok = false;
  }
  if (got.old_beat != 1.0e30f) {
    std::cerr << "driver default old_beat mismatch: got " << got.old_beat
              << "\n";
    ok = false;
  }
  if (got.beat_scale != 1.0f || got.blend_width != 1.0f) {
    std::cerr << "driver default scales mismatch: beat " << got.beat_scale
              << " blend " << got.blend_width << "\n";
    ok = false;
  }
  if (!got.clip_type.empty()) {
    std::cerr << "driver default clip_type mismatch\n";
    ok = false;
  }
  if (got.apply != ghogx::character::kSourceCharDriverApplyBlend) {
    std::cerr << "driver default apply mismatch: got " << got.apply << "\n";
    ok = false;
  }
  return ok;
}

bool expect_sync_decision(
    const ghogx::character::SourceCharDriverSyncDecision& got,
    bool changed,
    bool clear_stack,
    bool reset_last_node,
    bool delete_internal_bones,
    bool allocate_internal_bones,
    bool clear_internal_bones,
    bool stuff_internal_bones,
    bool has_internal_bones,
    const char* label) {
  bool ok = true;
  if (got.changed != changed || got.clear_stack != clear_stack ||
      got.reset_last_node != reset_last_node ||
      got.delete_internal_bones != delete_internal_bones ||
      got.allocate_internal_bones != allocate_internal_bones ||
      got.clear_internal_bones != clear_internal_bones ||
      got.stuff_internal_bones != stuff_internal_bones ||
      got.has_internal_bones != has_internal_bones) {
    std::cerr << "driver sync decision mismatch for " << label << ": got "
              << got.changed << "," << got.clear_stack << ","
              << got.reset_last_node << "," << got.delete_internal_bones
              << "," << got.allocate_internal_bones << ","
              << got.clear_internal_bones << "," << got.stuff_internal_bones
              << "," << got.has_internal_bones << "\n";
    ok = false;
  }
  return ok;
}

bool expect_enter_decision(
    const ghogx::character::SourceCharDriverEnterDecision& got,
    bool play_default_clip,
    const char* label) {
  bool ok = true;
  if (!got.changed || !got.clear_stack || !got.reset_last_node ||
      !got.reset_old_beat || !got.reset_beat_scale ||
      got.play_default_clip != play_default_clip ||
      got.default_play_flags != 1 ||
      got.default_requested_blend_width != -1.0f ||
      got.default_old_beat != 1.0e30f ||
      got.default_start != 0.0f) {
    std::cerr << "driver Enter decision mismatch for " << label << "\n";
    ok = false;
  }
  return ok;
}

bool expect_play_group_decision(bool has_clip_dir, bool found_group,
                                bool want_warn_no_clips,
                                bool want_warn_missing_group,
                                bool want_get_clip,
                                bool want_play,
                                const char* label) {
  const ghogx::character::SourceCharDriverPlayGroupDecision got =
      ghogx::character::source_char_driver_play_group_decision(has_clip_dir,
                                                               found_group);
  bool ok = true;
  if (got.has_clip_dir != has_clip_dir || got.found_group != found_group ||
      got.warn_no_clips != want_warn_no_clips ||
      got.warn_missing_group != want_warn_missing_group ||
      got.call_group_get_clip != want_get_clip ||
      got.request_play != want_play) {
    std::cerr << "driver PlayGroup decision mismatch for " << label << "\n";
    ok = false;
  }
  return ok;
}

bool expect_driver_play_decision(
    const ghogx::character::SourceCharDriverPlayDecision& got,
    bool found_clip,
    bool notify_missing_clip,
    bool set_last_node,
    bool duplicate_clip,
    bool create_clip_driver,
    bool new_stack_head,
    float resolved_blend_width,
    const char* label) {
  bool ok = true;
  if (got.found_clip != found_clip ||
      got.notify_missing_clip != notify_missing_clip ||
      got.set_last_node != set_last_node ||
      got.duplicate_clip != duplicate_clip ||
      got.create_clip_driver != create_clip_driver ||
      got.new_stack_head != new_stack_head ||
      !nearf(got.resolved_blend_width, resolved_blend_width)) {
    std::cerr << "driver Play decision mismatch for " << label << ": got "
              << got.found_clip << "," << got.notify_missing_clip << ","
              << got.set_last_node << "," << got.duplicate_clip << ","
              << got.create_clip_driver << "," << got.new_stack_head
              << " blend=" << got.resolved_blend_width << "\n";
    ok = false;
  }
  return ok;
}

bool expect_driver_play_node_decision(
    const ghogx::character::SourceCharDriverPlayNodeDecision& got,
    bool returned_driver,
    const char* label) {
  bool ok = true;
  if (!got.copied_requested_node || !got.find_clip_warn ||
      !got.final_last_node_from_request ||
      got.returned_driver != returned_driver) {
    std::cerr << "driver Play(DataNode) decision mismatch for " << label
              << "\n";
    ok = false;
  }
  return ok;
}

bool expect_driver_state_helpers() {
  bool ok = true;
  ghogx::character::SourceCharDriverState state =
      ghogx::character::source_char_driver_default_state();
  ok &= expect_driver_default_state(state);

  const auto destructor_plan =
      ghogx::character::source_char_driver_destructor_plan();
  if (!destructor_plan.delete_stack_when_first ||
      !destructor_plan.delete_internal_bones ||
      destructor_plan.calls_clear ||
      destructor_plan.clears_first_pointer ||
      destructor_plan.clears_internal_bones_pointer) {
    std::cerr << "driver destructor plan no longer matches source cleanup\n";
    ok = false;
  }

  const auto exit_plan = ghogx::character::source_char_driver_exit_plan();
  if (!exit_plan.call_rnd_pollable_exit || exit_plan.clear_stack ||
      exit_plan.reset_last_node || exit_plan.evaluate_starved) {
    std::cerr << "driver Exit plan no longer matches source delegation\n";
    ok = false;
  }

  const auto highlight_deferred =
      ghogx::character::source_char_driver_highlight_decision(-1.0f);
  if (!highlight_deferred.global_y_is_sentinel ||
      !highlight_deferred.defer_highlight ||
      highlight_deferred.call_display ||
      highlight_deferred.write_global_y_from_display) {
    std::cerr << "driver Highlight sentinel decision mismatch\n";
    ok = false;
  }

  const auto highlight_display =
      ghogx::character::source_char_driver_highlight_decision(3.5f);
  if (highlight_display.global_y_is_sentinel ||
      highlight_display.defer_highlight ||
      !highlight_display.call_display ||
      !highlight_display.write_global_y_from_display ||
      highlight_display.display_input != 3.5f) {
    std::cerr << "driver Highlight display decision mismatch\n";
    ok = false;
  }

  state.has_first = true;
  ghogx::character::source_char_driver_clear(state);
  if (state.has_first) {
    std::cerr << "driver clear left stack attached\n";
    ok = false;
  }

  state.has_first = true;
  state.last_node_valid = true;
  state.old_beat = 2.0f;
  state.beat_scale = 0.5f;
  ok &= expect_enter_decision(ghogx::character::source_char_driver_enter(state),
                              false, "without default clip");
  if (state.has_first || state.last_node_valid || state.old_beat != 1.0e30f ||
      state.beat_scale != 1.0f) {
    std::cerr << "driver Enter reset state mismatch\n";
    ok = false;
  }

  state.has_default_clip = true;
  ok &= expect_enter_decision(ghogx::character::source_char_driver_enter(state),
                              true, "with default clip");
  if (!state.last_node_valid) {
    std::cerr << "driver Enter default clip did not restore last node\n";
    ok = false;
  }

  state.last_node_valid = true;
  ghogx::character::source_char_driver_set_clips(state, false);
  if (!state.last_node_valid) {
    std::cerr << "driver SetClips no-op reset last node\n";
    ok = false;
  }
  ghogx::character::source_char_driver_set_clips(state, true);
  if (!state.has_clips || state.last_node_valid) {
    std::cerr << "driver SetClips changed-state mismatch\n";
    ok = false;
  }

  ghogx::character::source_char_driver_set_bones(state, true);
  if (!state.has_bones) {
    std::cerr << "driver SetBones did not store target\n";
    ok = false;
  }

  ghogx::character::source_char_driver_set_starved(state, "starved.msg");
  if (state.starved_handler != "starved.msg") {
    std::cerr << "driver SetStarved did not store handler\n";
    ok = false;
  }
  ghogx::character::source_char_driver_set_blend_width(state, 0.75f);
  if (state.blend_width != 0.75f) {
    std::cerr << "driver SetBlendWidth did not store width\n";
    ok = false;
  }

  state.has_first = true;
  state.last_node_valid = true;
  ok &= expect_sync_decision(
      ghogx::character::source_char_driver_set_apply(
          state, ghogx::character::kSourceCharDriverApplyBlend),
      false, false, false, false, false, false, false, false,
      "SetApply unchanged");
  if (!state.has_first || !state.last_node_valid) {
    std::cerr << "driver SetApply no-op mutated stack\n";
    ok = false;
  }

  ok &= expect_sync_decision(
      ghogx::character::source_char_driver_set_clip_type(state, "hair"),
      true, true, true, false, false, false, false, false,
      "SetClipType blend mode");
  if (state.has_first || state.last_node_valid || state.clip_type != "hair") {
    std::cerr << "driver SetClipType state mismatch\n";
    ok = false;
  }

  state.has_first = true;
  state.last_node_valid = true;
  ok &= expect_sync_decision(
      ghogx::character::source_char_driver_set_apply(
          state, ghogx::character::kSourceCharDriverApplyBlendWeights),
      true, true, true, false, true, true, true, true,
      "SetApply allocates internal bones");
  if (state.has_first || state.last_node_valid || !state.has_internal_bones) {
    std::cerr << "driver SetApply internal bone state mismatch\n";
    ok = false;
  }

  ok &= expect_sync_decision(
      ghogx::character::source_char_driver_set_clip_type(state, ""),
      true, true, true, true, false, false, false, false,
      "SetClipType deletes internal bones");
  if (state.has_internal_bones || !state.clip_type.empty()) {
    std::cerr << "driver SetClipType empty state mismatch\n";
    ok = false;
  }

  ghogx::character::SourceCharDriverState source;
  source.has_clips = true;
  source.has_first = true;
  source.last_node_valid = true;
  source.realign = true;
  source.beat_scale = 0.5f;
  source.blend_width = 0.25f;
  ghogx::character::SourceCharDriverState dest;
  dest.has_first = true;
  dest.has_bones = true;
  dest.has_test_clip = true;
  dest.has_default_clip = true;
  dest.default_play_starved = true;
  dest.starved_handler = "keep_starved";
  dest.old_beat = 14.0f;
  dest.clip_type = "keep_clip_type";
  dest.apply = ghogx::character::kSourceCharDriverApplyBlendWeights;
  dest.has_internal_bones = true;
  dest.play_multiple_clips = true;
  ghogx::character::source_char_driver_transfer(dest, source);
  if (!dest.has_clips || !dest.has_first || !dest.last_node_valid ||
      !dest.realign || dest.beat_scale != 0.5f ||
      dest.blend_width != 0.25f) {
    std::cerr << "driver Transfer copied source fields incorrectly\n";
    ok = false;
  }
  if (!dest.has_bones || !dest.has_test_clip || !dest.has_default_clip ||
      !dest.default_play_starved || dest.starved_handler != "keep_starved" ||
      dest.old_beat != 14.0f || dest.clip_type != "keep_clip_type" ||
      dest.apply != ghogx::character::kSourceCharDriverApplyBlendWeights ||
      !dest.has_internal_bones || !dest.play_multiple_clips) {
    std::cerr << "driver Transfer mutated source-untouched fields\n";
    ok = false;
  }
  const auto transfer_plan =
      ghogx::character::source_char_driver_transfer_plan(true);
  if (!transfer_plan.clear_stack ||
      !transfer_plan.create_first_driver_copy ||
      transfer_plan.copied_members.size() != 6 ||
      transfer_plan.copied_members[0] != "mClips" ||
      transfer_plan.copied_members[5] !=
          "mFirst:new CharClipDriver(this,*driver.mFirst)" ||
      transfer_plan.preserved_members.size() != 11 ||
      transfer_plan.preserved_members[2] != "mDefaultClip" ||
      transfer_plan.preserved_members[9] != "mPlayMultipleClips") {
    std::cerr << "driver Transfer plan no longer matches source boundary\n";
    ok = false;
  }
  ok &= expect_play_group_decision(false, false, true, false, false, false,
                                   "no clip directory");
  ok &= expect_play_group_decision(true, false, false, true, false, false,
                                   "missing group");
  ok &= expect_play_group_decision(true, true, false, false, true, true,
                                   "group found");

  ghogx::character::SourceCharDriverState play_state =
      ghogx::character::source_char_driver_default_state();
  play_state.blend_width = 0.75f;
  ok &= expect_driver_play_decision(
      ghogx::character::source_char_driver_play_decision(
          play_state, false, false, 7, -1.0f, 3.0f, 0.5f),
      false, true, false, false, false, false, 0.0f,
      "missing clip");
  if (play_state.last_node_valid || play_state.has_first) {
    std::cerr << "driver Play missing clip mutated state\n";
    ok = false;
  }

  ok &= expect_driver_play_decision(
      ghogx::character::source_char_driver_play_decision(
          play_state, true, false, 7, -1.0f, 3.0f, 0.5f),
      true, false, true, false, true, true, 0.75f,
      "new clip uses source blend fallback");
  if (!play_state.last_node_valid || !play_state.has_first) {
    std::cerr << "driver Play did not create stack head\n";
    ok = false;
  }

  play_state.play_multiple_clips = true;
  play_state.has_first = true;
  play_state.last_node_valid = false;
  ok &= expect_driver_play_decision(
      ghogx::character::source_char_driver_play_decision(
          play_state, true, true, 3, 0.25f, 9.0f, 1.0f),
      true, false, true, true, false, false, 0.25f,
      "duplicate clip still records last node");
  if (!play_state.last_node_valid || !play_state.has_first) {
    std::cerr << "driver Play duplicate gate lost state\n";
    ok = false;
  }

  ghogx::character::SourceCharDriverState node_play_state =
      ghogx::character::source_char_driver_default_state();
  node_play_state.blend_width = 0.5f;
  const ghogx::character::SourceCharDriverPlayNodeDecision node_missing =
      ghogx::character::source_char_driver_play_node_decision(
          node_play_state, false, false, 5, -1.0f, 2.0f, 0.25f);
  ok &= expect_driver_play_node_decision(node_missing, false,
                                         "missing clip");
  ok &= expect_driver_play_decision(node_missing.clip_play, false, true, false,
                                    false, false, false, 0.0f,
                                    "missing node clip");
  if (!node_play_state.last_node_valid || node_play_state.has_first) {
    std::cerr << "driver Play(DataNode) missing clip did not restore request\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverPlayNodeDecision node_found =
      ghogx::character::source_char_driver_play_node_decision(
          node_play_state, true, false, 6, -1.0f, 4.0f, 0.5f);
  ok &= expect_driver_play_node_decision(node_found, true, "found clip");
  ok &= expect_driver_play_decision(node_found.clip_play, true, false, true,
                                    false, true, true, 0.5f,
                                    "found node clip");
  if (!node_play_state.last_node_valid || !node_play_state.has_first) {
    std::cerr << "driver Play(DataNode) found clip state mismatch\n";
    ok = false;
  }

  ghogx::character::SourceCharDriverPollDeps deps;
  ghogx::character::source_char_driver_poll_deps(deps, "bone.servo");
  if (!deps.changed_by.empty() || deps.change.size() != 1 ||
      deps.change[0] != "bone.servo") {
    std::cerr << "driver PollDeps did not publish bones as change target\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverRuntimeDumpEvidence runtime_dump =
      ghogx::character::source_char_driver_runtime_dump_evidence();
  if (runtime_dump.play_if_safe_range != "0x8034D8A4 -> 0x8034DB54" ||
      runtime_dump.set_beat_scale_range != "0x8034DBB4 -> 0x8034DC4C" ||
      runtime_dump.evaluate_flags_range != "0x8034DC4C -> 0x8034DD64" ||
      runtime_dump.last_range != "0x8034DD64 -> 0x8034DD88" ||
      runtime_dump.before_range != "0x8034DD88 -> 0x8034DDAC" ||
      runtime_dump.most_playing_range != "0x8034DDD4 -> 0x8034DF00" ||
      runtime_dump.pre_load_range != "0x8034E0E0 -> 0x8034ED68" ||
      runtime_dump.post_load_range != "0x8034ED68 -> 0x8034F008" ||
      runtime_dump.play_if_safe_locals !=
          std::vector<std::string>({"d", "FindRestrictLength", "s"}) ||
      runtime_dump.play_if_safe_references !=
          std::vector<std::string>({"TheDebug", "kAssertStr"}) ||
      runtime_dump.set_beat_scale_locals !=
          std::vector<std::string>({"fp", "invScale", "cd"}) ||
      !runtime_dump.set_beat_scale_references.empty() ||
      runtime_dump.evaluate_flags_locals !=
          std::vector<std::string>({"weight", "flagWeight", "cd", "w"}) ||
      runtime_dump.evaluate_flags_references !=
          std::vector<std::string>({"TheDebug", "kAssertStr"}) ||
      runtime_dump.last_locals != std::vector<std::string>({"cd"}) ||
      runtime_dump.before_locals != std::vector<std::string>({"cd"}) ||
      runtime_dump.most_playing_locals !=
          std::vector<std::string>({"maxWeight", "best", "weight", "cd",
                                    "w"}) ||
      runtime_dump.most_playing_references !=
          std::vector<std::string>({"TheDebug", "kAssertStr"}) ||
      runtime_dump.pre_load_locals !=
          std::vector<std::string>({"tmp", "p"}) ||
      runtime_dump.pre_load_references !=
          std::vector<std::string>(
              {"__vt__8FilePath", "__RTTI__6Loader", "__RTTI__9DirLoader",
               "msg", "__vt__7Message", "__RTTI__Q23Hmx6Object",
               "__vt__32ObjPtr<11CharClipSet,9ObjectDir>",
               "__RTTI__9ObjectDir", "__RTTI__11CharClipSet", "TheLoadMgr",
               "sRoot", "sClipsPath", "TheDebug", "gRev"}) ||
      runtime_dump.post_load_references !=
          std::vector<std::string>(
              {"__RTTI__Q23Hmx6Object", "__RTTI__8CharClip", "gRev",
               "__RTTI__9ObjectDir", "__RTTI__11CharClipSet",
               "TheLoadMgr"}) ||
      runtime_dump.header_declarations_without_checked_bodies !=
          std::vector<std::string>(
              {"Handle", "SyncProperty", "Save", "Copy", "Load", "Poll",
               "Replace", "EvaluateFlags", "Display", "FindClip",
               "FirstClip", "FirstPlayingClip"}) ||
      runtime_dump.rb3_latest_has_poll_body ||
      runtime_dump.rb2_dump_has_poll_range ||
      runtime_dump.has_evaluate_flags_statement_body ||
      runtime_dump.has_set_beat_scale_statement_body ||
      runtime_dump.safe_to_find_clip || runtime_dump.safe_to_display ||
      runtime_dump.safe_to_evaluate_flags || runtime_dump.safe_to_import_poll) {
    std::cerr << "driver RB2 runtime dump evidence mismatch\n";
    ok = false;
  }
  return ok;
}

bool expect_driver_midi_helpers() {
  bool ok = true;
  ghogx::character::SourceCharDriverMidiState midi =
      ghogx::character::source_char_driver_midi_default_state();
  if (midi.unk89 || midi.clip_flags != 0 || midi.blend_override_pct != 1.0f ||
      midi.has_default_clip || !midi.parser.empty() ||
      !midi.flag_parser.empty()) {
    std::cerr << "driver MIDI default state mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverMidiLoadPlan load_v2 =
      ghogx::character::source_char_driver_midi_load_plan(2);
  if (!load_v2.known_revision || load_v2.read_order.size() != 4 ||
      load_v2.read_order[0] != "LOAD_REVS" ||
      load_v2.read_order[1] != "CharDriver" ||
      load_v2.read_order[2] != "mDefaultClip.Load(false,mClips)" ||
      load_v2.read_order[3] != "legacyString") {
    std::cerr << "driver MIDI rev2 load plan mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverMidiLoadPlan load_v6 =
      ghogx::character::source_char_driver_midi_load_plan(6);
  if (!load_v6.known_revision || load_v6.read_order.size() != 6 ||
      load_v6.read_order[2] != "mDefaultClip.Load(false,mClips)" ||
      load_v6.read_order[3] != "mParser" ||
      load_v6.read_order[4] != "mFlagParser" ||
      load_v6.read_order[5] != "mBlendOverridePct") {
    std::cerr << "driver MIDI rev6 load plan mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverMidiLoadPlan load_v7 =
      ghogx::character::source_char_driver_midi_load_plan(7);
  if (!load_v7.known_revision || load_v7.read_order.size() != 5 ||
      load_v7.read_order[2] != "mParser" ||
      load_v7.read_order[3] != "mFlagParser" ||
      load_v7.read_order[4] != "mBlendOverridePct") {
    std::cerr << "driver MIDI rev7 load plan mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverMidiLoadPlan load_bad =
      ghogx::character::source_char_driver_midi_load_plan(8);
  if (load_bad.known_revision || !load_bad.read_order.empty()) {
    std::cerr << "driver MIDI bad revision load plan mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverMidiHandlerPlan handler_plan =
      ghogx::character::source_char_driver_midi_handler_plan();
  if (handler_plan.handlers.size() != 3 ||
      handler_plan.handlers[0] != "midi_parser:OnMidiParser" ||
      handler_plan.handlers[1] !=
          "midi_parser_group:OnMidiParserGroup" ||
      handler_plan.handlers[2] !=
          "midi_parser_flags:OnMidiParserFlags" ||
      handler_plan.superclasses.size() != 1 ||
      handler_plan.superclasses[0] != "CharDriver" ||
      handler_plan.check != 0x99) {
    std::cerr << "driver MIDI handler plan mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverMidiPropSyncPlan prop_plan =
      ghogx::character::source_char_driver_midi_prop_sync_plan();
  if (prop_plan.properties.size() != 3 ||
      prop_plan.properties[0] != "parser" ||
      prop_plan.properties[1] != "flag_parser" ||
      prop_plan.properties[2] != "blend_override_pct" ||
      prop_plan.superclasses.size() != 1 ||
      prop_plan.superclasses[0] != "CharDriver") {
    std::cerr << "driver MIDI prop-sync plan mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverMidiCopyPlan copy_plan =
      ghogx::character::source_char_driver_midi_copy_plan();
  if (copy_plan.copied_superclasses.size() != 1 ||
      copy_plan.copied_superclasses[0] != "CharDriver" ||
      copy_plan.copied_members.size() != 4 ||
      copy_plan.copied_members[0] != "unk89" ||
      copy_plan.copied_members[1] != "mParser" ||
      copy_plan.copied_members[2] != "mFlagParser" ||
      copy_plan.copied_members[3] != "mBlendOverridePct" ||
      copy_plan.not_in_source_copy_members.size() != 1 ||
      copy_plan.not_in_source_copy_members[0] != "mClipFlags") {
    std::cerr << "driver MIDI copy plan mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverMidiSavePlan save_plan =
      ghogx::character::source_char_driver_midi_save_plan();
  if (save_plan.save_id != 0x58) {
    std::cerr << "driver MIDI save id mismatch\n";
    ok = false;
  }

  midi.parser = "note.parser";
  midi.flag_parser = "flag.parser";
  midi.has_default_clip = true;
  ghogx::character::SourceCharDriverState driver_state =
      ghogx::character::source_char_driver_default_state();
  driver_state.has_first = true;
  driver_state.last_node_valid = true;
  driver_state.has_default_clip = true;
  const ghogx::character::SourceCharDriverMidiEnterDecision enter =
      ghogx::character::source_char_driver_midi_enter(driver_state, midi,
                                                      true, false);
  if (!midi.unk89 || !enter.set_unk89 || !enter.add_parser_sink ||
      enter.add_flag_parser_sink || !enter.driver_enter.clear_stack ||
      driver_state.has_first || !driver_state.last_node_valid) {
    std::cerr << "driver MIDI Enter decision mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverMidiExitDecision exit =
      ghogx::character::source_char_driver_midi_exit(false, true);
  if (!exit.call_driver_exit || exit.remove_parser_sink ||
      !exit.remove_flag_parser_sink) {
    std::cerr << "driver MIDI Exit decision mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverMidiPollPlan poll_plan =
      ghogx::character::source_char_driver_midi_poll_plan();
  ghogx::character::SourceCharDriverPollDeps midi_deps;
  ghogx::character::source_char_driver_midi_poll_deps(midi_deps,
                                                      "midi.bones");
  if (!poll_plan.call_driver_poll || !poll_plan.call_driver_poll_deps ||
      !midi_deps.changed_by.empty() || midi_deps.change.size() != 1 ||
      midi_deps.change[0] != "midi.bones") {
    std::cerr << "driver MIDI Poll/PollDeps delegation mismatch\n";
    ok = false;
  }

  ghogx::character::source_char_driver_midi_on_parser_flags(midi, 0x1234);
  if (midi.clip_flags != 0x1234) {
    std::cerr << "driver MIDI flags message mismatch\n";
    ok = false;
  }

  midi.blend_override_pct = 0.5f;
  const ghogx::character::SourceCharDriverMidiParserDecision parser_normal =
      ghogx::character::source_char_driver_midi_on_parser(
          midi, true, false, 0.8f, 0.0f, 0.0f, 2.0f);
  if (parser_normal.used_default_clip || !parser_normal.request_play ||
      parser_normal.play_flags != 0 ||
      !nearf(parser_normal.requested_blend_width, 0.4f) ||
      !nearf(parser_normal.old_beat, -0.8f) ||
      !nearf(parser_normal.start, 0.0f)) {
    std::cerr << "driver MIDI parser normal blend mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverMidiParserDecision parser_realtime =
      ghogx::character::source_char_driver_midi_on_parser(
          midi, true, true, 99.0f, 12.0f, 10.0f, 3.0f);
  if (!parser_realtime.request_play ||
      !nearf(parser_realtime.requested_blend_width, 3.0f) ||
      !nearf(parser_realtime.old_beat, -6.0f)) {
    std::cerr << "driver MIDI parser real-time blend mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharDriverMidiParserDecision parser_missing =
      ghogx::character::source_char_driver_midi_on_parser(
          midi, false, false, 0.8f, 0.0f, 0.0f, 2.0f);
  if (parser_missing.request_play) {
    std::cerr << "driver MIDI parser played missing clip\n";
    ok = false;
  }

  midi.unk89 = false;
  const ghogx::character::SourceCharDriverMidiParserDecision parser_default =
      ghogx::character::source_char_driver_midi_on_parser(
          midi, false, false, 0.4f, 0.0f, 0.0f, 2.0f);
  if (!parser_default.used_default_clip || !parser_default.request_play ||
      !nearf(parser_default.requested_blend_width, 0.2f)) {
    std::cerr << "driver MIDI parser default-clip branch mismatch\n";
    ok = false;
  }

  midi.unk89 = true;
  const ghogx::character::SourceCharDriverMidiParserDecision group_missing =
      ghogx::character::source_char_driver_midi_on_parser_group(
          midi, false, true, false, 0.5f, 2.0f);
  if (group_missing.request_play || group_missing.call_group_get_clip) {
    std::cerr << "driver MIDI parser group played missing group\n";
    ok = false;
  }
  const ghogx::character::SourceCharDriverMidiParserDecision group_realtime =
      ghogx::character::source_char_driver_midi_on_parser_group(
          midi, true, true, true, 0.5f, 4.0f);
  if (!group_realtime.call_group_get_clip ||
      group_realtime.group_clip_flags != 0x1234 ||
      !group_realtime.request_play ||
      !nearf(group_realtime.requested_blend_width, -2.0f) ||
      !nearf(group_realtime.old_beat, 1.0e30f) ||
      !nearf(group_realtime.assigned_blend_width, 1.0f)) {
    std::cerr << "driver MIDI parser group blend mismatch\n";
    ok = false;
  }
  midi.unk89 = false;
  const ghogx::character::SourceCharDriverMidiParserDecision group_default =
      ghogx::character::source_char_driver_midi_on_parser_group(
          midi, true, false, false, 0.25f, 4.0f);
  if (!group_default.used_default_clip || group_default.call_group_get_clip ||
      !group_default.request_play ||
      !nearf(group_default.assigned_blend_width, 0.125f)) {
    std::cerr << "driver MIDI parser group default-clip branch mismatch\n";
    ok = false;
  }
  return ok;
}

bool expect_clip_group_source_plans() {
  bool ok = true;
  const ghogx::character::SourceCharClipGroupLoadPlan load_v1 =
      ghogx::character::source_char_clip_group_load_plan(1);
  if (!load_v1.known_revision || load_v1.read_flags ||
      load_v1.default_flags != 0 || load_v1.read_order.size() != 4 ||
      load_v1.read_order[0] != "LOAD_REVS" ||
      load_v1.read_order[1] != "Hmx::Object" ||
      load_v1.read_order[2] != "mClips" ||
      load_v1.read_order[3] != "mWhich") {
    std::cerr << "CharClipGroup rev1 load plan mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharClipGroupLoadPlan load_v2 =
      ghogx::character::source_char_clip_group_load_plan(2);
  if (!load_v2.known_revision || !load_v2.read_flags ||
      load_v2.read_order.size() != 5 || load_v2.read_order[4] != "mFlags") {
    std::cerr << "CharClipGroup rev2 load plan mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharClipGroupLoadPlan load_bad =
      ghogx::character::source_char_clip_group_load_plan(3);
  if (load_bad.known_revision || !load_bad.read_order.empty()) {
    std::cerr << "CharClipGroup bad revision load plan mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharClipGroupHandlerPlan handler =
      ghogx::character::source_char_clip_group_handler_plan();
  if (handler.handlers.size() != 8 ||
      handler.handlers[0] != "get_clip:GetClip" ||
      handler.handlers[1] != "delete_remaining:DeleteRemaining" ||
      handler.handlers[2] != "get_size:mClips.size" ||
      handler.handlers[3] != "has_clip:HasClip" ||
      handler.handlers[4] != "find_clip:GetClip(int)" ||
      handler.handlers[5] != "add_clip:AddClip" ||
      handler.handlers[6] != "set_clip_flags:SetClipFlags" ||
      handler.handlers[7] != "randomize_index:RandomizeIndex" ||
      handler.superclasses.size() != 1 ||
      handler.superclasses[0] != "Hmx::Object" || handler.check != 0x179) {
    std::cerr << "CharClipGroup handler plan mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharClipGroupPropSyncPlan props =
      ghogx::character::source_char_clip_group_prop_sync_plan();
  if (props.properties.size() != 2 || props.properties[0] != "clips" ||
      props.properties[1] != "flags") {
    std::cerr << "CharClipGroup prop-sync plan mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharClipGroupSavePlan save_plan =
      ghogx::character::source_char_clip_group_save_plan();
  if (save_plan.save_id != 0x127) {
    std::cerr << "CharClipGroup save id mismatch\n";
    ok = false;
  }
  const ghogx::character::SourceCharClipGroupDeleteRemainingPlan delete_plan =
      ghogx::character::source_char_clip_group_delete_remaining_plan(3, 2);
  if (delete_plan.requested_remaining != 2 ||
      delete_plan.visited_clip_count != 3 ||
      !delete_plan.increments_local_clip_pointer ||
      delete_plan.calls_lock_and_delete || delete_plan.mutates_group) {
    std::cerr << "CharClipGroup DeleteRemaining plan mismatch\n";
    ok = false;
  }
  return ok;
}

bool expect_resource_lookup(
    const ghogx::character::SourceCharClipResourceLookup& got,
    bool has_type_def,
    bool has_resource_array,
    const std::string& resource_name,
    bool found_resource,
    bool warn_no_resource,
    const char* label) {
  bool ok = true;
  if (got.has_type_def != has_type_def ||
      got.has_resource_array != has_resource_array ||
      got.resource_name != resource_name ||
      got.found_resource != found_resource ||
      got.warn_no_resource != warn_no_resource) {
    std::cerr << "resource lookup mismatch for " << label << ": typeDef="
              << got.has_type_def << " resourceArray="
              << got.has_resource_array << " resource='" << got.resource_name
              << "' found=" << got.found_resource
              << " warn=" << got.warn_no_resource << "\n";
    ok = false;
  }
  return ok;
}

bool expect_context_lookup(
    const ghogx::character::SourceCharClipContextLookup& got,
    bool has_type_def,
    bool has_resource_array,
    const std::string& macro_name,
    int context,
    bool reads_macro,
    const char* label) {
  bool ok = true;
  if (got.has_type_def != has_type_def ||
      got.has_resource_array != has_resource_array ||
      got.macro_name != macro_name ||
      got.context != context ||
      got.reads_macro != reads_macro) {
    std::cerr << "context lookup mismatch for " << label << ": typeDef="
              << got.has_type_def << " resourceArray="
              << got.has_resource_array << " macro='" << got.macro_name
              << "' context=" << got.context
              << " readsMacro=" << got.reads_macro << "\n";
    ok = false;
  }
  return ok;
}

bool expect_clip_driver_helpers() {
  bool ok = true;
  const uint32_t masked =
      ghogx::character::source_char_clip_driver_masked_play_flags(
          0x12345678u, 0x0000f623u);
  if (masked != source_mask(0x12345678u, 0x0000f623u)) {
    std::cerr << "clip-driver raw masked flags mismatch\n";
    ok = false;
  }

  const ghogx::character::SourceCharClipDriverState state =
      ghogx::character::source_char_clip_driver_construct(
          0x12345678u, true, true, 0x0000f623u, 0.25f, true);
  if (state.play_flags != source_mask(0x12345678u, 0x0000f623u) ||
      state.blend_width != 0.25f || state.time_scale != 1.0f ||
      state.d_beat != 0.0f || state.advance_beat != 0.0f ||
      !state.has_clip || !state.has_next || state.next_event != -1 ||
      !state.play_multiple_clips) {
    std::cerr << "clip-driver constructor state mismatch\n";
    ok = false;
  }

  if (ghogx::character::source_char_driver_evaluate_flags_from_clip_flags(
          0x00c00000u, 0x00400000u) != 1.0f ||
      ghogx::character::source_char_driver_evaluate_flags_from_clip_flags(
          0x00800000u, 0x00400000u) != 0.0f) {
    std::cerr << "driver EvaluateFlags clip flag helper mismatch\n";
    ok = false;
  }
  ghogx::character::CharClip held_clip;
  held_clip.loaded = true;
  held_clip.frames.resize(1);
  held_clip.flags = 0x00400000u;
  ghogx::character::CharClip release_clip;
  release_clip.loaded = true;
  release_clip.frames.resize(1);
  release_clip.flags = 0x00000000u;
  ghogx::character::CharClipPlayer player;
  player.play(held_clip, ghogx::character::kCharPlayLoop |
                             ghogx::character::kCharPlayNoBlend);
  if (!nearf(player.evaluate_flags(0x00400000u), 1.0f)) {
    std::cerr << "driver EvaluateFlags active clip mismatch\n";
    ok = false;
  }
  player.set_source_driver_blend_width(1.0f);
  player.play(release_clip, ghogx::character::kCharPlayLoop);
  player.advance(0.5f);
  if (!nearf(player.evaluate_flags(0x00400000u), 0.5f)) {
    std::cerr << "driver EvaluateFlags blend mismatch\n";
    ok = false;
  }
  ghogx::character::CharClip outgoing_pose_clip;
  outgoing_pose_clip.loaded = true;
  outgoing_pose_clip.frames.resize(1);
  ghogx::character::ClipChannel outgoing_neck;
  outgoing_neck.type = ghogx::character::ClipChannel::kQuat;
  outgoing_neck.bone_name = "bone_neck.mesh";
  outgoing_neck.quat[2] = 0.70710678f;
  outgoing_neck.quat[3] = 0.70710678f;
  ghogx::character::ClipChannel outgoing_twist;
  outgoing_twist.type = ghogx::character::ClipChannel::kRotZ;
  outgoing_twist.bone_name = "bone_R-upperTwist.mesh";
  outgoing_twist.angle = 1.0f;
  outgoing_pose_clip.frames[0] = {outgoing_neck, outgoing_twist};

  ghogx::character::CharClip incoming_pose_clip;
  incoming_pose_clip.loaded = true;
  incoming_pose_clip.frames.resize(1);
  ghogx::character::ClipChannel incoming_neck;
  incoming_neck.type = ghogx::character::ClipChannel::kQuat;
  incoming_neck.bone_name = "bone_head.mesh";
  incoming_neck.quat[2] = 0.70710678f;
  incoming_neck.quat[3] = 0.70710678f;
  ghogx::character::ClipChannel incoming_twist;
  incoming_twist.type = ghogx::character::ClipChannel::kRotX;
  incoming_twist.bone_name = "bone_L-foreTwist.mesh";
  incoming_twist.angle = 0.8f;
  ghogx::character::ClipChannel incoming_pelvis;
  incoming_pelvis.type = ghogx::character::ClipChannel::kPos;
  incoming_pelvis.bone_name = "bone_pelvis.mesh";
  incoming_pelvis.pos[0] = 1.0f;
  incoming_pelvis.pos[1] = 2.0f;
  incoming_pelvis.pos[2] = 3.0f;
  const std::vector<std::string> lower_body_overlay_bones = {
      "bone_facing.mesh",   "bone_pelvis.mesh",  "bone_L-thigh.mesh",
      "bone_L-knee.mesh",  "bone_L-ankle.mesh", "bone_L-foot.mesh",
      "bone_L-toe.mesh",   "bone_R-thigh.mesh", "bone_R-knee.mesh",
      "bone_R-ankle.mesh", "bone_R-foot.mesh",  "bone_R-toe.mesh"};
  auto make_lower_body_channel =
      [](const std::string& bone_name, float value) {
        ghogx::character::ClipChannel channel;
        channel.type = ghogx::character::ClipChannel::kPos;
        channel.bone_name = bone_name;
        channel.pos[0] = value;
        return channel;
      };
  auto append_lower_body_channels =
      [&](std::vector<ghogx::character::ClipChannel>& channels) {
        for (size_t i = 0; i < lower_body_overlay_bones.size(); ++i) {
          channels.push_back(make_lower_body_channel(
              lower_body_overlay_bones[i], static_cast<float>(i + 1)));
        }
      };
  incoming_pose_clip.frames[0] = {incoming_neck, incoming_twist,
                                  incoming_pelvis};
  append_lower_body_channels(incoming_pose_clip.frames[0]);

  ghogx::character::CharClipPlayer transition_player;
  transition_player.play(outgoing_pose_clip,
                         ghogx::character::kCharPlayLoop |
                             ghogx::character::kCharPlayNoBlend);
  transition_player.set_source_driver_blend_width(1.0f);
  transition_player.play(incoming_pose_clip, ghogx::character::kCharPlayLoop);
  transition_player.advance(0.5f);
  const std::vector<ghogx::character::ClipChannel> transition_pose =
      transition_player.sampled_pose();
  const ghogx::character::ClipChannel* faded_out_neck = find_pose_channel(
      transition_pose, ghogx::character::ClipChannel::kQuat,
      "bone_neck.mesh");
  const ghogx::character::ClipChannel* faded_in_neck = find_pose_channel(
      transition_pose, ghogx::character::ClipChannel::kQuat,
      "bone_head.mesh");
  const ghogx::character::ClipChannel* faded_out_twist = find_pose_channel(
      transition_pose, ghogx::character::ClipChannel::kRotZ,
      "bone_R-upperTwist.mesh");
  const ghogx::character::ClipChannel* faded_in_twist = find_pose_channel(
      transition_pose, ghogx::character::ClipChannel::kRotX,
      "bone_L-foreTwist.mesh");
  if (!faded_out_neck || !faded_in_neck || !faded_out_twist ||
      !faded_in_twist) {
    std::cerr << "transition-only channel blend dropped a pose row\n";
    ok = false;
  } else {
    if (!nearf(faded_out_neck->quat[2], 0.38268343f) ||
        !nearf(faded_out_neck->quat[3], 0.9238795f) ||
        !nearf(faded_in_neck->quat[2], 0.38268343f) ||
        !nearf(faded_in_neck->quat[3], 0.9238795f)) {
      std::cerr << "transition-only quat channel did not fade through identity\n";
      ok = false;
    }
    if (!nearf(faded_out_twist->angle, 0.5f) ||
        !nearf(faded_in_twist->angle, 0.4f)) {
      std::cerr << "transition-only twist channel did not fade through identity\n";
      ok = false;
    }
  }

  constexpr float kPi = 3.14159265358979323846f;
  if (!nearf(ghogx::character::source_grim_char_bones_samples_pose_axis_angle(
                 ghogx::character::ClipChannel::kRotZ, 0.5f),
             kPi * 0.5f) ||
      !nearf(ghogx::character::source_grim_char_bones_samples_pose_axis_angle(
                 ghogx::character::ClipChannel::kRotX, 0.5f),
             0.5f)) {
    std::cerr << "Grim pose axis angle helper mismatch\n";
    ok = false;
  }
  ghogx::character::Character rotz_character;
  ghogx::milo_scene::TransObj rotz_bone;
  rotz_bone.name = "bone_pelvis.mesh";
  rotz_character.bones.push_back(rotz_bone);
  ghogx::character::ClipChannel rotz_channel;
  rotz_channel.type = ghogx::character::ClipChannel::kRotZ;
  rotz_channel.bone_name = "bone_pelvis.mesh";
  rotz_channel.angle = 0.5f;
  ghogx::character::apply_clip_pose_sampled({rotz_channel}, 1.0f,
                                            rotz_character);
  const auto& rotz_local = rotz_character.bones[0].local;
  if (!nearf(rotz_local.rot[0][0], 0.0f) ||
      !nearf(rotz_local.rot[0][1], 1.0f) ||
      !nearf(rotz_local.rot[1][0], -1.0f) ||
      !nearf(rotz_local.rot[1][1], 0.0f)) {
    std::cerr << "Grim rotz pose application did not use PI-scaled angle\n";
    ok = false;
  }
  ghogx::character::Character weighted_rotz_character;
  ghogx::milo_scene::TransObj weighted_rotz_bone;
  weighted_rotz_bone.name = "bone_pelvis.mesh";
  weighted_rotz_character.bones.push_back(weighted_rotz_bone);
  ghogx::character::ClipChannel weighted_rotz_channel = rotz_channel;
  weighted_rotz_channel.source_weight = 0.5f;
  ghogx::character::apply_clip_pose_sampled({weighted_rotz_channel}, 1.0f,
                                            weighted_rotz_character);
  const auto& weighted_rotz_local = weighted_rotz_character.bones[0].local;
  constexpr float kSqrtHalf = 0.70710678f;
  if (!nearf(weighted_rotz_local.rot[0][0], kSqrtHalf) ||
      !nearf(weighted_rotz_local.rot[0][1], kSqrtHalf) ||
      !nearf(weighted_rotz_local.rot[1][0], -kSqrtHalf) ||
      !nearf(weighted_rotz_local.rot[1][1], kSqrtHalf)) {
    std::cerr << "source-weighted Grim rotz pose application ignored channel weight\n";
    ok = false;
  }
  ghogx::character::Character lower_output_character;
  ghogx::milo_scene::TransObj lower_output_toe;
  lower_output_toe.name = "bone_R-toe.mesh";
  lower_output_toe.local.pos[0] = 90.0f;
  lower_output_toe.local.pos[1] = 91.0f;
  lower_output_toe.local.pos[2] = 92.0f;
  lower_output_toe.local.rot[0][0] = 0.0f;
  lower_output_toe.local.rot[0][1] = 1.0f;
  lower_output_toe.local.rot[1][0] = -1.0f;
  lower_output_toe.local.rot[1][1] = 0.0f;
  lower_output_character.bones.push_back(lower_output_toe);
  ghogx::character::CharClip lower_output_clip;
  lower_output_clip.loaded = true;
  lower_output_clip.name = "lower_output_test";
  lower_output_clip.frames.resize(1);
  lower_output_clip.frames[0].push_back(rotz_channel);
  lower_output_clip.frames[0].back().bone_name = "bone_R-toe.mesh";
  ghogx::character::CharClip::OutputBone lower_output_bone;
  lower_output_bone.name = "bone_R-toe.trans";
  lower_output_bone.local.pos[0] = 4.0f;
  lower_output_bone.local.pos[1] = 5.0f;
  lower_output_bone.local.pos[2] = 6.0f;
  lower_output_clip.output_bones.push_back(lower_output_bone);
  ghogx::character::apply_clip_frame(lower_output_clip, 0,
                                     lower_output_character);
  const auto& lower_output_local = lower_output_character.bones[0].local;
  if (!nearf(lower_output_local.pos[0], 4.0f) ||
      !nearf(lower_output_local.pos[1], 5.0f) ||
      !nearf(lower_output_local.pos[2], 6.0f)) {
    std::cerr << "lower-body output bridge did not preserve authored output local position\n";
    ok = false;
  }
  if (!nearf(lower_output_local.rot[0][0], 0.0f) ||
      !nearf(lower_output_local.rot[0][1], 1.0f) ||
      !nearf(lower_output_local.rot[1][0], -1.0f) ||
      !nearf(lower_output_local.rot[1][1], 0.0f)) {
    std::cerr << "lower-body output bridge did not rebuild axis row from authored output graph\n";
    ok = false;
  }

  ghogx::character::Character weighted_lower_output_character;
  ghogx::milo_scene::TransObj weighted_lower_output_toe;
  weighted_lower_output_toe.name = "bone_R-toe.mesh";
  weighted_lower_output_toe.local.pos[0] = 90.0f;
  weighted_lower_output_toe.local.pos[1] = 91.0f;
  weighted_lower_output_toe.local.pos[2] = 92.0f;
  weighted_lower_output_character.bones.push_back(weighted_lower_output_toe);
  ghogx::character::CharClip weighted_lower_output_clip;
  weighted_lower_output_clip.loaded = true;
  weighted_lower_output_clip.name = "weighted_lower_output_test";
  weighted_lower_output_clip.frames.resize(1);
  ghogx::character::ClipChannel weighted_lower_pos;
  weighted_lower_pos.type = ghogx::character::ClipChannel::kPos;
  weighted_lower_pos.bone_name = "bone_R-toe.mesh";
  weighted_lower_pos.pos[0] = 10.0f;
  weighted_lower_pos.pos[1] = 20.0f;
  weighted_lower_pos.pos[2] = 30.0f;
  weighted_lower_output_clip.frames[0].push_back(weighted_lower_pos);
  ghogx::character::CharClip::OutputBone weighted_lower_output_bone;
  weighted_lower_output_bone.name = "bone_R-toe.trans";
  weighted_lower_output_bone.local.pos[0] = 4.0f;
  weighted_lower_output_bone.local.pos[1] = 5.0f;
  weighted_lower_output_bone.local.pos[2] = 6.0f;
  weighted_lower_output_clip.output_bones.push_back(weighted_lower_output_bone);
  ghogx::character::apply_clip_frame_weighted(weighted_lower_output_clip, 0,
                                              0.25f,
                                              weighted_lower_output_character);
  const auto& weighted_lower_output_local =
      weighted_lower_output_character.bones[0].local;
  if (!nearf(weighted_lower_output_local.pos[0], 5.5f) ||
      !nearf(weighted_lower_output_local.pos[1], 8.75f) ||
      !nearf(weighted_lower_output_local.pos[2], 12.0f)) {
    std::cerr << "weighted lower-body output bridge did not blend from authored output local\n";
    ok = false;
  }

  ghogx::character::Character shared_lower_output_character;
  ghogx::milo_scene::TransObj shared_lower_output_toe;
  shared_lower_output_toe.name = "bone_R-toe.mesh";
  shared_lower_output_toe.local.pos[0] = 90.0f;
  shared_lower_output_toe.local.pos[1] = 91.0f;
  shared_lower_output_toe.local.pos[2] = 92.0f;
  shared_lower_output_character.bones.push_back(shared_lower_output_toe);
  ghogx::character::CharClip shared_lower_output_clip;
  shared_lower_output_clip.loaded = true;
  shared_lower_output_clip.name = "shared_lower_output_stack_test";
  shared_lower_output_clip.frames.resize(1);
  shared_lower_output_clip.frames[0].push_back(rotz_channel);
  shared_lower_output_clip.frames[0].back().bone_name = "bone_R-toe.mesh";
  ghogx::character::CharClip::OutputBone shared_lower_output_bone;
  shared_lower_output_bone.name = "bone_R-toe.trans";
  shared_lower_output_bone.local.pos[0] = 4.0f;
  shared_lower_output_bone.local.pos[1] = 5.0f;
  shared_lower_output_bone.local.pos[2] = 6.0f;
  shared_lower_output_clip.output_bones.push_back(shared_lower_output_bone);
  ghogx::character::ClipChannelLayerStack shared_lower_output_stack;
  if (!ghogx::character::append_clip_frame_layer(
          shared_lower_output_stack, shared_lower_output_clip, 0, 0.25f,
          false)) {
    std::cerr << "shared lower-body layer stack did not append source clip\n";
    ok = false;
  }
  const auto shared_lower_output_result =
      ghogx::character::apply_character_pose_stack_frame(
          shared_lower_output_character, &shared_lower_output_stack);
  const auto& shared_lower_output_local =
      shared_lower_output_character.bones[0].local;
  if (!shared_lower_output_result.applied_clip_layers ||
      shared_lower_output_result.applied_layer_count != 1 ||
      !shared_lower_output_result.source_pose_publisher_fenced ||
      !nearf(shared_lower_output_local.pos[0], 4.0f) ||
      !nearf(shared_lower_output_local.pos[1], 5.0f) ||
      !nearf(shared_lower_output_local.pos[2], 6.0f)) {
    std::cerr << "shared lower-body layer stack did not use authored output local\n";
    ok = false;
  }

  ghogx::character::CharClip stack_base_clip;
  stack_base_clip.loaded = true;
  stack_base_clip.frames.resize(30);
  stack_base_clip.flags = 0x00400000u;
  ghogx::character::CharClip stack_transient_clip;
  stack_transient_clip.loaded = true;
  stack_transient_clip.frames.resize(30);
  stack_transient_clip.flags = 0x00000000u;
  ghogx::character::CharClipPlayer stack_player;
  stack_player.play(stack_base_clip,
                    ghogx::character::kCharPlayLoop |
                        ghogx::character::kCharPlayNoBlend);
  stack_player.set_source_driver_blend_width(0.2f);
  stack_player.play(stack_transient_clip, ghogx::character::kCharPlayNoLoop);
  stack_player.advance(0.1f);
  if (!nearf(stack_player.evaluate_flags(0x00400000u), 0.5f) ||
      stack_player.current_clip() != &stack_transient_clip) {
    std::cerr << "non-loop transient stack did not preserve source blend\n";
    ok = false;
  }
  stack_player.advance(1.0f);
  if (stack_player.current_clip() != &stack_base_clip ||
      !nearf(stack_player.evaluate_flags(0x00400000u), 1.0f)) {
    std::cerr << "non-loop transient stack did not exit back to previous clip\n";
    ok = false;
  }

  incoming_pose_clip.name = "incoming.main";
  ghogx::character::CharClip::OutputBone incoming_output;
  incoming_output.name = "bone_L-foreTwist.mesh";
  incoming_pose_clip.output_bones.push_back(incoming_output);
  ghogx::character::ClipChannelLayerStack layer_stack;
  if (!ghogx::character::append_clip_player_layer(
          layer_stack, transition_player, 0.25f, true) ||
      layer_stack.layers.size() != 1 ||
      !nearf(layer_stack.layers[0].weight, 0.25f) ||
      !layer_stack.layers[0].overlay_override ||
      layer_stack.layers[0].debug_name != "incoming.main" ||
      layer_stack.layers[0].output_bones != &incoming_pose_clip.output_bones ||
      has_any_pose_channel(layer_stack.layers[0].channels,
                           ghogx::character::ClipChannel::kPos,
                           lower_body_overlay_bones)) {
    std::cerr << "shared player layer builder kept lower-body rows\n";
    ok = false;
  }

  ghogx::character::CharClip frame_clip;
  frame_clip.loaded = true;
  frame_clip.name = "frame.main";
  frame_clip.relative = true;
  frame_clip.frames.resize(2);
  frame_clip.frames[1].push_back(incoming_twist);
  append_lower_body_channels(frame_clip.frames[1]);
  if (!ghogx::character::append_clip_frame_layer(layer_stack, frame_clip, 99,
                                                 0.75f, false) ||
      layer_stack.layers.size() != 2 ||
      layer_stack.layers[1].debug_name != "frame.main@f1" ||
      !nearf(layer_stack.layers[1].weight, 0.75f) ||
      layer_stack.layers[1].channels[0].bone_name !=
          "bone_L-foreTwist.mesh" ||
      !has_any_pose_channel(layer_stack.layers[1].channels,
                            ghogx::character::ClipChannel::kPos,
                            lower_body_overlay_bones) ||
      layer_stack.layers[1].output_bones != &frame_clip.output_bones ||
      layer_stack.relative) {
    std::cerr << "shared frame layer builder mismatch\n";
    ok = false;
  }
  ghogx::character::ClipChannelLayerStack overlay_frame_stack;
  if (!ghogx::character::append_clip_frame_layer(overlay_frame_stack,
                                                 frame_clip, 99, 0.75f, true) ||
      overlay_frame_stack.layers.size() != 1 ||
      overlay_frame_stack.layers[0].channels.size() != 1 ||
      has_any_pose_channel(overlay_frame_stack.layers[0].channels,
                           ghogx::character::ClipChannel::kPos,
                           lower_body_overlay_bones)) {
    std::cerr << "shared frame overlay kept a lower-body row\n";
    ok = false;
  }
  ghogx::character::CharClip split_clip;
  split_clip.loaded = true;
  split_clip.name = "split.main";
  split_clip.fps = 30;
  split_clip.frames.resize(2);
  split_clip.frames[0].push_back(incoming_twist);
  ghogx::character::ClipChannel split_next_twist = incoming_twist;
  split_next_twist.angle = 1.2f;
  split_clip.frames[1].push_back(split_next_twist);
  ghogx::character::CharClipPlayer split_player;
  split_player.play(split_clip,
                    ghogx::character::kCharPlayLoop |
                        ghogx::character::kCharPlayNoBlend);
  split_player.advance(1.0f / 60.0f);
  ghogx::character::ClipChannelLayerStack split_stack;
  if (!ghogx::character::append_clip_player_layer(split_stack, split_player,
                                                  1.0f, false) ||
      split_stack.layers.size() != 2 ||
      split_stack.layers[0].debug_name != "split.main@f0" ||
      split_stack.layers[1].debug_name != "split.main@f1" ||
      !nearf(split_stack.layers[0].weight, 0.5f) ||
      !nearf(split_stack.layers[1].weight, 0.5f) ||
      split_stack.layers[0].channels[0].angle != incoming_twist.angle ||
      split_stack.layers[1].channels[0].angle != split_next_twist.angle) {
    std::cerr << "shared player layer did not preserve ScaleAddSample split\n";
    ok = false;
  }
  ghogx::character::ClipChannelLayerStack batch_layer_stack;
  const std::vector<ghogx::character::ClipPlayerLayerSource> batch_players = {
      {&transition_player, 0.25f, true},
      {nullptr, 1.0f, false}};
  if (!ghogx::character::append_clip_player_layers(batch_layer_stack,
                                                   batch_players) ||
      batch_layer_stack.layers.size() != 1 ||
      batch_layer_stack.layers[0].debug_name != "incoming.main" ||
      !batch_layer_stack.layers[0].overlay_override ||
      has_any_pose_channel(batch_layer_stack.layers[0].channels,
                           ghogx::character::ClipChannel::kPos,
                           lower_body_overlay_bones)) {
    std::cerr << "shared player layer batch builder mismatch\n";
    ok = false;
  }
  const std::vector<ghogx::character::ClipFrameLayerSource> batch_frames = {
      {&frame_clip, 1, 0.75f, false},
      {nullptr, 0, 1.0f, false}};
  if (!ghogx::character::append_clip_frame_layers(batch_layer_stack,
                                                  batch_frames) ||
      batch_layer_stack.layers.size() != 2 ||
      batch_layer_stack.layers[1].debug_name != "frame.main@f1" ||
      !has_any_pose_channel(batch_layer_stack.layers[1].channels,
                            ghogx::character::ClipChannel::kPos,
                            lower_body_overlay_bones) ||
      batch_layer_stack.relative) {
    std::cerr << "shared frame layer batch builder mismatch\n";
    ok = false;
  }

  ghogx::character::ClipChannelLayerStack performer_player_stack;
  ghogx::character::CharacterPosePlayerLayerSources performer_players;
  performer_players.main = &transition_player;
  performer_players.strum = &transition_player;
  performer_players.fret_extras = {&transition_player};
  performer_players.strum_weight = 0.30f;
  performer_players.fret_weight = 0.60f;
  if (!ghogx::character::append_character_pose_player_layers(
          performer_player_stack, performer_players) ||
      performer_player_stack.layers.size() != 3 ||
      performer_player_stack.layers[0].overlay_override ||
      !nearf(performer_player_stack.layers[0].weight, 1.0f) ||
      !performer_player_stack.layers[1].overlay_override ||
      !nearf(performer_player_stack.layers[1].weight, 0.30f) ||
      !performer_player_stack.layers[2].overlay_override ||
      !nearf(performer_player_stack.layers[2].weight, 0.60f) ||
      has_any_pose_channel(performer_player_stack.layers[1].channels,
                           ghogx::character::ClipChannel::kPos,
                           lower_body_overlay_bones) ||
      has_any_pose_channel(performer_player_stack.layers[2].channels,
                           ghogx::character::ClipChannel::kPos,
                           lower_body_overlay_bones)) {
    std::cerr << "shared performer player layer helper mismatch\n";
    ok = false;
  }

  ghogx::character::ClipChannelLayerStack performer_frame_stack;
  ghogx::character::CharacterPoseFrameLayerSources performer_frames;
  performer_frames.main = &frame_clip;
  performer_frames.strum = &frame_clip;
  performer_frames.face = &frame_clip;
  performer_frames.frame_idx = 1;
  performer_frames.strum_weight = 0.35f;
  if (!ghogx::character::append_character_pose_frame_layers(
          performer_frame_stack, performer_frames) ||
      performer_frame_stack.layers.size() != 3 ||
      performer_frame_stack.layers[0].overlay_override ||
      !performer_frame_stack.layers[1].overlay_override ||
      !nearf(performer_frame_stack.layers[1].weight, 0.35f) ||
      has_any_pose_channel(performer_frame_stack.layers[1].channels,
                           ghogx::character::ClipChannel::kPos,
                           lower_body_overlay_bones) ||
      performer_frame_stack.layers[2].overlay_override) {
    std::cerr << "shared performer frame layer helper mismatch\n";
    ok = false;
  }

  ghogx::character::Character empty_character;
  ghogx::character::ClipChannelLayerStack empty_layer_stack;
  ghogx::character::apply_clip_layer_stack(empty_layer_stack, empty_character);

  ghogx::character::Character empty_stack_frame_character;
  empty_stack_frame_character.runtime_world_overrides["stale.mesh"] =
      {1.0f, 0.0f, 0.0f, 0.0f,
       0.0f, 1.0f, 0.0f, 0.0f,
       0.0f, 0.0f, 1.0f, 0.0f,
       1.0f, 2.0f, 3.0f, 1.0f};
  const auto empty_stack_frame_result =
      ghogx::character::apply_character_pose_stack_frame(
          empty_stack_frame_character, &empty_layer_stack);
  if (empty_stack_frame_result.applied_clip_layers ||
      empty_stack_frame_result.applied_layer_count != 0 ||
      empty_stack_frame_result.source_pose_publisher_fenced ||
      !empty_stack_frame_character.runtime_world_overrides.empty()) {
    std::cerr << "shared empty pose-stack frame helper mismatch\n";
    ok = false;
  }

  ghogx::character::Character null_stack_frame_character;
  null_stack_frame_character.runtime_world_overrides["stale.mesh"] =
      {1.0f, 0.0f, 0.0f, 0.0f,
       0.0f, 1.0f, 0.0f, 0.0f,
       0.0f, 0.0f, 1.0f, 0.0f,
       4.0f, 5.0f, 6.0f, 1.0f};
  const auto null_stack_frame_result =
      ghogx::character::apply_character_pose_stack_frame(
          null_stack_frame_character, nullptr);
  if (null_stack_frame_result.applied_clip_layers ||
      null_stack_frame_result.applied_layer_count != 0 ||
      null_stack_frame_result.source_pose_publisher_fenced ||
      !null_stack_frame_character.runtime_world_overrides.empty()) {
    std::cerr << "shared null pose-stack frame helper mismatch\n";
    ok = false;
  }

  ghogx::character::Character stack_frame_character;
  stack_frame_character.runtime_world_overrides["stale.mesh"] =
      {1.0f, 0.0f, 0.0f, 0.0f,
       0.0f, 1.0f, 0.0f, 0.0f,
       0.0f, 0.0f, 1.0f, 0.0f,
       7.0f, 8.0f, 9.0f, 1.0f};
  const auto stack_frame_result =
      ghogx::character::apply_character_pose_stack_frame(
          stack_frame_character, &performer_frame_stack);
  if (!stack_frame_result.applied_clip_layers ||
      stack_frame_result.applied_layer_count != performer_frame_stack.layers.size() ||
      !stack_frame_result.source_pose_publisher_fenced ||
      !stack_frame_character.runtime_world_overrides.empty()) {
    std::cerr << "shared populated pose-stack frame helper mismatch\n";
    ok = false;
  }

  ghogx::character::Character controller_character;
  controller_character.runtime_world_overrides["stale.mesh"] =
      {1.0f, 0.0f, 0.0f, 0.0f,
       0.0f, 1.0f, 0.0f, 0.0f,
       0.0f, 0.0f, 1.0f, 0.0f,
       4.0f, 5.0f, 6.0f, 1.0f};
  controller_character.runtime_weight_props["old.weight"] = 0.9f;
  controller_character.runtime_driver_flag_weights["old.drv"][0x10u] = 0.9f;
  ghogx::character::SourceCharMainDriverHandWeights driver_weights;
  driver_weights.driver_flags.push_back({"main.drv", 0x00400000u, 0.25f});
  ghogx::character::CharacterPoseControllerFrameSources controller_sources;
  controller_sources.pose_stack = &performer_frame_stack;
  controller_sources.driver_weights = &driver_weights;
  controller_sources.fallback_ik_weights = {
      {"left.weight", 0.60f},
      {"", 0.75f},
      {"right.weight", 1.50f}};
  controller_sources.time_seconds = 1.25f;
  const auto controller_result =
      ghogx::character::apply_character_pose_controller_frame(
          controller_character, controller_sources);
  const auto main_driver =
      controller_character.runtime_driver_flag_weights.find("main.drv");
  const auto old_driver =
      controller_character.runtime_driver_flag_weights.find("old.drv");
  const auto left_weight =
      controller_character.runtime_weight_props.find("left.weight");
  const auto right_weight =
      controller_character.runtime_weight_props.find("right.weight");
  const auto old_weight =
      controller_character.runtime_weight_props.find("old.weight");
  if (!controller_result.applied_clip_layers ||
      controller_result.applied_layer_count != performer_frame_stack.layers.size() ||
      !controller_result.source_pose_publisher_fenced ||
      !controller_result.fed_driver_flags ||
      controller_result.fallback_ik_weights != 2 ||
      controller_result.applied_midi_fret_target ||
      !controller_result.applied_controllers ||
      !controller_character.runtime_world_overrides.empty() ||
      old_driver != controller_character.runtime_driver_flag_weights.end() ||
      main_driver == controller_character.runtime_driver_flag_weights.end() ||
      main_driver->second.find(0x00400000u) == main_driver->second.end() ||
      !nearf(main_driver->second[0x00400000u], 0.25f) ||
      old_weight != controller_character.runtime_weight_props.end() ||
      left_weight == controller_character.runtime_weight_props.end() ||
      !nearf(left_weight->second, 0.60f) ||
      right_weight == controller_character.runtime_weight_props.end() ||
      !nearf(right_weight->second, 1.0f)) {
    std::cerr << "shared pose controller frame helper mismatch\n";
    ok = false;
  }

  ghogx::character::Character disabled_controller_character;
  disabled_controller_character.runtime_world_overrides["stale.mesh"] =
      {1.0f, 0.0f, 0.0f, 0.0f,
       0.0f, 1.0f, 0.0f, 0.0f,
       0.0f, 0.0f, 1.0f, 0.0f,
       1.0f, 2.0f, 3.0f, 1.0f};
  disabled_controller_character.runtime_weight_props["keep.weight"] = 0.4f;
  ghogx::character::CharacterPoseControllerFrameSources disabled_sources;
  disabled_sources.controllers_enabled = false;
  const auto disabled_result =
      ghogx::character::apply_character_pose_controller_frame(
          disabled_controller_character, disabled_sources);
  if (disabled_result.applied_clip_layers ||
      disabled_result.applied_layer_count != 0 ||
      disabled_result.source_pose_publisher_fenced ||
      disabled_result.fed_driver_flags ||
      disabled_result.fallback_ik_weights != 0 ||
      disabled_result.applied_midi_fret_target ||
      disabled_result.applied_controllers ||
      !disabled_controller_character.runtime_world_overrides.empty() ||
      disabled_controller_character.runtime_weight_props.find("keep.weight") ==
          disabled_controller_character.runtime_weight_props.end()) {
    std::cerr << "disabled shared pose controller frame helper mismatch\n";
    ok = false;
  }

  ok &= expect_indices(
      ghogx::character::source_char_clip_driver_delete_stack_order(3),
      {2, 1, 0}, "DeleteStack tail-first order");

  ghogx::character::SourceCharClipDriverExitDecision exit_all =
      ghogx::character::source_char_clip_driver_exit_decision(3, true, true);
  if (!exit_all.recurse_next || !exit_all.execute_exit_event ||
      !exit_all.end_sync_anim || !exit_all.delete_self ||
      exit_all.returned_stack_head ||
      !expect_indices(exit_all.deleted_indices, {2, 1, 0},
                      "Exit(true) deleted order")) {
    std::cerr << "clip-driver Exit(true) decision mismatch\n";
    ok = false;
  }

  ghogx::character::SourceCharClipDriverExitDecision exit_self =
      ghogx::character::source_char_clip_driver_exit_decision(3, false, false);
  if (exit_self.recurse_next || !exit_self.execute_exit_event ||
      exit_self.end_sync_anim || !exit_self.delete_self ||
      !exit_self.returned_stack_head || *exit_self.returned_stack_head != 1 ||
      !expect_indices(exit_self.deleted_indices, {0},
                      "Exit(false) deleted order")) {
    std::cerr << "clip-driver Exit(false) decision mismatch\n";
    ok = false;
  }

  ghogx::character::SourceCharClipDriverDeleteClipResult delete_mid =
      ghogx::character::source_char_clip_driver_delete_clip_result(
          {false, true, true});
  if (!delete_mid.deleted_index || *delete_mid.deleted_index != 1 ||
      !expect_indices(delete_mid.remaining_indices, {0, 2},
                      "DeleteClip first match remains")) {
    std::cerr << "clip-driver DeleteClip first-match mismatch\n";
    ok = false;
  }

  ghogx::character::SourceCharClipDriverDeleteClipResult delete_none =
      ghogx::character::source_char_clip_driver_delete_clip_result(
          {false, false});
  if (delete_none.deleted_index ||
      !expect_indices(delete_none.remaining_indices, {0, 1},
                      "DeleteClip no match remains")) {
    std::cerr << "clip-driver DeleteClip no-match mismatch\n";
    ok = false;
  }

  if (ghogx::character::source_char_clip_driver_should_execute_event(
          true, true)) {
    std::cerr << "clip-driver ExecuteEvent accepted null symbol\n";
    ok = false;
  }
  if (ghogx::character::source_char_clip_driver_should_execute_event(
          false, false)) {
    std::cerr << "clip-driver ExecuteEvent accepted missing TypeDef\n";
    ok = false;
  }
  if (!ghogx::character::source_char_clip_driver_should_execute_event(
          false, true)) {
    std::cerr << "clip-driver ExecuteEvent rejected source-valid event\n";
    ok = false;
  }

  const ghogx::character::SourceCharClipDriverRuntimeDumpEvidence dump =
      ghogx::character::source_char_clip_driver_runtime_dump_evidence();
  if (dump.copy_ctor_range != "0x8032D060 -> 0x8032D168" ||
      dump.destructor_range != "0x8032D168 -> 0x8032D1E8" ||
      dump.exit_range != "0x8032D1E8 -> 0x8032D28C" ||
      dump.delete_stack_range != "0x8032D28C -> 0x8032D2D4" ||
      dump.delete_clip_range != "0x8032D2D4 -> 0x8032D33C" ||
      dump.evaluate_range != "0x8032D33C -> 0x8032DA1C" ||
      dump.scale_add_range != "0x8032DA1C -> 0x8032DB3C" ||
      dump.rotate_to_range != "0x8032DB3C -> 0x8032DC90" ||
      dump.align_to_frame_range != "0x8032DC90 -> 0x8032DDD0" ||
      dump.play_events_range != "0x8032DDD0 -> 0x8032DFB4" ||
      dump.execute_event_range != "0x8032DFB4 -> 0x8032E290" ||
      dump.copy_ctor_references != std::vector<std::string>(
          {"__vt__33ObjOwnerPtr<8CharClip,9ObjectDir>"}) ||
      dump.destructor_references != std::vector<std::string>(
          {"__vt__33ObjOwnerPtr<8CharClip,9ObjectDir>"}) ||
      dump.exit_locals !=
          std::vector<std::string>({"CharClipDriver* next r31"}) ||
      dump.exit_references !=
          std::vector<std::string>({"static Symbol exit"}) ||
      dump.evaluate_locals !=
          std::vector<std::string>({"nextWeight", "rt", "ut", "rampDelta",
                                    "oldFrame", "delta", "dfrac", "length",
                                    "w"}) ||
      dump.evaluate_references !=
          std::vector<std::string>({"Debug TheDebug",
                                    "const char * kAssertStr"}) ||
      dump.scale_add_locals != std::vector<std::string>({"w"}) ||
      dump.scale_add_references !=
          std::vector<std::string>({"Debug TheDebug",
                                    "const char * kAssertStr"}) ||
      dump.rotate_to_locals != std::vector<std::string>({"w"}) ||
      dump.rotate_to_references !=
          std::vector<std::string>({"Debug TheDebug",
                                    "const char * kAssertStr"}) ||
      dump.align_to_frame_locals !=
          std::vector<std::string>({"alignBeat", "delta"}) ||
      dump.align_to_frame_references !=
          std::vector<std::string>({"Debug TheDebug",
                                    "const char * kAssertStr"}) ||
      dump.play_events_locals != std::vector<std::string>({"frame"}) ||
      dump.play_events_references != std::vector<std::string>(
          {"Debug TheDebug", "const char * kAssertStr",
           "static DataNode& instant", "static Symbol enter"}) ||
      dump.execute_event_references != std::vector<std::string>(
          {"static Message h", "__vt__7Message", "static DataNode& dude",
           "Debug TheDebug", "const char * kAssertStr",
           "const char * gNullStr"}) ||
      dump.has_evaluate_statement_body || dump.has_scale_add_statement_body ||
      dump.has_rotate_to_statement_body || dump.safe_to_import_runtime) {
    std::cerr << "clip-driver RB2 runtime dump evidence mismatch\n";
    ok = false;
  }
  return ok;
}

bool expect_blend(float requested, float driver, float want,
                  const char* label) {
  const float got =
      ghogx::character::source_char_driver_resolve_blend_width(requested,
                                                               driver);
  if (got == want) return true;
  std::cerr << "blend fallback mismatch for " << label << ": got " << got
            << " want " << want << "\n";
  return false;
}

bool expect_should_start(bool play_multiple, bool already_playing, bool want,
                         const char* label) {
  const bool got = ghogx::character::source_char_driver_should_start_clip(
      play_multiple, already_playing);
  if (got == want) return true;
  std::cerr << "duplicate gate mismatch for " << label << ": got " << got
            << " want " << want << "\n";
  return false;
}

bool expect_first_playing(const std::vector<float>& blend_fracs,
                          std::optional<size_t> want, const char* label) {
  const std::optional<size_t> got =
      ghogx::character::source_char_driver_first_playing_index(blend_fracs);
  if (got == want) return true;
  std::cerr << "first-playing mismatch for " << label << ": got ";
  if (got) {
    std::cerr << *got;
  } else {
    std::cerr << "<none>";
  }
  std::cerr << " want ";
  if (want) {
    std::cerr << *want;
  } else {
    std::cerr << "<none>";
  }
  std::cerr << "\n";
  return false;
}

}  // namespace

int main() {
  bool ok = true;
  ok &= expect_flags(0x12345678u, 0x00000000u, "default");
  ok &= expect_flags(0x12345678u, ghogx::character::kCharPlayNoBlend,
                     "low mode");
  ok &= expect_flags(0x12345678u, ghogx::character::kCharPlayLoop,
                     "loop mode");
  ok &= expect_flags(0x12345678u, ghogx::character::kCharPlayRealTime |
                                       ghogx::character::kCharPlayUserTime,
                     "time bits");
  ok &= expect_flags(0x12345678u, 0x0000f623u, "all source groups");
  ok &= expect_group_duplicates({0x11u, 0x12u, 0x21u, 0x31u}, 0, 0x0fu, 2,
                                "low flag duplicates");
  ok &= expect_group_duplicates({0x11u, 0x12u, 0x21u, 0x31u}, 2, 0xf0u, 0,
                                "high flag unique");
  ok &= expect_group_duplicates({0x11u, 0x12u, 0x21u, 0x31u}, 9, 0x0fu, 0,
                                "invalid source index");
  ok &= expect_group_sort({"z_idle", "A_intro", "mid"}, {"A_intro", "mid", "z_idle"},
                          "alphabetical source order");
  ok &= expect_group_add({"idle", "solo"}, "ending",
                         {"idle", "solo", "ending"},
                         "append absent clip");
  ok &= expect_group_add({"idle", "solo"}, "solo", {"idle", "solo"},
                         "ignore duplicate clip");
  ok &= expect_group_remove({"idle", "solo", "solo", "ending"}, "solo",
                            {"idle", "solo", "ending"},
                            "source iterator skip after non-match");
  ok &= expect_group_remove({"solo", "solo", "ending"}, "solo",
                            {"solo", "ending"},
                            "source iterator skip after erase");
  ok &= expect_clip_group_source_plans();
  ok &= expect_clip_driver_helpers();
  ok &= expect_driver_midi_helpers();
  ok &= expect_starved(false, false, 0, true, "empty stack");
  ok &= expect_starved(true, true, ghogx::character::kCharPlayLoop, false,
                       "stack has next");
  ok &= expect_starved(true, false, ghogx::character::kCharPlayNoLoop, false,
                       "single no-loop clip");
  ok &= expect_starved(true, false, ghogx::character::kCharPlayLoop, true,
                       "single looping clip");
  ok &= expect_beat_align(0, "NoAlign", "default align");
  ok &= expect_beat_align(ghogx::character::kCharPlayRealTime, "RealTime",
                          "real-time align");
  ok &= expect_beat_align(ghogx::character::kCharPlayUserTime, "UserTime",
                          "user-time align");
  ok &= expect_beat_align(0x1000u, "BeatAlign1", "beat align 1");
  ok &= expect_beat_align(0x2000u, "BeatAlign2", "beat align 2");
  ok &= expect_beat_align(0x4000u, "BeatAlign4", "beat align 4");
  ok &= expect_beat_align(0x8000u, "BeatAlign8", "beat align 8");
  ok &= expect_beat_align(0xF623u, "NoAlign", "masked unknown align");
  ok &= expect_flag_update(
      ghogx::character::source_char_clip_set_flags(0x12u, false, 0x12u),
      0x12u, false, false, "SetFlags unchanged clean");
  ok &= expect_flag_update(
      ghogx::character::source_char_clip_set_flags(0x12u, true, 0x12u),
      0x12u, true, false, "SetFlags unchanged dirty");
  ok &= expect_flag_update(
      ghogx::character::source_char_clip_set_flags(0x12u, false, 0x34u),
      0x34u, true, true, "SetFlags changed");
  ok &= expect_flag_update(
      ghogx::character::source_char_clip_set_play_flags(0x20u, false, 0x20u),
      0x20u, false, false, "SetPlayFlags unchanged clean");
  ok &= expect_flag_update(
      ghogx::character::source_char_clip_set_play_flags(0x20u, true, 0x20u),
      0x20u, true, false, "SetPlayFlags unchanged dirty");
  ok &= expect_flag_update(
      ghogx::character::source_char_clip_set_play_flags(0x20u, false, 0x10u),
      0x10u, true, true, "SetPlayFlags changed");
  ok &= expect_shares_groups(
      {{false, {"idle"}}, {true, {"idle", "solo", "ending"}}},
      "solo", true, "candidate in source group owner");
  ok &= expect_shares_groups(
      {{false, {"solo"}}, {true, {"idle", "ending"}}},
      "solo", false, "non-group owner ignored");
  ok &= expect_shares_groups(
      {{true, {"idle"}}, {true, {"ending"}}},
      "solo", false, "candidate absent from groups");
  ok &= expect_default_state(ghogx::character::source_char_clip_default_state());
  ok &= expect_num_frames(
      ghogx::character::source_char_clip_num_frames_plan(12, 4, 99), 12,
      "full sample count wins");
  ok &= expect_num_frames(
      ghogx::character::source_char_clip_num_frames_plan(2, 16, 99), 16,
      "full frame count wins");
  ok &= expect_num_frames(
      ghogx::character::source_char_clip_num_frames_plan(0, 0, 99), 1,
      "minimum frame count");
  ok &= expect_num_frames(
      ghogx::character::source_char_clip_num_frames_plan(2, 3, 1000), 3,
      "one sample count ignored");
  const ghogx::character::SourceCharClipTimingBodyBoundary timing_boundary =
      ghogx::character::source_char_clip_timing_body_boundary();
  if (!timing_boundary.constructor_sets_frames_per_sec_30 ||
      !timing_boundary.inline_start_end_length_beats_present ||
      !timing_boundary.prop_sync_exposes_length_seconds ||
      !timing_boundary.prop_sync_exposes_average_beats_per_sec ||
      timing_boundary.length_seconds_body_visible ||
      timing_boundary.average_beats_per_second_body_visible ||
      timing_boundary.safe_to_import_seconds_math ||
      timing_boundary.fenced_bodies.size() != 2 ||
      timing_boundary.fenced_bodies[0] != "CharClip::LengthSeconds" ||
      timing_boundary.fenced_bodies[1] !=
          "CharClip::AverageBeatsPerSecond") {
    std::cerr << "CharClip timing body boundary mismatch\n";
    ok = false;
  }
  ok &= expect_beat_event(
      ghogx::character::source_char_clip_beat_event_default(), "", 0.0f,
      "BeatEvent default");
  const ghogx::character::SourceCharClipBeatEvent loaded_event =
      ghogx::character::source_char_clip_beat_event_loaded("solo_hit", 12.5f);
  ok &= expect_beat_event(loaded_event, "solo_hit", 12.5f,
                          "BeatEvent load order");
  ok &= expect_beat_event(
      ghogx::character::source_char_clip_beat_event_copy(loaded_event),
      "solo_hit", 12.5f, "BeatEvent copy");
  ghogx::character::SourceCharClipBeatEvent assigned_event =
      ghogx::character::source_char_clip_beat_event_default();
  ghogx::character::source_char_clip_beat_event_assign(assigned_event,
                                                       loaded_event);
  ok &= expect_beat_event(assigned_event, "solo_hit", 12.5f,
                          "BeatEvent assignment");
  const ghogx::character::SourceCharClipPropSyncPlan prop_sync =
      ghogx::character::source_char_clip_prop_sync_plan();
  if (prop_sync.graph_node_properties.size() != 2 ||
      prop_sync.graph_node_properties[0] != "cur_beat" ||
      prop_sync.graph_node_properties[1] != "next_beat" ||
      !prop_sync.node_vector_size_query ||
      prop_sync.node_vector_properties.size() != 2 ||
      prop_sync.node_vector_properties[0] != "clip" ||
      prop_sync.node_vector_properties[1] != "nodes" ||
      prop_sync.beat_event_set_properties.size() != 2 ||
      prop_sync.beat_event_set_properties[0] != "beat" ||
      prop_sync.beat_event_set_properties[1] != "event") {
    std::cerr << "CharClip prop sync nested rows mismatch\n";
    ok = false;
  }
  if (prop_sync.clip_set_properties.size() != 15 ||
      prop_sync.clip_set_properties[0] != "start_beat" ||
      prop_sync.clip_set_properties[6] != "flags" ||
      prop_sync.clip_set_properties[10] != "relative" ||
      prop_sync.clip_set_properties[14] != "num_frames") {
    std::cerr << "CharClip prop sync set rows mismatch\n";
    ok = false;
  }
  if (prop_sync.clip_properties.size() != 5 ||
      prop_sync.clip_properties[0] != "range" ||
      prop_sync.clip_properties[4] != "sync_anim" ||
      prop_sync.sample_subobjects.size() != 2 ||
      prop_sync.sample_subobjects[0] != "full" ||
      prop_sync.sample_subobjects[1] != "one") {
    std::cerr << "CharClip prop sync property/subobject rows mismatch\n";
    ok = false;
  }
  ok &= expect_resource_lookup(
      ghogx::character::source_char_clip_get_resource(true, true,
                                                      "rock1_resource", true),
      true, true, "rock1_resource", true, false, "found resource");
  ok &= expect_resource_lookup(
      ghogx::character::source_char_clip_get_resource(true, true,
                                                      "missing_resource", false),
      true, true, "missing_resource", false, true, "missing resource");
  ok &= expect_resource_lookup(
      ghogx::character::source_char_clip_get_resource(false, true,
                                                      "ignored_resource", true),
      false, false, "", false, true, "missing typedef");
  ok &= expect_context_lookup(
      ghogx::character::source_char_clip_get_context_lookup(
          true, true, "clip_resource_context", 0x27),
      true, true, "clip_resource_context", 0x27, true,
      "found context macro");
  ok &= expect_context_lookup(
      ghogx::character::source_char_clip_get_context_lookup(
          true, false, "ignored_context", 0x27),
      true, false, "", 0, false, "missing resource array");
  ok &= expect_context_lookup(
      ghogx::character::source_char_clip_get_context_lookup(
          false, true, "ignored_context", 0x27),
      false, false, "", 0, false, "missing typedef context");
  if (ghogx::character::source_char_clip_get_context(true, true, 0x27) !=
      0x27) {
    std::cerr << "GetContext resource macro mismatch\n";
    ok = false;
  }
  if (ghogx::character::source_char_clip_get_context(true, false, 0x27) != 0 ||
      ghogx::character::source_char_clip_get_context(false, true, 0x27) != 0) {
    std::cerr << "GetContext missing resource fallback mismatch\n";
    ok = false;
  }
  ghogx::character::SourceCharClipTransitionsState transitions =
      ghogx::character::source_char_clip_transitions_construct(true);
  if (!transitions.has_owner ||
      ghogx::character::source_char_clip_transitions_size(transitions) != 0) {
    std::cerr << "Transitions constructor state mismatch\n";
    ok = false;
  }
  transitions.node_sizes = {2, 4, 1};
  if (ghogx::character::source_char_clip_transitions_size(transitions) != 3) {
    std::cerr << "Transitions Size mismatch\n";
    ok = false;
  }
  const ghogx::character::SourceCharClipTransitionsClearResult clear_result =
      ghogx::character::source_char_clip_transitions_clear(transitions);
  if (clear_result.released_clips != 3 || !clear_result.resized_zero ||
      !transitions.node_sizes.empty()) {
    std::cerr << "Transitions Clear release/resize mismatch\n";
    ok = false;
  }
  const ghogx::character::SourceCharClipTransitionsDumpEvidence
      transitions_dump =
          ghogx::character::source_char_clip_transitions_dump_evidence();
  if (transitions_dump.remove_nodes_range != "0x803286D0 -> 0x80328774" ||
      transitions_dump.resize_nodes_range != "0x80328774 -> 0x803288A4" ||
      transitions_dump.add_node_range != "0x803288A4 -> 0x80328A1C" ||
      !transitions_dump.has_remove_nodes_locals ||
      !transitions_dump.has_resize_nodes_locals ||
      !transitions_dump.has_add_node_locals ||
      transitions_dump.has_statement_bodies) {
    std::cerr << "Transitions dump evidence mismatch\n";
    ok = false;
  }
  const ghogx::character::SourceCharClipRuntimeDumpEvidence clip_dump =
      ghogx::character::source_char_clip_runtime_dump_evidence();
  if (clip_dump.find_nodes_range != "0x80328218 -> 0x80328258" ||
      clip_dump.find_first_node_range != "0x80328258 -> 0x803282D0" ||
      clip_dump.find_last_node_range != "0x803282D0 -> 0x80328348" ||
      clip_dump.find_node_range != "0x80328348 -> 0x80328564" ||
      clip_dump.load_range != "0x80328D70 -> 0x803296AC" ||
      clip_dump.set_default_blend_range != "0x803298AC -> 0x803298DC" ||
      clip_dump.set_default_loop_range != "0x803298DC -> 0x8032990C" ||
      clip_dump.set_beat_align_mode_range != "0x8032990C -> 0x80329944" ||
      clip_dump.in_groups_range != "0x80329944 -> 0x803299FC" ||
      clip_dump.make_mru_range != "0x803299FC -> 0x80329B54" ||
      clip_dump.lock_and_delete_range != "0x80329B54 -> 0x80329C78" ||
      clip_dump.handle_range != "0x80329C78 -> 0x8032A470" ||
      clip_dump.on_groups_range != "0x8032A470 -> 0x8032A5DC" ||
      clip_dump.check_stick_range != "0x8032A5DC -> 0x8032A8B8" ||
      clip_dump.sync_property_range != "0x8032AA84 -> 0x8032B76C" ||
      clip_dump.find_node_locals !=
          std::vector<std::string>({"CharGraphNode* n", "float beatAlign",
                                    "float endBorder", "float f"}) ||
      clip_dump.load_locals.size() != 26 ||
      clip_dump.load_locals[0] != "int num" ||
      clip_dump.load_locals[2] != "char name[256]" ||
      clip_dump.load_locals[17] != "String tmp" ||
      clip_dump.load_locals[21] != "String s" ||
      clip_dump.load_locals[25] != "float frame" ||
      clip_dump.default_flag_setter_locals !=
          std::vector<std::string>({"int f"}) ||
      clip_dump.in_groups_locals !=
          std::vector<std::string>({"int count", "_List_iterator i",
                                    "Object* o"}) ||
      clip_dump.make_mru_locals !=
          std::vector<std::string>({"CharClipGroup* groups[256]", "int num",
                                    "_List_iterator i", "Object* o",
                                    "CharClipGroup* g"}) ||
      clip_dump.lock_and_delete_locals !=
          std::vector<std::string>({"int i", "CharClip* c",
                                    "CharClip* c"}) ||
      clip_dump.on_groups_locals !=
          std::vector<std::string>({"_List_iterator i", "Object* o",
                                    "CharClipGroup* group"}) ||
      clip_dump.check_stick_locals !=
          std::vector<std::string>({"RndTransformable* stick",
                                    "RndTransformable* arm",
                                    "CharBonesMeshes bones",
                                    "Vector3 stickDown", "Vector3 armDown",
                                    "float angle"}) ||
      clip_dump.has_load_statement_body ||
      clip_dump.has_default_flag_setter_statement_bodies ||
      clip_dump.has_group_helper_statement_bodies ||
      clip_dump.has_check_stick_statement_body ||
      clip_dump.has_sync_property_statement_body ||
      clip_dump.safe_to_import_load ||
      clip_dump.safe_to_import_default_flag_setters ||
      clip_dump.safe_to_import_group_helpers ||
      clip_dump.safe_to_import_check_stick ||
      clip_dump.safe_to_import_sync_property) {
    std::cerr << "CharClip runtime dump evidence mismatch\n";
    ok = false;
  }
  const std::vector<ghogx::character::SourceCharBonesBone> stuffed =
      ghogx::character::source_char_clip_stuff_bones(
          {{"preexisting.pos", 0.25f}},
          {{"bone_head.pos", 1.0f}, {"bone_head.rotz", 0.5f}});
  if (stuffed.size() != 3 || stuffed[0].name != "preexisting.pos" ||
      stuffed[1].name != "bone_head.pos" ||
      stuffed[2].name != "bone_head.rotz" ||
      !nearf(stuffed[2].weight, 0.5f)) {
    std::cerr << "CharClip StuffBones append order mismatch\n";
    ok = false;
  }
  const ghogx::character::SourceCharClipPoseMeshesSteps pose_steps =
      ghogx::character::source_char_clip_pose_meshes_steps(14.25f);
  if (pose_steps.temp_meshes_name != "tmp_viseme_bones" ||
      pose_steps.call_order.size() != 6 ||
      pose_steps.call_order[0] != "CharBonesMeshes meshes" ||
      pose_steps.call_order[4] != "ScaleAdd" ||
      !pose_steps.stuff_bones || !pose_steps.scale_down ||
      pose_steps.scale_down_target != "meshes" ||
      !nearf(pose_steps.scale_down_weight, 0.0f) ||
      !pose_steps.scale_add || pose_steps.scale_add_target != "meshes" ||
      !nearf(pose_steps.scale_add_weight, 1.0f) ||
      !nearf(pose_steps.scale_add_frame, 14.25f) ||
      !nearf(pose_steps.scale_add_blend, 0.0f) ||
      pose_steps.pose_meshes_target != "meshes" || !pose_steps.pose_meshes) {
    std::cerr << "CharClip PoseMeshes step mismatch\n";
    ok = false;
  }
  ok &= expect_driver_state_helpers();
  ok &= expect_blend(-1.0f, 1.0f, 1.0f, "source default blend");
  ok &= expect_blend(-1.0f, 0.25f, 0.25f, "custom driver blend");
  ok &= expect_blend(0.0f, 1.0f, 0.0f, "explicit zero blend");
  ok &= expect_blend(-0.5f, 1.0f, -0.5f, "non-sentinel negative blend");
  ok &= expect_should_start(false, true, true, "duplicates allowed default");
  ok &= expect_should_start(true, false, true, "new clip in multi mode");
  ok &= expect_should_start(true, true, false, "duplicate clip in multi mode");
  ok &= expect_first_playing({}, std::nullopt, "empty source stack");
  ok &= expect_first_playing({0.0f, 0.0f}, std::nullopt,
                             "all zero source stack");
  ok &= expect_first_playing({0.5f, 0.0f}, static_cast<size_t>(0),
                             "first source node playing");
  ok &= expect_first_playing({0.0f, 0.25f, 1.0f}, static_cast<size_t>(1),
                             "skip zero blend nodes");
  return ok ? 0 : 1;
}
