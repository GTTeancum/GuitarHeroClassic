#include "acg.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace {

void append_u32(std::vector<uint8_t>& bytes, uint32_t value) {
    bytes.push_back(static_cast<uint8_t>(value));
    bytes.push_back(static_cast<uint8_t>(value >> 8));
    bytes.push_back(static_cast<uint8_t>(value >> 16));
    bytes.push_back(static_cast<uint8_t>(value >> 24));
}

void test_exact_round_trip() {
    std::vector<uint8_t> source;
    append_u32(source, 1);
    append_u32(source, 2);
    append_u32(source, 2);
    append_u32(source, 0);
    append_u32(source, 0x3f800000);
    append_u32(source, 0xbf000000);
    append_u32(source, 1);
    append_u32(source, 0x40000000);
    append_u32(source, 0x40400000);
    append_u32(source, 1);
    append_u32(source, 0);
    append_u32(source, 0x40800000);
    append_u32(source, 0x40a00000);
    source.push_back(0xaa);
    source.push_back(0x55);

    const auto graph = gh::acg::parse(source);
    assert(graph.version == 1);
    assert(graph.clips.size() == 2);
    assert(graph.clips[0].nodes.size() == 2);
    assert(graph.clips[1].nodes.size() == 1);
    assert(graph.clips[0].nodes[1].target_clip_index == 1);
    assert(graph.clips[0].nodes[1].current_beat == 2.0f);
    assert(graph.clips[1].nodes[0].next_beat == 5.0f);
    assert(graph.trailing_bytes == std::vector<uint8_t>({0xaa, 0x55}));
    assert(gh::acg::serialize(graph) == source);
}

void test_invalid_target_rejected() {
    std::vector<uint8_t> source;
    append_u32(source, 1);
    append_u32(source, 1);
    append_u32(source, 1);
    append_u32(source, 1);
    append_u32(source, 0);
    append_u32(source, 0);
    bool threw = false;
    try {
        (void)gh::acg::parse(source);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_edit_and_reparse() {
    gh::acg::Graph graph;
    graph.clips.resize(1);
    graph.clips[0].nodes.push_back({0, 1.25f, -2.5f});
    auto bytes = gh::acg::serialize(graph);
    auto parsed = gh::acg::parse(bytes);
    assert(parsed.clips[0].nodes[0].current_beat == 1.25f);
    assert(parsed.clips[0].nodes[0].next_beat == -2.5f);
}

}  // namespace

int main() {
    test_exact_round_trip();
    test_invalid_target_rejected();
    test_edit_and_reparse();
    std::puts("acg_test: all checks passed");
    return 0;
}
