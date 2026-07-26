// engine/src/chart/midi_reader_test.cpp
//
// Hermetic unit test for parse_midi. Builds a minimal synthetic SMF type 1
// file in memory and verifies that note count, timing, HOPO detection, and
// star power flags are correct.

#include "chart/midi_reader.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
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

    void meta_text(uint32_t abs_tick, const char* text) {
        push_vlq(ev, abs_tick - cursor);
        ev.push_back(0xFF);
        ev.push_back(0x01);
        const auto len = static_cast<uint8_t>(std::strlen(text));
        ev.push_back(len);
        for (int i = 0; i < static_cast<int>(len); ++i)
            ev.push_back(static_cast<uint8_t>(text[i]));
        cursor = abs_tick;
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
//   tick 120: Red   (97)  on/off at 240  — gap=120, FoFiX/GH2 170-tick cutoff → HOPO
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
//   tick 811: Red   (73)  on/off at 963  — gap=170 → within FoFiX/GH2 cutoff → HOPO
//
// Hard (pitch 84-88):
//   tick 1200: Green+Yellow chord starts a FoFiX after-chord tappable run.
//   tick 1320: Red is valid after the chord because it is not the chord top note.
//   tick 1440: Blue ends that tappable run.
//   tick 1800: Green+Yellow chord followed by Yellow repeats the top note, so no HOPO.
//   tick 2100: Red followed by a chord verifies chords are not accidentally playable HOPOs.
//   tick 2500: Red followed by Red verifies same-fret repeats stay strummed.
//
// Track 2 "TRIGGERS":
//   lighting_parser cue notes for next/prev/first keyframe.
//   effect_parser pitch 52 for world venue_effect.
//
// Track 3 "EVENTS":
//   same-tick world text markers, verifying authored order is stable.
//
// Track 4 "BAND SINGER":
//   same-tick performer text markers, verifying authored order is stable.
//
// ticks_per_beat = 480, tempo = 120 BPM (500000 us/beat).
// HOPO threshold = FoFiX/GH2 default 170 ticks.
static std::vector<uint8_t> build_test_smf() {
    // Track 0: tempo.
    TrackBuilder t0;
    t0.meta_tempo(500000);  // 120 BPM
    t0.meta_eot();

    // Track 1: guitar notes.
    TrackBuilder t1;
    t1.meta_name("PART GUITAR");

    // Events in ascending tick order:
    // tick 0: SP on (103), Expert Green on (96), Easy Green on (60),
    // and a player*_fret_pos cue (40 -> spot_neck_fret01).
    t1.note_on(0,   103);  // star power on
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
    t1.note_off(481, 103); // star power off
    // Medium notes (second note is exactly on the FoFiX/GH2 cutoff):
    t1.note_on (641, 72);  // Medium Green on
    t1.note_off(802, 72);  // Medium Green off
    t1.note_on (811, 73);  // Medium Red on
    t1.note_on (900, 59);  // fret-position animation spot 20
    t1.note_off(963, 73);  // Medium Red off
    // Hard after-chord valid run: G+Y chord -> R -> B.
    t1.note_on (1200, 84);  // Hard Green chord gem on
    t1.note_on (1200, 86);  // Hard Yellow chord gem on
    t1.note_off(1280, 84);
    t1.note_off(1280, 86);
    t1.note_on (1320, 85);  // Hard Red valid after-chord HOPO
    t1.note_off(1400, 85);
    t1.note_on (1440, 87);  // Hard Blue tappable run end
    t1.note_off(1520, 87);
    // Hard after-chord top-note repeat is not a HOPO.
    t1.note_on (1800, 84);
    t1.note_on (1800, 86);
    t1.note_off(1860, 84);
    t1.note_off(1860, 86);
    t1.note_on (1920, 86);  // Repeats chord top lane Yellow.
    t1.note_off(2000, 86);
    // Hard single -> chord should not mark the chord playable as a HOPO.
    t1.note_on (2100, 85);
    t1.note_off(2160, 85);
    t1.note_on (2220, 84);
    t1.note_on (2220, 87);
    t1.note_off(2300, 84);
    t1.note_off(2300, 87);
    // Hard same-fret repeat stays strummed.
    t1.note_on (2500, 85);
    t1.note_off(2560, 85);
    t1.note_on (2620, 85);
    t1.note_off(2680, 85);
    // GH1 midi_parsers.dtb singer EventList source.
    t1.note_on (2690, 108);
    t1.note_off(2750, 108);
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

    // Track 3: world text events in authored same-tick order.
    TrackBuilder t3;
    t3.meta_name("EVENTS");
    t3.meta_text(2400, "[camera_a]");
    t3.meta_text(2400, "[camera_b]");
    t3.meta_text(2410, "[camera_c]");
    t3.meta_eot();

    // Track 4: performer text events in authored same-tick order.
    TrackBuilder t4;
    t4.meta_name("BAND SINGER");
    t4.meta_text(2400, "[sing_idle]");
    t4.meta_text(2400, "[sing_phrase]");
    t4.meta_text(2410, "[sing_release]");
    t4.meta_eot();

    // Assemble SMF.
    std::vector<uint8_t> smf;
    smf.push_back('M'); smf.push_back('T'); smf.push_back('h'); smf.push_back('d');
    push_u32be(smf, 6);    // MThd length = 6
    push_u16be(smf, 1);    // format 1
    push_u16be(smf, 5);    // 5 tracks
    push_u16be(smf, 480);  // ticks per beat
    push_chunk(smf, "MTrk", t0.ev);
    push_chunk(smf, "MTrk", t1.ev);
    push_chunk(smf, "MTrk", t2.ev);
    push_chunk(smf, "MTrk", t3.ev);
    push_chunk(smf, "MTrk", t4.ev);
    return smf;
}

static std::vector<uint8_t> build_gh1_anim_smf() {
    TrackBuilder tempo;
    tempo.meta_tempo(500000);
    tempo.meta_eot();

    TrackBuilder gems;
    gems.meta_name("T1 GEMS");
    gems.note_on(0, 96);
    gems.note_off(120, 96);
    gems.meta_eot();

    TrackBuilder anim;
    anim.meta_name("ANIM");
    anim.meta_text(0, "HandMap_Default");
    anim.note_on(0, 40);
    anim.note_off(20, 40);
    anim.meta_text(480, "HandMap_Solo");
    anim.note_on(480, 59);
    anim.note_off(500, 59);
    anim.meta_text(960, "StrumMap_punk");
    anim.note_on(960, 60);
    anim.note_off(1020, 60);
    anim.meta_eot();

    TrackBuilder events;
    events.meta_name("EVENTS");
    events.meta_text(240, "StrumMap_softpick");
    events.meta_text(480, "gtr_on");
    events.meta_eot();

    std::vector<uint8_t> smf;
    smf.push_back('M'); smf.push_back('T'); smf.push_back('h'); smf.push_back('d');
    push_u32be(smf, 6);
    push_u16be(smf, 1);
    push_u16be(smf, 4);
    push_u16be(smf, 480);
    push_chunk(smf, "MTrk", tempo.ev);
    push_chunk(smf, "MTrk", gems.ev);
    push_chunk(smf, "MTrk", anim.ev);
    push_chunk(smf, "MTrk", events.ev);
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
        CHECK(n.hopo_tappable == 0, "Easy[0]: no FoFiX tappable class");
        CHECK(n.star_power,      "Easy[0]: inside SP region [0,481)");
        CHECK(n.final_star,      "Easy[0]: single-note phrase is final star");
    }

    // --- Medium: 2 notes, second is HOPO (gap=170 <= FoFiX/GH2 cutoff 170) ---
    CHECK(chart.notes[1].size() == 2, "Medium: 2 notes");
    if (chart.notes[1].size() >= 2) {
        CHECK(!chart.notes[1][0].is_hopo, "Medium[0]: not HOPO (first note)");
        CHECK(chart.notes[1][0].hopo_tappable == 1,
              "Medium[0]: FoFiX tappable starter class");
        CHECK(chart.notes[1][1].tick_on == 811, "Medium[1]: tick_on = 811");
        CHECK(chart.notes[1][1].is_hopo, "Medium[1]: HOPO (gap 170 <= 170)");
        CHECK(chart.notes[1][1].hopo_tappable == 3,
              "Medium[1]: FoFiX tappable end class");
        // Tick 641 is after SP end (481) -> not star power.
        CHECK(!chart.notes[1][0].star_power, "Medium[0]: not SP");
        CHECK(!chart.notes[1][1].star_power, "Medium[1]: not SP");
        CHECK(!chart.notes[1][0].final_star &&
              !chart.notes[1][1].final_star,
              "Medium notes outside SP are not final stars");
    }

    // --- Hard: FoFiX grouped HOPO edge cases ---
    CHECK(chart.notes[2].size() == 12, "Hard: 12 notes");
    if (chart.notes[2].size() == 12) {
        const auto& h = chart.notes[2];

        CHECK(h[0].lane == 0 && h[0].tick_on == 1200,
              "Hard[0]: Green chord gem @1200");
        CHECK(!h[0].is_hopo && h[0].hopo_tappable == 1,
              "Hard[0]: after-chord starter class, not playable HOPO");
        CHECK(h[1].lane == 2 && h[1].tick_on == 1200,
              "Hard[1]: Yellow chord top gem @1200");
        CHECK(!h[1].is_hopo && h[1].hopo_tappable == 1,
              "Hard[1]: all chord gems get FoFiX after-chord starter class");
        CHECK(h[2].lane == 1 && h[2].tick_on == 1320,
              "Hard[2]: Red valid after-chord single");
        CHECK(h[2].is_hopo && h[2].hopo_tappable == 2,
              "Hard[2]: after-chord single is tappable run middle");
        CHECK(h[3].lane == 3 && h[3].tick_on == 1440,
              "Hard[3]: Blue closes after-chord run");
        CHECK(h[3].is_hopo && h[3].hopo_tappable == 3,
              "Hard[3]: after-chord run terminal class");

        CHECK(h[4].lane == 0 && h[4].tick_on == 1800,
              "Hard[4]: invalid-repeat chord green");
        CHECK(h[5].lane == 2 && h[5].tick_on == 1800,
              "Hard[5]: invalid-repeat chord yellow");
        CHECK(!h[4].is_hopo && h[4].hopo_tappable == 0 &&
              !h[5].is_hopo && h[5].hopo_tappable == 0,
              "Hard[4-5]: chord without a valid following single stays strummed");
        CHECK(h[6].lane == 2 && h[6].tick_on == 1920,
              "Hard[6]: Yellow repeats prior chord top lane");
        CHECK(!h[6].is_hopo && h[6].hopo_tappable == 0,
              "Hard[6]: top-lane repeat after chord is not tappable");

        CHECK(h[7].lane == 1 && h[7].tick_on == 2100,
              "Hard[7]: Red before chord");
        CHECK(h[8].lane == 0 && h[8].tick_on == 2220 &&
              h[9].lane == 3 && h[9].tick_on == 2220,
              "Hard[8-9]: Green+Blue chord after a single");
        CHECK(!h[7].is_hopo && h[7].hopo_tappable == 0 &&
              !h[8].is_hopo && h[8].hopo_tappable == 0 &&
              !h[9].is_hopo && h[9].hopo_tappable == 0,
              "Hard[7-9]: single-to-chord does not create playable chord HOPOs");

        CHECK(h[10].lane == 1 && h[10].tick_on == 2500 &&
              h[11].lane == 1 && h[11].tick_on == 2620,
              "Hard[10-11]: same-fret Red repeat pair");
        CHECK(!h[10].is_hopo && h[10].hopo_tappable == 0 &&
              !h[11].is_hopo && h[11].hopo_tappable == 0,
              "Hard[10-11]: same-fret repeats stay strummed");
    }

    // --- Expert: 5 notes ---
    CHECK(chart.notes[3].size() == 5, "Expert: 5 notes");
    if (chart.notes[3].size() == 5) {
        const auto& e = chart.notes[3];

        CHECK(e[0].lane == 0 && e[0].tick_on == 0,   "Expert[0]: Green@0");
        CHECK(!e[0].is_hopo,                          "Expert[0]: not HOPO (first)");
        CHECK(e[0].hopo_tappable == 1,                "Expert[0]: tappable run starter");
        CHECK(e[0].star_power,                        "Expert[0]: SP");
        CHECK(!e[0].final_star,                       "Expert[0]: not final SP note");

        CHECK(e[1].lane == 1 && e[1].tick_on == 120,  "Expert[1]: Red@120");
        CHECK(e[1].is_hopo,                            "Expert[1]: HOPO (gap 120<=170)");
        CHECK(e[1].hopo_tappable == 2,                 "Expert[1]: tappable run middle");
        CHECK(e[1].star_power,                         "Expert[1]: SP");
        CHECK(!e[1].final_star,                        "Expert[1]: not final SP note");

        CHECK(e[2].lane == 2 && e[2].tick_on == 240,  "Expert[2]: Yellow@240");
        CHECK(e[2].is_hopo,                            "Expert[2]: HOPO (gap 120<=170)");
        CHECK(e[2].hopo_tappable == 2,                 "Expert[2]: tappable run middle");
        CHECK(e[2].star_power,                         "Expert[2]: SP");
        CHECK(!e[2].final_star,                        "Expert[2]: not final SP note");

        CHECK(e[3].lane == 3 && e[3].tick_on == 244,  "Expert[3]: Blue@244");
        CHECK(e[3].is_hopo,                            "Expert[3]: HOPO (gap 4<=170)");
        CHECK(e[3].hopo_tappable == 2,                 "Expert[3]: tappable run middle");
        CHECK(e[3].star_power,                         "Expert[3]: SP");
        CHECK(!e[3].final_star,                        "Expert[3]: not final SP note");

        CHECK(e[4].lane == 4 && e[4].tick_on == 360,  "Expert[4]: Orange@360");
        CHECK(e[4].is_hopo,                            "Expert[4]: HOPO (gap 116<=170)");
        CHECK(e[4].hopo_tappable == 3,                 "Expert[4]: tappable run end");
        CHECK(e[4].star_power,                         "Expert[4]: SP (tick 360 < 481)");
        CHECK(e[4].final_star,                         "Expert[4]: final SP note");
    }

    // --- Duration: last note off at tick 2680 ---
    // 2680 ticks @ 120 BPM, 480 tpb -> 2680/480 * 0.5 s
    const double expected_dur = 2680.0 / 480.0 * 0.5;
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

    // --- same-tick script text ordering ---
    CHECK(chart.text_events.size() == 3, "World text events: 3");
    if (chart.text_events.size() == 3) {
        CHECK(chart.text_events[0].tick == 2400 &&
              chart.text_events[0].text == "[camera_a]",
              "TextEvent[0]: same-tick authored order A");
        CHECK(chart.text_events[1].tick == 2400 &&
              chart.text_events[1].text == "[camera_b]",
              "TextEvent[1]: same-tick authored order B");
        CHECK(chart.text_events[2].tick == 2410 &&
              chart.text_events[2].text == "[camera_c]",
              "TextEvent[2]: later marker remains after same-tick rows");
    }

    CHECK(chart.performer_events.size() == 3, "Performer text events: 3");
    if (chart.performer_events.size() == 3) {
        CHECK(chart.performer_events[0].tick == 2400 &&
              chart.performer_events[0].track == "BAND SINGER" &&
              chart.performer_events[0].text == "[sing_idle]",
              "PerformerEvent[0]: same-tick authored order idle");
        CHECK(chart.performer_events[1].tick == 2400 &&
              chart.performer_events[1].track == "BAND SINGER" &&
              chart.performer_events[1].text == "[sing_phrase]",
              "PerformerEvent[1]: same-tick authored order phrase");
        CHECK(chart.performer_events[2].tick == 2410 &&
              chart.performer_events[2].track == "BAND SINGER" &&
              chart.performer_events[2].text == "[sing_release]",
              "PerformerEvent[2]: later marker remains after same-tick rows");
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

    // --- GH1 singer DataEventList cue ---
    CHECK(chart.singer_face_cues.size() == 1,
          "GH1 singer face cues: 1");
    if (chart.singer_face_cues.size() == 1) {
        CHECK(chart.singer_face_cues[0].tick_on == 2690 &&
              chart.singer_face_cues[0].tick_off == 2750 &&
              chart.singer_face_cues[0].pitch == 108,
              "SingerFace[0]: preserve pitch 108 authored span");
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

    const ghogx::chart::Chart gh1_anim_chart =
        ghogx::chart::parse_midi(build_gh1_anim_smf());
    CHECK(gh1_anim_chart.hand_map_cues.size() == 2,
          "GH1 ANIM HandMap cues: 2");
    if (gh1_anim_chart.hand_map_cues.size() == 2) {
        CHECK(gh1_anim_chart.hand_map_cues[0].tick == 0 &&
                  gh1_anim_chart.hand_map_cues[0].map == "HandMap_Default",
              "GH1 HandMap default at tick 0");
        CHECK(gh1_anim_chart.hand_map_cues[1].tick == 480 &&
                  gh1_anim_chart.hand_map_cues[1].map == "HandMap_Solo",
              "GH1 HandMap solo at tick 480");
    }
    CHECK(gh1_anim_chart.strum_map_cues.size() == 2,
          "GH1 ANIM/EVENTS StrumMap cues: 2");
    if (gh1_anim_chart.strum_map_cues.size() == 2) {
        CHECK(gh1_anim_chart.strum_map_cues[0].tick == 240 &&
                  gh1_anim_chart.strum_map_cues[0].map ==
                      "StrumMap_softpick",
              "GH1 EVENTS StrumMap cue");
        CHECK(gh1_anim_chart.strum_map_cues[1].tick == 960 &&
                  gh1_anim_chart.strum_map_cues[1].map == "StrumMap_punk",
              "GH1 ANIM StrumMap cue");
    }
    CHECK(gh1_anim_chart.fret_positions.size() == 2,
          "GH1 ANIM fret-position cues: 2");
    if (gh1_anim_chart.fret_positions.size() == 2) {
        CHECK(gh1_anim_chart.fret_positions[0].spot_index == 1 &&
                  gh1_anim_chart.fret_positions[1].spot_index == 20,
              "GH1 ANIM pitches 40/59 map to fret spots 1/20");
    }
    CHECK(gh1_anim_chart.hand_animation_cues.size() == 1 &&
              gh1_anim_chart.hand_animation_cues[0].tick_on == 960 &&
              gh1_anim_chart.hand_animation_cues[0].tick_off == 1020 &&
              gh1_anim_chart.hand_animation_cues[0].pitch == 60,
          "GH1 ANIM explicit pitch-60 hand span");
    CHECK(gh1_anim_chart.performer_events.empty(),
          "GH1 ANIM mapper text is not a performer-state stream");

    if (failures == 0)
        std::fprintf(stderr, "midi_reader_test: ALL PASS\n");
    else
        std::fprintf(stderr, "midi_reader_test: %d FAILURE(S)\n", failures);

    return (failures != 0) ? 1 : 0;
}
