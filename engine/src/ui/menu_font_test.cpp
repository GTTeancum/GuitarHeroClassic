// engine/src/ui/menu_font_test.cpp
//
// Loads the real "impact" font from the stock PS2 ARK and asserts the grounded
// facts from the byte-exact RE (see FIDELITY 2c): 104-char set in the exact
// order, 480 kerning pairs, a 512x256 atlas, and that the glyph segmentation
// maps the ASCII range (rows 0-2) 1:1 onto the charset. Skips if the ARK is
// absent (same convention as ghogx_ui_test).

#include "ui/menu_font.h"

#include "ark_v3.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

static int g_failures = 0;
#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      std::fprintf(stderr, "FAIL %s:%d  CHECK(%s)\n", __FILE__, __LINE__, \
                   #cond);                                                \
      ++g_failures;                                                       \
    }                                                                     \
  } while (0)

static std::string first_existing(const std::string& dir, std::vector<std::string> names) {
  for (auto& n : names) {
    std::string p = dir + "/" + n;
    if (fs::exists(p)) return p;
  }
  return {};
}

int main(int argc, char** argv) {
  std::string ark_dir =
      argc > 1 ? argv[1]
               : "C:/Programming/GitHub/Guitar Hero II/Guitar Hero II PS2 (USA)/GEN";
  std::string hdr = first_existing(ark_dir, {"MAIN.HDR", "main.hdr"});
  std::string ark0 = first_existing(ark_dir, {"MAIN_0.ARK", "main_0.ark"});
  if (hdr.empty() || ark0.empty()) {
    std::printf("ghogx_menu_font_test: SKIP (no stock ARK at %s)\n", ark_dir.c_str());
    return 0;
  }

  ghogx::ui::MenuFont font;
  bool ok = font.load(hdr, ark0, "ui/gen/impact.milo_ps2");
  CHECK(ok);
  CHECK(font.valid());

  // Header metrics (byte-exact from impact.font).
  CHECK(font.cap_height() == 34.0f);
  CHECK(font.line_height() == 50.0f);

  // Atlas.
  CHECK(font.atlas().width == 512);
  CHECK(font.atlas().height == 256);

  // The ASCII glyphs the menus actually use (rows 0-2) must all be present.
  const char* needed = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (const char* p = needed; *p; ++p)
    CHECK(font.glyph(static_cast<uint8_t>(*p)) != nullptr);

  // 'A' is the first glyph of row 0 — sane ink box near the atlas's top-left.
  if (const ghogx::ui::Glyph* a = font.glyph('A')) {
    CHECK(a->pw > 8 && a->pw < 40);
    CHECK(a->ph > 20 && a->ph < 50);
    CHECK(a->v0 < 0.25f);          // top row
    CHECK(a->advance > 0.0f);
  }
  // 'W' is wider than 'I' (variable width sanity).
  const ghogx::ui::Glyph* w = font.glyph('W');
  const ghogx::ui::Glyph* i = font.glyph('I');
  if (w && i) CHECK(w->pw > i->pw);

  // Kerning table is populated (480 pairs) and a known pair resolves.
  // '(' + 'A' is the first listed pair (+1px push); 'A' + 'V' tightens.
  CHECK(font.kerning('(', 'A') != 0.0f);
  CHECK(font.kerning('A', 'V') < 0.0f);

  // A real menu label measures to a positive, plausible width.
  float wlen = font.measure("CAREER");
  CHECK(wlen > 60.0f && wlen < 260.0f);
  auto quads = font.layout("CAREER");
  CHECK(quads.size() == 6);       // C A R E E R, all inked

  // Setlist song rows use dyingmarker.milo_ps2; these source metrics feed the
  // UIList text-height scale in menu_app.cpp.
  ghogx::ui::MenuFont song_font;
  bool song_ok = song_font.load(hdr, ark0, "ui/gen/dyingmarker.milo_ps2");
  CHECK(song_ok);
  CHECK(song_font.valid());
  CHECK(song_font.cap_height() == 25.0f);
  CHECK(song_font.line_height() == 28.0f);
  CHECK(song_font.has_source_char_info());
  if (song_font.valid()) {
    auto letter = song_font.layout("A");
    auto comma = song_font.layout(",");
    auto dash = song_font.layout("-");
    CHECK(!letter.empty());
    CHECK(!comma.empty());
    CHECK(!dash.empty());
    if (!letter.empty() && !comma.empty() && !dash.empty()) {
      CHECK(comma[0].y0 > letter[0].y0);
      CHECK(dash[0].y0 > letter[0].y0);
      CHECK(comma[0].y1 <= song_font.line_height());
      CHECK(dash[0].y1 <= song_font.line_height());
    }
  }

  ghogx::ui::MenuFont helvetica_black;
  bool helvetica_ok =
      helvetica_black.load(hdr, ark0, "ui/gen/helveticablack.milo_ps2");
  CHECK(helvetica_ok);
  CHECK(helvetica_black.valid());
  CHECK(helvetica_black.has_material_color());
  CHECK(helvetica_black.material_color()[0] == 1.0f);
  CHECK(helvetica_black.material_color()[1] == 1.0f);
  CHECK(helvetica_black.material_color()[2] == 1.0f);
  CHECK(helvetica_black.material_color()[3] == 1.0f);
  if (helvetica_black.valid()) {
    // Exact RndFont cellSize from helveticablack.font. RndText::SetupCharVerts
    // scales both axes by mSize / cellSize.x, makes the quad
    // mSize * CellDiff() high, and kMiddle* offsets it by half that full cell.
    // The capital ink is therefore centered against Y=18, not cellSize.x/2
    // (14.5), which would visibly leave outfit labels low in their arrows.
    CHECK(helvetica_black.cap_height() == 29.0f);
    CHECK(helvetica_black.line_height() == 36.0f);
    CHECK(helvetica_black.rnd_text_local_z(18.0f, 30.0f) == 0.0f);
    CHECK(helvetica_black.rnd_text_local_z(0.0f, 29.0f) == 18.0f);
    CHECK(helvetica_black.rnd_text_local_z(36.0f, 29.0f) == -18.0f);
    CHECK(helvetica_black.rnd_text_cell_height(29.0f) == 36.0f);
    const ghogx::ui::Glyph* r = helvetica_black.glyph('R');
    const ghogx::ui::Glyph* o = helvetica_black.glyph('O');
    CHECK(r != nullptr);
    CHECK(o != nullptr);
    if (r && o) {
      CHECK(r->yoff == 8.0f);
      CHECK(r->ph == 19.0f);
      CHECK(o->yoff == 7.0f);
      CHECK(o->ph == 21.0f);
    }
    const ghogx::ui::Glyph* apostrophe = helvetica_black.glyph('\'');
    CHECK(apostrophe != nullptr);
    if (apostrophe) {
      CHECK(apostrophe->pw > 0.0f);
      CHECK(apostrophe->ph > 0.0f);
    }
    const auto ascii_apostrophe = helvetica_black.layout("DOESN'T");
    const std::string cp1252_apostrophe =
        std::string("DOESN") + static_cast<char>(0x92) + "T";
    const std::string utf8_apostrophe =
        std::string("DOESN") + "\xE2\x80\x99" + "T";
    CHECK(!ascii_apostrophe.empty());
    CHECK(ascii_apostrophe.size() == 7);
    CHECK(helvetica_black.layout(cp1252_apostrophe).size() ==
          ascii_apostrophe.size());
    CHECK(helvetica_black.layout(utf8_apostrophe).size() ==
          ascii_apostrophe.size());
  }

  for (const auto& career_font :
       std::vector<std::pair<const char*, const char*>>{
           {"tapeworm", "YOUR BAND"},
           {"clarendon", "FEATURING"}}) {
    ghogx::ui::MenuFont source;
    CHECK(source.load(hdr, ark0,
                      std::string("ui/gen/") + career_font.first +
                          ".milo_ps2"));
    if (!source.valid()) continue;
    float min_y = 1.0e9f, max_y = -1.0e9f;
    for (const auto& quad : source.layout(career_font.second)) {
      min_y = std::min(min_y, quad.y0);
      max_y = std::max(max_y, quad.y1);
    }
    if (std::string(career_font.first) == "tapeworm") {
      CHECK(source.cap_height() == 39.0f);
      CHECK(source.line_height() == 54.0f);
      CHECK(min_y == 0.0f);
      CHECK(max_y == 40.0f);
    } else {
      CHECK(source.cap_height() == 30.0f);
      CHECK(source.line_height() == 36.0f);
      CHECK(min_y == 7.0f);
      CHECK(max_y == 27.0f);
    }
    std::printf("career font %s cell=(%.3f %.3f) inkY=(%.3f %.3f) "
                "localZ40=(%.3f %.3f)\n",
                career_font.first, source.cap_height(), source.line_height(),
                min_y, max_y, source.rnd_text_local_z(min_y, 40.0f),
                source.rnd_text_local_z(max_y, 40.0f));
  }

  if (g_failures == 0) {
    std::printf("ghogx_menu_font_test: OK (impact: charset+kern byte-exact, "
                "glyphs segmented, '%s'... measured %.1fpx)\n", "CAREER", wlen);
  } else {
    std::printf("ghogx_menu_font_test: %d FAILURE(S)\n", g_failures);
  }
  return g_failures == 0 ? 0 : 1;
}
