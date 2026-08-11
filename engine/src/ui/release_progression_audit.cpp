// Headless release audit for career/profile persistence.
//
// Run `write` and `read` as separate processes against the same isolated
// profile path. This proves what survives a real process boundary without
// synthesizing menu/controller input or mutating the user's active save.

#include "core/data_node.h"
#include "core/object.h"
#include "core/symbol.h"
#include "ui/config_db.h"
#include "ui/meta_objects.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include "ark_v3.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

using namespace ghogx;

namespace {

void set_profile_environment(const char* path) {
#ifdef _WIN32
  _putenv_s("GHOGX_DISABLE_PROFILE_PERSISTENCE", "0");
  _putenv_s("GHOGX_PROFILE_PATH", path);
#else
  setenv("GHOGX_DISABLE_PROFILE_PERSISTENCE", "0", 1);
  setenv("GHOGX_PROFILE_PATH", path, 1);
#endif
}

int value(Object* object, const char* property) {
  return object
      ? object->handle_property(Symbol(property), DataArray())
            .as_int()
            .value_or(0)
      : 0;
}

Symbol canonical_difficulty(Symbol difficulty) {
  if (difficulty == Symbol("easy")) return Symbol("kDifficultyEasy");
  if (difficulty == Symbol("medium")) return Symbol("kDifficultyMedium");
  if (difficulty == Symbol("hard")) return Symbol("kDifficultyHard");
  if (difficulty == Symbol("expert")) return Symbol("kDifficultyExpert");
  return difficulty.valid() ? difficulty : Symbol("kDifficultyMedium");
}

Symbol final_campaign_song(const ui::ConfigDb& db) {
  Symbol last;
  for (std::size_t tier = 0;; ++tier) {
    const Symbol venue = db.campaign_venue_at(tier);
    if (!venue.valid()) break;
    for (Symbol song : db.campaign_songs(venue)) last = song;
  }
  return last;
}

bool file_has_line(const char* path, const std::string& line) {
  std::ifstream stream(path);
  std::string current;
  while (std::getline(stream, current))
    if (current == line) return true;
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  const std::string mode = argc > 1 ? argv[1] : "";
  if (argc != 6 ||
      (mode != "write" && mode != "read" && mode != "purchase-write" &&
       mode != "purchase-read" && mode != "profiles-write" &&
       mode != "profiles-read")) {
    std::fprintf(stderr,
                 "usage: ghogx_release_progression_audit "
                 "<write|read|purchase-write|purchase-read|profiles-write|"
                 "profiles-read> "
                 "<MAIN.HDR> <MAIN_0.ARK> <profile-path> <difficulty>\n");
    return 64;
  }
  set_profile_environment(argv[4]);
  ui::register_ui_classes();
  ui::ScreenManager manager;
  ui::install_default_singletons(manager);
  const gh::ark::ArkV3Reader ark = gh::ark::ArkV3Reader::load(argv[2]);
  ui::ConfigDb db;
  db.load(ark, {argv[3]});
  ui::install_meta_singletons(manager, db);
  Object* campaign = manager.resolve_object(Symbol("campaign"));
  Object* game = manager.resolve_object(Symbol("game"));
  Object* options = manager.resolve_object(Symbol("options"));
  Object* character_provider =
      manager.resolve_object(Symbol("character_provider"));
  if (!campaign || !game || !options || !character_provider) {
    std::fprintf(stderr,
                 "AUDIT_ERROR missing campaign/game/options/character singleton\n");
    return 65;
  }

  const Symbol difficulty = canonical_difficulty(Symbol(argv[5]));
  const Symbol final_song = final_campaign_song(db);
  if (!final_song.valid()) {
    std::fprintf(stderr, "AUDIT_ERROR campaign has no final song\n");
    return 66;
  }
  const Symbol beat_key(std::string("beat.") + difficulty.c_str() + "." +
                        final_song.c_str());
  const std::vector<Symbol> presentation = db.quickplay_songs();
  Object* song_provider = manager.resolve_object(Symbol("song_provider"));
  DataArray quickplay;
  quickplay.push(DataNode::Int(1));
  if (song_provider)
    song_provider->handle_property(Symbol("set_quickplay"), quickplay);
  const std::size_t provider_count = song_provider
                                         ? static_cast<std::size_t>(std::max(
                                               0, song_provider
                                                      ->handle_property(
                                                          Symbol("list_length"),
                                                          DataArray())
                                                      .as_int()
                                                      .value_or(0)))
                                         : 0;
  std::size_t quickplay_mismatches = 0;
  Symbol first_presented;
  Symbol first_provider;
  const std::size_t compared = std::min(presentation.size(), provider_count);
  for (std::size_t i = 0; i < compared; ++i) {
    DataArray provider_args;
    provider_args.push(DataNode::Int(static_cast<int>(i)));
    const Symbol provider =
        song_provider
            ? song_provider->handle_property(Symbol("get_symbol"), provider_args)
                  .as_symbol()
                  .value_or(Symbol())
            : Symbol();
    if (presentation[i] == provider) continue;
    if (quickplay_mismatches == 0) {
      first_presented = presentation[i];
      first_provider = provider;
    }
    ++quickplay_mismatches;
  }
  quickplay_mismatches +=
      presentation.size() > provider_count
          ? presentation.size() - provider_count
          : provider_count - presentation.size();
  std::printf(
      "AUDIT_QUICKPLAY presentation=%zu provider=%zu mismatches=%zu "
      "first_presented=%s first_provider=%s\n",
      presentation.size(), provider_count, quickplay_mismatches,
      first_presented.valid() ? first_presented.c_str() : "<none>",
      first_provider.valid() ? first_provider.c_str() : "<none>");
  DataArray select_first;
  select_first.push(DataNode::Int(0));
  game->handle_property(Symbol("set_song_index"), select_first);
  const Symbol loaded_first =
      game->handle_property(Symbol("get_song"), DataArray())
          .as_symbol()
          .value_or(Symbol());
  const int selected_list_index =
      game->handle_property(Symbol("get_song_index"), DataArray())
          .as_int()
          .value_or(-1);
  const int selected_db_index =
      game->get_property(Symbol("song_index")).as_int().value_or(-1);
  std::printf(
      "AUDIT_QUICKPLAY_SELECTION presented=%s loaded=%s list_index=%d "
      "db_index=%d\n",
      presentation.empty() ? "<none>" : presentation.front().c_str(),
      loaded_first.valid() ? loaded_first.c_str() : "<none>",
      selected_list_index, selected_db_index);
  std::size_t missing_midi = 0;
  std::size_t missing_vgs = 0;
  Symbol first_missing_midi;
  Symbol first_missing_vgs;
  for (Symbol song : presentation) {
    std::string midi = db.song_midi_path(song);
    std::string audio = db.song_audio_path(song);
    if (midi.empty())
      midi = std::string("songs/") + song.c_str() + "/" + song.c_str() +
             ".mid";
    if (audio.empty())
      audio = std::string("songs/") + song.c_str() + "/" + song.c_str();
    if (!ark.find(midi)) {
      if (!first_missing_midi.valid()) first_missing_midi = song;
      ++missing_midi;
    }
    if (!ark.find(audio + ".vgs")) {
      if (!first_missing_vgs.valid()) first_missing_vgs = song;
      ++missing_vgs;
    }
  }
  std::printf(
      "AUDIT_QUICKPLAY_ASSETS songs=%zu missing_midi=%zu missing_vgs=%zu "
      "first_missing_midi=%s first_missing_vgs=%s\n",
      presentation.size(), missing_midi, missing_vgs,
      first_missing_midi.valid() ? first_missing_midi.c_str() : "<none>",
      first_missing_vgs.valid() ? first_missing_vgs.c_str() : "<none>");
  auto unlocked = [&](Symbol key) {
    DataArray args;
    args.push(DataNode::Sym(key));
    return campaign->handle_property(Symbol("is_unlocked"), args)
        .as_int()
        .value_or(0);
  };
  const Symbol first_venue = db.campaign_venue_at(0);
  const Symbol second_venue = db.campaign_venue_at(1);
  const std::vector<Symbol> first_tier = db.campaign_songs(first_venue);
  const Symbol first_regular =
      first_tier.empty() ? Symbol() : first_tier.front();
  const Symbol first_encore =
      first_tier.empty() ? Symbol() : first_tier.back();
  const std::vector<Symbol> store_songs = db.store_items(Symbol("song"));
  const Symbol first_store =
      store_songs.empty() ? Symbol() : store_songs.front();
  std::printf(
      "AUDIT_UNLOCKS first_venue=%s:%d second_venue=%s:%d "
      "first_regular=%s:%d first_encore=%s:%d first_store=%s:%d\n",
      first_venue.c_str(), unlocked(first_venue), second_venue.c_str(),
      unlocked(second_venue), first_regular.c_str(), unlocked(first_regular),
      first_encore.c_str(), unlocked(first_encore), first_store.c_str(),
      unlocked(first_store));
  const bool common_ok =
      song_provider && !presentation.empty() && quickplay_mismatches == 0 &&
      loaded_first == presentation.front() && selected_list_index == 0 &&
      selected_db_index == db.song_index(presentation.front()) &&
      missing_midi == 0 && missing_vgs == 0;
  if (!common_ok) {
    std::fprintf(stderr, "AUDIT_ERROR canonical quickplay/media gate failed\n");
    return 70;
  }

  if (mode == "profiles-write") {
    auto select = [&](int slot) {
      DataArray args;
      args.push(DataNode::Int(slot));
      campaign->handle_property(Symbol("set_profile_slot"), args);
    };
    auto name = [&](const char* value, int slot) {
      DataArray args;
      args.push(DataNode::Str(value));
      args.push(DataNode::Int(slot));
      campaign->handle_property(Symbol("set_profile_name"), args);
    };
    auto add = [&](int cash) {
      DataArray args;
      args.push(DataNode::Int(cash));
      campaign->handle_property(Symbol("add_cash"), args);
    };
    select(0);
    name("ALPHA", 0);
    add(100);
    campaign->handle_property(Symbol("save_complete"), DataArray());
    select(1);
    name("BETA", 1);
    add(200);
    campaign->handle_property(Symbol("save_complete"), DataArray());
    select(0);
    campaign->handle_property(Symbol("save_complete"), DataArray());
    const int profiles = value(campaign, "num_profiles");
    const int active = value(campaign, "profile_slot");
    const int cash = value(campaign, "cash");
    const bool alpha_saved = file_has_line(argv[4], "profile.0.name=ALPHA");
    const bool beta_saved = file_has_line(argv[4], "profile.1.name=BETA");
    std::printf("AUDIT_PROFILES_WRITE profiles=%d active=%d cash=%d "
                 "alpha=%d beta=%d\n",
                profiles, active, cash, alpha_saved ? 1 : 0,
                beta_saved ? 1 : 0);
    return profiles == 2 && active == 0 && cash == 100 && alpha_saved &&
                   beta_saved
               ? 0
               : 71;
  }

  if (mode == "profiles-read") {
    auto select = [&](int slot) {
      DataArray args;
      args.push(DataNode::Int(slot));
      campaign->handle_property(Symbol("set_profile_slot"), args);
    };
    auto profile_name = [&](int slot) {
      DataArray args;
      args.push(DataNode::Int(slot));
      return std::string(campaign->handle_property(Symbol("profile_name"), args)
                             .as_string()
                             .value_or(""));
    };
    const int profiles = value(campaign, "num_profiles");
    select(0);
    const int alpha_cash = value(campaign, "cash");
    const std::string alpha = profile_name(0);
    select(1);
    const int beta_cash = value(campaign, "cash");
    const std::string beta = profile_name(1);
    std::printf("AUDIT_PROFILES_READ profiles=%d alpha=%s:%d beta=%s:%d\n",
                profiles, alpha.c_str(), alpha_cash, beta.c_str(), beta_cash);
    return profiles == 2 && alpha == "ALPHA" && alpha_cash == 100 &&
                   beta == "BETA" && beta_cash == 200
               ? 0
               : 72;
  }

  if (mode == "purchase-write") {
    const Symbol item("video3");
    DataArray denied_buy;
    denied_buy.push(DataNode::Sym(item));
    denied_buy.push(db.store_field(Symbol("video"), item, Symbol("price")));
    const int denied_cash =
        campaign->handle_property(Symbol("buy_item"), denied_buy)
            .as_int()
            .value_or(-1);
    DataArray item_arg;
    item_arg.push(DataNode::Sym(item));
    const int denied_unlock =
        campaign->handle_property(Symbol("is_unlocked"), item_arg)
            .as_int()
            .value_or(0);
    DataArray add;
    add.push(DataNode::Int(3000));
    campaign->handle_property(Symbol("add_cash"), add);
    DataArray buy;
    buy.push(DataNode::Sym(item));
    buy.push(db.store_field(Symbol("video"), item, Symbol("price")));
    const int cash_after = campaign->handle_property(Symbol("buy_item"), buy)
                               .as_int()
                               .value_or(-1);
    campaign->handle_property(Symbol("save_complete"), DataArray());
    const bool cash_saved =
        file_has_line(argv[4], "root.cash=" + std::to_string(cash_after));
    const bool unlock_saved = file_has_line(argv[4], "root.unlock=video3");
    std::printf(
        "AUDIT_PURCHASE_WRITE item=%s denied_cash=%d denied_unlock=%d "
        "cash=%d file_cash=%d file_unlock=%d\n",
        item.c_str(), denied_cash, denied_unlock, cash_after,
        cash_saved ? 1 : 0, unlock_saved ? 1 : 0);
    return denied_cash == 0 && denied_unlock == 0 && cash_after >= 0 &&
                   cash_saved && unlock_saved
               ? 0
               : 73;
  }

  if (mode == "purchase-read") {
    DataArray item;
    item.push(DataNode::Sym(Symbol("video3")));
    const int cash = value(campaign, "cash");
    const int owned = campaign->handle_property(Symbol("is_unlocked"), item)
                          .as_int()
                          .value_or(0);
    const int profiles = value(campaign, "num_profiles");
    std::printf(
        "AUDIT_PURCHASE_READ cash=%d video3_unlocked=%d profiles=%d\n",
        cash, owned, profiles);
    return cash >= 0 && owned == 1 && profiles == 0 ? 0 : 74;
  }

  if (mode == "write") {
    DataArray profile_name;
    profile_name.push(DataNode::Str("RELEASE AUDIT"));
    profile_name.push(DataNode::Int(0));
    campaign->handle_property(Symbol("set_profile_name"), profile_name);
    campaign->set_property(Symbol("last_difficulty"),
                           DataNode::Sym(difficulty));
    DataArray set_sync_offset;
    set_sync_offset.push(DataNode::Int(-73));
    options->handle_property(Symbol("set_sync_offset"), set_sync_offset);
    DataArray set_song;
    set_song.push(DataNode::Sym(final_song));
    game->handle_property(Symbol("set_song"), set_song);
    DataArray finish;
    finish.push(DataNode::Int(100000));
    finish.push(DataNode::Int(500));
    finish.push(DataNode::Sym(Symbol("career_cash_reason")));
    finish.push(DataNode::Str(""));
    finish.push(DataNode::Int(1));
    finish.push(DataNode::Int(300));
    campaign->handle_property(Symbol("finish_song"), finish);
    campaign->handle_property(Symbol("save_complete"), DataArray());
    const int profiles = value(campaign, "num_profiles");
    const int cash = value(campaign, "cash");
    const int status = value(campaign, "status");
    const int score = value(campaign, "career_score");
    const int won = value(campaign, "won_campaign");
    const int beat = campaign->get_property(beat_key).as_int().value_or(0);
    const bool cash_saved = file_has_line(argv[4], "profile.0.cash=800");
    const bool name_saved =
        file_has_line(argv[4], "profile.0.name=RELEASE AUDIT");
    const bool progress_saved =
        file_has_line(argv[4],
                      std::string("profile.0.value.") + beat_key.c_str() + "=1");
    const bool sync_saved =
        file_has_line(argv[4], "profile.0.value.sync_offset=-73");
    std::printf(
        "AUDIT_WRITE final_song=%s profiles=%d cash=%d status=%d score=%d "
        "won=%d beat=%d file_cash_800=%d file_profile_name=%d "
        "file_progress=%d file_sync_offset=%d\n",
        final_song.c_str(), profiles, cash, status, score, won, beat,
        cash_saved ? 1 : 0, name_saved ? 1 : 0, progress_saved ? 1 : 0,
        sync_saved ? 1 : 0);
    return profiles == 1 && cash == 800 && status == 1 && score == 100000 &&
                   won == 1 && beat == 1 && cash_saved && name_saved &&
                   progress_saved && sync_saved
               ? 0
               : 75;
  }

  DataArray unlock;
  unlock.push(DataNode::Sym(final_song));
  const int profiles = value(campaign, "num_profiles");
  const int cash = value(campaign, "cash");
  const int status = value(campaign, "status");
  const int score = value(campaign, "career_score");
  const int won = value(campaign, "won_campaign");
  const int beat = campaign->get_property(beat_key).as_int().value_or(0);
  const int final_unlocked =
      campaign->handle_property(Symbol("is_unlocked"), unlock)
          .as_int()
          .value_or(0);
  const int sync_offset =
      options->handle_property(Symbol("get_sync_offset"), DataArray())
          .as_int()
          .value_or(999);
  bool female_singer_visible = false;
  const int character_count =
      character_provider->handle_property(Symbol("list_length"), DataArray())
          .as_int()
          .value_or(0);
  for (int i = 0; i < character_count; ++i) {
    DataArray index;
    index.push(DataNode::Int(i));
    if (character_provider
            ->handle_property(Symbol("get_symbol"), index)
            .as_symbol()
            .value_or(Symbol()) == Symbol("female_singer")) {
      female_singer_visible = true;
      break;
    }
  }
  std::printf(
      "AUDIT_READ final_song=%s profiles=%d cash=%d status=%d score=%d "
      "won=%d beat=%d reported_unlocked=%d sync_offset=%d "
      "female_singer_visible=%d\n",
      final_song.c_str(), profiles, cash, status, score, won, beat,
      final_unlocked, sync_offset, female_singer_visible ? 1 : 0);
  return profiles == 1 && cash == 800 && status == 1 && score == 100000 &&
                 won == 1 && beat == 1 && final_unlocked == 1 &&
                 sync_offset == -73 && female_singer_visible
             ? 0
             : 76;
}
