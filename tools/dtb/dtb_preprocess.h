// Shared Harmonix DTA/DTB preprocessor.
#pragma once

#include "dtb.h"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace gh::dtb {

using MacroTable = std::map<std::string, std::shared_ptr<Node>>;

struct PreprocessOptions {
    std::set<std::string> defines;
    std::function<NodeList(const std::string& file)> include_resolver;
    struct IncludedFile {
        std::string path;
        NodeList roots;
    };
    // Path-aware resolver for nested archive includes. When supplied, this
    // takes precedence over include_resolver and the returned path becomes
    // the base for directives within the included file.
    std::function<IncludedFile(
        const std::string& including_path,
        const std::string& file)> contextual_include_resolver;
    std::string source_path;
    MacroTable* macro_table = nullptr;
};

// Resolves #ifdef/#ifndef/#else/#endif, #define, #include/#merge, and
// recursive bare-symbol macro substitution. The implementation is shared by
// the runtime and offline format/conversion tools so both consume the same
// authored Harmonix data contract.
NodeList preprocess(
    const NodeList& roots, const PreprocessOptions& options);

// Resolve an authored .dta include from a compiled archive .dtb. Harmonix
// inserts "gen" immediately before the compiled filename, so resolution first
// returns to the authored source directory, applies the relative include, and
// then inserts that included file's own "gen" directory.
std::string resolve_compiled_include_path(
    const std::string& compiled_path,
    const std::string& authored_include_path);

}  // namespace gh::dtb
