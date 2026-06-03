// engine/src/ui/config_db.cpp -- see config_db.h.

#include "ui/config_db.h"

#include "dtb_bridge/dtb_bridge.h"

#include "dtb.h"

#include <cstdio>
#include <exception>

namespace ghogx::ui {

void ConfigDb::load(const gh::ark::ArkV3Reader& ark, const std::vector<std::string>& ark_paths) {
  static const struct { const char* name; const char* path; } kFiles[] = {
      {"songs", "config/gen/songs.dtb"},     {"guitars", "config/gen/guitars.dtb"},
      {"store", "config/gen/store.dtb"},     {"campaign", "config/gen/campaign.dtb"},
      {"gh2", "config/gen/gh2.dtb"}};
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

DataNode ConfigDb::field(const DataArray* record, Symbol key) {
  if (!record) return DataNode();
  auto kv = record->find_keyed(key);
  return (kv && kv->size() > 1) ? kv->at(1) : DataNode();
}

}  // namespace ghogx::ui
