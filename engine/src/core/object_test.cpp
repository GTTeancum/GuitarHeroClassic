// engine/src/core/object_test.cpp
//
// Unit test for Object + ClassReg: class hierarchy / is_a, instance-vs-prototype
// property resolution, the universal message handler, subclass overrides, and
// the update() tick.

#include "core/class_reg.h"
#include "core/data_node.h"
#include "core/object.h"
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

// A concrete test class: derives from a registered "Widget" base, exposes one
// computed property ("doubled" = 2 * "value"), one custom message ("ping"),
// and counts update() ticks.
class TestWidget : public Object {
 public:
  Symbol class_name() const override { return Symbol("TestWidget"); }

  bool get_property_override(Symbol key, DataNode& out) const override {
    if (key == Symbol("doubled")) {
      int v = find_property(Symbol("value")) ? find_property(Symbol("value"))->as_int().value_or(0) : 0;
      out = DataNode::Int(v * 2);
      return true;
    }
    return false;
  }

  DataNode handle_property(Symbol msg, const DataArray& args) override {
    if (msg == Symbol("ping")) {
      // (ping <n>) -> n + 1
      int n = args.size() ? args.at(0).as_int().value_or(0) : 0;
      return DataNode::Int(n + 1);
    }
    return Object::handle_property(msg, args);  // fall through to universal
  }

  void update(float dt) override {
    ++ticks;
    last_dt = dt;
  }

  int ticks = 0;
  float last_dt = 0.0f;
};

void test_class_hierarchy() {
  ClassReg& reg = ClassReg::instance();
  reg.clear();
  reg.define(Symbol("Object"));
  reg.define(Symbol("Widget"), Symbol("Object"));
  reg.define(Symbol("TestWidget"), Symbol("Widget"));

  CHECK(reg.is_a(Symbol("TestWidget"), Symbol("TestWidget")));
  CHECK(reg.is_a(Symbol("TestWidget"), Symbol("Widget")));
  CHECK(reg.is_a(Symbol("TestWidget"), Symbol("Object")));
  CHECK(!reg.is_a(Symbol("Widget"), Symbol("TestWidget")));
  CHECK(!reg.is_a(Symbol("TestWidget"), Symbol("Camera")));

  TestWidget w;
  CHECK(w.is_a(Symbol("Widget")));
  CHECK(w.is_a(Symbol("Object")));
  CHECK(!w.is_a(Symbol("Camera")));
}

void test_property_resolution() {
  ClassReg& reg = ClassReg::instance();
  reg.clear();

  // Class prototype provides defaults; instance overrides them.
  PropertyTable proto;
  proto.set(Symbol("value"), DataNode::Int(10));
  proto.set(Symbol("color"), DataNode::Sym(Symbol("red")));
  reg.define(Symbol("TestWidget"), Symbol(), &proto);

  TestWidget w;
  w.bind_class_defaults();

  // Inherited from prototype.
  CHECK(w.get_property(Symbol("value")).as_int().value() == 10);
  CHECK(w.get_property(Symbol("color")).as_symbol().value() == Symbol("red"));
  CHECK(w.has_property(Symbol("color")));
  CHECK(!w.has_property(Symbol("nonexistent")));

  // Instance set shadows the prototype.
  w.set_property(Symbol("value"), DataNode::Int(20));
  CHECK(w.get_property(Symbol("value")).as_int().value() == 20);
  // Prototype itself is untouched.
  CHECK(proto.find(Symbol("value"))->as_int().value() == 10);

  // Computed override.
  CHECK(w.get_property(Symbol("doubled")).as_int().value() == 40);  // 2 * 20
  CHECK(w.has_property(Symbol("doubled")));
}

void test_universal_messages() {
  ClassReg& reg = ClassReg::instance();
  reg.clear();
  reg.define(Symbol("TestWidget"));

  TestWidget w;
  w.set_name(Symbol("widget0"));

  // (set value 7)
  DataArray set_args;
  set_args.push(DataNode::Sym(Symbol("value")));
  set_args.push(DataNode::Int(7));
  w.handle_property(Symbol("set"), set_args);

  // (get value) -> 7
  DataArray get_args;
  get_args.push(DataNode::Sym(Symbol("value")));
  CHECK(w.handle_property(Symbol("get"), get_args).as_int().value() == 7);

  // (has value) -> 1 ; (has missing) -> 0
  CHECK(w.handle_property(Symbol("has"), get_args).as_int().value() == 1);
  DataArray miss_args;
  miss_args.push(DataNode::Sym(Symbol("missing")));
  CHECK(w.handle_property(Symbol("has"), miss_args).as_int().value() == 0);

  // name / class_name / get_type
  CHECK(w.handle_property(Symbol("name"), {}).as_symbol().value() == Symbol("widget0"));
  CHECK(w.handle_property(Symbol("class_name"), {}).as_symbol().value() == Symbol("TestWidget"));
  CHECK(w.handle_property(Symbol("get_type"), {}).as_symbol().value() == Symbol("TestWidget"));

  // is_a via message
  reg.define(Symbol("TestWidget"), Symbol("Widget"));
  reg.define(Symbol("Widget"));
  DataArray isa_args;
  isa_args.push(DataNode::Sym(Symbol("Widget")));
  CHECK(w.handle_property(Symbol("is_a"), isa_args).as_int().value() == 1);

  // Custom subclass message falls NOT through to base.
  DataArray ping_args;
  ping_args.push(DataNode::Int(41));
  CHECK(w.handle_property(Symbol("ping"), ping_args).as_int().value() == 42);
}

void test_update_tick() {
  TestWidget w;
  CHECK(w.ticks == 0);
  w.update(0.016f);
  w.update(0.016f);
  CHECK(w.ticks == 2);
  CHECK(w.last_dt == 0.016f);
}

void test_factory() {
  ClassReg& reg = ClassReg::instance();
  reg.clear();

  PropertyTable proto;
  proto.set(Symbol("value"), DataNode::Int(5));
  reg.define(Symbol("TestWidget"), Symbol(), &proto);
  reg.set_creator(Symbol("TestWidget"),
                  [] { return std::make_unique<TestWidget>(); });

  CHECK(reg.creatable(Symbol("TestWidget")));
  CHECK(!reg.creatable(Symbol("Unregistered")));

  auto obj = reg.create(Symbol("TestWidget"));
  CHECK(obj != nullptr);
  CHECK(obj->class_name() == Symbol("TestWidget"));
  // create() bound class defaults: the prototype value resolves on the instance.
  CHECK(obj->get_property(Symbol("value")).as_int().value() == 5);
  // The subclass computed override works on a factory-made instance too.
  CHECK(obj->get_property(Symbol("doubled")).as_int().value() == 10);  // 2 * 5

  // A class with no creator cannot be instantiated.
  reg.define(Symbol("Abstract"));
  CHECK(!reg.creatable(Symbol("Abstract")));
  CHECK(reg.create(Symbol("Abstract")) == nullptr);
  CHECK(reg.create(Symbol("Unregistered")) == nullptr);
}

void test_object_ref() {
  TestWidget target;
  target.set_name(Symbol("target"));

  DataNode ref = DataNode::Obj(&target);
  CHECK(ref.type() == DataType::kObject);
  CHECK(ref.as_object() == &target);
  CHECK(!ref.as_int().has_value());
  CHECK(!ref.as_array());

  // An object reference stored as a property and read back.
  TestWidget holder;
  holder.set_property(Symbol("link"), DataNode::Obj(&target));
  Object* got = holder.get_property(Symbol("link")).as_object();
  CHECK(got == &target);
  CHECK(got->name() == Symbol("target"));

  // Null reference round-trips as null.
  DataNode nullref = DataNode::Obj(nullptr);
  CHECK(nullref.type() == DataType::kObject);
  CHECK(nullref.as_object() == nullptr);
}

}  // namespace

int main() {
  test_class_hierarchy();
  test_property_resolution();
  test_universal_messages();
  test_update_tick();
  test_factory();
  test_object_ref();

  if (g_failures == 0) {
    std::printf("ghogx_object_test: all checks passed\n");
    return 0;
  }
  std::fprintf(stderr, "ghogx_object_test: %d check(s) failed\n", g_failures);
  return 1;
}
