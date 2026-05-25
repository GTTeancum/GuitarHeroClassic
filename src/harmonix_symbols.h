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
// `recomp_symbols.md` (HIGH / MEDIUM / LOW). Only HIGH-confidence names live
// here so a code reader can trust them; MEDIUM/LOW guesses stay as raw
// sub_xxx in code (with their guessed Harmonix name in a comment) until we
// verify.
//
// Naming: hmx_<Class>_<Method> for member functions, hmx_<Subsys>_<Func>
// for free functions, hmx_<NAME>_addr for data globals. Mirrors Harmonix's
// own Hmx namespace usage.

#pragma once

// ---- Boot path (HIGH conf) ------------------------------------------------

#define hmx_main_xstart            sub_82120B58
#define hmx_main_ProgramInit       sub_82120208
#define hmx_App_EngineInit         sub_82271618
#define hmx_App_LoadBootAssets     sub_82271070

// ---- FileSystem / IO (HIGH conf) ------------------------------------------

#define hmx_FileMgr_Lookup         sub_82277878   // path -> (off, size, ...)
#define hmx_File_ctor              sub_82357A10   // File(path)
#define hmx_File_IsMissing         sub_82357D40   // returns u8 *(File+36)
#define hmx_File_Read              sub_82359250   // (File*, buf, count)
#define hmx_File_InitCrypto        sub_823596E8   // first-4-bytes-as-seed
#define hmx_FileOps_OpenAndParse   sub_8231C520   // path -> ParserObj

// ---- DataNode handler registry (HIGH conf) --------------------------------
//
// Linked list at hmx_HandlerList_head_addr. Each node: +0 = next, +8 =
// payload, payload+12 = name string. Used to look up the runtime's
// developer-view screen handlers ('time', 'rate', 'heap', 'stats', 'input',
// 'camera', etc.).

#define hmx_DataHandler_Find       sub_821E04B8

// ---- Class / Property registry (HIGH conf) --------------------------------
//
// Two-level lookup: hmx_ClassReg_Lookup(class_symbol) returns a per-class
// PropertyTable; hmx_PropertyTable_Find(table, prop_symbol) returns the
// property entry. PropertyTable struct:
//   +0 : data array (16 slots)
//   +8 : i16 capacity (at +8) and flags (at +10)
//   +12: i16 count (at +14)
//   +16: chained parent-class PropertyTable

#define hmx_ClassReg_Lookup        sub_82270D20
#define hmx_PropertyTable_Find     sub_82319448
#define hmx_PropertyTable_Find0    sub_82319530   // wrapper, sets r5=0

// ---- DataNode primitives (HIGH conf) --------------------------------------

#define hmx_String_HashMod         sub_82691050   // hash(str, mod)
#define hmx_String_CopyOrIntern    sub_82355DA8   // copy into target buffer

// ---- Memory (HIGH conf) ---------------------------------------------------

#define hmx_Mem_Alloc              sub_82354FD8
#define hmx_memset                 sub_8239CD50

// ---- Data globals (HIGH conf) ---------------------------------------------

#define hmx_HandlerList_head_addr      0x82782E3Cu
#define hmx_GlobalClassTable_addr      0x8278492Cu
#define hmx_File_vtable_addr           0x8205C014u
#define hmx_FileHandle_vtable_addr     0x8200ED5Cu
