#include "dtb.h"

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
    if (!ok) return 1;
    std::printf("dtb_test: all checks passed\n");
    return 0;
}
