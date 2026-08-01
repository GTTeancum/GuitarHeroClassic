#include "gh1_character_manifest.h"

#include "dtb.h"
#include "dtb_preprocess.h"

#include <algorithm>
#include <filesystem>
#include <set>
#include <sstream>
#include <stdexcept>

namespace gh::milo_convert {
namespace {

using Node = gh::dtb::Node;
using NodeList = gh::dtb::NodeList;
using NodePtr = std::shared_ptr<Node>;

std::string string_value(const NodePtr& node) {
    if (!node) return {};
    return gh::dtb::as_string(*node).value_or(std::string());
}

bool keyed(const NodePtr& node, const std::string& key) {
    if (!node || !gh::dtb::is_array(*node)) return false;
    const auto& values = gh::dtb::children(*node);
    return !values.empty() && string_value(values.front()) == key;
}

NodePtr direct_keyed(
    const NodePtr& node, const std::string& key) {
    if (!node || !gh::dtb::is_array(*node)) return nullptr;
    for (const auto& child : gh::dtb::children(*node))
        if (keyed(child, key)) return child;
    return nullptr;
}

NodePtr first_keyed(
    const NodePtr& node, const std::string& key) {
    if (!node || !gh::dtb::is_array(*node)) return nullptr;
    if (keyed(node, key)) return node;
    for (const auto& child : gh::dtb::children(*node)) {
        if (const auto found = first_keyed(child, key))
            return found;
    }
    return nullptr;
}

void collect_keyed(
    const NodePtr& node, const std::string& key,
    std::vector<NodePtr>& output) {
    if (!node || !gh::dtb::is_array(*node)) return;
    if (keyed(node, key)) output.push_back(node);
    for (const auto& child : gh::dtb::children(*node))
        collect_keyed(child, key, output);
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

void collect_floats(
    const NodePtr& node, std::vector<float>& output,
    bool skip_first = false) {
    if (!node) return;
    if (gh::dtb::is_array(*node)) {
        const auto& values = gh::dtb::children(*node);
        for (size_t index = skip_first ? 1 : 0;
             index < values.size(); ++index)
            collect_floats(values[index], output);
        return;
    }
    if (const auto value = gh::dtb::as_float(*node))
        output.push_back(*value);
    else if (const auto value = gh::dtb::as_int(*node))
        output.push_back(static_cast<float>(*value));
}

std::string first_string_after_key(const NodePtr& node) {
    std::vector<std::string> values;
    collect_strings(node, values, true);
    return values.empty() ? std::string() : values.front();
}

float first_float_after_key(
    const NodePtr& node, float fallback = 0.0f) {
    std::vector<float> values;
    collect_floats(node, values, true);
    return values.empty() ? fallback : values.front();
}

int32_t first_int_after_key(
    const NodePtr& node, int32_t fallback = 0) {
    if (!node || !gh::dtb::is_array(*node)) return fallback;
    const auto& values = gh::dtb::children(*node);
    for (size_t index = 1; index < values.size(); ++index) {
        if (const auto value = gh::dtb::as_int(*values[index]))
            return *value;
        if (const auto value = gh::dtb::as_float(*values[index]))
            return static_cast<int32_t>(*value);
    }
    return fallback;
}

bool bool_field(
    const NodePtr& owner, const std::string& key,
    bool fallback = false) {
    const auto field = first_keyed(owner, key);
    if (!field) return fallback;
    return first_float_after_key(field, fallback ? 1.0f : 0.0f) !=
           0.0f;
}

std::string asset_stem(const std::string& value) {
    namespace fs = std::filesystem;
    return fs::path(value).stem().string();
}

std::string compiled_skeleton_path(
    const std::string& directory, const std::string& skeleton) {
    namespace fs = std::filesystem;
    fs::path result(directory);
    result /= "gen";
    result /= skeleton + ".rnd_ps2";
    return result.lexically_normal().generic_string();
}

void collect_character_nodes(
    const NodePtr& node, std::vector<NodePtr>& output) {
    if (!node || !gh::dtb::is_array(*node)) return;
    if (direct_keyed(node, "outfit") &&
        direct_keyed(node, "bone.servo") &&
        direct_keyed(node, "lod_screen_sizes")) {
        output.push_back(node);
        return;
    }
    for (const auto& child : gh::dtb::children(*node))
        collect_character_nodes(child, output);
}

NodePtr find_command(
    const NodePtr& node, const std::string& command) {
    if (!node || !gh::dtb::is_array(*node)) return nullptr;
    const auto& values = gh::dtb::children(*node);
    if (values.size() > 1 &&
        string_value(values[1]) == command)
        return node;
    for (const auto& child : values)
        if (const auto found = find_command(child, command))
            return found;
    return nullptr;
}

void collect_commands(
    const NodePtr& node, const std::string& command,
    std::vector<NodePtr>& output) {
    if (!node || !gh::dtb::is_array(*node)) return;
    const auto& values = gh::dtb::children(*node);
    if (values.size() > 1 &&
        string_value(values[1]) == command)
        output.push_back(node);
    for (const auto& child : values)
        collect_commands(child, command, output);
}

std::vector<Gh1FaceEventSpec> parse_face_events(
    const NodePtr& node) {
    std::vector<Gh1FaceEventSpec> result;
    if (!node || !gh::dtb::is_array(*node)) return result;
    const auto& rows = gh::dtb::children(*node);
    for (size_t index = 1; index < rows.size(); ++index) {
        if (!gh::dtb::is_array(*rows[index])) continue;
        const auto& values = gh::dtb::children(*rows[index]);
        if (values.size() < 2) continue;
        const auto event = gh::dtb::as_int(*values[0]);
        const std::string pose = string_value(values[1]);
        if (!event || pose.empty())
            throw std::runtime_error(
                "GH1 character manifest: malformed face event");
        result.push_back({*event, pose});
    }
    return result;
}

Gh1CharacterFaceSpec parse_face(
    const NodePtr& node,
    const std::vector<Gh1FaceEventSpec>& face_events,
    const std::vector<Gh1FaceEventSpec>& singer_events) {
    Gh1CharacterFaceSpec result;
    const auto face_data = first_keyed(node, "face_data");
    if (!face_data) return result;
    result.present = true;
    if (const auto poses = direct_keyed(face_data, "poses"))
        collect_strings(poses, result.poses, true);
    if (const auto excitement =
            direct_keyed(face_data, "excitement_poses")) {
        const auto& rows = gh::dtb::children(*excitement);
        for (size_t index = 1; index < rows.size(); ++index) {
            if (!gh::dtb::is_array(*rows[index])) continue;
            const auto& values = gh::dtb::children(*rows[index]);
            if (values.empty()) continue;
            Gh1FaceExcitementSpec entry;
            const auto state = gh::dtb::as_int(*values.front());
            if (!state)
                throw std::runtime_error(
                    "GH1 character manifest: face excitement state "
                    "is not an integer");
            entry.state = *state;
            collect_strings(rows[index], entry.poses, true);
            if (entry.poses.empty())
                throw std::runtime_error(
                    "GH1 character manifest: malformed face "
                    "excitement row");
            result.excitement_poses.push_back(std::move(entry));
        }
    }
    result.blend_time = first_float_after_key(
        direct_keyed(face_data, "blend_time"));
    if (const auto pose_length =
            direct_keyed(face_data, "pose_length")) {
        result.has_pose_length = true;
        result.pose_length = first_float_after_key(pose_length);
    }
    result.event_list = first_string_after_key(
        direct_keyed(face_data, "event_list"));
    if (const auto event_offset =
            direct_keyed(face_data, "event_offset")) {
        result.has_event_offset = true;
        result.event_offset = first_int_after_key(event_offset);
    }
    if (result.event_list.empty()) {
        // The GH1 face controller defaults this symbol to `hero` before
        // looking up the optional authored event_list override.
        result.event_list = "hero";
        result.events = face_events;
    } else if (result.event_list == "singer") {
        result.events = singer_events;
    } else if (result.event_list == "hero") {
        result.events = face_events;
    } else {
        throw std::runtime_error(
            "GH1 character manifest: unsupported face event list " +
            result.event_list);
    }
    if (result.poses.empty())
        throw std::runtime_error(
            "GH1 character manifest: face data has no poses");
    return result;
}

Gh1CharacterControllerSpec parse_controller(
    const NodePtr& node) {
    const auto& values = gh::dtb::children(*node);
    if (values.size() < 3)
        throw std::runtime_error(
            "GH1 character manifest: short new-controller row");
    Gh1CharacterControllerSpec result;
    result.class_name = string_value(values[1]);
    result.name = string_value(values[2]);
    if (result.class_name == "AnimServoForeTwist")
        result.kind = Gh1CharacterControllerKind::ForeTwist;
    else if (result.class_name == "AnimServoUpperTwist")
        result.kind = Gh1CharacterControllerKind::UpperTwist;
    else if (result.class_name == "AnimServoIK")
        result.kind = Gh1CharacterControllerKind::HandIk;
    else if (result.class_name == "AnimServoBone")
        result.kind = Gh1CharacterControllerKind::ServoBone;
    else if (result.class_name == "AnimDriver")
        result.kind = Gh1CharacterControllerKind::Driver;
    else if (result.class_name == "AnimServoRig")
        result.kind = Gh1CharacterControllerKind::Rig;
    else if (result.class_name == "AnimServoPosConstraint")
        result.kind = Gh1CharacterControllerKind::PosConstraint;
    else
        throw std::runtime_error(
            "GH1 character manifest: unsupported controller class " +
            result.class_name);
    if (const auto bones = direct_keyed(node, "bones")) {
        collect_strings(bones, result.bones, true);
        const auto& children = gh::dtb::children(*bones);
        for (size_t index = 1; index < children.size(); ++index)
            if (const auto bend = gh::dtb::as_int(*children[index]))
                result.bend = *bend;
    }
    result.source =
        first_string_after_key(direct_keyed(node, "source"));
    result.destination =
        first_string_after_key(direct_keyed(node, "dest"));
    if (const auto sources = direct_keyed(node, "srcbones"))
        collect_strings(sources, result.sources, true);
    if (const auto destinations = direct_keyed(node, "dstbones"))
        collect_strings(
            destinations, result.destinations, true);
    if (const auto destination = direct_keyed(node, "dstbone"))
        collect_strings(
            destination, result.destinations, true);
    if (const auto channels = direct_keyed(node, "channels"))
        collect_strings(channels, result.channels, true);
    if (const auto rig_bones = direct_keyed(node, "rigbones")) {
        const auto& rows = gh::dtb::children(*rig_bones);
        for (size_t index = 1; index < rows.size(); ++index) {
            if (!gh::dtb::is_array(*rows[index])) continue;
            const auto& pair = gh::dtb::children(*rows[index]);
            if (pair.size() < 2) continue;
            const std::string bone = string_value(pair[0]);
            float weight = 0.0f;
            if (const auto value = gh::dtb::as_float(*pair[1]))
                weight = *value;
            else if (const auto value = gh::dtb::as_int(*pair[1]))
                weight = static_cast<float>(*value);
            if (!bone.empty())
                result.rig_bones.push_back({bone, weight});
        }
    }
    result.side_axis =
        first_string_after_key(direct_keyed(node, "side_axis"));
    result.offset =
        first_float_after_key(direct_keyed(node, "offset"));
    result.align_quaternion =
        bool_field(node, "align_quat");
    result.stretch = bool_field(node, "stretch");
    result.vertical = bool_field(node, "vertical");
    if (result.name.empty())
        throw std::runtime_error(
            "GH1 character manifest: controller lacks name");
    return result;
}

Gh1CharacterSpec parse_character(
    const NodePtr& node, bool band_character,
    const std::vector<Gh1FaceEventSpec>& face_events,
    const std::vector<Gh1FaceEventSpec>& singer_events) {
    const auto& values = gh::dtb::children(*node);
    if (values.empty())
        throw std::runtime_error(
            "GH1 character manifest: empty character row");
    Gh1CharacterSpec result;
    result.authored_name = string_value(values.front());
    result.band_character = band_character;
    if (!band_character) {
        result.role = Gh1CharacterRole::Guitarist;
    } else if (result.authored_name == "singer") {
        result.role = Gh1CharacterRole::Singer;
    } else if (result.authored_name == "bass") {
        result.role = Gh1CharacterRole::Bassist;
    } else if (result.authored_name == "drummer") {
        result.role = Gh1CharacterRole::Drummer;
    } else if (result.authored_name == "keyboard") {
        result.role = Gh1CharacterRole::Keyboardist;
    } else {
        throw std::runtime_error(
            "GH1 character manifest: unsupported authored band role " +
            result.authored_name);
    }

    const auto outfit = direct_keyed(node, "outfit");
    result.source_directory = first_string_after_key(
        first_keyed(outfit, "directory"));
    const auto skeleton = first_keyed(outfit, "skeleton");
    result.source_skeleton =
        first_string_after_key(first_keyed(skeleton, "load"));
    result.package_name = asset_stem(result.source_skeleton);
    result.compiled_skeleton = compiled_skeleton_path(
        result.source_directory, result.source_skeleton);

    collect_floats(
        direct_keyed(node, "lod_screen_sizes"),
        result.lod_screen_sizes, true);
    const auto sphere = direct_keyed(node, "sphere");
    std::vector<float> sphere_numbers;
    collect_floats(sphere, sphere_numbers, true);
    if (!sphere_numbers.empty()) {
        if (sphere_numbers.size() != 4)
            throw std::runtime_error(
                "GH1 character manifest: sphere does not have four "
                "numbers");
        std::copy(
            sphere_numbers.begin(), sphere_numbers.end(),
            result.sphere.begin());
        std::vector<std::string> sphere_strings;
        collect_strings(sphere, sphere_strings, true);
        if (!sphere_strings.empty())
            result.sphere_base = sphere_strings.back();
    }

    const auto servo = direct_keyed(node, "bone.servo");
    result.use_delta = bool_field(servo, "use_delta");
    if (const auto channels = first_keyed(servo, "channels"))
        collect_strings(
            channels, result.servo_channels, true);

    std::vector<NodePtr> driver_rows;
    collect_keyed(node, "main.drv", driver_rows);
    for (const auto& row : driver_rows) {
        if (const auto realign = direct_keyed(row, "realign"))
            result.main_driver_realign =
                first_float_after_key(realign) != 0.0f;
    }

    std::vector<NodePtr> controller_rows;
    collect_keyed(node, "new", controller_rows);
    for (const auto& row : controller_rows) {
        const auto& fields = gh::dtb::children(*row);
        if (fields.size() > 1) {
            const std::string class_name =
                string_value(fields[1]);
            if (class_name == "AnimServoForeTwist" ||
                class_name == "AnimServoUpperTwist" ||
                class_name == "AnimServoIK" ||
                class_name == "AnimServoBone" ||
                class_name == "AnimDriver" ||
                class_name == "AnimServoRig" ||
                class_name == "AnimServoPosConstraint")
                result.controllers.push_back(
                    parse_controller(row));
        }
    }
    std::vector<NodePtr> connect_rows;
    collect_commands(node, "connect", connect_rows);
    for (const auto& row : connect_rows) {
        const auto& fields = gh::dtb::children(*row);
        if (fields.size() < 3) continue;
        const std::string servo = string_value(fields[0]);
        const std::string driver = string_value(fields[2]);
        const auto found = std::find_if(
            result.controllers.begin(), result.controllers.end(),
            [&](const Gh1CharacterControllerSpec& controller) {
                return controller.name == servo;
            });
        if (found != result.controllers.end())
            found->connected_driver = driver;
    }

    if (const auto eyes = find_command(node, "create_eyes")) {
        result.eyes.present = true;
        result.eyes.parent = first_string_after_key(
            direct_keyed(eyes, "parent"));
        result.eyes.constraint = first_float_after_key(
            direct_keyed(eyes, "constraint"));
        result.eyes.lower_lid = first_float_after_key(
            direct_keyed(eyes, "lid_lower"));
    }
    if (const auto walk = find_command(node, "create_walk")) {
        result.walk.present = true;
        result.walk.turn_flags = static_cast<uint32_t>(
            first_float_after_key(direct_keyed(walk, "turn")));
        result.walk.stop_flags = static_cast<uint32_t>(
            first_float_after_key(direct_keyed(walk, "stop")));
        result.walk.walk_flags = static_cast<uint32_t>(
            first_float_after_key(direct_keyed(walk, "walk")));
    }
    result.face = parse_face(node, face_events, singer_events);
    result.face_file = first_string_after_key(
        direct_keyed(node, "face_file"));
    if (const auto shadow = find_command(node, "show_shadow")) {
        const auto& fields = gh::dtb::children(*shadow);
        if (fields.size() > 2)
            result.shadow_file = string_value(fields[2]);
    }

    if (result.authored_name.empty() ||
        result.source_directory.empty() ||
        result.source_skeleton.empty() ||
        result.lod_screen_sizes.empty() ||
        result.servo_channels.empty())
        throw std::runtime_error(
            "GH1 character manifest: incomplete character row " +
            result.authored_name);
    return result;
}

}  // namespace

Gh1CharacterManifest compile_gh1_character_manifest(
    const std::string& dtb_path,
    const std::vector<uint8_t>& dtb_bytes,
    const VirtualAssetReader& reader) {
    gh::dtb::MacroTable macros;
    gh::dtb::PreprocessOptions options;
    options.source_path = dtb_path;
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
    const auto parsed = gh::dtb::parse(dtb_bytes);
    const NodeList expanded =
        gh::dtb::preprocess(parsed.root, options);

    const NodePtr synthetic = std::make_shared<Node>();
    synthetic->tag = 0x10;
    synthetic->value = expanded;
    const auto archetypes =
        first_keyed(synthetic, "archetypes");
    const auto band =
        first_keyed(synthetic, "band_for_budget");
    if (!archetypes || !band)
        throw std::runtime_error(
            "GH1 character manifest: missing archetype or band domain");
    const auto face_events = parse_face_events(
        first_keyed(synthetic, "face_events"));
    const auto singer_events = parse_face_events(
        first_keyed(synthetic, "singer_events"));

    std::vector<NodePtr> guitarist_nodes;
    std::vector<NodePtr> band_nodes;
    collect_character_nodes(archetypes, guitarist_nodes);
    collect_character_nodes(band, band_nodes);

    Gh1CharacterManifest manifest;
    std::set<std::string> identities;
    for (const auto& node : guitarist_nodes) {
        auto character = parse_character(
            node, false, face_events, singer_events);
        if (identities.insert(character.compiled_skeleton).second)
            manifest.characters.push_back(std::move(character));
    }
    for (const auto& node : band_nodes) {
        auto character = parse_character(
            node, true, face_events, singer_events);
        if (identities.insert(character.compiled_skeleton).second)
            manifest.characters.push_back(std::move(character));
    }
    if (manifest.characters.empty())
        throw std::runtime_error(
            "GH1 character manifest: no characters compiled");
    return manifest;
}

std::vector<Gh1CharacterTrackSurfaceSpec>
compile_gh1_character_track_surfaces(
    const std::vector<uint8_t>& dtb_bytes) {
    const auto parsed = gh::dtb::parse(dtb_bytes);
    std::vector<Gh1CharacterTrackSurfaceSpec> result;
    std::set<std::string> identities;
    for (const auto& row : parsed.root) {
        if (!row || !gh::dtb::is_array(*row)) continue;
        const auto& values = gh::dtb::children(*row);
        if (values.empty()) continue;
        const std::string authored_name =
            string_value(values.front());
        const auto surface =
            direct_keyed(row, "track_surface");
        const std::string source_surface =
            first_string_after_key(surface);
        if (authored_name.empty() || source_surface.empty())
            throw std::runtime_error(
                "GH1 character track surfaces: incomplete row");
        if (!identities.insert(authored_name).second)
            throw std::runtime_error(
                "GH1 character track surfaces: duplicate character " +
                authored_name);
        result.push_back({authored_name, source_surface});
    }
    if (result.empty())
        throw std::runtime_error(
            "GH1 character track surfaces: no rows compiled");
    return result;
}

std::string gh1_character_manifest_tsv(
    const Gh1CharacterManifest& manifest) {
    const auto role_name = [](Gh1CharacterRole role) {
        switch (role) {
        case Gh1CharacterRole::Guitarist:
            return "guitarist";
        case Gh1CharacterRole::Singer:
            return "singer";
        case Gh1CharacterRole::Bassist:
            return "bassist";
        case Gh1CharacterRole::Drummer:
            return "drummer";
        case Gh1CharacterRole::Keyboardist:
            return "keyboardist";
        }
        throw std::runtime_error(
            "GH1 character manifest: invalid character role");
    };
    std::ostringstream output;
    output
        << "authored_name\tpackage\tband\trole\tdirectory\tskeleton"
           "\tcompiled_skeleton\tlods\tsphere\tsphere_base"
           "\tuse_delta\tservo_channels\trealign\tcontrollers"
           "\teyes\teye_parent\teye_constraint\teye_lower_lid"
           "\twalk\tface_file\tface_poses"
           "\tface_pose_names\tface_excitement_states"
           "\tface_excitement_map\tface_blend_time"
           "\tface_pose_length\tface_event_list"
           "\tface_event_offset\tface_events\tshadow_file\n";
    for (const auto& character : manifest.characters) {
        output << character.authored_name << '\t'
               << character.package_name << '\t'
               << (character.band_character ? 1 : 0) << '\t'
               << role_name(character.role) << '\t'
               << character.source_directory << '\t'
               << character.source_skeleton << '\t'
               << character.compiled_skeleton << '\t';
        for (size_t i = 0;
             i < character.lod_screen_sizes.size(); ++i) {
            if (i) output << ',';
            output << character.lod_screen_sizes[i];
        }
        output << '\t';
        for (size_t i = 0; i < character.sphere.size(); ++i) {
            if (i) output << ',';
            output << character.sphere[i];
        }
        output << '\t' << character.sphere_base << '\t'
               << (character.use_delta ? 1 : 0) << '\t'
               << character.servo_channels.size() << '\t'
               << (character.main_driver_realign ? 1 : 0)
               << '\t' << character.controllers.size()
               << '\t' << (character.eyes.present ? 1 : 0)
               << '\t' << character.eyes.parent
               << '\t' << character.eyes.constraint
               << '\t' << character.eyes.lower_lid
               << '\t' << (character.walk.present ? 1 : 0)
               << '\t' << character.face_file << '\t'
               << character.face.poses.size() << '\t';
        for (size_t i = 0; i < character.face.poses.size(); ++i) {
            if (i) output << ',';
            output << character.face.poses[i];
        }
        output << '\t'
               << character.face.excitement_poses.size() << '\t';
        for (size_t i = 0;
             i < character.face.excitement_poses.size(); ++i) {
            if (i) output << ';';
            const auto& excitement =
                character.face.excitement_poses[i];
            output << excitement.state << ':';
            for (size_t pose = 0;
                 pose < excitement.poses.size(); ++pose) {
                if (pose) output << ',';
                output << excitement.poses[pose];
            }
        }
        output << '\t'
               << character.face.blend_time << '\t';
        if (character.face.has_pose_length)
            output << character.face.pose_length;
        output << '\t' << character.face.event_list << '\t';
        if (character.face.has_event_offset)
            output << character.face.event_offset;
        output << '\t';
        for (size_t i = 0; i < character.face.events.size(); ++i) {
            if (i) output << ',';
            output << character.face.events[i].event << ':'
                   << character.face.events[i].pose;
        }
        output << '\t' << character.shadow_file << '\n';
    }
    return output.str();
}

}  // namespace gh::milo_convert
