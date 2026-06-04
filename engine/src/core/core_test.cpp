// engine/src/core/core_test.cpp
//
// Self-contained unit test for the runtime core (Symbol, DataNode, DataArray,
// PropertyTable). No test framework dependency — a tiny CHECK macro that
// tallies failures and returns nonzero so CTest reports pass/fail.

#include "core/data_node.h"
#include "core/property_table.h"
#include "core/symbol.h"

#include <cstdio>
#include <memory>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      std::fprintf(stderr, "FAIL %s:%d  CHECK(%s)\n", __FILE__, __LINE__, \
                   #cond);                                                \
      ++g_failures;                                                       \
    }                                                                     \
  } while (0)

using namespace ghogx;

void test_symbol_interning() {
  Symbol a("track_speed");
  Symbol b("track_speed");
  Symbol c("hopo_threshold");

  // Equal text -> identical interned pointer.
  CHECK(a == b);
  CHECK(a.id() == b.id());
  // Distinct text -> distinct identity.
  CHECK(a != c);
  CHECK(a.id() != c.id());

  // Empty symbol is the null symbol.
  Symbol e;
  Symbol e2("");
  CHECK(!e.valid());
  CHECK(!e2.valid());
  CHECK(e == e2);

  // Re-interning the same text does not grow the pool.
  std::size_t n = symbol_pool_size();
  Symbol a2("track_speed");
  CHECK(a2 == a);
  CHECK(symbol_pool_size() == n);
}

void test_data_node() {
  DataNode i = DataNode::Int(42);
  CHECK(i.type() == DataType::kInt);
  CHECK(i.as_int().value_or(-1) == 42);
  CHECK(!i.as_float().has_value() == false);   // int coerces to float
  CHECK(i.as_float().value() == 42.0f);
  CHECK(!i.as_symbol().has_value());

  DataNode f = DataNode::Float(1.5f);
  CHECK(f.as_float().value() == 1.5f);
  CHECK(!f.as_int().has_value());              // no float->int coercion

  DataNode s = DataNode::Sym(Symbol("multiplier"));
  CHECK(s.type() == DataType::kSymbol);
  CHECK(s.as_symbol().value() == Symbol("multiplier"));
  CHECK(s.as_string().value() == "multiplier");  // symbol exposes its text

  DataNode str = DataNode::Str("Surrender");
  CHECK(str.type() == DataType::kString);
  CHECK(str.as_string().value() == "Surrender");
  CHECK(!str.as_symbol().has_value());           // a string is not a symbol

  DataNode empty;
  CHECK(empty.empty());
  CHECK(!empty.as_int().has_value());
}

void test_data_array_keyed() {
  // Build  ( (artist "X") (preview 1000 2000) )  and look up by key.
  auto artist = std::make_shared<DataArray>();
  artist->push(DataNode::Sym(Symbol("artist")));
  artist->push(DataNode::Str("Cheap Trick"));

  auto preview = std::make_shared<DataArray>();
  preview->push(DataNode::Sym(Symbol("preview")));
  preview->push(DataNode::Int(1000));
  preview->push(DataNode::Int(2000));

  DataArray root;
  root.push(DataNode::Array(artist));
  root.push(DataNode::Array(preview));

  auto found = root.find_keyed(Symbol("artist"));
  CHECK(found != nullptr);
  CHECK(found->at(1).as_string().value() == "Cheap Trick");

  auto p = root.find_keyed(Symbol("preview"));
  CHECK(p != nullptr);
  CHECK(p->at(1).as_int().value() == 1000);
  CHECK(p->at(2).as_int().value() == 2000);

  CHECK(root.find_keyed(Symbol("nonexistent")) == nullptr);
}

void test_property_table() {
  PropertyTable t;
  // Insert several keys out of identity order; lookups must still succeed.
  t.set(Symbol("deploy_rate"), DataNode::Float(0.5f));
  t.set(Symbol("multiplier"), DataNode::Int(2));
  t.set(Symbol("recharge_rate"), DataNode::Float(0.1f));
  CHECK(t.size() == 3);

  CHECK(t.find(Symbol("multiplier"))->as_int().value() == 2);
  CHECK(t.find(Symbol("deploy_rate"))->as_float().value() == 0.5f);
  CHECK(t.find(Symbol("recharge_rate"))->as_float().value() == 0.1f);
  CHECK(t.find(Symbol("missing")) == nullptr);

  // Overwrite keeps size stable and updates the value.
  t.set(Symbol("multiplier"), DataNode::Int(4));
  CHECK(t.size() == 3);
  CHECK(t.find(Symbol("multiplier"))->as_int().value() == 4);

  // Parent-chain fallback = class-hierarchy inheritance.
  PropertyTable base;
  base.set(Symbol("ready_level"), DataNode::Float(0.5f));
  base.set(Symbol("multiplier"), DataNode::Int(99));  // shadowed by child

  PropertyTable child;
  child.set_parent(&base);
  child.set(Symbol("multiplier"), DataNode::Int(2));

  // Inherited-only key resolves through the parent.
  CHECK(child.find(Symbol("ready_level"))->as_float().value() == 0.5f);
  CHECK(child.find_local(Symbol("ready_level")) == nullptr);
  // Child shadows the parent's value.
  CHECK(child.find(Symbol("multiplier"))->as_int().value() == 2);
}

}  // namespace

int main() {
  test_symbol_interning();
  test_data_node();
  test_data_array_keyed();
  test_property_table();

  if (g_failures == 0) {
    std::printf("ghogx_core_test: all checks passed\n");
    return 0;
  }
  std::fprintf(stderr, "ghogx_core_test: %d check(s) failed\n", g_failures);
  return 1;
}
