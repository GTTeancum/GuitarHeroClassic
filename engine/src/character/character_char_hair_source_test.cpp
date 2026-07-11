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

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_int(int got, int want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::Character;
  using ghogx::character::CharHair;
  using ghogx::character::apply_character_controllers;
  using ghogx::character::source_char_hair_freeze_pose_raw;
  using ghogx::character::source_char_hair_hookup_plan;
  using ghogx::character::source_char_hair_poll_decision;
  using ghogx::character::source_char_hair_simulate_loops_plan;

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
  auto& state = state_it->second;
  bool ok = true;
  ok &= state.use_post_proc;
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

  Character freeze_character;
  auto parent = make_trans("parent");
  set_pos(parent.local, 10.0f, 20.0f, 30.0f);
  add_trans(freeze_character, parent);
  add_trans(freeze_character, make_trans("root", "parent"));

  CharHair freeze_hair;
  freeze_hair.name = "freeze.hair";
  freeze_hair.strands.resize(1);
  freeze_hair.strands[0].root = "root";
  freeze_hair.strands[0].points.resize(1);
  ghogx::character::SourceCharHairRuntime freeze_state;
  freeze_state.strands.resize(1);
  freeze_state.strands[0].points.resize(1);
  freeze_state.strands[0].points[0].pos = {12.0f, 23.0f, 34.0f};
  const int freeze_writes =
      source_char_hair_freeze_pose_raw(freeze_character, freeze_hair,
                                       freeze_state);
  ok &= freeze_writes == 1;
  ok &= near(freeze_hair.strands[0].points[0].unk5c[0], 2.0f,
             "freeze-local x");
  ok &= near(freeze_hair.strands[0].points[0].unk5c[1], 3.0f,
             "freeze-local y");
  ok &= near(freeze_hair.strands[0].points[0].unk5c[2], 4.0f,
             "freeze-local z");

  const auto sync_decision =
      source_char_hair_poll_decision(true, true, false, 0, 0, 0.25f);
  ok &= expect_bool(sync_decision.hookup, true, "poll sync hookup");
  ok &= expect_bool(sync_decision.do_reset, false, "poll sync no reset");
  ok &= expect_bool(sync_decision.simulate_loops, true, "poll sync loops");
  ok &= expect_bool(sync_decision.simulate_zero_time, false,
                   "poll sync not zero time");

  const auto teleport_decision =
      source_char_hair_poll_decision(true, false, true, 0, 0, 0.25f);
  ok &= expect_bool(teleport_decision.teleported_reset, true,
                    "poll teleport marks reset");
  ok &= expect_bool(teleport_decision.do_reset, true,
                    "poll teleport reset");
  ok &= expect_int(teleport_decision.reset_count, 1,
                   "poll teleport reset count");
  ok &= expect_bool(teleport_decision.simulate_loops, true,
                    "poll teleport still simulates after reset");

  const auto lod_decision =
      source_char_hair_poll_decision(true, false, false, 1, 3, 0.25f);
  ok &= expect_bool(lod_decision.do_reset, true, "poll lod reset");
  ok &= expect_int(lod_decision.reset_count, 0, "poll lod reset count");
  ok &= expect_bool(lod_decision.return_after_reset, true,
                    "poll lod returns after reset");
  ok &= expect_bool(lod_decision.simulate_loops, false,
                    "poll lod skips loops");

  const auto zero_time_decision =
      source_char_hair_poll_decision(false, false, false, 0, 0, 0.0f);
  ok &= expect_bool(zero_time_decision.hookup, false,
                    "poll no owner no hookup");
  ok &= expect_bool(zero_time_decision.simulate_loops, false,
                    "poll zero delta no loops");
  ok &= expect_bool(zero_time_decision.simulate_zero_time, true,
                    "poll zero delta zero time");

  const auto managed_hookup =
      source_char_hair_hookup_plan(true, {"head.collide", "neck.collide"});
  ok &= expect_bool(managed_hookup.returned_for_managed_hookup, true,
                    "managed hookup returns");
  ok &= expect_bool(managed_hookup.called_overloaded_hookup, false,
                    "managed hookup skips overloaded hookup");
  ok &= expect_int(static_cast<int>(managed_hookup.collected_collides.size()),
                   0, "managed hookup collects none");

  const auto hookup =
      source_char_hair_hookup_plan(false, {"head.collide", "neck.collide"});
  ok &= expect_bool(hookup.returned_for_managed_hookup, false,
                    "hookup not managed");
  ok &= expect_bool(hookup.called_overloaded_hookup, true,
                    "hookup calls overloaded hookup");
  ok &= expect_int(static_cast<int>(hookup.collected_collides.size()), 2,
                   "hookup collide count");
  ok &= expect_bool(hookup.collected_collides[0] == "head.collide",
                    true, "hookup collide order");

  const auto skipped_loops =
      source_char_hair_simulate_loops_plan(false, 2, 3, 4, 60.0f);
  ok &= expect_bool(skipped_loops.entered, false,
                    "simulate loops skips disabled sim");
  const auto empty_loops =
      source_char_hair_simulate_loops_plan(true, 0, 3, 4, 60.0f);
  ok &= expect_bool(empty_loops.entered, false,
                    "simulate loops skips empty strands");
  const auto loops =
      source_char_hair_simulate_loops_plan(true, 2, 3, 4, 30.0f);
  ok &= expect_bool(loops.entered, true, "simulate loops enters");
  ok &= expect_int(loops.collide_maintenance_count, 3,
                   "simulate loops collide maintenance");
  ok &= expect_int(loops.simulate_internal_calls, 4,
                   "simulate loops internal calls");
  ok &= near(loops.fps, 30.0f, "simulate loops fps");

  return ok ? 0 : 1;
}
