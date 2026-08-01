// engine/src/core/symbol.h
//
// Symbol — an interned string identifier.
//
// In the Sandbox object model, property names, class names, and script
// message names are all Symbols. They are interned (each distinct spelling
// stored exactly once) so equality is a single pointer compare instead of a
// strcmp. The decoded engine never strcmp's a property name at a dispatch
// site — it compares interned pointers; the property tables are even kept
// sorted by interned-pointer value for binary search. See
// memory/subsystems/engine_plumbing.md, "Interned-string-as-Symbol equality".
//
// Reimplemented fresh for GuitarHeroOGX from that observed *behavior*; not
// derived from the reference binary's code.

#pragma once

#include <cstddef>
#include <string_view>

namespace ghogx {

class Symbol {
 public:
  // The null/empty symbol: valid() == false, c_str() == "".
  Symbol() = default;

  // Interns `text` and binds this Symbol to the pooled copy. Two Symbols
  // constructed from equal text share one pooled string and therefore
  // compare equal by pointer. Empty text yields the null symbol.
  explicit Symbol(std::string_view text);

  bool valid() const { return str_ != nullptr; }
  const char* c_str() const { return str_ ? str_ : ""; }

  // Stable interned address — usable as an identity / map key.
  const void* id() const { return str_; }

  bool operator==(const Symbol& o) const { return str_ == o.str_; }
  bool operator!=(const Symbol& o) const { return str_ != o.str_; }
  // Ordering by interned address: arbitrary but stable for the process, which
  // is all a sorted-by-pointer PropertyTable needs (it matches the decoded
  // engine, which orders entries by interned pointer value).
  bool operator<(const Symbol& o) const { return str_ < o.str_; }

 private:
  // Pool-owned, stable for the life of the process. null == empty symbol.
  const char* str_ = nullptr;
};

// Number of distinct strings currently interned (diagnostics / tests).
std::size_t symbol_pool_size();

}  // namespace ghogx
