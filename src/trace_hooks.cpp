// trace_hooks.cpp - REX_HOOK_RAW wrappers that feed the trace_recorder.
//
// Each hook here wraps a known-named guest function (see harmonix_symbols.h
// / recomp_symbols.md), captures the salient inputs/outputs, delegates to
// the original via __imp__, and writes a structured event to the recorder.
//
// All hooks are pass-through (delegate to __imp__) so the recompile runs
// normally. Adding hooks here doesn't change behavior, only observability.

#include "harmonix_symbols.h"
#include "trace_recorder.h"

#include "generated/gh2test_init.h"

#include <rex/hook.h>

#include <cstring>
#include <string>

namespace {

// Read a null-terminated guest string with a bounded scan so a stray
// pointer can't run off the end of memory.
std::string read_guest_string(uint8_t* base, uint32_t guest_addr) {
    if (!guest_addr) return {};
    const char* p = reinterpret_cast<const char*>(base + guest_addr);
    constexpr size_t kMax = 1024;
    size_t n = 0;
    while (n < kMax && p[n]) ++n;
    return std::string(p, n);
}

}  // anonymous namespace

// --- File opens ------------------------------------------------------------
//
// hmx_FileMgr_Lookup(table, path, *out_a, *out_b, *out_c, *out_d) -> u8 found
//
//   r3 = global ARK table object (we don't read it)
//   r4 = guest ptr to path string
//   r5..r8 = out-pointer slots (entry index, aux, ARK offset, ARK size)
//   returns u8 (1 = found, 0 = miss)
//
// After delegating to the original we read the offset/size that got
// written into out_c (r7) / out_d (r8) so the trace records the actual
// resolved location.

// NB: the __imp__ prefix on an extern decl must use the underlying linker
// symbol (sub_XXXX); the preprocessor doesn't expand #defines inside a
// larger identifier like __imp__hmx_X. The REX_HOOK_RAW macro itself does
// see the hmx_ alias as a standalone token and expands it correctly.
extern "C" void __imp__sub_82277878(PPCContext& ctx, uint8_t* base);  // hmx_FileMgr_Lookup
REX_HOOK_RAW(hmx_FileMgr_Lookup) {
    const uint32_t path_addr = ctx.r4.u32;
    const uint32_t out_c     = ctx.r7.u32;
    const uint32_t out_d     = ctx.r8.u32;

    __imp__sub_82277878(ctx, base);

    const bool found = (ctx.r3.u32 & 0xFF) != 0;
    const uint64_t off = (found && out_c) ? REX_LOAD_U32(out_c) : 0;
    const uint32_t sz  = (found && out_d) ? REX_LOAD_U32(out_d) : 0;
    auto path = read_guest_string(base, path_addr);
    trace360::LogFileOpen(path, off, sz, found);
}

// --- Property registry lookups ---------------------------------------------
//
// hmx_PropertyTable_Find0(table, key, _) -> entry
//
//   r3 = class PropertyTable
//   r4 = key (typically a pointer to an interned string)
//   returns r3 = entry pointer (NULL on miss)
//
// The "class" identity is upstream of this call (the caller does
// hmx_ClassReg_Lookup first to get the table). We don't track the class
// here; just the key string and the returned pointer. Pairing class to
// lookup is left to offline analysis using nearby string-copies.

extern "C" void __imp__sub_82319530(PPCContext& ctx, uint8_t* base);  // hmx_PropertyTable_Find0
REX_HOOK_RAW(hmx_PropertyTable_Find0) {
    const uint32_t key_addr = ctx.r4.u32;
    __imp__sub_82319530(ctx, base);
    auto key = read_guest_string(base, key_addr);
    trace360::LogPropertyLookupA("?", key, key_addr, ctx.r3.u32);
}

// --- DataHandler (named handler list) lookups ------------------------------
//
// hmx_DataHandler_Find(name) -> payload
//
//   r3 = name string ptr
//   returns r3 = payload (NULL on miss)

extern "C" void __imp__sub_821E04B8(PPCContext& ctx, uint8_t* base);  // hmx_DataHandler_Find
REX_HOOK_RAW(hmx_DataHandler_Find) {
    const uint32_t name_addr = ctx.r3.u32;
    __imp__sub_821E04B8(ctx, base);
    auto name = read_guest_string(base, name_addr);
    trace360::LogHandlerLookupA(name, name_addr, ctx.r3.u32);
}

// --- Class registry lookups (highest-leverage hook) ------------------------
//
// hmx_ClassReg_Lookup(class_symbol) -> PropertyTable*
//
//   r3 = class symbol (Sandbox Symbol = pointer to interned string)
//   returns r3 = per-class PropertyTable pointer (null on miss)
//
// Fires every time the engine asks "what's the PropertyTable for class
// X?", which happens at the head of most class method dispatches and
// every PropertyTable-Find chain. Capturing this gives us a per-frame
// list of WHICH CLASSES are active in the engine — directly surfacing
// the lighting / camera / anim / vfx subsystems that are otherwise
// invisible to FileMgr / PropertyTable / Handler hooks. Required for
// the 1:1 in-song fidelity scope (see [[port-fidelity-scope]] memory).

extern "C" void __imp__sub_82270D20(PPCContext& ctx, uint8_t* base);  // hmx_ClassReg_Lookup
REX_HOOK_RAW(hmx_ClassReg_Lookup) {
    const uint32_t sym_addr = ctx.r3.u32;
    __imp__sub_82270D20(ctx, base);
    auto name = read_guest_string(base, sym_addr);
    trace360::LogClassLookupA(name, sym_addr, ctx.r3.u32);
}
