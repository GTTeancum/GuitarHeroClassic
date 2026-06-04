// engine/src/ui/menu_labels.h
//
// Extract the text-bearing objects (BandButton / Text / BandLabel) from a panel
// MILO, with each one's embedded font + label string and its world Trans matrix,
// so the menu renderer can draw the labels at their real positions.
//
// Grounded in the real bytes (see FIDELITY 2c): a BandButton body holds, in
// order, length-prefixed strings [font, parent.view, nav.btn, LABEL] and an
// embedded Trans (local matrix then the composed world matrix). The label is the
// LAST embedded string (a locale key, e.g. "CAREER" / "QUICK_PLAY"); the font is
// the FIRST ("impact"). The world matrix is found structurally (the first
// local+world 48-byte matrix pair), which is robust to the class-specific prefix
// length shifting the offset.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ghogx::ui {

struct MenuLabel {
  std::string name;   // entry name, e.g. "main_career.btn"
  std::string type;   // "BandButton" / "Text" / "BandLabel"
  std::string font;   // first embedded string ("impact"); "" if only one string
  std::string text;   // last embedded string = label / locale key ("CAREER")
  std::string nav;    // BandButton nav target (the embedded "*.btn" string)
                      // focus-down link; "" if none (e.g. Text objects)

  // BandButton text/layout tail, decoded from the bytes immediately after the
  // embedded label string. These names are conservative: the byte offsets are
  // grounded, while unknown slots still need loader/recomp mapping before the
  // renderer consumes them as semantics.
  struct ButtonTail {
    bool valid = false;
    std::uint8_t all_caps = 0;  // label_end + 0
    float label_width = 0.0f;   // label_end + 4 (CAREER=310, QUICK_PLAY=320...)
    float box_height = 0.0f;    // label_end + 8 (15)
    float field_0c = 0.0f;      // label_end + 12 (1)
    std::int32_t field_10 = 0;  // label_end + 16 (34)
    std::int32_t field_14 = 0;  // label_end + 20 (varies by label)
    float text_size = 0.0f;     // label_end + 28 (0.5)
    float field_20 = 0.0f;      // label_end + 32 (1)
    std::uint8_t field_24 = 0;  // label_end + 36 (1)
    float kerning = 0.0f;       // label_end + 37 (-0.05)
    float field_29 = 0.0f;      // label_end + 41 (30)
    float width_bound = 0.0f;   // label_end + 45 (280)
  } button_tail;

  // World Trans matrix: row-major 3x3 (world[0..8]) + translation (world[9..11]).
  std::array<float, 12> world{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_world = false;
};

// Parse a panel MILO straight from the PS2 ARK (hdr/ark) and return its
// text-bearing objects. Empty on failure (logged).
std::vector<MenuLabel> extract_menu_labels(const std::string& hdr_path,
                                           const std::string& ark_path,
                                           const std::string& milo_path);

}  // namespace ghogx::ui
