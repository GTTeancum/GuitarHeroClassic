// engine/src/ui/menu_font.h
//
// MenuFont — the GH2 menu bitmap font ("impact"), loaded straight from the PS2
// ARK and used to lay out the menu's text (button labels, song/venue/difficulty
// text, the help bar). 1:1 with the stock asset, grounded in the real bytes:
//
//   ui/gen/impact.milo_ps2  (RndDir "common") contains:
//     impact.tex   512x256 RGBA atlas — white glyphs in the ALPHA channel,
//                  variable-width PACKED across 5 rows.
//     impact.font  8125-byte RndFont:  (decoded byte-exact from the ARK)
//        i32 version=15 | 9 obj-meta | string mMaterial="impact.mat"
//        f32 cap_height=34, f32 line_height=50, f32 0, f32 0
//        i32 charcount=104 | charset[104]  (A-Z 0-9 punct Latin-1-accents, space
//                                           last; UPPERCASE only — exact order)
//        u8 flag=1 | i32 kerncount=480 | kern[480] each = [u8 L][u8 R][i16][f32]
//        ... self-name + sparse per-glyph region (NOT a plain rect table)
//
// impact.font does NOT store a decodable per-glyph atlas-rect table (its width/x
// sequences appear in no int/float encoding; the rect data is sparse+segmented
// and only the recomp RndFont::Load would yield it byte-for-byte). It doesn't
// need to: the glyph rectangles ARE the literal pixel locations in impact.tex.
// So MenuFont derives each glyph's atlas rect from the decoded atlas alpha
// (rows by horizontal projection — diacritic bands merged into the glyph row
// below — glyphs by vertical projection) and assigns them to the charset in
// order, which the atlas matches 1:1 (verified: row0 A-W=23, row1 X-Z0-9+punct
// =28, row2 punct+accents=25). The charset order and the 480-pair kerning table
// are taken byte-exact from impact.font.
//
// See engine/src/ui/FIDELITY.md (2c) for the full source audit.

#pragma once

#include "asset/milo_image.h"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ghogx::ui {

// One glyph's placement in the atlas + its horizontal advance.
struct Glyph {
  // Normalized atlas coords (0..1), ready for UVs.
  float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
  // Ink box in atlas pixels (the drawn quad's native size).
  int px = 0, py = 0, pw = 0, ph = 0;
  // Pen advance in native (atlas) pixels after drawing this glyph.
  float advance = 0;
  bool present = false;
};

class MenuFont {
 public:
  MenuFont() = default;

  // Load impact (or any RndFont .milo) from the PS2 ARK. `milo_path` is the ARK
  // entry, e.g. "ui/gen/impact.milo_ps2". Returns false (logged) on any failure.
  bool load(const std::string& hdr_path, const std::string& ark_path,
            const std::string& milo_path);

  bool valid() const { return atlas_.valid() && glyph_count_ > 0; }

  // The decoded atlas (RGBA8; glyph coverage in the alpha channel).
  const asset::Image& atlas() const { return atlas_; }

  float cap_height() const { return cap_height_; }    // 34 (header)
  float line_height() const { return line_height_; }  // 50 (header)

  // Glyph for a byte (Latin-1 code). nullptr if the font has no such glyph.
  // impact is an UPPERCASE-only font, so lowercase folds to its uppercase glyph
  // (this is how GH2 renders e.g. locale "training" as "TRAINING").
  const Glyph* glyph(uint8_t ch) const {
    if (ch >= 'a' && ch <= 'z') ch = static_cast<uint8_t>(ch - 32);
    return glyphs_[ch].present ? &glyphs_[ch] : nullptr;
  }

  // Kerning adjustment (native px, added to the pen) between two adjacent chars,
  // straight from impact.font's 480-pair table. 0 if the pair isn't listed.
  float kerning(uint8_t left, uint8_t right) const;

  // One laid-out glyph quad: native-pixel dest box (pen origin at x=0, baseline
  // semantics folded into py — top-left is (x+px - origin)), and atlas UVs.
  struct Quad {
    float x0, y0, x1, y1;  // dest, native px (string starts at x=0, top at y=0)
    float u0, v0, u1, v1;  // atlas UVs
  };

  // Lay out a string left-to-right with kerning. `out_width` (optional) gets the
  // total advanced pen width. Quads are positioned so the string's top-left sits
  // near (0,0); the caller scales/translates into screen or world space.
  std::vector<Quad> layout(const std::string& text, float* out_width = nullptr) const;

  // Measure a string's advanced width in native px (no allocation).
  float measure(const std::string& text) const;

 private:
  bool parse_font(const std::vector<uint8_t>& body);
  void segment_glyphs();  // derive rects from atlas alpha, assign to charset

  asset::Image atlas_;
  std::string charset_;                 // 104 chars, exact order from the file
  std::array<Glyph, 256> glyphs_{};     // indexed by Latin-1 code
  std::unordered_map<uint16_t, float> kern_;  // (L<<8|R) -> native px
  float cap_height_ = 34.0f;
  float line_height_ = 50.0f;
  int glyph_count_ = 0;
};

}  // namespace ghogx::ui
