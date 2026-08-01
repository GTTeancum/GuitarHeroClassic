// engine/src/core/object_dir.h
//
// ObjectDir — a directory of named child Objects; the runtime home a MILO
// scene (or a DTB object set) loads into.
//
// A MILO's decompressed payload is exactly this: dir_version, a dir_type
// ("ObjectDir" / "WorldDir" / ...), a dir_name, then a list of (type-string,
// name-string) entries whose bodies each become an Object of class `type`,
// named `name`, owned by the dir and cross-referenced by name. See
// memory/subsystems/milo_format.md, "Decompressed payload — object directory".
//
// This is the container + lookup half: own named children, find by name, find
// by "a/b/c" path descending through nested dirs, and create a child of a
// named class via the ClassReg factory (the MILO-load step, minus the
// per-class binary body read which is a later phase). Reimplemented fresh.

#pragma once

#include "core/object.h"
#include "core/symbol.h"

#include <cstddef>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ghogx {

class ObjectDir : public Object {
 public:
  Symbol class_name() const override;  // "ObjectDir"

  // MILO dir metadata. The dir's name() is the dir_name.
  Symbol dir_type() const { return dir_type_; }
  void set_dir_type(Symbol t) { dir_type_ = t; }

  // Take ownership of `child`, keyed by its name() (which must be valid).
  // A name collision replaces the previous child. Returns the raw pointer, or
  // nullptr if `child` is null or unnamed.
  Object* add(std::unique_ptr<Object> child);
  // Same, naming the child explicitly first.
  Object* add(Symbol name, std::unique_ptr<Object> child);

  // Instantiate a child of class `type` named `name` via the ClassReg factory
  // and add it. Returns nullptr if `type` has no registered creator.
  Object* create_child(Symbol type, Symbol name);

  // Direct child by name; nullptr if absent.
  Object* find(Symbol name) const;

  // Descend a "a/b/c" path: each non-final component must resolve to a nested
  // ObjectDir. Empty components are ignored. nullptr if any step is missing or
  // a non-dir is traversed.
  Object* find_path(std::string_view path) const;

  std::size_t size() const { return entries_.size(); }
  Object* at(std::size_t i) const { return entries_[i].obj.get(); }  // insertion order

 private:
  struct Entry {
    Symbol name;
    std::unique_ptr<Object> obj;
  };
  std::vector<Entry> entries_;                          // owns children, in order
  std::unordered_map<const void*, std::size_t> index_;  // name id -> entries_ index
  Symbol dir_type_;
};

}  // namespace ghogx
