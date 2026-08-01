// engine/src/core/engine.cpp

#include "core/engine.h"

namespace ghogx {

void Engine::tick(float dt) {
  now_ += dt;
  on_pre_frame(dt);       // input / pre-update
  scheduler_.walk(now_);  // drive every live Object's update()
  on_render(dt);          // build the frame
  on_present();           // flip
  ++frames_;
}

void Engine::run_frames(int frames, float dt) {
  for (int i = 0; i < frames; ++i) tick(dt);
}

}  // namespace ghogx
