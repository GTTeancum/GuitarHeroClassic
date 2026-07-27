#include "gh2_face_config_patch.h"

#include "dtb.h"

#include <array>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace gh::milo_convert {
namespace {

std::string transition_script(size_t pose_count) {
    std::ostringstream output;
    output
        << "  (current_pose 0)\n"
        << "  (transition_to\n"
        << "    ($pose)\n"
        << "    {switch [current_pose]\n";
    for (size_t from = 0; from < pose_count; ++from) {
        output << "      (" << from << " {switch $pose\n";
        for (size_t to = 0; to < pose_count; ++to)
            output
                << "        (" << to << " {gh1_face_"
                << from << '_' << to << ".trig trigger})\n";
        output << "      })\n";
    }
    output
        << "    }\n"
        << "    {set [current_pose] $pose}\n"
        << "  )\n";
    return output.str();
}

void append_pool_script(
    std::ostringstream& output,
    std::string_view name,
    const std::vector<size_t>& poses) {
    if (poses.empty())
        throw std::runtime_error(
            "milo convert: empty GH1 face pose pool");
    const auto append_switch = [&]() {
        output << "      {switch $choice\n";
        for (size_t index = 0; index < poses.size(); ++index)
            output
                << "        (" << index
                << " {set $pose " << poses[index] << "})\n";
        output << "      }\n";
    };
    output
        << "  (" << name << "\n"
        << "    {do\n"
        << "      ($choice)\n"
        << "      ($pose)\n"
        << "      {set $choice {random_int 0 "
        << poses.size() << "}}\n";
    append_switch();
    output
        << "      {if {== $pose [current_pose]}\n"
        << "        {do\n"
        << "          {++ $choice}\n"
        << "          {if {>= $choice " << poses.size()
        << "} {set $choice 0}}\n";
    append_switch();
    output
        << "        }\n"
        << "      }\n"
        << "      {$this transition_to $pose}\n"
        << "    }\n"
        << "  )\n";
}

std::string guitarist_type() {
    std::ostringstream output;
    output << "(gh1_guitarist_morph_face\n";
    output << transition_script(10);
    output
        << "  (override_expression FALSE)\n"
        << "  (pose_length 1.0)\n"
        << "  (do_expression\n"
        << "    ($expressionName $blend)\n"
        << "    {switch $expressionName\n"
        << "      (Neutral {$this transition_to 0})\n"
        << "      (expressionBad1 {$this transition_to 1})\n"
        << "      (expressionBad2 {$this transition_to 2})\n"
        << "      (expressionBad3 {$this transition_to 3})\n"
        << "      (expressionGood1 {$this transition_to 4})\n"
        << "      (expressionGood2 {$this transition_to 5})\n"
        << "      (expressionGood3 {$this transition_to 6})\n"
        << "      (expressionGood4 {$this transition_to 7})\n"
        << "      (expressionGood5 {$this transition_to 8})\n"
        << "      (EyesClosed {$this transition_to 9})\n"
        << "    }\n"
        << "  )\n"
        << "  (override_expression\n"
        << "    ($expressionName)\n"
        << "    {$this do_expression $expressionName 1}\n"
        << "    {set [override_expression] TRUE}\n"
        << "  )\n"
        << "  (resume_random_expression\n"
        << "    {if {== [override_expression] TRUE}\n"
        << "      {set [override_expression] FALSE}}\n"
        << "    {$this do_pick_expression}\n"
        << "  )\n";
    append_pool_script(
        output, "pick_bad_expression", {1, 2, 3});
    append_pool_script(
        output, "pick_okay_expression", {0, 4});
    append_pool_script(
        output, "pick_good_expression", {4, 5, 6, 7, 8});
    output
        << "  (do_pick_expression\n"
        << "    {if {&& {exists game} {! [override_expression]}}\n"
        << "      {switch {game get excitement}\n"
        << "        (kExcitementBoot {$this pick_bad_expression})\n"
        << "        (kExcitementBad {$this pick_bad_expression})\n"
        << "        (kExcitementOkay {$this pick_okay_expression})\n"
        << "        (kExcitementGreat {$this pick_good_expression})\n"
        << "        (kExcitementPeak {$this pick_good_expression})\n"
        << "      }\n"
        << "    }\n"
        << "  )\n"
        << "  (pick_expression\n"
        << "    {if {exists gh1_face_expression_task}\n"
        << "      {delete gh1_face_expression_task}}\n"
        << "    {thread_task\n"
        << "      (units kTaskSeconds)\n"
        << "      (name gh1_face_expression_task)\n"
        << "      (script\n"
        << "        {$this do_pick_expression}\n"
        << "        {$task sleep [pose_length]}\n"
        << "        {$task loop}\n"
        << "      )\n"
        << "    }\n"
        << "  )\n"
        << ")\n";
    return output.str();
}

std::string singer_type() {
    std::ostringstream output;
    output << "(gh1_singer_morph_face\n";
    output << transition_script(2);
    output
        << "  (do_expression\n"
        << "    ($expressionName $blend)\n"
        << "    {switch $expressionName\n"
        << "      (Neutral {$this transition_to 0})\n"
        << "      (ref {$this transition_to 0})\n"
        << "      (open {$this transition_to 1})\n"
        << "    }\n"
        << "  )\n"
        << "  (singer_face_open {$this transition_to 1})\n"
        << "  (singer_face_close {$this transition_to 0})\n"
        << ")\n";
    return output.str();
}

std::shared_ptr<gh::dtb::Node> find_group_root(
    gh::dtb::Tree& tree) {
    std::shared_ptr<gh::dtb::Node> result;
    for (const auto& root : tree.root) {
        if (!root || !gh::dtb::is_array(*root))
            continue;
        const auto& children = gh::dtb::children(*root);
        if (children.empty() || !children.front())
            continue;
        if (gh::dtb::as_string(*children.front()).value_or("") !=
            "Group")
            continue;
        if (result)
            throw std::runtime_error(
                "milo convert: rnd_objects has duplicate Group roots");
        result = root;
    }
    if (!result)
        throw std::runtime_error(
            "milo convert: rnd_objects has no Group root");
    return result;
}

}  // namespace

std::string gh1_face_controller_types_dta() {
    return guitarist_type() + singer_type();
}

std::string gh1_singer_face_midi_parser_dta() {
    return
        "(gh1_singer_face_parser\n"
        "  (type midi)\n"
        "  (eval TRUE)\n"
        "  (track_name GH1 SINGER FACE)\n"
        "  (zero_length TRUE)\n"
        "  (mappings\n"
        "    (default\n"
        "      (events\n"
        "        (108 ({singer_parser singer_face_open}))\n"
        "        (109 ({singer_parser singer_face_close}))\n"
        "      )\n"
        "    )\n"
        "  )\n"
        ")\n";
}

Gh2FaceConfigPatch patch_gh2_rnd_objects_for_gh1_faces(
    const std::vector<uint8_t>& clean_rnd_objects) {
    gh::dtb::Tree target =
        gh::dtb::parse(clean_rnd_objects);
    const auto group = find_group_root(target);
    const auto types =
        gh::dtb::find_keyed(*group, "types");
    if (!types)
        throw std::runtime_error(
            "milo convert: rnd_objects Group has no types row");
    auto& type_children =
        std::get<gh::dtb::NodeList>(types->value);

    const auto source =
        gh::dtb::parse_dta(
            gh1_face_controller_types_dta());
    if (source.root.size() != 2)
        throw std::runtime_error(
            "milo convert: generated face config type count differs");
    const std::array<std::string_view, 2> expected = {
        "gh1_guitarist_morph_face",
        "gh1_singer_morph_face"};
    for (size_t index = 0; index < expected.size(); ++index) {
        const auto& candidate = source.root[index];
        if (!candidate || !gh::dtb::is_array(*candidate) ||
            gh::dtb::children(*candidate).empty() ||
            gh::dtb::as_string(
                *gh::dtb::children(*candidate).front())
                    .value_or("") != expected[index])
            throw std::runtime_error(
                "milo convert: generated face config type malformed");
        for (size_t existing = 1;
             existing < type_children.size(); ++existing) {
            const auto& node = type_children[existing];
            if (node && gh::dtb::is_array(*node) &&
                !gh::dtb::children(*node).empty() &&
                gh::dtb::as_string(
                    *gh::dtb::children(*node).front())
                        .value_or("") == expected[index])
                throw std::runtime_error(
                    "milo convert: rnd_objects already defines " +
                    std::string(expected[index]));
        }
        type_children.push_back(candidate);
    }

    Gh2FaceConfigPatch result;
    result.bytes = gh::dtb::serialize(target);
    const auto reparsed = gh::dtb::parse(result.bytes);
    if (gh::dtb::serialize(reparsed) != result.bytes)
        throw std::runtime_error(
            "milo convert: patched face config does not round trip");
    result.dta = gh::dtb::to_dta(reparsed, true);
    result.types_added = source.root.size();
    return result;
}

Gh2FaceMidiParserPatch patch_gh2_midi_parsers_for_gh1_singer_face(
    const std::vector<uint8_t>& clean_midi_parsers) {
    gh::dtb::Tree target = gh::dtb::parse(clean_midi_parsers);
    constexpr std::string_view expected =
        "gh1_singer_face_parser";
    for (const auto& root : target.root) {
        if (!root || !gh::dtb::is_array(*root) ||
            gh::dtb::children(*root).empty() ||
            !gh::dtb::children(*root).front())
            continue;
        if (gh::dtb::as_string(
                *gh::dtb::children(*root).front())
                .value_or("") == expected)
            throw std::runtime_error(
                "milo convert: midi_parsers already defines " +
                std::string(expected));
    }
    const auto source =
        gh::dtb::parse_dta(
            gh1_singer_face_midi_parser_dta());
    if (source.root.size() != 1 || !source.root.front() ||
        !gh::dtb::is_array(*source.root.front()) ||
        gh::dtb::children(*source.root.front()).empty() ||
        gh::dtb::as_string(
            *gh::dtb::children(*source.root.front()).front())
            .value_or("") != expected)
        throw std::runtime_error(
            "milo convert: generated singer face parser malformed");
    target.root.push_back(source.root.front());

    Gh2FaceMidiParserPatch result;
    result.bytes = gh::dtb::serialize(target);
    const auto reparsed = gh::dtb::parse(result.bytes);
    if (gh::dtb::serialize(reparsed) != result.bytes)
        throw std::runtime_error(
            "milo convert: patched MIDI parser config does not "
            "round trip");
    result.dta = gh::dtb::to_dta(reparsed, true);
    result.parsers_added = 1;
    return result;
}

Gh2CharacterFaceConfigPatch
patch_gh2_char_objects_for_gh1_singer_face(
    const std::vector<uint8_t>& clean_char_objects) {
    gh::dtb::Tree target = gh::dtb::parse(clean_char_objects);
    std::shared_ptr<gh::dtb::Node> character_root;
    for (const auto& root : target.root) {
        if (!root || !gh::dtb::is_array(*root) ||
            gh::dtb::children(*root).empty() ||
            !gh::dtb::children(*root).front())
            continue;
        if (gh::dtb::as_string(
                *gh::dtb::children(*root).front())
                .value_or("") != "Character")
            continue;
        if (character_root)
            throw std::runtime_error(
                "milo convert: duplicate Character roots");
        character_root = root;
    }
    if (!character_root)
        throw std::runtime_error(
            "milo convert: char_objects has no Character root");
    const auto types =
        gh::dtb::find_keyed(*character_root, "types");
    if (!types)
        throw std::runtime_error(
            "milo convert: Character has no types row");
    std::shared_ptr<gh::dtb::Node> singer;
    std::function<void(const std::shared_ptr<gh::dtb::Node>&)>
        find_singer =
            [&](const std::shared_ptr<gh::dtb::Node>& candidate) {
                if (!candidate || singer ||
                    !gh::dtb::is_array(*candidate))
                    return;
                const auto& children =
                    gh::dtb::children(*candidate);
                if (!children.empty() && children.front() &&
                    gh::dtb::as_string(*children.front())
                            .value_or("") == "singer") {
                    const auto parser =
                        gh::dtb::find_keyed(*candidate, "parser");
                    if (parser &&
                        gh::dtb::children(*parser).size() >= 2 &&
                        gh::dtb::as_string(
                            *gh::dtb::children(*parser)[1])
                                .value_or("") ==
                            "singer_parser") {
                        singer = candidate;
                        return;
                    }
                }
                for (const auto& child : children)
                    find_singer(child);
            };
    find_singer(types);
    if (!singer) {
        std::string observed;
        for (const auto& candidate : gh::dtb::children(*types)) {
            if (!candidate || !gh::dtb::is_array(*candidate) ||
                gh::dtb::children(*candidate).empty() ||
                !gh::dtb::children(*candidate).front())
                continue;
            const auto name = gh::dtb::as_string(
                *gh::dtb::children(*candidate).front());
            if (!name) continue;
            if (!observed.empty()) observed += ',';
            observed += *name;
        }
        throw std::runtime_error(
            "milo convert: Character has no singer type; "
            "observed=" + observed);
    }

    const auto handlers = gh::dtb::parse_dta(
        "(singer_face_open {lip.servo singer_face_open})\n"
        "(singer_face_close {lip.servo singer_face_close})\n");
    if (handlers.root.size() != 2)
        throw std::runtime_error(
            "milo convert: generated singer handlers malformed");
    auto& singer_children =
        std::get<gh::dtb::NodeList>(singer->value);
    for (const auto& handler : handlers.root) {
        const auto& children = gh::dtb::children(*handler);
        const std::string name =
            gh::dtb::as_string(*children.front()).value_or("");
        if (name.empty() || gh::dtb::find_keyed(*singer, name))
            throw std::runtime_error(
                "milo convert: Character singer handler "
                "already exists or is malformed: " + name);
        singer_children.push_back(handler);
    }

    Gh2CharacterFaceConfigPatch result;
    result.bytes = gh::dtb::serialize(target);
    const auto reparsed = gh::dtb::parse(result.bytes);
    if (gh::dtb::serialize(reparsed) != result.bytes)
        throw std::runtime_error(
            "milo convert: patched character face config does not "
            "round trip");
    result.dta = gh::dtb::to_dta(reparsed, true);
    result.handlers_added = handlers.root.size();
    return result;
}

}  // namespace gh::milo_convert
