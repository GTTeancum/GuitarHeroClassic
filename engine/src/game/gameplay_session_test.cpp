#include "game/gameplay_session.h"

#include <cstdio>

namespace {

int failures = 0;

#define CHECK(expr, msg)                                                     \
  do {                                                                       \
    if (!(expr)) {                                                           \
      std::fprintf(stderr, "gameplay_session_test: FAIL: %s\n", (msg));     \
      ++failures;                                                            \
    }                                                                        \
  } while (0)

constexpr uint32_t kGreen = 1u << 0;
constexpr uint32_t kRed = 1u << 1;
constexpr uint32_t kYellow = 1u << 2;
constexpr uint32_t kStrum = 1u << 5;
constexpr uint32_t kStar = 1u << 6;

}  // namespace

int main() {
  using namespace ghogx::game;

  {
    FoFiXGameplaySession session({{1.0, 1.0, kGreen, false, false}});
    session.tick(1.0, kGreen | kStrum);
    CHECK(session.score() == 50 && session.streak() == 1 &&
              session.hits() == 1,
          "strummed note hit scores and starts streak");
    session.tick(1.3, kRed | kStrum);
    CHECK(session.overstrums() == 1 && session.streak() == 0,
          "wrong extra strum resets streak as overstrum");
  }

  {
    FoFiXGameplaySession session({
        {1.0, 1.0, kGreen, false, false},
        {1.1, 1.1, kRed, true, false},
    });
    session.tick(1.0, kGreen | kStrum);
    session.tick(1.1, kRed);
    CHECK(session.score() == 100 && session.streak() == 2 &&
              session.hits() == 2,
          "HOPO note hits on fret edge after prior streak");
  }

  {
    FoFiXGameplaySession session({{1.0, 2.0, kGreen, false, false}});
    session.tick(1.0, kGreen | kStrum);
    session.tick(2.0, kGreen);
    CHECK(session.score() == 150,
          "one-second held sustain adds FoFiX sustain score");
  }

  {
    FoFiXGameplaySession session({
        {1.0, 1.0, kGreen, false, true},
        {1.5, 1.5, kRed, false, true},
        {2.0, 2.0, kYellow, false, false},
    });
    session.tick(1.0, kGreen | kStrum);
    session.tick(1.5, kRed | kStrum);
    session.tick(2.0, kYellow | kStrum);
    CHECK(session.star_power_fill() > 0.249 &&
              session.star_power_fill() < 0.251,
          "completed star phrase awards quarter meter at phrase boundary");
  }

  {
    FoFiXGameplaySession session({
        {1.0, 1.0, kGreen, false, true},
        {1.5, 1.5, kRed, false, false},
        {2.0, 2.0, kGreen, false, true},
        {2.5, 2.5, kRed, false, false},
        {3.0, 3.0, kGreen, false, true},
        {3.5, 3.5, kRed, false, false},
        {4.0, 4.0, kGreen, false, true},
        {4.5, 4.5, kRed, false, false},
        {5.2, 5.2, kGreen, false, false},
    });
    session.tick(1.0, kGreen | kStrum);
    session.tick(1.5, kRed | kStrum);
    session.tick(2.0, kGreen | kStrum);
    session.tick(2.5, kRed | kStrum);
    session.tick(3.0, kGreen | kStrum);
    session.tick(3.5, kRed | kStrum);
    session.tick(4.0, kGreen | kStrum);
    session.tick(4.5, kRed | kStrum);
    CHECK(session.star_power_fill() > 0.999, "four phrases fill star meter");
    session.tick(5.1, kStar);
    CHECK(session.star_power_active(), "star power activates at half or more");
    const int before_powered_note = session.score();
    session.tick(5.2, kGreen | kStrum);
    CHECK(session.score() - before_powered_note == 100,
          "active star power doubles subsequent note score");
    session.tick(25.2, 0);
    CHECK(!session.star_power_active() && session.star_power_fill() == 0.0,
          "star power drains out over time");
  }

  if (failures == 0) {
    std::fprintf(stderr, "gameplay_session_test: PASS\n");
  }
  return failures == 0 ? 0 : 1;
}
