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
  using ghogx::character::source_char_cuff_default_state;
  using ghogx::character::source_char_cuff_eccentricity;

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
