#include "render/milo_scene_renderer.h"

#include <cmath>
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
using ghogx::render::source_texgen_xfm_2d;

bool near(float actual, float expected) {
  return std::fabs(actual - expected) <= 1.0e-5f;
}

MiloSceneRenderer::MaterialUvSamplerDecision decide(
    MiloSceneRenderer::MaterialUvBounds bounds, float scale_u = 1.0f,
    float scale_v = 1.0f, bool material_tex_anim = false) {
  return choose_material_uv_sampler(bounds, scale_u, scale_v, material_tex_anim);
}

}  // namespace

int main() {
  const auto plain =
      source_texgen_xfm_2d(0, 2.0f, 0.25f, -0.5f, 3.0f, -0.2f, 0.4f);
  CHECK(near(plain.m00, 2.0f));
  CHECK(near(plain.m01, 0.25f));
  CHECK(near(plain.m10, -0.5f));
  CHECK(near(plain.m11, 3.0f));
  CHECK(near(plain.tu, -0.2f));
  CHECK(near(plain.tv, 0.4f));

  const auto character_wall =
      source_texgen_xfm_2d(1, 2.0f, 0.0f, 0.0f, 1.0f, -1.2495f, 0.0f);
  CHECK(near(character_wall.m00, 2.0f));
  CHECK(near(character_wall.m11, 1.0f));
  CHECK(near(character_wall.tu, 1.999f));
  CHECK(near(character_wall.tv, 0.0f));

  const auto rotated_star =
      source_texgen_xfm_2d(1, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
  CHECK(near(rotated_star.m00, 0.0f));
  CHECK(near(rotated_star.m01, 1.0f));
  CHECK(near(rotated_star.m10, -1.0f));
  CHECK(near(rotated_star.m11, 0.0f));
  CHECK(near(rotated_star.tu, 1.0f));
  CHECK(near(rotated_star.tv, 0.0f));

  const auto backdrop =
      source_texgen_xfm_2d(1, 1.0f, 0.0f, 0.0f, 0.95f, 0.0f, 0.03f);
  CHECK(near(backdrop.tu, 0.0f));
  CHECK(near(backdrop.tv, 0.0535f));

  const auto spraypaint =
      source_texgen_xfm_2d(1, 0.95f, 0.0f, 0.0f, 1.0f, 0.009f, 0.02f);
  CHECK(near(spraypaint.tu, 0.01645f));
  CHECK(near(spraypaint.tv, 0.02f));

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

  std::printf("  [ok] retail texture-generator transform and sampler decision\n");
  return 0;
}
