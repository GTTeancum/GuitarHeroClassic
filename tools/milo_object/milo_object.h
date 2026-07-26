// Revision-aware semantic MILO object-body readers/writers.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace gh::milo_object {

struct LegacyAnimEntry {
    std::string object;
    float start_frame = 0.0f;
    float end_frame = 0.0f;
};

struct LegacyAnimatable {
    uint32_t revision = 0;
    std::vector<LegacyAnimEntry> entries;
    std::vector<std::string> objects;
};

struct MorphKey {
    float value = 0.0f;
    float frame = 0.0f;
};

struct MorphPose {
    std::string mesh;
    std::vector<MorphKey> keys;
};

struct Morph {
    uint32_t revision = 3;
    LegacyAnimatable animatable;
    std::vector<MorphPose> poses;
    std::string target;
    bool normals = false;
    bool spline = false;
    float intensity = 1.0f;
};

struct LegacyDrawable {
    uint32_t revision = 0;
    bool showing = true;
    std::vector<std::string> objects;
    std::array<float, 4> sphere{};
    float draw_order = 0.0f;
    std::string legacy_target;
};

struct LegacyTransformable {
    uint32_t revision = 8;
    std::array<float, 12> local{};
    std::array<float, 12> world{};
    std::vector<std::string> children;
    uint32_t constraint = 0;
    std::string target;
    bool preserve_scale = false;
    std::string parent;
};

struct Vec3Key {
    std::array<float, 3> value{};
    float frame = 0.0f;
};

struct QuatKey {
    std::array<float, 4> value{};
    float frame = 0.0f;
};

struct Vec2Key {
    std::array<float, 2> value{};
    float frame = 0.0f;
};

struct ColorKey {
    std::array<float, 4> value{};
    float frame = 0.0f;
};

struct TransAnim {
    uint32_t revision = 4;
    LegacyAnimatable animatable;
    LegacyDrawable drawable;
    std::string target;
    std::vector<QuatKey> rotation_keys;
    std::vector<Vec3Key> translation_keys;
    std::string keys_owner;
    bool translation_spline = false;
    bool repeat_translation = false;
    std::vector<Vec3Key> scale_keys;
    bool scale_spline = false;
    bool follow_path = false;
    bool rotation_slerp = false;
};

struct MultiMesh {
    uint32_t revision = 0;
    LegacyDrawable drawable;
    std::string mesh;
    std::vector<std::array<float, 12>> transforms;
};

template <typename T>
struct VectorKey {
    std::vector<T> values;
    float frame = 0.0f;
};

struct MeshAnim {
    uint32_t revision = 0;
    LegacyAnimatable animatable;
    std::string mesh;
    std::vector<VectorKey<std::array<float, 3>>> point_keys;
    std::vector<VectorKey<std::array<float, 2>>> texcoord_keys;
    std::vector<VectorKey<uint32_t>> color_keys;
    std::string keys_owner;
};

struct CamAnim {
    uint32_t revision = 0;
    LegacyAnimatable animatable;
    std::string camera;
    std::vector<MorphKey> fov_keys;
    std::string keys_owner;
};

struct EnvAnim {
    uint32_t revision = 3;
    LegacyAnimatable animatable;
    std::string environment;
    std::vector<ColorKey> ambient_color_keys;
    std::string keys_owner;
    std::vector<ColorKey> fog_color_keys;
    std::vector<Vec2Key> fog_range_keys;
};

struct LightAnim {
    uint32_t revision = 1;
    LegacyAnimatable animatable;
    std::string light;
    std::vector<ColorKey> color_keys;
    std::string keys_owner;
};

struct ParticleSysAnim {
    uint32_t revision = 2;
    LegacyAnimatable animatable;
    std::string particle_system;
    std::vector<ColorKey> start_color_keys;
    std::vector<ColorKey> end_color_keys;
    std::vector<Vec2Key> emit_rate_keys;
    std::string keys_owner;
    std::vector<Vec2Key> speed_keys;
    std::vector<Vec2Key> life_keys;
    std::vector<Vec2Key> start_size_keys;
};

struct ObjectKey {
    std::string object;
    float frame = 0.0f;
};

struct MatAnimStage {
    std::vector<Vec3Key> translation_keys;
    std::vector<Vec3Key> scale_keys;
    std::vector<Vec3Key> rotation_keys;
    std::vector<ObjectKey> texture_keys;
};

struct MatAnim {
    uint32_t revision = 5;
    LegacyAnimatable animatable;
    std::string material;
    std::vector<MatAnimStage> stages;
    std::string keys_owner;
    std::vector<ColorKey> color_keys;
    std::vector<MorphKey> alpha_keys;
};

struct Text {
    uint32_t revision = 15;
    LegacyDrawable drawable;
    LegacyTransformable transformable;
    std::string font;
    int32_t alignment = 0;
    std::string text;
    std::array<float, 4> color = {1, 1, 1, 1};
    float wrap_width = 0.0f;
    float leading = 1.0f;
    int32_t fixed_length = 0;
    float italics = 0.0f;
    float size = 1.0f;
    bool markup = false;
    int32_t caps_mode = 0;
};

struct Movie {
    uint32_t revision = 6;
    LegacyAnimatable animatable;
    std::string file;
    std::string texture;
    bool stream = false;
    bool loop = true;
};

struct FontKerning {
    uint32_t packed_char_pair = 0;
    float kerning = 0.0f;
};

struct Font {
    uint32_t revision = 7;
    std::string material;
    std::array<float, 2> cell_size{};
    float deprecated_size = 0.0f;
    float base_kerning = 0.0f;
    std::string characters;
    bool has_kerning_table = false;
    std::vector<FontKerning> kerning;
};

struct HmxBitmap {
    uint8_t header_kind = 1;
    uint8_t bits_per_pixel = 0;
    int32_t encoding = 0;
    uint8_t mipmap_count = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t bytes_per_line = 0;
    uint16_t wii_alpha = 0;
    std::array<uint8_t, 17> reserved{};
    std::vector<uint8_t> data;
};

struct Tex {
    uint32_t revision = 8;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t bits_per_pixel = 0;
    std::string external_path;
    float mipmap_bias = 0.0f;
    int32_t type = 0;
    bool use_external = false;
    bool has_bitmap = false;
    HmxBitmap bitmap;
};

struct View {
    uint32_t revision = 7;
    LegacyAnimatable animatable;
    LegacyTransformable transformable;
    LegacyDrawable drawable;
    std::string children_owner;
    std::array<float, 2> showing_range{};
};

struct Cam {
    uint32_t revision = 9;
    LegacyTransformable transformable;
    LegacyDrawable drawable;
    float near_plane = 1.0f;
    float far_plane = 1000.0f;
    float fov = 0.5f;
    std::array<float, 4> screen_rect = {0, 0, 1, 1};
    std::array<float, 2> z_range = {0, 1};
    std::string target_texture;
};

struct Flare {
    uint32_t revision = 3;
    LegacyTransformable transformable;
    LegacyDrawable drawable;
    std::string material;
    std::array<float, 2> sizes{};
    std::array<float, 2> range{};
    int32_t steps = 1;
};

struct Light {
    uint32_t revision = 3;
    LegacyTransformable transformable;
    std::array<float, 4> color = {1, 1, 1, 1};
    float range = 0.0f;
    int32_t serialized_type = 0;
};

struct Environ {
    uint32_t revision = 1;
    LegacyDrawable legacy_drawable;
    std::vector<std::string> lights;
    std::array<float, 4> ambient_color = {1, 1, 1, 1};
    std::array<float, 2> fog_range{};
    std::array<float, 4> fog_color = {1, 1, 1, 1};
    bool fog_enabled = false;
};

struct MatTexture {
    uint32_t slot = 0;
    uint32_t map_type = 0;
    std::array<float, 12> transform{};
    uint32_t wrap = 0;
    std::string texture;
};

struct Mat {
    uint32_t revision = 21;
    std::vector<MatTexture> textures;
    uint32_t primary_blend = 0;
    std::array<float, 4> color = {1, 1, 1, 1};
    bool use_environment = false;
    bool prelit = false;
    uint8_t z_mode = 0;
    int32_t legacy_state_0 = 0;
    uint16_t legacy_state_1 = 0;
    uint32_t tail_blend = 0;
    uint16_t legacy_state_2 = 0;
};

struct Particle {
    // GH1 revision 22 uses the pre-RB3 32-byte row: Vector3 position,
    // Color, size. The later row expands position to Vector4.
    std::array<float, 3> position{};
    std::array<float, 4> color{};
    float size = 0.0f;
};

struct ParticleSys {
    uint32_t revision = 22;
    LegacyAnimatable animatable;
    LegacyTransformable transformable;
    LegacyDrawable drawable;
    std::array<float, 2> life{};
    std::array<float, 3> box_extent_1{};
    std::array<float, 3> box_extent_2{};
    std::array<float, 2> speed{};
    std::array<float, 2> pitch{};
    std::array<float, 2> yaw{};
    std::array<float, 2> emit_rate{};
    std::array<float, 2> start_size{};
    std::array<float, 2> delta_size{};
    std::array<float, 4> start_color_low{};
    std::array<float, 4> start_color_high{};
    std::array<float, 4> end_color_low{};
    std::array<float, 4> end_color_high{};
    bool bounce_enabled = false;
    std::array<float, 4> bounce_plane{};
    std::array<float, 3> force_direction{};
    std::string material;
    uint32_t type = 0;
    float grow_ratio = 0.0f;
    float shrink_ratio = 0.0f;
    float mid_color_ratio = 0.0f;
    std::array<float, 4> mid_color_low{};
    std::array<float, 4> mid_color_high{};
    uint32_t max_particles = 0;
    std::array<float, 2> bubble_period{};
    std::array<float, 2> bubble_size{};
    bool bubble = false;
    float relative_motion = 0.0f;
    std::string emitter_mesh;
    bool preserve_particles = false;
    std::vector<Particle> particles;
};

struct MeshVertex {
    std::array<float, 3> position{};
    std::array<float, 3> normal{};
    // Revision 25 serializes Hmx::Color32 through Hmx::Color as four floats.
    // For skinned meshes PostLoad transfers these channels to bone weights.
    std::array<float, 4> color_or_weights{};
    std::array<float, 2> uv{};
};

struct MeshBoneSlot {
    std::string bone;
    std::array<float, 12> offset{};
};

struct MeshStripResult {
    std::vector<uint32_t> cumulative_strip_lengths;
    std::vector<uint16_t> strip_runs;
};

struct Mesh {
    uint32_t revision = 25;
    LegacyTransformable transformable;
    LegacyDrawable drawable;
    std::string material;
    std::string geometry_owner;
    uint32_t mutable_flags = 0;
    uint32_t volume = 0;
    bool has_bsp_tree = false;
    std::vector<MeshVertex> vertices;
    std::vector<std::array<uint16_t, 3>> faces;
    std::vector<uint8_t> patches;
    bool has_bones = false;
    std::array<MeshBoneSlot, 4> bone_slots;
    // GH1 PS2 revision 25 stores one platform strip cache per patch.
    std::vector<MeshStripResult> strip_results;
};

Morph parse_morph(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_morph(const Morph& morph);
TransAnim parse_trans_anim(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_trans_anim(const TransAnim& anim);
MultiMesh parse_multi_mesh(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_multi_mesh(const MultiMesh& multi_mesh);
MeshAnim parse_mesh_anim(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_mesh_anim(const MeshAnim& anim);
CamAnim parse_cam_anim(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_cam_anim(const CamAnim& anim);
EnvAnim parse_env_anim(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_env_anim(const EnvAnim& anim);
LightAnim parse_light_anim(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_light_anim(const LightAnim& anim);
ParticleSysAnim parse_particle_sys_anim(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_particle_sys_anim(
    const ParticleSysAnim& anim);
MatAnim parse_mat_anim(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_mat_anim(const MatAnim& anim);
Text parse_text(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_text(const Text& text);
Movie parse_movie(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_movie(const Movie& movie);
Font parse_font(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_font(const Font& font);
Tex parse_tex(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_tex(const Tex& tex);
View parse_view(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_view(const View& view);
Cam parse_cam(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_cam(const Cam& cam);
Flare parse_flare(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_flare(const Flare& flare);
Light parse_light(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_light(const Light& light);
Environ parse_environ(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_environ(const Environ& environment);
Mat parse_mat(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_mat(const Mat& mat);
ParticleSys parse_particle_sys(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_particle_sys(const ParticleSys& particles);
Mesh parse_mesh(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> serialize_mesh(const Mesh& mesh);

}  // namespace gh::milo_object
