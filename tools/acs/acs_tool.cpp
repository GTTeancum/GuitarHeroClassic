#include "acs.h"

#include <cstdio>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3 ||
        (std::string(argv[1]) != "info" &&
         std::string(argv[1]) != "verify")) {
        std::fprintf(stderr, "Usage: acs_tool <info|verify> <file.acs>\n");
        return 2;
    }
    try {
        const auto bytes = gh::acs::read_file(argv[2]);
        const auto file = gh::acs::parse(bytes);
        size_t includes = 0;
        size_t invocations = 0;
        for (const auto& line : file.lines) {
            if (line.kind == gh::acs::LineKind::Include) ++includes;
            if (line.kind == gh::acs::LineKind::Invocation) ++invocations;
        }
        if (std::string(argv[1]) == "verify" &&
            gh::acs::serialize(file) != bytes) {
            std::fprintf(stderr, "ACS round trip differs\n");
            return 1;
        }
        std::printf("lines=%zu includes=%zu invocations=%zu bytes=%zu\n",
                    file.lines.size(), includes, invocations, bytes.size());
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "acs_tool: %s\n", ex.what());
        return 1;
    }
}
