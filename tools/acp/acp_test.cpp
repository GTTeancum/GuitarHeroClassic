#include "acp.h"

#include <cstdio>
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
    file.field_28 = 5;
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
        if (round_trip != bytes || parsed.field_28 != 5 ||
            parsed.channel_sets[0].frame_size != 20 ||
            !parsed.trailing_bytes.empty()) {
            std::fprintf(stderr, "acp_test: round trip mismatch\n");
            return 1;
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "acp_test: %s\n", ex.what());
        return 1;
    }
    std::printf("acp_test: all checks passed\n");
    return 0;
}
