// GH1 animation clip-set manifest (.acs) reader/writer.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gh::acs {

enum class LineKind {
    Blank,
    Comment,
    Include,
    Invocation,
};

struct Line {
    LineKind kind = LineKind::Blank;
    // Blank/comment lines retain their entire body here. Include/invocation
    // lines retain formatting around the editable semantic value.
    std::string raw_body;
    std::string prefix;
    std::string value;
    std::string suffix;
    std::string ending;
};

struct File {
    std::vector<Line> lines;
};

File parse(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize(const File& file);

// Harmonix compiled-data lookup used by an ACS include. For example,
// charsys/anims.acs + anims_macros.dta resolves to
// charsys/gen/anims_macros.dtb.
std::string compiled_include_path(const std::string& acs_path,
                                  const std::string& include_path);

std::vector<uint8_t> read_file(const std::string& path);

}  // namespace gh::acs
