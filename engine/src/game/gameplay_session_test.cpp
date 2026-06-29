#include "game/gameplay_session.h"

#include "chart/midi_reader.h"

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
    CHECK(session.last_events().size() == 1 &&
              session.last_events()[0].type == FoFiXSessionEventType::Hit &&
              session.last_events()[0].mask == kGreen &&
              session.last_events()[0].gem_count == 1 &&
              session.last_events()[0].score_delta == 50 &&
              session.last_events()[0].score == 50 &&
              session.last_events()[0].streak == 1 &&
              session.last_events()[0].multiplier == 1 &&
              session.last_events()[0].rock_fill > 0.5001 &&
              session.last_events()[0].star_power_fill == 0.0 &&
              !session.last_events()[0].failed,
          "FoFiX session reports hit delta for native presentation");
    session.tick(1.1, kGreen);
    session.tick(1.3, kRed | kStrum);
    CHECK(session.overstrums() == 1 && session.streak() == 0,
          "wrong extra strum resets streak as overstrum");
    CHECK(session.last_events().size() == 1 &&
              session.last_events()[0].type ==
                  FoFiXSessionEventType::Overstrum &&
              session.last_events()[0].mask == kRed,
          "FoFiX session reports overstrum delta for native presentation");
    CHECK(session.last_events()[0].streak == 0 &&
              session.last_events()[0].multiplier == 1,
          "FoFiX overstrum event snapshots reset score state");
  }

  {
    FoFiXGameplaySession session({{1.0, 1.0, kGreen, false, false}});
    session.tick(1.0, kGreen | kStrum);
    session.tick(1.01, kGreen | kStrum);
    CHECK(session.score() == 50 && session.streak() == 1 &&
              session.overstrums() == 0,
          "held strum does not create repeated FoFiX overstrums");
  }

  {
    FoFiXGameplaySession session({{1.0, 1.0, kGreen, false, false}});
    session.tick(1.0, kGreen | kStrum);
    session.tick(1.1, kGreen);
    session.tick(1.3, kStrum);
    CHECK(session.overstrums() == 1 && session.streak() == 0,
          "empty extra strum resets streak as FoFiX overstrum");
    CHECK(session.last_events().size() == 1 &&
              session.last_events()[0].type ==
                  FoFiXSessionEventType::Overstrum &&
              session.last_events()[0].mask == 0,
          "empty FoFiX overstrum reports no held fret mask");
  }

  {
    FoFiXGameplaySession session({{1.0, 1.0, kGreen, false, false}});
    session.tick(1.2, kStrum);
    CHECK(session.misses() == 1 && session.overstrums() == 1,
          "late bad strum applies both FoFiX missed-note and bad-pick penalties");
    CHECK(session.last_events().size() == 2 &&
              session.last_events()[0].type == FoFiXSessionEventType::Miss &&
              session.last_events()[1].type ==
                  FoFiXSessionEventType::Overstrum &&
              session.last_events()[1].mask == 0 &&
              session.last_events()[1].rock_fill < 0.5,
          "FoFiX session reports miss and bad-pick deltas on the same tick");
  }

  {
    FoFiXGameplaySession session({{100.0, 100.0, kGreen, false, false}});
    double t = 1.0;
    for (int i = 0; i < 450 && !session.failed(); ++i, t += 0.1) {
      session.tick(t, (i & 1) ? 0 : kStrum);
    }
    CHECK(session.failed(), "repeated bad picks can fail the song");
    const int overstrums = session.overstrums();
    const int score = session.score();
    session.tick(t + 0.1, 0);
    session.tick(t + 0.2, kGreen | kStrum);
    CHECK(session.overstrums() == overstrums && session.score() == score,
          "failed FoFiX session ignores later picks and scoring");
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
    FoFiXGameplaySession session({
        {1.0, 1.0, kGreen, false, false},
        {1.05, 1.05, kRed, false, false},
    });
    session.tick(1.05, kRed | kStrum);
    session.tick(1.3, 0);
    CHECK(session.score() == 50 && session.streak() == 1 &&
              session.hits() == 1 && session.misses() == 0,
          "strumming a later matched note skips earlier in-window candidates without a miss");
  }

  {
    FoFiXGameplaySession session({
        {1.0, 1.0, kGreen, false, false},
        {1.05, 1.05, kGreen, false, false},
    });
    session.tick(1.03, kGreen | kStrum);
    CHECK(session.score() == 50 && session.hits() == 1,
          "single FoFiX strum consumes only one note group");
    session.tick(1.3, kGreen);
    CHECK(session.misses() == 1,
          "second in-window group remains live after the first strum hit");
  }

  {
    FoFiXGameplaySession session({
        {0.8, 0.8, kYellow, false, false},
        {1.0, 1.0, kGreen, false, false},
        {1.05, 1.05, kRed, true, false},
    });
    session.tick(0.8, kYellow | kStrum);
    session.tick(1.05, kRed);
    CHECK(session.score() == 50 && session.streak() == 1 &&
              session.hits() == 1,
          "HOPO cannot jump over an earlier unmatched in-window candidate");
    session.tick(1.3, 0);
    CHECK(session.misses() == 2 && session.streak() == 0,
          "blocked HOPO candidates still miss later through the normal miss path");
  }

  {
    FoFiXGameplaySession session({{1.0, 2.0, kGreen, false, false}});
    session.tick(1.0, kGreen | kStrum);
    session.tick(2.0, kGreen);
    CHECK(session.score() == 150,
          "one-second held sustain adds FoFiX sustain score");
    CHECK(session.last_events().size() == 1 &&
              session.last_events()[0].type ==
                  FoFiXSessionEventType::Sustain &&
              session.last_events()[0].mask == kGreen &&
              session.last_events()[0].score_delta == 100,
          "FoFiX session reports sustain delta for native presentation");
  }

  {
    FoFiXGameplaySession session({{1.0, 2.0, kGreen, false, false}});
    session.tick(1.0, kGreen | kStrum);
    session.tick(1.1, kGreen);
    session.tick(1.5, kGreen | kStrum);
    session.tick(2.0, kGreen);
    CHECK(session.score() == 50 && session.overstrums() == 1,
          "manual strum during sustain clears FoFiX tail bonus");
  }

  {
    FoFiXGameplaySession session({
        {1.0, 2.0, kGreen, false, false},
        {1.5, 1.5, kRed, false, false},
    });
    session.tick(1.0, kGreen | kStrum);
    session.tick(1.1, kGreen);
    session.tick(1.5, kRed | kStrum);
    session.tick(2.0, kRed);
    CHECK(session.score() == 100 && session.hits() == 2,
          "new hit during sustain does not award a repick tail bonus");
  }

  {
    FoFiXGameplaySession session({
        {1.0, 1.0, kGreen, false, true},
        {1.5, 1.5, kRed, false, true},
        {2.0, 2.0, kYellow, false, false},
    });
    session.tick(1.0, kGreen | kStrum);
    session.tick(1.1, kGreen);
    session.tick(1.5, kRed | kStrum);
    session.tick(1.6, kRed);
    session.tick(2.0, kYellow | kStrum);
    CHECK(session.star_power_fill() > 0.249 &&
              session.star_power_fill() < 0.251,
          "completed star phrase awards quarter meter at phrase boundary");
    CHECK(session.last_events().size() == 2 &&
              session.last_events()[0].type ==
                  FoFiXSessionEventType::StarPhraseComplete &&
              session.last_events()[0].star_power_fill > 0.249 &&
              session.last_events()[0].star_power_fill < 0.251 &&
              session.last_events()[1].type == FoFiXSessionEventType::Hit,
          "FoFiX session reports star phrase completion before boundary hit");
  }

  {
    FoFiXGameplaySession session({
        {1.0, 1.0, kGreen, false, true},
        {1.5, 1.5, kRed, false, false},
    });
    session.tick(1.0, kRed | kStrum);
    session.tick(1.005, kRed);
    session.tick(1.01, kGreen | kStrum);
    session.tick(1.1, kGreen);
    session.tick(1.5, kRed | kStrum);
    CHECK(session.overstrums() == 1 && session.hits() == 2 &&
              session.star_power_fill() == 0.0,
          "wrong strum inside a star note window breaks FoFiX star phrase");
    CHECK(session.last_events().size() == 2 &&
              session.last_events()[0].type ==
                  FoFiXSessionEventType::StarPhraseMiss &&
              session.last_events()[1].type == FoFiXSessionEventType::Hit,
          "FoFiX session reports missed star phrase before boundary hit");
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
    session.tick(1.1, kGreen);
    session.tick(1.5, kRed | kStrum);
    session.tick(1.6, kRed);
    session.tick(2.0, kGreen | kStrum);
    session.tick(2.1, kGreen);
    session.tick(2.5, kRed | kStrum);
    session.tick(2.6, kRed);
    session.tick(3.0, kGreen | kStrum);
    session.tick(3.1, kGreen);
    session.tick(3.5, kRed | kStrum);
    session.tick(3.6, kRed);
    session.tick(4.0, kGreen | kStrum);
    session.tick(4.1, kGreen);
    session.tick(4.5, kRed | kStrum);
    session.tick(4.6, kRed);
    CHECK(session.star_power_fill() > 0.999, "four phrases fill star meter");
    CHECK(!session.star_power_state().active &&
              session.star_power_state().value > 99.9,
          "session exposes raw full star meter for live gameplay adoption");
    session.tick(5.1, kStar);
    CHECK(session.star_power_active(), "star power activates at half or more");
    CHECK(session.star_power_state().active,
          "session exposes raw active star power state for live gameplay adoption");
    CHECK(session.last_events().size() == 1 &&
              session.last_events()[0].type ==
                  FoFiXSessionEventType::StarPowerActivate &&
              session.last_events()[0].mask == kStar,
          "FoFiX session reports star power activation for native presentation");
    const int before_powered_note = session.score();
    session.tick(5.2, kGreen | kStrum);
    CHECK(session.score() - before_powered_note == 100,
          "active star power doubles subsequent note score");
    CHECK(session.rock_state().value > 15000.0,
          "session exposes raw FoFiX rock state for live gameplay adoption");
    session.tick(25.2, 0);
    CHECK(!session.star_power_active() && session.star_power_fill() == 0.0,
          "star power drains out over time");
  }

  {
    ghogx::chart::Chart chart;
    chart.ticks_per_beat = 480;
    chart.tempo_map = {
        {0, 500000},
        {960, 1000000},
    };
    chart.notes[3] = {
        {0, 0, 0, false, false},
        {240, 240, 1, true, false},
        {960, 1440, 2, false, false},
    };

    FoFiXGameplaySession session =
        FoFiXGameplaySession::FromChart(chart, 3);
    session.tick(0.0, kGreen | kStrum);
    CHECK(session.last_events().size() == 1 &&
              session.last_events()[0].source_index == 0 &&
              session.last_events()[0].source_tick == 0,
          "chart-backed session reports source note index and tick for hit presentation");
    session.tick(0.25, kRed);
    CHECK(session.last_events().size() == 1 &&
              session.last_events()[0].source_index == 1 &&
              session.last_events()[0].source_tick == 240,
          "chart-backed session reports source tick for HOPO presentation");
    session.tick(0.847, kYellow | kStrum);
    session.tick(2.0, kYellow);
    CHECK(session.last_events().size() == 1 &&
              session.last_events()[0].type ==
                  FoFiXSessionEventType::Sustain &&
              session.last_events()[0].source_index == 2 &&
              session.last_events()[0].source_tick == 960,
          "chart-backed session reports source tick for sustain presentation");
    CHECK(session.score() == 250 && session.hits() == 3,
          "chart-backed session uses MIDI ticks, HOPOs, tempo hit windows, and sustain beat scale");
  }

  {
    FoFiXGameplaySession session({
        {1.0, 1.0, kGreen, false, false},
        {2.0, 2.0, kRed, false, false},
    });
    session.seek_without_scoring(1.3);
    CHECK(session.score() == 0 && session.misses() == 0,
          "diagnostic seek consumes earlier notes without scoring or miss penalties");
    session.tick(2.0, kRed | kStrum);
    CHECK(session.score() == 50 && session.hits() == 1,
          "session remains playable after no-score seek");
  }

  if (failures == 0) {
    std::fprintf(stderr, "gameplay_session_test: PASS\n");
  }
  return failures == 0 ? 0 : 1;
}
