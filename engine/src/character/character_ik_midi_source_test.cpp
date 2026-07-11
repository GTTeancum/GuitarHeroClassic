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
  using ghogx::character::SourceCharIKMidiPollDeps;
  using ghogx::character::source_char_ik_midi_copy_plan;
  using ghogx::character::source_char_ik_midi_default_state;
  using ghogx::character::source_char_ik_midi_enter;
  using ghogx::character::source_char_ik_midi_load_steps;
  using ghogx::character::source_char_ik_midi_poll_deps;

  bool ok = true;

  auto midi = source_char_ik_midi_default_state();
  ok &= expect_string(midi.bone, "", "default bone");
  ok &= expect_string(midi.cur_spot, "", "default current spot");
  ok &= expect_string(midi.new_spot, "", "default new spot");
  ok &= expect_bool(midi.spot_changed, false, "default spot changed");
  ok &= expect_bool(midi.local_xfm_reset, true, "default local reset");
  ok &= expect_bool(midi.old_local_xfm_reset, true, "default old local reset");
  ok &= near(midi.frac, 0.0f, "default frac");
  ok &= near(midi.frac_per_beat, 0.0f, "default frac per beat");
  ok &= expect_string(midi.anim_blender, "", "default anim blender");
  ok &= near(midi.max_anim_blend, 1.0f, "default max anim blend");
  ok &= near(midi.anim_frac_per_beat, 0.0f, "default anim frac per beat");
  ok &= near(midi.anim_frac, 0.0f, "default anim frac");

  midi.cur_spot = "spot_old";
  midi.new_spot = "spot_new";
  midi.spot_changed = true;
  midi.frac = 0.75f;
  midi.frac_per_beat = 2.0f;
  midi.local_xfm_reset = false;
  midi.old_local_xfm_reset = false;
  const auto enter = source_char_ik_midi_enter(midi);
  ok &= expect_bool(enter.clear_cur_spot, true, "Enter clear cur spot");
  ok &= expect_bool(enter.clear_new_spot, true, "Enter clear new spot");
  ok &= expect_bool(enter.clear_spot_changed, true, "Enter clear changed");
  ok &= expect_bool(enter.reset_frac, true, "Enter reset frac");
  ok &= expect_bool(enter.reset_frac_per_beat, true,
                    "Enter reset frac per beat");
  ok &= expect_bool(enter.reset_local_xfm, true, "Enter reset local");
  ok &= expect_bool(enter.reset_old_local_xfm, true, "Enter reset old local");
  ok &= expect_bool(enter.call_rnd_pollable_enter, true,
                    "Enter calls RndPollable");
  ok &= expect_string(midi.cur_spot, "", "Enter cur spot value");
  ok &= expect_string(midi.new_spot, "", "Enter new spot value");
  ok &= expect_bool(midi.spot_changed, false, "Enter changed value");
  ok &= near(midi.frac, 0.0f, "Enter frac value");
  ok &= near(midi.frac_per_beat, 0.0f, "Enter frac per beat value");
  ok &= expect_bool(midi.local_xfm_reset, true, "Enter local reset value");
  ok &= expect_bool(midi.old_local_xfm_reset, true,
                    "Enter old local reset value");

  midi.bone = "bone_fret.mesh";
  midi.cur_spot = "spot_neck_fret20.mesh";
  SourceCharIKMidiPollDeps deps;
  source_char_ik_midi_poll_deps(deps, midi);
  ok &= expect_size(deps.change.size(), 1, "PollDeps change count");
  ok &= expect_string(deps.change[0], "bone_fret.mesh", "PollDeps change");
  ok &= expect_size(deps.changed_by.size(), 2, "PollDeps changed_by count");
  ok &= expect_string(deps.changed_by[0], "bone_fret.mesh",
                      "PollDeps changed_by bone");
  ok &= expect_string(deps.changed_by[1], "spot_neck_fret20.mesh",
                      "PollDeps changed_by spot");

  auto load = source_char_ik_midi_load_steps(2);
  ok &= expect_bool(load.known_revision, true, "Load rev2 known");
  ok &= expect_bool(load.load_hmx_object, true, "Load object");
  ok &= expect_bool(load.load_bone, true, "Load bone");
  ok &= expect_bool(load.load_legacy_spots, true, "Load rev2 spots");
  ok &= expect_bool(load.load_legacy_string, true, "Load rev2 string");
  ok &= expect_bool(load.load_anim_blend, false, "Load rev2 anim blend");
  load = source_char_ik_midi_load_steps(4);
  ok &= expect_bool(load.load_legacy_spots, false, "Load rev4 no spots");
  ok &= expect_bool(load.load_legacy_string, false, "Load rev4 no string");
  ok &= expect_bool(load.load_anim_blend, false, "Load rev4 no anim blend");
  load = source_char_ik_midi_load_steps(5);
  ok &= expect_bool(load.load_anim_blend, true, "Load rev5 anim blend");
  load = source_char_ik_midi_load_steps(6);
  ok &= expect_bool(load.known_revision, false, "Load rev6 rejected");

  const auto copy = source_char_ik_midi_copy_plan();
  ok &= expect_size(copy.copied_superclasses.size(), 1,
                    "Copy superclass count");
  ok &= expect_string(copy.copied_superclasses[0], "Hmx::Object",
                      "Copy superclass");
  ok &= expect_size(copy.copied_members.size(), 3, "Copy member count");
  ok &= expect_string(copy.copied_members[0], "mBone", "Copy bone");
  ok &= expect_string(copy.copied_members[1], "mAnimBlender",
                      "Copy anim blender");
  ok &= expect_string(copy.copied_members[2], "mMaxAnimBlend",
                      "Copy max anim blend");

  return ok ? 0 : 1;
}
