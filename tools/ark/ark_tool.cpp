// ark_tool - CLI for inspecting and extracting Harmonix v3 ARKs.
//
// Subcommands:
//   list      <hdr> [--ext-summary] [--limit N]
//   paths     <hdr>
//   verify    <hdr> [ark...]
//   extract   <hdr> <ark>... --path <full_path> --out <file>
//   extract-all <hdr> <ark>... --out <dir>
//   extract-prefix <hdr> <ark>... --prefix <dir/> --out <dir>
//   pack      <root> --hdr <main.hdr> --ark <main_0.ark>

#include "ark_v3.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static void die(const std::string& msg) {
    std::fprintf(stderr, "ark_tool: %s\n", msg.c_str());
    std::exit(2);
}

static void usage() {
    std::fprintf(stderr,
        "Usage:\n"
        "  ark_tool list <hdr> [--ext-summary] [--limit N]\n"
        "  ark_tool paths <hdr>\n"
        "  ark_tool verify <hdr> [ark...]\n"
        "  ark_tool extract <hdr> <ark>... --path <full_path> --out <file>\n"
        "  ark_tool extract-all <hdr> <ark>... --out <dir>\n"
        "  ark_tool extract-prefix <hdr> <ark>... --prefix <dir/> --out <dir>\n"
        "  ark_tool pack <root> --hdr <main.hdr> --ark <main_0.ark>\n"
        "  ark_tool overlay <hdr> <ark> --root <dir> --manifest <tsv>\n");
    std::exit(2);
}

static std::vector<uint8_t> read_all(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) die("cannot open: " + path);
    input.seekg(0, std::ios::end);
    const std::streamoff size = input.tellg();
    if (size < 0) die("cannot determine size: " + path);
    input.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), size);
        if (!input) die("cannot read: " + path);
    }
    return bytes;
}

static int cmd_verify(int argc, char** argv) {
    if (argc < 1) usage();
    const std::string hdr_path = argv[0];
    const auto source = read_all(hdr_path);
    const auto index = gh::ark::parse_index(source);
    const auto serialized = gh::ark::serialize_index(index);
    const bool exact = source == serialized;
    bool payload_valid = true;
    std::string payload_status = "not-checked";
    if (argc > 1) {
        payload_status = "valid";
        const std::size_t supplied = static_cast<std::size_t>(argc - 1);
        if (supplied != index.ark_part_sizes.size()) {
            std::fprintf(
                stderr,
                "ark_tool: HDR declares %zu ARK parts, supplied %zu\n",
                index.ark_part_sizes.size(), supplied);
            payload_valid = false;
        }
        const std::size_t checked =
            std::min(supplied, index.ark_part_sizes.size());
        for (std::size_t part = 0; part < checked; ++part) {
            std::error_code error;
            const std::uintmax_t actual = fs::file_size(argv[part + 1], error);
            if (error || actual != index.ark_part_sizes[part]) {
                std::fprintf(
                    stderr,
                    "ark_tool: ARK part %zu size mismatch: expected %llu, "
                    "found %s\n",
                    part,
                    static_cast<unsigned long long>(index.ark_part_sizes[part]),
                    error ? "unreadable"
                          : std::to_string(
                                static_cast<unsigned long long>(actual))
                                .c_str());
                payload_valid = false;
            }
        }
        std::uint64_t total_size = 0;
        for (const std::uint32_t size : index.ark_part_sizes)
            total_size += size;
        for (std::size_t entry_index = 0;
             entry_index < index.entries.size(); ++entry_index) {
            const auto& entry = index.entries[entry_index];
            const std::uint64_t begin = entry.raw_offset;
            const std::uint64_t end = begin + entry.size;
            bool within_one_part = false;
            std::uint64_t part_begin = 0;
            for (const std::uint32_t part_size : index.ark_part_sizes) {
                const std::uint64_t part_end = part_begin + part_size;
                if (begin >= part_begin && end <= part_end) {
                    within_one_part = true;
                    break;
                }
                part_begin = part_end;
            }
            if (end < begin || end > total_size || !within_one_part) {
                std::fprintf(
                    stderr,
                    "ark_tool: entry %zu range [%llu,%llu) is outside one "
                    "declared ARK part\n",
                    entry_index,
                    static_cast<unsigned long long>(begin),
                    static_cast<unsigned long long>(end));
                payload_valid = false;
            }
        }
        if (!payload_valid) payload_status = "invalid";
    }
    std::printf(
        "%s: version=%u flag=%u parts=%zu strings=%zu entries=%zu "
        "trailing=%zu exact=%s payload=%s\n",
        hdr_path.c_str(), index.version, index.flag, index.ark_part_sizes.size(),
        index.string_offsets.size(), index.entries.size(),
        index.trailing_bytes.size(),
        exact ? "yes" : "no", payload_status.c_str());
    return exact && payload_valid ? 0 : 1;
}

static int cmd_list(int argc, char** argv) {
    if (argc < 1) usage();
    std::string hdr = argv[0];
    bool ext_summary = false;
    int limit = 25;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--ext-summary") ext_summary = true;
        else if (a == "--limit" && i + 1 < argc) limit = std::atoi(argv[++i]);
        else die("unknown arg: " + a);
    }
    auto ark = gh::ark::ArkV3Reader::load(hdr);
    std::printf("HDR: %s\n", hdr.c_str());
    std::printf("  version       : %u\n", ark.version());
    std::printf("  ark_parts     : %zu\n", ark.ark_part_sizes().size());
    for (size_t i = 0; i < ark.ark_part_sizes().size(); ++i) {
        std::printf("    main_%zu.ark   : %llu bytes\n", i,
                    (unsigned long long)ark.ark_part_sizes()[i]);
    }
    std::printf("  entry_count   : %zu\n", ark.entries().size());

    if (ext_summary) {
        std::map<std::string, size_t> counts;
        for (const auto& e : ark.entries()) {
            auto dot = e.name.rfind('.');
            std::string ext = (dot == std::string::npos) ? "<noext>" : e.name.substr(dot + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            counts[ext]++;
        }
        std::printf("\nExtension summary:\n");
        std::vector<std::pair<std::string, size_t>> v(counts.begin(), counts.end());
        std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
        for (auto& [ext, n] : v) {
            std::printf("  .%-12s  %zu\n", ext.c_str(), n);
        }
    }

    std::printf("\nFirst %d entries:\n", limit);
    int shown = 0;
    for (const auto& e : ark.entries()) {
        if (shown >= limit) break;
        std::printf("  part=%u off=0x%010llx size=%10u  %s\n", e.ark_part,
                    (unsigned long long)e.offset, e.size, e.full_path.c_str());
        ++shown;
    }
    return 0;
}

static int cmd_extract(int argc, char** argv, bool all) {
    if (argc < 1) usage();
    std::string hdr = argv[0];
    std::vector<std::string> arks;
    std::string path_arg, out_arg;
    int i = 1;
    for (; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--path" && i + 1 < argc) { path_arg = argv[++i]; continue; }
        if (a == "--out"  && i + 1 < argc) { out_arg  = argv[++i]; continue; }
        arks.push_back(a);
    }
    if (arks.empty()) die("need at least one ARK path");
    if (out_arg.empty()) die("--out required");
    if (!all && path_arg.empty()) die("--path required for extract");

    auto ark = gh::ark::ArkV3Reader::load(hdr);
    if (arks.size() != ark.ark_part_sizes().size()) {
        std::fprintf(stderr,
            "warning: HDR says %zu ark parts, got %zu paths; using what was given\n",
            ark.ark_part_sizes().size(), arks.size());
    }

    if (!all) {
        auto e = ark.find(path_arg);
        if (!e) die("path not found in ARK: " + path_arg);
        auto bytes = ark.read_entry(*e, arks);
        fs::create_directories(fs::path(out_arg).parent_path());
        std::ofstream o(out_arg, std::ios::binary);
        o.write(reinterpret_cast<const char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size()));
        std::printf("wrote %zu bytes to %s\n", bytes.size(), out_arg.c_str());
        return 0;
    }

    fs::create_directories(out_arg);
    size_t ok = 0, fail = 0;
    for (const auto& e : ark.entries()) {
        try {
            auto bytes = ark.read_entry(e, arks);
            fs::path dst = fs::path(out_arg) / e.full_path;
            fs::create_directories(dst.parent_path());
            std::ofstream o(dst, std::ios::binary);
            o.write(reinterpret_cast<const char*>(bytes.data()),
                    static_cast<std::streamsize>(bytes.size()));
            ++ok;
        } catch (const std::exception& ex) {
            std::fprintf(stderr, "fail %s: %s\n", e.full_path.c_str(), ex.what());
            ++fail;
        }
    }
    std::printf("extracted %zu, failed %zu\n", ok, fail);
    return fail == 0 ? 0 : 1;
}

static int cmd_extract_prefix(int argc, char** argv) {
    if (argc < 1) usage();
    const std::string hdr = argv[0];
    std::vector<std::string> arks;
    std::vector<std::string> prefixes;
    std::string out_arg;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--prefix" && i + 1 < argc) {
            std::string prefix = argv[++i];
            std::replace(prefix.begin(), prefix.end(), '\\', '/');
            while (prefix.rfind("./", 0) == 0) prefix.erase(0, 2);
            const fs::path parsed(prefix);
            const std::string normalized =
                parsed.lexically_normal().generic_string();
            if (normalized.empty() || normalized == "." ||
                normalized == ".." || parsed.is_absolute() ||
                normalized.rfind("../", 0) == 0) {
                die("prefix must be ARK-relative: " + prefix);
            }
            prefix = normalized;
            if (prefix.back() != '/') prefix.push_back('/');
            prefixes.push_back(prefix);
        } else if (argument == "--out" && i + 1 < argc) {
            out_arg = argv[++i];
        } else {
            arks.push_back(argument);
        }
    }
    if (arks.empty()) die("need at least one ARK path");
    if (prefixes.empty()) die("at least one --prefix is required");
    if (out_arg.empty()) die("--out required");
    std::sort(prefixes.begin(), prefixes.end());
    prefixes.erase(std::unique(prefixes.begin(), prefixes.end()),
                   prefixes.end());

    const auto ark = gh::ark::ArkV3Reader::load(hdr);
    fs::create_directories(out_arg);
    std::size_t extracted = 0;
    for (const auto& entry : ark.entries()) {
        const bool selected = std::any_of(
            prefixes.begin(), prefixes.end(), [&](const std::string& prefix) {
                return entry.full_path.rfind(prefix, 0) == 0;
            });
        if (!selected) continue;
        const fs::path relative =
            fs::path(entry.full_path).lexically_normal();
        if (relative.empty() || relative.is_absolute() ||
            *relative.begin() == "..") {
            die("ARK entry escapes output root: " + entry.full_path);
        }
        const auto bytes = ark.read_entry(entry, arks);
        const fs::path destination = fs::path(out_arg) / relative;
        fs::create_directories(destination.parent_path());
        std::ofstream output(destination, std::ios::binary);
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        if (!output) die("cannot write: " + destination.string());
        ++extracted;
    }
    if (extracted == 0) die("no entries matched requested prefixes");
    std::printf("extracted %zu entries below %zu prefix(es)\n", extracted,
                prefixes.size());
    return 0;
}

static int cmd_paths(int argc, char** argv) {
    if (argc != 1) usage();
    const auto ark = gh::ark::ArkV3Reader::load(argv[0]);
    for (const auto& entry : ark.entries())
        std::printf("%s\n", entry.full_path.c_str());
    return 0;
}

static int cmd_pack(int argc, char** argv) {
    if (argc < 1) usage();
    const fs::path root = fs::absolute(argv[0]).lexically_normal();
    fs::path hdr_path;
    fs::path ark_path;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--hdr" && i + 1 < argc) {
            hdr_path = argv[++i];
        } else if (argument == "--ark" && i + 1 < argc) {
            ark_path = argv[++i];
        } else {
            die("unknown pack argument: " + argument);
        }
    }
    if (!fs::is_directory(root)) die("pack root is not a directory");
    if (hdr_path.empty() || ark_path.empty())
        die("pack requires --hdr and --ark");
    std::vector<std::pair<std::string, fs::path>> files;
    std::error_code error;
    for (fs::recursive_directory_iterator it(root, error), end;
         it != end && !error; it.increment(error)) {
        if (!it->is_regular_file(error)) continue;
        const fs::path relative = fs::relative(it->path(), root, error);
        if (error) break;
        files.emplace_back(relative.generic_string(), it->path());
    }
    if (error) die("cannot enumerate pack root");
    std::sort(files.begin(), files.end(),
              [](const auto& left, const auto& right) {
                  return left.first < right.first;
              });
    if (files.empty()) die("pack root has no files");
    fs::create_directories(ark_path.parent_path());
    fs::create_directories(hdr_path.parent_path());
    std::ofstream archive(ark_path, std::ios::binary);
    if (!archive) die("cannot create ARK: " + ark_path.string());
    std::vector<gh::ark::LayoutEntry> layout;
    std::uint64_t cursor = 0;
    for (const auto& [relative, path] : files) {
        const auto bytes = read_all(path.string());
        if (cursor + bytes.size() > UINT32_MAX)
            die("packed ARK exceeds v3 32-bit layout capacity");
        archive.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        if (!archive) die("cannot write ARK: " + ark_path.string());
        layout.push_back({relative, static_cast<std::uint32_t>(cursor),
                          static_cast<std::uint32_t>(bytes.size()), 0});
        cursor += bytes.size();
    }
    archive.close();
    const auto index = gh::ark::make_index(
        {static_cast<std::uint32_t>(cursor)}, layout, 1);
    const auto header = gh::ark::serialize_index(index);
    std::ofstream header_stream(hdr_path, std::ios::binary);
    header_stream.write(reinterpret_cast<const char*>(header.data()),
                        static_cast<std::streamsize>(header.size()));
    if (!header_stream) die("cannot write HDR: " + hdr_path.string());
    header_stream.close();
    const auto verify = gh::ark::ArkV3Reader::load(hdr_path.string());
    if (verify.entries().size() != files.size() ||
        verify.ark_part_sizes().size() != 1 ||
        verify.ark_part_sizes()[0] != cursor)
        die("packed archive verification failed");
    std::printf("packed %zu entries, %llu bytes\n", files.size(),
                static_cast<unsigned long long>(cursor));
    return 0;
}

static std::vector<std::string> read_overlay_manifest(
    const fs::path& manifest_path) {
    std::ifstream input(manifest_path, std::ios::binary);
    if (!input) die("cannot open manifest: " + manifest_path.string());
    std::vector<std::string> paths;
    std::set<std::string> unique;
    std::string line;
    bool first = true;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (first) {
            first = false;
            if (line.rfind("relative_path\t", 0) == 0) continue;
        }
        const size_t tab = line.find('\t');
        const std::string path = line.substr(0, tab);
        if (path.empty()) continue;
        if (!unique.insert(path).second)
            die("duplicate manifest path: " + path);
        paths.push_back(path);
    }
    if (paths.empty()) die("overlay manifest is empty");
    std::sort(paths.begin(), paths.end());
    return paths;
}

static int cmd_overlay(int argc, char** argv) {
    if (argc < 2) usage();
    const fs::path hdr_path = argv[0];
    const fs::path ark_path = argv[1];
    fs::path root_path;
    fs::path manifest_path;
    for (int i = 2; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--root" && i + 1 < argc) {
            root_path = argv[++i];
        } else if (argument == "--manifest" && i + 1 < argc) {
            manifest_path = argv[++i];
        } else {
            die("unknown overlay argument: " + argument);
        }
    }
    if (root_path.empty() || manifest_path.empty())
        die("overlay requires --root and --manifest");

    const auto reader = gh::ark::ArkV3Reader::load(hdr_path.string());
    if (reader.ark_part_sizes().size() != 1)
        die("overlay currently requires a single-part ARK");
    const uintmax_t source_size = fs::file_size(ark_path);
    if (source_size > UINT32_MAX)
        die("overlay ARK exceeds v3 32-bit layout capacity");

    std::map<std::string, std::vector<uint8_t>> overlay;
    const fs::path canonical_root = fs::weakly_canonical(root_path);
    for (const auto& relative_text :
         read_overlay_manifest(manifest_path)) {
        const fs::path relative =
            fs::path(relative_text).lexically_normal();
        if (relative.empty() || relative.is_absolute() ||
            *relative.begin() == "..")
            die("manifest path escapes overlay root: " + relative_text);
        const fs::path source =
            fs::weakly_canonical(canonical_root / relative);
        auto root_it = canonical_root.begin();
        auto source_it = source.begin();
        for (; root_it != canonical_root.end() &&
               source_it != source.end() && *root_it == *source_it;
             ++root_it, ++source_it) {
        }
        if (root_it != canonical_root.end())
            die("manifest path escapes overlay root: " + relative_text);
        overlay.emplace(relative_text, read_all(source.string()));
    }

    std::vector<gh::ark::LayoutEntry> layout;
    layout.reserve(reader.entries().size() + overlay.size());
    std::vector<std::vector<uint8_t>> appended;
    uint64_t cursor = source_size;
    size_t reused = 0;
    size_t replaced = 0;
    for (const auto& entry : reader.entries()) {
        const auto found = overlay.find(entry.full_path);
        if (found == overlay.end()) {
            layout.push_back(
                {entry.full_path, static_cast<uint32_t>(entry.offset),
                 entry.size, entry.inflated_size});
            continue;
        }
        const auto current =
            reader.read_entry(entry, {ark_path.string()});
        if (current == found->second) {
            layout.push_back(
                {entry.full_path, static_cast<uint32_t>(entry.offset),
                 entry.size, entry.inflated_size});
            ++reused;
        } else {
            if (cursor + found->second.size() > UINT32_MAX)
                die("overlay exceeds v3 32-bit layout capacity");
            layout.push_back(
                {entry.full_path, static_cast<uint32_t>(cursor),
                 static_cast<uint32_t>(found->second.size()), 0});
            cursor += found->second.size();
            appended.push_back(found->second);
            ++replaced;
        }
        overlay.erase(found);
    }
    const size_t added = overlay.size();
    for (const auto& [path, bytes] : overlay) {
        if (cursor + bytes.size() > UINT32_MAX)
            die("overlay exceeds v3 32-bit layout capacity");
        layout.push_back(
            {path, static_cast<uint32_t>(cursor),
             static_cast<uint32_t>(bytes.size()), 0});
        cursor += bytes.size();
        appended.push_back(bytes);
    }

    if (!appended.empty()) {
        std::ofstream ark_output(
            ark_path, std::ios::binary | std::ios::app);
        if (!ark_output) die("cannot append ARK: " + ark_path.string());
        for (const auto& bytes : appended)
            ark_output.write(
                reinterpret_cast<const char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size()));
        if (!ark_output) die("cannot append overlay bytes");
    }

    const auto index = gh::ark::make_index(
        {static_cast<uint32_t>(cursor)}, layout, reader.flag());
    const auto header_bytes = gh::ark::serialize_index(index);
    const fs::path backup_path =
        hdr_path.string() + ".pre-overlay.bak";
    if (!fs::exists(backup_path))
        fs::copy_file(hdr_path, backup_path);
    const fs::path temporary_path =
        hdr_path.string() + ".overlay.tmp";
    {
        std::ofstream output(temporary_path, std::ios::binary);
        if (!output)
            die("cannot create overlay header: " +
                temporary_path.string());
        output.write(
            reinterpret_cast<const char*>(header_bytes.data()),
            static_cast<std::streamsize>(header_bytes.size()));
        if (!output) die("cannot write overlay header");
    }
    fs::copy_file(
        temporary_path, hdr_path,
        fs::copy_options::overwrite_existing);
    fs::remove(temporary_path);

    const auto verify = gh::ark::ArkV3Reader::load(hdr_path.string());
    if (verify.ark_part_sizes().size() != 1 ||
        verify.ark_part_sizes().front() != cursor)
        die("overlay header verification failed");
    std::printf(
        "overlay reused=%zu replaced=%zu added=%zu appended=%llu "
        "entries=%zu\n",
        reused, replaced, added,
        static_cast<unsigned long long>(cursor - source_size),
        verify.entries().size());
    return 0;
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) usage();
        std::string sub = argv[1];
        if (sub == "list")        return cmd_list(argc - 2, argv + 2);
        if (sub == "paths")       return cmd_paths(argc - 2, argv + 2);
        if (sub == "verify")      return cmd_verify(argc - 2, argv + 2);
        if (sub == "extract")
            return cmd_extract(argc - 2, argv + 2, /*all=*/false);
        if (sub == "extract-all")
            return cmd_extract(argc - 2, argv + 2, /*all=*/true);
        if (sub == "extract-prefix")
            return cmd_extract_prefix(argc - 2, argv + 2);
        if (sub == "pack")
            return cmd_pack(argc - 2, argv + 2);
        if (sub == "overlay")
            return cmd_overlay(argc - 2, argv + 2);
        usage();
        return 2;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "ark_tool: %s\n", ex.what());
        return 1;
    }
}
