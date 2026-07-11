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
  using ghogx::character::SourceCharLookAtPollDeps;
  using ghogx::character::source_char_lookat_enter;
  using ghogx::character::source_char_lookat_poll_deps;
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

  const auto entered = source_char_lookat_enter(true);
  ok &= near(entered.smoothed_dir[0], 1.0e29f, "enter smoothed dir x");
  ok &= near(entered.smoothed_dir[1], 0.0f, "enter smoothed dir y");
  ok &= near(entered.smoothed_dir[2], 0.0f, "enter smoothed dir z");
  if (!entered.reset_pivot_local) {
    std::cerr << "enter should request pivot local identity\n";
    ok = false;
  }
  const auto no_pivot_enter = source_char_lookat_enter(false);
  if (no_pivot_enter.reset_pivot_local) {
    std::cerr << "enter should not reset missing pivot\n";
    ok = false;
  }

  SourceCharLookAtPollDeps deps;
  source_char_lookat_poll_deps(deps, "explicit.source", "pivot.lookat",
                               "target.lookat");
  if (deps.changed_by.size() != 2 || deps.changed_by[0] != "explicit.source" ||
      deps.changed_by[1] != "target.lookat" || deps.change.size() != 1 ||
      deps.change[0] != "pivot.lookat") {
    std::cerr << "poll deps explicit source mismatch\n";
    ok = false;
  }

  SourceCharLookAtPollDeps fallback_deps;
  source_char_lookat_poll_deps(fallback_deps, "", "pivot.lookat",
                               "target.lookat");
  if (fallback_deps.changed_by.size() != 2 ||
      fallback_deps.changed_by[0] != "pivot.lookat" ||
      fallback_deps.changed_by[1] != "target.lookat" ||
      fallback_deps.change.size() != 1 ||
      fallback_deps.change[0] != "pivot.lookat") {
    std::cerr << "poll deps pivot fallback mismatch\n";
    ok = false;
  }

  return ok ? 0 : 1;
}
