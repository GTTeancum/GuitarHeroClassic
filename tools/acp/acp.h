// GH1 standalone AnimClipSamples (.acp) reader/writer.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gh::acp {

struct ChannelSet {
    std::vector<std::string> channels;
    uint32_t sample_count = 0;
    uint32_t compression = 0;
    size_t frame_size = 0;
    std::vector<uint8_t> sample_bytes;
};

struct File {
    std::string class_name;
    std::string object_name;
    uint32_t revision = 0;
    float start_beat = 0.0f;
    float end_beat = 0.0f;
    float beats_per_second = 0.0f;
    uint32_t flags = 0;
    uint32_t play_flags = 0;
    float blend_width = 0.0f;
    // Nested SampleSet serialization revision. GH1 AnimClipSamples::Save
    // writes 5 here; Load stores it in the revision gate consulted by both
    // channel-set loaders.
    uint32_t sample_set_revision = 0;
    std::array<ChannelSet, 2> channel_sets;
    std::vector<uint8_t> trailing_bytes;
};

size_t channel_file_size(const std::string& channel, uint32_t compression);
File parse(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize(const File& file);
std::vector<uint8_t> read_file(const std::string& path);

}  // namespace gh::acp
