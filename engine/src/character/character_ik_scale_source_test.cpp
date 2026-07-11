#include "character/char_mesh.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

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
  using ghogx::character::SourceCharIKScalePollDeps;
  using ghogx::character::source_char_ik_scale_capture_after;
  using ghogx::character::source_char_ik_scale_capture_before;
  using ghogx::character::source_char_ik_scale_default_state;
  using ghogx::character::source_char_ik_scale_poll_deps;
  using ghogx::character::source_char_ik_scale_poll_enters;

  bool ok = true;

  const auto defaults = source_char_ik_scale_default_state();
  ok &= near(defaults.scale, 1.0f, "default scale");
  ok &= near(defaults.bottom_height, 0.0f, "default bottom height");
  ok &= near(defaults.top_height, 0.0f, "default top height");
  ok &= expect_bool(defaults.auto_weight, false, "default auto weight");

  ok &= expect_bool(source_char_ik_scale_poll_enters(false, 1.0f), false,
                    "missing dest skips poll body");
  ok &= expect_bool(source_char_ik_scale_poll_enters(true, 0.0f), false,
                    "zero weight skips poll body");
  ok &= expect_bool(source_char_ik_scale_poll_enters(true, -0.25f), true,
                    "nonzero weight enters source poll gate");

  ok &= near(source_char_ik_scale_capture_before(false, 8.0f, 2.5f), 2.5f,
             "capture before missing dest preserves scale");
  ok &= near(source_char_ik_scale_capture_before(true, 8.0f, 2.5f), 8.0f,
             "capture before stores dest local z");

  ok &= near(source_char_ik_scale_capture_after(false, 12.0f, 4.0f), 4.0f,
             "capture after missing dest preserves scale");
  ok &= near(source_char_ik_scale_capture_after(true, 12.0f, 4.0f), 3.0f,
             "capture after divides dest local z by stored scale");

  SourceCharIKScalePollDeps deps;
  source_char_ik_scale_poll_deps(
      deps, "dest.scale", {"secondary.a", "secondary.b"});
  ok &= expect_size(deps.change.size(), 3, "PollDeps change count");
  ok &= expect_size(deps.changed_by.size(), 1, "PollDeps changed_by count");
  ok &= expect_string(deps.change[0], "dest.scale", "PollDeps dest change");
  ok &= expect_string(deps.change[1], "secondary.a",
                      "PollDeps first secondary change");
  ok &= expect_string(deps.change[2], "secondary.b",
                      "PollDeps second secondary change");
  ok &= expect_string(deps.changed_by[0], "dest.scale",
                      "PollDeps changed_by dest");

  return ok ? 0 : 1;
}
