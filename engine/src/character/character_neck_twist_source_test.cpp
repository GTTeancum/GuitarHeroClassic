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

}  // namespace

int main() {
  using ghogx::character::SourceCharNeckTwistPollDeps;
  using ghogx::character::source_char_neck_twist_defaults;
  using ghogx::character::source_char_neck_twist_half_limited_angle;
  using ghogx::character::source_char_neck_twist_load_revision_known;
  using ghogx::character::source_char_neck_twist_poll_deps;

  constexpr float kPi = 3.14159265358979323846f;
  bool ok = true;

  const auto defaults = source_char_neck_twist_defaults();
  ok &= expect_string(defaults.head, "", "default head");
  ok &= expect_string(defaults.twist, "", "default twist");

  ok &= expect_bool(source_char_neck_twist_load_revision_known(-1), false,
                    "revision -1 rejected");
  ok &= expect_bool(source_char_neck_twist_load_revision_known(0), true,
                    "revision 0 accepted");
  ok &= expect_bool(source_char_neck_twist_load_revision_known(1), true,
                    "revision 1 accepted");
  ok &= expect_bool(source_char_neck_twist_load_revision_known(2), false,
                    "revision 2 rejected");

  SourceCharNeckTwistPollDeps deps;
  source_char_neck_twist_poll_deps(deps, "bone_head.mesh",
                                   "bone_neck.mesh");
  ok &= expect_size(deps.changed_by.size(), 1, "changed_by count");
  ok &= expect_size(deps.change.size(), 1, "change count");
  ok &= expect_string(deps.changed_by[0], "bone_head.mesh", "changed_by head");
  ok &= expect_string(deps.change[0], "bone_neck.mesh", "change twist");

  ok &= near(source_char_neck_twist_half_limited_angle(1.0f, 0.0f), 0.0f,
             "zero twist angle");
  ok &= near(source_char_neck_twist_half_limited_angle(0.0f, 1.0f),
             kPi * 0.25f, "positive quarter twist angle");
  ok &= near(source_char_neck_twist_half_limited_angle(0.0f, -1.0f),
             -kPi * 0.25f, "negative quarter twist angle");

  return ok ? 0 : 1;
}
