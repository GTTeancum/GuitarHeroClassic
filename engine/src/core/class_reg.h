// engine/src/core/class_reg.h
//
// ClassReg — the Sandbox class registry.
//
// Maps a class Symbol to (a) its superclass Symbol and (b) its prototype
// PropertyTable, the table of default property values an instance inherits.
// The decoded engine resolves class -> prototype through a chain of thin
// lookups; class-hierarchy `is_a` checks walk the superclass links. See
// memory/subsystems/engine_plumbing.md, "Hmx::ClassReg".
//
// Single global registry, populated at engine init as classes are defined.

#pragma once

#include "core/property_table.h"
#include "core/symbol.h"

#include <functional>
#include <memory>
#include <unordered_map>

namespace ghogx {

class Object;

class ClassReg {
 public:
  // Factory that produces a default-constructed instance of a class.
  using Creator = std::function<std::unique_ptr<Object>()>;

  static ClassReg& instance();

  // Define class `cls`, optionally with a `super`class and a `prototype`
  // table of default properties. The prototype is borrowed -- the caller owns
  // its storage for the registry's lifetime. Re-defining updates in place
  // (preserving any creator already set).
  void define(Symbol cls, Symbol super = Symbol(),
              const PropertyTable* prototype = nullptr);

  // Set the instance factory for `cls` (defining the class if needed).
  void set_creator(Symbol cls, Creator creator);

  bool defined(Symbol cls) const;
  Symbol super_of(Symbol cls) const;
  const PropertyTable* prototype(Symbol cls) const;
  bool creatable(Symbol cls) const;

  // True if `cls` equals `query` or `query` is one of its ancestors.
  bool is_a(Symbol cls, Symbol query) const;

  // Instantiate `cls` via its creator and bind its class-prototype defaults.
  // Returns nullptr if the class has no creator registered.
  std::unique_ptr<Object> create(Symbol cls) const;

  // Test-only: forget all registrations.
  void clear();

 private:
  struct Info {
    Symbol super;
    const PropertyTable* prototype = nullptr;
    Creator creator;
  };
  // Keyed by the interned Symbol identity (stable pointer).
  std::unordered_map<const void*, Info> classes_;
};

}  // namespace ghogx
