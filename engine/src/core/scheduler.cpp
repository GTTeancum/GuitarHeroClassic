// engine/src/core/scheduler.cpp

#include "core/scheduler.h"

#include "core/object.h"

#include <algorithm>

namespace ghogx {

void Scheduler::schedule(Object* obj, float at, int priority) {
  if (priority < 0) priority = 0;
  if (priority >= kPriorities) priority = kPriorities - 1;

  auto& q = queues_[priority];
  // Insert keeping the queue sorted ascending by scheduled time. upper_bound
  // places the new entry AFTER any with an equal time -> stable FIFO order for
  // same-time ticks, matching the decoded engine's sorted insert (which walks
  // until it finds a strictly-greater time).
  auto it = std::upper_bound(
      q.begin(), q.end(), at,
      [](float t, const Entry& e) { return t < e.at; });
  q.insert(it, Entry{at, obj});
}

void Scheduler::walk(float now) {
  for (int p = 0; p < kPriorities; ++p) {
    auto& q = queues_[p];
    // Queue is sorted by `at`; everything before `cut` is due (at <= now).
    auto cut = std::upper_bound(
        q.begin(), q.end(), now,
        [](float t, const Entry& e) { return t < e.at; });

    // Detach the due prefix before firing, so update()'s self-reschedule can
    // append to this same queue without disturbing what we're iterating.
    std::vector<Entry> due(q.begin(), cut);
    q.erase(q.begin(), cut);

    for (const Entry& e : due) {
      if (e.obj) e.obj->update(now - e.at);
    }
  }
}

void Scheduler::clear() {
  for (auto& q : queues_) q.clear();
}

std::size_t Scheduler::pending() const {
  std::size_t n = 0;
  for (const auto& q : queues_) n += q.size();
  return n;
}

}  // namespace ghogx
