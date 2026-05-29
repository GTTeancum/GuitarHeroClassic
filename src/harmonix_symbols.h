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

// ---- App (Hmx::App / GHApp) vtable at 0x8205093C (HIGH conf, all 18 slots) ----
//
// App singleton B ptr lives at 0x827469D0 (r30=0x82740000, offset +27088).
// Per-frame loop calls vtable[+88] (BeginPresent) then vtable[+92] (Present) on it.
// vtable[+72] is a STUB on App — the per-frame [+72] call is on a DIFFERENT singleton A.
//
// Singleton A ptr @ 0x8280E274 — class UNKNOWN (MsgMgr/TaskMgr?); vtable[+72] = per-frame dispatch

#define hmx_App_GetTypeProp        sub_821224F0   // vtable[+12] — copies type DataNode from cache
#define hmx_App_HandleType         sub_82122EA0   // vtable[+16] — ClassReg lookup + PropertyTable dispatch
#define hmx_App_Init               sub_82301578   // vtable[+20] — app init: parse config, init render ctx
#define hmx_RenderHelper_GetNode   sub_823150B8   // vtable[+24] — 1-call DataNode getter
#define hmx_FileLoader_Seek        sub_82314FF8   // vtable[+28] — file seek ops
#define hmx_FileLoader_Open        sub_82315258   // vtable[+32] — alloc buffer + open file
#define hmx_FileLoader_Read        sub_82315350   // vtable[+36] — read into buffer (calls hmx_File_Read)
#define hmx_FileLoader_Release     sub_823151E0   // vtable[+40] — ref-count dec / close
#define hmx_FileLoader_Tell        sub_82315150   // vtable[+44] — position / size query
#define hmx_Object_GetFileMgr      sub_82314D68   // vtable[+48] — null-check + return default FileMgr singleton
#define hmx_App_vtable36_thunk     sub_82122520   // vtable[+52] — re-dispatches to vtable[+36]
// vtable[+56] = sub_822F5468  STUB
// vtable[+60] = sub_823055E0  hmx_Game_PostInit_thunk (boot init, not per-frame)
#define hmx_App_PreRender          sub_82303270   // vtable[+64] — begin-scene setup (5 direct calls)
#define hmx_App_GetFrameFlag       sub_821E7298   // vtable[+68] — tiny getter (10 insns, no callees)
// vtable[+72] = sub_822F5468  STUB (App; per-frame [+72] is on singleton A, different class)
#define hmx_App_DrawViewport       sub_82301720   // vtable[+76] — thunk(type=13) → sub_82300F20 scene compositor
#define hmx_App_Render             sub_82302CF8   // vtable[+80] — main scene render: GPU state + geometry batch ⭐
#define hmx_App_RenderOverlay      sub_82300B90   // vtable[+84] — secondary render pass (overlay/HUD layer)
#define hmx_App_BeginPresent       sub_82305BA8   // vtable[+88] — GPU frame-submit: flip back-buffer, submit cmd bufs ⭐
#define hmx_App_Present            sub_823055E8   // vtable[+92] — end-of-frame: batch dispatch + db16cyc GPU stall + mftb ⭐

// ---- Per-frame ticks called from MainLoop (HIGH conf, order verified) ----
//
// MainLoop body calls these in fixed order each frame. See memory
// subsystems/frame_loop.md for the full 10-call sequence including the
// 3 virtual calls on the singleton objects.
//
// NOTE: sub_82274A60 is NOT Time::FrameTick — it is the Xbox notification event
// pump (hmx_Xbox_PollNotifications). The song clock lives in Beatmatch.
// Step 4 in the frame loop is Xbox live/storage/controller connect notifications.

#define hmx_Input_FrameTick        sub_82271228   // input pump (controller poll)
#define hmx_Audio_FrameTick        sub_822B0588   // audio mixer / buffer pump
#define hmx_Xbox_PollNotifications sub_82274A60   // XNotify event pump: storage/profile/sign-in (NOT Time::FrameTick)
#define hmx_Stats_FrameTick        sub_82281D50   // achievement/stats poll; state 997=terminal; this+40/+52/+360
#define hmx_UIWidget_FrameTick     sub_8236B460   // UI widget tree pre-update; mftb at entry; child walk at this+48
#define hmx_UIWidget_PostFrame     sub_8236ADC8   // UI post-frame layout finalize
#define hmx_Scheduler_FrameTickWrapper sub_82313DD0  // 65-insn wrapper: mftb→dt, updates 4 SchedList.now, calls Walk×4. Pause flag at Scheduler+64. HIGH 2026-05-28
#define hmx_Scheduler_Walk         sub_82313CB0   // 38-insn: walks one SchedList (sorted by time); calls obj->vtable[60](obj,dt); tombstone-frees null-obj nodes. HIGH 2026-05-28
#define hmx_Scheduler_Register     sub_82313EE8   // (Scheduler*, Object*, priority_u8, parent*) → alloc SchedNode, sorted-insert into lists[priority]. HIGH 2026-05-28
#define hmx_Scheduler_InsertSorted sub_82313D48   // sorted insert into SchedList by scheduled_time. HIGH 2026-05-28
#define hmx_SchedList_LinkBefore   sub_82313A10   // doubly-linked list insert-before pointer surgery. HIGH 2026-05-28
#define hmx_Scheduler_Reschedule   sub_823143A0   // (SchedNode*, dt) → stamp new time, sorted-insert; called from Object::Update to re-queue self. HIGH 2026-05-28
#define hmx_SchedList_Clear        sub_82313A78   // destroy+free all nodes in a SchedList; reset sentinel. HIGH 2026-05-28
#define hmx_SchedNode_SetTime      sub_82312D90   // writes scheduled_time to node+12, vtable 0x82FF2554 to node+0. HIGH 2026-05-28
// Scheduler singleton: 0x827FEC40 (lis -32127 + addi -5056 = 0x82810000 - 0x13C0)

// ---- Time / wall-clock subsystem — DECODED 2026-05-28 ----
//
// Wall-clock is PPC mftb (REX_QUERY_TIMEBASE). 176 mftb call sites across all shards.
// Frequency constants (CORRECTED — NOT 0x82004970/74 as previously documented):
//   0x82784970 = freq_lo_f32 = float(1 / timebase_hz)     ≈ 2.0e-8 at 50MHz
//   0x82784974 = freq_hi_f32 = float(4294967296 / hz)     ≈ 85.9 s per 2^32 ticks
//
// NOTE: The song clock is NOT a mftb reader. It lives in Beatmatch::Update (sub_823123D0)
// reading this+40 (float seconds, driven by MIDI event timing).
// Timer is a PROPERTY-SYSTEM construct (not a Scheduler Object) — polled via hmx_Timer_SlotInit.
#define hmx_Time_FreqInit          sub_82271910   // boot-time: reads PPC timebase hz, computes freq_lo/hi_f32 constants → 0x82784970/74 HIGH
#define hmx_Time_TicksToSeconds    sub_82271840   // convert accumulated ticks to float seconds; formula: lo*freq_lo + hi*freq_hi HIGH
#define hmx_Time_SpinWait_us       sub_822717B8   // raw mftb busy-wait; r3=microseconds; used only in controller detect HIGH
#define hmx_Timer_Unregister       sub_82272408   // 8-insn: clears bit 0 of 0x82784870 (registration guard) HIGH

// ---- Xbox notification dispatch sub-handlers (called from hmx_Xbox_PollNotifications) ----
#define hmx_Joypad_ScanControllers sub_82272F98   // 4-port bitmask poll; this+24=new, this+28=changed
#define hmx_Timer_SlotInit         sub_82273090   // DataArray prop init for timer slot
#define hmx_Timer_GetByName        sub_822731B0   // gets timer by interned symbol
#define hmx_Timer_Register         sub_82272240   // DECODED 2026-05-28: boot-time class reg only (NOT per-frame tick); property-system timer; global guard at 0x82784870 HIGH
#define hmx_EventLog_Append        sub_82279FB0   // ring buffer 5 slots at 0x82270000+22228

// ---- Song-select trigger chain — DECODED 2026-05-28 ----
// User selects song → SongSelect_Handler → SongLoader_ProcessQueue → RefcountGate → vtable[9] → MidiParser_Load
#define hmx_SongSelect_Handler     sub_8236AA38   // UI/script layer: user picks song → checks state+40/60/64, fires vtable[19] on old target, calls SongLoader HIGH
#define hmx_SongLoader_RefcountGate sub_82376308  // 5-insn refcount gate: payload+48++; when→1: fires payload->vtable[9]() = MidiParser_Load HIGH

// ---- UIWidget sub-helpers (called from hmx_UIWidget_FrameTick) ----
#define hmx_UIWidget_SetFocusScale sub_823118A8   // 9 insns; writes float to inner+40 and +44
#define hmx_UIWidget_AnimTransition sub_8236A718  // DataArray property set for focus transition

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
// DataNode struct — DECODED 2026-05-28 (HIGH conf):
//   struct DataNode { uint32_t payload; uint32_t type; };  // 8 bytes, NO vtable (pure tagged union)
//   payload = int / float bits / char* / Symbol* / DataArray* / Object*  per type tag
//
// Complete tag → runtime type enum (selected important entries):
//   Tag 2  → type 2  (INT)       literal integers, MIDI pitch values
//   Tag 7  → type 16 (Symbol)    compiled-DTB symbol constants
//   Tag 16 → type 6  (DataArray) '{...}' array-begin
//   Tag 17 → type 2  (INT)       hex integer 0x...
//   Tag 19 → type 19 (Symbol)    PRIMARY property name type (all property + class names use this)
//   Tags 0/8/10/12 → NULL        all four are end-of-array markers
//
// DataArray struct: { DataNode* nodes(+0); ...; int16_t count(+8); ... }
// PropertyTable struct: { DataNode* data(+0); int16_t capacity(+8); int16_t flags(+10);
//                         int16_t count(+14); PropertyTable* parent(+16) }
//   count==0: linear scan (pointer equality); count>0: binary search.
#define hmx_DataNode_Resolve       sub_82317EF8   // tagged-union dispatch: tag 2=INT passthru, 17=DeferredRef, 19=SymbolRef→GameState::GetProp, default=self HIGH
#define hmx_DataNode_AsInt         sub_82317FE8   // extract int32 from DataNode HIGH
#define hmx_DataNode_AsFloat       sub_823180E8   // extract float from DataNode HIGH
// CORRECTED 2026-05-28: sub_82318010 is NOT DataNode_GetSuperclassName — it is DataNode_GetObjectPtr
#define hmx_DataNode_GetObjectPtr  sub_82318010   // DECODED: Resolve DataNode → Object* (NOT superclass name) HIGH
#define hmx_DataNode_AsSymbol      sub_823180A8   // extract Symbol (interned string ptr) from DataNode HIGH
// CORRECTED 2026-05-28: sub_82120BD8 is NOT DataNode::TypeName — it is XTL_LibraryName.
// 17-case switch on bits 8-15 of XEX2 import library ID; returns XTL library name string.
// DataNode type system uses sparse integer tags (2=INT, 17=DeferredRef, 19=SymbolRef) — no 0-16 enum.
#define hmx_XTL_LibraryName        sub_82120BD8   // DECODED: XEX import library ID → XTL lib name string (NOT DataNode::TypeName) HIGH
#define hmx_DataArray_Destruct     sub_8229E968   // ? destructor (called by Scheduler for null-obj nodes)

// ---- MILO format functions (HIGH conf) -------------------------------------
//
// Magic: 0xCABEDEAF (standard) / 0xCBBEDEAF (streaming/zlib) /
//        0xCCBEDEAF (3rd variant) / 0xCDBEDEAF (4th, community-only)
// Magic is big-endian in file; byte-swapped on read; stored at MiloLoader+44

#define hmx_MiloLoader_ReadLoop    sub_8235F678   // streaming read loop: byte-swaps magic, compares all 3 variants HIGH
#define hmx_MiloLoader_MagicStd    sub_8235F2D8   // dispatch: 0xCABEDEAF → seek-mode read via vtable[8]
#define hmx_MiloLoader_MagicStream sub_8235F358   // dispatch: 0xCBBEDEAF → sub_82362310 (streaming decompression)
#define hmx_MiloLoader_Decompress  sub_82362310   // streaming MILO decompressor (zlib/lzo path)
#define hmx_MiloPlayerSlot_PosUpdate sub_82362440 // CORRECTED 2026-05-28: NOT a binary reader. 10-insn leaf: decrements 3 PlayerSlot counters: load_count-=1, secondary_val-=r4, stream_pos-=r5. Called on sync-load completion.
#define hmx_MiloStreamBlock_Complete sub_823618B0 // stream completion: lazy pool init (sub_823617B0), then free block to pool via hmx_MiloFreeList_Push HIGH
#define hmx_MiloStreamBlock_Alloc  sub_82361888   // alloc 24B MiloStreamBlock from free-list pool (sub_823617B0 init + sub_82362EB0 pop) HIGH
#define hmx_MiloStreamBlock_Fill   sub_82361950   // 10-insn leaf: fills all 8 fields of MiloStreamBlock (+0/+4/+8/+12 u32, +16/+17/+20 u8, +18 u16 from global load_seq) HIGH
#define hmx_MiloPlayerSlot_Accum   sub_82362788   // 25-insn: increments PlayerSlot load_count/secondary_val/stream_pos; tracks max at +12/+16. Called on block registration. HIGH
#define hmx_MiloHeader_Read        sub_82361A30   // reads outer MILO header: first_block_offset(+0) + block_count(+4) via File_Read(4), then block size table. Actual file-byte reader. HIGH
//
// MILO per-class object dispatch (DECODED 2026-05-28):
// sub_82362ED0 CORRECTED: NOT per-class dispatcher — it is a 5-insn free-list push (pool return).
// The actual per-class Read() dispatch is sub_82364720 (and siblings per object type in 0x82364xxx/0x82366xxx).
// Hmx::Object vtable[3] = Read(BinStream&), vtable[10] = Load/Reload — confirmed from dispatch bodies.
#define hmx_MiloFreeList_Push        sub_82362ED0 // CORRECTED 2026-05-28: NOT per-class dispatcher. 5-insn free-list push: if(r4==0)ret; r4->next=*r3; *r3=r4. Returns 24B block to pool. HIGH
#define hmx_MiloFreeList_Pop         sub_82362EB0 // 5-insn free-list pop: read head, if null ret null, advance head. Companion to Push. Called from MiloStreamBlock_Alloc. HIGH
#define hmx_MiloObj_ClassRegDispatch sub_82364720 // THE MILO per-class Read() dispatcher: lazy-intern type+name strings, ClassReg_Lookup_v3, dispatch vtable[10](re-load) or vtable[3](Read). HIGH
#define hmx_MiloObj_ClassRegDispatch2 sub_823660A8 // Second per-class dispatcher (same pattern, different XEX-rodata type string offset). HIGH
#define hmx_MiloTypeStr_Intern_A     sub_82362EF0 // Lazy type-string intern helper A: one-time String_CopyOrIntern from lis(-32256)+2988; init flag at lis(-32127)+15780. HIGH
#define hmx_MiloTypeStr_Intern_B     sub_82362F78 // Lazy type-string intern helper B: init flag at lis(-32127)+15788. Same pattern. HIGH

// ---- Symbols / strings (HIGH conf) ---------------------------------------

#define hmx_String_CopyOrIntern    sub_82355DA8   // intern → Symbol
#define hmx_String_HashMod         sub_82691050   // (str, mod) hash

// ---- Base Object property names (HIGH conf — extracted from sub_82316428 body) ---
//
// These 20 strings are passed to hmx_String_CopyOrIntern inside
// Object::HandleProperty. They are the universal properties every Sandbox
// object responds to (the first 20 of 59 branch labels; remaining 39 are
// interstitial connectors):
//
//   get, get_array, size, set, insert, remove, has, prop_handle, copy,
//   replace, class_name, name, note, set_note, iterate_refs, dir,
//   set_name, set_type, is_a, get_type

// ---- Memory / generic primitives (HIGH conf) -----------------------------

#define hmx_Mem_Alloc              sub_82354FD8   // malloc
#define hmx_Mem_Alloc_Inner        sub_82354930   // 32-byte slab pool + buddy-split free-list; large(>128B)→raw heap; min alloc 8B HIGH
#define hmx_Mem_Alloc_Guarded      sub_82354AB0   // (size, tag_str, flags) — mutex-guarded wrapper of Mem_Alloc_Inner
#define hmx_Mem_Free               sub_82355020   // (size,ptr): size>128→FreeLarge, size<=128→FreeSmall HIGH
#define hmx_Mem_Realloc            sub_82354FF8   // (size,ptr): size>128→Alloc_Inner, size<=128→FreeSmall arena HIGH
#define hmx_Mem_FreeLarge          sub_82354B48   // large free (>128B), acquires spinlock HIGH
#define hmx_Mem_FreeSmall          sub_823567C0   // small fixed-size arena free HIGH
#define hmx_Mem_FindFreeBlock      sub_823539C8   // finds free 32-byte slab slot from pool index table HIGH
#define hmx_Mem_BuddySplit         sub_823541C0   // buddy-split: inserts remainder back into free-list (4 sub-paths based on this+28 tag) HIGH
#define hmx_memset                 sub_8239CD50   // memset shim
#define hmx_EventLog_Append        sub_82279FB0   // ring buffer 5-slot at 0x82270000+22228; (r3,r4) stored as 12-byte record HIGH

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
#define hmx_FileOps_OpenAndParse   sub_8231C520   // path -> ParserObj; no DTB magic — pure token stream; ARK-active? then crypto-init first HIGH
#define hmx_File_OpenDispatch      sub_822767F8   // file-open dispatcher: path→canonical→extension flags→stream ctor→vtable open HIGH
#define hmx_FileHandle_ctor        sub_8227DC18   // FileHandle ctor: +0=vtable, +4=open_flags, +8/+12/+16=file-pos/size/arc-idx via ARK_PathResolve, +40=compression bits HIGH
#define hmx_ArkRead_LowLevel_C     sub_8227DAC8   // ? companion stream open variant (MED)

// ---- ARK file system path resolution (HIGH conf — decoded 2026-05-28) ----
//
// sub_82275E90 splits "arkfile.hdr:internal/path" at the colon.
// sub_82276480 takes the components, hash-queries the in-memory ARK index,
// and outputs {file_offset, size, ?, arc_index} to 4 caller-supplied pointers.
// The ARK index itself is loaded from the .hdr file during hmx_App_LoadBootAssets.

#define hmx_ARK_PathSplitAtColon   sub_82275E90   // scans path until ':' (chr 58); returns split ptr HIGH
#define hmx_ARK_PathNormalizeSlash sub_82275D68   // copies path into 256-byte global buf, normalizes \ to /  HIGH
#define hmx_ARK_PathResolve        sub_82276480   // full ARK index query: 3× path-hash → outputs {off,size,?,arc-idx} to 4 ptrs HIGH
#define hmx_ARK_NullCallback       sub_82276048   // fires parse-complete callback with empty data (null-path fallback) MED

// ---- DTB (DataTree Binary) parser (HIGH conf — decoded 2026-05-28) ------
//
// DTB format has NO file-level magic. It is a pure stream of type-tagged tokens.
// The file may be crypto-initialized first (for encrypted .dtx), then parsed.
// Token dispatch table has 256 entries; tag 0/8/10/12=end, 16=array, 19-22=strings.

#define hmx_DTB_ParseDriver        sub_8231C270   // stream driver: follows linked-list cursor, calls ParseArenaInit HIGH
#define hmx_DTB_ParseArenaInit     sub_8231C1A8   // alloc parse arena; main parse loop calling DTB_ReadNode until 0; restores state after HIGH
#define hmx_DTB_ReadNode           sub_8231B5D0   // 576-insn node-by-node decoder; first call reads type-tag byte HIGH
#define hmx_DTB_StreamLexer        sub_8232F458   // 328-insn state-machine lexer; 256-entry dispatch table at r27+240; 16384-byte read buffer HIGH
#define hmx_DTB_SetCurrentFile     sub_82319E60   // stores interned filepath ptr at global -4264 (current-parse-file context) MED

// ---- String management (HIGH conf — decoded 2026-05-28) -----------------
#define hmx_String_Assign          sub_82353618   // hmx_String::operator=(src): strlen+reserve+memcpy HIGH
#define hmx_String_Reserve         sub_82352CB8   // ensures capacity (this+12); realloc + memmove if needed HIGH
#define hmx_MidiFile_PathCtor      sub_82352C68   // MidiFile path member ctor: vtable=0x82261090, +4=char*, +8=flags, +12=cap, +16=interned HIGH
#define hmx_MidiFile_PathAssign    sub_82353920   // canonicalize via ARK_PathResolve then String_Assign HIGH

// ---- Type system inheritance (HIGH conf — decoded 2026-05-28) -----------
//
// sub_8231C660 walks the superclasses PropertyTable chain recursively.
// Each superclass slot is 8 bytes; count is int16 at PropertyTable+8.

#define hmx_TypeSystem_SuperclassLookup sub_8231C660  // recursive superclass walker; calls DataNode_GetObjectPtr (sub_82318010) per slot HIGH

// ---- Asset loaders by file extension (MEDIUM conf, pinned via file_ext --
//      stack-sample hook; bodies decoded partially) ---------------------

#define hmx_MidiParser_Load        sub_822D59C0   // .mid -> chart event list ⭐ (opens file + registers callbacks; note parsing via callbacks below)
#define hmx_MidiFile_ctor          sub_822D5828   // MidiFile object ctor (sets vtable, calls sub_82352C68+sub_82353920 for path/filename)
#define hmx_File_OpenAndRead       sub_8235A458   // open + read file + dispatch per-event callbacks (called from MidiParser::Load)
#define hmx_MidiParser_EventTranslate sub_823769E8 // MIDI text events -> HandleProperty("load",{proj_file,file}) messages ⭐ HIGH (3 strings: load/proj_file/file)
#define hmx_MidiTrack_DispatchEvent sub_82346B30  // outer event router: mode=1 direct, mode=2→NoteRangeFilter; chains vtable[4] listener HIGH
#define hmx_MidiTrack_NoteRangeFilter sub_82345608 // filters by [+20]..[+24] pitch range → NoteOn/NoteOff HIGH
#define hmx_MidiTrack_NoteAccumulator sub_8232FD90 // pairs note-on/off: pitch*4 slot table at [+8]; produces duration HIGH
#define hmx_MidiTrack_NoteOn       sub_82344EA8   // allocates NoteRecord(tick/pitch/vel/name), inserts into sorted NoteList HIGH
#define hmx_MidiTrack_NoteOff      sub_82344828   // finalizes duration, writes completed note to output buffer HIGH
#define hmx_NoteRecord_ctor        sub_82344910   // ? allocates + fills per-note record struct
#define hmx_NoteList_Insert        sub_82341E18   // ? sorted insert of NoteRecord into NoteList
#define hmx_SongLoader_ProcessQueue sub_82378D28  // walks pending load list, gates each via TriggerLoad ref-count HIGH
#define hmx_SongLoader_TriggerLoad  sub_82376308  // increments [this+48] ref-count; calls vtable[9] on 0->1 transition HIGH
#define hmx_MiloLoader_Load        sub_8235F4B0   // .milo_xbox scene/asset bundle ctor+load ⭐ (magic 0xCABEDEAF std / 0xCBBEDEAF stream)
#define hmx_MiloLoader_StreamBase_ctor sub_82359130  // 3-field base ctor: vtable=0x8205C4F8, +4=byte, +8=0
#define hmx_MiloLoader_String_ctor sub_823536E0   // Hmx::String member ctor + filename assign (at MiloLoader+16)
#define hmx_MiloLoader_ReadDispatch sub_82358888  // (this,?,filename,buf,?,?,?,bool) — file read dispatcher, calls sub_82360FE8
#define hmx_MiloLoader_BinaryRead  sub_82360FE8   // THE actual MILO binary parser (8 args; needs decode)
#define hmx_MiloScene_Poll         sub_82323CA8   // timed scene iterator / async loader lifecycle (NOT file parser)
#define hmx_Mem_Lock_Acquire       sub_82353C40   // scoped mutex acquire (used by Mem_Alloc_Guarded)
#define hmx_Mem_Lock_Release       sub_82353CC8   // scoped mutex release
#define hmx_FaceAnim_Load          sub_8214E030   // .fac face / viseme data
#define hmx_VocalTrack_Load        sub_821915D8   // .voc vocal audio
#define hmx_MoggDecoder_Open       sub_826820D8   // .mogg OGG/XMA dispatch; format flag at this+96 (0=OGG, nonzero=XMA) HIGH
#define hmx_MoggStream_Open        sub_82681E08   // per-channel stream opener (264B OGG alloc, 104B XMA alloc) HIGH
#define hmx_MoggDecoder_Init       sub_82682558   // 5-channel bringup + XA9 volume init HIGH
#define hmx_MoggDecoder_Register   sub_82682338   // registers MoggDecoder with sound system MED
#define hmx_MoggStream_Start       sub_826822C8   // sets this+97=started, calls XA9 Submit+Resume HIGH
#define hmx_OggDecoder_Decode      sub_82681188   // Vorbis core decode (228 insns); state struct ≥5816B HIGH
#define hmx_XA9_SetVolume          sub_826A9480   // crowd stem volume hook: r7=channel_ptr, f1=volume*master_vol HIGH
#define hmx_TextureLoader_Load     sub_821C8A30   // ? .bmp_xbox texture
#define hmx_ShaderLoader_Load      sub_82307530   // ? .fx_xbox shader / effect
#define hmx_ParticleSys_Load       sub_82307F30   // .dtx particle config (also ParticleSys::RegisterFactory)
// NOTE: hmx_File_OpenAndRead (sub_8235A458) is the generic callback-open helper — defined above in FILE SYSTEM section

// ===========================================================================
//                       SUBSYSTEM REGISTERS (boot-time init)
// ===========================================================================
//
// Each *_Register is the immediate caller of hmx_ClassReg_Lookup for that
// subsystem's class name at boot. Pinned via Phase 4b stack-sample hook.
// Confidence MEDIUM unless promoted by reading the function body.

// ---- Gameplay-mechanic subsystems ----
//
// Beatmatch vtable: 0x82046C94; vtable[15] @ 0x82046CD0 → sub_823123D0 (Beatmatch::Update) CORRECTED 2026-05-28
// Beatmatcher vtable: 0x820568CC; vtable[15] → sub_82338A48 (Beatmatcher::Update) DECODED 2026-05-28
//
// Per-frame chain (Scheduler::Walk step 7):
//   sub_823123D0 (Beatmatch::Update, vtable[15])
//     → sub_8231AE98 → sub_822CD180 (SongEventDispatch, 602 insns)
//       → sub_82377300 (MIDI event walk to playhead, 406 insns)
//       → for each Beatmatcher: sub_82338A48 (Beatmatcher::Update, vtable[15])
//           → sub_82330E90 (CalcHitWindow → this+28..+44)
//           → gem→vtable[2](hit_window) for each gem (Gem::CheckHit)
//           → sub_82338778 (fire gem_pass_callback for elapsed gems)
//           → sub_82338820 (advance gem queue to current tick)
//           → sub_82338940 (sustain + streak + SP phrase advance)
//
// sub_822CF2E0 is NOT vtable[15] — it is an alternative NoteTracker update path.
// Scoring+StarPower is NOT Scheduler-driven — pure callback system via vtable[16] dispatch.
//
// BoostMeter is a plain 10-float struct at Scoring+36..+72, no vtable.
// CORRECTED 2026-05-28 (from scoring.dtb decode): field names were previously wrong.
//   Scoring+36  (+0)  recharge_rate   = 0.0   (no auto-recharge)
//   Scoring+40  (+4)  deploy_rate     = 0.125  (gauge drain per sec while active)
//   Scoring+44  (+8)  star_phrase     = 0.25   (gauge fill per phrase; 4 phrases = full)
//   Scoring+48  (+12) whammy_rate     = 0.034  (gauge fill per sec from whammy)
//   Scoring+52  (+16) ready_level     = 0.5    (min gauge level to activate SP)
//   Scoring+56  (+20) multiplier      = 2      (int; score × 2 while SP active)
//   Scoring+60  (+24) crowd_boost     = 6.0    (crowd meter boost on SP activation)
//   Scoring+64  (+28) min_length      = 2.0 × scale
//   Scoring+68  (+32) whammy_speed    = 0.05 × scale
//   Scoring+72  (+36) whammy_timeout  = 0.5 × scale
//
// Scoring formula (CONFIRMED from scoring.dtb):
//   per_note = 50 × streak_mult × sp_mult
//   sustain  = 25 per tick held
//   streak thresholds: 10→2×, 20→3×, 30→4× (EXACT)
//   SP multiplier = 2
//   Max = 400 pts/note at streak≥30 + active SP
//
// Timing windows (from beatmatch.dtb — CONFIRMED 2026-05-28):
//   HOPO window = 170 ms
//   hit slop    = ±100 ms

#define hmx_Beatmatch_Register          sub_822D08F0   // class register; vtable 0x82046C94; writes vtable+60=sub_823123D0 HIGH
#define hmx_Beatmatch_Update            sub_823123D0   // vtable[15]: per-frame MIDI dispatch + Beatmatcher iteration HIGH
#define hmx_Beatmatch_SongEventDispatch sub_822CD180   // 602-insn: MIDI event list walk + foreach-Beatmatcher iteration HIGH
#define hmx_Beatmatch_MIDIEventWalk     sub_82377300   // walks MIDI event list to current playhead; fires callbacks (406 insns) HIGH
#define hmx_Beatmatch_GetSongTime       sub_82311960   // reads this+40→ptr+8 as float song position HIGH
#define hmx_Beatmatch_NotifyPlayerTime  sub_8232FC08   // notifies global Beatmatch of player's current time (sync from Beatmatcher) MED
#define hmx_Beatmatch_NoteWindowSetup   sub_822CEAB0   // one-time init: registers timing callbacks, sets window size HIGH
#define hmx_Beatmatch_NoteTrackerInit   sub_822CEA18   // per-player NoteTracker init + HOPO threshold via sub_822BC818 HIGH
#define hmx_Beatmatch_CalcHitWindow     sub_82330E90   // (stack_buf, this+8, dt): song time→ticks, writes 5 timing values HIGH
#define hmx_Beatmatch_NoteTrackerUpdate sub_822CF2E0   // iterates NoteTracker vector at beatmatch+132 (NOT vtable[15]) MED

// CORRECTION 2026-05-28: Beatmatcher ctor body read directly. Vtable = 0x820D68CC.
// Zeros this+8..+16 (3 u32). Gem queue is a doubly-linked list at this+20 (fwd=bck=self initially).
// this+12 was previously incorrectly listed as gem_vec_ptr — that was wrong.
// The linked list at this+20 is the true gem queue walked per-frame by Beatmatcher::Update.
#define hmx_Beatmatcher_Register        sub_82330038   // Beatmatcher ctor+register; vtable 0x820D68CC; gem list sentinel at this+20 HIGH
#define hmx_Beatmatcher_Update          sub_82338A48   // vtable[15]: song-active check, hit-window calc, gem iterate, gem_pass fire HIGH
#define hmx_Beatmatcher_GemPassFire     sub_82338778   // fires gem_pass_callback for any gem whose time has elapsed HIGH
#define hmx_Beatmatcher_AdvanceGemQueue sub_82338820   // advances gem queue read pointer to int_song_time HIGH
#define hmx_Beatmatcher_AdvanceState    sub_82338940   // sustain + streak timeout + SP phrase tracking HIGH
#define hmx_Beatmatcher_ConfigInit_Full sub_8233CF08   // 136-insn 12-prop table init (hopo_threshold etc.) HIGH
#define hmx_Beatmatcher_ConfigInit      sub_823341D0   // config entry point; calls DataNode_AsInt for timing window HIGH
#define hmx_Beatmatcher_Factory         sub_82338C78   // alloc + ctor + ClassReg register MED
#define hmx_Beatmatcher_LazyAlloc       sub_82338518   // lazy-init gem queue buffer via hmx_Mem_Alloc MED
#define hmx_Beatmatcher_SetTimingFields sub_8233D310   // 2-field setter: writes this+40 and this+48 MED
#define hmx_Beatmatcher_SongStateCheck  sub_823347B8   // checks this+48+this+140; branches to sub_822F7878 MED

// NoteList → Beatmatcher gem queue bridge (decoded 2026-05-28)
//
// Full chain: MidiTrack_NoteOn → NoteList_Insert → GemSet_Init → NoteList_to_GemQueue → Gem_Init → GemQueue_Insert
//
// GemNode struct (24 bytes, linked list node at Beatmatcher+20):
//   +0  fwd ptr (doubly-linked list)
//   +4  bck ptr
//   +8  note_idx (NoteList index)
//   +12 lane (init=-1 sentinel)
//   +16 tick start
//   +20 duration ticks
//
// GemRender struct (56 bytes, per-fret visual object in GemSet):
//   +0  ClassReg ptr
//   +4  lane index
//   +8  note index
//   +12 child_a ptr (sub_82150688)
//   +16 child_b ptr (sub_82140908)
//   +20 child_c ptr (sub_82140908)
//   +44 status byte = 0
//   +48 y_pos float = 0.0 (updated per-frame by TickToY)
//   +52 Beatmatcher* ref
//
// CORRECTION: sub_8229EA50 was labeled hmx_BeatmatcherAPI_GetCurrentTick — WRONG.
// It is an observer/listener ctor. NOT a tick query. Renamed below.

// CORRECTION 2026-05-28: sub_8229EA50 is NOT a tick query. It is BeatmatcherObserver_Ctor:
// writes vtable 0x820C6B4C to dest+0, stores object ref at dest+4, parent ref at dest+8,
// calls sub_82317480 to link the observer if parent is non-null. Called from GemSet_Init
// to register a Beatmatcher observer at GemSet+256.
#define hmx_BeatmatcherObserver_Ctor    sub_8229EA50   // observer/listener ctor: vtable=0x820C6B4C, stores obj+parent refs, links via sub_82317480 HIGH
// NOTE: the old name hmx_BeatmatcherAPI_GetCurrentTick for sub_8229EA50 was WRONG — delete it.

#define hmx_NoteList_to_GemQueue        sub_8269AB80   // bridge: NoteList*→per-fret GemNode linked list; 5 lanes; also allocs 2x152B GemAnim HIGH
#define hmx_GemRender_Init              sub_8269D110   // init 56-byte GemRender: ClassReg*(+0),lane(+4),note_idx(+8),children(+12/16/20),y_pos(+48)=0.0,Beatmatcher*(+52) HIGH
#define hmx_GemQueue_Insert             sub_8269AAB8   // insert GemNode into Beatmatcher doubly-linked list at Beatmatcher+20 HIGH

#define hmx_GemPass_VtableDispatch      sub_8234A598   // THE gem_pass fire: vtable[16] on callback objects HIGH
#define hmx_NoteTracker_CallbackDispatch sub_8233AE28  // iterates callback list obj+44..+56 HIGH
#define hmx_NoteTracker_FireCallbacks   sub_822E20D0   // 2-insn: (tracker+44, song_time) → CallbackDispatch HIGH
//
// Per-note vtable[16] handlers (called by hmx_GemPass_VtableDispatch on each gem pass):
//   r3 = callback object ptr, f1 = song_time float
//   Scoring callback obj: vtable 0x82047CD4 (ctor sub_822D58A8), vtable[16] at 0x82047D14
//   StarPower callback obj: vtable 0x82047B0C (ctor sub_822D4F68), vtable[16] at 0x82047B4C
#define hmx_Scoring_GemPassHandler      sub_822D5944   // scoring vtable[16]: called on each gem pass; r3=Scoring callback obj, f1=song_time HIGH
#define hmx_StarPower_GemPassHandler    sub_822D5024   // starpower vtable[16]: called on each gem pass; r3=SP callback obj, f1=song_time HIGH
#define hmx_Scoring_CallbackCtor        sub_822D58A8   // ctor for Scoring callback obj: writes vtable 0x82047CD4 at obj+0, sub-vtable 0x82047C94 at obj+4 HIGH
#define hmx_StarPower_CallbackCtor      sub_822D4F68   // ctor for StarPower callback obj: writes vtable 0x82047B0C at obj+0, sub-vtable 0x82047ACC at obj+4 HIGH
#define hmx_HOPO_WindowInstall          sub_822BC818   // HOPO/hit-window config; r5=4 for 4 lanes MED
#define hmx_HOPO_LaneStateInit          sub_822C9618   // per-lane HOPO state init MED
#define hmx_BeatmatchSink_SetField16    sub_82699E60   // 2-insn: stw r4, 16(r3) HIGH
#define hmx_Vector_PushBack8B           sub_822D6950   // push_back (8-byte elements); calls GrowAndInsert on overflow HIGH
#define hmx_Vector_GrowAndInsert        sub_822D6710   // grow: realloc + copy/move + update begin/cur/end ptrs HIGH
#define hmx_Note_ConfigInit        sub_822DE640   // ? note_weight config
#define hmx_TrackConfig_Init       sub_822CCA60   // TrackConfig: 5-bit dirty-flag state machine; drives gem visuals (fret/tail/color/SP/beat); starved flag at +89 HIGH
#define hmx_TrackMapping_Ctor      sub_82346FA8   // TrackMapping constructor: writes fixed lane-index table then reads track_mapping/drum_style_instruments/vocal_style_instruments/watcher HIGH
#define hmx_TrackGraphics_Ctor     sub_82692BA8   // TrackGraphics ctor: class:track_graphics confirmed; vtable 0x820D6D20 HIGH
#define hmx_TrackGraphics_Update   sub_8236C4C0   // vtable[15]=0x820D6D5C; per-frame: reads track_graphics props, calls UIWidget_FrameTick + child scheduler HIGH
#define hmx_TrackWidget_PropertyDispatch sub_82693A20 // 343-insn dirty-flag router: 13 bits, each dispatches gem visual update HIGH
#define hmx_TrackWidget_AnimTempo  sub_822B3BB8   // anim_tempo accessor: reads 1 prop from this+604 sub-table, writes to this+60 HIGH
#define hmx_TrackWidget_GetScrollPosition sub_822CA5D0 // returns current highway Y scroll position float MED
#define hmx_TrackWidget_SetScrollFactor sub_822CB2B0  // sets highway stretch factor for scroll transform MED
#define hmx_GemHighway_Init        sub_826967B8   // 450-insn highway init: allocs mesh, sets up scroll transform + note window HIGH
// CORRECTION 2026-05-28: GemSet_Init does NOT "call BeatmatcherAPI" (that was the old wrong label).
// It calls BeatmatcherObserver_Ctor (sub_8229EA50) to register an observer at GemSet+256,
// then calls NoteList_to_GemQueue (sub_8269AB80) to bridge NoteList to the GemNode linked list.
// Visibility bounds computed by TickToY×2, stored at GemSet+240/+244.
// 3 scroll config floats from DTB stored at GemSet+272/+276/+280.
#define hmx_GemSet_Init            sub_82699538   // GemSet ctor: GemRow_Init×5 + BeatmatcherObserver_Ctor + NoteList_to_GemQueue + TickToY×2 HIGH
#define hmx_GemRow_Init            sub_8269C438   // per-fret init (5 calls); calls Gem_RenderInit HIGH
#define hmx_GemHighway_CalcVisWindow sub_82699D18 // computes ±lookahead tick bounds for gem visibility HIGH
#define hmx_Gem_RenderInit         sub_826A0270   // 353-insn full gem render data init; sparkle_len at this+116; GemRenderData layout decoded HIGH
#define hmx_GemSparkle_Emit        sub_82696560   // fires sparkle particle for fret N; called from dirty-bit 0x400 path (1 frame after gem hit) HIGH
#define hmx_Gem_SetVisibility      sub_826924C8   // fret active/inactive toggle (dirty-bit 0x001) MED
#define hmx_Gem_SetType            sub_82692548   // gem shape update (dirty-bit 0x002) MED
#define hmx_Gem_SetTailLength      sub_826925C8   // sustain tail length update (dirty-bit 0x004) MED
#define hmx_Gem_SetTailVisibility  sub_82692648   // tail show/hide (dirty-bit 0x008) MED
#define hmx_Gem_SetColor           sub_826926B8   // color/mat swap (dirty-bit 0x010) MED
#define hmx_Gem_SetStarPower       sub_826927C0   // star-power glow transition (dirty-bit 0x800) MED
#define hmx_Scoring_Register       sub_822D69C8   // Scoring+StarPower Register; NOT vtable-bearing; pure callback system HIGH
#define hmx_Scoring_Factory        sub_822D6C90   // Scoring object factory: alloc 80-byte Scoring struct + fill int configs at +0..+16 HIGH
#define hmx_Scoring_GetSingleton   sub_822D6250   // returns Scoring singleton ptr (shared by per-gem callback chain) HIGH
#define hmx_BoostMeter_Init        sub_822D5FC0   // 10 float config reads: plain struct at Scoring+36..+72; reads from scoring.dtb HIGH
#define hmx_StarPower_AudioFXVolume sub_822E44B0  // ? star_power_fx_volume init

// ---- Visual / character subsystems ----

// ---- Lighting subsystem (HIGH conf — decoded 2026-05-28) ----------------
//
// LightPreset::Update (vtable[15]=sub_821BCAE8) is the per-frame keyframe evaluator.
// Keyframes are 64 bytes each, loaded at song-start by LightPreset::Read.
// MIDI fires HandleProperty messages that advance the keyframe cursor.
// Light struct has 4 RGB channels at +96, +160, +224, +288 (optional).
//
// LightPreset vtable at 0x8201B430; vtable[15] = 0x8201B46C → sub_821BCAE8

#define hmx_Light_Register         sub_821BBA20   // Light+LightPreset class register (interns both names) HIGH
#define hmx_LightPreset_ctor       sub_821BB3F0   // LightPreset ctor: sets vtable 0x8201B430 HIGH
#define hmx_LightPreset_Update     sub_821BCAE8   // vtable[15] per-frame keyframe eval; r3=this, r4=Light out, r5=force HIGH
#define hmx_LightPreset_EvalAllTracks sub_821C5F60 // force-eval all 64-byte keyframe blocks; scales by global intensity HIGH
#define hmx_LightPreset_ScaleColors sub_821BCC80  // multiplies all 4 Light RGB channels by scalar f1 HIGH
#define hmx_LightPreset_Read       sub_821C2B60   // loads keyframe data from .milo stream at song-start (788 insns) MED
#define hmx_KeyframeNode_Eval      sub_821AF988   // dirty-flag gated; 5 interp modes: 0=step,1=nearest,2=cubic,3-4=vtable[0] HIGH
#define hmx_Vec3SplineInterpolate  sub_8214F950   // cubic Hermite via 3-float control points at +0/+4/+8/+16/+20/+24/+32/+36/+40/+48 HIGH
#define hmx_Vec3ClampMinOf         sub_8226AC18   // copies 4 floats (flag=1) or per-component min with tolerance (flag=0) HIGH
#define hmx_KeyframeArray_Resize   sub_821BCE48   // each entry 64B; inits RGB=0.0, intensity=1.0 HIGH
#define hmx_KeyframeBatch_Load     sub_821BCF90   // loops over 64-byte array calling single-keyframe loader per entry HIGH
#define hmx_Keyframe_Read          sub_821BC118   // single keyframe: version-conditional (+56/+60 for ver 10-22); reads time/value/tangent HIGH

// ---- ParticleSystem / VFX (HIGH conf — decoded 2026-05-28) ------------------
//
// CORRECTION: sub_82307530 was labeled hmx_ShaderLoader_Load and guessed as
// hmx_ParticleSys_Spawn. Correct: hmx_RndEffect_LoadConfig. Loads vtx+pix GPU
// shader pair for one of 12 particle effect slots (NOT a raw particle spawn).
//
// Particle emitter chain (GemSparkle hit → queue):
//   hmx_GemSparkle_Emit (sub_82696560)
//     → hmx_GemFretEmitter_GetLaneY (sub_82699988) — world-space Y for fret N
//     → hmx_ParticleEmitter_Emit (sub_821E8DB0) — alloc+init 108B Particle, queue
//         → hmx_Particle_Init (sub_821E84D0) — vtable + list nodes + Y range + vel
//         → hmx_ParticleQueue_Insert (sub_82314420) — insert into global queue
//
// GemRenderData+76 = ParticleSysAnim* for the GEM SPARKLE (NOT firebird).
// Firebird lives in Guitar+56 (Vector<GuitarChildRef>, child named "firebird").
// SP activation walks Guitar+56 by name and calls vtable[Enable] on "firebird".
//
// 108-byte Particle struct:
//   +0   vtable (= lis(-32254)+5844)
//   +4   list_vtable (= lis(-32254)+6028); self-link at +8, next at +12
//   +16  attach sub-obj vtable
//   +28  vel_vtable (= lis(-32254)+6044); emitter_back at +32, next at +36
//   +76  u8 active (= 0)
//   +80..+103  f32 start_Y/end_Y/vel_lo/vel_hi (see crowd_camera_vfx.md)
//   +104 u8 flag (= 0 for sparkle)
//
// GemSparkle struct:
//   +88  particle_sys handle
//   +112 fret_emitter_ptrs[5] (stride 4; slot = fret-2)
//   +116 sparkle_len float (from DTB "sparkle_len")
//
// Guitar+56 = Vector<GuitarChildRef> (16-byte elements).

// NOTE: hmx_ShaderLoader_Load label (sub_82307530) is REPLACED by hmx_RndEffect_LoadConfig.
#define hmx_RndEffect_LoadConfig    sub_82307530   // load vtx+pix shader pair for 1/12 particle effect slots; r3=this r4=slot r5=config r6=trigger HIGH
#define hmx_ParticleSys_Load        sub_82307F30   // load all 12 effect slots from particles.dtx; 12-loop calls hmx_RndEffect_LoadConfig HIGH
#define hmx_ParticleSys_RegisterFactory sub_82307F30 // alias
#define hmx_ParticleSys_SlotLookup  sub_8268D028   // (slot_int) → slot ptr from 4B LUT at lis(-32137)+7672 HIGH
#define hmx_ParticleSys_BuildPath   sub_8268D130   // ARK path builder for effect asset; calls hmx_ARK_PathResolve HIGH
#define hmx_ParticleSys_ReadConfig  sub_8268D1D0   // DTB property reader vtx/pix shader name (mode 0=vtx 1=pix) HIGH
#define hmx_ParticleSys_BindMaterial sub_8268D280  // binds material/texture to particle geometry MED
#define hmx_ParticleSys_AllocSlots  sub_8268D898   // allocates particle slot pair storage MED
#define hmx_RndEffect_Spawn         sub_82306A38   // actual effect instance spawn (trigger=1 path in hmx_RndEffect_LoadConfig) HIGH
#define hmx_ParticleEmitter_Emit    sub_821E8DB0   // alloc 108B Particle, init physics range [f1,f2], insert into queue HIGH
#define hmx_Particle_Init           sub_821E84D0   // init 108B Particle: vtable+list+Y_range+vel±f3 HIGH
#define hmx_ParticleQueue_Insert    sub_82314420   // insert initialized Particle into global queue with fade params HIGH
#define hmx_GemFretEmitter_GetLaneY sub_82699988   // world-space Y float for fret N; reads array at GemSparkle+16 HIGH
#define hmx_GemAnims_FindParticleSysAnim  sub_8269F338  // PropertyTable walk → ParticleSysAnim* (= GemRenderData+76 gem sparkle) HIGH
#define hmx_GemAnims_FindParticleSysAnim2 sub_8269F3E0  // same, second ParticleSysAnim (= GemRenderData+84) MED
#define hmx_GuitarChildVec_PushBack sub_822A62B0   // push 16B GuitarChildRef into Guitar+56 vector HIGH
#define hmx_GuitarChildVec_Grow     sub_822A60F0   // grow Guitar+56 vector on overflow MED
#define hmx_GuitarChildRef_Pack     sub_822AE698   // pack {obj_ptr, name_key, flags} into 16B GuitarChildRef HIGH

#define hmx_Character_VFXInit      sub_8232E9F8   // ? hand_flames VFX setup (property name CORRECTED 2026-05-29; was flame_hands)
#define hmx_Gem_RenderInit         sub_826A0270   // ? sparkle_len config
// ---- Character subsystem (HIGH conf — decoded 2026-05-28/29) ---------------
//
// CORRECTED 2026-05-29 (full XEX rodata decode):
//
// Character (top-level Sandbox class):
//   vtable base: 0x82038624  (16-slot large Sandbox vtable)
//   vtable[15] @ 0x82038660 = 0xFFFFFFFF  → NOT scheduled; Character has NO Update
//   ClassReg::Register is at slot 11 (offset 44) for all top-level Sandbox classes
//
// CharXxx animation sub-objects use a SMALLER 16-slot vtable format:
//   ClassReg::Register is at slot 9 (offset 36) for CharXxx classes
//   CharIK instance vtable: 0x82009764
//     slot 0  = ctor            (sub_8214A768)
//     slot 9  = ClassReg::Reg  (sub_82315150)
//     slot 15 = CharIK::Update  (sub_8214CD88)
//
// 7 register functions in recomp.1.cpp (sub_821456E8..sub_8215F3C0 range):
//   sub_821456E8 → BandSlider   (UI slider widget)
//   sub_821467F8 → LayerDir     (layer directory)
//   sub_8214A980 → BandLabel    (UI label widget)  ← PREVIOUSLY MISLABELED as CharIK_Register
//   sub_821294D8 → Trans        (transform node, actually in recomp.0.cpp)
//   sub_822870A0 → Character    (top-level character class, actually in recomp.11.cpp)
//   sub_8215D9B8 → OutfitLoader (outfit/costume loader)
//   sub_8215F3C0 → CharBones    (skeletal bone set)
//
// IK target stored at CharIK+112/116/120

#define hmx_Character_ClassReg_ctor sub_822870A0  // writes vtable 0x82038624; ClassReg singleton at 0x82805C40; in recomp.11.cpp HIGH
#define hmx_Character_BindProps    sub_822A7DD8   // 59-insn property binder: PropertyTable_Find0 ×4 + CopyOrIntern ×3 HIGH
#define hmx_Characters_ParentRegister sub_822A88C8 // parent: calls OGG audio init (sub_8212DD00) then 4 sub-registers HIGH
#define hmx_Character_Register     sub_822A7EC8   // top-level Character class register HIGH
// CORRECTED: sub_8214A980 is BandLabel_Register, NOT CharIK_Register
#define hmx_BandLabel_Register     sub_8214A980   // BandLabel UI widget class register (guard 0x82782000; class name "BandLabel" at 0x82009708) HIGH
#define hmx_BandSlider_Register    sub_821456E8   // BandSlider UI widget class register HIGH
#define hmx_LayerDir_Register      sub_821467F8   // LayerDir layer directory class register HIGH
#define hmx_OutfitLoader_Register  sub_8215D9B8   // OutfitLoader costume loader class register HIGH
#define hmx_CharBones_Register     sub_8215F3C0   // CharBones skeletal bone set class register HIGH
#define hmx_CharIKHand_Register    sub_82174160   // CharIKHand class register; ClassReg singleton at 0x827824D4 HIGH
#define hmx_CharIK_HandleProperty  sub_8214B400   // 533-insn, 45 labels; writes IK target to this+112/116/120 HIGH
#define hmx_CharIK_Update          sub_8214CD88   // CharIK vtable[15] (0x82009764+60=0x820097A0); 87 insns; transform helper + 2-bone solver → this+112/116/120 HIGH
#define hmx_CharIK_2BoneSolver     sub_8214CBA8   // 96-insn IK math solver HIGH
#define hmx_CharIKHand_VtableCopy  sub_8214D500   // CharIKHand vtable-copy ctor; reads ClassReg singleton @ 0x827820B8 HIGH
// CORRECTED 2026-05-28: GemRenderData+76..+92 are ParticleSysAnim* for gem sparkle variants — NOT firebird.
// Firebird lives in Guitar+56 (Vector<GuitarChildRef>). SP activation walks Guitar+56 by name to enable "firebird".
#define hmx_Guitar_Construct       sub_822A7B30   // constructs Guitar: populates Guitar+56 (Vector<GuitarChildRef>) with "firebird"+"hand_flames"+other named SP children HIGH (property name CORRECTED 2026-05-29)
#define hmx_Venue_Register         sub_822A75D0   // ? venues class register
#define hmx_DefaultBand_Register   sub_822B7670   // ? default_band register

// ---- Audio subsystems ----

// Hmx::Synth singleton — step 2 of per-frame loop (ptr @ 0x8280E274) ⭐ HIGH
#define hmx_Synth_ClassReg_ctor    sub_822E5938   // Synth ClassReg ctor (interns class name from 0x8204A8B8)
#define hmx_Synth_FrameTick        sub_822E5A48   // Synth per-frame flush: pops sound queue + walks 0x82809A7C list
#define hmx_Synth_HandleProperty   sub_822E5B88   // Synth property dispatcher
#define hmx_Synth_Destructor       sub_822E5C90   // Synth dtor: final flush, nulls 0x8280E274
#define hmx_Synth_SoundQueue_Walk  sub_822F9050   // walks sound-event list @ 0x82809A7C, calls vtable[4] on each
#define hmx_Synth_Register         sub_822E9BC0   // ? synth class register (may be same or companion to ClassReg_ctor)
#define hmx_Sound_Register         sub_822B1E58   // ? sound class register
// WorldCrowd — decoded 2026-05-28
// NOTE: system has 4 tiers (0-3) with 3 float thresholds — NOT 5 tiers. Correction from earlier doc.
// The 5 .mogg files (0intro..4good) are audio assets; gameplay rating value tops out at 3.
#define hmx_WorldCrowd_Ctor        sub_822C9E80   // outer WorldCrowd ctor; chains to sub_822C9720 HIGH
#define hmx_WorldCrowd_CtorInner   sub_822C9720   // inner ctor; writes vtable 0x8204579C to this+0; also sets this+4=0x82045894, this+132=0x8204588C (sub-object vtables); 320-byte alloc; called from outer ctor (822C9E80) and crowd factory (822DE970). Conflict resolved 2026-05-29: Agent B claim "BandStarMeterDir_ctor" is WRONG — no star-meter calls in body; BandStarMeterDir ctor is elsewhere (near sub_822C80F8). HIGH
#define hmx_WorldCrowd_Update      sub_822CC848   // vtable[15] best candidate (71 insns, 11 labels); reads this+156/+160 (render-dims), calls sub_82376330 + sub_823763D8 HIGH
#define hmx_WorldCrowd_TierEval    sub_822CAAB0   // 4-tier classifier: fcmpu x4 → returns 0-3; reads both player crowd ratings via vtable[6] through this+176 HIGH
#define hmx_Crowd_FindProp         sub_822D6320   // crowd property accessor: reads "crowd" sub-table from this+32/this+76 (NOT a class register; 29 insns, thin) HIGH
// CrowdAudio subsystem — DECODED 2026-05-28
//
// CrowdAudio (168 bytes, at WorldCrowd+836) drives the 5 crowd MOGG stems.
// NOT Scheduler-driven. Updates flow: WorldCrowd::Update (vtable[15])
//   → hmx_CrowdAudio_PollStemChannels (if tier changed)
//   → hmx_CrowdAudio_SetTier → AssignStemVolumes → hmx_XA9_SetVolume.
//
// CrowdAudio struct key offsets:
//   +0   vtable (lis -32251; addi -27212)
//   +40  float: init/master volume (lis -32256 + 2480 static)
//   +48  float: crowd_audio volume scalar
//   +52  float: current effective volume
//   +56  u8: enabled   +57  u8: has_reactions   +58  u8: armed
//   +64  u32: max_tier = 4
//   +88/+92/+96: DataNode ptrs: intro / encore_intro / levels
//   +132/+136: reactions vector begin/end
//   +156/+160: stem channel vector begin/end (5 entries, stride 16)
//
// Tier→volume: ((tier+14)*2 + clamp)*4 offset into struct → DataNode float.
// SP boost: StarPower_CrowdBoost_Init (sub_822E44B0) reads crowd_boost float
//   from BoostMeter config and multiplies into the tier volume pre-XA9.
//
// hmx_XA9_SetVolume confirmed:
//   int(XA9Chan* r3, Track* r4, int r5, int r6, void* r7, float f1)
//   RtlEnterCriticalSection → voice→vtable[32](...f1) = IXAudio2SourceVoice::SetVolume
//   f1 = tier_vol * master_vol (pre-multiplied by caller)
#define hmx_CrowdAudio_Ctor             sub_822E0928   // 168B ctor; 7 props; calls SetTierAndUpdate at end HIGH
#define hmx_CrowdAudio_SetTierAndUpdate sub_822DF9D8   // (CrowdAudio*, bool enabled): enabled→+56; 5-stem loop; SetTier HIGH
#define hmx_CrowdAudio_SetTier          sub_822DF590   // (CrowdAudio*, int tier 0-3): index→ResolveTierVolume→AssignStemVolumes→XA9 HIGH
#define hmx_CrowdAudio_PollStemChannels sub_822DF480   // WorldCrowd::Update→this; loop stride16 over +156..+160; vtable[11] per stem HIGH
#define hmx_WorldCrowd_WalkReactions    sub_822CB1E0   // iterate WorldCrowd+844..+848 reaction vec; vtable[1] per entry; returns bool HIGH
#define hmx_CrowdAudio_ResolveTierVolume sub_822DF350  // read levels DataArray at +96, indexed by tier → float MED
#define hmx_CrowdAudio_AssignStemVolumes sub_822DEF30  // assign vol floats to all 5 MOGG crowd channels MED
#define hmx_CrowdAudio_CommitStemVolumes sub_82312240  // commit resolved tier volumes to stem channel array MED
#define hmx_CrowdAudio_XA9VolumeCommit  sub_82314420   // final XA9 commit; audio singleton at this+(-5056) MED
#define hmx_CrowdAudio_Mute             sub_822DF0E8   // mute all 5 crowd stems when enabled=0 MED
#define hmx_SongMgr_GetCrowdState       sub_823257C0   // song-state query for crowd audio arming MED
#define hmx_CrowdStem_SetChannelVolume  sub_823262C8   // per-stem volume dispatch (string_key, tier_ptr) MED
#define hmx_StarPower_CrowdBoost_Init   sub_822E44B0   // SP crowd boost setup; stores scalar at +72; calls ApplyBoost HIGH
#define hmx_SP_GetBoostVolume           sub_826910A0   // returns SP-active crowd boost volume scalar MED
#define hmx_CrowdAudio_ApplyBoost       sub_8233CC78   // applies SP boost scalar r6 to crowd audio MED

// Camera system — decoded 2026-05-28
// GH2 uses INTERPOLATED camera blending (quaternion slerp + matrix interp), NOT hard cuts.
#define hmx_CamShot_Ctor           sub_822E9870   // CamShot ctor; writes vtable 0x8204B18C; also calls sub_8232D1D0 x2 for blend/enabled props HIGH
#define hmx_CamShot_Update         sub_822F5A70   // vtable[15] best candidate (157 insns, 8 labels); reads this+0/+4; calls sub_8226F5B8 + sub_822F7F78 HIGH
#define hmx_CamShot_Blend          sub_822F6B58   // camera blend (232 insns, 44 labels); sub_821A6ED8 x3=quat slerp, sub_821A6F38 x3=matrix interp; reads/writes this+0..+12 HIGH
// CORRECTED 2026-05-28: sub_82317958 = PropertyTable::Insert (BST insert via sub_823178A0 + AVL rebalance via sub_823174E8)
// Called 9× from Camera_SystemCtor to insert camera config DataNodes.
#define hmx_PropertyTable_Insert   sub_82317958   // BST insert DataNode* into sorted PropertyTable; returns DataNode* at new_node+20 HIGH
#define hmx_Camera_SystemCtor      sub_822EA138   // camera system ctor; calls PropertyTable_Insert ×9 for camera props; allocates 76-byte shot table at this+52 HIGH
#define hmx_Quat_Slerp             sub_821A6ED8   // quaternion slerp (rotation blend); called 3x from CamShot_Blend HIGH
#define hmx_Matrix_Interp          sub_821A6F38   // matrix interpolation (position blend); called 3x from CamShot_Blend HIGH

// ---- HUD / UI subsystems — DECODED 2026-05-28 ----
//
// Score/Streak/Multiplier use a property-notification system — NOT a monolithic struct.
// When gameplay updates score: SetProperty('score', new_int) → BandScoreDisplay::HandleProperty
// → sub_82126DC0(this-56) re-renders digit meshes.
// Multiplier: 'multiplier' property on BandMultiplierDisplay → 20-prop HandleProperty.
// Star meter: float via DataHandler 'score' at BandStarMeterDir+164.
// Star thresholds: 3 tiers (burn/star/bonus) from track_graphics PropertyTable — NOT hardcoded.
#define hmx_HUD_Register                sub_82690A00   // hud class init HIGH
#define hmx_UI_Register                 sub_8236D1F0   // UI class init; min_splash_time HIGH
#define hmx_FocusAnim_Constants         sub_82124C08   // reads focus_anim_duration from ui/button.milo → 0x827403D4 HIGH
#define hmx_BandButton_ColorResolve     sub_82122920   // per-button RGBA from DTB × (1/255.0); writes 3-float RGB out HIGH
#define hmx_BandScoreDisplay_HandleProp sub_8212C918   // 154 insns, 14 labels; 5 props: score/display_meshs/digit_mats/blank_mat/player HIGH
#define hmx_BandScoreDisplay_UpdateDigits sub_82126DC0 // re-renders score digit meshes from current score int HIGH
#define hmx_BandMultiplierDisplay_HandleProp sub_8212EF20 // 504 insns, 48 labels; 20 props incl. multiplier/led_mats/blown_mesh HIGH
#define hmx_BandStarMeterDir_HandleProp sub_822C80F8   // star_meter activate/deactivate dispatcher HIGH
#define hmx_StarRating_Render           sub_8269C968   // 3-tier threshold constructor: burn/star/bonus from track_graphics PropertyTable HIGH
// NOTE: hmx_StarPower_LoadConfig = hmx_BoostMeter_Init = sub_822D5FC0 (same function, two aliases; defined above near Scoring section)
// CORRECTED 2026-05-29: sub_8214A980 = hmx_BandLabel_Register (defined in character section above).
// The old hmx_BootScreen_StarsAnim label for this address was wrong. The real boot-screen anim sub is unknown.
#define hmx_Locale_Register             sub_82356F30   // i18n: DataArray → per-locale parsed DTB map HIGH
#define hmx_XTL_LibraryName             sub_82120BD8   // DECODED: XEX import library ID → XTL lib name (NOT DataNode::TypeName) HIGH

// ---- GPU render pipeline — SURVEYED 2026-05-28 ----
//
// 4 major passes + ~7 compositor sub-passes per frame:
//   Pass 0 (PreRender vtable+64): begin-scene, clear, default state
//   Pass 1 (DrawViewport→SceneCompositor): camera viewport quad, depth ON/OFF
//   Pass 2 (Render vtable+80): all 3-D world geometry (venue/chars/gems), depth ON
//   Pass 3 (RenderOverlay vtable+84): 2-D HUD quads, one quad per element, no depth
//   Pass 4 (SceneRoot_Render): opaque batch (mode 20) + transparent batch (mode 768)
// Vertex stride: 16B (XY+UV+ARGB8 packed) for 2D; scene uses variable stride.
// Gems: NO dedicated GemHighway::Render — gems are in TrackGraphics scene graph,
//       submitted via SceneRoot_Render transparent batch (mode 768).
#define hmx_GPU_DrawPrimitive       sub_823C7478   // r4=prim-type(2=strip,3=list,6=quad,20=opq,768=xp), r5=count, r6=vtx, r7=stride HIGH
#define hmx_GPU_DrawBatch           sub_823C2770   // scene-graph batch draw; r4=mode(20=opaque/768=transparent) HIGH
#define hmx_GPU_SetDepthWrite       sub_823BEC10   // 5 insns: r4=0→test off(1024), r4!=0→on(1087); dirty bits 32+128 HIGH
#define hmx_RndState_SetRasterMode  sub_823CBF70   // stores r4 at this+12296, calls SetVertexType, ORs 0x80000 into dirty flags HIGH
#define hmx_SceneRoot_Render        sub_821E4288   // scene-root draw: iterates renderable list, calls DrawBatch per mode HIGH
#define hmx_SceneCompositor         sub_82300F20   // viewport compositor: camera+rect→full-screen quad, uses rasterMode 322 HIGH
#define hmx_GPU_Flush               sub_823D2520   // end-of-frame GPU flush HIGH

// ---- Input subsystems (FULLY DECODED 2026-05-28) ----
//
// Full per-frame pipeline:
//   hmx_Input_FrameTick (sub_82271228)
//     → hmx_GuitarInput_Poll (sub_8227CC98)            [guard at 0x82788000+22416]
//         [per-port loop, up to 8 ports × 48B table at 0x82788000+22424]
//         → hmx_GuitarPort_Poll (sub_8227B538)
//             → hmx_XamInputGetState (sub_823B5B68)     [XamInputGetState trampoline]
//             → hmx_GuitarPort_RemapButtons (sub_8227B368)
//                  [XInput wButtons u16 → engine lane u32 via 16-entry table at 0x82247000+26416]
//             → hmx_GuitarPort_RemapAnalog (sub_8227B3F0) [×4: strum/whammy/tilt axes]
//         → hmx_Input_ExtractState (sub_8227C690)        [build typed snapshot]
//
// Joypad object (904 bytes at JoypadConfig+56 when enabled):
//   +0  vtable  +40  controller_idx=-1  +44  JoypadConfig* parent
//   +52/+56  float thresholds/scale     +64  8×104B per-port slots  +896  u8 ready_flag
//   Per-port slot: +0=24(max_entries), +8/+12=strum thresholds, +16=button-map ptr
//                  +48/+52=whammy, +72=tilt float
//
// Button press → gem hit chain:
//   hmx_Beatmatch_PlayerUpdate (sub_822CDEB8)
//     → hmx_Beatmatcher_TryHit (sub_822CFA70)            ← BUTTON-PRESS GATE ⭐
//         → hmx_Beatmatch_InputEventPump (sub_822CEC60)   [reads ro_guitar_xbox snapshot]
//         → hmx_Beatmatcher_CheckAnyPress (sub_822CEB70)  [NoteTracker list; any press?]
//         → hmx_Beatmatcher_Update (sub_82338A48)         [gem CheckHit loop]
//         → hmx_NoteTracker_TryHit (sub_822E2510)         [per-tracker hit attempt]
//
// TrackMapping lane-index table (fixed before DTB reads in ctor):
//   green=5  red=7  yellow=9  blue=36  orange=96
//   strum_enable@+56=1  invalid@+64=-1
// TrackMapping 4 DTB props: "track_mapping" (top-level), "drum_style_instruments" (cond.),
//   per-fret sub-props (green/red/yellow/blue/orange), strum_map, watcher

#define hmx_Joypad_Register             sub_8227B4A0   // ? joypad class register
// NOTE: sub_8236C4C0 = hmx_TrackGraphics_Update (CORRECTED 2026-05-28; NOT JoypadConfig_Init)
#define hmx_JoypadConfig_SetJoypadMode  sub_8236A338   // HIGH; 47 insns; enable: alloc 904B+init+register+bind(0xF000); disable: vtable[0]+null
#define hmx_JoypadObj_Init              sub_8227B1F8   // HIGH; 904B Joypad object ctor; 8×104B per-port slots at +64; +896=ready_flag; r4=JoypadConfig*→+44
#define hmx_JoypadObj_Register          sub_8227A238   // MEDIUM; registers Joypad with input system; r4=1
#define hmx_JoypadObj_BindMask          sub_8227A220   // MEDIUM; binds button mask 0xF000 (dpad+shoulder+start+back)
#define hmx_GuitarInput_Poll            sub_8227CC98   // HIGH; per-frame guitar/controller poller; guard at 0x82788000+22416; 8-port loop
#define hmx_GuitarPort_Poll             sub_8227B538   // HIGH; per-port poll; r3=port_obj r4..r9=output ptrs; calls XamInputGetState+RemapButtons
#define hmx_XamInputGetState            sub_823B5B68   // HIGH; 3-insn trampoline: __imp__XamInputGetState(controller_idx, 0, &XINPUT_STATE)
#define hmx_GuitarPort_RemapButtons     sub_8227B368   // HIGH; XInput wButtons u16 → engine lane u32 via 16-entry table; strum_up→bit0, strum_dn→bit1
#define hmx_GuitarPort_RemapAnalog      sub_8227B3F0   // HIGH; int16 XInput axis → float; scale at 0x82247000+26484; called ×4 for strum/whammy/tilt
#define hmx_Input_ExtractState          sub_8227C690   // HIGH; builds typed input snapshot from remapped button/axis data; called once per port
#define hmx_Input_GetProperty           sub_8227C540   // MEDIUM; property accessor for input class

// ---- Button press → gem hit pipeline (DECODED 2026-05-28) ----
//
// Critical finding: Beatmatcher_Update (gem CheckHit loop) is ONLY called when
// hmx_Beatmatcher_CheckAnyPress returns nonzero. The strum/button press is the gate.
// Port must replicate: poll XInput → remap → set press flag → TryHit gates on that flag.

#define hmx_Beatmatch_PlayerUpdate      sub_822CDEB8   // HIGH; per-player per-frame; calls TryHit (once normally, twice on SP activation)
#define hmx_Beatmatcher_TryHit          sub_822CFA70   // HIGH ⭐ THE button-press→gem-hit gate; disabled(+128)/started(+129) checks; CheckAnyPress; Beatmatcher_Update; NoteTracker_TryHit
#define hmx_Beatmatch_InputEventPump    sub_822CEC60   // MEDIUM; reads ro_guitar_xbox into Beatmatch player state; top of TryHit
#define hmx_Beatmatcher_CheckAnyPress   sub_822CEB70   // HIGH; iterates NoteTracker vec at this+132..+136; returns 1 if any HasPendingPress
#define hmx_NoteTracker_HasPendingPress sub_822E1F18   // MEDIUM; per-NoteTracker: returns u8 bool press-this-frame
#define hmx_NoteTracker_GetTime         sub_822E1F28   // MEDIUM; returns song_time float f1 for NoteTracker current state
#define hmx_NoteTracker_TryHit          sub_822E2510   // MEDIUM; per-NoteTracker hit attempt; r3=NoteTracker*, f1=song_time
#define hmx_Beatmatch_SPActivate        sub_822CB260   // MEDIUM; star-power activation; triggers second TryHit call in PlayerUpdate

// ---- SP hand_flames dispatch chain (MEDIUM conf — decoded 2026-05-29 from full-song trace) ----
//
// When SP activates, the property "hand_flames" (NOT "flame_hands" — name confirmed via
// PropertyTable_Find0 hook) is dispatched to character sub-objects via HandleProperty.
//
// Call chain (outermost → innermost dispatcher):
//   hmx_SP_OutcomeEvaluator (sub_822C9498)         ← entry from Beatmatch pipeline
//     hmx_SP_StateHandler (sub_822C8D40)
//       hmx_SP_EffectDispatch (sub_822DD3E0)
//         hmx_SP_EffectInner (sub_822DD108)
//           hmx_Beatmatch_SPCallback (sub_822CB5B8)  ← near SPActivate (sub_822CB260)
//             hmx_ObjectTree_Propagate (sub_82325DA8) ×2
//               hmx_ObjectTree_Walk (sub_823267B8) ×2
//                 hmx_Object_PollChildren (sub_821B9E40) ×5
//                   hmx_Char_InnerDispatch (sub_82126568)
//                     hmx_Char_SubDispatch (sub_82152108)
//                       hmx_Object_MsgReDispatch (sub_823216E0)
//                         hmx_Object_HandleProperty (sub_82316428)
//                           → dispatches "hand_flames" to character sub-objects
//
// All new addresses below: MEDIUM confidence (call chain observed; bodies not yet read).

#define hmx_SP_OutcomeEvaluator     sub_822C9498   // ? SP state chain outermost; entry from Beatmatch pipeline for hand_flames dispatch
#define hmx_SP_StateHandler         sub_822C8D40   // ? called by SP_OutcomeEvaluator
#define hmx_SP_EffectDispatch       sub_822DD3E0   // ? called by SP_StateHandler
#define hmx_SP_EffectInner          sub_822DD108   // ? called by SP_EffectDispatch
#define hmx_Beatmatch_SPCallback    sub_822CB5B8   // ? SP callback near SPActivate (sub_822CB260); fires on SP activation
#define hmx_ObjectTree_Propagate    sub_82325DA8   // ? object-tree message propagator (appears 2× in SP chain)
#define hmx_ObjectTree_Walk         sub_823267B8   // ? object-tree walker (appears 2× alongside Propagate)
#define hmx_Object_PollChildren     sub_821B9E40   // ? child iterator / recursive poll (appears 5× in SP chain)
#define hmx_Char_SubDispatch        sub_82152108   // ? character sub-object handler (between PollChildren and HandleProperty in SP chain)
#define hmx_Char_InnerDispatch      sub_82126568   // ? one level inside Char_SubDispatch
#define hmx_Object_MsgReDispatch    sub_823216E0   // ? message re-dispatcher between HandleProperty layers

// ---- Game-state / engine subsystems ----

#define hmx_GameState_GetProp      sub_82315440   // ? gameplay-state property reader
// hmx_TypeSystem_SuperclassLookup (sub_8231C660) defined above in FILE SYSTEM / type-system section (HIGH conf)
#define hmx_Context_Register       sub_822E1058   // ? contexts class register
// CORRECTED 2026-05-28: sub_822A3910 is NOT Campaign-specific. It is the generic ClassReg
// base-class initializer used by ALL Sandbox classes. Interns 4-5 class names, does 2-4
// ClassReg lookups, stores PropertyTable ptrs at this+0/+4/+8/+12. Tail-calls Campaign::Init proper.
#define hmx_ClassReg_BaseInit      sub_822A3910   // generic ClassReg base-class initializer (used by Campaign, Character, etc.) HIGH
// ---- Campaign / song-select flow (DECODED 2026-05-28) ----
//
// Song-start trigger chain:
//   [UI selects song] → hmx_Campaign_StartSong (sub_8229FC00)
//     → hmx_Campaign_IsSongCurrent (sub_8229E8E0)  [walks song list at Campaign+64]
//     → hmx_Campaign_SetCurrentSong (sub_8229E6E8)  [stores active flag at Campaign+69]
//     → hmx_Campaign_PreloadHUD (sub_8229FB20)       [difficulty/practice setup]
//     → hmx_SongLoader_StartLoad (sub_82379308)
//         sets flag at 0x8281410C (load-in-progress)
//         fires Object::HandleProperty("scene start") on load list
//         → hmx_SongLoader_ProcessQueue (sub_82378D28)
//             → hmx_SongLoader_TriggerLoad (sub_82376308) per entry
//                 vtable[9](entry) → hmx_MidiParser_Load for .mid
//                                  → hmx_MiloLoader_Load for .milo_xbox
//
// Scene state IDs: 0=IDLE 1=NORMAL 2/18/19=MENU 3=LOADING(practice) 7=PRACTICE
//                  11=SPLASH 13=RESULT 17=SONG_SELECT→SONG (reads next scene from obj+16)
// Campaign song list: doubly-linked list at Campaign+64. Current-song active flag at Campaign+69.
// Load-in-progress global: 0x8281410C.
#define hmx_Campaign_StartSong      sub_8229FC00   // top-level song start: IsSongCurrent+SetCurrentSong+PreloadHUD+SongLoader_StartLoad HIGH
#define hmx_Campaign_IsSongCurrent  sub_8229E8E0   // walks song list at Campaign+64; returns bool MED
#define hmx_Campaign_SetCurrentSong sub_8229E6E8   // stores active-song flag at Campaign+69 HIGH
#define hmx_Campaign_PreloadHUD     sub_8229FB20   // difficulty/practice HUD pre-init before song load MED
#define hmx_SongLoader_StartLoad    sub_82379308   // sets load flag 0x8281410C; fires "scene start"; calls ProcessQueue HIGH
#define hmx_SceneTransition_Dispatch  sub_82308098 // full scene transition function; reads scene type + game-mode HIGH
#define hmx_SceneTransition_DecodeEvent sub_82305EB8 // reads this+41 (scene-type override), App+204 (game-mode byte), scene ptrs at this+16/+20 HIGH

#define hmx_Achievements_Register  sub_8229D910   // ? achievements class register
#define hmx_Leaderboards_Register  sub_822D6EE0   // ? leaderboards class register
#define hmx_Store_Register         sub_822A7DD8   // ? store class register
#define hmx_Tips_Register          sub_822825E8   // ? tips class register
#define hmx_Locale_Register        sub_82356F30   // ? locale class register
#define hmx_LongCheats_Register    sub_8235C610   // ? long_cheats class register
#define hmx_Math_TrigLUT_Build     sub_8226DDF0   // DECODED 2026-05-28: NOT a class register. Trig LUT builder: fills 2x256-entry float tables at 0x827449B8 (sin+cos + delta tables); sentinel at r28+2044 HIGH
#define hmx_Math_SinCos            sub_8239DDC0   // sin/cos impl; called once per entry by TrigLUT_Build HIGH
#define hmx_Mem_Register           sub_82355048   // ? mem class register
// hmx_Timer_Register (sub_82272240) defined above in per-frame-ticks section (HIGH conf)
// DECODED 2026-05-28: sub_821E68E0 = hmx_Game_RegisterDataNodes — THE big class-factory sweep.
// This is Game_PostInit step 3. ALL engine subsystem class factories are registered here:
// Beatmatch, Scoring, CharIK, LightPreset, TrackWidget, Beatmatcher, etc.
// Port MUST call this (or equivalent) before any singleton instantiation.
#define hmx_Game_RegisterDataNodes sub_821E68E0   // ALL class factories registered; called as PostInit step 3 HIGH
#define hmx_Rnd_SetVideoMode       sub_823032C8   // CORRECTED (was misidentified as hang site): XGetVideoMode; sets screen rects at this+252/+504 HIGH
#define hmx_App_FirePostInitCallbacks sub_821E4A80 // walks this+172/+160 callback lists; fires vtable[0](r4=1) on each — boot-complete signal HIGH
#define hmx_FileMgr_SetCallback    sub_82275810   // Game_PostInit step 1: sets global logger/FileMgr sink at 0x82344BDC HIGH
#define hmx_StringTable_Init       sub_82358018   // Game_PostInit step 2: interns string into global table at 0x82784B40 HIGH
#define hmx_FileLoader_SetCallback sub_82272430   // Game_PostInit step 4: installs load-complete callback into FileLoader at 0x82784958 HIGH
#define hmx_ParticleSys_IsRegistered sub_82270CE8 // check particle factory guard (0x82784870); returns bool HIGH
#define hmx_ParticleSys_RegisterFactory sub_82307F30 // conditional: register particle factory if not already done HIGH
#define hmx_Beatmatch_SingletonInit sub_8230FB18  // Game_PostInit step 19: Beatmatch singleton instantiation → stored at 0x827FEB48 MED
#define hmx_ShaderList_Install     sub_823082C0   // reads vector at 0x827FEA5C; pads/trims shader slot list to 12 entries MED
#define hmx_Rnd_CreateRenderTarget sub_82302830   // checks this+236/+672; calls sub_823C6CD0 on first init MED

// ---- Result / boot UI ----

// NOTE: sub_8214A980 was labeled hmx_BootScreen_StarsAnim here — WRONG.
// It is hmx_BandLabel_Register (defined in character section). Remove this alias.
// Real boot-screen star animation sub is not yet decoded.
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
#define hmx_SongLoad_InProgress_addr   0x8281410Cu   // u32 flag: set by SongLoader_StartLoad; cleared when queue drains
#define hmx_Beatmatch_Singleton_addr   0x827FEB48u   // Beatmatch* singleton ptr; set by Game_PostInit step 19
#define hmx_Scheduler_Singleton_addr   0x827FEC40u   // Scheduler* singleton; 4 SchedList[0..3] each 16B
#define hmx_MILO_vtable_addr           0x82038BFCu   // MILO object vtable (set by MiloLoader_Load)
