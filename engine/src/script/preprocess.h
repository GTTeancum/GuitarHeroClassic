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

#include "dtb.h"  // gh::dtb::NodeList / Node

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace ghogx::script {

using NodeList = gh::dtb::NodeList;

// Shared #define macro table: name -> body node. Carried across files so an
// earlier DTB's macros are visible to a later one (ui.dtb defines CHARACTERS
// used by sel_character.dtb, etc.).
using MacroTable = std::map<std::string, std::shared_ptr<gh::dtb::Node>>;

struct PreprocessOptions {
  // Names that count as defined for #ifdef (e.g. "HX_EE" for PS2 data).
  std::set<std::string> defines;
  // Resolve an #include/#merge target (the directive's filename payload, e.g.
  // "splash.dta") to that file's RAW parsed roots. Return empty to skip. The
  // returned roots are themselves preprocessed (sharing this pass's macros).
  std::function<NodeList(const std::string& file)> include_resolver;
  // Optional in/out macro table shared across preprocess() calls. When set,
  // macros defined here are visible, and new #defines accumulate back into it.
  MacroTable* macro_table = nullptr;
};

// Resolve all directives in `roots`; returns a directive-free node list.
NodeList preprocess(const NodeList& roots, const PreprocessOptions& opts);

}  // namespace ghogx::script
