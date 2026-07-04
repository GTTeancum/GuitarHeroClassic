// engine/src/ui/meta_objects.cpp -- see meta_objects.h.

#include "ui/meta_objects.h"

#include "ui/config_db.h"
#include "ui/screen_manager.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace ghogx::ui {

namespace {

bool node_truthy(const DataNode& node) {
  if (auto i = node.as_int()) return *i != 0;
  if (auto f = node.as_float()) return *f != 0.0f;
  if (auto s = node.as_string())
    return *s == "TRUE" || *s == "true" || *s == "1";
  return false;
}

Symbol symbol_arg(const DataArray& args, std::size_t index) {
  if (index >= args.size()) return Symbol();
  if (auto s = args.at(index).as_symbol()) return *s;
  if (auto s = args.at(index).as_string())
    return Symbol(std::string(*s).c_str());
  return Symbol();
}

int int_arg(const DataArray& args, std::size_t index, int fallback = 0) {
  return index < args.size() ? args.at(index).as_int().value_or(fallback)
                             : fallback;
}

DataNode array_node(std::initializer_list<DataNode> values) {
  auto arr = std::make_shared<DataArray>();
  for (const auto& value : values) arr->push(value);
  return DataNode::Array(arr);
}

std::string node_text(const DataNode& node) {
  if (auto s = node.as_string()) return std::string(s->data(), s->size());
  if (auto i = node.as_int()) return std::to_string(*i);
  return {};
}

bool symbol_eq(const DataNode& node, Symbol sym) {
  if (auto s = node.as_symbol()) return *s == sym;
  if (auto s = node.as_string()) return *s == sym.c_str();
  return false;
}

Symbol difficulty_locale_symbol(Symbol difficulty) {
  if (difficulty == Symbol("kDifficultyEasy")) return Symbol("easy");
  if (difficulty == Symbol("kDifficultyMedium")) return Symbol("medium");
  if (difficulty == Symbol("kDifficultyHard")) return Symbol("hard");
  if (difficulty == Symbol("kDifficultyExpert")) return Symbol("expert");
  return difficulty;
}

void flatten_symbols(const DataNode& node, std::vector<Symbol>& out) {
  if (auto s = node.as_symbol()) {
    out.push_back(*s);
    return;
  }
  if (auto arr = node.as_array()) {
    for (std::size_t i = 0; i < arr->size(); ++i)
      flatten_symbols(arr->at(i), out);
  }
}

std::vector<Symbol> ui_macro_symbols(const ConfigDb& db, Symbol macro_name) {
  std::vector<Symbol> out;
  const DataArray* ui = db.table(Symbol("ui"));
  if (!ui) return out;
  for (std::size_t i = 0; i < ui->size(); ++i) {
    auto entry = ui->at(i).as_array();
    if (!entry || entry->size() < 3) continue;
    if (node_text(entry->at(0)) != "#define" ||
        !symbol_eq(entry->at(1), macro_name)) {
      continue;
    }
    for (std::size_t j = 2; j < entry->size(); ++j)
      flatten_symbols(entry->at(j), out);
    break;
  }
  return out;
}

std::vector<Symbol> character_symbols(const ConfigDb& db) {
  std::vector<Symbol> chars = ui_macro_symbols(db, Symbol("CHARACTERS"));
  if (!chars.empty()) return chars;
  return {Symbol("punk"), Symbol("alterna"), Symbol("glam"), Symbol("goth"),
          Symbol("metal"), Symbol("rockabill"), Symbol("rock"),
          Symbol("deathmetal"), Symbol("classic"), Symbol("funk1"),
          Symbol("grim")};
}

std::vector<Symbol> load_character_outfits(const ConfigDb& db) {
  std::vector<Symbol> outfits = ui_macro_symbols(db, Symbol("LOAD_CHARACTERS"));
  if (!outfits.empty()) return outfits;
  return {Symbol("punk1"),      Symbol("punk2"),      Symbol("alterna1"),
          Symbol("alterna2"),   Symbol("glam1"),      Symbol("glam2"),
          Symbol("goth2"),      Symbol("goth1"),      Symbol("metal1"),
          Symbol("metal2"),     Symbol("rockabill1"), Symbol("rockabill2"),
          Symbol("rock2"),      Symbol("rock1"),      Symbol("deathmetal1"),
          Symbol("deathmetal2"), Symbol("classic"),   Symbol("funk1"),
          Symbol("grim")};
}

std::vector<Symbol> outfits_for_character(const ConfigDb& db, Symbol character) {
  const std::string base = character.c_str();
  std::vector<Symbol> out;
  for (Symbol outfit : load_character_outfits(db)) {
    const std::string name = outfit.c_str();
    if (name == base ||
        (name.rfind(base, 0) == 0 && name.size() == base.size() + 1 &&
         name.back() >= '0' && name.back() <= '9')) {
      out.push_back(outfit);
    }
  }
  if (out.empty()) out.push_back(character);
  return out;
}

Symbol character_for_outfit(const ConfigDb& db, Symbol outfit) {
  for (Symbol character : character_symbols(db)) {
    std::vector<Symbol> outfits = outfits_for_character(db, character);
    if (std::find(outfits.begin(), outfits.end(), outfit) != outfits.end())
      return character;
  }
  return outfit;
}

std::vector<Symbol> ordered_song_symbols(const ConfigDb& db, bool quickplay) {
  (void)quickplay;
  std::vector<Symbol> out;
  const DataArray* campaign = db.table(Symbol("campaign"));
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  if (order) {
    for (std::size_t i = 1; i < order->size(); ++i) {
      auto tier = order->at(i).as_array();
      if (!tier || tier->empty()) continue;
      for (std::size_t j = 1; j < tier->size(); ++j) {
        Symbol song = tier->at(j).as_symbol().value_or(Symbol());
        if (song.valid()) out.push_back(song);
      }
    }
  }
  if (!out.empty()) return out;

  for (std::size_t i = 0; i < db.song_count(); ++i) {
    Symbol song = db.song_key(i);
    if (song.valid()) out.push_back(song);
  }
  return out;
}

Symbol first_campaign_venue_symbol(const ConfigDb& db) {
  const DataArray* campaign = db.table(Symbol("campaign"));
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  if (!order || order->size() < 2) return Symbol();
  auto first = order->at(1).as_array();
  return (first && first->size() > 0) ? first->at(0).as_symbol().value_or(Symbol())
                                     : Symbol();
}

Symbol first_campaign_song_symbol(const ConfigDb& db) {
  const DataArray* campaign = db.table(Symbol("campaign"));
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  if (!order || order->size() < 2) return Symbol();
  auto first = order->at(1).as_array();
  return (first && first->size() > 1) ? first->at(1).as_symbol().value_or(Symbol())
                                     : Symbol();
}

int header_count_before_song(const ConfigDb& db, bool quickplay, int song_pos) {
  (void)quickplay;
  const DataArray* campaign = db.table(Symbol("campaign"));
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  if (!order) return 0;

  int headers = 0;
  int pos = 0;
  for (std::size_t i = 1; i < order->size(); ++i) {
    auto tier = order->at(i).as_array();
    if (!tier || tier->empty()) continue;
    ++headers;
    for (std::size_t j = 1; j < tier->size(); ++j) {
      if (pos == song_pos) return headers;
      ++pos;
    }
  }
  return headers;
}

int song_index_by_key(const ConfigDb& db, Symbol key) {
  for (std::size_t i = 0; i < db.song_count(); ++i) {
    if (db.song_key(i) == key) return static_cast<int>(i);
  }
  return -1;
}

Symbol ordered_song_at(const ConfigDb& db, int index, bool quickplay) {
  std::vector<Symbol> songs = ordered_song_symbols(db, quickplay);
  if (songs.empty()) return Symbol();
  if (index < 0) index = 0;
  if (index >= static_cast<int>(songs.size()))
    index = static_cast<int>(songs.size() - 1);
  return songs[static_cast<std::size_t>(index)];
}

int ordered_song_index_by_key(const ConfigDb& db, Symbol key, bool quickplay) {
  std::vector<Symbol> songs = ordered_song_symbols(db, quickplay);
  auto it = std::find(songs.begin(), songs.end(), key);
  return it == songs.end() ? -1 : static_cast<int>(it - songs.begin());
}

DataNode song_field_by_key(const ConfigDb& db, Symbol key, Symbol field_name) {
  const int raw_index = song_index_by_key(db, key);
  return raw_index < 0 ? DataNode()
                       : db.song_field(static_cast<std::size_t>(raw_index),
                                       field_name);
}

std::vector<Symbol> song_instruments(const ConfigDb& db, Symbol key) {
  std::vector<Symbol> out;
  const int index = song_index_by_key(db, key);
  if (index < 0) return out;
  const DataArray* record = db.song(static_cast<std::size_t>(index));
  auto song = record ? record->find_keyed(Symbol("song")) : nullptr;
  auto tracks_entry = song ? song->find_keyed(Symbol("tracks")) : nullptr;
  auto tracks = (tracks_entry && tracks_entry->size() > 1)
                    ? tracks_entry->at(1).as_array()
                    : nullptr;
  if (!tracks) return out;
  for (std::size_t i = 0; i < tracks->size(); ++i) {
    auto track = tracks->at(i).as_array();
    if (!track || track->empty()) continue;
    Symbol instrument = track->at(0).as_symbol().value_or(Symbol());
    if (instrument.valid()) out.push_back(instrument);
  }
  return out;
}

const DataArray* keyed_record(const DataArray* table, Symbol key) {
  if (!table) return nullptr;
  auto rec = table->find_keyed(key);
  return rec.get();
}

DataNode mode_field(const ConfigDb& db, Symbol mode, Symbol key) {
  const DataArray* modes = db.table(Symbol("modes"));
  DataNode value = ConfigDb::field(keyed_record(modes, mode), key);
  if (!value.empty()) return value;
  return ConfigDb::field(keyed_record(modes, Symbol("defaults")), key);
}

const DataArray* guitar_record(const ConfigDb& db, Symbol guitar) {
  return keyed_record(db.table(Symbol("guitars")), guitar);
}

std::vector<Symbol> skins_for_guitar(const ConfigDb& db, Symbol guitar) {
  std::vector<Symbol> out;
  const DataArray* rec = guitar_record(db, guitar);
  auto skins = rec ? rec->find_keyed(Symbol("skins")) : nullptr;
  if (!skins) return out;
  for (std::size_t i = 1; i < skins->size(); ++i) {
    auto skin = skins->at(i).as_array();
    if (!skin || skin->empty()) continue;
    Symbol sym = skin->at(0).as_symbol().value_or(Symbol());
    if (sym.valid()) out.push_back(sym);
  }
  return out;
}

Symbol first_guitar_symbol(const ConfigDb& db) {
  const DataArray* guitars = db.table(Symbol("guitars"));
  if (guitars) {
    for (std::size_t i = 0; i < guitars->size(); ++i) {
      auto rec = guitars->at(i).as_array();
      if (!rec || rec->empty()) continue;
      Symbol sym = rec->at(0).as_symbol().value_or(Symbol());
      if (sym.valid()) return sym;
    }
  }
  return Symbol("lespaul");
}

Symbol default_guitar_symbol(const ConfigDb& db) {
  return guitar_record(db, Symbol("lespaul")) ? Symbol("lespaul")
                                               : first_guitar_symbol(db);
}

int guitar_count(const ConfigDb& db);
Symbol guitar_for_skin(const ConfigDb& db, Symbol skin);

Symbol guitar_symbol_at(const ConfigDb& db, int index) {
  const DataArray* guitars = db.table(Symbol("guitars"));
  const int count = guitar_count(db);
  if (!guitars || count <= 0) return Symbol();
  if (index < 0) index = 0;
  if (index >= count) index = count - 1;
  auto rec = guitars->at(static_cast<std::size_t>(index)).as_array();
  return (rec && !rec->empty()) ? rec->at(0).as_symbol().value_or(Symbol())
                                : Symbol();
}

int guitar_index_by_key(const ConfigDb& db, Symbol guitar) {
  const DataArray* guitars = db.table(Symbol("guitars"));
  if (!guitars || !guitar.valid()) return -1;
  for (std::size_t i = 0; i < guitars->size(); ++i) {
    auto rec = guitars->at(i).as_array();
    if (rec && !rec->empty() &&
        rec->at(0).as_symbol().value_or(Symbol()) == guitar) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

Symbol first_skin_for_guitar(const ConfigDb& db, Symbol guitar) {
  std::vector<Symbol> skins = skins_for_guitar(db, guitar);
  return skins.empty() ? Symbol() : skins.front();
}

int skin_index_for_guitar(const ConfigDb& db, Symbol guitar, Symbol skin) {
  std::vector<Symbol> skins = skins_for_guitar(db, guitar);
  auto it = std::find(skins.begin(), skins.end(), skin);
  return it == skins.end() ? -1 : static_cast<int>(it - skins.begin());
}

void set_guitar_selection(Object& obj, const ConfigDb* db, Symbol guitar,
                          Symbol skin) {
  if (guitar.valid()) {
    obj.set_property(Symbol("guitar"), DataNode::Sym(guitar));
    if (db) {
      const int index = guitar_index_by_key(*db, guitar);
      if (index >= 0) obj.set_property(Symbol("guitar_index"), DataNode::Int(index));
      if (!skin.valid()) skin = first_skin_for_guitar(*db, guitar);
    }
  }
  if (skin.valid()) {
    obj.set_property(Symbol("guitar_skin"), DataNode::Sym(skin));
    if (db) {
      Symbol skin_guitar = guitar.valid() ? guitar : guitar_for_skin(*db, skin);
      const int index = skin_index_for_guitar(*db, skin_guitar, skin);
      if (index >= 0)
        obj.set_property(Symbol("guitar_skin_index"), DataNode::Int(index));
    }
  }
}

void set_guitar_selection_by_index(Object& obj, const ConfigDb* db, int index) {
  if (index < 0) index = 0;
  if (!db) {
    obj.set_property(Symbol("guitar_index"), DataNode::Int(index));
    return;
  }
  const int count = guitar_count(*db);
  if (count <= 0) {
    obj.set_property(Symbol("guitar_index"), DataNode::Int(0));
    return;
  }
  if (index >= count) index = count - 1;
  Symbol guitar = guitar_symbol_at(*db, index);
  obj.set_property(Symbol("guitar_index"), DataNode::Int(index));
  if (guitar.valid())
    set_guitar_selection(obj, db, guitar, first_skin_for_guitar(*db, guitar));
}

Symbol guitar_for_skin(const ConfigDb& db, Symbol skin) {
  const DataArray* guitars = db.table(Symbol("guitars"));
  if (!guitars) return default_guitar_symbol(db);
  for (std::size_t i = 0; i < guitars->size(); ++i) {
    auto rec = guitars->at(i).as_array();
    if (!rec || rec->empty()) continue;
    Symbol guitar = rec->at(0).as_symbol().value_or(Symbol());
    if (!guitar.valid()) continue;
    std::vector<Symbol> skins = skins_for_guitar(db, guitar);
    if (std::find(skins.begin(), skins.end(), skin) != skins.end())
      return guitar;
  }
  return default_guitar_symbol(db);
}

const DataArray* store_category(const ConfigDb& db, Symbol category) {
  return keyed_record(db.table(Symbol("store")), category);
}

std::vector<Symbol> store_items(const ConfigDb& db, Symbol category) {
  std::vector<Symbol> out;
  const DataArray* cat = store_category(db, category);
  if (!cat) return out;
  for (std::size_t i = 1; i < cat->size(); ++i) {
    auto item = cat->at(i).as_array();
    if (!item || item->empty()) continue;
    Symbol sym = item->at(0).as_symbol().value_or(Symbol());
    if (sym.valid()) out.push_back(sym);
  }
  return out;
}

int store_price(const ConfigDb& db, Symbol category, Symbol item) {
  const DataArray* cat = store_category(db, category);
  auto rec = cat ? cat->find_keyed(item) : nullptr;
  DataNode price = ConfigDb::field(rec.get(), Symbol("price"));
  return price.as_int().value_or(0);
}

int store_cost_bound(const ConfigDb& db, Symbol category, bool high) {
  std::vector<Symbol> items = store_items(db, category);
  if (items.empty()) return 0;
  int bound = high ? 0 : 999999;
  for (Symbol item : items) {
    const int price = store_price(db, category, item);
    bound = high ? std::max(bound, price) : std::min(bound, price);
  }
  return bound == 999999 ? 0 : bound;
}

int campaign_starting_cash(const ConfigDb& db) {
  const DataArray* campaign = db.table(Symbol("campaign"));
  auto cash = campaign ? campaign->find_keyed(Symbol("cash")) : nullptr;
  return ConfigDb::field(cash.get(), Symbol("starting")).as_int().value_or(0);
}

int campaign_star_award(const ConfigDb& db, Symbol difficulty, int stars) {
  if (!difficulty.valid() || stars < 3) return 0;
  const DataArray* campaign = db.table(Symbol("campaign"));
  auto cash = campaign ? campaign->find_keyed(Symbol("cash")) : nullptr;
  auto star_awards = cash ? cash->find_keyed(Symbol("star_awards")) : nullptr;
  auto row = star_awards ? star_awards->find_keyed(difficulty) : nullptr;
  if (!row || row->size() <= 1) return 0;
  const std::size_t slot =
      std::min<std::size_t>(row->size() - 1,
                            static_cast<std::size_t>(stars - 2));
  return row->at(slot).as_int().value_or(0);
}

int campaign_required_songs(const ConfigDb& db, Symbol difficulty,
                            bool final_venue) {
  if (!difficulty.valid()) return 0;
  const DataArray* campaign = db.table(Symbol("campaign"));
  auto required = campaign ? campaign->find_keyed(Symbol("required_songs"))
                           : nullptr;
  auto row = required ? required->find_keyed(difficulty) : nullptr;
  if (!row || row->size() <= 1) return 0;
  const std::size_t slot =
      final_venue && row->size() > 2 ? std::size_t{2} : std::size_t{1};
  return row->at(slot).as_int().value_or(0);
}

int campaign_venue_index(const ConfigDb& db, Symbol venue) {
  if (!venue.valid()) return 0;
  const DataArray* campaign = db.table(Symbol("campaign"));
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  if (!order) return 0;
  for (std::size_t i = 1; i < order->size(); ++i) {
    auto row = order->at(i).as_array();
    if (!row || row->empty()) continue;
    if (row->at(0).as_symbol().value_or(Symbol()) == venue)
      return static_cast<int>(i - 1);
  }
  return 0;
}

int macro_int(ScreenManager* mgr, Symbol name, int fallback) {
  if (!mgr) return fallback;
  DataNode value = mgr->get_global(name);
  if (auto i = value.as_int()) return *i;
  if (auto arr = value.as_array()) {
    if (!arr->empty()) return arr->at(0).as_int().value_or(fallback);
  }
  return fallback;
}

int guitar_count(const ConfigDb& db) {
  const DataArray* guitars = db.table(Symbol("guitars"));
  if (!guitars) return 0;
  int count = 0;
  for (std::size_t i = 0; i < guitars->size(); ++i) {
    auto rec = guitars->at(i).as_array();
    if (rec && rec->size() && rec->at(0).as_symbol().value_or(Symbol()).valid())
      ++count;
  }
  return count;
}

bool has_require_clause(const DataArray* record) {
  return record && record->find_keyed(Symbol("require")) != nullptr;
}

int required_song_count(const DataArray* record) {
  auto require = record ? record->find_keyed(Symbol("require")) : nullptr;
  auto songs = require ? require->find_keyed(Symbol("songs")) : nullptr;
  return (songs && songs->size() > 1)
             ? songs->at(1).as_int().value_or(std::numeric_limits<int>::max())
             : std::numeric_limits<int>::max();
}

Symbol first_required_instrument_symbol(const ConfigDb& db, Symbol type,
                                        bool prefer_lowest_song_requirement) {
  const DataArray* guitars = db.table(Symbol("guitars"));
  if (!guitars) return Symbol();
  Symbol best;
  int best_rank = std::numeric_limits<int>::max();
  for (std::size_t i = 0; i < guitars->size(); ++i) {
    auto rec = guitars->at(i).as_array();
    if (!rec || rec->empty()) continue;
    if (!symbol_eq(ConfigDb::field(rec.get(), Symbol("type")), type))
      continue;
    if (!has_require_clause(rec.get()))
      continue;
    int rank = static_cast<int>(i);
    if (prefer_lowest_song_requirement) {
      const int songs = required_song_count(rec.get());
      if (songs != std::numeric_limits<int>::max()) rank = songs;
    }
    if (rank >= best_rank)
      continue;
    Symbol sym = rec->at(0).as_symbol().value_or(Symbol());
    if (!sym.valid()) continue;
    best = sym;
    best_rank = rank;
  }
  return best;
}

class PlayerConfig : public MetaObject {
 public:
  PlayerConfig(ScreenManager* mgr, ConfigDb* db)
      : MetaObject(Symbol("PlayerCfg"), mgr, db) {}

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    const char* m = msg.c_str();
    if (std::strcmp(m, "set_character") == 0) {
      Symbol outfit = symbol_arg(args, 0);
      if (outfit.valid()) {
        set_property(Symbol("character_outfit"), DataNode::Sym(outfit));
        Symbol character = db_ ? character_for_outfit(*db_, outfit) : outfit;
        set_property(Symbol("character"), DataNode::Sym(character));
      }
      return true;
    }
  if (std::strcmp(m, "set_guitar") == 0) {
    Symbol guitar = symbol_arg(args, 0);
    Symbol skin = symbol_arg(args, 1);
    set_guitar_selection(*this, db_, guitar, skin);
    return true;
  }
  if (std::strcmp(m, "set_guitar_index") == 0) {
    set_guitar_selection_by_index(*this, db_, int_arg(args, 0, 0));
    return true;
  }
  if (std::strcmp(m, "get_num_skins") == 0) {
      Symbol guitar = args.size() ? symbol_arg(args, 0)
                                  : get_property(Symbol("guitar")).as_symbol().value_or(Symbol());
      if (!guitar.valid()) guitar = get_property(Symbol("guitar")).as_symbol().value_or(Symbol());
      out = DataNode::Int(db_ ? static_cast<int>(skins_for_guitar(*db_, guitar).size()) : 0);
      return true;
    }
    return false;
  }
};

}  // namespace

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
  if (args.size() == 0 && has_property(msg)) {
    out = get_property(msg);
    return true;
  }
  return false;
}

// --- GameConfig ------------------------------------------------------------
GameConfig::GameConfig(ScreenManager* mgr, ConfigDb* db) : MetaObject(Symbol("game"), mgr, db) {
  Symbol default_song = db ? first_campaign_song_symbol(*db) : Symbol();
  if (!default_song.valid()) default_song = Symbol("shoutatthedevil");
  const int default_song_index =
      db ? std::max(0, song_index_by_key(*db, default_song)) : 0;
  set_property(Symbol("song_index"), DataNode::Int(default_song_index));
  set_property(Symbol("song"), DataNode::Sym(default_song));
  set_property(Symbol("mode"), DataNode::Sym(Symbol("quickplay")));
  Symbol default_venue = db ? first_campaign_venue_symbol(*db) : Symbol();
  if (!default_venue.valid()) default_venue = Symbol("battle");
  set_property(Symbol("venue"), DataNode::Sym(default_venue));
  set_property(Symbol("character"), DataNode::Sym(Symbol("rockabill")));
  set_property(Symbol("character_outfit"), DataNode::Sym(Symbol("rockabill1")));
  const Symbol guitar = db ? default_guitar_symbol(*db) : Symbol("lespaul");
  Symbol skin = db ? first_skin_for_guitar(*db, guitar) : Symbol("lespaul_cherry");
  if (!skin.valid()) skin = Symbol("lespaul_cherry");
  set_guitar_selection(*this, db, guitar, skin);
  std::vector<Symbol> chars = db ? character_symbols(*db) : std::vector<Symbol>();
  std::vector<Symbol> instruments = db ? song_instruments(*db, default_song)
                                       : std::vector<Symbol>();
  for (int i = 0; i < 2; ++i) {
    auto player = std::make_unique<PlayerConfig>(mgr, db);
    Symbol character = chars.empty() ? get_property(Symbol("character")).as_symbol().value_or(Symbol("rockabill"))
                                     : chars[std::min<int>(i, static_cast<int>(chars.size() - 1))];
    std::vector<Symbol> outfits = db ? outfits_for_character(*db, character) : std::vector<Symbol>();
    Symbol outfit = outfits.empty() ? character : outfits.front();
    Symbol instrument = instruments.empty()
                            ? Symbol("guitar")
                            : instruments[std::min<int>(i, static_cast<int>(instruments.size() - 1))];
    player->set_property(Symbol("character"), DataNode::Sym(character));
    player->set_property(Symbol("character_outfit"), DataNode::Sym(outfit));
    player->set_property(Symbol("outfit_index"), DataNode::Int(0));
    set_guitar_selection(*player, db, guitar, skin);
    player->set_property(Symbol("instrument_type"), DataNode::Sym(instrument));
    player->set_property(Symbol("difficulty"), DataNode::Sym(Symbol("kDifficultyMedium")));
    player->set_property(Symbol("score"), DataNode::Int(i == 0 ? 287537 : 263125));
    player->set_property(Symbol("percent_hit"), DataNode::Int(i == 0 ? 93 : 88));
    player->set_property(Symbol("longest_streak"), DataNode::Int(i == 0 ? 312 : 244));
    player->set_property(Symbol("star_rating"), DataNode::Str(i == 0 ? "*****" : "****"));
    player->set_property(Symbol("num_stars"), DataNode::Int(i == 0 ? 5 : 4));
    player->set_property(Symbol("gems_hit"), DataNode::Int(i == 0 ? 418 : 396));
    player->set_property(Symbol("gems_passed"), DataNode::Int(450));
    player->set_property(Symbol("sp_phrases"), DataNode::Int(i == 0 ? 8 : 6));
    player->set_property(Symbol("avg_multiplier"), DataNode::Float(i == 0 ? 3.6f : 3.1f));
    player->set_property(Symbol("streaks_broken"), DataNode::Int(i == 0 ? 5 : 7));
    player->set_property(Symbol("lead_time_sec"), DataNode::Int(i == 0 ? 112 : 94));
    players_.push_back(std::move(player));
  }
}

bool GameConfig::handle_meta(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();

  auto select_song_index = [&](int index, bool quickplay) {
    if (index < 0) index = 0;
    set_property(Symbol("song_index"), DataNode::Int(index));
    if (db_) {
      Symbol song = ordered_song_at(*db_, index, quickplay);
      if (song.valid()) set_property(Symbol("song"), DataNode::Sym(song));
    }
  };

  if (std::strcmp(m, "get_player_config") == 0) {
    int i = args.size() ? args.at(0).as_int().value_or(0) : 0;
    if (i < 0 || i >= static_cast<int>(players_.size())) i = 0;
    out = DataNode::Obj(players_[i].get());
    return true;
  }

  if (std::strcmp(m, "set") == 0) {
    Symbol key = symbol_arg(args, 0);
    if (key.valid()) set_property(key, args.size() > 1 ? args.at(1) : DataNode());
    return true;
  }

  if (std::strcmp(m, "get") == 0) {
    Symbol key = symbol_arg(args, 0);
    if (!key.valid()) return true;
    DataNode value = get_property(key);
    if (value.empty() && db_) {
      Symbol mode = get_property(Symbol("mode")).as_symbol().value_or(Symbol("quickplay"));
      value = mode_field(*db_, mode, key);
    }
    out = value;
    return true;
  }

  if (std::strcmp(m, "set_character") == 0) {
    Symbol outfit = symbol_arg(args, 0);
    if (outfit.valid()) {
      set_property(Symbol("character_outfit"), DataNode::Sym(outfit));
      if (db_) set_property(Symbol("character"),
                            DataNode::Sym(character_for_outfit(*db_, outfit)));
    }
    return true;
  }
  if (std::strcmp(m, "set_song_index") == 0) {
    const Symbol mode =
        get_property(Symbol("mode")).as_symbol().value_or(Symbol("quickplay"));
    select_song_index(int_arg(args, 0, 0), mode != Symbol("career"));
    return true;
  }
  if (std::strcmp(m, "set_career_song") == 0) {
    set_property(Symbol("mode"), DataNode::Sym(Symbol("career")));
    select_song_index(get_property(Symbol("song_index")).as_int().value_or(0),
                      false);
    return true;
  }
  if (std::strcmp(m, "set_quickplay") == 0) {
    set_property(Symbol("mode"), DataNode::Sym(Symbol("quickplay")));
    select_song_index(get_property(Symbol("song_index")).as_int().value_or(0),
                      true);
    return true;
  }
  if (std::strcmp(m, "set_song") == 0) {
    Symbol song = symbol_arg(args, 0);
    if (song.valid()) {
      set_property(Symbol("song"), DataNode::Sym(song));
      if (db_) {
        const Symbol mode = get_property(Symbol("mode"))
                                .as_symbol()
                                .value_or(Symbol("quickplay"));
        int index = ordered_song_index_by_key(*db_, song,
                                              mode != Symbol("career"));
        if (index < 0) index = song_index_by_key(*db_, song);
        if (index >= 0) set_property(Symbol("song_index"), DataNode::Int(index));
      }
    }
    return true;
  }
  if (std::strcmp(m, "set_guitar") == 0) {
    Symbol guitar = symbol_arg(args, 0);
    Symbol skin = symbol_arg(args, 1);
    set_guitar_selection(*this, db_, guitar, skin);
    return true;
  }
  if (std::strcmp(m, "set_guitar_index") == 0) {
    set_guitar_selection_by_index(*this, db_, int_arg(args, 0, 0));
    return true;
  }
  if (std::strcmp(m, "get_guitar_desc") == 0) {
    const std::string guitar = symbol_arg(args, 0).c_str();
    out = guitar.empty() ? DataNode() : DataNode::Sym(Symbol((guitar + "_desc").c_str()));
    return true;
  }
  if (std::strcmp(m, "get_guitar_skin_desc") == 0) {
    const std::string skin = symbol_arg(args, 0).c_str();
    out = skin.empty() ? DataNode() : DataNode::Sym(Symbol((skin + "_desc").c_str()));
    return true;
  }
  if (std::strcmp(m, "get_total_num_skins") == 0 ||
      std::strcmp(m, "get_num_skins") == 0) {
    Symbol guitar = args.size() > 1 ? symbol_arg(args, 1)
                                    : get_property(Symbol("guitar")).as_symbol().value_or(Symbol());
    if (!guitar.valid()) guitar = get_property(Symbol("guitar")).as_symbol().value_or(Symbol());
    out = DataNode::Int(db_ ? static_cast<int>(skins_for_guitar(*db_, guitar).size()) : 0);
    return true;
  }
  if (std::strcmp(m, "get_num_guitars") == 0) {
    out = DataNode::Int(db_ ? guitar_count(*db_) : 0);
    return true;
  }
  if (std::strcmp(m, "practice_section_provider") == 0) {
    out = mgr_ ? DataNode::Obj(mgr_->resolve_object(Symbol("section_provider")))
               : DataNode();
    return true;
  }
  if (std::strcmp(m, "get_section_bounds") == 0) {
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

  // Data-backed song lookups (song_index is a stock setlist position; song is
  // kept in sync through campaign.dtb order rather than raw songs.dtb order).
  std::size_t si = static_cast<std::size_t>(
      std::max(0, get_property(Symbol("song_index")).as_int().value_or(0)));
  Symbol current_song = get_property(Symbol("song")).as_symbol().value_or(Symbol());
  if (!current_song.valid() && db_) {
    const Symbol mode =
        get_property(Symbol("mode")).as_symbol().value_or(Symbol("quickplay"));
    current_song = ordered_song_at(*db_, static_cast<int>(si),
                                   mode != Symbol("career"));
  }
  if (std::strcmp(m, "get_song_text") == 0) {
    out = db_ ? song_field_by_key(*db_, current_song, Symbol("name"))
              : DataNode();
    return true;
  }
  if (std::strcmp(m, "get_song_artist_text") == 0) {
    out = db_ ? song_field_by_key(*db_, current_song, Symbol("artist"))
              : DataNode();
    return true;
  }
  if (std::strcmp(m, "get_song_caption") == 0) {
    out = db_ ? song_field_by_key(*db_, current_song, Symbol("name"))
              : DataNode();
    return true;
  }
  if (std::strcmp(m, "get_venue") == 0) {
    out = get_property(Symbol("venue"));
    return true;
  }
  if (std::strcmp(m, "get_venue_index") == 0) {
    Symbol venue = get_property(Symbol("venue")).as_symbol().value_or(Symbol());
    out = DataNode::Int(db_ ? campaign_venue_index(*db_, venue) : 0);
    return true;
  }
  if (std::strcmp(m, "song_duration_sec") == 0) {
    out = DataNode::Int(db_ ? db_->song_duration_sec(current_song) : 0);
    return true;
  }
  if (std::strcmp(m, "get_num_players") == 0 || std::strcmp(m, "num_players") == 0) {
    if (db_) {
      Symbol mode = get_property(Symbol("mode")).as_symbol().value_or(Symbol("quickplay"));
      out = mode_field(*db_, mode, Symbol("num_players"));
    }
    if (out.empty()) out = DataNode::Int(1);
    return true;
  }
  if (std::strcmp(m, "get_difficulty") == 0 ||
      std::strcmp(m, "get_difficulty_sym") == 0) {
    int i = int_arg(args, 0, 0);
    if (i < 0 || i >= static_cast<int>(players_.size())) i = 0;
    Object* player = players_[i].get();
    Symbol difficulty =
        player ? player->get_property(Symbol("difficulty")).as_symbol().value_or(
                     Symbol("kDifficultyMedium"))
               : Symbol("kDifficultyMedium");
    if (std::strcmp(m, "get_difficulty_sym") == 0)
      difficulty = difficulty_locale_symbol(difficulty);
    out = DataNode::Sym(difficulty);
    return true;
  }
  return false;
}

// --- Campaign --------------------------------------------------------------
Campaign::Campaign(ScreenManager* mgr, ConfigDb* db) : MetaObject(Symbol("campaign"), mgr, db) {
  const Symbol next_guitar_award =
      db ? first_required_instrument_symbol(*db, Symbol("guitar"),
                                            false)
         : Symbol();
  const int starting_cash = db ? campaign_starting_cash(*db) : 0;
  profiles_.resize(static_cast<std::size_t>(
      std::max(1, macro_int(mgr, Symbol("MAX_NUM_PROFILES"), 8))));
  set_property(Symbol("cash"), DataNode::Int(starting_cash));
  set_property(Symbol("starting_cash"), DataNode::Int(starting_cash));
  set_property(Symbol("career_score"), DataNode::Int(287537));
  set_property(Symbol("num_guitar_awards"), DataNode::Int(1));
  set_property(Symbol("next_guitar_award"), DataNode::Sym(next_guitar_award));
  set_property(Symbol("status"), DataNode::Int(1));
  set_property(Symbol("profile_slot"), DataNode::Int(0));
}

bool Campaign::handle_meta(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();
  if (std::strcmp(m, "num_profiles") == 0) {
    int count = 0;
    for (const Profile& profile : profiles_) {
      if (!profile.name.empty()) ++count;
    }
    out = DataNode::Int(count);
    return true;
  }
  if (std::strcmp(m, "cash") == 0) {
    out = get_property(Symbol("cash"));
    return true;
  }
  if (std::strcmp(m, "next_guitar_award") == 0) {
    const bool bass_unlock =
        mgr_ && mgr_->current_screen() &&
        mgr_->current_screen()->name() == Symbol("multi_unlock_bass_screen");
    Symbol award = db_
                       ? first_required_instrument_symbol(
                             *db_, bass_unlock ? Symbol("bass")
                                               : Symbol("guitar"),
                             bass_unlock)
                       : Symbol();
    out = award.valid() ? DataNode::Sym(award) : get_property(msg);
    return true;
  }
  if (std::strcmp(m, "starting_cash") == 0 ||
      std::strcmp(m, "career_score") == 0 ||
      std::strcmp(m, "num_guitar_awards") == 0 ||
      std::strcmp(m, "status") == 0 ||
      std::strcmp(m, "profile_slot") == 0) {
    out = get_property(msg);
    return true;
  }
  if (std::strcmp(m, "profile_name") == 0) {
    int slot = args.size() ? int_arg(args, 0, 0)
                           : get_property(Symbol("profile_slot")).as_int().value_or(0);
    if (slot < 0 || slot >= static_cast<int>(profiles_.size())) {
      out = DataNode::Str("");
    } else {
      out = DataNode::Str(profiles_[static_cast<std::size_t>(slot)].name);
    }
    return true;
  }
  if (std::strcmp(m, "is_empty_profile") == 0) {
    int slot = int_arg(args, 0, get_property(Symbol("profile_slot")).as_int().value_or(0));
    const bool empty = slot < 0 || slot >= static_cast<int>(profiles_.size()) ||
                       profiles_[static_cast<std::size_t>(slot)].name.empty();
    out = DataNode::Sym(Symbol(empty ? "TRUE" : "FALSE"));
    return true;
  }
  if (std::strcmp(m, "empty_slot") == 0) {
    int slot = 0;
    for (; slot < static_cast<int>(profiles_.size()); ++slot) {
      if (profiles_[static_cast<std::size_t>(slot)].name.empty()) break;
    }
    if (slot >= static_cast<int>(profiles_.size())) slot = 0;
    out = DataNode::Int(slot);
    return true;
  }
  if (std::strcmp(m, "set_profile_slot") == 0) {
    int slot = int_arg(args, 0, 0);
    if (slot < 0) slot = 0;
    if (slot >= static_cast<int>(profiles_.size()))
      slot = static_cast<int>(profiles_.empty() ? 0 : profiles_.size() - 1);
    set_property(Symbol("profile_slot"), DataNode::Int(slot));
    return true;
  }
  if (std::strcmp(m, "has_profile_name") == 0) {
    std::string name = node_text(args.size() ? args.at(0) : DataNode());
    const bool editing = args.size() > 1 && node_truthy(args.at(1));
    const int current_slot = get_property(Symbol("profile_slot")).as_int().value_or(-1);
    bool found = false;
    for (std::size_t i = 0; i < profiles_.size(); ++i) {
      if (editing && static_cast<int>(i) == current_slot) continue;
      if (!name.empty() && profiles_[i].name == name) {
        found = true;
        break;
      }
    }
    out = DataNode::Sym(Symbol(found ? "TRUE" : "FALSE"));
    return true;
  }
  if (std::strcmp(m, "set_profile_name") == 0) {
    std::string name = node_text(args.size() ? args.at(0) : DataNode());
    int slot = args.size() > 1 ? int_arg(args, 1, 0)
                               : get_property(Symbol("profile_slot")).as_int().value_or(0);
    if (slot >= 0 && slot < static_cast<int>(profiles_.size())) {
      profiles_[static_cast<std::size_t>(slot)].name = std::move(name);
      set_property(Symbol("profile_slot"), DataNode::Int(slot));
    }
    return true;
  }
  if (std::strcmp(m, "is_max_status") == 0 ||
      std::strcmp(m, "encore_newly_unlocked") == 0) {
    out = DataNode::Sym(Symbol("FALSE"));
    return true;
  }
  if (std::strcmp(m, "get_status_progress") == 0) {
    Symbol difficulty = symbol_arg(args, 0);
    out = DataNode::Int(db_ ? campaign_required_songs(*db_, difficulty, false)
                            : 0);
    return true;
  }
  if (std::strcmp(m, "add_cash") == 0) {
    int cash = get_property(Symbol("cash")).as_int().value_or(0);
    cash += int_arg(args, 0, 0);
    set_property(Symbol("cash"), DataNode::Int(cash));
    out = DataNode::Int(cash);
    return true;
  }
  if (std::strcmp(m, "is_unlocked") == 0) {
    out = DataNode::Sym(Symbol("FALSE"));
    return true;
  }
  if (std::strcmp(m, "finish_song") == 0) {
    const int score = int_arg(args, 0, 287537);
    Object* player = mgr_ ? mgr_->resolve_object(Symbol("player0")) : nullptr;
    Symbol difficulty =
        player ? player->get_property(Symbol("difficulty"))
                     .as_symbol()
                     .value_or(Symbol("kDifficultyMedium"))
               : Symbol("kDifficultyMedium");
    const int stars = player ? player->get_property(Symbol("num_stars"))
                                      .as_int()
                                      .value_or(0)
                             : 0;
    const int cash_award =
        db_ ? campaign_star_award(*db_, difficulty, stars) : 0;
    out = array_node({DataNode::Int(score), DataNode::Int(cash_award),
                      DataNode::Sym(Symbol("ca_reason")),
                      DataNode::Str(""), DataNode::Int(0), DataNode::Int(0)});
    return true;
  }
  if (std::strcmp(m, "finish_coop_song") == 0 ||
      std::strcmp(m, "beat_song") == 0 ||
      std::strcmp(m, "cheat_beat_song") == 0) {
    out = DataNode::Sym(Symbol("FALSE"));
    return true;
  }
  if (std::strcmp(m, "new_campaign") == 0 ||
      std::strcmp(m, "set_character_info") == 0) {
    out = DataNode::Sym(Symbol("FALSE"));
    return true;
  }
  return false;
}

namespace {

class SongProvider : public MetaObject {
 public:
  SongProvider(ScreenManager* mgr, ConfigDb* db)
      : MetaObject(Symbol("song_provider"), mgr, db) {
    set_property(Symbol("quickplay"), DataNode::Sym(Symbol("FALSE")));
  }

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    const char* m = msg.c_str();
    const bool quickplay = node_truthy(get_property(Symbol("quickplay")));

    if (std::strcmp(m, "set_quickplay") == 0) {
      const bool enabled = args.size() == 0 ? true : node_truthy(args.at(0));
      set_property(Symbol("quickplay"),
                   DataNode::Sym(Symbol(enabled ? "TRUE" : "FALSE")));
      return true;
    }
    if (std::strcmp(m, "get_quickplay") == 0) {
      out = get_property(Symbol("quickplay"));
      return true;
    }
    if (std::strcmp(m, "list_length") == 0 ||
        std::strcmp(m, "num_data") == 0) {
      out = DataNode::Int(static_cast<int>(
          db_ ? ordered_song_symbols(*db_, quickplay).size() : 0));
      return true;
    }
    if (std::strcmp(m, "num_headers") == 0) {
      out = DataNode::Int(
          db_ ? header_count_before_song(*db_, quickplay, int_arg(args, 0))
              : 0);
      return true;
    }
    if (std::strcmp(m, "get_symbol") == 0) {
      if (!db_) return true;
      std::vector<Symbol> songs = ordered_song_symbols(*db_, quickplay);
      int index = int_arg(args, 0);
      if (index < 0) index = 0;
      if (index >= static_cast<int>(songs.size()))
        index = songs.empty() ? 0 : static_cast<int>(songs.size() - 1);
      out = songs.empty() ? DataNode() : DataNode::Sym(songs[index]);
      return true;
    }
    if (std::strcmp(m, "has_instrument") == 0) {
      if (!db_) {
        out = DataNode::Sym(Symbol("FALSE"));
        return true;
      }
      Symbol song = symbol_arg(args, 0);
      Symbol instrument = symbol_arg(args, 1);
      std::vector<Symbol> instruments = song_instruments(*db_, song);
      const bool found =
          std::find(instruments.begin(), instruments.end(), instrument) !=
          instruments.end();
      out = DataNode::Sym(Symbol(found ? "TRUE" : "FALSE"));
      return true;
    }
    if (std::strcmp(m, "get_instrument") == 0) {
      if (!db_) return true;
      Symbol song = symbol_arg(args, 0);
      int player = int_arg(args, 1);
      std::vector<Symbol> instruments = song_instruments(*db_, song);
      if (instruments.empty()) return true;
      if (player < 0) player = 0;
      if (player >= static_cast<int>(instruments.size()))
        player = static_cast<int>(instruments.size() - 1);
      out = DataNode::Sym(instruments[player]);
      return true;
    }
    return false;
  }
};

class CharacterProvider : public MetaObject {
 public:
  CharacterProvider(ScreenManager* mgr, ConfigDb* db)
      : MetaObject(Symbol("character_provider"), mgr, db) {}

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    if (!db_) return false;
    const char* m = msg.c_str();
    std::vector<Symbol> chars = character_symbols(*db_);
    if (std::strcmp(m, "list_length") == 0 ||
        std::strcmp(m, "num_data") == 0) {
      out = DataNode::Int(static_cast<int>(chars.size()));
      return true;
    }
    if (std::strcmp(m, "get_index") == 0) {
      Symbol character = symbol_arg(args, 0);
      auto it = std::find(chars.begin(), chars.end(), character);
      out = DataNode::Int(it == chars.end() ? 0 : static_cast<int>(it - chars.begin()));
      return true;
    }
    if (std::strcmp(m, "get_symbol") == 0 ||
        std::strcmp(m, "get_text") == 0) {
      int index = int_arg(args, 0);
      if (index < 0) index = 0;
      if (index >= static_cast<int>(chars.size()))
        index = chars.empty() ? 0 : static_cast<int>(chars.size() - 1);
      out = chars.empty() ? DataNode() : DataNode::Sym(chars[index]);
      return true;
    }
    if (std::strcmp(m, "is_active") == 0) {
      out = DataNode::Sym(Symbol("TRUE"));
      return true;
    }
    if (std::strcmp(m, "num_outfits") == 0) {
      out = DataNode::Int(static_cast<int>(
          outfits_for_character(*db_, symbol_arg(args, 0)).size()));
      return true;
    }
    if (std::strcmp(m, "get_outfit") == 0) {
      std::vector<Symbol> outfits =
          outfits_for_character(*db_, symbol_arg(args, 0));
      int index = int_arg(args, 1);
      if (index < 0) index = 0;
      if (index >= static_cast<int>(outfits.size()))
        index = outfits.empty() ? 0 : static_cast<int>(outfits.size() - 1);
      out = outfits.empty() ? DataNode() : DataNode::Sym(outfits[index]);
      return true;
    }
    return false;
  }
};

class PracticeSectionProvider : public MetaObject {
 public:
  PracticeSectionProvider(ScreenManager* mgr, ConfigDb* db)
      : MetaObject(Symbol("section_provider"), mgr, db) {}

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    if (!db_) return false;
    const char* m = msg.c_str();
    const auto& sections = db_->practice_sections();
    if (std::strcmp(m, "list_length") == 0 ||
        std::strcmp(m, "num_data") == 0) {
      out = DataNode::Int(static_cast<int>(sections.size()));
      return true;
    }
    if (std::strcmp(m, "get_symbol") == 0 ||
        std::strcmp(m, "get_text") == 0) {
      int index = int_arg(args, 0);
      if (index < 0) index = 0;
      if (index >= static_cast<int>(sections.size()))
        index = sections.empty() ? 0 : static_cast<int>(sections.size() - 1);
      out = sections.empty() ? DataNode() : DataNode::Sym(sections[index]);
      return true;
    }
    if (std::strcmp(m, "set_start_section") == 0) {
      set_property(Symbol("start_section"), args.size() ? args.at(0) : DataNode());
      return true;
    }
    if (std::strcmp(m, "section_after_last") == 0) {
      out = DataNode::Sym(Symbol("FALSE"));
      return true;
    }
    return false;
  }
};

class StoreProvider : public MetaObject {
 public:
  StoreProvider(Symbol name, ScreenManager* mgr, ConfigDb* db, Symbol category,
                bool dynamic_category = false)
      : MetaObject(name, mgr, db),
        default_category_(category),
        dynamic_category_(dynamic_category) {
    set_property(Symbol("category"), DataNode::Sym(category));
  }

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    if (!db_) return false;
    const char* m = msg.c_str();
    if (std::strcmp(m, "init_data") == 0) return true;
    if (std::strcmp(m, "set_category") == 0) {
      Symbol category = symbol_arg(args, 0);
      if (category.valid()) set_property(Symbol("category"), DataNode::Sym(category));
      return true;
    }

    Symbol cat = category();
    std::vector<Symbol> items = store_items(*db_, cat);
    if (std::strcmp(m, "list_length") == 0 ||
        std::strcmp(m, "num_data") == 0) {
      out = DataNode::Int(static_cast<int>(items.size()));
      return true;
    }
    if (std::strcmp(m, "get_symbol") == 0 ||
        std::strcmp(m, "get_text") == 0) {
      int index = int_arg(args, 0);
      if (index < 0) index = 0;
      if (index >= static_cast<int>(items.size()))
        index = items.empty() ? 0 : static_cast<int>(items.size() - 1);
      out = items.empty() ? DataNode() : DataNode::Sym(items[index]);
      return true;
    }
    if (std::strcmp(m, "in_stock") == 0) {
      out = DataNode::Sym(Symbol("TRUE"));
      return true;
    }
    if (std::strcmp(m, "price") == 0) {
      Symbol price_cat = args.size() > 1 ? symbol_arg(args, 0) : cat;
      Symbol item = args.size() > 1 ? symbol_arg(args, 1) : symbol_arg(args, 0);
      out = DataNode::Int(store_price(*db_, price_cat, item));
      return true;
    }
    if (std::strcmp(m, "low_cost") == 0 ||
        std::strcmp(m, "high_cost") == 0) {
      Symbol price_cat = args.size() ? symbol_arg(args, 0) : cat;
      out = DataNode::Int(store_cost_bound(
          *db_, price_cat, std::strcmp(m, "high_cost") == 0));
      return true;
    }
    if (std::strcmp(m, "get_guitar") == 0) {
      int index = int_arg(args, 0);
      if (index < 0) index = 0;
      if (index >= static_cast<int>(items.size()))
        index = items.empty() ? 0 : static_cast<int>(items.size() - 1);
      out = items.empty() ? DataNode()
                          : DataNode::Sym(guitar_for_skin(*db_, items[index]));
      return true;
    }
    return false;
  }

 private:
  Symbol category() const {
    if (dynamic_category_) {
      Symbol cat = get_property(Symbol("category")).as_symbol().value_or(Symbol());
      if (cat.valid()) return cat;
    }
    return default_category_;
  }

  Symbol default_category_;
  bool dynamic_category_ = false;
};

class BandStats : public MetaObject {
 public:
  BandStats(ScreenManager* mgr, ConfigDb* db)
      : MetaObject(Symbol("band"), mgr, db) {
    set_property(Symbol("score"), DataNode::Int(550662));
    set_property(Symbol("longest_streak"), DataNode::Int(431));
    set_property(Symbol("star_rating"), DataNode::Str("*****"));
  }
};

class Highscores : public MetaObject {
 public:
  Highscores(ScreenManager* mgr, ConfigDb* db)
      : MetaObject(Symbol("highscores"), mgr, db) {
    set_property(Symbol("default_name"), DataNode::Str("AAAA"));
  }

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    const char* m = msg.c_str();
    if (std::strcmp(m, "check_highscore") == 0) {
      const int score = int_arg(args, 0, 0);
      out = DataNode::Int(highscore_index(score));
      return true;
    }
    if (std::strcmp(m, "get_default_name") == 0) {
      out = get_property(Symbol("default_name"));
      return true;
    }
    if (std::strcmp(m, "set_default_name") == 0) {
      set_property(Symbol("default_name"), args.size() ? args.at(0) : DataNode::Str("AAAA"));
      return true;
    }
    if (std::strcmp(m, "add") == 0) {
      add_entry(node_text(args.size() > 0 ? args.at(0) : get_property(Symbol("default_name"))),
                int_arg(args, 1, 0));
      return true;
    }
    if (std::strcmp(m, "get_highscore") == 0) {
      int slot = int_arg(args, 0, 0);
      if (slot < 0) slot = 0;
      if (slot > 5) slot = 5;
      if (slot < static_cast<int>(entries_.size())) {
        const Entry& e = entries_[static_cast<std::size_t>(slot)];
        out = array_node({DataNode::Int(slot), DataNode::Str(e.name),
                          DataNode::Int(e.score)});
      } else {
        out = array_node({DataNode::Int(slot), DataNode::Str(""),
                          DataNode::Int(0)});
      }
      return true;
    }
    return false;
  }

 private:
  struct Entry {
    std::string name;
    int score = 0;
  };

  int highscore_index(int score) const {
    if (score <= 0) return -1;
    for (std::size_t i = 0; i < entries_.size(); ++i) {
      if (score > entries_[i].score) return static_cast<int>(i);
    }
    return entries_.size() < 6 ? static_cast<int>(entries_.size()) : -1;
  }

  void add_entry(std::string name, int score) {
    if (score <= 0) return;
    if (name.empty()) name = node_text(get_property(Symbol("default_name")));
    const int index = highscore_index(score);
    if (index < 0) return;
    entries_.insert(entries_.begin() + index, Entry{std::move(name), score});
    if (entries_.size() > 6) entries_.resize(6);
  }

  std::vector<Entry> entries_;
};

class StatsProvider : public MetaObject {
 public:
  StatsProvider(Symbol cls, ScreenManager* mgr, ConfigDb* db)
      : MetaObject(cls, mgr, db) {}

 protected:
  struct PlayerNotes {
    int hit = 0;
    int total = 0;
  };

  struct Row {
    Symbol section;
    PlayerNotes p0;
    PlayerNotes p1;
  };

  static std::string notes_text(const PlayerNotes& notes) {
    return std::to_string(notes.hit) + "/" + std::to_string(notes.total);
  }

  PlayerNotes player_notes(Symbol player_name) const {
    PlayerNotes out;
    Object* player = mgr_ ? mgr_->resolve_object(player_name) : nullptr;
    if (!player) return out;
    out.hit = std::max(0, player->handle_property(Symbol("gems_hit"), DataArray())
                              .as_int()
                              .value_or(0));
    const int passed =
        std::max(0, player->handle_property(Symbol("gems_passed"), DataArray())
                        .as_int()
                        .value_or(0));
    out.total = std::max(out.hit, out.hit + passed);
    return out;
  }

  static PlayerNotes slice_notes(PlayerNotes total, int index, int count) {
    if (count <= 0 || total.total <= 0) return {};
    const int row_total = total.total / count +
                          (index < (total.total % count) ? 1 : 0);
    const double ratio =
        total.total > 0 ? static_cast<double>(total.hit) /
                              static_cast<double>(total.total)
                        : 0.0;
    const int row_hit = std::clamp(
        static_cast<int>(std::lround(static_cast<double>(row_total) * ratio)),
        0, row_total);
    return {row_hit, row_total};
  }

  std::vector<Row> rows() const {
    std::vector<Symbol> sections =
        db_ ? db_->practice_sections() : std::vector<Symbol>();
    if (!sections.empty() && sections.front() == Symbol("full_song") &&
        sections.size() > 1) {
      sections.erase(sections.begin());
    }
    const int count = static_cast<int>(sections.size());
    std::vector<Row> out;
    out.reserve(sections.size());
    const PlayerNotes p0_total = player_notes(Symbol("player0"));
    const PlayerNotes p1_total = player_notes(Symbol("player1"));
    for (int i = 0; i < count; ++i) {
      out.push_back({sections[static_cast<std::size_t>(i)],
                     slice_notes(p0_total, i, count),
                     slice_notes(p1_total, i, count)});
    }
    return out;
  }

  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    const char* m = msg.c_str();
    std::vector<Row> data = rows();
    if (std::strcmp(m, "list_length") == 0 ||
        std::strcmp(m, "num_data") == 0) {
      out = DataNode::Int(static_cast<int>(data.size()));
      return true;
    }
    int index = int_arg(args, 0);
    if (index < 0) index = 0;
    if (index >= static_cast<int>(data.size()))
      index = data.empty() ? 0 : static_cast<int>(data.size() - 1);
    if (data.empty()) return true;

    const Row& row = data[static_cast<std::size_t>(index)];
    if (std::strcmp(m, "get_symbol") == 0 ||
        std::strcmp(m, "get_section") == 0) {
      out = DataNode::Sym(row.section);
      return true;
    }
    if (std::strcmp(m, "get_notes1") == 0) {
      out = DataNode::Str(notes_text(row.p0));
      return true;
    }
    if (std::strcmp(m, "get_notes2") == 0) {
      out = DataNode::Str(notes_text(row.p1));
      return true;
    }
    if (std::strcmp(m, "get_text") == 0) {
      const int column = int_arg(args, 1, 0);
      if (column == 0) out = DataNode::Sym(row.section);
      else if (column == 1) out = DataNode::Str(notes_text(row.p0));
      else if (column == 4) out = DataNode::Str(notes_text(row.p1));
      else out = DataNode::Str("");
      return true;
    }
    return false;
  }
};

}  // namespace

// --- registration ----------------------------------------------------------
void install_meta_singletons(ScreenManager& mgr, ConfigDb& db) {
  mgr.register_runtime_class(
      Symbol("StatsProvider"),
      [&mgr, &db](Symbol) -> std::unique_ptr<Object> {
        return std::make_unique<StatsProvider>(Symbol("StatsProvider"), &mgr,
                                               &db);
      });

  if (Object* credits_screen = mgr.find_object(Symbol("credits_screen"))) {
    const DataArray* credits = db.table(Symbol("credits"));
    if (credits) {
      credits_screen->set_property(
          Symbol("num_lines"),
          DataNode::Int(static_cast<int32_t>(credits->size())));
    }
  }

  auto game = std::make_unique<GameConfig>(&mgr, &db);
  GameConfig* g = game.get();
  mgr.add_singleton(Symbol("game"), std::move(game));
  mgr.alias_singleton(Symbol("gamecfg"), g);
  if (g->player(0)) mgr.alias_singleton(Symbol("player0"), g->player(0));
  if (g->player(1)) mgr.alias_singleton(Symbol("player1"), g->player(1));

  mgr.add_singleton(Symbol("campaign"), std::make_unique<Campaign>(&mgr, &db));
  mgr.add_singleton(Symbol("band"), std::make_unique<BandStats>(&mgr, &db));
  mgr.add_singleton(Symbol("highscores"), std::make_unique<Highscores>(&mgr, &db));
  mgr.add_singleton(Symbol("song_provider"),
                    std::make_unique<SongProvider>(&mgr, &db));
  mgr.add_singleton(Symbol("character_provider"),
                    std::make_unique<CharacterProvider>(&mgr, &db));
  mgr.add_singleton(Symbol("section_provider"),
                    std::make_unique<PracticeSectionProvider>(&mgr, &db));
  mgr.add_singleton(Symbol("store_item_provider"),
                    std::make_unique<StoreProvider>(
                        Symbol("store_item_provider"), &mgr, &db,
                        Symbol("character"), true));
  mgr.add_singleton(Symbol("store_guitar_provider"),
                    std::make_unique<StoreProvider>(
                        Symbol("store_guitar_provider"), &mgr, &db,
                        Symbol("guitar")));
  mgr.add_singleton(Symbol("store_skin_provider"),
                    std::make_unique<StoreProvider>(
                        Symbol("store_skin_provider"), &mgr, &db,
                        Symbol("skin")));
  mgr.add_singleton(Symbol("store_video_provider"),
                    std::make_unique<StoreProvider>(
                        Symbol("store_video_provider"), &mgr, &db,
                        Symbol("video")));
}

}  // namespace ghogx::ui
