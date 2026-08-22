// engine/src/ui/meta_objects.h
//
// The menu's game-side objects: game / gamecfg / campaign / per-player config /
// song_provider. The stock menus message these ~120 times (STOCK_SURFACE.txt:
// game 54, campaign 44, ...). Most messages are get_<x>/set_<x> accessor pairs
// the menus round-trip (set_character punk1 ... get_character -> punk1); a
// MetaObject base implements that Sandbox convention over a property bag, so the
// bulk needs no per-message code. The computed/structural/data-backed messages
// (get_song_text from songs.dtb, get_player_config, num_profiles, ...) are
// overridden per class and grounded in ConfigDb data -- never canned constants.
// See FIDELITY.md for the source tags.

#pragma once

#include "core/object.h"
#include "core/symbol.h"

#include <memory>
#include <string>
#include <vector>

namespace ghogx::ui {

class ScreenManager;
class ConfigDb;

// Base game-side object: get_<x>/set_<x> accessor convention + universal Object
// messages. Subclasses override the non-accessor messages via handle_meta().
class MetaObject : public Object {
 public:
  MetaObject(Symbol cls, ScreenManager* mgr, ConfigDb* db) : cls_(cls), mgr_(mgr), db_(db) {}
  Symbol class_name() const override { return cls_; }
  DataNode handle_property(Symbol msg, const DataArray& args) override;

 protected:
  virtual bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) {
    (void)msg; (void)args; (void)out; return false;
  }
  // set_<x>(v) -> property x = v ; get_<x>() -> property x. Returns true if the
  // message matched the convention.
  bool generic_accessor(Symbol msg, const DataArray& args, DataNode& out);

  Symbol cls_;
  ScreenManager* mgr_;
  ConfigDb* db_;
};

// `game` / `gamecfg`: current game setup (per-player character/guitar/difficulty,
// song_index, venue, mode) via the accessor convention, + data-backed song
// lookups (get_song_text/get_song_artist_text from songs.dtb) + per-player
// config objects (get_player_config).
class GameConfig : public MetaObject {
 public:
  GameConfig(ScreenManager* mgr, ConfigDb* db);
  MetaObject* player(int i) {
    return (i >= 0 && i < static_cast<int>(players_.size())) ? players_[i].get() : nullptr;
  }

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override;

 private:
  std::vector<std::unique_ptr<MetaObject>> players_;
};

// `campaign`: a real (initially empty) profile store + campaign.dtb-backed
// queries. num_profiles == profiles_.size() (0 on a fresh boot -- the real
// value of an empty store, not a canned 0).
class Campaign : public MetaObject {
 public:
  Campaign(ScreenManager* mgr, ConfigDb* db);

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override;

 private:
  struct Profile {
    std::string name;
  };
  std::vector<Profile> profiles_;
};

// Create + register the data-backed game-side singletons (game, gamecfg [alias],
// campaign, song_provider, player0/player1 [aliases]) on the manager, replacing
// the bootstrap stubs.
void install_meta_singletons(ScreenManager& mgr, ConfigDb& db);

}  // namespace ghogx::ui
