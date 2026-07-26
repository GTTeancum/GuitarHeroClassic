// GH1 external character animation transition graph (.acg) reader/writer.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gh::acg {

// A transition from the owning/source clip to target_clip_index. The beats
// identify where blending leaves the current clip and enters the target clip.
struct Node {
    uint32_t target_clip_index = 0;
    float current_beat = 0.0f;
    float next_beat = 0.0f;
};

struct ClipTransitions {
    std::vector<Node> nodes;
};

struct Graph {
    uint32_t version = 1;
    // Position in this vector is the source clip index. Node targets use the
    // same index space; clip names and sample paths live in the paired ACS.
    std::vector<ClipTransitions> clips;
    std::vector<uint8_t> trailing_bytes;
};

Graph parse(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize(const Graph& graph);
std::vector<uint8_t> read_file(const std::string& path);

}  // namespace gh::acg
