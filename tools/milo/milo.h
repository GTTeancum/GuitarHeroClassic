// milo.h - Harmonix .milo container reader (structural pass).
//
// A milo file is a compressed block-structured container holding a tree of
// game objects: meshes, textures, materials, transforms, character data,
// scripts, etc. This reader handles the outer container (compression +
// block layout), inflates payload via miniz, and parses the post-
// decompression object directory enough to enumerate child objects by
// (type, name) -- the structural pass. Deep parsing of individual object
// classes (Mesh / Tex / BandCharacter / ...) is deliberately out of scope
// here and is its own follow-up.
//
// Format reference: PikminGuts92/Mackiloha (MIT), local at
// third_party/Mackiloha/Src/Core/Mackiloha/Milo/MiloFile.cs and
// IO/Serializers/MiloObjectDirSerializer.cs.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gh::milo {

enum class BlockStructure : uint32_t {
    NONE   = 0,
    GZIP   = 1,
    MILO_A = 0xCABEDEAFu,  // structured, no compression (RBN)
    MILO_B = 0xCBBEDEAFu,  // structured, ZLIB blocks   (GH1/GH2/GH80s/RB1/RB2/...)
    MILO_C = 0xCCBEDEAFu,  // structured, GZIP blocks   (Amp, KR1-3)
    MILO_D = 0xCDBEDEAFu,  // structured, ZLIB with 4-byte prefix per block (RB3, DC)
};

const char* block_structure_name(BlockStructure s);

struct Header {
    BlockStructure structure = BlockStructure::NONE;
    uint32_t       first_block_offset = 0;
    uint32_t       block_count = 0;
    uint32_t       max_block_uncompressed_size = 0;
    std::vector<uint32_t> block_sizes;  // size on disk (high byte flags stripped for MILO_D)
};

struct Entry {
    std::string type;       // e.g. "Mesh", "Tex", "Mat", "Trans"
    std::string name;       // e.g. "head.tex"
    uint64_t    offset = 0; // start in the decompressed payload (after directory header)
    uint64_t    size = 0;   // bytes (computed by scanning forward to next 0xADDEADDE)
};

struct Directory {
    int32_t     dir_version = 0;     // milo "magic" version: 10 (GH1), 24 (GH2), 25 (RB1), ...
    std::string dir_type;            // e.g. "ObjectDir"
    std::string dir_name;            // e.g. "chartest"
    std::vector<Entry> entries;
};

// Parse the container header only (no decompression).
Header parse_header(const std::vector<uint8_t>& bytes);

// Inflate all blocks and concatenate. For BlockStructure::MILO_B this uses
// raw DEFLATE (no zlib wrapper). For MILO_C it uses GZIP. For MILO_D the
// first 4 bytes of each block are an uncompressed-size prefix before the
// deflate stream. For MILO_A it's just concatenation.
std::vector<uint8_t> inflate_payload(const std::vector<uint8_t>& bytes,
                                     const Header& header);

// Parse the post-decompression object directory. Reads dir version, type,
// name, entry list, and computes each entry's byte size by scanning for
// the 0xADDEADDE marker that terminates every object.
Directory parse_directory(const std::vector<uint8_t>& payload);

// File helper.
std::vector<uint8_t> read_file(const std::string& path);

}  // namespace gh::milo
