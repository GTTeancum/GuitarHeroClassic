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
  using ghogx::character::source_char_hair_enter_plan;
  using ghogx::character::source_char_hair_freeze_pose_plan;
  using ghogx::character::source_char_hair_freeze_pose_raw;
  using ghogx::character::source_char_hair_hookup_plan;
  using ghogx::character::source_char_hair_load_plan;
  using ghogx::character::source_char_hair_point_load_plan;
  using ghogx::character::source_char_hair_poll_decision;
  using ghogx::character::source_char_hair_simulate_loops_plan;
  using ghogx::character::source_char_hair_strand_load_plan;

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

  const auto point_v2 = source_char_hair_point_load_plan(2);
  ok &= expect_bool(point_v2.known_revision, true,
                    "point load accepts v2");
  ok &= expect_bool(point_v2.read_order[3] == "legacyCollideType", true,
                    "point v2 reads legacy collide type");
  ok &= expect_bool(point_v2.read_order[4] == "legacyCollisionName", true,
                    "point v2 reads legacy collision name");
  ok &= expect_bool(point_v2.branches.back() == "zeroLastZ", true,
                    "point load clears runtime z");

  const auto point_v6 = source_char_hair_point_load_plan(6);
  ok &= expect_bool(point_v6.known_revision, true,
                    "point load accepts v6");
  ok &= expect_bool(point_v6.read_order[5] == "addToRadius", true,
                    "point v6 reads radius add");
  ok &= expect_bool(point_v6.read_order[6] == "legacyCollisionName", true,
                    "point v6 reads legacy collision string");
  ok &= expect_bool(point_v6.read_order[7] == "legacySideLengthInt0", true,
                    "point v6 reads first legacy side int");
  ok &= expect_bool(point_v6.branches[0] ==
                        "addToRadiusAppliesToRadiusAndOuterRadius",
                    true, "point v6 adds radius to both radii");

  const auto point_v8 = source_char_hair_point_load_plan(8);
  ok &= expect_bool(point_v8.known_revision, true,
                    "point load accepts v8");
  ok &= expect_bool(point_v8.read_order[5] == "addToRadius", true,
                    "point v8 reads radius add");
  ok &= expect_bool(point_v8.read_order[6] == "sideLengthEnabled", true,
                    "point v8 reads side enabled");
  ok &= expect_bool(point_v8.read_order[7] == "sideLength", true,
                    "point v8 reads side length");
  ok &= expect_bool(point_v8.branches[1] ==
                        "disabledSideLengthForcesMinusOne",
                    true, "point v8 disabled side length branch");

  const auto point_v10 = source_char_hair_point_load_plan(10);
  ok &= expect_bool(point_v10.known_revision, true,
                    "point load accepts v10");
  ok &= expect_bool(point_v10.read_order.back() == "unk5c", true,
                    "point v10 reads frozen-pose vector");

  const auto point_bad = source_char_hair_point_load_plan(12);
  ok &= expect_bool(point_bad.known_revision, false,
                    "point load rejects unknown revision");
  ok &= expect_int(static_cast<int>(point_bad.read_order.size()), 0,
                   "point bad revision has no reads");

  const auto strand_v2 = source_char_hair_strand_load_plan(2);
  ok &= expect_bool(strand_v2.known_revision, true,
                    "strand load accepts v2");
  ok &= expect_bool(strand_v2.read_order[2] == "mPoints", true,
                    "strand reads points");
  ok &= expect_bool(strand_v2.branches[0] == "mHookupFlags=0", true,
                    "strand v2 defaults hookup flags");

  const auto strand_v3 = source_char_hair_strand_load_plan(3);
  ok &= expect_bool(strand_v3.known_revision, true,
                    "strand load accepts v3");
  ok &= expect_bool(strand_v3.read_order.back() == "mHookupFlags", true,
                    "strand v3 reads hookup flags");

  const auto load_v7 = source_char_hair_load_plan(7);
  ok &= expect_bool(load_v7.known_revision, true,
                    "hair load accepts v7");
  ok &= expect_bool(load_v7.branches[0] == "mMinSlack=0", true,
                    "hair v7 defaults min slack");
  ok &= expect_bool(load_v7.branches[1] == "mMaxSlack=0", true,
                    "hair v7 defaults max slack");
  ok &= expect_bool(load_v7.read_order.back() == "mSimulate", true,
                    "hair v7 ends at simulate");

  const auto load_v11 = source_char_hair_load_plan(11);
  ok &= expect_bool(load_v11.known_revision, true,
                    "hair load accepts v11");
  ok &= expect_bool(load_v11.read_order[8] == "mMinSlack", true,
                    "hair v11 reads min slack");
  ok &= expect_bool(load_v11.read_order[9] == "mMaxSlack", true,
                    "hair v11 reads max slack");
  ok &= expect_bool(load_v11.read_order.back() == "mWind", true,
                    "hair v11 reads wind");

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

  const auto enter_plan =
      source_char_hair_enter_plan(false, {"head.collide", "neck.collide"});
  ok &= expect_int(enter_plan.next_reset, 1, "enter sets reset");
  ok &= expect_bool(enter_plan.called_rnd_pollable_enter, true,
                    "enter calls RndPollable Enter");
  ok &= expect_bool(enter_plan.hookup.called_overloaded_hookup, true,
                    "enter calls hookup");
  ok &= expect_int(static_cast<int>(enter_plan.hookup.collected_collides.size()),
                   2, "enter hookup collide count");

  const auto managed_enter_plan =
      source_char_hair_enter_plan(true, {"head.collide"});
  ok &= expect_bool(managed_enter_plan.hookup.returned_for_managed_hookup, true,
                    "managed enter returns from hookup");

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

  const auto freeze_plan =
      source_char_hair_freeze_pose_plan(true, 2, 3);
  ok &= expect_bool(freeze_plan.called_hookup, true,
                    "freeze pose calls hookup");
  ok &= expect_bool(freeze_plan.simulate_loops.entered, true,
                    "freeze pose simulates when enabled");
  ok &= expect_int(freeze_plan.simulate_loops.simulate_internal_calls, 200,
                   "freeze pose sim loop count");
  ok &= near(freeze_plan.simulate_loops.fps, 60.0f, "freeze pose sim fps");
  ok &= expect_bool(freeze_plan.restored_simulate, true,
                    "freeze pose restores simulate");
  ok &= expect_bool(freeze_plan.restored_simulate_value, true,
                    "freeze pose restore value");
  ok &= expect_bool(freeze_plan.called_freeze_pose_raw, true,
                    "freeze pose calls raw freeze");

  const auto freeze_disabled =
      source_char_hair_freeze_pose_plan(false, 2, 3);
  ok &= expect_bool(freeze_disabled.simulate_loops.entered, false,
                    "freeze pose respects disabled sim");
  ok &= expect_bool(freeze_disabled.restored_simulate_value, false,
                    "freeze pose restores disabled sim");

  return ok ? 0 : 1;
}
