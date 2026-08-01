// engine/src/core/engine.h
//
// Engine — the per-frame main loop.
//
// The decoded main loop (sub_82120090) runs a fixed sequence of phases each
// frame, with the Scheduler walk at the centre (call #7) driving every live
// Object's update(). See memory/subsystems/frame_loop.md and engine_plumbing.md
// ("The scheduler").
//
// This is the headless skeleton: it advances a clock, fires the scheduler, and
// calls render/present phase hooks that default to no-ops. The PC/OG-Xbox
// presentation build overrides those hooks to draw and flip; the gameplay
// logic and timing are identical with or without a screen, which is what lets
// us develop and test the whole engine headlessly first.

#pragma once

#include "core/scheduler.h"

#include <cstdint>

namespace ghogx {

class Engine {
 public:
  virtual ~Engine() = default;

  Scheduler& scheduler() { return scheduler_; }

  // Advance one frame by `dt` seconds: pre-frame -> scheduler walk -> render
  // -> present, then bump the frame counter and clock.
  void tick(float dt);

  // Headless harness: run `frames` ticks at a fixed `dt`.
  void run_frames(int frames, float dt);

  float time() const { return now_; }
  uint64_t frame_count() const { return frames_; }

 protected:
  // Frame-phase hooks. Default no-ops; the presentation build overrides them.
  virtual void on_pre_frame(float dt) { (void)dt; }   // input, pre-update setup
  virtual void on_render(float dt) { (void)dt; }      // submit draw calls
  virtual void on_present() {}                        // flip the back buffer

 private:
  Scheduler scheduler_;
  float now_ = 0.0f;
  uint64_t frames_ = 0;
};

}  // namespace ghogx
