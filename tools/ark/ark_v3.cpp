// ark_v3.cpp - implementation, see ark_v3.h for notes on format provenance.

#include "ark_v3.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <mutex>
#include <set>
#include <sstream>

namespace gh::ark {

namespace {

std::mutex& loose_mount_mutex() {
    static std::mutex mutex;
    return mutex;
}

std::vector<LooseFileMount>& loose_mount_storage() {
    static std::vector<LooseFileMount> mounts;
    return mounts;
}

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
        if (n > data_.size() - p_) {
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
    if (off >= blob.size())
        throw std::runtime_error("HDR string offset outside blob");
    size_t end = off;
    while (end < blob.size() && blob[end] != 0) ++end;
    if (end == blob.size())
        throw std::runtime_error("HDR string is not NUL terminated");
    return std::string(reinterpret_cast<const char*>(blob.data() + off),
                       end - off);
}

void append_u32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 24));
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

Index parse_index(const std::vector<uint8_t>& bytes) {
    Reader reader(bytes);
    Index index;
    index.version = reader.u32();
    if (index.version != 3)
        throw std::runtime_error("ARK HDR is not version 3");
    index.flag = reader.u32();

    const uint32_t ark_count = reader.u32();
    if (ark_count > reader.remaining() / 4)
        throw std::runtime_error("HDR ARK part count exceeds remaining bytes");
    index.ark_part_sizes.reserve(ark_count);
    for (uint32_t i = 0; i < ark_count; ++i)
        index.ark_part_sizes.push_back(reader.u32());

    const uint32_t blob_size = reader.u32();
    if (blob_size > reader.remaining())
        throw std::runtime_error("HDR string blob exceeds remaining bytes");
    index.string_blob.resize(blob_size);
    reader.read_into(index.string_blob.data(), index.string_blob.size());

    const uint32_t string_count = reader.u32();
    if (string_count > reader.remaining() / 4)
        throw std::runtime_error(
            "HDR string-offset count exceeds remaining bytes");
    index.string_offsets.reserve(string_count);
    for (uint32_t i = 0; i < string_count; ++i) {
        const uint32_t offset = reader.u32();
        (void)string_at(index.string_blob, offset);
        index.string_offsets.push_back(offset);
    }

    const uint32_t entry_count = reader.u32();
    if (entry_count > reader.remaining() / 20)
        throw std::runtime_error("HDR entry count exceeds remaining bytes");
    index.entries.reserve(entry_count);
    for (uint32_t i = 0; i < entry_count; ++i) {
        IndexEntry entry;
        entry.raw_offset = reader.u32();
        entry.name_idx = reader.u32();
        entry.folder_idx = reader.u32();
        entry.size = reader.u32();
        entry.inflated_size = reader.u32();
        if (entry.name_idx >= index.string_offsets.size())
            throw std::runtime_error("HDR name index outside string table");
        if (entry.folder_idx != 0xffffffffu &&
            entry.folder_idx >= index.string_offsets.size())
            throw std::runtime_error("HDR folder index outside string table");
        index.entries.push_back(entry);
    }
    uint64_t archive_size = 0;
    for (uint32_t size : index.ark_part_sizes) archive_size += size;
    for (const auto& entry : index.entries) {
        if (static_cast<uint64_t>(entry.raw_offset) + entry.size >
            archive_size)
            throw std::runtime_error(
                "HDR entry range exceeds declared ARK part sizes");
    }
    if (reader.remaining()) {
        index.trailing_bytes.resize(reader.remaining());
        reader.read_into(index.trailing_bytes.data(),
                         index.trailing_bytes.size());
    }
    return index;
}

std::vector<uint8_t> serialize_index(const Index& index) {
    if (index.version != 3)
        throw std::runtime_error("only ARK HDR version 3 can be serialized");
    const auto fits_u32 = [](size_t value, const char* what) {
        if (value > std::numeric_limits<uint32_t>::max())
            throw std::runtime_error(std::string("HDR ") + what +
                                     " exceeds u32");
    };
    fits_u32(index.ark_part_sizes.size(), "ARK part count");
    fits_u32(index.string_blob.size(), "string blob size");
    fits_u32(index.string_offsets.size(), "string count");
    fits_u32(index.entries.size(), "entry count");
    for (uint32_t offset : index.string_offsets)
        (void)string_at(index.string_blob, offset);

    std::vector<uint8_t> bytes;
    append_u32(bytes, index.version);
    append_u32(bytes, index.flag);
    append_u32(bytes,
               static_cast<uint32_t>(index.ark_part_sizes.size()));
    for (uint32_t size : index.ark_part_sizes) append_u32(bytes, size);
    append_u32(bytes, static_cast<uint32_t>(index.string_blob.size()));
    bytes.insert(bytes.end(), index.string_blob.begin(),
                 index.string_blob.end());
    append_u32(bytes,
               static_cast<uint32_t>(index.string_offsets.size()));
    for (uint32_t offset : index.string_offsets)
        append_u32(bytes, offset);
    append_u32(bytes, static_cast<uint32_t>(index.entries.size()));
    for (const auto& entry : index.entries) {
        if (entry.name_idx >= index.string_offsets.size())
            throw std::runtime_error("HDR name index outside string table");
        if (entry.folder_idx != 0xffffffffu &&
            entry.folder_idx >= index.string_offsets.size())
            throw std::runtime_error("HDR folder index outside string table");
        append_u32(bytes, entry.raw_offset);
        append_u32(bytes, entry.name_idx);
        append_u32(bytes, entry.folder_idx);
        append_u32(bytes, entry.size);
        append_u32(bytes, entry.inflated_size);
    }
    bytes.insert(bytes.end(), index.trailing_bytes.begin(),
                 index.trailing_bytes.end());
    return bytes;
}

Index make_index(const std::vector<uint32_t>& ark_part_sizes,
                 const std::vector<LayoutEntry>& layout_entries,
                 uint32_t flag) {
    if (ark_part_sizes.empty())
        throw std::runtime_error("ARK layout requires at least one part");
    uint64_t archive_size = 0;
    for (uint32_t size : ark_part_sizes) archive_size += size;

    struct SplitPath {
        std::string folder;
        std::string name;
    };
    std::vector<SplitPath> paths;
    paths.reserve(layout_entries.size());
    std::set<std::string> unique_strings;
    for (const auto& layout : layout_entries) {
        std::string path = layout.full_path;
        std::replace(path.begin(), path.end(), '\\', '/');
        const size_t slash = path.rfind('/');
        SplitPath split;
        if (slash == std::string::npos) {
            split.name = path;
        } else {
            split.folder = path.substr(0, slash);
            split.name = path.substr(slash + 1);
        }
        if (split.name.empty())
            throw std::runtime_error("ARK layout has empty file name");
        if (static_cast<uint64_t>(layout.raw_offset) + layout.size >
            archive_size)
            throw std::runtime_error("ARK layout entry exceeds part sizes");
        unique_strings.insert(split.name);
        if (!split.folder.empty()) unique_strings.insert(split.folder);
        paths.push_back(std::move(split));
    }

    Index index;
    index.flag = flag;
    index.ark_part_sizes = ark_part_sizes;
    std::map<std::string, uint32_t> string_indices;
    for (const auto& value : unique_strings) {
        if (index.string_offsets.size() >
            std::numeric_limits<uint32_t>::max())
            throw std::runtime_error("ARK layout has too many strings");
        if (index.string_blob.size() >
            std::numeric_limits<uint32_t>::max())
            throw std::runtime_error("ARK string blob exceeds u32");
        const uint32_t idx =
            static_cast<uint32_t>(index.string_offsets.size());
        string_indices[value] = idx;
        index.string_offsets.push_back(
            static_cast<uint32_t>(index.string_blob.size()));
        index.string_blob.insert(index.string_blob.end(), value.begin(),
                                 value.end());
        index.string_blob.push_back(0);
    }

    index.entries.reserve(layout_entries.size());
    for (size_t i = 0; i < layout_entries.size(); ++i) {
        IndexEntry entry;
        entry.raw_offset = layout_entries[i].raw_offset;
        entry.name_idx = string_indices.at(paths[i].name);
        entry.folder_idx = paths[i].folder.empty()
                               ? 0xffffffffu
                               : string_indices.at(paths[i].folder);
        entry.size = layout_entries[i].size;
        entry.inflated_size = layout_entries[i].inflated_size;
        index.entries.push_back(entry);
    }
    return index;
}

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

    const auto bytes = read_file(hdr_path);
    const Index index = parse_index(bytes);
    ArkV3Reader out;
    out.version_ = index.version;
    out.flag_ = index.flag;
    out.ark_part_sizes_.assign(index.ark_part_sizes.begin(),
                               index.ark_part_sizes.end());

    auto resolve = [&](uint32_t idx) -> std::string {
        if (idx == 0xFFFFFFFFu) return {};
        if (idx >= index.string_offsets.size())
            throw std::runtime_error("HDR string index outside table");
        return string_at(index.string_blob, index.string_offsets[idx]);
    };

    out.entries_.reserve(index.entries.size());
    for (const auto& raw : index.entries) {
        Entry e{};
        const uint32_t off = raw.raw_offset;
        e.ark_part = 0;  // v3 with single ARK part covers GH1/GH2/GH80s PS2
        e.name_idx = raw.name_idx;
        e.folder_idx = raw.folder_idx;
        e.size = raw.size;
        e.inflated_size = raw.inflated_size;

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

    if (!index.trailing_bytes.empty()) {
        std::ostringstream oss;
        oss << "HDR has " << index.trailing_bytes.size()
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
    if (!e.loose_path.empty()) {
        std::ifstream f(e.loose_path, std::ios::binary | std::ios::ate);
        if (!f) throw std::runtime_error("cannot open loose file " + e.loose_path);
        const std::streamoff size = f.tellg();
        if (size < 0) throw std::runtime_error("cannot size loose file " + e.loose_path);
        f.seekg(0);
        std::vector<uint8_t> buf(static_cast<size_t>(size));
        if (!buf.empty())
            f.read(reinterpret_cast<char*>(buf.data()), size);
        if (!f) throw std::runtime_error("short read on loose file " + e.loose_path);
        return buf;
    }
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
    {
        std::lock_guard<std::mutex> lock(loose_mount_mutex());
        const auto& mounts = loose_mount_storage();
        const auto mounted = std::lower_bound(
            mounts.begin(), mounts.end(), full_path,
            [](const LooseFileMount& mount, std::string_view path) {
                return mount.virtual_path < path;
            });
        if (mounted != mounts.end() && mounted->virtual_path == full_path) {
            Entry entry{};
            entry.ark_part = 0;
            entry.offset = 0;
            std::error_code size_error;
            const uintmax_t mounted_size =
                std::filesystem::file_size(mounted->file_path, size_error);
            if (size_error || mounted_size > std::numeric_limits<uint32_t>::max())
                return std::optional<Entry>{};
            entry.size = static_cast<uint32_t>(mounted_size);
            entry.inflated_size = 0;
            entry.full_path = mounted->virtual_path;
            const size_t slash = entry.full_path.rfind('/');
            entry.name = slash == std::string::npos
                             ? entry.full_path
                             : entry.full_path.substr(slash + 1);
            entry.folder = slash == std::string::npos
                               ? std::string{}
                               : entry.full_path.substr(0, slash);
            entry.loose_path = mounted->file_path.string();
            return entry;
        }
    }
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

void ArkV3Reader::set_loose_file_mounts(std::vector<LooseFileMount> mounts) {
    std::sort(mounts.begin(), mounts.end(),
              [](const LooseFileMount& lhs, const LooseFileMount& rhs) {
                  return lhs.virtual_path < rhs.virtual_path;
              });
    std::lock_guard<std::mutex> lock(loose_mount_mutex());
    loose_mount_storage() = std::move(mounts);
}

void ArkV3Reader::clear_loose_file_mounts() {
    set_loose_file_mounts({});
}

std::vector<LooseFileMount> ArkV3Reader::loose_file_mounts() {
    std::lock_guard<std::mutex> lock(loose_mount_mutex());
    return loose_mount_storage();
}

}  // namespace gh::ark
