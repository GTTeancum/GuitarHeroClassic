#include "character/char_clip.h"

#include <cmath>
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

}  // namespace

int main() {
  using ghogx::character::SourceCharLipSyncDriverPollDeps;
  using ghogx::character::source_char_lip_sync_default_state;
  using ghogx::character::source_char_lip_sync_driver_default_state;
  using ghogx::character::source_char_lip_sync_driver_poll_deps;
  using ghogx::character::source_char_lip_sync_generator_default_state;
  using ghogx::character::source_char_lip_sync_load_steps;

  bool ok = true;

  const auto generator = source_char_lip_sync_generator_default_state();
  ok &= expect_bool(generator.lip_sync_null, true,
                    "Generator default lip sync null");
  ok &= near(static_cast<float>(generator.last_count), 0.0f,
             "Generator default last count");
  ok &= expect_size(generator.weights.size(), 0,
                    "Generator default weights empty");

  const auto lip_sync = source_char_lip_sync_default_state();
  ok &= expect_string(lip_sync.prop_anim, "", "CharLipSync default prop anim");
  ok &= expect_size(lip_sync.visemes.size(), 0,
                    "CharLipSync default visemes empty");
  ok &= near(static_cast<float>(lip_sync.frames), 0.0f,
             "CharLipSync default frames");
  ok &= expect_size(lip_sync.data.size(), 0, "CharLipSync default data empty");

  auto load = source_char_lip_sync_load_steps(0);
  ok &= expect_bool(load.known_revision, true, "Load rev0 known");
  ok &= expect_bool(load.load_hmx_object, true, "Load Hmx object");
  ok &= expect_bool(load.load_visemes, true, "Load visemes");
  ok &= expect_bool(load.load_frames, true, "Load frames");
  ok &= expect_bool(load.load_data, true, "Load data");
  ok &= expect_bool(load.load_prop_anim, false, "Load rev0 skips prop anim");
  load = source_char_lip_sync_load_steps(1);
  ok &= expect_bool(load.load_prop_anim, true, "Load rev1 reads prop anim");
  load = source_char_lip_sync_load_steps(2);
  ok &= expect_bool(load.known_revision, false, "Load rev2 rejected");

  auto driver = source_char_lip_sync_driver_default_state("lips.driver");
  ok &= near(driver.weightable.weight, 1.0f, "Driver inherited weight");
  ok &= expect_string(driver.weightable.weight_owner, "lips.driver",
                      "Driver weight owner");
  ok &= expect_string(driver.lip_sync, "", "Driver lip sync ref");
  ok &= expect_string(driver.clips, "", "Driver clips ref");
  ok &= expect_string(driver.blink_clip, "", "Driver blink clip ref");
  ok &= expect_string(driver.song_owner, "", "Driver song owner ref");
  ok &= near(driver.song_offset, 0.0f, "Driver song offset");
  ok &= expect_bool(driver.loop, false, "Driver loop default");
  ok &= expect_bool(driver.song_player_null, true, "Driver song player null");
  ok &= expect_string(driver.bones, "", "Driver bones ref");
  ok &= expect_string(driver.test_clip, "", "Driver test clip");
  ok &= near(driver.test_weight, 1.0f, "Driver test weight");
  ok &= expect_string(driver.override_clip, "", "Driver override clip");
  ok &= near(driver.override_weight, 0.0f, "Driver override weight");
  ok &= expect_string(driver.override_options, "", "Driver override options");
  ok &= expect_bool(driver.apply_override_additively, false,
                    "Driver override additive default");
  ok &= expect_string(driver.alternate_driver, "", "Driver alternate driver");

  driver.bones = "tmp_viseme_bones";
  SourceCharLipSyncDriverPollDeps deps;
  source_char_lip_sync_driver_poll_deps(deps, driver);
  ok &= expect_size(deps.changed_by.size(), 0,
                    "Driver PollDeps changed_by empty");
  ok &= expect_size(deps.change.size(), 1, "Driver PollDeps change count");
  ok &= expect_string(deps.change[0], "tmp_viseme_bones",
                      "Driver PollDeps changes bones");

  return ok ? 0 : 1;
}
