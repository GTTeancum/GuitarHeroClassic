#include "render/milo_scene_renderer.h"

#include <cstdio>
#include <cstdlib>

namespace {

#define CHECK(cond)                                                        \
  do {                                                                     \
    if (!(cond)) {                                                         \
      std::printf("  [FAIL] %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
      std::exit(1);                                                        \
    }                                                                      \
  } while (0)

}  // namespace

int main() {
  using ghogx::render::source_material_bypasses_fixed_lighting;

  CHECK(!source_material_bypasses_fixed_lighting(false, false));
  CHECK(!source_material_bypasses_fixed_lighting(false, true));
  CHECK(source_material_bypasses_fixed_lighting(true, false));
  CHECK(!source_material_bypasses_fixed_lighting(true, true));

  std::printf(
      "  [ok] material light channel follows use_environ || !prelit\n");
  return 0;
}
