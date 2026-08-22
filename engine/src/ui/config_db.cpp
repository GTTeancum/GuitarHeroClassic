// engine/src/ui/config_db.cpp -- see config_db.h.

#include "ui/config_db.h"

#include "chart/midi_reader.h"
#include "dtb_bridge/dtb_bridge.h"

#include "dtb.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <string>
#include <string_view>

namespace ghogx::ui {

namespace {

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

bool starts_with(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() &&
         s.substr(0, prefix.size()) == prefix;
}

}  // namespace

void ConfigDb::load(const gh::ark::ArkV3Reader& ark, const std::vector<std::string>& ark_paths) {
  ark_ = ark;
  ark_paths_ = ark_paths;
  has_ark_ = true;
  song_duration_sec_.clear();
  static const struct { const char* name; const char* path; } kFiles[] = {
      {"songs", "config/gen/songs.dtb"},     {"guitars", "config/gen/guitars.dtb"},
      {"store", "config/gen/store.dtb"},     {"campaign", "config/gen/campaign.dtb"},
      {"gh2", "config/gen/gh2.dtb"},          {"credits", "config/gen/credits.dtb"},
      {"modes", "config/gen/modes.dtb"},
      {"ui", "ui/gen/ui.dtb"}};
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

int ConfigDb::song_duration_sec(Symbol song) const {
  if (!song.valid() || !has_ark_) return 0;
  const void* key_id = song.id();
  auto cached = song_duration_sec_.find(key_id);
  if (cached != song_duration_sec_.end()) return cached->second;

  int seconds = 0;
  try {
    const std::string key = song.c_str();
    const std::string midi_path = "songs/" + key + "/" + key + ".mid";
    auto entry = ark_.find(midi_path);
    if (entry) {
      auto bytes = ark_.read_entry(*entry, ark_paths_);
      const ghogx::chart::Chart chart = ghogx::chart::parse_midi(bytes);
      seconds = std::max(0, static_cast<int>(std::lround(chart.duration_sec())));
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[configdb] song duration %s: %s\n",
                 song.c_str(), ex.what());
  }
  song_duration_sec_[key_id] = seconds;
  return seconds;
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

    auto bytes = ark.read_entry(*entry, ark_paths);
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
      Symbol sym(name.c_str());
      if (std::find(practice_sections_.begin(), practice_sections_.end(), sym) ==
          practice_sections_.end()) {
        practice_sections_.push_back(sym);
      }
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[configdb] practice sections: %s\n", ex.what());
  }
}

}  // namespace ghogx::ui
