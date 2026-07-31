#include "acp.h"

#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace gh::acp {
namespace {

class Cursor {
public:
    explicit Cursor(const std::vector<uint8_t>& bytes) : bytes_(bytes) {}

    uint32_t u32() {
        need(4);
        uint32_t value = 0;
        std::memcpy(&value, bytes_.data() + pos_, 4);
        pos_ += 4;
        return value;
    }
    float f32() {
        const uint32_t bits = u32();
        float value = 0.0f;
        std::memcpy(&value, &bits, 4);
        return value;
    }
    std::string string() {
        const uint32_t length = u32();
        need(length);
        std::string value(
            reinterpret_cast<const char*>(bytes_.data() + pos_), length);
        pos_ += length;
        return value;
    }
    std::vector<uint8_t> bytes(size_t count) {
        need(count);
        std::vector<uint8_t> value(bytes_.begin() + pos_,
                                   bytes_.begin() + pos_ + count);
        pos_ += count;
        return value;
    }
    size_t pos() const { return pos_; }
    size_t remaining() const { return bytes_.size() - pos_; }

private:
    void need(size_t count) const {
        if (count > bytes_.size() - pos_)
            throw std::runtime_error("ACP: read past end");
    }
    const std::vector<uint8_t>& bytes_;
    size_t pos_ = 0;
};

void append_u32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 24));
}

void append_f32(std::vector<uint8_t>& out, float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, 4);
    append_u32(out, bits);
}

void append_string(std::vector<uint8_t>& out, const std::string& value) {
    if (value.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("ACP: string too large");
    append_u32(out, static_cast<uint32_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

bool ends_with(const std::string& value, const char* suffix) {
    const size_t length = std::strlen(suffix);
    return value.size() >= length &&
           value.compare(value.size() - length, length, suffix) == 0;
}

}  // namespace

size_t channel_file_size(const std::string& channel, uint32_t compression) {
    if (compression > 4)
        throw std::runtime_error("ACP: compression mode exceeds 4");
    if (ends_with(channel, ".pos") || ends_with(channel, ".scale"))
        return 12;
    if (ends_with(channel, ".quat"))
        return compression == 0 ? 16 : 8;
    if (ends_with(channel, ".rotx") || ends_with(channel, ".roty") ||
        ends_with(channel, ".rotz") || ends_with(channel, ".drotx") ||
        ends_with(channel, ".droty") || ends_with(channel, ".drotz"))
        return compression == 0 ? 4 : 2;
    throw std::runtime_error("ACP: unknown channel type: " + channel);
}

DecodedChannelSample decode_channel_sample(
    const ChannelSet& set, size_t channel_index, uint32_t sample_index) {
    if (channel_index >= set.channels.size())
        throw std::runtime_error("ACP: channel index out of range");
    if (set.sample_count == 0)
        throw std::runtime_error("ACP: channel set has no samples");
    const uint32_t resolved_sample =
        set.sample_count == 1 ? 0 : sample_index;
    if (resolved_sample >= set.sample_count)
        throw std::runtime_error("ACP: sample index out of range");

    size_t channel_offset = 0;
    for (size_t index = 0; index < channel_index; ++index) {
        channel_offset +=
            channel_file_size(set.channels[index], set.compression);
    }
    const std::string& channel = set.channels[channel_index];
    const size_t channel_size =
        channel_file_size(channel, set.compression);
    const size_t base =
        static_cast<size_t>(resolved_sample) * set.frame_size +
        channel_offset;
    if (base > set.sample_bytes.size() ||
        channel_size > set.sample_bytes.size() - base) {
        throw std::runtime_error("ACP: sample bytes exceed channel set");
    }

    auto read_i16 = [&](size_t offset) {
        int16_t value = 0;
        std::memcpy(&value, set.sample_bytes.data() + base + offset,
                    sizeof(value));
        return value;
    };
    auto read_f32_at = [&](size_t offset) {
        float value = 0.0f;
        std::memcpy(&value, set.sample_bytes.data() + base + offset,
                    sizeof(value));
        return value;
    };

    DecodedChannelSample decoded;
    if (ends_with(channel, ".pos") || ends_with(channel, ".scale")) {
        decoded.component_count = 3;
        for (size_t axis = 0; axis < 3; ++axis)
            decoded.values[axis] = read_f32_at(axis * sizeof(float));
        return decoded;
    }
    if (ends_with(channel, ".quat")) {
        decoded.component_count = 4;
        if (set.compression == 0) {
            for (size_t axis = 0; axis < 4; ++axis)
                decoded.values[axis] =
                    read_f32_at(axis * sizeof(float));
        } else {
            for (size_t axis = 0; axis < 4; ++axis) {
                decoded.values[axis] =
                    std::max(static_cast<float>(
                                 read_i16(axis * sizeof(int16_t))) /
                                 32767.0f,
                             -1.0f);
            }
        }
        return decoded;
    }

    decoded.component_count = 1;
    decoded.values[0] =
        set.compression == 0
            ? read_f32_at(0)
            : static_cast<float>(read_i16(0)) * 0.0006103515625f;
    return decoded;
}

File parse(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    File file;
    file.class_name = cursor.string();
    file.object_name = cursor.string();
    file.revision = cursor.u32();
    if (file.class_name != "AnimClipSamples")
        throw std::runtime_error("ACP: class is not AnimClipSamples");
    if (file.revision != 18)
        throw std::runtime_error("ACP: unsupported body revision");
    file.start_beat = cursor.f32();
    file.end_beat = cursor.f32();
    file.beats_per_second = cursor.f32();
    file.flags = cursor.u32();
    file.play_flags = cursor.u32();
    file.blend_width = cursor.f32();
    file.sample_set_revision = cursor.u32();
    if (file.sample_set_revision != 5)
        throw std::runtime_error(
            "ACP: unsupported SampleSet serialization revision");

    for (auto& set : file.channel_sets) {
        const uint32_t count = cursor.u32();
        if (count > 10000)
            throw std::runtime_error("ACP: implausible channel count");
        set.channels.reserve(count);
        for (uint32_t i = 0; i < count; ++i)
            set.channels.push_back(cursor.string());
        set.sample_count = cursor.u32();
        set.compression = cursor.u32();
        if (set.compression > 4)
            throw std::runtime_error("ACP: unsupported compression");
        if (!set.channels.empty() && set.sample_count == 0)
            throw std::runtime_error(
                "ACP: populated channel set has zero samples");
        for (const auto& channel : set.channels)
            set.frame_size += channel_file_size(channel, set.compression);
    }

    for (auto& set : file.channel_sets) {
        const uint64_t byte_count =
            static_cast<uint64_t>(set.frame_size) * set.sample_count;
        if (byte_count > std::numeric_limits<size_t>::max())
            throw std::runtime_error("ACP: sample block too large");
        set.sample_bytes = cursor.bytes(static_cast<size_t>(byte_count));
    }
    file.trailing_bytes = cursor.bytes(cursor.remaining());
    return file;
}

std::vector<uint8_t> serialize(const File& file) {
    if (file.sample_set_revision != 5)
        throw std::runtime_error(
            "ACP: unsupported SampleSet serialization revision");
    std::vector<uint8_t> out;
    append_string(out, file.class_name);
    append_string(out, file.object_name);
    append_u32(out, file.revision);
    append_f32(out, file.start_beat);
    append_f32(out, file.end_beat);
    append_f32(out, file.beats_per_second);
    append_u32(out, file.flags);
    append_u32(out, file.play_flags);
    append_f32(out, file.blend_width);
    append_u32(out, file.sample_set_revision);
    for (const auto& set : file.channel_sets) {
        if (set.channels.size() >
            std::numeric_limits<uint32_t>::max())
            throw std::runtime_error("ACP: too many channels");
        size_t frame_size = 0;
        for (const auto& channel : set.channels)
            frame_size += channel_file_size(channel, set.compression);
        const uint64_t expected =
            static_cast<uint64_t>(frame_size) * set.sample_count;
        if (expected != set.sample_bytes.size())
            throw std::runtime_error(
                "ACP: sample byte count does not match header");
        append_u32(out, static_cast<uint32_t>(set.channels.size()));
        for (const auto& channel : set.channels)
            append_string(out, channel);
        append_u32(out, set.sample_count);
        append_u32(out, set.compression);
    }
    for (const auto& set : file.channel_sets)
        out.insert(out.end(), set.sample_bytes.begin(),
                   set.sample_bytes.end());
    out.insert(out.end(), file.trailing_bytes.begin(),
               file.trailing_bytes.end());
    return out;
}

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("cannot open " + path);
    const auto size = static_cast<size_t>(file.tellg());
    std::vector<uint8_t> bytes(size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    if (!file) throw std::runtime_error("short read on " + path);
    return bytes;
}

}  // namespace gh::acp
