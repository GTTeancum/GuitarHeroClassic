#include "character/char_clip.h"

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
  ok &= expect_default_state(ghogx::character::source_char_clip_default_state());
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
