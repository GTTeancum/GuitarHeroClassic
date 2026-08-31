#include "gh1_character_package.h"

#include "milo_convert.h"
#include "milo_object.h"

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace gh::milo_convert {
namespace {

constexpr uint32_t kObjectTerminator = 0xDEADDEADu;

std::string base_name(const std::string& channel) {
    const size_t dot = channel.rfind('.');
    return dot == std::string::npos
               ? channel
               : channel.substr(0, dot);
}

std::string suffix(const std::string& channel) {
    const size_t dot = channel.rfind('.');
    return dot == std::string::npos
               ? std::string()
               : channel.substr(dot);
}

bool ends_with(
    const std::string& value, const std::string& ending) {
    return value.size() >= ending.size() &&
           value.compare(
               value.size() - ending.size(),
               ending.size(), ending) == 0;
}

bool has_channel_base(
    const std::vector<std::string>& channels,
    const std::string& base) {
    for (const auto& channel : channels)
        if (base_name(channel) == base) return true;
    return false;
}

bool spec_has_channel_base(
    const Gh1ClipSetSpec& spec, const std::string& base) {
    for (const auto& animation : spec.animations)
        if (has_channel_base(animation.channels, base))
            return true;
    return false;
}

bool is_facing_base(const std::string& base) {
    return base == "bone_facing" ||
           base == "bone_facing_delta";
}

std::string replace_extension(
    const std::string& name, const std::string& extension) {
    if (name.empty()) return {};
    const size_t dot = name.rfind('.');
    return (dot == std::string::npos
                ? name
                : name.substr(0, dot)) +
           extension;
}

std::string hand_base_name(const Gh1ClipSetSpec& spec) {
    if (!ends_with(spec.target_name, "_hand"))
        throw std::runtime_error(
            "milo convert: hand clip set lacks _hand package suffix");
    return spec.target_name.substr(
        0, spec.target_name.size() - 5);
}

std::string asset_stem(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    const size_t begin =
        slash == std::string::npos ? 0 : slash + 1;
    const size_t dot = path.rfind('.');
    const size_t end =
        dot == std::string::npos || dot < begin
            ? path.size()
            : dot;
    return path.substr(begin, end - begin);
}

Gh2ClipSetRole infer_role(const Gh1ClipSetSpec& spec) {
    const bool fret =
        spec_has_channel_base(spec, "bone_fret_hand");
    const bool strum =
        spec_has_channel_base(spec, "bone_strum_hand");
    if (fret || strum) {
        if (!fret || !strum)
            throw std::runtime_error(
                "milo convert: incomplete GH1 hand-anchor domain");
        // The caller partitions this bundle before package construction.
        return Gh2ClipSetRole::GuitarFret;
    }
    if (spec.play_flags == 16 &&
        spec.animations.size() == 1)
        return Gh2ClipSetRole::GuitarUi;
    if (spec_has_channel_base(spec, "bone_facing") ||
        spec_has_channel_base(spec, "bone_facing_delta"))
        return Gh2ClipSetRole::GuitarMain;
    if (spec.play_flags == 2)
        return Gh2ClipSetRole::Band;
    if (spec.play_flags == 0 &&
        !spec.recenter_channels.empty())
        return Gh2ClipSetRole::Crowd;
    return Gh2ClipSetRole::Generic;
}

std::string package_name(
    const Gh1ClipSetSpec& spec, Gh2ClipSetRole role) {
    switch (role) {
        case Gh2ClipSetRole::GuitarMain:
        case Gh2ClipSetRole::Crowd:
            return spec.target_name + "_main";
        case Gh2ClipSetRole::Band:
            return asset_stem(spec.archetype_rnd) + "_main";
        case Gh2ClipSetRole::GuitarUi:
            return spec.target_name;
        case Gh2ClipSetRole::GuitarFret:
            return hand_base_name(spec) + "_fret";
        case Gh2ClipSetRole::GuitarStrum:
            return hand_base_name(spec) + "_strum";
        case Gh2ClipSetRole::Generic:
            return spec.target_name;
    }
    throw std::runtime_error(
        "milo convert: unknown GH2 clip-set role");
}

std::pair<std::string, int32_t> role_type(
    Gh2ClipSetRole role) {
    switch (role) {
        case Gh2ClipSetRole::GuitarMain:
            return {"guitarist", 15};
        case Gh2ClipSetRole::GuitarUi:
            return {"guitarist_ui", 2};
        case Gh2ClipSetRole::GuitarStrum:
            return {"guitarist_strum", 1};
        case Gh2ClipSetRole::Band:
            return {"band", 4};
        case Gh2ClipSetRole::Crowd:
            return {"crowd", 1};
        case Gh2ClipSetRole::GuitarFret:
        case Gh2ClipSetRole::Generic:
            return {"", -1};
    }
    throw std::runtime_error(
        "milo convert: unknown GH2 clip-set role");
}

std::vector<gh::milo_object::ObjectDirViewport16>
standard_clip_set_viewports() {
    constexpr int32_t kLegacyViewportValue = 5583746;
    const std::array<std::array<float, 12>, 7> transforms = {{
        {0.70710677f, -0.70710677f, 0.0f,
         0.57735026f, 0.57735026f, -0.57735026f,
         0.40824828f, 0.40824828f, 0.81649655f,
         -443.405f, -443.405f, 443.405f},
        {0, -1, 0, 1, 0, 0, 0, 0, 1, -768, 0, 0},
        {0, 1, 0, -1, 0, 0, 0, 0, 1, 768, 0, 0},
        {1, 0, 0, 0, 0, -1, 0, 1, 0, 0, 0, 768},
        {1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, -768},
        {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -768, 0},
        {-1, 0, 0, 0, -1, 0, 0, 0, 1, 0, 768, 0},
    }};
    std::vector<gh::milo_object::ObjectDirViewport16> result;
    for (const auto& transform : transforms)
        result.push_back(
            {transform, kLegacyViewportValue});
    return result;
}

gh::milo::Entry make_entry(
    std::string type, std::string name,
    std::vector<uint8_t> body) {
    gh::milo::Entry entry;
    entry.type = std::move(type);
    entry.name = std::move(name);
    entry.body_bytes = std::move(body);
    entry.size = entry.body_bytes.size();
    entry.terminator_value = kObjectTerminator;
    return entry;
}

void set_identity(std::array<float, 12>& transform) {
    transform =
        {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
}

struct ChannelContext {
    bool position = false;
    bool scale = false;
    bool quat = false;
    bool rotx = false;
    bool roty = false;
    bool rotz = false;
};

void add_context(
    std::map<std::string, ChannelContext>& contexts,
    const std::string& channel) {
    const std::string base = base_name(channel);
    const std::string type = suffix(channel);
    ChannelContext& context = contexts[base];
    if (type == ".pos") context.position = true;
    else if (type == ".scale") context.scale = true;
    else if (type == ".quat") context.quat = true;
    else if (type == ".rotx" || type == ".drotx")
        context.rotx = true;
    else if (type == ".roty" || type == ".droty")
        context.roty = true;
    else if (type == ".rotz" || type == ".drotz")
        context.rotz = true;
    else
        throw std::runtime_error(
            "milo convert: unsupported character channel " +
            channel);
}

int32_t rotation_context(const ChannelContext& context) {
    const int count =
        (context.quat ? 1 : 0) +
        (context.rotx ? 1 : 0) +
        (context.roty ? 1 : 0) +
        (context.rotz ? 1 : 0);
    if (count > 1)
        throw std::runtime_error(
            "milo convert: multiple rotation encodings for one bone");
    if (context.quat) return 2;
    if (context.rotx) return 3;
    if (context.roty) return 4;
    if (context.rotz) return 5;
    return 9;
}

std::vector<std::pair<std::string, uint32_t>>
guitar_group_masks() {
    return {
        {"walk_turn", 0x00000020u},
        {"walk_stop", 0x00000040u},
        {"walk_walk", 0x00000080u},
        {"bad", 0x00004000u},
        {"extreme", 0x00008000u},
        {"idle", 0x00010000u},
        {"intro", 0x00020000u},
        {"lose", 0x00040000u},
        {"normal", 0x00080000u},
        {"solo", 0x00100000u},
        {"win", 0x00200000u},
        {"star_power", 0x02000000u},
        {"win_finals", 0x04000000u},
    };
}

uint32_t guitar_group_membership_mask() {
    uint32_t mask = 0;
    for (const auto& [name, group_mask] : guitar_group_masks()) {
        (void)name;
        mask |= group_mask;
    }
    return mask;
}

int guitar_group_clip_priority(
    const std::string& group_name,
    const std::string& clip_name) {
    if (group_name == "idle") {
        if (clip_name == "idle_medium_01") return 0;
        if (clip_name.find("medium") != std::string::npos ||
            clip_name.find("_med_") != std::string::npos)
            return 10;
        if (clip_name.find("slow") != std::string::npos ||
            clip_name.find("_slw_") != std::string::npos)
            return 20;
        if (clip_name.find("fast") != std::string::npos ||
            clip_name.find("_fst_") != std::string::npos)
            return 30;
        return 40;
    }
    if (group_name == "normal") {
        if (clip_name == "idle_medium_01") return 0;
        if (clip_name.rfind("stand_medium_", 0) == 0) return 10;
        if (clip_name.rfind("stand_slow_", 0) == 0) return 20;
        if (clip_name.rfind("stand_fast_", 0) == 0) return 30;
        if (clip_name.find("medium") != std::string::npos ||
            clip_name.find("_med_") != std::string::npos)
            return 40;
        if (clip_name.find("slow") != std::string::npos ||
            clip_name.find("_slw_") != std::string::npos)
            return 50;
        if (clip_name.find("fast") != std::string::npos ||
            clip_name.find("_fst_") != std::string::npos)
            return 60;
        return 70;
    }
    return 0;
}

void sort_guitar_group_clips(
    const std::string& group_name,
    std::vector<std::string>& clips) {
    std::stable_sort(
        clips.begin(), clips.end(),
        [&](const std::string& left, const std::string& right) {
            const int left_priority =
                guitar_group_clip_priority(group_name, left);
            const int right_priority =
                guitar_group_clip_priority(group_name, right);
            if (left_priority != right_priority)
                return left_priority < right_priority;
            return left < right;
        });
}

void add_guitar_groups(
    gh::milo::Directory& directory,
    const Gh1ClipSetSpec& spec,
    const std::vector<size_t>& indices) {
    std::map<std::string, std::vector<std::string>> groups;
    for (const auto& [name, mask] : guitar_group_masks()) {
        auto& clips = groups[name];
        for (const size_t index : indices) {
            if ((spec.animations[index].flags & mask) != 0)
                clips.push_back(spec.animations[index].name);
        }
    }
    for (const size_t index : indices) {
        const auto& animation = spec.animations[index];
        for (const auto& venue : animation.excluded_venues)
            groups[venue + "_exclude"].push_back(
                animation.name);
    }
    for (const auto& [name, clips] : groups) {
        if (clips.empty()) continue;
        gh::milo_object::CharClipGroup1 group;
        group.clips = clips;
        sort_guitar_group_clips(name, group.clips);
        group.which = 0;
        directory.entries.push_back(make_entry(
            "CharClipGroup", name,
            gh::milo_object::serialize_char_clip_group1(group)));
    }
}

void add_skeleton(
    gh::milo::Directory& directory,
    const Gh1ClipSetBuildInput& input,
    Gh2ClipSetRole role,
    const std::map<std::string, ChannelContext>& contexts) {
    const auto converted_archetype =
        convert_gh1_directory_to_gh2_rnddir(
            input.archetype, "character_archetype");
    std::map<std::string, gh::milo_object::Mesh28>
        converted_meshes;
    for (const auto& entry : converted_archetype.directory.entries) {
        if (entry.type == "Mesh")
            converted_meshes.emplace(
                entry.name,
                gh::milo_object::parse_mesh28(entry.body_bytes));
    }

    const std::string object_extension =
        role == Gh2ClipSetRole::GuitarFret
            ? ".mesh"
            : ".trans";
    std::set<std::string> skeleton_bases;
    for (const auto& source_entry : input.archetype.entries) {
        if (source_entry.type != "Mesh") continue;
        const auto source_mesh =
            gh::milo_object::parse_mesh(source_entry.body_bytes);
        if (!source_mesh.vertices.empty() ||
            !source_mesh.faces.empty())
            continue;
        const std::string base =
            base_name(source_entry.name);
        skeleton_bases.insert(base);
        const auto converted =
            converted_meshes.find(source_entry.name);
        if (converted == converted_meshes.end())
            throw std::runtime_error(
                "milo convert: converted skeleton mesh missing " +
                source_entry.name);
        gh::milo_object::CharBone2 bone;
        bone.legacy_transform = converted->second.transformable;
        bone.legacy_transform.parent = replace_extension(
            bone.legacy_transform.parent, object_extension);
        bone.legacy_transform.target = replace_extension(
            bone.legacy_transform.target, object_extension);
        const auto context = contexts.find(base);
        if (context != contexts.end()) {
            bone.position_context = context->second.position;
            bone.scale_context = context->second.scale;
            bone.rotation = rotation_context(context->second);
        }
        bone.legacy_rotation = 9;
        directory.entries.push_back(make_entry(
            "CharBone", base + object_extension,
            gh::milo_object::serialize_char_bone2(bone)));
    }

    std::set<std::string> all_channel_bases;
    for (const auto& animation : input.spec.animations)
        for (const auto& channel : animation.channels)
            all_channel_bases.insert(base_name(channel));
    for (const auto& base : all_channel_bases) {
        if (skeleton_bases.count(base) ||
            is_facing_base(base))
            continue;
        if (!ends_with(base, "_hand"))
            throw std::runtime_error(
                "milo convert: unresolved non-anchor skeleton channel " +
                base);
        gh::milo_object::CharBone2 anchor;
        set_identity(anchor.legacy_transform.local);
        set_identity(anchor.legacy_transform.world);
        anchor.legacy_transform.parent =
            base.substr(0, base.size() - 5) +
            object_extension;
        const auto context = contexts.find(base);
        if (context != contexts.end()) {
            anchor.position_context = context->second.position;
            anchor.scale_context = context->second.scale;
            anchor.rotation = rotation_context(context->second);
        }
        anchor.legacy_rotation = 9;
        directory.entries.push_back(make_entry(
            "CharBone", base + object_extension,
            gh::milo_object::serialize_char_bone2(anchor)));
    }
}

bool all_recenter_targets_are_dynamic(
    const Gh1ClipSetBuildInput& input,
    const std::vector<size_t>& indices) {
    if (input.spec.recenter_channels.empty())
        return false;
    for (const size_t index : indices) {
        const auto& full = input.clips[index].channel_sets[0];
        if (full.sample_count < 2) return false;
        for (const auto& target : input.spec.recenter_channels) {
            const std::string position = target + ".pos";
            if (std::find(
                    full.channels.begin(), full.channels.end(),
                    position) == full.channels.end())
                return false;
        }
    }
    return true;
}

Gh2ClipSetPackage make_package(
    const Gh1ClipSetBuildInput& input,
    Gh2ClipSetRole role,
    const std::vector<size_t>& indices,
    const std::vector<
        std::vector<gh::milo_object::CharClipTransition5>>&
        all_transitions) {
    if (indices.empty())
        throw std::runtime_error(
            "milo convert: empty GH2 clip-set partition");

    Gh2ClipSetPackage package;
    package.role = role;
    package.directory_name =
        package_name(input.spec, role);
    auto& directory = package.directory;
    directory.dir_version = 24;
    directory.dir_type = "CharClipSet";
    directory.dir_name = package.directory_name;
    directory.boundaries_exact = true;
    directory.dir_terminator_value = kObjectTerminator;

    const auto [type, type_version] = role_type(role);
    gh::milo_object::CharClipSet14 root;
    root.object_directory.object_fields.type = type;
    root.object_directory.viewports =
        standard_clip_set_viewports();
    root.blend_width = input.spec.blend_width;
    root.play_flags =
        role == Gh2ClipSetRole::GuitarFret
            ? 32
            : convert_gh1_clip_time_flags_to_gh2(
                  input.spec.play_flags);
    root.move_self =
        input.spec.move_self ||
        role == Gh2ClipSetRole::GuitarMain ||
        all_recenter_targets_are_dynamic(input, indices);
    for (const auto& target : input.spec.recenter_channels)
        root.recenter_targets.push_back(
            target + ".trans");
    for (const auto& bone : input.spec.recenter_bones)
        root.recenter_average.push_back(bone + ".trans");
    root.recenter_slide = input.spec.recenter_slide;
    root.legacy_type = type;
    root.legacy_type_version = type_version;

    const auto source_has_band_flag =
        [&](uint32_t flag) {
            return std::any_of(
                input.spec.animations.begin(),
                input.spec.animations.end(),
                [flag](const Gh1AnimationSpec& animation) {
                    return (animation.flags & flag) != 0;
                });
        };
    const bool source_has_drum_modes =
        role == Gh2ClipSetRole::Band &&
        source_has_band_flag(0x00000008u) &&
        source_has_band_flag(0x00000010u) &&
        source_has_band_flag(0x00000020u);

    std::set<std::string> allowed_names;
    std::set<std::string> emitted_clip_names;
    for (const size_t index : indices)
        allowed_names.insert(
            input.spec.animations[index].name);
    std::map<std::string, ChannelContext> contexts;
    for (const size_t index : indices) {
        const auto& animation = input.spec.animations[index];
        const auto& source = input.clips[index];
        if (source.object_name != animation.name ||
            (source.flags & 0x7fffffffu) != animation.flags ||
            source.play_flags != animation.play_flags ||
            source.blend_width != animation.blend_width)
            throw std::runtime_error(
                "milo convert: manifest/ACP clip metadata differs for " +
                animation.name);
        std::vector<gh::milo_object::CharClipTransition5>
            transitions;
        if (!all_transitions.empty()) {
            for (const auto& transition :
                 all_transitions[index]) {
                if (allowed_names.count(transition.clip))
                    transitions.push_back(transition);
            }
        }
        auto clip =
            convert_gh1_acp_to_gh2_char_clip_samples10(
                source, transitions);
        if (role == Gh2ClipSetRole::GuitarMain) {
            clip.flags =
                convert_gh1_guitar_clip_flags_to_gh2(
                    clip.flags);
        } else if (role == Gh2ClipSetRole::Band) {
            clip.flags =
                convert_gh1_band_clip_flags_to_gh2(
                    clip.flags, source_has_drum_modes);
        }
        for (const auto& channel : clip.full.channels)
            add_context(contexts, channel);
        for (const auto& channel : clip.one.channels)
            add_context(contexts, channel);
        const auto body =
            gh::milo_object::serialize_char_clip_samples10(clip);
        const uint32_t allocation_size =
            static_cast<uint32_t>(
                gh::milo_object::
                    char_clip_samples10_ps2_allocate_size(clip));
        const auto append_clip =
            [&](const std::string& name) {
                if (!emitted_clip_names.insert(name).second)
                    throw std::runtime_error(
                        "milo convert: duplicate target clip " +
                        name);
                directory.entries.push_back(make_entry(
                    "CharClipSamples", name, body));
                root.clips.push_back(
                    {name, clip.flags, allocation_size});
            };
        append_clip(animation.name);
        if (role == Gh2ClipSetRole::GuitarStrum) {
            for (const auto& alias :
                 gh2_strum_clip_aliases_for_gh1_source(
                     animation.name))
                append_clip(alias);
        }
        package.source_clips.push_back(animation.name);
    }

    gh::milo_object::CharClipFilter0 filter;
    directory.entries.push_back(make_entry(
        "CharClipFilter", "clip_filter.ccf",
        gh::milo_object::serialize_char_clip_filter0(filter)));
    if (role == Gh2ClipSetRole::GuitarMain)
        add_guitar_groups(directory, input.spec, indices);
    add_skeleton(directory, input, role, contexts);
    directory.dir_body_bytes =
        gh::milo_object::serialize_char_clip_set14(root);

    const auto payload =
        gh::milo::serialize_directory(directory);
    const auto verify = gh::milo::parse_directory(payload);
    if (!verify.boundaries_exact ||
        gh::milo::serialize_directory(verify) != payload)
        throw std::runtime_error(
            "milo convert: generated CharClipSet round trip differs");
    package.directory = verify;
    return package;
}

}  // namespace

const char* gh2_clip_set_role_name(Gh2ClipSetRole role) {
    switch (role) {
        case Gh2ClipSetRole::GuitarMain: return "guitar-main";
        case Gh2ClipSetRole::GuitarUi: return "guitar-ui";
        case Gh2ClipSetRole::GuitarFret: return "guitar-fret";
        case Gh2ClipSetRole::GuitarStrum: return "guitar-strum";
        case Gh2ClipSetRole::Band: return "band";
        case Gh2ClipSetRole::Crowd: return "crowd";
        case Gh2ClipSetRole::Generic: return "generic";
    }
    return "unknown";
}

uint32_t convert_gh1_guitar_clip_flags_to_gh2(
    uint32_t source_flags) {
    return source_flags & ~guitar_group_membership_mask();
}

uint32_t convert_gh1_band_clip_flags_to_gh2(
    uint32_t source_flags,
    bool source_has_drum_modes) {
    constexpr uint32_t kGh1BandActive = 0x00000004u;
    constexpr uint32_t kGh1BandIdle = 0x00000040u;
    constexpr uint32_t kGh2BandNosnare = 0x00000200u;
    constexpr uint32_t kGh2BandIntro = 0x00000400u;
    constexpr uint32_t kGh2BandIntroIdle = 0x00001000u;
    if (source_has_drum_modes &&
        (source_flags & kGh1BandActive) != 0) {
        source_flags |= kGh2BandNosnare;
    }
    if ((source_flags & kGh1BandIdle) != 0) {
        source_flags |= kGh2BandIntro | kGh2BandIntroIdle;
    }
    return source_flags;
}

std::vector<std::string>
gh2_strum_clip_aliases_for_gh1_source(
    const std::string& source_clip) {
    if (source_clip == "strum_pluck_short")
        return {
            "strum_short_01", "strum_short_02",
            "strum_short_03", "strum_short_04"};
    if (source_clip == "strum_down_long")
        return {
            "strum_long_01", "strum_long_02",
            "strum_long_03", "strum_long_04"};
    if (source_clip == "strum_pluck_down")
        return {"strum_pick_01", "strum_pick_02"};
    return {};
}

std::vector<Gh2ClipSetPackage>
convert_gh1_clip_set_to_gh2_packages(
    const Gh1ClipSetBuildInput& input) {
    if (input.spec.animations.size() != input.clips.size())
        throw std::runtime_error(
            "milo convert: clip-set manifest/ACP count differs");
    std::vector<std::string> names;
    names.reserve(input.spec.animations.size());
    for (const auto& animation : input.spec.animations)
        names.push_back(animation.name);
    std::vector<
        std::vector<gh::milo_object::CharClipTransition5>>
        transitions;
    if (input.graph)
        transitions =
            convert_gh1_acg_to_gh2_char_clip_transitions(
                *input.graph, names);

    const Gh2ClipSetRole inferred =
        infer_role(input.spec);
    if (inferred != Gh2ClipSetRole::GuitarFret) {
        std::vector<size_t> indices(input.clips.size());
        for (size_t index = 0; index < indices.size(); ++index)
            indices[index] = index;
        return {make_package(
            input, inferred, indices, transitions)};
    }

    std::vector<size_t> fret;
    std::vector<size_t> strum;
    for (size_t index = 0;
         index < input.spec.animations.size(); ++index) {
        const auto& channels =
            input.spec.animations[index].channels;
        const bool has_fret =
            has_channel_base(channels, "bone_fret_hand");
        const bool has_strum =
            has_channel_base(channels, "bone_strum_hand");
        if (has_fret == has_strum)
            throw std::runtime_error(
                "milo convert: hand animation does not select exactly "
                "one instrument anchor");
        (has_fret ? fret : strum).push_back(index);
    }
    return {
        make_package(
            input, Gh2ClipSetRole::GuitarFret,
            fret, transitions),
        make_package(
            input, Gh2ClipSetRole::GuitarStrum,
            strum, transitions),
    };
}

}  // namespace gh::milo_convert
