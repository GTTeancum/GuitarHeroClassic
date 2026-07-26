#include "acg.h"

#include <cstdio>
#include <exception>
#include <string>

namespace {

void usage() {
    std::fprintf(stderr,
                 "Usage:\n"
                 "  acg_tool info <file.acg>\n"
                 "  acg_tool verify <file.acg>\n");
}

size_t node_count(const gh::acg::Graph& graph) {
    size_t result = 0;
    for (const auto& clip : graph.clips) result += clip.nodes.size();
    return result;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            usage();
            return 2;
        }
        const std::string command = argv[1];
        const auto source = gh::acg::read_file(argv[2]);
        const auto graph = gh::acg::parse(source);
        if (command == "info") {
            std::printf("version=%u clips=%zu nodes=%zu trailing=%zu\n",
                        graph.version, graph.clips.size(), node_count(graph),
                        graph.trailing_bytes.size());
            return 0;
        }
        if (command == "verify") {
            const auto serialized = gh::acg::serialize(graph);
            const bool exact = source == serialized;
            std::printf(
                "%s: version=%u clips=%zu nodes=%zu trailing=%zu exact=%s\n",
                argv[2], graph.version, graph.clips.size(), node_count(graph),
                graph.trailing_bytes.size(), exact ? "yes" : "no");
            return exact ? 0 : 1;
        }
        usage();
        return 2;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "acg_tool: %s\n", ex.what());
        return 1;
    }
}
