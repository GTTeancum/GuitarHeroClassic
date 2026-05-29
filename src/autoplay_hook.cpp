// autoplay_hook.cpp - Two hooks that together give unattended, full-fidelity
// gameplay capture:
//
//   1. GemPass_VtableDispatch hook  — forces every gem outcome to HIT so the
//      player never fails out. Scoring, streak multiplier, star-phrase fill,
//      and crowd meter all update normally.
//
//   2. Beatmatch_Update hook        — periodically fires star-power activation
//      so that SP deploys, firebird VFX spawn, flame_hands attach, gem SP-glow,
//      crowd-boost audio, and SP character-animation states are all exercised
//      during a headless trace run.
//
// ============================================================
// HOW TO ENABLE
// ============================================================
// Build with -DAUTOPLAY_ENABLED.  CMakeLists.txt for trace-360 always sets
// this.  When absent the entire file compiles to nothing.
//
// ============================================================
// HOOK 1 — GemPass::VtableDispatch (sub_8234A598)
// ============================================================
// The canonical gem-outcome choke-point.  Called from:
//
//   NoteTracker::CallbackDispatch (sub_8233AE28)
//     <- Beatmatcher::Update (per-gem window expiry)
//
// Calling convention:
//   r3 = NoteTracker* (callback-list object)
//   r4 = song_time float (pass-through)
//   r5 = hit bool  (0 = miss, non-zero = hit)
//
// The hook forces r5 = 1 before vtable[16] fires, so both registered
// callbacks (Scoring at sub_822D5944, StarPower at sub_822D5024) see a HIT.
// Star phrases therefore fill the SP gauge naturally from phrase hits.
//
// ============================================================
// HOOK 2 — Beatmatch::Update (sub_823123D0, vtable[15])
// ============================================================
// In headless mode there is no controller, so the SP activation button
// (tilt / face-button in joypad mode) is never pressed.  The gauge fills from
// phrase hits but never deploys, meaning these systems are never exercised:
//
//   - firebird particle spawn on the guitar (Guitar+56 vector, named child)
//   - flame_hands particle attach to character IK hands (sub_8232E7A8)
//   - gem SP-glow dirty-bit 0x800 (sub_826927C0 / hmx_Gem_SetStarPower)
//   - SP audio start / crowd-boost scalar (sub_822DF590 / sub_8233CC78)
//   - SP character animation states (unknown; need observation to decode)
//   - SP multiplier (sp_mult = 2) in scoring
//
// This hook fires hmx_Beatmatch_SPActivate (sub_822CB260) after every
// SP_DEPLOY_INTERVAL Beatmatch::Update calls (~6s at 60Hz).
//
// CONFIDENCE NOTE: sub_822CB260 is MEDIUM confidence — decoded as
// "star-power activation handler; triggers second TryHit call in PlayerUpdate."
// If it turns out to only cover the double-TryHit path and not the full
// visual/audio SP deploy, a follow-up decode pass will identify the actual
// state-machine transition.  Adding the hook now lets us observe at runtime
// (via trace events) exactly which downstream calls fire, which is the
// information we need to complete that decode.
//
// Timing: g_sp_deploy_timer starts at SP_DEPLOY_INTERVAL/2 (180) so the
// first activation fires ~3s into gameplay, before the second phrase hit.
// After that it fires every ~6s.  SPActivate is a no-op if the gauge is
// below ready_level (0.5), so early firings before the gauge fills are safe.
//
// ============================================================
// RUNTIME TOGGLE
// ============================================================
// g_autoplay_enabled is a std::atomic<bool> initialized to true.  Set it
// false from a debugger or another hook to disable both hooks mid-session
// without a recompile.

#ifdef AUTOPLAY_ENABLED

#include "harmonix_symbols.h"
#include "generated/gh2test_init.h"
#include <rex/hook.h>
#include <atomic>

// Runtime toggle.  Both hooks respect this flag.
static std::atomic<bool> g_autoplay_enabled{true};

// ===========================================================================
// HOOK 1 — GemPass::VtableDispatch
// Force every gem outcome to HIT before vtable[16] callbacks fire.
// ===========================================================================

extern "C" void __imp__sub_8234A598(PPCContext& ctx, uint8_t* base);  // hmx_GemPass_VtableDispatch

REX_HOOK_RAW(hmx_GemPass_VtableDispatch) {
    if (g_autoplay_enabled.load(std::memory_order_relaxed)) {
        // r5 = hit bool.  Force to 1 (HIT) so Scoring and StarPower callbacks
        // both see a successful gem pass regardless of player timing.
        ctx.r5.u64 = 1u;
    }
    __imp__sub_8234A598(ctx, base);
}

// ===========================================================================
// HOOK 2 — Beatmatch::Update (vtable[15])
// Periodically fire SP activation so deployment effects are exercised.
// ===========================================================================

// Frames between SP activation attempts.  At ~60Hz Scheduler cadence this is
// approximately 6 seconds.  The first fire is at half-interval (~3s) because
// g_sp_deploy_timer is pre-loaded to SP_DEPLOY_INTERVAL / 2.
static const int SP_DEPLOY_INTERVAL = 360;
static int       g_sp_deploy_timer  = SP_DEPLOY_INTERVAL / 2;

extern "C" void __imp__sub_823123D0(PPCContext& ctx, uint8_t* base);  // hmx_Beatmatch_Update (original)
extern "C" void __imp__sub_822CB260(PPCContext& ctx, uint8_t* base);  // hmx_Beatmatch_SPActivate

REX_HOOK_RAW(hmx_Beatmatch_Update) {
    // Save Beatmatch* (r3) before __imp__ clobbers the context registers.
    const uint64_t beatmatch_this = ctx.r3.u64;

    // Run the real per-frame update: MIDI dispatch, Beatmatcher iteration,
    // NoteTracker window evaluation, gem-pass firing.
    __imp__sub_823123D0(ctx, base);

    // After the update is complete (no re-entrancy), periodically trigger SP.
    if (g_autoplay_enabled.load(std::memory_order_relaxed)) {
        if (++g_sp_deploy_timer >= SP_DEPLOY_INTERVAL) {
            g_sp_deploy_timer = 0;
            // Restore r3 = Beatmatch* this for the SPActivate call.
            ctx.r3.u64 = beatmatch_this;
            __imp__sub_822CB260(ctx, base);
        }
    }
}

#endif  // AUTOPLAY_ENABLED
