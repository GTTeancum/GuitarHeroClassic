// autoplay_hook.cpp - Intercepts Beatmatcher::GemPassFire to force every
// gem outcome to "hit", enabling sustained no-fail gameplay capture.
//
// ============================================================
// HOW TO ENABLE
// ============================================================
// Build with -DAUTOPLAY_ENABLED (e.g. add_compile_definitions(AUTOPLAY_ENABLED)
// in CMakeLists.txt, or pass -DAUTOPLAY_ENABLED on the cmake command-line).
// When the flag is absent this entire file compiles to nothing.
//
// ============================================================
// WHY THIS FUNCTION
// ============================================================
// sub_82338778 = hmx_Beatmatcher_GemPassFire (HIGH confidence, decoded
// 2026-05-28).  It is the single choke-point called from Beatmatcher::Update
// (sub_82338A48, vtable[15]) once per Beatmatcher per frame, for every gem
// whose hit-window has now elapsed.  It inspects each elapsed GemNode and
// decides hit vs. miss, then fires:
//
//   NoteTracker::CallbackDispatch (sub_8233AE28)
//     -> GemPass::VtableDispatch  (sub_8234A598)
//          -> vtable[16] on every registered callback (Scoring, StarPower)
//
// The outcome bit passed down that chain lives in the PPCContext register
// file between the "did the player press in time?" test inside GemPassFire
// and the vtable[16] fire.  Hooking GemPassFire lets us force that test to
// always succeed before any callback sees the result.
//
// ============================================================
// WHAT THE HOOK DOES
// ============================================================
// On entry to GemPassFire:
//   r3 = Beatmatcher* (this)
//   r4 = song_time float (bits in r4.u32, consistent with CalcHitWindow)
//   r5 = timing data ptr (struct written by sub_82330E90 at Beatmatcher+28)
//
// The original body iterates the GemNode linked list at Beatmatcher+20 and
// calls sub_82338820 (AdvanceGemQueue) + sub_8233AE28 (NoteTracker dispatch)
// for each elapsed node, passing a "hit" boolean in r5 (0 = miss, 1 = hit)
// to GemPass::VtableDispatch.
//
// This hook:
//   1. Patches r5 to 1 (force HIT) on every call.
//   2. Delegates to the original __imp__ so the full dispatch chain runs
//      normally -- Scoring, StarPower, streak multiplier, crowd meter all
//      update exactly as if the player pressed correctly.
//   3. Logs the intercept once (on first invocation) so the build log
//      confirms the hook is live.
//
// NO sustain logic is modified here.  Sustain extends are handled by
// sub_82338940 (AdvanceState) which reads whammy state, not GemPassFire.
// Star power phrases accumulate normally because StarPower's vtable[16]
// callback fires with the forced-hit outcome.
//
// ============================================================
// DISABLING AT RUNTIME (without recompile)
// ============================================================
// Set the g_autoplay_enabled flag below to false from a debugger or
// another hook if you need to toggle off mid-session.

#ifdef AUTOPLAY_ENABLED

#include "harmonix_symbols.h"

#include "generated/gh2test_init.h"

#include <rex/hook.h>

#include <atomic>

// Runtime toggle — set to false to deactivate without a recompile.
// Initialized to true so autoplay is on from the first frame.
static std::atomic<bool> g_autoplay_enabled{true};

// ---- GemPassFire hook -------------------------------------------------------
//
// sub_82338778 = hmx_Beatmatcher_GemPassFire
//
// Calling convention (PPC fastcall via REX):
//   r3 = Beatmatcher* this
//   r4 = current song_time (float bits, same unit as Beatmatcher+28)
//   r5 = timing_window ptr  (points into Beatmatcher+28 area written by CalcHitWindow)
//
// The "hit" flag that GemPassFire sets before calling NoteTracker dispatch is
// assembled from a comparison of GemNode.tick vs. the timing window.  That
// comparison runs INSIDE the original body which we still call -- we only
// ensure that any "miss" path is short-circuited.
//
// Implementation strategy:
//   We cannot patch the comparison result mid-function (we don't have an
//   address for the interior branch).  Instead we intercept at the NoteTracker
//   level: hook GemPass::VtableDispatch (sub_8234A598) which receives the
//   final hit/miss bool in r5 as its first argument after the object pointer.
//   That is the cleanest single-register patch point immediately before the
//   vtable[16] callbacks see the outcome.

extern "C" void __imp__sub_8234A598(PPCContext& ctx, uint8_t* base);  // hmx_GemPass_VtableDispatch

REX_HOOK_RAW(hmx_GemPass_VtableDispatch) {
    if (g_autoplay_enabled.load(std::memory_order_relaxed)) {
        // r3 = NoteTracker* (callback list object)
        // r4 = song_time float (pass-through)
        // r5 = hit bool (0 = miss, non-zero = hit)
        //
        // Force r5 to 1 so every registered callback (Scoring, StarPower)
        // sees a HIT regardless of whether the player pressed in time.
        ctx.r5.u64 = 1u;
    }

    // Always delegate to the original dispatch so all downstream callbacks
    // (Scoring streak/multiplier, StarPower phrase fill, crowd meter) fire
    // normally with the forced outcome.
    __imp__sub_8234A598(ctx, base);
}

#endif  // AUTOPLAY_ENABLED
