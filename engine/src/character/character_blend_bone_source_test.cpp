#include "character/char_mesh.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

bool expect_bool(bool got, bool want, const char* label) {
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
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharBlendBoneConstraint;
  using ghogx::character::SourceCharBlendBonePollDeps;
  using ghogx::character::source_char_blend_bone_constraint_load_plan;
  using ghogx::character::source_char_blend_bone_constraint_prop_sync_plan;
  using ghogx::character::source_char_blend_bone_copy_plan;
  using ghogx::character::source_char_blend_bone_default_state;
  using ghogx::character::source_char_blend_bone_handler_plan;
  using ghogx::character::source_char_blend_bone_load_plan;
  using ghogx::character::source_char_blend_bone_poll_deps;
  using ghogx::character::source_char_blend_bone_prop_sync_plan;
  using ghogx::character::source_char_blend_bone_save_plan;

  bool ok = true;

  auto blend = source_char_blend_bone_default_state();
  ok &= expect_size(blend.targets.size(), 0, "default target count");
  ok &= expect_string(blend.src1, "", "default src1");
  ok &= expect_string(blend.src2, "", "default src2");
  ok &= expect_bool(blend.trans_x, false, "default trans_x");
  ok &= expect_bool(blend.trans_y, false, "default trans_y");
  ok &= expect_bool(blend.trans_z, false, "default trans_z");
  ok &= expect_bool(blend.rotation, false, "default rotation");

  const SourceCharBlendBoneConstraint constraint;
  ok &= expect_string(constraint.target, "", "default constraint target");
  ok &= near(constraint.weight, 0.5f, "default constraint weight");

  const auto constraint_load = source_char_blend_bone_constraint_load_plan();
  ok &= expect_size(constraint_load.read_order.size(), 2,
                    "constraint load read count");
  ok &= expect_string(constraint_load.read_order[0], "mTarget",
                      "constraint reads target first");
  ok &= expect_string(constraint_load.read_order[1], "mWeight",
                      "constraint reads weight second");

  const auto load_bad = source_char_blend_bone_load_plan(4);
  ok &= expect_bool(load_bad.known_revision, false,
                    "load rejects unknown revision");
  ok &= expect_size(load_bad.read_order.size(), 0,
                    "bad revision has no reads");

  const auto load = source_char_blend_bone_load_plan(3);
  ok &= expect_bool(load.known_revision, true, "load accepts rev3");
  ok &= expect_size(load.read_order.size(), 9, "load read count");
  ok &= expect_string(load.read_order[1], "Hmx::Object",
                      "load object superclass");
  ok &= expect_string(load.read_order[2], "mTargets",
                      "load targets first");
  ok &= expect_string(load.read_order[3], "mSrc1", "load source one");
  ok &= expect_string(load.read_order[4], "mSrc2", "load source two");
  ok &= expect_string(load.read_order[8], "mRotation",
                      "load rotation last");

  const auto save = source_char_blend_bone_save_plan();
  ok &= expect_bool(save.save_id == 0x44, true, "save id value");

  const auto copy = source_char_blend_bone_copy_plan();
  ok &= expect_size(copy.copied_superclasses.size(), 1,
                    "copy superclass count");
  ok &= expect_string(copy.copied_superclasses[0], "Hmx::Object",
                      "copy object superclass");
  ok &= expect_size(copy.copied_members.size(), 7,
                    "copy member count");
  ok &= expect_string(copy.copied_members[0], "mTargets",
                      "copy targets first");
  ok &= expect_string(copy.copied_members[6], "mRotation",
                      "copy rotation last");

  const auto handler = source_char_blend_bone_handler_plan();
  ok &= expect_size(handler.superclasses.size(), 1,
                    "handler superclass count");
  ok &= expect_string(handler.superclasses[0], "Hmx::Object",
                      "handler object superclass");
  ok &= expect_bool(handler.check == 0x8F, true, "handler check value");

  const auto constraint_props =
      source_char_blend_bone_constraint_prop_sync_plan();
  ok &= expect_size(constraint_props.properties.size(), 2,
                    "constraint prop-sync count");
  ok &= expect_string(constraint_props.properties[0], "target",
                      "constraint prop-sync target");
  ok &= expect_string(constraint_props.properties[1], "weight",
                      "constraint prop-sync weight");

  const auto props = source_char_blend_bone_prop_sync_plan();
  ok &= expect_size(props.properties.size(), 7, "prop-sync count");
  ok &= expect_string(props.properties[0], "targets", "prop-sync targets");
  ok &= expect_string(props.properties[1], "src_one", "prop-sync src_one");
  ok &= expect_string(props.properties[5], "trans_z", "prop-sync trans_z");
  ok &= expect_string(props.properties[6], "rotation", "prop-sync rotation");

  blend.src1 = "source.one";
  blend.src2 = "source.two";
  blend.targets = {{"target.a", 0.25f}, {"target.b", 0.75f}};
  SourceCharBlendBonePollDeps deps;
  source_char_blend_bone_poll_deps(deps, blend);
  ok &= expect_size(deps.changed_by.size(), 2, "PollDeps changed_by count");
  ok &= expect_string(deps.changed_by[0], "source.one",
                      "PollDeps first source");
  ok &= expect_string(deps.changed_by[1], "source.two",
                      "PollDeps second source");
  ok &= expect_size(deps.change.size(), 2, "PollDeps target count");
  ok &= expect_string(deps.change[0], "target.a", "PollDeps first target");
  ok &= expect_string(deps.change[1], "target.b", "PollDeps second target");

  return ok ? 0 : 1;
}
