#include "acs.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace gh::acs {
namespace {

bool horizontal_space(char ch) {
    return ch == ' ' || ch == '\t';
}

bool invocation_char(unsigned char ch) {
    return std::isalnum(ch) || ch == '_' || ch == ':' || ch == '-' ||
           ch == '.';
}

Line parse_line(const std::string& body, std::string ending) {
    Line line;
    line.ending = std::move(ending);
    size_t start = 0;
    while (start < body.size() && horizontal_space(body[start])) ++start;
    if (start == body.size()) {
        line.kind = LineKind::Blank;
        line.raw_body = body;
        return line;
    }
    if (body[start] == ';') {
        line.kind = LineKind::Comment;
        line.raw_body = body;
        return line;
    }

    constexpr const char* include = "#include";
    constexpr size_t include_length = 8;
    if (body.compare(start, include_length, include) == 0 &&
        start + include_length < body.size() &&
        horizontal_space(body[start + include_length])) {
        size_t value_start = start + include_length;
        while (value_start < body.size() &&
               horizontal_space(body[value_start]))
            ++value_start;
        if (value_start == body.size())
            throw std::runtime_error("ACS: include has no path");
        size_t value_end = value_start;
        while (value_end < body.size() &&
               !horizontal_space(body[value_end]) &&
               body[value_end] != ';')
            ++value_end;
        if (value_end == value_start)
            throw std::runtime_error("ACS: include has no path");
        size_t suffix_nonspace = value_end;
        while (suffix_nonspace < body.size() &&
               horizontal_space(body[suffix_nonspace]))
            ++suffix_nonspace;
        if (suffix_nonspace < body.size() &&
            body[suffix_nonspace] != ';')
            throw std::runtime_error("ACS: unexpected include suffix");
        line.kind = LineKind::Include;
        line.prefix = body.substr(0, value_start);
        line.value = body.substr(value_start, value_end - value_start);
        line.suffix = body.substr(value_end);
        return line;
    }

    size_t value_end = start;
    while (value_end < body.size() &&
           invocation_char(static_cast<unsigned char>(body[value_end])))
        ++value_end;
    if (value_end == start)
        throw std::runtime_error("ACS: expected macro invocation");
    size_t suffix_nonspace = value_end;
    while (suffix_nonspace < body.size() &&
           horizontal_space(body[suffix_nonspace]))
        ++suffix_nonspace;
    if (suffix_nonspace < body.size() && body[suffix_nonspace] != ';')
        throw std::runtime_error("ACS: invocation has extra tokens");
    line.kind = LineKind::Invocation;
    line.prefix = body.substr(0, start);
    line.value = body.substr(start, value_end - start);
    line.suffix = body.substr(value_end);
    return line;
}

}  // namespace

File parse(const std::vector<uint8_t>& bytes) {
    for (uint8_t byte : bytes) {
        if (byte == 0 || (byte < 0x20 && byte != '\r' && byte != '\n' &&
                          byte != '\t'))
            throw std::runtime_error("ACS: non-text byte");
        if (byte >= 0x80)
            throw std::runtime_error("ACS: unproven non-ASCII byte");
    }

    File file;
    size_t pos = 0;
    while (pos < bytes.size()) {
        const size_t body_start = pos;
        while (pos < bytes.size() && bytes[pos] != '\r' &&
               bytes[pos] != '\n')
            ++pos;
        const std::string body(
            reinterpret_cast<const char*>(bytes.data() + body_start),
            pos - body_start);
        std::string ending;
        if (pos < bytes.size()) {
            if (bytes[pos] == '\r' && pos + 1 < bytes.size() &&
                bytes[pos + 1] == '\n') {
                ending = "\r\n";
                pos += 2;
            } else {
                ending.assign(1, static_cast<char>(bytes[pos++]));
            }
        }
        file.lines.push_back(parse_line(body, std::move(ending)));
    }
    return file;
}

std::vector<uint8_t> serialize(const File& file) {
    std::vector<uint8_t> out;
    for (const auto& line : file.lines) {
        std::string body;
        if (line.kind == LineKind::Blank ||
            line.kind == LineKind::Comment) {
            body = line.raw_body;
        } else {
            if (line.value.empty())
                throw std::runtime_error("ACS: empty semantic line value");
            body = line.prefix + line.value + line.suffix;
        }
        out.insert(out.end(), body.begin(), body.end());
        out.insert(out.end(), line.ending.begin(), line.ending.end());
    }
    return out;
}

std::string compiled_include_path(const std::string& acs_path,
                                  const std::string& include_path) {
    namespace fs = std::filesystem;
    fs::path include(include_path);
    if (include.is_absolute())
        throw std::runtime_error("ACS: absolute include path");
    if (include.extension() != ".dta")
        throw std::runtime_error("ACS: include is not a .dta path");
    include.replace_extension(".dtb");
    fs::path result =
        fs::path(acs_path).parent_path() / "gen" / include;
    result = result.lexically_normal();
    std::string normalized = result.generic_string();
    if (normalized == ".." || normalized.rfind("../", 0) == 0)
        throw std::runtime_error("ACS: include escapes archive root");
    return normalized;
}

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("cannot open " + path);
    const auto size = static_cast<size_t>(file.tellg());
    std::vector<uint8_t> bytes(size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    if (!file) throw std::runtime_error("short read on " + path);
    return bytes;
}

}  // namespace gh::acs
