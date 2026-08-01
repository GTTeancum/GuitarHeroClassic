// engine/src/core/engine_test.cpp
//
// Unit test for the headless Engine loop: frame-phase ordering (pre-frame ->
// scheduler -> render -> present) and a self-rescheduling object ticking once
// per frame across a 60-frame run.

#include "core/engine.h"
#include "core/object.h"
#include "core/scheduler.h"
#include "core/symbol.h"

#include <cstdio>
#include <string>
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

// Engine that records each frame phase into a shared log, so we can assert the
// scheduler walk happens between pre-frame and render.
class RecordingEngine : public Engine {
 public:
  std::vector<std::string> log;

 protected:
  void on_pre_frame(float) override { log.push_back("pre"); }
  void on_render(float) override { log.push_back("render"); }
  void on_present() override { log.push_back("present"); }
};

// Object that logs "tick" (proving it ran inside the scheduler walk) and
// re-schedules itself every frame.
class FrameObject : public Object {
 public:
  FrameObject(Scheduler* s, float dt, std::vector<std::string>* log)
      : sched_(s), dt_(dt), log_(log) {}
  Symbol class_name() const override { return Symbol("FrameObject"); }
  void update(float) override {
    ++ticks;
    if (log_) log_->push_back("tick");
    next_ += dt_;
    sched_->schedule(this, next_, 2);
  }
  void start() { sched_->schedule(this, next_, 2); }
  int ticks = 0;

 private:
  Scheduler* sched_;
  float dt_;
  std::vector<std::string>* log_;
  float next_ = 0.0f;
};

void test_phase_order() {
  RecordingEngine e;
  FrameObject obj(&e.scheduler(), 1.0f / 60.0f, &e.log);
  obj.start();

  e.tick(1.0f / 60.0f);

  // One frame: pre-frame, then the object's tick (scheduler walk), then render,
  // then present.
  CHECK(e.log.size() == 4);
  CHECK(e.log[0] == "pre");
  CHECK(e.log[1] == "tick");
  CHECK(e.log[2] == "render");
  CHECK(e.log[3] == "present");
  CHECK(e.frame_count() == 1);
}

void test_sixty_frames() {
  Engine e;
  FrameObject obj(&e.scheduler(), 1.0f / 60.0f, nullptr);
  obj.start();

  e.run_frames(60, 1.0f / 60.0f);

  CHECK(obj.ticks == 60);
  CHECK(e.frame_count() == 60);
  // ~1 second elapsed (float accumulation tolerance).
  CHECK(e.time() > 0.99f && e.time() < 1.01f);
  // Self-rescheduling keeps exactly one pending entry.
  CHECK(e.scheduler().pending() == 1);
}

void test_empty_engine_runs() {
  // No scheduled objects: the loop must still advance cleanly.
  Engine e;
  e.run_frames(10, 0.016f);
  CHECK(e.frame_count() == 10);
  CHECK(e.scheduler().pending() == 0);
}

}  // namespace

int main() {
  test_phase_order();
  test_sixty_frames();
  test_empty_engine_runs();

  if (g_failures == 0) {
    std::printf("ghogx_engine_test: all checks passed\n");
    return 0;
  }
  std::fprintf(stderr, "ghogx_engine_test: %d check(s) failed\n", g_failures);
  return 1;
}
