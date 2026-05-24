// ark_v3.h - Harmonix ARK v3 (PS2-era) reader.
//
// Targets the PS2 GH1, GH2, and GH80s ARKs. v3 is the plaintext index format
// used before the Xbox 360 / RB era introduced HDR encryption (v4+).
//
// Format reference: malictus/arkexpander (Apache-2.0) on GitHub. The format
// itself is publicly documented; this is a clean C++17 implementation, not a
// translation of any specific source file.

#pragma once

#include <cstdint>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace gh::ark {

struct Entry {
    uint32_t ark_part;       // index into ArkV3Reader::ark_part_sizes
    uint64_t offset;         // absolute byte offset within the chosen part
    uint32_t name_idx;
    uint32_t folder_idx;     // 0xFFFFFFFF when at root
    uint32_t size;           // bytes in ARK
    uint32_t inflated_size;  // 0 when uncompressed; otherwise pre-compression size
    std::string name;        // resolved leaf name
    std::string folder;      // resolved folder; empty for root
    std::string full_path;   // folder + "/" + name (or just name)
};

class ArkV3Reader {
public:
    // Parse the given main.hdr. Throws std::runtime_error on any format mismatch.
    static ArkV3Reader load(const std::string& hdr_path);

    uint32_t version() const { return version_; }
    const std::vector<uint64_t>& ark_part_sizes() const { return ark_part_sizes_; }
    const std::vector<Entry>& entries() const { return entries_; }

    // Pull entry bytes from the appropriate ARK part file. ark_paths must have
    // entries for every ark_part; missing slots can be empty if you only need
    // a subset of files. Throws on out-of-range reads or missing path.
    std::vector<uint8_t> read_entry(const Entry& e,
                                    const std::vector<std::string>& ark_paths) const;

    std::optional<Entry> find(std::string_view full_path) const;

private:
    uint32_t version_ = 0;
    uint32_t flag_ = 0;
    std::vector<uint64_t> ark_part_sizes_;
    std::vector<Entry> entries_;
};

}  // namespace gh::ark
