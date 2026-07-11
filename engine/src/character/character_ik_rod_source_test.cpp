#include "character/char_clip.h"

#include <array>
#include <cmath>
#include <iostream>
#include <string>

namespace {

void set_row(ghogx::milo_scene::Xfm& xfm, int row,
             float x, float y, float z) {
  xfm.rot[row][0] = x;
  xfm.rot[row][1] = y;
  xfm.rot[row][2] = z;
}

void set_pos(ghogx::milo_scene::Xfm& xfm, float x, float y, float z) {
  xfm.pos[0] = x;
  xfm.pos[1] = y;
  xfm.pos[2] = z;
}

ghogx::milo_scene::TransObj make_trans(const std::string& name,
                                       float x, float y, float z) {
  ghogx::milo_scene::TransObj trans;
  trans.name = name;
  set_pos(trans.local, x, y, z);
  set_pos(trans.world_stored, x, y, z);
  return trans;
}

void add_trans(ghogx::character::Character& character,
               const ghogx::milo_scene::TransObj& trans) {
  character.bones.push_back(trans);
  character.bind_bone_local.push_back(trans.local);
}

ghogx::character::CharIKRod make_identity_rod() {
  ghogx::character::CharIKRod rod;
  rod.name = "test.rod";
  rod.version = 2;
  rod.left_end = "left";
  rod.right_end = "right";
  rod.dest = "dest";
  rod.dest_pos = 0.25f;
  rod.xfm[0][0] = 1.0f;
  rod.xfm[1][1] = 1.0f;
  rod.xfm[2][2] = 1.0f;
  return rod;
}

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_matrix(const std::array<float, 16>& m, const char* label,
                   float x0, float x1, float x2,
                   float y0, float y1, float y2,
                   float z0, float z1, float z2,
                   float px, float py, float pz) {
  bool ok = true;
  ok &= near(m[0], x0, label);
  ok &= near(m[1], x1, label);
  ok &= near(m[2], x2, label);
  ok &= near(m[4], y0, label);
  ok &= near(m[5], y1, label);
  ok &= near(m[6], y2, label);
  ok &= near(m[8], z0, label);
  ok &= near(m[9], z1, label);
  ok &= near(m[10], z2, label);
  ok &= near(m[12], px, label);
  ok &= near(m[13], py, label);
  ok &= near(m[14], pz, label);
  return ok;
}

}  // namespace

int main() {
  using ghogx::character::Character;
  using ghogx::character::CharIKRod;
  using ghogx::character::apply_character_controllers;
  using ghogx::character::source_char_ik_rod_compute_world;

  Character character;
  add_trans(character, make_trans("left", 0.0f, 0.0f, 0.0f));
  add_trans(character, make_trans("right", 0.0f, 10.0f, 0.0f));
  add_trans(character, make_trans("dest", 0.0f, 0.0f, 0.0f));

  CharIKRod rod = make_identity_rod();
  std::array<float, 16> world{};
  bool ok = true;
  ok &= source_char_ik_rod_compute_world(rod, character, world);
  ok &= expect_matrix(world, "basic rod",
                      1.0f, 0.0f, 0.0f,
                      0.0f, 0.0f, 1.0f,
                      0.0f, -1.0f, 0.0f,
                      0.0f, 2.5f, 0.0f);

  auto side = make_trans("side", 0.0f, 0.0f, 0.0f);
  set_row(side.local, 2, 0.0f, 1.0f, 0.0f);
  add_trans(character, side);
  rod.dest_pos = 0.5f;
  rod.side_axis = "side";
  rod.vertical = true;
  ok &= source_char_ik_rod_compute_world(rod, character, world);
  ok &= expect_matrix(world, "vertical side-axis rod",
                      0.0f, 0.0f, -1.0f,
                      -1.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 0.0f,
                      0.0f, 5.0f, 0.0f);

  CharIKRod missing_dest = make_identity_rod();
  missing_dest.dest.clear();
  ok &= !source_char_ik_rod_compute_world(missing_dest, character, world);

  character.ik_rods.clear();
  character.ik_rods.push_back(make_identity_rod());
  apply_character_controllers(character, 0.0f, nullptr);
  const auto runtime = character.runtime_world_overrides.find("dest");
  ok &= runtime != character.runtime_world_overrides.end();
  if (runtime != character.runtime_world_overrides.end()) {
    ok &= expect_matrix(runtime->second, "controller writeback",
                        1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f,
                        0.0f, -1.0f, 0.0f,
                        0.0f, 2.5f, 0.0f);
  }

  return ok ? 0 : 1;
}
