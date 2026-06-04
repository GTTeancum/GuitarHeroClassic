// engine/src/core/object_dir_test.cpp
//
// Unit test for ObjectDir: add/find/iteration, factory-created children, name
// collision replacement, and path descent through nested dirs. Synthetic data.

#include "core/class_reg.h"
#include "core/object.h"
#include "core/object_dir.h"
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

// A trivial leaf object for populating dirs.
class Thing : public Object {
 public:
  Symbol class_name() const override { return Symbol("Thing"); }
  int tag = 0;
};

std::unique_ptr<Thing> make_thing(const char* name, int tag) {
  auto t = std::make_unique<Thing>();
  t->set_name(Symbol(name));
  t->tag = tag;
  return t;
}

void register_thing() {
  ClassReg& reg = ClassReg::instance();
  reg.clear();
  reg.define(Symbol("Thing"));
  reg.set_creator(Symbol("Thing"), [] { return std::make_unique<Thing>(); });
}

void test_add_find_order() {
  ObjectDir dir;
  dir.set_name(Symbol("root"));
  CHECK(dir.class_name() == Symbol("ObjectDir"));

  Object* a = dir.add(make_thing("a", 1));
  Object* b = dir.add(make_thing("b", 2));
  CHECK(a && b);
  CHECK(dir.size() == 2);

  CHECK(dir.find(Symbol("a")) == a);
  CHECK(dir.find(Symbol("b")) == b);
  CHECK(dir.find(Symbol("missing")) == nullptr);

  // Insertion order preserved.
  CHECK(dir.at(0) == a);
  CHECK(dir.at(1) == b);

  // Unnamed child is rejected.
  auto unnamed = std::make_unique<Thing>();
  CHECK(dir.add(std::move(unnamed)) == nullptr);
  CHECK(dir.size() == 2);
}

void test_name_collision_replaces() {
  ObjectDir dir;
  Object* first = dir.add(make_thing("dup", 10));
  CHECK(dir.size() == 1);
  CHECK(static_cast<Thing*>(dir.find(Symbol("dup")))->tag == 10);

  Object* second = dir.add(make_thing("dup", 20));  // same name
  CHECK(dir.size() == 1);                            // replaced, not appended
  CHECK(second != first);
  CHECK(static_cast<Thing*>(dir.find(Symbol("dup")))->tag == 20);
}

void test_create_child_via_factory() {
  register_thing();
  ObjectDir dir;

  Object* a = dir.create_child(Symbol("Thing"), Symbol("a"));
  CHECK(a != nullptr);
  CHECK(a->class_name() == Symbol("Thing"));
  CHECK(a->name() == Symbol("a"));
  CHECK(dir.find(Symbol("a")) == a);

  // Unregistered class -> no creation.
  CHECK(dir.create_child(Symbol("Unregistered"), Symbol("x")) == nullptr);
  CHECK(dir.size() == 1);
}

void test_find_path_nested() {
  ObjectDir root;
  root.set_name(Symbol("root"));

  // root/sub/x  where sub is a nested ObjectDir and x a Thing.
  auto sub = std::make_unique<ObjectDir>();
  sub->set_name(Symbol("sub"));
  Object* x = sub->add(make_thing("x", 7));
  Object* subraw = root.add(std::move(sub));

  Object* a = root.add(make_thing("a", 1));  // a leaf at root level

  CHECK(root.find_path("sub") == subraw);
  CHECK(root.find_path("sub/x") == x);
  CHECK(root.find_path("a") == a);

  // Missing leaf, missing intermediate, and descending through a non-dir.
  CHECK(root.find_path("sub/missing") == nullptr);
  CHECK(root.find_path("nope/x") == nullptr);
  CHECK(root.find_path("a/x") == nullptr);  // 'a' is a Thing, not a dir

  // Empty / redundant separators are ignored.
  CHECK(root.find_path("/sub//x/") == x);
  CHECK(root.find_path("") == nullptr);
}

void test_dir_type() {
  ObjectDir dir;
  CHECK(!dir.dir_type().valid());
  dir.set_dir_type(Symbol("WorldDir"));
  CHECK(dir.dir_type() == Symbol("WorldDir"));
}

}  // namespace

int main() {
  test_add_find_order();
  test_name_collision_replaces();
  test_create_child_via_factory();
  test_find_path_nested();
  test_dir_type();

  if (g_failures == 0) {
    std::printf("ghogx_object_dir_test: all checks passed\n");
    return 0;
  }
  std::fprintf(stderr, "ghogx_object_dir_test: %d check(s) failed\n", g_failures);
  return 1;
}
