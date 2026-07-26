#include "milo.h"

#include <exception>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

void append_u32(std::vector<uint8_t>& bytes, uint32_t value) {
    bytes.push_back(static_cast<uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xffu));
    bytes.push_back(static_cast<uint8_t>((value >> 16) & 0xffu));
    bytes.push_back(static_cast<uint8_t>((value >> 24) & 0xffu));
}

void append_string(std::vector<uint8_t>& bytes, const char* value) {
    const size_t len = std::strlen(value);
    append_u32(bytes, static_cast<uint32_t>(len));
    bytes.insert(bytes.end(), value, value + len);
}

}  // namespace

int main() {
    std::vector<uint8_t> bytes;
    append_u32(bytes, static_cast<uint32_t>(gh::milo::BlockStructure::MILO_B));
    append_u32(bytes, 20);  // First block follows the 16-byte header + one size.
    append_u32(bytes, 1);
    append_u32(bytes, 0);
    append_u32(bytes, 2);
    bytes.push_back(0x03);
    bytes.push_back(0x00);

    try {
        const auto header = gh::milo::parse_header(bytes);
        const auto payload = gh::milo::inflate_payload(bytes, header);
        if (!payload.empty()) {
            std::fprintf(stderr,
                         "milo_test: empty raw deflate block produced %zu bytes\n",
                         payload.size());
            return 1;
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr,
                     "milo_test: empty raw deflate block failed: %s\n",
                     ex.what());
        return 1;
    }

    // Revision-10 directories have no root-directory body. Verify that the
    // first table entry receives the first body rather than being discarded
    // at its DEADDEAD terminator and shifting every subsequent association.
    std::vector<uint8_t> gh1;
    append_u32(gh1, 10);
    append_u32(gh1, 2);
    append_string(gh1, "Tex"); append_string(gh1, "first.tex");
    append_string(gh1, "Mat"); append_string(gh1, "first.mat");
    append_u32(gh1, 1);         // external resource count
    append_string(gh1, "external.milo_ps2");
    const size_t first_body = gh1.size();
    append_u32(gh1, 8); append_u32(gh1, 0x11111111);
    append_u32(gh1, 0xDEADDEAD);
    const size_t second_body = gh1.size();
    append_u32(gh1, 21); append_u32(gh1, 0x22222222);
    append_u32(gh1, 0xDEADDEAD);
    const auto gh1_dir = gh::milo::parse_directory(gh1);
    if (gh1_dir.entries.size() != 2 ||
        gh1_dir.dir_entry_size != 0 ||
        gh1_dir.entries[0].offset != first_body ||
        gh1_dir.entries[0].size != 8 ||
        gh1_dir.entries[1].offset != second_body ||
        gh1_dir.entries[1].size != 8) {
        std::fprintf(stderr, "milo_test: GH1 child-body alignment failed\n");
        return 1;
    }

    std::printf("milo_test: all checks passed\n");
    return 0;
}
