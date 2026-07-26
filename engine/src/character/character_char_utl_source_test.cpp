#include "character/char_clip.h"

#include <array>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

bool expect_bool(bool got, bool expected, const char* label) {
  if (got == expected) return true;
  std::cerr << label << ": got " << (got ? "true" : "false")
            << " expected " << (expected ? "true" : "false") << "\n";
  return false;
}

bool expect_string(const std::string& got, const std::string& expected,
                   const char* label) {
  if (got == expected) return true;
  std::cerr << label << ": got '" << got << "' expected '" << expected << "'\n";
  return false;
}

bool expect_present(bool got, const char* label) {
  if (got) return true;
  std::cerr << label << ": missing result\n";
  return false;
}

bool expect_int(int got, int expected, const char* label) {
  if (got == expected) return true;
  std::cerr << label << ": got " << got << " expected " << expected << "\n";
  return false;
}

bool expect_size(size_t got, size_t expected, const char* label) {
  if (got == expected) return true;
  std::cerr << label << ": got " << got << " expected " << expected << "\n";
  return false;
}

bool expect_near(float got, float expected, const char* label) {
  if (std::fabs(got - expected) <= 0.0001f) return true;
  std::cerr << label << ": got " << got << " expected near " << expected
            << "\n";
  return false;
}

ghogx::character::SourceCharUtlObject obj(
    const std::string& name,
    ghogx::character::SourceCharUtlObjectKind kind,
    int mesh_bone_count = 0,
    const std::string& char_bone_transform = std::string()) {
  ghogx::character::SourceCharUtlObject object;
  object.name = name;
  object.kind = kind;
  object.mesh_bone_count = mesh_bone_count;
  object.char_bone_transform = char_bone_transform;
  return object;
}

}  // namespace

int main() {
  constexpr float kPi = 3.14159265358979323846f;

  using ghogx::character::SourceCharUtlBoneTransResult;
  using ghogx::character::SourceCharUtlClipPredictFrame;
  using ghogx::character::SourceCharUtlClipPredictState;
  using ghogx::character::SourceCharUtlInitPlan;
  using ghogx::character::SourceCharUtlMergeBone;
  using ghogx::character::SourceCharUtlMergeResult;
  using ghogx::character::SourceCharUtlObject;
  using ghogx::character::SourceCharUtlObjectKind;
  using ghogx::character::SourceCharUtlTransformRow;
  using ghogx::character::kSourceCharBonesTypeEnd;
  using ghogx::character::kSourceCharBonesTypeRotX;
  using ghogx::character::kSourceCharBonesTypeRotY;
  using ghogx::character::kSourceCharBonesTypeRotZ;
  using ghogx::character::source_char_utl_bone_saver_capture_names;
  using ghogx::character::source_char_utl_clip_predict;
  using ghogx::character::source_char_utl_find_bone;
  using ghogx::character::source_char_utl_find_bone_trans;
  using ghogx::character::source_char_utl_init_plan;
  using ghogx::character::source_char_utl_is_animatable;
  using ghogx::character::source_char_utl_merge_bones;
  using ghogx::character::source_char_utl_name_with_suffix;
  using ghogx::character::source_char_utl_reset_hair_names;
  using ghogx::character::source_char_utl_reset_transform_names;
  using ghogx::character::source_char_walk_facing_sample;

  bool ok = true;

  ok &= expect_string(source_char_utl_name_with_suffix("bone_head.mesh", "cb"),
                      "bone_head.cb", "suffix replaces existing extension");
  ok &= expect_string(source_char_utl_name_with_suffix("bone_head", "cb"),
                      "bone_head.cb", "suffix appends when extension missing");
  ok &= expect_string(source_char_utl_name_with_suffix("face.bone.mesh", "cb"),
                      "face.bone.cb", "suffix replaces final extension only");

  std::vector<SourceCharUtlObject> objects = {
      obj("bone_head.cb", SourceCharUtlObjectKind::kCharBone, 0,
          "bone_head_from_cb.trans"),
      obj("bone_head.trans", SourceCharUtlObjectKind::kTransformable),
      obj("bone_head.mesh", SourceCharUtlObjectKind::kMesh, 4),
      obj("bone_spine.trans", SourceCharUtlObjectKind::kTransformable),
      obj("bone_pelvis.mesh", SourceCharUtlObjectKind::kMesh, 2),
      obj("spot_neck_fret20.mesh", SourceCharUtlObjectKind::kMesh, 0),
  };

  const auto bone = source_char_utl_find_bone("bone_head.mesh", objects);
  ok &= expect_present(bone.has_value(), "FindBone returns .cb row");
  if (bone) {
    ok &= expect_string(bone->name, "bone_head.cb", "FindBone .cb name");
  }

  const std::optional<SourceCharUtlBoneTransResult> cb_trans =
      source_char_utl_find_bone_trans("bone_head.mesh", objects);
  ok &= expect_present(cb_trans.has_value(), "FindBoneTrans .cb hit");
  if (cb_trans) {
    ok &= expect_string(cb_trans->lookup_name, "bone_head.cb",
                        "FindBoneTrans .cb lookup");
    ok &= expect_string(cb_trans->resolved_name, "bone_head_from_cb.trans",
                        "FindBoneTrans returns CharBone transform");
    ok &= expect_bool(cb_trans->via_char_bone, true,
                      "FindBoneTrans marks CharBone path");
  }

  const std::optional<SourceCharUtlBoneTransResult> trans =
      source_char_utl_find_bone_trans("bone_spine.mesh", objects);
  ok &= expect_present(trans.has_value(), "FindBoneTrans .trans fallback");
  if (trans) {
    ok &= expect_string(trans->lookup_name, "bone_spine.trans",
                        "FindBoneTrans .trans lookup");
    ok &= expect_string(trans->resolved_name, "bone_spine.trans",
                        "FindBoneTrans returns .trans fallback");
    ok &= expect_bool(trans->via_char_bone, false,
                      "FindBoneTrans .trans is not CharBone path");
  }

  const std::optional<SourceCharUtlBoneTransResult> mesh =
      source_char_utl_find_bone_trans("bone_pelvis", objects);
  ok &= expect_present(mesh.has_value(), "FindBoneTrans .mesh fallback");
  if (mesh) {
    ok &= expect_string(mesh->lookup_name, "bone_pelvis.mesh",
                        "FindBoneTrans .mesh lookup");
    ok &= expect_string(mesh->resolved_name, "bone_pelvis.mesh",
                        "FindBoneTrans returns .mesh fallback");
  }

  const std::optional<SourceCharUtlBoneTransResult> missing =
      source_char_utl_find_bone_trans("bone_missing.mesh", objects);
  ok &= expect_bool(missing.has_value(), false, "FindBoneTrans missing row");

  ok &= expect_bool(
      source_char_utl_is_animatable(obj(
          "plain.trans", SourceCharUtlObjectKind::kTransformable)),
      true, "plain transform is animatable");
  ok &= expect_bool(
      source_char_utl_is_animatable(obj(
          "static_card.mesh", SourceCharUtlObjectKind::kMesh, 0)),
      true, "mesh without skin bones is animatable");
  ok &= expect_bool(
      source_char_utl_is_animatable(obj(
          "skinned.mesh", SourceCharUtlObjectKind::kMesh, 3)),
      false, "mesh with bones is not animatable");
  ok &= expect_bool(
      source_char_utl_is_animatable(obj(
          "shot.cam", SourceCharUtlObjectKind::kCamera)),
      false, "camera is not animatable");
  ok &= expect_bool(
      source_char_utl_is_animatable(obj(
          "hair_collide.trans", SourceCharUtlObjectKind::kCharCollide)),
      false, "CharCollide is not animatable");
  ok &= expect_bool(
      source_char_utl_is_animatable(obj(
          "cuff.trans", SourceCharUtlObjectKind::kCharCuff)),
      false, "CharCuff is not animatable");
  ok &= expect_bool(
      source_char_utl_is_animatable(obj(
          "lod0.grp", SourceCharUtlObjectKind::kDirectory)),
      false, "RndDir is not animatable");
  ok &= expect_bool(
      source_char_utl_is_animatable(obj(
          "spot_neck_fret20.mesh", SourceCharUtlObjectKind::kMesh, 0)),
      false, "spot_ transform is not animatable");

  const std::vector<SourceCharUtlMergeBone> source_bones = {
      {"bone_hand.cb", "bone_arm.cb", 0x1, 0x2, kSourceCharBonesTypeRotZ,
       0x4},
      {"bone_arm.cb", "", 0x0, 0x0, kSourceCharBonesTypeEnd, 0x0},
      {"bone_missing.cb", "bone_arm.cb", 0x1, 0x0,
       kSourceCharBonesTypeRotX, 0x4},
      {"bone_conflict.cb", "bone_arm.cb", 0x0, 0x0,
       kSourceCharBonesTypeRotX, 0x4},
      {"bone_targeted.cb", "bone_missing_target.cb", 0x0, 0x0,
       kSourceCharBonesTypeEnd, 0x0},
  };
  const std::vector<SourceCharUtlMergeBone> dest_bones = {
      {"bone_hand.cb", "", 0x10, 0x20, kSourceCharBonesTypeEnd, 0x40},
      {"bone_arm.cb", "", 0x0, 0x0, kSourceCharBonesTypeEnd, 0x0},
      {"bone_conflict.cb", "bone_other.cb", 0x0, 0x0,
       kSourceCharBonesTypeRotY, 0x80},
      {"bone_targeted.cb", "", 0x0, 0x0, kSourceCharBonesTypeEnd, 0x0},
  };
  const SourceCharUtlMergeResult merge =
      source_char_utl_merge_bones(source_bones, dest_bones, 0x8);
  ok &= expect_size(merge.dest_bones.size(), 4, "MergeBones dest count");
  ok &= expect_string(merge.dest_bones[0].target, "bone_arm.cb",
                      "MergeBones assigns missing target from dest CharBone");
  ok &= expect_int(merge.dest_bones[0].position_context, 0x28,
                   "MergeBones scale branch uses dest scale context");
  ok &= expect_int(merge.dest_bones[0].scale_context, 0x20,
                   "MergeBones preserves dest scale context");
  ok &= expect_int(merge.dest_bones[0].rotation_type,
                   kSourceCharBonesTypeRotZ,
                   "MergeBones assigns missing rotation type");
  ok &= expect_int(merge.dest_bones[0].rotation_context, 0x48,
                   "MergeBones ORs rotation context");
  ok &= expect_size(merge.warnings.size(), 6, "MergeBones warning count");
  ok &= expect_string(merge.warnings[0].code, "missing_bone",
                      "MergeBones missing source target warning");
  ok &= expect_string(merge.warnings[1].code, "missing_bone",
                      "MergeBones repeated missing position warning");
  ok &= expect_string(merge.warnings[2].code, "missing_bone",
                      "MergeBones repeated missing rotation warning");
  ok &= expect_string(merge.warnings[3].code, "different_target",
                      "MergeBones target mismatch warning");
  ok &= expect_string(merge.warnings[4].code, "different_rotation",
                      "MergeBones rotation mismatch warning");
  ok &= expect_string(merge.warnings[5].code, "missing_target",
                      "MergeBones missing target row warning");

  const std::vector<SourceCharUtlTransformRow> transforms = {
      {"bone_root.trans", false},
      {"prop_guitar.trans", false},
      {"bone_hand.trans", true},
      {"spot_neck_fret20.mesh", false},
  };
  const std::vector<std::string> saved =
      source_char_utl_bone_saver_capture_names(transforms);
  ok &= expect_size(saved.size(), 2, "BoneSaver captures bone_ rows");
  ok &= expect_string(saved[0], "bone_root.trans",
                      "BoneSaver first bone row");
  ok &= expect_string(saved[1], "bone_hand.trans",
                      "BoneSaver keeps child bone row");
  const std::vector<std::string> reset_transforms =
      source_char_utl_reset_transform_names(transforms);
  ok &= expect_size(reset_transforms.size(), 3,
                    "ResetTransform resets top-level transforms");
  ok &= expect_string(reset_transforms[0], "bone_root.trans",
                      "ResetTransform includes root bone");
  ok &= expect_string(reset_transforms[1], "prop_guitar.trans",
                      "ResetTransform includes root prop");
  ok &= expect_string(reset_transforms[2], "spot_neck_fret20.mesh",
                      "ResetTransform includes root spot");
  const std::vector<std::string> reset_hair =
      source_char_utl_reset_hair_names({"hair_front1.hair", "scarf.hair"});
  ok &= expect_size(reset_hair.size(), 2, "ResetHair enters every hair row");
  ok &= expect_string(reset_hair[0], "hair_front1.hair",
                      "ResetHair first row");
  ok &= expect_string(reset_hair[1], "scarf.hair", "ResetHair second row");

  const SourceCharUtlInitPlan init_plan = source_char_utl_init_plan();
  ok &= expect_size(init_plan.registered_functions.size(), 2,
                    "CharUtlInit registration count");
  ok &= expect_string(init_plan.registered_functions[0], "reset_hair",
                      "CharUtlInit reset_hair registration");
  ok &= expect_string(init_plan.registered_functions[1], "char_merge_bones",
                      "CharUtlInit char_merge_bones registration");
  ok &= expect_size(init_plan.reset_hair_handler_steps.size(), 1,
                    "OnResetHair handler step count");
  ok &= expect_string(init_plan.reset_hair_handler_steps[0],
                      "CharUtlResetHair(Obj<Character>(1))",
                      "OnResetHair calls source reset helper");
  ok &= expect_size(init_plan.char_merge_bones_handler_steps.size(), 6,
                    "OnCharMergeBones handler step count");
  ok &= expect_string(init_plan.char_merge_bones_handler_steps[0],
                      "FilePath(Str(1))",
                      "OnCharMergeBones reads path arg");
  ok &= expect_string(init_plan.char_merge_bones_handler_steps[4],
                      "CharUtlMergeBones",
                      "OnCharMergeBones calls merge helper");
  ok &= expect_int(init_plan.char_merge_bones_deletes_loaded_dir ? 1 : 0, 1,
                   "OnCharMergeBones deletes loaded dir");

  SourceCharUtlClipPredictState predict_state;
  predict_state.pos = {10.0f, 0.0f, 1.0f};
  predict_state.ang = kPi * 0.5f;
  const SourceCharUtlClipPredictFrame predict_first{{1.0f, 2.0f, 3.0f},
                                                    0.0f};
  const SourceCharUtlClipPredictFrame predict_second{{2.0f, 2.0f, 5.0f},
                                                     kPi * 0.5f};
  source_char_utl_clip_predict(predict_state, predict_first, predict_second);
  ok &= expect_near(predict_state.pos[0], 10.0f,
                    "ClipPredict rotated position x");
  ok &= expect_near(predict_state.pos[1], 1.0f,
                    "ClipPredict rotated position y");
  ok &= expect_near(predict_state.pos[2], 3.0f,
                    "ClipPredict position z");
  ok &= expect_near(predict_state.ang, kPi, "ClipPredict angle advance");
  ok &= expect_near(predict_state.last_pos[0], 2.0f,
                    "ClipPredict last position x");
  ok &= expect_near(predict_state.last_pos[1], 2.0f,
                    "ClipPredict last position y");
  ok &= expect_near(predict_state.last_pos[2], 5.0f,
                    "ClipPredict last position z");
  ok &= expect_near(predict_state.last_ang, kPi * 0.5f,
                    "ClipPredict last angle");

  SourceCharUtlClipPredictState wrap_state;
  wrap_state.ang = 3.0f;
  source_char_utl_clip_predict(wrap_state, {{{0.0f, 0.0f, 0.0f}}, 3.0f},
                               {{{0.0f, 0.0f, 0.0f}}, -3.0f});
  ok &= expect_near(wrap_state.ang, -3.0f, "ClipPredict wraps angle");

  ghogx::character::ClipChannel facing_pos;
  facing_pos.type = ghogx::character::ClipChannel::kPos;
  facing_pos.bone_name = "bone_facing.mesh";
  facing_pos.pos[0] = 4.0f;
  facing_pos.pos[1] = -2.0f;
  facing_pos.pos[2] = 1.5f;
  ghogx::character::ClipChannel facing_rot;
  facing_rot.type = ghogx::character::ClipChannel::kRotZ;
  facing_rot.bone_name = "bone_facing.mesh";
  facing_rot.angle = 0.75f;
  const auto facing =
      source_char_walk_facing_sample({facing_pos, facing_rot});
  ok &= expect_present(facing.has_value(),
                       "CharWalk facing channel sample");
  if (facing) {
    ok &= expect_near(facing->facing_pos[0], 4.0f,
                      "CharWalk facing position x");
    ok &= expect_near(facing->facing_pos[1], -2.0f,
                      "CharWalk facing position y");
    ok &= expect_near(facing->facing_pos[2], 1.5f,
                      "CharWalk facing position z");
    ok &= expect_near(facing->facing_rot, 0.75f,
                      "CharWalk facing rotation remains radians");
  }
  ok &= expect_bool(
      source_char_walk_facing_sample({facing_rot}).has_value(), false,
      "CharWalk facing sample requires authored position");

  return ok ? 0 : 1;
}
