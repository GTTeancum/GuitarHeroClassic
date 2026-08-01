// engine/src/core/class_reg.cpp

#include "core/class_reg.h"

#include "core/object.h"

namespace ghogx {

ClassReg& ClassReg::instance() {
  static ClassReg reg;
  return reg;
}

void ClassReg::define(Symbol cls, Symbol super, const PropertyTable* prototype) {
  if (!cls.valid()) return;
  Info& info = classes_[cls.id()];  // preserves an existing creator
  info.super = super;
  info.prototype = prototype;
}

void ClassReg::set_creator(Symbol cls, Creator creator) {
  if (!cls.valid()) return;
  classes_[cls.id()].creator = std::move(creator);
}

bool ClassReg::creatable(Symbol cls) const {
  auto it = classes_.find(cls.id());
  return it != classes_.end() && static_cast<bool>(it->second.creator);
}

std::unique_ptr<Object> ClassReg::create(Symbol cls) const {
  auto it = classes_.find(cls.id());
  if (it == classes_.end() || !it->second.creator) return nullptr;
  std::unique_ptr<Object> obj = it->second.creator();
  if (obj) obj->bind_class_defaults();  // chain instance props to class defaults
  return obj;
}

bool ClassReg::defined(Symbol cls) const {
  return classes_.find(cls.id()) != classes_.end();
}

Symbol ClassReg::super_of(Symbol cls) const {
  auto it = classes_.find(cls.id());
  return it == classes_.end() ? Symbol() : it->second.super;
}

const PropertyTable* ClassReg::prototype(Symbol cls) const {
  auto it = classes_.find(cls.id());
  return it == classes_.end() ? nullptr : it->second.prototype;
}

bool ClassReg::is_a(Symbol cls, Symbol query) const {
  // Walk the superclass chain from `cls` upward, looking for `query`.
  for (Symbol c = cls; c.valid(); c = super_of(c)) {
    if (c == query) return true;
    if (!defined(c)) break;  // undefined class -> chain ends here
  }
  return false;
}

void ClassReg::clear() { classes_.clear(); }

}  // namespace ghogx
