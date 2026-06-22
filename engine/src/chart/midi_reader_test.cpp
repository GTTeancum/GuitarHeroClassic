// engine/src/chart/midi_reader_test.cpp
//
// Hermetic unit test for parse_midi. Builds a minimal synthetic SMF type 1
// file in memory and verifies that note count, timing, HOPO detection, and
// star power flags are correct.

#include "chart/midi_reader.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

// ---------------------------------------------------------------------------
// Helper: build a synthetic SMF byte buffer
// ---------------------------------------------------------------------------

namespace {

static void push_u32be(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >>  8) & 0xFF));
    v.push_back(static_cast<uint8_t>(x & 0xFF));
}

static void push_u16be(std::vector<uint8_t>& v, uint16_t x) {
    v.push_back(static_cast<uint8_t>((x >> 8) & 0xFF));
    v.push_back(static_cast<uint8_t>(x & 0xFF));
}

// Encode a MIDI variable-length quantity.
static void push_vlq(std::vector<uint8_t>& v, uint32_t val) {
    uint8_t buf[4];
    int n = 0;
    buf[n++] = static_cast<uint8_t>(val & 0x7F);
    val >>= 7;
    while (val > 0) {
        buf[n++] = static_cast<uint8_t>(0x80 | (val & 0x7F));
        val >>= 7;
    }
    for (int i = n - 1; i >= 0; --i)
        v.push_back(buf[i]);
}

// Emit a MIDI event into a track event buffer.
// Note: caller must provide the correct absolute tick for delta computation.
struct TrackBuilder {
    std::vector<uint8_t> ev;
    uint32_t cursor = 0;

    void note_on(uint32_t abs_tick, uint8_t pitch, uint8_t vel = 100) {
        push_vlq(ev, abs_tick - cursor);
        ev.push_back(0x90);  // channel 0 note-on
        ev.push_back(pitch);
        ev.push_back(vel);
        cursor = abs_tick;
    }

    void note_off(uint32_t abs_tick, uint8_t pitch) {
        push_vlq(ev, abs_tick - cursor);
        ev.push_back(0x80);  // channel 0 note-off
        ev.push_back(pitch);
        ev.push_back(0);
        cursor = abs_tick;
    }

    void meta_name(const char* name) {
        push_vlq(ev, 0);  // delta 0
        ev.push_back(0xFF);
        ev.push_back(0x03);  // track name
        const auto len = static_cast<uint8_t>(std::strlen(name));
        ev.push_back(len);
        for (int i = 0; i < static_cast<int>(len); ++i)
            ev.push_back(static_cast<uint8_t>(name[i]));
    }

    void meta_tempo(uint32_t us_per_beat) {
        push_vlq(ev, 0);
        ev.push_back(0xFF);
        ev.push_back(0x51);
        ev.push_back(0x03);
        ev.push_back(static_cast<uint8_t>((us_per_beat >> 16) & 0xFF));
        ev.push_back(static_cast<uint8_t>((us_per_beat >>  8) & 0xFF));
        ev.push_back(static_cast<uint8_t>(us_per_beat & 0xFF));
    }

    void meta_eot() {
        push_vlq(ev, 0);
        ev.push_back(0xFF);
        ev.push_back(0x2F);
        ev.push_back(0x00);
    }
};

// Build a chunk: 4-byte tag + 4-byte length + data.
static void push_chunk(std::vector<uint8_t>& out, const char* tag,
                        const std::vector<uint8_t>& data) {
    out.push_back(static_cast<uint8_t>(tag[0]));
    out.push_back(static_cast<uint8_t>(tag[1]));
    out.push_back(static_cast<uint8_t>(tag[2]));
    out.push_back(static_cast<uint8_t>(tag[3]));
    push_u32be(out, static_cast<uint32_t>(data.size()));
    out.insert(out.end(), data.begin(), data.end());
}

// Build a complete SMF type 1 file with:
//
// Track 0: tempo track (120 BPM).
// Track 1 "PART GUITAR":
//
// Expert (pitch 96-100):
//   tick 0:   Green (96)  on/off at 120  — first note, not HOPO
//   tick 120: Red   (97)  on/off at 240  — gap=120, 120/3=160 threshold → 120 < 160 → HOPO
//   tick 240: Yellow(98)  on/off at 244  — gap=120 → HOPO
//   tick 244: Blue  (99)  on/off at 360  — gap=4 → HOPO
//   tick 360: Orange(100) on/off at 480  — gap=116 → HOPO
//   (All notes inside SP region [0, 481])
//
// Easy (pitch 60-64):
//   tick 0:   Green (60)  on/off at 121  — 1 note total, not HOPO (first note), inside SP
//
// Medium (pitch 72-76):
//   tick 641: Green (72)  on/off at 802  — gap=161 from start (no prev note) → not HOPO
//   tick 802: Red   (73)  on/off at 963  — gap=161 → 161 > 160 → not HOPO
//
// Track 2 "TRIGGERS":
//   lighting_parser cue notes for next/prev/first keyframe.
//   effect_parser pitch 52 for world venue_effect.
//
// ticks_per_beat = 480, tempo = 120 BPM (500000 us/beat).
// HOPO threshold = 480/3 = 160 ticks.
static std::vector<uint8_t> build_test_smf() {
    // Track 0: tempo.
    TrackBuilder t0;
    t0.meta_tempo(500000);  // 120 BPM
    t0.meta_eot();

    // Track 1: guitar notes.
    TrackBuilder t1;
    t1.meta_name("PART GUITAR");

    // Events in ascending tick order:
    // tick 0: SP on (116), Expert Green on (96), Easy Green on (60),
    // and a player*_fret_pos cue (40 -> spot_neck_fret01).
    t1.note_on(0,   116);  // star power on
    t1.note_on(0,   96);   // Expert Green on
    t1.note_on(0,   60);   // Easy Green on
    t1.note_on(0,   40);   // fret-position animation spot 1
    t1.note_on(60,  44);   // dense fret-position cue filtered by min_gap
    // tick 120: Expert Green off, Red on.
    t1.note_off(120, 96);
    t1.note_on (120, 97);  // Expert Red on
    // tick 121: Easy Green off.
    t1.note_off(121, 60);
    // tick 240: Expert Red off, Yellow on.
    t1.note_off(240, 97);
    t1.note_on (240, 98);  // Expert Yellow on
    // tick 244: Expert Yellow off, Blue on.
    t1.note_off(244, 98);
    t1.note_on (244, 99);  // Expert Blue on
    // tick 360: Expert Blue off, Orange on.
    t1.note_off(360, 99);
    t1.note_on (360, 100); // Expert Orange on
    // tick 480: Expert Orange off.
    t1.note_off(480, 100);
    // tick 481: SP off (so Orange at 360 is inside [0,481)).
    t1.note_off(481, 116); // star power off
    // Medium notes (no HOPO, gap=161):
    t1.note_on (641, 72);  // Medium Green on
    t1.note_off(802, 72);  // Medium Green off
    t1.note_on (802, 73);  // Medium Red on
    t1.note_on (900, 59);  // fret-position animation spot 20
    t1.note_off(963, 73);  // Medium Red off
    t1.meta_eot();

    // Track 2: lighting trigger notes.
    TrackBuilder t2;
    t2.meta_name("TRIGGERS");
    t2.note_on (3600, 52); // venue_effect, no parser offset
    t2.note_off(3600, 52);
    t2.note_on (4800, 50); // lighting first, minus traced 4s => tick 960
    t2.note_off(4800, 50);
    t2.note_on (5280, 48); // lighting next, minus traced 4s => tick 1440
    t2.note_off(5280, 48);
    t2.note_on (5760, 49); // lighting prev, minus traced 4s => tick 1920
    t2.note_off(5760, 49);
    t2.meta_eot();

    // Assemble SMF.
    std::vector<uint8_t> smf;
    smf.push_back('M'); smf.push_back('T'); smf.push_back('h'); smf.push_back('d');
    push_u32be(smf, 6);    // MThd length = 6
    push_u16be(smf, 1);    // format 1
    push_u16be(smf, 3);    // 3 tracks
    push_u16be(smf, 480);  // ticks per beat
    push_chunk(smf, "MTrk", t0.ev);
    push_chunk(smf, "MTrk", t1.ev);
    push_chunk(smf, "MTrk", t2.ev);
    return smf;
}

}  // namespace

// ---------------------------------------------------------------------------
// Test harness
// ---------------------------------------------------------------------------

static int failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "FAIL: %s  (at line %d)\n", (msg), __LINE__); \
            ++failures; \
        } \
    } while (0)

int main() {
    std::fprintf(stderr, "midi_reader_test: building synthetic SMF...\n");

    const std::vector<uint8_t> smf = build_test_smf();
    std::fprintf(stderr, "midi_reader_test: SMF size = %zu bytes\n", smf.size());

    const ghogx::chart::Chart chart = ghogx::chart::parse_midi(smf);

    // --- Timing infrastructure ---
    CHECK(chart.ticks_per_beat == 480, "ticks_per_beat == 480");

    // At 120 BPM: 480 ticks = 0.5 seconds. tick 120 = 120/480*0.5 = 0.125 s.
    const double t120 = chart.tick_to_sec(120);
    const double kEps = 1e-6;
    CHECK(std::abs(t120 - 0.125) < kEps, "tick 120 => 0.125 s at 120 BPM");

    // --- Easy: 1 note ---
    CHECK(chart.notes[0].size() == 1, "Easy: 1 note");
    if (!chart.notes[0].empty()) {
        const auto& n = chart.notes[0][0];
        CHECK(n.lane == 0,       "Easy[0]: lane = Green (0)");
        CHECK(n.tick_on  == 0,   "Easy[0]: tick_on = 0");
        CHECK(n.tick_off == 121, "Easy[0]: tick_off = 121");
        CHECK(!n.is_hopo,        "Easy[0]: first note not HOPO");
        CHECK(n.star_power,      "Easy[0]: inside SP region [0,481)");
    }

    // --- Medium: 2 notes, neither HOPO (gap=161 > threshold 160) ---
    CHECK(chart.notes[1].size() == 2, "Medium: 2 notes");
    if (chart.notes[1].size() >= 2) {
        CHECK(!chart.notes[1][0].is_hopo, "Medium[0]: not HOPO (first note)");
        CHECK(!chart.notes[1][1].is_hopo, "Medium[1]: not HOPO (gap 161 > 160)");
        // Tick 641 is after SP end (481) -> not star power.
        CHECK(!chart.notes[1][0].star_power, "Medium[0]: not SP");
        CHECK(!chart.notes[1][1].star_power, "Medium[1]: not SP");
    }

    // --- Hard: 0 notes ---
    CHECK(chart.notes[2].empty(), "Hard: 0 notes");

    // --- Expert: 5 notes ---
    CHECK(chart.notes[3].size() == 5, "Expert: 5 notes");
    if (chart.notes[3].size() == 5) {
        const auto& e = chart.notes[3];

        CHECK(e[0].lane == 0 && e[0].tick_on == 0,   "Expert[0]: Green@0");
        CHECK(!e[0].is_hopo,                          "Expert[0]: not HOPO (first)");
        CHECK(e[0].star_power,                        "Expert[0]: SP");

        CHECK(e[1].lane == 1 && e[1].tick_on == 120,  "Expert[1]: Red@120");
        CHECK(e[1].is_hopo,                            "Expert[1]: HOPO (gap 120<160)");
        CHECK(e[1].star_power,                         "Expert[1]: SP");

        CHECK(e[2].lane == 2 && e[2].tick_on == 240,  "Expert[2]: Yellow@240");
        CHECK(e[2].is_hopo,                            "Expert[2]: HOPO (gap 120<160)");
        CHECK(e[2].star_power,                         "Expert[2]: SP");

        CHECK(e[3].lane == 3 && e[3].tick_on == 244,  "Expert[3]: Blue@244");
        CHECK(e[3].is_hopo,                            "Expert[3]: HOPO (gap 4<160)");
        CHECK(e[3].star_power,                         "Expert[3]: SP");

        CHECK(e[4].lane == 4 && e[4].tick_on == 360,  "Expert[4]: Orange@360");
        CHECK(e[4].is_hopo,                            "Expert[4]: HOPO (gap 116<160)");
        CHECK(e[4].star_power,                         "Expert[4]: SP (tick 360 < 481)");
    }

    // --- Duration: last note off at tick 963 ---
    // 963 ticks @ 120 BPM, 480 tpb -> 963/480 * 0.5 s = 1.003125 s
    const double expected_dur = 963.0 / 480.0 * 0.5;
    const double dur = chart.duration_sec();
    CHECK(std::abs(dur - expected_dur) < 1e-4, "duration = last note-off time");

    // --- Lighting parser cues from TRIGGERS pitch 48/49/50 ---
    CHECK(chart.lighting_cues.size() == 3, "Lighting cues: 3");
    if (chart.lighting_cues.size() == 3) {
        CHECK(chart.lighting_cues[0].event == "first" &&
              chart.lighting_cues[0].tick == 960,
              "Lighting[0]: first at tick 960 after traced -4s offset");
        CHECK(chart.lighting_cues[1].event == "next" &&
              chart.lighting_cues[1].tick == 1440,
              "Lighting[1]: next at tick 1440 after traced -4s offset");
        CHECK(chart.lighting_cues[2].event == "prev" &&
              chart.lighting_cues[2].tick == 1920,
              "Lighting[2]: prev at tick 1920 after traced -4s offset");
    }

    // --- effect_parser cue from TRIGGERS pitch 52 ---
    CHECK(chart.venue_cues.size() == 1, "Venue cues: 1");
    if (chart.venue_cues.size() == 1) {
        CHECK(chart.venue_cues[0].event == "venue_effect" &&
              chart.venue_cues[0].pitch == 52 &&
              chart.venue_cues[0].tick == 3600,
              "VenueCue[0]: venue_effect at authored tick 3600");
    }

    // --- player*_fret_pos cues from PART GUITAR pitch 40..59 ---
    // TRIGGERS pitch 50 above is a lighting cue and must not leak into this
    // selected-guitar-track stream. The accepted GH2DXu parser also declares
    // min_gap 0.22, so the tick-60 dense cue is intentionally filtered.
    CHECK(chart.fret_positions.size() == 2, "Fret position cues: 2");
    if (chart.fret_positions.size() == 2) {
        CHECK(chart.fret_positions[0].tick == 0 &&
              chart.fret_positions[0].pitch == 40 &&
              chart.fret_positions[0].spot_index == 1,
              "FretPos[0]: pitch 40 -> spot 1");
        CHECK(chart.fret_positions[1].tick == 900 &&
              chart.fret_positions[1].pitch == 59 &&
              chart.fret_positions[1].spot_index == 20,
              "FretPos[1]: pitch 59 -> spot 20");
    }

    // --- player*_fret hand-driver cues from selected guitar gems ---
    // This is separate from the player*_fret_pos MIDI notes above; it feeds
    // left_hand.drv clip scheduling through GUITARFRETMAPPINGS. The tick-244
    // Expert Blue gem is filtered by parser min_gap 0.12s after tick 240.
    CHECK(chart.fret_hand_cues[3].size() == 4,
          "Expert player_fret hand cues: 4");
    if (chart.fret_hand_cues[3].size() == 4) {
        CHECK(chart.fret_hand_cues[3][0].tick == 0 &&
              chart.fret_hand_cues[3][0].mask == 0x01,
              "HandCue[0]: Expert Green");
        CHECK(chart.fret_hand_cues[3][1].tick == 120 &&
              chart.fret_hand_cues[3][1].mask == 0x02,
              "HandCue[1]: Expert Red");
        CHECK(chart.fret_hand_cues[3][2].tick == 240 &&
              chart.fret_hand_cues[3][2].mask == 0x04,
              "HandCue[2]: Expert Yellow");
        CHECK(chart.fret_hand_cues[3][3].tick == 360 &&
              chart.fret_hand_cues[3][3].mask == 0x10,
              "HandCue[3]: Expert Orange");
    }

    if (failures == 0)
        std::fprintf(stderr, "midi_reader_test: ALL PASS\n");
    else
        std::fprintf(stderr, "midi_reader_test: %d FAILURE(S)\n", failures);

    return (failures != 0) ? 1 : 0;
}
