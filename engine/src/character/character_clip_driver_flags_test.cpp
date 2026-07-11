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
                                bool want_play, const char* label) {
  const ghogx::character::SourceCharDriverPlayGroupDecision got =
      ghogx::character::source_char_driver_play_group_decision(has_clip_dir,
                                                               found_group);
  bool ok = true;
  if (got.has_clip_dir != has_clip_dir || got.found_group != found_group ||
      got.request_play != want_play) {
    std::cerr << "driver PlayGroup decision mismatch for " << label << "\n";
    ok = false;
  }
  return ok;
}

bool expect_driver_state_helpers() {
  bool ok = true;
  ghogx::character::SourceCharDriverState state =
      ghogx::character::source_char_driver_default_state();
  ok &= expect_driver_default_state(state);

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
  ghogx::character::source_char_driver_transfer(dest, source);
  if (!dest.has_clips || !dest.has_first || !dest.last_node_valid ||
      !dest.realign || dest.beat_scale != 0.5f ||
      dest.blend_width != 0.25f) {
    std::cerr << "driver Transfer copied source fields incorrectly\n";
    ok = false;
  }
  ok &= expect_play_group_decision(false, false, false, "no clip directory");
  ok &= expect_play_group_decision(true, false, false, "missing group");
  ok &= expect_play_group_decision(true, true, true, "group found");

  ghogx::character::SourceCharDriverPollDeps deps;
  ghogx::character::source_char_driver_poll_deps(deps, "bone.servo");
  if (!deps.changed_by.empty() || deps.change.size() != 1 ||
      deps.change[0] != "bone.servo") {
    std::cerr << "driver PollDeps did not publish bones as change target\n";
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
  if (group_missing.request_play) {
    std::cerr << "driver MIDI parser group played missing group\n";
    ok = false;
  }
  const ghogx::character::SourceCharDriverMidiParserDecision group_realtime =
      ghogx::character::source_char_driver_midi_on_parser_group(
          midi, true, true, true, 0.5f, 4.0f);
  if (!group_realtime.request_play ||
      !nearf(group_realtime.requested_blend_width, -2.0f) ||
      !nearf(group_realtime.old_beat, 1.0e30f) ||
      !nearf(group_realtime.assigned_blend_width, 1.0f)) {
    std::cerr << "driver MIDI parser group blend mismatch\n";
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
