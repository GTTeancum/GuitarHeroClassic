#include "milo_convert.h"

#include "milo_object.h"
#include "ps2_texture.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>

namespace gh::milo_convert {
namespace {

constexpr uint32_t kObjectTerminator = 0xDEADDEADu;

std::string lower(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
    return value;
}

std::string extension(const std::string& value) {
    const size_t dot = value.rfind('.');
    return dot == std::string::npos
               ? std::string()
               : lower(value.substr(dot));
}

std::string base_name(const std::string& value) {
    const size_t dot = value.rfind('.');
    return dot == std::string::npos ? value : value.substr(0, dot);
}

std::string inferred_reference_type(const std::string& name) {
    static const std::map<std::string, std::string> types = {
        {".anim", "View"},
        {".cam", "Cam"},
        {".env", "Environ"},
        {".lt", "Light"},
        {".mat", "Mat"},
        {".mesh", "Mesh"},
        {".mnm", "MatAnim"},
        {".panim", "ParticleSysAnim"},
        {".part", "ParticleSys"},
        {".tnm", "TransAnim"},
        {".view", "View"},
    };
    const auto found = types.find(extension(name));
    return found == types.end() ? std::string() : found->second;
}

void set_identity(std::array<float, 12>& transform) {
    transform = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
}

gh::milo::Entry make_entry(
    std::string type, std::string name, std::vector<uint8_t> body) {
    gh::milo::Entry entry;
    entry.type = std::move(type);
    entry.name = std::move(name);
    entry.body_bytes = std::move(body);
    entry.size = entry.body_bytes.size();
    entry.terminator_value = kObjectTerminator;
    return entry;
}

void add_row(
    Result& result, const std::string& source_type,
    const std::string& source_name, const std::string& target_type,
    const std::string& target_name, const std::string& status,
    const std::string& detail) {
    result.manifest.push_back(
        {source_type, source_name, target_type, target_name, status, detail});
}

template <typename T>
void clear_legacy_graph(T& value) {
    value.animatable.operations.clear();
    value.animatable.objects.clear();
}

void add_legacy_filter_if_required(
    Result& result, const gh::milo_object::LegacyAnimatable& animatable,
    const std::string& source_type, const std::string& source_name,
    std::set<std::string>& target_names) {
    const auto settings =
        gh::milo_object::reduce_legacy_animatable(animatable);
    if (!settings.requires_filter()) return;
    const std::string filter_name = base_name(source_name) + ".filt";
    if (!target_names.insert(filter_name).second) {
        add_row(
            result, source_type, source_name, "AnimFilter", filter_name,
            "blocked", "deterministic legacy filter name collides");
        return;
    }
    const auto filter =
        gh::milo_object::convert_legacy_animatable_to_anim_filter1(
            animatable, source_name);
    result.directory.entries.push_back(
        make_entry(
            "AnimFilter", filter_name,
            gh::milo_object::serialize_anim_filter1(filter)));
    add_row(
        result, source_type, source_name, "AnimFilter", filter_name,
        "synthesized",
        "Animatable0 scale/range program expanded using retail loader rules");
}

std::optional<gh::milo_object::LegacyTransformable>
legacy_transformable(const gh::milo::Entry& entry) {
    if (entry.type == "Cam")
        return gh::milo_object::parse_cam(entry.body_bytes).transformable;
    if (entry.type == "Flare")
        return gh::milo_object::parse_flare(entry.body_bytes).transformable;
    if (entry.type == "Light")
        return gh::milo_object::parse_light(entry.body_bytes).transformable;
    if (entry.type == "Mesh")
        return gh::milo_object::parse_mesh(entry.body_bytes).transformable;
    if (entry.type == "ParticleSys")
        return gh::milo_object::parse_particle_sys(entry.body_bytes)
            .transformable;
    if (entry.type == "Text")
        return gh::milo_object::parse_text(entry.body_bytes).transformable;
    if (entry.type == "View")
        return gh::milo_object::parse_view(entry.body_bytes).transformable;
    return std::nullopt;
}

struct EffectiveTransform {
    gh::milo_object::LegacyTransformable source;
    std::string parent;
    uint32_t constraint = 0;
};

std::map<std::string, EffectiveTransform>
resolve_legacy_transform_graph(const gh::milo::Directory& source) {
    std::map<std::string, EffectiveTransform> transforms;
    for (const auto& entry : source.entries) {
        const auto transform = legacy_transformable(entry);
        if (!transform) continue;
        EffectiveTransform state;
        state.source = *transform;
        state.constraint =
            gh::milo_object::convert_transformable_constraint8_to_9(
                transform->constraint);
        transforms.emplace(entry.name, std::move(state));
    }

    // GH2's revision-8 compatibility loader first applies the serialized
    // legacy child list, then reads the object's explicit parent. A self
    // parent is a sentinel that preserves the parent established by a child
    // list. Any other explicit parent (including null) wins at that point and
    // forces the old parent constraint, enum value 2.
    for (const auto& entry : source.entries) {
        const auto current = transforms.find(entry.name);
        if (current == transforms.end()) continue;
        for (const auto& child : current->second.source.children) {
            const auto found = transforms.find(child);
            if (found != transforms.end())
                found->second.parent = entry.name;
        }
        if (current->second.source.parent != entry.name) {
            current->second.parent = current->second.source.parent;
            current->second.constraint = 2;
        }
    }
    return transforms;
}

void apply_effective_transform(
    gh::milo_object::Transformable9& target,
    const std::map<std::string, EffectiveTransform>& transforms,
    const std::string& name) {
    const auto found = transforms.find(name);
    if (found == transforms.end())
        throw std::runtime_error(
            "milo convert: missing transform graph entry " + name);
    target.parent = found->second.parent;
    target.constraint = found->second.constraint;
}

std::string tsv_cell(std::string value) {
    for (char& ch : value) {
        if (ch == '\t' || ch == '\r' || ch == '\n') ch = ' ';
    }
    return value;
}

}  // namespace

uint32_t convert_gh1_clip_time_flags_to_gh2(
    uint32_t source) {
    // GH1 stores the time/alignment enum in the old kAnim domain. GH2
    // moved the same meanings into the kPlay time mask.
    switch (source) {
        case 0: return 0;
        case 1: return 0x1000;  // BeatAlign1
        case 2: return 0x2000;  // BeatAlign2
        case 4: return 0x4000;  // BeatAlign4
        case 8: return 0x8000;  // BeatAlign8
        case 16: return 0x0200; // RealTime
        case 32: return 0x0400; // GCLock -> UserTime
        default:
            throw std::runtime_error(
                "milo convert: unsupported GH1 clip time flags " +
                std::to_string(source));
    }
}

gh::milo_object::CharClipSamples10
convert_gh1_acp_to_gh2_char_clip_samples10(
    const gh::acp::File& source,
    const std::vector<gh::milo_object::CharClipTransition5>&
        transitions) {
    if (source.class_name != "AnimClipSamples" ||
        source.revision != 18 ||
        source.sample_set_revision != 5 ||
        !source.trailing_bytes.empty())
        throw std::runtime_error(
            "milo convert: ACP source contract is not exact revision 18/5");

    auto convert_samples =
        [](const gh::acp::ChannelSet& source_set) {
            gh::milo_object::CharBonesSamples10 target;
            target.channels = source_set.channels;
            target.compression = source_set.compression;
            target.sample_count = source_set.sample_count;
            target.sample_bytes = source_set.sample_bytes;

            auto category = [](const std::string& channel) -> size_t {
                const std::string suffix = extension(channel);
                if (suffix == ".pos") return 0;
                if (suffix == ".scale") return 1;
                if (suffix == ".quat") return 2;
                if (suffix == ".rotx") return 3;
                if (suffix == ".roty") return 4;
                if (suffix == ".rotz") return 5;
                if (suffix == ".drotx") return 6;
                if (suffix == ".droty") return 7;
                if (suffix == ".drotz") return 8;
                throw std::runtime_error(
                    "milo convert: ACP channel type is not GH2-native: " +
                    channel);
            };

            size_t previous = 0;
            bool have_previous = false;
            std::array<uint32_t, 9> category_counts{};
            for (const auto& channel : source_set.channels) {
                const size_t current = category(channel);
                if (have_previous && current < previous)
                    throw std::runtime_error(
                        "milo convert: ACP channels are not in GH2 "
                        "CharBones type order");
                previous = current;
                have_previous = true;
                ++category_counts[current];
            }
            target.counts[0] = 0;
            uint32_t cumulative = 0;
            for (size_t i = 0; i < category_counts.size(); ++i) {
                cumulative += category_counts[i];
                target.counts[i + 1] = cumulative;
            }

            uint64_t expected = 0;
            for (const auto& channel : source_set.channels)
                expected += gh::acp::channel_file_size(
                    channel, source_set.compression);
            expected *= source_set.sample_count;
            if (expected != source_set.sample_bytes.size() ||
                expected != static_cast<uint64_t>(
                    source_set.frame_size) *
                    source_set.sample_count)
                throw std::runtime_error(
                    "milo convert: ACP sample byte accounting differs");
            return target;
        };

    gh::milo_object::CharClipSamples10 target;
    target.start_beat = source.start_beat;
    target.end_beat = source.end_beat;
    target.beats_per_second = source.beats_per_second;
    // GH1's standalone ACP writer sets bit 31 as an AnimClipSamples export
    // marker. It is not a CharClip gameplay flag and does not appear in GH2
    // native clip bodies.
    target.flags = source.flags & 0x7fffffffu;
    target.play_flags =
        convert_gh1_clip_time_flags_to_gh2(
            source.play_flags);
    target.blend_width = source.blend_width;
    target.transitions = transitions;
    target.full = convert_samples(source.channel_sets[0]);
    target.one = convert_samples(source.channel_sets[1]);
    // GH2 CharClipSamples revisions 8-12 serialize a third legacy header
    // between the full/one headers and their data. The loader consumes and
    // discards that header, and no third sample-data block exists. Retail GH2
    // bodies therefore carry zero channels/counts/bytes here. Preserve the
    // full-set compression/sample-count metadata deterministically, matching
    // the common retail writer form, without inventing duplicate sample data.
    target.duplicate.compression = target.full.compression;
    target.duplicate.sample_count = target.full.sample_count;
    return target;
}

std::vector<std::vector<gh::milo_object::CharClipTransition5>>
convert_gh1_acg_to_gh2_char_clip_transitions(
    const gh::acg::Graph& source,
    const std::vector<std::string>& clip_names) {
    if (source.version != 1 || !source.trailing_bytes.empty() ||
        source.clips.size() != clip_names.size())
        throw std::runtime_error(
            "milo convert: ACG source contract/clip domain differs");
    std::vector<
        std::vector<gh::milo_object::CharClipTransition5>>
        result(source.clips.size());
    for (size_t source_index = 0;
         source_index < source.clips.size(); ++source_index) {
        std::map<uint32_t, size_t> target_groups;
        auto& output = result[source_index];
        for (const auto& node :
             source.clips[source_index].nodes) {
            if (node.target_clip_index >= clip_names.size())
                throw std::runtime_error(
                    "milo convert: ACG target outside clip domain");
            auto found =
                target_groups.find(node.target_clip_index);
            if (found == target_groups.end()) {
                found = target_groups
                            .emplace(
                                node.target_clip_index,
                                output.size())
                            .first;
                gh::milo_object::CharClipTransition5 group;
                group.clip =
                    clip_names[node.target_clip_index];
                output.push_back(std::move(group));
            }
            output[found->second].nodes.push_back(
                {node.current_beat, node.next_beat});
        }
    }
    return result;
}

Result convert_gh1_directory_to_gh2_rnddir(
    const gh::milo::Directory& source,
    const std::string& target_directory_name) {
    if (source.dir_version != 10 || !source.boundaries_exact)
        throw std::runtime_error(
            "milo convert: source must be an exact GH1 revision-10 directory");
    if (target_directory_name.empty())
        throw std::runtime_error(
            "milo convert: target directory name cannot be empty");

    Result result;
    result.directory.dir_version = 24;
    result.directory.dir_type = "RndDir";
    result.directory.dir_name = target_directory_name;
    result.directory.boundaries_exact = true;
    result.directory.dir_terminator_value = kObjectTerminator;

    std::map<std::string, std::string> source_types;
    std::set<std::string> target_names;
    for (const auto& entry : source.entries) {
        if (!source_types.emplace(entry.name, entry.type).second)
            throw std::runtime_error(
                "milo convert: duplicate source object name " + entry.name);
        target_names.insert(entry.name);
    }

    std::map<std::string, gh::milo_object::View> views;
    std::map<std::string, gh::milo_object::Mat> materials;
    std::map<std::string, gh::milo_object::Tex> textures;
    std::map<std::string, std::vector<std::string>>
        mat_anim_expansions;
    std::set<std::string> planned_material_names;
    const auto effective_transforms =
        resolve_legacy_transform_graph(source);
    for (const auto& entry : source.entries) {
        if (entry.type == "View") {
            views.emplace(
                entry.name,
                gh::milo_object::parse_view(entry.body_bytes));
        } else if (entry.type == "MatAnim") {
            auto source_anim =
                gh::milo_object::parse_mat_anim(entry.body_bytes);
            source_anim.animatable.operations.clear();
            source_anim.animatable.objects.clear();
            const auto passes =
                gh::milo_object::
                    convert_mat_anim5_to_mat_anim7_passes(
                        source_anim, entry.name);
            auto& names = mat_anim_expansions[entry.name];
            for (size_t i = 1; i < passes.size(); ++i)
                names.push_back(passes[i].name);
        } else if (entry.type == "Mat") {
            const auto source_material =
                gh::milo_object::parse_mat(entry.body_bytes);
            materials.emplace(entry.name, source_material);
            const auto passes =
                gh::milo_object::convert_mat21_to_mat27_passes(
                    source_material,
                    entry.name);
            for (const auto& pass : passes)
                planned_material_names.insert(pass.name);
        } else if (entry.type == "Tex") {
            textures.emplace(
                entry.name,
                gh::milo_object::parse_tex(entry.body_bytes));
        }
    }

    auto reference_type =
        [&](const std::string& name) -> std::string {
        const auto found = source_types.find(name);
        if (found != source_types.end()) return found->second;
        return inferred_reference_type(name);
    };

    for (const auto& entry : source.entries) {
        try {
            if (entry.type == "Cam") {
                auto target =
                    gh::milo_object::convert_cam9_to_cam12(
                        gh::milo_object::parse_cam(entry.body_bytes));
                apply_effective_transform(
                    target.transformable, effective_transforms,
                    entry.name);
                result.directory.entries.push_back(make_entry(
                    "Cam", entry.name,
                    gh::milo_object::serialize_cam12(target)));
            } else if (entry.type == "Flare") {
                auto target =
                    gh::milo_object::convert_flare3_to_flare4(
                        gh::milo_object::parse_flare(
                            entry.body_bytes));
                apply_effective_transform(
                    target.transformable, effective_transforms,
                    entry.name);
                result.directory.entries.push_back(make_entry(
                    "Flare", entry.name,
                    gh::milo_object::serialize_flare4(target)));
            } else if (entry.type == "Light") {
                auto target =
                    gh::milo_object::convert_light3_to_light6(
                        gh::milo_object::parse_light(entry.body_bytes));
                apply_effective_transform(
                    target.transformable, effective_transforms,
                    entry.name);
                result.directory.entries.push_back(make_entry(
                    "Light", entry.name,
                    gh::milo_object::serialize_light6(target)));
            } else if (entry.type == "Environ") {
                const auto target =
                    gh::milo_object::convert_environ1_to_environ5(
                        gh::milo_object::parse_environ(entry.body_bytes));
                result.directory.entries.push_back(make_entry(
                    "Environ", entry.name,
                    gh::milo_object::serialize_environ5(target)));
            } else if (entry.type == "Mat") {
                const auto passes =
                    gh::milo_object::convert_mat21_to_mat27_passes(
                        gh::milo_object::parse_mat(entry.body_bytes),
                        entry.name);
                std::vector<gh::milo::Entry> converted_passes;
                converted_passes.reserve(passes.size());
                for (size_t i = 0; i < passes.size(); ++i) {
                    if (i != 0 &&
                        target_names.find(passes[i].name) !=
                            target_names.end())
                        throw std::runtime_error(
                            "synthesized material pass name collides: " +
                            passes[i].name);
                    converted_passes.push_back(make_entry(
                        "Mat", passes[i].name,
                        gh::milo_object::serialize_mat27(
                            passes[i].material)));
                }
                for (size_t i = 0; i < converted_passes.size(); ++i) {
                    if (i != 0) {
                        target_names.insert(converted_passes[i].name);
                        add_row(
                            result, entry.type, entry.name, "Mat",
                            converted_passes[i].name, "synthesized",
                            "Mat21 texture stage expanded to a native "
                            "Mat27 next-pass object");
                    }
                    result.directory.entries.push_back(
                        std::move(converted_passes[i]));
                }
            } else if (entry.type == "Tex") {
                const auto target =
                    gh::milo_object::convert_tex8_to_tex10(
                        gh::milo_object::parse_tex(entry.body_bytes));
                result.directory.entries.push_back(make_entry(
                    "Tex", entry.name,
                    gh::milo_object::serialize_tex10(target)));
            } else if (entry.type == "Text") {
                auto target =
                    gh::milo_object::convert_text15_to_text17(
                        gh::milo_object::parse_text(entry.body_bytes));
                apply_effective_transform(
                    target.transformable, effective_transforms,
                    entry.name);
                result.directory.entries.push_back(make_entry(
                    "Text", entry.name,
                    gh::milo_object::serialize_text17(target)));
            } else if (entry.type == "Font") {
                const auto source_font =
                    gh::milo_object::parse_font(entry.body_bytes);
                const auto material =
                    materials.find(source_font.material);
                if (material == materials.end())
                    throw std::runtime_error(
                        "Font material does not resolve: " +
                        source_font.material);
                const auto passes =
                    gh::milo_object::convert_mat21_to_mat27_passes(
                        material->second, source_font.material);
                if (passes.empty() ||
                    passes.front().material.diffuse_texture.empty())
                    throw std::runtime_error(
                        "Font material has no effective diffuse texture: " +
                        source_font.material);
                const std::string& texture_name =
                    passes.front().material.diffuse_texture;
                const auto texture = textures.find(texture_name);
                if (texture == textures.end())
                    throw std::runtime_error(
                        "Font diffuse texture does not resolve: " +
                        texture_name);
                if (!texture->second.has_bitmap)
                    throw std::runtime_error(
                        "Font diffuse texture has no embedded bitmap: " +
                        texture_name);
                gh::tex::HmxBitmap bitmap;
                bitmap.magic =
                    texture->second.bitmap.header_kind;
                bitmap.bpp =
                    texture->second.bitmap.bits_per_pixel;
                bitmap.encoding =
                    texture->second.bitmap.encoding;
                bitmap.mipmaps =
                    texture->second.bitmap.mipmap_count;
                bitmap.width = texture->second.bitmap.width;
                bitmap.height = texture->second.bitmap.height;
                bitmap.bpl =
                    texture->second.bitmap.bytes_per_line;
                bitmap.wii_alpha =
                    texture->second.bitmap.wii_alpha;
                bitmap.raw = texture->second.bitmap.data;
                const auto rgba = gh::tex::decode_to_rgba(bitmap);
                const auto target =
                    gh::milo_object::convert_font7_to_font15(
                        source_font, entry.name, bitmap.width,
                        bitmap.height, rgba);
                result.directory.entries.push_back(make_entry(
                    "Font", entry.name,
                    gh::milo_object::serialize_font15(target)));
            } else if (entry.type == "Mesh") {
                auto target =
                    gh::milo_object::convert_mesh25_to_mesh28(
                        gh::milo_object::parse_mesh(entry.body_bytes));
                apply_effective_transform(
                    target.transformable, effective_transforms,
                    entry.name);
                result.directory.entries.push_back(make_entry(
                    "Mesh", entry.name,
                    gh::milo_object::serialize_mesh28(target)));
            } else if (entry.type == "MultiMesh") {
                const auto target =
                    gh::milo_object::convert_multi_mesh0_to_multi_mesh1(
                        gh::milo_object::parse_multi_mesh(
                            entry.body_bytes));
                result.directory.entries.push_back(make_entry(
                    "MultiMesh", entry.name,
                    gh::milo_object::serialize_multi_mesh1(target)));
            } else if (entry.type == "Movie") {
                auto source_body =
                    gh::milo_object::parse_movie(entry.body_bytes);
                const auto graph = source_body.animatable;
                clear_legacy_graph(source_body);
                const auto target =
                    gh::milo_object::convert_movie6_to_movie8(
                        source_body);
                result.directory.entries.push_back(make_entry(
                    "Movie", entry.name,
                    gh::milo_object::serialize_movie8(target)));
                add_legacy_filter_if_required(
                    result, graph, entry.type, entry.name, target_names);
            } else if (entry.type == "Morph") {
                auto source_body =
                    gh::milo_object::parse_morph(entry.body_bytes);
                const auto graph = source_body.animatable;
                clear_legacy_graph(source_body);
                const auto target =
                    gh::milo_object::convert_morph3_to_morph4(
                        source_body);
                result.directory.entries.push_back(make_entry(
                    "Morph", entry.name,
                    gh::milo_object::serialize_morph4(target)));
                add_legacy_filter_if_required(
                    result, graph, entry.type, entry.name, target_names);
            } else if (entry.type == "TransAnim") {
                auto source_body =
                    gh::milo_object::parse_trans_anim(entry.body_bytes);
                const auto graph = source_body.animatable;
                clear_legacy_graph(source_body);
                const auto target =
                    gh::milo_object::convert_trans_anim4_to_trans_anim6(
                        source_body);
                result.directory.entries.push_back(make_entry(
                    "TransAnim", entry.name,
                    gh::milo_object::serialize_trans_anim6(target)));
                add_legacy_filter_if_required(
                    result, graph, entry.type, entry.name, target_names);
            } else if (entry.type == "MeshAnim") {
                auto source_body =
                    gh::milo_object::parse_mesh_anim(entry.body_bytes);
                const auto graph = source_body.animatable;
                clear_legacy_graph(source_body);
                const auto target =
                    gh::milo_object::convert_mesh_anim0_to_mesh_anim1(
                        source_body);
                result.directory.entries.push_back(make_entry(
                    "MeshAnim", entry.name,
                    gh::milo_object::serialize_mesh_anim1(target)));
                add_legacy_filter_if_required(
                    result, graph, entry.type, entry.name, target_names);
            } else if (entry.type == "CamAnim") {
                auto source_body =
                    gh::milo_object::parse_cam_anim(entry.body_bytes);
                const auto graph = source_body.animatable;
                clear_legacy_graph(source_body);
                const auto target =
                    gh::milo_object::convert_cam_anim0_to_cam_anim2(
                        source_body);
                result.directory.entries.push_back(make_entry(
                    "CamAnim", entry.name,
                    gh::milo_object::serialize_cam_anim2(target)));
                add_legacy_filter_if_required(
                    result, graph, entry.type, entry.name, target_names);
            } else if (entry.type == "EnvAnim") {
                auto source_body =
                    gh::milo_object::parse_env_anim(entry.body_bytes);
                const auto graph = source_body.animatable;
                clear_legacy_graph(source_body);
                const auto target =
                    gh::milo_object::convert_env_anim3_to_env_anim4(
                        source_body);
                result.directory.entries.push_back(make_entry(
                    "EnvAnim", entry.name,
                    gh::milo_object::serialize_env_anim4(target)));
                add_legacy_filter_if_required(
                    result, graph, entry.type, entry.name, target_names);
            } else if (entry.type == "LightAnim") {
                auto source_body =
                    gh::milo_object::parse_light_anim(entry.body_bytes);
                const auto graph = source_body.animatable;
                clear_legacy_graph(source_body);
                const auto target =
                    gh::milo_object::convert_light_anim1_to_light_anim2(
                        source_body);
                result.directory.entries.push_back(make_entry(
                    "LightAnim", entry.name,
                    gh::milo_object::serialize_light_anim2(target)));
                add_legacy_filter_if_required(
                    result, graph, entry.type, entry.name, target_names);
            } else if (entry.type == "ParticleSysAnim") {
                auto source_body =
                    gh::milo_object::parse_particle_sys_anim(
                        entry.body_bytes);
                const auto graph = source_body.animatable;
                clear_legacy_graph(source_body);
                const auto target =
                    gh::milo_object::
                        convert_particle_sys_anim2_to_particle_sys_anim3(
                            source_body);
                result.directory.entries.push_back(make_entry(
                    "ParticleSysAnim", entry.name,
                    gh::milo_object::serialize_particle_sys_anim3(target)));
                add_legacy_filter_if_required(
                    result, graph, entry.type, entry.name, target_names);
            } else if (entry.type == "MatAnim") {
                auto source_body =
                    gh::milo_object::parse_mat_anim(entry.body_bytes);
                const auto graph = source_body.animatable;
                clear_legacy_graph(source_body);
                const auto passes =
                    gh::milo_object::
                        convert_mat_anim5_to_mat_anim7_passes(
                            source_body, entry.name);
                std::vector<gh::milo::Entry> converted_passes;
                std::vector<size_t> emitted_passes;
                converted_passes.reserve(passes.size());
                for (size_t i = 0; i < passes.size(); ++i) {
                    if (i != 0 &&
                        target_names.find(passes[i].name) !=
                            target_names.end()) {
                        const auto existing =
                            source_types.find(passes[i].name);
                        if (existing != source_types.end() &&
                            existing->second == "MatAnim") {
                            add_row(
                                result, entry.type, entry.name, "MatAnim",
                                passes[i].name, "resolved",
                                "serialized MatAnim with the HMX split name "
                                "supersedes the loader-generated object");
                            continue;
                        }
                        throw std::runtime_error(
                            "synthesized MatAnim pass name collides: " +
                            passes[i].name);
                    }
                    if (!passes[i].animation.material.empty() &&
                        planned_material_names.find(
                            passes[i].animation.material) ==
                            planned_material_names.end()) {
                        if (target_names.find(
                                passes[i].animation.material) ==
                            target_names.end()) {
                            gh::milo_object::Mat27 fallback;
                            result.directory.entries.push_back(make_entry(
                                "Mat", passes[i].animation.material,
                                gh::milo_object::serialize_mat27(fallback)));
                            target_names.insert(
                                passes[i].animation.material);
                            add_row(
                                result, entry.type, entry.name, "Mat",
                                passes[i].animation.material, "synthesized",
                                "LookupOrCreateMat target did not exist; "
                                "created with authoritative RndMat defaults");
                        }
                        planned_material_names.insert(
                            passes[i].animation.material);
                    }
                    converted_passes.push_back(make_entry(
                        "MatAnim", passes[i].name,
                        gh::milo_object::serialize_mat_anim7(
                            passes[i].animation)));
                    emitted_passes.push_back(i);
                }
                for (size_t j = 0; j < converted_passes.size(); ++j) {
                    const size_t source_index = emitted_passes[j];
                    if (source_index != 0) {
                        target_names.insert(converted_passes[j].name);
                        add_row(
                            result, entry.type, entry.name, "MatAnim",
                            converted_passes[j].name, "synthesized",
                            "MatAnim5 stage split using the HMX revision-7 "
                            "loader contract");
                    }
                    result.directory.entries.push_back(
                        std::move(converted_passes[j]));
                }
                add_legacy_filter_if_required(
                    result, graph, entry.type, entry.name, target_names);
                for (size_t source_index : emitted_passes) {
                    if (source_index != 0)
                        add_legacy_filter_if_required(
                            result, graph, entry.type,
                            passes[source_index].name, target_names);
                }
            } else if (entry.type == "ParticleSys") {
                auto source_body =
                    gh::milo_object::parse_particle_sys(entry.body_bytes);
                const auto graph = source_body.animatable;
                clear_legacy_graph(source_body);
                std::string bounce_name;
                if (source_body.bounce_enabled) {
                    const float normal_squared =
                        source_body.bounce_plane[0] *
                            source_body.bounce_plane[0] +
                        source_body.bounce_plane[1] *
                            source_body.bounce_plane[1] +
                        source_body.bounce_plane[2] *
                            source_body.bounce_plane[2];
                    if (normal_squared > 0.0f) {
                        bounce_name =
                            base_name(entry.name) + "_bounce.trans";
                        if (target_names.find(bounce_name) !=
                            target_names.end())
                            throw std::runtime_error(
                                "synthesized particle bounce Trans name "
                                "collides: " + bounce_name);
                        const auto bounce =
                            gh::milo_object::convert_bounce_plane_to_trans9(
                                source_body.bounce_plane);
                        result.directory.entries.push_back(make_entry(
                            "Trans", bounce_name,
                            gh::milo_object::serialize_trans9(bounce)));
                        target_names.insert(bounce_name);
                        add_row(
                            result, entry.type, entry.name, "Trans",
                            bounce_name, "synthesized",
                            "Plane converted to the HMX revision-26 bounce "
                            "transform contract");
                    } else if (normal_squared == 0.0f) {
                        add_row(
                            result, entry.type, entry.name, "", "",
                            "resolved",
                            "zero-normal legacy plane cannot define a "
                            "collision plane and remains unbound");
                    } else {
                        throw std::runtime_error(
                            "particle bounce plane is non-finite");
                    }
                }
                auto target =
                    gh::milo_object::
                        convert_particle_sys22_to_particle_sys27(
                            source_body, bounce_name);
                apply_effective_transform(
                    target.transformable, effective_transforms,
                    entry.name);
                result.directory.entries.push_back(make_entry(
                    "ParticleSys", entry.name,
                    gh::milo_object::serialize_particle_sys27(target)));
                add_legacy_filter_if_required(
                    result, graph, entry.type, entry.name, target_names);
            } else if (entry.type == "View") {
                const auto source_view = views.at(entry.name);
                const auto owner = views.find(source_view.children_owner);
                if (owner == views.end())
                    throw std::runtime_error(
                        "View children_owner does not resolve to a View");
                gh::milo_object::ResolvedViewGraph graph;
                for (const auto& name :
                     owner->second.animatable.objects) {
                    const std::string type = reference_type(name);
                    if (type.empty())
                        throw std::runtime_error(
                            "View animation reference type is unresolved: " +
                            name);
                    graph.animation_objects.push_back({name, type});
                    if (type == "MatAnim") {
                        const auto split =
                            mat_anim_expansions.find(name);
                        if (split != mat_anim_expansions.end()) {
                            for (const auto& split_name : split->second)
                                graph.animation_objects.push_back(
                                    {split_name, "MatAnim"});
                        }
                    }
                }
                for (const auto& name : owner->second.drawable.objects) {
                    const std::string type = reference_type(name);
                    if (type.empty())
                        throw std::runtime_error(
                            "View drawable reference type is unresolved: " +
                            name);
                    graph.drawable_objects.push_back({name, type});
                }
                auto target =
                    gh::milo_object::convert_view7_to_group12(
                        source_view, graph);
                apply_effective_transform(
                    target.transformable, effective_transforms,
                    entry.name);
                result.directory.entries.push_back(make_entry(
                    "Group", entry.name,
                    gh::milo_object::serialize_group12(target)));
                add_legacy_filter_if_required(
                    result, source_view.animatable, entry.type, entry.name,
                    target_names);
            } else {
                add_row(
                    result, entry.type, entry.name, "", "", "blocked",
                    "no source-backed GH1-to-GH2 class mapping yet");
                continue;
            }
            const auto converted =
                std::find_if(
                    result.directory.entries.rbegin(),
                    result.directory.entries.rend(),
                    [&](const gh::milo::Entry& target) {
                        return target.name == entry.name;
                    });
            if (converted == result.directory.entries.rend())
                throw std::runtime_error(
                    "milo convert: converted primary object is missing");
            add_row(
                result, entry.type, entry.name,
                converted->type, converted->name, "converted",
                "revision-aware semantic conversion");
        } catch (const std::exception& ex) {
            add_row(
                result, entry.type, entry.name, "", "", "blocked",
                ex.what());
        }
    }

    gh::milo_object::RndDir8 root;
    set_identity(root.transformable.local);
    set_identity(root.transformable.world);
    result.directory.dir_body_bytes =
        gh::milo_object::serialize_rnd_dir8(root);
    result.directory.dir_entry_size =
        result.directory.dir_body_bytes.size();

    const uint64_t symbol_capacity =
        static_cast<uint64_t>(result.directory.dir_type.size()) + 1 +
        target_directory_name.size() + 1 +
        std::accumulate(
            result.directory.entries.begin(),
            result.directory.entries.end(), uint64_t{0},
            [](uint64_t total, const gh::milo::Entry& entry) {
                return total + entry.type.size() + 1 +
                       entry.name.size() + 1;
            });
    if (symbol_capacity > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "milo convert: target string table capacity exceeds u32");
    result.directory.hash_table_hint =
        static_cast<uint32_t>((result.directory.entries.size() + 1) * 2);
    result.directory.string_table_hint =
        static_cast<uint32_t>(symbol_capacity);

    result.complete =
        std::none_of(
            result.manifest.begin(), result.manifest.end(),
            [](const ManifestRow& row) { return row.status == "blocked"; });
    return result;
}

std::string manifest_tsv(const Result& result) {
    std::ostringstream out;
    out << "source_type\tsource_name\ttarget_type\ttarget_name"
           "\tstatus\tdetail\n";
    for (const auto& row : result.manifest) {
        out << tsv_cell(row.source_type) << '\t'
            << tsv_cell(row.source_name) << '\t'
            << tsv_cell(row.target_type) << '\t'
            << tsv_cell(row.target_name) << '\t'
            << tsv_cell(row.status) << '\t'
            << tsv_cell(row.detail) << '\n';
    }
    return out.str();
}

}  // namespace gh::milo_convert
