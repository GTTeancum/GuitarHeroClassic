// engine/src/ui/config_db.h
//
// ConfigDb -- loads the stock config/gen/*.dtb data tables (songs, guitars,
// store, campaign, gh2) the menu's game-side objects answer queries from. This
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

  // A loaded table by name ("songs"/"guitars"/"store"/"campaign"/"gh2"), or null.
  const DataArray* table(Symbol name) const;

  // --- song table convenience (the most-queried) ---
  std::size_t song_count() const;
  const DataArray* song(std::size_t index) const;      // the Nth (key (...)...)
  Symbol song_key(std::size_t index) const;            // the Nth song's symbol key
  DataNode song_field(std::size_t index, Symbol field) const;  // name/artist/...

  // Generic keyed-record field: record's `find_keyed(field)` value (at(1)).
  static DataNode field(const DataArray* record, Symbol key);

 private:
  std::map<const void*, std::shared_ptr<DataArray>> tables_;
};

}  // namespace ghogx::ui
