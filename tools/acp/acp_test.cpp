#include "acp.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

int main() {
    gh::acp::File file;
    file.class_name = "AnimClipSamples";
    file.object_name = "format_test";
    file.revision = 18;
    file.start_beat = -1.0f;
    file.end_beat = 2.0f;
    file.beats_per_second = 1.5f;
    file.flags = 0x80000000u;
    file.play_flags = 2;
    file.blend_width = 0.25f;
    file.sample_set_revision = 5;
    file.channel_sets[0].channels = {"bone_test.pos", "bone_test.quat"};
    file.channel_sets[0].sample_count = 2;
    file.channel_sets[0].compression = 1;
    file.channel_sets[0].sample_bytes.resize((12 + 8) * 2);
    for (size_t i = 0; i < file.channel_sets[0].sample_bytes.size(); ++i)
        file.channel_sets[0].sample_bytes[i] =
            static_cast<uint8_t>(i * 13u);

    try {
        const auto bytes = gh::acp::serialize(file);
        const auto parsed = gh::acp::parse(bytes);
        const auto round_trip = gh::acp::serialize(parsed);
        if (round_trip != bytes || parsed.sample_set_revision != 5 ||
            parsed.channel_sets[0].frame_size != 20 ||
            !parsed.trailing_bytes.empty()) {
            std::fprintf(stderr, "acp_test: round trip mismatch\n");
            return 1;
        }

        gh::acp::ChannelSet samples;
        samples.channels = {
            "bone_test.pos", "bone_test.quat", "bone_test.rotx"};
        samples.sample_count = 1;
        samples.compression = 1;
        samples.frame_size = 22;
        samples.sample_bytes.resize(samples.frame_size);
        auto put_f32 = [&](size_t offset, float value) {
            std::memcpy(samples.sample_bytes.data() + offset, &value,
                        sizeof(value));
        };
        auto put_i16 = [&](size_t offset, int16_t value) {
            std::memcpy(samples.sample_bytes.data() + offset, &value,
                        sizeof(value));
        };
        put_f32(0, 1.0f);
        put_f32(4, 2.0f);
        put_f32(8, 3.0f);
        put_i16(12, 16384);
        put_i16(14, -16384);
        put_i16(16, 32767);
        put_i16(18, -32768);
        put_i16(20, 1024);
        const auto pos =
            gh::acp::decode_channel_sample(samples, 0, 99);
        const auto quat =
            gh::acp::decode_channel_sample(samples, 1, 99);
        const auto angle =
            gh::acp::decode_channel_sample(samples, 2, 99);
        if (pos.component_count != 3 || pos.values[0] != 1.0f ||
            pos.values[1] != 2.0f || pos.values[2] != 3.0f ||
            quat.component_count != 4 ||
            std::fabs(quat.values[0] - 16384.0f / 32767.0f) > 1e-6f ||
            std::fabs(quat.values[1] + 16384.0f / 32767.0f) > 1e-6f ||
            quat.values[2] != 1.0f || quat.values[3] != -1.0f ||
            angle.component_count != 1 ||
            std::fabs(angle.values[0] - 0.625f) > 1e-6f) {
            std::fprintf(stderr, "acp_test: sample decode mismatch\n");
            return 1;
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "acp_test: %s\n", ex.what());
        return 1;
    }
    std::printf("acp_test: all checks passed\n");
    return 0;
}
