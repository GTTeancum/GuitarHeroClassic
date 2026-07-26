#include "ui/menu_font.h"
#include "ui/menu_labels.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {
int failures = 0;
#define CHECK(value)                                                      \
  do {                                                                    \
    if (!(value)) {                                                       \
      std::fprintf(stderr, "FAIL %s:%d CHECK(%s)\n", __FILE__, __LINE__, \
                   #value);                                               \
      ++failures;                                                         \
    }                                                                     \
  } while (0)

std::string first_existing(const fs::path& dir,
                           const std::vector<std::string>& names) {
  for (const auto& name : names) {
    const fs::path path = dir / name;
    if (fs::exists(path)) return path.string();
  }
  return {};
}

bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }
}  // namespace

int main(int argc, char** argv) {
  const fs::path dir =
      argc > 1 ? fs::path(argv[1])
               : fs::path("C:/Programming/GitHub/Guitar Hero II/GH1/GEN");
  const std::string hdr = first_existing(dir, {"MAIN.HDR", "main.hdr"});
  const std::string ark = first_existing(dir, {"MAIN_0.ARK", "main_0.ark"});
  if (hdr.empty() || ark.empty()) {
    std::printf("ghogx_song_intro_overlay_format_test: SKIP (no GH1 ARK)\n");
    return 0;
  }

  const auto labels =
      ghogx::ui::extract_menu_labels(hdr, ark, "ghui/mtv_overlay.gh");
  std::map<std::string, ghogx::ui::MenuLabel> by_name;
  for (const auto& label : labels) by_name.emplace(label.name, label);

  const char* names[] = {
      "mtv_campaign_line1.lbl", "mtv_campaign_line1_shadow.lbl",
      "mtv_campaign_line2.lbl", "mtv_campaign_line2_shadow.lbl",
      "mtv_campaign_line3.lbl", "mtv_campaign_line3_shadow.lbl",
  };
  for (const char* name : names) {
    const auto it = by_name.find(name);
    CHECK(it != by_name.end());
    if (it == by_name.end()) continue;
    const auto& label = it->second;
    CHECK(label.type == "BandLabel");
    CHECK(label.has_world);
    CHECK(label.text_tail.valid);
    CHECK(label.font == "impactor_mtv.font");
    CHECK(near(label.text_tail.text_size, 30.0f));
  }

  CHECK(by_name["mtv_campaign_line1.lbl"].world[9] < -280.0f);
  CHECK(near(by_name["mtv_campaign_line1.lbl"].world[11], 191.983f));
  CHECK(near(by_name["mtv_campaign_line1.lbl"].text_tail.color[0], 0.9f));
  CHECK(near(by_name["mtv_campaign_line1_shadow.lbl"].text_tail.color[0],
             0.0f));

  ghogx::ui::MenuFont font;
  CHECK(font.load(hdr, ark, "ghui/gen/resources.rnd_ps2",
                  "impactor_mtv.font"));
  CHECK(font.valid());
  CHECK(font.atlas().width == 512);
  CHECK(font.atlas().height == 256);
  CHECK(near(font.cap_height(), 42.0f));
  CHECK(near(font.line_height(), 28.0f));
  CHECK(font.layout("AS MADE FAMOUS BY").size() == 14);

  std::printf("ghogx_song_intro_overlay_format_test: %s\n",
              failures == 0 ? "OK" : "FAILED");
  return failures == 0 ? 0 : 1;
}
