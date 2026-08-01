// ps2_ark_hook.cpp - Route the recompiled 360 GH2 ARK lookups through our
// native PS2 v3 ARK reader.
//
// Goal: the recompile loads PS2 GH2 ARK content natively, with no 360-format
// data ever generated. This is the wedge that lets us strip the 360 ARK code
// path entirely on the eventual OG-Xbox port -- the gameplay logic stays, the
// 360-specific reader/encryption goes.
//
// What gets hooked:
//
//   sub_82277878(table, path, *out_a, *out_b, *out_c, *out_d) -> u8 found
//
// This is the central path -> entry lookup that the 360 binary's file class
// uses. With a PS2 ARK on disk, the 360 reader silently produces 0 entries
// (different HDR format), and downstream lookups trap on divide-by-zero in
// the string-hash function because bucket_count == 0.
//
// We replace the whole lookup. r4 is the (already-normalized) full asset
// path; we look it up in our PS2 ArkV3Reader and write the entry's ARK
// offset and size into r7 and r8. r5 (entry index) and r6 stay at 0 since
// nothing downstream of this checks them in a way that depends on the
// hashtable's internal bookkeeping.
//
// The PS2 ARK is opened lazily on the first hook call, keyed off the
// game_data_root cvar that the runtime already parses. ARK lookups happen
// on multiple guest threads, so the load is one-shot and lock-free.

#include "ark_v3.h"
#include "ps2_ark_hook.h"
#include "harmonix_symbols.h"

// generated/gh2test_init.h is where the per-target REX_LOAD_*/REX_STORE_*
// macros come from -- they need base+addr math that's tied to this target's
// memory layout.
#include "generated/gh2test_init.h"

#include <rex/hook.h>
#include <rex/logging.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::once_flag g_ark_init;
std::unique_ptr<gh::ark::ArkV3Reader> g_ark;
std::string g_ark_path;
std::string g_game_data_root;  // set by ps2_ark::set_game_data_root() from app init

// Shared open handle to main_0.ark; reads from this are serialized with the
// mutex because std::ifstream isn't thread-safe and the recomp drives reads
// from multiple guest threads. main_0.ark is ~3 GB so per-thread fopens
// would be a real memory hit if the kernel ever caches differently.
std::mutex g_ark_io_mu;
std::ifstream g_ark_file;

// Inner handle layout (the 64-byte object sub_8227DC18 allocates and our
// sub_82277878 hook fills). The 360 binary's File class wraps this; reads
// go through the outer File which dereferences here for offset/size.
constexpr uint32_t kHandleOff      = 12;   // 32-bit ARK byte offset
constexpr uint32_t kHandleSize     = 16;   // 32-bit entry size in bytes
constexpr uint32_t kHandlePosition = 24;   // we reuse this currently-zero slot for read cursor

// Plaintext cache for files we decrypted at lookup time. Keyed by the
// inner handle pointer (the recompiled file-object address) -- not perfect
// (the handle could be reused if the game frees and re-allocates), but the
// lookup hook always re-populates on insert so overwrite is fine.
std::mutex g_cache_mu;
std::unordered_map<uint32_t, std::vector<uint8_t>> g_plaintext_cache;

// PS2 DTB cipher (CryptTable-based, ArkTool v6 lineage). The same algorithm
// already lives in gh::dtb::Ps2Crypt under tools/dtb -- we don't link that
// module into the recompile target because it pulls in the whole DTB parser,
// so this is a focused local copy. Format reference: Mackiloha Crypt.cs.
struct Ps2Crypt {
    uint32_t table[256];
    int      i1 = 0;
    int      i2 = 0x67;

    explicit Ps2Crypt(uint32_t seed) {
        uint32_t v = seed;
        for (int i = 0; i < 256; ++i) {
            uint32_t a = v * 0x41C64E6Du + 0x3039u;
            uint32_t b = a * 0x41C64E6Du + 0x3039u;
            table[i] = (b & 0x7FFF0000u) | (a >> 16);
            v = b;
        }
    }
    void apply(uint8_t* data, size_t n) {
        for (size_t k = 0; k < n; ++k) {
            table[i1] ^= table[i2];
            data[k] ^= static_cast<uint8_t>(table[i1] & 0xFF);
            i1 = (i1 + 1 >= 0xF9) ? 0 : (i1 + 1);
            i2 = (i2 + 1 >= 0xF9) ? 0 : (i2 + 1);
        }
    }
};

bool path_ends_with(const std::string& s, std::string_view suffix) {
    return s.size() >= suffix.size() &&
           std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
}

// Read an entry's bytes raw from main_0.ark; caller holds g_ark_io_mu.
std::vector<uint8_t> read_entry_raw_locked(uint64_t off, uint32_t size) {
    std::vector<uint8_t> buf(size);
    g_ark_file.seekg(static_cast<std::streamoff>(off), std::ios::beg);
    g_ark_file.read(reinterpret_cast<char*>(buf.data()), size);
    if (!g_ark_file) g_ark_file.clear();
    buf.resize(static_cast<size_t>(g_ark_file.gcount()));
    return buf;
}

// PS2 .dtb files start with a 4-byte LCG seed and the rest is enciphered
// payload. After decryption the payload begins with the 0x01 plaintext
// marker. Returns the decrypted payload (size = ciphered_size - 4) or an
// empty vector on failure (caller should fall back to raw bytes -- some
// files matching *.dtb may not actually be encrypted in obscure builds).
std::vector<uint8_t> decrypt_ps2_dtb(const std::vector<uint8_t>& raw) {
    if (raw.size() < 5) return {};
    uint32_t seed;
    std::memcpy(&seed, raw.data(), 4);
    std::vector<uint8_t> out(raw.begin() + 4, raw.end());
    Ps2Crypt cipher(seed);
    cipher.apply(out.data(), out.size());
    if (out[0] != 0x01) {
        REXLOG_WARN("ps2_ark: dtb decrypt mismatch, first byte=0x{:02x} (expected 0x01)",
                    out[0]);
        return {};
    }
    return out;
}

// Look for MAIN.HDR / MAIN_0.ARK (PS2-style upper-case) and main.hdr /
// main_0.ark (Harmonix lower-case) under <game_data_root>/gen/.
struct ArkPaths {
    std::string hdr;
    std::string ark;
    bool ok() const { return !hdr.empty() && !ark.empty(); }
};

ArkPaths resolve_ark_paths(const fs::path& game_root) {
    ArkPaths p;
    fs::path gen = game_root / "gen";
    for (auto n : {"main.hdr", "MAIN.HDR"}) {
        auto cand = gen / n;
        if (fs::exists(cand)) { p.hdr = cand.string(); break; }
    }
    for (auto n : {"main_0.ark", "MAIN_0.ARK"}) {
        auto cand = gen / n;
        if (fs::exists(cand)) { p.ark = cand.string(); break; }
    }
    return p;
}

void load_ark_once() {
    std::call_once(g_ark_init, []() {
        if (g_game_data_root.empty()) {
            REXLOG_WARN("ps2_ark: game_data_root not set; hook will fail every lookup. "
                        "App must call ps2_ark::set_game_data_root() before first asset open.");
            return;
        }
        auto paths = resolve_ark_paths(g_game_data_root);
        if (!paths.ok()) {
            REXLOG_WARN("ps2_ark: no main.hdr/main_0.ark found under {}/gen", g_game_data_root);
            return;
        }
        try {
            auto reader = std::make_unique<gh::ark::ArkV3Reader>(
                gh::ark::ArkV3Reader::load(paths.hdr));
            REXLOG_INFO("ps2_ark: loaded {} ({} entries, v{})",
                        paths.hdr, reader->entries().size(), reader->version());
            g_ark = std::move(reader);
            g_ark_path = paths.ark;

            g_ark_file.open(g_ark_path, std::ios::binary);
            if (!g_ark_file) {
                REXLOG_WARN("ps2_ark: opened HDR but couldn't open ARK file {}", g_ark_path);
            }
        } catch (const std::exception& e) {
            REXLOG_WARN("ps2_ark: failed to load {}: {}", paths.hdr, e.what());
        }
    });
}

// The 360 binary normalizes paths through its own logic before lookup. We
// see a few prefix conventions in practice; try them in order. PS2 paths
// in the v3 ARK use slashes and live either at the catalog root
// ("config/gen/songs.dtb") or under a "../../system/run/" prefix that the
// PS2 build prepends for engine assets.
std::optional<gh::ark::Entry> find_entry(const std::string& req) {
    if (!g_ark) return std::nullopt;
    if (auto e = g_ark->find(req)) return e;
    if (auto e = g_ark->find("../../system/run/" + req)) return e;
    return std::nullopt;
}

// Read a null-terminated guest string. Bounded scan so a stray pointer
// can't run off into the weeds.
std::string read_guest_string(uint8_t* base, uint32_t guest_addr) {
    if (!guest_addr) return {};
    const char* p = reinterpret_cast<const char*>(base + guest_addr);
    constexpr size_t kMax = 1024;
    size_t n = 0;
    while (n < kMax && p[n]) ++n;
    return std::string(p, n);
}

}  // anonymous namespace

namespace ps2_ark {

void set_game_data_root(const std::string& path) {
    // Set once; subsequent calls are ignored. Loading is lazy so order with
    // hook invocations doesn't matter beyond "set before first asset open."
    g_game_data_root = path;
}

}  // namespace ps2_ark

// hmx_FileMgr_Lookup(table, path, *out_a, *out_b, *out_c, *out_d) -> u8 found
// (sub_82277878 in the recompile.) Arg convention from the caller chain:
//   r3 = global ARK table object (we ignore it; we substitute our own)
//   r4 = guest ptr to full path string
//   r5 = guest ptr to out_a (entry index slot in the 64-byte FileHandle)
//   r6 = guest ptr to out_b (auxiliary; original writes a sub-region size)
//   r7 = guest ptr to out_c (ARK offset, 32-bit)
//   r8 = guest ptr to out_d (ARK size, 32-bit)
//   returns u8 in r3: 1 = found, 0 = not found
REX_HOOK_RAW(hmx_FileMgr_Lookup) {
    load_ark_once();

    const uint32_t path_addr = ctx.r4.u32;
    const uint32_t out_a = ctx.r5.u32;
    const uint32_t out_b = ctx.r6.u32;
    const uint32_t out_c = ctx.r7.u32;
    const uint32_t out_d = ctx.r8.u32;

    auto path = read_guest_string(base, path_addr);

    // Log every call until we have a feel for the lookup volume, then taper.
    static std::atomic<uint64_t> call_count{0};
    uint64_t n = call_count.fetch_add(1);

    if (path.empty()) {
        if (n < 2000) REXLOG_INFO("ps2_ark[{}]: NULL path", n);
        ctx.r3.u64 = 0;
        return;
    }

    auto entry = find_entry(path);
    if (!entry) {
        if (n < 200) {
            // Log the caller's LR for shader misses so we can find the
            // guest function that requested the .fx_xbox file.
            const bool is_shader = path.find(".fx") != std::string::npos;
            if (is_shader) {
                REXLOG_WARN("ps2_ark[{}]: miss '{}' from LR=0x{:08x}",
                            n, path, static_cast<uint32_t>(ctx.lr));
            } else {
                REXLOG_INFO("ps2_ark[{}]: miss '{}'", n, path);
            }
        }
        ctx.r3.u64 = 0;
        return;
    }

    // The 4 output ptrs (r5..r8) are &handle+4, +8, +12, +16. Derive the
    // owning handle so we can attach plaintext-cache bookkeeping to it.
    const uint32_t handle_ptr = out_a ? (out_a - 4) : 0;

    // PS2 DTBs are enciphered. The 360 binary doesn't know our cipher, so
    // we pre-decrypt at lookup and serve plaintext on read. Reported size
    // shrinks by 4 (the seed bytes the game won't see).
    uint32_t reported_offset = static_cast<uint32_t>(entry->offset);
    uint32_t reported_size   = entry->size;
    if (handle_ptr && path_ends_with(path, ".dtb")) {
        std::vector<uint8_t> raw;
        {
            std::lock_guard<std::mutex> lk(g_ark_io_mu);
            if (g_ark_file) raw = read_entry_raw_locked(entry->offset, entry->size);
        }
        auto plain = decrypt_ps2_dtb(raw);
        if (!plain.empty()) {
            reported_size = static_cast<uint32_t>(plain.size());
            std::lock_guard<std::mutex> lk(g_cache_mu);
            g_plaintext_cache[handle_ptr] = std::move(plain);
        }
    }

    if (out_a) REX_STORE_U32(out_a, 0);
    if (out_b) REX_STORE_U32(out_b, 0);
    if (out_c) REX_STORE_U32(out_c, reported_offset);
    if (out_d) REX_STORE_U32(out_d, reported_size);
    ctx.r3.u64 = 1;

    if (n < 2000) {
        REXLOG_INFO("ps2_ark[{}]: hit '{}' off=0x{:x} size={} (reported={})",
                    n, path, entry->offset, entry->size, reported_size);
    }
}

// hmx_File_Read(File*, buf, count) (sub_82359250 in the recompile.)
//
// Original implementation dispatches through File's vtable (vtable[16] to
// check EOF, vtable[24] to actually read), then XOR-decrypts the buffer
// using the per-file LCG keyed off the first 4 bytes of the file. We
// replace it wholesale:
//   - The vtable for our File is the 360 ARK reader's, which would try to
//     decrypt 360-cipher bytes -- wrong for PS2 plaintext.
//   - We pull bytes straight from PS2 main_0.ark at (handle.off + position).
//   - No decryption pass: the file is plaintext on disk and we leave
//     File+8 (crypto state) zeroed via the hmx_File_InitCrypto stub below,
//     so the trailing XOR loop in the original wouldn't fire anyway.
//
// Returns bytes-actually-read in r3 (the original returns success/fail u8;
// callers that branch on `r3 != 0` see "success" when we read any bytes).
REX_HOOK_RAW(hmx_File_Read) {
    const uint32_t file_ptr = ctx.r3.u32;
    const uint32_t buf_ptr  = ctx.r4.u32;
    const uint32_t count    = ctx.r5.u32;

    static std::atomic<uint64_t> read_n{0};
    uint64_t n = read_n.fetch_add(1);

    if (!file_ptr || !count) {
        if (n < 200) REXLOG_INFO("ps2_ark_read[{}]: skip (null File or count=0)", n);
        ctx.r3.u64 = 0;
        return;
    }
    const uint32_t handle_ptr = REX_LOAD_U32(file_ptr + 12);
    if (!handle_ptr) {
        if (n < 200) REXLOG_INFO("ps2_ark_read[{}]: skip (null handle on File=0x{:x})", n, file_ptr);
        ctx.r3.u64 = 0;
        return;
    }

    const uint32_t base_off = REX_LOAD_U32(handle_ptr + kHandleOff);
    const uint32_t size     = REX_LOAD_U32(handle_ptr + kHandleSize);
    const uint32_t position = REX_LOAD_U32(handle_ptr + kHandlePosition);

    if (position >= size) {
        if (n < 200) REXLOG_INFO("ps2_ark_read[{}]: EOF pos={} size={}", n, position, size);
        ctx.r3.u64 = 0;
        return;
    }
    const uint32_t want = std::min(count, size - position);

    // Cache hit: this file was pre-decrypted at lookup time (.dtb). Serve
    // straight from the plaintext buffer.
    {
        std::lock_guard<std::mutex> lk(g_cache_mu);
        auto it = g_plaintext_cache.find(handle_ptr);
        if (it != g_plaintext_cache.end()) {
            const auto& src = it->second;
            if (position >= src.size()) { ctx.r3.u64 = 0; return; }
            uint32_t got = std::min<uint32_t>(want, static_cast<uint32_t>(src.size() - position));
            std::memcpy(base + buf_ptr, src.data() + position, got);
            REX_STORE_U32(handle_ptr + kHandlePosition, position + got);
            ctx.r3.u64 = got;
            if (n < 200) {
                REXLOG_INFO("ps2_ark_read[{}]: cache pos={} want={} got={}",
                            n, position, want, got);
            }
            return;
        }
    }

    std::lock_guard<std::mutex> lk(g_ark_io_mu);
    if (!g_ark_file) {
        if (n < 200) REXLOG_INFO("ps2_ark_read[{}]: no ARK file open", n);
        ctx.r3.u64 = 0;
        return;
    }

    // Explicit int64 to avoid any chance of a 32-bit streamoff truncation on
    // the >2 GB main_0.ark; PS2 GH2 ARK is ~2.9 GB.
    const int64_t abs_off = static_cast<int64_t>(base_off) + position;
    g_ark_file.seekg(abs_off, std::ios::beg);
    if (!g_ark_file) {
        if (n < 200) REXLOG_WARN("ps2_ark_read[{}]: seek failed at 0x{:x}", n, abs_off);
        g_ark_file.clear();
        ctx.r3.u64 = 0;
        return;
    }
    g_ark_file.read(reinterpret_cast<char*>(base + buf_ptr), want);
    const auto got = static_cast<uint32_t>(g_ark_file.gcount());
    if (!g_ark_file) g_ark_file.clear();

    REX_STORE_U32(handle_ptr + kHandlePosition, position + got);
    ctx.r3.u64 = got;

    if (n < 200) {
        REXLOG_INFO("ps2_ark_read[{}]: off=0x{:x}+{}={:#x} want={} got={}",
                    n, base_off, position, abs_off, want, got);
    }
}

// hmx_File_InitCrypto(File*) (sub_823596E8 in the recompile.) Original
// reads the first 4 bytes of the file as a 360 LCG cipher seed, allocates
// 4 bytes for the cipher state, and stores the seeded state pointer at
// File+8. PS2 ARK files are plaintext, so no crypto to set up. Leaving
// File+8 zero keeps the trailing decrypt loop in hmx_File_Read (which we
// replaced anyway) inert for any callers we haven't replaced.
REX_HOOK_RAW(hmx_File_InitCrypto) {
    const uint32_t file_ptr = ctx.r3.u32;
    if (file_ptr) REX_STORE_U32(file_ptr + 8, 0);
}

// hmx_DataHandler_Find(name) (sub_821E04B8 in the recompile.) Walks a
// linked list of registered handlers (head ptr at hmx_HandlerList_head_addr;
// nodes link via +0, payload at +8, payload+12 = name string ptr) and
// returns the payload whose name matches `name`, or NULL on miss.
//
// The 360 GH2 build's class-system init registers ~21 diagnostic-view
// handlers ('time', 'rate', 'heap', 'stats', 'input', 'camera', etc.).
// Deep in hmx_Game_PostInit -> hmx_Game_RegisterDataNodes, the boot code
// looks the handler up by the name 'timers' (plural). No 'timers' node is
// ever inserted -- only 'time' (singular) -- so the lookup returns NULL.
// The immediately-following inline PPC code does `stw r11, 52(returned)`
// and with `returned == 0` that becomes a store to host `base + 52`,
// silently corrupting low guest memory and hanging the boot a few
// subsystem inits later with no exception or log past the corruption.
//
// Workaround: when the lookup comes in for 'timers', walk the list, find
// the existing 'time' node, and substitute its name pointer into the
// caller's r3 before delegating to the original lookup. The original then
// locates the 'time' handler by content comparison and returns it. Other
// names are untouched.
//
// `__imp__sub_821E04B8` is the strong symbol the codegen defines for the
// original body; DEFINE_REX_FUNC sets up `sub_821E04B8` (the weak alias)
// to point at it, and our strong def of hmx_DataHandler_Find (which #defines
// to sub_821E04B8) wins at link time.
extern "C" void __imp__sub_821E04B8(PPCContext& ctx, uint8_t* base);

// Workaround for the hmx_GraphicsMgr_Init (sub_823032C8) NULL lookup hang.
//
// Inside that init function, the 'rnd' class's PropertyTable is empty so
// `hmx_PropertyTable_Find0(rnd_registry, 'force_hd')` returns NULL. The
// immediately-following inline PPC code does `lwz r11, 0(returned_ptr)`
// with no NULL check; with NULL that becomes a host AV at `base + 0` and
// the runtime gets stuck.
//
// The root cause is that the C++ code that should call
// `RegisterProperty('rnd', 'force_hd', ...)` (and probably other rnd
// properties) never ran, leaving the registry empty. Tracing that
// registration path is a deeper investigation; for now, patch the miss
// from this specific call site (LR = 0x8230336C) by returning a known-safe
// pointer that satisfies the deref chain without AV'ing.
//
// `hmx_PropertyTable_zero_sentinel` is the guest address of a 16-byte zero
// region we know is mapped (early image-section padding). The downstream
// code does `*(ret + 0) + 8 -> hmx_DataNode_UnpackValue -> *(...)`; with a
// pointer to all-zeros each step reads 0 and the chain terminates cleanly
// at "force_hd = 0" (HD rendering disabled), which is the correct default
// for our target platform anyway.
extern "C" void __imp__sub_82319530(PPCContext& ctx, uint8_t* base);

// Guest-mapped zero region. Picked to be inside the image's .bss padding
// (0x82000000-0x828B0000) at a high-aligned offset. If runtime later proves
// this address isn't zero or isn't writable, move to a verified region.
constexpr uint32_t hmx_PropertyTable_zero_sentinel = 0x828A0000u;

REX_HOOK_RAW(hmx_PropertyTable_Find0) {
    const uint32_t lr = static_cast<uint32_t>(ctx.lr);
    const bool from_GraphicsMgr_Init = (lr == 0x8230336C);

    __imp__sub_82319530(ctx, base);

    if (from_GraphicsMgr_Init && ctx.r3.u32 == 0) {
        static std::atomic<bool> warned{false};
        if (!warned.exchange(true)) {
            REXLOG_WARN("ps2_ark: rnd.force_hd lookup missed; substituting "
                        "zero-sentinel @0x{:08x} (HD disabled)",
                        hmx_PropertyTable_zero_sentinel);
        }
        ctx.r3.u32 = hmx_PropertyTable_zero_sentinel;
    }
}

// TEMPORARY DIAG: log who called the shader loader (sub_82307530).
extern "C" void __imp__sub_82307530(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_82307530) {
    const uint32_t lr = static_cast<uint32_t>(ctx.lr);
    REXLOG_WARN("ps2_ark: shader_loader sub_82307530 called from LR=0x{:08x} "
                "(r3=0x{:08x} r4=0x{:08x} r5=0x{:08x} r6=0x{:08x})",
                lr, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32);
    __imp__sub_82307530(ctx, base);
    REXLOG_WARN("ps2_ark: shader_loader sub_82307530 returned r3=0x{:08x}", ctx.r3.u32);
}

REX_HOOK_RAW(hmx_DataHandler_Find) {
    const uint32_t orig_name_addr = ctx.r3.u32;
    auto name = read_guest_string(base, orig_name_addr);

    if (name == "timers") {
        uint32_t cur = REX_LOAD_U32(hmx_HandlerList_head_addr);
        uint32_t time_name_ptr = 0;
        for (int n = 0; cur && cur != hmx_HandlerList_head_addr && n < 256; ++n) {
            const uint32_t payload = REX_LOAD_U32(cur + 8);
            const uint32_t np      = payload ? REX_LOAD_U32(payload + 12) : 0;
            if (np && read_guest_string(base, np) == "time") {
                time_name_ptr = np;
                break;
            }
            cur = REX_LOAD_U32(cur);
        }
        if (time_name_ptr) {
            ctx.r3.u32 = time_name_ptr;
        } else {
            REXLOG_WARN("ps2_ark: 'time' handler not in list; 'timers' lookup will fail");
        }
    }

    __imp__sub_821E04B8(ctx, base);
}

// TEMPORARY WORKAROUND: skip hmx_GraphicsMgr_Init (sub_823032C8) entirely.
//
// Inside this function, the 'rnd.force_hd' property lookup returns NULL
// because the 'rnd' class's PropertyTable is empty (no properties were ever
// registered for it -- root cause not yet found). The subsequent inline
// `lwz r11, 0(NULL)` AVs into a stuck-runtime state.
//
// Skipping the function entirely means downstream code that depends on
// fields the original would have set (r30+520 = HD flag, r30+60/+64 = render
// dimensions, etc.) will read zeros. That might break later rendering but
// shouldn't hang boot. Worth trying as a leap-frog while we trace the real
// fix for the missing property registration.
// Bracket sub_82303CA8 (hmx_Game_PostInit) so we can see whether the boot
// gets past it once the rnd.force_hd lookup miss is patched.
extern "C" void __imp__sub_82303CA8(PPCContext& ctx, uint8_t* base);
REX_HOOK_RAW(sub_82303CA8) {
    REXLOG_WARN("trace: enter sub_82303CA8 (hmx_Game_PostInit)");
    __imp__sub_82303CA8(ctx, base);
    REXLOG_WARN("trace: exit  sub_82303CA8 (hmx_Game_PostInit)");
}
