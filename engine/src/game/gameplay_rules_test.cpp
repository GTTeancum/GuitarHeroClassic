#include "game/gameplay_rules.h"

#include <cstdio>

namespace {

int failures = 0;

#define CHECK(expr, msg)                                                    \
  do {                                                                      \
    if (!(expr)) {                                                          \
      std::fprintf(stderr, "gameplay_rules_test: FAIL: %s\n", (msg));      \
      ++failures;                                                           \
    }                                                                       \
  } while (0)

}  // namespace

int main() {
  using namespace ghogx::game;

  const FoFiXHitWindow window = fofix_hit_window_for_bpm(120.0);
  CHECK(window.early_sec > 0.141 && window.early_sec < 0.143,
        "FoFiX standard 120 BPM early margin is about 142 ms");
  CHECK(window.late_sec > 0.141 && window.late_sec < 0.143,
        "FoFiX standard 120 BPM late margin is about 142 ms");
  CHECK(fofix_note_in_window(10.0, 10.142, window),
        "note at positive edge is hittable");
  CHECK(!fofix_note_in_window(10.0, 10.143, window),
        "note beyond positive edge is too early");
  CHECK(fofix_note_missed(10.143, 10.0, window),
        "note beyond late edge is missed");

  CHECK(fofix_match_frets(0b00001, 0b00001), "green matches green");
  CHECK(fofix_match_frets(0b00011, 0b00010),
        "lower fret may be held under a single note");
  CHECK(!fofix_match_frets(0b00110, 0b00010),
        "higher fret blocks a single note");
  CHECK(fofix_match_frets(0b00110, 0b00110), "chord requires exact frets");
  CHECK(!fofix_match_frets(0b00111, 0b00110),
        "extra fret blocks chord");

  FoFiXScoreState score;
  for (int i = 0; i < 9; ++i) {
    fofix_apply_hit(score, 1);
  }
  CHECK(score.score == 450 && score.streak == 9 && score.multiplier == 1,
        "first nine notes score at 1x");
  const FoFiXScoreAward tenth = fofix_apply_hit(score, 1);
  CHECK(tenth.points == 100 && score.multiplier == 2,
        "tenth note scores at 2x");
  fofix_apply_hit(score, 2);
  CHECK(score.score == 750, "two-note chord scores both gems at multiplier");
  fofix_apply_miss(score);
  CHECK(score.streak == 0 && score.multiplier == 1,
        "miss resets streak and multiplier");

  if (failures == 0) {
    std::fprintf(stderr, "gameplay_rules_test: PASS\n");
  }
  return failures == 0 ? 0 : 1;
}
