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
  using ghogx::character::SourceCharBlendBoneConstraint;
  using ghogx::character::SourceCharBlendBonePollDeps;
  using ghogx::character::source_char_blend_bone_default_state;
  using ghogx::character::source_char_blend_bone_poll_deps;

  bool ok = true;

  auto blend = source_char_blend_bone_default_state();
  ok &= expect_size(blend.targets.size(), 0, "default target count");
  ok &= expect_string(blend.src1, "", "default src1");
  ok &= expect_string(blend.src2, "", "default src2");
  ok &= expect_bool(blend.trans_x, false, "default trans_x");
  ok &= expect_bool(blend.trans_y, false, "default trans_y");
  ok &= expect_bool(blend.trans_z, false, "default trans_z");
  ok &= expect_bool(blend.rotation, false, "default rotation");

  const SourceCharBlendBoneConstraint constraint;
  ok &= expect_string(constraint.target, "", "default constraint target");
  ok &= near(constraint.weight, 0.5f, "default constraint weight");

  blend.src1 = "source.one";
  blend.src2 = "source.two";
  blend.targets = {{"target.a", 0.25f}, {"target.b", 0.75f}};
  SourceCharBlendBonePollDeps deps;
  source_char_blend_bone_poll_deps(deps, blend);
  ok &= expect_size(deps.changed_by.size(), 2, "PollDeps changed_by count");
  ok &= expect_string(deps.changed_by[0], "source.one",
                      "PollDeps first source");
  ok &= expect_string(deps.changed_by[1], "source.two",
                      "PollDeps second source");
  ok &= expect_size(deps.change.size(), 2, "PollDeps target count");
  ok &= expect_string(deps.change[0], "target.a", "PollDeps first target");
  ok &= expect_string(deps.change[1], "target.b", "PollDeps second target");

  return ok ? 0 : 1;
}
