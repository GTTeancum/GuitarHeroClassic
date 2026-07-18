// ps2_texture_test.cpp - small regression tests for Harmonix PS2 texture decode.

#include "ps2_texture.h"

#include <cstdio>
#include <exception>

namespace {

int g_failures = 0;

void check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++g_failures;
    }
}

}  // namespace

int main() {
    try {
        gh::tex::HmxBitmap empty32{};
        empty32.magic = 0x01;
        empty32.bpp = 32;
        empty32.encoding = 3;
        auto empty = gh::tex::decode_to_rgba(empty32);
        check(empty.empty(), "0x0 32bpp direct-color texture decodes as empty");

        gh::tex::HmxBitmap one32{};
        one32.magic = 0x01;
        one32.bpp = 32;
        one32.encoding = 3;
        one32.width = 1;
        one32.height = 1;
        one32.raw = {10, 20, 30, 0x40};
        auto rgba32 = gh::tex::decode_to_rgba(one32);
        check(rgba32.size() == 4, "1x1 32bpp direct-color size");
        check(rgba32[0] == 10 && rgba32[1] == 20 && rgba32[2] == 30 &&
                  rgba32[3] == 0x80,
              "1x1 32bpp direct-color channel order and PS2 alpha scale");

        gh::tex::HmxBitmap one24{};
        one24.magic = 0x01;
        one24.bpp = 24;
        one24.encoding = 3;
        one24.width = 1;
        one24.height = 1;
        one24.raw = {1, 2, 3};
        auto rgba24 = gh::tex::decode_to_rgba(one24);
        check(rgba24.size() == 4, "1x1 24bpp direct-color size");
        check(rgba24[0] == 1 && rgba24[1] == 2 && rgba24[2] == 3 &&
                  rgba24[3] == 0xFF,
              "1x1 24bpp direct-color channel order and opaque alpha");
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "unexpected exception: %s\n", ex.what());
        return 1;
    }

    if (g_failures) {
        std::fprintf(stderr, "ps2_texture_test: %d failure(s)\n", g_failures);
        return 1;
    }
    std::printf("ps2_texture_test: all checks passed\n");
    return 0;
}
