#include "milo_object.h"

#include <cstdio>

int main() {
    gh::milo_object::Morph morph;
    morph.revision = 3;
    morph.animatable.revision = 0;
    morph.animatable.entries.push_back({"face.mesh", 1.0f, 9.0f});
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

        gh::milo_object::MeshAnim mesh_anim;
        mesh_anim.revision = 0;
        mesh_anim.animatable.revision = 0;
        mesh_anim.mesh = "cloth.mesh";
        mesh_anim.point_keys.push_back(
            {{{{1, 2, 3}}, {{4, 5, 6}}}, 10.0f});
        mesh_anim.texcoord_keys.push_back(
            {{{{0, 0}}, {{1, 1}}}, 10.0f});
        mesh_anim.color_keys.push_back(
            {{0xff000000u, 0xffffffffu}, 10.0f});
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

        gh::milo_object::Mat material;
        gh::milo_object::MatTexture diffuse;
        diffuse.slot = 0;
        diffuse.map_type = 0;
        diffuse.transform =
            {1, 0, 0, 0, 1, 0, 0, 0, 1, 0.25f, 0.5f, 0};
        diffuse.wrap = 1;
        diffuse.texture = "stage.tex";
        material.textures.push_back(diffuse);
        material.primary_blend = 3;
        material.color = {1, 0.5f, 0.25f, 1};
        material.use_environment = true;
        material.prelit = false;
        material.z_mode = 1;
        material.tail_blend = 3;
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
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "milo_object_test: %s\n", ex.what());
        return 1;
    }
    std::printf("milo_object_test: all checks passed\n");
    return 0;
}
