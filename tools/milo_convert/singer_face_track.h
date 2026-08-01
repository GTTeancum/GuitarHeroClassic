#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gh::milo_convert {

struct SingerFaceTickSpan {
    uint32_t tick_on = 0;
    uint32_t tick_off = 0;
};

struct SingerFaceTimeSpan {
    float time_on = 0.0f;
    float time_off = 0.0f;
};

struct FaceFxAnimationKey {
    float time = 0.0f;
    float value = 0.0f;
};

struct FaceFxAnimationCurve {
    std::string name;
    std::vector<FaceFxAnimationKey> keys;
};

struct FaceFxAnimation {
    uint32_t version = 0;
    std::string name;
    std::vector<FaceFxAnimationCurve> curves;
    // Version-scoped archive footer retained for explicit residual accounting:
    // 14 bytes in v1200 and 36 bytes in v1500 across the GH2 PS2 corpus.
    std::vector<uint8_t> archive_footer;
};

// Strict reader for the two GH2 song .voc FACE animation revisions observed
// in the packed corpus (1200 and 1500).
FaceFxAnimation parse_gh2_facefx_animation(
    const std::vector<uint8_t>& bytes);

// Reduces the 15 non-neutral GH2 singer viseme curves to the binary open/ref
// contract authored by GH1. A span is open whenever any mouth-viseme curve is
// above zero under the target's piecewise-linear sampler.
std::vector<SingerFaceTimeSpan>
derive_gh1_singer_open_spans(
    const FaceFxAnimation& animation);

// Extracts the exact GH1 pitch-108 spans from PART GUITAR/T1 GEMS.
std::vector<SingerFaceTickSpan>
extract_gh1_singer_face_spans(
    const std::vector<uint8_t>& midi);

// Converts .voc-derived seconds to source MIDI ticks using its complete tempo
// map, then merges intervals changed only by tick quantization.
std::vector<SingerFaceTickSpan>
map_singer_face_times_to_midi(
    const std::vector<uint8_t>& midi,
    const std::vector<SingerFaceTimeSpan>& spans);

// Preserves every source MIDI byte except the header track count and appends
// one deterministic "GH1 SINGER FACE" MTrk. Zero-length pitch 108/109 notes
// carry open/close events for the patched target MIDI parser.
std::vector<uint8_t> append_gh1_singer_face_track(
    const std::vector<uint8_t>& midi,
    const std::vector<SingerFaceTickSpan>& spans);

}  // namespace gh::milo_convert
