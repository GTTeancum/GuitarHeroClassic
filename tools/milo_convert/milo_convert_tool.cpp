#include "milo_convert.h"

#include "gh2_face_config_patch.h"
#include "milo.h"
#include "milo_object.h"
#include "singer_face_track.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace {

void write_file(const fs::path& path, const std::vector<uint8_t>& bytes) {
    if (!path.parent_path().empty())
        fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("cannot create " + path.string());
    output.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    if (!output) throw std::runtime_error("cannot write " + path.string());
}

void write_text(const fs::path& path, const std::string& text) {
    if (!path.parent_path().empty())
        fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("cannot create " + path.string());
    output << text;
    if (!output) throw std::runtime_error("cannot write " + path.string());
}

void usage() {
    std::cerr
        << "Usage:\n"
        << "  milo_convert_tool convert <GH1.rnd_ps2> --name <dir-name> "
           "--out <GH2.milo_ps2> --manifest <manifest.tsv>\n"
        << "  milo_convert_tool inspect-clipset <GH2.milo_ps2> "
           "[--channels]\n"
        << "  milo_convert_tool inspect-character <GH2.milo_ps2> "
           "[--controllers]\n"
        << "  milo_convert_tool inspect-groups <GH2.milo_ps2>\n"
        << "  milo_convert_tool inspect-skeleton <GH1.rnd_ps2> [--all]\n"
        << "  milo_convert_tool patch-face-config <rnd_objects.dtb> "
           "--out <patched.dtb> [--dta <patched.dta>]\n"
        << "  milo_convert_tool patch-face-midi-config "
           "<midi_parsers.dtb> --out <patched.dtb> "
           "[--dta <patched.dta>]\n"
        << "  milo_convert_tool patch-face-character-config "
           "<char_objects.dtb> --out <patched.dtb> "
           "[--dta <patched.dta>]\n"
        << "  milo_convert_tool translate-singer-face <song.mid> "
           "--out <patched.mid> [--voc <song.voc>]\n";
    std::exit(2);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) usage();
    const std::string command = argv[1];
    fs::path input = argv[2];
    if (command == "patch-face-config") {
        try {
            if (argc != 5 && argc != 7)
                usage();
            if (std::string(argv[3]) != "--out")
                usage();
            if (argc == 7 &&
                std::string(argv[5]) != "--dta")
                usage();
            const auto patch =
                gh::milo_convert::
                    patch_gh2_rnd_objects_for_gh1_faces(
                        gh::milo::read_file(input.string()));
            write_file(argv[4], patch.bytes);
            if (argc == 7)
                write_text(argv[6], patch.dta);
            std::cout
                << "types_added=" << patch.types_added
                << " bytes=" << patch.bytes.size()
                << " output=" << argv[4] << '\n';
            return 0;
        } catch (const std::exception& ex) {
            std::cerr
                << "milo_convert_tool: " << ex.what() << "\n";
            return 2;
        }
    }
    if (command == "patch-face-midi-config") {
        try {
            if (argc != 5 && argc != 7)
                usage();
            if (std::string(argv[3]) != "--out")
                usage();
            if (argc == 7 &&
                std::string(argv[5]) != "--dta")
                usage();
            const auto patch =
                gh::milo_convert::
                    patch_gh2_midi_parsers_for_gh1_singer_face(
                        gh::milo::read_file(input.string()));
            write_file(argv[4], patch.bytes);
            if (argc == 7)
                write_text(argv[6], patch.dta);
            std::cout
                << "parsers_added=" << patch.parsers_added
                << " bytes=" << patch.bytes.size()
                << " output=" << argv[4] << '\n';
            return 0;
        } catch (const std::exception& ex) {
            std::cerr
                << "milo_convert_tool: " << ex.what() << "\n";
            return 2;
        }
    }
    if (command == "patch-face-character-config") {
        try {
            if (argc != 5 && argc != 7)
                usage();
            if (std::string(argv[3]) != "--out")
                usage();
            if (argc == 7 &&
                std::string(argv[5]) != "--dta")
                usage();
            const auto patch =
                gh::milo_convert::
                    patch_gh2_char_objects_for_gh1_singer_face(
                        gh::milo::read_file(input.string()));
            write_file(argv[4], patch.bytes);
            if (argc == 7)
                write_text(argv[6], patch.dta);
            std::cout
                << "handlers_added=" << patch.handlers_added
                << " bytes=" << patch.bytes.size()
                << " output=" << argv[4] << '\n';
            return 0;
        } catch (const std::exception& ex) {
            std::cerr
                << "milo_convert_tool: " << ex.what() << "\n";
            return 2;
        }
    }
    if (command == "translate-singer-face") {
        fs::path output;
        fs::path voc;
        for (int index = 3; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--out" && index + 1 < argc)
                output = argv[++index];
            else if (argument == "--voc" && index + 1 < argc)
                voc = argv[++index];
            else
                usage();
        }
        if (output.empty()) usage();
        try {
            const auto midi =
                gh::milo::read_file(input.string());
            std::vector<
                gh::milo_convert::SingerFaceTickSpan> spans;
            std::string mode;
            if (voc.empty()) {
                spans =
                    gh::milo_convert::
                        extract_gh1_singer_face_spans(midi);
                mode = "gh1-pitch108";
            } else {
                const auto animation =
                    gh::milo_convert::
                        parse_gh2_facefx_animation(
                            gh::milo::read_file(voc.string()));
                spans =
                    gh::milo_convert::
                        map_singer_face_times_to_midi(
                            midi,
                            gh::milo_convert::
                                derive_gh1_singer_open_spans(
                                    animation));
                mode = "gh2-facefx";
            }
            const auto patched =
                gh::milo_convert::
                    append_gh1_singer_face_track(midi, spans);
            write_file(output, patched);
            std::cout
                << "mode=" << mode
                << " spans=" << spans.size()
                << " source_bytes=" << midi.size()
                << " output_bytes=" << patched.size()
                << " output=" << output.string() << '\n';
            return 0;
        } catch (const std::exception& ex) {
            std::cerr
                << "milo_convert_tool: " << ex.what() << "\n";
            return 2;
        }
    }
    if (command == "inspect-character") {
        const bool print_controllers =
            argc == 4 && std::string(argv[3]) == "--controllers";
        if (argc > 4 || (argc == 4 && !print_controllers))
            usage();
        try {
            const auto container = gh::milo::parse_container(
                gh::milo::read_file(input.string()));
            const auto directory = gh::milo::parse_directory(
                gh::milo::container_payload(container));
            gh::milo_object::Character9 character;
            if (directory.dir_type == "BandCharacter") {
                character =
                    gh::milo_object::parse_band_character1(
                        directory.dir_body_bytes)
                        .character;
            } else if (directory.dir_type == "Character") {
                character = gh::milo_object::parse_character9(
                    directory.dir_body_bytes);
            } else {
                throw std::runtime_error(
                    "directory is not a Character or BandCharacter");
            }
            std::cout << "type=" << directory.dir_type
                      << " name=" << directory.dir_name
                      << " object_type="
                      << character.render_directory.object_directory
                             .object_fields.type
                      << " lods=" << character.lods.size()
                      << " shadow=" << character.shadow
                      << " self_shadow=" << character.self_shadow
                      << " sphere_base=" << character.sphere_base
                      << '\n';
            for (size_t index = 0;
                 index < character.lods.size(); ++index) {
                std::cout << "lod[" << index << "] screen_size="
                          << character.lods[index].screen_size
                          << " group="
                          << character.lods[index].group << '\n';
            }
            if (print_controllers) {
                for (const auto& entry : directory.entries) {
                    if (entry.type == "CharDriver") {
                        const auto driver =
                            gh::milo_object::parse_char_driver3(
                                entry.body_bytes);
                        std::cout << entry.type << ' ' << entry.name
                                  << " type=" << driver.object_fields.type
                                  << " bones=" << driver.bones
                                  << " clips=" << driver.clips
                                  << " realign=" << driver.realign
                                  << '\n';
                    } else if (entry.type == "CharDriverMidi") {
                        const auto driver =
                            gh::milo_object::parse_char_driver_midi3(
                                entry.body_bytes);
                        std::cout
                            << entry.type << ' ' << entry.name
                            << " type="
                            << driver.driver.object_fields.type
                            << " bones=" << driver.driver.bones
                            << " clips=" << driver.driver.clips
                            << " default=" << driver.default_clip
                            << '\n';
                    } else if (entry.type == "CharIKRod") {
                        const auto rod =
                            gh::milo_object::parse_char_ik_rod2(
                                entry.body_bytes);
                        std::cout << entry.type << ' ' << entry.name
                                  << " type="
                                  << rod.object_fields.type
                                  << " left=" << rod.left_end
                                  << " right=" << rod.right_end
                                  << " position=" << rod.dest_pos
                                  << " side=" << rod.side_axis
                                  << " vertical=" << rod.vertical
                                  << " dest=" << rod.dest
                                  << " transform=";
                        for (size_t index = 0;
                             index < rod.transform.size();
                             ++index) {
                            if (index) std::cout << ',';
                            std::cout << rod.transform[index];
                        }
                        std::cout << '\n';
                    } else if (
                        entry.type == "CharPosConstraint") {
                        const auto constraint =
                            gh::milo_object::
                                parse_char_pos_constraint2(
                                    entry.body_bytes);
                        std::cout
                            << entry.type << ' ' << entry.name
                            << " type="
                            << constraint.object_fields.type
                            << " source=" << constraint.source
                            << " targets=";
                        for (const auto& target :
                             constraint.targets)
                            std::cout << target << ',';
                        std::cout << '\n';
                    } else if (entry.type == "CharServoBone") {
                        const auto servo =
                            gh::milo_object::
                                parse_char_servo_bone2(
                                    entry.body_bytes);
                        std::cout << entry.type << ' ' << entry.name
                                  << " rev=" << servo.revision
                                  << " type="
                                  << servo.object_fields.type
                                  << " clip_type="
                                  << servo.clip_type << '\n';
                    } else if (entry.type == "CharForeTwist") {
                        const auto twist =
                            gh::milo_object::
                                parse_char_fore_twist4(
                                    entry.body_bytes);
                        std::cout << entry.type << ' ' << entry.name
                                  << " type="
                                  << twist.object_fields.type
                                  << " offset=" << twist.offset
                                  << " hand=" << twist.hand
                                  << " twist2=" << twist.twist2
                                  << '\n';
                    } else if (entry.type == "CharUpperTwist") {
                        const auto twist =
                            gh::milo_object::
                                parse_char_upper_twist1(
                                    entry.body_bytes);
                        std::cout << entry.type << ' ' << entry.name
                                  << " type="
                                  << twist.object_fields.type
                                  << " upper=" << twist.upper_arm
                                  << " twist1=" << twist.twist1
                                  << " twist2=" << twist.twist2
                                  << '\n';
                    } else if (entry.type == "CharIKHand") {
                        const auto hand =
                            gh::milo_object::parse_char_ik_hand2(
                                entry.body_bytes);
                        std::cout << entry.type << ' ' << entry.name
                                  << " type="
                                  << hand.object_fields.type
                                  << " hand=" << hand.hand
                                  << " target=" << hand.target
                                  << " orientation="
                                  << hand.orientation
                                  << " stretch=" << hand.stretch
                                  << " scalable=" << hand.scalable
                                  << '\n';
                    } else if (entry.type == "CharIKMidi") {
                        const auto midi =
                            gh::milo_object::parse_char_ik_midi4(
                                entry.body_bytes);
                        std::cout << entry.type << ' ' << entry.name
                                  << " type="
                                  << midi.object_fields.type
                                  << " bone=" << midi.bone << '\n';
                    } else if (
                        entry.type == "CharWeightSetter") {
                        const auto setter =
                            gh::milo_object::
                                parse_char_weight_setter2(
                                    entry.body_bytes);
                        std::cout
                            << entry.type << ' ' << entry.name
                            << " type=" << setter.object_fields.type
                            << " weight=" << setter.weightable.weight
                            << " owner="
                            << setter.weightable.weight_owner
                            << " driver=" << setter.driver
                            << " flags=" << setter.flags << '\n';
                    } else if (entry.type == "CharLookAt") {
                        const auto look =
                            gh::milo_object::parse_char_look_at2(
                                entry.body_bytes);
                        std::cout << entry.type << ' ' << entry.name
                                  << " type="
                                  << look.object_fields.type
                                  << " source=" << look.source
                                  << " pivot=" << look.pivot
                                  << " target=" << look.target
                                  << " half_time=" << look.half_time
                                  << " yaw=" << look.min_yaw << ','
                                  << look.max_yaw
                                  << " pitch=" << look.min_pitch << ','
                                  << look.max_pitch << '\n';
                    } else if (entry.type == "CharEyes") {
                        const auto eyes =
                            gh::milo_object::parse_char_eyes3(
                                entry.body_bytes);
                        std::cout << entry.type << ' ' << entry.name
                                  << " type="
                                  << eyes.object_fields.type
                                  << " eyes=";
                        for (const auto& eye : eyes.eyes)
                            std::cout << eye << ',';
                        std::cout << " legacy="
                                  << eyes.legacy_transform << '\n';
                    } else if (entry.type == "CharWalk") {
                        const auto walk =
                            gh::milo_object::parse_char_walk1(
                                entry.body_bytes);
                        std::cout << entry.type << ' ' << entry.name
                                  << " type="
                                  << walk.object_fields.type << '\n';
                    } else if (
                        entry.type == "FaceFxLipSyncServo") {
                        const auto servo =
                            gh::milo_object::
                                parse_facefx_lip_sync_servo5(
                                    entry.body_bytes);
                        std::cout
                            << entry.type << ' ' << entry.name
                            << " type=" << servo.object_fields.type
                            << " weight=" << servo.weightable.weight
                            << " facefx=" << servo.facefx_path
                            << " visemes=" << servo.viseme_milo
                            << " targets=";
                        for (const auto& target : servo.targets)
                            std::cout
                                << target.object << ':'
                                << target.property_type << ':'
                                << target.property << ',';
                        std::cout << '\n';
                    } else if (entry.type == "OutfitLoader") {
                        const auto loader =
                            gh::milo_object::parse_outfit_loader1(
                                entry.body_bytes);
                        std::cout
                            << entry.type << ' ' << entry.name
                            << " type=" << loader.object_fields.type
                            << " directory=" << loader.directory
                            << " categories=" << loader.categories.size();
                        for (size_t category_index = 0;
                             category_index < loader.categories.size();
                             ++category_index) {
                            const auto& category =
                                loader.categories[category_index];
                            std::cout
                                << " category[" << category_index
                                << "]=" << static_cast<int>(
                                    category.selected)
                                << ',' << static_cast<int>(
                                    category.shown)
                                << ',' << category.outfits.size()
                                << ':';
                            for (const auto& outfit :
                                 category.outfits)
                                std::cout
                                    << static_cast<int>(outfit.hide)
                                    << static_cast<int>(outfit.desire)
                                    << static_cast<int>(outfit.exclude)
                                    << ',';
                        }
                        std::cout << '\n';
                    } else if (entry.type == "AnimFilter") {
                        const auto filter =
                            gh::milo_object::parse_anim_filter1(
                                entry.body_bytes);
                        std::cout
                            << entry.type << ' ' << entry.name
                            << " type=" << filter.object_fields.type
                            << " anim=" << filter.anim
                            << " scale=" << filter.scale
                            << " offset=" << filter.offset
                            << " start=" << filter.start
                            << " end=" << filter.end
                            << " filter_type=" << filter.type
                            << " period=" << filter.period << '\n';
                    } else if (entry.type == "EventTrigger") {
                        const auto trigger =
                            gh::milo_object::parse_event_trigger8(
                                entry.body_bytes);
                        std::cout
                            << entry.type << ' ' << entry.name
                            << " type=" << trigger.object_fields.type
                            << " event=" << trigger.trigger_event
                            << " animations=";
                        for (const auto& animation :
                             trigger.animations)
                            std::cout
                                << animation.animation << ':'
                                << animation.blend << ':'
                                << animation.wait << ':'
                                << animation.delay << ',';
                        std::cout << " sounds=";
                        for (const auto& sound : trigger.sounds)
                            std::cout << sound << ',';
                        std::cout << " shows=";
                        for (const auto& show : trigger.shows)
                            std::cout << show << ',';
                        std::cout << " hides=";
                        for (const auto& hide : trigger.legacy_hides)
                            std::cout << hide << ',';
                        std::cout << " enable=";
                        for (const auto& event :
                             trigger.enable_events)
                            std::cout << event << ',';
                        std::cout << " disable=";
                        for (const auto& event :
                             trigger.disable_events)
                            std::cout << event << ',';
                        std::cout << " wait_for=";
                        for (const auto& event :
                             trigger.wait_for_events)
                            std::cout << event << ',';
                        std::cout << " next=" << trigger.next_link
                                  << " proxies=";
                        for (const auto& proxy :
                             trigger.proxy_calls)
                            std::cout << proxy.proxy << ':'
                                      << proxy.call << ',';
                        std::cout << '\n';
                    } else if (entry.type == "Morph") {
                        const auto morph =
                            gh::milo_object::parse_morph4(
                                entry.body_bytes);
                        std::cout
                            << entry.type << ' ' << entry.name
                            << " target=" << morph.target
                            << " normals=" << morph.normals
                            << " spline=" << morph.spline
                            << " intensity=" << morph.intensity
                            << " poses=";
                        for (const auto& pose : morph.poses) {
                            std::cout << pose.mesh << '[';
                            for (const auto& key : pose.keys)
                                std::cout << key.frame << ':'
                                          << key.value << ',';
                            std::cout << "],";
                        }
                        std::cout << '\n';
                    }
                }
            }
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "milo_convert_tool: " << ex.what() << "\n";
            return 2;
        }
    }
    if (command == "inspect-skeleton") {
        try {
            const bool print_all =
                argc == 4 && std::string(argv[3]) == "--all";
            if (argc > 4 || (argc == 4 && !print_all))
                usage();
            const auto container = gh::milo::parse_container(
                gh::milo::read_file(input.string()));
            const auto directory = gh::milo::parse_directory(
                gh::milo::container_payload(container));
            const auto converted =
                gh::milo_convert::
                    convert_gh1_directory_to_gh2_rnddir(
                        directory, "skeleton_inspect");
            std::map<std::string, gh::milo_object::Mesh28>
                converted_meshes;
            for (const auto& entry : converted.directory.entries) {
                if (entry.type == "Mesh")
                    converted_meshes.emplace(
                        entry.name,
                        gh::milo_object::parse_mesh28(
                            entry.body_bytes));
                if (print_all && entry.type == "Group") {
                    const auto group =
                        gh::milo_object::parse_group12(
                            entry.body_bytes);
                    std::cout << "Group " << entry.name
                              << "\tobjects=";
                    for (size_t index = 0;
                         index < group.objects.size();
                         ++index) {
                        if (index) std::cout << ',';
                        std::cout << group.objects[index];
                    }
                    std::cout << '\n';
                }
                if (print_all && entry.type == "Morph") {
                    const auto morph =
                        gh::milo_object::parse_morph4(
                            entry.body_bytes);
                    std::cout << "Morph " << entry.name
                              << "\ttarget=" << morph.target
                              << "\tnormals=" << morph.normals
                              << "\tspline=" << morph.spline
                              << "\tintensity=" << morph.intensity
                              << "\tposes=";
                    for (const auto& pose : morph.poses) {
                        std::cout << pose.mesh << '[';
                        for (const auto& key : pose.keys)
                            std::cout << key.frame << ':'
                                      << key.value << ',';
                        std::cout << "],";
                    }
                    std::cout << '\n';
                }
            }
            for (const auto& entry : directory.entries) {
                if (entry.type != "Mesh") continue;
                const auto mesh =
                    gh::milo_object::parse_mesh(entry.body_bytes);
                if (!print_all &&
                    (!mesh.vertices.empty() || !mesh.faces.empty()))
                    continue;
                const auto effective =
                    converted_meshes.find(entry.name);
                if (effective == converted_meshes.end())
                    throw std::runtime_error(
                        "converted skeleton mesh missing");
                std::cout << entry.name
                          << "\tparent="
                          << effective->second.transformable.parent
                          << "\tconstraint="
                          << effective->second.transformable.constraint
                          << "\ttranslation="
                          << effective->second.transformable.local[9]
                          << ','
                          << effective->second.transformable.local[10]
                          << ','
                          << effective->second.transformable.local[11]
                          << "\tvertices=" << mesh.vertices.size()
                          << "\tfaces=" << mesh.faces.size()
                          << "\tmaterial="
                          << effective->second.material
                          << "\tgeometry_owner="
                          << effective->second.geometry_owner;
                if (print_all) {
                    std::cout << "\tlocal=";
                    for (size_t index = 0;
                         index < effective->second.transformable.local.size();
                         ++index) {
                        if (index) std::cout << ',';
                        std::cout <<
                            effective->second.transformable.local[index];
                    }
                }
                std::cout << '\n';
            }
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "milo_convert_tool: " << ex.what() << "\n";
            return 2;
        }
    }
    if (command == "inspect-groups") {
        try {
            if (argc != 3) usage();
            const auto container = gh::milo::parse_container(
                gh::milo::read_file(input.string()));
            const auto directory = gh::milo::parse_directory(
                gh::milo::container_payload(container));
            for (const auto& entry : directory.entries) {
                if (entry.type != "Group") continue;
                const auto group =
                    gh::milo_object::parse_group12(entry.body_bytes);
                std::cout << entry.name << "\tobjects=";
                for (size_t index = 0; index < group.objects.size();
                     ++index) {
                    if (index) std::cout << ',';
                    std::cout << group.objects[index];
                }
                std::cout << "\tenvironment=" << group.environment
                          << "\tlod=" << group.lod << '\n';
            }
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "milo_convert_tool: " << ex.what() << "\n";
            return 2;
        }
    }
    if (command == "inspect-clipset") {
        const bool print_channels =
            argc == 4 && std::string(argv[3]) == "--channels";
        if (argc > 4 ||
            (argc == 4 && !print_channels))
            usage();
        try {
            const auto container = gh::milo::parse_container(
                gh::milo::read_file(input.string()));
            const auto directory = gh::milo::parse_directory(
                gh::milo::container_payload(container));
            if (directory.dir_type != "CharClipSet")
                throw std::runtime_error(
                    "directory is not a CharClipSet");
            size_t clip_count = 0;
            for (const auto& entry : directory.entries)
                if (entry.type == "CharClipSamples") ++clip_count;
            const auto root =
                gh::milo_object::parse_char_clip_set14(
                    directory.dir_body_bytes,
                    static_cast<uint32_t>(clip_count));
            std::cout
                << "name=" << directory.dir_name
                << " clips=" << clip_count
                << " entries=" << directory.entries.size()
                << " object_type=" << root.object_directory.object_fields.type
                << " viewports=" << root.object_directory.viewports.size()
                << " current_viewport="
                << root.object_directory.current_viewport
                << " proxy=" << root.object_directory.proxy_path
                << " subdirs="
                << root.object_directory.subdirectories.size()
                << " blend_width=" << root.blend_width
                << " play_flags=" << root.play_flags
                << " move_self=" << root.move_self
                << " recenter_targets="
                << root.recenter_targets.size()
                << " recenter_average="
                << root.recenter_average.size()
                << " recenter_slide=" << root.recenter_slide
                << " legacy_type=" << root.legacy_type
                << " legacy_type_version="
                << root.legacy_type_version
                << '\n';
            for (const auto& value : root.recenter_targets)
                std::cout << "recenter_target\t" << value << '\n';
            for (const auto& value : root.recenter_average)
                std::cout << "recenter_average\t" << value << '\n';
            for (size_t index = 0;
                 index < root.object_directory.viewports.size();
                 ++index) {
                const auto& viewport =
                    root.object_directory.viewports[index];
                std::cout << "viewport\t" << index;
                for (float value : viewport.transform)
                    std::cout << '\t' << value;
                std::cout << '\t' << viewport.legacy_value << '\n';
            }
            for (const auto& entry : directory.entries) {
                if (entry.type != "CharClipSamples")
                    std::cout << "object\t" << entry.type << '\t'
                              << entry.name << '\n';
                if (entry.type == "CharClipGroup") {
                    const auto group =
                        gh::milo_object::parse_char_clip_group1(
                            entry.body_bytes);
                    std::cout << "group\t" << entry.name
                              << "\twhich=" << group.which
                              << "\tclips=";
                    for (size_t index = 0;
                         index < group.clips.size(); ++index) {
                        if (index) std::cout << ',';
                        std::cout << group.clips[index];
                    }
                    std::cout << '\n';
                } else if (entry.type == "CharBone") {
                    const auto bone =
                        gh::milo_object::parse_char_bone2(
                            entry.body_bytes);
                    std::cout
                        << "bone\t" << entry.name
                        << "\tparent=" << bone.legacy_transform.parent
                        << "\tposition=" << bone.position_context
                        << "\tscale=" << bone.scale_context
                        << "\trotation=" << bone.rotation
                        << "\tlegacy_rotation="
                        << bone.legacy_rotation << '\n';
                }
            }
            for (const auto& clip : root.clips) {
                const gh::milo::Entry* entry = nullptr;
                for (const auto& candidate : directory.entries) {
                    if (candidate.type == "CharClipSamples" &&
                        candidate.name == clip.clip) {
                        entry = &candidate;
                        break;
                    }
                }
                if (!entry)
                    throw std::runtime_error(
                        "clip summary has no CharClipSamples body: " +
                        clip.clip);
                const auto body =
                    gh::milo_object::parse_char_clip_samples10(
                        entry->body_bytes);
                std::cout << "clip\t" << clip.clip << '\t'
                          << clip.flags << '\t' << clip.size_bytes
                          << "\tbody_flags=" << body.flags
                          << "\tbody_bytes=" << entry->body_bytes.size()
                          << "\tsample_bytes="
                          << body.full.sample_bytes.size() +
                                 body.one.sample_bytes.size() +
                                 body.duplicate.sample_bytes.size()
                          << '\n';
                if (print_channels) {
                    for (const auto& channel : body.full.channels)
                        std::cout << "channel\t" << clip.clip
                                  << "\tfull\t" << channel << '\n';
                    for (const auto& channel : body.one.channels)
                        std::cout << "channel\t" << clip.clip
                                  << "\tone\t" << channel << '\n';
                }
            }
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "milo_convert_tool: " << ex.what() << "\n";
            return 2;
        }
    }
    if (command != "convert") usage();
    fs::path output;
    fs::path manifest;
    std::string name;
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--name" && i + 1 < argc) name = argv[++i];
        else if (arg == "--out" && i + 1 < argc) output = argv[++i];
        else if (arg == "--manifest" && i + 1 < argc)
            manifest = argv[++i];
        else usage();
    }
    if (output.empty() || manifest.empty() || name.empty()) usage();

    try {
        const auto source_bytes = gh::milo::read_file(input.string());
        const auto source_container =
            gh::milo::parse_container(source_bytes);
        const auto source_payload =
            gh::milo::container_payload(source_container);
        const auto source_directory =
            gh::milo::parse_directory(source_payload);
        const auto result =
            gh::milo_convert::convert_gh1_directory_to_gh2_rnddir(
                source_directory, name);
        write_text(manifest, gh::milo_convert::manifest_tsv(result));
        if (!result.complete) {
            std::cerr
                << "conversion blocked; manifest written to "
                << manifest.string() << "\n";
            return 1;
        }

        const auto target_payload =
            gh::milo::serialize_directory(result.directory);
        const auto target_container =
            gh::milo::make_container(target_payload);
        const auto target_bytes =
            gh::milo::serialize_container(target_container);
        const auto verify_container =
            gh::milo::parse_container(target_bytes);
        const auto verify_directory = gh::milo::parse_directory(
            gh::milo::container_payload(verify_container));
        if (!verify_directory.boundaries_exact ||
            gh::milo::serialize_directory(verify_directory) !=
                target_payload)
            throw std::runtime_error(
                "converted directory failed native GH2 round trip");
        write_file(output, target_bytes);
        std::cout << "converted " << source_directory.entries.size()
                  << " source objects into "
                  << result.directory.entries.size()
                  << " GH2 objects\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "milo_convert_tool: " << ex.what() << "\n";
        return 2;
    }
}
