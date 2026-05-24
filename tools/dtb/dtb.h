// dtb.h - Harmonix DTB (compiled DTA) reader.
//
// DTB is a compact binary form of Harmonix's Lisp-like DTA scripting/data
// language. Used across all Harmonix titles (PS2 GH/RB era through 360+).
// This reader covers the "Classic" encoding (the variant used by PS2 GH1 /
// GH2 / GH80s and the Xbox 360 GH2 era).
//
// Format reference: PikminGuts92/Mackiloha (MIT), local at
// third_party/Mackiloha/Src/Core/Mackiloha/{DTB,Crypt}.cs. Format itself is
// publicly known across the Harmonix modding community.

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace gh::dtb {

struct Node;  // recursive

// One element in a DTB tree.
//
// Tag categories (per Harmonix DTA reference):
//   - Integer   : tag 0x00, payload s32
//   - Float     : tag 0x01, payload f32
//   - Variable  : tag 0x02, payload string ($foo)
//   - Symbol    : tag 0x05, payload string (bareword)
//   - String    : tag 0x12, payload string ("quoted")
//   - Other strings: tags 0x04,0x06,0x07,0x08,0x09,0x20,0x21,0x22,0x23,0x24
//                   (kDataUnhandled, #ifdef, #else, #endif, #define, #include,
//                    #merge, #ifndef, etc.). Payload string in all cases.
//   - Array     : tags 0x10 (default "()"), 0x11 (script "{}"), 0x13 (property "[]")
//                   payload is a child node list.
using Atom = std::variant<std::monostate, int32_t, float, std::string,
                          std::vector<std::shared_ptr<Node>>>;

struct Node {
    uint32_t tag = 0;
    Atom    value;
    // Line number metadata, only meaningful for array nodes; preserved for
    // round-trip fidelity to the source DTA.
    uint32_t line = 0;
};

struct Tree {
    bool embedded = false;       // version 0 = embedded (e.g. inside .milo), 1 = standalone
    uint32_t root_line = 0;
    std::vector<std::shared_ptr<Node>> root;
};

// Parse a DTB blob. Auto-detects encryption: if first byte is 0x01 the bytes
// are treated as plaintext; otherwise the first 4 bytes are interpreted as
// the PS2 cipher seed and the rest is decrypted in place.
//
// Throws std::runtime_error on malformed input.
Tree parse(const std::vector<uint8_t>& bytes);

// Render a parsed tree back to human-readable DTA-style text. Useful for
// inspection / diffing.
std::string to_dta(const Tree& tree, bool show_line_numbers = false);

// File helpers.
std::vector<uint8_t> read_file(const std::string& path);
inline Tree parse_file(const std::string& path) { return parse(read_file(path)); }

}  // namespace gh::dtb
