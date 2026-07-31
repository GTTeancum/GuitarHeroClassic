#include "ui/menu_app.h"

#include <cstdio>

namespace {

int failures = 0;

#define CHECK(condition)                                                   \
  do {                                                                     \
    if (!(condition)) {                                                    \
      std::fprintf(stderr, "FAIL %s:%d CHECK(%s)\n", __FILE__, __LINE__,   \
                   #condition);                                            \
      ++failures;                                                          \
    }                                                                      \
  } while (0)

}  // namespace

int main() {
  using ghogx::ui::source_panel_external_dependency_plan;

  const auto character_select =
      source_panel_external_dependency_plan("meta_proxy.cam", "ui.env", false);
  CHECK(!character_select.import_metacam_cameras);
  CHECK(character_select.import_metacam_environment);

  const auto direct_camera =
      source_panel_external_dependency_plan("meta.cam", "ui.env", false);
  CHECK(direct_camera.import_metacam_cameras);
  CHECK(direct_camera.import_metacam_environment);

  const auto local_environment =
      source_panel_external_dependency_plan("meta_proxy.cam", "ui.env", true);
  CHECK(!local_environment.import_metacam_cameras);
  CHECK(!local_environment.import_metacam_environment);

  const auto no_external_objects =
      source_panel_external_dependency_plan("panel.cam", "", false);
  CHECK(!no_external_objects.import_metacam_cameras);
  CHECK(!no_external_objects.import_metacam_environment);

  if (failures == 0) {
    std::puts("panel external dependency contract: PASS");
    return 0;
  }
  return 1;
}
