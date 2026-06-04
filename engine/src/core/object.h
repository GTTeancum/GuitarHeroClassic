// engine/src/core/object.h
//
// Object — the scripted-object base class every Sandbox class inherits from
// (Character, Cam, Light, gem, TrackWidget, Beatmatch, ...).
//
// Three idioms from the decoded engine (engine_plumbing.md, "Engine vtable
// patterns to mirror in the port") are reproduced here as plain C++ virtuals:
//
//   1. Polymorphic GET with class-inheritance fallback. A subclass gets first
//      shot via get_property_override(); the base then looks in the instance
//      PropertyTable, which chains to the class prototype's defaults.
//   2. Polymorphic HANDLE with a universal-message switch. handle_property()
//      dispatches the class's own messages, falling through to the base for
//      the ~20 universal property messages (get/set/has/name/...).
//   3. Interned-Symbol equality. Message and property names are Symbols, never
//      strcmp'd at a dispatch site.
//
// Per-frame ticking is a virtual update(float dt) the Scheduler calls.

#pragma once

#include "core/data_node.h"
#include "core/property_table.h"
#include "core/symbol.h"

namespace ghogx {

class Object {
 public:
  virtual ~Object() = default;

  // Each concrete class returns its registered class Symbol. Drives is_a() and
  // the class-prototype property fallback.
  virtual Symbol class_name() const = 0;

  // Instance name ("name" / "set_name").
  Symbol name() const { return name_; }
  void set_name(Symbol n) { name_ = n; }

  // True if this object's class is, or derives from, `query` (via ClassReg).
  bool is_a(Symbol query) const;

  // --- property access -----------------------------------------------------
  // Table-only lookup (instance table -> class prototype chain), no subclass
  // override. Returns a pointer into the resolving table, or nullptr.
  const DataNode* find_property(Symbol key) const;

  // Full resolution: subclass override -> instance table -> class prototype
  // chain. Returns the value by copy (computed overrides have no stable
  // storage), or an empty DataNode if unresolved.
  DataNode get_property(Symbol key) const;

  void set_property(Symbol key, DataNode v) { props_.set(key, std::move(v)); }
  bool has_property(Symbol key) const;

  PropertyTable& props() { return props_; }
  const PropertyTable& props() const { return props_; }

  // Bind the instance table's fallback to this class's prototype defaults
  // (looked up in ClassReg by class_name()). Call once after construction.
  void bind_class_defaults();

  // --- polymorphic hooks ---------------------------------------------------
  // Subclass computed getter; return true and fill `out` if handled.
  virtual bool get_property_override(Symbol key, DataNode& out) const {
    (void)key;
    (void)out;
    return false;
  }

  // Message / property dispatch. Subclasses handle their own message Symbols
  // and call Object::handle_property() for anything they don't recognize; the
  // base implements the universal messages. Returns an empty DataNode for
  // messages with no value result (or that are not yet implemented).
  virtual DataNode handle_property(Symbol msg, const DataArray& args);

  // Per-frame tick (Scheduler-driven). Default: no-op.
  virtual void update(float dt) { (void)dt; }

 protected:
  Symbol name_;
  PropertyTable props_;  // instance properties; parent chained to class defaults
};

}  // namespace ghogx
