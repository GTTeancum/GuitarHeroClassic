// ark_tool - CLI for inspecting and extracting Harmonix v3 ARKs.
//
// Subcommands:
//   list      <hdr> [--ext-summary] [--limit N]
//   verify    <hdr>
//   extract   <hdr> <ark>... --path <full_path> --out <file>
//   extract-all <hdr> <ark>... --out <dir>

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
        "  ark_tool verify <hdr>\n"
        "  ark_tool extract <hdr> <ark>... --path <full_path> --out <file>\n"
        "  ark_tool extract-all <hdr> <ark>... --out <dir>\n"
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
    if (argc != 1) usage();
    const std::string hdr_path = argv[0];
    const auto source = read_all(hdr_path);
    const auto index = gh::ark::parse_index(source);
    const auto serialized = gh::ark::serialize_index(index);
    const bool exact = source == serialized;
    std::printf(
        "%s: version=%u flag=%u parts=%zu strings=%zu entries=%zu "
        "trailing=%zu exact=%s\n",
        hdr_path.c_str(), index.version, index.flag, index.ark_part_sizes.size(),
        index.string_offsets.size(), index.entries.size(),
        index.trailing_bytes.size(),
        exact ? "yes" : "no");
    return exact ? 0 : 1;
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
        if (sub == "verify")      return cmd_verify(argc - 2, argv + 2);
        if (sub == "extract")
            return cmd_extract(argc - 2, argv + 2, /*all=*/false);
        if (sub == "extract-all")
            return cmd_extract(argc - 2, argv + 2, /*all=*/true);
        if (sub == "overlay")
            return cmd_overlay(argc - 2, argv + 2);
        usage();
        return 2;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "ark_tool: %s\n", ex.what());
        return 1;
    }
}
