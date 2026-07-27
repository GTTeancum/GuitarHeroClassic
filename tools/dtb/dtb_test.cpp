#include "dtb.h"
#include "dtb_preprocess.h"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

std::shared_ptr<gh::dtb::Node> leaf(uint32_t tag, gh::dtb::Atom value) {
    auto node = std::make_shared<gh::dtb::Node>();
    node->tag = tag;
    node->value = std::move(value);
    return node;
}

std::shared_ptr<gh::dtb::Node> array(
    uint32_t tag, uint32_t line,
    std::vector<std::shared_ptr<gh::dtb::Node>> children) {
    auto node = leaf(tag, std::move(children));
    node->line = line;
    return node;
}

bool check_storage(gh::dtb::Tree tree, gh::dtb::Storage storage,
                   uint32_t seed) {
    tree.storage = storage;
    tree.cipher_seed = seed;
    const auto bytes = gh::dtb::serialize(tree);
    const auto parsed = gh::dtb::parse(bytes);
    const auto round_trip = gh::dtb::serialize(parsed);
    if (round_trip != bytes || parsed.storage != storage ||
        parsed.version != tree.version ||
        parsed.trailing_bytes != tree.trailing_bytes) {
        std::fprintf(stderr, "dtb_test: storage round trip failed\n");
        return false;
    }
    return true;
}

}  // namespace

int main() {
    gh::dtb::Tree tree;
    tree.version = 1;
    tree.embedded = false;
    tree.trailing_bytes = {0xaa, 0x55};
    tree.root = {
        array(0x10, 37,
              {leaf(0x05, std::string("format_test")),
               leaf(0x00, int32_t{-17}),
               leaf(0x01, 3.25f),
               leaf(0x12, std::string("quoted")),
               array(0x11, 41,
                     {leaf(0x02, std::string("this")),
                      leaf(0x05, std::string("message"))})})};

    bool ok = true;
    ok &= check_storage(tree, gh::dtb::Storage::Plain, 0);
    ok &= check_storage(tree, gh::dtb::Storage::ZeroPrefixedPlain, 0);
    ok &= check_storage(tree, gh::dtb::Storage::Encrypted, 0x12345678u);
    ok &= gh::dtb::resolve_compiled_include_path(
              "charsys/gen/anims_macros.dtb",
              "../config/macros.dta") ==
          "config/gen/macros.dtb";
    ok &= gh::dtb::resolve_compiled_include_path(
              "charsys/gen/anims_macros.dtb",
              "hero_graphs.dta") ==
          "charsys/gen/hero_graphs.dtb";
    {
        const auto compiled = gh::dtb::parse_dta(
            "; source comment\n"
            "(face_type\n"
            "  (current_pose -1)\n"
            "  (blend 0.25)\n"
            "  (label \"quoted\\ntext\")\n"
            "  (transition ($pose) {$this trigger [$pose]}))\n");
        const auto bytes = gh::dtb::serialize(compiled);
        const auto parsed = gh::dtb::parse(bytes);
        const auto rendered = gh::dtb::to_dta(parsed);
        ok &= parsed.root.size() == 1 &&
              rendered.find("(current_pose -1)") !=
                  std::string::npos &&
              rendered.find("(blend 0.25)") !=
                  std::string::npos &&
              rendered.find("$pose") != std::string::npos &&
              rendered.find("[$pose]") != std::string::npos &&
              gh::dtb::serialize(parsed) == bytes;
    }
    {
        gh::dtb::Tree nested;
        nested.root = {
            leaf(0x20, std::string("NESTED")),
            array(0x10, 1, {leaf(0x00, int32_t{7})})};
        gh::dtb::Tree owner;
        owner.root = {
            leaf(0x21, std::string("nested.dta")),
            leaf(0x05, std::string("NESTED"))};
        gh::dtb::PreprocessOptions options;
        options.source_path = "charsys/gen/owner.dtb";
        options.contextual_include_resolver =
            [&](const std::string& including,
                const std::string& include) {
                gh::dtb::PreprocessOptions::IncludedFile result;
                result.path =
                    gh::dtb::resolve_compiled_include_path(
                        including, include);
                result.roots = nested.root;
                return result;
            };
        const auto processed =
            gh::dtb::preprocess(owner.root, options);
        ok &= processed.size() == 1 &&
              gh::dtb::as_int(*processed.front()).value_or(0) == 7;
    }
    {
        gh::dtb::MacroTable macros;
        macros["BASE"] = leaf(0x00, int32_t{11});
        macros["ALIAS"] = leaf(0x05, std::string("BASE"));
        gh::dtb::PreprocessOptions options;
        options.macro_table = &macros;
        const auto processed = gh::dtb::preprocess(
            {leaf(0x05, std::string("ALIAS"))}, options);
        ok &= processed.size() == 1 &&
              gh::dtb::as_int(*processed.front()).value_or(0) == 11;
    }
    if (!ok) return 1;
    std::printf("dtb_test: all checks passed\n");
    return 0;
}
