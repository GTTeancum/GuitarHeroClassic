#include "milo_object.h"

#include <cmath>
#include <cstdio>

int main() {
    gh::milo_object::Morph morph;
    morph.revision = 3;
    morph.animatable.revision = 0;
    morph.animatable.operations.push_back({0, 1.0f, 9.0f});
    morph.animatable.objects.push_back("face.mesh");
    gh::milo_object::MorphPose pose;
    pose.mesh = "open.mesh";
    pose.keys.push_back({-1.0f, 0.0f});
    pose.keys.push_back({1.0f, 10.0f});
    morph.poses.push_back(pose);
    morph.target = "face.mesh";
    morph.normals = true;
    morph.spline = false;
    morph.intensity = 0.75f;
    try {
        const auto bytes = gh::milo_object::serialize_morph(morph);
        const auto parsed = gh::milo_object::parse_morph(bytes);
        if (gh::milo_object::serialize_morph(parsed) != bytes ||
            parsed.poses.size() != 1 ||
            parsed.poses[0].keys.size() != 2 ||
            parsed.poses[0].mesh != "open.mesh" ||
            parsed.target != "face.mesh") {
            std::fprintf(stderr, "milo_object_test: Morph mismatch\n");
            return 1;
        }
        auto morph_without_graph = parsed;
        morph_without_graph.animatable.operations.clear();
        morph_without_graph.animatable.objects.clear();
        const auto converted_morph =
            gh::milo_object::convert_morph3_to_morph4(
                morph_without_graph);
        const auto morph4_bytes =
            gh::milo_object::serialize_morph4(converted_morph);
        const auto parsed_morph4 =
            gh::milo_object::parse_morph4(morph4_bytes);
        if (gh::milo_object::serialize_morph4(parsed_morph4) !=
                morph4_bytes ||
            parsed_morph4.revision != 4 ||
            parsed_morph4.object_fields.revision != 0 ||
            parsed_morph4.animatable.revision != 4 ||
            parsed_morph4.poses.size() != 1 ||
            parsed_morph4.poses[0].keys.size() != 2 ||
            parsed_morph4.target != morph.target ||
            parsed_morph4.normals != morph.normals ||
            parsed_morph4.intensity != morph.intensity) {
            std::fprintf(
                stderr, "milo_object_test: Morph3->Morph4 mismatch\n");
            return 1;
        }

        gh::milo_object::TransAnim trans;
        trans.revision = 4;
        trans.animatable.revision = 0;
        trans.drawable.revision = 1;
        trans.drawable.showing = true;
        trans.drawable.objects = {"track.view"};
        trans.drawable.sphere = {1.0f, 2.0f, 3.0f, 4.0f};
        trans.target = "track.trans";
        trans.rotation_keys.push_back({{0, 0, 0, 1}, 0.0f});
        trans.translation_keys.push_back({{1, 2, 3}, 0.0f});
        trans.keys_owner = "owner.tnm";
        trans.translation_spline = true;
        trans.scale_keys.push_back({{1, 1, 1}, 0.0f});
        trans.rotation_slerp = true;
        const auto trans_bytes =
            gh::milo_object::serialize_trans_anim(trans);
        const auto parsed_trans =
            gh::milo_object::parse_trans_anim(trans_bytes);
        if (gh::milo_object::serialize_trans_anim(parsed_trans) !=
                trans_bytes ||
            parsed_trans.target != "track.trans" ||
            parsed_trans.translation_keys.size() != 1) {
            std::fprintf(stderr,
                         "milo_object_test: TransAnim mismatch\n");
            return 1;
        }
        const auto converted_trans =
            gh::milo_object::convert_trans_anim4_to_trans_anim6(
                parsed_trans);
        const auto trans6_bytes =
            gh::milo_object::serialize_trans_anim6(converted_trans);
        const auto parsed_trans6 =
            gh::milo_object::parse_trans_anim6(trans6_bytes);
        if (gh::milo_object::serialize_trans_anim6(parsed_trans6) !=
                trans6_bytes ||
            parsed_trans6.revision != 6 ||
            parsed_trans6.animatable.revision != 4 ||
            parsed_trans6.target != "track.trans" ||
            parsed_trans6.rotation_keys.size() != 1 ||
            parsed_trans6.translation_keys.size() != 1 ||
            parsed_trans6.scale_keys.size() != 1 ||
            parsed_trans6.keys_owner != "owner.tnm" ||
            !parsed_trans6.translation_spline ||
            !parsed_trans6.rotation_slerp) {
            std::fprintf(
                stderr,
                "milo_object_test: TransAnim4->TransAnim6 mismatch\n");
            return 1;
        }

        gh::milo_object::CamShot20 cam_shot;
        cam_shot.object_fields.type = "gh1_camera";
        cam_shot.object_fields.has_type_properties = true;
        gh::milo_object::TypePropertyNode hide_key;
        hide_key.type = 0x05;
        hide_key.symbol = "hide_crowd";
        cam_shot.object_fields.type_properties.push_back(hide_key);
        gh::milo_object::TypePropertyNode hide_value;
        hide_value.type = 0x00;
        hide_value.integer = 1;
        cam_shot.object_fields.type_properties.push_back(hide_value);
        cam_shot.animatable.rate = 1;
        gh::milo_object::CamShotFrame20 cam_frame;
        cam_frame.duration = 300.0f;
        cam_frame.field_of_view = 0.7853982f;
        cam_frame.screen_offset = {-0.25f, 0.2f};
        cam_frame.targets.push_back(
            {0, "singer", "bone_spine1.mesh"});
        cam_frame.parent = {0, "arena", "venue.view"};
        cam_frame.use_parent_rotation = true;
        cam_frame.shake_noise_amplitude = 0.1f;
        cam_shot.keyframes.push_back(cam_frame);
        cam_shot.path = "Cam_intro_gh1.tnm";
        cam_shot.category = "INTRO";
        cam_shot.use_depth_of_field = true;
        cam_shot.legacy_crowd_pairs.push_back({2, 4});
        cam_shot.hide_list.push_back("crowd01.mm");
        const auto cam_shot_bytes =
            gh::milo_object::serialize_cam_shot20(cam_shot);
        const auto parsed_cam_shot =
            gh::milo_object::parse_cam_shot20(cam_shot_bytes);
        if (gh::milo_object::serialize_cam_shot20(parsed_cam_shot) !=
                cam_shot_bytes ||
            parsed_cam_shot.keyframes.size() != 1 ||
            parsed_cam_shot.keyframes[0].targets.size() != 1 ||
            parsed_cam_shot.keyframes[0].parent.object != "arena" ||
            parsed_cam_shot.path != "Cam_intro_gh1.tnm" ||
            parsed_cam_shot.category != "INTRO" ||
            parsed_cam_shot.object_fields.type_properties.size() != 2 ||
            parsed_cam_shot.object_fields.type_properties[0].symbol !=
                "hide_crowd" ||
            parsed_cam_shot.object_fields.type_properties[1].integer != 1 ||
            parsed_cam_shot.legacy_crowd_pairs.size() != 1) {
            std::fprintf(
                stderr, "milo_object_test: CamShot20 mismatch\n");
            return 1;
        }

        gh::milo_object::MultiMesh multi;
        multi.revision = 0;
        multi.drawable.revision = 1;
        multi.mesh = "crowd.mesh";
        multi.transforms.push_back(
            {1, 0, 0, 0, 1, 0, 0, 0, 1, 10, 20, 30});
        const auto multi_bytes =
            gh::milo_object::serialize_multi_mesh(multi);
        const auto parsed_multi =
            gh::milo_object::parse_multi_mesh(multi_bytes);
        if (gh::milo_object::serialize_multi_mesh(parsed_multi) !=
                multi_bytes ||
            parsed_multi.transforms.size() != 1 ||
            parsed_multi.mesh != "crowd.mesh") {
            std::fprintf(stderr,
                         "milo_object_test: MultiMesh mismatch\n");
            return 1;
        }
        const auto converted_multi =
            gh::milo_object::convert_multi_mesh0_to_multi_mesh1(
                parsed_multi);
        const auto multi1_bytes =
            gh::milo_object::serialize_multi_mesh1(converted_multi);
        const auto parsed_multi1 =
            gh::milo_object::parse_multi_mesh1(multi1_bytes);
        if (gh::milo_object::serialize_multi_mesh1(parsed_multi1) !=
                multi1_bytes ||
            parsed_multi1.revision != 1 ||
            parsed_multi1.object_fields.revision != 0 ||
            parsed_multi1.drawable.revision != 3 ||
            parsed_multi1.transforms != parsed_multi.transforms ||
            parsed_multi1.mesh != "crowd.mesh") {
            std::fprintf(
                stderr,
                "milo_object_test: MultiMesh0->MultiMesh1 mismatch\n");
            return 1;
        }

        gh::milo_object::MeshAnim mesh_anim;
        mesh_anim.revision = 0;
        mesh_anim.animatable.revision = 0;
        mesh_anim.mesh = "cloth.mesh";
        mesh_anim.point_keys.push_back(
            {{{{1, 2, 3}}, {{4, 5, 6}}}, 10.0f});
        mesh_anim.texcoord_keys.push_back(
            {{{{0, 0}}, {{1, 1}}}, 10.0f});
        mesh_anim.color_keys.push_back(
            {{{{0, 0, 0, 1}}, {{1, 1, 1, 1}}}, 10.0f});
        mesh_anim.keys_owner = "cloth.mnm";
        const auto mesh_anim_bytes =
            gh::milo_object::serialize_mesh_anim(mesh_anim);
        const auto parsed_mesh_anim =
            gh::milo_object::parse_mesh_anim(mesh_anim_bytes);
        if (gh::milo_object::serialize_mesh_anim(parsed_mesh_anim) !=
                mesh_anim_bytes ||
            parsed_mesh_anim.point_keys.size() != 1 ||
            parsed_mesh_anim.color_keys[0].values.size() != 2) {
            std::fprintf(stderr,
                         "milo_object_test: MeshAnim mismatch\n");
            return 1;
        }
        const auto converted_mesh_anim =
            gh::milo_object::convert_mesh_anim0_to_mesh_anim1(
                parsed_mesh_anim);
        const auto mesh_anim1_bytes =
            gh::milo_object::serialize_mesh_anim1(
                converted_mesh_anim);
        const auto parsed_mesh_anim1 =
            gh::milo_object::parse_mesh_anim1(mesh_anim1_bytes);
        if (gh::milo_object::serialize_mesh_anim1(
                parsed_mesh_anim1) != mesh_anim1_bytes ||
            parsed_mesh_anim1.revision != 1 ||
            parsed_mesh_anim1.animatable.revision != 4 ||
            parsed_mesh_anim1.mesh != "cloth.mesh" ||
            parsed_mesh_anim1.point_keys.size() != 1 ||
            parsed_mesh_anim1.color_keys[0].values.size() != 2 ||
            parsed_mesh_anim1.keys_owner != "cloth.mnm") {
            std::fprintf(
                stderr,
                "milo_object_test: MeshAnim0->MeshAnim1 mismatch\n");
            return 1;
        }

        gh::milo_object::CamAnim cam_anim;
        cam_anim.revision = 0;
        cam_anim.animatable.revision = 0;
        cam_anim.camera = "intro.cam";
        cam_anim.fov_keys.push_back({0.75f, 30.0f});
        cam_anim.keys_owner = "intro.cnm";
        const auto cam_anim_bytes =
            gh::milo_object::serialize_cam_anim(cam_anim);
        const auto parsed_cam_anim =
            gh::milo_object::parse_cam_anim(cam_anim_bytes);
        if (gh::milo_object::serialize_cam_anim(parsed_cam_anim) !=
                cam_anim_bytes ||
            parsed_cam_anim.fov_keys.size() != 1 ||
            parsed_cam_anim.camera != "intro.cam") {
            std::fprintf(stderr,
                         "milo_object_test: CamAnim mismatch\n");
            return 1;
        }
        const auto converted_cam_anim =
            gh::milo_object::convert_cam_anim0_to_cam_anim2(
                parsed_cam_anim);
        const auto cam_anim2_bytes =
            gh::milo_object::serialize_cam_anim2(converted_cam_anim);
        const auto parsed_cam_anim2 =
            gh::milo_object::parse_cam_anim2(cam_anim2_bytes);
        const float expected_anim_y_fov =
            std::atan(0.75f * std::tan(0.75f * 0.5f)) * 2.0f;
        if (gh::milo_object::serialize_cam_anim2(parsed_cam_anim2) !=
                cam_anim2_bytes ||
            parsed_cam_anim2.revision != 2 ||
            parsed_cam_anim2.animatable.revision != 4 ||
            parsed_cam_anim2.camera != "intro.cam" ||
            parsed_cam_anim2.fov_keys.size() != 1 ||
            std::fabs(parsed_cam_anim2.fov_keys[0].value -
                      expected_anim_y_fov) > 1e-6f) {
            std::fprintf(
                stderr, "milo_object_test: CamAnim0->CamAnim2 mismatch\n");
            return 1;
        }

        gh::milo_object::EnvAnim env_anim;
        env_anim.animatable.revision = 0;
        env_anim.environment = "stage.env";
        env_anim.ambient_color_keys.push_back(
            {{1, 0.5f, 0.25f, 1}, 12.0f});
        env_anim.keys_owner = "stage.enm";
        env_anim.fog_range_keys.push_back({{0, 1000}, 12.0f});
        const auto env_bytes =
            gh::milo_object::serialize_env_anim(env_anim);
        if (gh::milo_object::serialize_env_anim(
                gh::milo_object::parse_env_anim(env_bytes)) != env_bytes) {
            std::fprintf(stderr, "milo_object_test: EnvAnim mismatch\n");
            return 1;
        }
        const auto converted_env_anim =
            gh::milo_object::convert_env_anim3_to_env_anim4(
                gh::milo_object::parse_env_anim(env_bytes));
        const auto env_anim4_bytes =
            gh::milo_object::serialize_env_anim4(converted_env_anim);
        const auto parsed_env_anim4 =
            gh::milo_object::parse_env_anim4(env_anim4_bytes);
        if (gh::milo_object::serialize_env_anim4(parsed_env_anim4) !=
                env_anim4_bytes ||
            parsed_env_anim4.revision != 4 ||
            parsed_env_anim4.animatable.revision != 4 ||
            parsed_env_anim4.environment != "stage.env" ||
            parsed_env_anim4.ambient_color_keys.size() != 1 ||
            parsed_env_anim4.fog_range_keys.size() != 1) {
            std::fprintf(
                stderr, "milo_object_test: EnvAnim3->EnvAnim4 mismatch\n");
            return 1;
        }

        gh::milo_object::LightAnim light_anim;
        light_anim.animatable.revision = 0;
        light_anim.light = "key.lit";
        light_anim.color_keys.push_back({{1, 0, 0, 1}, 4.0f});
        light_anim.keys_owner = "key.lnm";
        const auto light_bytes =
            gh::milo_object::serialize_light_anim(light_anim);
        if (gh::milo_object::serialize_light_anim(
                gh::milo_object::parse_light_anim(light_bytes)) !=
            light_bytes) {
            std::fprintf(stderr,
                         "milo_object_test: LightAnim mismatch\n");
            return 1;
        }
        const auto converted_light_anim =
            gh::milo_object::convert_light_anim1_to_light_anim2(
                gh::milo_object::parse_light_anim(light_bytes));
        const auto light_anim2_bytes =
            gh::milo_object::serialize_light_anim2(
                converted_light_anim);
        const auto parsed_light_anim2 =
            gh::milo_object::parse_light_anim2(light_anim2_bytes);
        if (gh::milo_object::serialize_light_anim2(
                parsed_light_anim2) != light_anim2_bytes ||
            parsed_light_anim2.revision != 2 ||
            parsed_light_anim2.animatable.revision != 4 ||
            parsed_light_anim2.light != "key.lit" ||
            parsed_light_anim2.color_keys.size() != 1 ||
            parsed_light_anim2.keys_owner != "key.lnm") {
            std::fprintf(
                stderr,
                "milo_object_test: LightAnim1->LightAnim2 mismatch\n");
            return 1;
        }

        gh::milo_object::ParticleSysAnim part_anim;
        part_anim.animatable.revision = 0;
        part_anim.particle_system = "sparks.part";
        part_anim.start_color_keys.push_back({{1, 1, 1, 1}, 0.0f});
        part_anim.emit_rate_keys.push_back({{1, 2}, 0.0f});
        part_anim.keys_owner = "sparks.pnm";
        const auto part_bytes =
            gh::milo_object::serialize_particle_sys_anim(part_anim);
        if (gh::milo_object::serialize_particle_sys_anim(
                gh::milo_object::parse_particle_sys_anim(part_bytes)) !=
            part_bytes) {
            std::fprintf(
                stderr, "milo_object_test: ParticleSysAnim mismatch\n");
            return 1;
        }
        const auto converted_part_anim =
            gh::milo_object::
                convert_particle_sys_anim2_to_particle_sys_anim3(
                    gh::milo_object::parse_particle_sys_anim(
                        part_bytes));
        const auto part_anim3_bytes =
            gh::milo_object::serialize_particle_sys_anim3(
                converted_part_anim);
        const auto parsed_part_anim3 =
            gh::milo_object::parse_particle_sys_anim3(
                part_anim3_bytes);
        if (gh::milo_object::serialize_particle_sys_anim3(
                parsed_part_anim3) != part_anim3_bytes ||
            parsed_part_anim3.revision != 3 ||
            parsed_part_anim3.animatable.revision != 4 ||
            parsed_part_anim3.particle_system != "sparks.part" ||
            parsed_part_anim3.start_color_keys.size() != 1 ||
            parsed_part_anim3.emit_rate_keys.size() != 1 ||
            parsed_part_anim3.keys_owner != "sparks.pnm") {
            std::fprintf(
                stderr,
                "milo_object_test: ParticleSysAnim2->3 mismatch\n");
            return 1;
        }

        gh::milo_object::MatAnim mat_anim;
        mat_anim.animatable.revision = 0;
        mat_anim.material = "screen.mat";
        gh::milo_object::MatAnimStage stage;
        stage.translation_keys.push_back({{1, 2, 3}, 0.0f});
        stage.scale_keys.push_back({{1, 1, 1}, 0.0f});
        stage.rotation_keys.push_back({{0, 0, 0}, 0.0f});
        stage.texture_keys.push_back({"frame.tex", 0.0f});
        mat_anim.stages.push_back(stage);
        mat_anim.keys_owner = "screen.mnm";
        mat_anim.color_keys.push_back({{1, 1, 1, 1}, 0.0f});
        mat_anim.alpha_keys.push_back({1.0f, 0.0f});
        const auto mat_bytes =
            gh::milo_object::serialize_mat_anim(mat_anim);
        if (gh::milo_object::serialize_mat_anim(
                gh::milo_object::parse_mat_anim(mat_bytes)) != mat_bytes) {
            std::fprintf(stderr,
                         "milo_object_test: MatAnim mismatch\n");
            return 1;
        }
        const auto converted_mat_anim =
            gh::milo_object::convert_mat_anim5_to_mat_anim7(
                gh::milo_object::parse_mat_anim(mat_bytes));
        const auto mat_anim7_bytes =
            gh::milo_object::serialize_mat_anim7(
                converted_mat_anim);
        const auto parsed_mat_anim7 =
            gh::milo_object::parse_mat_anim7(mat_anim7_bytes);
        if (gh::milo_object::serialize_mat_anim7(
                parsed_mat_anim7) != mat_anim7_bytes ||
            parsed_mat_anim7.revision != 7 ||
            parsed_mat_anim7.animatable.revision != 4 ||
            parsed_mat_anim7.material != "screen.mat" ||
            parsed_mat_anim7.translation_keys.size() != 1 ||
            parsed_mat_anim7.texture_keys.size() != 1 ||
            parsed_mat_anim7.color_keys.size() != 1 ||
            parsed_mat_anim7.alpha_keys.size() != 1 ||
            parsed_mat_anim7.keys_owner != "screen.mnm") {
            std::fprintf(
                stderr,
                "milo_object_test: MatAnim5->MatAnim7 mismatch\n");
            return 1;
        }

        gh::milo_object::Text text;
        text.drawable.revision = 1;
        text.transformable.revision = 8;
        text.transformable.local =
            {1, 0, 0, 0, 1, 0, 0, 0, 1, 10, 20, 30};
        text.transformable.world = text.transformable.local;
        text.font = "ui.font";
        text.alignment = 0x22;
        text.text = "SETLIST";
        text.wrap_width = 640.0f;
        text.leading = 1.0f;
        text.size = 32.0f;
        text.markup = true;
        text.caps_mode = 2;
        const auto text_bytes = gh::milo_object::serialize_text(text);
        if (gh::milo_object::serialize_text(
                gh::milo_object::parse_text(text_bytes)) != text_bytes) {
            std::fprintf(stderr, "milo_object_test: Text mismatch\n");
            return 1;
        }
        const auto converted_text =
            gh::milo_object::convert_text15_to_text17(
                gh::milo_object::parse_text(text_bytes));
        const auto text17_bytes =
            gh::milo_object::serialize_text17(converted_text);
        const auto parsed_text17 =
            gh::milo_object::parse_text17(text17_bytes);
        if (gh::milo_object::serialize_text17(parsed_text17) !=
                text17_bytes ||
            parsed_text17.revision != 17 ||
            parsed_text17.drawable.revision != 3 ||
            parsed_text17.transformable.revision != 9 ||
            parsed_text17.font != "ui.font" ||
            parsed_text17.text != "SETLIST" ||
            parsed_text17.size != 32.0f ||
            !parsed_text17.markup ||
            parsed_text17.caps_mode != 2) {
            std::fprintf(
                stderr,
                "milo_object_test: Text15->Text17 mismatch\n");
            return 1;
        }

        gh::milo_object::Movie movie;
        movie.animatable.revision = 0;
        movie.file = "ui/intro.pss";
        movie.texture = "intro.tex";
        movie.stream = true;
        movie.loop = false;
        const auto movie_bytes =
            gh::milo_object::serialize_movie(movie);
        if (gh::milo_object::serialize_movie(
                gh::milo_object::parse_movie(movie_bytes)) !=
            movie_bytes) {
            std::fprintf(stderr, "milo_object_test: Movie mismatch\n");
            return 1;
        }
        const auto converted_movie =
            gh::milo_object::convert_movie6_to_movie8(movie);
        const auto movie8_bytes =
            gh::milo_object::serialize_movie8(converted_movie);
        const auto parsed_movie8 =
            gh::milo_object::parse_movie8(movie8_bytes);
        if (gh::milo_object::serialize_movie8(parsed_movie8) !=
                movie8_bytes ||
            parsed_movie8.revision != 8 ||
            parsed_movie8.object_fields.revision != 0 ||
            parsed_movie8.animatable.revision != 4 ||
            parsed_movie8.file != movie.file ||
            parsed_movie8.texture != movie.texture ||
            !parsed_movie8.stream || parsed_movie8.loop) {
            std::fprintf(
                stderr, "milo_object_test: Movie6->Movie8 mismatch\n");
            return 1;
        }

        gh::milo_object::Font font;
        font.material = "ui_font.mat";
        font.cell_size = {32.0f, 48.0f};
        font.deprecated_size = 48.0f;
        font.base_kerning = 0.05f;
        font.characters = " ABC";
        font.has_kerning_table = true;
        font.kerning.push_back({0x00004241u, -0.1f});
        const auto font_bytes = gh::milo_object::serialize_font(font);
        if (gh::milo_object::serialize_font(
                gh::milo_object::parse_font(font_bytes)) != font_bytes) {
            std::fprintf(stderr, "milo_object_test: Font mismatch\n");
            return 1;
        }
        gh::milo_object::Font15 font15;
        font15.material = "ui_font.mat";
        font15.cell_size = {32.0f, 48.0f};
        font15.deprecated_size = 48.0f;
        font15.base_kerning = 0.05f;
        font15.characters = " ABC";
        font15.has_kerning_table = true;
        font15.kerning.push_back({0x00004241u, -0.1f});
        font15.texture_owner = "ui_font.font";
        font15.bitmap_width = 256;
        font15.bitmap_height = 256;
        font15.texture_cell_size = {0.125f, 0.1875f};
        font15.character_info[65] = {0.0f, 0.0f, 0.75f, 0.8f};
        const auto font15_bytes =
            gh::milo_object::serialize_font15(font15);
        const auto parsed_font15 =
            gh::milo_object::parse_font15(font15_bytes);
        if (gh::milo_object::serialize_font15(parsed_font15) !=
                font15_bytes ||
            parsed_font15.material != "ui_font.mat" ||
            parsed_font15.texture_owner != "ui_font.font" ||
            parsed_font15.kerning.size() != 1 ||
            parsed_font15.character_info[65].character_advance !=
                0.8f) {
            std::fprintf(stderr, "milo_object_test: Font15 mismatch\n");
            return 1;
        }
        gh::milo_object::Font metric_source;
        metric_source.material = "metric.mat";
        metric_source.cell_size = {4.0f, 4.0f};
        metric_source.deprecated_size = 4.0f;
        metric_source.characters = " A";
        std::vector<uint8_t> metric_rgba(8 * 4 * 4, 0);
        for (int y = 0; y < 4; ++y)
            for (int x = 5; x < 7; ++x)
                metric_rgba[(y * 8 + x) * 4 + 3] = 255;
        const auto converted_font =
            gh::milo_object::convert_font7_to_font15(
                metric_source, "metric.font", 8, 4, metric_rgba);
        const auto converted_font_bytes =
            gh::milo_object::serialize_font15(converted_font);
        const auto parsed_converted_font =
            gh::milo_object::parse_font15(converted_font_bytes);
        if (parsed_converted_font.texture_owner != "metric.font" ||
            parsed_converted_font.bitmap_width != 8 ||
            parsed_converted_font.bitmap_height != 4 ||
            parsed_converted_font.texture_cell_size[0] != 0.5f ||
            parsed_converted_font.character_info[32].character_width !=
                0.25f ||
            parsed_converted_font.character_info[65].texture_u !=
                0.625f ||
            parsed_converted_font.character_info[65].character_width !=
                0.5f ||
            parsed_converted_font.character_info[9].character_advance !=
                0.75f) {
            std::fprintf(
                stderr, "milo_object_test: Font7->Font15 mismatch\n");
            return 1;
        }

        gh::milo_object::Tex tex;
        tex.width = 2;
        tex.height = 2;
        tex.bits_per_pixel = 8;
        tex.external_path = "source/test.bmp";
        tex.mipmap_bias = -13.0f;
        tex.use_external = true;
        tex.has_bitmap = true;
        tex.bitmap.header_kind = 1;
        tex.bitmap.bits_per_pixel = 8;
        tex.bitmap.encoding = 3;
        tex.bitmap.width = 2;
        tex.bitmap.height = 2;
        tex.bitmap.bytes_per_line = 2;
        tex.bitmap.data = {0, 1, 2, 3};
        const auto tex_bytes = gh::milo_object::serialize_tex(tex);
        const auto parsed_tex =
            gh::milo_object::parse_tex(tex_bytes);
        if (gh::milo_object::serialize_tex(parsed_tex) != tex_bytes ||
            !parsed_tex.has_bitmap ||
            parsed_tex.bitmap.data.size() != 4 ||
            parsed_tex.external_path != "source/test.bmp") {
            std::fprintf(stderr, "milo_object_test: Tex mismatch\n");
            return 1;
        }
        const auto converted_tex =
            gh::milo_object::convert_tex8_to_tex10(parsed_tex);
        const auto tex10_bytes =
            gh::milo_object::serialize_tex10(converted_tex);
        const auto parsed_tex10 =
            gh::milo_object::parse_tex10(tex10_bytes);
        if (gh::milo_object::serialize_tex10(parsed_tex10) !=
                tex10_bytes ||
            parsed_tex10.revision != 10 ||
            parsed_tex10.object_fields.revision != 0 ||
            parsed_tex10.width != 2 ||
            parsed_tex10.bitmap.data.size() != 4) {
            std::fprintf(stderr,
                         "milo_object_test: Tex8->Tex10 mismatch\n");
            return 1;
        }

        gh::milo_object::View view;
        view.animatable.revision = 0;
        view.animatable.objects.push_back("wave.tnm");
        view.transformable.revision = 8;
        view.transformable.local =
            {1, 0, 0, 0, 1, 0, 0, 0, 1, 10, 20, 30};
        view.transformable.world = view.transformable.local;
        view.transformable.parent = "stage.view";
        view.drawable.revision = 1;
        view.drawable.showing = true;
        view.drawable.objects = {"stage.mesh", "lights.view"};
        view.drawable.sphere = {0, 0, 0, 100};
        view.children_owner = "stage.view";
        view.showing_range = {0, 100};
        const auto view_bytes =
            gh::milo_object::serialize_view(view);
        const auto parsed_view =
            gh::milo_object::parse_view(view_bytes);
        if (gh::milo_object::serialize_view(parsed_view) != view_bytes ||
            parsed_view.animatable.objects.size() != 1 ||
            parsed_view.drawable.objects.size() != 2 ||
            parsed_view.children_owner != "stage.view") {
            std::fprintf(stderr, "milo_object_test: View mismatch\n");
            return 1;
        }

        gh::milo_object::Cam camera;
        camera.transformable = view.transformable;
        camera.drawable = view.drawable;
        camera.near_plane = 1.0f;
        camera.far_plane = 1000.0f;
        camera.fov = 0.75f;
        camera.target_texture = "camera.tex";
        const auto camera_bytes =
            gh::milo_object::serialize_cam(camera);
        const auto parsed_camera =
            gh::milo_object::parse_cam(camera_bytes);
        if (gh::milo_object::serialize_cam(parsed_camera) != camera_bytes ||
            parsed_camera.target_texture != "camera.tex") {
            std::fprintf(stderr, "milo_object_test: Cam mismatch\n");
            return 1;
        }
        const auto converted_camera =
            gh::milo_object::convert_cam9_to_cam12(parsed_camera);
        const auto camera12_bytes =
            gh::milo_object::serialize_cam12(converted_camera);
        const auto parsed_camera12 =
            gh::milo_object::parse_cam12(camera12_bytes);
        const float expected_y_fov =
            std::atan(0.75f * std::tan(0.75f * 0.5f)) * 2.0f;
        if (gh::milo_object::serialize_cam12(parsed_camera12) !=
                camera12_bytes ||
            parsed_camera12.revision != 12 ||
            parsed_camera12.transformable.revision != 9 ||
            parsed_camera12.transformable.parent != "stage.view" ||
            std::fabs(parsed_camera12.y_fov - expected_y_fov) > 1e-6f ||
            parsed_camera12.target_texture != "camera.tex") {
            std::fprintf(
                stderr, "milo_object_test: Cam9->Cam12 mismatch\n");
            return 1;
        }

        gh::milo_object::Flare flare;
        flare.transformable = view.transformable;
        flare.drawable = view.drawable;
        flare.material = "flare.mat";
        flare.sizes = {2, 3};
        flare.range = {10, 100};
        flare.steps = 4;
        const auto flare_bytes =
            gh::milo_object::serialize_flare(flare);
        const auto parsed_flare =
            gh::milo_object::parse_flare(flare_bytes);
        if (gh::milo_object::serialize_flare(parsed_flare) != flare_bytes ||
            parsed_flare.material != "flare.mat") {
            std::fprintf(stderr, "milo_object_test: Flare mismatch\n");
            return 1;
        }
        const auto converted_flare =
            gh::milo_object::convert_flare3_to_flare4(parsed_flare);
        const auto flare4_bytes =
            gh::milo_object::serialize_flare4(converted_flare);
        const auto parsed_flare4 =
            gh::milo_object::parse_flare4(flare4_bytes);
        if (gh::milo_object::serialize_flare4(parsed_flare4) !=
                flare4_bytes ||
            parsed_flare4.revision != 4 ||
            parsed_flare4.object_fields.revision != 0 ||
            parsed_flare4.transformable.revision != 9 ||
            parsed_flare4.drawable.revision != 3 ||
            parsed_flare4.material != flare.material ||
            parsed_flare4.sizes != flare.sizes ||
            parsed_flare4.range != flare.range ||
            parsed_flare4.steps != flare.steps) {
            std::fprintf(
                stderr, "milo_object_test: Flare3->Flare4 mismatch\n");
            return 1;
        }

        gh::milo_object::Light light;
        light.transformable = view.transformable;
        light.color = {0.25f, 0.5f, 1.0f, 1.0f};
        light.range = 250.0f;
        light.serialized_type = 2;
        const auto light_object_bytes =
            gh::milo_object::serialize_light(light);
        const auto parsed_light_object =
            gh::milo_object::parse_light(light_object_bytes);
        if (gh::milo_object::serialize_light(parsed_light_object) !=
                light_object_bytes ||
            parsed_light_object.serialized_type != 2) {
            std::fprintf(stderr, "milo_object_test: Light mismatch\n");
            return 1;
        }
        const auto converted_light =
            gh::milo_object::convert_light3_to_light6(
                parsed_light_object);
        const auto light6_bytes =
            gh::milo_object::serialize_light6(converted_light);
        const auto parsed_light6 =
            gh::milo_object::parse_light6(light6_bytes);
        if (gh::milo_object::serialize_light6(parsed_light6) !=
                light6_bytes ||
            parsed_light6.revision != 6 ||
            parsed_light6.transformable.revision != 9 ||
            parsed_light6.serialized_type != 2 ||
            !parsed_light6.animate_color_from_preset ||
            !parsed_light6.animate_position_from_preset) {
            std::fprintf(
                stderr, "milo_object_test: Light3->Light6 mismatch\n");
            return 1;
        }

        gh::milo_object::Environ environment;
        environment.legacy_drawable = view.drawable;
        environment.lights = {"key.light", "fill.light"};
        environment.ambient_color = {0.1f, 0.2f, 0.3f, 1.0f};
        environment.fog_range = {50, 500};
        environment.fog_color = {0.4f, 0.5f, 0.6f, 1.0f};
        environment.fog_enabled = true;
        const auto environ_bytes =
            gh::milo_object::serialize_environ(environment);
        const auto parsed_environ =
            gh::milo_object::parse_environ(environ_bytes);
        if (gh::milo_object::serialize_environ(parsed_environ) !=
                environ_bytes ||
            parsed_environ.lights.size() != 2 ||
            !parsed_environ.fog_enabled) {
            std::fprintf(stderr, "milo_object_test: Environ mismatch\n");
            return 1;
        }
        const auto converted_environ =
            gh::milo_object::convert_environ1_to_environ5(
                parsed_environ);
        const auto environ5_bytes =
            gh::milo_object::serialize_environ5(converted_environ);
        const auto parsed_environ5 =
            gh::milo_object::parse_environ5(environ5_bytes);
        if (gh::milo_object::serialize_environ5(parsed_environ5) !=
                environ5_bytes ||
            parsed_environ5.revision != 5 ||
            parsed_environ5.lights.size() != 2 ||
            !parsed_environ5.fog_enabled ||
            !parsed_environ5.animate_from_preset ||
            parsed_environ5.fade_out ||
            parsed_environ5.fade_end != 1000.0f) {
            std::fprintf(
                stderr, "milo_object_test: Environ1->Environ5 mismatch\n");
            return 1;
        }

        gh::milo_object::Mat material;
        gh::milo_object::MatTexture diffuse;
        diffuse.stage_blend = 0;
        diffuse.tex_gen = 0;
        diffuse.transform =
            {1, 0, 0, 0, 1, 0, 0, 0, 1, 0.25f, 0.5f, 0};
        diffuse.wrap = 1;
        diffuse.texture = "stage.tex";
        material.textures.push_back(diffuse);
        material.blend = 3;
        material.color = {1, 0.5f, 0.25f, 1};
        material.use_environment = true;
        material.vertex_ambient = true;
        material.vertex_dynamic = false;
        material.cull = true;
        material.multipass = 0;
        material.normalize = false;
        material.z_mode = 1;
        material.alpha_cut = false;
        material.alpha_write = true;
        const auto material_bytes =
            gh::milo_object::serialize_mat(material);
        const auto parsed_material =
            gh::milo_object::parse_mat(material_bytes);
        if (gh::milo_object::serialize_mat(parsed_material) !=
                material_bytes ||
            parsed_material.textures.size() != 1 ||
            parsed_material.textures[0].texture != "stage.tex") {
            std::fprintf(stderr, "milo_object_test: Mat mismatch\n");
            return 1;
        }
        gh::milo_object::Mat27 material27;
        material27.blend = 3;
        material27.color = material.color;
        material27.use_environment = true;
        material27.prelit = true;
        material27.z_mode = 1;
        material27.alpha_write = true;
        material27.tex_gen = 1;
        material27.tex_wrap = 1;
        material27.diffuse_texture = "stage.tex";
        material27.cull = true;
        material27.environment_map = "room.tex";
        const auto material27_bytes =
            gh::milo_object::serialize_mat27(material27);
        const auto parsed_material27 =
            gh::milo_object::parse_mat27(material27_bytes);
        if (gh::milo_object::serialize_mat27(parsed_material27) !=
                material27_bytes ||
            parsed_material27.diffuse_texture != "stage.tex" ||
            parsed_material27.environment_map != "room.tex") {
            std::fprintf(stderr, "milo_object_test: Mat27 mismatch\n");
            return 1;
        }
        gh::milo_object::Mat staged_material = material;
        staged_material.textures[0].stage_blend = 3;
        gh::milo_object::MatTexture light_stage = diffuse;
        light_stage.stage_blend = 2;
        light_stage.texture = "light.tex";
        staged_material.textures.push_back(light_stage);
        staged_material.multipass = 1;
        const auto converted_materials =
            gh::milo_object::convert_mat21_to_mat27_passes(
                staged_material, "body.mat");
        if (converted_materials.size() != 2 ||
            converted_materials[0].name != "body.mat" ||
            converted_materials[0].material.next_pass != "body_1.mat" ||
            !converted_materials[0].material.intensify ||
            converted_materials[1].name != "body_1.mat" ||
            converted_materials[1].material.blend != 2 ||
            converted_materials[1].material.z_mode != 2 ||
            converted_materials[1].material.use_environment ||
            converted_materials[1].material.diffuse_texture !=
                "light.tex") {
            std::fprintf(
                stderr, "milo_object_test: Mat21->Mat27 pass mismatch\n");
            return 1;
        }
        gh::milo_object::MatAnim staged_anim;
        staged_anim.material = "body.mat";
        staged_anim.keys_owner = "body.mnm";
        gh::milo_object::MatAnimStage animated_pass;
        animated_pass.translation_keys.push_back(
            {{{1, 2, 3}}, 10.0f});
        gh::milo_object::MatAnimStage root_stage;
        root_stage.texture_keys.push_back({"root.tex", 20.0f});
        staged_anim.stages = {animated_pass, root_stage};
        const auto converted_anims =
            gh::milo_object::convert_mat_anim5_to_mat_anim7_passes(
                staged_anim, "body.mnm");
        if (converted_anims.size() != 2 ||
            converted_anims[0].name != "body.mnm" ||
            converted_anims[0].animation.material != "body.mat" ||
            converted_anims[0].animation.texture_keys.size() != 1 ||
            converted_anims[1].name != "body_1.mnm" ||
            converted_anims[1].animation.material != "body_1.mat" ||
            converted_anims[1].animation.translation_keys.size() != 1) {
            std::fprintf(
                stderr,
                "milo_object_test: MatAnim5->MatAnim7 pass mismatch\n");
            return 1;
        }

        gh::milo_object::LegacyAnimatable legacy_animatable;
        legacy_animatable.operations.push_back(
            {0, -2.0f, 3.0f, false, {}});
        legacy_animatable.operations.push_back(
            {1, 10.0f, 90.0f, true, {}});
        const auto legacy_settings =
            gh::milo_object::reduce_legacy_animatable(
                legacy_animatable);
        const auto converted_filter =
            gh::milo_object::convert_legacy_animatable_to_anim_filter1(
                legacy_animatable, "verse.anim");
        const auto filter_bytes =
            gh::milo_object::serialize_anim_filter1(converted_filter);
        const auto parsed_filter =
            gh::milo_object::parse_anim_filter1(filter_bytes);
        if (!legacy_settings.requires_filter() ||
            legacy_settings.scale != -2.0f ||
            legacy_settings.offset != 3.0f ||
            legacy_settings.minimum != 10.0f ||
            legacy_settings.maximum != 90.0f ||
            !legacy_settings.loop ||
            gh::milo_object::serialize_anim_filter1(parsed_filter) !=
                filter_bytes ||
            parsed_filter.anim != "verse.anim" ||
            parsed_filter.scale != 2.0f ||
            parsed_filter.offset != 3.0f ||
            parsed_filter.start != 10.0f ||
            parsed_filter.end != 90.0f ||
            parsed_filter.type != 1 ||
            parsed_filter.period != 0.0f) {
            std::fprintf(
                stderr,
                "milo_object_test: Animatable0->AnimFilter1 mismatch\n");
            return 1;
        }
        gh::milo_object::Group12 group12;
        group12.transformable.local =
            {1, 0, 0, 0, 1, 0, 0, 0, 1, 2, 3, 4};
        group12.transformable.world = group12.transformable.local;
        group12.drawable.showing = true;
        group12.objects = {"verse.anim", "stage.mesh"};
        group12.environment = "stage.env";
        group12.lod = "stage_lod.grp";
        group12.lod_screen_size = 0.25f;
        const auto group12_bytes =
            gh::milo_object::serialize_group12(group12);
        const auto parsed_group12 =
            gh::milo_object::parse_group12(group12_bytes);
        if (gh::milo_object::serialize_group12(parsed_group12) !=
                group12_bytes ||
            parsed_group12.objects.size() != 2 ||
            parsed_group12.objects[1] != "stage.mesh" ||
            parsed_group12.environment != "stage.env" ||
            parsed_group12.lod != "stage_lod.grp" ||
            parsed_group12.lod_screen_size != 0.25f) {
            std::fprintf(stderr, "milo_object_test: Group12 mismatch\n");
            return 1;
        }
        gh::milo_object::View source_view;
        source_view.transformable.revision = 8;
        source_view.drawable.revision = 1;
        source_view.showing_range = {0, 0};
        gh::milo_object::ResolvedViewGraph view_graph;
        view_graph.animation_objects = {
            {"verse.anim", "TransAnim"},
            {"shared.mesh", "Mesh"},
        };
        view_graph.drawable_objects = {
            {"stage.env", "Environ"},
            {"shot.cam", "Cam"},
            {"shared.mesh", "Mesh"},
            {"stage.mesh", "Mesh"},
        };
        const auto converted_group =
            gh::milo_object::convert_view7_to_group12(
                source_view, view_graph);
        if (converted_group.objects.size() != 3 ||
            converted_group.objects[0] != "verse.anim" ||
            converted_group.objects[1] != "shared.mesh" ||
            converted_group.objects[2] != "stage.mesh" ||
            converted_group.environment != "stage.env" ||
            converted_group.transformable.revision != 9 ||
            converted_group.drawable.revision != 3) {
            std::fprintf(
                stderr, "milo_object_test: View7->Group12 mismatch\n");
            return 1;
        }
        gh::milo_object::ObjectDir16 object_dir16;
        gh::milo_object::ObjectDirViewport16 viewport16;
        viewport16.transform =
            {1, 0, 0, 0, 1, 0, 0, 0, 1, 4, 5, 6};
        viewport16.legacy_value = 7;
        object_dir16.viewports.push_back(viewport16);
        object_dir16.current_viewport = 1;
        object_dir16.proxy_path = "venue.milo_ps2";
        object_dir16.subdirectories = {"lighting.milo_ps2"};
        const auto object_dir16_bytes =
            gh::milo_object::serialize_object_dir16(object_dir16);
        const auto parsed_object_dir16 =
            gh::milo_object::parse_object_dir16(object_dir16_bytes);
        if (gh::milo_object::serialize_object_dir16(
                parsed_object_dir16) != object_dir16_bytes ||
            parsed_object_dir16.viewports.size() != 1 ||
            parsed_object_dir16.subdirectories.size() != 1) {
            std::fprintf(stderr, "milo_object_test: ObjectDir16 mismatch\n");
            return 1;
        }
        gh::milo_object::RndDir8 rnd_dir8;
        rnd_dir8.object_directory = object_dir16;
        rnd_dir8.environment = "stage.env";
        rnd_dir8.test_event = "test";
        const auto rnd_dir8_bytes =
            gh::milo_object::serialize_rnd_dir8(rnd_dir8);
        const auto parsed_rnd_dir8 =
            gh::milo_object::parse_rnd_dir8(rnd_dir8_bytes);
        if (gh::milo_object::serialize_rnd_dir8(parsed_rnd_dir8) !=
                rnd_dir8_bytes ||
            parsed_rnd_dir8.environment != "stage.env" ||
            parsed_rnd_dir8.test_event != "test") {
            std::fprintf(stderr, "milo_object_test: RndDir8 mismatch\n");
            return 1;
        }
        gh::milo_object::PanelDir2 panel_dir2;
        panel_dir2.render_directory = rnd_dir8;
        panel_dir2.camera = "venue.cam";
        panel_dir2.test_event = "test";
        const auto panel_dir2_bytes =
            gh::milo_object::serialize_panel_dir2(panel_dir2);
        const auto parsed_panel_dir2 =
            gh::milo_object::parse_panel_dir2(panel_dir2_bytes);
        if (gh::milo_object::serialize_panel_dir2(parsed_panel_dir2) !=
                panel_dir2_bytes ||
            parsed_panel_dir2.camera != "venue.cam") {
            std::fprintf(stderr, "milo_object_test: PanelDir2 mismatch\n");
            return 1;
        }
        gh::milo_object::WorldDir11 world_dir11;
        world_dir11.legacy_value = 1;
        world_dir11.legacy_float = 2.0f;
        world_dir11.fake_hud_filename = "hud.milo_ps2";
        world_dir11.panel_directory = panel_dir2;
        world_dir11.legacy_transform =
            {1, 0, 0, 0, 1, 0, 0, 0, 1, 7, 8, 9};
        const auto world_dir11_bytes =
            gh::milo_object::serialize_world_dir11(world_dir11);
        const auto parsed_world_dir11 =
            gh::milo_object::parse_world_dir11(world_dir11_bytes);
        if (gh::milo_object::serialize_world_dir11(
                parsed_world_dir11) != world_dir11_bytes ||
            parsed_world_dir11.fake_hud_filename != "hud.milo_ps2" ||
            parsed_world_dir11.legacy_transform[9] != 7.0f) {
            std::fprintf(stderr, "milo_object_test: WorldDir11 mismatch\n");
            return 1;
        }
        gh::milo_object::Character9 character9;
        character9.render_directory = rnd_dir8;
        character9.lods.push_back({0.5f, "lod0.grp"});
        character9.shadow = "shadow.grp";
        character9.self_shadow = true;
        character9.sphere_base = "bone_pelvis.mesh";
        const auto character9_bytes =
            gh::milo_object::serialize_character9(character9);
        const auto parsed_character9 =
            gh::milo_object::parse_character9(character9_bytes);
        if (gh::milo_object::serialize_character9(parsed_character9) !=
                character9_bytes ||
            parsed_character9.lods.size() != 1 ||
            parsed_character9.lods[0].group != "lod0.grp" ||
            parsed_character9.sphere_base != "bone_pelvis.mesh") {
            std::fprintf(stderr, "milo_object_test: Character9 mismatch\n");
            return 1;
        }
        gh::milo_object::BandCharacter1 band_character1;
        band_character1.character = character9;
        const auto band_character1_bytes =
            gh::milo_object::serialize_band_character1(
                band_character1);
        const auto parsed_band_character1 =
            gh::milo_object::parse_band_character1(
                band_character1_bytes);
        if (gh::milo_object::serialize_band_character1(
                parsed_band_character1) != band_character1_bytes ||
            parsed_band_character1.character.lods.size() != 1) {
            std::fprintf(
                stderr, "milo_object_test: BandCharacter1 mismatch\n");
            return 1;
        }
        gh::milo_object::CharDriver3 char_driver3;
        char_driver3.weightable.weight = 0.75f;
        char_driver3.weightable.weight_owner = "main.drv";
        char_driver3.bones = "bone.servo";
        char_driver3.clips = "../../anims/main.milo";
        char_driver3.realign = true;
        const auto char_driver3_bytes =
            gh::milo_object::serialize_char_driver3(char_driver3);
        const auto parsed_char_driver3 =
            gh::milo_object::parse_char_driver3(
                char_driver3_bytes);
        if (gh::milo_object::serialize_char_driver3(
                parsed_char_driver3) != char_driver3_bytes ||
            parsed_char_driver3.weightable.weight != 0.75f ||
            !parsed_char_driver3.realign) {
            std::fprintf(
                stderr, "milo_object_test: CharDriver3 mismatch\n");
            return 1;
        }
        gh::milo_object::CharDriverMidi3 driver_midi3;
        driver_midi3.driver = char_driver3;
        driver_midi3.default_clip = "idle.clip";
        const auto driver_midi3_bytes =
            gh::milo_object::serialize_char_driver_midi3(
                driver_midi3);
        if (gh::milo_object::serialize_char_driver_midi3(
                gh::milo_object::parse_char_driver_midi3(
                    driver_midi3_bytes)) != driver_midi3_bytes) {
            std::fprintf(
                stderr, "milo_object_test: CharDriverMidi3 mismatch\n");
            return 1;
        }
        gh::milo_object::CharWeightSetter2 weight_setter2;
        weight_setter2.weightable.weight = 0.5f;
        weight_setter2.driver = "main.drv";
        weight_setter2.flags = 7;
        const auto weight_setter2_bytes =
            gh::milo_object::serialize_char_weight_setter2(
                weight_setter2);
        if (gh::milo_object::serialize_char_weight_setter2(
                gh::milo_object::parse_char_weight_setter2(
                    weight_setter2_bytes)) != weight_setter2_bytes) {
            std::fprintf(
                stderr, "milo_object_test: CharWeightSetter2 mismatch\n");
            return 1;
        }
        gh::milo_object::CharIKHand2 ik_hand2;
        ik_hand2.hand = "bone_R-hand.mesh";
        ik_hand2.target = "bone_R-hand-target.mesh";
        ik_hand2.orientation = true;
        ik_hand2.stretch = true;
        ik_hand2.scalable = false;
        const auto ik_hand2_bytes =
            gh::milo_object::serialize_char_ik_hand2(ik_hand2);
        if (gh::milo_object::serialize_char_ik_hand2(
                gh::milo_object::parse_char_ik_hand2(
                    ik_hand2_bytes)) != ik_hand2_bytes) {
            std::fprintf(
                stderr, "milo_object_test: CharIKHand2 mismatch\n");
            return 1;
        }
        gh::milo_object::CharIKMidi4 ik_midi4;
        ik_midi4.bone = "bone_fret.mesh";
        const auto ik_midi4_bytes =
            gh::milo_object::serialize_char_ik_midi4(ik_midi4);
        if (gh::milo_object::serialize_char_ik_midi4(
                gh::milo_object::parse_char_ik_midi4(
                    ik_midi4_bytes)) != ik_midi4_bytes) {
            std::fprintf(
                stderr, "milo_object_test: CharIKMidi4 mismatch\n");
            return 1;
        }
        gh::milo_object::CharIKRod2 ik_rod2;
        ik_rod2.left_end = "bone_L-knee.mesh";
        ik_rod2.right_end = "bone_R-knee.mesh";
        ik_rod2.dest_pos = 0.25f;
        ik_rod2.side_axis = "bone_pelvis.mesh";
        ik_rod2.vertical = true;
        ik_rod2.dest = "knee_target.mesh";
        ik_rod2.transform[11] = 1.0f;
        const auto ik_rod2_bytes =
            gh::milo_object::serialize_char_ik_rod2(ik_rod2);
        if (gh::milo_object::serialize_char_ik_rod2(
                gh::milo_object::parse_char_ik_rod2(
                    ik_rod2_bytes)) != ik_rod2_bytes) {
            std::fprintf(
                stderr, "milo_object_test: CharIKRod2 mismatch\n");
            return 1;
        }
        gh::milo_object::CharHair2 hair2;
        hair2.stiffness = 1.0f;
        hair2.torsion = 2.0f;
        hair2.inertia = 3.0f;
        hair2.gravity = 4.0f;
        hair2.weight = 5.0f;
        hair2.friction = 6.0f;
        gh::milo_object::CharHairStrand2 hair_strand2;
        hair_strand2.root = "bone_head.mesh";
        hair_strand2.angle = 0.5f;
        gh::milo_object::CharHairPoint2 hair_point2;
        hair_point2.position = {1.0f, 2.0f, 3.0f};
        hair_point2.bone = "hair_01.mesh";
        hair_point2.length = 4.0f;
        hair_point2.legacy_value = -1;
        hair_point2.legacy_name = "legacy";
        hair_point2.radius = 5.0f;
        hair_point2.outer_radius = 6.0f;
        hair_strand2.points.push_back(hair_point2);
        hair_strand2.base_matrix[0] = 1.0f;
        hair_strand2.root_matrix[8] = 1.0f;
        hair2.strands.push_back(hair_strand2);
        hair2.simulate = true;
        const auto hair2_bytes =
            gh::milo_object::serialize_char_hair2(hair2);
        if (gh::milo_object::serialize_char_hair2(
                gh::milo_object::parse_char_hair2(
                    hair2_bytes)) != hair2_bytes) {
            std::fprintf(
                stderr, "milo_object_test: CharHair2 mismatch\n");
            return 1;
        }
        gh::milo_object::FaceFxLipSyncServo5 lip_servo5;
        lip_servo5.object_fields.type = "gh2";
        lip_servo5.weightable.weight_owner = "lip.servo";
        lip_servo5.facefx_path = "../../guitarist.fac";
        lip_servo5.viseme_milo = "../../anims/viseme.milo";
        lip_servo5.targets.push_back(
            {"eye-L.mesh", 2, "L-eyeZ"});
        const auto lip_servo5_bytes =
            gh::milo_object::serialize_facefx_lip_sync_servo5(
                lip_servo5);
        if (gh::milo_object::serialize_facefx_lip_sync_servo5(
                gh::milo_object::parse_facefx_lip_sync_servo5(
                    lip_servo5_bytes)) != lip_servo5_bytes) {
            std::fprintf(
                stderr,
                "milo_object_test: FaceFxLipSyncServo5 mismatch\n");
            return 1;
        }
        gh::milo_object::EventTrigger8 event_trigger8;
        event_trigger8.trigger_event = "game_over";
        event_trigger8.animations.push_back(
            {"crash_static.filt", 0.0f, false, 0.0f});
        event_trigger8.sounds = {"crash.cue"};
        event_trigger8.shows = {"visible.grp"};
        event_trigger8.legacy_hides = {"hidden.grp"};
        event_trigger8.enable_events = {"enable"};
        event_trigger8.disable_events = {"disable"};
        event_trigger8.wait_for_events = {"wait_for"};
        event_trigger8.next_link = "next.trig";
        event_trigger8.proxy_calls.push_back(
            {"proxy.dir", "trigger"});
        const auto event_trigger8_bytes =
            gh::milo_object::serialize_event_trigger8(
                event_trigger8);
        if (gh::milo_object::serialize_event_trigger8(
                gh::milo_object::parse_event_trigger8(
                    event_trigger8_bytes)) != event_trigger8_bytes) {
            std::fprintf(
                stderr, "milo_object_test: EventTrigger8 mismatch\n");
            return 1;
        }
        gh::milo_object::OutfitLoader1 outfit_loader1;
        outfit_loader1.object_fields.type = "guitar";
        outfit_loader1.directory = "../../../og";
        gh::milo_object::OutfitLoaderCategory1 outfit_category1;
        outfit_category1.selected = 1;
        outfit_category1.shown = 2;
        outfit_category1.outfits.push_back({3, 4, 5});
        outfit_loader1.categories.push_back(outfit_category1);
        const auto outfit_loader1_bytes =
            gh::milo_object::serialize_outfit_loader1(
                outfit_loader1);
        if (gh::milo_object::serialize_outfit_loader1(
                gh::milo_object::parse_outfit_loader1(
                    outfit_loader1_bytes)) != outfit_loader1_bytes) {
            std::fprintf(
                stderr, "milo_object_test: OutfitLoader1 mismatch\n");
            return 1;
        }
        gh::milo_object::WorldFx1 world_fx1;
        world_fx1.render_directory = rnd_dir8;
        const auto world_fx1_bytes =
            gh::milo_object::serialize_world_fx1(world_fx1);
        if (gh::milo_object::serialize_world_fx1(
                gh::milo_object::parse_world_fx1(
                    world_fx1_bytes)) != world_fx1_bytes) {
            std::fprintf(
                stderr, "milo_object_test: WorldFx1 mismatch\n");
            return 1;
        }
        gh::milo_object::CharLookAt2 look_at2;
        look_at2.source = "bone_head.mesh";
        look_at2.pivot = "bone_neck.mesh";
        look_at2.target = "look_target.mesh";
        look_at2.half_time = 0.25f;
        look_at2.min_yaw = -45.0f;
        look_at2.max_yaw = 45.0f;
        const auto look_at2_bytes =
            gh::milo_object::serialize_char_look_at2(look_at2);
        if (gh::milo_object::serialize_char_look_at2(
                gh::milo_object::parse_char_look_at2(
                    look_at2_bytes)) != look_at2_bytes) {
            std::fprintf(
                stderr, "milo_object_test: CharLookAt2 mismatch\n");
            return 1;
        }
        gh::milo_object::CharEyes3 eyes3;
        eyes3.eyes = {"eye_L.mesh", "eye_R.mesh"};
        eyes3.legacy_transform = "bone_head.mesh";
        const auto eyes3_bytes =
            gh::milo_object::serialize_char_eyes3(eyes3);
        if (gh::milo_object::serialize_char_eyes3(
                gh::milo_object::parse_char_eyes3(
                    eyes3_bytes)) != eyes3_bytes) {
            std::fprintf(
                stderr, "milo_object_test: CharEyes3 mismatch\n");
            return 1;
        }
        gh::milo_object::CharWalk1 walk1;
        walk1.object_fields.type = "guitarist";
        const auto walk1_bytes =
            gh::milo_object::serialize_char_walk1(walk1);
        if (gh::milo_object::serialize_char_walk1(
                gh::milo_object::parse_char_walk1(
                    walk1_bytes)) != walk1_bytes) {
            std::fprintf(
                stderr, "milo_object_test: CharWalk1 mismatch\n");
            return 1;
        }
        gh::milo_object::CharServoBone2 char_servo2;
        char_servo2.revision = 2;
        char_servo2.clip_type = "guitarist";
        const auto char_servo2_bytes =
            gh::milo_object::serialize_char_servo_bone2(char_servo2);
        if (gh::milo_object::serialize_char_servo_bone2(
                gh::milo_object::parse_char_servo_bone2(
                    char_servo2_bytes)) != char_servo2_bytes) {
            std::fprintf(
                stderr, "milo_object_test: CharServoBone2 mismatch\n");
            return 1;
        }
        gh::milo_object::CharUpperTwist1 upper_twist1;
        upper_twist1.upper_arm = "upper.mesh";
        upper_twist1.twist1 = "twist1.mesh";
        upper_twist1.twist2 = "twist2.mesh";
        const auto upper_twist1_bytes =
            gh::milo_object::serialize_char_upper_twist1(
                upper_twist1);
        if (gh::milo_object::serialize_char_upper_twist1(
                gh::milo_object::parse_char_upper_twist1(
                    upper_twist1_bytes)) != upper_twist1_bytes) {
            std::fprintf(
                stderr, "milo_object_test: CharUpperTwist1 mismatch\n");
            return 1;
        }
        gh::milo_object::CharForeTwist4 fore_twist4;
        fore_twist4.revision = 4;
        fore_twist4.offset = 90.0f;
        fore_twist4.hand = "hand.mesh";
        fore_twist4.twist2 = "fore_twist2.mesh";
        fore_twist4.bias = 0.25f;
        const auto fore_twist4_bytes =
            gh::milo_object::serialize_char_fore_twist4(
                fore_twist4);
        if (gh::milo_object::serialize_char_fore_twist4(
                gh::milo_object::parse_char_fore_twist4(
                    fore_twist4_bytes)) != fore_twist4_bytes) {
            std::fprintf(
                stderr, "milo_object_test: CharForeTwist4 mismatch\n");
            return 1;
        }
        gh::milo_object::CharPosConstraint2 pos_constraint2;
        pos_constraint2.targets = {"target.mesh"};
        pos_constraint2.source = "source.mesh";
        pos_constraint2.box_min = {-1, -2, -3};
        pos_constraint2.box_max = {1, 2, 3};
        const auto pos_constraint2_bytes =
            gh::milo_object::serialize_char_pos_constraint2(
                pos_constraint2);
        if (gh::milo_object::serialize_char_pos_constraint2(
                gh::milo_object::parse_char_pos_constraint2(
                    pos_constraint2_bytes)) != pos_constraint2_bytes) {
            std::fprintf(
                stderr,
                "milo_object_test: CharPosConstraint2 mismatch\n");
            return 1;
        }
        gh::milo_object::CharClipSet14 clip_set14;
        clip_set14.object_directory = object_dir16;
        clip_set14.blend_width = 1.25f;
        clip_set14.play_flags = 2;
        clip_set14.clips.push_back({"idle.clip", 3, 4});
        clip_set14.move_self = true;
        clip_set14.recenter_targets = {"first"};
        clip_set14.recenter_average = {"second"};
        clip_set14.recenter_slide = true;
        clip_set14.legacy_type = "legacy";
        clip_set14.legacy_type_version = 5;
        const auto clip_set14_bytes =
            gh::milo_object::serialize_char_clip_set14(clip_set14);
        const auto parsed_clip_set14 =
            gh::milo_object::parse_char_clip_set14(
                clip_set14_bytes, 1);
        if (gh::milo_object::serialize_char_clip_set14(
                parsed_clip_set14) != clip_set14_bytes ||
            parsed_clip_set14.clips.size() != 1 ||
            parsed_clip_set14.clips[0].clip != "idle.clip") {
            std::fprintf(stderr, "milo_object_test: CharClipSet14 mismatch\n");
            return 1;
        }

        gh::milo_object::ParticleSys particles;
        particles.animatable.revision = 0;
        particles.transformable.revision = 8;
        particles.drawable.revision = 1;
        particles.life = {30, 60};
        particles.box_extent_1 = {-1, -2, -3};
        particles.box_extent_2 = {1, 2, 3};
        particles.speed = {5, 10};
        particles.emit_rate = {2, 4};
        particles.start_size = {1, 2};
        particles.delta_size = {0, 1};
        particles.start_color_low = {1, 0, 0, 1};
        particles.start_color_high = {1, 1, 0, 1};
        particles.end_color_low = {0, 0, 1, 0};
        particles.end_color_high = {0, 1, 1, 0};
        particles.bounce_enabled = true;
        particles.bounce_plane = {0, 1, 0, 0};
        particles.force_direction = {0, -1, 0};
        particles.material = "sparks.mat";
        particles.type = 1;
        particles.grow_ratio = 0.2f;
        particles.shrink_ratio = 0.8f;
        particles.mid_color_ratio = 0.5f;
        particles.max_particles = 128;
        particles.bubble_period = {10, 20};
        particles.bubble_size = {1, 2};
        particles.relative_motion = 0.25f;
        particles.emitter_mesh = "emitter.mesh";
        particles.preserve_particles = true;
        particles.particles.push_back(
            {{1, 2, 3}, {1, 0.5f, 0.25f, 1}, 2});
        const auto particles_bytes =
            gh::milo_object::serialize_particle_sys(particles);
        const auto parsed_particles =
            gh::milo_object::parse_particle_sys(particles_bytes);
        if (gh::milo_object::serialize_particle_sys(parsed_particles) !=
                particles_bytes ||
            parsed_particles.material != "sparks.mat" ||
            parsed_particles.emitter_mesh != "emitter.mesh" ||
            parsed_particles.particles.size() != 1) {
            std::fprintf(stderr,
                         "milo_object_test: ParticleSys mismatch\n");
            return 1;
        }
        auto convertible_particles = parsed_particles;
        convertible_particles.bounce_enabled = false;
        convertible_particles.preserve_particles = false;
        convertible_particles.particles.clear();
        const auto converted_particles =
            gh::milo_object::convert_particle_sys22_to_particle_sys27(
                convertible_particles);
        const auto particles27_bytes =
            gh::milo_object::serialize_particle_sys27(
                converted_particles);
        const auto parsed_particles27 =
            gh::milo_object::parse_particle_sys27(
                particles27_bytes);
        if (gh::milo_object::serialize_particle_sys27(
                parsed_particles27) != particles27_bytes ||
            parsed_particles27.revision != 27 ||
            parsed_particles27.animatable.revision != 4 ||
            parsed_particles27.transformable.revision != 9 ||
            parsed_particles27.drawable.revision != 3 ||
            parsed_particles27.material != "sparks.mat" ||
            parsed_particles27.emitter_mesh != "emitter.mesh" ||
            parsed_particles27.max_particles != 128) {
            std::fprintf(
                stderr,
                "milo_object_test: ParticleSys22->27 mismatch\n");
            return 1;
        }
        const auto bounce_trans =
            gh::milo_object::convert_bounce_plane_to_trans9(
                {0, 2, 0, -4});
        const auto bounce_trans_bytes =
            gh::milo_object::serialize_trans9(bounce_trans);
        const auto parsed_bounce_trans =
            gh::milo_object::parse_trans9(bounce_trans_bytes);
        const auto bounced_particles =
            gh::milo_object::convert_particle_sys22_to_particle_sys27(
                parsed_particles, "sparks_bounce.trans");
        if (gh::milo_object::serialize_trans9(parsed_bounce_trans) !=
                bounce_trans_bytes ||
            parsed_bounce_trans.local[7] != 1.0f ||
            parsed_bounce_trans.local[10] != 2.0f ||
            bounced_particles.bounce != "sparks_bounce.trans") {
            std::fprintf(
                stderr,
                "milo_object_test: ParticleSys bounce conversion mismatch\n");
            return 1;
        }
        gh::milo_object::ParticleSys27 preserved_particles27;
        preserved_particles27.preserve_particles = true;
        preserved_particles27.particles.push_back(
            {{1, 2, 3}, {1, 0.5f, 0.25f, 1}, 2});
        const auto preserved_particles27_bytes =
            gh::milo_object::serialize_particle_sys27(
                preserved_particles27);
        const auto parsed_preserved_particles27 =
            gh::milo_object::parse_particle_sys27(
                preserved_particles27_bytes);
        if (parsed_preserved_particles27.particles.size() != 1 ||
            gh::milo_object::serialize_particle_sys27(
                parsed_preserved_particles27) !=
                preserved_particles27_bytes) {
            std::fprintf(
                stderr,
                "milo_object_test: ParticleSys27 row mismatch\n");
            return 1;
        }

        gh::milo_object::Mesh mesh;
        mesh.transformable.revision = 8;
        mesh.drawable.revision = 1;
        mesh.material = "stage.mat";
        mesh.geometry_owner = "stage.mesh";
        mesh.volume = 1;
        gh::milo_object::MeshVertex mesh_vertex;
        mesh_vertex.position = {1, 2, 3};
        mesh_vertex.normal = {0, 0, 1};
        mesh_vertex.color_or_weights = {1, 1, 1, 1};
        mesh_vertex.uv = {0.25f, 0.75f};
        mesh.vertices.push_back(mesh_vertex);
        mesh.faces.push_back({0, 0, 0});
        mesh.patches.push_back(3);
        mesh.strip_results.push_back({{3}, {0, 0, 0}});
        mesh.has_bones = true;
        mesh.bone_slots[0].bone = "root.mesh";
        mesh.bone_slots[0].offset =
            {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
        const auto mesh_bytes =
            gh::milo_object::serialize_mesh(mesh);
        const auto parsed_mesh =
            gh::milo_object::parse_mesh(mesh_bytes);
        if (gh::milo_object::serialize_mesh(parsed_mesh) != mesh_bytes ||
            parsed_mesh.vertices.size() != 1 ||
            parsed_mesh.faces.size() != 1 ||
            parsed_mesh.bone_slots[0].bone != "root.mesh") {
            std::fprintf(stderr, "milo_object_test: Mesh mismatch\n");
            return 1;
        }

        const auto converted_mesh =
            gh::milo_object::convert_mesh25_to_mesh28(parsed_mesh);
        const auto mesh28_bytes =
            gh::milo_object::serialize_mesh28(converted_mesh);
        const auto parsed_mesh28 =
            gh::milo_object::parse_mesh28(mesh28_bytes);
        if (gh::milo_object::serialize_mesh28(parsed_mesh28) !=
                mesh28_bytes ||
            parsed_mesh28.revision != 28 ||
            parsed_mesh28.transformable.revision != 9 ||
            parsed_mesh28.drawable.revision != 3 ||
            parsed_mesh28.vertices.size() != 1 ||
            parsed_mesh28.vertices[0].position[0] != 1.0f ||
            parsed_mesh28.group_sections.size() != 1 ||
            parsed_mesh28.bone_slots[0].bone != "root.mesh") {
            std::fprintf(stderr,
                         "milo_object_test: Mesh25->Mesh28 mismatch\n");
            return 1;
        }
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "milo_object_test: %s\n", ex.what());
        return 1;
    }
    std::printf("milo_object_test: all checks passed\n");
    return 0;
}
