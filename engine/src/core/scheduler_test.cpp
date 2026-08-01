// engine/src/core/scheduler_test.cpp
//
// Unit test for Scheduler: due-time firing, dt computation, priority ordering,
// self-reschedule (the every-frame tick pattern), and removal accounting.

#include "core/object.h"
#include "core/scheduler.h"
#include "core/symbol.h"

#include <cstdio>
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

// Records each tick's dt; optionally appends its id to a shared fire-order log.
class Ticker : public Object {
 public:
  Symbol class_name() const override { return Symbol("Ticker"); }
  void update(float dt) override {
    ++ticks;
    last_dt = dt;
    if (order) order->push_back(id);
  }
  int ticks = 0;
  float last_dt = 0.0f;
  int id = 0;
  std::vector<int>* order = nullptr;
};

// Re-schedules itself every `interval` from inside update() -> ticks forever.
class RepeatTicker : public Object {
 public:
  RepeatTicker(Scheduler* s, float interval, int priority)
      : sched_(s), interval_(interval), priority_(priority) {}
  Symbol class_name() const override { return Symbol("RepeatTicker"); }
  void update(float dt) override {
    ++ticks;
    (void)dt;
    next_ += interval_;
    sched_->schedule(this, next_, priority_);
  }
  void start(float at) {
    next_ = at;
    sched_->schedule(this, next_, priority_);
  }
  int ticks = 0;

 private:
  Scheduler* sched_;
  float interval_;
  int priority_;
  float next_ = 0.0f;
};

void test_basic_fire_and_dt() {
  Scheduler s;
  Ticker t;
  s.schedule(&t, 1.0f);
  CHECK(s.pending() == 1);

  s.walk(0.5f);  // before due time
  CHECK(t.ticks == 0);
  CHECK(s.pending() == 1);

  s.walk(1.5f);  // past due time
  CHECK(t.ticks == 1);
  CHECK(t.last_dt == 0.5f);  // dt = now - scheduled_time = 1.5 - 1.0
  CHECK(s.pending() == 0);   // removed after firing
}

void test_priority_order() {
  Scheduler s;
  std::vector<int> order;
  Ticker a, b, c;
  a.id = 0; a.order = &order;  // priority 0 (highest)
  b.id = 1; b.order = &order;  // priority 1
  c.id = 2; c.order = &order;  // priority 0, scheduled later in code

  // Schedule out of priority order; all due at t=0.
  s.schedule(&b, 0.0f, 1);
  s.schedule(&a, 0.0f, 0);
  s.schedule(&c, 0.0f, 0);

  s.walk(0.0f);
  // Priority 0 entries (a, c) fire before priority 1 (b); within a priority,
  // insertion/time order is preserved (a before c).
  CHECK(order.size() == 3);
  CHECK(order[0] == 0);  // a (pri 0)
  CHECK(order[1] == 2);  // c (pri 0)
  CHECK(order[2] == 1);  // b (pri 1)
}

void test_self_reschedule() {
  Scheduler s;
  RepeatTicker r(&s, /*interval=*/0.1f, /*priority=*/2);
  r.start(0.0f);
  CHECK(s.pending() == 1);

  // Advance time in 0.1 steps; each walk fires exactly one tick and the object
  // re-schedules itself, so pending stays at 1 and ticks accumulate.
  float now = 0.0f;
  for (int i = 0; i < 5; ++i) {
    s.walk(now);
    CHECK(s.pending() == 1);
    now += 0.1f;
  }
  CHECK(r.ticks == 5);
}

void test_multiple_due_in_one_walk() {
  Scheduler s;
  Ticker a, b, c;
  s.schedule(&a, 0.0f);
  s.schedule(&b, 0.5f);
  s.schedule(&c, 2.0f);

  s.walk(1.0f);  // a and b are due; c is not
  CHECK(a.ticks == 1);
  CHECK(b.ticks == 1);
  CHECK(c.ticks == 0);
  CHECK(s.pending() == 1);  // only c remains
}

}  // namespace

int main() {
  test_basic_fire_and_dt();
  test_priority_order();
  test_self_reschedule();
  test_multiple_due_in_one_walk();

  if (g_failures == 0) {
    std::printf("ghogx_scheduler_test: all checks passed\n");
    return 0;
  }
  std::fprintf(stderr, "ghogx_scheduler_test: %d check(s) failed\n", g_failures);
  return 1;
}
