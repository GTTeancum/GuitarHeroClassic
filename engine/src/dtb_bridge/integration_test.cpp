// engine/src/dtb_bridge/integration_test.cpp
//
// End-to-end foundation test (the V1.0/V1.1 "engine boots and ticks" check):
// a synthetic DTB config -> runtime PropertyTable -> class prototype ->
// factory-instantiated Object -> scheduled in the Engine -> 60 headless frames.
// Proves the whole stack composes. All data is synthetic placeholder content.

#include "core/class_reg.h"
#include "core/data_node.h"
#include "core/engine.h"
#include "core/object.h"
#include "core/property_table.h"
#include "core/scheduler.h"
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

using namespace ghogx;
using Node = gh::dtb::Node;
using NodePtr = std::shared_ptr<Node>;

NodePtr mkint(int v)        { auto n = std::make_shared<Node>(); n->tag = 0x00; n->value = static_cast<int32_t>(v); return n; }
NodePtr mksym(const char* s){ auto n = std::make_shared<Node>(); n->tag = 0x05; n->value = std::string(s); return n; }
NodePtr mkstr(const char* s){ auto n = std::make_shared<Node>(); n->tag = 0x12; n->value = std::string(s); return n; }
NodePtr mkarr(std::vector<NodePtr> kids) { auto n = std::make_shared<Node>(); n->tag = 0x10; n->value = std::move(kids); return n; }

// A config-driven, self-rescheduling subsystem object. Reads its tick period
// from the "tick_rate" property (frames per second), ticking once per frame.
class DemoSystem : public Object {
 public:
  Symbol class_name() const override { return Symbol("DemoSystem"); }

  void start(Scheduler* s, float at) {
    sched_ = s;
    next_ = at;
    sched_->schedule(this, next_, 2);
  }

  void update(float dt) override {
    ++ticks;
    last_dt = dt;
    const float rate = get_property(Symbol("tick_rate")).as_float().value_or(60.0f);
    const float period = (rate > 0.0f) ? 1.0f / rate : 1.0f;
    next_ += period;
    sched_->schedule(this, next_, 2);
  }

  int ticks = 0;
  float last_dt = 0.0f;

 private:
  Scheduler* sched_ = nullptr;
  float next_ = 0.0f;
};

void test_boot_and_tick() {
  // 1. Synthetic config DTB:  ( (tick_rate 60) (greeting "hello") )
  std::vector<NodePtr> cfg = {
      mkarr({mksym("tick_rate"), mkint(60)}),
      mkarr({mksym("greeting"), mkstr("hello")}),
  };

  // 2. Bridge -> runtime, load into a class prototype table.
  auto arr = ghogx::dtb_bridge::from_node_list(cfg);
  PropertyTable proto;
  ghogx::dtb_bridge::load_property_table(*arr, proto);
  CHECK(proto.find(Symbol("tick_rate"))->as_int().value() == 60);

  // 3. Register the class with that prototype + a factory.
  ClassReg& reg = ClassReg::instance();
  reg.clear();
  reg.define(Symbol("DemoSystem"), Symbol(), &proto);
  reg.set_creator(Symbol("DemoSystem"),
                  [] { return std::make_unique<DemoSystem>(); });

  // 4. Instantiate via the factory; config resolves through class defaults.
  std::unique_ptr<Object> obj = reg.create(Symbol("DemoSystem"));
  CHECK(obj != nullptr);
  CHECK(obj->get_property(Symbol("greeting")).as_string().value() == "hello");
  // We registered DemoSystem's creator, so this is a DemoSystem.
  auto* demo = static_cast<DemoSystem*>(obj.get());

  // 5. Schedule it and run 60 headless frames at 1/60 s.
  Engine engine;
  demo->start(&engine.scheduler(), 0.0f);
  engine.run_frames(60, 1.0f / 60.0f);

  // The config-driven object ticked once per frame for the whole second.
  CHECK(demo->ticks == 60);
  CHECK(engine.frame_count() == 60);
  CHECK(engine.scheduler().pending() == 1);  // still self-rescheduling
}

}  // namespace

int main() {
  test_boot_and_tick();

  if (g_failures == 0) {
    std::printf("ghogx_integration_test: engine boots, loads config, ticks 60/60 -- all checks passed\n");
    return 0;
  }
  std::fprintf(stderr, "ghogx_integration_test: %d check(s) failed\n", g_failures);
  return 1;
}
