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
        "  ark_tool extract-all <hdr> <ark>... --out <dir>\n");
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
        usage();
        return 2;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "ark_tool: %s\n", ex.what());
        return 1;
    }
}
