// engine/src/ui/menu_font.cpp -- see menu_font.h.

#include "ui/menu_font.h"

#include "ark_v3.h"
#include "milo.h"
#include "milo_scene/milo_scene.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <optional>

namespace ghogx::ui {

namespace {

// Alpha coverage threshold for "glyph ink" when segmenting the atlas (the atlas
// stores glyphs as white with coverage in the alpha channel).
constexpr int kInkAlpha = 40;
// A content band shorter than this (px) is a diacritic strip, not a glyph row;
// it gets merged into the glyph row directly below it.
constexpr int kMinRowHeight = 18;
// Fallback inter-glyph tracking when an old font lacks serialized CharInfo.
constexpr float kDefaultTrackingPx = 2.0f;

uint8_t next_font_char(const std::string& text, size_t& index) {
  uint8_t c = static_cast<uint8_t>(text[index++]);
  switch (c) {
    case 0x91:
    case 0x92:
    case 0xB4:
    case '`':
      return '\'';
    case 0x93:
    case 0x94:
      return '"';
    case 0x96:
    case 0x97:
      return '-';
    default:
      break;
  }

  if (c == 0xC2 && index < text.size()) {
    const uint8_t d = static_cast<uint8_t>(text[index]);
    if (d == 0xB4) {
      ++index;
      return '\'';
    }
  }

  if (c == 0xE2 && index + 1 < text.size()) {
    const uint8_t d = static_cast<uint8_t>(text[index]);
    const uint8_t e = static_cast<uint8_t>(text[index + 1]);
    if (d == 0x80) {
      if (e == 0x98 || e == 0x99) {
        index += 2;
        return '\'';
      }
      if (e == 0x9C || e == 0x9D) {
        index += 2;
        return '"';
      }
      if (e == 0x93 || e == 0x94) {
        index += 2;
        return '-';
      }
    }
  }

  if (c >= 'a' && c <= 'z')
    c = static_cast<uint8_t>(c - 0x20);
  else if ((c >= 0xE0 && c <= 0xF6) || (c >= 0xF8 && c <= 0xFE))
    c = static_cast<uint8_t>(c - 0x20);
  return c;
}

// Little-endian readers over a byte body, with bounds checks.
struct Reader {
  const uint8_t* p;
  size_t n;
  size_t pos = 0;
  bool ok = true;

  bool need(size_t k) {
    if (pos + k > n) { ok = false; return false; }
    return true;
  }
  int32_t i32() {
    if (!need(4)) return 0;
    int32_t v;
    std::memcpy(&v, p + pos, 4);
    pos += 4;
    return v;
  }
  float f32() {
    if (!need(4)) return 0;
    float v;
    std::memcpy(&v, p + pos, 4);
    pos += 4;
    return v;
  }
  uint8_t u8() {
    if (!need(1)) return 0;
    return p[pos++];
  }
  void skip(size_t k) { pos += k; if (pos > n) ok = false; }
  std::string str() {
    int32_t len = i32();
    if (len < 0 || !need(static_cast<size_t>(len))) { ok = false; return {}; }
    std::string s(reinterpret_cast<const char*>(p + pos), static_cast<size_t>(len));
    pos += static_cast<size_t>(len);
    return s;
  }
};

}  // namespace

bool MenuFont::load(const std::string& hdr_path, const std::string& ark_path,
                    const std::string& milo_path,
                    const std::string& font_entry_name) {
  std::string atlas_entry_name;
  material_color_ = {{1.0f, 1.0f, 1.0f, 1.0f}};
  has_material_color_ = false;
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = ark.find(milo_path);
    if (!entry) entry = ark.find("../../system/run/" + milo_path);
    if (!entry) {
      std::fprintf(stderr, "[font] milo not in ARK: %s\n", milo_path.c_str());
      return false;
    }
    auto bytes = ark.read_entry(*entry, {ark_path});
    auto h = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, h);
    auto dir = gh::milo::parse_directory(payload);

    // Find the Font entry body.
    const gh::milo::Entry* fe = nullptr;
    for (const auto& e : dir.entries)
      if (e.type == "Font" &&
          (font_entry_name.empty() || e.name == font_entry_name)) {
        fe = &e;
        break;
      }
    if (!fe || fe->offset + fe->size > payload.size()) {
      std::fprintf(stderr, "[font] no Font entry in %s\n", milo_path.c_str());
      return false;
    }
    std::vector<uint8_t> body(payload.begin() + fe->offset,
                              payload.begin() + fe->offset + fe->size);
    if (!parse_font(body)) return false;
    if (!material_name_.empty()) {
      for (const auto& e : dir.entries) {
        if (e.type != "Mat" || e.name != material_name_ ||
            e.offset + e.size > payload.size())
          continue;
        std::vector<uint8_t> mat_body(payload.begin() + e.offset,
                                      payload.begin() + e.offset + e.size);
        const auto mat =
            milo_scene::decode_mat(e.name, mat_body);
        if (mat.decoded) {
          atlas_entry_name = mat.diffuse_tex;
          for (std::size_t channel = 0; channel < material_color_.size();
               ++channel)
            material_color_[channel] = mat.color[channel];
          has_material_color_ = true;
        }
        break;
      }
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "[font] load failed: %s\n", e.what());
    return false;
  }

  // Decode the atlas (white glyphs in the alpha channel) and segment glyphs.
  atlas_ = atlas_entry_name.empty()
               ? asset::load_milo_texture(hdr_path, ark_path, milo_path)
               : asset::load_milo_texture_named(
                     hdr_path, ark_path, milo_path, atlas_entry_name);
  if (!atlas_.valid()) {
    std::fprintf(stderr, "[font] atlas decode failed for %s\n", milo_path.c_str());
    return false;
  }
  segment_glyphs();
  std::fprintf(stderr,
               "[font] %s: charset=%zu kern=%zu atlas=%dx%d glyphs=%d cap=%.0f line=%.0f charinfo=%d\n",
               milo_path.c_str(), charset_.size(), kern_.size(), atlas_.width,
               atlas_.height, glyph_count_, cap_height_, line_height_,
               has_source_char_info_ ? 1 : 0);
  return valid();
}

bool MenuFont::parse_font(const std::vector<uint8_t>& body) {
  Reader r{body.data(), body.size()};
  int32_t version = r.i32();
  has_source_char_info_ = false;
  tex_cell_u_ = 0.0f;
  tex_cell_v_ = 0.0f;
  char_info_.fill(SourceCharInfo{});
  // GH1 RndFont revision 7 predates the serialized Hmx::Object metadata
  // inherited by later GH2-era revisions.
  if (version > 7) r.skip(9);
  std::string material = r.str(); // "impact.mat"
  material_name_ = material;
  cap_height_ = r.f32();          // 34
  line_height_ = r.f32();         // 50
  r.f32();                        // deprecatedSize
  const float base_kerning = r.f32();
  base_kerning_px_ =
      std::isfinite(base_kerning) ? base_kerning * cap_height_
                                  : kDefaultTrackingPx;
  int32_t charcount = r.i32();    // 104
  if (!r.ok || charcount <= 0 || charcount > 4096) {
    std::fprintf(stderr, "[font] bad header (v=%d mat='%s' cc=%d)\n", version,
                 material.c_str(), charcount);
    return false;
  }
  if (!r.need(static_cast<size_t>(charcount))) return false;
  charset_.assign(reinterpret_cast<const char*>(body.data() + r.pos),
                  static_cast<size_t>(charcount));
  r.pos += static_cast<size_t>(charcount);

  // Kerning: u8 flag, i32 count, then count * [u8 L][u8 R][i16][f32 kern].
  const bool has_kerning = r.u8() != 0;
  int32_t kerncount = has_kerning ? r.i32() : 0;
  kern_.clear();
  for (int32_t i = 0; i < kerncount && r.ok; ++i) {
    if (!r.need(8)) break;
    uint8_t left = body[r.pos + 0];
    uint8_t right = body[r.pos + 1];
    float frac;
    std::memcpy(&frac, body.data() + r.pos + 4, 4);
    r.pos += 8;
    // The kern is an em-fraction (±1/34 ⇒ ±1px at cap height 34). Store native px.
    kern_[static_cast<uint16_t>((left << 8) | right)] = frac * cap_height_;
  }
  // MiloLib RndFont.cs source order after kerning:
  // textureOwner, monospace, packed, bitmap size, texCellSize, CharInfo.
  if (version > 8) r.str();       // textureOwner
  if (version > 10) r.u8();       // monospace
  if (version > 0x0e) r.u8();     // packed
  if (version > 0x0c) {
    r.i32();                      // bitmapWidth
    r.i32();                      // bitmapHeight
  }
  if (version > 0x0d) {
    tex_cell_u_ = r.f32();
    tex_cell_v_ = r.f32();
    if (version < 0x11) {
      int valid_count = 0;
      for (int i = 0; i < 256 && r.ok; ++i) {
        SourceCharInfo info;
        info.tex_u = r.f32();
        info.tex_v = r.f32();
        info.width = r.f32();
        info.advance = version > 0x0e ? r.f32() : info.width;
        const bool plausible =
            std::isfinite(info.tex_u) && std::isfinite(info.tex_v) &&
            std::isfinite(info.width) && std::isfinite(info.advance) &&
            info.tex_u >= 0.0f && info.tex_u <= 1.0f &&
            info.tex_v >= 0.0f && info.tex_v <= 1.0f &&
            info.width >= 0.0f && info.width < 8.0f &&
            info.advance >= 0.0f && info.advance < 8.0f;
        if (plausible && (info.width > 0.0f || info.advance > 0.0f)) {
          info.valid = true;
          ++valid_count;
        }
        char_info_[static_cast<uint8_t>(i)] = info;
      }
      has_source_char_info_ =
          r.ok && valid_count > 0 && tex_cell_u_ > 0.0f && tex_cell_v_ > 0.0f;
    } else {
      const int32_t count = r.i32();
      if (count >= 0 && count <= 4096) {
        int valid_count = 0;
        for (int32_t i = 0; i < count && r.ok; ++i) {
          if (!r.need(2)) break;
          uint16_t key;
          std::memcpy(&key, body.data() + r.pos, 2);
          r.pos += 2;
          SourceCharInfo info;
          info.tex_u = r.f32();
          info.tex_v = r.f32();
          info.width = r.f32();
          info.advance = r.f32();
          const bool plausible =
              std::isfinite(info.tex_u) && std::isfinite(info.tex_v) &&
              std::isfinite(info.width) && std::isfinite(info.advance) &&
              info.tex_u >= 0.0f && info.tex_u <= 1.0f &&
              info.tex_v >= 0.0f && info.tex_v <= 1.0f &&
              info.width >= 0.0f && info.width < 8.0f &&
              info.advance >= 0.0f && info.advance < 8.0f && key < 256;
          if (plausible && (info.width > 0.0f || info.advance > 0.0f)) {
            info.valid = true;
            char_info_[static_cast<uint8_t>(key)] = info;
            ++valid_count;
          }
        }
        has_source_char_info_ =
            r.ok && valid_count > 0 && tex_cell_u_ > 0.0f &&
            tex_cell_v_ > 0.0f;
      }
    }
  }
  return !charset_.empty();
}

void MenuFont::segment_glyphs() {
  const int W = atlas_.width, H = atlas_.height;
  const uint8_t* px = atlas_.rgba.data();
  auto alpha = [&](int x, int y) -> int { return px[(y * W + x) * 4 + 3]; };

  // 1. Content bands: maximal y-runs with any ink.
  std::vector<std::pair<int, int>> bands;  // [y0, y1)
  for (int y = 0; y < H;) {
    int cover = 0;
    for (int x = 0; x < W; ++x) if (alpha(x, y) > kInkAlpha) { cover = 1; break; }
    if (!cover) { ++y; continue; }
    int y0 = y;
    while (y < H) {
      int c = 0;
      for (int x = 0; x < W; ++x) if (alpha(x, y) > kInkAlpha) { c = 1; break; }
      if (!c) break;
      ++y;
    }
    bands.push_back({y0, y});
  }

  // 2. Build glyph rows = tall bands, each extended UP to swallow any short
  //    (diacritic) band sitting directly above it.
  struct Row { int y0, y1; };
  std::vector<Row> rows;
  for (size_t i = 0; i < bands.size(); ++i) {
    int h = bands[i].second - bands[i].first;
    if (h >= kMinRowHeight) {
      int top = bands[i].first;
      // Pull in immediately-preceding short bands (accents) as part of this row.
      for (size_t j = i; j-- > 0;) {
        int jh = bands[j].second - bands[j].first;
        if (jh >= kMinRowHeight) break;          // previous glyph row, stop
        if (top - bands[j].second > kMinRowHeight) break;  // too far above
        top = bands[j].first;
      }
      rows.push_back({top, bands[i].second});
    }
  }

  // 3. Within each row, segment glyph columns (x-runs with ink), in reading
  //    order; collect boxes (x0,x1,y0,y1).
  struct Box { int x0, x1, y0, y1, row_y0; };
  std::vector<Box> boxes;
  for (const Row& row : rows) {
    for (int x = 0; x < W;) {
      int cover = 0;
      for (int y = row.y0; y < row.y1; ++y) if (alpha(x, y) > kInkAlpha) { cover = 1; break; }
      if (!cover) { ++x; continue; }
      int x0 = x;
      while (x < W) {
        int c = 0;
        for (int y = row.y0; y < row.y1; ++y) if (alpha(x, y) > kInkAlpha) { c = 1; break; }
        if (!c) break;
        ++x;
      }
      // Tight vertical extent for this glyph (so v0/v1 hug the ink).
      int gy0 = row.y1, gy1 = row.y0;
      for (int yy = row.y0; yy < row.y1; ++yy)
        for (int xx = x0; xx < x; ++xx)
          if (alpha(xx, yy) > kInkAlpha) { gy0 = std::min(gy0, yy); gy1 = std::max(gy1, yy + 1); break; }
      boxes.push_back({x0, x, gy0, gy1, row.y0});
    }
  }

  // 4. Assign boxes to the charset in order. The charset's space (last) has no
  //    ink, so it consumes no box; every other char takes the next box. Rows
  //    0-2 (all menu letters/digits/punct) match the atlas 1:1, so the early
  //    assignments are exact even if the accent rows mis-segment.
  glyphs_.fill(Glyph{});
  glyph_count_ = 0;
  size_t bi = 0;
  for (char cc : charset_) {
    uint8_t ch = static_cast<uint8_t>(cc);
    if (ch == ' ') {
      Glyph& g = glyphs_[ch];
      g.present = true;
      g.advance = cap_height_ * 0.30f;   // synthetic space advance
      continue;
    }
    if (bi >= boxes.size()) break;
    const Box& b = boxes[bi++];
    Glyph& g = glyphs_[ch];
    g.px = static_cast<float>(b.x0);
    g.py = static_cast<float>(b.y0);
    g.pw = static_cast<float>(b.x1 - b.x0);
    g.ph = static_cast<float>(b.y1 - b.y0);
    g.yoff = static_cast<float>(b.y0 - b.row_y0);
    g.u0 = static_cast<float>(b.x0) / W;
    g.u1 = static_cast<float>(b.x1) / W;
    g.v0 = static_cast<float>(b.y0) / H;
    g.v1 = static_cast<float>(b.y1) / H;
    g.advance = g.pw + base_kerning_px_;
    g.present = true;
    ++glyph_count_;
  }
  apply_source_char_info();
}

void MenuFont::apply_source_char_info() {
  if (!has_source_char_info_) return;
  const int W = atlas_.width;
  const int H = atlas_.height;
  const uint8_t* px = atlas_.rgba.data();
  auto alpha = [&](int x, int y) -> int { return px[(y * W + x) * 4 + 3]; };
  for (char cc : charset_) {
    const uint8_t ch = static_cast<uint8_t>(cc);
    const SourceCharInfo& info = char_info_[ch];
    if (!info.valid) continue;

    Glyph& g = glyphs_[ch];
    g.present = true;
    const float source_width = info.width * cap_height_;
    const float source_advance =
        (info.advance > 0.0f ? info.advance : info.width) * cap_height_;

    if (source_width > 0.0f) {
      g.pw = source_width;
      if (tex_cell_u_ > 0.0f) {
        g.u0 = info.tex_u;
        g.u1 = info.tex_u + info.width * tex_cell_u_;
      }
    }
    if (source_width > 0.0f && tex_cell_u_ > 0.0f && tex_cell_v_ > 0.0f &&
        atlas_.valid()) {
      const int cell_x0 =
          std::clamp(static_cast<int>(std::floor(info.tex_u * W + 0.5f)), 0, W);
      const int cell_y0 =
          std::clamp(static_cast<int>(std::floor(info.tex_v * H + 0.5f)), 0, H);
      const int cell_x1 = std::clamp(
          static_cast<int>(
              std::ceil((info.tex_u + info.width * tex_cell_u_) * W)),
          cell_x0, W);
      const int cell_y1 = std::clamp(
          static_cast<int>(std::ceil((info.tex_v + tex_cell_v_) * H)),
          cell_y0, H);
      int gx0 = cell_x1, gx1 = cell_x0, gy0 = cell_y1, gy1 = cell_y0;
      for (int y = cell_y0; y < cell_y1; ++y) {
        for (int x = cell_x0; x < cell_x1; ++x) {
          if (alpha(x, y) <= kInkAlpha) continue;
          gx0 = std::min(gx0, x);
          gx1 = std::max(gx1, x + 1);
          gy0 = std::min(gy0, y);
          gy1 = std::max(gy1, y + 1);
        }
      }
      if (gx1 > gx0 && gy1 > gy0) {
        g.px = static_cast<float>(gx0);
        g.py = static_cast<float>(gy0);
        g.pw = static_cast<float>(gx1 - gx0);
        g.ph = static_cast<float>(gy1 - gy0);
        g.yoff = static_cast<float>(gy0 - cell_y0);
        g.u0 = static_cast<float>(gx0) / W;
        g.u1 = static_cast<float>(gx1) / W;
        g.v0 = static_cast<float>(gy0) / H;
        g.v1 = static_cast<float>(gy1) / H;
      }
    }
    if (source_advance > 0.0f) g.advance = source_advance;
  }
}

float MenuFont::kerning(uint8_t left, uint8_t right) const {
  auto it = kern_.find(static_cast<uint16_t>((left << 8) | right));
  return it == kern_.end() ? 0.0f : it->second;
}

std::vector<MenuFont::Quad> MenuFont::layout(const std::string& text,
                                             float* out_width) const {
  std::vector<Quad> quads;
  float pen = 0.0f;
  uint8_t prev = 0;
  // Baseline: all-caps menu labels — align glyph tops within the cap band.
  for (size_t i = 0; i < text.size();) {
    uint8_t ch = next_font_char(text, i);
    const Glyph* g = glyph(ch);
    if (!g) { g = glyph(' '); if (!g) continue; }
    if (prev) pen += kerning(prev, ch);
    if (g->present && g->pw > 0) {
      Quad q;
      q.x0 = pen;
      q.x1 = pen + g->pw;
      q.y0 = static_cast<float>(g->yoff);
      q.y1 = static_cast<float>(g->yoff + g->ph);
      q.u0 = g->u0; q.v0 = g->v0; q.u1 = g->u1; q.v1 = g->v1;
      quads.push_back(q);
    }
    pen += g->advance;
    prev = ch;
  }
  if (out_width) *out_width = pen;
  return quads;
}

float MenuFont::measure(const std::string& text) const {
  float w = 0.0f;
  uint8_t prev = 0;
  for (size_t i = 0; i < text.size();) {
    uint8_t ch = next_font_char(text, i);
    const Glyph* g = glyph(ch);
    if (!g) { g = glyph(' '); if (!g) continue; }
    if (prev) w += kerning(prev, ch);
    w += g->advance;
    prev = ch;
  }
  return w;
}

}  // namespace ghogx::ui
