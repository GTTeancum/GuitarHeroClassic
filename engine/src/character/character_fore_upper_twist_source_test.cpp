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
  using ghogx::character::SourceCharForeTwistPollDeps;
  using ghogx::character::SourceCharForeTwistPollWorldResult;
  using ghogx::character::SourceCharUpperTwistPollDeps;
  using ghogx::character::SourceCharUpperTwistPollWorldResult;
  using ghogx::character::source_char_fore_twist_poll_deps;
  using ghogx::character::source_char_fore_twist_poll_world;
  using ghogx::character::source_char_fore_twist_save_plan;
  using ghogx::character::source_char_upper_twist_poll_deps;
  using ghogx::character::source_char_upper_twist_poll_world;
  using ghogx::character::source_char_upper_twist_save_plan;

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

  return ok ? 0 : 1;
}
