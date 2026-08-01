#include "acs.h"

#include <cstdio>
#include <string>
#include <vector>

int main() {
    const std::string text =
        "#include anims_macros.dta\r\n"
        "\r\n"
        "; declarations\r\n"
        "METAL_ANIMS\r\n"
        "HAIR_UI_ANIMS ; retained note";
    const std::vector<uint8_t> bytes(text.begin(), text.end());
    try {
        auto file = gh::acs::parse(bytes);
        if (gh::acs::serialize(file) != bytes || file.lines.size() != 5 ||
            file.lines[0].kind != gh::acs::LineKind::Include ||
            file.lines[3].kind != gh::acs::LineKind::Invocation ||
            gh::acs::compiled_include_path(
                "charsys/anims.acs", file.lines[0].value) !=
                "charsys/gen/anims_macros.dtb") {
            std::fprintf(stderr, "acs_test: parse mismatch\n");
            return 1;
        }
        file.lines[0].value = "replacement.dta";
        const auto edited = gh::acs::serialize(file);
        const std::string edited_text(edited.begin(), edited.end());
        if (edited_text.find("#include replacement.dta\r\n") != 0) {
            std::fprintf(stderr, "acs_test: edit mismatch\n");
            return 1;
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "acs_test: %s\n", ex.what());
        return 1;
    }
    std::printf("acs_test: all checks passed\n");
    return 0;
}
