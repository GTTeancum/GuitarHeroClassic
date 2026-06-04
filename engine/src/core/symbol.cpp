// engine/src/core/symbol.cpp

#include "core/symbol.h"

#include <string>
#include <unordered_set>

namespace ghogx {
namespace {

// Process-lifetime intern pool.
//
// std::unordered_set is node-based: inserting (and rehashing) never moves the
// stored std::string objects, so the char* returned by each element's c_str()
// is stable for the life of the process. That stability is exactly what makes
// Symbol pointer-identity valid. The engine is single-threaded, so no lock.
std::unordered_set<std::string>& pool() {
  static std::unordered_set<std::string> p;
  return p;
}

}  // namespace

Symbol::Symbol(std::string_view text) {
  if (text.empty()) {
    str_ = nullptr;
    return;
  }
  // emplace returns the existing element on a duplicate, so equal text always
  // resolves to the same pooled string (and the same c_str() pointer).
  str_ = pool().emplace(text).first->c_str();
}

std::size_t symbol_pool_size() { return pool().size(); }

}  // namespace ghogx
