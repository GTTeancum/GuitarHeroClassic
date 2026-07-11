#include "character/char_clip.h"

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
  using ghogx::character::SourceCharUtlBoneTransResult;
  using ghogx::character::SourceCharUtlObject;
  using ghogx::character::SourceCharUtlObjectKind;
  using ghogx::character::source_char_utl_find_bone;
  using ghogx::character::source_char_utl_find_bone_trans;
  using ghogx::character::source_char_utl_is_animatable;
  using ghogx::character::source_char_utl_name_with_suffix;

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

  return ok ? 0 : 1;
}
