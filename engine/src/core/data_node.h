// engine/src/core/data_node.h
//
// DataNode — the engine's small tagged value type — and DataArray, a list of
// DataNodes (the Sandbox script array).
//
// In the reference binary a DataNode is a raw 8-byte {payload, type} tagged
// union with no vtable (engine_plumbing.md, "DataNode struct layout"). We are
// a fresh engine on a different machine, so we are NOT bound to that byte
// layout. We keep the *semantics* — a small, copyable, type-tagged value — in
// an idiomatic form. (Compaction toward the 8-byte form is a later concern for
// the OG-Xbox 64 MB memory budget; clarity first.)
//
// The numeric DataType values mirror the engine's runtime type IDs so the
// future DTB-tag -> runtime bridge stays a straight mapping.

#pragma once

#include "core/symbol.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace ghogx {

class DataArray;
class Object;

enum class DataType : uint8_t {
  kEmpty  = 0,
  kFloat  = 1,   // float literal
  kInt    = 2,   // integer literal
  kString = 5,   // string value
  kArray  = 6,   // nested DataArray
  kObject = 17,  // reference to an Object (non-owning)
  kSymbol = 19,  // interned symbol — property / class / message names
};

class DataNode {
 public:
  DataNode() = default;

  static DataNode Int(int32_t v);
  static DataNode Float(float v);
  static DataNode Str(std::string_view v);
  static DataNode Sym(Symbol s);
  static DataNode Array(std::shared_ptr<DataArray> a);
  static DataNode Obj(Object* o);  // non-owning reference

  DataType type() const { return type_; }
  bool empty() const { return type_ == DataType::kEmpty; }

  // Typed accessors. Return nullopt / nullptr when the stored type does not
  // match (with the few coercions the engine itself performs).
  std::optional<int32_t> as_int() const;            // kInt only
  std::optional<float>   as_float() const;          // kFloat, or kInt -> float
  std::optional<Symbol>  as_symbol() const;         // kSymbol only
  std::optional<std::string_view> as_string() const;// kString or kSymbol text
  std::shared_ptr<DataArray> as_array() const;      // kArray only
  Object* as_object() const;                        // kObject, else nullptr

 private:
  DataType type_ = DataType::kEmpty;
  int32_t  int_ = 0;
  float    float_ = 0.0f;
  Symbol   sym_;                     // kSymbol; also backs kString text
  std::shared_ptr<DataArray> arr_;   // kArray
  Object*  obj_ = nullptr;           // kObject (non-owning)
};

class DataArray {
 public:
  std::size_t size() const { return nodes_.size(); }
  bool empty() const { return nodes_.empty(); }
  const DataNode& at(std::size_t i) const { return nodes_.at(i); }
  DataNode& at(std::size_t i) { return nodes_.at(i); }
  void push(DataNode n) { nodes_.push_back(std::move(n)); }

  // Harmonix keyed-array convention: a child whose first element is the
  // symbol `key`, e.g. (artist "Skid Row") found by key "artist". Returns the
  // child sub-array, or nullptr if no such child exists.
  std::shared_ptr<DataArray> find_keyed(Symbol key) const;

 private:
  std::vector<DataNode> nodes_;
};

}  // namespace ghogx
