#include "milo.h"

#include <exception>
#include <cstdio>
#include <vector>

namespace {

void append_u32(std::vector<uint8_t>& bytes, uint32_t value) {
    bytes.push_back(static_cast<uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xffu));
    bytes.push_back(static_cast<uint8_t>((value >> 16) & 0xffu));
    bytes.push_back(static_cast<uint8_t>((value >> 24) & 0xffu));
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

    std::printf("milo_test: all checks passed\n");
    return 0;
}
