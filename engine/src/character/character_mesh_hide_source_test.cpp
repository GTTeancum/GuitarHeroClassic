#include "character/char_mesh.h"

#include <iostream>
#include <vector>

namespace {

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_int(int32_t got, int32_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharMeshHideObject;
  using ghogx::character::SourceCharMeshHideRow;
  using ghogx::character::source_char_mesh_hide_all;
  using ghogx::character::source_char_mesh_hide_combined_flags;
  using ghogx::character::source_char_mesh_hide_draws;

  bool ok = true;

  const SourceCharMeshHideRow default_row;
  ok &= expect_int(default_row.flags, 0, "default row flags");
  ok &= expect_bool(default_row.draw_showing, false,
                    "default row draw showing");
  ok &= expect_bool(default_row.has_draw, false, "default row has draw");
  ok &= expect_bool(default_row.show, false, "default row show");

  std::vector<SourceCharMeshHideObject> empty_objects;
  ok &= expect_int(source_char_mesh_hide_combined_flags(empty_objects, 0x40),
                   0x40, "empty combined keeps initial flags");
  ok &= expect_int(source_char_mesh_hide_all(empty_objects, 0x40), 0x40,
                   "empty hide all keeps initial flags");

  SourceCharMeshHideObject single;
  single.hides.push_back({0x1, true, true, false});
  single.hides.push_back({0x2, false, true, true});
  single.hides.push_back({0x4, true, false, true});
  source_char_mesh_hide_draws(single, 0x1);
  ok &= expect_bool(single.hides[0].show, false,
                    "matching flags hide visible draw");
  ok &= expect_bool(single.hides[1].show, false,
                    "source draw showing gate is applied");
  ok &= expect_bool(single.hides[2].show, true,
                    "missing draw row preserves stored show");

  std::vector<SourceCharMeshHideObject> objects;
  SourceCharMeshHideObject first;
  first.flags = 0x1;
  first.hides.push_back({0x1, true, true, true});
  first.hides.push_back({0x2, true, true, false});
  first.hides.push_back({0x4, true, true, true});
  first.hides.push_back({0x10, true, false, true});
  objects.push_back(first);

  SourceCharMeshHideObject second;
  second.flags = 0x8;
  second.hides.push_back({0x8, true, true, true});
  second.hides.push_back({0x20, true, true, false});
  objects.push_back(second);

  const int32_t combined = source_char_mesh_hide_all(objects, 0x4);
  ok &= expect_int(combined, 0xd, "HideAll ORs initial and owner flags");
  ok &= expect_bool(objects[0].hides[0].show, false,
                    "HideAll applies first owner flags");
  ok &= expect_bool(objects[0].hides[1].show, true,
                    "HideAll leaves non-matching visible draw shown");
  ok &= expect_bool(objects[0].hides[2].show, false,
                    "HideAll applies initial flags");
  ok &= expect_bool(objects[0].hides[3].show, true,
                    "HideAll preserves row with no draw");
  ok &= expect_bool(objects[1].hides[0].show, false,
                    "HideAll applies second owner flags");
  ok &= expect_bool(objects[1].hides[1].show, true,
                    "HideAll keeps non-hidden second object row visible");

  return ok ? 0 : 1;
}
