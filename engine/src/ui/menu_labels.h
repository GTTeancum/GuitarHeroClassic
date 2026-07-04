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
  std::string parent; // authored parent group/view string, when serialized
  std::string text;   // last embedded string = label / locale key ("CAREER")
  std::string nav;    // BandButton nav target (the embedded "*.btn" string)
                      // focus-down link; "" if none (e.g. Text objects)
  bool has_showing = false; // Draw base visibility was decoded from MILO.
  bool showing = true;      // Authored initial Draw visibility.

  // BandButton text/layout tail, decoded from the bytes immediately after the
  // embedded label string. These names are conservative: the byte offsets are
  // grounded, while unknown slots still need loader/recomp mapping before the
  // renderer consumes them as semantics.
  struct ButtonTail {
    bool valid = false;
    std::int32_t fit_text = 0;  // label_end + 0 (kNormal/kStretched/kJustFit)
    float label_width = 0.0f;   // label_end + 4 (CAREER=310, QUICK_PLAY=320...)
    float box_height = 0.0f;    // label_end + 8 (15)
    float leading = 0.0f;       // label_end + 12
    std::uint32_t align_flags = 0;  // label_end + 16 (0x11/0x22/etc.)
    std::int32_t field_14 = 0;  // label_end + 20 (varies by label)
    float scale = 0.0f;         // label_end + 28 (main menu uses 0.5)
    float field_20 = 0.0f;      // label_end + 32 (1)
    std::uint8_t all_caps = 0;  // label_end + 36
    float kerning = 0.0f;       // label_end + 37 (-0.05)
    float text_size = 0.0f;     // label_end + 41
    float width_bound = 0.0f;   // label_end + 45 (wrap_width)
  } button_tail;

  // BandLabel/RndText text/layout tail. Like BandButton, this block is
  // byte-aligned to the end of the variable-length label string.
  struct TextTail {
    bool valid = false;
    std::int32_t fit_text = 0;    // label_end + 0 (kNormal/kStretched/kJustFit)
    float label_width = 0.0f;     // label_end + 4
    float box_height = 0.0f;      // label_end + 8
    float leading = 0.0f;         // label_end + 12
    std::uint32_t align_flags = 0; // label_end + 16 (0x11/0x22/etc.)
    std::int32_t field_14 = 0;    // label_end + 20
    std::uint8_t all_caps = 0;    // label_end + 36
    float kerning = 0.0f;         // label_end + 37
    float text_size = 0.0f;       // label_end + 41
    float width_bound = 0.0f;     // label_end + 45 (wrap_width)
    std::array<float, 4> color{{1.0f, 1.0f, 1.0f, 1.0f}}; // label_end + 49
  } text_tail;

  // Local + world Trans matrices: row-major 3x3 (m[0..8]) + translation
  // (m[9..11]).
  std::array<float, 12> local{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_local = false;
  std::array<float, 12> world{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_world = false;
};

struct UiListLayout {
  bool valid = false;
  std::string provider;
  std::string parent;
  float local_x = 0.0f;
  float local_z = 0.0f;
  float world_x = 0.0f;
  float world_z = 0.0f;
  int visible_slots = 0;
  float row_height = 0.0f;
  float text_height = 0.0f;
  int width_bound = 0;
};

struct UiListTextTemplate {
  bool valid = false;
  std::string name;
  std::string font;
  std::string text;
  float local_x = 0.0f;
  float local_z = 0.0f;
  float world_x = 0.0f;
  float world_z = 0.0f;
  float text_size = 0.0f;
  std::array<float, 4> color{{1.0f, 1.0f, 1.0f, 1.0f}};
  float wrap_width = 0.0f;
  float field_14 = 0.0f;
  float field_18 = 0.0f;
  float field_1c = 0.0f;
  std::uint32_t flags = 0;
  std::array<float, 12> world{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
};

struct UiListTemplateLayout {
  bool valid = false;
  UiListTextTemplate header;
  UiListTextTemplate list;
  UiListTextTemplate stars;
  UiListTextTemplate score;
  UiListTextTemplate blurb;
};

// Parse a panel MILO straight from the PS2 ARK (hdr/ark) and return its
// text-bearing objects. Empty on failure (logged).
std::vector<MenuLabel> extract_menu_labels(const std::string& hdr_path,
                                           const std::string& ark_path,
                                           const std::string& milo_path);

// Decode the layout tail and transform from a UIList entry in a PS2 menu MILO.
UiListLayout extract_ui_list_layout(const std::string& hdr_path,
                                    const std::string& ark_path,
                                    const std::string& milo_path,
                                    const std::string& list_name);

// Decode the Text entries used by a UIList resource file, such as
// ui/gen/list_song2.milo_ps2::header.txt/list.txt.
UiListTemplateLayout extract_ui_list_template_layout(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path);

// Decode one named Text template from a UIList resource MILO.
UiListTextTemplate extract_text_template_layout(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path, const std::string& text_name);

}  // namespace ghogx::ui
