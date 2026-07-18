// engine/src/ui/config_db.cpp -- see config_db.h.

#include "ui/config_db.h"

#include "chart/midi_reader.h"
#include "dtb_bridge/dtb_bridge.h"

#include "dtb.h"

#include <algorithm>
#include <cstdio>
#include <exception>
#include <string>
#include <string_view>

namespace ghogx::ui {

namespace {

bool starts_with(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

Symbol first_campaign_song(const ConfigDb& db) {
  const DataArray* campaign = db.table(Symbol("campaign"));
  auto order = campaign ? campaign->find_keyed(Symbol("order")) : nullptr;
  if (!order) return Symbol("shoutatthedevil");
  for (std::size_t i = 1; i < order->size(); ++i) {
    auto tier = order->at(i).as_array();
    if (!tier || tier->size() < 2) continue;
    Symbol song = tier->at(1).as_symbol().value_or(Symbol());
    if (song.valid()) return song;
  }
  return Symbol("shoutatthedevil");
}

}  // namespace

void ConfigDb::load(const gh::ark::ArkV3Reader& ark, const std::vector<std::string>& ark_paths) {
  static const struct { const char* name; const char* path; } kFiles[] = {
      {"songs", "config/gen/songs.dtb"},     {"guitars", "config/gen/guitars.dtb"},
      {"store", "config/gen/store.dtb"},     {"campaign", "config/gen/campaign.dtb"},
      {"gh2", "config/gen/gh2.dtb"},         {"credits", "config/gen/credits.dtb"},
      {"tips", "config/gen/tips.dtb"}};
  for (const auto& f : kFiles) {
    try {
      auto entry = ark.find(f.path);
      if (!entry) continue;
      std::vector<uint8_t> bytes = ark.read_entry(*entry, ark_paths);
      gh::dtb::Tree tree = gh::dtb::parse(bytes);
      tables_[Symbol(f.name).id()] = dtb_bridge::from_tree(tree);
    } catch (const std::exception& ex) {
      std::fprintf(stderr, "[configdb] %s: %s\n", f.path, ex.what());
    }
  }
  load_practice_sections(ark, ark_paths);
}

const DataArray* ConfigDb::table(Symbol name) const {
  auto it = tables_.find(name.id());
  return it == tables_.end() ? nullptr : it->second.get();
}

std::size_t ConfigDb::song_count() const {
  const DataArray* t = table(Symbol("songs"));
  return t ? t->size() : 0;
}

const DataArray* ConfigDb::song(std::size_t index) const {
  const DataArray* t = table(Symbol("songs"));
  if (!t || index >= t->size()) return nullptr;
  // The record's DataArray is owned by the table; .get() stays valid while this
  // ConfigDb is alive.
  return t->at(index).as_array().get();
}

Symbol ConfigDb::song_key(std::size_t index) const {
  const DataArray* rec = song(index);
  if (!rec || rec->size() == 0) return Symbol();
  return rec->at(0).as_symbol().value_or(Symbol());
}

DataNode ConfigDb::song_field(std::size_t index, Symbol field_name) const {
  return field(song(index), field_name);
}

DataNode ConfigDb::store_field(Symbol category, Symbol item, Symbol field_name) const {
  const DataArray* store = table(Symbol("store"));
  if (!store) return DataNode();
  auto category_record = store->find_keyed(category);
  if (!category_record || category_record->size() <= 1) return DataNode();
  auto item_record = category_record->find_keyed(item);
  return field(item_record.get(), field_name);
}

std::vector<Symbol> ConfigDb::store_items(Symbol category) const {
  std::vector<Symbol> out;
  const DataArray* store = table(Symbol("store"));
  if (!store) return out;
  auto category_record = store->find_keyed(category);
  if (!category_record || category_record->size() <= 1) return out;
  for (std::size_t i = 1; i < category_record->size(); ++i) {
    auto item = category_record->at(i).as_array();
    if (!item || item->empty()) continue;
    Symbol key = item->at(0).as_symbol().value_or(Symbol());
    if (key.valid()) out.push_back(key);
  }
  return out;
}

std::size_t ConfigDb::store_item_count(Symbol category) const {
  return store_items(category).size();
}

Symbol ConfigDb::store_item(Symbol category, std::size_t index) const {
  const auto items = store_items(category);
  return index < items.size() ? items[index] : Symbol();
}

std::vector<Symbol> ConfigDb::venues() const {
  std::vector<Symbol> out;
  const DataArray* gh2 = table(Symbol("gh2"));
  if (!gh2) return out;
  auto row = gh2->find_keyed(Symbol("venues"));
  if (!row || row->size() <= 1) return out;
  for (std::size_t i = 1; i < row->size(); ++i) {
    Symbol key = row->at(i).as_symbol().value_or(Symbol());
    if (key.valid()) out.push_back(key);
  }
  return out;
}

std::size_t ConfigDb::venue_count() const {
  return venues().size();
}

bool ConfigDb::is_venue(Symbol venue) const {
  if (!venue.valid()) return false;
  const auto list = venues();
  return std::find(list.begin(), list.end(), venue) != list.end();
}

int ConfigDb::venue_index(Symbol venue) const {
  const auto list = venues();
  for (std::size_t i = 0; i < list.size(); ++i)
    if (list[i] == venue) return static_cast<int>(i);
  return -1;
}

Symbol ConfigDb::default_venue() const {
  const Symbol stock_main_default("small2");
  if (is_venue(stock_main_default)) return stock_main_default;
  const auto list = venues();
  return list.empty() ? Symbol() : list.front();
}

const DataArray* ConfigDb::guitar(Symbol guitar_name) const {
  const DataArray* guitars = table(Symbol("guitars"));
  if (!guitars) return nullptr;
  auto record = guitars->find_keyed(guitar_name);
  return record.get();
}

Symbol ConfigDb::first_guitar(Symbol type) const {
  const DataArray* guitars = table(Symbol("guitars"));
  if (!guitars) return Symbol();
  for (std::size_t i = 0; i < guitars->size(); ++i) {
    auto record = guitars->at(i).as_array();
    if (!record || record->empty()) continue;
    Symbol key = record->at(0).as_symbol().value_or(Symbol());
    if (!key.valid()) continue;
    if (type.valid()) {
      Symbol record_type = field(record.get(), Symbol("type"))
                               .as_symbol()
                               .value_or(Symbol());
      if (record_type != type) continue;
    }
    return key;
  }
  return Symbol();
}

std::size_t ConfigDb::guitar_skin_count(Symbol guitar_name) const {
  const DataArray* record = guitar(guitar_name);
  if (!record) return 0;
  auto skins = record->find_keyed(Symbol("skins"));
  if (!skins || skins->size() <= 1) return 0;
  std::size_t count = 0;
  for (std::size_t i = 1; i < skins->size(); ++i) {
    auto skin = skins->at(i).as_array();
    if (skin && !skin->empty() && skin->at(0).as_symbol()) ++count;
  }
  return count;
}

Symbol ConfigDb::first_guitar_skin(Symbol guitar_name) const {
  const DataArray* record = guitar(guitar_name);
  if (!record) return Symbol();
  auto skins = record->find_keyed(Symbol("skins"));
  if (!skins) return Symbol();
  for (std::size_t i = 1; i < skins->size(); ++i) {
    auto skin = skins->at(i).as_array();
    if (skin && !skin->empty()) {
      Symbol key = skin->at(0).as_symbol().value_or(Symbol());
      if (key.valid()) return key;
    }
  }
  return Symbol();
}

const DataArray* ConfigDb::guitar_skin(Symbol guitar_name, Symbol skin_name) const {
  const DataArray* record = guitar(guitar_name);
  if (!record) return nullptr;
  auto skins = record->find_keyed(Symbol("skins"));
  if (!skins) return nullptr;
  auto skin = skins->find_keyed(skin_name);
  return skin.get();
}

DataNode ConfigDb::guitar_skin_field(Symbol guitar_name, Symbol skin_name,
                                     Symbol field_name) const {
  return field(guitar_skin(guitar_name, skin_name), field_name);
}

Symbol ConfigDb::guitar_for_skin(Symbol skin_name) const {
  const DataArray* guitars = table(Symbol("guitars"));
  if (!guitars || !skin_name.valid()) return Symbol();
  for (std::size_t i = 0; i < guitars->size(); ++i) {
    auto record = guitars->at(i).as_array();
    if (!record || record->empty()) continue;
    Symbol guitar_key = record->at(0).as_symbol().value_or(Symbol());
    if (!guitar_key.valid()) continue;
    if (guitar_skin(guitar_key, skin_name)) return guitar_key;
  }
  return Symbol();
}

DataNode ConfigDb::field(const DataArray* record, Symbol key) {
  if (!record) return DataNode();
  auto kv = record->find_keyed(key);
  return (kv && kv->size() > 1) ? kv->at(1) : DataNode();
}

void ConfigDb::load_practice_sections(
    const gh::ark::ArkV3Reader& ark,
    const std::vector<std::string>& ark_paths) {
  practice_sections_.clear();
  practice_sections_.push_back(Symbol("full_song"));

  try {
    const Symbol song = first_campaign_song(*this);
    const std::string key = song.c_str();
    const std::string midi_path = "songs/" + key + "/" + key + ".mid";
    auto entry = ark.find(midi_path);
    if (!entry) return;

    const std::vector<uint8_t> bytes = ark.read_entry(*entry, ark_paths);
    const ghogx::chart::Chart chart = ghogx::chart::parse_midi(bytes);
    for (const auto& ev : chart.text_events) {
      constexpr std::string_view kPrefix = "[section ";
      if (!starts_with(ev.text, kPrefix) || ev.text.empty() ||
          ev.text.back() != ']') {
        continue;
      }
      std::string name = ev.text.substr(kPrefix.size());
      name.pop_back();
      if (name.empty()) continue;
      Symbol section(name.c_str());
      if (std::find(practice_sections_.begin(), practice_sections_.end(),
                    section) == practice_sections_.end()) {
        practice_sections_.push_back(section);
      }
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[configdb] practice sections: %s\n", ex.what());
  }
}

}  // namespace ghogx::ui
