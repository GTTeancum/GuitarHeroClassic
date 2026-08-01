// engine/src/core/scheduler.h
//
// Scheduler — the per-frame tick driver.
//
// The decoded engine's scheduler owns four priority queues (0 = highest), each
// a time-sorted list of (Object, scheduled_time) entries. Each frame, every
// entry whose time has arrived has its object's update(dt) called, with
// dt = now - scheduled_time, queues walked in priority order. Objects do NOT
// stay in the list after firing -- they re-schedule themselves from inside
// update() for their next tick. This is THE architectural pattern: every
// per-frame system (beatmatch, lighting, particles, character anim) is an
// Object the scheduler drives. See memory/subsystems/engine_plumbing.md,
// "The scheduler".
//
// Reimplemented fresh: clean sorted vectors per priority rather than the
// engine's intrusive linked lists (we are not ABI-bound to it), same
// observable behavior.

#pragma once

#include <cstddef>
#include <vector>

namespace ghogx {

class Object;

class Scheduler {
 public:
  static constexpr int kPriorities = 4;
  static constexpr int kDefaultPriority = 2;

  // Schedule `obj` to tick once at absolute time `at`, in `priority`
  // (clamped to 0..3). To tick every frame an object re-schedules itself
  // inside its update().
  void schedule(Object* obj, float at, int priority = kDefaultPriority);

  // Fire every entry with scheduled_time <= now, priorities 0..3 in order,
  // each removed before its update() runs (so a self-reschedule appends
  // cleanly for a later time). dt passed to update() is now - scheduled_time.
  void walk(float now);

  void clear();
  std::size_t pending() const;  // total entries across all queues

 private:
  struct Entry {
    float at;
    Object* obj;
  };
  // Each queue kept sorted ascending by `at`.
  std::vector<Entry> queues_[kPriorities];
};

}  // namespace ghogx
