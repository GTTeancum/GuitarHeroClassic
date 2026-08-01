#include "gh1_venue_script_conversion.h"

#include "dtb.h"
#include "dtb_preprocess.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gh::milo_convert {
namespace {

using Node = gh::dtb::Node;
using NodeList = gh::dtb::NodeList;
using NodePtr = std::shared_ptr<Node>;

NodePtr atom(uint32_t tag, gh::dtb::Atom value) {
    auto out = std::make_shared<Node>();
    out->tag = tag;
    out->value = std::move(value);
    return out;
}

NodePtr symbol(std::string value) {
    return atom(0x05, std::move(value));
}

NodePtr integer(int32_t value) {
    return atom(0x00, value);
}

NodePtr number(double value) {
    const double rounded = std::round(value);
    if (std::fabs(value - rounded) <= 1.0e-6 &&
        rounded >= static_cast<double>(INT32_MIN) &&
        rounded <= static_cast<double>(INT32_MAX))
        return integer(static_cast<int32_t>(rounded));
    return atom(0x01, static_cast<float>(value));
}

NodePtr collection(uint32_t tag, NodeList children) {
    auto out = atom(tag, std::move(children));
    return out;
}

NodePtr row(std::string key, NodeList values) {
    NodeList children;
    children.reserve(values.size() + 1);
    children.push_back(symbol(std::move(key)));
    children.insert(
        children.end(),
        std::make_move_iterator(values.begin()),
        std::make_move_iterator(values.end()));
    return collection(0x10, std::move(children));
}

NodePtr script(NodeList children) {
    return collection(0x11, std::move(children));
}

std::string text(const NodePtr& node) {
    return node ? gh::dtb::as_string(*node).value_or("") : std::string{};
}

std::optional<double> numeric(const NodePtr& node) {
    if (!node) return std::nullopt;
    if (const auto value = gh::dtb::as_float(*node))
        return static_cast<double>(*value);
    if (const auto value = gh::dtb::as_int(*node))
        return static_cast<double>(*value);
    return std::nullopt;
}

NodePtr clone_node(const NodePtr& source) {
    if (!source) return {};
    auto out = std::make_shared<Node>();
    out->tag = source->tag;
    out->line = source->line;
    if (!gh::dtb::is_array(*source)) {
        out->value = source->value;
        return out;
    }
    NodeList children;
    for (const auto& child : gh::dtb::children(*source))
        children.push_back(clone_node(child));
    out->value = std::move(children);
    return out;
}

struct Function {
    std::vector<std::string> parameters;
    NodeList body;
};

using Environment = std::map<std::string, NodePtr>;
using FunctionMap = std::map<std::string, Function>;

class Expander {
public:
    Expander(
        const FunctionMap& functions,
        Gh2VenueScriptConversion& metrics)
        : functions_(functions), metrics_(metrics) {}

    NodeList expand_sequence(
        const NodeList& source, const Environment& environment = {}) {
        NodeList out;
        std::vector<std::string> stack;
        for (const auto& node : source) {
            auto expanded = expand(node, environment, stack);
            out.insert(
                out.end(),
                std::make_move_iterator(expanded.begin()),
                std::make_move_iterator(expanded.end()));
        }
        return out;
    }

private:
    NodeList expand(
        const NodePtr& source, const Environment& environment,
        std::vector<std::string>& stack) {
        if (!source) return {};
        if (source->tag == 0x02) {
            const auto found = environment.find(text(source));
            if (found != environment.end())
                return {clone_node(found->second)};
        }
        if (!gh::dtb::is_array(*source))
            return {clone_node(source)};

        const auto& children = gh::dtb::children(*source);
        if (source->tag == 0x11 && !children.empty()) {
            const std::string head = text(children.front());
            const auto function = functions_.find(head);
            if (function != functions_.end()) {
                if (std::find(stack.begin(), stack.end(), head) != stack.end())
                    throw std::runtime_error(
                        "milo convert: recursive GH1 venue function " + head);
                if (children.size() - 1 !=
                    function->second.parameters.size())
                    throw std::runtime_error(
                        "milo convert: GH1 venue function argument count "
                        "differs for " + head);
                Environment bound = environment;
                for (size_t index = 0;
                     index < function->second.parameters.size(); ++index) {
                    auto argument =
                        expand(children[index + 1], environment, stack);
                    if (argument.size() != 1)
                        throw std::runtime_error(
                            "milo convert: GH1 venue function argument "
                            "expanded to a sequence");
                    bound[function->second.parameters[index]] =
                        argument.front();
                }
                ++metrics_.function_calls_inlined;
                stack.push_back(head);
                NodeList out;
                for (const auto& body : function->second.body) {
                    auto expanded = expand(body, bound, stack);
                    out.insert(
                        out.end(),
                        std::make_move_iterator(expanded.begin()),
                        std::make_move_iterator(expanded.end()));
                }
                stack.pop_back();
                return out;
            }
            if (head == "foreach" && children.size() >= 4 &&
                children[1] && children[1]->tag == 0x02) {
                auto values = expand(children[2], environment, stack);
                if (values.size() != 1 || !values.front() ||
                    !gh::dtb::is_array(*values.front()))
                    throw std::runtime_error(
                        "milo convert: GH1 venue foreach list is not finite");
                const std::string variable = text(children[1]);
                const auto& value_children =
                    gh::dtb::children(*values.front());
                // GH1 commonly passes a finite `switch $slot (...)`
                // expression as a function's foreach collection. Preserve
                // the dynamic selector and distribute the foreach body into
                // each finite branch. Iterating the switch AST itself would
                // incorrectly treat "switch", "$slot", and branch records as
                // animation targets.
                if (values.front()->tag == 0x11 &&
                    value_children.size() >= 3 &&
                    text(value_children.front()) == "switch") {
                    NodeList switch_children = {
                        clone_node(value_children.front())};
                    auto selector =
                        expand(value_children[1], environment, stack);
                    if (selector.size() != 1)
                        throw std::runtime_error(
                            "milo convert: GH1 venue switch selector "
                            "expanded to a sequence");
                    switch_children.push_back(selector.front());
                    for (size_t branch_index = 2;
                         branch_index < value_children.size();
                         ++branch_index) {
                        const auto& branch_node =
                            value_children[branch_index];
                        if (!branch_node ||
                            !gh::dtb::is_array(*branch_node)) {
                            throw std::runtime_error(
                                "milo convert: GH1 venue foreach switch "
                                "branch is not finite");
                        }
                        const auto& branch =
                            gh::dtb::children(*branch_node);
                        if (branch.size() < 2 || !branch[0] || !branch[1]) {
                            throw std::runtime_error(
                                "milo convert: GH1 venue foreach switch "
                                "branch is incomplete");
                        }
                        std::vector<NodePtr> branch_values;
                        if (gh::dtb::is_array(*branch[1])) {
                            for (const auto& value :
                                 gh::dtb::children(*branch[1]))
                                branch_values.push_back(value);
                        } else {
                            branch_values.push_back(branch[1]);
                        }
                        NodeList branch_out = {clone_node(branch[0])};
                        for (const auto& value : branch_values) {
                            Environment bound = environment;
                            bound[variable] = clone_node(value);
                            for (size_t index = 3;
                                 index < children.size(); ++index) {
                                auto expanded =
                                    expand(children[index], bound, stack);
                                branch_out.insert(
                                    branch_out.end(),
                                    std::make_move_iterator(
                                        expanded.begin()),
                                    std::make_move_iterator(
                                        expanded.end()));
                            }
                        }
                        switch_children.push_back(
                            collection(branch_node->tag,
                                       std::move(branch_out)));
                    }
                    ++metrics_.foreach_loops_unrolled;
                    return {
                        collection(values.front()->tag,
                                   std::move(switch_children))};
                }
                NodeList out;
                for (const auto& value :
                     value_children) {
                    Environment bound = environment;
                    bound[variable] = clone_node(value);
                    for (size_t index = 3;
                         index < children.size(); ++index) {
                        auto expanded =
                            expand(children[index], bound, stack);
                        out.insert(
                            out.end(),
                            std::make_move_iterator(expanded.begin()),
                            std::make_move_iterator(expanded.end()));
                    }
                }
                ++metrics_.foreach_loops_unrolled;
                return out;
            }
        }

        NodeList out_children;
        for (const auto& child : children) {
            auto expanded = expand(child, environment, stack);
            out_children.insert(
                out_children.end(),
                std::make_move_iterator(expanded.begin()),
                std::make_move_iterator(expanded.end()));
        }
        auto out = collection(source->tag, std::move(out_children));
        out->line = source->line;
        return {std::move(out)};
    }

    const FunctionMap& functions_;
    Gh2VenueScriptConversion& metrics_;
};

struct SwitchOptions {
    bool has_range = false;
    bool loop = false;
    NodePtr start;
    NodePtr end;
    double scale = 1.0;
    double blend = 0.0;
};

struct AuthoredRange {
    bool loop = false;
    double start = 0.0;
    double end = 0.0;
};

using AuthoredRanges =
    std::map<std::string, std::vector<AuthoredRange>>;

void parse_switch_option(
    const NodePtr& option, SwitchOptions& out) {
    if (!option || !gh::dtb::is_array(*option)) return;
    const auto& children = gh::dtb::children(*option);
    if (children.empty()) return;
    const std::string key = text(children.front());
    if ((key == "range" || key == "loop") &&
        children.size() >= 3) {
        out.has_range = true;
        out.loop = key == "loop";
        out.start = clone_node(children[1]);
        out.end = clone_node(children[2]);
        return;
    }
    if (key == "scale" && children.size() >= 2) {
        const auto value = numeric(children[1]);
        if (!value)
            throw std::runtime_error(
                "milo convert: nonnumeric GH1 venue animation scale");
        out.scale = *value;
        return;
    }
    if (key == "blend" && children.size() >= 2) {
        const auto value = numeric(children[1]);
        if (!value)
            throw std::runtime_error(
                "milo convert: nonnumeric GH1 venue animation blend");
        out.blend = *value;
        return;
    }
    for (const auto& child : children)
        parse_switch_option(child, out);
}

void collect_authored_ranges(
    const NodePtr& source, AuthoredRanges& ranges) {
    if (!source || !gh::dtb::is_array(*source)) return;
    const auto& children = gh::dtb::children(*source);
    if (source->tag == 0x11 && children.size() >= 3 &&
        text(children[0]) == "arena" &&
        (text(children[1]) == "switch_anim" ||
         text(children[1]) == "switch_anim_rt")) {
        SwitchOptions options;
        for (size_t index = 3; index < children.size(); ++index)
            parse_switch_option(children[index], options);
        const auto start = numeric(options.start);
        const auto end = numeric(options.end);
        const std::string target = text(children[2]);
        if (options.has_range && start && end && !target.empty()) {
            const AuthoredRange fact{
                options.loop, *start, *end};
            auto& target_ranges = ranges[target];
            const bool duplicate =
                std::any_of(
                    target_ranges.begin(), target_ranges.end(),
                    [&](const AuthoredRange& existing) {
                        return existing.loop == fact.loop &&
                               std::fabs(existing.start - fact.start) <=
                                   1.0e-6 &&
                               std::fabs(existing.end - fact.end) <=
                                   1.0e-6;
                    });
            if (!duplicate) target_ranges.push_back(fact);
        }
    }
    for (const auto& child : children)
        collect_authored_ranges(child, ranges);
}

std::optional<std::pair<int32_t, int32_t>>
random_int_bounds(const NodePtr& node) {
    if (!node || node->tag != 0x11 ||
        !gh::dtb::is_array(*node))
        return std::nullopt;
    const auto& children = gh::dtb::children(*node);
    if (children.size() != 3 ||
        text(children.front()) != "random_int")
        return std::nullopt;
    const auto minimum =
        children[1] ? gh::dtb::as_int(*children[1]) : std::nullopt;
    const auto maximum =
        children[2] ? gh::dtb::as_int(*children[2]) : std::nullopt;
    if (!minimum || !maximum) return std::nullopt;
    return std::pair<int32_t, int32_t>{
        std::min(*minimum, *maximum),
        std::max(*minimum, *maximum)};
}

NodePtr native_animate_call(
    const NodePtr& target, bool loop, const NodePtr& start,
    const NodePtr& end, double scale, double blend,
    bool realtime) {
    const auto start_value = numeric(start);
    const auto end_value = numeric(end);
    if (!start_value || !end_value)
        throw std::runtime_error(
            "milo convert: nonnumeric GH1 venue animation range");
    if (!std::isfinite(scale) || std::fabs(scale) <= 1.0e-8)
        throw std::runtime_error(
            "milo convert: invalid GH1 venue animation scale");
    if (!std::isfinite(blend) || blend < 0.0)
        throw std::runtime_error(
            "milo convert: invalid GH1 venue animation blend");

    NodeList call = {
        clone_node(target),
        symbol("animate"),
        row(
            loop ? "loop" : "range",
            {clone_node(start), clone_node(end)})};
    const double span = std::fabs(*end_value - *start_value);
    if (span > 1.0e-6) {
        const double source_frames_per_unit =
            realtime ? 1000.0 : 480.0;
        call.push_back(
            row(
                "period",
                {number(
                    span /
                    (source_frames_per_unit * std::fabs(scale)))}));
    }
    call.push_back(
        row(
            "units",
            {symbol(realtime ? "kTaskSeconds"
                             : "kTaskBeats")}));
    if (blend > 0.0) {
        call.push_back(
            row(
                "blend",
                {number(
                    blend / (realtime ? 1000.0 : 480.0))}));
    }
    return script(std::move(call));
}

class Lowerer {
public:
    Lowerer(
        Gh2VenueScriptConversion& metrics,
        const AuthoredRanges& authored_ranges,
        const std::set<std::string>& state_variables)
        : metrics_(metrics), authored_ranges_(authored_ranges),
          state_variables_(state_variables) {}

    NodeList lower_sequence(const NodeList& source) {
        NodeList out;
        for (const auto& node : source) {
            auto lowered = lower(node);
            out.insert(
                out.end(),
                std::make_move_iterator(lowered.begin()),
                std::make_move_iterator(lowered.end()));
        }
        return out;
    }

private:
    NodeList lower(const NodePtr& source) {
        if (source && source->tag == 0x02) {
            const std::string name = text(source);
            if (name.empty())
                throw std::runtime_error(
                    "milo convert: empty GH1 venue DataVariable");
            if (state_variables_.find(name) ==
                state_variables_.end())
                return {clone_node(source)};
            return {collection(0x13, {symbol(name)})};
        }
        if (!source || !gh::dtb::is_array(*source))
            return {clone_node(source)};
        const auto& children = gh::dtb::children(*source);
        if (source->tag == 0x11 && children.size() >= 3 &&
            text(children[0]) == "arena" &&
            (text(children[1]) == "switch_anim" ||
             text(children[1]) == "switch_anim_rt")) {
            const bool realtime =
                text(children[1]) == "switch_anim_rt";
            SwitchOptions options;
            for (size_t index = 3; index < children.size(); ++index)
                parse_switch_option(children[index], options);
            if (!options.has_range) {
                const std::string target = text(children[2]);
                const auto found = authored_ranges_.find(target);
                if (found == authored_ranges_.end() ||
                    found->second.size() != 1)
                    throw std::runtime_error(
                        "milo convert: GH1 stateful animation range is "
                        "not uniquely recoverable for " + target);
                options.has_range = true;
                options.loop = found->second.front().loop;
                options.start = number(found->second.front().start);
                options.end = number(found->second.front().end);
                ++metrics_.stateful_ranges_resolved;
            }
            if (realtime)
                ++metrics_.switch_anim_rt_calls;
            else
                ++metrics_.switch_anim_calls;

            const auto random = random_int_bounds(options.start);
            if (!random)
                return {native_animate_call(
                    children[2], options.loop, options.start,
                    options.end, options.scale, options.blend,
                    realtime)};

            NodeList switch_call = {
                symbol("switch"),
                script(
                    {symbol("random_int"),
                     integer(random->first),
                     integer(random->second)})};
            for (int64_t value = random->first;
                 value <= random->second; ++value) {
                switch_call.push_back(
                    row(
                        std::to_string(value),
                        {native_animate_call(
                            children[2], options.loop,
                            integer(static_cast<int32_t>(value)),
                            options.end, options.scale,
                            options.blend, realtime)}));
            }
            // Switch branch keys are integers rather than symbols.
            for (size_t index = 2;
                 index < switch_call.size(); ++index) {
                auto& branch =
                    std::get<NodeList>(switch_call[index]->value);
                branch[0] =
                    integer(static_cast<int32_t>(
                        random->first + index - 2));
            }
            ++metrics_.random_ranges_expanded;
            return {script(std::move(switch_call))};
        }

        if (source->tag == 0x11 && children.size() >= 6 &&
            text(children[0]) == "game" &&
            text(children[1]) == "anim_task") {
            const auto period_ms = numeric(children[3]);
            if (!period_ms || *period_ms <= 0.0)
                throw std::runtime_error(
                    "milo convert: invalid GH1 game anim_task period");
            ++metrics_.anim_task_calls;
            return {
                script(
                    {clone_node(children[2]),
                     symbol("animate"),
                     row(
                         "range",
                         {clone_node(children[4]),
                          clone_node(children[5])}),
                     row(
                         "period",
                         {number(*period_ms / 1000.0)}),
                     row(
                         "units",
                         {symbol("kTaskSeconds")})})};
        }

        if (source->tag == 0x11 && children.size() >= 5 &&
            text(children[0]) == "animate_to" &&
            text(children[1]) == "arena") {
            const auto period_ms = numeric(children[4]);
            if (!period_ms || *period_ms <= 0.0)
                throw std::runtime_error(
                    "milo convert: invalid GH1 animate_to period");
            ++metrics_.animate_to_calls;
            return {
                script(
                    {clone_node(children[2]),
                     symbol("animate"),
                     row("dest", {clone_node(children[3])}),
                     row(
                         "period",
                         {number(*period_ms / 1000.0)}),
                     row(
                         "units",
                         {symbol("kTaskSeconds")})})};
        }

        if (source->tag == 0x11 && children.size() >= 4 &&
            text(children[0]) == "arena" &&
            text(children[1]) == "delay_task") {
            const auto delay_frames = numeric(children[2]);
            if (!delay_frames || *delay_frames < 0.0)
                throw std::runtime_error(
                    "milo convert: invalid GH1 Arena delay_task delay");
            NodeList body;
            for (size_t index = 3; index < children.size(); ++index) {
                auto lowered = lower(children[index]);
                body.insert(
                    body.end(),
                    std::make_move_iterator(lowered.begin()),
                    std::make_move_iterator(lowered.end()));
            }
            NodeList script_row = {symbol("script")};
            script_row.insert(
                script_row.end(),
                std::make_move_iterator(body.begin()),
                std::make_move_iterator(body.end()));
            ++metrics_.delay_task_calls;
            return {
                script(
                    {symbol("script_task"),
                     row("units", {symbol("kTaskBeats")}),
                     row(
                         "delay",
                         {number(*delay_frames / 480.0)}),
                     collection(0x10, std::move(script_row))})};
        }

        NodeList lowered_children;
        for (const auto& child : children) {
            auto lowered = lower(child);
            lowered_children.insert(
                lowered_children.end(),
                std::make_move_iterator(lowered.begin()),
                std::make_move_iterator(lowered.end()));
        }
        auto out =
            collection(source->tag, std::move(lowered_children));
        out->line = source->line;
        return {std::move(out)};
    }

    Gh2VenueScriptConversion& metrics_;
    const AuthoredRanges& authored_ranges_;
    const std::set<std::string>& state_variables_;
};

bool is_parameter_list(const NodePtr& node) {
    if (!node || node->tag != 0x10 ||
        !gh::dtb::is_array(*node))
        return false;
    for (const auto& child : gh::dtb::children(*node)) {
        if (!child || child->tag != 0x02) return false;
    }
    return true;
}

}  // namespace

Gh2VenueScriptConversion convert_gh1_venue_script_to_gh2_worlddir(
    const std::vector<uint8_t>& source_dtb,
    const std::string& venue) {
    if (venue.empty())
        throw std::runtime_error(
            "milo convert: empty GH1 venue script name");
    const gh::dtb::Tree source = gh::dtb::parse(source_dtb);
    gh::dtb::PreprocessOptions options;
    options.defines = {"HX_EE", "PS2"};
    const NodeList roots =
        gh::dtb::preprocess(source.root, options);

    FunctionMap functions;
    NodeList handler_rows;
    std::vector<std::pair<std::string, std::string>> loaded_sections;
    std::vector<std::pair<std::string, NodePtr>> initial_states;
    size_t source_roots = 0;
    size_t recognized_roots = 0;
    size_t unrecognized_roots = 0;
    std::vector<std::string> unrecognized_root_forms;
    for (const auto& root : roots) {
        ++source_roots;
        if (!root || root->tag != 0x11 ||
            !gh::dtb::is_array(*root)) {
            ++unrecognized_roots;
            unrecognized_root_forms.push_back(
                !root ? "<null>" :
                "<tag:" + std::to_string(root->tag) + ">");
            continue;
        }
        const auto& children = gh::dtb::children(*root);
        if (children.size() >= 2 &&
            text(children[0]) == "func") {
            const std::string name = text(children[1]);
            if (name.empty() || functions.find(name) != functions.end())
                throw std::runtime_error(
                    "milo convert: invalid or duplicate GH1 venue "
                    "function");
            Function function;
            size_t body_start = 2;
            if (children.size() > 2 &&
                is_parameter_list(children[2])) {
                for (const auto& parameter :
                     gh::dtb::children(*children[2]))
                    function.parameters.push_back(text(parameter));
                body_start = 3;
            }
            for (size_t index = body_start;
                 index < children.size(); ++index)
                function.body.push_back(children[index]);
            functions.emplace(name, std::move(function));
            ++recognized_roots;
            continue;
        }
        if (children.size() >= 3 &&
            text(children[0]) == "arena" &&
            text(children[1]) == "add_handlers") {
            for (size_t index = 2;
                 index < children.size(); ++index) {
                if (!children[index] ||
                    children[index]->tag != 0x10)
                    throw std::runtime_error(
                        "milo convert: GH1 venue add_handlers contains "
                        "a non-handler entry");
                handler_rows.push_back(children[index]);
            }
            ++recognized_roots;
            continue;
        }
        if (children.size() == 4 &&
            text(children[0]) == "arena" &&
            text(children[1]) == "load_section") {
            const std::string section = text(children[2]);
            const std::string directory = text(children[3]);
            if (section.empty() || directory.empty())
                throw std::runtime_error(
                    "milo convert: GH1 venue load_section has an empty "
                    "section or directory");
            loaded_sections.emplace_back(section, directory);
            ++recognized_roots;
            continue;
        }
        if (children.size() == 3 &&
            text(children[0]) == "set" &&
            children[1] && children[1]->tag == 0x02) {
            const std::string variable = text(children[1]);
            if (variable.empty())
                throw std::runtime_error(
                    "milo convert: GH1 venue top-level state initializer "
                    "has an empty variable");
            if (std::any_of(
                    initial_states.begin(), initial_states.end(),
                    [&](const auto& state) {
                        return state.first == variable;
                    }))
                throw std::runtime_error(
                    "milo convert: duplicate GH1 venue top-level state "
                    "initializer " + variable);
            initial_states.emplace_back(
                variable, clone_node(children[2]));
            ++recognized_roots;
            continue;
        }
        ++unrecognized_roots;
        std::string form = "<array>";
        if (!children.empty() && !text(children[0]).empty())
            form = text(children[0]);
        if (children.size() > 1 && !text(children[1]).empty())
            form += " " + text(children[1]);
        unrecognized_root_forms.push_back(std::move(form));
    }
    if (unrecognized_roots != 0) {
        std::string forms;
        for (const auto& form : unrecognized_root_forms) {
            if (!forms.empty()) forms += ", ";
            forms += form;
        }
        throw std::runtime_error(
            "milo convert: GH1 venue script contains " +
            std::to_string(unrecognized_roots) +
            " unrecognized top-level root(s): " + forms);
    }
    if (handler_rows.empty())
        throw std::runtime_error(
            "milo convert: GH1 venue script has no Arena handlers");

    Gh2VenueScriptConversion result;
    result.source_roots = source_roots;
    result.recognized_roots = recognized_roots;
    result.unrecognized_roots = unrecognized_roots;
    result.loaded_sections = loaded_sections;
    for (const auto& [name, value] : initial_states) {
        (void)value;
        result.initialized_states.push_back(name);
    }
    result.source_functions = functions.size();
    Expander expander(functions, result);
    std::map<std::string, size_t> handler_indices;
    NodeList unique_handler_rows;
    for (const auto& source_handler : handler_rows) {
        const auto& children =
            gh::dtb::children(*source_handler);
        if (children.empty()) continue;
        const std::string name = text(children.front());
        if (name.empty())
            throw std::runtime_error(
                "milo convert: empty GH1 venue handler name");
        const auto found = handler_indices.find(name);
        if (found == handler_indices.end()) {
            handler_indices.emplace(name, unique_handler_rows.size());
            unique_handler_rows.push_back(source_handler);
        } else {
            unique_handler_rows[found->second] = source_handler;
        }
    }

    struct ExpandedHandler {
        std::string name;
        NodeList body;
    };
    std::vector<ExpandedHandler> expanded_handlers;
    AuthoredRanges authored_ranges;
    for (const auto& source_handler : unique_handler_rows) {
        const auto& children =
            gh::dtb::children(*source_handler);
        NodeList body(children.begin() + 1, children.end());
        body = expander.expand_sequence(body);
        for (const auto& node : body)
            collect_authored_ranges(node, authored_ranges);
        expanded_handlers.push_back(
            {text(children.front()), std::move(body)});
    }

    std::set<std::string> state_variables;
    for (const auto& [name, value] : initial_states) {
        (void)value;
        state_variables.insert(name);
    }
    Lowerer lowerer(result, authored_ranges, state_variables);
    NodeList target_type = {
        symbol(venue), symbol("WORLD_OBJECT_BASE")};
    for (const auto& [name, value] : initial_states)
        target_type.push_back(
            row(name, {clone_node(value)}));
    for (auto& handler : expanded_handlers) {
        handler.body = lowerer.lower_sequence(handler.body);
        result.handler_names.push_back(handler.name);
        NodeList target_handler = {symbol(handler.name)};
        target_handler.insert(
            target_handler.end(),
            std::make_move_iterator(handler.body.begin()),
            std::make_move_iterator(handler.body.end()));
        target_type.push_back(
            collection(0x10, std::move(target_handler)));
    }
    result.handlers = expanded_handlers.size();

    gh::dtb::Tree target;
    target.version = source.version;
    target.root_line = source.root_line;
    target.storage = source.storage;
    target.cipher_seed = source.cipher_seed;
    target.trailing_bytes = source.trailing_bytes;
    target.root = {
        collection(
            0x10,
            {symbol("WorldDir"),
             collection(
                 0x10,
                 {symbol("types"),
                  collection(0x10, std::move(target_type))})})};
    result.bytes = gh::dtb::serialize(target);
    const auto reparsed = gh::dtb::parse(result.bytes);
    if (gh::dtb::serialize(reparsed) != result.bytes)
        throw std::runtime_error(
            "milo convert: target GH2 venue script does not round trip");
    result.dta = gh::dtb::to_dta(reparsed, true);
    return result;
}

}  // namespace gh::milo_convert
