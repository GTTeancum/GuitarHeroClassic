// engine/src/ui/config_db.h
//
// ConfigDb -- loads the stock config/gen/*.dtb data tables (songs, guitars,
// store, campaign, gh2, credits, tips) the menu's game-side objects answer queries from. This
// is the DATA the original menus read, loaded verbatim via dtb_bridge -- so
// game-side answers (song titles, artists, guitar descriptions, ...) come from
// the real data, never canned constants.
//
// songs.dtb shape (verified): a list of records
//   (badreputation (name "Bad Reputation") (artist "Thin Lizzy") (song ...)
//                  (quickplay (character_outfit funk1)(guitar flying_v)(venue fest)) ...)
// so song N's title = songs[N] find_keyed "name" -> value.

#pragma once

#include "core/data_node.h"
#include "core/symbol.h"

#include "ark_v3.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ghogx::ui {

class ConfigDb {
 public:
  // Load the named config/gen DTBs from the ARK. Missing/unparsable files are
  // skipped (logged), so this never aborts.
  void load(const gh::ark::ArkV3Reader& ark, const std::vector<std::string>& ark_paths);
  // Replace only the song catalog from a mounted content archive while
  // retaining the front-end's guitars/store/campaign/UI configuration.
  void load_songs(const gh::ark::ArkV3Reader& ark,
                  const std::vector<std::string>& ark_paths);

  // A loaded table by name ("songs"/"guitars"/"store"/"campaign"/"gh2"/...), or null.
  const DataArray* table(Symbol name) const;

  // --- song table convenience (the most-queried) ---
  std::size_t song_count() const;
  const DataArray* song(std::size_t index) const;      // the Nth (key (...)...)
  Symbol song_key(std::size_t index) const;            // the Nth song's symbol key
  int song_index(Symbol song) const;                   // zero-based, -1 when absent
  DataNode song_field(std::size_t index, Symbol field) const;  // name/artist/...
  DataNode store_field(Symbol category, Symbol item, Symbol field) const;
  std::size_t store_item_count(Symbol category) const;
  Symbol store_item(Symbol category, std::size_t index) const;
  std::vector<Symbol> store_items(Symbol category) const;
  const std::vector<Symbol>& practice_sections() const { return practice_sections_; }

  // --- venue list (config/gen/gh2.dtb: (venues ...)) ---
  std::vector<Symbol> venues() const;
  std::size_t venue_count() const;
  bool is_venue(Symbol venue) const;
  int venue_index(Symbol venue) const;  // zero-based, -1 when absent
  Symbol default_venue() const;
  std::vector<Symbol> campaign_songs(Symbol venue) const;
  Symbol campaign_venue(Symbol song) const;
  Symbol campaign_venue_at(std::size_t tier_index) const;

  // --- guitar table convenience (config/gen/guitars.dtb) ---
  const DataArray* guitar(Symbol guitar) const;
  Symbol first_guitar(Symbol type = Symbol("guitar")) const;
  std::size_t guitar_skin_count(Symbol guitar) const;
  Symbol first_guitar_skin(Symbol guitar) const;
  const DataArray* guitar_skin(Symbol guitar, Symbol skin) const;
  DataNode guitar_skin_field(Symbol guitar, Symbol skin, Symbol field) const;
  Symbol guitar_for_skin(Symbol skin) const;

  // Generic keyed-record field: record's `find_keyed(field)` value (at(1)).
  static DataNode field(const DataArray* record, Symbol key);

 private:
  void load_practice_sections(const gh::ark::ArkV3Reader& ark,
                              const std::vector<std::string>& ark_paths);

  std::map<const void*, std::shared_ptr<DataArray>> tables_;
  std::vector<Symbol> practice_sections_;
};

}  // namespace ghogx::ui
