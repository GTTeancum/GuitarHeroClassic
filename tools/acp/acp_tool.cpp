#include "acp.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

bool ends_with(const std::string& value, const char* suffix) {
    const size_t size = std::strlen(suffix);
    return value.size() >= size &&
           value.compare(value.size() - size, size, suffix) == 0;
}

float read_f32(const std::vector<uint8_t>& bytes, size_t offset) {
    if (offset + sizeof(float) > bytes.size())
        throw std::runtime_error("ACP sample value exceeds data block");
    float value = 0.0f;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

void print_positions(const gh::acp::File& file) {
    for (size_t set_index = 0;
         set_index < file.channel_sets.size(); ++set_index) {
        const auto& set = file.channel_sets[set_index];
        size_t channel_offset = 0;
        for (const auto& channel : set.channels) {
            const size_t channel_size =
                gh::acp::channel_file_size(channel, set.compression);
            if (ends_with(channel, ".pos") && set.sample_count != 0) {
                std::array<float, 3> first{};
                std::array<float, 3> last{};
                std::array<float, 3> minimum{
                    std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity()};
                std::array<float, 3> maximum{
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity()};
                for (uint32_t sample = 0;
                     sample < set.sample_count; ++sample) {
                    const size_t base =
                        static_cast<size_t>(sample) * set.frame_size +
                        channel_offset;
                    std::array<float, 3> value{};
                    for (size_t axis = 0; axis < value.size(); ++axis) {
                        value[axis] =
                            read_f32(set.sample_bytes,
                                     base + axis * sizeof(float));
                        minimum[axis] =
                            std::min(minimum[axis], value[axis]);
                        maximum[axis] =
                            std::max(maximum[axis], value[axis]);
                    }
                    if (sample == 0) first = value;
                    if (sample + 1 == set.sample_count) last = value;
                }
                std::printf(
                    "set%zu %s samples=%u "
                    "first=%g,%g,%g last=%g,%g,%g "
                    "range=%g,%g,%g\n",
                    set_index, channel.c_str(), set.sample_count,
                    first[0], first[1], first[2],
                    last[0], last[1], last[2],
                    maximum[0] - minimum[0],
                    maximum[1] - minimum[1],
                    maximum[2] - minimum[2]);
            }
            channel_offset += channel_size;
        }
    }
}

void print_sample(const gh::acp::File& file, uint32_t sample_index,
                  const std::string& filter) {
    for (size_t set_index = 0;
         set_index < file.channel_sets.size(); ++set_index) {
        const auto& set = file.channel_sets[set_index];
        if (set.sample_count == 0) continue;
        for (size_t channel_index = 0;
             channel_index < set.channels.size(); ++channel_index) {
            const std::string& channel = set.channels[channel_index];
            if (!filter.empty() &&
                channel.find(filter) == std::string::npos) {
                continue;
            }
            const auto decoded = gh::acp::decode_channel_sample(
                set, channel_index, sample_index);
            std::printf("set%zu sample=%u source_sample=%u %s",
                        set_index, sample_index,
                        set.sample_count == 1 ? 0u : sample_index,
                        channel.c_str());
            for (size_t component = 0;
                 component < decoded.component_count; ++component) {
                std::printf("%s%.9g",
                            component == 0 ? " " : ",",
                            decoded.values[component]);
            }
            std::printf("\n");
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::string command = argc > 1 ? argv[1] : "";
    const bool sample_command =
        argc >= 4 && argc <= 5 && command == "sample";
    const bool basic_command =
        argc == 3 &&
        (command == "info" || command == "channels" ||
         command == "positions" || command == "verify");
    if (!sample_command && !basic_command) {
        std::fprintf(stderr,
                     "Usage: acp_tool "
                     "<info|channels|positions|verify> <file.acp>\n"
                     "       acp_tool sample <file.acp> <index> [filter]\n");
        return 2;
    }
    try {
        const auto bytes = gh::acp::read_file(argv[2]);
        const auto file = gh::acp::parse(bytes);
        if (std::string(argv[1]) == "channels") {
            for (size_t set = 0; set < file.channel_sets.size(); ++set) {
                std::printf("set%zu samples=%u compression=%u channels=%zu\n",
                            set, file.channel_sets[set].sample_count,
                            file.channel_sets[set].compression,
                            file.channel_sets[set].channels.size());
                for (const auto& channel : file.channel_sets[set].channels)
                    std::printf("  %s\n", channel.c_str());
            }
            return 0;
        }
        if (std::string(argv[1]) == "positions") {
            print_positions(file);
            return 0;
        }
        if (sample_command) {
            const unsigned long parsed =
                std::strtoul(argv[3], nullptr, 0);
            if (parsed > std::numeric_limits<uint32_t>::max())
                throw std::runtime_error("sample index exceeds uint32");
            print_sample(file, static_cast<uint32_t>(parsed),
                         argc == 5 ? argv[4] : "");
            return 0;
        }
        if (std::string(argv[1]) == "verify") {
            const auto serialized = gh::acp::serialize(file);
            if (serialized != bytes) {
                std::fprintf(stderr,
                             "ACP round trip differs: source=%zu output=%zu\n",
                             bytes.size(), serialized.size());
                return 1;
            }
        }
        std::printf("class=%s name=%s revision=%u start=%g end=%g bps=%g "
                    "flags=0x%08x play=0x%08x blend=%g sample_set_rev=%u "
                    "set0=%zu/%u/%u set1=%zu/%u/%u trailing=%zu\n",
                    file.class_name.c_str(), file.object_name.c_str(),
                    file.revision, file.start_beat, file.end_beat,
                    file.beats_per_second, file.flags, file.play_flags,
                    file.blend_width, file.sample_set_revision,
                    file.channel_sets[0].channels.size(),
                    file.channel_sets[0].sample_count,
                    file.channel_sets[0].compression,
                    file.channel_sets[1].channels.size(),
                    file.channel_sets[1].sample_count,
                    file.channel_sets[1].compression,
                    file.trailing_bytes.size());
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "acp_tool: %s\n", ex.what());
        return 1;
    }
}
