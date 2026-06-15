// bone_capture_hook.cpp
//
// Hook sub_82359400 (BinStream::Read) to log every read during a brief
// capture window, triggered by --bone_capture on the command line.
//
// Purpose: establish the ground-truth byte layout of CharClipSamples::Read()
// without needing to trace through the vtable dispatch manually. The log
// shows exactly what the engine reads, in what order, how many bytes each
// time — the canonical format spec.
//
// Usage:
//   gh2test.exe [normal args] --bone_capture --no_autoplay
//
// Output: writes bone_capture.txt with lines:
//   READ  <call_count>  nbytes=<n>  bytes=<hex>
//
// The first ~10 000 reads cover the full MILO load of the character animation
// file and give the complete CharClipSamples binary layout.

#include "harmonix_symbols.h"
#include "generated/gh2test_init.h"
#include <rex/hook.h>

#include <atomic>
#include <cstdio>
#include <cstring>

#if REX_PLATFORM_WIN32
#include <Windows.h>
#endif

// ─── command-line check ────────────────────────────────────────────────────
static bool cmdline_has(const char* flag) {
#if REX_PLATFORM_WIN32
    const char* cl = GetCommandLineA();
    return cl && std::strstr(cl, flag) != nullptr;
#else
    return false;
#endif
}

static const bool g_capture = cmdline_has("--bone_capture");

// ─── capture state ─────────────────────────────────────────────────────────
// We capture the first MAX_READS calls to BinStream::Read after startup.
// This covers the full MILO load before the game loop starts.
static constexpr int MAX_READS = 20000;
static std::atomic<int>  g_count{0};
static FILE*             g_out  = nullptr;

// ─── hook body ─────────────────────────────────────────────────────────────
// sub_82359400 (BinStream::Read)
//   r3 = BinStream* (stream object)
//   r4 = dest buffer (guest addr)
//   r5 = byte count
//
// After calling the original we read the bytes that were placed in the
// destination buffer and log them.

extern "C" void __imp__sub_82359400(PPCContext& ctx, uint8_t* base);

// Write marker to several candidate paths at static-init so we know the binary ran.
static FILE* open_capture_file() {
    const char* paths[] = {
        "C:\\Programming\\GitHub\\Guitar Hero II\\GuitarHeroOGX-trace360\\bone_capture.txt",
        ".\\bone_capture.txt",
        nullptr
    };
    for (int i = 0; paths[i]; ++i) {
        FILE* f = std::fopen(paths[i], "w");
        if (f) {
            std::fprintf(f, "# BinStream::Read capture  (max %d)\n", MAX_READS);
            std::fprintf(f, "# capture=%d\n", g_capture ? 1 : 0);
            std::fflush(f);
            return f;
        }
    }
    return nullptr;
}

REX_HOOK_RAW(hmx_BinStream_Read) {
    // Capture r4/r5 before __imp__ clobbers them.
    const uint32_t dest   = ctx.r4.u32;
    const uint32_t nbytes = ctx.r5.u32;

    // Always delegate to the real function.
    __imp__sub_82359400(ctx, base);

    if (!g_capture) return;
    int idx = g_count.fetch_add(1, std::memory_order_relaxed);
    if (idx >= MAX_READS) return;

    // Lazy-open the output file on the first captured read.
    if (idx == 0) g_out = open_capture_file();
    if (!g_out) return;

    // Log: read index, byte count, hex bytes.
    std::fprintf(g_out, "READ %6d  n=%3u  ", idx, nbytes);
    const uint8_t* src = base + dest;
    const uint32_t print_n = nbytes < 64 ? nbytes : 64;
    for (uint32_t i = 0; i < print_n; ++i)
        std::fprintf(g_out, "%02x ", src[i]);
    if (nbytes > 64) std::fprintf(g_out, "...");
    std::fputc('\n', g_out);
    std::fflush(g_out);
}
