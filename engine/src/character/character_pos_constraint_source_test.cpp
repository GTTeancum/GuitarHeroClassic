#include "character/char_mesh.h"

#include <cstdint>
#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {

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
  using ghogx::character::decode_pos_constraint;
  using ghogx::character::source_char_pos_constraint_copy_plan;
  using ghogx::character::source_char_pos_constraint_load_plan;
  using ghogx::character::source_char_pos_constraint_poll_deps_plan;
  using ghogx::character::source_char_pos_constraint_save_plan;
  using ghogx::character::source_char_pos_constraint_target_position;

  bool ok = true;

  const auto load_bad = source_char_pos_constraint_load_plan(3);
  ok &= expect_bool(load_bad.known_revision, false,
                    "load rejects unknown revision");
  ok &= expect_size(load_bad.read_order.size(), 0,
                    "bad revision has no reads");

  const auto load_rev1 = source_char_pos_constraint_load_plan(1);
  ok &= expect_bool(load_rev1.known_revision, true, "load accepts rev1");
  ok &= expect_string(load_rev1.read_order[1], "Hmx::Object",
                      "load object superclass");
  ok &= expect_string(load_rev1.read_order[2], "mTargets",
                      "load targets before source");
  ok &= expect_string(load_rev1.read_order[3], "mSrc",
                      "load source after targets");
  ok &= expect_size(load_rev1.read_order.size(), 4, "rev1 read count");
  ok &= expect_string(load_rev1.branches[0], "mBox.min=(1,1,0)",
                      "rev1 default min box");
  ok &= expect_string(load_rev1.branches[1], "mBox.max=(-1,-1,1000)",
                      "rev1 default max box");

  const auto load_rev2 = source_char_pos_constraint_load_plan(2);
  ok &= expect_bool(load_rev2.known_revision, true, "load accepts rev2");
  ok &= expect_string(load_rev2.read_order.back(), "mBox",
                      "rev2 reads box");
  ok &= expect_size(load_rev2.branches.size(), 0,
                    "rev2 no default branches");

  const auto save = source_char_pos_constraint_save_plan();
  ok &= expect_bool(save.save_id == 0x64, true, "save id");

  const auto copy_plan = source_char_pos_constraint_copy_plan();
  ok &= expect_size(copy_plan.copied_superclasses.size(), 1,
                    "copy superclass count");
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "copy object superclass");
  ok &= expect_size(copy_plan.copied_members.size(), 3,
                    "copy member count");
  ok &= expect_string(copy_plan.copied_members[0], "mTargets",
                      "copy targets");
  ok &= expect_string(copy_plan.copied_members[1], "mSrc",
                      "copy source");
  ok &= expect_string(copy_plan.copied_members[2], "mBox",
                      "copy box");

  const auto deps = source_char_pos_constraint_poll_deps_plan(
      "source.trans", {"target_a.trans", "target_b.trans"});
  ok &= expect_size(deps.changed_by.size(), 3, "PollDeps changed_by count");
  ok &= expect_size(deps.change.size(), 2, "PollDeps change count");
  ok &= expect_string(deps.changed_by[0], "source.trans",
                      "PollDeps source first");
  ok &= expect_string(deps.change[0], "target_a.trans",
                      "PollDeps first changed target");
  ok &= expect_string(deps.changed_by[2], "target_b.trans",
                      "PollDeps second target dependency");

  bool bad_version_threw = false;
  try {
    (void)decode_pos_constraint("bad.pos_constraint", {3, 0, 0, 0});
  } catch (const std::exception&) {
    bad_version_threw = true;
  }
  ok &= expect_bool(bad_version_threw, true,
                    "decoder rejects bad source revision");

  ok &= vec_near(
      source_char_pos_constraint_target_position(
          {10.0f, 20.0f, 30.0f},
          {14.0f, 21.0f, 41.0f},
          {1.0f, -5.0f, 0.0f},
          {-1.0f, 5.0f, 1000.0f}),
      {14.0f, 21.0f, 41.0f},
      "old-revision x disabled and y/z in range");

  ok &= vec_near(
      source_char_pos_constraint_target_position(
          {10.0f, 20.0f, 30.0f},
          {30.0f, -10.0f, 40.0f},
          {-2.0f, -3.0f, 20.0f},
          {4.0f, 6.0f, 100.0f}),
      {14.0f, 17.0f, 50.0f},
      "clamps each enabled axis by target-source delta");

  ok &= vec_near(
      source_char_pos_constraint_target_position(
          {-5.0f, 0.0f, 5.0f},
          {-9.0f, 7.0f, 1.0f},
          {-10.0f, 1.0f, -2.0f},
          {-1.0f, 3.0f, 2.0f}),
      {-9.0f, 3.0f, 3.0f},
      "preserves in-range low axis and clamps high/low axes");

  return ok ? 0 : 1;
}
