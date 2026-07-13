#include "character/char_clip.h"

#include <array>
#include <cmath>
#include <iostream>

namespace {

void set_identity(ghogx::milo_scene::Xfm& xfm) {
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) xfm.rot[r][c] = r == c ? 1.0f : 0.0f;
  }
  xfm.pos[0] = 0.0f;
  xfm.pos[1] = 0.0f;
  xfm.pos[2] = 0.0f;
}

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << ": got " << got << " want " << want << "\n";
  return false;
}

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << ": got " << (got ? "true" : "false")
            << " want " << (want ? "true" : "false") << "\n";
  return false;
}

bool expect_world_pos(const std::array<float, 16>& world,
                      float x, float y, float z,
                      const char* label) {
  bool ok = true;
  ok &= near(world[12], x, label);
  ok &= near(world[13], y, label);
  ok &= near(world[14], z, label);
  return ok;
}

}  // namespace

int main() {
  using ghogx::character::CharBoneOffset;
  using ghogx::character::source_char_bone_offset_apply_to_local;
  using ghogx::character::source_char_bone_offset_poll_world;
  using ghogx::character::source_char_bone_offset_save_plan;

  CharBoneOffset offset;
  offset.name = "test.offset";
  offset.version = 1;
  offset.dest = "bone_hat.trans";
  offset.offset[0] = 1.0f;
  offset.offset[1] = 2.0f;
  offset.offset[2] = 3.0f;

  ghogx::milo_scene::Xfm local;
  set_identity(local);
  local.pos[0] = 4.0f;
  local.pos[1] = 5.0f;
  local.pos[2] = 6.0f;

  const std::array<float, 16> parent_world =
      {1, 0, 0, 0,
       0, 1, 0, 0,
       0, 0, 1, 0,
       10, 20, 30, 1};

  std::array<float, 16> world{};
  bool ok = true;
  ok &= expect_bool(source_char_bone_offset_save_plan().save_id == 0x5E,
                    true, "CharBoneOffset save id");
  ok &= expect_bool(source_char_bone_offset_poll_world(
                        offset, false, true, local, parent_world, world),
                    false, "Poll returns false without destination");
  ok &= expect_bool(source_char_bone_offset_poll_world(
                        offset, true, false, local, parent_world, world),
                    false, "Poll returns false without parent");
  ok &= expect_bool(source_char_bone_offset_poll_world(
                        offset, true, true, local, parent_world, world),
                    true, "Poll returns true with destination and parent");
  ok &= expect_world_pos(world, 15.0f, 27.0f, 39.0f,
                         "Poll multiplies offset local by parent world");
  ok &= near(world[0], 1.0f, "Poll keeps local row 0");
  ok &= near(world[5], 1.0f, "Poll keeps local row 1");
  ok &= near(world[10], 1.0f, "Poll keeps local row 2");

  source_char_bone_offset_apply_to_local(offset, local);
  ok &= near(local.pos[0], 5.0f, "ApplyToLocal x");
  ok &= near(local.pos[1], 7.0f, "ApplyToLocal y");
  ok &= near(local.pos[2], 9.0f, "ApplyToLocal z");

  return ok ? 0 : 1;
}
