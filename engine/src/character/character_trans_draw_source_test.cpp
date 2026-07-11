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
  using ghogx::character::source_char_trans_draw_destruct_modes;
  using ghogx::character::source_char_trans_draw_draw_showing;
  using ghogx::character::source_char_trans_draw_load_modes;
  using ghogx::character::source_char_trans_draw_set_draw_modes;

  bool ok = true;
  const std::vector<std::string> chars = {"rock1", "rock2"};

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
