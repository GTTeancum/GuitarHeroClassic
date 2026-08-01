#include "character/char_mesh.h"

#include <cstddef>
#include <iostream>
#include <string>
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

bool expect_size(std::size_t got, std::size_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_string(const std::string& got,
                   const std::string& want,
                   const char* label) {
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
  using ghogx::character::source_char_mesh_hide_copy_plan;
  using ghogx::character::source_char_mesh_hide_draws;
  using ghogx::character::source_char_mesh_hide_handler_plan;
  using ghogx::character::source_char_mesh_hide_load_plan;
  using ghogx::character::source_char_mesh_hide_prop_sync_plan;
  using ghogx::character::source_char_mesh_hide_row_load_plan;
  using ghogx::character::source_char_mesh_hide_save_plan;

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

  auto row_load = source_char_mesh_hide_row_load_plan(1);
  ok &= expect_bool(row_load.known_revision, true, "row load rev1 known");
  ok &= expect_size(row_load.read_order.size(), 2,
                    "row load rev1 skips show");
  row_load = source_char_mesh_hide_row_load_plan(2);
  ok &= expect_string(row_load.read_order.back(), "mShow",
                      "row load rev2 reads show");
  ok &= expect_bool(source_char_mesh_hide_row_load_plan(3).known_revision,
                    false, "row load rev3 rejected");

  const auto load = source_char_mesh_hide_load_plan(2);
  ok &= expect_bool(load.known_revision, true, "object load rev2 known");
  ok &= expect_int(load.max_revision, 2, "object load max revision");
  ok &= expect_string(load.read_order[0], "LOAD_REVS", "object load revs");
  ok &= expect_string(load.read_order[1], "Hmx::Object", "object load base");
  ok &= expect_string(load.read_order[2], "mFlags", "object load flags");
  ok &= expect_string(load.read_order[3], "mHides", "object load hides");
  ok &= expect_bool(source_char_mesh_hide_load_plan(3).known_revision, false,
                    "object load rev3 rejected");

  const auto save = source_char_mesh_hide_save_plan();
  ok &= expect_int(save.save_id, 0x6A, "CharMeshHide save id");

  const auto copy = source_char_mesh_hide_copy_plan();
  ok &= expect_string(copy.copied_superclasses[0], "Hmx::Object",
                      "copy superclass");
  ok &= expect_string(copy.copied_members[0], "mFlags", "copy flags");
  ok &= expect_string(copy.copied_members[1], "mHides", "copy hides");
  ok &= expect_bool(copy.guards_hides_self_copy, true,
                    "copy guards mHides self-copy");

  const auto handlers = source_char_mesh_hide_handler_plan();
  ok &= expect_string(handlers.superclasses[0], "Hmx::Object",
                      "handler superclass");
  ok &= expect_int(handlers.check, 0xA1, "handler check");

  const auto props = source_char_mesh_hide_prop_sync_plan();
  ok &= expect_string(props.hide_properties[0], "drawable",
                      "hide prop drawable");
  ok &= expect_string(props.hide_properties[2], "show", "hide prop show");
  ok &= expect_string(props.properties[0], "flags", "object prop flags");
  ok &= expect_string(props.properties[1], "hides", "object prop hides");

  return ok ? 0 : 1;
}
