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

}  // namespace

int main() {
  using ghogx::character::SourceCharGuitarStringPollDeps;
  using ghogx::character::source_char_guitar_string_copy_plan;
  using ghogx::character::source_char_guitar_string_default_state;
  using ghogx::character::source_char_guitar_string_handler_plan;
  using ghogx::character::source_char_guitar_string_load_plan;
  using ghogx::character::source_char_guitar_string_poll;
  using ghogx::character::source_char_guitar_string_poll_deps;
  using ghogx::character::source_char_guitar_string_prop_sync_plan;
  using ghogx::character::source_char_guitar_string_save_plan;

  bool ok = true;

  const auto defaults = source_char_guitar_string_default_state();
  ok &= expect_bool(defaults.open, false, "default closed string");

  auto result = source_char_guitar_string_poll(
      true, true, true, false, false,
      {0.0f, 0.0f, 0.0f},
      {10.0f, 0.0f, 0.0f},
      {5.0f, 1.0f, 0.0f},
      {3.0f, 0.0f, 0.0f});
  ok &= expect_bool(result.wrote_bend, false, "missing target skips poll");
  ok &= vec_near(result.bend_pos, {5.0f, 1.0f, 0.0f},
                 "missing target preserves bend position");

  result = source_char_guitar_string_poll(
      true, true, true, true, false,
      {0.0f, 0.0f, 0.0f},
      {10.0f, 0.0f, 0.0f},
      {5.0f, 1.0f, 0.0f},
      {3.0f, 4.0f, 0.0f});
  ok &= expect_bool(result.wrote_bend, true, "target projection writes bend");
  ok &= vec_near(result.bend_pos, {3.0f, 0.0f, 0.0f},
                 "target projection follows nut bridge line");

  result = source_char_guitar_string_poll(
      true, true, true, true, false,
      {0.0f, 0.0f, 0.0f},
      {10.0f, 0.0f, 0.0f},
      {5.0f, 1.0f, 0.0f},
      {30.0f, 0.0f, 0.0f});
  ok &= vec_near(result.bend_pos, {10.0f, 0.0f, 0.0f},
                 "target projection clamps above bridge");

  result = source_char_guitar_string_poll(
      true, true, true, true, false,
      {0.0f, 0.0f, 0.0f},
      {10.0f, 0.0f, 0.0f},
      {5.0f, 1.0f, 0.0f},
      {-2.0f, 0.0f, 0.0f});
  ok &= vec_near(result.bend_pos, {0.0f, 0.0f, 0.0f},
                 "target projection clamps below nut");

  result = source_char_guitar_string_poll(
      true, true, true, true, true,
      {0.0f, 0.0f, 0.0f},
      {10.0f, 0.0f, 0.0f},
      {5.0f, 1.0f, 0.0f},
      {7.0f, 0.0f, 0.0f});
  ok &= vec_near(result.bend_pos, {0.0f, 0.0f, 0.0f},
                 "open string forces bend to nut");

  result = source_char_guitar_string_poll(
      true, true, true, true, false,
      {1.0f, 2.0f, 3.0f},
      {1.0f, 6.0f, 3.0f},
      {0.0f, 0.0f, 0.0f},
      {10.0f, 4.0f, 20.0f});
  ok &= vec_near(result.bend_pos, {1.0f, 4.0f, 3.0f},
                 "projection uses full source dot product");

  SourceCharGuitarStringPollDeps deps;
  source_char_guitar_string_poll_deps(deps, "nut.trans", "bridge.trans",
                                      "target.trans", "bend.trans");
  ok &= expect_size(deps.changed_by.size(), 3, "deps changed_by count");
  ok &= expect_size(deps.change.size(), 1, "deps change count");
  ok &= expect_string(deps.changed_by[0], "nut.trans", "deps nut");
  ok &= expect_string(deps.changed_by[1], "bridge.trans", "deps bridge");
  ok &= expect_string(deps.changed_by[2], "target.trans", "deps target");
  ok &= expect_string(deps.change[0], "bend.trans", "deps bend");

  const auto load_plan = source_char_guitar_string_load_plan(0);
  ok &= expect_bool(load_plan.known_revision, true,
                    "guitar string load revision known");
  ok &= expect_size(load_plan.read_order.size(), 5,
                    "guitar string load row count");
  ok &= expect_string(load_plan.read_order[0], "Hmx::Object",
                      "guitar string load object");
  ok &= expect_string(load_plan.read_order[1], "mNut",
                      "guitar string load nut");
  ok &= expect_string(load_plan.read_order[4], "mTarget",
                      "guitar string load target");
  const auto rejected_load = source_char_guitar_string_load_plan(1);
  ok &= expect_bool(rejected_load.known_revision, false,
                    "guitar string load rejects revision 1");
  ok &= expect_size(rejected_load.read_order.size(), 0,
                    "guitar string rejected load empty");

  const auto copy_plan = source_char_guitar_string_copy_plan();
  ok &= expect_size(copy_plan.copied_superclasses.size(), 1,
                    "guitar string copy superclass count");
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "guitar string copy superclass");
  ok &= expect_size(copy_plan.copied_members.size(), 4,
                    "guitar string copy member count");
  ok &= expect_string(copy_plan.copied_members[0], "mTarget",
                      "guitar string copy target first");
  ok &= expect_string(copy_plan.copied_members[3], "mBend",
                      "guitar string copy bend last");

  const auto handler_plan = source_char_guitar_string_handler_plan();
  ok &= expect_size(handler_plan.actions.size(), 1,
                    "guitar string handler action count");
  ok &= expect_string(handler_plan.actions[0],
                      "set_open:mOpen=_msg->Int(2)!=0",
                      "guitar string handler set open");
  ok &= expect_size(handler_plan.superclasses.size(), 1,
                    "guitar string handler superclass count");
  ok &= expect_string(handler_plan.superclasses[0], "Hmx::Object",
                      "guitar string handler superclass");
  ok &= expect_bool(handler_plan.check == 0x70, true,
                    "guitar string handler check row");
  ok &= expect_bool(source_char_guitar_string_save_plan().save_id == 0x47,
                    true, "guitar string save id");

  const auto prop_plan = source_char_guitar_string_prop_sync_plan();
  ok &= expect_size(prop_plan.properties.size(), 4,
                    "guitar string prop-sync count");
  ok &= expect_string(prop_plan.properties[0], "nut",
                      "guitar string prop nut");
  ok &= expect_string(prop_plan.properties[3], "target",
                      "guitar string prop target");

  return ok ? 0 : 1;
}
