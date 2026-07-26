// milo.cpp - see milo.h for format provenance notes.

#include "milo.h"

// Pull tinfl from vendored miniz (MIT) for raw-DEFLATE and zlib/gzip wrappers.
#include "../../third_party/miniz/miniz_tinfl.h"
#include "../../third_party/miniz/miniz_tdef.h"

#include <algorithm>
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
    const uint32_t mesh_version = tmp;
    if (mesh_version == 25) {
        if (!read_u32_at(p, pos, tmp) || tmp != 8) return false; // Trans version.
        if (pos + 96 + 4 > end) return false;
        pos += 96;
        uint32_t child_count = 0;
        if (!read_u32_at(p, pos, child_count) || child_count > 4096) return false;
        for (uint32_t i = 0; i < child_count; ++i)
            if (!skip_string_at(p, pos, end)) return false;
        if (!read_u32_at(p, pos, tmp)) return false;    // constraint
        if (!skip_string_at(p, pos, end)) return false; // target
        if (pos + 1 > end) return false;
        ++pos;                                          // preserve scale
        if (!skip_string_at(p, pos, end)) return false; // parent
        if (!read_u32_at(p, pos, tmp) || tmp != 1) return false; // Draw version.
        if (pos + 1 + 4 > end) return false;
        ++pos;                                          // showing
        uint32_t draw_count = 0;
        if (!read_u32_at(p, pos, draw_count) || draw_count > 4096) return false;
        for (uint32_t i = 0; i < draw_count; ++i)
            if (!skip_string_at(p, pos, end)) return false;
        if (pos + 16 > end) return false;               // sphere
        pos += 16;
        if (!skip_string_at(p, pos, end)) return false; // material
        if (!skip_string_at(p, pos, end)) return false; // geometry owner
        if (pos + 4 + 4 + 1 + 4 > end) return false;   // mutable, volume, bsp, count
        pos += 8;
        if (p[pos++] != 0) return false;
        uint32_t vcount = 0;
        if (!read_u32_at(p, pos, vcount)) return false;
        const uint64_t vertex_bytes = static_cast<uint64_t>(vcount) * 48u;
        if (vertex_bytes > end - pos || pos + static_cast<size_t>(vertex_bytes) + 4 > end)
            return false;
        pos += static_cast<size_t>(vertex_bytes);
        uint32_t fcount = 0;
        if (!read_u32_at(p, pos, fcount)) return false;
        return static_cast<uint64_t>(fcount) * 6u <= end - pos;
    }
    if (mesh_version != 28) return false;
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

void wr_u32(std::vector<uint8_t>& bytes, size_t pos, uint32_t value) {
    if (pos + 4 > bytes.size()) throw std::runtime_error("milo: header write exceeds prefix");
    bytes[pos + 0] = static_cast<uint8_t>(value);
    bytes[pos + 1] = static_cast<uint8_t>(value >> 8);
    bytes[pos + 2] = static_cast<uint8_t>(value >> 16);
    bytes[pos + 3] = static_cast<uint8_t>(value >> 24);
}

void append_u32_le(std::vector<uint8_t>& bytes, uint32_t value) {
    bytes.push_back(static_cast<uint8_t>(value));
    bytes.push_back(static_cast<uint8_t>(value >> 8));
    bytes.push_back(static_cast<uint8_t>(value >> 16));
    bytes.push_back(static_cast<uint8_t>(value >> 24));
}

void append_string(std::vector<uint8_t>& bytes, const std::string& value) {
    if (value.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("milo dir: string exceeds u32");
    append_u32_le(bytes, static_cast<uint32_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
}

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

std::vector<uint8_t> deflate_raw(const std::vector<uint8_t>& src) {
    size_t out_len = 0;
    const int flags = static_cast<int>(
        tdefl_create_comp_flags_from_zip_params(6, -15, 0));
    void* compressed =
        tdefl_compress_mem_to_heap(src.data(), src.size(), &out_len, flags);
    if (!compressed) throw std::runtime_error("milo: raw deflate encode failed");
    const auto* begin = static_cast<const uint8_t*>(compressed);
    std::vector<uint8_t> out(begin, begin + out_len);
    std::free(compressed);
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

Container parse_container(const std::vector<uint8_t>& src) {
    Container container;
    container.header = parse_header(src);
    const Header& h = container.header;
    if (h.first_block_offset > src.size())
        throw std::runtime_error("milo: first block offset past EOF");
    container.prefix_bytes.assign(src.begin(), src.begin() + h.first_block_offset);

    if (h.structure == BlockStructure::NONE) {
        ContainerBlock block;
        block.stored = true;
        block.disk_bytes.assign(src.begin() + h.first_block_offset, src.end());
        block.payload_bytes = block.disk_bytes;
        block.original_payload_bytes = block.payload_bytes;
        container.blocks.push_back(std::move(block));
        return container;
    }
    if (h.structure == BlockStructure::GZIP) {
        ContainerBlock block;
        block.disk_bytes.assign(src.begin() + h.first_block_offset, src.end());
        block.payload_bytes =
            inflate_gzip(block.disk_bytes.data(), block.disk_bytes.size(),
                         h.max_block_uncompressed_size);
        block.original_payload_bytes = block.payload_bytes;
        container.blocks.push_back(std::move(block));
        return container;
    }

    size_t pos = h.first_block_offset;
    container.blocks.reserve(h.block_count);
    for (uint32_t i = 0; i < h.block_count; ++i) {
        ContainerBlock block;
        block.table_value = h.block_sizes[i];
        block.stored =
            h.structure == BlockStructure::MILO_A ||
            (h.structure == BlockStructure::MILO_D &&
             (block.table_value & 0xff000000u) != 0);
        const uint32_t disk_size =
            h.structure == BlockStructure::MILO_D
                ? (block.table_value & 0x00ffffffu)
                : block.table_value;
        if (pos + disk_size > src.size())
            throw std::runtime_error("milo: block extends past EOF");
        block.disk_bytes.assign(src.begin() + pos, src.begin() + pos + disk_size);
        if (block.stored) {
            block.payload_bytes = block.disk_bytes;
        } else if (h.structure == BlockStructure::MILO_B) {
            block.payload_bytes =
                inflate_raw(block.disk_bytes.data(), block.disk_bytes.size(),
                            h.max_block_uncompressed_size);
        } else if (h.structure == BlockStructure::MILO_C) {
            block.payload_bytes =
                inflate_gzip(block.disk_bytes.data(), block.disk_bytes.size(),
                             h.max_block_uncompressed_size);
        } else if (h.structure == BlockStructure::MILO_D) {
            if (block.disk_bytes.size() < 4)
                throw std::runtime_error("milo_d: block too small for size prefix");
            block.payload_bytes =
                inflate_raw(block.disk_bytes.data() + 4,
                            block.disk_bytes.size() - 4,
                            h.max_block_uncompressed_size);
        }
        block.original_payload_bytes = block.payload_bytes;
        container.blocks.push_back(std::move(block));
        pos += disk_size;
    }
    container.trailing_bytes.assign(src.begin() + pos, src.end());
    return container;
}

std::vector<uint8_t> container_payload(const Container& container) {
    size_t total = 0;
    for (const auto& block : container.blocks) total += block.payload_bytes.size();
    std::vector<uint8_t> payload;
    payload.reserve(total);
    for (const auto& block : container.blocks)
        payload.insert(payload.end(), block.payload_bytes.begin(),
                       block.payload_bytes.end());
    return payload;
}

std::vector<uint8_t> serialize_container(const Container& container) {
    if (container.header.structure == BlockStructure::NONE) {
        if (container.blocks.size() != 1)
            throw std::runtime_error("milo: NONE container requires one block");
        std::vector<uint8_t> out = container.prefix_bytes;
        out.insert(out.end(), container.blocks[0].payload_bytes.begin(),
                   container.blocks[0].payload_bytes.end());
        return out;
    }
    if (container.header.structure == BlockStructure::GZIP) {
        if (container.blocks.size() != 1 ||
            container.blocks[0].payload_bytes !=
                container.blocks[0].original_payload_bytes)
            throw std::runtime_error(
                "milo: modified standalone GZIP writing is not supported");
        std::vector<uint8_t> out = container.prefix_bytes;
        out.insert(out.end(), container.blocks[0].disk_bytes.begin(),
                   container.blocks[0].disk_bytes.end());
        return out;
    }
    if (container.blocks.size() > 128u)
        throw std::runtime_error("milo: GH1/GH2 fixed block table holds at most 128 blocks");

    struct EncodedBlock {
        uint32_t table_value = 0;
        std::vector<uint8_t> bytes;
    };
    std::vector<EncodedBlock> encoded;
    encoded.reserve(container.blocks.size());
    uint32_t max_uncompressed = 0;
    for (const auto& block : container.blocks) {
        EncodedBlock out_block;
        const bool unchanged =
            block.payload_bytes == block.original_payload_bytes &&
            !block.disk_bytes.empty();
        if (unchanged) {
            out_block.table_value = block.table_value;
            out_block.bytes = block.disk_bytes;
        } else if (container.header.structure == BlockStructure::MILO_A) {
            out_block.bytes = block.payload_bytes;
            out_block.table_value = static_cast<uint32_t>(out_block.bytes.size());
        } else if (container.header.structure == BlockStructure::MILO_B) {
            out_block.bytes = deflate_raw(block.payload_bytes);
            out_block.table_value = static_cast<uint32_t>(out_block.bytes.size());
        } else {
            throw std::runtime_error(
                "milo: changed MILO_C/MILO_D block writing is not yet supported");
        }
        max_uncompressed =
            std::max(max_uncompressed,
                     static_cast<uint32_t>(block.payload_bytes.size()));
        encoded.push_back(std::move(out_block));
    }

    uint32_t first_block_offset = container.header.first_block_offset;
    const uint32_t table_end =
        16u + static_cast<uint32_t>(encoded.size()) * 4u;
    if (first_block_offset < table_end)
        first_block_offset = (table_end + 15u) & ~15u;
    std::vector<uint8_t> prefix = container.prefix_bytes;
    prefix.resize(first_block_offset, 0);
    wr_u32(prefix, 0, static_cast<uint32_t>(container.header.structure));
    wr_u32(prefix, 4, first_block_offset);
    wr_u32(prefix, 8, static_cast<uint32_t>(encoded.size()));
    wr_u32(prefix, 12,
           std::max(container.header.max_block_uncompressed_size,
                    max_uncompressed));
    for (size_t i = 0; i < encoded.size(); ++i)
        wr_u32(prefix, 16 + i * 4, encoded[i].table_value);

    std::vector<uint8_t> out = std::move(prefix);
    for (const auto& block : encoded)
        out.insert(out.end(), block.bytes.begin(), block.bytes.end());
    out.insert(out.end(), container.trailing_bytes.begin(),
               container.trailing_bytes.end());
    return out;
}

Container make_container(const std::vector<uint8_t>& payload,
                         BlockStructure structure,
                         uint32_t block_uncompressed_limit,
                         uint32_t first_block_offset) {
    if (structure != BlockStructure::MILO_A &&
        structure != BlockStructure::MILO_B)
        throw std::runtime_error("milo: new containers support MILO_A/MILO_B");
    if (block_uncompressed_limit == 0)
        throw std::runtime_error("milo: block size limit must be nonzero");
    Container container;
    container.header.structure = structure;
    container.header.first_block_offset = first_block_offset;
    container.prefix_bytes.resize(first_block_offset, 0);
    for (size_t pos = 0; pos < payload.size();) {
        const size_t count =
            std::min<size_t>(block_uncompressed_limit, payload.size() - pos);
        ContainerBlock block;
        block.payload_bytes.assign(payload.begin() + pos,
                                   payload.begin() + pos + count);
        container.blocks.push_back(std::move(block));
        pos += count;
    }
    if (payload.empty()) container.blocks.emplace_back();
    container.header.block_count =
        static_cast<uint32_t>(container.blocks.size());
    container.header.max_block_uncompressed_size =
        static_cast<uint32_t>(
            std::min<size_t>(payload.size(), block_uncompressed_limit));
    return container;
}

Directory parse_directory(const std::vector<uint8_t>& p) {
    Directory d{};
    if (p.size() < 4) throw std::runtime_error("milo dir: too short");

    size_t pos = 0;
    d.dir_version = rd_i32(p.data()); pos += 4;

    if (d.dir_version >= 14) {
        d.dir_type = read_string(p.data(), p.size(), pos);
        d.dir_name = read_string(p.data(), p.size(), pos);
        if (pos + 8 > p.size()) throw std::runtime_error("milo dir: truncated hash sizing");
        d.hash_table_hint = rd_u32(p.data() + pos); pos += 4;
        d.string_table_hint = rd_u32(p.data() + pos); pos += 4;
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

    // GH1 revision-10 directories carry an external-resource vector here and
    // have no serialized root-directory object. Child body 0 therefore starts
    // immediately after this vector. Treating the first child's terminator as
    // a root-object terminator shifts every table name onto the next body.
    if (d.dir_version >= 7 && d.dir_version <= 16) {
        if (pos + 4 > p.size())
            throw std::runtime_error(
                "milo dir: missing external resource count");
        const uint32_t external_count = rd_u32(p.data() + pos); pos += 4;
        if (external_count > d.entries.size() + 1024u)
            throw std::runtime_error(
                "milo dir: implausible external resource count");
        d.external_resources.reserve(external_count);
        for (uint32_t i = 0; i < external_count; ++i)
            d.external_resources.push_back(
                read_string(p.data(), p.size(), pos));
    }
    d.object_data_offset = pos;

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
            // GH1 directory revision 10 remaps legacy View objects to Group.
            // A View body starts with Group revision 7.  This transition is a
            // stronger boundary than the conservative Mesh-tail validator,
            // which cannot fully model every small rigid-bone Mesh variant.
            if (type && *type == "Mesh" && d.dir_version == 10 &&
                k + 8 <= p.size() &&
                (rd_u32(p.data() + k + 4) & 0xffffu) == 7u) {
                return k;
            }
            if (type && *type == "Mesh" && !mesh_body_fits_before(p, body_start, k)) {
                continue;
            }
            if (!type || *type != "Mesh") return k;
            if (k + 4 >= p.size()) return k;
            if (k + 8 <= p.size()) {
                const uint32_t revision = rd_u32(p.data() + k + 4);
                const uint16_t main_revision =
                    static_cast<uint16_t>(revision & 0xffffu);
                const uint16_t alt_revision =
                    static_cast<uint16_t>(revision >> 16);
                // Old object bodies use packed main/alternate revisions
                // (for example 0x00010001), not only a small scalar.  GH1's
                // final Mesh is followed by legacy View-as-Group objects, so
                // rejecting that packed value discarded the entire tail.
                if (main_revision <= 0xff && alt_revision <= 0xff) return k;
            }
        }
        return p.size();
    };

    size_t cursor = pos;
    if (d.dir_version == 10) {
        d.dir_entry_offset = pos;
        d.dir_entry_size = 0;
    } else {
        cursor = scan(pos);
        d.dir_entry_offset = pos;              // root dir's own object body
        d.dir_entry_size = cursor - pos;
        if (cursor == p.size()) return d;       // no markers; nothing to size
        cursor += 4;                            // root-object terminator
    }

    for (auto& e : d.entries) {
        const int32_t expected_mesh_version = d.dir_version == 10 ? 25 : 28;
        while (e.type == "Mesh" && cursor + 4 <= p.size() &&
               rd_i32(p.data() + cursor) != expected_mesh_version) {
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

std::vector<uint8_t> serialize_directory_prefix(const Directory& d) {
    if (d.dir_version < 7)
        throw std::runtime_error(
            "milo dir: revisions below 7 are not supported");
    if (d.entries.size() >
        static_cast<size_t>(std::numeric_limits<int32_t>::max()))
        throw std::runtime_error("milo dir: entry count exceeds i32");
    if (d.external_resources.size() >
        std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "milo dir: external resource count exceeds u32");

    std::vector<uint8_t> bytes;
    append_u32_le(bytes, static_cast<uint32_t>(d.dir_version));
    if (d.dir_version >= 14) {
        append_string(bytes, d.dir_type);
        append_string(bytes, d.dir_name);
        append_u32_le(bytes, d.hash_table_hint);
        append_u32_le(bytes, d.string_table_hint);
    }
    append_u32_le(bytes, static_cast<uint32_t>(d.entries.size()));
    for (const auto& entry : d.entries) {
        append_string(bytes, entry.type);
        append_string(bytes, entry.name);
    }
    if (d.dir_version <= 16) {
        append_u32_le(
            bytes, static_cast<uint32_t>(d.external_resources.size()));
        for (const auto& path : d.external_resources)
            append_string(bytes, path);
    } else if (!d.external_resources.empty()) {
        throw std::runtime_error(
            "milo dir: external resources are invalid after revision 16");
    }
    return bytes;
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
