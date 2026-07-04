// engine/src/ui/ui_test.cpp
//
// Headless boot test for the UI object layer + screen manager, run against the
// REAL stock GH2 DTBs from the PS2 ARK. Loads ui/gen/{main,splash,quickplay}.dtb,
// asserts the {new GHPanel/GHScreen ...} objects + their parsed handler blocks
// exist, then drives the manager: goto_screen(main_screen) runs the authored
// (enter)/reset_player_settings scripts, and a simulated SELECT_START on
// main_quickspin.btn runs the real SELECT_START_MSG switch -> {ui goto_screen
// qp_selsong_screen}. Skips (exit 0) when the ARK is absent.

#include "core/data_node.h"
#include "core/symbol.h"
#include "ui/config_db.h"
#include "ui/meta_objects.h"
#include "ui/screen_loader.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include "ark_v3.h"

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace ghogx;
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
    std::printf("ghogx_ui_test: SKIP (no stock ARK at %s)\n", ark_dir.c_str());
    return 0;
  }

  ui::register_ui_classes();
  ui::ScreenManager mgr;
  ui::install_default_singletons(mgr);

  gh::ark::ArkV3Reader ark = gh::ark::ArkV3Reader::load(hdr);
  std::vector<std::string> arks = {ark0};

  // Load the FULL stock screen set verbatim (every ui/gen/*.dtb).
  int n = ui::load_all_ui_screens(ark, arks, mgr);
  std::printf("ghogx_ui_test: loaded %d ui/gen DTBs, %zu objects registered\n",
              n, mgr.registry().size());
  CHECK(n >= 35);                      // ~40 ui/gen DTBs
  CHECK(mgr.registry().size() > 100);  // ~180 screen/panel objects
  if (auto cash_deductions =
          mgr.get_global(Symbol("CASH_AWARD_DEDUCTIONS")).as_array()) {
    CHECK(cash_deductions->size() > 20);
    auto first = cash_deductions->at(0).as_array();
    CHECK(first && first->size() >= 2);
    CHECK(first && first->at(0).as_symbol().value_or(Symbol()) ==
                       Symbol("ca_blurb2"));
    CHECK(first && first->at(1).as_int().value_or(0) == 460);
  } else {
    CHECK(false);
  }
  if (auto headline_adjs = mgr.get_global(Symbol("ADJS")).as_array()) {
    CHECK(headline_adjs->size() == 3);
    CHECK(headline_adjs->at(0).as_array() &&
          headline_adjs->at(0).as_array()->size() >= 9);
  } else {
    CHECK(false);
  }
  if (auto tutorial_states = mgr.get_global(Symbol("TUTORIAL_STATES")).as_array()) {
    CHECK(tutorial_states->size() > 10);
    CHECK(tutorial_states->at(0).as_symbol().value_or(Symbol()) ==
          Symbol("intro"));
  } else {
    CHECK(false);
  }
  for (const char* s : {"main_screen", "main_panel", "qp_selsong_screen", "options_screen"})
    CHECK(mgr.find_object(Symbol(s)) != nullptr);
  for (const char* s : {"meta", "helpbar"}) {
    Object* panel = mgr.find_object(Symbol(s));
    CHECK(panel != nullptr);
    CHECK(mgr.resolve_object(Symbol(s)) == panel);
  }
  if (Object* multi_song = mgr.find_object(Symbol("multi_coop_selsong_screen"))) {
    auto panels = multi_song->get_property(Symbol("panels")).as_array();
    CHECK(panels && panels->size() == 3);
    CHECK(panels && panels->at(1).as_symbol().value_or(Symbol()) ==
                        Symbol("sel_song_panel"));
  } else {
    CHECK(false);
  }
  if (Object* multi_venue = mgr.find_object(Symbol("multi_coop_venue_screen"))) {
    auto panels = multi_venue->get_property(Symbol("panels")).as_array();
    CHECK(panels && panels->size() == 3);
    CHECK(panels && panels->at(1).as_symbol().value_or(Symbol()) ==
                        Symbol("sel_venue_panel"));
  } else {
    CHECK(false);
  }

  // Game-side data layer: config/gen-DTB-backed objects (no canned constants).
  ui::ConfigDb db;
  db.load(ark, arks);
  ui::install_meta_singletons(mgr, db);
  CHECK(db.song_count() > 40);  // GH2 songs.dtb has ~74 songs
  const DataArray* modes = db.table(Symbol("modes"));
  CHECK(modes != nullptr);
  CHECK(modes && modes->find_keyed(Symbol("quickplay")) != nullptr);
  const DataArray* campaign_table = db.table(Symbol("campaign"));
  CHECK(campaign_table != nullptr);
  auto campaign_cash =
      campaign_table ? campaign_table->find_keyed(Symbol("cash")) : nullptr;
  const int authored_starting_cash =
      ui::ConfigDb::field(campaign_cash.get(), Symbol("starting"))
          .as_int()
          .value_or(-1);
  CHECK(authored_starting_cash == 50);
  auto star_awards =
      campaign_cash ? campaign_cash->find_keyed(Symbol("star_awards")) : nullptr;
  auto medium_awards =
      star_awards ? star_awards->find_keyed(Symbol("kDifficultyMedium")) : nullptr;
  const int authored_medium_five_star =
      (medium_awards && medium_awards->size() > 3)
          ? medium_awards->at(3).as_int().value_or(-1)
          : -1;
  CHECK(authored_medium_five_star == 250);
  auto required_songs =
      campaign_table ? campaign_table->find_keyed(Symbol("required_songs"))
                     : nullptr;
  auto easy_required =
      required_songs ? required_songs->find_keyed(Symbol("kDifficultyEasy"))
                     : nullptr;
  auto medium_required =
      required_songs ? required_songs->find_keyed(Symbol("kDifficultyMedium"))
                     : nullptr;
  auto hard_required =
      required_songs ? required_songs->find_keyed(Symbol("kDifficultyHard"))
                     : nullptr;
  auto expert_required =
      required_songs ? required_songs->find_keyed(Symbol("kDifficultyExpert"))
                     : nullptr;
  const int authored_easy_required =
      (easy_required && easy_required->size() > 1)
          ? easy_required->at(1).as_int().value_or(-1)
          : -1;
  const int authored_medium_required =
      (medium_required && medium_required->size() > 1)
          ? medium_required->at(1).as_int().value_or(-1)
          : -1;
  const int authored_hard_required =
      (hard_required && hard_required->size() > 1)
          ? hard_required->at(1).as_int().value_or(-1)
          : -1;
  const int authored_expert_required =
      (expert_required && expert_required->size() > 1)
          ? expert_required->at(1).as_int().value_or(-1)
          : -1;
  CHECK(authored_easy_required == 3);
  CHECK(authored_medium_required == 4);
  CHECK(authored_hard_required == 5);
  CHECK(authored_expert_required == 5);
  auto campaign_order =
      campaign_table ? campaign_table->find_keyed(Symbol("order")) : nullptr;
  Symbol authored_first_venue;
  Symbol authored_first_song;
  Symbol authored_second_song;
  Symbol authored_first_encore;
  int authored_campaign_song_count = 0;
  if (campaign_order && campaign_order->size() > 1) {
    auto first = campaign_order->at(1).as_array();
    if (first && first->size() > 0)
      authored_first_venue = first->at(0).as_symbol().value_or(Symbol());
    if (first && first->size() > 1)
      authored_first_song = first->at(1).as_symbol().value_or(Symbol());
    if (first && first->size() > 2)
      authored_second_song = first->at(2).as_symbol().value_or(Symbol());
    if (first && first->size() > 1)
      authored_first_encore =
          first->at(first->size() - 1).as_symbol().value_or(Symbol());
    for (std::size_t i = 1; i < campaign_order->size(); ++i) {
      auto tier = campaign_order->at(i).as_array();
      if (!tier || tier->size() < 2) continue;
      authored_campaign_song_count += static_cast<int>(tier->size() - 1);
    }
  }
  CHECK(authored_first_venue == Symbol("battle"));
  CHECK(authored_first_song == Symbol("shoutatthedevil"));
  CHECK(authored_second_song == Symbol("mother"));
  CHECK(authored_first_encore == Symbol("tonightimgonna"));
  CHECK(authored_campaign_song_count == 40);
  int authored_battle_index = -1;
  if (campaign_order) {
    for (std::size_t i = 1; i < campaign_order->size(); ++i) {
      auto row = campaign_order->at(i).as_array();
      if (row && row->size() &&
          row->at(0).as_symbol().value_or(Symbol()) == Symbol("battle")) {
        authored_battle_index = static_cast<int>(i - 1);
        break;
      }
    }
  }
  CHECK(authored_battle_index == 0);
  const int authored_shout_duration =
      db.song_duration_sec(Symbol("shoutatthedevil"));
  CHECK(authored_shout_duration > 0);
  const int authored_mother_duration = db.song_duration_sec(authored_second_song);
  CHECK(authored_mother_duration > 0);
  const int authored_encore_duration = db.song_duration_sec(authored_first_encore);
  CHECK(authored_encore_duration > 0);
  auto song_name = [&](Symbol song) {
    for (std::size_t i = 0; i < db.song_count(); ++i) {
      if (db.song_key(i) == song)
        return std::string(db.song_field(i, Symbol("name"))
                               .as_string()
                               .value_or(""));
    }
    return std::string();
  };
  const std::string authored_first_name = song_name(authored_first_song);
  const std::string authored_second_name = song_name(authored_second_song);
  const std::string authored_encore_name = song_name(authored_first_encore);
  CHECK(!authored_first_name.empty());
  CHECK(!authored_second_name.empty());
  CHECK(!authored_encore_name.empty());
  const DataArray* guitars = db.table(Symbol("guitars"));
  CHECK(guitars != nullptr);
  auto guitar_at = [&](int index) {
    if (!guitars || index < 0 || index >= static_cast<int>(guitars->size()))
      return Symbol();
    auto rec = guitars->at(static_cast<std::size_t>(index)).as_array();
    return (rec && !rec->empty()) ? rec->at(0).as_symbol().value_or(Symbol())
                                  : Symbol();
  };
  auto first_skin_for_guitar = [&](Symbol guitar) {
    auto rec = guitars ? guitars->find_keyed(guitar) : nullptr;
    auto skins = rec ? rec->find_keyed(Symbol("skins")) : nullptr;
    if (!skins || skins->size() < 2) return Symbol();
    auto skin = skins->at(1).as_array();
    return (skin && !skin->empty()) ? skin->at(0).as_symbol().value_or(Symbol())
                                    : Symbol();
  };
  const Symbol authored_first_guitar = guitar_at(0);
  const Symbol authored_second_guitar = guitar_at(1);
  const Symbol authored_first_guitar_skin =
      first_skin_for_guitar(authored_first_guitar);
  const Symbol authored_second_guitar_skin =
      first_skin_for_guitar(authored_second_guitar);
  CHECK(authored_first_guitar == Symbol("lespaul"));
  CHECK(authored_second_guitar == Symbol("sg"));
  CHECK(authored_first_guitar_skin == Symbol("lp_cherry"));
  CHECK(authored_second_guitar_skin == Symbol("sg_cherry"));
  const DataArray* credits = db.table(Symbol("credits"));
  CHECK(credits != nullptr);
  CHECK(credits && credits->size() > 1000);
  Object* credits_screen = mgr.find_object(Symbol("credits_screen"));
  CHECK(credits_screen != nullptr);
  if (credits && credits_screen) {
    CHECK(credits_screen->handle_property(Symbol("num_lines"), DataArray())
              .as_int()
              .value_or(0) == static_cast<int>(credits->size()));
  }
  Object* stats_provider =
      mgr.create_object(Symbol("StatsProvider"), Symbol("stats_provider"));
  CHECK(stats_provider != nullptr);
  if (stats_provider) {
    const int stats_rows =
        stats_provider->handle_property(Symbol("list_length"), DataArray())
            .as_int()
            .value_or(0);
    CHECK(stats_rows > 0);
    DataArray first_row;
    first_row.push(DataNode::Int(0));
    CHECK(stats_provider->handle_property(Symbol("get_section"), first_row)
              .as_symbol()
              .has_value());
    CHECK(stats_provider->handle_property(Symbol("get_notes1"), first_row)
              .as_string()
              .has_value());
  }
  if (Object* game = mgr.resolve_object(Symbol("game"))) {
    CHECK(game->get_property(Symbol("song")).as_symbol().value_or(Symbol()) ==
          authored_first_song);
    CHECK(game->get_property(Symbol("venue")).as_symbol().value_or(Symbol()) ==
          authored_first_venue);
    DataNode title = game->handle_property(Symbol("get_song_text"), DataArray());
    std::printf("ghogx_ui_test: songs=%zu  song[0]=\"%s\"\n", db.song_count(),
                 std::string(title.as_string().value_or("")).c_str());
    CHECK(std::string(title.as_string().value_or("")) == authored_first_name);
    CHECK(game->handle_property(Symbol("get_venue_index"), DataArray())
              .as_int()
              .value_or(-1) == authored_battle_index);
    CHECK(game->handle_property(Symbol("song_duration_sec"), DataArray())
              .as_int()
              .value_or(-1) == authored_shout_duration);
    DataArray set_second_song;
    set_second_song.push(DataNode::Int(1));
    game->handle_property(Symbol("set_song_index"), set_second_song);
    CHECK(game->get_property(Symbol("song")).as_symbol().value_or(Symbol()) ==
          authored_second_song);
    CHECK(std::string(game->handle_property(Symbol("get_song_text"), DataArray())
                          .as_string()
                          .value_or("")) == authored_second_name);
    CHECK(game->handle_property(Symbol("song_duration_sec"), DataArray())
              .as_int()
              .value_or(-1) == authored_mother_duration);
    DataArray set_first_encore_index;
    set_first_encore_index.push(DataNode::Int(4));
    game->handle_property(Symbol("set_song_index"), set_first_encore_index);
    game->handle_property(Symbol("set_career_song"), DataArray());
    CHECK(game->get_property(Symbol("song")).as_symbol().value_or(Symbol()) ==
          authored_first_encore);
    CHECK(std::string(game->handle_property(Symbol("get_song_text"), DataArray())
                          .as_string()
                          .value_or("")) == authored_encore_name);
    CHECK(game->handle_property(Symbol("song_duration_sec"), DataArray())
              .as_int()
              .value_or(-1) == authored_encore_duration);
    game->handle_property(Symbol("set_quickplay"), DataArray());
    DataArray reset_song;
    reset_song.push(DataNode::Int(0));
    game->handle_property(Symbol("set_song_index"), reset_song);
    CHECK(game->handle_property(Symbol("get_num_guitars"), DataArray()).as_int().value_or(0) > 0);
    CHECK(game->get_property(Symbol("guitar")).as_symbol().value_or(Symbol()) ==
          authored_first_guitar);
    CHECK(game->get_property(Symbol("guitar_skin")).as_symbol().value_or(Symbol()) ==
          authored_first_guitar_skin);
    DataArray set_second_guitar;
    set_second_guitar.push(DataNode::Int(1));
    game->handle_property(Symbol("set_guitar_index"), set_second_guitar);
    CHECK(game->get_property(Symbol("guitar_index")).as_int().value_or(-1) == 1);
    CHECK(game->get_property(Symbol("guitar")).as_symbol().value_or(Symbol()) ==
          authored_second_guitar);
    CHECK(game->get_property(Symbol("guitar_skin")).as_symbol().value_or(Symbol()) ==
          authored_second_guitar_skin);
    DataArray p1_args;
    p1_args.push(DataNode::Int(1));
    Object* p1 = game->handle_property(Symbol("get_player_config"), p1_args).as_object();
    CHECK(p1 != nullptr);
    if (p1) {
      CHECK(p1->handle_property(Symbol("get_character"), DataArray()).as_string().has_value());
      CHECK(p1->handle_property(Symbol("get_character_outfit"), DataArray()).as_string().has_value());
      CHECK(p1->handle_property(Symbol("get_guitar"), DataArray()).as_string().has_value());
      CHECK(p1->handle_property(Symbol("get_guitar_skin"), DataArray()).as_string().has_value());
      CHECK(p1->handle_property(Symbol("get_instrument_type"), DataArray()).as_string().has_value());
      CHECK(p1->handle_property(Symbol("score"), DataArray()).as_int().value_or(0) > 0);
      p1->handle_property(Symbol("set_guitar_index"), set_second_guitar);
      CHECK(p1->get_property(Symbol("guitar")).as_symbol().value_or(Symbol()) ==
            authored_second_guitar);
      CHECK(p1->get_property(Symbol("guitar_skin")).as_symbol().value_or(Symbol()) ==
            authored_second_guitar_skin);
    }
    CHECK(game->handle_property(Symbol("get_difficulty_sym"), p1_args)
              .as_string()
              .has_value());
    DataArray get_hud_file;
    get_hud_file.push(DataNode::Sym(Symbol("hud_file")));
    CHECK(game->handle_property(Symbol("get"), get_hud_file)
              .as_symbol()
              .value_or(Symbol()) == Symbol("hud_sp.milo"));
    DataArray get_show_hud;
    get_show_hud.push(DataNode::Sym(Symbol("show_hud")));
    CHECK(game->handle_property(Symbol("get"), get_show_hud)
              .as_symbol()
              .value_or(Symbol()) == Symbol("TRUE"));
    DataArray set_mode_practice;
    set_mode_practice.push(DataNode::Sym(Symbol("mode")));
    set_mode_practice.push(DataNode::Sym(Symbol("practice")));
    game->handle_property(Symbol("set"), set_mode_practice);
    DataArray get_game_screen;
    get_game_screen.push(DataNode::Sym(Symbol("game_screen")));
    CHECK(game->handle_property(Symbol("get"), get_game_screen)
              .as_symbol()
              .value_or(Symbol()) == Symbol("practice_game_screen"));
    CHECK(game->handle_property(Symbol("get"), get_hud_file)
              .as_symbol()
              .value_or(Symbol()) == Symbol("hud_practice.milo"));
    DataArray set_mode_coop;
    set_mode_coop.push(DataNode::Sym(Symbol("mode")));
    set_mode_coop.push(DataNode::Sym(Symbol("multi_coop")));
    game->handle_property(Symbol("set"), set_mode_coop);
    CHECK(game->handle_property(Symbol("get"), get_hud_file)
              .as_symbol()
              .value_or(Symbol()) == Symbol("hud_coop.milo"));
  } else {
    CHECK(false);
  }
  if (Object* band = mgr.resolve_object(Symbol("band"))) {
    CHECK(band->handle_property(Symbol("score"), DataArray()).as_int().value_or(0) > 0);
    CHECK(band->handle_property(Symbol("star_rating"), DataArray()).as_string().has_value());
  } else {
    CHECK(false);
  }
  if (Object* highscores = mgr.resolve_object(Symbol("highscores"))) {
    DataArray rank_args;
    rank_args.push(DataNode::Int(123456));
    CHECK(highscores->handle_property(Symbol("check_highscore"), rank_args)
              .as_int()
              .value_or(-1) == 0);
    DataArray row_args;
    row_args.push(DataNode::Int(0));
    auto empty_row =
        highscores->handle_property(Symbol("get_highscore"), row_args).as_array();
    CHECK(empty_row && empty_row->size() == 3);
    CHECK(empty_row && empty_row->at(1).as_string().value_or("") == "");
    CHECK(empty_row && empty_row->at(2).as_int().value_or(-1) == 0);
    DataArray add_args;
    add_args.push(DataNode::Str("AAAA"));
    add_args.push(DataNode::Int(123456));
    highscores->handle_property(Symbol("add"), add_args);
    auto stored_row =
        highscores->handle_property(Symbol("get_highscore"), row_args).as_array();
    CHECK(stored_row && stored_row->at(1).as_string().value_or("") == "AAAA");
    CHECK(stored_row && stored_row->at(2).as_int().value_or(0) == 123456);
    DataArray better_rank_args;
    better_rank_args.push(DataNode::Int(200000));
    CHECK(highscores->handle_property(Symbol("check_highscore"), better_rank_args)
              .as_int()
              .value_or(-1) == 0);
  } else {
    CHECK(false);
  }
  if (Object* song_provider = mgr.resolve_object(Symbol("song_provider"))) {
    CHECK(song_provider->handle_property(Symbol("list_length"), DataArray())
              .as_int()
              .value_or(-1) == authored_campaign_song_count);
    DataArray set_qp_provider;
    set_qp_provider.push(DataNode::Sym(Symbol("TRUE")));
    song_provider->handle_property(Symbol("set_quickplay"), set_qp_provider);
    DataArray first_encore_args;
    first_encore_args.push(DataNode::Int(4));
    CHECK(song_provider->handle_property(Symbol("get_symbol"), first_encore_args)
              .as_symbol()
              .value_or(Symbol()) == authored_first_encore);
    DataArray second_tier_args;
    second_tier_args.push(DataNode::Int(5));
    CHECK(song_provider->handle_property(Symbol("num_headers"), second_tier_args)
              .as_int()
              .value_or(-1) == 2);
    DataArray p0_inst_args;
    p0_inst_args.push(DataNode::Sym(Symbol("shoutatthedevil")));
    p0_inst_args.push(DataNode::Int(0));
    CHECK(song_provider->handle_property(Symbol("get_instrument"), p0_inst_args)
              .as_symbol()
              .value_or(Symbol()) == Symbol("guitar"));
    DataArray p1_inst_args;
    p1_inst_args.push(DataNode::Sym(Symbol("shoutatthedevil")));
    p1_inst_args.push(DataNode::Int(1));
    CHECK(song_provider->handle_property(Symbol("get_instrument"), p1_inst_args)
              .as_symbol()
              .value_or(Symbol()) == Symbol("bass"));
    DataArray has_bass_args;
    has_bass_args.push(DataNode::Sym(Symbol("shoutatthedevil")));
    has_bass_args.push(DataNode::Sym(Symbol("bass")));
    CHECK(song_provider->handle_property(Symbol("has_instrument"), has_bass_args)
              .as_symbol()
              .value_or(Symbol()) == Symbol("TRUE"));
  } else {
    CHECK(false);
  }
  if (Object* campaign = mgr.resolve_object(Symbol("campaign"))) {
    CHECK(campaign->handle_property(Symbol("starting_cash"), DataArray())
              .as_int()
              .value_or(-1) == authored_starting_cash);
    CHECK(campaign->handle_property(Symbol("cash"), DataArray())
              .as_int()
              .value_or(-1) == authored_starting_cash);
    DataArray finish_args;
    finish_args.push(DataNode::Int(287537));
    auto award = campaign->handle_property(Symbol("finish_song"), finish_args).as_array();
    CHECK(award && award->size() >= 3);
    CHECK(award && award->at(1).as_int().value_or(-1) ==
                       authored_medium_five_star);
    CHECK(award && award->at(2).as_symbol().value_or(Symbol()) == Symbol("ca_reason"));
    DataArray easy_progress_args;
    easy_progress_args.push(DataNode::Sym(Symbol("kDifficultyEasy")));
    DataArray medium_progress_args;
    medium_progress_args.push(DataNode::Sym(Symbol("kDifficultyMedium")));
    DataArray hard_progress_args;
    hard_progress_args.push(DataNode::Sym(Symbol("kDifficultyHard")));
    DataArray expert_progress_args;
    expert_progress_args.push(DataNode::Sym(Symbol("kDifficultyExpert")));
    CHECK(campaign->handle_property(Symbol("get_status_progress"),
                                    easy_progress_args)
              .as_int()
              .value_or(-1) == authored_easy_required);
    CHECK(campaign->handle_property(Symbol("get_status_progress"),
                                    medium_progress_args)
              .as_int()
              .value_or(-1) == authored_medium_required);
    CHECK(campaign->handle_property(Symbol("get_status_progress"),
                                    hard_progress_args)
              .as_int()
              .value_or(-1) == authored_hard_required);
    CHECK(campaign->handle_property(Symbol("get_status_progress"),
                                    expert_progress_args)
              .as_int()
              .value_or(-1) == authored_expert_required);
    CHECK(campaign->handle_property(Symbol("num_profiles"), DataArray())
              .as_int()
              .value_or(-1) == 0);
    DataArray slot0;
    slot0.push(DataNode::Int(0));
    CHECK(campaign->handle_property(Symbol("is_empty_profile"), slot0)
              .as_symbol()
              .value_or(Symbol("FALSE")) == Symbol("TRUE"));
    CHECK(campaign->handle_property(Symbol("profile_name"), slot0)
              .as_string()
              .value_or("") == "");
    CHECK(campaign->handle_property(Symbol("empty_slot"), DataArray())
              .as_int()
              .value_or(-1) == 0);
    DataArray name_slot0;
    name_slot0.push(DataNode::Str("AAAA"));
    name_slot0.push(DataNode::Int(0));
    campaign->handle_property(Symbol("set_profile_name"), name_slot0);
    CHECK(campaign->handle_property(Symbol("num_profiles"), DataArray())
              .as_int()
              .value_or(-1) == 1);
    CHECK(campaign->handle_property(Symbol("is_empty_profile"), slot0)
              .as_symbol()
              .value_or(Symbol("TRUE")) == Symbol("FALSE"));
    CHECK(campaign->handle_property(Symbol("profile_name"), slot0)
              .as_string()
              .value_or("") == "AAAA");
    DataArray has_name;
    has_name.push(DataNode::Str("AAAA"));
    has_name.push(DataNode::Sym(Symbol("FALSE")));
    CHECK(campaign->handle_property(Symbol("has_profile_name"), has_name)
              .as_symbol()
              .value_or(Symbol("FALSE")) == Symbol("TRUE"));
    DataArray slot1;
    slot1.push(DataNode::Int(1));
    CHECK(campaign->handle_property(Symbol("empty_slot"), DataArray())
              .as_int()
              .value_or(-1) == 1);
    CHECK(campaign->handle_property(Symbol("is_empty_profile"), slot1)
              .as_symbol()
              .value_or(Symbol("FALSE")) == Symbol("TRUE"));
  } else {
    CHECK(false);
  }

  if (Object* char_multi = mgr.find_object(Symbol("char_multi"))) {
    DataArray loaded_args;
    loaded_args.push(DataNode::Int(0));
    CHECK(char_multi->handle_property(Symbol("is_char_loaded"), loaded_args)
              .as_string()
              .value_or("") == "TRUE");
  }
  if (Object* guitar_display = mgr.find_object(Symbol("guitar_display_panel"))) {
    DataArray show_args;
    show_args.push(DataNode::Int(0));
    show_args.push(DataNode::Sym(Symbol("sg")));
    show_args.push(DataNode::Sym(Symbol("sg_cherry")));
    show_args.push(DataNode::Sym(Symbol("guitar.pxy")));
    show_args.push(DataNode::Sym(Symbol("guitar_single.filt")));
    guitar_display->handle_property(Symbol("show_guitar"), show_args);
    CHECK(guitar_display->get_property(Symbol("guitar0"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("sg"));
    CHECK(guitar_display->get_property(Symbol("guitar_skin0"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("sg_cherry"));
    CHECK(guitar_display->get_property(Symbol("guitar_proxy0"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("guitar.pxy"));
    CHECK(guitar_display->get_property(Symbol("guitar_filter0"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("guitar_single.filt"));
    DataArray store_show_args;
    store_show_args.push(DataNode::Int(1));
    store_show_args.push(DataNode::Sym(Symbol("sg")));
    store_show_args.push(DataNode::Sym(Symbol("guitar.pxy")));
    store_show_args.push(DataNode::Sym(Symbol("guitar_single.filt")));
    guitar_display->handle_property(Symbol("show_guitar"), store_show_args);
    CHECK(guitar_display->get_property(Symbol("guitar1"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("sg"));
    CHECK(guitar_display->get_property(Symbol("guitar_skin1")).empty());
    CHECK(guitar_display->get_property(Symbol("guitar_proxy1"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("guitar.pxy"));
    CHECK(guitar_display->get_property(Symbol("guitar_filter1"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("guitar_single.filt"));
  } else {
    CHECK(false);
  }
  if (Object* char_single = mgr.find_object(Symbol("char_single"))) {
    DataArray char_args;
    char_args.push(DataNode::Int(0));
    char_args.push(DataNode::Sym(Symbol("punk1")));
    char_single->handle_property(Symbol("show_char"), char_args);
    CHECK(char_single->get_property(Symbol("char_outfit0"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("punk1"));
    CHECK(char_single->get_property(Symbol("char0")).as_symbol().value_or(Symbol()) ==
          Symbol("punk1"));
    DataArray get_char_args;
    get_char_args.push(DataNode::Int(0));
    CHECK(char_single->handle_property(Symbol("get_char"), get_char_args)
              .as_symbol()
              .value_or(Symbol()) == Symbol("punk1"));
    DataArray placer_args;
    placer_args.push(DataNode::Int(0));
    placer_args.push(DataNode::Sym(Symbol("char_single.placer")));
    char_single->handle_property(Symbol("set_placer"), placer_args);
    CHECK(char_single->get_property(Symbol("placer0"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("char_single.placer"));
    if (Object* panel = mgr.find_object(Symbol("sel_character_panel"))) {
      DataArray env_args;
      env_args.push(DataNode::Int(0));
      env_args.push(DataNode::Obj(panel));
      char_single->handle_property(Symbol("set_env"), env_args);
      CHECK(char_single->get_property(Symbol("env0")).as_object() == panel);
    } else {
      CHECK(false);
    }
  } else {
    CHECK(false);
  }
  if (Object* guitar_panel = mgr.find_object(Symbol("multi_sel_guitar_panel"))) {
    DataArray player_args;
    player_args.push(DataNode::Int(1));
    CHECK(guitar_panel->handle_property(Symbol("get_instrument_type"), player_args)
              .as_string()
              .has_value());
    CHECK(guitar_panel->handle_property(Symbol("get_num_guitars"), player_args)
              .as_int()
              .value_or(0) > 0);
  }
  // 1. The {new ...} objects exist.
  Object* main_panel = mgr.find_object(Symbol("main_panel"));
  Object* main_screen = mgr.find_object(Symbol("main_screen"));
  CHECK(main_panel != nullptr);
  CHECK(main_screen != nullptr);
  CHECK(main_panel && main_panel->class_name() == Symbol("GHPanel"));
  CHECK(main_screen && main_screen->class_name() == Symbol("GHScreen"));

  // 2. main_panel's authored handler blocks parsed (enter/poll/SELECT_START_MSG
  //    + the custom reset_player_settings).
  if (auto* mp = dynamic_cast<ui::UiObject*>(main_panel)) {
    CHECK(mp->has_handler(Symbol("enter")));
    CHECK(mp->has_handler(Symbol("poll")));
    CHECK(mp->has_handler(Symbol("SELECT_START_MSG")));
    CHECK(mp->has_handler(Symbol("reset_player_settings")));
    // ...but config entries are properties, not handlers.
    CHECK(!mp->has_handler(Symbol("file")));
    CHECK(main_panel->get_property(Symbol("file")).as_symbol().has_value());
  }

  // 3. goto_screen(main_screen) runs the real (enter) scripts. reset_player_
  //    settings hits the game stub -> proves the authored script executed.
  mgr.goto_screen(Symbol("main_screen"));
  CHECK(mgr.current_screen() == main_screen);
  // the authored (enter) -> reset_player_settings ran on the REAL game-side
  // objects: {game set_venue small2}{game set_character punk1 TRUE} and
  // {{game get_player_config 0} set_difficulty kDifficultyMedium}.
  if (Object* game = mgr.resolve_object(Symbol("game"))) {
    CHECK(game->get_property(Symbol("venue")).as_symbol().value_or(Symbol()) == Symbol("small2"));
    CHECK(game->get_property(Symbol("character")).as_symbol().value_or(Symbol()) == Symbol("punk"));
    CHECK(game->get_property(Symbol("character_outfit")).as_symbol().value_or(Symbol()) ==
          Symbol("punk1"));
  }
  if (Object* p0 = mgr.resolve_object(Symbol("player0")))
    CHECK(p0->get_property(Symbol("difficulty")).as_symbol().value_or(Symbol()) ==
          Symbol("kDifficultyMedium"));

  // 4. Simulate a SELECT_START on main_quickspin.btn: the real SELECT_START_MSG
  //    switch must route to {ui goto_screen qp_selsong_screen}.
  CHECK(mgr.find_object(Symbol("qp_selsong_screen")) != nullptr);  // quickplay.dtb loaded it
  mgr.set_global(Symbol("component"), DataNode::Sym(Symbol("main_quickspin.btn")));
  main_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
  CHECK(mgr.current_screen() != nullptr &&
        mgr.current_screen()->name() == Symbol("qp_selsong_screen"));

  if (credits && credits_screen) {
    Object* credits_panel = mgr.find_object(Symbol("credits_panel"));
    auto* credits_dir = dynamic_cast<ObjectDir*>(credits_panel);
    CHECK(credits_dir != nullptr);
    if (credits_dir && !credits_dir->find(Symbol("credits.lst"))) {
      auto list = std::make_unique<ui::UiObject>(Symbol("UIList"));
      list->set_name(Symbol("credits.lst"));
      list->set_manager(&mgr);
      credits_dir->add(std::move(list));
    }
    Object* credits_list =
        credits_dir ? credits_dir->find(Symbol("credits.lst")) : nullptr;
    CHECK(credits_list != nullptr);
    if (credits_list) {
      DataArray select_last;
      select_last.push(DataNode::Int(static_cast<int>(credits->size() - 1)));
      credits_list->handle_property(Symbol("set_selected"), select_last);
      mgr.goto_screen(Symbol("credits_screen"));
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("credits_screen"));
      credits_screen->handle_property(Symbol("SCROLL_MSG"), DataArray());
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("options_screen"));
    }
  }

  if (Object* selpart0 = mgr.find_object(Symbol("selpart0"))) {
    mgr.goto_screen(Symbol("selpart_screen"));
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("selpart_screen"));
    CHECK(selpart0->get_property(Symbol("ready_label"))
              .as_string()
              .value_or("") == "ready.lbl");
    DataArray is_ready_args;
    is_ready_args.push(DataNode::Int(0));
    CHECK(selpart0->handle_property(Symbol("is_select_done"), is_ready_args)
              .as_symbol()
              .value_or(Symbol("TRUE")) == Symbol("FALSE"));
    DataArray ready_args;
    ready_args.push(DataNode::Int(0));
    ready_args.push(DataNode::Sym(Symbol("TRUE")));
    selpart0->handle_property(Symbol("set_select_done"), ready_args);
    CHECK(selpart0->handle_property(Symbol("is_select_done"), is_ready_args)
              .as_symbol()
              .value_or(Symbol("FALSE")) == Symbol("TRUE"));
    ready_args = DataArray();
    ready_args.push(DataNode::Int(0));
    ready_args.push(DataNode::Sym(Symbol("FALSE")));
    selpart0->handle_property(Symbol("set_select_done"), ready_args);
    CHECK(selpart0->handle_property(Symbol("is_select_done"), is_ready_args)
              .as_symbol()
              .value_or(Symbol("TRUE")) == Symbol("FALSE"));
  } else {
    CHECK(false);
  }

  mgr.set_locale({{"lag_setting", "Current Lag Offset is %d ms"}});
  if (Object* options = mgr.resolve_object(Symbol("options"))) {
    DataArray sync_args;
    sync_args.push(DataNode::Int(-37));
    options->handle_property(Symbol("set_sync_offset"), sync_args);
    CHECK(options->handle_property(Symbol("get_sync_offset"), DataArray())
              .as_int()
              .value_or(0) == -37);
  } else {
    CHECK(false);
  }
  if (Object* lag_panel = mgr.find_object(Symbol("lag_panel"))) {
    mgr.goto_screen(Symbol("lag_screen"));
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("lag_screen"));
    CHECK(lag_panel->get_property(Symbol("lag")).as_int().value_or(0) == 37);
  } else {
    CHECK(false);
  }
  ui::UiObject lag_label(Symbol("BandLabel"));
  DataArray localized_args;
  localized_args.push(DataNode::Str("Current Lag Offset is 37 ms"));
  lag_label.handle_property(Symbol("set_localized"), localized_args);
  CHECK(lag_label.get_property(Symbol("text")).as_string().value_or("") ==
        "Current Lag Offset is 37 ms");

  // Direct-audit boots still run the stock store_screen enter path. Its
  // hide_models handler hides the video/guitar display panels in the base store
  // category, so those panels must not contribute MILO meshes or cameras.
  if (Object* store_video = mgr.find_object(Symbol("store_video_panel"))) {
    store_video->set_property(Symbol("showing"), DataNode::Sym(Symbol("TRUE")));
  }
  if (Object* store_guitar = mgr.find_object(Symbol("store_guitar_display_panel"))) {
    store_guitar->set_property(Symbol("showing"), DataNode::Sym(Symbol("TRUE")));
  }
  mgr.goto_screen(Symbol("store_screen"));
  CHECK(mgr.current_screen() != nullptr &&
        mgr.current_screen()->name() == Symbol("store_screen"));
  if (Object* store_video = mgr.find_object(Symbol("store_video_panel"))) {
    CHECK(store_video->get_property(Symbol("showing"))
              .as_symbol()
              .value_or(Symbol("TRUE")) == Symbol("FALSE"));
  } else {
    CHECK(false);
  }
  if (Object* store_guitar = mgr.find_object(Symbol("store_guitar_display_panel"))) {
    CHECK(store_guitar->get_property(Symbol("showing"))
              .as_symbol()
              .value_or(Symbol("TRUE")) == Symbol("FALSE"));
  } else {
    CHECK(false);
  }
  if (Object* helpbar = mgr.find_object(Symbol("helpbar"))) {
    CHECK(helpbar->get_property(Symbol("button_spacing")).as_float().value_or(0.0f) == 35.0f);
    CHECK(helpbar->get_property(Symbol("strumbar_spacing")).as_float().value_or(0.0f) == 70.0f);
    CHECK(helpbar->get_property(Symbol("text_spacing")).as_float().value_or(0.0f) == 30.0f);
    auto display = helpbar->get_property(Symbol("display")).as_array();
    CHECK(display && display->size() >= 3);
    bool has_fret1 = false;
    bool has_back = false;
    bool has_updown = false;
    if (display) {
      for (std::size_t i = 0; i < display->size(); ++i) {
        auto row = display->at(i).as_array();
        if (!row || row->size() < 2) continue;
        const Symbol control = row->at(0).as_symbol().value_or(Symbol());
        const Symbol token = row->at(1).as_symbol().value_or(Symbol());
        has_fret1 = has_fret1 || control == Symbol("fret1");
        has_back = has_back ||
                   (control == Symbol("fret2") && token == Symbol("help_back"));
        has_updown = has_updown ||
                     (control == Symbol("strum") && token == Symbol("help_updown"));
      }
    }
    CHECK(has_fret1);
    CHECK(has_back);
    CHECK(has_updown);
  } else {
    CHECK(false);
  }

  mgr.goto_screen(Symbol("tut_script_screen"));
  CHECK(mgr.current_screen() != nullptr &&
        mgr.current_screen()->name() == Symbol("tut_script_screen"));
  if (Object* tutorial = mgr.find_object(Symbol("tutorial"))) {
    CHECK(tutorial->get_property(Symbol("running"))
              .as_symbol()
              .value_or(Symbol("FALSE")) == Symbol("TRUE"));
  } else {
    CHECK(false);
  }

  if (g_failures == 0) {
    std::printf("ghogx_ui_test: stock main.dtb boots -> object tree + handlers + "
                "goto_screen + SELECT_START all run -- passed\n");
    return 0;
  }
  std::fprintf(stderr, "ghogx_ui_test: %d check(s) failed\n", g_failures);
  return 1;
}
