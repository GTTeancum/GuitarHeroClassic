#include "acg.h"
#include "acp.h"
#include "ark_v3.h"
#include "dtb.h"
#include "milo.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Counts {
    size_t seen = 0;
    size_t passed = 0;
    size_t failed = 0;
};

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

std::string extension(const std::string& path) {
    const size_t dot = path.rfind('.');
    return dot == std::string::npos ? std::string() : lower(path.substr(dot));
}

std::string clean_tsv(std::string value) {
    for (char& ch : value) {
        if (ch == '\t' || ch == '\r' || ch == '\n') ch = ' ';
    }
    return value;
}

std::string read_string(const std::vector<uint8_t>& bytes, size_t& pos) {
    if (pos + 4 > bytes.size())
        throw std::runtime_error("truncated string length");
    uint32_t length = 0;
    std::memcpy(&length, bytes.data() + pos, 4);
    pos += 4;
    if (length > bytes.size() - pos)
        throw std::runtime_error("string extends past entry");
    std::string result(reinterpret_cast<const char*>(bytes.data() + pos),
                       length);
    pos += length;
    return result;
}

uint32_t read_u32(const std::vector<uint8_t>& bytes, size_t pos) {
    if (pos + 4 > bytes.size()) throw std::runtime_error("truncated u32");
    uint32_t value = 0;
    std::memcpy(&value, bytes.data() + pos, 4);
    return value;
}

const char* dtb_storage_name(gh::dtb::Storage storage) {
    switch (storage) {
        case gh::dtb::Storage::Plain: return "plain";
        case gh::dtb::Storage::ZeroPrefixedPlain: return "zero_plain";
        case gh::dtb::Storage::Encrypted: return "encrypted";
    }
    return "unknown";
}

void usage() {
    std::fprintf(stderr,
                 "Usage:\n"
                 "  format_audit scan <main.hdr> <main_0.ark> [main_1.ark ...] "
                 "--report <report.tsv>\n");
    std::exit(2);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 6 || std::string(argv[1]) != "scan") usage();
    const std::string hdr_path = argv[2];
    std::vector<std::string> ark_paths;
    std::string report_path;
    for (int i = 3; i < argc; ++i) {
        if (std::string(argv[i]) == "--report") {
            if (++i >= argc) usage();
            report_path = argv[i];
        } else {
            ark_paths.push_back(argv[i]);
        }
    }
    if (ark_paths.empty() || report_path.empty()) usage();

    try {
        const auto archive = gh::ark::ArkV3Reader::load(hdr_path);
        fs::create_directories(fs::path(report_path).parent_path());
        std::ofstream report(report_path, std::ios::binary);
        if (!report) throw std::runtime_error("cannot create " + report_path);
        report << "kind\tpath\tsize\tstatus\tdetail\n";

        std::map<std::string, Counts> counts;
        std::map<std::string, size_t> object_types;
        std::map<std::tuple<std::string, uint16_t, uint16_t>, size_t>
            object_revisions;
        std::map<std::string, size_t> short_object_bodies;
        std::map<int32_t, size_t> directory_revisions;
        std::map<uint32_t, size_t> acp_revisions;
        std::vector<std::string> failures;

        auto row = [&](const std::string& kind, const gh::ark::Entry& entry,
                       bool passed, const std::string& detail) {
            Counts& c = counts[kind];
            ++c.seen;
            if (passed) ++c.passed; else ++c.failed;
            report << clean_tsv(kind) << '\t' << clean_tsv(entry.full_path)
                   << '\t' << entry.size << '\t'
                   << (passed ? "pass" : "fail") << '\t'
                   << clean_tsv(detail) << '\n';
            if (!passed && failures.size() < 25)
                failures.push_back(entry.full_path + ": " + detail);
        };

        for (const auto& entry : archive.entries()) {
            const std::string ext = extension(entry.name);
            if (ext != ".dtb" && ext != ".rnd_ps2" &&
                ext != ".milo_ps2" && ext != ".acp" &&
                ext != ".acg" && ext != ".acs")
                continue;
            try {
                const auto bytes = archive.read_entry(entry, ark_paths);
                if (ext == ".dtb") {
                    const auto tree = gh::dtb::parse(bytes);
                    const auto serialized = gh::dtb::serialize(tree);
                    std::ostringstream detail;
                    detail << "storage=" << dtb_storage_name(tree.storage)
                           << " version=" << tree.version
                           << " roots=" << tree.root.size()
                           << " trailing=" << tree.trailing_bytes.size();
                    row("dtb", entry, serialized == bytes, detail.str());
                } else if (ext == ".rnd_ps2" || ext == ".milo_ps2") {
                    const auto container = gh::milo::parse_container(bytes);
                    const bool outer_exact =
                        gh::milo::serialize_container(container) == bytes;
                    const auto payload = gh::milo::container_payload(container);
                    const auto directory = gh::milo::parse_directory(payload);
                    const auto directory_prefix =
                        gh::milo::serialize_directory_prefix(directory);
                    const bool prefix_exact =
                        directory.object_data_offset <= payload.size() &&
                        directory_prefix.size() ==
                            directory.object_data_offset &&
                        std::equal(
                            directory_prefix.begin(),
                            directory_prefix.end(), payload.begin());
                    ++directory_revisions[directory.dir_version];
                    std::map<std::string, size_t> local_types;
                    for (const auto& object : directory.entries) {
                        ++local_types[object.type];
                        ++object_types[object.type];
                        if (object.offset <= payload.size() &&
                            object.size >= 4 &&
                            object.size <= payload.size() - object.offset) {
                            const uint32_t packed_revision =
                                read_u32(payload,
                                         static_cast<size_t>(object.offset));
                            ++object_revisions[std::make_tuple(
                                object.type,
                                static_cast<uint16_t>(
                                    packed_revision & 0xffffu),
                                static_cast<uint16_t>(
                                    packed_revision >> 16))];
                        } else {
                            ++short_object_bodies[object.type];
                        }
                    }
                    std::ostringstream detail;
                    detail << "structure=0x" << std::hex
                           << static_cast<uint32_t>(
                                  container.header.structure)
                           << std::dec << " blocks=" << container.blocks.size()
                           << " payload=" << payload.size()
                           << " dir_rev=" << directory.dir_version
                           << " dir_type=" << directory.dir_type
                           << " dir_name=" << directory.dir_name
                           << " objects=" << directory.entries.size()
                           << " prefix_exact="
                           << (prefix_exact ? "yes" : "no")
                           << " externals="
                           << directory.external_resources.size()
                           << " trailing=" << container.trailing_bytes.size()
                           << " types=";
                    bool first = true;
                    for (const auto& pair : local_types) {
                        if (!first) detail << ',';
                        first = false;
                        detail << pair.first << ':' << pair.second;
                    }
                    row("milo", entry, outer_exact && prefix_exact,
                        detail.str());
                } else if (ext == ".acp") {
                    const auto file = gh::acp::parse(bytes);
                    const bool exact = gh::acp::serialize(file) == bytes;
                    ++acp_revisions[file.revision];
                    std::ostringstream detail;
                    detail << "class=" << file.class_name
                           << " name=" << file.object_name
                           << " body_rev=" << file.revision
                           << " field_28=" << file.field_28
                           << " set0_channels="
                           << file.channel_sets[0].channels.size()
                           << " set0_samples="
                           << file.channel_sets[0].sample_count
                           << " set0_compression="
                           << file.channel_sets[0].compression
                           << " set1_channels="
                           << file.channel_sets[1].channels.size()
                           << " set1_samples="
                           << file.channel_sets[1].sample_count
                           << " set1_compression="
                           << file.channel_sets[1].compression
                           << " trailing=" << file.trailing_bytes.size();
                    row("acp", entry,
                        exact && file.trailing_bytes.empty(), detail.str());
                } else if (ext == ".acg") {
                    const auto graph = gh::acg::parse(bytes);
                    const auto serialized = gh::acg::serialize(graph);
                    size_t nodes = 0;
                    for (const auto& clip : graph.clips)
                        nodes += clip.nodes.size();
                    std::ostringstream detail;
                    detail << "version=" << graph.version
                           << " clips=" << graph.clips.size()
                           << " nodes=" << nodes
                           << " trailing=" << graph.trailing_bytes.size();
                    row("acg", entry,
                        serialized == bytes &&
                            graph.trailing_bytes.empty(),
                        detail.str());
                } else if (ext == ".acs") {
                    size_t lines = 0;
                    size_t includes = 0;
                    bool text = true;
                    for (size_t i = 0; i < bytes.size(); ++i) {
                        const uint8_t ch = bytes[i];
                        if (ch == '\n') ++lines;
                        if (ch == 0 ||
                            (ch < 0x20 && ch != '\r' && ch != '\n' &&
                             ch != '\t')) {
                            text = false;
                            break;
                        }
                    }
                    if (!bytes.empty() && bytes.back() != '\n') ++lines;
                    const std::string contents(bytes.begin(), bytes.end());
                    size_t pos = 0;
                    while ((pos = contents.find("#include", pos)) !=
                           std::string::npos) {
                        ++includes;
                        pos += 8;
                    }
                    std::ostringstream detail;
                    detail << "text=" << (text ? "yes" : "no")
                           << " bytes=" << bytes.size()
                           << " lines=" << lines
                           << " includes=" << includes;
                    row("acs", entry, text, detail.str());
                }
            } catch (const std::exception& ex) {
                const std::string kind =
                    ext == ".dtb" ? "dtb" :
                    (ext == ".acp" ? "acp" :
                     (ext == ".acg" ? "acg" :
                      (ext == ".acs" ? "acs" : "milo")));
                row(kind, entry, false, ex.what());
            }
        }

        std::printf("archive entries: %zu\n", archive.entries().size());
        for (const auto& pair : counts) {
            std::printf("%-5s seen=%zu pass=%zu fail=%zu\n",
                        pair.first.c_str(), pair.second.seen,
                        pair.second.passed, pair.second.failed);
        }
        std::printf("directory revisions:");
        for (const auto& pair : directory_revisions)
            std::printf(" %d:%zu", pair.first, pair.second);
        std::printf("\nACP revisions:");
        for (const auto& pair : acp_revisions)
            std::printf(" %u:%zu", pair.first, pair.second);
        std::printf("\nobject types: %zu\n", object_types.size());
        std::printf("object type/revision rows: %zu\n",
                    object_revisions.size());
        for (const auto& failure : failures)
            std::printf("FAIL %s\n", failure.c_str());
        std::printf("report: %s\n", report_path.c_str());

        const std::string revision_report_path =
            report_path + ".revisions.tsv";
        std::ofstream revision_report(revision_report_path,
                                      std::ios::binary);
        if (!revision_report)
            throw std::runtime_error("cannot create " +
                                     revision_report_path);
        revision_report << "type\tmain_revision\talt_revision\tcount\n";
        for (const auto& pair : object_revisions) {
            revision_report << std::get<0>(pair.first) << '\t'
                            << std::get<1>(pair.first) << '\t'
                            << std::get<2>(pair.first) << '\t'
                            << pair.second << '\n';
        }
        for (const auto& pair : short_object_bodies)
            revision_report << pair.first << "\tshort\tshort\t"
                            << pair.second << '\n';
        std::printf("revision report: %s\n",
                    revision_report_path.c_str());

        size_t failures_total = 0;
        for (const auto& pair : counts) failures_total += pair.second.failed;
        return failures_total == 0 ? 0 : 1;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "format_audit: %s\n", ex.what());
        return 2;
    }
}
