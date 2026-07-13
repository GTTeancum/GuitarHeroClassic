#include "character/char_clip.h"

#include <array>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

std::array<float, 16> world_row(float px, float py, float pz) {
  return {1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0,
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

}  // namespace

int main() {
  using ghogx::character::CharBoneTwist;
  using ghogx::character::SourceCharBoneTwistPollDeps;
  using ghogx::character::source_char_bone_twist_poll_deps;
  using ghogx::character::source_char_bone_twist_poll_world;
  using ghogx::character::source_char_bone_twist_save_plan;
  using ghogx::character::source_char_bone_twist_weight;

  CharBoneTwist twist;
  twist.name = "test.twist";
  twist.version = 0;
  twist.weightable_version = 2;
  twist.weight = 0.5f;
  twist.bone = "bone_twist.mesh";
  twist.targets = {"target_a.mesh", "target_b.mesh"};

  std::array<float, 16> bone_world =
      {2, 0, 0, 0,
       0, 1, 0, 0,
       0, 0, 1, 0,
       1, 2, 3, 1};
  std::vector<std::array<float, 16>> target_worlds = {
      world_row(1.0f, 2.0f, 13.0f),
      world_row(1.0f, 2.0f, 13.0f),
  };
  std::unordered_map<std::string, float> weights;

  bool ok = true;
  ok &= expect_bool(source_char_bone_twist_save_plan().save_id == 0x59,
                    true, "CharBoneTwist save id");
  SourceCharBoneTwistPollDeps deps;
  source_char_bone_twist_poll_deps(deps, twist.bone, {});
  ok &= expect_size(deps.change.size(), 1,
                    "PollDeps publishes bone change without targets");
  ok &= expect_string(deps.change[0], twist.bone,
                      "PollDeps driven bone change row");
  ok &= expect_size(deps.changed_by.size(), 0,
                    "PollDeps empty target dependency count");
  deps = SourceCharBoneTwistPollDeps{};
  source_char_bone_twist_poll_deps(deps, twist.bone, twist.targets);
  ok &= expect_size(deps.change.size(), 1,
                    "PollDeps resolved change count");
  ok &= expect_string(deps.change[0], twist.bone,
                      "PollDeps resolved bone change row");
  ok &= expect_size(deps.changed_by.size(), 2,
                    "PollDeps target dependency count");
  ok &= expect_string(deps.changed_by[0], "target_a.mesh",
                      "PollDeps first target");
  ok &= expect_string(deps.changed_by[1], "target_b.mesh",
                      "PollDeps second target");
  ok &= near(source_char_bone_twist_weight(twist, weights), 0.5f,
             "local twist weight");
  twist.weight_owner = "owner.weight";
  weights["owner.weight"] = 1.0f;
  ok &= near(source_char_bone_twist_weight(twist, weights), 1.0f,
             "owner twist weight");
  weights.clear();
  ok &= near(source_char_bone_twist_weight(twist, weights), 0.5f,
             "missing owner falls back to local weight");

  twist.weight_owner.clear();
  std::array<float, 16> out{};
  ok &= expect_bool(source_char_bone_twist_poll_world(
                        twist, false, bone_world, target_worlds, weights, out),
                    false, "Poll returns false without bone");
  ok &= expect_bool(source_char_bone_twist_poll_world(
                        twist, true, bone_world, {}, weights, out),
                    false, "Poll returns false without targets");
  ok &= expect_bool(source_char_bone_twist_poll_world(
                        twist, true, bone_world, target_worlds, weights, out),
                    true, "Poll returns true with bone and targets");

  constexpr float kHalf = 0.70710678f;
  ok &= near(out[0], 2.0f, "Poll preserves x row length");
  ok &= near(out[1], 0.0f, "Poll preserves x row y");
  ok &= near(out[2], 0.0f, "Poll preserves x row z");
  ok &= near(out[4], 0.0f, "Poll solved y row x");
  ok &= near(out[5], kHalf, "Poll solved y row y");
  ok &= near(out[6], kHalf, "Poll solved y row z");
  ok &= near(out[8], 0.0f, "Poll solved z row x");
  ok &= near(out[9], -2.0f * kHalf, "Poll solved z row y");
  ok &= near(out[10], 2.0f * kHalf, "Poll solved z row z");
  ok &= near(out[12], 1.0f, "Poll preserves world x");
  ok &= near(out[13], 2.0f, "Poll preserves world y");
  ok &= near(out[14], 3.0f, "Poll preserves world z");

  twist.weight_owner = "owner.weight";
  weights["owner.weight"] = 1.0f;
  ok &= expect_bool(source_char_bone_twist_poll_world(
                        twist, true, bone_world, target_worlds, weights, out),
                    true, "Poll uses owner weight");
  ok &= near(out[4], 0.0f, "Owner weight y row x");
  ok &= near(out[5], 0.0f, "Owner weight y row y");
  ok &= near(out[6], 1.0f, "Owner weight y row z");
  ok &= near(out[9], -2.0f, "Owner weight z row y");
  ok &= near(out[10], 0.0f, "Owner weight z row z");

  return ok ? 0 : 1;
}
