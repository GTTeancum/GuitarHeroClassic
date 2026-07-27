// engine/src/script/preprocess.h
//
// DTA preprocessor pass over a parsed DTB node list. GH2's compiled DTBs
// PRESERVE preprocessor directive nodes (dtb tags 0x07 #ifdef, 0x23 #ifndef,
// 0x08 #else, 0x09 #endif, 0x20 #define, 0x21 #include, 0x22 #merge) rather
// than resolving them at compile time, so the runtime must resolve them before
// interpreting. This produces a clean node list containing no directive nodes:
//   - #ifdef/#ifndef/#else/#endif select branches against a define set,
//   - #define NAME BODY  collects a macro (BODY is the following sibling node)
//     and later bareword references to NAME are replaced by BODY,
//   - #include/#merge FILE splice another DTB's (recursively preprocessed)
//     roots, resolved through a caller-supplied loader (keeps this module free
//     of any ARK dependency).
// Arrays are recursed into, so nested directives resolve too.

#pragma once

#include "dtb_preprocess.h"

namespace ghogx::script {

using NodeList = gh::dtb::NodeList;

// Shared #define macro table: name -> body node. Carried across files so an
// earlier DTB's macros are visible to a later one (ui.dtb defines CHARACTERS
// used by sel_character.dtb, etc.).
using MacroTable = gh::dtb::MacroTable;
using PreprocessOptions = gh::dtb::PreprocessOptions;

// Resolve all directives in `roots`; returns a directive-free node list.
using gh::dtb::preprocess;

}  // namespace ghogx::script
