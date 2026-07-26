// ark_v3.cpp - implementation, see ark_v3.h for notes on format provenance.

#include "ark_v3.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>

namespace gh::ark {

namespace {

std::string ark_part_path_from_seed(const std::string& seed, size_t part) {
    namespace fs = std::filesystem;
    const fs::path seed_path(seed);
    const fs::path parent = seed_path.parent_path();
    const std::string filename = seed_path.filename().string();
    const std::string stem = seed_path.stem().string();
    const std::string ext = seed_path.extension().string();

    const size_t underscore = stem.find_last_of('_');
    if (underscore == std::string::npos || underscore + 1 >= stem.size()) {
        return {};
    }
    const std::string suffix = stem.substr(underscore + 1);
    if (!std::all_of(suffix.begin(), suffix.end(), [](unsigned char ch) {
            return std::isdigit(ch) != 0;
        })) {
        return {};
    }

    const std::string prefix = stem.substr(0, underscore + 1);
    std::ostringstream suffix_stream;
    suffix_stream << part;
    std::string candidate =
        (parent / (prefix + suffix_stream.str() + ext)).string();
    if (fs::exists(candidate)) return candidate;

    std::string upper = filename;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::toupper(ch));
                   });
    std::string upper_stem = fs::path(upper).stem().string();
    const std::string upper_ext = fs::path(upper).extension().string();
    const size_t upper_underscore = upper_stem.find_last_of('_');
    if (upper_underscore != std::string::npos) {
        candidate =
            (parent / (upper_stem.substr(0, upper_underscore + 1) +
                       suffix_stream.str() + upper_ext))
                .string();
        if (fs::exists(candidate)) return candidate;
    }

    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    std::string lower_stem = fs::path(lower).stem().string();
    const std::string lower_ext = fs::path(lower).extension().string();
    const size_t lower_underscore = lower_stem.find_last_of('_');
    if (lower_underscore != std::string::npos) {
        candidate =
            (parent / (lower_stem.substr(0, lower_underscore + 1) +
                       suffix_stream.str() + lower_ext))
                .string();
        if (fs::exists(candidate)) return candidate;
    }

    return {};
}

std::vector<std::string> expand_ark_paths_from_first_part(
    const std::vector<std::string>& ark_paths, size_t part_count) {
    std::vector<std::string> expanded = ark_paths;
    if (expanded.empty() || expanded.front().empty()) return expanded;
    if (expanded.size() < part_count) expanded.resize(part_count);
    for (size_t part = 1; part < part_count; ++part) {
        if (!expanded[part].empty()) continue;
        expanded[part] = ark_part_path_from_seed(expanded.front(), part);
    }
    return expanded;
}

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

struct FileStamp {
    std::filesystem::file_time_type write_time{};
    uintmax_t size = 0;
    bool valid = false;
};

FileStamp stamp_file(const std::string& path) {
    std::error_code ec;
    FileStamp stamp;
    stamp.write_time = std::filesystem::last_write_time(path, ec);
    if (ec) return {};
    stamp.size = std::filesystem::file_size(path, ec);
    if (ec) return {};
    stamp.valid = true;
    return stamp;
}

std::string cache_key_for(const std::string& path) {
    std::error_code ec;
    auto absolute = std::filesystem::absolute(path, ec);
    if (ec) return path;
    return absolute.lexically_normal().string();
}

struct CachedHdr {
    FileStamp stamp;
    ArkV3Reader reader;
};

}  // anonymous namespace

ArkV3Reader ArkV3Reader::load(const std::string& hdr_path) {
    static std::mutex cache_mutex;
    static std::map<std::string, CachedHdr> cache;

    const std::string cache_key = cache_key_for(hdr_path);
    const FileStamp stamp = stamp_file(hdr_path);
    if (stamp.valid) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        const auto cached = cache.find(cache_key);
        if (cached != cache.end() &&
            cached->second.stamp.size == stamp.size &&
            cached->second.stamp.write_time == stamp.write_time) {
            return cached->second.reader;
        }
    }

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

    if (stamp.valid) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        cache[cache_key] = CachedHdr{stamp, out};
    }

    return out;
}

std::vector<uint8_t> ArkV3Reader::read_entry(const Entry& e,
                                             const std::vector<std::string>& ark_paths) const {
    const std::vector<std::string> expanded_paths =
        expand_ark_paths_from_first_part(ark_paths, ark_part_sizes_.size());
    if (e.ark_part >= expanded_paths.size() ||
        expanded_paths[e.ark_part].empty()) {
        throw std::runtime_error("ark_paths missing entry for part " + std::to_string(e.ark_part));
    }
    std::ifstream f(expanded_paths[e.ark_part], std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + expanded_paths[e.ark_part]);
    f.seekg(static_cast<std::streamoff>(e.offset));
    if (!f) throw std::runtime_error("seek past end of " + expanded_paths[e.ark_part]);
    std::vector<uint8_t> buf(e.size);
    f.read(reinterpret_cast<char*>(buf.data()), e.size);
    if (!f) throw std::runtime_error("short read on " + expanded_paths[e.ark_part]);
    return buf;
}

std::optional<Entry> ArkV3Reader::find(std::string_view full_path) const {
    const auto find_exact = [&](std::string_view path) {
        return std::find_if(entries_.begin(), entries_.end(),
                            [&](const Entry& e) { return e.full_path == path; });
    };

    auto it = find_exact(full_path);
    if (it != entries_.end()) return *it;

    // Guitar Hero 1 uses the original .rnd_ps2 suffix for the same Milo
    // object-directory containers renamed .milo_ps2 by GH2.  Keep this at the
    // archive boundary so every scene, texture, character, HUD, and venue
    // caller receives the native GH1 counterpart without game-specific path
    // rewrites.  Exact paths above always win for GH2/GH80s archives.
    constexpr std::string_view kMiloSuffix = ".milo_ps2";
    constexpr std::string_view kRndSuffix = ".rnd_ps2";
    if (full_path.size() >= kMiloSuffix.size() &&
        full_path.substr(full_path.size() - kMiloSuffix.size()) == kMiloSuffix) {
        std::string gh1_path(full_path.substr(0, full_path.size() - kMiloSuffix.size()));
        gh1_path.append(kRndSuffix);
        it = find_exact(gh1_path);
        if (it != entries_.end()) return *it;

        // GH1 roots venue scenes under venues/; GH2 renamed that root world/.
        // Apply the namespace bridge only after both exact GH2 forms miss.
        constexpr std::string_view kGh2WorldRoot = "world/";
        if (gh1_path.rfind(kGh2WorldRoot, 0) == 0) {
            gh1_path.replace(0, kGh2WorldRoot.size(), "venues/");
            it = find_exact(gh1_path);
            if (it != entries_.end()) return *it;
        }
    }

    return std::nullopt;
}

}  // namespace gh::ark
