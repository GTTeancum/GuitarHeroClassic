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
#include <optional>
#include <string>
#include <vector>

namespace ghogx::ui {

struct MenuLabel {
  std::string runtime_owner; // owning UIPanel; disambiguates duplicate child names
  std::string name;   // entry name, e.g. "main_career.btn"
  std::string type;   // "BandButton" / "Text" / "BandLabel"
  std::string font;   // first embedded string ("impact"); "" if only one string
  std::string parent; // authored parent group/view string, when serialized
  std::vector<std::string> visibility_ancestors; // resolved MILO parent chain
  // True when this label or one of its resolved transform ancestors is the
  // target of an authored TransAnim in the owning panel. Such labels must be
  // built from their serialized WorldXfm bind pose even when that pose starts
  // off the normal text plane; the animation brings it onto the plane.
  bool has_transform_animated_ancestor = false;
  std::string text;   // last embedded string = label / locale key ("CAREER")
  std::string nav;    // BandButton nav target (the embedded "*.btn" string)
                      // focus-down link; "" if none (e.g. Text objects)
  bool has_showing = false; // Draw base visibility was decoded from MILO.
  bool showing = true;      // Authored initial Draw visibility.

  // BandButton text/layout tail, decoded from the bytes immediately after the
  // embedded label string. Main-menu buttons use the compact GH2 tail; newer
  // menu rows use the UILabel/BandButton fit/layout tail mirrored from MiloLib.
  // These names are conservative where the old and new layouts overlap.
  struct ButtonTail {
    bool valid = false;
    bool legacy_layout = false;
    std::int32_t fit_text = 0;   // UILabel/BandButton fitType when present.
    std::int32_t alignment = 34; // UILabel::TextAlignments / RndText bits.
    std::uint8_t all_caps = 0;
    float width = 0.0f;
    float height = 0.0f;
    float leading = 0.0f;
    std::int32_t unknown_10 = 0;
    std::int32_t unknown_14 = 0;
    float text_size = 0.0f;
    float unknown_20 = 0.0f;
    std::uint8_t unknown_24 = 0;
    float kerning = 0.0f;
    float wrap_width = 0.0f;
    float width_bound = 0.0f;
  } button_tail;

  // BandLabel/RndText text/layout tail. Like BandButton, this block is
  // byte-aligned to the end of the variable-length label string.
  struct TextTail {
    bool valid = false;
    std::int32_t fit_text = 0;     // label_end + 0
    float width = 0.0f;            // label_end + 4
    float height = 0.0f;           // label_end + 8
    float leading = 0.0f;          // label_end + 12
    std::int32_t alignment = 0;    // label_end + 16 (raw; not consumed yet)
    std::int32_t unknown_14 = 0;   // label_end + 20
    std::uint8_t all_caps = 0;     // label_end + 36
    float kerning = 0.0f;          // label_end + 37
    float text_size = 0.0f;        // label_end + 41
    float width_bound = 0.0f;      // label_end + 45
    std::array<float, 4> color{{1.0f, 1.0f, 1.0f, 1.0f}};
  } text_tail;

  // BandTextEntry revision-3 fields are the final 44 bytes of the component:
  // timing/selection geometry followed by entered/current-character colors.
  struct TextEntryTail {
    bool valid = false;
    float flash_time = 0.0f;
    float text_scale = 0.0f;
    float arrow_offset = 0.0f;
    std::array<float, 4> entered_color{{1.0f, 1.0f, 1.0f, 1.0f}};
    std::array<float, 4> dynamic_color{{1.0f, 1.0f, 1.0f, 1.0f}};
  } text_entry_tail;

  // Local + world Trans matrices: row-major 3x3 (m[0..8]) + translation
  // (m[9..11]).
  std::array<float, 12> local{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_local = false;
  std::array<float, 12> world{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_world = false;
};

// Transform a point on RndText's local X/Z plane through a serialized
// Harmonix Trans. Trans matrices use row-vector basis rows; local X follows
// row 0 and local Z follows row 2.
std::array<float, 3> transform_menu_text_point(
    const std::array<float, 12>& xfm, float local_x, float local_z);

// Select the authored transform used to build a label's bind-pose vertices.
// Static labels with a far scene-space WorldXfm retain the legacy local-plane
// fallback. Labels in an authored transform-animation chain use WorldXfm so
// their vertices and the renderer's bind_world matrix share the same basis.
bool menu_label_uses_authored_world_transform(const MenuLabel& label);

// The stock character panel's outfit rows are the two helveticablack
// BandButtons authored directly under text_skin.grp. Retail renders that
// panel-local treatment black in every component state.
bool menu_label_uses_black_outfit_button_text(const MenuLabel& label);

struct MenuCheckbox {
  std::string name;      // entry name, e.g. "p_scan.chk"
  std::string type;      // "CheckBox" / "CheckboxDisplay"
  std::string resource;  // UIComponent resource string, e.g. "default"
  std::string parent;    // authored parent group/view string
  bool checked = false;  // authored checked state before scripts override it
  bool showing = true;   // Draw base visibility

  std::array<float, 12> local{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_local = false;
  std::array<float, 12> world{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_world = false;
};

struct MenuSlider {
  std::string name;      // entry name, e.g. "gs_band.sld"
  std::string type;      // "BandSlider" / "UISlider"
  std::string resource;  // UI resource style, e.g. "aaron" or "char"
  std::string parent;    // authored parent group/view string
  std::string nav;       // neighbouring slider/button target, when serialized
  std::string token;     // label/config token at the tail, e.g. "BAND"
  std::int32_t current = 0;
  std::int32_t num_steps = 1;
  bool vertical = false;
  bool showing = true;

  std::array<float, 12> local{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_local = false;
  std::array<float, 12> world{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_world = false;
};

struct MenuTransVecKey {
  float frame = 0.0f;
  std::array<float, 3> value{{0.0f, 0.0f, 0.0f}};
};

struct MenuTransQuatKey {
  float frame = 0.0f;
  std::array<float, 4> quat_xyzw{{0.0f, 0.0f, 0.0f, 1.0f}};
};

struct MenuSliderAnim {
  bool valid = false;
  std::string name;
  std::string target; // TransAnim target, e.g. "char_slider_pod.mesh"
  std::string keys_owner; // authored owner reference; self for embedded key sets
  std::vector<MenuTransQuatKey> rotation_keys;
  std::vector<MenuTransVecKey> translation_keys;
  std::vector<MenuTransVecKey> scale_keys;
  std::array<float, 3> first{{0.0f, 0.0f, 0.0f}};
  float first_frame = 0.0f;
  std::array<float, 3> last{{0.0f, 0.0f, 0.0f}};
  float last_frame = 1.0f;
};

struct MenuAnimFilter {
  bool valid = false;
  std::string name;
  std::string trans_anim;  // referenced TransAnim, e.g. "guitar_store.tnm"
  float frame = 0.0f;
  float scale = 1.0f;
  float offset = 0.0f;
  float start = 0.0f;
  float end = 0.0f;
  std::int32_t type = 0;  // RndAnimFilter::Type: 0 range, 1 loop, 2 shuttle.
  float period = 0.0f;
};

// GH2's legacy UITrigger revision stores one event, one RndAnimatable reference,
// and a transition-blocking flag after an inherited UIComponent. PanelDir sends
// ui_enter/ui_exit plus the direction-specific event; only a triggered entry
// with a live animation and block_transition=true delays UIManager.
struct MenuUiTrigger {
  bool valid = false;
  std::string name;
  std::uint16_t revision = 0;
  std::uint16_t component_revision = 0;
  std::string event;
  std::string anim_ref;
  bool block_transition = false;
};

// Body-level decoders used by the runtime MILO loader after it has already
// inflated a panel. Invalid input returns a value with valid=false.
MenuSliderAnim decode_menu_trans_anim_body(
    const std::vector<std::uint8_t>& body, const std::string& name);
MenuAnimFilter decode_menu_anim_filter_body(
    const std::vector<std::uint8_t>& body, const std::string& name);
MenuUiTrigger decode_menu_ui_trigger_body(
    const std::vector<std::uint8_t>& body, const std::string& name);

struct MenuMaterialColorKey {
  float frame = 0.0f;
  std::array<float, 4> color{{1.0f, 1.0f, 1.0f, 1.0f}};
};

struct MenuMaterialFloatKey {
  float frame = 0.0f;
  float value = 0.0f;
};

struct MenuMaterialTextureKey {
  float frame = 0.0f;
  std::string texture;  // keyed .tex entry, e.g. "loading_word2_gw.tex"
};

struct MenuMaterialAnim {
  bool valid = false;
  std::string name;
  std::string material;  // RndMat target, e.g. "loading_word.mat"
  std::string keys_owner;  // authored MatAnim key owner; may be another .mnm
  std::vector<MenuMaterialColorKey> color_keys;
  std::vector<MenuMaterialFloatKey> alpha_keys;
  std::vector<MenuTransVecKey> translation_keys;
  std::vector<MenuTransVecKey> scale_keys;
  std::vector<MenuTransVecKey> rotation_keys;
  std::vector<MenuMaterialTextureKey> texture_keys;
  float first_frame = 0.0f;
  float last_frame = 0.0f;
};

MenuMaterialAnim decode_menu_material_anim_body(
    const std::vector<std::uint8_t>& body, const std::string& name);

struct MenuProxyTransform {
  bool valid = false;
  std::string name;    // UIProxy entry name, e.g. "guitar.pxy"
  std::string parent;  // authored parent transform, e.g. "guitar.grp"
  std::string target;  // inherited RndTransformable target, usually empty
  std::uint32_t constraint = 0;
  bool preserve_scale = false;
  std::array<float, 12> local{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  std::array<float, 12> world{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
};

struct UiListLayout {
  bool valid = false;
  std::string name;
  std::uint16_t revision = 0;
  std::uint16_t alt_revision = 0;

  // MiloLib Assets/UI/UIList.cs source order, decoded from the UIList tail after
  // the inherited UIComponent block.
  std::int32_t legacy_i = 0;
  std::int32_t legacy_j = 0;
  std::int32_t legacy_k = 0;
  std::int32_t legacy_x = 0;
  std::int32_t legacy_unk3 = 0;
  bool legacy_b8 = false;
  bool legacy_b9 = false;
  bool legacy_ba = false;

  std::int32_t num_display = 0;
  std::int32_t grid_span = 0;
  bool circular = false;
  float speed = 0.0f;
  bool scroll_past_min = false;
  bool scroll_past_max = false;
  bool paginate = false;
  bool select_to_scroll = false;
  std::int32_t min_display = 0;
  std::int32_t max_display = -1;
  std::int32_t num_data = 0;
  float auto_scroll_pause = 0.0f;
  bool auto_scroll_send_messages = false;
  std::vector<std::string> extended_label_entries;
  std::vector<std::string> extended_mesh_entries;
  std::vector<std::string> extended_custom_entries;
  std::string in_anim;
  std::string out_anim;

  // GH2 PS2 revision-2 UILists carry early compact list metrics that MiloLib's
  // newer UIList source order does not name. These are decoded from the stock
  // bytes after the inherited UIComponent/RndDrawable block.
  bool has_legacy_row_metrics = false;
  float legacy_visible_slots = 0.0f;
  float legacy_row_height = 0.0f;
  float legacy_text_height = 0.0f;

  std::array<float, 12> world{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_world = false;
  std::array<float, 12> local{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_local = false;
  std::string parent;
};

struct MenuTextStyle {
  bool valid = false;
  std::string name;       // Text entry name, e.g. "list.txt"
  std::string parent;     // authored RndTransformable parent
  std::string font;       // RndText mFont, e.g. "dyingmarker.font"
  std::string text;       // serialized sample/default string
  std::int32_t alignment = 0;
  std::array<float, 4> color{{1.0f, 1.0f, 1.0f, 1.0f}};
  float wrap_width = 0.0f;
  float leading = 1.0f;
  std::int32_t fixed_length = 0;
  float italic_strength = 0.0f;
  float text_size = 0.0f;  // RndText::mSize
  bool markup = false;
  std::int32_t caps_mode = 0;

  std::array<float, 12> local{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_local = false;
  std::array<float, 12> world{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  bool has_world = false;
};

// Parse a panel MILO straight from the PS2 ARK (hdr/ark) and return its
// text-bearing objects. Empty on failure (logged).
std::vector<MenuLabel> extract_menu_labels(const std::string& hdr_path,
                                           const std::string& ark_path,
                                           const std::string& milo_path);

// Decode CheckBox/CheckboxDisplay widget state and RndTrans matrices. The
// renderer uses this to draw the shared checkbox.milo resource at the authored
// widget WorldXfm.
std::vector<MenuCheckbox> extract_menu_checkboxes(const std::string& hdr_path,
                                                  const std::string& ark_path,
                                                  const std::string& milo_path);

// Decode BandSlider/UISlider widget state, using the current/num_steps/Frame()
// shape from Harmonix UISlider and GH2's compact BandSlider bytes.
std::vector<MenuSlider> extract_menu_sliders(const std::string& hdr_path,
                                             const std::string& ark_path,
                                             const std::string& milo_path);

// Decode the stock slider resource TransAnim translation endpoints. UISlider
// drives the resource RndDir frame; this exposes the target mesh travel range.
MenuSliderAnim extract_menu_slider_anim(const std::string& hdr_path,
                                        const std::string& ark_path,
                                        const std::string& milo_path,
                                        const std::string& anim_name);

// Decode the complete authored TransAnim corpus for one panel in one ARK read.
// External keys_owner references are resolved onto the consuming animation,
// matching RndTransAnim's runtime delegation while preserving its own target.
std::vector<MenuSliderAnim> extract_menu_transform_anims(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path);

// Decode an AnimFilter entry enough to recover its source TransAnim reference
// and stored frame. Harmonix AnimFilter drives a RndAnimatable to an authored
// frame; this pins that source route for menu pose work.
MenuAnimFilter extract_menu_anim_filter(const std::string& hdr_path,
                                        const std::string& ark_path,
                                        const std::string& milo_path,
                                         const std::string& filter_name);

// Decode every legacy UITrigger in one stock panel MILO using Harmonix's
// serialized UITrigger -> UIComponent -> RndTrans/RndDrawable field order.
std::vector<MenuUiTrigger> extract_menu_ui_triggers(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path);

// Decode a MatAnim entry enough to recover its material target and keyed diffuse
// texture swaps. Loading's LOADING word uses this for the stock blink/flip.
MenuMaterialAnim extract_menu_material_anim(const std::string& hdr_path,
                                            const std::string& ark_path,
                                            const std::string& milo_path,
                                            const std::string& anim_name);

std::vector<MenuMaterialAnim> extract_menu_material_anims(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path);

// Decode a UIProxy's authored transform. GuitarDisplayPanel::show_guitar passes
// panel-local UIProxy objects such as sel_guitar/guitar.pxy as live placement
// targets for the 3D guitar overlay.
MenuProxyTransform extract_menu_proxy_transform(const std::string& hdr_path,
                                                const std::string& ark_path,
                                                const std::string& milo_path,
                                                const std::string& proxy_name);

// Decode a UIList entry using the MiloLib UIList field order. Returns an invalid
// layout on failure.
UiListLayout extract_ui_list_layout(const std::string& hdr_path,
                                    const std::string& ark_path,
                                    const std::string& milo_path,
                                    const std::string& list_name);

// Decode a RndText entry using ihatecompvir's RndText source order. Used by
// UIList slot resources such as list_song.milo, where the slot text object
// owns the real font size.
MenuTextStyle extract_menu_text_style(const std::string& hdr_path,
                                      const std::string& ark_path,
                                      const std::string& milo_path,
                                      const std::string& text_name);

}  // namespace ghogx::ui
