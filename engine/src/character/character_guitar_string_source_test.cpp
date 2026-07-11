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
  using ghogx::character::source_char_guitar_string_poll;
  using ghogx::character::source_char_guitar_string_poll_deps;

  bool ok = true;

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

  return ok ? 0 : 1;
}
