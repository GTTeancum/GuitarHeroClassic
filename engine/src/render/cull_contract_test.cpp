#include "render/milo_scene_renderer.h"

#include <array>
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

std::array<float, 16> diagonal(float x, float y, float z) {
  return {x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1};
}

}  // namespace

int main() {
  using ghogx::render::mesh_source_winding_reversed;
  using ghogx::render::source_mesh_cull_mode;
  using ghogx::render::world_transform_reverses_winding;

  CHECK(!world_transform_reverses_winding(diagonal(1.0f, 1.0f, 1.0f)));
  CHECK(world_transform_reverses_winding(diagonal(-1.0f, 1.0f, 1.0f)));
  CHECK(world_transform_reverses_winding(diagonal(2.0f, -3.0f, 4.0f)));
  CHECK(!world_transform_reverses_winding(diagonal(-1.0f, -1.0f, 1.0f)));
  CHECK(!world_transform_reverses_winding(diagonal(0.0f, 1.0f, 1.0f)));

  ghogx::milo_scene::MeshObj forward;
  forward.verts = {
      {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
      {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
      {0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
  };
  forward.indices = {0, 1, 2};
  CHECK(!mesh_source_winding_reversed(forward));

  ghogx::milo_scene::MeshObj reversed = forward;
  reversed.indices = {0, 2, 1};
  CHECK(mesh_source_winding_reversed(reversed));

  constexpr uint32_t kCullNone = 1;
  constexpr uint32_t kCullCw = 2;
  constexpr uint32_t kCullCcw = 3;
  CHECK(source_mesh_cull_mode(true, false, false, false, kCullCw) ==
        kCullCw);
  CHECK(source_mesh_cull_mode(true, false, true, false, kCullCw) ==
        kCullCcw);
  CHECK(source_mesh_cull_mode(true, false, false, true, kCullCw) ==
        kCullCcw);
  CHECK(source_mesh_cull_mode(true, false, true, true, kCullCw) ==
        kCullCw);
  CHECK(source_mesh_cull_mode(false, false, true, true, kCullCw) ==
        kCullNone);
  CHECK(source_mesh_cull_mode(true, true, true, true, kCullCw) ==
        kCullNone);

  std::printf(
      "  [ok] cull winding combines source triangle order and transform "
      "handedness\n");
  return 0;
}
