#include "milo_object.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open " + path);
    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
}

void print_property(
    const gh::milo_object::TypePropertyNode& node, unsigned depth) {
    std::printf("%*stype=0x%02x", depth * 2, "", node.type);
    if (node.type == 0) std::printf(" int=%d", static_cast<int32_t>(node.integer));
    else if (node.type == 1) std::printf(" float=%.9g", node.floating);
    else if (!node.symbol.empty()) std::printf(" symbol=%s", node.symbol.c_str());
    std::printf("\n");
    for (const auto& child : node.children)
        print_property(child, depth + 1);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3 || std::string(argv[1]) != "camshot20") {
        std::fprintf(
            stderr,
            "Usage: milo_object_inspect camshot20 <object-body>\n");
        return 2;
    }
    try {
        const auto shot =
            gh::milo_object::parse_cam_shot20(read_file(argv[2]));
        std::printf(
            "revision=%u object_type=%s anim_rate=%d keyframes=%zu "
            "looping=%d near=%.9g far=%.9g dof=%d filter=%.9g "
            "clamp=%.9g path=%s path_frame=%.9g category=%s "
            "crowd_pairs=%zu hide=%zu glow=%s\n",
            shot.revision, shot.object_fields.type.c_str(),
            shot.animatable.rate, shot.keyframes.size(),
            shot.looping ? 1 : 0, shot.near_plane, shot.far_plane,
            shot.use_depth_of_field ? 1 : 0, shot.filter,
            shot.clamp_height, shot.path.c_str(),
            shot.legacy_path_frame, shot.category.c_str(),
            shot.legacy_crowd_pairs.size(), shot.hide_list.size(),
            shot.glow_spot.c_str());
        for (size_t i = 0; i < shot.keyframes.size(); ++i) {
            const auto& frame = shot.keyframes[i];
            std::printf(
                "frame=%zu duration=%.9g blend=%.9g ease=%.9g "
                "fov=%.9g pos=(%.9g %.9g %.9g) "
                "screen=(%.9g %.9g) targets=%zu "
                "parent=%s:%s parent_rotation=%d "
                "shake=(%.9g %.9g %.9g %.9g)\n",
                i, frame.duration, frame.blend, frame.blend_ease,
                frame.field_of_view, frame.world_offset[9],
                frame.world_offset[10], frame.world_offset[11],
                frame.screen_offset[0], frame.screen_offset[1],
                frame.targets.size(), frame.parent.object.c_str(),
                frame.parent.part.c_str(),
                frame.use_parent_rotation ? 1 : 0,
                frame.shake_noise_amplitude,
                frame.shake_noise_frequency,
                frame.maximum_angular_offset[0],
                frame.maximum_angular_offset[1]);
        }
        for (const auto& property :
             shot.object_fields.type_properties) {
            print_property(property, 0);
        }
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "milo_object_inspect: %s\n", ex.what());
        return 1;
    }
}
