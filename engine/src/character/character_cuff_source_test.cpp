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
  using ghogx::character::SourceCharCuffState;
  using ghogx::character::source_char_cuff_apply_revision_defaults;
  using ghogx::character::source_char_cuff_copy_plan;
  using ghogx::character::source_char_cuff_default_state;
  using ghogx::character::source_char_cuff_eccentricity;
  using ghogx::character::source_char_cuff_handler_plan;
  using ghogx::character::source_char_cuff_load_plan;
  using ghogx::character::source_char_cuff_prop_sync_plan;

  bool ok = true;

  SourceCharCuffState cuff = source_char_cuff_default_state();
  ok &= near(cuff.shape[0].offset, -2.9f, "default shape0 offset");
  ok &= near(cuff.shape[0].radius, 1.9f, "default shape0 radius");
  ok &= near(cuff.shape[1].offset, 0.0f, "default shape1 offset");
  ok &= near(cuff.shape[1].radius, 2.6f, "default shape1 radius");
  ok &= near(cuff.shape[2].offset, 2.0f, "default shape2 offset");
  ok &= near(cuff.shape[2].radius, 3.5f, "default shape2 radius");
  ok &= near(cuff.outer_radius, 3.1f, "default outer radius");
  ok &= expect_bool(cuff.open_end, false, "default open end");
  ok &= near(cuff.eccentricity, 1.0f, "default eccentricity");
  ok &= expect_string(cuff.category, "", "default category");
  ok &= expect_size(cuff.ignore.size(), 0, "default ignore count");

  const auto load_bad = source_char_cuff_load_plan(9);
  ok &= expect_bool(load_bad.known_revision, false,
                    "load rejects unknown revision");
  ok &= expect_size(load_bad.read_order.size(), 0,
                    "unknown revision no reads");

  const auto load_rev0 = source_char_cuff_load_plan(0);
  ok &= expect_bool(load_rev0.known_revision, true, "load accepts rev0");
  ok &= expect_string(load_rev0.read_order[1], "Hmx::Object",
                      "load object superclass");
  ok &= expect_string(load_rev0.read_order[2], "RndTransformable",
                      "load transform superclass");
  ok &= expect_string(load_rev0.read_order[3], "mShape[0].radius",
                      "load shape radius first");
  ok &= expect_string(load_rev0.read_order[4], "mShape[0].offset",
                      "load shape offset second");
  ok &= expect_size(load_rev0.read_order.size(), 9, "rev0 read count");
  ok &= expect_string(load_rev0.branches[0],
                      "mOuterRadius=mShape[1].radius+0.5",
                      "rev0 outer radius default branch");
  ok &= expect_string(load_rev0.branches[1], "mOpenEnd=false",
                      "rev0 open end default branch");
  ok &= expect_string(load_rev0.branches[2], "mBone=TransParent",
                      "rev0 bone parent branch");
  ok &= expect_string(load_rev0.branches[3], "mEccentricity=1",
                      "rev0 eccentricity branch");
  ok &= expect_string(load_rev0.branches[4], "mCategory=empty",
                      "rev0 category branch");
  ok &= expect_string(load_rev0.branches[5], "warnOldCharCuff",
                      "rev0 warning branch");
  ok &= expect_bool(load_rev0.warns_old_revision, true,
                    "rev0 warns old revision");

  const auto load_rev6 = source_char_cuff_load_plan(6);
  ok &= expect_bool(load_rev6.known_revision, true, "load accepts rev6");
  ok &= expect_string(load_rev6.read_order[9], "mOuterRadius",
                      "rev6 reads outer radius");
  ok &= expect_string(load_rev6.read_order[10], "mOpenEnd",
                      "rev6 reads open end");
  ok &= expect_string(load_rev6.read_order[11], "mBone",
                      "rev6 reads bone");
  ok &= expect_string(load_rev6.read_order[12], "mEccentricity",
                      "rev6 reads eccentricity");
  ok &= expect_string(load_rev6.read_order[13], "mCategory",
                      "rev6 reads category");
  ok &= expect_bool(load_rev6.warns_old_revision, true,
                    "rev6 still warns old revision");
  ok &= expect_size(load_rev6.branches.size(), 1,
                    "rev6 only warning branch");

  const auto load_rev8 = source_char_cuff_load_plan(8);
  ok &= expect_bool(load_rev8.known_revision, true, "load accepts rev8");
  ok &= expect_string(load_rev8.read_order.back(), "mIgnore",
                      "rev8 reads ignore list");
  ok &= expect_size(load_rev8.branches.size(), 0,
                    "rev8 no default branches");
  ok &= expect_bool(load_rev8.warns_old_revision, false,
                    "rev8 does not warn");

  const auto copy_plan = source_char_cuff_copy_plan();
  ok &= expect_size(copy_plan.copied_superclasses.size(), 2,
                    "copy superclass count");
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "copy object superclass");
  ok &= expect_string(copy_plan.copied_superclasses[1], "RndTransformable",
                      "copy transform superclass");
  ok &= expect_size(copy_plan.copied_members.size(), 7,
                    "copy member count");
  ok &= expect_string(copy_plan.copied_members[0], "mShape", "copy shape");
  ok &= expect_string(copy_plan.copied_members[6], "mIgnore", "copy ignore");
  const auto handlers = source_char_cuff_handler_plan();
  ok &= expect_size(handlers.superclasses.size(), 2,
                    "handler superclass count");
  ok &= expect_string(handlers.superclasses[0], "RndTransformable",
                      "handler transform superclass");
  ok &= expect_string(handlers.superclasses[1], "Hmx::Object",
                      "handler object superclass");
  ok &= expect_bool(handlers.check == 0x1FE, true, "handler check");
  const auto props = source_char_cuff_prop_sync_plan();
  ok &= expect_size(props.properties.size(), 12, "prop sync row count");
  ok &= expect_string(props.properties[0], "offset0", "prop offset0");
  ok &= expect_string(props.properties[1], "radius0", "prop radius0");
  ok &= expect_string(props.properties[6], "outer_radius",
                      "prop outer radius");
  ok &= expect_string(props.properties[8], "bone", "prop bone");
  ok &= expect_string(props.properties.back(), "ignore", "prop ignore");
  ok &= expect_string(props.superclasses[0], "RndTransformable",
                      "prop sync superclass");

  ok &= near(source_char_cuff_eccentricity(3.0f, 4.0f, 2.0f),
             std::sqrt(25.0f / (16.0f * 0.25f + 9.0f)),
             "source eccentricity formula");

  cuff.outer_radius = 42.0f;
  cuff.open_end = true;
  cuff.bone = "old.bone";
  cuff.eccentricity = 7.0f;
  cuff.category = "old.category";
  cuff.ignore = {"old.mesh"};
  source_char_cuff_apply_revision_defaults(cuff, 0, "parent.trans");
  ok &= near(cuff.outer_radius, cuff.shape[1].radius + 0.5f,
             "rev0 outer radius default");
  ok &= expect_bool(cuff.open_end, false, "rev0 open end default");
  ok &= expect_string(cuff.bone, "parent.trans", "rev0 parent bone default");
  ok &= near(cuff.eccentricity, 1.0f, "rev0 eccentricity default");
  ok &= expect_string(cuff.category, "", "rev0 category default");
  ok &= expect_size(cuff.ignore.size(), 0, "rev0 ignore not read");

  cuff = source_char_cuff_default_state();
  cuff.outer_radius = 5.0f;
  cuff.open_end = true;
  cuff.bone = "loaded.bone";
  cuff.eccentricity = 1.75f;
  cuff.category = "loaded.category";
  cuff.ignore = {"ignore.mesh"};
  source_char_cuff_apply_revision_defaults(cuff, 8, "parent.trans");
  ok &= near(cuff.outer_radius, 5.0f, "rev8 keeps loaded outer radius");
  ok &= expect_bool(cuff.open_end, true, "rev8 keeps loaded open end");
  ok &= expect_string(cuff.bone, "loaded.bone", "rev8 keeps loaded bone");
  ok &= near(cuff.eccentricity, 1.75f, "rev8 keeps loaded eccentricity");
  ok &= expect_string(cuff.category, "loaded.category",
                      "rev8 keeps loaded category");
  ok &= expect_size(cuff.ignore.size(), 1, "rev8 keeps ignore list");

  return ok ? 0 : 1;
}
