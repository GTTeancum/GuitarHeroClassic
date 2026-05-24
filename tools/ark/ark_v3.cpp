// ark_v3.cpp - implementation, see ark_v3.h for notes on format provenance.

#include "ark_v3.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

namespace gh::ark {

namespace {

class Reader {
public:
    Reader(const std::vector<uint8_t>& bytes) : data_(bytes) {}

    uint32_t u32() {
        check(4);
        uint32_t v;
        std::memcpy(&v, data_.data() + p_, 4);  // file is little-endian; host assumed LE
        p_ += 4;
        return v;
    }

    void read_into(void* dst, size_t n) {
        check(n);
        std::memcpy(dst, data_.data() + p_, n);
        p_ += n;
    }

    size_t pos() const { return p_; }
    size_t remaining() const { return data_.size() - p_; }

private:
    void check(size_t n) {
        if (p_ + n > data_.size()) {
            throw std::runtime_error("HDR read past end");
        }
    }
    const std::vector<uint8_t>& data_;
    size_t p_ = 0;
};

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

std::string string_at(const std::vector<uint8_t>& blob, uint32_t off) {
    if (off >= blob.size()) return {};
    const auto* s = reinterpret_cast<const char*>(blob.data()) + off;
    return std::string(s, std::strlen(s));
}

}  // anonymous namespace

ArkV3Reader ArkV3Reader::load(const std::string& hdr_path) {
    auto bytes = read_file(hdr_path);
    Reader r(bytes);

    ArkV3Reader out;
    out.version_ = r.u32();
    if (out.version_ != 3) {
        std::ostringstream oss;
        oss << "ARK HDR version " << out.version_ << " not supported (this reader is v3 only)";
        throw std::runtime_error(oss.str());
    }
    out.flag_ = r.u32();
    uint32_t ark_count = r.u32();
    out.ark_part_sizes_.reserve(ark_count);
    for (uint32_t i = 0; i < ark_count; ++i) {
        out.ark_part_sizes_.push_back(r.u32());
    }

    // String blob
    uint32_t blob_size = r.u32();
    std::vector<uint8_t> blob(blob_size);
    r.read_into(blob.data(), blob_size);

    // String offset table
    uint32_t str_off_count = r.u32();
    std::vector<uint32_t> str_offsets(str_off_count);
    for (uint32_t i = 0; i < str_off_count; ++i) {
        str_offsets[i] = r.u32();
    }

    auto resolve = [&](uint32_t idx) -> std::string {
        if (idx == 0xFFFFFFFFu) return {};
        if (idx >= str_offsets.size()) return {};
        return string_at(blob, str_offsets[idx]);
    };

    // Entry table
    uint32_t entry_count = r.u32();
    out.entries_.reserve(entry_count);
    for (uint32_t i = 0; i < entry_count; ++i) {
        Entry e{};
        uint32_t off = r.u32();
        e.ark_part = 0;  // v3 with single ARK part covers GH1/GH2/GH80s PS2
        e.name_idx = r.u32();
        e.folder_idx = r.u32();
        e.size = r.u32();
        e.inflated_size = r.u32();

        // For multi-part ARKs, offset becomes (part_idx | (raw_offset))
        // depending on game; for the v3-singlepart targets we care about,
        // it's a plain absolute byte offset within main_0.ark.
        if (out.ark_part_sizes_.size() == 1) {
            e.offset = off;
        } else {
            // Walk parts to figure out which part contains this offset.
            uint64_t acc = 0;
            uint64_t abs_off = off;
            for (size_t p = 0; p < out.ark_part_sizes_.size(); ++p) {
                if (abs_off < acc + out.ark_part_sizes_[p]) {
                    e.ark_part = static_cast<uint32_t>(p);
                    e.offset = abs_off - acc;
                    break;
                }
                acc += out.ark_part_sizes_[p];
            }
        }

        e.name = resolve(e.name_idx);
        e.folder = resolve(e.folder_idx);
        e.full_path = e.folder.empty() ? e.name : (e.folder + "/" + e.name);
        out.entries_.push_back(std::move(e));
    }

    if (r.remaining() != 0) {
        // Not fatal but tells us the format guess was off; surface for visibility.
        std::ostringstream oss;
        oss << "HDR has " << r.remaining()
            << " unread bytes after entry table; format may differ for this ARK";
        throw std::runtime_error(oss.str());
    }

    return out;
}

std::vector<uint8_t> ArkV3Reader::read_entry(const Entry& e,
                                             const std::vector<std::string>& ark_paths) const {
    if (e.ark_part >= ark_paths.size() || ark_paths[e.ark_part].empty()) {
        throw std::runtime_error("ark_paths missing entry for part " + std::to_string(e.ark_part));
    }
    std::ifstream f(ark_paths[e.ark_part], std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + ark_paths[e.ark_part]);
    f.seekg(static_cast<std::streamoff>(e.offset));
    if (!f) throw std::runtime_error("seek past end of " + ark_paths[e.ark_part]);
    std::vector<uint8_t> buf(e.size);
    f.read(reinterpret_cast<char*>(buf.data()), e.size);
    if (!f) throw std::runtime_error("short read on " + ark_paths[e.ark_part]);
    return buf;
}

std::optional<Entry> ArkV3Reader::find(std::string_view full_path) const {
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [&](const Entry& e) { return e.full_path == full_path; });
    if (it == entries_.end()) return std::nullopt;
    return *it;
}

}  // namespace gh::ark
