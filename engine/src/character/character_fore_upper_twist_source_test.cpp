#include "character/char_clip.h"

#include <array>
#include <cmath>
#include <iostream>
#include <string>

namespace {

std::array<float, 16> world_row(float px, float py, float pz) {
  return {1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0,
          px, py, pz, 1};
}

std::array<float, 16> rot_x_world(float angle, float px, float py, float pz) {
  const float ca = std::cos(angle);
  const float sa = std::sin(angle);
  return {1, 0, 0, 0,
          0, ca, -sa, 0,
          0, sa, ca, 0,
          px, py, pz, 1};
}

ghogx::milo_scene::Xfm trace_local_x_source(float angle,
                                            float px = 0.0f,
                                            float py = 0.0f,
                                            float pz = 0.0f) {
  ghogx::milo_scene::Xfm xfm;
  const float ca = std::cos(angle);
  const float sa = std::sin(angle);
  xfm.rot[1][1] = ca;
  xfm.rot[1][2] = sa;
  xfm.rot[2][1] = -sa;
  xfm.rot[2][2] = ca;
  xfm.pos[0] = px;
  xfm.pos[1] = py;
  xfm.pos[2] = pz;
  return xfm;
}

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << ": got " << got << " want " << want << "\n";
  return false;
}

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got " << (got ? "true" : "false")
            << " want " << (want ? "true" : "false") << "\n";
  return false;
}

bool expect_size(size_t got, size_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got " << got << " want " << want << "\n";
  return false;
}

bool expect_string(const std::string& got, const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got '" << got << "' want '" << want << "'\n";
  return false;
}

bool expect_upper_rows(const std::array<float, 16>& world, float weight,
                       float px, float py, float pz, const char* label) {
  const float y = 1.0f - weight;
  const float z = weight;
  const float len = std::sqrt(y * y + z * z);
  bool ok = true;
  ok &= near(world[0], 1.0f, label);
  ok &= near(world[1], 0.0f, label);
  ok &= near(world[2], 0.0f, label);
  ok &= near(world[4], 0.0f, label);
  ok &= near(world[5], y / len, label);
  ok &= near(world[6], z / len, label);
  ok &= near(world[8], 0.0f, label);
  ok &= near(world[9], -z / len, label);
  ok &= near(world[10], y / len, label);
  ok &= near(world[12], px, label);
  ok &= near(world[13], py, label);
  ok &= near(world[14], pz, label);
  return ok;
}

}  // namespace

int main() {
  using ghogx::character::CharForeTwist;
  using ghogx::character::Character;
  using ghogx::character::Gh1AnimServoForeTwist;
  using ghogx::character::Gh1AnimServoUpperTwist;
  using ghogx::character::SkinnedMesh;
  using ghogx::character::SourceGh2TraceForeTwistLocalResult;
  using ghogx::character::SourceGh2TraceUpperTwistLocalResult;
  using ghogx::character::SourceCharForeTwistPollDeps;
  using ghogx::character::SourceCharForeTwistPollWorldResult;
  using ghogx::character::SourceCharUpperTwistPollDeps;
  using ghogx::character::SourceCharUpperTwistPollWorldResult;
  using ghogx::character::source_gh2_trace_fore_twist_poll_local;
  using ghogx::character::source_gh2_trace_local_twist_angle;
  using ghogx::character::source_gh2_trace_upper_twist_poll_local;
  using ghogx::character::source_gh2_trace_write_x_twist;
  using ghogx::character::source_char_fore_twist_poll_deps;
  using ghogx::character::source_char_fore_twist_poll_world;
  using ghogx::character::source_char_fore_twist_save_plan;
  using ghogx::character::source_char_upper_twist_poll_deps;
  using ghogx::character::source_char_upper_twist_poll_world;
  using ghogx::character::source_char_upper_twist_save_plan;
  using ghogx::character::apply_character_controllers;

  constexpr float kPi = 3.14159265358979323846f;
  bool ok = true;
  ok &= expect_bool(source_char_fore_twist_save_plan().save_id == 0x79,
                    true, "CharForeTwist save id");
  ok &= expect_bool(source_char_upper_twist_save_plan().save_id == 0x5D,
                    true, "CharUpperTwist save id");

  CharForeTwist fore;
  fore.name = "foreTwist.test";
  fore.offset_degrees = 90.0f;
  fore.bias_degrees = 30.0f;
  SourceCharForeTwistPollDeps fore_deps;
  source_char_fore_twist_poll_deps(
      fore_deps, "bone_l_hand", "bone_l_foretwist2", false,
      "bone_l_foretwist2_parent");
  ok &= expect_size(fore_deps.changed_by.size(), 1,
                    "ForeTwist PollDeps changed-by count");
  ok &= expect_string(fore_deps.changed_by[0], "bone_l_hand",
                      "ForeTwist PollDeps hand dependency");
  ok &= expect_size(fore_deps.change.size(), 1,
                    "ForeTwist PollDeps missing twist2 change count");
  ok &= expect_string(fore_deps.change[0], "bone_l_foretwist2",
                      "ForeTwist PollDeps twist2 change row");
  fore_deps = SourceCharForeTwistPollDeps{};
  source_char_fore_twist_poll_deps(
      fore_deps, "bone_l_hand", "bone_l_foretwist2", true,
      "bone_l_foretwist2_parent");
  ok &= expect_size(fore_deps.change.size(), 2,
                    "ForeTwist PollDeps parent change count");
  ok &= expect_string(fore_deps.change[1], "bone_l_foretwist2_parent",
                      "ForeTwist PollDeps twist2 parent change row");
  SourceCharForeTwistPollWorldResult fore_out;
  ok &= expect_bool(source_char_fore_twist_poll_world(
                        fore, false, true, true, true, world_row(0, 0, 0),
                        world_row(0, 0, 0), 30.0f, 10.0f, fore_out),
                    false, "ForeTwist rejects missing hand");

  std::array<float, 16> hand_world =
      {1, 0, 0, 0,
       0, 0, -1, 0,
       0, 1, 0, 0,
       40, 0, 0, 1};
  ok &= expect_bool(source_char_fore_twist_poll_world(
                        fore, true, true, true, true, world_row(10, 0, 0),
                        hand_world, 30.0f, 10.0f, fore_out),
                    true, "ForeTwist applies with source pointers");
  ok &= near(fore_out.source_angle_radians, kPi * 2.0f / 3.0f,
             "ForeTwist source angle includes bias");
  ok &= near(fore_out.applied_rotation_radians, kPi / 6.0f,
             "ForeTwist applies one third final angle");
  ok &= near(fore_out.twist2_position_ratio, 1.0f / 3.0f,
             "ForeTwist uses twist2.local.x / hand.local.x");
  ok &= near(fore_out.twist_parent_world[5], std::cos(kPi / 6.0f),
             "ForeTwist parent y.y");
  ok &= near(fore_out.twist_parent_world[6], -std::sin(kPi / 6.0f),
             "ForeTwist parent y.z");
  ok &= near(fore_out.twist_parent_world[9], std::sin(kPi / 6.0f),
             "ForeTwist parent z.y");
  ok &= near(fore_out.twist_parent_world[10], std::cos(kPi / 6.0f),
             "ForeTwist parent z.z");
  ok &= near(fore_out.twist_parent_world[12], 10.0f,
             "ForeTwist parent preserves hand-parent x");
  ok &= near(fore_out.twist2_world[5], std::cos(kPi / 3.0f),
             "ForeTwist twist2 y.y");
  ok &= near(fore_out.twist2_world[6], -std::sin(kPi / 3.0f),
             "ForeTwist twist2 y.z");
  ok &= near(fore_out.twist2_world[9], std::sin(kPi / 3.0f),
             "ForeTwist twist2 z.y");
  ok &= near(fore_out.twist2_world[10], std::cos(kPi / 3.0f),
             "ForeTwist twist2 z.z");
  ok &= near(fore_out.twist2_world[12], 20.0f,
             "ForeTwist interpolates twist2 toward hand position");

  SourceCharUpperTwistPollDeps upper_deps;
  source_char_upper_twist_poll_deps(
      upper_deps, "bone_l_upper_arm", "bone_l_upper_twist1",
      "bone_l_upper_twist2");
  ok &= expect_size(upper_deps.changed_by.size(), 1,
                    "UpperTwist PollDeps changed-by count");
  ok &= expect_string(upper_deps.changed_by[0], "bone_l_upper_arm",
                      "UpperTwist PollDeps source dependency");
  ok &= expect_size(upper_deps.change.size(), 2,
                    "UpperTwist PollDeps change count");
  ok &= expect_string(upper_deps.change[0], "bone_l_upper_twist1",
                      "UpperTwist PollDeps first output");
  ok &= expect_string(upper_deps.change[1], "bone_l_upper_twist2",
                      "UpperTwist PollDeps second output");

  SourceCharUpperTwistPollWorldResult upper_out;
  ok &= expect_bool(source_char_upper_twist_poll_world(
                        true, true, true, false, world_row(0, 0, 0),
                        world_row(0, 0, 0), world_row(0, 0, 0),
                        world_row(0, 0, 0), upper_out),
                    false, "UpperTwist rejects missing source parent");

  const std::array<float, 16> source_parent_world = world_row(0, 0, 0);
  const std::array<float, 16> source_world =
      rot_x_world(-kPi * 0.5f, 1.0f, 2.0f, 3.0f);
  ok &= expect_bool(source_char_upper_twist_poll_world(
                        true, true, true, true, source_parent_world,
                        source_world, world_row(5, 6, 7),
                        world_row(8, 9, 10), upper_out),
                    true, "UpperTwist applies with source pointers");
  ok &= expect_upper_rows(upper_out.twist1_world, 0.333f, 5.0f, 6.0f,
                          7.0f, "UpperTwist first output rows");
  ok &= expect_upper_rows(upper_out.twist2_world, 0.666f, 8.0f, 9.0f,
                          10.0f, "UpperTwist second output rows");

  const auto gh2_source = trace_local_x_source(kPi / 3.0f);
  ok &= near(source_gh2_trace_local_twist_angle(gh2_source), kPi / 3.0f,
             "GH2 trace local twist angle");

  ghogx::milo_scene::Xfm gh2_written;
  const auto write_basis = trace_local_x_source(0.0f, 2.0f, 3.0f, 4.0f);
  source_gh2_trace_write_x_twist(gh2_written, write_basis, kPi / 6.0f);
  ok &= near(gh2_written.pos[0], 2.0f, "GH2 trace write preserves pos x");
  ok &= near(gh2_written.pos[1], 3.0f, "GH2 trace write preserves pos y");
  ok &= near(gh2_written.pos[2], 4.0f, "GH2 trace write preserves pos z");
  ok &= near(gh2_written.rot[0][0], 1.0f,
             "GH2 trace write preserves basis row0 x");
  ok &= near(gh2_written.rot[1][1], std::cos(kPi / 6.0f),
             "GH2 trace write rotates row1 y");
  ok &= near(gh2_written.rot[1][2], -std::sin(kPi / 6.0f),
             "GH2 trace write rotates row1 z");
  ok &= near(gh2_written.rot[2][1], std::sin(kPi / 6.0f),
             "GH2 trace write rotates row2 y");
  ok &= near(gh2_written.rot[2][2], std::cos(kPi / 6.0f),
             "GH2 trace write rotates row2 z");

  CharForeTwist gh2_fore;
  gh2_fore.name = "foreTwist.gh2";
  gh2_fore.offset_degrees = 90.0f;
  gh2_fore.bias_degrees = 30.0f;
  SourceGh2TraceForeTwistLocalResult gh2_fore_out;
  const auto fore_basis = trace_local_x_source(0.0f, 11.0f, 12.0f, 13.0f);
  const auto twist2_basis = trace_local_x_source(0.0f, 21.0f, 22.0f, 23.0f);
  ok &= expect_bool(source_gh2_trace_fore_twist_poll_local(
                        gh2_fore, true, true, true, true,
                        trace_local_x_source(0.0f), fore_basis, twist2_basis,
                        gh2_fore_out),
                    true, "GH2 trace ForeTwist local applies");
  ok &= near(gh2_fore_out.roll_radians, -kPi / 6.0f,
             "GH2 trace ForeTwist rolls one third of offset");
  ok &= near(gh2_fore_out.twist1_local.pos[0], 11.0f,
             "GH2 trace ForeTwist twist1 preserves forearm pos");
  ok &= near(gh2_fore_out.twist2_local.pos[0], 21.0f,
             "GH2 trace ForeTwist twist2 preserves authored pos");
  ok &= near(gh2_fore_out.twist1_local.rot[1][2], 0.5f,
             "GH2 trace ForeTwist twist1 row sign");
  ok &= near(gh2_fore_out.twist2_local.rot[2][1], -0.5f,
             "GH2 trace ForeTwist twist2 row sign");

  SourceGh2TraceUpperTwistLocalResult gh2_upper_out;
  ok &= expect_bool(source_gh2_trace_upper_twist_poll_local(
                        true, true, true,
                        trace_local_x_source(kPi * 0.5f, 31.0f, 32.0f,
                                             33.0f),
                        trace_local_x_source(0.0f, 41.0f, 42.0f, 43.0f),
                        gh2_upper_out),
                    true, "GH2 trace UpperTwist local applies");
  ok &= near(gh2_upper_out.roll_radians, kPi * 0.5f,
             "GH2 trace UpperTwist reads source roll");
  ok &= near(gh2_upper_out.twist1_local.pos[0], 31.0f,
             "GH2 trace UpperTwist twist1 preserves upper pos");
  ok &= near(gh2_upper_out.twist2_local.pos[0], 41.0f,
             "GH2 trace UpperTwist twist2 preserves authored pos");
  ok &= near(gh2_upper_out.twist1_local.rot[1][1],
             std::sin(kPi * 0.5f * 0.6660000086f),
             "GH2 trace UpperTwist twist1 uses upper basis");
  ok &= near(gh2_upper_out.twist1_local.rot[1][2],
             std::cos(kPi * 0.5f * 0.6660000086f),
             "GH2 trace UpperTwist twist1 row z");
  ok &= near(gh2_upper_out.twist2_local.rot[1][2],
             std::sin(kPi * 0.5f * 0.3330000043f),
             "GH2 trace UpperTwist twist2 inverse sign");

  auto mesh = [](const std::string& name, const std::string& parent,
                 float x, float roll = 0.0f) {
    SkinnedMesh row;
    row.name = name;
    row.parent = parent;
    row.local = trace_local_x_source(roll, x, 0.0f, 0.0f);
    return row;
  };
  Character gh1;
  gh1.dir_version = 10;
  gh1.meshes = {
      mesh("root.mesh", "", 0.0f),
      mesh("clavicle.mesh", "root.mesh", 0.0f),
      mesh("upperArm.mesh", "clavicle.mesh", 1.0f, kPi * 0.5f),
      mesh("upperTwist1.mesh", "clavicle.mesh", 1.0f),
      mesh("upperTwist2.mesh", "clavicle.mesh", 1.0f),
      mesh("foreArm.mesh", "upperArm.mesh", 10.0f),
      mesh("foreTwist1.mesh", "upperArm.mesh", 10.0f),
      mesh("foreTwist2.mesh", "foreTwist1.mesh", 4.5f),
      mesh("hand.mesh", "foreArm.mesh", 9.0f),
  };
  Gh1AnimServoForeTwist gh1_fore;
  gh1_fore.name = "fore.servo";
  gh1_fore.fore_arm = "foreArm.mesh";
  gh1_fore.twist1 = "foreTwist1.mesh";
  gh1_fore.twist2 = "foreTwist2.mesh";
  gh1_fore.hand = "hand.mesh";
  gh1_fore.offset_degrees = 90.0f;
  gh1.gh1_fore_twists.push_back(gh1_fore);
  Gh1AnimServoUpperTwist gh1_upper;
  gh1_upper.name = "upper.servo";
  gh1_upper.twist1 = "upperTwist1.mesh";
  gh1_upper.twist2 = "upperTwist2.mesh";
  gh1_upper.upper_arm = "upperArm.mesh";
  gh1.gh1_upper_twists.push_back(gh1_upper);
  apply_character_controllers(gh1, 0.0f);
  ok &= expect_bool(std::fabs(gh1.meshes[6].local.rot[1][2]) > 0.1f,
                    true, "GH1 mesh foreTwist1 servo writes rotation");
  ok &= expect_bool(std::fabs(gh1.meshes[7].local.rot[1][2]) > 0.1f,
                    true, "GH1 mesh foreTwist2 servo writes rotation");
  ok &= near(gh1.meshes[7].local.pos[0], 4.5f,
             "GH1 mesh foreTwist2 preserves authored half-arm position");
  ok &= expect_bool(std::fabs(gh1.meshes[3].local.rot[1][2]) > 0.1f,
                    true, "GH1 mesh upperTwist1 servo writes rotation");
  ok &= expect_bool(std::fabs(gh1.meshes[4].local.rot[1][2]) > 0.1f,
                    true, "GH1 mesh upperTwist2 servo writes rotation");

  Character gh2_gate = gh1;
  gh2_gate.dir_type = "BandCharacter";
  gh2_gate.meshes[3].local = trace_local_x_source(0.0f, 1.0f);
  gh2_gate.meshes[4].local = trace_local_x_source(0.0f, 1.0f);
  gh2_gate.meshes[6].local = trace_local_x_source(0.0f, 10.0f);
  gh2_gate.meshes[7].local = trace_local_x_source(0.0f, 4.5f);
  apply_character_controllers(gh2_gate, 0.0f);
  ok &= near(gh2_gate.meshes[3].local.rot[1][2], 0.0f,
             "GH1 upper servo does not enter GH2 graph");
  ok &= near(gh2_gate.meshes[6].local.rot[1][2], 0.0f,
             "GH1 fore servo does not enter GH2 graph");

  return ok ? 0 : 1;
}
