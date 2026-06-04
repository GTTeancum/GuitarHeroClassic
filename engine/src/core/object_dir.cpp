// engine/src/core/object_dir.cpp

#include "core/object_dir.h"

#include "core/class_reg.h"

namespace ghogx {

Symbol ObjectDir::class_name() const { return Symbol("ObjectDir"); }

Object* ObjectDir::add(std::unique_ptr<Object> child) {
  if (!child) return nullptr;
  Symbol nm = child->name();
  if (!nm.valid()) return nullptr;  // children are keyed by name

  Object* raw = child.get();
  auto it = index_.find(nm.id());
  if (it != index_.end()) {
    entries_[it->second].obj = std::move(child);  // replace existing
  } else {
    index_[nm.id()] = entries_.size();
    entries_.push_back(Entry{nm, std::move(child)});
  }
  return raw;
}

Object* ObjectDir::add(Symbol name, std::unique_ptr<Object> child) {
  if (child) child->set_name(name);
  return add(std::move(child));
}

Object* ObjectDir::create_child(Symbol type, Symbol name) {
  std::unique_ptr<Object> obj = ClassReg::instance().create(type);
  if (!obj) return nullptr;  // class has no registered creator
  obj->set_name(name);
  return add(std::move(obj));
}

Object* ObjectDir::find(Symbol name) const {
  auto it = index_.find(name.id());
  if (it == index_.end()) return nullptr;
  return entries_[it->second].obj.get();
}

Object* ObjectDir::find_path(std::string_view path) const {
  // Split into non-empty '/'-separated components.
  std::vector<std::string_view> parts;
  std::size_t i = 0;
  while (i < path.size()) {
    std::size_t j = path.find('/', i);
    if (j == std::string_view::npos) j = path.size();
    if (j > i) parts.push_back(path.substr(i, j - i));
    i = j + 1;
  }
  if (parts.empty()) return nullptr;

  const ObjectDir* cur = this;
  for (std::size_t k = 0; k < parts.size(); ++k) {
    Object* child = cur->find(Symbol(parts[k]));
    if (!child) return nullptr;
    if (k + 1 == parts.size()) return child;  // final component
    // Must descend: the child has to be an ObjectDir.
    if (!child->is_a(Symbol("ObjectDir"))) return nullptr;
    cur = static_cast<const ObjectDir*>(child);
  }
  return nullptr;  // unreachable
}

}  // namespace ghogx
