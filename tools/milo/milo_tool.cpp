// milo_tool - CLI for Harmonix .milo container inspection.
//
// Usage:
//   milo_tool info    <file>             header only
//   milo_tool list    <file>             header + decompress + list (type,name,size)
//   milo_tool extract <file> --out <dir> decompress + dump each entry's raw bytes

#include "milo.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static void usage() {
    std::fprintf(stderr,
        "Usage:\n"
        "  milo_tool info    <file>\n"
        "  milo_tool list    <file>\n"
        "  milo_tool extract <file> --out <dir>\n");
    std::exit(2);
}

int main(int argc, char** argv) {
    if (argc < 3) usage();
    std::string sub = argv[1];
    std::string path = argv[2];
    try {
        auto bytes = gh::milo::read_file(path);
        auto h = gh::milo::parse_header(bytes);

        if (sub == "info") {
            std::printf("file              : %s\n", path.c_str());
            std::printf("size              : %zu bytes\n", bytes.size());
            std::printf("structure         : 0x%08X  (%s)\n",
                        static_cast<uint32_t>(h.structure),
                        gh::milo::block_structure_name(h.structure));
            std::printf("first_block_off   : 0x%X (%u)\n",
                        h.first_block_offset, h.first_block_offset);
            std::printf("block_count       : %u\n", h.block_count);
            std::printf("max_uncompressed  : %u bytes\n", h.max_block_uncompressed_size);
            for (uint32_t i = 0; i < h.block_count; ++i) {
                std::printf("  block[%u] size = %u\n", i, h.block_sizes[i]);
            }
            return 0;
        }

        auto payload = gh::milo::inflate_payload(bytes, h);
        if (sub == "list") {
            std::printf("decompressed payload: %zu bytes\n", payload.size());
            auto d = gh::milo::parse_directory(payload);
            std::printf("dir_version : %d\n", d.dir_version);
            std::printf("dir_type    : %s\n", d.dir_type.c_str());
            std::printf("dir_name    : %s\n", d.dir_name.c_str());
            std::printf("entries     : %zu\n", d.entries.size());

            std::map<std::string, size_t> by_type;
            size_t total_body_bytes = 0;
            for (const auto& e : d.entries) {
                by_type[e.type]++;
                total_body_bytes += e.size;
            }
            std::printf("\nType summary:\n");
            for (auto& [t, n] : by_type) {
                std::printf("  %-22s  %zu\n", t.c_str(), n);
            }
            std::printf("\nEntries:\n");
            for (const auto& e : d.entries) {
                std::printf("  %-22s  size=%-8llu  %s\n",
                            e.type.c_str(),
                            (unsigned long long)e.size, e.name.c_str());
            }
            std::printf("\ntotal body bytes (sum of entry sizes): %zu\n", total_body_bytes);
            return 0;
        }
        if (sub == "extract") {
            std::string outdir;
            for (int i = 3; i < argc; ++i) {
                if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc) { outdir = argv[++i]; continue; }
                std::fprintf(stderr, "unknown arg: %s\n", argv[i]); return 2;
            }
            if (outdir.empty()) { std::fprintf(stderr, "--out required\n"); return 2; }
            auto d = gh::milo::parse_directory(payload);
            fs::create_directories(outdir);
            // Also write the full decompressed payload for reference / diffing.
            {
                std::ofstream o(fs::path(outdir) / "_payload.bin", std::ios::binary);
                o.write(reinterpret_cast<const char*>(payload.data()),
                        static_cast<std::streamsize>(payload.size()));
            }
            int ok = 0;
            for (const auto& e : d.entries) {
                if (e.offset + e.size > payload.size()) continue;
                std::string safe = e.name;
                for (auto& c : safe) { if (c == '/' || c == '\\') c = '_'; }
                fs::path dst = fs::path(outdir) / (e.type + "__" + safe);
                std::ofstream o(dst, std::ios::binary);
                o.write(reinterpret_cast<const char*>(payload.data() + e.offset),
                        static_cast<std::streamsize>(e.size));
                ++ok;
            }
            std::printf("extracted %d entries -> %s\n", ok, outdir.c_str());
            return 0;
        }
        usage();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "milo_tool: %s\n", e.what());
        return 1;
    }
    return 0;
}
