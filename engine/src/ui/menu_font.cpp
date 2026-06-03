// engine/src/ui/menu_font.cpp -- see menu_font.h.

#include "ui/menu_font.h"

#include "ark_v3.h"
#include "milo.h"

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
// Inter-glyph tracking added to each glyph's ink width to get its pen advance.
// This is the ONE metric impact.font doesn't expose decodably (see FIDELITY 2c);
// seeded small and meant to be pinned against a GH2 screenshot.
constexpr float kDefaultTrackingPx = 2.0f;

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
                    const std::string& milo_path) {
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
      if (e.type == "Font") { fe = &e; break; }
    if (!fe || fe->offset + fe->size > payload.size()) {
      std::fprintf(stderr, "[font] no Font entry in %s\n", milo_path.c_str());
      return false;
    }
    std::vector<uint8_t> body(payload.begin() + fe->offset,
                              payload.begin() + fe->offset + fe->size);
    if (!parse_font(body)) return false;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "[font] load failed: %s\n", e.what());
    return false;
  }

  // Decode the atlas (white glyphs in the alpha channel) and segment glyphs.
  atlas_ = asset::load_milo_texture(hdr_path, ark_path, milo_path);
  if (!atlas_.valid()) {
    std::fprintf(stderr, "[font] atlas decode failed for %s\n", milo_path.c_str());
    return false;
  }
  segment_glyphs();
  std::fprintf(stderr,
               "[font] %s: charset=%zu kern=%zu atlas=%dx%d glyphs=%d cap=%.0f line=%.0f\n",
               milo_path.c_str(), charset_.size(), kern_.size(), atlas_.width,
               atlas_.height, glyph_count_, cap_height_, line_height_);
  return valid();
}

bool MenuFont::parse_font(const std::vector<uint8_t>& body) {
  Reader r{body.data(), body.size()};
  int32_t version = r.i32();
  r.skip(9);                      // Hmx::Object metadata (all zero in practice)
  std::string material = r.str(); // "impact.mat"
  cap_height_ = r.f32();          // 34
  line_height_ = r.f32();         // 50
  r.f32();                        // 0
  r.f32();                        // 0
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
  r.u8();                         // flag (1)
  int32_t kerncount = r.i32();
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
  struct Box { int x0, x1, y0, y1; };
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
      boxes.push_back({x0, x, gy0, gy1});
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
    g.px = b.x0; g.py = b.y0; g.pw = b.x1 - b.x0; g.ph = b.y1 - b.y0;
    g.u0 = static_cast<float>(b.x0) / W;
    g.u1 = static_cast<float>(b.x1) / W;
    g.v0 = static_cast<float>(b.y0) / H;
    g.v1 = static_cast<float>(b.y1) / H;
    g.advance = g.pw + kDefaultTrackingPx;
    g.present = true;
    ++glyph_count_;
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
  for (size_t i = 0; i < text.size(); ++i) {
    uint8_t ch = static_cast<uint8_t>(text[i]);
    const Glyph* g = glyph(ch);
    if (!g) { g = glyph(' '); if (!g) continue; }
    if (prev) pen += kerning(prev, ch);
    if (g->present && g->pw > 0) {
      Quad q;
      q.x0 = pen;
      q.x1 = pen + g->pw;
      q.y0 = 0.0f;
      q.y1 = static_cast<float>(g->ph);
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
  for (char cc : text) {
    uint8_t ch = static_cast<uint8_t>(cc);
    const Glyph* g = glyph(ch);
    if (!g) { g = glyph(' '); if (!g) continue; }
    if (prev) w += kerning(prev, ch);
    w += g->advance;
    prev = ch;
  }
  return w;
}

}  // namespace ghogx::ui
