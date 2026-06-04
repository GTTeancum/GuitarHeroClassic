// engine/src/ui/menu_labels_test.cpp
//
// Verifies the main-menu BandButton labels and unaligned text/layout tail fields
// directly from the stock PS2 main.milo_ps2.

#include "ui/menu_labels.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
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

static bool near(float a, float b) { return std::fabs(a - b) < 0.001f; }

int main(int argc, char** argv) {
  std::string ark_dir =
      argc > 1 ? argv[1]
               : "C:/Programming/GitHub/Guitar Hero II/Guitar Hero II PS2 (USA)/GEN";
  std::string hdr = first_existing(ark_dir, {"MAIN.HDR", "main.hdr"});
  std::string ark0 = first_existing(ark_dir, {"MAIN_0.ARK", "main_0.ark"});
  if (hdr.empty() || ark0.empty()) {
    std::printf("ghogx_menu_labels_test: SKIP (no stock ARK at %s)\n", ark_dir.c_str());
    return 0;
  }

  auto labels = ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/main.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> by_name;
  for (auto& l : labels) by_name[l.name] = l;

  struct Expect {
    const char* name;
    const char* text;
    const char* nav;
    float width;
  };
  const Expect expects[] = {
      {"main_career.btn", "CAREER", "main_quickspin.btn", 310.0f},
      {"main_quickspin.btn", "QUICK_PLAY", "main_multiplayer.btn", 320.0f},
      {"main_multiplayer.btn", "MULTIPLAYER", "main_tutorial.btn", 310.0f},
      {"main_tutorial.btn", "TRAINING", "main_options.btn", 290.0f},
      {"main_options.btn", "OPTIONS", "main_career.btn", 270.0f},
  };

  for (const auto& e : expects) {
    auto it = by_name.find(e.name);
    CHECK(it != by_name.end());
    if (it == by_name.end()) continue;
    const auto& lbl = it->second;
    CHECK(lbl.type == "BandButton");
    CHECK(lbl.font == "impact");
    CHECK(lbl.text == e.text);
    CHECK(lbl.nav == e.nav);
    CHECK(lbl.button_tail.valid);
    if (lbl.button_tail.valid) {
      CHECK(lbl.button_tail.all_caps == 1);
      CHECK(near(lbl.button_tail.label_width, e.width));
      CHECK(near(lbl.button_tail.box_height, 15.0f));
      CHECK(near(lbl.button_tail.field_0c, 1.0f));
      CHECK(lbl.button_tail.field_10 == 34);
      CHECK(near(lbl.button_tail.text_size, 0.5f));
      CHECK(near(lbl.button_tail.field_20, 1.0f));
      CHECK(lbl.button_tail.field_24 == 1);
      CHECK(near(lbl.button_tail.kerning, -0.05f));
      CHECK(near(lbl.button_tail.field_29, 30.0f));
      CHECK(near(lbl.button_tail.width_bound, 280.0f));
    }
  }

  if (g_failures == 0) {
    std::printf("ghogx_menu_labels_test: OK (main.milo BandButton labels + "
                "unaligned tail fields decoded)\n");
  } else {
    std::printf("ghogx_menu_labels_test: %d FAILURE(S)\n", g_failures);
  }
  return g_failures == 0 ? 0 : 1;
}
