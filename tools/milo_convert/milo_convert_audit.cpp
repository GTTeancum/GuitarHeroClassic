#include "ark_v3.h"
#include "acg.h"
#include "dtb.h"
#include "gh1_animation_manifest.h"
#include "gh1_character_manifest.h"
#include "gh1_character_model_package.h"
#include "gh1_character_package.h"
#include "gh1_venue_camera_conversion.h"
#include "gh1_venue_script_conversion.h"
#include "gh2_face_config_patch.h"
#include "milo.h"
#include "milo_convert.h"
#include "milo_object.h"
#include "singer_face_track.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string lower(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
    return value;
}

std::string extension(const std::string& path) {
    const size_t dot = path.rfind('.');
    return dot == std::string::npos
               ? std::string()
               : lower(path.substr(dot));
}

std::string stem(const std::string& name) {
    const size_t dot = name.rfind('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

std::string cell(std::string value) {
    for (char& ch : value) {
        if (ch == '\t' || ch == '\r' || ch == '\n') ch = ' ';
    }
    return value;
}

struct BundleRecord {
    std::string relative_path;
    std::string kind;
    std::string source;
    size_t bytes = 0;
};

struct Gh1VenuePath {
    std::string source_venue;
    std::string target_venue;
    std::string target_path;
    bool primary = false;
};

std::optional<Gh1VenuePath> gh1_venue_target_path(
    const std::string& source_path) {
    constexpr std::string_view prefix = "venues/";
    if (source_path.rfind(prefix, 0) != 0) return std::nullopt;
    const size_t venue_end = source_path.find('/', prefix.size());
    if (venue_end == std::string::npos) return std::nullopt;
    const std::string venue =
        source_path.substr(prefix.size(), venue_end - prefix.size());
    const std::string gen_prefix =
        "venues/" + venue + "/gen/";
    if (venue.empty() || source_path.rfind(gen_prefix, 0) != 0)
        return std::nullopt;
    std::string leaf = source_path.substr(gen_prefix.size());
    if (leaf.empty() || leaf.find('/') != std::string::npos)
        return std::nullopt;
    const std::string target = "gh1_" + venue;
    const std::string leaf_extension = extension(leaf);
    const std::string leaf_stem = stem(leaf);
    const bool primary = leaf_stem == venue;
    if (leaf_extension == ".rnd_ps2") {
        leaf = (primary ? target : leaf_stem) + ".milo_ps2";
    } else if (primary) {
        leaf = target + leaf_extension;
    }
    return Gh1VenuePath{
        venue, target, "world/" + target + "/gen/" + leaf, primary};
}

void write_bundle_file(
    const fs::path& root,
    const std::string& relative_path,
    const std::vector<uint8_t>& bytes) {
    const fs::path relative =
        fs::path(relative_path).lexically_normal();
    if (relative.empty() || relative.is_absolute() ||
        *relative.begin() == "..")
        throw std::runtime_error(
            "bundle output path escapes root: " + relative_path);
    const fs::path output_path = root / relative;
    fs::create_directories(output_path.parent_path());
    std::ofstream output(output_path, std::ios::binary);
    if (!output)
        throw std::runtime_error(
            "cannot create " + output_path.string());
    output.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    if (!output)
        throw std::runtime_error(
            "cannot write " + output_path.string());
}

void link_bundle_file(
    const fs::path& root,
    const std::string& relative_path,
    const fs::path& source_path) {
    const fs::path relative =
        fs::path(relative_path).lexically_normal();
    if (relative.empty() || relative.is_absolute() ||
        *relative.begin() == "..")
        throw std::runtime_error(
            "bundle output path escapes root: " + relative_path);
    const fs::path output_path = root / relative;
    fs::create_directories(output_path.parent_path());
    std::error_code error;
    if (fs::exists(output_path)) {
        if (fs::equivalent(source_path, output_path, error) && !error)
            return;
        error.clear();
        if (!fs::remove(output_path, error) || error)
            throw std::runtime_error(
                "cannot replace " + output_path.string());
    }
    fs::create_hard_link(source_path, output_path, error);
    if (!error)
        return;
    error.clear();
    fs::copy_file(
        source_path, output_path,
        fs::copy_options::overwrite_existing, error);
    if (error)
        throw std::runtime_error(
            "cannot link or copy " + output_path.string());
}

void write_bundle_manifest(
    const fs::path& root,
    const std::string& name,
    std::vector<BundleRecord> records) {
    std::sort(
        records.begin(), records.end(),
        [](const BundleRecord& left, const BundleRecord& right) {
            return left.relative_path < right.relative_path;
        });
    for (size_t index = 1; index < records.size(); ++index)
        if (records[index - 1].relative_path ==
            records[index].relative_path)
            throw std::runtime_error(
                "duplicate bundle path: " +
                records[index].relative_path);
    fs::create_directories(root);
    const fs::path manifest_path = root / name;
    std::ofstream output(manifest_path, std::ios::binary);
    if (!output)
        throw std::runtime_error(
            "cannot create " + manifest_path.string());
    output << "relative_path\tkind\tsource\tbytes\n";
    for (const auto& record : records)
        output << cell(record.relative_path) << '\t'
               << cell(record.kind) << '\t'
               << cell(record.source) << '\t'
               << record.bytes << '\n';
    if (!output)
        throw std::runtime_error(
            "cannot write " + manifest_path.string());
}

void collect_dtb_strings(
    const std::shared_ptr<gh::dtb::Node>& node,
    std::vector<std::string>& output) {
    if (!node) return;
    if (gh::dtb::is_array(*node)) {
        for (const auto& child : gh::dtb::children(*node))
            collect_dtb_strings(child, output);
        return;
    }
    if (const auto value = gh::dtb::as_string(*node))
        output.push_back(*value);
}

void usage() {
    std::fprintf(
        stderr,
        "Usage:\n"
        "  milo_convert_audit scan <main.hdr> <main_0.ark> "
        "[main_1.ark ...] --report <report.tsv> "
        "[--bundle <dir> --target-rnd-objects <file> "
        "--target-midi-parsers <file> --target-char-objects <file>]\n"
        "  milo_convert_audit target-summaries <main.hdr> "
        "<main_0.ark> [main_1.ark ...] --report <report.tsv>\n"
        "  milo_convert_audit target-face-sweep <main.hdr> "
        "<main_0.ark> [main_1.ark ...] --report <report.tsv> "
        "[--out <dir>]\n");
    std::exit(2);
}

int target_summaries(
    const gh::ark::ArkV3Reader& archive,
    const std::vector<std::string>& ark_paths,
    const std::string& report_path) {
    fs::create_directories(fs::path(report_path).parent_path());
    std::ofstream report(report_path, std::ios::binary);
    if (!report)
        throw std::runtime_error("cannot create " + report_path);
    report
        << "archive_path\tclip\tflags\tsize_bytes\tbody_flags"
           "\tbody_play_flags\tbody_blend_width\tbody_range"
           "\tbody_legacy_flag\tbody_bytes"
           "\tsample_bytes\tfull_bytes\tone_bytes\tduplicate_bytes"
           "\tfull_channels\tone_channels"
           "\tduplicate_channels\tfull_samples\tone_samples"
           "\tduplicate_samples\ttransition_groups"
           "\ttransition_nodes\tevents\tstring_bytes"
           "\tfull_compression\tone_compression"
           "\tduplicate_compression\tfull_counts\tone_counts"
           "\tduplicate_counts\tfull_facing_channels"
           "\tone_facing_channels\n";
    const std::string root_report_path =
        report_path + ".roots.tsv";
    std::ofstream root_report(root_report_path, std::ios::binary);
    if (!root_report)
        throw std::runtime_error("cannot create " + root_report_path);
    root_report
        << "archive_path\tdirectory\tclips\tentries\tobject_type"
           "\tviewports\tblend_width\tplay_flags\tmove_self"
           "\trecenter_targets\trecenter_average\trecenter_slide"
           "\tlegacy_type\tlegacy_type_version\n";
    size_t directories = 0;
    size_t clips = 0;
    for (const auto& entry : archive.entries()) {
        if (extension(entry.name) != ".milo_ps2") continue;
        const auto container = gh::milo::parse_container(
            archive.read_entry(entry, ark_paths));
        const auto directory = gh::milo::parse_directory(
            gh::milo::container_payload(container));
        if (directory.dir_type != "CharClipSet") continue;
        uint32_t clip_count = 0;
        for (const auto& object : directory.entries)
            if (object.type == "CharClipSamples") ++clip_count;
        const auto root = gh::milo_object::parse_char_clip_set14(
            directory.dir_body_bytes, clip_count);
        const auto join =
            [](const std::vector<std::string>& values) {
                std::string result;
                for (size_t index = 0;
                     index < values.size(); ++index) {
                    if (index) result += ',';
                    result += values[index];
                }
                return result;
            };
        root_report
            << cell(entry.full_path) << '\t'
            << cell(directory.dir_name) << '\t'
            << clip_count << '\t'
            << directory.entries.size() << '\t'
            << cell(root.object_directory.object_fields.type) << '\t'
            << root.object_directory.viewports.size() << '\t'
            << root.blend_width << '\t'
            << root.play_flags << '\t'
            << (root.move_self ? 1 : 0) << '\t'
            << cell(join(root.recenter_targets)) << '\t'
            << cell(join(root.recenter_average)) << '\t'
            << (root.recenter_slide ? 1 : 0) << '\t'
            << cell(root.legacy_type) << '\t'
            << root.legacy_type_version << '\n';
        for (const auto& summary : root.clips) {
            const gh::milo::Entry* object = nullptr;
            for (const auto& candidate : directory.entries) {
                if (candidate.type == "CharClipSamples" &&
                    candidate.name == summary.clip) {
                    object = &candidate;
                    break;
                }
            }
            if (!object)
                throw std::runtime_error(
                    "clip summary body missing: " +
                    entry.full_path + "::" + summary.clip);
            const auto clip =
                gh::milo_object::parse_char_clip_samples10(
                    object->body_bytes);
            size_t transition_nodes = 0;
            size_t string_bytes =
                clip.legacy_enter_event.size() +
                clip.legacy_exit_event.size();
            for (const auto& transition : clip.transitions) {
                transition_nodes += transition.nodes.size();
                string_bytes += transition.clip.size();
            }
            for (const auto& event : clip.events)
                string_bytes += event.script.size();
            for (const auto& channel : clip.full.channels)
                string_bytes += channel.size();
            for (const auto& channel : clip.one.channels)
                string_bytes += channel.size();
            for (const auto& channel : clip.duplicate.channels)
                string_bytes += channel.size();
            const size_t sample_bytes =
                clip.full.sample_bytes.size() +
                clip.one.sample_bytes.size() +
                clip.duplicate.sample_bytes.size();
            report << cell(entry.full_path) << '\t'
                   << cell(summary.clip) << '\t'
                   << summary.flags << '\t'
                   << summary.size_bytes << '\t'
                   << clip.flags << '\t'
                   << clip.play_flags << '\t'
                   << clip.blend_width << '\t'
                   << clip.range << '\t'
                   << (clip.legacy_flag ? 1 : 0) << '\t'
                   << object->body_bytes.size() << '\t'
                   << sample_bytes << '\t'
                   << clip.full.sample_bytes.size() << '\t'
                   << clip.one.sample_bytes.size() << '\t'
                   << clip.duplicate.sample_bytes.size() << '\t'
                   << clip.full.channels.size() << '\t'
                   << clip.one.channels.size() << '\t'
                   << clip.duplicate.channels.size() << '\t'
                   << clip.full.sample_count << '\t'
                   << clip.one.sample_count << '\t'
                   << clip.duplicate.sample_count << '\t'
                   << clip.transitions.size() << '\t'
                   << transition_nodes << '\t'
                   << clip.events.size() << '\t'
                   << string_bytes << '\t'
                   << clip.full.compression << '\t'
                   << clip.one.compression << '\t'
                   << clip.duplicate.compression << '\t';
            const auto write_counts =
                [&](const auto& counts) {
                    for (size_t index = 0;
                         index < counts.size(); ++index) {
                        if (index) report << ',';
                        report << counts[index];
                    }
                };
            write_counts(clip.full.counts);
            report << '\t';
            write_counts(clip.one.counts);
            report << '\t';
            write_counts(clip.duplicate.counts);
            const auto facing_channels =
                [](const auto& channels) {
                    size_t count = 0;
                    for (const auto& channel : channels) {
                        if (channel.rfind("bone_facing.", 0) == 0 ||
                            channel.rfind("bone_facing_delta.", 0) == 0)
                            ++count;
                    }
                    return count;
                };
            report << '\t' << facing_channels(clip.full.channels)
                   << '\t' << facing_channels(clip.one.channels)
                   << '\n';
            ++clips;
        }
        ++directories;
    }
    std::printf(
        "target_summaries directories=%zu clips=%zu report=%s "
        "roots=%s\n",
        directories, clips, report_path.c_str(),
        root_report_path.c_str());
    return 0;
}

int target_face_sweep(
    const gh::ark::ArkV3Reader& archive,
    const std::vector<std::string>& ark_paths,
    const std::string& report_path,
    const std::string& output_root) {
    fs::create_directories(fs::path(report_path).parent_path());
    std::ofstream report(report_path, std::ios::binary);
    if (!report)
        throw std::runtime_error("cannot create " + report_path);
    report
        << "voc\tmidi\tversion\tcurves\tfooter_bytes"
           "\ttime_spans\ttick_spans\tmidi_bytes"
           "\tpatched_bytes\tprefix_preserved\tstatus\n";
    size_t voc_count = 0;
    size_t paired_count = 0;
    size_t version_1200 = 0;
    size_t version_1500 = 0;
    size_t total_spans = 0;
    std::vector<BundleRecord> bundle_records;
    for (const auto& entry : archive.entries()) {
        if (extension(entry.name) != ".voc" ||
            entry.full_path.rfind("songs/", 0) != 0)
            continue;
        const auto animation =
            gh::milo_convert::parse_gh2_facefx_animation(
                archive.read_entry(entry, ark_paths));
        if (animation.version == 1200)
            ++version_1200;
        else if (animation.version == 1500)
            ++version_1500;
        const auto time_spans =
            gh::milo_convert::derive_gh1_singer_open_spans(
                animation);
        std::string midi_path = entry.full_path;
        midi_path.replace(
            midi_path.size() - 4, 4, ".mid");
        const auto midi_entry = archive.find(midi_path);
        if (!midi_entry) {
            report
                << cell(entry.full_path) << '\t'
                << cell(midi_path) << '\t'
                << animation.version << '\t'
                << animation.curves.size() << '\t'
                << animation.archive_footer.size() << '\t'
                << time_spans.size()
                << "\t0\t0\t0\t0\tmissing-midi\n";
            ++voc_count;
            continue;
        }
        const auto midi =
            archive.read_entry(*midi_entry, ark_paths);
        const auto tick_spans =
            gh::milo_convert::map_singer_face_times_to_midi(
                midi, time_spans);
        const auto patched =
            gh::milo_convert::append_gh1_singer_face_track(
                midi, tick_spans);
        const auto repeated =
            gh::milo_convert::append_gh1_singer_face_track(
                midi, tick_spans);
        if (repeated != patched)
            throw std::runtime_error(
                "target face sweep: nondeterministic MIDI: " +
                midi_path);
        const bool prefix_preserved =
            patched.size() > midi.size() &&
            std::equal(
                midi.begin(), midi.begin() + 10,
                patched.begin()) &&
            std::equal(
                midi.begin() + 12, midi.end(),
                patched.begin() + 12);
        if (!prefix_preserved)
            throw std::runtime_error(
                "target face sweep: MIDI prefix differs: " +
                midi_path);
        if (!output_root.empty()) {
            write_bundle_file(
                fs::path(output_root), midi_path, patched);
            bundle_records.push_back(
                {midi_path, "singer-face-midi",
                 entry.full_path, patched.size()});
        }
        report
            << cell(entry.full_path) << '\t'
            << cell(midi_path) << '\t'
            << animation.version << '\t'
            << animation.curves.size() << '\t'
            << animation.archive_footer.size() << '\t'
            << time_spans.size() << '\t'
            << tick_spans.size() << '\t'
            << midi.size() << '\t'
            << patched.size() << "\t1\tconverted\n";
        total_spans += tick_spans.size();
        ++paired_count;
        ++voc_count;
    }
    if (!output_root.empty())
        write_bundle_manifest(
            fs::path(output_root),
            "gh1-singer-face-bundle.tsv",
            std::move(bundle_records));
    std::printf(
        "target_face_sweep voc=%zu paired=%zu v1200=%zu "
        "v1500=%zu spans=%zu report=%s output=%s\n",
        voc_count, paired_count, version_1200, version_1500,
        total_spans, report_path.c_str(),
        output_root.empty() ? "<none>" : output_root.c_str());
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 6) usage();
    const std::string command = argv[1];
    if (command != "scan" && command != "target-summaries" &&
        command != "target-face-sweep")
        usage();
    const std::string hdr_path = argv[2];
    std::vector<std::string> ark_paths;
    std::string report_path;
    std::string bundle_path;
    std::string output_path;
    std::string target_rnd_objects;
    std::string target_midi_parsers;
    std::string target_char_objects;
    for (int i = 3; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--report") {
            if (++i >= argc) usage();
            report_path = argv[i];
        } else if (argument == "--bundle") {
            if (++i >= argc) usage();
            bundle_path = argv[i];
        } else if (argument == "--out") {
            if (++i >= argc) usage();
            output_path = argv[i];
        } else if (argument == "--target-rnd-objects") {
            if (++i >= argc) usage();
            target_rnd_objects = argv[i];
        } else if (argument == "--target-midi-parsers") {
            if (++i >= argc) usage();
            target_midi_parsers = argv[i];
        } else if (argument == "--target-char-objects") {
            if (++i >= argc) usage();
            target_char_objects = argv[i];
        } else if (argument.rfind("--", 0) == 0) {
            usage();
        } else {
            ark_paths.push_back(argument);
        }
    }
    if (ark_paths.empty() || report_path.empty()) usage();
    if (command == "scan") {
        if (!output_path.empty()) usage();
        const bool any_target =
            !target_rnd_objects.empty() ||
            !target_midi_parsers.empty() ||
            !target_char_objects.empty();
        if (bundle_path.empty() != !any_target ||
            (!bundle_path.empty() &&
             (target_rnd_objects.empty() ||
              target_midi_parsers.empty() ||
              target_char_objects.empty())))
            usage();
    } else {
        if (!bundle_path.empty() ||
            !target_rnd_objects.empty() ||
            !target_midi_parsers.empty() ||
            !target_char_objects.empty())
            usage();
        if (command != "target-face-sweep" &&
            !output_path.empty())
            usage();
    }

    try {
        const auto archive = gh::ark::ArkV3Reader::load(hdr_path);
        if (command == "target-summaries")
            return target_summaries(
                archive, ark_paths, report_path);
        if (command == "target-face-sweep")
            return target_face_sweep(
                archive, ark_paths, report_path,
                output_path);
        fs::create_directories(fs::path(report_path).parent_path());
        std::vector<BundleRecord> character_bundle_records;
        std::vector<BundleRecord> venue_bundle_records;
        std::set<std::string> generated_model_dependencies;
        {
            const auto read_virtual =
                [&](const std::string& path) {
                    const auto entry = archive.find(path);
                    if (!entry)
                        throw std::runtime_error(
                            "animation manifest dependency missing: " +
                            path);
                    return archive.read_entry(*entry, ark_paths);
                };
            gh::milo_convert::Gh1AnimationManifest
                animation_manifest;
            size_t acs_count = 0;
            for (const auto& entry : archive.entries()) {
                if (extension(entry.name) != ".acs") continue;
                auto manifest =
                    gh::milo_convert::
                        compile_gh1_animation_manifest(
                            entry.full_path,
                            archive.read_entry(entry, ark_paths),
                            read_virtual);
                animation_manifest.clip_sets.insert(
                    animation_manifest.clip_sets.end(),
                    manifest.clip_sets.begin(),
                    manifest.clip_sets.end());
                ++acs_count;
            }
            const auto character_manifest =
                gh::milo_convert::compile_gh1_character_manifest(
                    "charsys/gen/charsys.dtb",
                    read_virtual("charsys/gen/charsys.dtb"),
                    read_virtual);
            std::map<std::string, std::string>
                character_package_by_archetype;
            for (const auto& character :
                 character_manifest.characters) {
                if (!character_package_by_archetype.emplace(
                         character.compiled_skeleton,
                         character.package_name).second)
                    throw std::runtime_error(
                        "duplicate character archetype owner: " +
                        character.compiled_skeleton);
            }
            size_t guitarist_characters = 0;
            size_t band_characters = 0;
            size_t character_controllers = 0;
            for (const auto& character :
                 character_manifest.characters) {
                if (!archive.find(character.compiled_skeleton))
                    throw std::runtime_error(
                        "character manifest skeleton missing: " +
                        character.compiled_skeleton);
                if (character.band_character)
                    ++band_characters;
                else
                    ++guitarist_characters;
                character_controllers +=
                    character.controllers.size();
            }
            size_t animation_count = 0;
            size_t graph_count = 0;
            size_t graph_converted_clips = 0;
            size_t package_count = 0;
            size_t package_clip_count = 0;
            std::map<
                std::string,
                std::vector<
                    gh::milo_convert::
                        Gh2CharacterAnimationBinding>>
                character_animation_bindings;
            std::map<std::string, std::string> graph_owners;
            const std::string package_report_path =
                report_path + ".packages.tsv";
            std::ofstream package_output(
                package_report_path, std::ios::binary);
            if (!package_output)
                throw std::runtime_error(
                    "cannot create " + package_report_path);
            package_output
                << "clip_set\trole\tdirectory\tclips\tentries"
                   "\tpayload_bytes\tcontainer_bytes\n";
            const fs::path package_directory =
                fs::path(report_path).parent_path() /
                "gh1-character-packages";
            fs::create_directories(package_directory);
            const std::string skeleton_report =
                report_path + ".skeletons.tsv";
            std::ofstream skeleton_output(
                skeleton_report, std::ios::binary);
            if (!skeleton_output)
                throw std::runtime_error(
                    "cannot create " + skeleton_report);
            skeleton_output
                << "clip_set\tarchetype\tchannel_bases"
                   "\tzero_geometry_meshes\tresolved\tmissing\n";
            size_t skeleton_channel_bases = 0;
            size_t skeleton_resolved = 0;
            size_t skeleton_missing = 0;
            for (const auto& clip_set :
                 animation_manifest.clip_sets) {
                animation_count += clip_set.animations.size();
                const std::string archetype_path =
                    gh::milo_convert::gh1_compiled_rnd_path(
                        clip_set.archetype_rnd);
                const auto archetype_entry =
                    archive.find(archetype_path);
                if (!archetype_entry)
                    throw std::runtime_error(
                        "animation archetype missing: " +
                        archetype_path);
                const auto archetype_container =
                    gh::milo::parse_container(
                        archive.read_entry(
                            *archetype_entry, ark_paths));
                const auto archetype =
                    gh::milo::parse_directory(
                        gh::milo::container_payload(
                            archetype_container));
                gh::milo_convert::Gh1ClipSetBuildInput
                    package_input;
                package_input.spec = clip_set;
                package_input.archetype = archetype;
                package_input.clips.reserve(
                    clip_set.animations.size());
                for (const auto& animation :
                     clip_set.animations) {
                    const std::string acp_path =
                        clip_set.source_directory + "/gen/" +
                        animation.name + ".acp";
                    const auto acp_entry = archive.find(acp_path);
                    if (!acp_entry)
                        throw std::runtime_error(
                            "animation package ACP missing: " +
                            acp_path);
                    package_input.clips.push_back(
                        gh::acp::parse(
                            archive.read_entry(
                                *acp_entry, ark_paths)));
                }
                std::set<std::string> zero_geometry_meshes;
                for (const auto& object : archetype.entries) {
                    if (object.type != "Mesh") continue;
                    const auto mesh =
                        gh::milo_object::parse_mesh(
                            object.body_bytes);
                    if (mesh.vertices.empty() &&
                        mesh.faces.empty())
                        zero_geometry_meshes.insert(object.name);
                }
                std::set<std::string> channel_bases;
                for (const auto& animation :
                     clip_set.animations) {
                    const std::string acp_path =
                        clip_set.source_directory + "/gen/" +
                        animation.name + ".acp";
                    if (!archive.find(acp_path))
                        throw std::runtime_error(
                            "animation manifest ACP missing: " +
                            acp_path);
                    for (const auto& channel :
                         animation.channels) {
                        const size_t dot =
                            channel.rfind('.');
                        if (dot == std::string::npos)
                            throw std::runtime_error(
                                "animation channel lacks suffix: " +
                                channel);
                        channel_bases.insert(
                            channel.substr(0, dot));
                    }
                }
                std::vector<std::string> missing_bases;
                size_t resolved_bases = 0;
                for (const auto& base : channel_bases) {
                    if (zero_geometry_meshes.find(
                            base + ".mesh") !=
                        zero_geometry_meshes.end())
                        ++resolved_bases;
                    else
                        missing_bases.push_back(base);
                }
                skeleton_output
                    << cell(clip_set.qualified_name) << '\t'
                    << cell(archetype_path) << '\t'
                    << channel_bases.size() << '\t'
                    << zero_geometry_meshes.size() << '\t'
                    << resolved_bases << '\t';
                for (size_t index = 0;
                     index < missing_bases.size(); ++index) {
                    if (index) skeleton_output << ',';
                    skeleton_output << cell(missing_bases[index]);
                }
                skeleton_output << '\n';
                skeleton_channel_bases += channel_bases.size();
                skeleton_resolved += resolved_bases;
                skeleton_missing += missing_bases.size();
                if (const auto graph =
                        archive.find(clip_set.dependency_acg)) {
                    const auto parsed = gh::acg::parse(
                        archive.read_entry(*graph, ark_paths));
                    package_input.graph = parsed;
                    if (parsed.clips.size() !=
                        clip_set.animations.size())
                        throw std::runtime_error(
                            "animation graph/manifest clip count "
                            "differs: " +
                            clip_set.dependency_acg);
                    std::vector<std::string> clip_names;
                    clip_names.reserve(
                        clip_set.animations.size());
                    for (const auto& animation :
                         clip_set.animations)
                        clip_names.push_back(animation.name);
                    const auto transitions =
                        gh::milo_convert::
                            convert_gh1_acg_to_gh2_char_clip_transitions(
                                parsed, clip_names);
                    for (size_t index = 0;
                         index < clip_set.animations.size();
                         ++index) {
                        const std::string acp_path =
                            clip_set.source_directory + "/gen/" +
                            clip_set.animations[index].name +
                            ".acp";
                        const auto acp_entry =
                            archive.find(acp_path);
                        if (!acp_entry)
                            throw std::runtime_error(
                                "animation graph ACP missing: " +
                                acp_path);
                        const auto source_clip = gh::acp::parse(
                            archive.read_entry(
                                *acp_entry, ark_paths));
                        const auto target_clip =
                            gh::milo_convert::
                                convert_gh1_acp_to_gh2_char_clip_samples10(
                                    source_clip,
                                    transitions[index]);
                        const auto target_bytes =
                            gh::milo_object::
                                serialize_char_clip_samples10(
                                    target_clip);
                        if (gh::milo_object::
                                serialize_char_clip_samples10(
                                    gh::milo_object::
                                        parse_char_clip_samples10(
                                            target_bytes)) !=
                            target_bytes)
                            throw std::runtime_error(
                                "animation graph target clip "
                                "round trip differs: " +
                                acp_path);
                        (void)gh::milo_object::
                            char_clip_samples10_ps2_allocate_size(
                                target_clip);
                        ++graph_converted_clips;
                    }
                    graph_owners[clip_set.dependency_acg] =
                        clip_set.qualified_name;
                    ++graph_count;
                }
                const auto packages =
                    gh::milo_convert::
                        convert_gh1_clip_set_to_gh2_packages(
                            package_input);
                const auto repeat_packages =
                    gh::milo_convert::
                        convert_gh1_clip_set_to_gh2_packages(
                            package_input);
                if (repeat_packages.size() != packages.size())
                    throw std::runtime_error(
                        "animation package count is nondeterministic");
                for (size_t package_index = 0;
                     package_index < packages.size();
                     ++package_index) {
                    const auto& package =
                        packages[package_index];
                    const auto payload =
                        gh::milo::serialize_directory(
                            package.directory);
                    const auto container =
                        gh::milo::serialize_container(
                            gh::milo::make_container(payload));
                    const auto repeat_payload =
                        gh::milo::serialize_directory(
                            repeat_packages[package_index].directory);
                    const auto repeat_container =
                        gh::milo::serialize_container(
                            gh::milo::make_container(
                                repeat_payload));
                    if (payload != repeat_payload ||
                        container != repeat_container)
                        throw std::runtime_error(
                            "animation package bytes are "
                            "nondeterministic: " +
                            package.directory_name);
                    const fs::path output_path =
                        package_directory /
                        (package.directory_name + ".milo_ps2");
                    std::ofstream output(
                        output_path, std::ios::binary);
                    if (!output)
                        throw std::runtime_error(
                            "cannot create " +
                            output_path.string());
                    output.write(
                        reinterpret_cast<const char*>(
                            container.data()),
                        static_cast<std::streamsize>(
                            container.size()));
                    if (!output)
                        throw std::runtime_error(
                            "cannot write " +
                            output_path.string());
                    if (!bundle_path.empty()) {
                        const auto owner =
                            character_package_by_archetype.find(
                                archetype_path);
                        if (owner !=
                            character_package_by_archetype.end()) {
                            const std::string relative =
                                "char/" + owner->second +
                                "/anims/gen/" +
                                package.directory_name +
                                ".milo_ps2";
                        link_bundle_file(
                            fs::path(bundle_path),
                            relative, output_path);
                            character_bundle_records.push_back(
                                {relative, "character-animation",
                                 clip_set.qualified_name,
                                 container.size()});
                        } else if (
                            archetype_path.rfind(
                                "charsys/crowd/", 0) == 0) {
                            const std::string relative =
                                "char/crowd/anims/gen/" +
                                package.directory_name +
                                ".milo_ps2";
                            link_bundle_file(
                                fs::path(bundle_path),
                                relative, output_path);
                            venue_bundle_records.push_back(
                                {relative, "venue-crowd-animation",
                                 clip_set.qualified_name,
                                 container.size()});
                        }
                    }
                    character_animation_bindings[archetype_path]
                        .push_back(
                            {package.role, archetype_path,
                             "../../anims/" +
                                 package.directory_name +
                                 ".milo"});
                    package_output
                        << cell(clip_set.qualified_name) << '\t'
                        << gh::milo_convert::
                               gh2_clip_set_role_name(
                                   package.role)
                        << '\t'
                        << cell(package.directory_name) << '\t'
                        << package.source_clips.size() << '\t'
                        << package.directory.entries.size()
                        << '\t' << payload.size() << '\t'
                        << container.size() << '\n';
                    ++package_count;
                    package_clip_count +=
                        package.source_clips.size();
                }
            }
            const std::string model_report_path =
                report_path + ".models.tsv";
            std::ofstream model_output(
                model_report_path, std::ios::binary);
            if (!model_output)
                throw std::runtime_error(
                    "cannot create " + model_report_path);
            model_output
                << "character\tdirectory\troot_type\tentries"
                   "\tpayload_bytes\tcontainer_bytes"
                   "\tinternal_references\tface_transitions"
                   "\tface_controller_type\tcomplete"
                   "\tunresolved_dependencies"
                   "\tgenerated_dependencies\n";
            const fs::path model_directory =
                fs::path(report_path).parent_path() /
                "gh1-character-models";
            fs::create_directories(model_directory);
            size_t model_count = 0;
            size_t model_complete = 0;
            for (const auto& character :
                 character_manifest.characters) {
                const auto source_container =
                    gh::milo::parse_container(
                        read_virtual(
                            character.compiled_skeleton));
                const auto source_model =
                    gh::milo::parse_directory(
                        gh::milo::container_payload(
                            source_container));
                gh::milo_convert::Gh1CharacterModelBuildInput
                    model_input;
                model_input.spec = character;
                model_input.source_model = source_model;
                if (!character.shadow_file.empty()) {
                    const std::string shadow_path =
                        gh::milo_convert::
                            gh1_compiled_rnd_path(
                                character.shadow_file);
                    const auto shadow_container =
                        gh::milo::parse_container(
                            read_virtual(shadow_path));
                    model_input.shadow_model =
                        gh::milo::parse_directory(
                            gh::milo::container_payload(
                                shadow_container));
                }
                if (!character.face_file.empty()) {
                    const std::string face_path =
                        gh::milo_convert::
                            gh1_compiled_rnd_path(
                                character.face_file);
                    const auto face_container =
                        gh::milo::parse_container(
                            read_virtual(face_path));
                    model_input.face_model =
                        gh::milo::parse_directory(
                            gh::milo::container_payload(
                                face_container));
                }
                model_input.animations =
                    character_animation_bindings[
                        character.compiled_skeleton];
                const auto model =
                    gh::milo_convert::
                        convert_gh1_character_to_gh2_model_package(
                            model_input);
                const auto repeat_model =
                    gh::milo_convert::
                        convert_gh1_character_to_gh2_model_package(
                            model_input);
                const auto payload =
                    gh::milo::serialize_directory(
                        model.directory);
                const auto repeat_payload =
                    gh::milo::serialize_directory(
                        repeat_model.directory);
                const auto container =
                    gh::milo::serialize_container(
                        gh::milo::make_container(payload));
                const auto repeat_container =
                    gh::milo::serialize_container(
                        gh::milo::make_container(
                            repeat_payload));
                if (payload != repeat_payload ||
                    container != repeat_container)
                    throw std::runtime_error(
                        "character model bytes are nondeterministic: " +
                        character.package_name);
                const auto reparsed =
                    gh::milo::parse_directory(payload);
                if (!reparsed.boundaries_exact ||
                    gh::milo::serialize_directory(reparsed) !=
                        payload)
                    throw std::runtime_error(
                        "character model does not reparse exactly: " +
                        character.package_name);
                const fs::path output_path =
                    model_directory /
                    (model.directory_name + ".milo_ps2");
                std::ofstream output(
                    output_path, std::ios::binary);
                if (!output)
                    throw std::runtime_error(
                        "cannot create " +
                        output_path.string());
                output.write(
                    reinterpret_cast<const char*>(
                        container.data()),
                    static_cast<std::streamsize>(
                        container.size()));
                if (!output)
                    throw std::runtime_error(
                        "cannot write " +
                        output_path.string());
                if (!bundle_path.empty()) {
                    const std::string relative =
                        "char/" + character.package_name +
                        "/og/gen/" +
                        model.directory_name +
                        ".milo_ps2";
                    link_bundle_file(
                        fs::path(bundle_path),
                        relative, output_path);
                    character_bundle_records.push_back(
                        {relative, "character-model",
                         character.compiled_skeleton,
                         container.size()});
                }
                generated_model_dependencies.insert(
                    model.generated_dependencies.begin(),
                    model.generated_dependencies.end());
                model_output
                    << cell(character.authored_name) << '\t'
                    << cell(model.directory_name) << '\t'
                    << cell(model.directory.dir_type) << '\t'
                    << model.directory.entries.size() << '\t'
                    << payload.size() << '\t'
                    << container.size() << '\t'
                    << model.internal_reference_count << '\t'
                    << model.face_transition_count << '\t'
                    << cell(model.face_controller_type) << '\t'
                    << (model.complete ? 1 : 0) << '\t';
                for (size_t index = 0;
                     index <
                     model.unresolved_dependencies.size();
                     ++index) {
                    if (index) model_output << ',';
                    model_output << cell(
                        model.unresolved_dependencies[index]);
                }
                model_output << '\t';
                for (size_t index = 0;
                     index <
                     model.generated_dependencies.size();
                     ++index) {
                    if (index) model_output << ',';
                    model_output << cell(
                        model.generated_dependencies[index]);
                }
                model_output << '\n';
                ++model_count;
                if (model.complete) ++model_complete;
            }
            if (!bundle_path.empty()) {
                const std::set<std::string> expected_dependencies = {
                    "face-control-config:gh1_guitarist_morph_face",
                    "face-control-config:gh1_singer_morph_face",
                };
                if (generated_model_dependencies !=
                    expected_dependencies)
                    throw std::runtime_error(
                        "generated character dependency set differs");
                const auto clean_rnd =
                    gh::milo::read_file(target_rnd_objects);
                const auto clean_midi =
                    gh::milo::read_file(target_midi_parsers);
                const auto clean_char =
                    gh::milo::read_file(target_char_objects);
                const auto rnd_patch =
                    gh::milo_convert::
                        patch_gh2_rnd_objects_for_gh1_faces(
                            clean_rnd);
                const auto midi_patch =
                    gh::milo_convert::
                        patch_gh2_midi_parsers_for_gh1_singer_face(
                            clean_midi);
                const auto char_patch =
                    gh::milo_convert::
                        patch_gh2_char_objects_for_gh1_singer_face(
                            clean_char);
                if (rnd_patch.types_added != 2 ||
                    midi_patch.parsers_added != 1 ||
                    char_patch.handlers_added != 2 ||
                    gh::milo_convert::
                        patch_gh2_rnd_objects_for_gh1_faces(
                            clean_rnd).bytes != rnd_patch.bytes ||
                    gh::milo_convert::
                        patch_gh2_midi_parsers_for_gh1_singer_face(
                            clean_midi).bytes != midi_patch.bytes ||
                    gh::milo_convert::
                        patch_gh2_char_objects_for_gh1_singer_face(
                            clean_char).bytes != char_patch.bytes)
                    throw std::runtime_error(
                        "character config bundle is incomplete or "
                        "nondeterministic");
                const auto append_config =
                    [&](const std::string& relative,
                        const std::string& source,
                        const std::vector<uint8_t>& bytes) {
                        write_bundle_file(
                            fs::path(bundle_path),
                            relative, bytes);
                        character_bundle_records.push_back(
                            {relative, "character-config",
                             source, bytes.size()});
                    };
                append_config(
                    "system/run/milo/gen/rnd_objects.dtb",
                    target_rnd_objects, rnd_patch.bytes);
                append_config(
                    "config/gen/midi_parsers.dtb",
                    target_midi_parsers, midi_patch.bytes);
                append_config(
                    "char/gen/char_objects.dtb",
                    target_char_objects, char_patch.bytes);
                write_bundle_manifest(
                    fs::path(bundle_path),
                    "gh1-character-bundle.tsv",
                    character_bundle_records);
            }
            if (acs_count != 0) {
                const std::string animation_report =
                    report_path + ".animations.tsv";
                std::ofstream animation_output(
                    animation_report, std::ios::binary);
                if (!animation_output)
                    throw std::runtime_error(
                        "cannot create " + animation_report);
                animation_output <<
                    gh::milo_convert::gh1_animation_manifest_tsv(
                        animation_manifest);
                const std::string character_report =
                    report_path + ".characters.tsv";
                std::ofstream character_output(
                    character_report, std::ios::binary);
                if (!character_output)
                    throw std::runtime_error(
                        "cannot create " + character_report);
                character_output <<
                    gh::milo_convert::gh1_character_manifest_tsv(
                        character_manifest);
                std::printf(
                    "animation_manifest acs=%zu sets=%zu "
                    "animations=%zu graphs=%zu graph_clips=%zu "
                    "report=%s\n",
                    acs_count, animation_manifest.clip_sets.size(),
                    animation_count, graph_count,
                    graph_converted_clips,
                    animation_report.c_str());
                std::printf(
                    "character_manifest characters=%zu guitarists=%zu "
                    "band=%zu controllers=%zu report=%s\n",
                    character_manifest.characters.size(),
                    guitarist_characters, band_characters,
                    character_controllers,
                    character_report.c_str());
                std::printf(
                    "character_model_inventory packages=%zu "
                    "complete=%zu incomplete=%zu report=%s "
                    "output=%s bundle=%s\n",
                    model_count, model_complete,
                    model_count - model_complete,
                    model_report_path.c_str(),
                    model_directory.string().c_str(),
                    bundle_path.empty()
                        ? "<none>" : bundle_path.c_str());

                const std::string graph_report =
                    report_path + ".graphs.tsv";
                std::ofstream graph_output(
                    graph_report, std::ios::binary);
                if (!graph_output)
                    throw std::runtime_error(
                        "cannot create " + graph_report);
                std::vector<
                    std::pair<std::string, std::string>>
                    dtb_strings;
                for (const auto& entry : archive.entries()) {
                    if (extension(entry.name) != ".dtb") continue;
                    const auto tree = gh::dtb::parse(
                        archive.read_entry(entry, ark_paths));
                    std::vector<std::string> values;
                    for (const auto& root : tree.root)
                        collect_dtb_strings(root, values);
                    for (auto& value : values)
                        dtb_strings.emplace_back(
                            entry.full_path, std::move(value));
                }
                graph_output
                    << "archive_path\tclips\tnodes\tmanifest_owner"
                       "\tfirst_target\tfirst_current_beat"
                       "\tfirst_next_beat\tdtb_references\n";
                size_t graph_assets = 0;
                size_t graph_nodes = 0;
                for (const auto& entry : archive.entries()) {
                    if (extension(entry.name) != ".acg") continue;
                    const auto graph = gh::acg::parse(
                        archive.read_entry(entry, ark_paths));
                    size_t nodes = 0;
                    for (const auto& clip : graph.clips)
                        nodes += clip.nodes.size();
                    graph_output << cell(entry.full_path) << '\t'
                                 << graph.clips.size() << '\t'
                                 << nodes << '\t';
                    if (const auto owner =
                            graph_owners.find(entry.full_path);
                        owner != graph_owners.end())
                        graph_output << cell(owner->second);
                    graph_output << '\t';
                    if (!graph.clips.empty() &&
                        !graph.clips.front().nodes.empty()) {
                        const auto& first =
                            graph.clips.front().nodes.front();
                        graph_output << first.target_clip_index;
                        graph_output << '\t' << first.current_beat;
                        graph_output << '\t' << first.next_beat;
                    } else {
                        graph_output << "\t\t";
                    }
                    graph_output << '\t';
                    const std::string graph_path =
                        lower(entry.full_path);
                    const std::string graph_name =
                        lower(entry.name);
                    bool first_reference = true;
                    for (const auto& [dtb_path, value] :
                         dtb_strings) {
                        const std::string normalized =
                            lower(value);
                        if (normalized != graph_path &&
                            normalized != graph_name)
                            continue;
                        if (!first_reference)
                            graph_output << ',';
                        graph_output
                            << cell(dtb_path + "::" + value);
                        first_reference = false;
                    }
                    graph_output << '\n';
                    ++graph_assets;
                    graph_nodes += nodes;
                }
                std::printf(
                    "animation_graph_inventory assets=%zu nodes=%zu "
                    "report=%s\n",
                    graph_assets, graph_nodes,
                    graph_report.c_str());
                std::printf(
                    "animation_skeleton_inventory channel_bases=%zu "
                    "resolved=%zu missing=%zu report=%s\n",
                    skeleton_channel_bases, skeleton_resolved,
                    skeleton_missing, skeleton_report.c_str());
                std::printf(
                    "animation_package_inventory packages=%zu "
                    "clips=%zu report=%s output=%s\n",
                    package_count, package_clip_count,
                    package_report_path.c_str(),
                    package_directory.string().c_str());
            }
        }
        std::ofstream report(report_path, std::ios::binary);
        if (!report)
            throw std::runtime_error("cannot create " + report_path);
        const std::string acp_report_path =
            report_path + ".acp.tsv";
        std::ofstream acp_report(acp_report_path, std::ios::binary);
        if (!acp_report)
            throw std::runtime_error("cannot create " + acp_report_path);
        const std::string venue_report_path =
            report_path + ".venues.tsv";
        std::ofstream venue_report(
            venue_report_path, std::ios::binary);
        if (!venue_report)
            throw std::runtime_error(
                "cannot create " + venue_report_path);
        venue_report
            << "archive_path\tentries\tinternal_references"
               "\tpayload_bytes\tcontainer_bytes\tstatus\n";
        const std::string venue_script_report_path =
            report_path + ".venue-scripts.tsv";
        std::ofstream venue_script_report(
            venue_script_report_path, std::ios::binary);
        if (!venue_script_report)
            throw std::runtime_error(
                "cannot create " + venue_script_report_path);
        venue_script_report
            << "source_path\ttarget_path\tfunctions\thandlers"
               "\tfunction_calls\tforeach_loops\tswitch_anim"
               "\tswitch_anim_rt\tanim_task\tanimate_to"
               "\tdelay_task\trandom_ranges\ttarget_bytes\tstatus"
               "\tstateful_ranges\tdetail\n";
        const std::string venue_camera_report_path =
            report_path + ".venue-cameras.tsv";
        std::ofstream venue_camera_report(
            venue_camera_report_path, std::ios::binary);
        if (!venue_camera_report)
            throw std::runtime_error(
                "cannot create " + venue_camera_report_path);
        venue_camera_report
            << "source_path\ttarget_path\trecords\tkeyframes"
               "\tshaky_records\tadaptive_subdivisions"
               "\tmax_position_error"
               "\tmax_rotation_error\tmax_screen_error"
               "\tmax_fov_error\tstatus\n";
        acp_report
            << "archive_path\tobject\tset0_channels\tset0_samples"
               "\tset0_compression\tset0_frame_bytes"
               "\tset0_sample_bytes\tset0_facing_channels"
               "\tset1_channels\tset1_samples\tset1_compression"
               "\tset1_frame_bytes\tset1_sample_bytes"
               "\tset1_facing_channels\toverlap_channels\n";
        report
            << "archive_path\tsource_type\tsource_name\ttarget_type"
               "\ttarget_name\tstatus\tdetail\n";

        size_t assets = 0;
        size_t complete_assets = 0;
        size_t acp_assets = 0;
        size_t complete_acp_assets = 0;
        size_t converted_objects = 0;
        size_t synthesized_objects = 0;
        size_t blocked_objects = 0;
        size_t emitted_assets = 0;
        size_t semantic_objects = 0;
        size_t acp_set1_nonconstant = 0;
        size_t acp_overlapping_channels = 0;
        size_t acp_facing_in_set1 = 0;
        size_t venue_assets = 0;
        size_t venue_references = 0;
        size_t venue_scripts = 0;
        size_t venue_script_bytes = 0;
        size_t venue_script_blocked = 0;
        size_t venue_camera_records = 0;
        size_t venue_camera_keyframes = 0;
        size_t venue_camera_blocked = 0;
        std::map<std::string, gh::milo::Directory>
            venue_directories;
        std::map<std::string, std::string>
            venue_directory_sources;
        std::map<std::string, gh::milo_object::TransAnim6>
            shared_camera_animations;
        std::map<std::string, size_t> blockers;
        for (const auto& entry : archive.entries()) {
            if (extension(entry.name) == ".acp") {
                ++acp_assets;
                try {
                    const auto bytes =
                        archive.read_entry(entry, ark_paths);
                    const auto source = gh::acp::parse(bytes);
                    const auto facing_count =
                        [](const auto& channels) {
                            size_t count = 0;
                            for (const auto& channel : channels) {
                                if (channel.rfind(
                                        "bone_facing.", 0) == 0 ||
                                    channel.rfind(
                                        "bone_facing_delta.", 0) == 0)
                                    ++count;
                            }
                            return count;
                        };
                    std::set<std::string> set0_channels(
                        source.channel_sets[0].channels.begin(),
                        source.channel_sets[0].channels.end());
                    size_t overlap = 0;
                    for (const auto& channel :
                         source.channel_sets[1].channels) {
                        if (set0_channels.find(channel) !=
                            set0_channels.end())
                            ++overlap;
                    }
                    const size_t set0_facing =
                        facing_count(
                            source.channel_sets[0].channels);
                    const size_t set1_facing =
                        facing_count(
                            source.channel_sets[1].channels);
                    acp_report
                        << cell(entry.full_path) << '\t'
                        << cell(source.object_name) << '\t'
                        << source.channel_sets[0].channels.size()
                        << '\t'
                        << source.channel_sets[0].sample_count
                        << '\t'
                        << source.channel_sets[0].compression
                        << '\t'
                        << source.channel_sets[0].frame_size
                        << '\t'
                        << source.channel_sets[0].sample_bytes.size()
                        << '\t' << set0_facing << '\t'
                        << source.channel_sets[1].channels.size()
                        << '\t'
                        << source.channel_sets[1].sample_count
                        << '\t'
                        << source.channel_sets[1].compression
                        << '\t'
                        << source.channel_sets[1].frame_size
                        << '\t'
                        << source.channel_sets[1].sample_bytes.size()
                        << '\t' << set1_facing << '\t'
                        << overlap << '\n';
                    if (!source.channel_sets[1].channels.empty() &&
                        source.channel_sets[1].sample_count != 1)
                        ++acp_set1_nonconstant;
                    if (overlap != 0)
                        ++acp_overlapping_channels;
                    if (set1_facing != 0)
                        ++acp_facing_in_set1;
                    const auto target =
                        gh::milo_convert::
                            convert_gh1_acp_to_gh2_char_clip_samples10(
                                source);
                    const auto target_bytes =
                        gh::milo_object::
                            serialize_char_clip_samples10(target);
                    const auto verify =
                        gh::milo_object::
                            parse_char_clip_samples10(target_bytes);
                    if (gh::milo_object::
                            serialize_char_clip_samples10(verify) !=
                        target_bytes)
                        throw std::runtime_error(
                            "target CharClipSamples round trip differs");
                    const auto repeat =
                        gh::milo_convert::
                            convert_gh1_acp_to_gh2_char_clip_samples10(
                                source);
                    if (gh::milo_object::
                            serialize_char_clip_samples10(repeat) !=
                        target_bytes)
                        throw std::runtime_error(
                            "target CharClipSamples conversion is "
                            "nondeterministic");
                    report << cell(entry.full_path) << "\tACP\t"
                           << cell(source.object_name)
                           << "\tCharClipSamples\t"
                           << cell(source.object_name)
                           << "\tconverted\trevision-18/5 ACP mapped to "
                              "native revision-10 CharClipSamples\n";
                    ++complete_acp_assets;
                    ++converted_objects;
                    ++semantic_objects;
                } catch (const std::exception& ex) {
                    ++blocked_objects;
                    ++blockers["ACP"];
                    report << cell(entry.full_path)
                           << "\tACP\t\tCharClipSamples\t\tblocked\t"
                           << cell(ex.what()) << '\n';
                }
                continue;
            }
            if (extension(entry.name) != ".rnd_ps2") continue;
            ++assets;
            try {
                const auto bytes = archive.read_entry(entry, ark_paths);
                const auto container = gh::milo::parse_container(bytes);
                const auto source = gh::milo::parse_directory(
                    gh::milo::container_payload(container));
                std::string target_directory_name = stem(entry.name);
                if (const auto venue_path =
                        gh1_venue_target_path(entry.full_path);
                    venue_path && venue_path->primary) {
                    target_directory_name = venue_path->target_venue;
                }
                const auto result =
                    gh::milo_convert::
                        convert_gh1_directory_to_gh2_rnddir(
                            source, target_directory_name);
                for (const auto& row : result.manifest) {
                    report << cell(entry.full_path) << '\t'
                           << cell(row.source_type) << '\t'
                           << cell(row.source_name) << '\t'
                           << cell(row.target_type) << '\t'
                           << cell(row.target_name) << '\t'
                           << cell(row.status) << '\t'
                           << cell(row.detail) << '\n';
                    if (row.status == "converted")
                        ++converted_objects;
                    else if (row.status == "synthesized")
                        ++synthesized_objects;
                    else if (row.status == "blocked") {
                        ++blocked_objects;
                        ++blockers[row.source_type];
                    }
                }
                if (!result.complete) continue;

                const auto target_payload =
                    gh::milo::serialize_directory(result.directory);
                const auto target_container =
                    gh::milo::make_container(target_payload);
                const auto target_bytes =
                    gh::milo::serialize_container(target_container);
                const auto verify_container =
                    gh::milo::parse_container(target_bytes);
                if (gh::milo::serialize_container(verify_container) !=
                    target_bytes)
                    throw std::runtime_error(
                        "target MILO container round trip differs");
                const auto verify_directory =
                    gh::milo::parse_directory(
                        gh::milo::container_payload(verify_container));
                if (!verify_directory.boundaries_exact ||
                    gh::milo::serialize_directory(verify_directory) !=
                        target_payload)
                    throw std::runtime_error(
                        "target GH2 directory round trip differs");
                if (gh::milo_object::serialize_rnd_dir8(
                        gh::milo_object::parse_rnd_dir8(
                            verify_directory.dir_body_bytes)) !=
                    verify_directory.dir_body_bytes)
                    throw std::runtime_error(
                        "target GH2 RndDir root round trip differs");
                for (const auto& target : verify_directory.entries) {
                    if (gh::milo_object::round_trip_gh2_object_body(
                            target.type, target.body_bytes,
                            verify_directory.dir_version) !=
                        target.body_bytes)
                        throw std::runtime_error(
                            "target GH2 object round trip differs: " +
                            target.type + " " + target.name);
                    ++semantic_objects;
                    if (target.type == "TransAnim" &&
                        target.name == "shaky_cam1.tnm") {
                        const auto animation =
                            gh::milo_object::parse_trans_anim6(
                                target.body_bytes);
                        const auto [found, inserted] =
                            shared_camera_animations.emplace(
                                target.name, animation);
                        if (!inserted &&
                            gh::milo_object::serialize_trans_anim6(
                                found->second) !=
                                target.body_bytes) {
                            throw std::runtime_error(
                                "conflicting shared camera animation: " +
                                target.name);
                        }
                    }
                }
                if (entry.full_path.rfind("venues/", 0) == 0) {
                    const auto target_path =
                        gh1_venue_target_path(entry.full_path);
                    if (!target_path)
                        throw std::runtime_error(
                            "invalid GH1 venue bundle path: " +
                            entry.full_path);
                    if (!venue_directories.emplace(
                             target_path->target_path,
                             verify_directory).second)
                        throw std::runtime_error(
                            "duplicate target venue directory: " +
                            target_path->target_path);
                    venue_directory_sources.emplace(
                        target_path->target_path,
                        entry.full_path);
                }

                const auto repeat =
                    gh::milo_convert::
                        convert_gh1_directory_to_gh2_rnddir(
                            source, target_directory_name);
                const auto repeat_payload =
                    gh::milo::serialize_directory(repeat.directory);
                const auto repeat_bytes =
                    gh::milo::serialize_container(
                        gh::milo::make_container(repeat_payload));
                if (repeat_payload != target_payload ||
                    repeat_bytes != target_bytes ||
                    gh::milo_convert::manifest_tsv(repeat) !=
                        gh::milo_convert::manifest_tsv(result))
                    throw std::runtime_error(
                        "target GH2 conversion is nondeterministic");
                ++emitted_assets;
                ++complete_assets;
            } catch (const std::exception& ex) {
                ++blocked_objects;
                ++blockers["<directory>"];
                report << cell(entry.full_path)
                       << "\t\t\t\t\tblocked\t"
                       << cell(ex.what()) << '\n';
            }
        }
        const std::set<std::string> venue_support_extensions = {
            ".bnk", ".dtb", ".nse", ".seq",
        };
        for (const auto& entry : archive.entries()) {
            if (entry.full_path.rfind("venues/", 0) != 0 ||
                venue_support_extensions.find(
                    extension(entry.name)) ==
                    venue_support_extensions.end())
                continue;
            const auto target_info =
                gh1_venue_target_path(entry.full_path);
            if (!target_info)
                throw std::runtime_error(
                    "invalid GH1 venue support path: " +
                    entry.full_path);
            if (lower(entry.name) == "camera.dtb") {
                const std::string main_path =
                    "world/" + target_info->target_venue + "/gen/" +
                    target_info->target_venue + ".milo_ps2";
                const std::string campaths_path =
                    "world/" + target_info->target_venue +
                    "/gen/campaths.milo_ps2";
                try {
                    const auto main = venue_directories.find(main_path);
                    const auto campaths =
                        venue_directories.find(campaths_path);
                    if (main == venue_directories.end())
                        throw std::runtime_error(
                            "converted main venue directory is missing");
                    if (campaths == venue_directories.end())
                        throw std::runtime_error(
                            "converted campaths directory is missing");
                    const auto camera_bytes =
                        archive.read_entry(entry, ark_paths);
                    const auto converted =
                        gh::milo_convert::
                            convert_gh1_venue_cameras_to_gh2_camshots(
                                camera_bytes, main->second,
                                campaths->second,
                                shared_camera_animations);
                    const auto repeat =
                        gh::milo_convert::
                            convert_gh1_venue_cameras_to_gh2_camshots(
                                camera_bytes, main->second,
                                campaths->second,
                                shared_camera_animations);
                    if (gh::milo::serialize_directory(
                            converted.main_directory) !=
                            gh::milo::serialize_directory(
                                repeat.main_directory) ||
                        converted.records != repeat.records ||
                        converted.keyframes != repeat.keyframes ||
                        converted.shaky_records !=
                            repeat.shaky_records ||
                        converted.adaptive_subdivisions !=
                            repeat.adaptive_subdivisions)
                        throw std::runtime_error(
                            "native venue camera conversion is "
                            "nondeterministic");
                    venue_directories[main_path] =
                        converted.main_directory;
                    venue_camera_records += converted.records;
                    venue_camera_keyframes += converted.keyframes;
                    synthesized_objects += converted.records;
                    semantic_objects += converted.records;
                    venue_camera_report
                        << cell(entry.full_path) << '\t'
                        << cell(main_path) << '\t'
                        << converted.records << '\t'
                        << converted.keyframes << '\t'
                        << converted.shaky_records << '\t'
                        << converted.adaptive_subdivisions << '\t'
                        << converted
                               .maximum_position_linearization_error
                        << '\t'
                        << converted
                               .maximum_rotation_linearization_error
                        << '\t'
                        << converted
                               .maximum_screen_linearization_error
                        << '\t'
                        << converted
                               .maximum_fov_linearization_error
                        << "\tconverted\n";
                } catch (const std::exception& ex) {
                    ++venue_camera_blocked;
                    ++blocked_objects;
                    ++blockers["<venue-camera>"];
                    venue_camera_report
                        << cell(entry.full_path) << '\t'
                        << cell(main_path)
                        << "\t0\t0\t0\t0\t0\t0\t0\t0\tblocked:"
                        << cell(ex.what()) << '\n';
                }
                continue;
            }
            if (target_info->primary &&
                extension(entry.name) == ".dtb") {
                const std::string target_path =
                    target_info->target_path;
                try {
                    const auto bytes =
                        archive.read_entry(entry, ark_paths);
                    const auto converted =
                        gh::milo_convert::
                            convert_gh1_venue_script_to_gh2_worlddir(
                                bytes, target_info->target_venue);
                    const auto repeat =
                        gh::milo_convert::
                            convert_gh1_venue_script_to_gh2_worlddir(
                                bytes, target_info->target_venue);
                    if (repeat.bytes != converted.bytes ||
                        repeat.dta != converted.dta)
                        throw std::runtime_error(
                            "native venue script conversion is "
                            "nondeterministic");
                    venue_script_report
                        << cell(entry.full_path) << '\t'
                        << cell(target_path) << '\t'
                        << converted.source_functions << '\t'
                        << converted.handlers << '\t'
                        << converted.function_calls_inlined << '\t'
                        << converted.foreach_loops_unrolled << '\t'
                        << converted.switch_anim_calls << '\t'
                        << converted.switch_anim_rt_calls << '\t'
                        << converted.anim_task_calls << '\t'
                        << converted.animate_to_calls << '\t'
                        << converted.delay_task_calls << '\t'
                        << converted.random_ranges_expanded << '\t'
                        << converted.bytes.size()
                        << "\tconverted\t"
                        << converted.stateful_ranges_resolved
                        << "\tGH1 Arena wrappers lowered to "
                           "native GH2 WorldDir script\n";
                    ++venue_scripts;
                    venue_script_bytes += converted.bytes.size();
                    if (!bundle_path.empty()) {
                        write_bundle_file(
                            fs::path(bundle_path),
                            target_path, converted.bytes);
                        venue_bundle_records.push_back(
                            {target_path, "venue-native-script",
                             entry.full_path, converted.bytes.size()});
                    }
                } catch (const std::exception& ex) {
                    ++venue_script_blocked;
                    ++blockers["<venue-script>"];
                    venue_script_report
                        << cell(entry.full_path) << "\t\t0\t0\t0\t0"
                           "\t0\t0\t0\t0\t0\t0\t0\tblocked\t0\t"
                        << cell(ex.what()) << '\n';
                }
                continue;
            }
            if (!bundle_path.empty()) {
                const auto bytes =
                    archive.read_entry(entry, ark_paths);
                write_bundle_file(
                    fs::path(bundle_path),
                    target_info->target_path, bytes);
                venue_bundle_records.push_back(
                    {target_info->target_path, "venue-support",
                     entry.full_path, bytes.size()});
            }
        }
        for (const auto& [target_path, directory] :
             venue_directories) {
            const auto payload =
                gh::milo::serialize_directory(directory);
            const auto bytes = gh::milo::serialize_container(
                gh::milo::make_container(payload));
            const auto verify_container =
                gh::milo::parse_container(bytes);
            const auto verify_directory =
                gh::milo::parse_directory(
                    gh::milo::container_payload(verify_container));
            if (!verify_directory.boundaries_exact ||
                gh::milo::serialize_directory(verify_directory) !=
                    payload ||
                gh::milo::serialize_container(verify_container) !=
                    bytes)
                throw std::runtime_error(
                    "final native venue directory round trip differs: " +
                    target_path);
            for (const auto& object : verify_directory.entries) {
                if (gh::milo_object::round_trip_gh2_object_body(
                        object.type, object.body_bytes,
                        verify_directory.dir_version) !=
                    object.body_bytes)
                    throw std::runtime_error(
                        "final native venue object round trip differs: " +
                        target_path + "::" + object.name);
            }
            const size_t references =
                gh::milo_convert::
                    validate_gh2_directory_references(
                        verify_directory);
            const std::string& source_path =
                venue_directory_sources.at(target_path);
            venue_report
                << cell(source_path) << '\t'
                << verify_directory.entries.size() << '\t'
                << references << '\t'
                << payload.size() << '\t'
                << bytes.size()
                << "\tconverted\n";
            ++venue_assets;
            venue_references += references;
            if (!bundle_path.empty()) {
                write_bundle_file(
                    fs::path(bundle_path), target_path, bytes);
                venue_bundle_records.push_back(
                    {target_path, "venue-rnd", source_path,
                     bytes.size()});
            }
        }
        if (!bundle_path.empty()) {
            write_bundle_manifest(
                fs::path(bundle_path),
                "gh1-venue-bundle.tsv",
                venue_bundle_records);
        }
        std::printf(
            "assets=%zu complete=%zu incomplete=%zu acp=%zu "
            "acp_complete=%zu acp_incomplete=%zu converted=%zu "
            "synthesized=%zu blocked=%zu emitted=%zu semantic=%zu\n",
            assets, complete_assets, assets - complete_assets,
            acp_assets, complete_acp_assets,
            acp_assets - complete_acp_assets,
            converted_objects, synthesized_objects, blocked_objects,
            emitted_assets, semantic_objects);
        std::printf(
            "venue_conversion assets=%zu references=%zu "
            "report=%s bundle=%s\n",
            venue_assets, venue_references,
            venue_report_path.c_str(),
            bundle_path.empty()
                ? "<none>" : bundle_path.c_str());
        std::printf(
            "venue_scripts converted=%zu blocked=%zu bytes=%zu "
            "report=%s\n",
            venue_scripts, venue_script_blocked, venue_script_bytes,
            venue_script_report_path.c_str());
        std::printf(
            "venue_cameras records=%zu keyframes=%zu blocked=%zu "
            "report=%s\n",
            venue_camera_records, venue_camera_keyframes,
            venue_camera_blocked,
            venue_camera_report_path.c_str());
        std::printf(
            "acp_channel_inventory set1_nonconstant=%zu overlap=%zu "
            "facing_in_set1=%zu report=%s\n",
            acp_set1_nonconstant, acp_overlapping_channels,
            acp_facing_in_set1, acp_report_path.c_str());
        for (const auto& blocker : blockers)
            std::printf(
                "blocked %-16s %zu\n", blocker.first.c_str(),
                blocker.second);
        std::printf("report: %s\n", report_path.c_str());
        return 0;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "milo_convert_audit: %s\n", ex.what());
        return 2;
    }
}
