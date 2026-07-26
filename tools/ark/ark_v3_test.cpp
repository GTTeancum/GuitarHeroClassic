#include "ark_v3.h"

#include <cstdio>
#include <exception>
#include <vector>

int main() {
    gh::ark::Index source;
    source.flag = 0x12345678u;
    source.ark_part_sizes = {100, 200};
    source.string_blob = {
        'z', '.', 'b', 'i', 'n', 0,
        'f', 'o', 'l', 'd', 'e', 'r', 0,
        'a', '.', 'b', 'i', 'n', 0};
    source.string_offsets = {0, 6, 13};
    source.entries = {
        {25, 0, 1, 10, 0},
        {125, 2, 0xffffffffu, 20, 40}};
    source.trailing_bytes = {0xaa, 0x55};

    try {
        const auto bytes = gh::ark::serialize_index(source);
        const auto parsed = gh::ark::parse_index(bytes);
        if (gh::ark::serialize_index(parsed) != bytes ||
            parsed.trailing_bytes != source.trailing_bytes ||
            parsed.entries.size() != 2) {
            std::fprintf(stderr, "ark_v3_test: lossless index mismatch\n");
            return 1;
        }

        const std::vector<gh::ark::LayoutEntry> layout = {
            {"folder/z.bin", 25, 10, 0},
            {"a.bin", 125, 20, 40}};
        const auto made = gh::ark::make_index({100, 200}, layout, 7);
        const auto made_bytes = gh::ark::serialize_index(made);
        const auto made_parsed = gh::ark::parse_index(made_bytes);
        if (made_parsed.flag != 7 || made_parsed.entries.size() != 2 ||
            gh::ark::serialize_index(made_parsed) != made_bytes) {
            std::fprintf(stderr,
                         "ark_v3_test: deterministic index mismatch\n");
            return 1;
        }

        auto malformed = bytes;
        // First string offset table value. Header is 12 + 8 part bytes,
        // followed by blob size/data and then string count.
        const size_t first_string_offset =
            12 + 8 + 4 + source.string_blob.size() + 4;
        malformed[first_string_offset + 0] = 0xff;
        malformed[first_string_offset + 1] = 0xff;
        malformed[first_string_offset + 2] = 0xff;
        malformed[first_string_offset + 3] = 0x7f;
        bool rejected = false;
        try {
            (void)gh::ark::parse_index(malformed);
        } catch (...) {
            rejected = true;
        }
        if (!rejected) {
            std::fprintf(stderr,
                         "ark_v3_test: malformed string offset accepted\n");
            return 1;
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "ark_v3_test: %s\n", ex.what());
        return 1;
    }
    std::printf("ark_v3_test: all checks passed\n");
    return 0;
}
