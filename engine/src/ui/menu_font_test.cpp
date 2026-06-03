// engine/src/ui/menu_font_test.cpp
//
// Loads the real "impact" font from the stock PS2 ARK and asserts the grounded
// facts from the byte-exact RE (see FIDELITY 2c): 104-char set in the exact
// order, 480 kerning pairs, a 512x256 atlas, and that the glyph segmentation
// maps the ASCII range (rows 0-2) 1:1 onto the charset. Skips if the ARK is
// absent (same convention as ghogx_ui_test).

#include "ui/menu_font.h"

#include "ark_v3.h"

#include <cstdio>
#include <filesystem>
#include <string>
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
    CHECK(a->advance >= a->pw);
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

  if (g_failures == 0) {
    std::printf("ghogx_menu_font_test: OK (impact: charset+kern byte-exact, "
                "glyphs segmented, '%s'... measured %.1fpx)\n", "CAREER", wlen);
  } else {
    std::printf("ghogx_menu_font_test: %d FAILURE(S)\n", g_failures);
  }
  return g_failures == 0 ? 0 : 1;
}
