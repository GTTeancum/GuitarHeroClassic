// engine/src/core/property_table.cpp

#include "core/property_table.h"

#include <algorithm>

namespace ghogx {

const DataNode* PropertyTable::find_local(Symbol key) const {
  // entries_ is sorted by key; binary search on key identity.
  auto it = std::lower_bound(
      entries_.begin(), entries_.end(), key,
      [](const Entry& e, Symbol k) { return e.key < k; });
  if (it != entries_.end() && it->key == key) {
    return &it->value;
  }
  return nullptr;
}

const DataNode* PropertyTable::find(Symbol key) const {
  for (const PropertyTable* t = this; t != nullptr; t = t->parent_) {
    if (const DataNode* n = t->find_local(key)) {
      return n;
    }
  }
  return nullptr;
}

void PropertyTable::set(Symbol key, DataNode value) {
  auto it = std::lower_bound(
      entries_.begin(), entries_.end(), key,
      [](const Entry& e, Symbol k) { return e.key < k; });
  if (it != entries_.end() && it->key == key) {
    it->value = std::move(value);  // overwrite existing
  } else {
    entries_.insert(it, Entry{key, std::move(value)});  // keep sorted
  }
}

}  // namespace ghogx
