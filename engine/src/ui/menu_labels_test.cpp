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
    const char* parent;
    const char* text;
    const char* nav;
    float width;
  };
  const Expect expects[] = {
      {"main_career.btn", "main_buttons.view", "CAREER", "main_quickspin.btn", 310.0f},
      {"main_quickspin.btn", "main_buttons.view", "QUICK_PLAY", "main_multiplayer.btn", 320.0f},
      {"main_multiplayer.btn", "main_buttons.view", "MULTIPLAYER", "main_tutorial.btn", 310.0f},
      {"main_tutorial.btn", "main_buttons.view", "TRAINING", "main_options.btn", 290.0f},
      {"main_options.btn", "main_buttons.view", "OPTIONS", "main_career.btn", 270.0f},
  };

  for (const auto& e : expects) {
    auto it = by_name.find(e.name);
    CHECK(it != by_name.end());
    if (it == by_name.end()) continue;
    const auto& lbl = it->second;
    CHECK(lbl.type == "BandButton");
    CHECK(lbl.font == "impact");
    CHECK(lbl.parent == e.parent);
    CHECK(lbl.text == e.text);
    CHECK(lbl.nav == e.nav);
    CHECK(lbl.button_tail.valid);
    if (lbl.button_tail.valid) {
      CHECK(lbl.button_tail.fit_text == 1);
      CHECK(lbl.button_tail.all_caps == 1);
      CHECK(near(lbl.button_tail.label_width, e.width));
      CHECK(near(lbl.button_tail.box_height, 15.0f));
      CHECK(near(lbl.button_tail.leading, 1.0f));
      CHECK(lbl.button_tail.align_flags == 0x22u);
      CHECK(near(lbl.button_tail.scale, 0.5f));
      CHECK(near(lbl.button_tail.field_20, 1.0f));
      CHECK(near(lbl.button_tail.kerning, -0.05f));
      CHECK(near(lbl.button_tail.text_size, 30.0f));
      CHECK(near(lbl.button_tail.width_bound, 280.0f));
    }
  }

  auto store_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/store.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> store_by_name;
  for (auto& l : store_labels) store_by_name[l.name] = l;
  auto st = store_by_name.find("st_guitars.btn");
  CHECK(st != store_by_name.end());
  if (st != store_by_name.end()) {
    CHECK(st->second.type == "BandButton");
    CHECK(st->second.font == "tapeworm");
    CHECK(st->second.parent == "st_buttons.view");
    CHECK(st->second.text == "store_guitar");
    CHECK(st->second.button_tail.valid);
    if (st->second.button_tail.valid) {
      CHECK(st->second.button_tail.fit_text == 1);
      CHECK(st->second.button_tail.align_flags == 0x22u);
      CHECK(st->second.button_tail.all_caps == 1);
      CHECK(near(st->second.button_tail.label_width, 160.0f));
      CHECK(near(st->second.button_tail.box_height, 20.0f));
      CHECK(near(st->second.button_tail.scale, 0.0f));
      CHECK(near(st->second.button_tail.text_size, 30.0f));
      CHECK(near(st->second.button_tail.width_bound, 200.0f));
    }
  }
  auto unlock_shop = store_by_name.find("st_unlock_shop.lbl");
  CHECK(unlock_shop != store_by_name.end());
  if (unlock_shop != store_by_name.end()) {
    CHECK(unlock_shop->second.type == "BandLabel");
    CHECK(unlock_shop->second.parent == "st_screen2.view");
    CHECK(unlock_shop->second.text == "UNLOCK SHOP");
    CHECK(unlock_shop->second.has_showing);
    CHECK(!unlock_shop->second.showing);
  }
  auto st_item_name = store_by_name.find("st_item_name.lbl");
  CHECK(st_item_name != store_by_name.end());
  if (st_item_name != store_by_name.end()) {
    CHECK(st_item_name->second.has_showing);
    CHECK(st_item_name->second.showing);
  }

  auto store_bought_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/store_bought.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> store_bought_by_name;
  for (auto& l : store_bought_labels) store_bought_by_name[l.name] = l;
  auto sb = store_bought_by_name.find("store_bought.lbl");
  CHECK(sb != store_bought_by_name.end());
  if (sb != store_bought_by_name.end()) {
    CHECK(sb->second.type == "BandLabel");
    CHECK(sb->second.font == "dyingmarker");
    CHECK(sb->second.parent == "store_bought.view");
    CHECK(sb->second.text == "Congratulations on your new Guitar Hero.");
    CHECK(sb->second.has_local);
    CHECK(sb->second.has_world);
    CHECK(near(sb->second.world[0], 0.994421f));
    CHECK(near(sb->second.world[2], 0.104516f));
    CHECK(near(sb->second.world[6], -0.104516f));
    CHECK(near(sb->second.world[8], 0.994421f));
    CHECK(near(sb->second.world[9], -245.0f));
    CHECK(near(sb->second.world[10], 0.0f));
    CHECK(near(sb->second.world[11], 104.0f));
    CHECK(sb->second.text_tail.valid);
    if (sb->second.text_tail.valid) {
      CHECK(sb->second.text_tail.fit_text == 2);
      CHECK(sb->second.text_tail.align_flags == 0x11u);
      CHECK(near(sb->second.text_tail.label_width, 310.0f));
      CHECK(near(sb->second.text_tail.box_height, 250.0f));
      CHECK(near(sb->second.text_tail.text_size, 35.0f));
      CHECK(near(sb->second.text_tail.width_bound, 340.0f));
      CHECK(near(sb->second.text_tail.color[0], 0.25f));
      CHECK(near(sb->second.text_tail.color[1], 0.25f));
      CHECK(near(sb->second.text_tail.color[2], 0.25f));
      CHECK(near(sb->second.text_tail.color[3], 1.0f));
    }
  }

  auto option_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/options.milo_ps2");
  int option_buttons = 0;
  for (const auto& lbl : option_labels) {
    if (lbl.type != "BandButton") continue;
    ++option_buttons;
    CHECK(!lbl.parent.empty());
  }
  CHECK(option_buttons == 6);

  auto venue_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/sel_venue.milo_ps2");
  int venue_buttons = 0;
  for (const auto& lbl : venue_labels) {
    if (lbl.type != "BandButton") continue;
    ++venue_buttons;
    CHECK(lbl.parent.empty());
    CHECK(lbl.font == "hand");
  }
  CHECK(venue_buttons == 8);

  auto lag_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/lag.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> lag_by_name;
  for (auto& l : lag_labels) lag_by_name[l.name] = l;
  auto ac = lag_by_name.find("autocalibrate.btn");
  CHECK(ac != lag_by_name.end());
  if (ac != lag_by_name.end()) {
    CHECK(ac->second.type == "BandButton");
    CHECK(ac->second.parent.empty());
    CHECK(ac->second.font == "helveticablack");
    CHECK(ac->second.text == "lag_button_calibrate");
    CHECK(ac->second.nav == "reset_to_zero.btn");
    CHECK(ac->second.has_world);
    CHECK(near(ac->second.world[9], -237.0f));
    CHECK(near(ac->second.world[11], -80.0f));
    CHECK(ac->second.button_tail.valid);
    if (ac->second.button_tail.valid) {
      CHECK(near(ac->second.button_tail.label_width, 200.0f));
      CHECK(near(ac->second.button_tail.box_height, 30.0f));
      CHECK(near(ac->second.button_tail.text_size, 30.0f));
      CHECK(near(ac->second.button_tail.width_bound, 400.0f));
      CHECK(ac->second.button_tail.align_flags == 0x21u);
    }
  }
  auto rz = lag_by_name.find("reset_to_zero.btn");
  CHECK(rz != lag_by_name.end());
  if (rz != lag_by_name.end()) {
    CHECK(rz->second.type == "BandButton");
    CHECK(rz->second.parent.empty());
    CHECK(rz->second.font == "helveticablack");
    CHECK(rz->second.text == "lag_button_reset");
    CHECK(rz->second.nav == "autocalibrate.btn");
    CHECK(rz->second.has_world);
    CHECK(near(rz->second.world[9], -237.0f));
    CHECK(near(rz->second.world[11], -105.0f));
  }
  auto lag_setting = lag_by_name.find("setting.lbl");
  CHECK(lag_setting != lag_by_name.end());
  if (lag_setting != lag_by_name.end()) {
    CHECK(lag_setting->second.type == "BandLabel");
    CHECK(lag_setting->second.font == "helveticablackcondensed");
    CHECK(lag_setting->second.text == "lag_setting");
    CHECK(lag_setting->second.has_world);
    CHECK(near(lag_setting->second.world[9], -237.0f));
    CHECK(near(lag_setting->second.world[11], -132.0f));
    CHECK(lag_setting->second.text_tail.valid);
    if (lag_setting->second.text_tail.valid) {
      CHECK(lag_setting->second.text_tail.fit_text == 2);
      CHECK(lag_setting->second.text_tail.align_flags == 0x21u);
      CHECK(near(lag_setting->second.text_tail.label_width, 250.0f));
      CHECK(near(lag_setting->second.text_tail.box_height, 20.0f));
      CHECK(near(lag_setting->second.text_tail.leading, 1.0f));
      CHECK(near(lag_setting->second.text_tail.text_size, 25.0f));
      CHECK(near(lag_setting->second.text_tail.width_bound, 10000.0f));
    }
  }

  auto character_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/sel_character.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> character_by_name;
  for (auto& l : character_labels) character_by_name[l.name] = l;
  auto sc = character_by_name.find("sc_char_nm.lbl");
  CHECK(sc != character_by_name.end());
  if (sc != character_by_name.end()) {
    CHECK(sc->second.type == "BandLabel");
    CHECK(sc->second.font == "helveticablackcondensed");
    CHECK(sc->second.parent == "text_character.grp");
    CHECK(sc->second.text == "KING KENDALL");
    CHECK(sc->second.has_local);
    CHECK(sc->second.has_world);
    CHECK(near(sc->second.local[9], -280.0f));
    CHECK(near(sc->second.local[11], 138.0f));
    CHECK(sc->second.text_tail.valid);
    CHECK(sc->second.text_tail.fit_text == 2);
    CHECK(sc->second.text_tail.align_flags == 0x21u);
    CHECK(near(sc->second.text_tail.label_width, 200.0f));
    CHECK(near(sc->second.text_tail.box_height, 70.0f));
    CHECK(near(sc->second.text_tail.text_size, 40.0f));
    CHECK(near(sc->second.text_tail.width_bound, 200.0f));
  }
  auto sc_blurb = character_by_name.find("sc_char_blurb.lbl");
  CHECK(sc_blurb != character_by_name.end());
  if (sc_blurb != character_by_name.end()) {
    CHECK(sc_blurb->second.type == "BandLabel");
    CHECK(sc_blurb->second.font == "helveticablackcondensed");
    CHECK(sc_blurb->second.parent == "text_character.grp");
    CHECK(sc_blurb->second.has_local);
    CHECK(sc_blurb->second.has_world);
    CHECK(sc_blurb->second.text_tail.valid);
    if (sc_blurb->second.text_tail.valid) {
      CHECK(sc_blurb->second.text_tail.fit_text == 2);
      CHECK(sc_blurb->second.text_tail.align_flags == 0x11u);
      CHECK(near(sc_blurb->second.text_tail.label_width, 185.0f));
      CHECK(near(sc_blurb->second.text_tail.box_height, 280.0f));
      CHECK(near(sc_blurb->second.text_tail.text_size, 20.0f));
      CHECK(near(sc_blurb->second.text_tail.width_bound, 185.0f));
    }
  }

  auto guitar_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/sel_guitar.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> guitar_by_name;
  for (auto& l : guitar_labels) guitar_by_name[l.name] = l;
  auto sg = guitar_by_name.find("sg_guitar_nm.lbl");
  CHECK(sg != guitar_by_name.end());
  if (sg != guitar_by_name.end()) {
    CHECK(sg->second.type == "BandLabel");
    CHECK(sg->second.font == "helveticablackcondensed");
    CHECK(sg->second.parent == "sg_text_guitar.grp");
    CHECK(sg->second.text == "LOG");
    CHECK(sg->second.has_local);
    CHECK(sg->second.has_world);
    CHECK(near(sg->second.local[9], -280.0f));
    CHECK(near(sg->second.local[11], 138.0f));
    CHECK(near(sg->second.world[9], -702.0f));
    CHECK(near(sg->second.world[10], -975.0f));
    CHECK(sg->second.text_tail.valid);
    CHECK(sg->second.text_tail.fit_text == 2);
    CHECK(sg->second.text_tail.align_flags == 0x21u);
    CHECK(near(sg->second.text_tail.label_width, 300.0f));
    CHECK(near(sg->second.text_tail.box_height, 70.0f));
    CHECK(near(sg->second.text_tail.text_size, 40.0f));
      CHECK(near(sg->second.text_tail.width_bound, 340.0f));
  }

  auto multi_guitar_labels = ghogx::ui::extract_menu_labels(
      hdr, ark0, "ui/gen/multi_sel_guitar.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> multi_guitar_by_name;
  for (auto& l : multi_guitar_labels) multi_guitar_by_name[l.name] = l;
  CHECK(multi_guitar_by_name.find("sg1_guitar_nm1.lbl") ==
        multi_guitar_by_name.end());
  CHECK(multi_guitar_by_name.find("sg2_guitar_nm1.lbl") ==
        multi_guitar_by_name.end());

  auto endgame_labels =
      ghogx::ui::extract_menu_labels(hdr, ark0, "ui/gen/endgame.milo_ps2");
  std::map<std::string, ghogx::ui::MenuLabel> endgame_by_name;
  for (auto& l : endgame_labels) endgame_by_name[l.name] = l;
  auto eg_song = endgame_by_name.find("endgame_song_data.lbl");
  CHECK(eg_song != endgame_by_name.end());
  if (eg_song != endgame_by_name.end()) {
    CHECK(eg_song->second.type == "BandLabel");
    CHECK(eg_song->second.font == "clarendon");
    CHECK(eg_song->second.parent == "newspaper.grp");
    CHECK(eg_song->second.text_tail.valid);
    if (eg_song->second.text_tail.valid) {
      CHECK(eg_song->second.text_tail.fit_text == 2);
      CHECK(eg_song->second.text_tail.align_flags == 0x22u);
      CHECK(near(eg_song->second.text_tail.label_width, 440.0f));
      CHECK(near(eg_song->second.text_tail.box_height, 18.0f));
      CHECK(near(eg_song->second.text_tail.text_size, 24.0f));
      CHECK(near(eg_song->second.text_tail.width_bound, 2000.0f));
    }
  }

  auto setlist = ghogx::ui::extract_ui_list_layout(
      hdr, ark0, "ui/gen/sel_song_quickplay.milo_ps2", "ss_song.lst");
  std::printf("ghogx_menu_labels_test: ss_song.lst provider=%s parent=%s "
              "local=(%.1f %.1f) world=(%.1f %.1f) slots=%d row=%.1f "
              "text=%.1f width=%d\n",
              setlist.provider.c_str(), setlist.parent.c_str(),
              setlist.local_x, setlist.local_z, setlist.world_x,
              setlist.world_z, setlist.visible_slots, setlist.row_height,
              setlist.text_height, setlist.width_bound);
  CHECK(setlist.valid);
  CHECK(setlist.provider == "song2");
  CHECK(setlist.parent == "ss_songlist.view");
  CHECK(near(setlist.local_x, 25.0f));
  CHECK(near(setlist.local_z, -40.0f));
  CHECK(near(setlist.world_x, 25.0f));
  CHECK(near(setlist.world_z, 940.0f));
  CHECK(setlist.visible_slots == 5);
  CHECK(near(setlist.row_height, 40.0f));
  CHECK(near(setlist.text_height, 30.0f));
  CHECK(setlist.width_bound == 75);

  auto song2 = ghogx::ui::extract_ui_list_template_layout(
      hdr, ark0, "ui/gen/list_song2.milo_ps2");
  std::printf("ghogx_menu_labels_test: list_song2 header=(%.1f %.1f) "
              "list=(%.1f %.1f) size=(%.1f %.1f)\n",
              song2.header.world_x, song2.header.world_z,
              song2.list.world_x, song2.list.world_z,
              song2.header.text_size, song2.list.text_size);
  CHECK(song2.valid);
  CHECK(song2.header.name == "header.txt");
  CHECK(song2.header.font == "dyingmarker.font");
  CHECK(song2.header.text == "LIST TEXT");
  CHECK(near(song2.header.world_x, -294.0f));
  CHECK(near(song2.header.world_z, 1.0f));
  CHECK(near(song2.header.text_size, 25.0f));
  CHECK(near(song2.header.color[0], 1.0f));
  CHECK(near(song2.header.color[1], 1.0f));
  CHECK(near(song2.header.color[2], 1.0f));
  CHECK(near(song2.header.color[3], 1.0f));
  CHECK(near(song2.header.wrap_width, 0.0f));
  CHECK(near(song2.header.field_14, 1.0f));
  CHECK(near(song2.header.field_18, 0.0f));
  CHECK(near(song2.header.field_1c, 0.0f));
  CHECK(song2.header.flags == 0x00000200u);
  CHECK(song2.list.name == "list.txt");
  CHECK(song2.list.font == "dyingmarker.font");
  CHECK(song2.list.text == "TONIGHT I'M GONNA ROCK YOU TONIGHT");
  CHECK(near(song2.list.world_x, -263.0f));
  CHECK(near(song2.list.world_z, -30.0f));
  CHECK(near(song2.list.text_size, 26.0f));
  CHECK(near(song2.list.color[0], 1.0f));
  CHECK(near(song2.list.color[1], 1.0f));
  CHECK(near(song2.list.color[2], 1.0f));
  CHECK(near(song2.list.color[3], 1.0f));
  CHECK(near(song2.list.wrap_width, 600.0f));
  CHECK(near(song2.list.field_14, 0.75f));
  CHECK(near(song2.list.field_18, 0.0f));
  CHECK(near(song2.list.field_1c, 0.0f));
  CHECK(song2.list.flags == 0x00000200u);
  CHECK(song2.stars.valid);
  CHECK(near(song2.stars.world_x, 48.0f));
  CHECK(near(song2.stars.world_z, -30.0f));
  CHECK(near(song2.stars.text_size, 25.0f));
  CHECK(song2.score.valid);
  CHECK(near(song2.score.world_x, 250.0f));
  CHECK(near(song2.score.world_z, -28.0f));
  CHECK(near(song2.score.text_size, 25.0f));
  CHECK(song2.blurb.valid);
  CHECK(near(song2.blurb.world_x, -240.0f));
  CHECK(near(song2.blurb.world_z, -55.0f));
  CHECK(near(song2.blurb.text_size, 25.0f));

  auto helpbar_text = ghogx::ui::extract_text_template_layout(
      hdr, ark0, "ui/gen/helpbar.milo_ps2", "help_bar.txt");
  std::printf("ghogx_menu_labels_test: helpbar text font=%s size=%.1f "
              "wrap=%.2f field_14=%.2f\n",
              helpbar_text.font.c_str(), helpbar_text.text_size,
              helpbar_text.wrap_width, helpbar_text.field_14);
  CHECK(helpbar_text.valid);
  CHECK(helpbar_text.name == "help_bar.txt");
  CHECK(helpbar_text.font == "helveticablackcondensed.font");
  CHECK(helpbar_text.text == "help text");
  CHECK(near(helpbar_text.text_size, 18.0f));
  CHECK(std::fabs(helpbar_text.wrap_width - 375.84f) < 0.05f);
  CHECK(near(helpbar_text.field_14, 0.75f));
  CHECK(near(helpbar_text.color[0], 0.9f));
  CHECK(near(helpbar_text.color[1], 0.9f));
  CHECK(near(helpbar_text.color[2], 0.9f));
  CHECK(near(helpbar_text.color[3], 1.0f));
  CHECK(helpbar_text.flags == 0x00000001u);

  auto credits = ghogx::ui::extract_ui_list_layout(
      hdr, ark0, "ui/gen/credits.milo_ps2", "credits.lst");
  std::printf("ghogx_menu_labels_test: credits.lst world=(%.1f %.1f) "
              "slots=%d row=%.1f text=%.1f\n",
              credits.world_x, credits.world_z, credits.visible_slots,
              credits.row_height, credits.text_height);
  CHECK(credits.valid);
  CHECK(near(credits.world_x, 0.0f));
  CHECK(near(credits.world_z, 186.0f));
  CHECK(credits.visible_slots == 16);
  CHECK(near(credits.row_height, 30.0f));
  CHECK(near(credits.text_height, 25.0f));

  auto credit_title = ghogx::ui::extract_text_template_layout(
      hdr, ark0, "ui/gen/list_credits.milo_ps2", "title.txt");
  auto credit_name = ghogx::ui::extract_text_template_layout(
      hdr, ark0, "ui/gen/list_credits.milo_ps2", "name.txt");
  auto credit_center = ghogx::ui::extract_text_template_layout(
      hdr, ark0, "ui/gen/list_credits.milo_ps2", "center.txt");
  auto credit_center_name = ghogx::ui::extract_text_template_layout(
      hdr, ark0, "ui/gen/list_credits.milo_ps2", "centername.txt");
  CHECK(credit_title.valid);
  CHECK(credit_title.font == "clarendon.font");
  CHECK(near(credit_title.world_x, -10.0f));
  CHECK(near(credit_title.text_size, 20.0f));
  CHECK(credit_name.valid);
  CHECK(credit_name.font == "clarendon.font");
  CHECK(near(credit_name.world_x, 10.0f));
  CHECK(near(credit_name.text_size, 20.0f));
  CHECK(credit_center.valid);
  CHECK(credit_center.font == "rockletters.font");
  CHECK(near(credit_center.world_x, 0.0f));
  CHECK(near(credit_center.text_size, 30.0f));
  CHECK(credit_center_name.valid);
  CHECK(credit_center_name.font == "clarendon.font");
  CHECK(near(credit_center_name.world_x, 0.0f));
  CHECK(near(credit_center_name.text_size, 20.0f));

  auto stats = ghogx::ui::extract_ui_list_layout(
      hdr, ark0, "ui/gen/endgame_stats.milo_ps2", "stats_sections.lst");
  std::printf("ghogx_menu_labels_test: stats_sections.lst provider=%s "
              "parent=%s world=(%.1f %.1f) row=%.1f text=%.1f\n",
              stats.provider.c_str(), stats.parent.c_str(), stats.world_x,
              stats.world_z, stats.row_height, stats.text_height);
  CHECK(stats.valid);
  CHECK(stats.provider == "stats");
  CHECK(stats.parent == "endgame_stats.view");
  CHECK(std::fabs(stats.world_x - 1.0f) < 0.1f);
  CHECK(std::fabs(stats.world_z + 70.0f) < 0.1f);
  CHECK(near(stats.row_height, 24.0f));
  CHECK(near(stats.text_height, 30.0f));

  auto stats_section = ghogx::ui::extract_text_template_layout(
      hdr, ark0, "ui/gen/list_stats.milo_ps2", "section.txt");
  auto stats_notes1 = ghogx::ui::extract_text_template_layout(
      hdr, ark0, "ui/gen/list_stats.milo_ps2", "notes1.txt");
  CHECK(stats_section.valid);
  CHECK(stats_section.font == "receipt.font");
  CHECK(stats_section.text == "INTRO");
  CHECK(near(stats_section.world_x, -225.0f));
  CHECK(near(stats_section.text_size, 18.0f));
  CHECK(stats_notes1.valid);
  CHECK(stats_notes1.font == "receipt.font");
  CHECK(stats_notes1.text == "NOTES1");
  CHECK(near(stats_notes1.world_x, 40.0f));
  CHECK(near(stats_notes1.text_size, 14.0f));

  if (g_failures == 0) {
    std::printf("ghogx_menu_labels_test: OK (main.milo BandButton labels + "
                "stock UIList layouts decoded)\n");
  } else {
    std::printf("ghogx_menu_labels_test: %d FAILURE(S)\n", g_failures);
  }
  return g_failures == 0 ? 0 : 1;
}
