#include "acp.h"

#include <cstdio>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3 ||
        (std::string(argv[1]) != "info" &&
         std::string(argv[1]) != "verify")) {
        std::fprintf(stderr,
                     "Usage: acp_tool <info|verify> <file.acp>\n");
        return 2;
    }
    try {
        const auto bytes = gh::acp::read_file(argv[2]);
        const auto file = gh::acp::parse(bytes);
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
