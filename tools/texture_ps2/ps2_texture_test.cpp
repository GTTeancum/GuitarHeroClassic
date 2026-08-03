// ps2_texture_test.cpp - small regression tests for Harmonix PS2 texture decode.

#include "ps2_texture.h"

#include <cstdio>
#include <exception>
#include <vector>

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

        gh::tex::HmxBitmap black24 = one24;
        black24.raw = {0, 0, 0};
        auto rgba_black24 = gh::tex::decode_to_rgba(black24);
        check(gh::tex::uses_ps2_transparent_black(black24),
              "RGB24 selects the PS2 transparent-black expansion path");
        check(rgba_black24[3] == 0,
              "RGB24 exact black follows TEXA/AEM transparent expansion");

        gh::tex::HmxBitmap opaque8{};
        opaque8.magic = 0x01;
        opaque8.bpp = 8;
        opaque8.encoding = 3;
        opaque8.width = 2;
        opaque8.height = 1;
        opaque8.raw.resize(256 * 4 + 2, 0);
        // Palette entries 0 and 1 are exact black and red. All palette alpha
        // is opaque, matching the cached Theatre crowd source.
        for (size_t i = 0; i < 256; ++i)
            opaque8.raw[i * 4 + 3] = 0x80;
        opaque8.raw[1 * 4 + 0] = 0x7f;
        opaque8.raw[256 * 4 + 0] = 0;
        opaque8.raw[256 * 4 + 1] = 1;
        auto rgba_opaque8 = gh::tex::decode_to_rgba(opaque8);
        check(!gh::tex::uses_ps2_transparent_black(opaque8),
              "indexed bitmap retains CLUT alpha even when fully opaque");
        check(rgba_opaque8[3] == 0xff && rgba_opaque8[7] == 0xff,
              "indexed decode does not infer transparent black");
        check(gh::tex::apply_transparent_black_alpha(rgba_opaque8) == 1,
              "explicit transparent-black operation changes exact black");
        check(rgba_opaque8[3] == 0 && rgba_opaque8[7] == 0xff,
              "explicit transparent-black operation preserves nonblack");

        gh::tex::HmxBitmap authored8 = opaque8;
        // A used translucent palette entry proves this is an alpha-bearing
        // bitmap; its opaque black must remain opaque.
        authored8.raw[1 * 4 + 3] = 0x40;
        auto rgba_authored8 = gh::tex::decode_to_rgba(authored8);
        check(!gh::tex::uses_ps2_transparent_black(authored8),
              "authored indexed alpha stays on CLUT-alpha path");
        check(rgba_authored8[3] == 0xff && rgba_authored8[7] == 0x80,
              "authored alpha preserves opaque black and translucent color");
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
