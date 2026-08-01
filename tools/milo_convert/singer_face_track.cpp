#include "singer_face_track.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>

namespace gh::milo_convert {
namespace {

uint16_t read_be16(
    const std::vector<uint8_t>& bytes, size_t offset) {
    if (offset + 2 > bytes.size())
        throw std::runtime_error("singer face: truncated MIDI u16");
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(bytes[offset]) << 8) |
        bytes[offset + 1]);
}

uint32_t read_be32(
    const std::vector<uint8_t>& bytes, size_t offset) {
    if (offset + 4 > bytes.size())
        throw std::runtime_error("singer face: truncated MIDI u32");
    return
        (static_cast<uint32_t>(bytes[offset]) << 24) |
        (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
        (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
        bytes[offset + 3];
}

void append_be32(
    std::vector<uint8_t>& output, uint32_t value) {
    output.push_back(static_cast<uint8_t>(value >> 24));
    output.push_back(static_cast<uint8_t>(value >> 16));
    output.push_back(static_cast<uint8_t>(value >> 8));
    output.push_back(static_cast<uint8_t>(value));
}

uint32_t read_vlq(
    const std::vector<uint8_t>& bytes,
    size_t& offset,
    size_t end) {
    uint32_t value = 0;
    for (int index = 0; index < 4; ++index) {
        if (offset >= end)
            throw std::runtime_error(
                "singer face: truncated MIDI VLQ");
        const uint8_t byte = bytes[offset++];
        if (value > 0x0FFFFFFFu >> 7)
            throw std::runtime_error(
                "singer face: MIDI VLQ overflow");
        value = (value << 7) | (byte & 0x7Fu);
        if ((byte & 0x80u) == 0) return value;
    }
    throw std::runtime_error("singer face: invalid MIDI VLQ");
}

void append_vlq(
    std::vector<uint8_t>& output, uint32_t value) {
    std::array<uint8_t, 4> encoded{};
    size_t count = 1;
    encoded[3] = static_cast<uint8_t>(value & 0x7Fu);
    while ((value >>= 7) != 0) {
        if (count == encoded.size())
            throw std::runtime_error(
                "singer face: MIDI delta exceeds VLQ");
        encoded[3 - count] =
            static_cast<uint8_t>((value & 0x7Fu) | 0x80u);
        ++count;
    }
    output.insert(
        output.end(), encoded.end() - count, encoded.end());
}

struct MidiNoteSpan {
    uint32_t tick_on = 0;
    uint32_t tick_off = 0;
    uint8_t channel = 0;
    uint8_t pitch = 0;
};

struct TempoEvent {
    uint32_t tick = 0;
    uint32_t microseconds_per_quarter = 500000;
    size_t order = 0;
};

struct MidiTrack {
    std::string name;
    std::vector<MidiNoteSpan> notes;
    std::vector<TempoEvent> tempos;
};

struct ParsedMidi {
    uint16_t format = 0;
    uint16_t declared_track_count = 0;
    uint16_t division = 0;
    std::vector<MidiTrack> tracks;
};

ParsedMidi parse_midi(
    const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 14 ||
        std::memcmp(bytes.data(), "MThd", 4) != 0)
        throw std::runtime_error(
            "singer face: missing MIDI header");
    const uint32_t header_size = read_be32(bytes, 4);
    if (header_size < 6 || 8ull + header_size > bytes.size())
        throw std::runtime_error(
            "singer face: invalid MIDI header size");
    ParsedMidi result;
    result.format = read_be16(bytes, 8);
    result.declared_track_count = read_be16(bytes, 10);
    result.division = read_be16(bytes, 12);
    if ((result.division & 0x8000u) != 0 || result.division == 0)
        throw std::runtime_error(
            "singer face: SMPTE/zero MIDI division unsupported");

    size_t offset = 8 + header_size;
    size_t tempo_order = 0;
    while (offset < bytes.size() &&
           result.tracks.size() < result.declared_track_count) {
        if (offset + 8 > bytes.size() ||
            std::memcmp(bytes.data() + offset, "MTrk", 4) != 0)
            throw std::runtime_error(
                "singer face: missing MIDI track chunk");
        const uint32_t size = read_be32(bytes, offset + 4);
        const size_t begin = offset + 8;
        const size_t end = begin + size;
        if (end > bytes.size())
            throw std::runtime_error(
                "singer face: truncated MIDI track");
        MidiTrack track;
        uint32_t tick = 0;
        uint8_t running_status = 0;
        std::map<uint16_t, std::vector<uint32_t>> active;
        size_t cursor = begin;
        while (cursor < end) {
            const uint32_t delta = read_vlq(bytes, cursor, end);
            if (tick > std::numeric_limits<uint32_t>::max() - delta)
                throw std::runtime_error(
                    "singer face: MIDI tick overflow");
            tick += delta;
            if (cursor >= end)
                throw std::runtime_error(
                    "singer face: MIDI event missing status");
            uint8_t status = bytes[cursor];
            bool used_running_status = false;
            if (status < 0x80u) {
                if (running_status < 0x80u ||
                    running_status >= 0xF0u)
                    throw std::runtime_error(
                        "singer face: invalid MIDI running status");
                status = running_status;
                used_running_status = true;
            } else {
                ++cursor;
                if (status < 0xF0u)
                    running_status = status;
                // GH2 PS2 files intentionally retain channel running status
                // across Meta and SysEx records.
            }
            if (status == 0xFFu) {
                if (cursor >= end)
                    throw std::runtime_error(
                        "singer face: truncated MIDI meta event");
                const uint8_t type = bytes[cursor++];
                const uint32_t length =
                    read_vlq(bytes, cursor, end);
                if (length > end - cursor)
                    throw std::runtime_error(
                        "singer face: truncated MIDI meta payload");
                if (type == 0x03u)
                    track.name.assign(
                        reinterpret_cast<const char*>(
                            bytes.data() + cursor),
                        length);
                if (type == 0x51u && length == 3) {
                    const uint32_t tempo =
                        (static_cast<uint32_t>(bytes[cursor]) << 16) |
                        (static_cast<uint32_t>(bytes[cursor + 1]) << 8) |
                        bytes[cursor + 2];
                    if (tempo == 0)
                        throw std::runtime_error(
                            "singer face: zero MIDI tempo");
                    track.tempos.push_back(
                        {tick, tempo, tempo_order++});
                }
                cursor += length;
                continue;
            }
            if (status == 0xF0u || status == 0xF7u) {
                const uint32_t length =
                    read_vlq(bytes, cursor, end);
                if (length > end - cursor)
                    throw std::runtime_error(
                        "singer face: truncated MIDI SysEx");
                cursor += length;
                continue;
            }
            const uint8_t command = status & 0xF0u;
            const uint8_t channel = status & 0x0Fu;
            const size_t data_size =
                (command == 0xC0u || command == 0xD0u) ? 1 : 2;
            if (cursor + data_size > end)
                throw std::runtime_error(
                    "singer face: truncated MIDI channel event");
            const uint8_t data0 = bytes[cursor++];
            const uint8_t data1 =
                data_size == 2 ? bytes[cursor++] : 0;
            (void)used_running_status;
            if (command != 0x80u && command != 0x90u)
                continue;
            const uint16_t key =
                static_cast<uint16_t>((channel << 8) | data0);
            const bool note_on =
                command == 0x90u && data1 != 0;
            if (note_on) {
                active[key].push_back(tick);
                continue;
            }
            auto found = active.find(key);
            if (found == active.end() || found->second.empty())
                continue;
            const uint32_t tick_on = found->second.front();
            found->second.erase(found->second.begin());
            track.notes.push_back(
                {tick_on, tick, channel, data0});
        }
        result.tracks.push_back(std::move(track));
        offset = end;
    }
    if (result.tracks.size() != result.declared_track_count)
        throw std::runtime_error(
            "singer face: MIDI track count mismatch");
    return result;
}

std::vector<SingerFaceTickSpan> merge_tick_spans(
    std::vector<SingerFaceTickSpan> spans) {
    spans.erase(
        std::remove_if(
            spans.begin(), spans.end(),
            [](const SingerFaceTickSpan& span) {
                return span.tick_off <= span.tick_on;
            }),
        spans.end());
    std::sort(
        spans.begin(), spans.end(),
        [](const SingerFaceTickSpan& left,
           const SingerFaceTickSpan& right) {
            if (left.tick_on != right.tick_on)
                return left.tick_on < right.tick_on;
            return left.tick_off < right.tick_off;
        });
    std::vector<SingerFaceTickSpan> merged;
    for (const auto& span : spans) {
        if (merged.empty() ||
            span.tick_on > merged.back().tick_off) {
            merged.push_back(span);
        } else {
            merged.back().tick_off =
                std::max(merged.back().tick_off, span.tick_off);
        }
    }
    return merged;
}

class FaceReader {
public:
    explicit FaceReader(const std::vector<uint8_t>& bytes)
        : bytes_(bytes) {}

    uint16_t u16() {
        need(2);
        const uint16_t value =
            static_cast<uint16_t>(
                bytes_[offset_] |
                (static_cast<uint16_t>(bytes_[offset_ + 1]) << 8));
        offset_ += 2;
        return value;
    }
    uint32_t u32() {
        need(4);
        const uint32_t value =
            static_cast<uint32_t>(bytes_[offset_]) |
            (static_cast<uint32_t>(bytes_[offset_ + 1]) << 8) |
            (static_cast<uint32_t>(bytes_[offset_ + 2]) << 16) |
            (static_cast<uint32_t>(bytes_[offset_ + 3]) << 24);
        offset_ += 4;
        return value;
    }
    float f32() {
        const uint32_t bits = u32();
        float value = 0.0f;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }
    std::string string() {
        (void)u16();
        const uint32_t size = u32();
        need(size);
        std::string value(
            reinterpret_cast<const char*>(
                bytes_.data() + offset_),
            size);
        offset_ += size;
        return value;
    }
    void skip(size_t size) {
        need(size);
        offset_ += size;
    }
    size_t offset() const { return offset_; }
    size_t remaining() const {
        return bytes_.size() - offset_;
    }

private:
    void need(size_t size) const {
        if (size > bytes_.size() - offset_)
            throw std::runtime_error(
                "singer face: truncated FaceFX animation");
    }
    const std::vector<uint8_t>& bytes_;
    size_t offset_ = 0;
};

bool mouth_viseme(std::string_view name) {
    static const std::set<std::string_view> names = {
        "Eat", "Earth", "If", "Ox", "Oat", "Wet", "Size",
        "Church", "Fave", "Though", "Told", "Bump", "New",
        "Roar", "Cage"};
    return names.find(name) != names.end();
}

}  // namespace

FaceFxAnimation parse_gh2_facefx_animation(
    const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 64 ||
        std::memcmp(bytes.data(), "FACE", 4) != 0)
        throw std::runtime_error(
            "singer face: missing FaceFX FACE header");
    FaceReader reader(bytes);
    reader.skip(4);
    FaceFxAnimation result;
    result.version = reader.u32();
    if (result.version != 1200 && result.version != 1500)
        throw std::runtime_error(
            "singer face: unsupported FaceFX animation version");
    (void)reader.string();
    (void)reader.string();
    if (reader.u32() != 1000 || reader.u32() != 0 ||
        reader.u16() != 0)
        throw std::runtime_error(
            "singer face: FaceFX animation header differs");
    result.name = reader.string();
    const uint16_t sub_revision = reader.u16();
    if ((result.version == 1200 && sub_revision != 0) ||
        (result.version == 1500 && sub_revision != 3))
        throw std::runtime_error(
            "singer face: FaceFX animation subrevision differs");
    if (reader.u32() != bytes.size() ||
        reader.u16() != 0)
        throw std::runtime_error(
            "singer face: FaceFX animation size/header differs");
    const uint32_t curve_count = reader.u32();
    if (curve_count == 0 || curve_count > 256 ||
        reader.u32() != 0 || reader.u16() != 0)
        throw std::runtime_error(
            "singer face: invalid FaceFX curve header");
    result.curves.reserve(curve_count);
    for (uint32_t curve_index = 0;
         curve_index < curve_count; ++curve_index) {
        FaceFxAnimationCurve curve;
        curve.name = reader.string();
        if (reader.u32() != 0 || reader.u32() != 0)
            throw std::runtime_error(
                "singer face: FaceFX curve header differs");
        const uint32_t key_count = reader.u32();
        if (key_count > reader.remaining() / 18)
            throw std::runtime_error(
                "singer face: invalid FaceFX key count");
        curve.keys.reserve(key_count);
        for (uint32_t key_index = 0;
             key_index < key_count; ++key_index) {
            (void)reader.u16();
            FaceFxAnimationKey key;
            key.time = reader.f32();
            key.value = reader.f32();
            (void)reader.f32();
            (void)reader.u32();
            if (!std::isfinite(key.time) ||
                !std::isfinite(key.value))
                throw std::runtime_error(
                    "singer face: non-finite FaceFX key");
            curve.keys.push_back(key);
        }
        if (curve_index + 1 < curve_count) {
            if (reader.u32() != 0 || reader.u16() != 0)
                throw std::runtime_error(
                    "singer face: FaceFX curve trailer differs");
        }
        if (!std::is_sorted(
                curve.keys.begin(), curve.keys.end(),
                [](const FaceFxAnimationKey& left,
                   const FaceFxAnimationKey& right) {
                    return left.time < right.time;
                }))
            throw std::runtime_error(
                "singer face: unsorted FaceFX curve keys");
        result.curves.push_back(std::move(curve));
    }
    const size_t expected_footer =
        result.version == 1200 ? 14 : 36;
    if (reader.remaining() != expected_footer)
        throw std::runtime_error(
            "singer face: FaceFX animation footer size differs");
    const size_t footer_begin = reader.offset();
    result.archive_footer.assign(
        bytes.begin() + footer_begin, bytes.end());
    reader.skip(expected_footer);
    return result;
}

std::vector<SingerFaceTimeSpan>
derive_gh1_singer_open_spans(
    const FaceFxAnimation& animation) {
    float animation_end = 0.0f;
    for (const auto& curve : animation.curves)
        for (const auto& key : curve.keys)
            animation_end = std::max(animation_end, key.time);
    std::vector<SingerFaceTimeSpan> spans;
    for (const auto& curve : animation.curves) {
        if (!mouth_viseme(curve.name) || curve.keys.empty())
            continue;
        if (curve.keys.front().value > 0.0f)
            spans.push_back(
                {0.0f, curve.keys.front().time});
        for (size_t index = 1;
             index < curve.keys.size(); ++index) {
            const auto& left = curve.keys[index - 1];
            const auto& right = curve.keys[index];
            if (left.value > 0.0f || right.value > 0.0f)
                spans.push_back({left.time, right.time});
        }
        if (curve.keys.back().value > 0.0f)
            spans.push_back(
                {curve.keys.back().time, animation_end});
    }
    spans.erase(
        std::remove_if(
            spans.begin(), spans.end(),
            [](const SingerFaceTimeSpan& span) {
                return !std::isfinite(span.time_on) ||
                    !std::isfinite(span.time_off) ||
                    span.time_off <= span.time_on;
            }),
        spans.end());
    std::sort(
        spans.begin(), spans.end(),
        [](const SingerFaceTimeSpan& left,
           const SingerFaceTimeSpan& right) {
            if (left.time_on != right.time_on)
                return left.time_on < right.time_on;
            return left.time_off < right.time_off;
        });
    std::vector<SingerFaceTimeSpan> merged;
    for (const auto& span : spans) {
        if (merged.empty() ||
            span.time_on > merged.back().time_off + 0.000001f) {
            merged.push_back(span);
        } else {
            merged.back().time_off =
                std::max(merged.back().time_off, span.time_off);
        }
    }
    return merged;
}

std::vector<SingerFaceTickSpan>
extract_gh1_singer_face_spans(
    const std::vector<uint8_t>& midi) {
    const ParsedMidi parsed = parse_midi(midi);
    std::vector<size_t> selected;
    if (parsed.format == 0) {
        if (!parsed.tracks.empty()) selected.push_back(0);
    } else {
        for (size_t index = 0;
             index < parsed.tracks.size(); ++index)
            if (parsed.tracks[index].name == "PART GUITAR" ||
                parsed.tracks[index].name == "T1 GEMS")
                selected.push_back(index);
        if (selected.empty() && parsed.tracks.size() > 1)
            selected.push_back(1);
    }
    std::vector<SingerFaceTickSpan> spans;
    for (const size_t index : selected)
        for (const auto& note : parsed.tracks[index].notes)
            if (note.pitch == 108)
                spans.push_back(
                    {note.tick_on, note.tick_off});
    return merge_tick_spans(std::move(spans));
}

std::vector<SingerFaceTickSpan>
map_singer_face_times_to_midi(
    const std::vector<uint8_t>& midi,
    const std::vector<SingerFaceTimeSpan>& spans) {
    const ParsedMidi parsed = parse_midi(midi);
    std::vector<TempoEvent> tempos;
    for (const auto& track : parsed.tracks)
        tempos.insert(
            tempos.end(), track.tempos.begin(), track.tempos.end());
    std::stable_sort(
        tempos.begin(), tempos.end(),
        [](const TempoEvent& left, const TempoEvent& right) {
            if (left.tick != right.tick)
                return left.tick < right.tick;
            return left.order < right.order;
        });
    std::vector<TempoEvent> normalized;
    normalized.push_back({0, 500000, 0});
    for (const auto& tempo : tempos) {
        if (!normalized.empty() &&
            normalized.back().tick == tempo.tick)
            normalized.back() = tempo;
        else
            normalized.push_back(tempo);
    }
    struct TimedTempo {
        uint32_t tick = 0;
        uint32_t microseconds_per_quarter = 500000;
        double seconds = 0.0;
    };
    std::vector<TimedTempo> timed;
    timed.reserve(normalized.size());
    for (const auto& tempo : normalized) {
        double seconds = 0.0;
        if (!timed.empty()) {
            const auto& prior = timed.back();
            seconds = prior.seconds +
                static_cast<double>(tempo.tick - prior.tick) *
                prior.microseconds_per_quarter /
                (1000000.0 * parsed.division);
        }
        timed.push_back(
            {tempo.tick, tempo.microseconds_per_quarter, seconds});
    }
    const auto to_tick = [&](float source_seconds) {
        const double seconds =
            std::max(0.0, static_cast<double>(source_seconds));
        auto upper = std::upper_bound(
            timed.begin(), timed.end(), seconds,
            [](double value, const TimedTempo& tempo) {
                return value < tempo.seconds;
            });
        const TimedTempo& tempo =
            upper == timed.begin() ? timed.front() : *std::prev(upper);
        const double tick =
            tempo.tick +
            (seconds - tempo.seconds) * 1000000.0 *
                parsed.division /
                tempo.microseconds_per_quarter;
        if (!std::isfinite(tick) || tick < 0.0 ||
            tick > std::numeric_limits<uint32_t>::max())
            throw std::runtime_error(
                "singer face: FaceFX time exceeds MIDI range");
        return static_cast<uint32_t>(std::llround(tick));
    };
    std::vector<SingerFaceTickSpan> ticks;
    ticks.reserve(spans.size());
    for (const auto& span : spans) {
        uint32_t tick_on = to_tick(span.time_on);
        uint32_t tick_off = to_tick(span.time_off);
        if (tick_off == tick_on &&
            tick_off != std::numeric_limits<uint32_t>::max())
            ++tick_off;
        ticks.push_back({tick_on, tick_off});
    }
    return merge_tick_spans(std::move(ticks));
}

std::vector<uint8_t> append_gh1_singer_face_track(
    const std::vector<uint8_t>& midi,
    const std::vector<SingerFaceTickSpan>& source_spans) {
    const ParsedMidi parsed = parse_midi(midi);
    for (const auto& track : parsed.tracks)
        if (track.name == "GH1 SINGER FACE")
            throw std::runtime_error(
                "singer face: MIDI already has generated face track");
    if (parsed.declared_track_count ==
        std::numeric_limits<uint16_t>::max())
        throw std::runtime_error(
            "singer face: MIDI track count overflow");
    const auto spans = merge_tick_spans(source_spans);
    struct Cue {
        uint32_t tick = 0;
        uint8_t pitch = 0;
    };
    std::vector<Cue> cues;
    cues.reserve(spans.size() * 2);
    for (const auto& span : spans) {
        cues.push_back({span.tick_on, 108});
        cues.push_back({span.tick_off, 109});
    }
    std::stable_sort(
        cues.begin(), cues.end(),
        [](const Cue& left, const Cue& right) {
            if (left.tick != right.tick)
                return left.tick < right.tick;
            return left.pitch > right.pitch;
        });

    std::vector<uint8_t> track;
    constexpr std::string_view name = "GH1 SINGER FACE";
    append_vlq(track, 0);
    track.push_back(0xFF);
    track.push_back(0x03);
    append_vlq(track, static_cast<uint32_t>(name.size()));
    track.insert(track.end(), name.begin(), name.end());
    uint32_t prior_tick = 0;
    for (const auto& cue : cues) {
        append_vlq(track, cue.tick - prior_tick);
        track.push_back(0x90);
        track.push_back(cue.pitch);
        track.push_back(100);
        append_vlq(track, 0);
        track.push_back(0x80);
        track.push_back(cue.pitch);
        track.push_back(0);
        prior_tick = cue.tick;
    }
    append_vlq(track, 0);
    track.push_back(0xFF);
    track.push_back(0x2F);
    track.push_back(0);

    std::vector<uint8_t> output = midi;
    const uint16_t track_count =
        static_cast<uint16_t>(parsed.declared_track_count + 1);
    output[10] = static_cast<uint8_t>(track_count >> 8);
    output[11] = static_cast<uint8_t>(track_count);
    output.insert(output.end(), {'M', 'T', 'r', 'k'});
    if (track.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "singer face: generated MIDI track too large");
    append_be32(output, static_cast<uint32_t>(track.size()));
    output.insert(output.end(), track.begin(), track.end());
    return output;
}

}  // namespace gh::milo_convert
