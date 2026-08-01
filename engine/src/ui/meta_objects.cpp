// engine/src/ui/meta_objects.cpp -- see meta_objects.h.

#include "ui/meta_objects.h"

#include "ui/config_db.h"
#include "ui/screen_manager.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace ghogx::ui {

namespace {
namespace fs = std::filesystem;

struct PersistentProfileState {
  int cash = 0;
  std::string guitar;
  std::string guitar_skin;
  std::array<std::string, 2> player_guitar;
  std::array<std::string, 2> player_guitar_skin;
  std::set<std::string> unlocked;
};

bool profile_persistence_enabled() {
  const char* disabled = std::getenv("GHOGX_DISABLE_PROFILE_PERSISTENCE");
  return !disabled || std::strcmp(disabled, "0") == 0 ||
         std::strcmp(disabled, "FALSE") == 0 ||
         std::strcmp(disabled, "false") == 0;
}

fs::path persistent_profile_path() {
  if (const char* override_path = std::getenv("GHOGX_PROFILE_PATH");
      override_path && *override_path)
    return fs::path(override_path);
  return fs::path("save") / "ghogx_profile_v1.txt";
}

PersistentProfileState load_persistent_profile_state() {
  PersistentProfileState state;
  if (!profile_persistence_enabled()) return state;
  const fs::path path = persistent_profile_path();
  std::ifstream stream(path);
  std::string line;
  if (!stream || !std::getline(stream, line) ||
      line != "GHOGX_PROFILE_V1")
    return state;
  while (std::getline(stream, line)) {
    const std::size_t split = line.find('=');
    if (split == std::string::npos) continue;
    const std::string key = line.substr(0, split);
    const std::string value = line.substr(split + 1);
    if (key == "cash") {
      try {
        state.cash = std::max(0, std::stoi(value));
      } catch (...) {
        state.cash = 0;
      }
    } else if (key == "guitar") {
      state.guitar = value;
    } else if (key == "guitar_skin") {
      state.guitar_skin = value;
    } else if (key == "player0_guitar") {
      state.player_guitar[0] = value;
    } else if (key == "player0_guitar_skin") {
      state.player_guitar_skin[0] = value;
    } else if (key == "player1_guitar") {
      state.player_guitar[1] = value;
    } else if (key == "player1_guitar_skin") {
      state.player_guitar_skin[1] = value;
    } else if (key == "unlock" && !value.empty()) {
      state.unlocked.insert(value);
    }
  }
  std::fprintf(stderr,
               "[profile] loaded path=%s cash=%d guitar=%s skin=%s "
               "unlocks=%zu\n",
               path.string().c_str(), state.cash, state.guitar.c_str(),
               state.guitar_skin.c_str(), state.unlocked.size());
  return state;
}

PersistentProfileState& persistent_profile_state() {
  static PersistentProfileState state = load_persistent_profile_state();
  return state;
}

bool save_persistent_profile_state() {
  if (!profile_persistence_enabled()) return true;
  const fs::path path = persistent_profile_path();
  const fs::path temp = path.string() + ".tmp";
  std::error_code error;
  if (!path.parent_path().empty())
    fs::create_directories(path.parent_path(), error);
  if (error) return false;
  const PersistentProfileState& state = persistent_profile_state();
  {
    std::ofstream stream(temp, std::ios::trunc);
    if (!stream) return false;
    stream << "GHOGX_PROFILE_V1\n";
    stream << "cash=" << std::max(0, state.cash) << "\n";
    stream << "guitar=" << state.guitar << "\n";
    stream << "guitar_skin=" << state.guitar_skin << "\n";
    for (std::size_t i = 0; i < state.player_guitar.size(); ++i) {
      stream << "player" << i << "_guitar=" << state.player_guitar[i] << "\n";
      stream << "player" << i << "_guitar_skin="
             << state.player_guitar_skin[i] << "\n";
    }
    for (const std::string& item : state.unlocked)
      stream << "unlock=" << item << "\n";
    if (!stream) return false;
  }
  fs::remove(path, error);
  error.clear();
  fs::rename(temp, path, error);
  if (error) {
    fs::remove(temp, error);
    return false;
  }
  std::fprintf(stderr,
               "[profile] saved path=%s cash=%d guitar=%s skin=%s "
               "unlocks=%zu\n",
               path.string().c_str(), state.cash, state.guitar.c_str(),
               state.guitar_skin.c_str(), state.unlocked.size());
  return true;
}

Symbol arg_symbol(const DataArray& args, std::size_t index,
                  Symbol fallback = Symbol()) {
  if (index >= args.size()) return fallback;
  if (auto s = args.at(index).as_symbol()) return *s;
  if (auto text = args.at(index).as_string()) return Symbol(*text);
  return fallback;
}

int arg_int(const DataArray& args, std::size_t index, int fallback = 0) {
  if (index >= args.size()) return fallback;
  return args.at(index).as_int().value_or(fallback);
}

std::string arg_text(const DataArray& args, std::size_t index,
                     std::string fallback = {}) {
  if (index >= args.size()) return fallback;
  if (auto text = args.at(index).as_string()) return std::string(*text);
  return fallback;
}

std::string node_text(const DataNode& node, std::string fallback = {}) {
  if (auto text = node.as_string()) return std::string(*text);
  if (auto symbol = node.as_symbol()) return std::string(symbol->c_str());
  return fallback;
}

DataNode array_node(std::initializer_list<DataNode> values) {
  auto array = std::make_shared<DataArray>();
  for (const auto& value : values) array->push(value);
  return DataNode::Array(std::move(array));
}

Symbol default_difficulty() { return Symbol("kDifficultyMedium"); }

Symbol difficulty_display_symbol(Symbol difficulty) {
  if (difficulty == Symbol("kDifficultyEasy")) return Symbol("easy");
  if (difficulty == Symbol("kDifficultyHard")) return Symbol("hard");
  if (difficulty == Symbol("kDifficultyExpert")) return Symbol("expert");
  return Symbol("medium");
}

Symbol canonical_difficulty_symbol(Symbol difficulty) {
  if (difficulty == Symbol("easy")) return Symbol("kDifficultyEasy");
  if (difficulty == Symbol("medium")) return Symbol("kDifficultyMedium");
  if (difficulty == Symbol("hard")) return Symbol("kDifficultyHard");
  if (difficulty == Symbol("expert")) return Symbol("kDifficultyExpert");
  return difficulty.valid() ? difficulty : default_difficulty();
}

Symbol node_difficulty_or_default(const DataNode& node) {
  Symbol difficulty = node.as_symbol().value_or(Symbol());
  return difficulty.valid() ? difficulty : default_difficulty();
}

bool node_bool(const DataNode& node) {
  if (auto i = node.as_int()) return *i != 0;
  if (auto f = node.as_float()) return *f != 0.0f;
  if (auto s = node.as_string())
    return !(*s == "FALSE" || *s == "false" || *s == "0" || s->empty());
  return node.as_object() != nullptr;
}

Symbol node_symbol_or_string(const DataNode& node) {
  if (auto sym = node.as_symbol()) return *sym;
  if (auto text = node.as_string()) return Symbol(*text);
  return Symbol();
}

void flatten_symbols(const DataNode& node, std::vector<Symbol>& out) {
  if (auto symbol = node.as_symbol()) {
    if (symbol->valid()) out.push_back(*symbol);
    return;
  }
  if (auto array = node.as_array()) {
    for (std::size_t i = 0; i < array->size(); ++i)
      flatten_symbols(array->at(i), out);
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
        node_symbol_or_string(entry->at(1)) != macro_name) {
      continue;
    }
    for (std::size_t j = 2; j < entry->size(); ++j)
      flatten_symbols(entry->at(j), out);
    break;
  }
  return out;
}

std::vector<Symbol> character_symbols(const ConfigDb& db) {
  std::vector<Symbol> characters = db.characters();
  return characters.empty()
             ? ui_macro_symbols(db, Symbol("CHARACTERS"))
             : characters;
}

std::vector<Symbol> outfits_for_character(const ConfigDb& db,
                                          Symbol character) {
  std::vector<Symbol> out;
  const auto variants = db.character_variants(character);
  for (const auto& variant : variants) out.push_back(variant.selection);
  if (!out.empty()) return out;

  const std::string base = character.c_str();
  for (Symbol outfit : ui_macro_symbols(db, Symbol("LOAD_CHARACTERS"))) {
    const std::string name = outfit.c_str();
    if (name == base ||
        (name.rfind(base, 0) == 0 && name.size() == base.size() + 1 &&
         name.back() >= '0' && name.back() <= '9')) {
      out.push_back(outfit);
    }
  }
  if (out.empty() && character.valid()) out.push_back(character);
  return out;
}

Symbol default_venue(const ConfigDb* db) {
  Symbol venue = db ? db->default_venue() : Symbol();
  return venue.valid() ? venue : Symbol("small2");
}

Symbol current_venue_or_default(const Object& obj, const ConfigDb* db) {
  Symbol venue = node_symbol_or_string(obj.get_property(Symbol("venue")));
  if (venue.valid() && (!db || db->is_venue(venue))) return venue;
  return default_venue(db);
}

int campaign_starting_cash(const ConfigDb* db) {
  const DataArray* campaign = db ? db->table(Symbol("campaign")) : nullptr;
  if (!campaign) return 0;
  auto cash = campaign->find_keyed(Symbol("cash"));
  if (!cash) return 0;
  auto starting = cash->find_keyed(Symbol("starting"));
  return (starting && starting->size() > 1)
             ? starting->at(1).as_int().value_or(0)
             : 0;
}

int store_price_for_item(const ConfigDb* db, Symbol item) {
  if (!db || !item.valid()) return 0;
  static constexpr const char* kCategories[] = {
      "guitar", "skin", "song", "character", "outfit", "video"};
  for (const char* category_name : kCategories) {
    const Symbol category(category_name);
    DataNode price = db->store_field(category, item, Symbol("price"));
    if (auto value = price.as_int()) return *value;
  }
  return 0;
}

bool campaign_item_unlocked(ScreenManager* mgr, Symbol item) {
  if (!mgr || !item.valid()) return false;
  Object* campaign = mgr->resolve_object(Symbol("campaign"));
  if (!campaign) return false;
  DataArray args;
  args.push(DataNode::Sym(item));
  return node_bool(campaign->handle_property(Symbol("is_unlocked"), args));
}

std::vector<Symbol> owned_guitars(ScreenManager* mgr, const ConfigDb* db,
                                  Symbol type) {
  std::vector<Symbol> output;
  if (!db) return output;
  const std::vector<Symbol> store_items = db->store_items(Symbol("guitar"));
  for (Symbol guitar : db->guitars(type)) {
    const DataArray* record = db->guitar(guitar);
    const bool is_store_item =
        std::find(store_items.begin(), store_items.end(), guitar) !=
        store_items.end();
    const bool has_requirement =
        record && !ConfigDb::field(record, Symbol("require")).empty();
    if (campaign_item_unlocked(mgr, guitar) ||
        (!is_store_item && !has_requirement))
      output.push_back(guitar);
  }
  return output;
}

bool guitar_award_require(Symbol require) {
  return require == Symbol("tour_passed") ||
         require == Symbol("tour_5_star");
}

std::vector<Symbol> campaign_reward_guitars(const ConfigDb* db) {
  std::vector<Symbol> out;
  if (!db) return out;
  for (Symbol item : db->store_items(Symbol("guitar"))) {
    const DataArray* guitar = db->guitar(item);
    if (!guitar) continue;
    Symbol type = ConfigDb::field(guitar, Symbol("type"))
                      .as_symbol()
                      .value_or(Symbol());
    Symbol require = ConfigDb::field(guitar, Symbol("require"))
                         .as_symbol()
                         .value_or(Symbol());
    Symbol difficulty = ConfigDb::field(guitar, Symbol("difficulty"))
                            .as_symbol()
                            .value_or(Symbol());
    if (type == Symbol("guitar") && guitar_award_require(require) &&
        difficulty.valid())
      out.push_back(item);
  }
  return out;
}

std::vector<Symbol> campaign_reward_guitars_for(const ConfigDb* db,
                                                Symbol difficulty,
                                                Symbol require) {
  std::vector<Symbol> out;
  if (!db || !difficulty.valid() || !guitar_award_require(require))
    return out;
  for (Symbol item : campaign_reward_guitars(db)) {
    const DataArray* guitar = db->guitar(item);
    if (!guitar) continue;
    Symbol item_difficulty = ConfigDb::field(guitar, Symbol("difficulty"))
                                 .as_symbol()
                                 .value_or(Symbol());
    Symbol item_require = ConfigDb::field(guitar, Symbol("require"))
                              .as_symbol()
                              .value_or(Symbol());
    if (item_difficulty == difficulty && item_require == require)
      out.push_back(item);
  }
  return out;
}

Symbol campaign_pending_guitar_award_key(Symbol award) {
  return Symbol(std::string("pending_guitar_award.") + award.c_str());
}

void campaign_enqueue_guitar_awards(Object& campaign, ConfigDb* db,
                                    Symbol difficulty, Symbol require) {
  for (Symbol award : campaign_reward_guitars_for(db, difficulty, require)) {
    if (node_bool(campaign.get_property(award))) continue;
    campaign.set_property(campaign_pending_guitar_award_key(award),
                          DataNode::Int(1));
  }
}

int campaign_pending_guitar_award_count(const Object& campaign,
                                        ConfigDb* db) {
  int count = 0;
  for (Symbol award : campaign_reward_guitars(db)) {
    if (node_bool(campaign.get_property(
            campaign_pending_guitar_award_key(award))))
      ++count;
  }
  return count;
}

Symbol campaign_next_guitar_award(Object& campaign, ConfigDb* db) {
  for (Symbol award : campaign_reward_guitars(db)) {
    const Symbol pending_key = campaign_pending_guitar_award_key(award);
    if (!node_bool(campaign.get_property(pending_key))) continue;
    campaign.set_property(pending_key, DataNode::Int(0));
    campaign.set_property(award, DataNode::Int(1));
    return award;
  }
  return Symbol();
}

std::vector<Symbol> campaign_song_order(const ConfigDb* db) {
  std::vector<Symbol> out;
  const DataArray* campaign = db ? db->table(Symbol("campaign")) : nullptr;
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  if (!order) return out;
  for (std::size_t i = 1; i < order->size(); ++i) {
    auto tier = order->at(i).as_array();
    if (!tier) continue;
    for (std::size_t j = 1; j < tier->size(); ++j) {
      Symbol song = tier->at(j).as_symbol().value_or(Symbol());
      if (song.valid()) out.push_back(song);
    }
  }
  return out;
}

Symbol campaign_pick_attract_song(Object& campaign, ConfigDb* db) {
  std::vector<Symbol> songs = campaign_song_order(db);
  if (songs.empty() && db && db->song_count() > 0) {
    Symbol fallback = db->song_key(0);
    if (fallback.valid()) songs.push_back(fallback);
  }
  if (songs.empty()) return Symbol();

  std::size_t index = 0;
  const DataNode override =
      campaign.get_property(Symbol("attract_song_index"));
  if (auto raw_index = override.as_int()) {
    // Explicit diagnostic/test override: advance predictably through the
    // shipped campaign order without changing production attract behavior.
    index = static_cast<std::size_t>(std::max(0, *raw_index)) % songs.size();
    campaign.set_property(
        Symbol("attract_song_index"),
        DataNode::Int(static_cast<int>((index + 1) % songs.size())));
  } else {
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> pick(0, songs.size() - 1);
    index = pick(rng);
    const Symbol last =
        campaign.get_property(Symbol("last_attract_song"))
            .as_symbol().value_or(Symbol());
    if (songs.size() > 1 && songs[index] == last)
      index = (index + 1) % songs.size();
  }
  campaign.set_property(Symbol("last_attract_song"), DataNode::Sym(songs[index]));
  return songs[index];
}

bool campaign_contains_song(const ConfigDb* db, Symbol song) {
  if (!song.valid()) return false;
  const auto songs = campaign_song_order(db);
  return std::find(songs.begin(), songs.end(), song) != songs.end();
}

Symbol campaign_last_song(const ConfigDb* db) {
  const auto songs = campaign_song_order(db);
  return songs.empty() ? Symbol() : songs.back();
}

Symbol campaign_current_song(ScreenManager* mgr) {
  if (!mgr) return Symbol();
  Object* game = mgr->resolve_object(Symbol("game"));
  return game ? node_symbol_or_string(game->handle_property(Symbol("get_song"),
                                                           DataArray()))
              : Symbol();
}

bool campaign_encore_song(const ConfigDb* db, Symbol song) {
  if (!song.valid()) return false;
  const DataArray* campaign = db ? db->table(Symbol("campaign")) : nullptr;
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  if (!order) return false;
  for (std::size_t i = 1; i < order->size(); ++i) {
    auto tier = order->at(i).as_array();
    if (!tier || tier->size() < 2) continue;
    Symbol encore = tier->at(tier->size() - 1).as_symbol().value_or(Symbol());
    if (encore == song) return true;
  }
  return false;
}

Symbol campaign_encore_for_venue(const ConfigDb* db, Symbol venue) {
  const DataArray* campaign = db ? db->table(Symbol("campaign")) : nullptr;
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  if (!order || !venue.valid()) return Symbol();
  for (std::size_t i = 1; i < order->size(); ++i) {
    auto tier = order->at(i).as_array();
    if (!tier || tier->size() < 2) continue;
    if (tier->at(0).as_symbol().value_or(Symbol()) != venue) continue;
    return tier->at(tier->size() - 1).as_symbol().value_or(Symbol());
  }
  return Symbol();
}

const DataArray* campaign_tier_for_venue(const ConfigDb* db, Symbol venue) {
  const DataArray* campaign = db ? db->table(Symbol("campaign")) : nullptr;
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  if (!order || !venue.valid()) return nullptr;
  for (std::size_t i = 1; i < order->size(); ++i) {
    auto tier = order->at(i).as_array();
    if (!tier || tier->empty()) continue;
    if (tier->at(0).as_symbol().value_or(Symbol()) == venue) return tier.get();
  }
  return nullptr;
}

Symbol campaign_venue_for_song(const ConfigDb* db, Symbol song) {
  const DataArray* campaign = db ? db->table(Symbol("campaign")) : nullptr;
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  if (!order || !song.valid()) return Symbol();
  for (std::size_t i = 1; i < order->size(); ++i) {
    auto tier = order->at(i).as_array();
    if (!tier || tier->size() < 2) continue;
    for (std::size_t j = 1; j < tier->size(); ++j) {
      if (tier->at(j).as_symbol().value_or(Symbol()) == song)
        return tier->at(0).as_symbol().value_or(Symbol());
    }
  }
  return Symbol();
}

bool campaign_regular_song_in_venue(const ConfigDb* db, Symbol venue,
                                    Symbol song) {
  const DataArray* tier = campaign_tier_for_venue(db, venue);
  if (!tier || tier->size() < 3 || !song.valid()) return false;
  for (std::size_t i = 1; i + 1 < tier->size(); ++i) {
    if (tier->at(i).as_symbol().value_or(Symbol()) == song) return true;
  }
  return false;
}

int campaign_required_songs_for_encore(const ConfigDb* db, Symbol difficulty) {
  const DataArray* campaign = db ? db->table(Symbol("campaign")) : nullptr;
  auto required = campaign ? campaign->find_keyed(Symbol("required_songs")) : nullptr;
  if (!required) return 0;
  for (std::size_t i = 1; i < required->size(); ++i) {
    auto row = required->at(i).as_array();
    if (!row || row->size() < 2) continue;
    if (row->at(0).as_symbol().value_or(Symbol()) == difficulty)
      return row->at(1).as_int().value_or(0);
  }
  return 0;
}

int campaign_max_status(const ConfigDb* db, Symbol difficulty) {
  const DataArray* campaign = db ? db->table(Symbol("campaign")) : nullptr;
  auto cash = campaign ? campaign->find_keyed(Symbol("cash")) : nullptr;
  auto awards = cash ? cash->find_keyed(Symbol("status_awards")) : nullptr;
  if (!awards) return 0;
  for (std::size_t i = 1; i < awards->size(); ++i) {
    auto row = awards->at(i).as_array();
    if (!row || row->empty()) continue;
    if (row->at(0).as_symbol().value_or(Symbol()) == difficulty)
      return row->size() > 1 ? static_cast<int>(row->size() - 2) : 0;
  }
  return 0;
}

Symbol campaign_progress_key(const char* prefix, Symbol difficulty, Symbol song) {
  return Symbol(std::string(prefix) + "." + difficulty.c_str() + "." +
                song.c_str());
}

bool campaign_song_beaten(const Object& campaign, Symbol difficulty,
                          Symbol song) {
  if (!song.valid()) return false;
  if (!difficulty.valid()) difficulty = default_difficulty();
  return node_bool(
      campaign.get_property(campaign_progress_key("beat", difficulty, song)));
}

int campaign_regular_beaten_count_for_venue(const Object& campaign,
                                            ConfigDb* db, Symbol difficulty,
                                            Symbol venue) {
  const DataArray* tier = campaign_tier_for_venue(db, venue);
  if (!tier || tier->size() < 3) return 0;
  int count = 0;
  for (std::size_t i = 1; i + 1 < tier->size(); ++i) {
    Symbol song = tier->at(i).as_symbol().value_or(Symbol());
    if (campaign_song_beaten(campaign, difficulty, song)) ++count;
  }
  return count;
}

bool campaign_encore_unlock_potential(const Object& campaign, ConfigDb* db,
                                      Symbol difficulty, Symbol song) {
  Symbol venue = campaign_venue_for_song(db, song);
  const int required = campaign_required_songs_for_encore(db, difficulty);
  if (!venue.valid() || required <= 0 ||
      !campaign_regular_song_in_venue(db, venue, song))
    return false;
  int count = campaign_regular_beaten_count_for_venue(campaign, db, difficulty,
                                                     venue);
  if (!campaign_song_beaten(campaign, difficulty, song)) ++count;
  return count == required;
}

bool campaign_encore_newly_unlocked(const Object& campaign, ConfigDb* db,
                                    Symbol difficulty, Symbol song) {
  Symbol finished = node_symbol_or_string(
      campaign.get_property(Symbol("last_finished_song")));
  if (finished.valid()) song = finished;
  Symbol venue = campaign_venue_for_song(db, song);
  const int required = campaign_required_songs_for_encore(db, difficulty);
  if (!venue.valid() || required <= 0 ||
      !campaign_regular_song_in_venue(db, venue, song) ||
      !node_bool(campaign.get_property(Symbol("last_finished_newly_beaten"))))
    return false;
  return campaign_regular_beaten_count_for_venue(campaign, db, difficulty,
                                                venue) == required;
}

void campaign_mark_song(Object& campaign, ConfigDb* db, Symbol difficulty,
                        Symbol song, int score) {
  if (!song.valid() || !campaign_contains_song(db, song)) return;
  if (!difficulty.valid()) difficulty = default_difficulty();
  campaign.set_property(campaign_progress_key("beat", difficulty, song),
                        DataNode::Int(1));
  campaign.set_property(song, DataNode::Int(1));
  if (score > 0) {
    const Symbol score_key = campaign_progress_key("score", difficulty, song);
    const int previous = campaign.get_property(score_key).as_int().value_or(0);
    if (score > previous) {
      campaign.set_property(score_key, DataNode::Int(score));
      const int total =
          campaign.get_property(Symbol("career_score")).as_int().value_or(0);
      campaign.set_property(Symbol("career_score"),
                            DataNode::Int(total + score - previous));
    }
  }
}

int campaign_beaten_count(const Object& campaign, ConfigDb* db,
                          Symbol difficulty) {
  int count = 0;
  for (Symbol song : campaign_song_order(db)) {
    if (node_bool(
            campaign.get_property(campaign_progress_key("beat", difficulty, song))))
      ++count;
  }
  return count;
}

std::size_t clamped_index(const DataArray& args) {
  const int idx = arg_int(args, 0, 0);
  return static_cast<std::size_t>(std::max(0, idx));
}

Symbol property_key(const DataNode& node) {
  if (auto s = node.as_symbol()) return *s;
  if (auto text = node.as_string()) return Symbol(*text);
  auto arr = node.as_array();
  if (!arr || arr->empty()) return Symbol();
  Symbol head = arr->at(0).as_symbol().value_or(Symbol());
  if (!head.valid()) return Symbol();
  std::string key = head.c_str();
  for (std::size_t i = 1; i < arr->size(); ++i) {
    key.push_back('.');
    if (auto s = arr->at(i).as_symbol())
      key += s->c_str();
    else if (auto text = arr->at(i).as_string())
      key += std::string(*text);
    else if (auto v = arr->at(i).as_int())
      key += std::to_string(*v);
    else
      key += "?";
  }
  return Symbol(key);
}
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
  if (std::strcmp(m, "set") == 0) {
    Symbol key = args.size() ? property_key(args.at(0)) : Symbol();
    if (key.valid()) set_property(key, args.size() > 1 ? args.at(1) : DataNode());
    return true;
  }
  if (std::strcmp(m, "get") == 0) {
    Symbol key = args.size() ? property_key(args.at(0)) : Symbol();
    out = key.valid() ? get_property(key) : DataNode();
    return true;
  }
  if (std::strcmp(m, "has") == 0) {
    Symbol key = args.size() ? property_key(args.at(0)) : Symbol();
    out = DataNode::Int(key.valid() && has_property(key) ? 1 : 0);
    return true;
  }
  if (std::strncmp(m, "set_", 4) == 0) {
    set_property(Symbol(m + 4), args.size() ? args.at(0) : DataNode());
    return true;
  }
  if (std::strncmp(m, "get_", 4) == 0) {
    out = get_property(Symbol(m + 4));
    return true;
  }
  if (args.empty() && has_property(msg)) {
    out = get_property(msg);
    return true;
  }
  return false;
}

namespace {

Symbol first_tip_key(const DataArray& group) {
  // tips.dtb groups are (tips_general (loading_tip1) ...). Pick the first
  // authored key deterministically so tests/screenshots are repeatable.
  for (std::size_t i = 1; i < group.size(); ++i) {
    auto row = group.at(i).as_array();
    if (!row || row->empty()) continue;
    Symbol key = row->at(0).as_symbol().value_or(Symbol());
    if (key.valid()) return key;
  }
  return Symbol();
}

class TipsObject : public MetaObject {
 public:
  TipsObject(ScreenManager* mgr, ConfigDb* db) : MetaObject(Symbol("tips"), mgr, db) {}

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    (void)args;
    if (msg != Symbol("random_tip")) return false;
    const DataArray* tips = db_ ? db_->table(Symbol("tips")) : nullptr;
    Symbol key;
    if (tips) {
      if (auto general = tips->find_keyed(Symbol("tips_general")))
        key = first_tip_key(*general);
      if (!key.valid()) {
        for (std::size_t i = 0; i < tips->size() && !key.valid(); ++i) {
          if (auto group = tips->at(i).as_array())
            key = first_tip_key(*group);
        }
      }
    }
    out = key.valid() ? DataNode::Sym(key) : DataNode();
    return true;
  }
};

class SongProvider : public MetaObject {
 public:
  SongProvider(ScreenManager* mgr, ConfigDb* db)
      : MetaObject(Symbol("song_provider"), mgr, db) {
    set_property(Symbol("quickplay"), DataNode::Int(0));
  }

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    const char* m = msg.c_str();
    if (std::strcmp(m, "set_quickplay") == 0) {
      const bool enabled = args.size() && node_bool(args.at(0));
      set_property(Symbol("quickplay"), DataNode::Int(enabled ? 1 : 0));
      return true;
    }
    if (std::strcmp(m, "get_quickplay") == 0) {
      out = DataNode::Int(
          node_bool(get_property(Symbol("quickplay"))) ? 1 : 0);
      return true;
    }

    const auto songs = current_songs();
    const int requested = arg_int(args, 0, 0);
    const std::size_t index =
        songs.empty()
            ? 0
            : std::min<std::size_t>(
                  static_cast<std::size_t>(std::max(0, requested)),
                  songs.size() - 1);
    const Symbol song = songs.empty() ? Symbol() : songs[index];

    if (std::strcmp(m, "list_length") == 0) {
      out = DataNode::Int(static_cast<int>(songs.size()));
      return true;
    }
    if (std::strcmp(m, "num_headers") == 0) {
      out = DataNode::Int(0);
      return true;
    }
    if (std::strcmp(m, "get_symbol") == 0) {
      out = song.valid() ? DataNode::Sym(song) : DataNode();
      return true;
    }
    if (std::strcmp(m, "get_text") == 0) {
      const int global_index = db_ ? db_->song_index(song) : -1;
      const int column = arg_int(args, 1, 0);
      const Symbol field = column == 1 ? Symbol("artist") : Symbol("name");
      out = global_index >= 0
                ? db_->song_field(static_cast<std::size_t>(global_index), field)
                : DataNode();
      if (out.empty() && song.valid()) out = DataNode::Sym(song);
      return true;
    }
    if (std::strcmp(m, "refresh") == 0 ||
        std::strcmp(m, "init_data") == 0)
      return true;
    return false;
  }

 private:
  std::vector<Symbol> current_songs() const {
    std::vector<Symbol> songs;
    if (!db_) return songs;
    if (node_bool(get_property(Symbol("quickplay")))) {
      songs.reserve(db_->song_count());
      for (std::size_t i = 0; i < db_->song_count(); ++i) {
        Symbol song = db_->song_key(i);
        if (song.valid()) songs.push_back(song);
      }
      return songs;
    }
    Symbol venue;
    if (mgr_) {
      if (Object* game = mgr_->resolve_object(Symbol("game")))
        venue = node_symbol_or_string(game->get_property(Symbol("venue")));
    }
    return db_->campaign_songs(venue);
  }
};

class StoreProvider : public MetaObject {
 public:
  StoreProvider(Symbol cls, ScreenManager* mgr, ConfigDb* db,
                Symbol fixed_category = Symbol())
      : MetaObject(cls, mgr, db), fixed_category_(fixed_category) {
    if (fixed_category_.valid())
      set_property(Symbol("category"), DataNode::Sym(fixed_category_));
  }

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    const char* m = msg.c_str();
    if (std::strcmp(m, "init_data") == 0) return true;
    if (std::strcmp(m, "set_category") == 0) {
      if (!fixed_category_.valid())
        set_property(Symbol("category"), args.size() ? args.at(0) : DataNode());
      return true;
    }

    const Symbol category = current_category();
    const std::size_t index = clamped_index(args);
    const Symbol item = db_ ? db_->store_item(category, index) : Symbol();

    if (std::strcmp(m, "list_length") == 0) {
      out = DataNode::Int(db_ ? static_cast<int>(db_->store_item_count(category)) : 0);
      return true;
    }
    if (std::strcmp(m, "get_symbol") == 0) {
      out = item.valid() ? DataNode::Sym(item) : DataNode();
      return true;
    }
    if (std::strcmp(m, "get_text") == 0) {
      DataNode name =
          item.valid() && db_ ? db_->store_field(category, item, Symbol("name"))
                              : DataNode();
      out = name.empty() && item.valid() ? DataNode::Sym(item) : name;
      return true;
    }
    if (std::strcmp(m, "in_stock") == 0) {
      out = DataNode::Int(item.valid() ? 1 : 0);
      return true;
    }
    if (std::strcmp(m, "price") == 0) {
      DataNode price =
          item.valid() && db_ ? db_->store_field(category, item, Symbol("price"))
                              : DataNode();
      out = price.empty() ? DataNode::Int(0) : price;
      return true;
    }
    if (std::strcmp(m, "get_guitar") == 0) {
      Symbol guitar;
      if (item.valid() && db_) {
        guitar = db_->store_field(category, item, Symbol("guitar"))
                     .as_symbol()
                     .value_or(Symbol());
        if (!guitar.valid() && category == Symbol("skin"))
          guitar = db_->guitar_for_skin(item);
      }
      out = guitar.valid() ? DataNode::Sym(guitar) : DataNode();
      return true;
    }
    return false;
  }

 private:
  Symbol current_category() const {
    if (fixed_category_.valid()) return fixed_category_;
    return get_property(Symbol("category")).as_symbol().value_or(Symbol());
  }

  Symbol fixed_category_;
};

class PlayerConfig : public MetaObject {
 public:
  PlayerConfig(ScreenManager* mgr, ConfigDb* db, int player_index)
      : MetaObject(Symbol("PlayerCfg"), mgr, db),
        player_index_(std::clamp(player_index, 0, 1)) {
    set_property(Symbol("score"), DataNode::Int(0));
    set_property(Symbol("percent_hit"), DataNode::Int(0));
    set_property(Symbol("percent_complete"), DataNode::Int(0));
    set_property(Symbol("longest_streak"), DataNode::Int(0));
    set_property(Symbol("star_rating"), DataNode::Str(""));
    set_property(Symbol("num_stars"), DataNode::Int(0));
    set_property(Symbol("gems_hit"), DataNode::Int(0));
    set_property(Symbol("gems_passed"), DataNode::Int(0));
    set_property(Symbol("avg_multiplier"), DataNode::Float(0.0f));
    set_property(Symbol("sp_phrases"), DataNode::Str("0/0"));
    set_property(Symbol("star_power_ready"), DataNode::Int(0));
    set_property(Symbol("in_star_mode"), DataNode::Int(0));
    set_property(Symbol("player_matcher"), DataNode::Obj(this));
    const PersistentProfileState& profile = persistent_profile_state();
    const std::string& guitar = profile.player_guitar[player_index_];
    const std::string& skin = profile.player_guitar_skin[player_index_];
    if (!guitar.empty() && db_ && db_->guitar(Symbol(guitar)))
      set_property(Symbol("guitar"), DataNode::Sym(Symbol(guitar)));
    if (!skin.empty() && db_ && db_->guitar_for_skin(Symbol(skin)).valid())
      set_property(Symbol("guitar_skin"), DataNode::Sym(Symbol(skin)));
  }

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    const char* m = msg.c_str();
    if (std::strcmp(m, "player_matcher") == 0) {
      out = DataNode::Obj(this);
      return true;
    }
    if (std::strcmp(m, "set_difficulty") == 0) {
      set_property(Symbol("difficulty"),
                   DataNode::Sym(canonical_difficulty_symbol(
                       arg_symbol(args, 0, default_difficulty()))));
      return true;
    }
    if (std::strcmp(m, "get_difficulty") == 0) {
      out = DataNode::Sym(canonical_difficulty_symbol(
          get_property(Symbol("difficulty")).as_symbol().value_or(Symbol())));
      return true;
    }
    if (std::strcmp(m, "set_guitar") == 0) {
      Symbol guitar =
          arg_symbol(args, 0,
                     get_property(Symbol("guitar"))
                         .as_symbol()
                         .value_or(Symbol()));
      Symbol skin = arg_symbol(args, 1);
      if (!skin.valid() && db_) skin = db_->first_guitar_skin(guitar);
      if (guitar.valid())
        set_property(Symbol("guitar"), DataNode::Sym(guitar));
      if (skin.valid())
        set_property(Symbol("guitar_skin"), DataNode::Sym(skin));
      PersistentProfileState& profile = persistent_profile_state();
      profile.player_guitar[player_index_] =
          guitar.valid() ? guitar.c_str() : "";
      profile.player_guitar_skin[player_index_] =
          skin.valid() ? skin.c_str() : "";
      save_persistent_profile_state();
      return true;
    }
    if (std::strcmp(m, "set_outfit_index") == 0) {
      Symbol character = node_symbol_or_string(
          get_property(Symbol("character")));
      const std::vector<Symbol> outfits =
          db_ ? outfits_for_character(*db_, character)
              : std::vector<Symbol>();
      const int index = outfits.empty()
                            ? 0
                            : std::clamp(arg_int(args, 0, 0), 0,
                                         static_cast<int>(outfits.size() - 1));
      set_property(Symbol("outfit_index"), DataNode::Int(index));
      const Symbol outfit =
          outfits.empty() ? character : outfits[static_cast<std::size_t>(index)];
      if (outfit.valid())
        set_property(Symbol("character_outfit"), DataNode::Sym(outfit));
      // multiplayer.dta binds this return value and passes it directly to
      // CharsysPanel::show_char before issuing the select event.
      out = outfit.valid() ? DataNode::Sym(outfit) : DataNode();
      return true;
    }
    if (std::strcmp(m, "fill_star_power") == 0) {
      set_property(Symbol("star_power_ready"), DataNode::Int(1));
      return true;
    }
    if (std::strcmp(m, "empty_star_power") == 0) {
      set_property(Symbol("star_power_ready"), DataNode::Int(0));
      set_property(Symbol("in_star_mode"), DataNode::Int(0));
      return true;
    }
    if (std::strcmp(m, "add_sink") == 0 ||
        std::strcmp(m, "remove_sink") == 0) {
      set_property(Symbol("last_sink"), args.size() ? args.at(0) : DataNode());
      set_property(Symbol("last_sink_op"), DataNode::Sym(msg));
      return true;
    }
    return false;
  }

 private:
  int player_index_ = 0;
};

class CharacterProvider : public MetaObject {
 public:
  CharacterProvider(ScreenManager* mgr, ConfigDb* db)
      : MetaObject(Symbol("character_provider"), mgr, db) {}

 protected:
  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    if (!db_) return false;
    const char* m = msg.c_str();
    const std::vector<Symbol> characters = character_symbols(*db_);
    if (std::strcmp(m, "list_length") == 0 ||
        std::strcmp(m, "num_data") == 0) {
      out = DataNode::Int(static_cast<int>(characters.size()));
      return true;
    }
    if (std::strcmp(m, "get_index") == 0) {
      Symbol character = arg_symbol(args, 0);
      if (Symbol canonical = db_->character_for_variant(character);
          canonical.valid()) {
        character = canonical;
      }
      const auto found =
          std::find(characters.begin(), characters.end(), character);
      out = DataNode::Int(found == characters.end()
                              ? 0
                              : static_cast<int>(found - characters.begin()));
      return true;
    }
    if (std::strcmp(m, "get_symbol") == 0 ||
        std::strcmp(m, "get_text") == 0) {
      int index = arg_int(args, 0, 0);
      if (index < 0) index = 0;
      if (index >= static_cast<int>(characters.size()))
        index = characters.empty() ? 0
                                   : static_cast<int>(characters.size() - 1);
      out = characters.empty() ? DataNode()
                               : DataNode::Sym(characters[index]);
      return true;
    }
    if (std::strcmp(m, "is_active") == 0) {
      out = DataNode::Int(1);
      return true;
    }
    const Symbol character = arg_symbol(args, 0);
    const std::vector<Symbol> outfits =
        outfits_for_character(*db_, character);
    if (std::strcmp(m, "num_outfits") == 0) {
      out = DataNode::Int(static_cast<int>(outfits.size()));
      return true;
    }
    if (std::strcmp(m, "get_outfit") == 0) {
      int index = arg_int(args, 1, 0);
      if (index < 0) index = 0;
      if (index >= static_cast<int>(outfits.size()))
        index = outfits.empty() ? 0 : static_cast<int>(outfits.size() - 1);
      out = outfits.empty() ? DataNode() : DataNode::Sym(outfits[index]);
      return true;
    }
    if (std::strcmp(m, "get_outfit_label") == 0) {
      int index = arg_int(args, 1, 0);
      if (index < 0) index = 0;
      if (index >= static_cast<int>(outfits.size()))
        index = outfits.empty() ? 0 : static_cast<int>(outfits.size() - 1);
      const CharacterVariant* variant =
          outfits.empty() ? nullptr : db_->character_variant(outfits[index]);
      out = variant ? DataNode::Str(variant->label) : DataNode();
      return true;
    }
    if (std::strcmp(m, "get_outfit_blurb") == 0) {
      int index = arg_int(args, 1, 0);
      if (index < 0) index = 0;
      if (index >= static_cast<int>(outfits.size()))
        index = outfits.empty() ? 0 : static_cast<int>(outfits.size() - 1);
      const CharacterVariant* variant =
          outfits.empty() ? nullptr : db_->character_variant(outfits[index]);
      // Imported variants have no GH2-authored outfit copy. Keep that absence
      // explicit instead of borrowing another game's or the character bio.
      if (!variant || variant->source_game != Symbol("gh2")) {
        out = DataNode::Str("");
        return true;
      }
      const std::string token =
          std::string(character.c_str()) + "_outfit_blurb";
      out = DataNode::Sym(Symbol(token.c_str()));
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
      int index = arg_int(args, 0, 0);
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

class BandStats : public MetaObject {
 public:
  BandStats(ScreenManager* mgr, ConfigDb* db)
      : MetaObject(Symbol("band"), mgr, db) {
    set_property(Symbol("score"), DataNode::Int(0));
    set_property(Symbol("longest_streak"), DataNode::Int(0));
    set_property(Symbol("star_rating"), DataNode::Int(0));
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
      out = DataNode::Int(highscore_index(arg_int(args, 0, 0)));
      return true;
    }
    if (std::strcmp(m, "get_default_name") == 0) {
      out = get_property(Symbol("default_name"));
      return true;
    }
    if (std::strcmp(m, "set_default_name") == 0) {
      set_property(Symbol("default_name"),
                   args.empty() ? DataNode::Str("AAAA") : args.at(0));
      return true;
    }
    if (std::strcmp(m, "add") == 0) {
      add_entry(node_text(args.size() > 0 ? args.at(0)
                                          : get_property(Symbol("default_name"))),
                arg_int(args, 1, 0));
      return true;
    }
    if (std::strcmp(m, "get_highscore") == 0) {
      const int slot = std::clamp(arg_int(args, 0, 0), 0, 5);
      const auto& list = entries();
      if (slot < static_cast<int>(list.size())) {
        const Entry& entry = list[static_cast<std::size_t>(slot)];
        out = array_node({DataNode::Int(slot), DataNode::Str(entry.name),
                          DataNode::Int(entry.score)});
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

  std::string current_key() const {
    if (!mgr_) return "default";
    Object* game = mgr_->resolve_object(Symbol("game"));
    Object* player = mgr_->resolve_object(Symbol("player0"));
    const int song = game ? game->get_property(Symbol("song_index"))
                                .as_int()
                                .value_or(0)
                          : 0;
    const Symbol difficulty =
        player ? player->get_property(Symbol("difficulty"))
                     .as_symbol()
                     .value_or(default_difficulty())
               : default_difficulty();
    return std::to_string(song) + ":" + difficulty.c_str();
  }

  std::vector<Entry>& entries() { return entries_[current_key()]; }
  const std::vector<Entry>& entries() const {
    static const std::vector<Entry> empty;
    const auto it = entries_.find(current_key());
    return it == entries_.end() ? empty : it->second;
  }

  int highscore_index(int score) const {
    if (score <= 0) return -1;
    const auto& list = entries();
    for (std::size_t i = 0; i < list.size(); ++i)
      if (score > list[i].score) return static_cast<int>(i);
    return list.size() < 6 ? static_cast<int>(list.size()) : -1;
  }

  void add_entry(std::string name, int score) {
    const int index = highscore_index(score);
    if (index < 0) return;
    if (name.empty()) name = node_text(get_property(Symbol("default_name")), "AAAA");
    auto& list = entries();
    list.insert(list.begin() + index, Entry{std::move(name), score});
    if (list.size() > 6) list.resize(6);
  }

  std::map<std::string, std::vector<Entry>> entries_;
};

class StatsProvider : public MetaObject {
 public:
  StatsProvider(ScreenManager* mgr, ConfigDb* db)
      : MetaObject(Symbol("StatsProvider"), mgr, db) {}

 protected:
  struct Row {
    Symbol section;
    int hit = 0;
    int total = 0;
  };

  std::vector<Row> rows() const {
    std::vector<Row> result;
    Object* player = mgr_ ? mgr_->resolve_object(Symbol("player0")) : nullptr;
    if (!player) return result;
    if (auto data = player->get_property(Symbol("section_stats")).as_array()) {
      for (std::size_t i = 0; i < data->size(); ++i) {
        auto row = data->at(i).as_array();
        if (!row || row->size() < 3) continue;
        const Symbol section = row->at(0).as_symbol().value_or(Symbol());
        if (!section.valid()) continue;
        const int total = std::max(0, row->at(2).as_int().value_or(0));
        result.push_back({section,
                          std::clamp(row->at(1).as_int().value_or(0), 0, total),
                          total});
      }
    }
    if (result.empty()) {
      const int hit = std::max(0, player->get_property(Symbol("gems_hit"))
                                      .as_int()
                                      .value_or(0));
      const int miss = std::max(0, player->get_property(Symbol("gems_passed"))
                                       .as_int()
                                       .value_or(0));
      if (hit + miss > 0) result.push_back({Symbol("full_song"), hit, hit + miss});
    }
    return result;
  }

  bool handle_meta(Symbol msg, const DataArray& args, DataNode& out) override {
    const char* m = msg.c_str();
    const std::vector<Row> data = rows();
    if (std::strcmp(m, "list_length") == 0 ||
        std::strcmp(m, "num_data") == 0) {
      out = DataNode::Int(static_cast<int>(data.size()));
      return true;
    }
    if (data.empty()) return false;
    const int index = std::clamp(arg_int(args, 0, 0), 0,
                                 static_cast<int>(data.size()) - 1);
    const Row& row = data[static_cast<std::size_t>(index)];
    const std::string notes =
        std::to_string(row.hit) + "/" + std::to_string(row.total);
    if (std::strcmp(m, "get_symbol") == 0 ||
        std::strcmp(m, "get_section") == 0) {
      out = DataNode::Sym(row.section);
      return true;
    }
    if (std::strcmp(m, "get_notes1") == 0) {
      out = DataNode::Str(notes);
      return true;
    }
    if (std::strcmp(m, "get_notes2") == 0) {
      out = DataNode::Str("");
      return true;
    }
    if (std::strcmp(m, "get_text") == 0) {
      const int column = arg_int(args, 1, 0);
      if (column == 0)
        out = DataNode::Sym(row.section);
      else if (column == 1)
        out = DataNode::Str(notes);
      else
        out = DataNode::Str("");
      return true;
    }
    return false;
  }
};

}  // namespace

// --- GameConfig ------------------------------------------------------------
GameConfig::GameConfig(ScreenManager* mgr, ConfigDb* db) : MetaObject(Symbol("game"), mgr, db) {
  set_property(Symbol("song_index"), DataNode::Int(0));
  set_property(Symbol("game_screen"), DataNode::Sym(Symbol("game_screen")));
  set_property(Symbol("continue_screen"), DataNode::Sym(Symbol("main_screen")));
  set_property(Symbol("lose_screen"), DataNode::Sym(Symbol("lose_screen")));
  set_property(Symbol("win_screen"), DataNode::Sym(Symbol("endgame_screen")));
  set_property(Symbol("venue"), DataNode::Sym(default_venue(db_)));
  set_property(Symbol("multiple_controllers"), DataNode::Int(0));
  set_property(Symbol("missing_multi_controller"), DataNode::Int(1));
  Symbol default_guitar = db_ ? db_->first_guitar() : Symbol();
  if (default_guitar.valid()) {
    set_property(Symbol("guitar"), DataNode::Sym(default_guitar));
    Symbol default_skin = db_->first_guitar_skin(default_guitar);
    if (default_skin.valid())
      set_property(Symbol("guitar_skin"), DataNode::Sym(default_skin));
  }
  const PersistentProfileState& profile = persistent_profile_state();
  if (!profile.guitar.empty() && db_ &&
      db_->guitar(Symbol(profile.guitar)))
    set_property(Symbol("guitar"), DataNode::Sym(Symbol(profile.guitar)));
  if (!profile.guitar_skin.empty() && db_ &&
      db_->guitar_for_skin(Symbol(profile.guitar_skin)).valid())
    set_property(Symbol("guitar_skin"),
                 DataNode::Sym(Symbol(profile.guitar_skin)));
  // Retail GameConfig already owns a valid character before career.dtb's first
  // sel_character_panel::enter. Source that native default from the stock
  // panel fields instead of duplicating the roster in C++.
  if (mgr_) {
    if (Object* panel = mgr_->find_object(Symbol("sel_character_panel"))) {
      const Symbol character =
          node_symbol_or_string(panel->get_property(Symbol("char_focus")));
      const Symbol outfit =
          node_symbol_or_string(panel->get_property(Symbol("char_outfit")));
      if (character.valid())
        set_property(Symbol("character"), DataNode::Sym(character));
      if (outfit.valid())
        set_property(Symbol("character_outfit"), DataNode::Sym(outfit));
    }
  }
  for (int i = 0; i < 2; ++i)
    players_.push_back(std::make_unique<PlayerConfig>(mgr, db, i));
  for (auto& player : players_)
    player->set_property(Symbol("difficulty"), DataNode::Sym(default_difficulty()));
}

bool GameConfig::handle_meta(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();

  if (std::strcmp(m, "set") == 0 && arg_symbol(args, 0) == Symbol("mode")) {
    const Symbol mode = arg_symbol(args, 1);
    set_property(Symbol("mode"),
                 mode.valid() ? DataNode::Sym(mode) : DataNode());
    std::fprintf(stderr, "[gamecfg] mode=%s\n", mode.c_str());
    return true;
  }
  if (std::strcmp(m, "foreach_player_values") == 0) {
    auto values = std::make_shared<DataArray>();
    for (const auto& player : players_)
      values->push(DataNode::Obj(player.get()));
    out = DataNode::Array(values);
    return true;
  }
  if (std::strcmp(m, "get_player_config") == 0) {
    int i = args.size() ? args.at(0).as_int().value_or(0) : 0;
    if (i < 0 || i >= static_cast<int>(players_.size())) i = 0;
    out = DataNode::Obj(players_[i].get());
    return true;
  }
  if (std::strcmp(m, "get_bank_loader") == 0) {
    out = DataNode::Obj(this);
    return true;
  }
  if (std::strcmp(m, "reset") == 0) {
    return true;
  }
  if (std::strcmp(m, "set_character") == 0) {
    const Symbol outfit = arg_symbol(
        args, 0,
        node_symbol_or_string(get_property(Symbol("character_outfit"))));
    if (outfit.valid()) {
      set_property(Symbol("character_outfit"), DataNode::Sym(outfit));
      Symbol character =
          db_ ? db_->character_for_variant(outfit) : Symbol();
      if (!character.valid()) {
        std::string legacy = outfit.c_str();
        while (!legacy.empty() && legacy.back() >= '0' &&
               legacy.back() <= '9')
          legacy.pop_back();
        character = Symbol(legacy);
      }
      if (character.valid())
        set_property(Symbol("character"), DataNode::Sym(character));
    }
    return true;
  }
  if (std::strcmp(m, "get_difficulty") == 0 ||
      std::strcmp(m, "get_difficulty_sym") == 0) {
    int i = arg_int(args, 0, 0);
    if (i < 0 || i >= static_cast<int>(players_.size())) i = 0;
    const Symbol difficulty =
        node_difficulty_or_default(players_[i]->get_property(Symbol("difficulty")));
    out = DataNode::Sym(std::strcmp(m, "get_difficulty_sym") == 0
                            ? difficulty_display_symbol(difficulty)
                            : difficulty);
    return true;
  }
  if (std::strcmp(m, "set_difficulty") == 0) {
    int player = 0;
    std::size_t difficulty_arg = 0;
    if (args.size() > 1 && args.at(0).as_int()) {
      player = arg_int(args, 0, 0);
      difficulty_arg = 1;
    }
    if (player < 0 || player >= static_cast<int>(players_.size())) player = 0;
    Symbol difficulty = canonical_difficulty_symbol(
        arg_symbol(args, difficulty_arg, default_difficulty()));
    players_[player]->set_property(Symbol("difficulty"), DataNode::Sym(difficulty));
    if (player == 0)
      set_property(Symbol("difficulty"), DataNode::Sym(difficulty));
    return true;
  }
  if (std::strcmp(m, "set_quickplay") == 0) {
    set_property(Symbol("mode"), DataNode::Sym(Symbol("quickplay")));
    set_property(Symbol("quickplay"), DataNode::Sym(Symbol("TRUE")));
    if (mgr_) {
      if (Object* provider = mgr_->resolve_object(Symbol("song_provider"))) {
        DataArray enabled;
        enabled.push(DataNode::Int(1));
        provider->handle_property(Symbol("set_quickplay"), enabled);
      }
    }
    // The complete screen's SELECT SONG route returns to the Quickplay song
    // browser.  This value is normally populated by the retail game config
    // object, outside the UI scripts themselves.
    set_property(Symbol("continue_screen"),
                 DataNode::Sym(Symbol("qp_selsong_screen")));
    set_property(Symbol("win_screen"),
                 DataNode::Sym(Symbol("endgame_screen")));
    return true;
  }
  if (std::strcmp(m, "get_venue") == 0) {
    Symbol venue = current_venue_or_default(*this, db_);
    set_property(Symbol("venue"), DataNode::Sym(venue));
    out = DataNode::Sym(venue);
    return true;
  }
  if (std::strcmp(m, "set_venue") == 0) {
    Symbol venue = arg_symbol(args, 0, current_venue_or_default(*this, db_));
    if (!venue.valid() || (db_ && !db_->is_venue(venue)))
      venue = current_venue_or_default(*this, db_);
    set_property(Symbol("venue"), DataNode::Sym(venue));
    return true;
  }
  if (std::strcmp(m, "get_venue_index") == 0) {
    Symbol venue = current_venue_or_default(*this, db_);
    set_property(Symbol("venue"), DataNode::Sym(venue));
    const int index = db_ ? db_->venue_index(venue) : -1;
    out = DataNode::Int(index >= 0 ? index : 0);
    return true;
  }
  if (std::strcmp(m, "set_career_venue") == 0) {
    // SEL_VENUE_SCREEN_HANDLERS is shared by the career and multiplayer venue
    // screens.  Retail's set_career_venue chooses the venue for all of them,
    // but it must not erase the multiplayer mode selected immediately before
    // entering multi_*_venue_screen.
    const Symbol current_mode =
        get_property(Symbol("mode")).as_symbol().value_or(Symbol());
    const bool multiplayer_mode =
        std::strncmp(current_mode.c_str(), "multi_", 6) == 0;
    if (!multiplayer_mode)
      set_property(Symbol("mode"), DataNode::Sym(Symbol("career")));
    set_property(Symbol("quickplay"), DataNode::Sym(Symbol("FALSE")));
    if (mgr_) {
      if (Object* provider = mgr_->resolve_object(Symbol("song_provider"))) {
        DataArray disabled;
        disabled.push(DataNode::Int(0));
        provider->handle_property(Symbol("set_quickplay"), disabled);
      }
    }
    int status = 0;
    if (mgr_) {
      if (Object* campaign = mgr_->resolve_object(Symbol("campaign")))
        status =
            std::max(0, campaign->get_property(Symbol("status"))
                            .as_int()
                            .value_or(0));
    }
    Symbol venue =
        !multiplayer_mode && db_
            ? db_->campaign_venue_at(static_cast<std::size_t>(status))
            : Symbol();
    if (!venue.valid()) venue = current_venue_or_default(*this, db_);
    set_property(Symbol("venue"), DataNode::Sym(venue));
    out = DataNode::Sym(venue);
    return true;
  }

  // Multiplayer gating remains owned by stock main.dtb.  The host updates these
  // two properties from its physical controller slots; the script decides
  // whether main_multiplayer.btn is enabled.
  if (std::strcmp(m, "is_multiple_controllers") == 0) {
    out = DataNode::Sym(node_bool(get_property(Symbol("multiple_controllers")))
                            ? Symbol("TRUE")
                            : Symbol("FALSE"));
    return true;
  }
  if (std::strcmp(m, "is_missing_multi_controller") == 0) {
    out = DataNode::Sym(
        node_bool(get_property(Symbol("missing_multi_controller")))
            ? Symbol("TRUE")
            : Symbol("FALSE"));
    return true;
  }
  if (std::strcmp(m, "is_missing_controller") == 0) {
    out = DataNode::Sym(node_bool(get_property(Symbol("missing_controller")))
                            ? Symbol("TRUE")
                            : Symbol("FALSE"));
    return true;
  }
  if (std::strcmp(m, "multiplayer") == 0) {
    out = DataNode::Sym(node_bool(get_property(Symbol("multiplayer")))
                            ? Symbol("TRUE")
                            : Symbol("FALSE"));
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
  if (std::strcmp(m, "get_controller") == 0) {
    // Stock main.dta checks this against `guitar`; training.dta separately
    // checks for `joypad_guitar` to gate DualShock tutorial access.
    out = DataNode::Sym(Symbol("guitar"));
    return true;
  }
  if (std::strcmp(m, "set_tutorial_running") == 0) {
    set_property(Symbol("tutorial_running"),
                 args.size() ? args.at(0) : DataNode::Int(0));
    return true;
  }
  if (std::strcmp(m, "is_tutorial_running") == 0) {
    out = DataNode::Int(node_bool(get_property(Symbol("tutorial_running"))) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "video_provider") == 0) {
    out = DataNode::Obj(this);
    return true;
  }
  if (std::strcmp(m, "practice_section_provider") == 0) {
    out = mgr_ ? DataNode::Obj(mgr_->resolve_object(Symbol("section_provider")))
               : DataNode();
    return true;
  }
  if (std::strcmp(m, "get_video_file") == 0) {
    Symbol video = arg_symbol(args, 0);
    out = db_ ? db_->store_field(Symbol("video"), video, Symbol("file"))
              : DataNode();
    return true;
  }
  if (std::strcmp(m, "set_guitar") == 0) {
    Symbol guitar = arg_symbol(args, 0, get_property(Symbol("guitar"))
                                            .as_symbol()
                                            .value_or(Symbol()));
    if (!guitar.valid() && db_) guitar = db_->first_guitar();
    Symbol skin = arg_symbol(args, 1);
    if (!skin.valid() && db_) skin = db_->first_guitar_skin(guitar);
    if (guitar.valid()) set_property(Symbol("guitar"), DataNode::Sym(guitar));
    if (skin.valid()) set_property(Symbol("guitar_skin"), DataNode::Sym(skin));
    PersistentProfileState& profile = persistent_profile_state();
    profile.guitar = guitar.valid() ? guitar.c_str() : "";
    profile.guitar_skin = skin.valid() ? skin.c_str() : "";
    save_persistent_profile_state();
    return true;
  }
  if (std::strcmp(m, "get_guitar_desc") == 0) {
    Symbol guitar = arg_symbol(args, 0, get_property(Symbol("guitar"))
                                            .as_symbol()
                                            .value_or(Symbol()));
    if (!guitar.valid()) {
      out = DataNode();
      return true;
    }
    const Symbol desc(Symbol(std::string(guitar.c_str()) + "_desc"));
    const Symbol shop_desc(
        Symbol(std::string(guitar.c_str()) + "_shop_desc"));
    out = DataNode::Sym(
        mgr_ && mgr_->localize(desc) == desc.c_str() &&
                mgr_->localize(shop_desc) != shop_desc.c_str()
            ? shop_desc
            : desc);
    return true;
  }
  if (std::strcmp(m, "get_guitar_skin_desc") == 0) {
    Symbol skin = arg_symbol(args, 0, get_property(Symbol("guitar_skin"))
                                          .as_symbol()
                                          .value_or(Symbol()));
    if (!skin.valid()) {
      out = DataNode();
      return true;
    }
    const Symbol desc(Symbol(std::string(skin.c_str()) + "_desc"));
    out = DataNode::Sym(mgr_ && mgr_->localize(desc) == desc.c_str() &&
                               mgr_->localize(skin) != skin.c_str()
                           ? skin
                           : desc);
    return true;
  }
  if (std::strcmp(m, "get_num_guitars") == 0) {
    const Symbol type = arg_symbol(args, 0, Symbol("guitar"));
    out = DataNode::Int(
        static_cast<int>(owned_guitars(mgr_, db_, type).size()));
    return true;
  }
  if (std::strcmp(m, "get_guitar_at") == 0) {
    const Symbol type = arg_symbol(args, 0, Symbol("guitar"));
    const std::vector<Symbol> guitars = owned_guitars(mgr_, db_, type);
    const int index = std::clamp(arg_int(args, 1, 0), 0,
                                 std::max(0, static_cast<int>(guitars.size()) - 1));
    out = guitars.empty() ? DataNode()
                          : DataNode::Sym(guitars[static_cast<std::size_t>(index)]);
    return true;
  }
  if (std::strcmp(m, "get_guitar_skin_at") == 0) {
    const Symbol guitar = arg_symbol(args, 0);
    const int index = std::max(0, arg_int(args, 1, 0));
    const Symbol skin =
        db_ ? db_->guitar_skin_at(guitar, static_cast<std::size_t>(index))
            : Symbol();
    out = skin.valid() ? DataNode::Sym(skin) : DataNode();
    return true;
  }
  if (std::strcmp(m, "get_num_skins") == 0) {
    Symbol guitar = arg_symbol(args, 0, get_property(Symbol("guitar"))
                                            .as_symbol()
                                            .value_or(Symbol()));
    out = DataNode::Int(
        db_ ? static_cast<int>(db_->guitar_skin_count(guitar)) : 0);
    return true;
  }
  return false;
}

// --- Campaign --------------------------------------------------------------
Campaign::Campaign(ScreenManager* mgr, ConfigDb* db)
    : MetaObject(Symbol("campaign"), mgr, db) {
  const PersistentProfileState& profile = persistent_profile_state();
  set_property(Symbol("cash"), DataNode::Int(profile.cash));
  for (const std::string& item : profile.unlocked)
    set_property(Symbol(item), DataNode::Int(1));
}

bool Campaign::handle_meta(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();
  if (std::strcmp(m, "num_profiles") == 0) {
    int count = 0;
    for (const auto& profile : profiles_)
      if (profile.used) ++count;
    out = DataNode::Int(count);  // empty store -> 0 (real)
    return true;
  }
  if (std::strcmp(m, "set_profile_slot") == 0) {
    set_property(Symbol("profile_slot"), args.size() ? args.at(0) : DataNode::Int(-1));
    return true;
  }
  if (std::strcmp(m, "profile_slot") == 0) {
    out = get_property(Symbol("profile_slot"));
    if (!out.as_int()) out = DataNode::Int(-1);
    return true;
  }
  if (std::strcmp(m, "empty_slot") == 0) {
    int slot = 0;
    for (; slot < static_cast<int>(profiles_.size()); ++slot)
      if (!profiles_[slot].used) break;
    out = DataNode::Int(slot < 8 ? slot : -1);
    return true;
  }
  if (std::strcmp(m, "is_empty_profile") == 0) {
    const int slot = arg_int(args, 0, 0);
    out = DataNode::Int(slot < 0 || slot >= static_cast<int>(profiles_.size()) ||
                                !profiles_[slot].used
                            ? 1
                            : 0);
    return true;
  }
  if (std::strcmp(m, "profile_name") == 0) {
    const int slot = arg_int(args, 0, get_property(Symbol("profile_slot"))
                                          .as_int()
                                          .value_or(-1));
    if (slot >= 0 && slot < static_cast<int>(profiles_.size()) &&
        profiles_[slot].used)
      out = DataNode::Str(profiles_[slot].name);
    else
      out = DataNode::Str("");
    return true;
  }
  if (std::strcmp(m, "has_profile_name") == 0) {
    const std::string wanted = arg_text(args, 0);
    const bool editing = args.size() > 1 && node_bool(args.at(1));
    const int current_slot = get_property(Symbol("profile_slot"))
                                 .as_int()
                                 .value_or(-1);
    bool found = false;
    for (std::size_t i = 0; i < profiles_.size(); ++i) {
      if (!profiles_[i].used || profiles_[i].name != wanted) continue;
      if (editing && static_cast<int>(i) == current_slot) continue;
      found = true;
      break;
    }
    out = DataNode::Int(found ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "set_profile_name") == 0) {
    const std::string name = arg_text(args, 0);
    const int slot = arg_int(args, 1, get_property(Symbol("profile_slot"))
                                          .as_int()
                                          .value_or(-1));
    if (slot >= 0 && slot < 8) {
      if (profiles_.size() <= static_cast<std::size_t>(slot))
        profiles_.resize(static_cast<std::size_t>(slot) + 1);
      const bool was_empty = !profiles_[slot].used;
      profiles_[slot].used = !name.empty();
      profiles_[slot].name = name;
      set_property(Symbol("profile_slot"), DataNode::Int(slot));
      set_property(Symbol("profile_dirty"), DataNode::Int(1));
      set_property(Symbol("new_campaign"),
                   DataNode::Int(was_empty && !name.empty() ? 1 : 0));
    }
    return true;
  }
  if (std::strcmp(m, "delete_slot") == 0) {
    const int slot = arg_int(args, 0, get_property(Symbol("profile_slot"))
                                          .as_int()
                                          .value_or(-1));
    if (slot >= 0 && slot < static_cast<int>(profiles_.size())) {
      profiles_[slot] = Profile{};
      set_property(Symbol("last_deleted_slot"), DataNode::Int(slot));
      if (get_property(Symbol("profile_slot")).as_int().value_or(-1) == slot)
        set_property(Symbol("profile_slot"), DataNode::Int(-1));
      set_property(Symbol("profile_dirty"), DataNode::Int(1));
    }
    return true;
  }
  if (std::strcmp(m, "save_complete") == 0) {
    set_property(Symbol("profile_dirty"), DataNode::Int(0));
    set_property(Symbol("save_complete"), DataNode::Int(1));
    set_property(Symbol("save_complete_count"),
                 DataNode::Int(get_property(Symbol("save_complete_count"))
                                   .as_int()
                                   .value_or(0) +
                                1));
    save_persistent_profile_state();
    return true;
  }
  if (std::strcmp(m, "pick_attract_song") == 0) {
    Symbol song = campaign_pick_attract_song(*this, db_);
    out = song.valid() ? DataNode::Sym(song) : DataNode();
    return true;
  }
  if (std::strcmp(m, "last_difficulty") == 0) {
    out = DataNode::Sym(
        node_difficulty_or_default(get_property(Symbol("last_difficulty"))));
    return true;
  }
  if (std::strcmp(m, "update_difficulty") == 0) {
    Symbol difficulty = default_difficulty();
    if (mgr_) {
      if (Object* player0 = mgr_->resolve_object(Symbol("player0")))
        difficulty = node_difficulty_or_default(
            player0->get_property(Symbol("difficulty")));
    }
    set_property(Symbol("last_difficulty"), DataNode::Sym(difficulty));
    out = DataNode::Sym(difficulty);
    return true;
  }
  if (std::strcmp(m, "get_status_progress") == 0) {
    Symbol difficulty = arg_symbol(args, 0, node_difficulty_or_default(
                                                get_property(Symbol("last_difficulty"))));
    const int total = static_cast<int>(campaign_song_order(db_).size());
    const int beaten = campaign_beaten_count(*this, db_, difficulty);
    const int pct = total > 0 ? (beaten * 100) / total : 0;
    out = DataNode::Str(std::to_string(pct) + "%");
    return true;
  }
  if (std::strcmp(m, "career_score") == 0) {
    out = DataNode::Int(get_property(Symbol("career_score")).as_int().value_or(0));
    return true;
  }
  if (std::strcmp(m, "status") == 0) {
    out = DataNode::Int(get_property(Symbol("status")).as_int().value_or(0));
    return true;
  }
  if (std::strcmp(m, "is_max_status") == 0) {
    Symbol difficulty = node_difficulty_or_default(get_property(Symbol("last_difficulty")));
    const int status = get_property(Symbol("status")).as_int().value_or(0);
    out = DataNode::Int(status >= campaign_max_status(db_, difficulty) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "encore_unlock_potential") == 0) {
    Symbol difficulty = node_difficulty_or_default(get_property(Symbol("last_difficulty")));
    Symbol song = arg_symbol(args, 0, campaign_current_song(mgr_));
    out = DataNode::Int(
        campaign_encore_unlock_potential(*this, db_, difficulty, song) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "encore_newly_unlocked") == 0) {
    Symbol difficulty = node_difficulty_or_default(get_property(Symbol("last_difficulty")));
    Symbol song = arg_symbol(args, 0, campaign_current_song(mgr_));
    const bool unlocked =
        campaign_encore_newly_unlocked(*this, db_, difficulty, song);
    Symbol venue = campaign_venue_for_song(db_, song);
    Symbol encore = campaign_encore_for_venue(db_, venue);
    const bool freebird = unlocked && encore.valid() &&
                          encore == campaign_last_song(db_);
    set_property(Symbol("last_encore_newly_unlocked"),
                 DataNode::Int(unlocked ? 1 : 0));
    set_property(Symbol("last_encore_freebird"),
                 DataNode::Int(freebird ? 1 : 0));
    out = DataNode::Int(unlocked ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "num_guitar_awards") == 0) {
    out = DataNode::Int(campaign_pending_guitar_award_count(*this, db_));
    return true;
  }
  if (std::strcmp(m, "next_guitar_award") == 0) {
    Symbol award = campaign_next_guitar_award(*this, db_);
    out = award.valid() ? DataNode::Sym(award) : DataNode();
    return true;
  }
  if (std::strcmp(m, "beat_song") == 0 ||
      std::strcmp(m, "cheat_beat_song") == 0 ||
      std::strcmp(m, "cheat_gold_song") == 0) {
    Symbol song = arg_symbol(args, 0, campaign_current_song(mgr_));
    const int score = arg_int(args, 1, 0);
    Symbol difficulty = node_difficulty_or_default(get_property(Symbol("last_difficulty")));
    campaign_mark_song(*this, db_, difficulty, song, score);
    out = song.valid() ? DataNode::Sym(song) : DataNode();
    return true;
  }
  if (std::strcmp(m, "finish_song") == 0 ||
      std::strcmp(m, "finish_coop_song") == 0) {
    Symbol song = campaign_current_song(mgr_);
    const int score = arg_int(args, 0, 0);
    const int cash_award = arg_int(args, 1, 0);
    const int new_status = arg_int(args, 4, get_property(Symbol("status"))
                                               .as_int()
                                               .value_or(0));
    const int new_status_award = arg_int(args, 5, 0);
    Symbol difficulty = node_difficulty_or_default(get_property(Symbol("last_difficulty")));
    const int old_status = get_property(Symbol("status")).as_int().value_or(0);
    const bool already_won =
        node_bool(get_property(Symbol("won_campaign")));
    const bool newly_beaten =
        song.valid() && campaign_contains_song(db_, song) &&
        !campaign_song_beaten(*this, difficulty, song);
    campaign_mark_song(*this, db_, difficulty, song, score);
    set_property(Symbol("last_finished_song"),
                 song.valid() ? DataNode::Sym(song) : DataNode());
    set_property(Symbol("last_finished_newly_beaten"),
                 DataNode::Int(newly_beaten ? 1 : 0));
    if (cash_award || new_status_award) {
      const int next = get_property(Symbol("cash")).as_int().value_or(0) +
                       cash_award + new_status_award;
      set_property(Symbol("cash"), DataNode::Int(next));
    }
    if (new_status > old_status)
      set_property(Symbol("status"), DataNode::Int(new_status));
    if (song.valid() && song == campaign_last_song(db_) && !already_won) {
      set_property(Symbol("won_campaign"), DataNode::Int(1));
      if (new_status >= campaign_max_status(db_, difficulty))
        campaign_enqueue_guitar_awards(*this, db_, difficulty,
                                       Symbol("tour_passed"));
    }
    return true;
  }
  if (std::strcmp(m, "won_campaign") == 0) {
    out = DataNode::Int(node_bool(get_property(Symbol("won_campaign"))) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "final_song") == 0) {
    Symbol song = arg_symbol(args, 0, campaign_current_song(mgr_));
    out = DataNode::Int(song.valid() && song == campaign_last_song(db_) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "is_encore_song") == 0) {
    Symbol song = arg_symbol(args, 0, campaign_current_song(mgr_));
    out = DataNode::Int(campaign_encore_song(db_, song) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "is_store_song") == 0) {
    Symbol song = arg_symbol(args, 0, campaign_current_song(mgr_));
    out = DataNode::Int(db_ && song.valid() &&
                                !db_->store_field(Symbol("song"), song,
                                                  Symbol("price"))
                                     .empty()
                            ? 1
                            : 0);
    return true;
  }
  if (std::strcmp(m, "get_cur_encore") == 0) {
    Symbol venue;
    if (mgr_) {
      if (Object* game = mgr_->resolve_object(Symbol("game")))
        venue = node_symbol_or_string(game->handle_property(Symbol("get_venue"),
                                                           DataArray()));
    }
    Symbol encore = campaign_encore_for_venue(db_, venue);
    out = encore.valid() ? DataNode::Sym(encore) : DataNode();
    return true;
  }
  if (std::strcmp(m, "cash") == 0) {
    out = DataNode::Int(get_property(Symbol("cash")).as_int().value_or(0));
    return true;
  }
  if (std::strcmp(m, "starting_cash") == 0) {
    out = DataNode::Int(campaign_starting_cash(db_));
    return true;
  }
  if (std::strcmp(m, "add_cash") == 0) {
    const int next =
        get_property(Symbol("cash")).as_int().value_or(0) + arg_int(args, 0, 0);
    set_property(Symbol("cash"), DataNode::Int(next));
    persistent_profile_state().cash = std::max(0, next);
    save_persistent_profile_state();
    out = DataNode::Int(next);
    return true;
  }
  if (std::strcmp(m, "buy_item") == 0) {
    const Symbol item = arg_symbol(args, 0);
    const int fallback_price = store_price_for_item(db_, item);
    const int price = arg_int(args, 1, fallback_price);
    const int next =
        get_property(Symbol("cash")).as_int().value_or(0) - std::max(0, price);
    set_property(Symbol("cash"), DataNode::Int(next));
    if (item.valid()) set_property(item, DataNode::Int(1));
    PersistentProfileState& profile = persistent_profile_state();
    profile.cash = std::max(0, next);
    if (item.valid()) profile.unlocked.insert(item.c_str());
    save_persistent_profile_state();
    out = DataNode::Int(next);
    return true;
  }
  if (std::strcmp(m, "new_campaign") == 0) {
    out = DataNode::Int(node_bool(get_property(Symbol("new_campaign"))) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "tutorial_access") == 0) {
    out = DataNode::Int(node_bool(get_property(Symbol("tutorial_access"))) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "tutorials_done") == 0) {
    // Existing no-profile menu boot behavior treats tutorials as already done;
    // the profile-backed value can replace this when campaign storage lands.
    DataNode stored = get_property(Symbol("tutorials_done"));
    out = stored.empty() ? DataNode::Int(1)
                         : DataNode::Int(node_bool(stored) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "do_tutorial") == 0) {
    set_property(Symbol("tutorial"), args.size() ? args.at(0) : DataNode());
    set_property(Symbol("tutorials_done"), DataNode::Int(0));
    return true;
  }
  if (std::strcmp(m, "is_video_unlocked") == 0) {
    const Symbol video = arg_symbol(args, 0);
    const bool exists =
        db_ && !db_->store_field(Symbol("video"), video, Symbol("file")).empty();
    // Fresh campaign profile store has no purchased videos. If a later store
    // flow records video2/video3 on campaign, this query will surface it.
    out = DataNode::Int(exists && node_bool(get_property(video)) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "foreach_venue_values") == 0) {
    auto values = std::make_shared<DataArray>();
    if (db_) {
      for (Symbol venue : db_->venues())
        values->push(DataNode::Sym(venue));
    }
    out = DataNode::Array(values);
    return true;
  }
  if (std::strcmp(m, "foreach_venue") == 0) {
    return true;
  }
  if (std::strcmp(m, "is_unlocked") == 0) {
    const Symbol key = arg_symbol(args, 0);
    // TEMPORARY REVIEW CHEAT (user-requested): expose every authored song,
    // including the store's bonus tracks, so the complete setlist can be
    // scrolled and visually audited.
    // This is deliberately always on and has no command-line/environment
    // switch. Remove it after the setlist review is accepted.
    if (key.valid() && db_) {
      for (std::size_t i = 0; i < db_->song_count(); ++i) {
        if (db_->song_key(i) != key) continue;
        out = DataNode::Int(1);
        return true;
      }
    }
    if (key.valid() && db_ && db_->is_venue(key)) {
      out = DataNode::Int(1);
      return true;
    }
    out = DataNode::Int(key.valid() && node_bool(get_property(key)) ? 1 : 0);
    return true;
  }
  return false;
}

// --- registration ----------------------------------------------------------
void install_meta_singletons(ScreenManager& mgr, ConfigDb& db) {
  mgr.register_runtime_class(
      Symbol("StatsProvider"),
      [&mgr, &db](Symbol) -> std::unique_ptr<Object> {
        return std::make_unique<StatsProvider>(&mgr, &db);
      });
  if (Object* credits_screen = mgr.find_object(Symbol("credits_screen"))) {
    const DataArray* credits = db.table(Symbol("credits"));
    if (credits) {
      const int32_t credit_count = static_cast<int32_t>(credits->size());
      credits_screen->set_property(
          Symbol("num_lines"),
          DataNode::Int(credit_count));
      if (auto* credits_panel =
              dynamic_cast<ObjectDir*>(mgr.find_object(Symbol("credits_panel")))) {
        if (Object* credits_list = credits_panel->find_path("credits.lst")) {
          credits_list->set_property(Symbol("provider_num_data"),
                                     DataNode::Int(credit_count));
        }
      }
    }
  }

  auto game = std::make_unique<GameConfig>(&mgr, &db);
  GameConfig* g = game.get();
  mgr.add_singleton(Symbol("game"), std::move(game));
  mgr.alias_singleton(Symbol("gamecfg"), g);
  if (g->player(0)) {
    g->player(0)->set_name(Symbol("player0"));
    mgr.alias_singleton(Symbol("player0"), g->player(0));
  }
  if (g->player(1)) {
    g->player(1)->set_name(Symbol("player1"));
    mgr.alias_singleton(Symbol("player1"), g->player(1));
  }

  mgr.add_singleton(Symbol("campaign"), std::make_unique<Campaign>(&mgr, &db));
  mgr.add_singleton(Symbol("band"), std::make_unique<BandStats>(&mgr, &db));
  mgr.add_singleton(Symbol("highscores"),
                    std::make_unique<Highscores>(&mgr, &db));
  mgr.add_singleton(Symbol("tips"), std::make_unique<TipsObject>(&mgr, &db));
  mgr.add_singleton(Symbol("section_provider"),
                    std::make_unique<PracticeSectionProvider>(&mgr, &db));
  mgr.add_singleton(Symbol("song_provider"),
                    std::make_unique<SongProvider>(&mgr, &db));
  mgr.add_singleton(Symbol("character_provider"),
                    std::make_unique<CharacterProvider>(&mgr, &db));
  mgr.add_singleton(Symbol("store_item_provider"),
                    std::make_unique<StoreProvider>(
                        Symbol("store_item_provider"), &mgr, &db));
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
