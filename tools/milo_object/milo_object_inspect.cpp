#include "milo_object.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>
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

template <typename MeshType>
void print_mesh_vertex_ranges(const MeshType& mesh) {
    std::array<float, 4> slot_min = {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()};
    std::array<float, 4> slot_max = {
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()};
    std::array<float, 4> loaded_min = slot_min;
    std::array<float, 4> loaded_max = slot_max;
    for (const auto& vertex : mesh.vertices) {
        for (size_t i = 0; i < slot_min.size(); ++i) {
            const float serialized = vertex.color_or_weights[i];
            const int packed = static_cast<int>(serialized * 255.0f);
            const float loaded = static_cast<float>(packed & 0xFF) / 255.0f;
            slot_min[i] = std::min(slot_min[i], serialized);
            slot_max[i] = std::max(slot_max[i], serialized);
            loaded_min[i] = std::min(loaded_min[i], loaded);
            loaded_max[i] = std::max(loaded_max[i], loaded);
        }
    }
    if (mesh.vertices.empty()) {
        slot_min.fill(0.0f);
        slot_max.fill(0.0f);
        loaded_min.fill(0.0f);
        loaded_max.fill(0.0f);
    }
    std::printf(
        "serialized_color_slot_range=(%.9g..%.9g %.9g..%.9g %.9g..%.9g %.9g..%.9g)\n"
        "source_color32_range=(%.9g..%.9g %.9g..%.9g %.9g..%.9g %.9g..%.9g)\n",
        slot_min[0], slot_max[0], slot_min[1], slot_max[1],
        slot_min[2], slot_max[2], slot_min[3], slot_max[3],
        loaded_min[0], loaded_max[0], loaded_min[1], loaded_max[1],
        loaded_min[2], loaded_max[2], loaded_min[3], loaded_max[3]);
}

template <typename MeshType>
void print_mesh_vertices(const MeshType& mesh) {
    for (size_t index = 0; index < mesh.vertices.size(); ++index) {
        const auto& vertex = mesh.vertices[index];
        std::printf(
            "vertex[%zu] pos=(%.9g %.9g %.9g) normal=(%.9g %.9g %.9g) "
            "slot=(%.9g %.9g %.9g %.9g) uv=(%.9g %.9g)\n",
            index, vertex.position[0], vertex.position[1], vertex.position[2],
            vertex.normal[0], vertex.normal[1], vertex.normal[2],
            vertex.color_or_weights[0], vertex.color_or_weights[1],
            vertex.color_or_weights[2], vertex.color_or_weights[3],
            vertex.uv[0], vertex.uv[1]);
    }
    for (size_t index = 0; index < mesh.faces.size(); ++index) {
        const auto& face = mesh.faces[index];
        std::printf(
            "face[%zu]=(%u %u %u)\n", index,
            static_cast<unsigned>(face[0]),
            static_cast<unsigned>(face[1]),
            static_cast<unsigned>(face[2]));
    }
}

}  // namespace

int main(int argc, char** argv) {
    const bool print_vertices =
        argc == 4 && std::string(argv[3]) == "--vertices";
    if (argc != 3 && !print_vertices) {
        std::fprintf(
            stderr,
            "Usage: milo_object_inspect "
            "<animfilter1|matanim5|bandcharacter1|camshot20|transanim4|transanim6|view7|group12|waypoint3|"
            "mesh25|mesh28|worldfx1> "
            "<object-body> [--vertices]\n");
        return 2;
    }
    try {
        if (std::string(argv[1]) == "animfilter1") {
            const auto filter =
                gh::milo_object::parse_anim_filter1(read_file(argv[2]));
            std::printf(
                "revision=%u object_type=%s anim_revision=%u "
                "frame=%.9g rate=%d anim=%s scale=%.9g offset=%.9g "
                "start=%.9g end=%.9g type=%d period=%.9g\n",
                filter.revision, filter.object_fields.type.c_str(),
                filter.animatable.revision, filter.animatable.frame,
                filter.animatable.rate, filter.anim.c_str(), filter.scale,
                filter.offset, filter.start, filter.end, filter.type,
                filter.period);
            return 0;
        }
        if (std::string(argv[1]) == "matanim5") {
            const auto anim =
                gh::milo_object::parse_mat_anim(read_file(argv[2]));
            std::printf(
                "revision=%u anim_revision=%u material=%s keys_owner=%s "
                "operations=%zu objects=%zu stages=%zu color_keys=%zu "
                "alpha_keys=%zu\n",
                anim.revision, anim.animatable.revision,
                anim.material.c_str(), anim.keys_owner.c_str(),
                anim.animatable.operations.size(),
                anim.animatable.objects.size(), anim.stages.size(),
                anim.color_keys.size(), anim.alpha_keys.size());
            for (size_t i = 0; i < anim.animatable.operations.size(); ++i) {
                const auto& op = anim.animatable.operations[i];
                std::printf(
                    "operation=%zu type=%u first=%.9g second=%.9g loop=%d "
                    "integers=(%d %d %d)\n",
                    i, op.type, op.first, op.second, op.loop ? 1 : 0,
                    op.integers[0], op.integers[1], op.integers[2]);
            }
            for (size_t i = 0; i < anim.animatable.objects.size(); ++i)
                std::printf(
                    "object=%zu name=%s\n", i,
                    anim.animatable.objects[i].c_str());
            for (size_t stage_index = 0;
                 stage_index < anim.stages.size(); ++stage_index) {
                const auto& stage = anim.stages[stage_index];
                std::printf(
                    "stage=%zu translation_keys=%zu scale_keys=%zu "
                    "rotation_keys=%zu texture_keys=%zu\n",
                    stage_index, stage.translation_keys.size(),
                    stage.scale_keys.size(), stage.rotation_keys.size(),
                    stage.texture_keys.size());
                const auto print_vec3_keys =
                    [stage_index](
                        const char* kind,
                        const std::vector<gh::milo_object::Vec3Key>& keys) {
                        for (size_t i = 0; i < keys.size(); ++i)
                            std::printf(
                                "stage=%zu %s=%zu frame=%.9g "
                                "value=(%.9g %.9g %.9g)\n",
                                stage_index, kind, i, keys[i].frame,
                                keys[i].value[0], keys[i].value[1],
                                keys[i].value[2]);
                    };
                print_vec3_keys("translation_key", stage.translation_keys);
                print_vec3_keys("scale_key", stage.scale_keys);
                print_vec3_keys("rotation_key", stage.rotation_keys);
                for (size_t i = 0; i < stage.texture_keys.size(); ++i)
                    std::printf(
                        "stage=%zu texture_key=%zu frame=%.9g object=%s\n",
                        stage_index, i, stage.texture_keys[i].frame,
                        stage.texture_keys[i].object.c_str());
            }
            for (size_t i = 0; i < anim.color_keys.size(); ++i)
                std::printf(
                    "color_key=%zu frame=%.9g value=(%.9g %.9g %.9g %.9g)\n",
                    i, anim.color_keys[i].frame,
                    anim.color_keys[i].value[0],
                    anim.color_keys[i].value[1],
                    anim.color_keys[i].value[2],
                    anim.color_keys[i].value[3]);
            for (size_t i = 0; i < anim.alpha_keys.size(); ++i)
                std::printf(
                    "alpha_key=%zu frame=%.9g value=%.9g\n",
                    i, anim.alpha_keys[i].frame,
                    anim.alpha_keys[i].value);
            return 0;
        }
        if (std::string(argv[1]) == "worldfx1") {
            const auto fx =
                gh::milo_object::parse_world_fx1(read_file(argv[2]));
            const auto& dir = fx.render_directory;
            const auto& objects = dir.object_directory;
            std::printf(
                "revision=%u rnd_dir_revision=%u object_dir_revision=%u "
                "object_type=%s subdirectories=%zu "
                "proxy_path=%s environment=%s test_event=%s "
                "legacy_symbol_1=%s legacy_symbol_2=%s "
                "showing=%d draw_order=%.9g "
                "frame=%.9g rate=%d parent=%s "
                "local=(%.9g %.9g %.9g) world=(%.9g %.9g %.9g)\n",
                fx.revision, dir.revision, objects.revision,
                objects.object_fields.type.c_str(),
                objects.subdirectories.size(), objects.proxy_path.c_str(),
                dir.environment.c_str(), dir.test_event.c_str(),
                dir.legacy_symbol_1.c_str(), dir.legacy_symbol_2.c_str(),
                dir.drawable.showing ? 1 : 0, dir.drawable.draw_order,
                dir.animatable.frame, dir.animatable.rate,
                dir.transformable.parent.c_str(),
                dir.transformable.local[9],
                dir.transformable.local[10],
                dir.transformable.local[11],
                dir.transformable.world[9],
                dir.transformable.world[10],
                dir.transformable.world[11]);
            for (size_t i = 0; i < objects.subdirectories.size(); ++i) {
                std::printf(
                    "subdirectory=%zu path=%s\n", i,
                    objects.subdirectories[i].c_str());
            }
            return 0;
        }
        if (std::string(argv[1]) == "bandcharacter1") {
            const auto band =
                gh::milo_object::parse_band_character1(read_file(argv[2]));
            const auto& character = band.character;
            const auto& directory = character.render_directory;
            std::printf(
                "revision=%u character_revision=%u rnd_dir_revision=%u "
                "object_dir_revision=%u object_type=%s proxy_path=%s "
                "environment=%s lods=%zu shadow=%s self_shadow=%d "
                "sphere_base=%s\n",
                band.revision, character.revision, directory.revision,
                directory.object_directory.revision,
                directory.object_directory.object_fields.type.c_str(),
                directory.object_directory.proxy_path.c_str(),
                directory.environment.c_str(), character.lods.size(),
                character.shadow.c_str(), character.self_shadow ? 1 : 0,
                character.sphere_base.c_str());
            return 0;
        }
        if (std::string(argv[1]) == "waypoint3") {
            const auto waypoint =
                gh::milo_object::parse_waypoint3(read_file(argv[2]));
            std::printf(
                "revision=%u object_type=%s drawable_revision=%u "
                "transform_revision=%u flags=%u connections=%zu "
                "radius=%.9g y_radius=%.9g angle_radius=%.9g "
                "position=(%.9g %.9g %.9g) parent=%s\n",
                waypoint.revision,
                waypoint.object_fields.type.c_str(),
                waypoint.legacy_drawable.revision,
                waypoint.transformable.revision,
                waypoint.flags, waypoint.connections.size(),
                waypoint.radius, waypoint.y_radius,
                waypoint.angle_radius,
                waypoint.transformable.local[9],
                waypoint.transformable.local[10],
                waypoint.transformable.local[11],
                waypoint.transformable.parent.c_str());
            for (size_t index = 0; index < waypoint.connections.size();
                 ++index) {
                std::printf("connection[%zu]=%s\n", index,
                            waypoint.connections[index].c_str());
            }
            return 0;
        }
        if (std::string(argv[1]) == "mesh25") {
            const auto mesh =
                gh::milo_object::parse_mesh(read_file(argv[2]));
            std::printf(
                "revision=%u transform_revision=%u drawable_revision=%u "
                "material=%s geometry_owner=%s vertices=%zu faces=%zu "
                "has_bones=%d showing=%d draw_order=%.9g parent=%s "
                "local=(%.9g %.9g %.9g) world=(%.9g %.9g %.9g)\n",
                mesh.revision, mesh.transformable.revision,
                mesh.drawable.revision, mesh.material.c_str(),
                mesh.geometry_owner.c_str(), mesh.vertices.size(),
                mesh.faces.size(), mesh.has_bones ? 1 : 0,
                mesh.drawable.showing ? 1 : 0, mesh.drawable.draw_order,
                mesh.transformable.parent.c_str(),
                mesh.transformable.local[9],
                mesh.transformable.local[10],
                mesh.transformable.local[11],
                mesh.transformable.world[9],
                mesh.transformable.world[10],
                mesh.transformable.world[11]);
            std::printf(
                "local_xfm=[%.9g %.9g %.9g | %.9g %.9g %.9g | "
                "%.9g %.9g %.9g | %.9g %.9g %.9g]\n",
                mesh.transformable.local[0], mesh.transformable.local[1],
                mesh.transformable.local[2], mesh.transformable.local[3],
                mesh.transformable.local[4], mesh.transformable.local[5],
                mesh.transformable.local[6], mesh.transformable.local[7],
                mesh.transformable.local[8], mesh.transformable.local[9],
                mesh.transformable.local[10], mesh.transformable.local[11]);
            print_mesh_vertex_ranges(mesh);
            if (print_vertices) print_mesh_vertices(mesh);
            return 0;
        }
        if (std::string(argv[1]) == "mesh28") {
            const auto mesh =
                gh::milo_object::parse_mesh28(read_file(argv[2]));
            std::printf(
                "revision=%u transform_revision=%u drawable_revision=%u "
                "material=%s geometry_owner=%s vertices=%zu faces=%zu "
                "has_bones=%d showing=%d draw_order=%.9g parent=%s "
                "local=(%.9g %.9g %.9g) world=(%.9g %.9g %.9g)\n",
                mesh.revision, mesh.transformable.revision,
                mesh.drawable.revision, mesh.material.c_str(),
                mesh.geometry_owner.c_str(), mesh.vertices.size(),
                mesh.faces.size(), mesh.has_bones ? 1 : 0,
                mesh.drawable.showing ? 1 : 0, mesh.drawable.draw_order,
                mesh.transformable.parent.c_str(),
                mesh.transformable.local[9],
                mesh.transformable.local[10],
                mesh.transformable.local[11],
                mesh.transformable.world[9],
                mesh.transformable.world[10],
                mesh.transformable.world[11]);
            std::printf(
                "local_xfm=[%.9g %.9g %.9g | %.9g %.9g %.9g | "
                "%.9g %.9g %.9g | %.9g %.9g %.9g]\n",
                mesh.transformable.local[0], mesh.transformable.local[1],
                mesh.transformable.local[2], mesh.transformable.local[3],
                mesh.transformable.local[4], mesh.transformable.local[5],
                mesh.transformable.local[6], mesh.transformable.local[7],
                mesh.transformable.local[8], mesh.transformable.local[9],
                mesh.transformable.local[10], mesh.transformable.local[11]);
            print_mesh_vertex_ranges(mesh);
            if (print_vertices) print_mesh_vertices(mesh);
            return 0;
        }
        if (std::string(argv[1]) == "view7") {
            const auto view =
                gh::milo_object::parse_view(read_file(argv[2]));
            std::printf(
                "revision=%u animation_objects=%zu drawable_objects=%zu "
                "children_owner=%s parent=%s showing=%d "
                "showing_range=(%.9g %.9g)\n",
                view.revision, view.animatable.objects.size(),
                view.drawable.objects.size(), view.children_owner.c_str(),
                view.transformable.parent.c_str(),
                view.drawable.showing ? 1 : 0,
                view.showing_range[0], view.showing_range[1]);
            for (size_t i = 0; i < view.animatable.objects.size(); ++i)
                std::printf(
                    "animation_object=%zu name=%s\n", i,
                    view.animatable.objects[i].c_str());
            for (size_t i = 0; i < view.drawable.objects.size(); ++i)
                std::printf(
                    "drawable_object=%zu name=%s\n", i,
                    view.drawable.objects[i].c_str());
            return 0;
        }
        if (std::string(argv[1]) == "group12") {
            const auto group =
                gh::milo_object::parse_group12(read_file(argv[2]));
            std::printf(
                "revision=%u objects=%zu environment=%s draw_only=%s lod=%s "
                "lod_screen_size=%.9g parent=%s showing=%d "
                "local=(%.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g "
                "%.9g %.9g %.9g) "
                "world=(%.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g "
                "%.9g %.9g %.9g)\n",
                group.revision, group.objects.size(),
                group.environment.c_str(), group.draw_only.c_str(),
                group.lod.c_str(),
                group.lod_screen_size,
                group.transformable.parent.c_str(),
                group.drawable.showing ? 1 : 0,
                group.transformable.local[0], group.transformable.local[1],
                group.transformable.local[2], group.transformable.local[3],
                group.transformable.local[4], group.transformable.local[5],
                group.transformable.local[6], group.transformable.local[7],
                group.transformable.local[8], group.transformable.local[9],
                group.transformable.local[10], group.transformable.local[11],
                group.transformable.world[0], group.transformable.world[1],
                group.transformable.world[2], group.transformable.world[3],
                group.transformable.world[4], group.transformable.world[5],
                group.transformable.world[6], group.transformable.world[7],
                group.transformable.world[8], group.transformable.world[9],
                group.transformable.world[10], group.transformable.world[11]);
            for (size_t i = 0; i < group.objects.size(); ++i)
                std::printf(
                    "object=%zu name=%s\n", i,
                    group.objects[i].c_str());
            return 0;
        }
        if (std::string(argv[1]) == "transanim6") {
            const auto anim =
                gh::milo_object::parse_trans_anim6(read_file(argv[2]));
            std::printf(
                "revision=%u anim_rate=%d target=%s keys_owner=%s "
                "rotation_keys=%zu translation_keys=%zu scale_keys=%zu "
                "translation_spline=%d repeat_translation=%d "
                "follow_path=%d rotation_slerp=%d\n",
                anim.revision, anim.animatable.rate,
                anim.target.c_str(), anim.keys_owner.c_str(),
                anim.rotation_keys.size(), anim.translation_keys.size(),
                anim.scale_keys.size(),
                anim.translation_spline ? 1 : 0,
                anim.repeat_translation ? 1 : 0,
                anim.follow_path ? 1 : 0,
                anim.rotation_slerp ? 1 : 0);
            for (size_t i = 0; i < anim.translation_keys.size(); ++i) {
                const auto& key = anim.translation_keys[i];
                std::printf(
                    "translation=%zu frame=%.9g value=(%.9g %.9g %.9g)\n",
                    i, key.frame, key.value[0], key.value[1], key.value[2]);
            }
            for (size_t i = 0; i < anim.rotation_keys.size(); ++i) {
                const auto& key = anim.rotation_keys[i];
                std::printf(
                    "rotation=%zu frame=%.9g value=(%.9g %.9g %.9g %.9g)\n",
                    i, key.frame, key.value[0], key.value[1], key.value[2],
                    key.value[3]);
            }
            return 0;
        }
        if (std::string(argv[1]) == "transanim4") {
            const auto anim =
                gh::milo_object::parse_trans_anim(read_file(argv[2]));
            std::printf(
                "revision=%u legacy_anim_revision=%u "
                "legacy_drawable_revision=%u target=%s keys_owner=%s "
                "rotation_keys=%zu translation_keys=%zu scale_keys=%zu "
                "translation_spline=%d repeat_translation=%d "
                "follow_path=%d rotation_slerp=%d\n",
                anim.revision, anim.animatable.revision,
                anim.drawable.revision, anim.target.c_str(),
                anim.keys_owner.c_str(), anim.rotation_keys.size(),
                anim.translation_keys.size(), anim.scale_keys.size(),
                anim.translation_spline ? 1 : 0,
                anim.repeat_translation ? 1 : 0,
                anim.follow_path ? 1 : 0,
                anim.rotation_slerp ? 1 : 0);
            for (size_t i = 0; i < anim.translation_keys.size(); ++i) {
                const auto& key = anim.translation_keys[i];
                std::printf(
                    "translation=%zu frame=%.9g value=(%.9g %.9g %.9g)\n",
                    i, key.frame, key.value[0], key.value[1], key.value[2]);
            }
            for (size_t i = 0; i < anim.rotation_keys.size(); ++i) {
                const auto& key = anim.rotation_keys[i];
                std::printf(
                    "rotation=%zu frame=%.9g value=(%.9g %.9g %.9g %.9g)\n",
                    i, key.frame, key.value[0], key.value[1], key.value[2],
                    key.value[3]);
            }
            return 0;
        }
        if (std::string(argv[1]) != "camshot20")
            throw std::runtime_error(
                "unknown inspection mode " + std::string(argv[1]));
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
