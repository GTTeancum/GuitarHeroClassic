// engine/src/core/data_node.cpp

#include "core/data_node.h"

namespace ghogx {

DataNode DataNode::Int(int32_t v) {
  DataNode n;
  n.type_ = DataType::kInt;
  n.int_ = v;
  return n;
}

DataNode DataNode::Float(float v) {
  DataNode n;
  n.type_ = DataType::kFloat;
  n.float_ = v;
  return n;
}

DataNode DataNode::Str(std::string_view v) {
  DataNode n;
  n.type_ = DataType::kString;
  n.sym_ = Symbol(v);  // string values interned for storage; see header note
  return n;
}

DataNode DataNode::Sym(Symbol s) {
  DataNode n;
  n.type_ = DataType::kSymbol;
  n.sym_ = s;
  return n;
}

DataNode DataNode::Array(std::shared_ptr<DataArray> a) {
  DataNode n;
  n.type_ = DataType::kArray;
  n.arr_ = std::move(a);
  return n;
}

DataNode DataNode::Obj(Object* o) {
  DataNode n;
  n.type_ = DataType::kObject;
  n.obj_ = o;  // non-owning
  return n;
}

std::optional<int32_t> DataNode::as_int() const {
  if (type_ == DataType::kInt) return int_;
  return std::nullopt;
}

std::optional<float> DataNode::as_float() const {
  if (type_ == DataType::kFloat) return float_;
  // The engine coerces an int DataNode to float on a float read; mirror that.
  if (type_ == DataType::kInt) return static_cast<float>(int_);
  return std::nullopt;
}

std::optional<Symbol> DataNode::as_symbol() const {
  if (type_ == DataType::kSymbol) return sym_;
  return std::nullopt;
}

std::optional<std::string_view> DataNode::as_string() const {
  // Both string values and symbols expose their text here.
  if (type_ == DataType::kString || type_ == DataType::kSymbol) {
    return std::string_view(sym_.c_str());
  }
  return std::nullopt;
}

std::shared_ptr<DataArray> DataNode::as_array() const {
  if (type_ == DataType::kArray) return arr_;
  return nullptr;
}

Object* DataNode::as_object() const {
  if (type_ == DataType::kObject) return obj_;
  return nullptr;
}

std::shared_ptr<DataArray> DataArray::find_keyed(Symbol key) const {
  for (const auto& n : nodes_) {
    auto child = n.as_array();
    if (!child || child->empty()) continue;
    if (auto head = child->at(0).as_symbol(); head && *head == key) {
      return child;
    }
  }
  return nullptr;
}

}  // namespace ghogx
