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

bool expect_int(int got, int want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_size(size_t got, size_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_string(const std::string& got, const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::cerr << label << " got '" << got << "' want '" << want << "'\n";
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
  using ghogx::character::SourceCharIKRodPollDeps;
  using ghogx::character::apply_character_controllers;
  using ghogx::character::source_char_ik_rod_copy_plan;
  using ghogx::character::source_char_ik_rod_compute_world;
  using ghogx::character::source_char_ik_rod_default_state;
  using ghogx::character::source_char_ik_rod_handler_plan;
  using ghogx::character::source_char_ik_rod_load_plan;
  using ghogx::character::source_char_ik_rod_poll_deps;
  using ghogx::character::source_char_ik_rod_prop_sync_plan;

  Character character;
  add_trans(character, make_trans("left", 0.0f, 0.0f, 0.0f));
  add_trans(character, make_trans("right", 0.0f, 10.0f, 0.0f));
  add_trans(character, make_trans("dest", 0.0f, 0.0f, 0.0f));

  bool ok = true;
  const auto defaults = source_char_ik_rod_default_state();
  ok &= expect_int(defaults.left_end_empty ? 1 : 0, 1,
                   "CharIKRod default left end");
  ok &= near(defaults.dest_pos, 0.5f, "CharIKRod default dest pos");
  ok &= expect_int(defaults.vertical ? 1 : 0, 0,
                   "CharIKRod default vertical");
  ok &= expect_int(defaults.xfm_identity ? 1 : 0, 1,
                   "CharIKRod default xfm");

  const auto load_v2 = source_char_ik_rod_load_plan(2);
  ok &= expect_int(load_v2.known_revision ? 1 : 0, 1,
                   "CharIKRod Load v2 known");
  ok &= expect_size(load_v2.read_order.size(), 8,
                    "CharIKRod Load row count");
  ok &= expect_string(load_v2.read_order[0], "Hmx::Object",
                      "CharIKRod Load object");
  ok &= expect_string(load_v2.read_order[3], "mDestPos",
                      "CharIKRod Load dest pos");
  ok &= expect_string(load_v2.read_order.back(), "mXfm",
                      "CharIKRod Load xfm");
  ok &= expect_int(source_char_ik_rod_load_plan(3).known_revision ? 1 : 0,
                   0, "CharIKRod Load rejects high revision");

  const auto copy_plan = source_char_ik_rod_copy_plan();
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "CharIKRod Copy superclass");
  ok &= expect_size(copy_plan.copied_members.size(), 7,
                    "CharIKRod Copy member count");
  ok &= expect_string(copy_plan.copied_members[0], "mLeftEnd",
                      "CharIKRod Copy first member");
  ok &= expect_string(copy_plan.copied_members.back(), "mXfm",
                      "CharIKRod Copy last member");

  const auto handlers = source_char_ik_rod_handler_plan();
  ok &= expect_string(handlers.superclasses[0], "Hmx::Object",
                      "CharIKRod handler superclass");
  ok &= expect_int(handlers.check, 0xAF, "CharIKRod handler check");

  const auto props = source_char_ik_rod_prop_sync_plan();
  ok &= expect_size(props.modify_alt_properties.size(), 6,
                    "CharIKRod prop count");
  ok &= expect_string(props.modify_alt_properties[0], "left_end",
                      "CharIKRod prop left end");
  ok &= expect_string(props.modify_alt_properties.back(), "dest",
                      "CharIKRod prop dest");
  ok &= expect_string(props.modify_actions[3], "SyncBones",
                      "CharIKRod prop SyncBones action");

  CharIKRod rod = make_identity_rod();
  rod.side_axis = "side";
  SourceCharIKRodPollDeps deps;
  source_char_ik_rod_poll_deps(deps, rod);
  ok &= expect_size(deps.change.size(), 1, "CharIKRod PollDeps change count");
  ok &= expect_string(deps.change[0], "dest", "CharIKRod PollDeps dest");
  ok &= expect_size(deps.changed_by.size(), 3,
                    "CharIKRod PollDeps changed_by count");
  ok &= expect_string(deps.changed_by[0], "left",
                      "CharIKRod PollDeps left");
  ok &= expect_string(deps.changed_by[1], "right",
                      "CharIKRod PollDeps right");
  ok &= expect_string(deps.changed_by[2], "side",
                      "CharIKRod PollDeps side axis");
  rod.side_axis.clear();

  std::array<float, 16> world{};
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
  apply_character_controllers(character, 0.0f);
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
