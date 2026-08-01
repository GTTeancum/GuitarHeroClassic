#include "acg.h"

#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace gh::acg {
namespace {

class Reader {
public:
    explicit Reader(const std::vector<uint8_t>& bytes) : bytes_(bytes) {}

    uint32_t u32() {
        require(4);
        uint32_t value = 0;
        std::memcpy(&value, bytes_.data() + pos_, 4);
        pos_ += 4;
        return value;
    }

    float f32() {
        const uint32_t bits = u32();
        float value = 0.0f;
        std::memcpy(&value, &bits, 4);
        return value;
    }

    size_t remaining() const { return bytes_.size() - pos_; }

    std::vector<uint8_t> remaining_bytes() {
        std::vector<uint8_t> result(bytes_.begin() + pos_, bytes_.end());
        pos_ = bytes_.size();
        return result;
    }

private:
    void require(size_t size) const {
        if (size > bytes_.size() - pos_)
            throw std::runtime_error("ACG read past end");
    }

    const std::vector<uint8_t>& bytes_;
    size_t pos_ = 0;
};

void append_u32(std::vector<uint8_t>& bytes, uint32_t value) {
    bytes.push_back(static_cast<uint8_t>(value));
    bytes.push_back(static_cast<uint8_t>(value >> 8));
    bytes.push_back(static_cast<uint8_t>(value >> 16));
    bytes.push_back(static_cast<uint8_t>(value >> 24));
}

void append_f32(std::vector<uint8_t>& bytes, float value) {
    uint32_t bits = 0;
    static_assert(sizeof(value) == sizeof(bits), "float must be binary32");
    std::memcpy(&bits, &value, 4);
    append_u32(bytes, bits);
}

uint32_t checked_count(size_t value, const char* what) {
    if (value > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(std::string("ACG ") + what +
                                 " count exceeds u32");
    return static_cast<uint32_t>(value);
}

}  // namespace

Graph parse(const std::vector<uint8_t>& bytes) {
    Reader reader(bytes);
    Graph graph;
    graph.version = reader.u32();
    if (graph.version != 1)
        throw std::runtime_error("unsupported ACG version");

    const uint32_t clip_count = reader.u32();
    if (clip_count > reader.remaining() / 4)
        throw std::runtime_error("ACG clip count exceeds remaining bytes");
    graph.clips.resize(clip_count);
    for (uint32_t source = 0; source < clip_count; ++source) {
        const uint32_t node_count = reader.u32();
        if (node_count > reader.remaining() / 12)
            throw std::runtime_error("ACG node count exceeds remaining bytes");
        auto& nodes = graph.clips[source].nodes;
        nodes.reserve(node_count);
        for (uint32_t i = 0; i < node_count; ++i) {
            Node node;
            node.target_clip_index = reader.u32();
            node.current_beat = reader.f32();
            node.next_beat = reader.f32();
            if (node.target_clip_index >= clip_count)
                throw std::runtime_error(
                    "ACG target clip index outside clip table");
            nodes.push_back(node);
        }
    }
    graph.trailing_bytes = reader.remaining_bytes();
    return graph;
}

std::vector<uint8_t> serialize(const Graph& graph) {
    if (graph.version != 1)
        throw std::runtime_error("only ACG version 1 can be serialized");
    const uint32_t clip_count = checked_count(graph.clips.size(), "clip");

    std::vector<uint8_t> bytes;
    append_u32(bytes, graph.version);
    append_u32(bytes, clip_count);
    for (const auto& clip : graph.clips) {
        append_u32(bytes, checked_count(clip.nodes.size(), "node"));
        for (const auto& node : clip.nodes) {
            if (node.target_clip_index >= clip_count)
                throw std::runtime_error(
                    "ACG target clip index outside clip table");
            append_u32(bytes, node.target_clip_index);
            append_f32(bytes, node.current_beat);
            append_f32(bytes, node.next_beat);
        }
    }
    bytes.insert(bytes.end(), graph.trailing_bytes.begin(),
                 graph.trailing_bytes.end());
    return bytes;
}

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("cannot open ACG: " + path);
    const std::streamoff length = input.tellg();
    if (length < 0) throw std::runtime_error("cannot size ACG: " + path);
    input.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), length);
        if (!input) throw std::runtime_error("short read on ACG: " + path);
    }
    return bytes;
}

}  // namespace gh::acg
