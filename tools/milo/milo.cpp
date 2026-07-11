// milo.cpp - see milo.h for format provenance notes.

#include "milo.h"

// Pull tinfl from vendored miniz (MIT) for raw-DEFLATE and zlib/gzip wrappers.
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ARCHIVE_WRITING_APIS
#define MINIZ_NO_TIME
#define MINIZ_NO_STDIO
#include "../../third_party/miniz/miniz_tinfl.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace gh::milo {

namespace {

bool read_u32_at(const std::vector<uint8_t>& p, size_t& pos, uint32_t& out) {
    if (pos + 4 > p.size()) return false;
    std::memcpy(&out, p.data() + pos, 4);
    pos += 4;
    return true;
}

bool skip_string_at(const std::vector<uint8_t>& p, size_t& pos, size_t limit) {
    uint32_t len = 0;
    if (!read_u32_at(p, pos, len)) return false;
    if (len > limit - pos || len > (1u << 20)) return false;
    pos += len;
    return true;
}

bool mesh_body_fits_before(const std::vector<uint8_t>& p, size_t start,
                           size_t end) {
    if (end > p.size() || start + 4 > end) return false;
    size_t pos = start;
    uint32_t tmp = 0;
    if (!read_u32_at(p, pos, tmp)) return false;       // Mesh version.
    if (!read_u32_at(p, pos, tmp)) return false;       // Trans version.
    if (pos + 9 + 96 + 9 > end) return false;
    pos += 9 + 96 + 9;
    if (!skip_string_at(p, pos, end)) return false;    // parent
    if (!read_u32_at(p, pos, tmp)) return false;       // Draw version.
    if (pos + 1 + 20 > end) return false;
    pos += 1 + 20;
    if (!skip_string_at(p, pos, end)) return false;    // material
    if (!skip_string_at(p, pos, end)) return false;    // geometry owner
    if (pos + 9 + 4 > end) return false;
    pos += 9;
    uint32_t vcount = 0;
    if (!read_u32_at(p, pos, vcount)) return false;
    const uint64_t vertex_bytes = static_cast<uint64_t>(vcount) * 48u;
    if (vertex_bytes > end - pos || pos + static_cast<size_t>(vertex_bytes) + 4 > end)
        return false;
    pos += static_cast<size_t>(vertex_bytes);
    uint32_t fcount = 0;
    if (!read_u32_at(p, pos, fcount)) return false;
    const uint64_t index_bytes = static_cast<uint64_t>(fcount) * 6u;
    return index_bytes <= end - pos;
}

}  // namespace

const char* block_structure_name(BlockStructure s) {
    switch (s) {
        case BlockStructure::NONE:   return "NONE";
        case BlockStructure::GZIP:   return "GZIP";
        case BlockStructure::MILO_A: return "MILO_A (uncompressed)";
        case BlockStructure::MILO_B: return "MILO_B (ZLIB, GH1/GH2/GH80s/RB1-2)";
        case BlockStructure::MILO_C: return "MILO_C (GZIP, Amp/KR)";
        case BlockStructure::MILO_D: return "MILO_D (ZLIB+prefix, RB3/DC)";
        default:                     return "<unknown>";
    }
}

namespace {

constexpr uint32_t kAddePadding = 0xDEADDEADu;
// Note: The 4-byte marker on disk is 0xAD 0xDE 0xAD 0xDE little-endian,
// which loads as uint32_t 0xDEADDEAD.

uint32_t rd_u32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
int32_t  rd_i32(const uint8_t* p) { int32_t  v; std::memcpy(&v, p, 4); return v; }

// AwesomeReader-style length-prefixed UTF-8 string.
std::string read_string(const uint8_t* base, size_t end, size_t& pos) {
    if (pos + 4 > end) throw std::runtime_error("milo dir: truncated string length");
    uint32_t len = rd_u32(base + pos); pos += 4;
    if (pos + len > end) {
        std::ostringstream oss;
        oss << "milo dir: string length " << len << " at " << pos << " exceeds payload (" << end << ")";
        throw std::runtime_error(oss.str());
    }
    std::string s(reinterpret_cast<const char*>(base + pos), len);
    pos += len;
    return s;
}

// Inflate a single raw DEFLATE block via miniz tinfl_decompress_mem_to_heap.
std::vector<uint8_t> inflate_raw(const uint8_t* src, size_t src_len,
                                 size_t hint_out_size) {
    (void)hint_out_size;
    size_t out_len = 0;
    // No zlib wrapper, no zlib adler, but tell the inflater to keep going if
    // the source has more deflate streams concatenated (it doesn't, but flag
    // hygiene matters).
    void* p = tinfl_decompress_mem_to_heap(src, src_len, &out_len,
                                           0 /* flags: raw deflate */);
    if (!p) {
        uint8_t scratch = 0;
        const size_t mem_to_mem_len =
            tinfl_decompress_mem_to_mem(&scratch, sizeof(scratch), src, src_len,
                                        0 /* flags: raw deflate */);
        if (mem_to_mem_len == 0) return {};
        throw std::runtime_error("milo: raw deflate inflate failed");
    }
    std::vector<uint8_t> out(static_cast<uint8_t*>(p),
                             static_cast<uint8_t*>(p) + out_len);
    // tinfl_decompress_mem_to_heap allocates with MZ_MALLOC (== malloc by default).
    std::free(p);
    return out;
}

// Strip a gzip header and trailer, then call inflate_raw on the deflate stream.
std::vector<uint8_t> inflate_gzip(const uint8_t* src, size_t src_len,
                                  size_t hint_out_size) {
    if (src_len < 18 || src[0] != 0x1F || src[1] != 0x8B) {
        throw std::runtime_error("milo: GZIP block missing magic");
    }
    uint8_t flg = src[3];
    size_t p = 10;
    if (flg & 0x04) {                                   // FEXTRA
        if (p + 2 > src_len) throw std::runtime_error("milo gzip: bad FEXTRA");
        uint16_t xlen = static_cast<uint16_t>(src[p]) | (static_cast<uint16_t>(src[p+1]) << 8);
        p += 2 + xlen;
    }
    if (flg & 0x08) { while (p < src_len && src[p] != 0) ++p; ++p; }  // FNAME
    if (flg & 0x10) { while (p < src_len && src[p] != 0) ++p; ++p; }  // FCOMMENT
    if (flg & 0x02) { p += 2; }                                       // FHCRC
    if (p + 8 > src_len) throw std::runtime_error("milo gzip: short stream");
    size_t deflate_len = src_len - p - 8;  // trailing 8 = crc32 + isize
    return inflate_raw(src + p, deflate_len, hint_out_size);
}

}  // anonymous namespace

Header parse_header(const std::vector<uint8_t>& src) {
    if (src.size() < 16) throw std::runtime_error("milo: input shorter than 16-byte header");
    Header h{};
    h.structure                    = static_cast<BlockStructure>(rd_u32(src.data()));
    h.first_block_offset           = rd_u32(src.data() + 4);
    h.block_count                  = rd_u32(src.data() + 8);
    h.max_block_uncompressed_size  = rd_u32(src.data() + 12);

    // Quick magic validation -- accept all four MILO_* structures plus NONE/GZIP.
    switch (h.structure) {
        case BlockStructure::NONE:
        case BlockStructure::GZIP:
        case BlockStructure::MILO_A:
        case BlockStructure::MILO_B:
        case BlockStructure::MILO_C:
        case BlockStructure::MILO_D:
            break;
        default: {
            std::ostringstream oss;
            oss << "milo: unknown structure 0x" << std::hex
                << static_cast<uint32_t>(h.structure);
            throw std::runtime_error(oss.str());
        }
    }

    if (h.structure == BlockStructure::NONE || h.structure == BlockStructure::GZIP) {
        return h;  // No block table for these structures.
    }
    if (16 + h.block_count * 4 > src.size()) {
        throw std::runtime_error("milo: block size table exceeds file");
    }
    h.block_sizes.reserve(h.block_count);
    for (uint32_t i = 0; i < h.block_count; ++i) {
        h.block_sizes.push_back(rd_u32(src.data() + 16 + i * 4));
    }
    return h;
}

std::vector<uint8_t> inflate_payload(const std::vector<uint8_t>& src,
                                     const Header& h) {
    if (h.structure == BlockStructure::NONE) {
        if (h.first_block_offset > src.size())
            throw std::runtime_error("milo: NONE first_block_offset past EOF");
        return std::vector<uint8_t>(src.begin() + h.first_block_offset, src.end());
    }
    if (h.structure == BlockStructure::GZIP) {
        return inflate_gzip(src.data() + h.first_block_offset,
                            src.size() - h.first_block_offset,
                            h.max_block_uncompressed_size);
    }

    std::vector<uint8_t> out;
    out.reserve(h.max_block_uncompressed_size * 2);
    size_t pos = h.first_block_offset;

    for (uint32_t i = 0; i < h.block_count; ++i) {
        uint32_t raw_size = h.block_sizes[i];
        // MILO_D uses the high byte to mark an UNCOMPRESSED block (raw size flag set);
        // for compressed blocks the high byte is 0 and the low 24 bits hold the size.
        bool size_uncompressed = (h.structure == BlockStructure::MILO_D
                                  && (raw_size & 0xFF000000u) != 0);
        uint32_t block_disk_size = raw_size & 0x00FFFFFFu;
        if (pos + block_disk_size > src.size()) {
            throw std::runtime_error("milo: block extends past EOF");
        }

        if (block_disk_size == 0) { /* skip */ }
        else if (h.structure == BlockStructure::MILO_A || size_uncompressed) {
            out.insert(out.end(), src.begin() + pos, src.begin() + pos + block_disk_size);
        } else if (h.structure == BlockStructure::MILO_B) {
            auto inflated = inflate_raw(src.data() + pos, block_disk_size,
                                        h.max_block_uncompressed_size);
            out.insert(out.end(), inflated.begin(), inflated.end());
        } else if (h.structure == BlockStructure::MILO_C) {
            auto inflated = inflate_gzip(src.data() + pos, block_disk_size,
                                         h.max_block_uncompressed_size);
            out.insert(out.end(), inflated.begin(), inflated.end());
        } else {  // MILO_D, compressed sub-form: 4 bytes uncompressed size, then deflate
            if (block_disk_size < 4)
                throw std::runtime_error("milo_d: block too small for size prefix");
            auto inflated = inflate_raw(src.data() + pos + 4, block_disk_size - 4,
                                        h.max_block_uncompressed_size);
            out.insert(out.end(), inflated.begin(), inflated.end());
        }
        pos += block_disk_size;
    }
    return out;
}

Directory parse_directory(const std::vector<uint8_t>& p) {
    Directory d{};
    if (p.size() < 4) throw std::runtime_error("milo dir: too short");

    size_t pos = 0;
    d.dir_version = rd_i32(p.data()); pos += 4;

    if (d.dir_version >= 24) {
        d.dir_type = read_string(p.data(), p.size(), pos);
        d.dir_name = read_string(p.data(), p.size(), pos);
        if (pos + 8 > p.size()) throw std::runtime_error("milo dir: truncated hash sizing");
        pos += 8;  // hash table size hints, irrelevant for read
    }

    if (pos + 4 > p.size()) throw std::runtime_error("milo dir: missing entry count");
    int32_t entry_count = rd_i32(p.data() + pos); pos += 4;
    if (entry_count < 0 || static_cast<size_t>(entry_count) > p.size())
        throw std::runtime_error("milo dir: implausible entry count");

    d.entries.reserve(entry_count);
    for (int32_t i = 0; i < entry_count; ++i) {
        Entry e{};
        e.type = read_string(p.data(), p.size(), pos);
        e.name = read_string(p.data(), p.size(), pos);
        d.entries.push_back(std::move(e));
    }

    // After the entry list, GH1 (version 10) has an external resource list,
    // GH2+ (version 24+) has a directory-entry blob (or a recursive subdir for
    // version 25 ObjectDir). Both terminate with the 0xADDEADDE marker before
    // the per-entry bodies begin. To keep this structural pass robust without
    // class-specific knowledge, skip ahead to the FIRST 0xADDEADDE then start
    // measuring entry bodies from there.
    auto scan = [&](size_t from) -> size_t {
        for (size_t k = from; k + 4 <= p.size(); ++k) {
            if (rd_u32(p.data() + k) == kAddePadding) return k;
        }
        return p.size();
    };
    auto scan_object_padding = [&](size_t from, const std::string* type = nullptr,
                                   size_t body_start = 0) -> size_t {
        for (size_t k = from; k + 4 <= p.size(); ++k) {
            if (rd_u32(p.data() + k) != kAddePadding) continue;
            if (type && *type == "Mesh" && !mesh_body_fits_before(p, body_start, k)) {
                continue;
            }
            if (!type || *type != "Mesh") return k;
            if (k + 4 >= p.size()) return k;
            if (k + 8 <= p.size()) {
                const int32_t magic = rd_i32(p.data() + k + 4);
                if (magic >= 0 && magic <= 0xff) return k;
            }
        }
        return p.size();
    };

    size_t cursor = scan(pos);
    d.dir_entry_offset = pos;                 // root dir's own object body
    d.dir_entry_size = cursor - pos;
    if (cursor == p.size()) return d;  // no markers; nothing to size
    cursor += 4;                       // skip directory-entry terminator

    for (auto& e : d.entries) {
        while (e.type == "Mesh" && cursor + 4 <= p.size() &&
               rd_i32(p.data() + cursor) != 28) {
            const size_t skipped = scan(cursor);
            if (skipped == p.size()) break;
            cursor = skipped + 4;
        }
        e.offset = cursor;
        size_t end = scan_object_padding(cursor, &e.type, cursor);
        e.size = end - cursor;
        cursor = (end == p.size()) ? p.size() : (end + 4);
        if (cursor >= p.size()) break;
    }
    return d;
}

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("cannot open " + path);
    auto sz = static_cast<std::streamsize>(f.tellg());
    std::vector<uint8_t> buf(static_cast<size_t>(sz));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    if (!f) throw std::runtime_error("short read on " + path);
    return buf;
}

}  // namespace gh::milo
