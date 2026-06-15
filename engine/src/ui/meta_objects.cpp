// engine/src/ui/meta_objects.cpp -- see meta_objects.h.

#include "ui/meta_objects.h"

#include "ui/config_db.h"
#include "ui/screen_manager.h"

#include <algorithm>
#include <cstring>

namespace ghogx::ui {

// --- MetaObject ------------------------------------------------------------
DataNode MetaObject::handle_property(Symbol msg, const DataArray& args) {
  DataNode out;
  if (handle_meta(msg, args, out)) return out;
  if (generic_accessor(msg, args, out)) return out;
  return Object::handle_property(msg, args);
}

bool MetaObject::generic_accessor(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();
  if (std::strncmp(m, "set_", 4) == 0) {
    set_property(Symbol(m + 4), args.size() ? args.at(0) : DataNode());
    return true;
  }
  if (std::strncmp(m, "get_", 4) == 0) {
    out = get_property(Symbol(m + 4));
    return true;
  }
  return false;
}

// --- GameConfig ------------------------------------------------------------
GameConfig::GameConfig(ScreenManager* mgr, ConfigDb* db) : MetaObject(Symbol("game"), mgr, db) {
  set_property(Symbol("song_index"), DataNode::Int(0));
  for (int i = 0; i < 2; ++i)
    players_.push_back(std::make_unique<MetaObject>(Symbol("PlayerCfg"), mgr, db));
}

bool GameConfig::handle_meta(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();

  if (std::strcmp(m, "get_player_config") == 0) {
    int i = args.size() ? args.at(0).as_int().value_or(0) : 0;
    if (i < 0 || i >= static_cast<int>(players_.size())) i = 0;
    out = DataNode::Obj(players_[i].get());
    return true;
  }

  // Multiplayer gating (main.dta poll): the main menu enables main_multiplayer.btn
  // only when {game is_multiple_controllers} AND NOT {game is_missing_multi_controller}.
  // Multiplayer is disabled for now (single-player port), expressed through the
  // original's own mechanism: one controller present, the second one missing. So
  // the stock poll's `{$this disable main_multiplayer.btn}` branch fires, exactly
  // as it would on a real console with a single pad.
  if (std::strcmp(m, "is_multiple_controllers") == 0) {
    out = DataNode::Sym(Symbol("FALSE"));
    return true;
  }
  if (std::strcmp(m, "is_missing_multi_controller") == 0) {
    out = DataNode::Sym(Symbol("TRUE"));
    return true;
  }

  // Data-backed song lookups (current song = song_index into songs.dtb).
  std::size_t si = static_cast<std::size_t>(
      std::max(0, get_property(Symbol("song_index")).as_int().value_or(0)));
  if (std::strcmp(m, "get_song_text") == 0) {
    out = db_ ? db_->song_field(si, Symbol("name")) : DataNode();
    return true;
  }
  if (std::strcmp(m, "get_song_artist_text") == 0) {
    out = db_ ? db_->song_field(si, Symbol("artist")) : DataNode();
    return true;
  }
  if (std::strcmp(m, "get_song_caption") == 0) {
    out = db_ ? db_->song_field(si, Symbol("name")) : DataNode();
    return true;
  }
  if (std::strcmp(m, "get_num_players") == 0 || std::strcmp(m, "num_players") == 0) {
    out = DataNode::Int(1);
    return true;
  }
  return false;
}

// --- Campaign --------------------------------------------------------------
Campaign::Campaign(ScreenManager* mgr, ConfigDb* db) : MetaObject(Symbol("campaign"), mgr, db) {}

bool Campaign::handle_meta(Symbol msg, const DataArray& args, DataNode& out) {
  (void)args;
  const char* m = msg.c_str();
  if (std::strcmp(m, "num_profiles") == 0) {
    out = DataNode::Int(static_cast<int>(profiles_.size()));  // empty store -> 0 (real)
    return true;
  }
  return false;
}

// --- registration ----------------------------------------------------------
void install_meta_singletons(ScreenManager& mgr, ConfigDb& db) {
  auto game = std::make_unique<GameConfig>(&mgr, &db);
  GameConfig* g = game.get();
  mgr.add_singleton(Symbol("game"), std::move(game));
  mgr.alias_singleton(Symbol("gamecfg"), g);
  if (g->player(0)) mgr.alias_singleton(Symbol("player0"), g->player(0));
  if (g->player(1)) mgr.alias_singleton(Symbol("player1"), g->player(1));

  mgr.add_singleton(Symbol("campaign"), std::make_unique<Campaign>(&mgr, &db));
  mgr.add_singleton(Symbol("song_provider"),
                    std::make_unique<MetaObject>(Symbol("song_provider"), &mgr, &db));
}

}  // namespace ghogx::ui
