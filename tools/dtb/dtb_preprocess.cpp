#include "dtb_preprocess.h"

#include <filesystem>
#include <utility>
#include <vector>

namespace gh::dtb {
namespace {

using NodePtr = std::shared_ptr<Node>;

std::string directive_name(const Node& node) {
    const auto value = as_string(node);
    return value ? *value : std::string();
}

struct Context {
    std::set<std::string> defined;
    MacroTable macros;
    std::set<std::string> expanding;
    const PreprocessOptions* options = nullptr;
};

struct Conditional {
    bool branch_active = false;
    bool ever_taken = false;
    bool parent_active = false;
};

NodeList process(
    const NodeList& input, Context& context,
    const std::string& source_path);

NodePtr process_node(
    const NodePtr& source, Context& context,
    const std::string& source_path) {
    if (source && is_array(*source)) {
        auto result = std::make_shared<Node>();
        result->tag = source->tag;
        result->line = source->line;
        result->value =
            process(children(*source), context, source_path);
        return result;
    }
    if (source && source->tag == 0x05) {
        const std::string name = directive_name(*source);
        const auto macro = context.macros.find(name);
        if (macro != context.macros.end()) {
            if (!context.expanding.insert(name).second)
                throw std::runtime_error(
                    "DTB: recursive macro expansion: " + name);
            NodePtr result =
                process_node(
                    macro->second, context, source_path);
            context.expanding.erase(name);
            return result;
        }
    }
    return source;
}

NodePtr macro_body_node(const NodePtr& body) {
    if (!body || body->tag != 0x10) return body;
    const NodeList& body_children = children(*body);
    if (body_children.size() == 1) return body_children.front();
    return body;
}

NodeList process(
    const NodeList& input, Context& context,
    const std::string& source_path) {
    NodeList output;
    std::vector<Conditional> conditionals;
    const auto active = [&]() {
        return conditionals.empty()
                   ? true
                   : conditionals.back().branch_active;
    };
    const auto is_defined = [&](const std::string& name) {
        return context.defined.count(name) ||
               context.macros.count(name) ||
               (context.options &&
                context.options->defines.count(name));
    };

    for (size_t index = 0; index < input.size(); ++index) {
        const Node& node = *input[index];
        switch (node.tag) {
            case 0x07: {
                const bool parent_active = active();
                const bool take =
                    parent_active &&
                    is_defined(directive_name(node));
                conditionals.push_back(
                    {take, take, parent_active});
                continue;
            }
            case 0x23: {
                const bool parent_active = active();
                const bool take =
                    parent_active &&
                    !is_defined(directive_name(node));
                conditionals.push_back(
                    {take, take, parent_active});
                continue;
            }
            case 0x08:
                if (!conditionals.empty()) {
                    auto& conditional = conditionals.back();
                    conditional.branch_active =
                        conditional.parent_active &&
                        !conditional.ever_taken;
                    conditional.ever_taken =
                        conditional.ever_taken ||
                        conditional.branch_active;
                }
                continue;
            case 0x09:
                if (!conditionals.empty()) conditionals.pop_back();
                continue;
            case 0x20:
                if (active() && index + 1 < input.size()) {
                    const std::string name =
                        directive_name(node);
                    context.defined.insert(name);
                    context.macros[name] =
                        macro_body_node(input[index + 1]);
                }
                if (index + 1 < input.size()) ++index;
                continue;
            case 0x21:
            case 0x22:
                if (active() && context.options &&
                    context.options->contextual_include_resolver) {
                    auto included =
                        context.options->contextual_include_resolver(
                            source_path, directive_name(node));
                    const NodeList processed = process(
                        included.roots, context, included.path);
                    output.insert(
                        output.end(), processed.begin(),
                        processed.end());
                } else if (
                    active() && context.options &&
                    context.options->include_resolver) {
                    const NodeList included =
                        context.options->include_resolver(
                            directive_name(node));
                    const NodeList processed =
                        process(included, context, source_path);
                    output.insert(
                        output.end(), processed.begin(),
                        processed.end());
                }
                continue;
            default:
                break;
        }

        if (!active()) continue;
        if (node.tag == 0x05) {
            const std::string name = directive_name(node);
            const auto macro = context.macros.find(name);
            if (macro != context.macros.end()) {
                if (!context.expanding.insert(name).second)
                    throw std::runtime_error(
                        "DTB: recursive macro expansion: " + name);
                output.push_back(
                    process_node(
                        macro->second, context, source_path));
                context.expanding.erase(name);
                continue;
            }
        }
        output.push_back(
            process_node(input[index], context, source_path));
    }
    return output;
}

}  // namespace

NodeList preprocess(
    const NodeList& roots, const PreprocessOptions& options) {
    Context context;
    context.options = &options;
    context.defined = options.defines;
    if (options.macro_table) {
        context.macros = *options.macro_table;
        for (const auto& macro : *options.macro_table)
            context.defined.insert(macro.first);
    }
    NodeList output =
        process(roots, context, options.source_path);
    if (options.macro_table)
        *options.macro_table = std::move(context.macros);
    return output;
}

std::string resolve_compiled_include_path(
    const std::string& compiled_path,
    const std::string& authored_include_path) {
    namespace fs = std::filesystem;
    fs::path compiled(compiled_path);
    fs::path include(authored_include_path);
    if (compiled.is_absolute() || include.is_absolute())
        throw std::runtime_error(
            "DTB: compiled include path must be virtual/relative");
    if (compiled.extension() != ".dtb" ||
        include.extension() != ".dta")
        throw std::runtime_error(
            "DTB: compiled include requires .dtb and .dta paths");
    fs::path source_directory = compiled.parent_path();
    if (source_directory.filename() != "gen")
        throw std::runtime_error(
            "DTB: compiled include owner is not in a gen directory");
    source_directory = source_directory.parent_path();
    fs::path authored =
        (source_directory / include).lexically_normal();
    fs::path result =
        authored.parent_path() / "gen" / authored.filename();
    result.replace_extension(".dtb");
    return result.lexically_normal().generic_string();
}

}  // namespace gh::dtb
