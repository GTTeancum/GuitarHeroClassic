#include "character/char_clip.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

void set_pos(ghogx::milo_scene::Xfm& xfm, float x, float y, float z) {
  xfm.pos[0] = x;
  xfm.pos[1] = y;
  xfm.pos[2] = z;
}

ghogx::milo_scene::TransObj make_trans(const std::string& name,
                                       const std::string& parent = "") {
  ghogx::milo_scene::TransObj trans;
  trans.name = name;
  trans.parent = parent;
  set_pos(trans.local, 0.0f, 0.0f, 0.0f);
  set_pos(trans.world_stored, 0.0f, 0.0f, 0.0f);
  return trans;
}

void add_trans(ghogx::character::Character& character,
               const ghogx::milo_scene::TransObj& trans) {
  character.bones.push_back(trans);
  character.bind_bone_local.push_back(trans.local);
}

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::Character;
  using ghogx::character::CharHair;
  using ghogx::character::apply_character_controllers;

  Character character;
  add_trans(character, make_trans("parent"));
  add_trans(character, make_trans("root", "parent"));

  CharHair hair;
  hair.name = "test.hair";
  hair.simulate = false;
  hair.strands.resize(1);
  hair.strands[0].root = "root";
  hair.strands[0].root_mat[0] = 1.0f;
  hair.strands[0].root_mat[4] = 1.0f;
  hair.strands[0].root_mat[8] = 1.0f;
  hair.strands[0].points.resize(1);
  hair.strands[0].points[0].bone = "hair_tip";
  hair.strands[0].points[0].length = 2.0f;
  character.hairs.push_back(hair);

  apply_character_controllers(character, 0.0f, nullptr);

  const auto state_it = character.source_char_hair_runtime.find("test.hair");
  if (state_it == character.source_char_hair_runtime.end()) {
    std::cerr << "missing CharHair runtime state\n";
    return 1;
  }
  const auto& state = state_it->second;
  bool ok = true;
  ok &= state.reset == 0;
  ok &= state.strands.size() == 1;
  ok &= state.strands[0].points.size() == 1;
  if (!state.strands.empty() && !state.strands[0].points.empty()) {
    const auto& point = state.strands[0].points[0];
    ok &= near(point.pos[0], 0.0f, "reset-forced-sim x");
    ok &= near(point.pos[1], 0.0f, "reset-forced-sim y");
    ok &= point.pos[2] < -1.9f && point.pos[2] > -2.1f;
    if (!(point.pos[2] < -1.9f && point.pos[2] > -2.1f)) {
      std::cerr << "reset-forced-sim z got " << point.pos[2]
                << " want near -2\n";
    }
  }

  return ok ? 0 : 1;
}
