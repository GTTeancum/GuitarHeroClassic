// engine/src/ui/menu_labels.cpp -- see menu_labels.h.

#include "ui/menu_labels.h"

#include "ark_v3.h"
#include "milo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <optional>

namespace ghogx::ui {

namespace {

constexpr int32_t kMaxEmbeddedStringLen = 512;

float rf(const std::vector<uint8_t>& d, size_t o) {
  float v;
  std::memcpy(&v, d.data() + o, 4);
  return v;
}

float rf_or(const std::vector<uint8_t>& d, size_t o, float fallback) {
  if (o + 4 > d.size()) return fallback;
  const float v = rf(d, o);
  return (v == v && std::fabs(v) < 1e30f) ? v : fallback;
}

int32_t ri32(const std::vector<uint8_t>& d, size_t o) {
  int32_t v;
  std::memcpy(&v, d.data() + o, 4);
  return v;
}

float f32_from_i32_bits(std::int32_t bits) {
  float v;
  std::uint32_t u = static_cast<std::uint32_t>(bits);
  std::memcpy(&v, &u, sizeof(v));
  return v;
}

uint32_t ru32(const std::vector<uint8_t>& d, size_t o) {
  uint32_t v;
  std::memcpy(&v, d.data() + o, 4);
  return v;
}

std::uint16_t low_revision(uint32_t combined) {
  return static_cast<std::uint16_t>(combined & 0xffffu);
}

std::uint16_t high_revision(uint32_t combined) {
  return static_cast<std::uint16_t>((combined >> 16) & 0xffffu);
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

// Find the embedded Trans and return its WORLD matrix (the second of the
// local+world pair). Scans every byte offset because the class prefix length
// (and its strings) shift the matrix off any fixed alignment.
bool find_world_matrix(const std::vector<uint8_t>& d, std::array<float, 12>& out) {
  std::array<float, 12> local{};
  return find_local_world_matrices(d, local, out);
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

bool is_parent_ref(const std::string& text) {
  return (text.size() > 5 &&
          (text.compare(text.size() - 5, 5, ".view") == 0 ||
           text.compare(text.size() - 4, 4, ".grp") == 0));
}

bool is_ui_object_ref(const std::string& text) {
  const char* suffixes[] = {".btn", ".lbl", ".grp", ".view", ".lst",
                            ".sld", ".chk"};
  for (const char* suffix : suffixes) {
    const std::size_t len = std::strlen(suffix);
    if (text.size() > len &&
        text.compare(text.size() - len, len, suffix) == 0)
      return true;
  }
  return false;
}

bool parse_draw_showing_after_parent(const std::vector<uint8_t>& body,
                                     const EmbeddedString& parent,
                                     bool& showing) {
  if (parent.end + 5 > body.size()) return false;
  if (ri32(body, parent.end) != 3) return false;
  showing = body[parent.end + 4] != 0;
  return true;
}

bool read_bool_byte(const std::vector<uint8_t>& body, size_t& pos, bool& out) {
  if (pos >= body.size()) return false;
  const uint8_t v = body[pos++];
  if (v > 1) return false;
  out = v != 0;
  return true;
}

bool read_i32_cursor(const std::vector<uint8_t>& body, size_t& pos,
                     std::int32_t& out) {
  if (pos + 4 > body.size()) return false;
  out = ri32(body, pos);
  pos += 4;
  return true;
}

bool read_u32_cursor(const std::vector<uint8_t>& body, size_t& pos,
                     std::uint32_t& out) {
  if (pos + 4 > body.size()) return false;
  out = ru32(body, pos);
  pos += 4;
  return true;
}

bool read_f32_cursor(const std::vector<uint8_t>& body, size_t& pos,
                     float& out) {
  if (pos + 4 > body.size()) return false;
  out = rf(body, pos);
  pos += 4;
  return finite(out);
}

bool read_symbol_cursor(const std::vector<uint8_t>& body, size_t& pos,
                        std::string& out, size_t max_len = 256) {
  if (pos + 4 > body.size()) return false;
  const uint32_t len = ru32(body, pos);
  pos += 4;
  if (len > max_len || pos + len > body.size()) return false;
  out.assign(reinterpret_cast<const char*>(body.data() + pos),
             static_cast<size_t>(len));
  pos += len;
  for (char c : out) {
    const unsigned char u = static_cast<unsigned char>(c);
    if (u < 32 || u >= 127) return false;
  }
  return true;
}

bool read_symbol_cursor_trim_nul(const std::vector<uint8_t>& body, size_t& pos,
                                 std::string& out, size_t max_len = 256) {
  if (pos + 4 > body.size()) return false;
  const uint32_t len = ru32(body, pos);
  pos += 4;
  if (len > max_len || pos + len > body.size()) return false;
  out.assign(reinterpret_cast<const char*>(body.data() + pos),
             static_cast<size_t>(len));
  pos += len;
  if (!out.empty() && out.back() == '\0') out.pop_back();
  for (char c : out) {
    const unsigned char u = static_cast<unsigned char>(c);
    if (u < 32 || u >= 127) return false;
  }
  return true;
}

bool read_symbol_list_cursor(const std::vector<uint8_t>& body, size_t& pos,
                             std::vector<std::string>& out,
                             size_t max_count = 256) {
  std::uint32_t count = 0;
  if (!read_u32_cursor(body, pos, count) || count > max_count) return false;
  out.clear();
  out.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    std::string s;
    if (!read_symbol_cursor(body, pos, s)) return false;
    out.push_back(std::move(s));
  }
  return true;
}

bool read_cstring_cursor(const std::vector<uint8_t>& body, size_t& pos,
                         std::string& out, size_t max_len = 256) {
  const size_t start = pos;
  while (pos < body.size() && body[pos] != 0) {
    const unsigned char u = body[pos];
    if (u < 32 || u >= 127) return false;
    if (pos - start >= max_len) return false;
    ++pos;
  }
  if (pos >= body.size()) return false;
  out.assign(reinterpret_cast<const char*>(body.data() + start), pos - start);
  ++pos;
  return true;
}

bool skip_trans_object_refs(const std::vector<uint8_t>& body, size_t& pos,
                            bool length_prefixed) {
  std::uint32_t count = 0;
  if (!read_u32_cursor(body, pos, count) || count > 256) return false;
  for (std::uint32_t i = 0; i < count; ++i) {
    std::string ignored;
    if (length_prefixed) {
      if (!read_symbol_cursor_trim_nul(body, pos, ignored)) return false;
    } else {
      if (!read_cstring_cursor(body, pos, ignored)) return false;
    }
  }
  return true;
}

struct ParsedRndTrans {
  std::uint16_t revision = 0;
  std::uint16_t alt_revision = 0;
  std::uint32_t constraint = 0;
  bool preserve_scale = false;
  std::string target;
  std::string parent;
  std::array<float, 12> local{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  std::array<float, 12> world{{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}};
  size_t end = 0;
};

std::optional<ParsedRndTrans> parse_rnd_trans_at(
    const std::vector<uint8_t>& body, size_t rev_offset) {
  if (rev_offset + 4 + 96 > body.size()) return std::nullopt;
  const std::uint32_t combined = ru32(body, rev_offset);
  const std::uint16_t revision = low_revision(combined);
  const std::uint16_t alt_revision = high_revision(combined);
  if (revision == 0 || revision > 32) return std::nullopt;

  const size_t matrix_offset = rev_offset + 4;
  if (!looks_like_matrix(body, matrix_offset) ||
      !looks_like_matrix(body, matrix_offset + 48)) {
    return std::nullopt;
  }

  ParsedRndTrans parsed;
  parsed.revision = revision;
  parsed.alt_revision = alt_revision;
  read_matrix(body, matrix_offset, parsed.local);
  read_matrix(body, matrix_offset + 48, parsed.world);

  auto parse_tail = [&](bool length_prefixed_refs)
      -> std::optional<ParsedRndTrans> {
    ParsedRndTrans candidate = parsed;
    size_t pos = matrix_offset + 96;
    if (revision < 9 &&
        !skip_trans_object_refs(body, pos, length_prefixed_refs)) {
      return std::nullopt;
    }
    if (revision > 6) {
      if (!read_u32_cursor(body, pos, candidate.constraint)) return std::nullopt;
      if (candidate.constraint > 8) return std::nullopt;
    }
    if (revision > 5 &&
        !read_symbol_cursor_trim_nul(body, pos, candidate.target)) {
      return std::nullopt;
    }
    if (revision > 6 &&
        !read_bool_byte(body, pos, candidate.preserve_scale)) {
      return std::nullopt;
    }
    if (!read_symbol_cursor_trim_nul(body, pos, candidate.parent))
      return std::nullopt;
    if (!candidate.parent.empty() && !is_parent_ref(candidate.parent))
      return std::nullopt;
    candidate.end = pos;
    return candidate;
  };

  if (revision < 9) {
    if (auto parsed_symbol_refs = parse_tail(true)) return parsed_symbol_refs;
    return parse_tail(false);
  }
  return parse_tail(true);
}

bool parse_ui_list_tail_at(const std::vector<uint8_t>& body, size_t tail,
                           UiListLayout& out) {
  if (body.size() < 4) return false;
  UiListLayout layout;
  const uint32_t combined = ru32(body, 0);
  layout.revision = low_revision(combined);
  layout.alt_revision = high_revision(combined);
  if (layout.revision > 32) return false;

  size_t pos = tail;

  // Mirrors MiloLib Assets/UI/UIList.cs. The inherited UIComponent has already
  // been consumed by the caller-selected tail offset.
  if (layout.revision < 0x0f) {
    if (!read_i32_cursor(body, pos, layout.legacy_i)) return false;
    if (!read_i32_cursor(body, pos, layout.legacy_j)) return false;
    if (layout.revision > 4) {
      if (layout.revision > 6) {
        if (!read_i32_cursor(body, pos, layout.legacy_k)) return false;
      } else {
        if (!read_bool_byte(body, pos, layout.legacy_b8)) return false;
      }
    }
    if (layout.revision > 6 &&
        !read_bool_byte(body, pos, layout.legacy_b9)) return false;
    if (layout.revision > 8 &&
        !read_bool_byte(body, pos, layout.legacy_ba)) return false;
    if (layout.revision > 10 &&
        !read_i32_cursor(body, pos, layout.legacy_unk3)) return false;
    if (!read_i32_cursor(body, pos, layout.legacy_x)) return false;
  }

  if (!read_i32_cursor(body, pos, layout.num_display)) return false;
  if (layout.revision > 0x11 &&
      !read_i32_cursor(body, pos, layout.grid_span)) return false;
  if (!read_bool_byte(body, pos, layout.circular)) return false;
  if (!read_f32_cursor(body, pos, layout.speed)) return false;
  if (layout.revision > 0x0c &&
      !read_bool_byte(body, pos, layout.scroll_past_min)) return false;
  if (layout.revision > 7 &&
      !read_bool_byte(body, pos, layout.scroll_past_max)) return false;
  if (layout.revision > 2 &&
      !read_bool_byte(body, pos, layout.paginate)) return false;
  if (layout.revision > 3 &&
      !read_bool_byte(body, pos, layout.select_to_scroll)) return false;
  if (layout.revision >= 10 &&
      !read_i32_cursor(body, pos, layout.min_display)) return false;
  if (layout.revision >= 6 &&
      !read_i32_cursor(body, pos, layout.max_display)) return false;
  if (layout.revision == 1) {
    std::int32_t ignored = 0;
    if (!read_i32_cursor(body, pos, ignored)) return false;
    if (!read_i32_cursor(body, pos, ignored)) return false;
  }
  if (layout.revision >= 12 &&
      !read_i32_cursor(body, pos, layout.num_data)) return false;
  if (layout.revision >= 14 &&
      !read_f32_cursor(body, pos, layout.auto_scroll_pause)) return false;
  if (layout.revision >= 19 &&
      !read_bool_byte(body, pos, layout.auto_scroll_send_messages)) return false;
  if (layout.revision >= 0x10) {
    if (!read_symbol_list_cursor(body, pos, layout.extended_label_entries))
      return false;
    if (!read_symbol_list_cursor(body, pos, layout.extended_mesh_entries))
      return false;
    if (!read_symbol_list_cursor(body, pos, layout.extended_custom_entries))
      return false;
  }
  if (layout.revision >= 17) {
    if (!read_symbol_cursor(body, pos, layout.in_anim)) return false;
    if (!read_symbol_cursor(body, pos, layout.out_anim)) return false;
  }

  if (layout.num_display <= 0 || layout.num_display > 64) return false;
  if (layout.grid_span < 0 || layout.grid_span > 64) return false;
  if (layout.speed < 0.0f || layout.speed > 10.0f) return false;
  if (layout.min_display < 0 || layout.min_display > 64) return false;
  if (layout.max_display < -1 || layout.max_display > 64) return false;
  if (layout.num_data < 0 || layout.num_data > 10000) return false;
  if (layout.auto_scroll_pause < 0.0f || layout.auto_scroll_pause > 60.0f)
    return false;

  // Standalone entries usually end with 0xDEADDEAD. Accept exact end or that
  // marker so the parser is not dependent on padding trivia.
  if (pos != body.size()) {
    if (pos + 4 != body.size() || ru32(body, pos) != 0xDEADDEADu)
      return false;
  }

  layout.valid = true;
  layout.has_world = find_world_matrix(body, layout.world);
  if (layout.revision == 2 && !layout.has_legacy_row_metrics) {
    const float row_height = f32_from_i32_bits(layout.legacy_i);
    const float text_height = f32_from_i32_bits(layout.legacy_j);
    if (finite(row_height) && row_height >= 1.0f && row_height <= 200.0f &&
        finite(text_height) && text_height >= 1.0f &&
        text_height <= 200.0f) {
      layout.has_legacy_row_metrics = true;
      layout.legacy_visible_slots = static_cast<float>(layout.num_display);
      layout.legacy_row_height = row_height;
      layout.legacy_text_height = text_height;
    }
  }
  out = std::move(layout);
  return true;
}

std::optional<UiListLayout> parse_gh2_ps2_legacy_ui_list(
    const std::vector<uint8_t>& body) {
  if (body.size() < 4) return std::nullopt;
  UiListLayout layout;
  const uint32_t combined = ru32(body, 0);
  layout.revision = low_revision(combined);
  layout.alt_revision = high_revision(combined);
  if (layout.revision != 2) return std::nullopt;

  // GH2 PS2's setlist UIList is an early revision. The inherited Trans block
  // follows MiloLib's UIComponent path (local matrix, world matrix, constraint,
  // target, preserve-scale, parent). The visible list window then lives in the
  // compact legacy bytes after that parent reference.
  for (size_t m = 0x10; m + 96 + 9 <= body.size(); ++m) {
    if (!looks_like_matrix(body, m) || !looks_like_matrix(body, m + 48))
      continue;

    size_t pos = m + 96;
    if (pos + 4 > body.size()) continue;
    pos += 4;  // Trans constraint.

    std::string target;
    if (!read_symbol_cursor(body, pos, target)) continue;

    bool preserve_scale = false;
    if (!read_bool_byte(body, pos, preserve_scale)) continue;

    std::string parent;
    if (!read_symbol_cursor(body, pos, parent)) continue;
    if (parent.find(".view") == std::string::npos) continue;

    const size_t compact = pos;
    if (compact + 54 > body.size()) continue;
    const float visible_slots = rf(body, compact + 21);
    const float row_height = rf(body, compact + 33);
    const float text_height = rf(body, compact + 37);
    const uint32_t width_or_count = ru32(body, compact + 45);
    if (!finite(visible_slots) || visible_slots < 1.0f ||
        visible_slots > 32.0f)
      continue;
    if (!finite(row_height) || row_height < 1.0f || row_height > 200.0f)
      continue;
    if (!finite(text_height) || text_height < 1.0f ||
        text_height > 200.0f)
      continue;
    if (width_or_count == 0 || width_or_count > 10000) continue;

    layout.num_display = static_cast<std::int32_t>(std::lround(visible_slots));
    layout.min_display = 0;
    layout.max_display = -1;
    layout.num_data = static_cast<std::int32_t>(width_or_count);
    layout.circular = body[compact + 53] != 0;
    layout.speed = 0.0f;
    layout.has_legacy_row_metrics = true;
    layout.legacy_visible_slots = visible_slots;
    layout.legacy_row_height = row_height;
    layout.legacy_text_height = text_height;
    layout.valid = true;
    layout.has_world = true;
    layout.has_local = true;
    for (int i = 0; i < 12; ++i)
      layout.local[i] = rf(body, m + static_cast<size_t>(i) * 4);
    for (int i = 0; i < 12; ++i)
      layout.world[i] = rf(body, m + 48 + static_cast<size_t>(i) * 4);
    layout.parent = parent;
    return layout;
  }

  return std::nullopt;
}

std::optional<MenuCheckbox> parse_checkbox_source_order(
    const std::vector<uint8_t>& body, const std::string& type,
    const std::string& name) {
  if (body.size() < 12) return std::nullopt;

  // GH2 PS2 uses the older CheckBox class: combined revision, the checked bool
  // in the low byte of the next word, a 4-byte zero field, then the component
  // resource string and RndTrans. Later Harmonix CheckboxDisplay keeps the same
  // semantic checked field (MiloLib CheckboxDisplay.isChecked /
  // RB3 CheckboxDisplay::mChecked).
  const uint32_t combined = ru32(body, 0);
  if (low_revision(combined) > 8) return std::nullopt;

  MenuCheckbox checkbox;
  checkbox.name = name;
  checkbox.type = type;
  checkbox.checked = body[4] != 0;

  size_t pos = 12;
  if (!read_symbol_cursor_trim_nul(body, pos, checkbox.resource))
    return std::nullopt;
  if (pos < body.size() && body[pos] == 0) ++pos;  // GH2 CheckBox resource pad.

  std::int32_t trans_rev = 0;
  if (!read_i32_cursor(body, pos, trans_rev) || trans_rev <= 0)
    return std::nullopt;
  if (pos + 96 > body.size()) return std::nullopt;
  read_matrix(body, pos, checkbox.local);
  checkbox.has_local = true;
  pos += 48;
  read_matrix(body, pos, checkbox.world);
  checkbox.has_world = true;
  pos += 48;

  std::uint32_t constraint = 0;
  if (!read_u32_cursor(body, pos, constraint)) return std::nullopt;
  (void)constraint;
  std::string target;
  if (!read_symbol_cursor_trim_nul(body, pos, target)) return std::nullopt;
  bool preserve_scale = false;
  if (!read_bool_byte(body, pos, preserve_scale)) return std::nullopt;
  (void)preserve_scale;
  if (!read_symbol_cursor_trim_nul(body, pos, checkbox.parent))
    return std::nullopt;

  if (pos + 5 <= body.size() && ri32(body, pos) == 3)
    checkbox.showing = body[pos + 4] != 0;
  return checkbox;
}

std::optional<MenuSlider> parse_slider_source_order(
    const std::vector<uint8_t>& body, const std::string& type,
    const std::string& name) {
  if (body.size() < 20) return std::nullopt;

  const uint32_t combined = ru32(body, 0);
  if (low_revision(combined) > 8) return std::nullopt;

  MenuSlider slider;
  slider.name = name;
  slider.type = type;

  // GH2 PS2's BandSlider compact prefix matches the Harmonix UISlider runtime
  // fields used by Frame(): mCurrent, mNumSteps, mVertical, then the resource
  // style symbol before the inherited Trans/Draw block.
  slider.current = ri32(body, 4);
  slider.num_steps = ri32(body, 8);
  slider.vertical = ri32(body, 12) != 0;
  if (slider.current < 0 || slider.current > 10000) return std::nullopt;
  if (slider.num_steps < 1 || slider.num_steps > 10000) return std::nullopt;

  size_t pos = 16;
  if (!read_symbol_cursor_trim_nul(body, pos, slider.resource))
    return std::nullopt;
  if (pos < body.size() && body[pos] == 0) ++pos;  // GH2 resource pad.

  std::int32_t trans_rev = 0;
  if (!read_i32_cursor(body, pos, trans_rev) || trans_rev <= 0)
    return std::nullopt;
  if (pos + 96 > body.size()) return std::nullopt;
  read_matrix(body, pos, slider.local);
  slider.has_local = true;
  pos += 48;
  read_matrix(body, pos, slider.world);
  slider.has_world = true;
  pos += 48;

  std::uint32_t constraint = 0;
  if (!read_u32_cursor(body, pos, constraint)) return std::nullopt;
  (void)constraint;
  std::string target;
  if (!read_symbol_cursor_trim_nul(body, pos, target)) return std::nullopt;
  bool preserve_scale = false;
  if (!read_bool_byte(body, pos, preserve_scale)) return std::nullopt;
  (void)preserve_scale;
  if (!read_symbol_cursor_trim_nul(body, pos, slider.parent))
    return std::nullopt;

  if (pos + 25 <= body.size() && ri32(body, pos) == 3) {
    slider.showing = body[pos + 4] != 0;
    pos += 25;  // RndDrawable rev/showing/bounds/draw-order.
  }

  while (pos + 4 <= body.size()) {
    std::string s;
    size_t candidate = pos;
    if (!read_symbol_cursor_trim_nul(body, candidate, s)) {
      ++pos;
      continue;
    }
    if (!s.empty()) {
      if ((s.size() > 4 &&
           (s.compare(s.size() - 4, 4, ".sld") == 0 ||
            s.compare(s.size() - 4, 4, ".btn") == 0)) &&
          s != name && slider.nav.empty()) {
        slider.nav = s;
      } else if (slider.token.empty() && s.find('.') == std::string::npos) {
        slider.token = s;
      }
    }
    pos = candidate;
  }

  return slider;
}

bool read_vec3_key_cursor(const std::vector<uint8_t>& body, size_t& pos,
                          std::array<float, 3>& vec, float& frame) {
  if (!read_f32_cursor(body, pos, vec[0])) return false;
  if (!read_f32_cursor(body, pos, vec[1])) return false;
  if (!read_f32_cursor(body, pos, vec[2])) return false;
  if (!read_f32_cursor(body, pos, frame)) return false;
  return true;
}

bool read_quat_key_cursor(const std::vector<uint8_t>& body, size_t& pos,
                          std::array<float, 4>& quat, float& frame) {
  if (!read_f32_cursor(body, pos, quat[0])) return false;
  if (!read_f32_cursor(body, pos, quat[1])) return false;
  if (!read_f32_cursor(body, pos, quat[2])) return false;
  if (!read_f32_cursor(body, pos, quat[3])) return false;
  if (!read_f32_cursor(body, pos, frame)) return false;
  return true;
}

std::optional<MenuSliderAnim> parse_slider_trans_anim_source_order(
    const std::vector<uint8_t>& body, const std::string& name) {
  if (body.size() < 32) return std::nullopt;
  const uint32_t combined = ru32(body, 0);
  const std::uint16_t revision = low_revision(combined);
  if (revision > 16) return std::nullopt;

  size_t pos = 4;
  if (revision > 4) {
    if (pos + 9 > body.size()) return std::nullopt;
    pos += 9;  // Hmx::Object base bytes.
  }

  std::uint32_t anim_combined = 0;
  if (!read_u32_cursor(body, pos, anim_combined)) return std::nullopt;
  const std::uint16_t anim_revision = low_revision(anim_combined);
  if (anim_revision > 16) return std::nullopt;
  if (anim_revision > 1) {
    float frame = 0.0f;
    if (!read_f32_cursor(body, pos, frame)) return std::nullopt;
  }
  if (anim_revision < 4) {
    if (anim_revision > 2) {
      bool rate = false;
      if (!read_bool_byte(body, pos, rate)) return std::nullopt;
    }
  } else {
    std::uint32_t rate = 0;
    if (!read_u32_cursor(body, pos, rate)) return std::nullopt;
  }

  if (revision < 6) {
    // The stock slider resource is rev 6; older TransAnim draw parsing is not
    // needed for this menu path yet.
    return std::nullopt;
  }

  MenuSliderAnim anim;
  anim.name = name;
  if (!read_symbol_cursor_trim_nul(body, pos, anim.target)) return std::nullopt;

  bool have_frame_range = false;
  const auto note_frame = [&](float frame) {
    if (!finite(frame)) return;
    if (!have_frame_range) {
      anim.first_frame = frame;
      anim.last_frame = frame;
      have_frame_range = true;
    } else {
      anim.first_frame = std::min(anim.first_frame, frame);
      anim.last_frame = std::max(anim.last_frame, frame);
    }
  };
  if (revision > 2) {
    std::uint32_t rot_count = 0;
    if (!read_u32_cursor(body, pos, rot_count) || rot_count > 1024)
      return std::nullopt;
    anim.rotation_keys.reserve(rot_count);
    for (std::uint32_t i = 0; i < rot_count; ++i) {
      MenuTransQuatKey key;
      if (!read_quat_key_cursor(body, pos, key.quat_xyzw, key.frame))
        return std::nullopt;
      note_frame(key.frame);
      anim.rotation_keys.push_back(key);
    }

    std::uint32_t trans_count = 0;
    if (!read_u32_cursor(body, pos, trans_count) || trans_count > 1024)
      return std::nullopt;
    anim.translation_keys.reserve(trans_count);
    for (std::uint32_t i = 0; i < trans_count; ++i) {
      MenuTransVecKey key;
      if (!read_vec3_key_cursor(body, pos, key.value, key.frame))
        return std::nullopt;
      anim.translation_keys.push_back(key);
      note_frame(key.frame);
      if (i == 0) {
        anim.first = key.value;
      }
      anim.last = key.value;
    }
  } else {
    return std::nullopt;
  }

  if (!read_symbol_cursor_trim_nul(body, pos, anim.keys_owner))
    return std::nullopt;
  if (revision > 3) {
    bool trans_spline = false;
    if (!read_bool_byte(body, pos, trans_spline)) return std::nullopt;
  } else {
    std::uint32_t trans_spline = 0;
    if (!read_u32_cursor(body, pos, trans_spline)) return std::nullopt;
  }
  bool repeat_trans = false;
  if (!read_bool_byte(body, pos, repeat_trans)) return std::nullopt;
  if (revision > 3) {
    std::uint32_t scale_count = 0;
    if (!read_u32_cursor(body, pos, scale_count) || scale_count > 1024)
      return std::nullopt;
    anim.scale_keys.reserve(scale_count);
    for (std::uint32_t i = 0; i < scale_count; ++i) {
      MenuTransVecKey key;
      if (!read_vec3_key_cursor(body, pos, key.value, key.frame))
        return std::nullopt;
      anim.scale_keys.push_back(key);
      note_frame(key.frame);
    }
    bool scale_spline = false;
    if (!read_bool_byte(body, pos, scale_spline)) return std::nullopt;
  } else if (revision > 0 && revision != 2) {
    std::uint32_t scale_count = 0;
    if (!read_u32_cursor(body, pos, scale_count) || scale_count > 1024)
      return std::nullopt;
    anim.scale_keys.reserve(scale_count);
    for (std::uint32_t i = 0; i < scale_count; ++i) {
      MenuTransVecKey key;
      if (!read_vec3_key_cursor(body, pos, key.value, key.frame))
        return std::nullopt;
      anim.scale_keys.push_back(key);
      note_frame(key.frame);
    }
    std::uint32_t scale_spline = 0;
    if (!read_u32_cursor(body, pos, scale_spline)) return std::nullopt;
  }

  if (anim.rotation_keys.empty() && anim.translation_keys.empty() &&
      anim.scale_keys.empty() && anim.keys_owner.empty())
    return std::nullopt;
  anim.valid = true;
  return anim;
}

std::optional<MenuAnimFilter> parse_anim_filter_source_order(
    const std::vector<uint8_t>& body, const std::string& name) {
  if (body.size() < 41) return std::nullopt;
  size_t pos = 0;
  std::uint32_t combined = 0;
  if (!read_u32_cursor(body, pos, combined) || low_revision(combined) != 1)
    return std::nullopt;

  // Hmx::Object fields for directory version 24: revision/type bytes plus the
  // legacy proxy/load flags. This is the exact nine-byte block consumed by
  // ObjectFields in MiloEditor/Harmonix source.
  if (pos + 9 > body.size()) return std::nullopt;
  pos += 9;

  // Embedded RndAnimatable. GH2 AnimFilter entries use revision 4: frame then
  // the serialized RndAnimatable::Rate enum.
  std::uint32_t anim_combined = 0;
  if (!read_u32_cursor(body, pos, anim_combined)) return std::nullopt;
  const std::uint16_t anim_revision = low_revision(anim_combined);
  if (anim_revision != 4) return std::nullopt;

  MenuAnimFilter filter;
  filter.name = name;
  std::uint32_t rate = 0;
  if (!read_f32_cursor(body, pos, filter.frame) ||
      !read_u32_cursor(body, pos, rate) ||
      !read_symbol_cursor_trim_nul(body, pos, filter.trans_anim) ||
      !read_f32_cursor(body, pos, filter.scale) ||
      !read_f32_cursor(body, pos, filter.offset) ||
      !read_f32_cursor(body, pos, filter.start) ||
      !read_f32_cursor(body, pos, filter.end) ||
      !read_i32_cursor(body, pos, filter.type) ||
      !read_f32_cursor(body, pos, filter.period)) {
    return std::nullopt;
  }
  (void)rate;
  if (pos != body.size()) return std::nullopt;
  if (!finite(filter.frame) || !finite(filter.scale) ||
      std::fabs(filter.scale) <= 0.0001f || !finite(filter.offset) ||
      !finite(filter.start) || !finite(filter.end) ||
      !finite(filter.period) || filter.type < 0 || filter.type > 2 ||
      filter.trans_anim.empty()) {
    return std::nullopt;
  }
  filter.valid = true;
  return filter;
}

std::optional<MenuUiTrigger> parse_ui_trigger_source_order(
    const std::vector<uint8_t>& body, const std::string& name) {
  if (body.size() < 0xB4) return std::nullopt;
  size_t pos = 0;
  std::uint32_t combined = 0;
  if (!read_u32_cursor(body, pos, combined)) return std::nullopt;

  MenuUiTrigger trigger;
  trigger.name = name;
  trigger.revision = low_revision(combined);
  if (trigger.revision != 0) return std::nullopt;

  std::uint32_t component_combined = 0;
  if (!read_u32_cursor(body, pos, component_combined)) return std::nullopt;
  trigger.component_revision = low_revision(component_combined);
  if (trigger.component_revision != 1) return std::nullopt;

  if (pos + 9 > body.size()) return std::nullopt;
  pos += 9;  // embedded UIComponent Hmx::Object fields

  const auto trans = parse_rnd_trans_at(body, pos);
  if (!trans || trans->revision != 9) return std::nullopt;
  pos = trans->end;

  // Embedded RndDrawable revision 3: rev, showing, bounding sphere, draw order.
  std::uint32_t drawable_combined = 0;
  if (!read_u32_cursor(body, pos, drawable_combined) ||
      low_revision(drawable_combined) != 3 || pos + 21 > body.size()) {
    return std::nullopt;
  }
  pos += 21;

  std::string nav_right;
  std::string nav_down;
  if (!read_symbol_cursor_trim_nul(body, pos, nav_right) ||
      !read_symbol_cursor_trim_nul(body, pos, nav_down) ||
      !read_symbol_cursor_trim_nul(body, pos, trigger.event) ||
      !read_symbol_cursor_trim_nul(body, pos, trigger.anim_ref) ||
      !read_bool_byte(body, pos, trigger.block_transition) ||
      pos != body.size()) {
    return std::nullopt;
  }
  trigger.valid = !trigger.event.empty();
  return trigger.valid ? std::optional<MenuUiTrigger>(std::move(trigger))
                       : std::nullopt;
}

bool skip_f32_values(const std::vector<uint8_t>& body, size_t& pos,
                     std::uint32_t count, std::uint32_t floats_per_key) {
  if (count > 256 || floats_per_key > 8) return false;
  const size_t bytes = static_cast<size_t>(count) * floats_per_key * 4u;
  if (pos + bytes > body.size()) return false;
  pos += bytes;
  return true;
}

std::optional<MenuMaterialAnim> parse_material_anim_source_order(
    const std::vector<uint8_t>& body, const std::string& name) {
  if (body.size() < 33 || ri32(body, 0) != 0x07) return std::nullopt;
  size_t pos = 25;
  MenuMaterialAnim out;
  out.name = name;
  if (!read_symbol_cursor(body, pos, out.material)) return std::nullopt;
  if (!read_symbol_cursor(body, pos, out.keys_owner)) return std::nullopt;

  bool have_frame = false;
  const auto note_frame = [&](float frame) {
    if (!finite(frame)) return;
    if (!have_frame) {
      out.first_frame = out.last_frame = frame;
      have_frame = true;
    } else {
      out.first_frame = std::min(out.first_frame, frame);
      out.last_frame = std::max(out.last_frame, frame);
    }
  };
  std::uint32_t count = 0;
  if (!read_u32_cursor(body, pos, count) || count > 256)
    return std::nullopt;
  out.color_keys.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    MenuMaterialColorKey key;
    for (float& channel : key.color)
      if (!read_f32_cursor(body, pos, channel)) return std::nullopt;
    if (!read_f32_cursor(body, pos, key.frame)) return std::nullopt;
    note_frame(key.frame);
    out.color_keys.push_back(key);
  }
  if (!read_u32_cursor(body, pos, count) || count > 256)
    return std::nullopt;
  out.alpha_keys.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    MenuMaterialFloatKey key;
    if (!read_f32_cursor(body, pos, key.value) ||
        !read_f32_cursor(body, pos, key.frame))
      return std::nullopt;
    note_frame(key.frame);
    out.alpha_keys.push_back(key);
  }
  const auto read_vec_keys = [&](std::vector<MenuTransVecKey>& keys) {
    std::uint32_t key_count = 0;
    if (!read_u32_cursor(body, pos, key_count) || key_count > 256)
      return false;
    keys.reserve(key_count);
    for (std::uint32_t i = 0; i < key_count; ++i) {
      MenuTransVecKey key;
      if (!read_vec3_key_cursor(body, pos, key.value, key.frame)) return false;
      note_frame(key.frame);
      keys.push_back(key);
    }
    return true;
  };
  if (!read_vec_keys(out.translation_keys) ||
      !read_vec_keys(out.scale_keys) || !read_vec_keys(out.rotation_keys))
    return std::nullopt;

  std::uint32_t texture_count = 0;
  if (!read_u32_cursor(body, pos, texture_count) || texture_count > 256)
    return std::nullopt;
  out.texture_keys.reserve(texture_count);
  for (std::uint32_t i = 0; i < texture_count; ++i) {
    MenuMaterialTextureKey key;
    if (!read_symbol_cursor(body, pos, key.texture) ||
        !read_f32_cursor(body, pos, key.frame)) {
      return std::nullopt;
    }
    if (!finite(key.frame)) key.frame = 0.0f;
    note_frame(key.frame);
    out.texture_keys.push_back(std::move(key));
  }
  // RndMatAnim::Load permits a null mMat.  Stock key-owner objects such as
  // cashaward/ca_highlight.mnm deliberately carry keys without a material;
  // another MatAnim points at them through mKeysOwner.  They are still fully
  // valid serialized animations even though SetFrame has no direct target.
  out.valid = !out.keys_owner.empty() && pos == body.size();
  return out.valid ? std::optional<MenuMaterialAnim>(std::move(out))
                   : std::nullopt;
}

std::optional<MenuProxyTransform> parse_proxy_transform_source_order(
    const std::vector<uint8_t>& body, const std::string& name) {
  MenuProxyTransform out;
  out.name = name;
  for (size_t rev_offset = 0; rev_offset + 4 + 96 <= body.size();
       ++rev_offset) {
    auto parsed = parse_rnd_trans_at(body, rev_offset);
    if (!parsed) continue;
    out.local = parsed->local;
    out.world = parsed->world;
    out.constraint = parsed->constraint;
    out.target = parsed->target;
    out.preserve_scale = parsed->preserve_scale;
    out.parent = parsed->parent;
    out.valid = true;
    return out;
  }

  bool found_structured = false;
  float best_score = 1e30f;
  MenuProxyTransform best;
  best.name = name;
  for (size_t m = 0x10; m + 96 + 9 <= body.size(); ++m) {
    if (!looks_like_matrix(body, m) || !looks_like_matrix(body, m + 48))
      continue;

    MenuProxyTransform candidate;
    candidate.name = name;
    read_matrix(body, m, candidate.local);
    read_matrix(body, m + 48, candidate.world);

    size_t pos = m + 96;
    if (!read_u32_cursor(body, pos, candidate.constraint)) continue;
    if (!read_symbol_cursor_trim_nul(body, pos, candidate.target)) continue;
    if (!read_bool_byte(body, pos, candidate.preserve_scale)) continue;
    if (!read_symbol_cursor_trim_nul(body, pos, candidate.parent)) continue;
    if (!candidate.parent.empty() && !is_parent_ref(candidate.parent))
      continue;

    const float score = matrix_basis_score(candidate.local) +
                        matrix_basis_score(candidate.world) +
                        (candidate.parent.empty() ? 100.0f : 0.0f);
    if (score < best_score) {
      best_score = score;
      best = std::move(candidate);
      found_structured = true;
    }
  }

  if (found_structured) {
    best.valid = true;
    return best;
  }

  if (!find_local_world_matrices(body, out.local, out.world))
    return std::nullopt;
  for (const EmbeddedString& s : embedded_strings(body)) {
    if (is_parent_ref(s.text)) out.parent = s.text;
  }
  out.valid = true;
  return out;
}

std::optional<UiListLayout> parse_ui_list_layout_source_order(
    const std::vector<uint8_t>& body) {
  if (auto legacy = parse_gh2_ps2_legacy_ui_list(body)) return legacy;

  // UIList inherits UIComponent. MiloLib shows the UIList-owned tail follows the
  // UIComponent resourceName symbol, so try string boundaries first. Fall back to
  // a full scan because some stock lists carry empty or unusual resource fields.
  const auto strings = embedded_strings(body);
  std::vector<size_t> candidates;
  for (const auto& s : strings) {
    if (s.text.find(".milo") != std::string::npos ||
        s.text.find(".lst") != std::string::npos ||
        s.text.find(".lbl") != std::string::npos) {
      candidates.push_back(s.end);
    }
  }
  for (size_t i = 4; i < body.size(); ++i) candidates.push_back(i);

  for (size_t c : candidates) {
    UiListLayout layout;
    if (parse_ui_list_tail_at(body, c, layout)) return layout;
  }
  return std::nullopt;
}

std::optional<MenuTextStyle> parse_rnd_text_style_source_order(
    const std::vector<uint8_t>& body, const std::string& name) {
  if (body.size() < 32) return std::nullopt;
  const uint32_t combined = ru32(body, 0);
  const std::uint16_t revision = low_revision(combined);
  if (revision < 13 || revision > 21) return std::nullopt;

  const auto parse_tail = [&](size_t pos, std::string font,
                              std::string parent)
      -> std::optional<MenuTextStyle> {
    MenuTextStyle style;
    style.name = name;
    style.font = std::move(font);
    style.parent = std::move(parent);

    if (!read_i32_cursor(body, pos, style.alignment)) return std::nullopt;
    if (style.alignment < 0 || style.alignment >= 255) return std::nullopt;
    if (!read_symbol_cursor(body, pos, style.text, 4096)) return std::nullopt;

    if (revision != 0) {
      for (int c = 0; c < 4; ++c) {
        if (!read_f32_cursor(body, pos, style.color[c])) return std::nullopt;
        if (style.color[c] < 0.0f || style.color[c] > 1.0f)
          return std::nullopt;
      }
    }
    if (revision > 0x0c) {
      if (!read_f32_cursor(body, pos, style.wrap_width)) return std::nullopt;
    } else if (revision > 3) {
      bool has_wrap = false;
      if (!read_bool_byte(body, pos, has_wrap)) return std::nullopt;
      if (!read_f32_cursor(body, pos, style.wrap_width)) return std::nullopt;
      if (!has_wrap) style.wrap_width = 0.0f;
    }
    if (revision > 7 &&
        !read_f32_cursor(body, pos, style.leading)) return std::nullopt;
    if (revision > 0x0b &&
        !read_i32_cursor(body, pos, style.fixed_length)) return std::nullopt;
    if (revision > 9 &&
        !read_f32_cursor(body, pos, style.italic_strength)) return std::nullopt;
    if (revision > 0x0c) {
      if (!read_f32_cursor(body, pos, style.text_size)) return std::nullopt;
    }
    if (revision > 0x0d &&
        !read_bool_byte(body, pos, style.markup)) return std::nullopt;
    if (revision > 0x0e &&
        !read_i32_cursor(body, pos, style.caps_mode)) return std::nullopt;

    const bool plausible =
        finite(style.wrap_width) && style.wrap_width >= 0.0f &&
        style.wrap_width < 20000.0f && finite(style.leading) &&
        style.leading >= 0.0f && style.leading < 20.0f &&
        finite(style.italic_strength) &&
        std::fabs(style.italic_strength) < 1000.0f &&
        finite(style.text_size) && style.text_size > 0.0f &&
        style.text_size < 1000.0f && style.fixed_length >= 0 &&
        style.fixed_length < 65535 && style.caps_mode >= 0 &&
        style.caps_mode < 255;
    if (!plausible) return std::nullopt;

    style.has_local = find_local_world_matrices(body, style.local, style.world);
    style.has_world = style.has_local;
    style.valid = true;
    return style;
  };

  const auto strings = embedded_strings(body);
  // RndText serializes its Transformable parent immediately before mFont.
  // mFont is a nullable ObjPtr, so an empty symbol is valid (sc_label1.txt is
  // the shipped GH2 example). Start from the parent boundary instead of
  // assuming the first printable string is the font.
  for (const auto& parent : strings) {
    if (!is_parent_ref(parent.text)) continue;
    size_t pos = parent.end;
    std::string font;
    if (!read_symbol_cursor(body, pos, font)) continue;
    if (!font.empty() && font.find(".font") == std::string::npos) continue;
    if (auto parsed = parse_tail(pos, std::move(font), parent.text))
      return parsed;
  }

  // Parentless RndText objects still expose a non-null mFont. Retain the
  // direct font boundary as the source-order fallback.
  for (size_t i = 0; i < strings.size(); ++i) {
    const auto& font = strings[i];
    if (font.text.find(".font") == std::string::npos) continue;
    if (auto parsed = parse_tail(font.end, font.text,
                                 i > 0 ? strings[i - 1].text : std::string{}))
      return parsed;
  }

  return std::nullopt;
}

bool parse_bandbutton_tail(const std::vector<uint8_t>& body, size_t label_end,
                           MenuLabel::ButtonTail& out) {
  if (label_end + 49 > body.size()) return false;

  // Legacy GH2 BandButton tail: first byte is all_caps, then the authored text
  // metrics. This is the shape used by main.milo_ps2.
  MenuLabel::ButtonTail legacy;
  legacy.valid = true;
  legacy.legacy_layout = true;
  legacy.all_caps = body[label_end + 0];
  legacy.width = rf(body, label_end + 4);
  legacy.height = rf(body, label_end + 8);
  legacy.leading = rf(body, label_end + 12);
  legacy.unknown_10 = ri32(body, label_end + 16);
  legacy.alignment = legacy.unknown_10;
  legacy.unknown_14 = ri32(body, label_end + 20);
  legacy.text_size = rf(body, label_end + 28);
  legacy.unknown_20 = rf(body, label_end + 32);
  legacy.unknown_24 = body[label_end + 36];
  legacy.kerning = rf(body, label_end + 37);
  legacy.wrap_width = rf(body, label_end + 41);
  legacy.width_bound = rf(body, label_end + 45);

  const bool legacy_plausible =
      legacy.all_caps <= 1 && finite(legacy.width) && legacy.width > 0.0f &&
      legacy.width < 1000.0f && finite(legacy.height) &&
      legacy.height > 0.0f && legacy.height < 100.0f &&
      finite(legacy.text_size) && legacy.text_size > 0.01f &&
      legacy.text_size < 10.0f && finite(legacy.kerning) &&
      std::fabs(legacy.kerning) < 10.0f && finite(legacy.width_bound) &&
      legacy.width_bound > 0.0f && legacy.width_bound < 1000.0f;
  if (legacy_plausible) {
    out = legacy;
    return true;
  }

  // Newer UILabel/BandButton tail: MiloLib names the first owned fields as
  // fitType, width, height, leading, and alignment. Video settings uses this
  // layout; its text size lives after the byte-sized all_caps/kerning slot.
  MenuLabel::ButtonTail modern;
  modern.legacy_layout = false;
  modern.fit_text = ri32(body, label_end + 0);
  modern.width = rf(body, label_end + 4);
  modern.height = rf(body, label_end + 8);
  modern.leading = rf(body, label_end + 12);
  modern.alignment = ri32(body, label_end + 16);
  modern.unknown_10 = modern.alignment;
  modern.unknown_14 = ri32(body, label_end + 20);
  modern.all_caps = body[label_end + 36];
  modern.unknown_24 = modern.all_caps;
  modern.kerning = rf(body, label_end + 37);
  modern.text_size = rf(body, label_end + 41);
  modern.width_bound = rf(body, label_end + 45);

  const bool modern_plausible =
      modern.fit_text >= 0 && modern.fit_text <= 2 &&
      modern.all_caps <= 1 && finite(modern.width) &&
      modern.width > 0.0f && modern.width < 1000.0f &&
      finite(modern.height) && modern.height > 0.0f &&
      modern.height < 200.0f && finite(modern.leading) &&
      modern.leading > 0.0f && modern.leading < 10.0f &&
      modern.alignment >= 0 && modern.alignment < 255 &&
      finite(modern.text_size) && modern.text_size > 0.01f &&
      modern.text_size < 200.0f && finite(modern.kerning) &&
      std::fabs(modern.kerning) < 10.0f &&
      finite(modern.width_bound) && modern.width_bound > 0.0f &&
      modern.width_bound < 2000.0f;
  if (!modern_plausible) return false;
  modern.valid = true;
  out = modern;
  return true;
}

bool parse_bandlabel_tail(const std::vector<uint8_t>& body, size_t label_end,
                          MenuLabel::TextTail& out) {
  // PS2 BandLabel bodies use the same unaligned tail style as BandButton, with
  // RGBA serialized after the width/text-size fields.
  if (label_end + 65 > body.size()) return false;
  MenuLabel::TextTail t;
  t.valid = true;
  t.fit_text = ri32(body, label_end + 0);
  t.width = rf(body, label_end + 4);
  t.height = rf(body, label_end + 8);
  t.leading = rf(body, label_end + 12);
  t.alignment = ri32(body, label_end + 16);
  t.unknown_14 = ri32(body, label_end + 20);
  t.all_caps = body[label_end + 36];
  t.kerning = rf(body, label_end + 37);
  t.text_size = rf(body, label_end + 41);
  t.width_bound = rf(body, label_end + 45);
  for (int i = 0; i < 4; ++i)
    t.color[i] = rf(body, label_end + 49 + static_cast<size_t>(i) * 4);

  const bool plausible =
      t.fit_text >= 0 && t.fit_text <= 2 && t.all_caps <= 1 &&
      finite(t.width) && t.width >= 0.0f &&
      t.width < 2000.0f && finite(t.height) &&
      t.height >= 0.0f && t.height < 500.0f &&
      finite(t.text_size) && t.text_size > 0.01f &&
      t.text_size < 200.0f && finite(t.kerning) &&
      std::fabs(t.kerning) < 10.0f && finite(t.width_bound) &&
      t.width_bound >= 0.0f && t.width_bound <= 20000.0f;
  if (!plausible) return false;
  for (float c : t.color) {
    if (!finite(c) || c < 0.0f || c > 1.0f) return false;
  }
  out = t;
  return true;
}

bool parse_bandtextentry_tail(const std::vector<uint8_t>& body,
                              MenuLabel::TextEntryTail& out) {
  constexpr size_t kTailBytes = 44;
  if (body.size() < kTailBytes) return false;
  const size_t pos = body.size() - kTailBytes;
  MenuLabel::TextEntryTail t;
  t.flash_time = rf(body, pos + 0);
  t.text_scale = rf(body, pos + 4);
  t.arrow_offset = rf(body, pos + 8);
  for (int i = 0; i < 4; ++i) {
    t.entered_color[i] = rf(body, pos + 12 + static_cast<size_t>(i) * 4);
    t.dynamic_color[i] = rf(body, pos + 28 + static_cast<size_t>(i) * 4);
  }
  const bool plausible =
      finite(t.flash_time) && t.flash_time > 0.0f && t.flash_time < 30.0f &&
      finite(t.text_scale) && t.text_scale >= 0.0f && t.text_scale <= 2.0f &&
      finite(t.arrow_offset) && std::fabs(t.arrow_offset) < 100.0f;
  if (!plausible) return false;
  for (float c : t.entered_color)
    if (!finite(c) || c < 0.0f || c > 1.0f) return false;
  for (float c : t.dynamic_color)
    if (!finite(c) || c < 0.0f || c > 1.0f) return false;
  t.valid = true;
  out = t;
  return true;
}

}  // namespace

MenuSliderAnim decode_menu_trans_anim_body(
    const std::vector<std::uint8_t>& body, const std::string& name) {
  if (auto parsed = parse_slider_trans_anim_source_order(body, name))
    return *parsed;
  MenuSliderAnim out;
  out.name = name;
  return out;
}

MenuAnimFilter decode_menu_anim_filter_body(
    const std::vector<std::uint8_t>& body, const std::string& name) {
  if (auto parsed = parse_anim_filter_source_order(body, name)) return *parsed;
  MenuAnimFilter out;
  out.name = name;
  return out;
}

MenuUiTrigger decode_menu_ui_trigger_body(
    const std::vector<std::uint8_t>& body, const std::string& name) {
  if (auto parsed = parse_ui_trigger_source_order(body, name)) return *parsed;
  MenuUiTrigger out;
  out.name = name;
  return out;
}

MenuMaterialAnim decode_menu_material_anim_body(
    const std::vector<std::uint8_t>& body, const std::string& name) {
  if (auto parsed = parse_material_anim_source_order(body, name))
    return *parsed;
  MenuMaterialAnim out;
  out.name = name;
  return out;
}

std::array<float, 3> transform_menu_text_point(
    const std::array<float, 12>& xfm, float local_x, float local_z) {
  return {{xfm[9] + local_x * xfm[0] + local_z * xfm[6],
           xfm[10] + local_x * xfm[1] + local_z * xfm[7],
           xfm[11] + local_x * xfm[2] + local_z * xfm[8]}};
}

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
      if (e.type != "BandButton" && e.type != "Text" &&
          e.type != "BandLabel" && e.type != "BandTextEntry")
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
          if (!lbl.text_tail.valid && strs.size() == 1) {
            continue;
          }
        }
        if (e.type == "BandTextEntry") {
          // config.dtb::textentry/styles/high_score points at
          // textentry.milo::label_hand_pen.txt, whose source RndText font is
          // rockletters.font. The editable text itself is runtime state.
          lbl.font = strs.front().text == "high_score" ? "rockletters" : "impact";
          lbl.text.clear();
          if (strs.front().text == "high_score") {
            const MenuTextStyle style = extract_menu_text_style(
                hdr_path, ark_path, "ui/gen/textentry.milo_ps2",
                "label_hand_pen.txt");
            if (style.valid) {
              lbl.text_tail.valid = true;
              lbl.text_tail.width = style.wrap_width;
              lbl.text_tail.height = style.text_size;
              lbl.text_tail.leading = style.leading;
              lbl.text_tail.alignment = style.alignment;
              lbl.text_tail.text_size = style.text_size;
              lbl.text_tail.width_bound = style.wrap_width;
            }
          }
          parse_bandtextentry_tail(body, lbl.text_entry_tail);
        }
        if (e.type == "Text") {
          if (auto style = parse_rnd_text_style_source_order(body, e.name)) {
            lbl.font = style->font;
            lbl.parent = style->parent;
            lbl.text = style->text;
            lbl.text_tail.valid = true;
            lbl.text_tail.width = style->wrap_width;
            lbl.text_tail.height = style->text_size;
            lbl.text_tail.leading = style->leading;
            lbl.text_tail.alignment = style->alignment;
            lbl.text_tail.text_size = style->text_size;
            lbl.text_tail.width_bound = style->wrap_width;
            lbl.text_tail.color = style->color;
            lbl.local = style->local;
            lbl.world = style->world;
            lbl.has_local = style->has_local;
            lbl.has_world = style->has_world;
          } else {
            // A Text object without a decoded/source mFont is not an Impact
            // label. Leave it fontless so the renderer does not fabricate one.
            lbl.font.clear();
          }
        }
        // BandButton inherits UIComponent before its label fields. Some stock
        // buttons, such as bonus_material/bm_hidden.btn, carry only nav/resource
        // object refs and no visible text token. Do not render those refs as
        // user-facing labels.
        if (e.type == "BandButton" && is_ui_object_ref(lbl.text)) {
          lbl.text.clear();
          lbl.button_tail.valid = false;
        }
      }
      if (!lbl.has_local) {
        lbl.has_local = find_local_world_matrices(body, lbl.local, lbl.world);
        lbl.has_world = lbl.has_local;
      }
      out.push_back(std::move(lbl));
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] %s: %s\n", milo_path.c_str(), ex.what());
  }
  return out;
}

std::vector<MenuCheckbox> extract_menu_checkboxes(const std::string& hdr_path,
                                                  const std::string& ark_path,
                                                  const std::string& milo_path) {
  std::vector<MenuCheckbox> out;
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
      if (e.type != "CheckBox" && e.type != "CheckboxDisplay") continue;
      if (e.offset + e.size > payload.size()) continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      if (auto parsed = parse_checkbox_source_order(body, e.type, e.name))
        out.push_back(std::move(*parsed));
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] CheckBox %s: %s\n", milo_path.c_str(),
                 ex.what());
  }
  return out;
}

std::vector<MenuSlider> extract_menu_sliders(const std::string& hdr_path,
                                             const std::string& ark_path,
                                             const std::string& milo_path) {
  std::vector<MenuSlider> out;
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
      if (e.type != "BandSlider" && e.type != "UISlider") continue;
      if (e.offset + e.size > payload.size()) continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      if (auto parsed = parse_slider_source_order(body, e.type, e.name))
        out.push_back(std::move(*parsed));
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] Slider %s: %s\n", milo_path.c_str(),
                 ex.what());
  }
  return out;
}

MenuSliderAnim extract_menu_slider_anim(const std::string& hdr_path,
                                        const std::string& ark_path,
                                        const std::string& milo_path,
                                        const std::string& anim_name) {
  MenuSliderAnim out;
  out.name = anim_name;
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
      if (e.type != "TransAnim" || e.name != anim_name) continue;
      if (e.offset + e.size > payload.size()) continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      if (auto parsed = parse_slider_trans_anim_source_order(body, e.name))
        return *parsed;
      return out;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] SliderAnim %s/%s: %s\n", milo_path.c_str(),
                 anim_name.c_str(), ex.what());
  }
  return out;
}

MenuAnimFilter extract_menu_anim_filter(const std::string& hdr_path,
                                        const std::string& ark_path,
                                        const std::string& milo_path,
                                        const std::string& filter_name) {
  MenuAnimFilter out;
  out.name = filter_name;
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
      if (e.type != "AnimFilter" || e.name != filter_name) continue;
      if (e.offset + e.size > payload.size()) continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      if (auto parsed = parse_anim_filter_source_order(body, e.name))
        return *parsed;
      return out;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] AnimFilter %s/%s: %s\n", milo_path.c_str(),
                 filter_name.c_str(), ex.what());
  }
  return out;
}

std::vector<MenuSliderAnim> extract_menu_transform_anims(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path) {
  std::vector<MenuSliderAnim> out;
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
      if (e.type != "TransAnim" || e.offset + e.size > payload.size())
        continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      if (auto parsed = parse_slider_trans_anim_source_order(body, e.name))
        out.push_back(std::move(*parsed));
    }
    for (MenuSliderAnim& anim : out) {
      if (anim.keys_owner.empty() || anim.keys_owner == anim.name) continue;
      const auto owner = std::find_if(
          out.begin(), out.end(), [&](const MenuSliderAnim& candidate) {
            return candidate.name == anim.keys_owner;
          });
      if (owner == out.end()) continue;
      anim.rotation_keys = owner->rotation_keys;
      anim.translation_keys = owner->translation_keys;
      anim.scale_keys = owner->scale_keys;
      anim.first = owner->first;
      anim.last = owner->last;
      anim.first_frame = owner->first_frame;
      anim.last_frame = owner->last_frame;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] TransAnim corpus %s: %s\n",
                 milo_path.c_str(), ex.what());
  }
  return out;
}

std::vector<MenuUiTrigger> extract_menu_ui_triggers(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path) {
  std::vector<MenuUiTrigger> out;
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
      if (e.type != "UITrigger" || e.offset + e.size > payload.size())
        continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      if (auto parsed = parse_ui_trigger_source_order(body, e.name))
        out.push_back(std::move(*parsed));
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] UITrigger %s: %s\n", milo_path.c_str(),
                 ex.what());
  }
  return out;
}

MenuMaterialAnim extract_menu_material_anim(const std::string& hdr_path,
                                            const std::string& ark_path,
                                            const std::string& milo_path,
                                            const std::string& anim_name) {
  MenuMaterialAnim out;
  out.name = anim_name;
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
      if (e.type != "MatAnim" || e.name != anim_name) continue;
      if (e.offset + e.size > payload.size()) continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      if (auto parsed = parse_material_anim_source_order(body, e.name))
        return *parsed;
      return out;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] MatAnim %s/%s: %s\n", milo_path.c_str(),
                 anim_name.c_str(), ex.what());
  }
  return out;
}

std::vector<MenuMaterialAnim> extract_menu_material_anims(
    const std::string& hdr_path, const std::string& ark_path,
    const std::string& milo_path) {
  std::vector<MenuMaterialAnim> out;
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
      if (e.type != "MatAnim" || e.offset + e.size > payload.size()) continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      if (auto parsed = parse_material_anim_source_order(body, e.name))
        out.push_back(std::move(*parsed));
    }
    for (MenuMaterialAnim& anim : out) {
      if (anim.keys_owner.empty() || anim.keys_owner == anim.name) continue;
      const auto owner = std::find_if(
          out.begin(), out.end(), [&](const MenuMaterialAnim& candidate) {
            return candidate.name == anim.keys_owner;
          });
      if (owner == out.end()) continue;
      anim.color_keys = owner->color_keys;
      anim.alpha_keys = owner->alpha_keys;
      anim.translation_keys = owner->translation_keys;
      anim.scale_keys = owner->scale_keys;
      anim.rotation_keys = owner->rotation_keys;
      anim.texture_keys = owner->texture_keys;
      anim.first_frame = owner->first_frame;
      anim.last_frame = owner->last_frame;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] MatAnim corpus %s: %s\n",
                 milo_path.c_str(), ex.what());
  }
  return out;
}

MenuProxyTransform extract_menu_proxy_transform(const std::string& hdr_path,
                                                const std::string& ark_path,
                                                const std::string& milo_path,
                                                const std::string& proxy_name) {
  MenuProxyTransform out;
  out.name = proxy_name;
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
      if (e.type != "UIProxy" || e.name != proxy_name) continue;
      if (e.offset + e.size > payload.size()) continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      if (auto parsed = parse_proxy_transform_source_order(body, e.name))
        return *parsed;
      return out;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] UIProxy %s/%s: %s\n", milo_path.c_str(),
                 proxy_name.c_str(), ex.what());
  }
  return out;
}

UiListLayout extract_ui_list_layout(const std::string& hdr_path,
                                    const std::string& ark_path,
                                    const std::string& milo_path,
                                    const std::string& list_name) {
  UiListLayout out;
  out.name = list_name;
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
      if (e.type != "UIList" || e.name != list_name) continue;
      if (e.offset + e.size > payload.size()) continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      auto parsed = parse_ui_list_layout_source_order(body);
      if (parsed) {
        parsed->name = e.name;
        return *parsed;
      }
      return out;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] UIList %s/%s: %s\n", milo_path.c_str(),
                 list_name.c_str(), ex.what());
  }
  return out;
}

MenuTextStyle extract_menu_text_style(const std::string& hdr_path,
                                      const std::string& ark_path,
                                      const std::string& milo_path,
                                      const std::string& text_name) {
  MenuTextStyle out;
  out.name = text_name;
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
      if (e.type != "Text" || e.name != text_name) continue;
      if (e.offset + e.size > payload.size()) continue;
      std::vector<uint8_t> body(payload.begin() + e.offset,
                                payload.begin() + e.offset + e.size);
      if (auto parsed = parse_rnd_text_style_source_order(body, e.name))
        return *parsed;
      return out;
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[labels] Text %s/%s: %s\n", milo_path.c_str(),
                 text_name.c_str(), ex.what());
  }
  return out;
}

}  // namespace ghogx::ui
