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

bool expect_int(int got, int want, const char* label) {
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
  using ghogx::character::SourceCharIKSliderMidiPollDeps;
  using ghogx::character::source_char_ik_slider_midi_copy;
  using ghogx::character::source_char_ik_slider_midi_default_state;
  using ghogx::character::source_char_ik_slider_midi_enter;
  using ghogx::character::source_char_ik_slider_midi_handler_plan;
  using ghogx::character::source_char_ik_slider_midi_load_steps;
  using ghogx::character::source_char_ik_slider_midi_poll_deps;
  using ghogx::character::source_char_ik_slider_midi_prop_sync_plan;
  using ghogx::character::source_char_ik_slider_midi_save_plan;
  using ghogx::character::source_char_ik_slider_midi_set_name;
  using ghogx::character::source_char_ik_slider_midi_setup_transforms;
  using ghogx::character::source_char_weightable_set_weight_owner;

  bool ok = true;

  auto slider = source_char_ik_slider_midi_default_state("slider.weight");
  ok &= near(slider.weightable.weight, 1.0f, "default inherited weight");
  ok &= expect_string(slider.weightable.weight_owner, "slider.weight",
                      "default weight owner");
  ok &= expect_string(slider.target, "", "default target");
  ok &= expect_string(slider.first_spot, "", "default first spot");
  ok &= expect_string(slider.second_spot, "", "default second spot");
  ok &= near(slider.target_percentage, 1.0f, "default target percentage");
  ok &= near(slider.frac, 0.0f, "constructor Enter frac");
  ok &= near(slider.frac_per_beat, 0.0f, "constructor Enter frac per beat");
  ok &= expect_bool(slider.percentage_changed, false,
                    "constructor Enter percentage changed");
  ok &= expect_bool(slider.reset_all, true, "default reset all");
  ok &= near(slider.tolerance, 0.0f, "default tolerance");

  slider.percentage_changed = true;
  slider.frac = 0.25f;
  slider.frac_per_beat = 2.0f;
  auto enter = source_char_ik_slider_midi_enter(slider);
  ok &= expect_bool(enter.cleared_percentage_changed, true,
                    "Enter clears percentage flag");
  ok &= expect_bool(enter.reset_frac, true, "Enter resets frac flag");
  ok &= expect_bool(enter.reset_frac_per_beat, true,
                    "Enter resets frac per beat flag");
  ok &= expect_bool(enter.call_rnd_pollable_enter, true,
                    "Enter calls RndPollable Enter");
  ok &= expect_bool(slider.percentage_changed, false,
                    "Enter percentage flag value");
  ok &= near(slider.frac, 0.0f, "Enter frac value");
  ok &= near(slider.frac_per_beat, 0.0f, "Enter frac per beat value");

  auto set_name = source_char_ik_slider_midi_set_name(slider, "glam1", true);
  ok &= expect_bool(set_name.call_hmx_set_name, true,
                    "SetName delegates Hmx object");
  ok &= expect_bool(set_name.assigned_character, true,
                    "SetName stores Character dir");
  ok &= expect_string(slider.character_dir, "glam1", "SetName character dir");
  set_name = source_char_ik_slider_midi_set_name(slider, "not_char", false);
  ok &= expect_bool(set_name.assigned_character, false,
                    "SetName rejects non-character dir");
  ok &= expect_string(slider.character_dir, "",
                      "SetName clears non-character dir");

  slider.reset_all = false;
  auto setup = source_char_ik_slider_midi_setup_transforms(slider);
  ok &= expect_bool(setup.reset_all, true, "SetupTransforms returns reset");
  ok &= expect_bool(slider.reset_all, true, "SetupTransforms sets reset");

  slider.target = "slider_target";
  slider.first_spot = "spot_a";
  slider.second_spot = "spot_b";
  SourceCharIKSliderMidiPollDeps deps;
  source_char_ik_slider_midi_poll_deps(deps, slider);
  ok &= expect_size(deps.change.size(), 1, "PollDeps change count");
  ok &= expect_string(deps.change[0], "slider_target",
                      "PollDeps change target");
  ok &= expect_size(deps.changed_by.size(), 3, "PollDeps changed_by count");
  ok &= expect_string(deps.changed_by[0], "slider_target",
                      "PollDeps changed_by target");
  ok &= expect_string(deps.changed_by[1], "spot_a",
                      "PollDeps changed_by first spot");
  ok &= expect_string(deps.changed_by[2], "spot_b",
                      "PollDeps changed_by second spot");

  auto load = source_char_ik_slider_midi_load_steps(1);
  ok &= expect_bool(load.known_revision, true, "Load rev1 known");
  ok &= expect_bool(load.load_hmx_object, true, "Load Hmx object");
  ok &= expect_bool(load.load_weightable, false,
                    "Load rev1 skips CharWeightable");
  ok &= expect_bool(load.load_target, true, "Load target");
  ok &= expect_bool(load.load_first_spot, true, "Load first spot");
  ok &= expect_bool(load.load_second_spot, true, "Load second spot");
  ok &= expect_bool(load.load_tolerance, true, "Load tolerance");
  load = source_char_ik_slider_midi_load_steps(2);
  ok &= expect_bool(load.load_weightable, true,
                    "Load rev2 reads CharWeightable");
  load = source_char_ik_slider_midi_load_steps(3);
  ok &= expect_bool(load.known_revision, false, "Load rev3 rejected");

  auto source = source_char_ik_slider_midi_default_state("source.slider");
  source.target = "source_target";
  source.first_spot = "source_first";
  source.second_spot = "source_second";
  source.tolerance = 0.125f;
  source_char_weightable_set_weight_owner(source.weightable, "shared.owner");
  auto dest = source_char_ik_slider_midi_default_state("dest.slider");
  auto copy = source_char_ik_slider_midi_copy(dest, source, true, 0.25f);
  ok &= expect_bool(copy.copy_hmx_object, true, "Copy Hmx object");
  ok &= expect_bool(copy.copy_weightable, true, "Copy CharWeightable");
  ok &= expect_bool(copy.copy_target, true, "Copy target");
  ok &= expect_bool(copy.copy_first_spot, true, "Copy first spot");
  ok &= expect_bool(copy.copy_second_spot, true, "Copy second spot");
  ok &= expect_bool(copy.copy_tolerance, true, "Copy tolerance");
  ok &= expect_string(dest.target, "source_target", "Copy target value");
  ok &= expect_string(dest.first_spot, "source_first",
                      "Copy first spot value");
  ok &= expect_string(dest.second_spot, "source_second",
                      "Copy second spot value");
  ok &= near(dest.tolerance, 0.125f, "Copy tolerance value");
  ok &= expect_string(dest.weightable.weight_owner, "shared.owner",
                      "Copy shallow keeps owner");

  dest = source_char_ik_slider_midi_default_state("dest.deep");
  copy = source_char_ik_slider_midi_copy(dest, source, false, 0.66f);
  ok &= expect_string(dest.weightable.weight_owner, "dest.deep",
                      "Copy deep owns itself");
  ok &= near(dest.weightable.weight, 0.66f, "Copy deep owner weight");

  const auto handlers = source_char_ik_slider_midi_handler_plan();
  ok &= expect_size(handlers.actions.size(), 2, "handler action count");
  ok &= expect_string(handlers.actions[0], "set_fraction",
                      "handler set_fraction action");
  ok &= expect_string(handlers.actions[1], "reset", "handler reset action");
  ok &= expect_size(handlers.superclasses.size(), 2,
                    "handler superclass count");
  ok &= expect_string(handlers.superclasses[0], "CharWeightable",
                      "handler first superclass");
  ok &= expect_string(handlers.superclasses[1], "Hmx::Object",
                      "handler second superclass");
  ok &= expect_int(handlers.check, 0xF8, "handler check");

  const auto props = source_char_ik_slider_midi_prop_sync_plan();
  ok &= expect_size(props.modify_properties.size(), 3,
                    "prop-sync modify property count");
  ok &= expect_string(props.modify_properties[0], "target",
                      "prop-sync target modify");
  ok &= expect_string(props.modify_properties[1], "first_spot",
                      "prop-sync first spot modify");
  ok &= expect_string(props.modify_properties[2], "second_spot",
                      "prop-sync second spot modify");
  ok &= expect_string(props.modify_actions[0], "SetupTransforms",
                      "prop-sync target action");
  ok &= expect_string(props.properties[0], "tolerance",
                      "prop-sync tolerance property");
  ok &= expect_string(props.superclasses[0], "CharWeightable",
                      "prop-sync superclass");
  ok &= expect_int(source_char_ik_slider_midi_save_plan().save_id, 0xC4,
                   "IKSliderMidi save id");

  return ok ? 0 : 1;
}
