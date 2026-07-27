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
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
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

enum class Storage {
    Plain,
    ZeroPrefixedPlain,
    Encrypted,
};

struct Tree {
    bool embedded = false;       // version 0 = embedded (e.g. inside .milo), 1 = standalone
    uint32_t version = 1;
    uint32_t root_line = 0;
    std::vector<std::shared_ptr<Node>> root;
    Storage storage = Storage::Plain;
    uint32_t cipher_seed = 0;
    // Bytes after the declared root node count. Retail files normally have
    // none, but retaining them makes parse/serialize lossless and exposes any
    // unexplained residual data to audits.
    std::vector<uint8_t> trailing_bytes;
};

// Parse a DTB blob. Auto-detects encryption: if first byte is 0x01 the bytes
// are treated as plaintext. A four-byte zero header followed by 0x01 (used by
// venue .seq files) is also plaintext. Otherwise the first 4 bytes are
// interpreted as the PS2 cipher seed and the rest is decrypted in place.
//
// Throws std::runtime_error on malformed input.
Tree parse(const std::vector<uint8_t>& bytes);

// Compile ordinary DTA text into the same semantic tree used by the DTB
// reader. Supports integer/float/symbol/string/variable atoms, the three
// Harmonix collection forms (), {}, and [], quoted escapes, and semicolon
// line comments. Preprocessor directives remain AST-level input to
// dtb_preprocess and are intentionally not expanded here.
Tree parse_dta(std::string_view text);

// Serialize a tree back to its original plaintext/zero-prefix/encrypted
// storage form. Parsed trees round-trip byte-for-byte.
std::vector<uint8_t> serialize(const Tree& tree);

// Render a parsed tree back to human-readable DTA-style text. Useful for
// inspection / diffing.
std::string to_dta(const Tree& tree, bool show_line_numbers = false);

// File helpers.
std::vector<uint8_t> read_file(const std::string& path);
inline Tree parse_file(const std::string& path) { return parse(read_file(path)); }

// ---------------------------------------------------------------------------
// Query helpers for walking parsed trees (engine-side convenience)
// ---------------------------------------------------------------------------

using NodeList = std::vector<std::shared_ptr<Node>>;

// True if the node is an array form (tag 0x10/0x11/0x13).
bool is_array(const Node& n);

// True if the node is a string form (any of the string-class tags).
bool is_string_like(const Node& n);

// Returns the children of an array node, or empty vector if not an array.
const NodeList& children(const Node& n);

// Extract a value as a basic C++ type when the node's payload matches.
// Returns nullopt if the node isn't of the matching kind.
std::optional<int32_t>     as_int(const Node& n);
std::optional<float>       as_float(const Node& n);
std::optional<std::string> as_string(const Node& n);

// Look inside an array node for a child array whose first child is a symbol
// matching `key`. Convention used throughout Harmonix DTAs:
//   (artist "Skid Row")   -> find_keyed(parent, "artist") returns this array
// Returns nullptr if not found.
std::shared_ptr<Node> find_keyed(const Node& parent, std::string_view key);

// Same, but at the top level of a parsed Tree.
std::shared_ptr<Node> find_keyed(const Tree& tree, std::string_view key);

}  // namespace gh::dtb
