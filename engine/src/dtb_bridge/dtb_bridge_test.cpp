// engine/src/dtb_bridge/dtb_bridge_test.cpp
//
// Hermetic test: build gh::dtb::Node trees by hand (no external asset files),
// bridge them to the runtime model, and verify the translation + the keyed
// PropertyTable load. Test data is synthetic placeholder content.

#include "core/data_node.h"
#include "core/property_table.h"
#include "core/symbol.h"
#include "dtb_bridge/dtb_bridge.h"

#include "dtb.h"

#include <cstdio>
#include <memory>
#include <vector>

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

using ghogx::DataArray;
using ghogx::DataNode;
using ghogx::PropertyTable;
using ghogx::Symbol;
using Node = gh::dtb::Node;
using NodePtr = std::shared_ptr<Node>;

// gh::dtb::Node builders (Classic DTB tag values).
NodePtr mkint(int v)        { auto n = std::make_shared<Node>(); n->tag = 0x00; n->value = static_cast<int32_t>(v); return n; }
NodePtr mkflt(float v)      { auto n = std::make_shared<Node>(); n->tag = 0x01; n->value = v; return n; }
NodePtr mksym(const char* s){ auto n = std::make_shared<Node>(); n->tag = 0x05; n->value = std::string(s); return n; }
NodePtr mkstr(const char* s){ auto n = std::make_shared<Node>(); n->tag = 0x12; n->value = std::string(s); return n; }
NodePtr mkarr(std::vector<NodePtr> kids) {
  auto n = std::make_shared<Node>(); n->tag = 0x10; n->value = std::move(kids); return n;
}

// Build a synthetic config tree:
//   ( (multiplier 2)
//     (deploy_rate 0.5)
//     (title "placeholder")
//     (preview 1000 2000)
//     (nested (a 1) (b 2)) )
std::vector<NodePtr> build_config() {
  return {
      mkarr({mksym("multiplier"),  mkint(2)}),
      mkarr({mksym("deploy_rate"), mkflt(0.5f)}),
      mkarr({mksym("title"),       mkstr("placeholder")}),
      mkarr({mksym("preview"),     mkint(1000), mkint(2000)}),
      mkarr({mksym("nested"),
             mkarr({mksym("a"), mkint(1)}),
             mkarr({mksym("b"), mkint(2)})}),
  };
}

void test_node_translation() {
  CHECK(ghogx::dtb_bridge::from_node(*mkint(7)).as_int().value() == 7);
  CHECK(ghogx::dtb_bridge::from_node(*mkflt(1.25f)).as_float().value() == 1.25f);
  CHECK(ghogx::dtb_bridge::from_node(*mksym("foo")).as_symbol().value() == Symbol("foo"));
  CHECK(ghogx::dtb_bridge::from_node(*mkstr("bar")).as_string().value() == "bar");
  // Symbols are not strings and vice versa.
  CHECK(!ghogx::dtb_bridge::from_node(*mksym("foo")).as_string().has_value() == false);
  CHECK(!ghogx::dtb_bridge::from_node(*mkstr("bar")).as_symbol().has_value());
}

void test_array_bridge() {
  auto cfg = build_config();
  auto arr = ghogx::dtb_bridge::from_node_list(cfg);
  CHECK(arr != nullptr);
  CHECK(arr->size() == 5);

  auto mult = arr->find_keyed(Symbol("multiplier"));
  CHECK(mult && mult->at(1).as_int().value() == 2);

  auto rate = arr->find_keyed(Symbol("deploy_rate"));
  CHECK(rate && rate->at(1).as_float().value() == 0.5f);

  auto title = arr->find_keyed(Symbol("title"));
  CHECK(title && title->at(1).as_string().value() == "placeholder");

  // Nested array, looked up two levels deep.
  auto nested = arr->find_keyed(Symbol("nested"));
  CHECK(nested != nullptr);
  auto a = nested->find_keyed(Symbol("a"));
  CHECK(a && a->at(1).as_int().value() == 1);
  auto b = nested->find_keyed(Symbol("b"));
  CHECK(b && b->at(1).as_int().value() == 2);
}

void test_property_table_load() {
  auto cfg = build_config();
  auto arr = ghogx::dtb_bridge::from_node_list(cfg);

  PropertyTable t;
  ghogx::dtb_bridge::load_property_table(*arr, t);

  CHECK(t.find(Symbol("multiplier"))->as_int().value() == 2);
  CHECK(t.find(Symbol("deploy_rate"))->as_float().value() == 0.5f);
  CHECK(t.find(Symbol("title"))->as_string().value() == "placeholder");

  // Multi-value keyed entry stored as an array.
  auto preview = t.find(Symbol("preview"));
  CHECK(preview != nullptr);
  auto pv = preview->as_array();
  CHECK(pv && pv->size() == 2);
  CHECK(pv->at(0).as_int().value() == 1000);
  CHECK(pv->at(1).as_int().value() == 2000);

  // Nested keyed entry: value is the tail array of its two sub-arrays.
  auto nested = t.find(Symbol("nested"));
  CHECK(nested != nullptr);
  CHECK(nested->as_array() && nested->as_array()->size() == 2);
}

}  // namespace

int main() {
  test_node_translation();
  test_array_bridge();
  test_property_table_load();

  if (g_failures == 0) {
    std::printf("ghogx_dtb_bridge_test: all checks passed\n");
    return 0;
  }
  std::fprintf(stderr, "ghogx_dtb_bridge_test: %d check(s) failed\n", g_failures);
  return 1;
}
