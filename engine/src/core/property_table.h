// engine/src/core/property_table.h
//
// PropertyTable — a Symbol -> DataNode store with class-inheritance fallback.
//
// The decoded engine keeps each table's entries sorted by interned-symbol
// pointer and looks them up with a binary search (linear scan while tiny),
// falling back to a chained `parent` table on a miss — that is how class
// instances inherit default property values from their class prototype. See
// memory/subsystems/engine_plumbing.md, "PropertyTable struct layout" and
// "PropertyTable::Find algorithm". We keep that behavior with our own code.

#pragma once

#include "core/data_node.h"
#include "core/symbol.h"

#include <cstddef>
#include <vector>

namespace ghogx {

class PropertyTable {
 public:
  // Look up `key` in THIS table only (no parent walk). nullptr if absent.
  const DataNode* find_local(Symbol key) const;

  // Look up `key`, falling back along the parent chain (class-hierarchy
  // inheritance). nullptr if absent in this table and all ancestors.
  const DataNode* find(Symbol key) const;

  // Insert `value` under `key`, overwriting any existing entry. Keeps the
  // entry vector sorted by key identity so find_local stays a binary search.
  void set(Symbol key, DataNode value);

  // Chain this table to a parent (typically the class prototype). The parent
  // is borrowed; the caller owns its lifetime.
  void set_parent(const PropertyTable* parent) { parent_ = parent; }
  const PropertyTable* parent() const { return parent_; }

  std::size_t size() const { return entries_.size(); }

 private:
  struct Entry {
    Symbol   key;
    DataNode value;
  };
  // Sorted ascending by key (Symbol::operator<, i.e. interned pointer order).
  std::vector<Entry> entries_;
  const PropertyTable* parent_ = nullptr;
};

}  // namespace ghogx
