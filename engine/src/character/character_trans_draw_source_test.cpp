#include "character/char_mesh.h"

#include <iostream>
#include <string>
#include <vector>

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

bool expect_mode(ghogx::character::SourceCharacterDrawMode got,
                 ghogx::character::SourceCharacterDrawMode want,
                 const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << static_cast<int>(got) << " want "
            << static_cast<int>(want) << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharacterDrawMode;
  using ghogx::character::SourceCharTransDrawCharacter;
  using ghogx::character::source_char_trans_draw_copy_plan;
  using ghogx::character::source_char_trans_draw_destruct_modes;
  using ghogx::character::source_char_trans_draw_draw_showing;
  using ghogx::character::source_char_trans_draw_handler_plan;
  using ghogx::character::source_char_trans_draw_load_plan;
  using ghogx::character::source_char_trans_draw_load_modes;
  using ghogx::character::source_char_trans_draw_prop_sync_plan;
  using ghogx::character::source_char_trans_draw_save_plan;
  using ghogx::character::source_char_trans_draw_set_draw_modes;

  bool ok = true;
  const std::vector<std::string> chars = {"rock1", "rock2"};

  const auto bad_load = source_char_trans_draw_load_plan(2);
  ok &= expect_bool(bad_load.known_revision, false,
                    "load rejects revision 2");
  ok &= expect_size(bad_load.read_order.size(), 0,
                    "rejected load has no rows");
  const auto load_plan = source_char_trans_draw_load_plan(1);
  ok &= expect_bool(load_plan.known_revision, true,
                    "load accepts revision 1");
  ok &= expect_size(load_plan.read_order.size(), 3,
                    "load read count");
  ok &= expect_string(load_plan.read_order[0], "Hmx::Object",
                      "load object first");
  ok &= expect_string(load_plan.read_order[1], "RndDrawable",
                      "load drawable second");
  ok &= expect_string(load_plan.read_order[2], "mChars",
                      "load chars third");
  ok &= expect_mode(load_plan.post_load_mode,
                    SourceCharacterDrawMode::kOpaque,
                    "load post mode opaque");

  const auto save = source_char_trans_draw_save_plan();
  ok &= expect_bool(save.save_id == 0x23, true, "save id");

  const auto copy_plan = source_char_trans_draw_copy_plan();
  ok &= expect_size(copy_plan.copied_superclasses.size(), 2,
                    "copy superclass count");
  ok &= expect_string(copy_plan.copied_superclasses[0], "Hmx::Object",
                      "copy object superclass");
  ok &= expect_string(copy_plan.copied_superclasses[1], "RndDrawable",
                      "copy drawable superclass");
  ok &= expect_size(copy_plan.copied_members.size(), 1,
                    "copy member count");
  ok &= expect_string(copy_plan.copied_members[0], "mChars",
                      "copy chars member");

  const auto handler_plan = source_char_trans_draw_handler_plan();
  ok &= expect_size(handler_plan.superclasses.size(), 2,
                    "handler superclass count");
  ok &= expect_string(handler_plan.superclasses[0], "RndDrawable",
                      "handler drawable superclass");
  ok &= expect_string(handler_plan.superclasses[1], "Hmx::Object",
                      "handler object superclass");
  ok &= expect_bool(handler_plan.check == 0x5e, true,
                    "handler check");
  ok &= expect_size(source_char_trans_draw_prop_sync_plan().properties.size(),
                    1, "prop row count");
  const auto prop_plan = source_char_trans_draw_prop_sync_plan();
  ok &= expect_string(prop_plan.properties[0], "chars",
                      "prop chars row");
  ok &= expect_string(prop_plan.superclasses[0], "RndDrawable",
                      "prop drawable superclass");

  auto steps =
      source_char_trans_draw_set_draw_modes(chars, SourceCharacterDrawMode::kAll);
  ok &= expect_size(steps.size(), 2, "SetDrawModes count");
  ok &= expect_string(steps[0].character, "rock1", "SetDrawModes first char");
  ok &= expect_mode(steps[0].mode, SourceCharacterDrawMode::kAll,
                    "SetDrawModes mode");
  ok &= expect_bool(steps[0].draw, false, "SetDrawModes does not draw");

  steps = source_char_trans_draw_load_modes(chars);
  ok &= expect_size(steps.size(), 2, "Load mode count");
  ok &= expect_mode(steps[1].mode, SourceCharacterDrawMode::kOpaque,
                    "Load sets opaque");

  steps = source_char_trans_draw_destruct_modes(chars);
  ok &= expect_size(steps.size(), 2, "destructor mode count");
  ok &= expect_mode(steps[0].mode, SourceCharacterDrawMode::kAll,
                    "destructor restores all");

  const std::vector<SourceCharTransDrawCharacter> draw_chars = {
      {"hidden", false}, {"visible.a", true}, {"visible.b", true}};
  steps = source_char_trans_draw_draw_showing(draw_chars);
  ok &= expect_size(steps.size(), 6, "DrawShowing step count");
  ok &= expect_string(steps[0].character, "visible.a",
                      "DrawShowing skips hidden character");
  ok &= expect_mode(steps[0].mode, SourceCharacterDrawMode::kTranslucent,
                    "DrawShowing first sets translucent");
  ok &= expect_bool(steps[0].draw, false, "DrawShowing set does not draw");
  ok &= expect_string(steps[1].character, "visible.a",
                      "DrawShowing draw same character");
  ok &= expect_bool(steps[1].draw, true, "DrawShowing draws visible character");
  ok &= expect_mode(steps[2].mode, SourceCharacterDrawMode::kOpaque,
                    "DrawShowing restores opaque");
  ok &= expect_string(steps[3].character, "visible.b",
                      "DrawShowing preserves source order");
  ok &= expect_bool(steps[4].draw, true,
                    "DrawShowing draws second visible character");

  return ok ? 0 : 1;
}
