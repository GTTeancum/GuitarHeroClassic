#include "character/char_mesh.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

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
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharEyesInterest;
  using ghogx::character::SourceCharEyesInterestRuntime;
  using ghogx::character::SourceCharEyesPollDeps;
  using ghogx::character::source_char_eyes_add_interest_object;
  using ghogx::character::source_char_eyes_clear_interest_objects;
  using ghogx::character::source_char_eyes_copy_plan;
  using ghogx::character::source_char_eyes_copy_state;
  using ghogx::character::source_char_eyes_current_interest;
  using ghogx::character::source_char_eyes_default_interest_categories_sync;
  using ghogx::character::source_char_eyes_default_state;
  using ghogx::character::source_char_eyes_either_eye_clamped;
  using ghogx::character::source_char_eyes_eye_desc_assign;
  using ghogx::character::source_char_eyes_eye_desc_copy;
  using ghogx::character::source_char_eyes_eye_desc_default;
  using ghogx::character::source_char_eyes_eye_desc_load_plan;
  using ghogx::character::source_char_eyes_enter_state;
  using ghogx::character::source_char_eyes_exit_state;
  using ghogx::character::source_char_eyes_force_blink;
  using ghogx::character::source_char_eyes_get_head;
  using ghogx::character::source_char_eyes_handler_plan;
  using ghogx::character::source_char_eyes_interest_begin_refractory;
  using ghogx::character::source_char_eyes_interest_in_refractory;
  using ghogx::character::source_char_eyes_interest_refractory_remaining;
  using ghogx::character::source_char_eyes_interest_reset;
  using ghogx::character::source_char_eyes_interest_state;
  using ghogx::character::source_char_eyes_list_poll_children;
  using ghogx::character::source_char_eyes_load_plan;
  using ghogx::character::source_char_eyes_poll_deps;
  using ghogx::character::source_char_eyes_prop_sync_plan;
  using ghogx::character::source_char_eyes_runtime_dump_evidence;
  using ghogx::character::source_char_eyes_set_focus_interest;
  using ghogx::character::source_char_eyes_toggle_force_focus;
  using ghogx::character::source_char_eyes_toggle_interest_overlay;

  bool ok = true;

  const auto children = source_char_eyes_list_poll_children(
      {"l-eye.lookat", "r-eye.lookat"});
  ok &= expect_size(children.size(), 2, "children count");
  ok &= expect_string(children[0], "l-eye.lookat", "left child");
  ok &= expect_string(children[1], "r-eye.lookat", "right child");

  ok &= expect_bool(
      source_char_eyes_either_eye_clamped({{false, true}, {true, false}}),
      false, "missing eye clamp flag ignored");
  ok &= expect_bool(
      source_char_eyes_either_eye_clamped({{true, false}, {true, true}}),
      true, "present clamped eye detected");
  ok &= expect_bool(source_char_eyes_either_eye_clamped({}), false,
                    "empty eye clamp list false");

  const auto eye_desc_v6 = source_char_eyes_eye_desc_load_plan(6);
  ok &= expect_size(eye_desc_v6.read_order.size(), 2,
                    "eye desc v6 read count");
  ok &= expect_string(eye_desc_v6.read_order[1], "mUpperLid",
                      "eye desc v6 upper lid");
  const auto eye_desc_v7 = source_char_eyes_eye_desc_load_plan(7);
  ok &= expect_string(eye_desc_v7.read_order[2], "mLowerLid",
                      "eye desc v7 lower lid");
  const auto eye_desc_v16 = source_char_eyes_eye_desc_load_plan(16);
  ok &= expect_string(eye_desc_v16.read_order[3], "mUpperLidBlink",
                      "eye desc v16 upper blink");
  ok &= expect_string(eye_desc_v16.read_order[4], "mLowerLidBlink",
                      "eye desc v16 lower blink");

  const auto load_v4 = source_char_eyes_load_plan(4);
  ok &= expect_bool(load_v4.revision_supported, true,
                    "CharEyes v4 load supported");
  ok &= expect_string(load_v4.read_order[0], "Hmx::Object",
                      "CharEyes v4 object first");
  ok &= expect_string(load_v4.read_order[1], "legacyLookAtList",
                      "CharEyes v4 legacy eye list");
  ok &= expect_string(load_v4.read_order[2], "legacyTransformPtr",
                      "CharEyes v4 legacy transform");
  ok &= expect_string(load_v4.read_order[3],
                      "legacyInterestTransformCount",
                      "CharEyes v4 legacy interest count");
  ok &= expect_string(load_v4.branches[0],
                      "legacy lookats become EyeDesc with lid refs null",
                      "CharEyes v4 legacy lookat branch");

  const auto load_v16 = source_char_eyes_load_plan(16);
  ok &= expect_string(load_v16.read_order[1], "CharWeightable",
                      "CharEyes v16 loads weightable");
  ok &= expect_string(load_v16.read_order[2], "mEyes",
                      "CharEyes v16 loads eye descs");
  ok &= expect_string(load_v16.read_order[3], "mInterests",
                      "CharEyes v16 loads interests");
  ok &= expect_string(load_v16.read_order[14],
                      "legacyLowerLidTrackDownPad0",
                      "CharEyes v16 first lid padding");
  ok &= expect_string(load_v16.read_order[15], "mLowerLidTrackDown",
                      "CharEyes v16 lower lid down");
  ok &= expect_string(load_v16.read_order[16],
                      "legacyLowerLidTrackDownPad1",
                      "CharEyes v16 second lid padding");

  const auto load_v18 = source_char_eyes_load_plan(18);
  ok &= expect_string(load_v18.read_order.back(), "mLowerLidTrackRotate",
                      "CharEyes v18 lower-lid rotate");
  ok &= expect_bool(source_char_eyes_load_plan(19).revision_supported, false,
                    "CharEyes rejects high revision");
  const auto copy_plan = source_char_eyes_copy_plan();
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "CharEyes copy object superclass");
  ok &= expect_string(copy_plan.copied_superclasses[1], "CharWeightable",
                      "CharEyes copy weightable superclass");
  ok &= expect_string(copy_plan.copied_members[0], "mEyes",
                      "CharEyes copy eyes first");
  ok &= expect_string(copy_plan.copied_members.back(), "mLowerLidTrackRotate",
                      "CharEyes copy lower-lid rotate last");

  const auto handler_plan = source_char_eyes_handler_plan();
  ok &= expect_string(handler_plan.handlers[0], "add_interest",
                      "CharEyes handler add_interest");
  ok &= expect_string(handler_plan.action_handlers[0], "force_blink",
                      "CharEyes handler force_blink");
  ok &= expect_string(handler_plan.debug_handlers[0], "toggle_force_focus",
                      "CharEyes debug handler force focus");
  ok &= expect_string(handler_plan.debug_handlers[1],
                      "toggle_interest_overlay",
                      "CharEyes debug handler overlay");
  ok &= expect_string(handler_plan.superclasses[0], "Hmx::Object",
                      "CharEyes handler superclass");
  ok &= expect_int(handler_plan.check, 0x660, "CharEyes handler check");

  const auto prop_sync = source_char_eyes_prop_sync_plan();
  ok &= expect_string(prop_sync.eye_desc_properties[0], "eye",
                      "CharEyes EyeDesc prop eye");
  ok &= expect_string(prop_sync.eye_desc_properties[4], "lower_lid_blink",
                      "CharEyes EyeDesc prop lower blink");
  ok &= expect_string(prop_sync.interest_state_properties[0], "interest",
                      "CharEyes interest-state prop");
  ok &= expect_string(prop_sync.properties[0], "eyes",
                      "CharEyes prop eyes");
  ok &= expect_string(prop_sync.properties[4], "camera_weight",
                      "CharEyes prop camera weight");
  ok &= expect_string(prop_sync.properties.back(), "llid_track_rotate",
                      "CharEyes prop lower-lid rotate");
  ok &= expect_string(prop_sync.bitfield_properties[0],
                      "default_interest_categories",
                      "CharEyes default interest bitfield prop");
  ok &= expect_string(prop_sync.debug_properties[4], "disable_eye_clamping",
                      "CharEyes debug disable eye clamping prop");
  ok &= expect_string(prop_sync.debug_properties[5],
                      "interest_filter_testing",
                      "CharEyes debug interest filter prop");
  ok &= expect_string(prop_sync.superclasses[0], "CharWeightable",
                      "CharEyes prop superclass");
  const auto runtime_dump = source_char_eyes_runtime_dump_evidence();
  ok &= expect_string(runtime_dump.poll_range, "0x80354D64->0x80355480",
                      "CharEyes rb2 poll range");
  ok &= expect_string(runtime_dump.next_look_range, "0x8035559C->0x80355A74",
                      "CharEyes rb2 next-look range");
  ok &= expect_string(runtime_dump.poll_deps_range, "0x80355E84->0x80356030",
                      "CharEyes rb2 poll deps range");
  ok &= expect_size(runtime_dump.poll_locals.size(), 15,
                    "CharEyes rb2 poll local count");
  ok &= expect_string(runtime_dump.poll_locals[0], "h",
                      "CharEyes rb2 poll head local");
  ok &= expect_string(runtime_dump.poll_locals[10], "srcCam",
                      "CharEyes rb2 poll camera local");
  ok &= expect_bool(runtime_dump.rb2_dump_has_statement_body, false,
                    "CharEyes rb2 dump is not a statement body");
  ok &= expect_bool(runtime_dump.latest_source_has_poll_body, false,
                    "CharEyes latest source lacks poll body");
  ok &= expect_bool(runtime_dump.latest_source_has_get_target_body, false,
                    "CharEyes latest source lacks GetTarget body");
  ok &= expect_bool(runtime_dump.safe_to_publish_eye_runtime_rows, false,
                    "CharEyes runtime row publishing remains fenced");
  ok &= expect_bool(runtime_dump.safe_to_infer_facefx_rows, false,
                    "CharEyes FaceFX inference remains fenced");

  const auto category_get =
      source_char_eyes_default_interest_categories_sync(0x24, 0x20, true,
                                                       false);
  ok &= expect_int(category_get.flags, 0x24,
                   "CharEyes category get preserves flags");
  ok &= expect_bool(category_get.get_value, true,
                    "CharEyes category get returns masked bit");
  const auto category_set =
      source_char_eyes_default_interest_categories_sync(0x04, 0x20, false,
                                                       true);
  ok &= expect_int(category_set.flags, 0x24,
                   "CharEyes category set enables bit");
  ok &= expect_bool(category_set.get_value, true,
                    "CharEyes category set reports enabled bit");
  const auto category_clear =
      source_char_eyes_default_interest_categories_sync(0x24, 0x20, false,
                                                       false);
  ok &= expect_int(category_clear.flags, 0x04,
                   "CharEyes category set clears bit");
  ok &= expect_bool(category_clear.get_value, false,
                    "CharEyes category clear reports disabled bit");

  SourceCharEyesPollDeps deps;
  source_char_eyes_poll_deps(
      deps,
      {SourceCharEyesInterest{"same.interest", true},
       SourceCharEyesInterest{"other.interest", false}},
      true,
      "head.trans",
      "target.trans",
      "head.lookat",
      "face.servo");
  ok &= expect_size(deps.changed_by.size(), 4, "poll deps changed_by");
  ok &= expect_size(deps.change.size(), 1, "poll deps change");
  ok &= expect_string(deps.changed_by[0], "same.interest",
                      "same-dir interest dependency");
  ok &= expect_string(deps.changed_by[1], "head.trans",
                      "head dependency when eyes exist");
  ok &= expect_string(deps.changed_by[2], "head.lookat",
                      "head lookat dependency");
  ok &= expect_string(deps.changed_by[3], "face.servo",
                      "face servo dependency");
  ok &= expect_string(deps.change[0], "target.trans",
                      "target change when eyes exist");

  deps = SourceCharEyesPollDeps{};
  source_char_eyes_poll_deps(
      deps,
      {SourceCharEyesInterest{"same.interest", true}},
      false,
      "head.trans",
      "target.trans",
      "",
      "");
  ok &= expect_size(deps.changed_by.size(), 1,
                    "no eyes keeps same-dir interest only");
  ok &= expect_size(deps.change.size(), 0, "no eyes has no target change");
  ok &= expect_string(deps.changed_by[0], "same.interest",
                      "no eyes interest dependency");

  const auto defaults = source_char_eyes_default_state();
  ok &= expect_size(defaults.eye_count, 0, "default eye count");
  ok &= expect_size(defaults.interest_count, 0, "default interest count");
  ok &= expect_bool(defaults.has_face_servo, false, "default face servo");
  ok &= expect_bool(defaults.has_cam_weight, false, "default camera weight");
  ok &= expect_float(defaults.unk58[0], 0.0f, "default unk58 x");
  ok &= expect_float(defaults.unk58[1], 0.0f, "default unk58 y");
  ok &= expect_float(defaults.unk58[2], 0.0f, "default unk58 z");
  ok &= expect_int(defaults.default_filter_flags, 0,
                   "default filter flags");
  ok &= expect_bool(defaults.has_view_direction, false,
                    "default view direction");
  ok &= expect_bool(defaults.has_head_lookat, false,
                    "default head lookat");
  ok &= expect_float(defaults.max_extrapolation, 19.5f,
                     "default max extrapolation");
  ok &= expect_float(defaults.min_target_dist, 35.0f,
                     "default min target distance");
  ok &= expect_float(defaults.upper_lid_track_up, 1.0f,
                     "default upper lid track up");
  ok &= expect_float(defaults.upper_lid_track_down, 1.0f,
                     "default upper lid track down");
  ok &= expect_float(defaults.lower_lid_track_up, 0.75f,
                     "default lower lid track up");
  ok &= expect_float(defaults.lower_lid_track_down, 0.75f,
                     "default lower lid track down");
  ok &= expect_int(defaults.lower_lid_track_rotate, 0,
                   "default lower lid rotate");
  ok &= expect_int(defaults.interest_filter_flags, 0,
                   "default interest filter flags");
  ok &= expect_float(defaults.unka4[0], 0.0f, "default unka4 x");
  ok &= expect_float(defaults.unka4[1], 0.0f, "default unka4 y");
  ok &= expect_float(defaults.unka4[2], 0.0f, "default unka4 z");
  ok &= expect_int(defaults.unkb4, 0, "default unkb4");
  ok &= expect_float(defaults.unkb8, 0.86602539f, "default unkb8");
  ok &= expect_float(defaults.unkc0, 0.0f, "default unkc0");
  ok &= expect_int(defaults.unkc4, 0, "default unkc4");
  ok &= expect_bool(defaults.unkc5, false, "default unkc5");
  ok &= expect_bool(defaults.has_current_interest, false,
                    "default current interest");
  ok &= expect_bool(defaults.has_focus_interest, false,
                    "default focus interest");
  ok &= expect_int(defaults.focus_priority, -1, "default focus priority");
  ok &= expect_bool(defaults.unke4, false, "default unke4");
  ok &= expect_bool(defaults.unke8, false, "default unke8");
  ok &= expect_float(defaults.unkec, 1.0f, "default unkec");
  ok &= expect_bool(defaults.unkf0, false, "default unkf0");
  ok &= expect_bool(defaults.unkf4, false, "default unkf4");
  ok &= expect_bool(defaults.unk124, false, "default unk124");
  ok &= expect_float(defaults.unk128, -1.0f, "default unk128");
  ok &= expect_int(defaults.unk12c, -1, "default unk12c");
  ok &= expect_bool(defaults.unk13c, false, "default unk13c");
  ok &= expect_float(defaults.unk140, -1.0f, "default unk140");
  ok &= expect_int(defaults.unk144, 0, "default unk144");
  ok &= expect_float(defaults.unk148, -1.0f, "default unk148");
  ok &= expect_float(defaults.unk14c, -1.0f, "default unk14c");
  ok &= expect_bool(defaults.unk15c, false, "default unk15c");
  ok &= expect_bool(defaults.unk15d, true, "default unk15d");
  ok &= expect_string(defaults.overlay_name, "eye_status",
                      "default overlay name");

  auto source_defaults = source_char_eyes_default_state();
  source_defaults.eye_count = 2;
  source_defaults.interest_count = 3;
  source_defaults.has_face_servo = true;
  source_defaults.has_cam_weight = true;
  source_defaults.unka4 = {0.2f, 0.3f, 0.4f};
  source_defaults.unkb4 = 7;
  source_defaults.default_filter_flags = 0x55;
  source_defaults.has_view_direction = true;
  source_defaults.has_head_lookat = true;
  source_defaults.max_extrapolation = 42.0f;
  source_defaults.min_target_dist = 12.0f;
  source_defaults.upper_lid_track_up = 1.5f;
  source_defaults.upper_lid_track_down = 1.6f;
  source_defaults.lower_lid_track_up = 0.4f;
  source_defaults.lower_lid_track_down = 0.5f;
  source_defaults.lower_lid_track_rotate = 1;
  source_defaults.interest_filter_flags = 0x33;
  source_defaults.has_focus_interest = true;
  source_defaults.focus_priority = 9;
  source_defaults.unk13c = true;
  source_defaults.unk140 = 99.0f;
  source_defaults.overlay_name = "custom_overlay";

  const auto copied_defaults = source_char_eyes_copy_state(source_defaults);
  ok &= expect_size(copied_defaults.eye_count, 2, "copy eye count");
  ok &= expect_size(copied_defaults.interest_count, 3,
                    "copy interest count");
  ok &= expect_bool(copied_defaults.has_face_servo, true,
                    "copy face servo");
  ok &= expect_bool(copied_defaults.has_cam_weight, true,
                    "copy cam weight");
  ok &= expect_float(copied_defaults.unka4[0], 0.2f, "copy unka4 x");
  ok &= expect_float(copied_defaults.unka4[1], 0.3f, "copy unka4 y");
  ok &= expect_float(copied_defaults.unka4[2], 0.4f, "copy unka4 z");
  ok &= expect_int(copied_defaults.unkb4, 7, "copy unkb4");
  ok &= expect_int(copied_defaults.default_filter_flags, 0x55,
                   "copy filter flags");
  ok &= expect_bool(copied_defaults.has_view_direction, true,
                    "copy view direction");
  ok &= expect_bool(copied_defaults.has_head_lookat, true,
                    "copy head lookat");
  ok &= expect_float(copied_defaults.max_extrapolation, 42.0f,
                     "copy max extrapolation");
  ok &= expect_float(copied_defaults.min_target_dist, 12.0f,
                     "copy min target distance");
  ok &= expect_float(copied_defaults.upper_lid_track_up, 1.5f,
                     "copy upper lid up");
  ok &= expect_float(copied_defaults.upper_lid_track_down, 1.6f,
                     "copy upper lid down");
  ok &= expect_float(copied_defaults.lower_lid_track_up, 0.4f,
                     "copy lower lid up");
  ok &= expect_float(copied_defaults.lower_lid_track_down, 0.5f,
                     "copy lower lid down");
  ok &= expect_int(copied_defaults.lower_lid_track_rotate, 1,
                   "copy lower lid rotate");
  ok &= expect_int(copied_defaults.interest_filter_flags, 0,
                   "copy resets interest filters");
  ok &= expect_bool(copied_defaults.has_focus_interest, false,
                    "copy resets focus runtime");
  ok &= expect_int(copied_defaults.focus_priority, -1,
                   "copy resets focus priority");
  ok &= expect_bool(copied_defaults.unk13c, false,
                    "copy resets blink flag");
  ok &= expect_float(copied_defaults.unk140, -1.0f,
                     "copy resets blink time");
  ok &= expect_string(copied_defaults.overlay_name, "eye_status",
                      "copy resets overlay name");

  const auto default_eye = source_char_eyes_eye_desc_default();
  ok &= expect_string(default_eye.eye, "", "default eye ref");
  ok &= expect_string(default_eye.upper_lid, "", "default upper lid ref");
  ok &= expect_string(default_eye.lower_lid, "", "default lower lid ref");
  ok &= expect_string(default_eye.lower_lid_blink, "",
                      "default lower blink ref");
  ok &= expect_string(default_eye.upper_lid_blink, "",
                      "default upper blink ref");

  auto eye_desc = source_char_eyes_eye_desc_default();
  eye_desc.eye = "eye.lookat";
  eye_desc.upper_lid = "upper.lid";
  eye_desc.lower_lid = "lower.lid";
  eye_desc.lower_lid_blink = "lower.blink";
  eye_desc.upper_lid_blink = "upper.blink";
  const auto copied_eye = source_char_eyes_eye_desc_copy(eye_desc);
  ok &= expect_string(copied_eye.eye, "eye.lookat", "copy eye ref");
  ok &= expect_string(copied_eye.upper_lid, "upper.lid",
                      "copy upper lid ref");
  ok &= expect_string(copied_eye.lower_lid, "lower.lid",
                      "copy lower lid ref");
  ok &= expect_string(copied_eye.lower_lid_blink, "lower.blink",
                      "copy lower blink ref");
  ok &= expect_string(copied_eye.upper_lid_blink, "upper.blink",
                      "copy upper blink ref");
  auto assigned_eye = source_char_eyes_eye_desc_default();
  source_char_eyes_eye_desc_assign(assigned_eye, eye_desc);
  ok &= expect_string(assigned_eye.eye, "eye.lookat", "assign eye ref");
  ok &= expect_string(assigned_eye.upper_lid_blink, "upper.blink",
                      "assign upper blink ref");
  ok &= expect_string(assigned_eye.lower_lid_blink, "lower.blink",
                      "assign lower blink ref");

  ok &= expect_string(
      source_char_eyes_get_head("view.trans", "eye.parent"), "view.trans",
      "GetHead view direction priority");
  ok &= expect_string(
      source_char_eyes_get_head("", "eye.parent"), "eye.parent",
      "GetHead eye source parent fallback");
  ok &= expect_string(source_char_eyes_get_head("", ""), "",
                      "GetHead no source");

  ok &= expect_string(
      source_char_eyes_current_interest("focus.interest", "look.interest"),
      "focus.interest", "current interest focus priority");
  ok &= expect_string(
      source_char_eyes_current_interest("", "look.interest"),
      "look.interest", "current interest fallback");
  ok &= expect_string(source_char_eyes_current_interest("", ""), "",
                      "current interest empty");

  const auto rejected_focus = source_char_eyes_set_focus_interest(
      "boss.focus", 5, "minor.focus", 3);
  ok &= expect_bool(rejected_focus.accepted, false, "focus priority reject");
  ok &= expect_string(rejected_focus.focus_interest, "boss.focus",
                      "focus reject preserves current");
  ok &= expect_int(rejected_focus.focus_priority, 5,
                   "focus reject preserves priority");

  const auto accepted_focus = source_char_eyes_set_focus_interest(
      "boss.focus", 5, "major.focus", 7);
  ok &= expect_bool(accepted_focus.accepted, true, "focus priority accept");
  ok &= expect_string(accepted_focus.focus_interest, "major.focus",
                      "focus accept writes requested");
  ok &= expect_int(accepted_focus.focus_priority, 7,
                   "focus accept writes priority");

  const auto cleared_focus =
      source_char_eyes_set_focus_interest("boss.focus", 5, "", 8);
  ok &= expect_bool(cleared_focus.accepted, true, "focus clear accept");
  ok &= expect_string(cleared_focus.focus_interest, "", "focus clear value");
  ok &= expect_int(cleared_focus.focus_priority, -1,
                   "focus clear resets priority");

  const auto toggled_clear =
      source_char_eyes_toggle_force_focus("soft.focus", 0, "look.interest");
  ok &= expect_bool(toggled_clear.accepted, true,
                    "toggle focus clear accepted");
  ok &= expect_string(toggled_clear.focus_interest, "",
                      "toggle focus clears current");
  ok &= expect_int(toggled_clear.focus_priority, -1,
                   "toggle focus clear priority");

  const auto toggled_rejected =
      source_char_eyes_toggle_force_focus("boss.focus", 5, "look.interest");
  ok &= expect_bool(toggled_rejected.accepted, false,
                    "toggle focus high priority reject");
  ok &= expect_string(toggled_rejected.focus_interest, "boss.focus",
                      "toggle focus reject preserves current");
  ok &= expect_int(toggled_rejected.focus_priority, 5,
                   "toggle focus reject priority");

  const auto toggled_set =
      source_char_eyes_toggle_force_focus("", -1, "look.interest");
  ok &= expect_bool(toggled_set.accepted, true,
                    "toggle focus set accepted");
  ok &= expect_string(toggled_set.focus_interest, "look.interest",
                      "toggle focus sets current interest");
  ok &= expect_int(toggled_set.focus_priority, 0,
                   "toggle focus set priority");

  const auto overlay_on = source_char_eyes_toggle_interest_overlay(true, false);
  ok &= expect_bool(overlay_on.has_overlay, true, "overlay present");
  ok &= expect_bool(overlay_on.showing, true, "overlay toggled on");
  ok &= expect_bool(overlay_on.timer_restarted, true,
                    "overlay timer restarted");
  const auto overlay_missing =
      source_char_eyes_toggle_interest_overlay(false, true);
  ok &= expect_bool(overlay_missing.has_overlay, false, "overlay missing");
  ok &= expect_bool(overlay_missing.showing, true,
                    "missing overlay leaves showing state");
  ok &= expect_bool(overlay_missing.timer_restarted, false,
                    "missing overlay no timer restart");

  const auto blink = source_char_eyes_force_blink(12.5f);
  ok &= expect_bool(blink.pending_blink, true, "force blink pending");
  ok &= expect_float(blink.blink_time, 12.5f, "force blink time");
  ok &= expect_int(blink.blink_count_delta, 1, "force blink count delta");

  const auto enter = source_char_eyes_enter_state(
      0x24, true, {0.0f, 3.0f, 4.0f}, 2, 3);
  ok &= expect_float(enter.unka4[0], 0.0f, "enter head dir x");
  ok &= expect_float(enter.unka4[1], 0.6f, "enter head dir y");
  ok &= expect_float(enter.unka4[2], 0.8f, "enter head dir z");
  ok &= expect_int(enter.unkb4, 0, "enter unkb4");
  ok &= expect_int(enter.unkbc, 0, "enter unkbc");
  ok &= expect_float(enter.unkb0, 1.0f, "enter unkb0");
  ok &= expect_float(enter.unkc0, -1.0f, "enter unkc0");
  ok &= expect_int(enter.unkc4, 0, "enter unkc4");
  ok &= expect_bool(enter.unk124, false, "enter unk124");
  ok &= expect_float(enter.unk128, -1.0f, "enter unk128");
  ok &= expect_int(enter.unk12c, -1, "enter unk12c");
  ok &= expect_bool(enter.unk13c, false, "enter blink flag");
  ok &= expect_float(enter.unk140, -1.0f, "enter blink time");
  ok &= expect_int(enter.unk144, 0, "enter blink count");
  ok &= expect_float(enter.unk148, -1.0f, "enter unk148");
  ok &= expect_float(enter.unk14c, -1.0f, "enter unk14c");
  ok &= expect_bool(enter.unkc5, false, "enter unkc5");
  ok &= expect_int(enter.interest_filter_flags, 0x24,
                   "enter default filter flags");
  ok &= expect_bool(enter.unk15c, false, "enter unk15c");
  ok &= expect_bool(enter.unke4, false, "enter unke4");
  ok &= expect_bool(enter.unkf4, false, "enter unkf4");
  ok &= expect_size(enter.eye_enter_count, 2, "enter eye count");
  ok &= expect_size(enter.interest_reset_count, 3, "enter interest reset count");
  ok &= expect_bool(enter.pollable_enter, true, "enter pollable enter");
  const auto no_head_enter = source_char_eyes_enter_state(
      0, false, {0.0f, 3.0f, 4.0f}, 0, 0);
  ok &= expect_float(no_head_enter.unka4[0], 0.0f,
                     "enter no-head dir x");
  ok &= expect_float(no_head_enter.unka4[1], 0.0f,
                     "enter no-head dir y");
  ok &= expect_float(no_head_enter.unka4[2], 0.0f,
                     "enter no-head dir z");

  const auto exit = source_char_eyes_exit_state(2);
  ok &= expect_string(exit.focus_interest, "", "exit focus cleared");
  ok &= expect_int(exit.focus_priority, -1, "exit focus priority");
  ok &= expect_bool(exit.clear_interests, true, "exit clears interests");
  ok &= expect_size(exit.eye_exit_count, 2, "exit eye count");
  ok &= expect_bool(exit.pollable_exit, true, "exit pollable exit");

  auto runtime = source_char_eyes_interest_state("stage.light");
  ok &= expect_string(runtime.interest, "stage.light",
                      "interest runtime stores interest");
  ok &= expect_float(runtime.refractory_start, -1.0f,
                     "interest runtime resets refractory start");
  ok &= expect_bool(
      source_char_eyes_interest_in_refractory(runtime, 20.0f, 6.1f),
      false, "inactive refractory before begin");
  ok &= expect_float(
      source_char_eyes_interest_refractory_remaining(runtime, 20.0f, 6.1f),
      0.0f, "inactive refractory remaining");

  source_char_eyes_interest_begin_refractory(runtime, 20.0f);
  ok &= expect_float(runtime.refractory_start, 20.0f,
                     "begin refractory stores task time");
  ok &= expect_bool(
      source_char_eyes_interest_in_refractory(runtime, 23.0f, 6.1f),
      true, "refractory active before period");
  ok &= expect_float(
      source_char_eyes_interest_refractory_remaining(runtime, 23.0f, 6.1f),
      3.1f, "refractory remaining before period");
  ok &= expect_bool(
      source_char_eyes_interest_in_refractory(runtime, 27.0f, 6.1f),
      false, "refractory inactive after period");
  ok &= expect_float(
      source_char_eyes_interest_refractory_remaining(runtime, 27.0f, 6.1f),
      0.0f, "refractory remaining after period");

  auto empty_runtime = source_char_eyes_interest_state("");
  source_char_eyes_interest_begin_refractory(empty_runtime, 20.0f);
  ok &= expect_bool(source_char_eyes_interest_in_refractory(
                        empty_runtime, 21.0f, 6.1f),
                    false, "missing interest never refractory");
  source_char_eyes_interest_reset(runtime);
  ok &= expect_float(runtime.refractory_start, -1.0f,
                     "reset refractory start");

  std::vector<SourceCharEyesInterestRuntime> interests;
  ok &= expect_bool(source_char_eyes_add_interest_object(interests, ""),
                    false, "add missing interest ignored");
  ok &= expect_size(interests.size(), 0, "missing interest not pushed");
  ok &= expect_bool(
      source_char_eyes_add_interest_object(interests, "stage.light"),
      true, "add valid interest accepted");
  ok &= expect_size(interests.size(), 1, "valid interest pushed");
  ok &= expect_string(interests[0].interest, "stage.light",
                      "added interest name");
  ok &= expect_float(interests[0].refractory_start, -1.0f,
                     "added interest reset state");
  source_char_eyes_clear_interest_objects(interests);
  ok &= expect_size(interests.size(), 0, "clear all interests");

  return ok ? 0 : 1;
}
