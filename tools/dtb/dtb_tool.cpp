// dtb_tool - CLI for Harmonix DTB inspection.
//
// Usage:
//   dtb_tool dump <file.dtb> [--lines]   Print as DTA-style text.
//   dtb_tool info <file.dtb>             Print metadata (encryption, sizes, root count).

#include "dtb.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static void usage() {
    std::fprintf(stderr,
        "Usage:\n"
        "  dtb_tool dump <file.dtb> [--lines]\n"
        "  dtb_tool info <file.dtb>\n");
    std::exit(2);
}

int main(int argc, char** argv) {
    if (argc < 3) usage();
    std::string sub = argv[1];
    std::string path = argv[2];

    try {
        auto bytes = gh::dtb::read_file(path);
        bool encrypted_guess = !bytes.empty() && bytes[0] != 0x01;

        auto tree = gh::dtb::parse(bytes);

        if (sub == "info") {
            std::printf("file        : %s\n", path.c_str());
            std::printf("size        : %zu bytes\n", bytes.size());
            std::printf("encrypted   : %s\n", encrypted_guess ? "yes (PS2 cipher)" : "no");
            std::printf("embedded    : %s\n", tree.embedded ? "yes (version=0)"
                                                            : "no  (version=1)");
            std::printf("root_count  : %zu\n", tree.root.size());
            return 0;
        }
        if (sub == "dump") {
            bool show_lines = false;
            for (int i = 3; i < argc; ++i) {
                if (std::strcmp(argv[i], "--lines") == 0) show_lines = true;
                else { std::fprintf(stderr, "unknown arg: %s\n", argv[i]); return 2; }
            }
            std::cout << gh::dtb::to_dta(tree, show_lines);
            return 0;
        }
        usage();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "dtb_tool: %s\n", e.what());
        return 1;
    }
    return 0;
}
