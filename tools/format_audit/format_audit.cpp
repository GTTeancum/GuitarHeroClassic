#include "acg.h"
#include "acp.h"
#include "acs.h"
#include "ark_v3.h"
#include "dtb.h"
#include "milo.h"
#include "milo_object.h"
#include "milo_scene/milo_scene.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
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

struct BodyCounts {
    size_t exact = 0;
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
        std::map<std::tuple<std::string, uint16_t, uint16_t>, size_t>
            root_revisions;
        std::map<std::string, size_t> short_object_bodies;
        std::map<int32_t, size_t> directory_revisions;
        std::map<uint32_t, size_t> acp_revisions;
        std::map<std::string, BodyCounts> body_counts;
        std::map<std::string, BodyCounts> root_body_counts;
        std::vector<std::string> body_failures;
        std::vector<std::string> mat_records;
        std::vector<std::string> view_records;
        std::vector<std::string> object_records;
        std::vector<std::string> failures;
        std::set<std::string> archive_paths;
        std::map<uint32_t, size_t> legacy_anim_operation_types;
        size_t legacy_anim_objects_with_operations = 0;
        size_t legacy_anim_objects_with_references = 0;
        size_t legacy_anim_reference_count = 0;
        auto note_legacy_animatable =
            [&](const gh::milo_object::LegacyAnimatable& animatable) {
                if (!animatable.operations.empty())
                    ++legacy_anim_objects_with_operations;
                if (!animatable.objects.empty())
                    ++legacy_anim_objects_with_references;
                legacy_anim_reference_count += animatable.objects.size();
                for (const auto& operation : animatable.operations)
                    ++legacy_anim_operation_types[operation.type];
            };
        for (const auto& entry : archive.entries())
            archive_paths.insert(lower(entry.full_path));

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
                    const bool directory_exact =
                        directory.boundaries_exact &&
                        gh::milo::serialize_directory(directory) == payload;
                    ++directory_revisions[directory.dir_version];
                    uint32_t packed_root_revision = 0;
                    bool has_root_revision =
                        directory.dir_version >= 24 &&
                        directory.object_data_offset <= payload.size() &&
                        payload.size() - directory.object_data_offset >= 4;
                    if (has_root_revision) {
                        packed_root_revision = read_u32(
                            payload,
                            static_cast<size_t>(
                                directory.object_data_offset));
                        ++root_revisions[std::make_tuple(
                            directory.dir_type,
                            static_cast<uint16_t>(
                                packed_root_revision & 0xffffu),
                            static_cast<uint16_t>(
                                packed_root_revision >> 16))];
                    }
                    if (directory.dir_version == 24 &&
                        (directory.dir_type == "ObjectDir" ||
                         directory.dir_type == "RndDir" ||
                         directory.dir_type == "PanelDir" ||
                         directory.dir_type == "WorldDir" ||
                         directory.dir_type == "Character" ||
                         directory.dir_type == "BandCharacter" ||
                         directory.dir_type == "CharClipSet")) {
                        bool exact = false;
                        std::string root_error;
                        try {
                            if (directory.dir_type == "ObjectDir") {
                                const auto decoded =
                                    gh::milo_object::
                                        parse_object_dir16(
                                            directory.dir_body_bytes);
                                exact =
                                    gh::milo_object::
                                        serialize_object_dir16(decoded) ==
                                    directory.dir_body_bytes;
                            } else if (directory.dir_type == "RndDir") {
                                const auto decoded =
                                    gh::milo_object::parse_rnd_dir8(
                                        directory.dir_body_bytes);
                                exact =
                                    gh::milo_object::serialize_rnd_dir8(
                                        decoded) ==
                                    directory.dir_body_bytes;
                            } else if (
                                directory.dir_type == "PanelDir") {
                                const auto decoded =
                                    gh::milo_object::parse_panel_dir2(
                                        directory.dir_body_bytes);
                                exact =
                                    gh::milo_object::
                                        serialize_panel_dir2(decoded) ==
                                    directory.dir_body_bytes;
                            } else if (
                                directory.dir_type == "WorldDir") {
                                const auto decoded =
                                    gh::milo_object::parse_world_dir11(
                                        directory.dir_body_bytes);
                                exact =
                                    gh::milo_object::
                                        serialize_world_dir11(decoded) ==
                                    directory.dir_body_bytes;
                            } else if (
                                directory.dir_type == "Character") {
                                const auto decoded =
                                    gh::milo_object::parse_character9(
                                        directory.dir_body_bytes);
                                exact =
                                    gh::milo_object::
                                        serialize_character9(decoded) ==
                                    directory.dir_body_bytes;
                            } else if (
                                directory.dir_type ==
                                "BandCharacter") {
                                const auto decoded =
                                    gh::milo_object::
                                        parse_band_character1(
                                            directory.dir_body_bytes);
                                exact =
                                    gh::milo_object::
                                        serialize_band_character1(
                                            decoded) ==
                                    directory.dir_body_bytes;
                            } else {
                                uint32_t clip_count = 0;
                                for (const auto& child :
                                     directory.entries) {
                                    if (child.type == "CharClip" ||
                                        child.type ==
                                            "CharClipSamples")
                                        ++clip_count;
                                }
                                const auto decoded =
                                    gh::milo_object::
                                        parse_char_clip_set14(
                                            directory.dir_body_bytes,
                                            clip_count);
                                exact =
                                    gh::milo_object::
                                        serialize_char_clip_set14(
                                            decoded) ==
                                    directory.dir_body_bytes;
                            }
                        } catch (const std::exception& ex) {
                            root_error = ex.what();
                        }
                        BodyCounts& audited =
                            root_body_counts[directory.dir_type];
                        if (exact) {
                            ++audited.exact;
                        } else {
                            ++audited.failed;
                            if (body_failures.size() < 25)
                                body_failures.push_back(
                                    entry.full_path + "::root::" +
                                    directory.dir_type + ": " +
                                    root_error);
                        }
                    }
                    std::map<std::string, size_t> local_types;
                    bool audited_bodies_exact =
                        directory.dir_version == 10;
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
                            {
                                std::ostringstream rec;
                                rec << clean_tsv(entry.full_path) << '\t'
                                    << clean_tsv(directory.dir_type) << '\t'
                                    << clean_tsv(object.type) << '\t'
                                    << clean_tsv(object.name) << '\t'
                                    << (packed_revision & 0xffffu) << '\t'
                                    << (packed_revision >> 16) << '\t'
                                    << object.size << '\n';
                                object_records.push_back(rec.str());
                            }
                            const bool audit_semantic_body =
                                (directory.dir_version == 24 &&
                                 (object.type == "Mesh" ||
                                  object.type == "Tex" ||
                                  object.type == "Mat" ||
                                  object.type == "Cam" ||
                                  object.type == "Light" ||
                                  object.type == "Environ" ||
                                  object.type == "CamAnim" ||
                                  object.type == "EnvAnim" ||
                                  object.type == "LightAnim" ||
                                  object.type == "ParticleSysAnim" ||
                                  object.type == "MeshAnim" ||
                                  object.type == "MatAnim" ||
                                  object.type == "TransAnim" ||
                                  object.type == "Text" ||
                                  object.type == "ParticleSys" ||
                                  object.type == "Font" ||
                                  object.type == "AnimFilter" ||
                                  object.type == "CharBone" ||
                                  object.type == "CharClipFilter" ||
                                  object.type == "CharClipGroup" ||
                                  object.type == "CharClipSamples" ||
                                  object.type == "CharDriver" ||
                                  object.type == "CharDriverMidi" ||
                                  object.type == "CharEyes" ||
                                  object.type == "CharForeTwist" ||
                                  object.type == "CharHair" ||
                                  object.type == "CharIKHand" ||
                                  object.type == "CharIKMidi" ||
                                  object.type == "CharIKRod" ||
                                  object.type == "CharLookAt" ||
                                  object.type == "CharPosConstraint" ||
                                  object.type == "CharServoBone" ||
                                  object.type == "CharUpperTwist" ||
                                  object.type == "CharWalk" ||
                                  object.type == "CharWeightSetter" ||
                                  object.type == "EventTrigger" ||
                                  object.type == "FaceFxLipSyncServo" ||
                                  object.type == "OutfitLoader" ||
                                  object.type == "WorldFx" ||
                                  object.type == "Group")) ||
                                (directory.dir_version == 10 &&
                                (object.type == "Cam" ||
                                 object.type == "Flare" ||
                                 object.type == "Light" ||
                                 object.type == "Environ" ||
                                 object.type == "Morph" ||
                                 object.type == "TransAnim" ||
                                 object.type == "MultiMesh" ||
                                 object.type == "MeshAnim" ||
                                 object.type == "CamAnim" ||
                                 object.type == "EnvAnim" ||
                                 object.type == "LightAnim" ||
                                 object.type == "ParticleSysAnim" ||
                                 object.type == "MatAnim" ||
                                 object.type == "Text" ||
                                 object.type == "Movie" ||
                                 object.type == "Font" ||
                                 object.type == "Tex" ||
                                 object.type == "View" ||
                                 object.type == "Mat" ||
                                 object.type == "ParticleSys" ||
                                 object.type == "Mesh"));
                            if (audit_semantic_body) {
                                const auto first =
                                    payload.begin() + object.offset;
                                const std::vector<uint8_t> body(
                                    first, first + object.size);
                                bool exact = false;
                                std::string body_error;
                                try {
                                    if (directory.dir_version == 24 &&
                                        object.type == "Mesh") {
                                        const auto decoded =
                                            gh::milo_object::parse_mesh28(
                                                body,
                                                static_cast<uint32_t>(
                                                    directory.dir_version));
                                        exact =
                                            gh::milo_object::
                                                serialize_mesh28(
                                                    decoded,
                                                    static_cast<uint32_t>(
                                                        directory.dir_version)) ==
                                            body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "Tex") {
                                        const auto decoded =
                                            gh::milo_object::parse_tex10(body);
                                        exact =
                                            gh::milo_object::serialize_tex10(
                                                decoded) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        (object.type == "CharBone" ||
                                         object.type ==
                                             "CharClipFilter" ||
                                         object.type ==
                                             "CharClipGroup" ||
                                         object.type == "CharDriver" ||
                                         object.type ==
                                             "CharDriverMidi" ||
                                         object.type == "CharEyes" ||
                                         object.type ==
                                             "CharForeTwist" ||
                                         object.type == "CharHair" ||
                                         object.type == "CharIKHand" ||
                                         object.type == "CharIKMidi" ||
                                         object.type == "CharIKRod" ||
                                         object.type == "CharLookAt" ||
                                         object.type ==
                                             "CharPosConstraint" ||
                                         object.type ==
                                             "CharServoBone" ||
                                         object.type ==
                                             "CharUpperTwist" ||
                                         object.type == "CharWalk" ||
                                         object.type ==
                                             "CharWeightSetter" ||
                                         object.type == "EventTrigger" ||
                                         object.type ==
                                             "FaceFxLipSyncServo" ||
                                         object.type == "OutfitLoader" ||
                                         object.type == "WorldFx")) {
                                        exact =
                                            gh::milo_object::
                                                round_trip_gh2_object_body(
                                                    object.type, body,
                                                    24) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type ==
                                            "CharClipSamples") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_char_clip_samples10(
                                                    body);
                                        exact =
                                            gh::milo_object::
                                                serialize_char_clip_samples10(
                                                    decoded) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "Mat") {
                                        const auto decoded =
                                            gh::milo_object::parse_mat27(body);
                                        exact =
                                            gh::milo_object::serialize_mat27(
                                                decoded) == body;
                                        std::ostringstream rec;
                                        rec << "gh2\t"
                                            << clean_tsv(entry.full_path) << '\t'
                                            << clean_tsv(object.name)
                                            << "\t27\t" << decoded.blend
                                            << '\t' << decoded.color[0]
                                            << '\t' << decoded.color[1]
                                            << '\t' << decoded.color[2]
                                            << '\t' << decoded.color[3]
                                            << '\t'
                                            << decoded.use_environment
                                            << "\tn/a\tn/a\t"
                                            << decoded.prelit << '\t'
                                            << decoded.cull
                                            << "\tn/a\tn/a\t"
                                            << decoded.z_mode << '\t'
                                            << decoded.alpha_cut << '\t'
                                            << decoded.alpha_write << '\t'
                                            << decoded.tex_gen << '\t'
                                            << decoded.tex_wrap << '\t'
                                            << clean_tsv(
                                                   decoded.diffuse_texture)
                                            << '\t'
                                            << clean_tsv(
                                                   decoded.environment_map)
                                            << '\t'
                                            << clean_tsv(decoded.next_pass)
                                            << "\t\n";
                                        mat_records.push_back(rec.str());
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "Cam") {
                                        const auto decoded =
                                            gh::milo_object::parse_cam12(body);
                                        exact =
                                            gh::milo_object::serialize_cam12(
                                                decoded) == body;
                                    } else if (object.type == "Cam") {
                                        const auto decoded =
                                            gh::milo_object::parse_cam(body);
                                        exact =
                                            gh::milo_object::serialize_cam(
                                                decoded) == body;
                                    } else if (object.type == "Flare") {
                                        const auto decoded =
                                            gh::milo_object::parse_flare(
                                                body);
                                        exact =
                                            gh::milo_object::serialize_flare(
                                                decoded) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "Light") {
                                        const auto decoded =
                                            gh::milo_object::parse_light6(
                                                body);
                                        exact =
                                            gh::milo_object::serialize_light6(
                                                decoded) == body;
                                    } else if (object.type == "Light") {
                                        const auto decoded =
                                            gh::milo_object::parse_light(
                                                body);
                                        exact =
                                            gh::milo_object::serialize_light(
                                                decoded) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "Environ") {
                                        const auto decoded =
                                            gh::milo_object::parse_environ5(
                                                body);
                                        exact =
                                            gh::milo_object::
                                                serialize_environ5(decoded) ==
                                            body;
                                    } else if (
                                        object.type == "Environ") {
                                        const auto decoded =
                                            gh::milo_object::parse_environ(
                                                body);
                                        exact =
                                            gh::milo_object::
                                                serialize_environ(decoded) ==
                                            body;
                                    } else if (object.type == "Morph") {
                                        const auto decoded =
                                            gh::milo_object::parse_morph(
                                                body);
                                        note_legacy_animatable(
                                            decoded.animatable);
                                        exact =
                                            gh::milo_object::
                                                serialize_morph(decoded) ==
                                            body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "TransAnim") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_trans_anim6(body);
                                        exact =
                                            gh::milo_object::
                                                serialize_trans_anim6(
                                                    decoded) == body;
                                    } else if (
                                        object.type == "TransAnim") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_trans_anim(body);
                                        note_legacy_animatable(
                                            decoded.animatable);
                                        exact =
                                            gh::milo_object::
                                                serialize_trans_anim(
                                                    decoded) == body;
                                    } else if (
                                        object.type == "MultiMesh") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_multi_mesh(body);
                                        exact =
                                            gh::milo_object::
                                                serialize_multi_mesh(
                                                    decoded) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "MeshAnim") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_mesh_anim1(body);
                                        exact =
                                            gh::milo_object::
                                                serialize_mesh_anim1(
                                                    decoded) == body;
                                    } else if (
                                        object.type == "MeshAnim") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_mesh_anim(body);
                                        note_legacy_animatable(
                                            decoded.animatable);
                                        exact =
                                            gh::milo_object::
                                                serialize_mesh_anim(
                                                    decoded) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "CamAnim") {
                                        const auto decoded =
                                            gh::milo_object::parse_cam_anim2(
                                                body);
                                        exact =
                                            gh::milo_object::
                                                serialize_cam_anim2(decoded) ==
                                            body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "EnvAnim") {
                                        const auto decoded =
                                            gh::milo_object::parse_env_anim4(
                                                body);
                                        exact =
                                            gh::milo_object::
                                                serialize_env_anim4(decoded) ==
                                            body;
                                    } else if (
                                        object.type == "CamAnim") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_cam_anim(body);
                                        note_legacy_animatable(
                                            decoded.animatable);
                                        exact =
                                            gh::milo_object::
                                                serialize_cam_anim(
                                                    decoded) == body;
                                    } else if (
                                        object.type == "EnvAnim") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_env_anim(body);
                                        note_legacy_animatable(
                                            decoded.animatable);
                                        exact =
                                            gh::milo_object::
                                                serialize_env_anim(
                                                    decoded) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "LightAnim") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_light_anim2(body);
                                        exact =
                                            gh::milo_object::
                                                serialize_light_anim2(
                                                    decoded) == body;
                                    } else if (
                                        object.type == "LightAnim") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_light_anim(body);
                                        note_legacy_animatable(
                                            decoded.animatable);
                                        exact =
                                            gh::milo_object::
                                                serialize_light_anim(
                                                    decoded) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type ==
                                            "ParticleSysAnim") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_particle_sys_anim3(
                                                    body);
                                        exact =
                                            gh::milo_object::
                                                serialize_particle_sys_anim3(
                                                    decoded) == body;
                                    } else if (
                                        object.type ==
                                        "ParticleSysAnim") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_particle_sys_anim(
                                                    body);
                                        note_legacy_animatable(
                                            decoded.animatable);
                                        exact =
                                            gh::milo_object::
                                                serialize_particle_sys_anim(
                                                    decoded) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "MatAnim") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_mat_anim7(body);
                                        exact =
                                            gh::milo_object::
                                                serialize_mat_anim7(
                                                    decoded) == body;
                                    } else if (
                                        object.type == "MatAnim") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_mat_anim(body);
                                        note_legacy_animatable(
                                            decoded.animatable);
                                        exact =
                                            gh::milo_object::
                                                serialize_mat_anim(
                                                    decoded) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "Text") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_text17(body);
                                        exact =
                                            gh::milo_object::
                                                serialize_text17(
                                                    decoded) == body;
                                    } else if (object.type == "Text") {
                                        const auto decoded =
                                            gh::milo_object::parse_text(
                                                body);
                                        exact =
                                            gh::milo_object::
                                                serialize_text(decoded) ==
                                            body;
                                    } else if (
                                        object.type == "Movie") {
                                        const auto decoded =
                                            gh::milo_object::parse_movie(
                                                body);
                                        note_legacy_animatable(
                                            decoded.animatable);
                                        exact =
                                            gh::milo_object::
                                                serialize_movie(decoded) ==
                                            body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "Font") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_font15(body);
                                        exact =
                                            gh::milo_object::
                                                serialize_font15(
                                                    decoded) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "AnimFilter") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_anim_filter1(body);
                                        exact =
                                            gh::milo_object::
                                                serialize_anim_filter1(
                                                    decoded) == body;
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "Group") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_group12(body);
                                        exact =
                                            gh::milo_object::
                                                serialize_group12(
                                                    decoded) == body;
                                    } else if (
                                        object.type == "Font") {
                                        const auto decoded =
                                            gh::milo_object::parse_font(
                                                body);
                                        exact =
                                            gh::milo_object::
                                                serialize_font(decoded) ==
                                            body;
                                    } else if (
                                        object.type == "Tex") {
                                        const auto decoded =
                                            gh::milo_object::parse_tex(
                                                body);
                                        exact =
                                            gh::milo_object::
                                                serialize_tex(decoded) ==
                                            body;
                                    } else if (
                                        object.type == "View") {
                                        const auto decoded =
                                            gh::milo_object::parse_view(
                                                body);
                                        note_legacy_animatable(
                                            decoded.animatable);
                                        std::ostringstream rec;
                                        rec << clean_tsv(entry.full_path)
                                            << '\t'
                                            << clean_tsv(object.name)
                                            << '\t'
                                            << clean_tsv(
                                                   decoded.children_owner)
                                            << '\t'
                                            << decoded.showing_range[0]
                                            << '\t'
                                            << decoded.showing_range[1]
                                            << '\t'
                                            << decoded.animatable.objects.size()
                                            << '\t'
                                            << decoded.drawable.objects.size()
                                            << '\t';
                                        for (size_t i = 0;
                                             i <
                                             decoded.animatable.objects.size();
                                             ++i) {
                                            if (i) rec << ';';
                                            rec << clean_tsv(
                                                decoded.animatable.objects[i]);
                                        }
                                        rec << '\t';
                                        for (size_t i = 0;
                                             i <
                                             decoded.drawable.objects.size();
                                             ++i) {
                                            if (i) rec << ';';
                                            rec << clean_tsv(
                                                decoded.drawable.objects[i]);
                                        }
                                        rec << '\n';
                                        view_records.push_back(rec.str());
                                        exact =
                                            gh::milo_object::
                                                serialize_view(decoded) ==
                                            body;
                                    } else if (
                                        object.type == "Mat") {
                                        const auto decoded =
                                            gh::milo_object::parse_mat(
                                                body);
                                        exact =
                                            gh::milo_object::
                                                serialize_mat(decoded) ==
                                            body;
                                        std::ostringstream textures;
                                        for (size_t i = 0;
                                             i < decoded.textures.size();
                                             ++i) {
                                            if (i) textures << ';';
                                            const auto& texture =
                                                decoded.textures[i];
                                            textures
                                                << texture.stage_blend << ','
                                                << texture.tex_gen << ','
                                                << texture.wrap << ','
                                                << clean_tsv(texture.texture);
                                        }
                                        std::ostringstream rec;
                                        rec << "gh1\t"
                                            << clean_tsv(entry.full_path) << '\t'
                                            << clean_tsv(object.name)
                                            << "\t21\t" << decoded.blend
                                            << '\t' << decoded.color[0]
                                            << '\t' << decoded.color[1]
                                            << '\t' << decoded.color[2]
                                            << '\t' << decoded.color[3]
                                            << '\t'
                                            << decoded.use_environment
                                            << '\t'
                                            << decoded.vertex_ambient
                                            << '\t'
                                            << decoded.vertex_dynamic
                                            << "\tn/a\t" << decoded.cull
                                            << '\t' << decoded.multipass
                                            << '\t' << decoded.normalize
                                            << '\t' << decoded.z_mode
                                            << '\t' << decoded.alpha_cut
                                            << '\t' << decoded.alpha_write
                                            << "\tn/a\tn/a\t\t\t\t"
                                            << textures.str() << '\n';
                                        mat_records.push_back(rec.str());
                                    } else if (
                                        directory.dir_version == 24 &&
                                        object.type == "ParticleSys") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_particle_sys27(
                                                    body);
                                        exact =
                                            gh::milo_object::
                                                serialize_particle_sys27(
                                                    decoded) == body;
                                    } else if (
                                        object.type == "ParticleSys") {
                                        const auto decoded =
                                            gh::milo_object::
                                                parse_particle_sys(body);
                                        note_legacy_animatable(
                                            decoded.animatable);
                                        exact =
                                            gh::milo_object::
                                                serialize_particle_sys(
                                                    decoded) == body;
                                    } else {
                                        const auto decoded =
                                            gh::milo_object::parse_mesh(
                                                body);
                                        exact =
                                            gh::milo_object::
                                                serialize_mesh(decoded) ==
                                            body;
                                    }
                                } catch (const std::exception& ex) {
                                    body_error = ex.what();
                                }
                                BodyCounts& audited =
                                    body_counts[object.type];
                                if (exact) {
                                    ++audited.exact;
                                } else {
                                    ++audited.failed;
                                    audited_bodies_exact = false;
                                    if (body_failures.size() < 25) {
                                        body_failures.push_back(
                                            entry.full_path + "::" +
                                            object.type + "::" + object.name +
                                            ": " + body_error);
                                    }
                                }
                            }
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
                           << " root_rev=";
                    if (has_root_revision) {
                        detail << (packed_root_revision & 0xffffu)
                               << ':' << (packed_root_revision >> 16);
                    } else {
                        detail << "n/a";
                    }
                    detail
                           << " objects=" << directory.entries.size()
                           << " prefix_exact="
                           << (prefix_exact ? "yes" : "no")
                           << " boundaries_exact="
                           << (directory.boundaries_exact ? "yes" : "no")
                           << " directory_exact="
                           << (directory_exact ? "yes" : "no")
                           << " audited_bodies_exact="
                           << (directory.dir_version == 10
                                   ? (audited_bodies_exact ? "yes" : "no")
                                   : "n/a")
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
                    row("milo", entry,
                        outer_exact && prefix_exact &&
                            (directory.dir_version != 10 ||
                             (directory.boundaries_exact &&
                              directory_exact &&
                              audited_bodies_exact)),
                        detail.str());
                } else if (ext == ".acp") {
                    const auto file = gh::acp::parse(bytes);
                    const bool exact = gh::acp::serialize(file) == bytes;
                    ++acp_revisions[file.revision];
                    std::ostringstream detail;
                    detail << "class=" << file.class_name
                           << " name=" << file.object_name
                           << " body_rev=" << file.revision
                           << " sample_set_revision="
                           << file.sample_set_revision
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
                    const auto file = gh::acs::parse(bytes);
                    size_t includes = 0;
                    size_t invocations = 0;
                    size_t resolved_includes = 0;
                    for (const auto& line : file.lines) {
                        if (line.kind == gh::acs::LineKind::Include) {
                            ++includes;
                            const std::string target =
                                gh::acs::compiled_include_path(
                                    entry.full_path, line.value);
                            if (archive_paths.count(lower(target)))
                                ++resolved_includes;
                        } else if (
                            line.kind == gh::acs::LineKind::Invocation) {
                            ++invocations;
                        }
                    }
                    const bool exact = gh::acs::serialize(file) == bytes;
                    std::ostringstream detail;
                    detail << "bytes=" << bytes.size()
                           << " lines=" << file.lines.size()
                           << " includes=" << includes;
                    detail << " resolved_includes=" << resolved_includes
                           << " invocations=" << invocations;
                    row("acs", entry,
                        exact && resolved_includes == includes,
                        detail.str());
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
        std::printf("root type/revision rows: %zu\n",
                    root_revisions.size());
        if (!legacy_anim_operation_types.empty() ||
            legacy_anim_objects_with_references != 0) {
            std::printf(
                "legacy Animatable0: operation_objects=%zu "
                "reference_objects=%zu references=%zu operation_types=",
                legacy_anim_objects_with_operations,
                legacy_anim_objects_with_references,
                legacy_anim_reference_count);
            for (const auto& pair : legacy_anim_operation_types)
                std::printf("%s%u:%zu",
                            pair == *legacy_anim_operation_types.begin()
                                ? "" : ",",
                            pair.first, pair.second);
            std::printf("\n");
        }
        for (const auto& pair : body_counts) {
            std::printf("body %-12s exact=%zu fail=%zu\n",
                        pair.first.c_str(), pair.second.exact,
                        pair.second.failed);
        }
        for (const auto& pair : root_body_counts) {
            std::printf("root %-12s exact=%zu fail=%zu\n",
                        pair.first.c_str(), pair.second.exact,
                        pair.second.failed);
        }
        for (const auto& failure : body_failures)
            std::printf("BODY_FAIL %s\n", failure.c_str());
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

        const std::string root_report_path =
            report_path + ".roots.tsv";
        std::ofstream root_report(root_report_path, std::ios::binary);
        if (!root_report)
            throw std::runtime_error("cannot create " +
                                     root_report_path);
        root_report << "type\tmain_revision\talt_revision\tcount\n";
        for (const auto& pair : root_revisions) {
            root_report << std::get<0>(pair.first) << '\t'
                        << std::get<1>(pair.first) << '\t'
                        << std::get<2>(pair.first) << '\t'
                        << pair.second << '\n';
        }
        std::printf("root report: %s\n", root_report_path.c_str());

        const std::string mat_report_path =
            report_path + ".materials.tsv";
        std::ofstream mat_report(mat_report_path, std::ios::binary);
        if (!mat_report)
            throw std::runtime_error("cannot create " + mat_report_path);
        mat_report
            << "generation\tarchive_path\tobject_name\trevision\tblend"
               "\tcolor_r\tcolor_g\tcolor_b\tcolor_a\tuse_environment"
               "\tvertex_ambient\tvertex_dynamic\tprelit\tcull"
               "\tmultipass\tnormalize\tz_mode\talpha_cut\talpha_write"
               "\ttex_gen\ttex_wrap\tdiffuse_texture\tenvironment_map"
               "\tnext_pass\tlegacy_textures\n";
        for (const auto& record : mat_records)
            mat_report << record;
        std::printf("material report: %s\n", mat_report_path.c_str());

        const std::string view_report_path =
            report_path + ".views.tsv";
        std::ofstream view_report(view_report_path, std::ios::binary);
        if (!view_report)
            throw std::runtime_error("cannot create " + view_report_path);
        view_report
            << "archive_path\tobject_name\tchildren_owner\tshowing_min"
               "\tshowing_max\tanimation_references\tdraw_references"
               "\tanimation_objects\tdraw_objects\n";
        for (const auto& record : view_records)
            view_report << record;
        std::printf("view report: %s\n", view_report_path.c_str());

        const std::string object_report_path =
            report_path + ".objects.tsv";
        std::ofstream object_report(object_report_path, std::ios::binary);
        if (!object_report)
            throw std::runtime_error(
                "cannot create " + object_report_path);
        object_report
            << "archive_path\tdirectory_type\tobject_type\tobject_name"
               "\tmain_revision\talt_revision\tbody_size\n";
        for (const auto& record : object_records)
            object_report << record;
        std::printf("object report: %s\n", object_report_path.c_str());

        size_t failures_total = 0;
        for (const auto& pair : counts) failures_total += pair.second.failed;
        return failures_total == 0 ? 0 : 1;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "format_audit: %s\n", ex.what());
        return 2;
    }
}
