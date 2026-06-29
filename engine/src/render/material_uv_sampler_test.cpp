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

using ghogx::render::MiloSceneRenderer;
using ghogx::render::choose_material_uv_sampler;

MiloSceneRenderer::MaterialUvSamplerDecision decide(
    MiloSceneRenderer::MaterialUvBounds bounds, float scale_u = 1.0f,
    float scale_v = 1.0f, bool material_tex_anim = false) {
  return choose_material_uv_sampler(bounds, scale_u, scale_v, material_tex_anim);
}

}  // namespace

int main() {
  const auto static_tile =
      decide({true, -1.18f, 2.00f, 3.13f, 3.00f});
  CHECK(static_tile.uv_repeats);
  CHECK(static_tile.wrap);

  const auto negative_tile =
      decide({true, -0.98f, -3.12f, 3.30f, 3.12f});
  CHECK(negative_tile.uv_repeats);
  CHECK(negative_tile.wrap);

  const auto small_edge_bleed =
      decide({true, -0.04f, 0.00f, 1.04f, 1.00f});
  CHECK(!small_edge_bleed.uv_repeats);
  CHECK(!small_edge_bleed.wrap);

  const auto scaled_tile =
      decide({true, 0.00f, 0.00f, 1.00f, 1.00f}, 1.20f, 1.00f);
  CHECK(!scaled_tile.uv_repeats);
  CHECK(scaled_tile.wrap);

  const auto animated_tile =
      decide({true, 0.00f, 0.00f, 1.00f, 1.00f}, 1.00f, 1.00f, true);
  CHECK(!animated_tile.uv_repeats);
  CHECK(animated_tile.wrap);

  const auto invalid_bounds =
      decide({false, -8.00f, -8.00f, 8.00f, 8.00f});
  CHECK(!invalid_bounds.uv_repeats);
  CHECK(!invalid_bounds.wrap);

  std::printf("  [ok] material UV sampler wrap decision\n");
  return 0;
}
