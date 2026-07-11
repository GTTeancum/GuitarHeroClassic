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
  using ghogx::character::source_char_sleeve_default_state;
  using ghogx::character::source_char_sleeve_poll;
  using ghogx::character::source_char_sleeve_poll_deps;

  bool ok = true;

  auto state = source_char_sleeve_default_state();
  ok &= vec_near(state.pos, {0.0f, 0.0f, 0.0f}, "default pos");
  ok &= vec_near(state.last_pos, {0.0f, 0.0f, 0.0f}, "default last pos");
  ok &= near(state.last_dt, 0.0f, "default last dt");
  ok &= near(state.inertia, 0.5f, "default inertia");
  ok &= near(state.gravity, 1.0f, "default gravity");
  ok &= near(state.stiffness, 0.02f, "default stiffness");

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

  return ok ? 0 : 1;
}
