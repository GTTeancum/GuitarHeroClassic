// engine/src/ui/menu_labels.cpp -- see menu_labels.h.

#include "ui/menu_labels.h"

#include "ark_v3.h"
#include "milo.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace ghogx::ui {

namespace {

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

// Find the embedded Trans and return its WORLD matrix (the second of the
// local+world pair). Scans every byte offset because the class prefix length
// (and its strings) shift the matrix off any fixed alignment.
bool find_world_matrix(const std::vector<uint8_t>& d, std::array<float, 12>& out) {
  for (size_t M = 0x10; M + 96 <= d.size(); ++M) {
    if (looks_like_matrix(d, M) && looks_like_matrix(d, M + 48)) {
      for (int i = 0; i < 12; ++i) out[i] = rf(d, M + 48 + i * 4);
      return true;
    }
  }
  // Fall back to a single matrix if no local+world pair exists.
  for (size_t M = 0x10; M + 48 <= d.size(); ++M) {
    if (looks_like_matrix(d, M)) {
      for (int i = 0; i < 12; ++i) out[i] = rf(d, M + i * 4);
      return true;
    }
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
    if (n >= 1 && n <= 64 && i + 4 + static_cast<size_t>(n) <= d.size()) {
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

bool parse_bandbutton_tail(const std::vector<uint8_t>& body, size_t label_end,
                           MenuLabel::ButtonTail& out) {
  // This tail is byte-aligned to the variable-length label string, not 4-byte
  // aligned. Offsets below are relative to the first byte after the label.
  if (label_end + 49 > body.size()) return false;
  MenuLabel::ButtonTail t;
  t.valid = true;
  t.all_caps = body[label_end + 0];
  t.label_width = rf(body, label_end + 4);
  t.box_height = rf(body, label_end + 8);
  t.field_0c = rf(body, label_end + 12);
  t.field_10 = ri32(body, label_end + 16);
  t.field_14 = ri32(body, label_end + 20);
  t.text_size = rf(body, label_end + 28);
  t.field_20 = rf(body, label_end + 32);
  t.field_24 = body[label_end + 36];
  t.kerning = rf(body, label_end + 37);
  t.field_29 = rf(body, label_end + 41);
  t.width_bound = rf(body, label_end + 45);

  const bool plausible =
      t.all_caps <= 1 && finite(t.label_width) && t.label_width > 0.0f &&
      t.label_width < 1000.0f && finite(t.box_height) && t.box_height > 0.0f &&
      t.box_height < 100.0f && finite(t.text_size) && t.text_size > 0.01f &&
      t.text_size < 10.0f && finite(t.kerning) && std::fabs(t.kerning) < 10.0f &&
      finite(t.width_bound) && t.width_bound > 0.0f && t.width_bound < 1000.0f;
  if (!plausible) return false;
  out = t;
  return true;
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
      }
      lbl.has_world = find_world_matrix(body, lbl.world);
      out.push_back(std::move(lbl));
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] %s: %s\n", milo_path.c_str(), ex.what());
  }
  return out;
}

}  // namespace ghogx::ui
