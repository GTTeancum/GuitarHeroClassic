#include "character/char_clip.h"

#include <cmath>
#include <iostream>

namespace {

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << ": got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::source_char_lookat_sync_limits;

  bool ok = true;

  const auto defaults = source_char_lookat_sync_limits(-80.0f, 80.0f,
                                                       -80.0f, 80.0f);
  ok &= near(defaults.min[1], 0.17364818f, "default min y");
  ok &= near(defaults.max[1], 1.0e29f, "default max y");
  ok &= near(defaults.min[2], -0.98480779f, "default min yaw z");
  ok &= near(defaults.max[2], 0.98480779f, "default max yaw z");
  ok &= near(defaults.min[0], -0.98480779f, "default min pitch x");
  ok &= near(defaults.max[0], 0.98480779f, "default max pitch x");

  const auto clamped = source_char_lookat_sync_limits(-120.0f, 120.0f,
                                                      -90.0f, 90.0f);
  ok &= near(clamped.min[1], defaults.min[1], "clamped min y");
  ok &= near(clamped.min[2], defaults.min[2], "clamped min yaw z");
  ok &= near(clamped.max[2], defaults.max[2], "clamped max yaw z");
  ok &= near(clamped.min[0], defaults.min[0], "clamped min pitch x");
  ok &= near(clamped.max[0], defaults.max[0], "clamped max pitch x");

  const auto asymmetric = source_char_lookat_sync_limits(-30.0f, 45.0f,
                                                         -10.0f, 20.0f);
  ok &= near(asymmetric.min[1], 0.70710677f, "asymmetric min y");
  ok &= near(asymmetric.min[2], -0.40824831f, "asymmetric min yaw z");
  ok &= near(asymmetric.max[2], 0.70710677f, "asymmetric max yaw z");
  ok &= near(asymmetric.min[0], -0.12468200f, "asymmetric min pitch x");
  ok &= near(asymmetric.max[0], 0.25735635f, "asymmetric max pitch x");

  return ok ? 0 : 1;
}
