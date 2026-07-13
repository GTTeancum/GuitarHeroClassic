#include "character/char_mesh.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

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

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool vec_near(const std::array<float, 3>& got,
              const std::array<float, 3>& want,
              const char* label) {
  bool ok = true;
  ok &= near(got[0], want[0], label);
  ok &= near(got[1], want[1], label);
  ok &= near(got[2], want[2], label);
  return ok;
}

ghogx::milo_scene::Xfm identity_xfm(float x, float y, float z) {
  ghogx::milo_scene::Xfm out;
  out.pos[0] = x;
  out.pos[1] = y;
  out.pos[2] = z;
  return out;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharSleevePollDeps;
  using ghogx::character::source_char_sleeve_copy_plan;
  using ghogx::character::source_char_sleeve_default_state;
  using ghogx::character::source_char_sleeve_handler_plan;
  using ghogx::character::source_char_sleeve_highlight_plan;
  using ghogx::character::source_char_sleeve_load_plan;
  using ghogx::character::source_char_sleeve_poll;
  using ghogx::character::source_char_sleeve_poll_deps;
  using ghogx::character::source_char_sleeve_prop_sync_plan;
  using ghogx::character::source_char_sleeve_save_plan;
  using ghogx::character::source_char_sleeve_set_name_step;

  bool ok = true;

  auto state = source_char_sleeve_default_state();
  ok &= vec_near(state.pos, {0.0f, 0.0f, 0.0f}, "default pos");
  ok &= vec_near(state.last_pos, {0.0f, 0.0f, 0.0f}, "default last pos");
  ok &= near(state.last_dt, 0.0f, "default last dt");
  ok &= near(state.inertia, 0.5f, "default inertia");
  ok &= near(state.gravity, 1.0f, "default gravity");
  ok &= near(state.stiffness, 0.02f, "default stiffness");

  const auto set_name_non_character =
      source_char_sleeve_set_name_step(false);
  ok &= expect_bool(set_name_non_character.calls_hmx_object_set_name, true,
                    "SetName calls object SetName");
  ok &= expect_bool(set_name_non_character.assigns_character_owner, false,
                    "SetName non-character owner");
  const auto set_name_character = source_char_sleeve_set_name_step(true);
  ok &= expect_bool(set_name_character.assigns_character_owner, true,
                    "SetName captures character owner");

  auto parent = identity_xfm(0.0f, 0.0f, 0.0f);
  auto sleeve_world = identity_xfm(0.0f, 0.0f, -2.0f);
  state.pos = {0.0f, 0.0f, -2.0f};
  state.last_pos = {0.0f, 0.0f, -2.0f};

  auto result = source_char_sleeve_poll(
      state, false, true, false, false, 0.0f, -2.0f, sleeve_world, parent);
  ok &= expect_bool(result.wrote_sleeve, false, "missing sleeve skips poll");
  ok &= vec_near(state.pos, {0.0f, 0.0f, -2.0f},
                 "missing sleeve preserves pos");

  result = source_char_sleeve_poll(
      state, true, true, true, false, 0.0f, -2.0f, sleeve_world, parent);
  ok &= expect_bool(result.wrote_sleeve, true, "poll writes sleeve");
  ok &= expect_bool(result.wrote_top_sleeve, true, "poll writes top sleeve");
  ok &= near(result.sleeve_world.pos[2], -2.0f, "sleeve z at zero delta");
  ok &= near(result.sleeve_world.rot[0][0], 1.0f, "sleeve basis x");
  ok &= near(result.sleeve_world.rot[1][1], 1.0f, "sleeve basis y");
  ok &= near(result.sleeve_world.rot[2][2], 1.0f, "sleeve basis z");
  ok &= near(result.top_sleeve_world.pos[2], -2.0f,
             "top sleeve z at zero delta");

  state = source_char_sleeve_default_state();
  state.pos_length = 1.0f;
  sleeve_world = identity_xfm(0.0f, 0.0f, 0.0f);
  result = source_char_sleeve_poll(
      state, true, true, false, true, 0.0f, -2.0f, sleeve_world, parent);
  ok &= expect_bool(result.wrote_sleeve, true, "teleport writes sleeve");
  ok &= near(result.sleeve_world.pos[2], -3.0f,
             "teleport seeds sleeve length");
  ok &= vec_near(state.pos, {0.0f, 0.0f, -3.0f}, "teleport updates pos");
  ok &= vec_near(state.last_pos, state.pos, "teleport resets last pos");

  const auto highlight_missing =
      source_char_sleeve_highlight_plan(false, true, true);
  ok &= expect_bool(highlight_missing.exits_without_sleeve_or_parent, true,
                    "Highlight exits without sleeve");
  ok &= expect_size(highlight_missing.draw_steps.size(), 0,
                    "Highlight missing sleeve no draws");
  const auto highlight_no_top =
      source_char_sleeve_highlight_plan(true, true, false);
  ok &= expect_bool(highlight_no_top.exits_without_sleeve_or_parent, false,
                    "Highlight with sleeve and parent runs");
  ok &= expect_size(highlight_no_top.draw_steps.size(), 2,
                    "Highlight no top draw count");
  ok &= expect_string(highlight_no_top.draw_steps[0],
                      "UtilDrawAxes(mSleeve, green)",
                      "Highlight sleeve axes draw");
  ok &= expect_string(highlight_no_top.draw_steps[1],
                      "DrawLine(mSleeve,parent, green)",
                      "Highlight sleeve parent line");
  const auto highlight_top =
      source_char_sleeve_highlight_plan(true, true, true);
  ok &= expect_size(highlight_top.draw_steps.size(), 4,
                    "Highlight top sleeve draw count");
  ok &= expect_string(highlight_top.draw_steps[2],
                      "UtilDrawAxes(mTopSleeve, cyan)",
                      "Highlight top sleeve axes draw");
  ok &= expect_string(highlight_top.draw_steps[3],
                      "DrawLine(mTopSleeve,parent, cyan)",
                      "Highlight top sleeve parent line");

  SourceCharSleevePollDeps deps;
  source_char_sleeve_poll_deps(deps, "parent.trans", "sleeve.trans",
                               "top.trans", false);
  ok &= expect_size(deps.changed_by.size(), 0, "no sleeve deps changed_by");
  ok &= expect_size(deps.change.size(), 0, "no sleeve deps change");
  source_char_sleeve_poll_deps(deps, "parent.trans", "sleeve.trans",
                               "top.trans", true);
  ok &= expect_size(deps.changed_by.size(), 1, "sleeve deps changed_by");
  ok &= expect_size(deps.change.size(), 2, "sleeve deps change count");
  ok &= expect_string(deps.changed_by[0], "parent.trans",
                      "sleeve deps parent");
  ok &= expect_string(deps.change[0], "sleeve.trans", "sleeve deps sleeve");
  ok &= expect_string(deps.change[1], "top.trans", "sleeve deps top sleeve");

  const auto load_plan = source_char_sleeve_load_plan(0);
  ok &= expect_bool(load_plan.revision_supported, true,
                    "sleeve load revision supported");
  ok &= expect_size(load_plan.read_order.size(), 9, "sleeve load order count");
  ok &= expect_string(load_plan.read_order[0], "Hmx::Object",
                      "sleeve load object first");
  ok &= expect_string(load_plan.read_order[1], "mSleeve",
                      "sleeve load sleeve pointer");
  ok &= expect_string(load_plan.read_order[2], "mTopSleeve",
                      "sleeve load top sleeve pointer");
  ok &= expect_string(load_plan.read_order[5], "mStiffness",
                      "sleeve load stiffness order");
  ok &= expect_string(load_plan.read_order[8], "mPosLength",
                      "sleeve load pos length last");
  const auto rejected_load = source_char_sleeve_load_plan(1);
  ok &= expect_bool(rejected_load.revision_supported, false,
                    "sleeve load rejects non-source revision");
  ok &= expect_size(rejected_load.read_order.size(), 0,
                    "sleeve rejected load has no reads");

  const auto save = source_char_sleeve_save_plan();
  ok &= expect_bool(save.save_id == 0xE1, true, "sleeve save id");

  const auto copy_plan = source_char_sleeve_copy_plan();
  ok &= expect_size(copy_plan.copied_superclasses.size(), 1,
                    "sleeve copy superclass count");
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "sleeve copy superclass");
  ok &= expect_size(copy_plan.copied_members.size(), 8,
                    "sleeve copy member count");
  ok &= expect_string(copy_plan.copied_members[0], "mSleeve",
                      "sleeve copy sleeve pointer");
  ok &= expect_string(copy_plan.copied_members[1], "mTopSleeve",
                      "sleeve copy top sleeve pointer");
  ok &= expect_string(copy_plan.copied_members[7], "mPosLength",
                      "sleeve copy pos length last");

  const auto handler_plan = source_char_sleeve_handler_plan();
  ok &= expect_size(handler_plan.superclasses.size(), 1,
                    "sleeve handler superclass count");
  ok &= expect_string(handler_plan.superclasses[0], "Hmx::Object",
                      "sleeve handler superclass");
  ok &= expect_bool(handler_plan.check == 0x112, true,
                    "sleeve handler check row");

  const auto prop_plan = source_char_sleeve_prop_sync_plan();
  ok &= expect_size(prop_plan.properties.size(), 8,
                    "sleeve prop-sync count");
  ok &= expect_string(prop_plan.properties[0], "sleeve",
                      "sleeve prop-sync sleeve");
  ok &= expect_string(prop_plan.properties[1], "top_sleeve",
                      "sleeve prop-sync top sleeve");
  ok &= expect_string(prop_plan.properties[4], "stiffness",
                      "sleeve prop-sync stiffness");
  ok &= expect_string(prop_plan.properties[7], "pos_length",
                      "sleeve prop-sync pos length");

  return ok ? 0 : 1;
}
