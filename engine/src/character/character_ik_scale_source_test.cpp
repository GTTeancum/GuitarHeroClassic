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

bool expect_int(int got, int want, const char* label) {
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
  using ghogx::character::source_char_ik_scale_copy_plan;
  using ghogx::character::source_char_ik_scale_default_state;
  using ghogx::character::source_char_ik_scale_handler_plan;
  using ghogx::character::source_char_ik_scale_load_plan;
  using ghogx::character::source_char_ik_scale_poll_deps;
  using ghogx::character::source_char_ik_scale_poll_enters;
  using ghogx::character::source_char_ik_scale_prop_sync_plan;

  bool ok = true;

  const auto defaults = source_char_ik_scale_default_state();
  ok &= near(defaults.scale, 1.0f, "default scale");
  ok &= near(defaults.bottom_height, 0.0f, "default bottom height");
  ok &= near(defaults.top_height, 0.0f, "default top height");
  ok &= expect_bool(defaults.auto_weight, false, "default auto weight");

  const auto load_v1 = source_char_ik_scale_load_plan(1);
  ok &= expect_bool(load_v1.known_revision, true, "Load v1 is accepted");
  ok &= expect_size(load_v1.read_order.size(), 4, "Load v1 row count");
  ok &= expect_string(load_v1.read_order[0], "Hmx::Object",
                      "Load v1 first row");
  ok &= expect_string(load_v1.read_order.back(), "mScale",
                      "Load v1 last row");

  const auto load_v2 = source_char_ik_scale_load_plan(2);
  ok &= expect_bool(load_v2.known_revision, true, "Load v2 is accepted");
  ok &= expect_size(load_v2.read_order.size(), 5, "Load v2 row count");
  ok &= expect_string(load_v2.read_order.back(), "mSecondaryTargets",
                      "Load v2 secondary target gate");

  const auto load_v3 = source_char_ik_scale_load_plan(3);
  ok &= expect_bool(load_v3.known_revision, true, "Load v3 is accepted");
  ok &= expect_size(load_v3.read_order.size(), 8, "Load v3 row count");
  ok &= expect_string(load_v3.read_order[5], "mAutoWeight",
                      "Load v3 auto-weight gate");
  ok &= expect_string(load_v3.read_order.back(), "mTopHeight",
                      "Load v3 tail row");
  ok &= expect_bool(source_char_ik_scale_load_plan(4).known_revision, false,
                    "Load rejects high revision");

  const auto copy_plan = source_char_ik_scale_copy_plan();
  ok &= expect_size(copy_plan.copied_superclasses.size(), 2,
                    "Copy superclass count");
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "Copy first superclass");
  ok &= expect_string(copy_plan.copied_superclasses[1], "CharWeightable",
                      "Copy second superclass");
  ok &= expect_size(copy_plan.copied_members.size(), 6, "Copy member count");
  ok &= expect_string(copy_plan.copied_members[0], "mDest",
                      "Copy first member");
  ok &= expect_string(copy_plan.copied_members.back(), "mTopHeight",
                      "Copy last member");

  const auto handler_plan = source_char_ik_scale_handler_plan();
  ok &= expect_size(handler_plan.actions.size(), 2, "handler action count");
  ok &= expect_string(handler_plan.actions[0], "capture_before",
                      "handler capture_before action");
  ok &= expect_string(handler_plan.actions[1], "capture_after",
                      "handler capture_after action");
  ok &= expect_int(handler_plan.check, 0xCC, "handler check value");

  const auto prop_sync_plan = source_char_ik_scale_prop_sync_plan();
  ok &= expect_size(prop_sync_plan.properties.size(), 6,
                    "prop-sync property count");
  ok &= expect_string(prop_sync_plan.properties[0], "dest",
                      "prop-sync first property");
  ok &= expect_string(prop_sync_plan.properties.back(), "top_height",
                      "prop-sync last property");
  ok &= expect_string(prop_sync_plan.superclasses[0], "CharWeightable",
                      "prop-sync superclass");

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
