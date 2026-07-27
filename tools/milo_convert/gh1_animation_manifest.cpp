#include "gh1_animation_manifest.h"

#include "acs.h"
#include "dtb.h"
#include "dtb_preprocess.h"

#include <algorithm>
#include <filesystem>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace gh::milo_convert {
namespace {

using Node = gh::dtb::Node;
using NodePtr = std::shared_ptr<Node>;
using NodeList = gh::dtb::NodeList;

std::string string_value(const NodePtr& node) {
    if (!node) return {};
    return gh::dtb::as_string(*node).value_or(std::string());
}

bool keyed(const NodePtr& node, const std::string& key) {
    if (!node || !gh::dtb::is_array(*node)) return false;
    const auto& values = gh::dtb::children(*node);
    return !values.empty() && string_value(values.front()) == key;
}

void collect_keyed(
    const NodePtr& node, const std::string& key,
    std::vector<NodePtr>& output) {
    if (!node || !gh::dtb::is_array(*node)) return;
    if (keyed(node, key)) {
        output.push_back(node);
        return;
    }
    for (const auto& child : gh::dtb::children(*node))
        collect_keyed(child, key, output);
}

NodePtr first_keyed(
    const NodePtr& node, const std::string& key) {
    std::vector<NodePtr> values;
    collect_keyed(node, key, values);
    return values.empty() ? nullptr : values.front();
}

NodePtr direct_keyed(
    const NodePtr& node, const std::string& key) {
    if (!node || !gh::dtb::is_array(*node)) return nullptr;
    for (const auto& child : gh::dtb::children(*node))
        if (keyed(child, key)) return child;
    return nullptr;
}

NodePtr first_clip_set_field(
    const NodePtr& node, const std::string& key) {
    if (!node || !gh::dtb::is_array(*node)) return nullptr;
    if (keyed(node, key)) return node;
    for (const char* excluded :
         {"animations", "recenter", "graph", "archetype", "deps"})
        if (keyed(node, excluded)) return nullptr;
    for (const auto& child : gh::dtb::children(*node)) {
        if (const NodePtr found =
                first_clip_set_field(child, key))
            return found;
    }
    return nullptr;
}

void collect_strings(
    const NodePtr& node, std::vector<std::string>& output,
    bool skip_first = false) {
    if (!node) return;
    if (gh::dtb::is_array(*node)) {
        const auto& values = gh::dtb::children(*node);
        for (size_t index = skip_first ? 1 : 0;
             index < values.size(); ++index)
            collect_strings(values[index], output);
        return;
    }
    const std::string value = string_value(node);
    if (!value.empty()) output.push_back(value);
}

void collect_numbers(
    const NodePtr& node, std::vector<uint32_t>& output,
    bool skip_first = false) {
    if (!node) return;
    if (gh::dtb::is_array(*node)) {
        const auto& values = gh::dtb::children(*node);
        for (size_t index = skip_first ? 1 : 0;
             index < values.size(); ++index)
            collect_numbers(values[index], output);
        return;
    }
    if (const auto integer = gh::dtb::as_int(*node))
        output.push_back(static_cast<uint32_t>(*integer));
}

uint32_t flags_node_value(
    const NodePtr& value, uint32_t fallback) {
    if (!value) return fallback;
    std::vector<uint32_t> numbers;
    collect_numbers(value, numbers, true);
    uint32_t result = 0;
    for (const uint32_t number : numbers) result |= number;
    return result;
}

uint32_t flags_value(
    const NodePtr& owner, const std::string& key,
    uint32_t fallback) {
    return flags_node_value(
        first_keyed(owner, key), fallback);
}

float scalar_node_value(
    const NodePtr& value, float fallback) {
    if (!value) return fallback;
    const auto& values = gh::dtb::children(*value);
    for (size_t index = 1; index < values.size(); ++index) {
        if (const auto floating = gh::dtb::as_float(*values[index]))
            return *floating;
        if (const auto integer = gh::dtb::as_int(*values[index]))
            return static_cast<float>(*integer);
    }
    return fallback;
}

float scalar_value(
    const NodePtr& owner, const std::string& key,
    float fallback) {
    return scalar_node_value(
        first_keyed(owner, key), fallback);
}

bool bool_node_value(
    const NodePtr& value, bool fallback) {
    if (!value) return fallback;
    const auto& values = gh::dtb::children(*value);
    for (size_t index = 1; index < values.size(); ++index) {
        if (const auto integer = gh::dtb::as_int(*values[index]))
            return *integer != 0;
        if (const auto floating =
                gh::dtb::as_float(*values[index]))
            return *floating != 0.0f;
    }
    return fallback;
}

bool bool_value(
    const NodePtr& owner, const std::string& key,
    bool fallback) {
    return bool_node_value(
        direct_keyed(owner, key), fallback);
}

std::vector<std::string> string_node_list(
    const NodePtr& value) {
    std::vector<std::string> result;
    if (value) collect_strings(value, result, true);
    return result;
}

std::vector<std::string> string_list(
    const NodePtr& owner, const std::string& key) {
    return string_node_list(direct_keyed(owner, key));
}

std::string single_string(
    const NodePtr& owner, const std::string& key) {
    const auto values = string_list(owner, key);
    return values.empty() ? std::string() : values.front();
}

std::string stem_without_extension(const std::string& value) {
    const size_t slash = value.find_last_of("/\\");
    const size_t begin =
        slash == std::string::npos ? 0 : slash + 1;
    const size_t dot = value.find_last_of('.');
    const size_t end =
        dot == std::string::npos || dot < begin ? value.size() : dot;
    return value.substr(begin, end - begin);
}

void collect_animation_entries(
    const NodePtr& animations, std::vector<NodePtr>& output) {
    if (!animations || !gh::dtb::is_array(*animations)) return;
    const auto& values = gh::dtb::children(*animations);
    const size_t begin = keyed(animations, "animations") ? 1 : 0;
    for (size_t index = begin; index < values.size(); ++index) {
        const auto& candidate = values[index];
        if (!candidate || !gh::dtb::is_array(*candidate)) continue;
        const auto& fields = gh::dtb::children(*candidate);
        const std::string first =
            fields.empty() ? std::string() :
                             string_value(fields.front());
        if (!first.empty() && first != "animations" &&
            first != "flags" && first != "play_flags" &&
            first != "blend_width" && first != "channels") {
            output.push_back(candidate);
        } else {
            collect_animation_entries(candidate, output);
        }
    }
}

std::string clip_set_target_name(
    const std::string& qualified_name) {
    const size_t scope = qualified_name.rfind("::");
    const std::string leaf =
        scope == std::string::npos
            ? qualified_name
            : qualified_name.substr(scope + 2);
    return stem_without_extension(leaf);
}

std::string clip_set_acg_path(
    const Gh1ClipSetSpec& set) {
    namespace fs = std::filesystem;
    fs::path result(set.source_directory);
    result /= set.target_name + ".acg";
    return result.lexically_normal().generic_string();
}

Gh1ClipSetSpec parse_clip_set(
    const std::string& invocation, const NodePtr& node) {
    if (!node || !gh::dtb::is_array(*node))
        throw std::runtime_error(
            "GH1 animation manifest: invocation is not an array: " +
            invocation);
    const auto& values = gh::dtb::children(*node);
    if (values.empty())
        throw std::runtime_error(
            "GH1 animation manifest: empty invocation: " +
            invocation);
    Gh1ClipSetSpec result;
    result.invocation = invocation;
    result.qualified_name = string_value(values.front());
    if (result.qualified_name.size() < 5 ||
        result.qualified_name.substr(
            result.qualified_name.size() - 5) != ".cset")
        throw std::runtime_error(
            "GH1 animation manifest: invocation does not produce a "
            "clip set: " + invocation);
    result.target_name =
        clip_set_target_name(result.qualified_name);
    result.source_directory =
        single_string(node, "directory");
    const NodePtr archetype = first_keyed(node, "archetype");
    result.archetype_rnd =
        archetype ? single_string(archetype, "rnd") :
                    std::string();
    result.play_flags = flags_node_value(
        first_clip_set_field(node, "play_flags"),
        0);
    result.blend_width = scalar_node_value(
        first_clip_set_field(node, "blend_width"),
        0.0f);
    result.move_self = bool_node_value(
        first_clip_set_field(node, "move_self"),
        false);
    result.channels = string_node_list(
        first_clip_set_field(node, "channels"));
    if (const NodePtr recenter =
            first_keyed(node, "recenter")) {
        result.recenter_channels =
            string_list(recenter, "channels");
        result.recenter_bones =
            string_list(recenter, "bones");
        result.recenter_slide =
            bool_value(recenter, "slide", false);
    }
    if (result.source_directory.empty())
        throw std::runtime_error(
            "GH1 animation manifest: missing directory: " +
            invocation);

    const NodePtr animations = first_keyed(node, "animations");
    if (!animations)
        throw std::runtime_error(
            "GH1 animation manifest: missing animations: " +
            invocation);
    std::vector<NodePtr> animation_nodes;
    collect_animation_entries(animations, animation_nodes);
    for (const auto& animation_node : animation_nodes) {
        const auto& fields = gh::dtb::children(*animation_node);
        Gh1AnimationSpec animation;
        animation.name = string_value(fields.front());
        animation.flags =
            flags_value(animation_node, "flags", 0);
        animation.play_flags = flags_value(
            animation_node, "play_flags", result.play_flags);
        animation.blend_width = scalar_value(
            animation_node, "blend_width", result.blend_width);
        animation.channels = string_node_list(
            first_keyed(animation_node, "channels"));
        if (animation.channels.empty())
            animation.channels = result.channels;
        animation.excluded_venues = string_node_list(
            first_keyed(animation_node, "exclude_venues"));
        if (animation.name.empty())
            throw std::runtime_error(
                "GH1 animation manifest: empty animation name: " +
                invocation);
        result.animations.push_back(std::move(animation));
    }
    if (result.animations.empty())
        throw std::runtime_error(
            "GH1 animation manifest: no animation entries: " +
            invocation);
    result.dependency_acg = clip_set_acg_path(result);
    return result;
}

NodePtr symbol_node(const std::string& value) {
    auto node = std::make_shared<Node>();
    node->tag = 0x05;
    node->value = value;
    return node;
}

}  // namespace

Gh1AnimationManifest compile_gh1_animation_manifest(
    const std::string& acs_path,
    const std::vector<uint8_t>& acs_bytes,
    const VirtualAssetReader& reader) {
    const auto acs = gh::acs::parse(acs_bytes);
    gh::dtb::MacroTable macros;
    std::vector<std::string> invocations;
    for (const auto& line : acs.lines) {
        if (line.kind == gh::acs::LineKind::Include) {
            const std::string compiled =
                gh::acs::compiled_include_path(
                    acs_path, line.value);
            gh::dtb::PreprocessOptions options;
            options.source_path = compiled;
            options.defines.insert("HX_EE");
            options.macro_table = &macros;
            options.contextual_include_resolver =
                [&](const std::string& including,
                    const std::string& include) {
                    gh::dtb::PreprocessOptions::IncludedFile result;
                    result.path =
                        gh::dtb::resolve_compiled_include_path(
                            including, include);
                    result.roots =
                        gh::dtb::parse(reader(result.path)).root;
                    return result;
                };
            const auto tree = gh::dtb::parse(reader(compiled));
            (void)gh::dtb::preprocess(tree.root, options);
        } else if (line.kind == gh::acs::LineKind::Invocation) {
            invocations.push_back(line.value);
        }
    }
    if (invocations.empty())
        throw std::runtime_error(
            "GH1 animation manifest: ACS has no invocations");

    Gh1AnimationManifest manifest;
    for (const auto& invocation : invocations) {
        gh::dtb::PreprocessOptions options;
        options.source_path = acs_path;
        options.defines.insert("HX_EE");
        options.macro_table = &macros;
        const NodeList expanded = gh::dtb::preprocess(
            {symbol_node(invocation)}, options);
        if (expanded.size() != 1)
            throw std::runtime_error(
                "GH1 animation manifest: invocation expansion count "
                "differs: " + invocation);
        manifest.clip_sets.push_back(
            parse_clip_set(invocation, expanded.front()));
    }
    return manifest;
}

std::string gh1_compiled_rnd_path(
    const std::string& authored_path) {
    namespace fs = std::filesystem;
    fs::path source(authored_path);
    if (source.extension() != ".rnd")
        throw std::runtime_error(
            "GH1 animation manifest: archetype is not .rnd: " +
            authored_path);
    fs::path filename = source.filename();
    filename.replace_extension(".rnd_ps2");
    return (source.parent_path() / "gen" / filename)
        .lexically_normal()
        .generic_string();
}

std::string gh1_animation_manifest_tsv(
    const Gh1AnimationManifest& manifest) {
    std::ostringstream output;
    output
        << "invocation\tclip_set\ttarget\tdirectory\tarchetype"
           "\tcompiled_archetype\tacg"
           "\tset_play_flags\tset_blend_width\tmove_self"
           "\trecenter_channels\trecenter_bones"
           "\trecenter_slide\tanimation\tflags\tplay_flags"
           "\tblend_width\tchannels\texcluded_venues\n";
    for (const auto& set : manifest.clip_sets) {
        for (const auto& animation : set.animations) {
            output << set.invocation << '\t'
                   << set.qualified_name << '\t'
                   << set.target_name << '\t'
                   << set.source_directory << '\t'
                   << set.archetype_rnd << '\t'
                   << gh1_compiled_rnd_path(
                          set.archetype_rnd) << '\t'
                   << set.dependency_acg << '\t'
                   << set.play_flags << '\t'
                   << set.blend_width << '\t'
                   << (set.move_self ? 1 : 0) << '\t';
            for (size_t index = 0;
                 index < set.recenter_channels.size(); ++index) {
                if (index) output << ',';
                output << set.recenter_channels[index];
            }
            output << '\t';
            for (size_t index = 0;
                 index < set.recenter_bones.size(); ++index) {
                if (index) output << ',';
                output << set.recenter_bones[index];
            }
            output << '\t'
                   << (set.recenter_slide ? 1 : 0) << '\t'
                   << animation.name << '\t'
                   << animation.flags << '\t'
                   << animation.play_flags << '\t'
                   << animation.blend_width << '\t';
            for (size_t index = 0;
                 index < animation.channels.size(); ++index) {
                if (index) output << ',';
                output << animation.channels[index];
            }
            output << '\t';
            for (size_t index = 0;
                 index < animation.excluded_venues.size(); ++index) {
                if (index) output << ',';
                output << animation.excluded_venues[index];
            }
            output << '\n';
        }
    }
    return output.str();
}

}  // namespace gh::milo_convert
