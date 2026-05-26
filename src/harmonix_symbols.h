// harmonix_symbols.h - Aliases from raw recompile symbols (sub_82XXXXXX) to
// Harmonix Sandbox-engine concepts as we decode them.
//
// The recompile codegen names everything `sub_<hex_address>` because it has
// no symbol info. Each name we figure out gets a #define here so the rest of
// our hook code can use it directly. The linker still sees the underlying
// sub_<addr> symbol -- DEFINE_REX_FUNC sets it up as a weak alias for
// __imp__sub_<addr>, and our hooks override it via REX_HOOK_RAW(<name>),
// which after #define expansion still targets the right linker name.
//
// Confidence levels for each name are tracked in the project memory file
// `recomp_symbols.md` (HIGH / MEDIUM / LOW). The header used to keep only
// HIGH-confidence names; that policy is relaxed to also include MEDIUM/LOW
// ones (with comment markers) so the code can use readable names everywhere
// rather than raw sub_XXXXXXXX. Promote a name's confidence by reading its
// body and updating the comment.
//
// Naming: hmx_<Class>_<Method> for member functions, hmx_<Subsys>_<Func>
// for free functions, hmx_<NAME>_addr for data globals. Mirrors Harmonix's
// own Hmx namespace usage. The "?" in a comment marks a MEDIUM/LOW guess.

#pragma once

// ===========================================================================
//                              BOOT & FRAME LOOP
// ===========================================================================

// ---- Boot path (HIGH conf, bodies decoded) -------------------------------

#define hmx_main_xstart            sub_82120B58   // XEX entrypoint
#define hmx_main_ProgramInit       sub_82120208   // boot init, returns when done
#define hmx_main_MainLoop          sub_82120090   // infinite per-frame loop ⭐
#define hmx_App_EngineInit         sub_82271618   // engine subsystem init umbrella
#define hmx_App_LoadBootAssets     sub_82271070   // ~108 boot DTBs

// ---- Per-frame ticks called from MainLoop (HIGH conf, order verified) ----
//
// MainLoop body calls these in fixed order each frame. See memory
// subsystems/frame_loop.md for the full 10-call sequence including the
// 3 virtual calls on the Game singleton.

#define hmx_Input_FrameTick        sub_82271228   // input pump (controller poll)
#define hmx_Audio_FrameTick        sub_822B0588   // audio mixer / buffer pump
#define hmx_Time_FrameTick         sub_82274A60   // clock advance + 10 timer subs
#define hmx_Stats_FrameTick        sub_82281D50   // net / stats / leaderboards
#define hmx_UIWidget_FrameTick     sub_8236B460   // UI widget tree pre-update
#define hmx_UIWidget_PostFrame     sub_8236ADC8   // UI post-frame layout finalize
#define hmx_Scheduler_FrameTickWrapper sub_82313DD0  // wraps Scheduler_Walk
#define hmx_Scheduler_Walk         sub_82313CB0   // per-frame Object::Update dispatcher ⭐

// ===========================================================================
//                          OBJECT & PROPERTY MACHINERY
// ===========================================================================

// ---- Object base class (HIGH conf where decoded) -------------------------
//
// struct Object {
//   void**   vtable;      // +0
//   Object*  parent;      // +4
//   /* +8 */
//   uint16_t refcount;    // +10
//   /* +12 */
// };
//
// vtable slots (HIGH where confirmed):
//   +24 (slot 6)  : bool GetPropertyOverride(Symbol, DataNode& out)
//   +60 (slot 15) : void Update(float dt)         ⭐ per-frame tick
//   +72 (slot 18) : Game::PreFrame  (Game subclass)
//   +88 (slot 22) : Game::Render
//   +92 (slot 23) : Game::Present

#define hmx_Object_HandleProperty  sub_82316428   // base msg dispatch (116 callers, 59-label switch)
#define hmx_Object_GetProperty     sub_82315440   // ? base GetProperty w/ parent chain
#define hmx_Object_Release         sub_82120818   // refcount decrement at +10, free at zero
#define hmx_Object_AddRef          sub_82120690   // ? companion to Release

// ---- Class registry (HIGH conf) ------------------------------------------
//
// Three lookup variants; v3 is most commonly used. Lookups all hit the
// PropertyTable_Find primitives internally.

#define hmx_ClassReg_Lookup        sub_82270D20   // top-level lookup
#define hmx_ClassReg_Lookup_alt    sub_82270D38   // 18-insn wrapper variant
#define hmx_ClassReg_Lookup_v3     sub_82270D80   // 24-insn variant, dominates lookups

// ---- PropertyTable (HIGH conf) -------------------------------------------
//
// struct PropertyTable {
//   DataNode data[16];          // +0
//   int16_t  capacity;          // +8
//   int16_t  flags;             // +10
//   int16_t  count;             // +14
//   PropertyTable* parent;       // +16  chained-class lookup
// };

#define hmx_PropertyTable_Find     sub_82319448   // typed search (binary / linear)
#define hmx_PropertyTable_Find0    sub_82319530   // wrapper, "find without create"

// ---- DataNode (HIGH conf, body decoded) ----------------------------------
//
// struct DataNode { uint32_t payload; uint32_t type; };  // 8 bytes
// Type IDs: 2=INT, 17=DeferredRef, 19=SymbolRef (more exist)

#define hmx_DataNode_Resolve       sub_82317EF8   // tagged-union dispatch
#define hmx_DataNode_AsInt         sub_82317FE8
#define hmx_DataNode_AsFloat       sub_823180E8
#define hmx_DataArray_Destruct     sub_8229E968   // ? destructor (called by Scheduler for null-obj nodes)

// ---- Symbols / strings (HIGH conf) ---------------------------------------

#define hmx_String_CopyOrIntern    sub_82355DA8   // intern → Symbol
#define hmx_String_HashMod         sub_82691050   // (str, mod) hash

// ---- Memory / generic primitives (HIGH conf) -----------------------------

#define hmx_Mem_Alloc              sub_82354FD8   // malloc
#define hmx_Mem_Free               sub_82355020   // ? free(size, ptr)
#define hmx_memset                 sub_8239CD50   // memset shim
#define hmx_FrameStats_RingPush    sub_82279FB0   // (u32,u32) into 5-slot ring at 0x82155F44

// ---- DataHandler list (HIGH conf) ----------------------------------------
//
// Linked list at hmx_HandlerList_head_addr. Each node: +0 = next, +8 =
// payload, payload+12 = name string. Used for the runtime's developer-view
// screen handlers ('time', 'rate', 'heap', 'stats', 'input', 'camera', etc.).

#define hmx_DataHandler_Find       sub_821E04B8

// ===========================================================================
//                              FILE SYSTEM / IO
// ===========================================================================

// ---- File / asset I/O (HIGH conf where decoded) --------------------------

#define hmx_FileMgr_Lookup         sub_82277878   // path -> (off, size, found)
#define hmx_File_ctor              sub_82357A10   // File(path)
#define hmx_File_IsMissing         sub_82357D40   // returns u8 *(File+36)
#define hmx_File_Read              sub_82359250   // (File*, buf, count)
#define hmx_File_InitCrypto        sub_823596E8   // first-4-bytes-as-seed
#define hmx_FileOps_OpenAndParse   sub_8231C520   // path -> ParserObj
#define hmx_ArkRead_LowLevel_A     sub_822767F8   // ? low-level ARK byte-read primitive
#define hmx_ArkRead_LowLevel_B     sub_8227DC18   // ? companion

// ---- Asset loaders by file extension (MEDIUM conf, pinned via file_ext --
//      stack-sample hook; bodies decoded partially) ---------------------

#define hmx_MidiParser_Load        sub_822D59C0   // .mid -> chart event list ⭐
#define hmx_MiloLoader_Load        sub_8235F4B0   // .milo_xbox scene/asset bundle ⭐
#define hmx_FaceAnim_Load          sub_8214E030   // .fac face / viseme data
#define hmx_VocalTrack_Load        sub_821915D8   // .voc vocal audio
#define hmx_MoggDecoder_Open       sub_826820D8   // ? .mogg multi-channel OGG stream
#define hmx_TextureLoader_Load     sub_821C8A30   // ? .bmp_xbox texture
#define hmx_ShaderLoader_Load      sub_82307530   // ? .fx_xbox shader / effect
#define hmx_ParticleSys_Load       sub_82307F30   // .dtx particle config (also ParticleSys::RegisterFactory)
#define hmx_Generic_File_Parser    sub_8235A458   // ? generic format-dispatch parser (called by .mid/.fac/.bmp loaders)

// ===========================================================================
//                       SUBSYSTEM REGISTERS (boot-time init)
// ===========================================================================
//
// Each *_Register is the immediate caller of hmx_ClassReg_Lookup for that
// subsystem's class name at boot. Pinned via Phase 4b stack-sample hook.
// Confidence MEDIUM unless promoted by reading the function body.

// ---- Gameplay-mechanic subsystems ----

#define hmx_Beatmatch_Register     sub_822D08F0   // ? singleton song clock register
#define hmx_Beatmatcher_Register   sub_82330038   // ? per-player gem queue register
#define hmx_Beatmatcher_ConfigInit sub_823341D0   // ? hopo_threshold + per-track config
#define hmx_Note_ConfigInit        sub_822DE640   // ? note_weight config
#define hmx_TrackConfig_GetExtendSec sub_822CCA60 // ? track_extend_sec accessor
#define hmx_TrackMapping_Lookup    sub_82346FA8   // ? track lane mapping lookup
#define hmx_TrackGraphics_Init     sub_82692BA8   // ? track_graphics class register
#define hmx_TrackWidget_BuildConfig sub_822B3BB8  // ? anim_tempo config builder
#define hmx_Scoring_Register       sub_822D69C8   // shares site with star_power; body partially decoded
#define hmx_BoostMeter_Init        sub_822D5FC0   // body decoded: 10 float config reads
#define hmx_StarPower_AudioFXVolume sub_822E44B0  // ? star_power_fx_volume init

// ---- Visual / character subsystems ----

#define hmx_Light_Construct        sub_821BBA20   // ? Light class register
#define hmx_ParticleSys_RegisterFactory sub_82307F30 // same fn as hmx_ParticleSys_Load
#define hmx_Character_VFXInit      sub_8232E9F8   // ? flame_hands VFX setup
#define hmx_Gem_RenderInit         sub_826A0270   // ? sparkle_len config
#define hmx_Character_Register     sub_822A7EC8   // ? characters class register
#define hmx_Guitar_Construct       sub_822A7B30   // walks guitar registry; also owns firebird VFX
#define hmx_Venue_Register         sub_822A75D0   // ? venues class register
#define hmx_DefaultBand_Register   sub_822B7670   // ? default_band register
#define hmx_Character_VenueParent  sub_822A88C8   // ? parent of characters/guitars register calls

// ---- Audio subsystems ----

#define hmx_Synth_Register         sub_822E9BC0   // ? synth class register
#define hmx_Sound_Register         sub_822B1E58   // ? sound class register
#define hmx_Crowd_Init             sub_822D6320   // ? crowd class register
#define hmx_Crowd_AudioConfig      sub_822E0928   // ? crowd_audio_delay + crowd_reactions

// ---- HUD / UI subsystems ----

#define hmx_HUD_Register           sub_82690A00   // ? hud class register
#define hmx_UI_Register            sub_8236D1F0   // ? ui class register
#define hmx_FocusAnim_Constants    sub_82124C08   // ? focus_anim_duration

// ---- Input subsystems ----

#define hmx_Joypad_Register        sub_8227B4A0   // ? joypad class register
#define hmx_JoypadConfig_Init      sub_8236C4C0   // reads use_joypad, applies via SetJoypadMode
#define hmx_JoypadConfig_SetJoypadMode sub_8236A338 // HIGH; body decoded: 904-byte joypad obj
#define hmx_GuitarInput_Poll       sub_8227CC98   // ? per-frame guitar/controller poller
#define hmx_Input_ExtractState     sub_8227C690   // ? reads packed input -> typed snapshot
#define hmx_Input_GetProperty      sub_8227C540   // ? property accessor for input class

// ---- Game-state / engine subsystems ----

#define hmx_GameState_GetProp      sub_82315440   // ? gameplay-state property reader
#define hmx_TypeSystem_Lookup      sub_8231C660   // ? superclasses chain lookup
#define hmx_Context_Register       sub_822E1058   // ? contexts class register
#define hmx_Campaign_Register      sub_822A3910   // ? campaign class register
#define hmx_Achievements_Register  sub_8229D910   // ? achievements class register
#define hmx_Leaderboards_Register  sub_822D6EE0   // ? leaderboards class register
#define hmx_Store_Register         sub_822A7DD8   // ? store class register
#define hmx_Tips_Register          sub_822825E8   // ? tips class register
#define hmx_Locale_Register        sub_82356F30   // ? locale class register
#define hmx_LongCheats_Register    sub_8235C610   // ? long_cheats class register
#define hmx_Math_Register          sub_8226DDF0   // ? math class register
#define hmx_Mem_Register           sub_82355048   // ? mem class register
#define hmx_Timer_Register         sub_82272240   // ? timer class register
#define hmx_Rnd_Register           sub_821E68E0   // ? rnd class register (also hmx_Game_RegisterDataNodes)

// ---- Result / boot UI ----

#define hmx_BootScreen_StarsAnim   sub_8214A980   // ? title-screen stars
#define hmx_StarRating_Render      sub_8269C968   // ? post-song star rating

// ===========================================================================
//                              DATA GLOBALS
// ===========================================================================

#define hmx_HandlerList_head_addr      0x82782E3Cu
#define hmx_GlobalClassTable_addr      0x8278492Cu
#define hmx_File_vtable_addr           0x8205C014u
#define hmx_FileHandle_vtable_addr     0x8200ED5Cu
#define hmx_GameSingleton_ptr_addr     0x82746950u   // *(GameSingleton_ptr_addr) = Game*
#define hmx_FrameStats_ring_addr       0x82155F44u   // 5-slot (u32,u32) ring used by FrameStats_RingPush
#define hmx_MILO_vtable_addr           0x82038BFCu   // MILO object vtable (set by MiloLoader_Load)
