// engine/src/ui/menu_labels.cpp -- see menu_labels.h.

#include "ui/menu_labels.h"

#include "ark_v3.h"
#include "milo.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace ghogx::ui {

namespace {

constexpr int32_t kMaxEmbeddedStringLen = 512;

float rf(const std::vector<uint8_t>& d, size_t o) {
  float v;
  std::memcpy(&v, d.data() + o, 4);
  return v;
}

int32_t ri32(const std::vector<uint8_t>& d, size_t o) {
  int32_t v;
  std::memcpy(&v, d.data() + o, 4);
  return v;
}

uint32_t ru32(const std::vector<uint8_t>& d, size_t o) {
  uint32_t v;
  std::memcpy(&v, d.data() + o, 4);
  return v;
}

bool finite(float f) { return f == f && std::fabs(f) < 1e30f; }

// A plausible Trans matrix at byte offset o: 12 finite floats whose three 3x3
// rows each have a sane magnitude and whose translation is in menu range.
bool looks_like_matrix(const std::vector<uint8_t>& d, size_t o) {
  if (o + 48 > d.size()) return false;
  float m[12];
  for (int i = 0; i < 12; ++i) {
    m[i] = rf(d, o + i * 4);
    if (!finite(m[i])) return false;
  }
  for (int r = 0; r < 3; ++r) {
    float mag = std::sqrt(m[r * 3] * m[r * 3] + m[r * 3 + 1] * m[r * 3 + 1] +
                          m[r * 3 + 2] * m[r * 3 + 2]);
    if (mag < 0.05f || mag > 5000.0f) return false;
  }
  for (int t = 9; t < 12; ++t)
    if (std::fabs(m[t]) > 1e5f) return false;
  return true;
}

void read_matrix(const std::vector<uint8_t>& d, size_t offset,
                 std::array<float, 12>& out) {
  for (int i = 0; i < 12; ++i)
    out[i] = rf(d, offset + static_cast<size_t>(i) * 4);
}

float matrix_basis_score(const std::array<float, 12>& m) {
  float score = 0.0f;
  for (int r = 0; r < 3; ++r) {
    const float mag =
        std::sqrt(m[r * 3] * m[r * 3] + m[r * 3 + 1] * m[r * 3 + 1] +
                  m[r * 3 + 2] * m[r * 3 + 2]);
    if (!finite(mag) || mag <= 0.0f) return 1e30f;
    score += std::fabs(std::log(std::max(mag, 0.001f)));
  }
  return score;
}

// Find the embedded local/world Trans pair. Scans every byte offset because the
// class prefix length (and its strings) shift the matrix off any fixed alignment.
bool find_local_world_matrices(const std::vector<uint8_t>& d,
                               std::array<float, 12>& local,
                               std::array<float, 12>& world) {
  bool found_pair = false;
  float best_pair_score = 1e30f;
  std::array<float, 12> best_local{};
  std::array<float, 12> best_world{};
  for (size_t M = 0x10; M + 96 <= d.size(); ++M) {
    if (looks_like_matrix(d, M) && looks_like_matrix(d, M + 48)) {
      std::array<float, 12> candidate_local{};
      std::array<float, 12> candidate_world{};
      read_matrix(d, M, candidate_local);
      read_matrix(d, M + 48, candidate_world);
      const float score = matrix_basis_score(candidate_local) +
                          matrix_basis_score(candidate_world);
      if (score < best_pair_score) {
        best_pair_score = score;
        best_local = candidate_local;
        best_world = candidate_world;
        found_pair = true;
      }
    }
  }
  if (found_pair) {
    local = best_local;
    world = best_world;
    return true;
  }
  // Fall back to a single matrix if no local+world pair exists.
  bool found_single = false;
  float best_single_score = 1e30f;
  std::array<float, 12> best_single{};
  for (size_t M = 0x10; M + 48 <= d.size(); ++M) {
    if (looks_like_matrix(d, M)) {
      std::array<float, 12> candidate{};
      read_matrix(d, M, candidate);
      const float score = matrix_basis_score(candidate);
      if (score < best_single_score) {
        best_single_score = score;
        best_single = candidate;
        found_single = true;
      }
    }
  }
  if (found_single) {
    local = best_single;
    world = local;
    return true;
  }
  return false;
}

struct EmbeddedString {
  std::string text;
  size_t offset = 0;  // offset of the i32 length prefix
  size_t end = 0;     // first byte after the string payload
};

// All length-prefixed printable-ASCII strings in the body, in order.
std::vector<EmbeddedString> embedded_strings(const std::vector<uint8_t>& d) {
  std::vector<EmbeddedString> out;
  for (size_t i = 0; i + 4 <= d.size();) {
    int32_t n;
    std::memcpy(&n, d.data() + i, 4);
    if (n >= 1 && n <= kMaxEmbeddedStringLen &&
        i + 4 + static_cast<size_t>(n) <= d.size()) {
      bool printable = true;
      for (int k = 0; k < n; ++k) {
        uint8_t c = d[i + 4 + k];
        if (c < 32 || c >= 127) { printable = false; break; }
      }
      if (printable) {
        EmbeddedString s;
        s.text.assign(reinterpret_cast<const char*>(d.data() + i + 4),
                      static_cast<size_t>(n));
        s.offset = i;
        s.end = i + 4 + static_cast<size_t>(n);
        out.push_back(std::move(s));
        i = out.back().end;
        continue;
      }
    }
    ++i;
  }
  return out;
}

bool decode_text_template(const std::vector<uint8_t>& body,
                          const std::string& name,
                          UiListTextTemplate& out) {
  // list_song*.milo Text entries store a local Trans at 0x2a and the composed
  // world Trans immediately after it. This is verified against stock PS2
  // list_song2.milo_ps2::header.txt/list.txt.
  constexpr size_t kLocalMatrixOffset = 0x2a;
  constexpr size_t kWorldMatrixOffset = kLocalMatrixOffset + 48;
  if (!looks_like_matrix(body, kLocalMatrixOffset) ||
      !looks_like_matrix(body, kWorldMatrixOffset)) {
    return false;
  }

  UiListTextTemplate t;
  t.valid = true;
  t.name = name;
  t.local_x = rf(body, kLocalMatrixOffset + 36);
  t.local_z = rf(body, kLocalMatrixOffset + 44);
  t.world_x = rf(body, kWorldMatrixOffset + 36);
  t.world_z = rf(body, kWorldMatrixOffset + 44);
  for (int i = 0; i < 12; ++i)
    t.world[i] = rf(body, kWorldMatrixOffset + static_cast<size_t>(i) * 4);

  const auto strs = embedded_strings(body);
  if (!strs.empty()) {
    t.text = strs.back().text;
    for (const auto& s : strs) {
      if (s.text.size() > 5 &&
          s.text.compare(s.text.size() - 5, 5, ".font") == 0) {
        t.font = s.text;
        break;
      }
    }
    const size_t text_tail = strs.back().end;
    if (text_tail + 40 <= body.size()) {
      for (int i = 0; i < 4; ++i) {
        const float c = rf(body, text_tail + static_cast<size_t>(i) * 4);
        if (finite(c)) t.color[i] = c;
      }
      t.wrap_width = rf(body, text_tail + 16);
      t.field_14 = rf(body, text_tail + 20);
      t.field_18 = rf(body, text_tail + 24);
      t.field_1c = rf(body, text_tail + 28);
      const float size = rf(body, text_tail + 32);
      if (finite(size) && size > 0.0f && size < 200.0f) t.text_size = size;
      t.flags = ru32(body, text_tail + 36);
    }
  }
  out = std::move(t);
  return true;
}

bool parse_bandbutton_tail(const std::vector<uint8_t>& body, size_t label_end,
                           MenuLabel::ButtonTail& out) {
  // This tail is byte-aligned to the variable-length label string, not 4-byte
  // aligned. Offsets below are relative to the first byte after the label.
  if (label_end + 49 > body.size()) return false;
  MenuLabel::ButtonTail t;
  t.valid = true;
  t.fit_text = ri32(body, label_end + 0);
  t.label_width = rf(body, label_end + 4);
  t.box_height = rf(body, label_end + 8);
  t.leading = rf(body, label_end + 12);
  t.align_flags = ru32(body, label_end + 16);
  t.field_14 = ri32(body, label_end + 20);
  t.scale = rf(body, label_end + 28);
  t.field_20 = rf(body, label_end + 32);
  t.all_caps = body[label_end + 36];
  t.kerning = rf(body, label_end + 37);
  t.text_size = rf(body, label_end + 41);
  t.width_bound = rf(body, label_end + 45);

  const bool plausible =
      t.fit_text >= 0 && t.fit_text <= 2 && t.all_caps <= 1 &&
      finite(t.label_width) && t.label_width > 0.0f &&
      t.label_width < 1000.0f && finite(t.box_height) && t.box_height > 0.0f &&
      t.box_height < 100.0f && finite(t.scale) && t.scale >= 0.0f &&
      t.scale < 10.0f && finite(t.text_size) && t.text_size >= 0.0f &&
      t.text_size < 200.0f && finite(t.kerning) &&
      std::fabs(t.kerning) < 10.0f && finite(t.width_bound) &&
      t.width_bound > 0.0f && t.width_bound < 5000.0f;
  if (!plausible) return false;
  out = t;
  return true;
}

bool parse_bandlabel_tail(const std::vector<uint8_t>& body, size_t label_end,
                          MenuLabel::TextTail& out) {
  // PS2 BandLabel bodies use the same unaligned tail style as BandButton, but
  // the text-size and bound fields are four bytes later:
  //   label_end+41 = text_size, label_end+45 = width_bound,
  //   label_end+49..64 = RGBA.
  if (label_end + 65 > body.size()) return false;
  MenuLabel::TextTail t;
  t.valid = true;
  t.fit_text = ri32(body, label_end + 0);
  t.label_width = rf(body, label_end + 4);
  t.box_height = rf(body, label_end + 8);
  t.leading = rf(body, label_end + 12);
  t.align_flags = ru32(body, label_end + 16);
  t.field_14 = ri32(body, label_end + 20);
  t.all_caps = body[label_end + 36];
  t.kerning = rf(body, label_end + 37);
  t.text_size = rf(body, label_end + 41);
  t.width_bound = rf(body, label_end + 45);
  for (int i = 0; i < 4; ++i)
    t.color[i] = rf(body, label_end + 49 + static_cast<size_t>(i) * 4);

  const bool plausible =
      t.fit_text >= 0 && t.fit_text <= 2 && t.all_caps <= 1 &&
      finite(t.label_width) &&
      t.label_width >= 0.0f && t.label_width < 2000.0f &&
      finite(t.box_height) && t.box_height >= 0.0f &&
      t.box_height < 500.0f && finite(t.text_size) &&
      t.text_size > 0.01f && t.text_size < 200.0f &&
      finite(t.kerning) && std::fabs(t.kerning) < 10.0f &&
      finite(t.width_bound) && t.width_bound >= 0.0f &&
      t.width_bound <= 20000.0f;
  if (!plausible) return false;
  out = t;
  return true;
}

void set_visible_slots_from_int_window(const std::vector<uint8_t>& body,
                                       size_t begin, size_t end,
                                       UiListLayout& layout) {
  if (layout.visible_slots > 0 || begin >= body.size()) return;
  end = std::min(end, body.size());
  for (size_t offset = begin; offset + 4 <= end; ++offset) {
    const uint32_t slots = ru32(body, offset);
    if (slots >= 1 && slots <= 60) {
      layout.visible_slots = static_cast<int>(slots);
      return;
    }
  }
}

bool read_string_at(const std::vector<uint8_t>& body, size_t& offset,
                    std::string& out) {
  if (offset + 4 > body.size()) return false;
  const uint32_t len = ru32(body, offset);
  offset += 4;
  if (len > body.size() - offset ||
      len > static_cast<uint32_t>(kMaxEmbeddedStringLen))
    return false;
  for (uint32_t i = 0; i < len; ++i) {
    const uint8_t c = body[offset + i];
    if (c < 0x20 || c >= 0x7f) return false;
  }
  out.assign(reinterpret_cast<const char*>(body.data() + offset), len);
  offset += len;
  return true;
}

bool is_menu_font_name(const std::string& text) {
  static constexpr const char* kNames[] = {
      "impact", "dyingmarker", "helveticablackcondensed",
      "clarendon", "rockletters", "hand", "helveticablack",
      "cutout", "gunsho", "receipt", "helveticathin", "impactor",
      "impactor2", "impactor_mtv", "rokk", "tapeworm",
      "tapewormscreen", "serif", "stars", "blockletters",
      "blockletters_fill"};
  for (const char* name : kNames)
    if (text == name) return true;
  return false;
}

bool is_parent_ref(const std::string& text) {
  return (text.size() > 5 &&
          (text.compare(text.size() - 5, 5, ".view") == 0 ||
           text.compare(text.size() - 4, 4, ".grp") == 0));
}

bool parse_draw_showing_after_parent(const std::vector<uint8_t>& body,
                                     const EmbeddedString& parent,
                                     bool& showing) {
  if (parent.end + 5 > body.size()) return false;
  if (ri32(body, parent.end) != 3) return false;
  showing = body[parent.end + 4] != 0;
  return true;
}

std::vector<uint8_t> load_entry_body(const std::string& hdr_path,
                                     const std::string& ark_path,
                                     const std::string& milo_path,
                                     const std::string& type,
                                     const std::string& name) {
  auto ark = gh::ark::ArkV3Reader::load(hdr_path);
  auto entry = ark.find(milo_path);
  if (!entry) entry = ark.find("../../system/run/" + milo_path);
  if (!entry) return {};
  auto bytes = ark.read_entry(*entry, {ark_path});
  auto h = gh::milo::parse_header(bytes);
  auto payload = gh::milo::inflate_payload(bytes, h);
  auto dir = gh::milo::parse_directory(payload);
  for (const auto& e : dir.entries) {
    if (e.type != type || e.name != name || e.offset + e.size > payload.size())
      continue;
    return std::vector<uint8_t>(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
  }
  return {};
}

}  // namespace

std::vector<MenuLabel> extract_menu_labels(const std::string& hdr_path,
                                           const std::string& ark_path,
                                           const std::string& milo_path) {
  std::vector<MenuLabel> out;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find(milo_path);
    if (!entry) entry = ark.find("../../system/run/" + milo_path);
    if (!entry) return out;
    auto bytes = ark.read_entry(*entry, {ark_path});
    auto h = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, h);
    auto dir = gh::milo::parse_directory(payload);

    for (const auto& e : dir.entries) {
      if (e.type != "BandButton" && e.type != "Text" && e.type != "BandLabel")
        continue;
      if (e.offset + e.size > payload.size()) continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      MenuLabel lbl;
      lbl.name = e.name;
      lbl.type = e.type;
      auto strs = embedded_strings(body);
      if (!strs.empty()) {
        lbl.text = strs.back().text;
        if (strs.size() >= 2) lbl.font = strs.front().text;
        for (const auto& s : strs) {
          if (is_parent_ref(s.text)) {
            lbl.parent = s.text;
            lbl.has_showing =
                parse_draw_showing_after_parent(body, s, lbl.showing);
            break;
          }
        }
        // nav target = an embedded "*.btn" string that isn't this object's name
        // (the focus-down link, e.g. main_career.btn -> main_quickspin.btn).
        for (const auto& s : strs) {
          if (s.text.size() > 4 && s.text.compare(s.text.size() - 4, 4, ".btn") == 0 &&
              s.text != e.name) {
            lbl.nav = s.text;
            break;
          }
        }
        if (e.type == "BandButton")
          parse_bandbutton_tail(body, strs.back().end, lbl.button_tail);
        if (e.type == "BandLabel") {
          parse_bandlabel_tail(body, strs.back().end, lbl.text_tail);
          if (!lbl.text_tail.valid && strs.size() == 1 &&
              is_menu_font_name(lbl.text)) {
            continue;
          }
        }
      }
      lbl.has_local =
          find_local_world_matrices(body, lbl.local, lbl.world);
      lbl.has_world = lbl.has_local;
      out.push_back(std::move(lbl));
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] %s: %s\n", milo_path.c_str(), ex.what());
  }
  return out;
}

UiListLayout extract_ui_list_layout(const std::string& hdr_path,
                                    const std::string& ark_path,
                                    const std::string& milo_path,
                                    const std::string& list_name) {
  UiListLayout layout;
  try {
    const std::vector<uint8_t> body =
        load_entry_body(hdr_path, ark_path, milo_path, "UIList", list_name);
    if (body.empty()) return layout;

    for (const auto& s : embedded_strings(body)) {
      if (!s.text.empty() && s.text != list_name && s.text.find('.') == std::string::npos) {
        layout.provider = s.text;
        break;
      }
    }

    for (size_t m = 0; m + 96 + 9 + 4 <= body.size(); ++m) {
      if (!looks_like_matrix(body, m) || !looks_like_matrix(body, m + 48))
        continue;
      size_t parent_offset = m + 96 + 9;
      std::string parent;
      if (!read_string_at(body, parent_offset, parent) ||
          parent.find(".view") == std::string::npos) {
        continue;
      }

      layout.local_x = rf(body, m + 36);
      layout.local_z = rf(body, m + 44);
      layout.world_x = rf(body, m + 48 + 36);
      layout.world_z = rf(body, m + 48 + 44);
      layout.parent = parent;

      if (parent_offset + 0x31 <= body.size()) {
        const float slots = rf(body, parent_offset + 0x15);
        const float row_h = rf(body, parent_offset + 0x21);
        const float text_h = rf(body, parent_offset + 0x25);
        const uint32_t width = ru32(body, parent_offset + 0x2d);
        if (slots >= 1.0f && slots <= 12.0f)
          layout.visible_slots = static_cast<int>(std::lround(slots));
        if (row_h >= 1.0f && row_h <= 200.0f) layout.row_height = row_h;
        if (text_h >= 1.0f && text_h <= 200.0f) layout.text_height = text_h;
        if (width > 0 && width < 2000) layout.width_bound = static_cast<int>(width);
      }
      layout.valid = true;
      return layout;
    }

    for (size_t m = 0; m + 96 <= body.size(); ++m) {
      if (!looks_like_matrix(body, m) || !looks_like_matrix(body, m + 48))
        continue;
      layout.local_x = rf(body, m + 36);
      layout.local_z = rf(body, m + 44);
      layout.world_x = rf(body, m + 48 + 36);
      layout.world_z = rf(body, m + 48 + 44);

      const size_t tail = m + 96 + 9;
      if (tail + 0x31 <= body.size()) {
        const float slots = rf(body, tail + 0x15);
        const float row_h = rf(body, tail + 0x21);
        const float text_h = rf(body, tail + 0x25);
        const float alt_row_h = rf(body, tail + 0x29);
        const uint32_t width = ru32(body, tail + 0x2d);
        if (slots >= 1.0f && slots <= 60.0f)
          layout.visible_slots = static_cast<int>(std::lround(slots));
        if (row_h >= 1.0f && row_h <= 200.0f) layout.row_height = row_h;
        if (layout.row_height == 0.0f && alt_row_h >= 1.0f && alt_row_h <= 200.0f)
          layout.row_height = alt_row_h;
        if (text_h >= 1.0f && text_h <= 200.0f) layout.text_height = text_h;
        // credits.lst stores its visible window as an unaligned integer after
        // the row/text-height fields rather than the float slot count used by
        // ss_song.lst. This is still the serialized UIList body, not a renderer
        // constant.
        set_visible_slots_from_int_window(body, tail + 0x2d, tail + 0x3d,
                                          layout);
        if (width > 0 && width < 2000) layout.width_bound = static_cast<int>(width);
      }
      layout.valid = true;
      return layout;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] UIList %s/%s: %s\n", milo_path.c_str(),
                 list_name.c_str(), ex.what());
  }
  return layout;
}

UiListTemplateLayout extract_ui_list_template_layout(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path) {
  UiListTemplateLayout layout;
  try {
    auto load_text = [&](const char* name, UiListTextTemplate& out) {
      const std::vector<uint8_t> body =
          load_entry_body(hdr_path, ark_path, milo_path, "Text", name);
      return decode_text_template(body, name, out);
    };
    if (!load_text("header.txt", layout.header) ||
        !load_text("list.txt", layout.list)) {
      return layout;
    }
    load_text("stars.txt", layout.stars);
    load_text("score.txt", layout.score);
    load_text("blurb.txt", layout.blurb);
    layout.valid = true;
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] UIList template %s: %s\n",
                 milo_path.c_str(), ex.what());
  }
  return layout;
}

UiListTextTemplate extract_text_template_layout(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path, const std::string& text_name) {
  UiListTextTemplate out;
  try {
    const std::vector<uint8_t> body =
        load_entry_body(hdr_path, ark_path, milo_path, "Text", text_name);
    decode_text_template(body, text_name, out);
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] Text template %s/%s: %s\n",
                 milo_path.c_str(), text_name.c_str(), ex.what());
  }
  return out;
}

}  // namespace ghogx::ui
