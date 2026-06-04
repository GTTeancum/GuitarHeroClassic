// engine/src/core/object.cpp

#include "core/object.h"

#include "core/class_reg.h"

namespace ghogx {

bool Object::is_a(Symbol query) const {
  return ClassReg::instance().is_a(class_name(), query);
}

void Object::bind_class_defaults() {
  props_.set_parent(ClassReg::instance().prototype(class_name()));
}

const DataNode* Object::find_property(Symbol key) const {
  return props_.find(key);  // instance table, then class-prototype chain
}

DataNode Object::get_property(Symbol key) const {
  DataNode out;
  if (get_property_override(key, out)) return out;
  if (const DataNode* n = find_property(key)) return *n;
  return DataNode();  // empty / unresolved
}

bool Object::has_property(Symbol key) const {
  DataNode tmp;
  return get_property_override(key, tmp) || find_property(key) != nullptr;
}

// The ~20 universal property messages every Sandbox object answers
// (engine_plumbing.md, "The 20 base Object property names"). Names are interned
// once into function-local statics and compared by identity.
DataNode Object::handle_property(Symbol msg, const DataArray& args) {
  static const Symbol kGet("get");
  static const Symbol kSet("set");
  static const Symbol kHas("has");
  static const Symbol kSize("size");
  static const Symbol kName("name");
  static const Symbol kSetName("set_name");
  static const Symbol kClassName("class_name");
  static const Symbol kGetType("get_type");
  static const Symbol kIsA("is_a");
  static const Symbol kNote("note");
  static const Symbol kSetNote("set_note");

  auto arg_sym = [&](std::size_t i) -> Symbol {
    if (i < args.size()) {
      if (auto s = args.at(i).as_symbol()) return *s;
    }
    return Symbol();
  };

  if (msg == kGet) {  // (get <key>) -> value
    return get_property(arg_sym(0));
  }
  if (msg == kSet) {  // (set <key> <value>) -> empty
    if (args.size() >= 2) set_property(arg_sym(0), args.at(1));
    return DataNode();
  }
  if (msg == kHas) {  // (has <key>) -> 0/1
    return DataNode::Int(has_property(arg_sym(0)) ? 1 : 0);
  }
  if (msg == kSize) {  // (size <key>) -> element count of an array property
    if (auto arr = get_property(arg_sym(0)).as_array()) {
      return DataNode::Int(static_cast<int32_t>(arr->size()));
    }
    return DataNode();
  }
  if (msg == kName) {  // -> instance name symbol
    return DataNode::Sym(name_);
  }
  if (msg == kSetName) {  // (set_name <sym>) -> empty
    set_name(arg_sym(0));
    return DataNode();
  }
  if (msg == kClassName || msg == kGetType) {  // -> class symbol
    return DataNode::Sym(class_name());
  }
  if (msg == kIsA) {  // (is_a <class>) -> 0/1
    return DataNode::Int(is_a(arg_sym(0)) ? 1 : 0);
  }
  if (msg == kNote) {  // dev annotation getter
    return get_property(kNote);
  }
  if (msg == kSetNote) {  // (set_note "...") -> empty
    if (args.size() >= 1) set_property(kNote, args.at(0));
    return DataNode();
  }

  // The remaining universal messages -- get_array, insert, remove, prop_handle,
  // copy, replace, iterate_refs, dir, set_type -- operate on the container /
  // object-tree layers that land later (scene graph, object refs). They are
  // intentionally not yet handled here; an unrecognized message yields empty.
  return DataNode();
}

}  // namespace ghogx
