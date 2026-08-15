#include "milo_convert.h"

#include "gh2_face_config_patch.h"
#include "gh1_character_package.h"
#include "gh1_venue_placement_conversion.h"
#include "acp.h"
#include "milo.h"
#include "milo_object.h"
#include "singer_face_track.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

uint32_t generated_block_uncompressed_limit(
    size_t payload_size) {
    uint32_t block_uncompressed_limit = 0x20000;
    constexpr size_t kFixedBlockTableSlots = 128;
    if (payload_size >
        static_cast<size_t>(block_uncompressed_limit) *
            kFixedBlockTableSlots) {
        const size_t minimum =
            (payload_size + kFixedBlockTableSlots - 1) /
            kFixedBlockTableSlots;
        block_uncompressed_limit =
            static_cast<uint32_t>(
                (minimum + 0xFFFu) & ~size_t{0xFFFu});
    }
    return block_uncompressed_limit;
}

std::vector<uint8_t> serialize_generated_milo(
    const std::vector<uint8_t>& payload) {
    return gh::milo::serialize_container(
        gh::milo::make_container(
            payload,
            gh::milo::BlockStructure::MILO_B,
            generated_block_uncompressed_limit(payload.size())));
}

std::string channel_base_name(const std::string& channel) {
    const size_t dot = channel.rfind('.');
    return dot == std::string::npos
               ? channel
               : channel.substr(0, dot);
}

std::string channel_suffix(const std::string& channel) {
    const size_t dot = channel.rfind('.');
    return dot == std::string::npos
               ? std::string()
               : channel.substr(dot);
}

gh::milo::Entry make_entry(
    std::string type, std::string name,
    std::vector<uint8_t> body) {
    gh::milo::Entry entry;
    entry.type = std::move(type);
    entry.name = std::move(name);
    entry.body_bytes = std::move(body);
    entry.size = entry.body_bytes.size();
    entry.terminator_value = 0xDEADDEADu;
    return entry;
}

void set_identity(std::array<float, 12>& transform) {
    transform =
        {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
}

std::string character_model_relative_ref(const std::string& path) {
    constexpr const char* kCharPrefix = "char/";
    if (path.rfind(kCharPrefix, 0) == 0) {
        return "../../../" + path.substr(std::strlen(kCharPrefix));
    }
    return path;
}

void append_guitarist_runtime_graph(
    gh::milo::Directory& directory,
    const std::string& main_anim,
    const std::string& strum_anim,
    const std::string& fret_anim) {
    gh::milo_object::CharServoBone2 servo;
    directory.entries.push_back(make_entry(
        "CharServoBone", "bone.servo",
        gh::milo_object::serialize_char_servo_bone2(servo)));

    gh::milo_object::CharDriver3 main_driver;
    main_driver.bones = "bone.servo";
    main_driver.clips = character_model_relative_ref(main_anim);
    directory.entries.push_back(make_entry(
        "CharDriver", "main.drv",
        gh::milo_object::serialize_char_driver3(main_driver)));

    gh::milo_object::CharDriverMidi3 left_driver;
    left_driver.driver.bones = "bone.servo";
    left_driver.driver.clips = character_model_relative_ref(fret_anim);
    directory.entries.push_back(make_entry(
        "CharDriverMidi", "left_hand.drv",
        gh::milo_object::serialize_char_driver_midi3(left_driver)));

    gh::milo_object::CharDriverMidi3 right_driver;
    right_driver.driver.bones = "bone.servo";
    right_driver.driver.clips = character_model_relative_ref(strum_anim);
    directory.entries.push_back(make_entry(
        "CharDriverMidi", "right_hand.drv",
        gh::milo_object::serialize_char_driver_midi3(right_driver)));

    gh::milo_object::CharIKMidi4 fret_midi;
    fret_midi.bone = "bone_fret.mesh";
    directory.entries.push_back(make_entry(
        "CharIKMidi", "fret.ik",
        gh::milo_object::serialize_char_ik_midi4(fret_midi)));

    gh::milo_object::CharIKHand2 left_hand;
    left_hand.hand = "bone_L-hand";
    left_hand.target = "bone_fret_hand.mesh";
    left_hand.orientation = true;
    left_hand.stretch = true;
    directory.entries.push_back(make_entry(
        "CharIKHand", "left_hand.ik",
        gh::milo_object::serialize_char_ik_hand2(left_hand)));

    gh::milo_object::CharIKHand2 right_hand;
    right_hand.hand = "bone_R-hand";
    right_hand.target = "bone_strum_hand.mesh";
    right_hand.orientation = true;
    right_hand.stretch = true;
    directory.entries.push_back(make_entry(
        "CharIKHand", "right_hand.ik",
        gh::milo_object::serialize_char_ik_hand2(right_hand)));

    gh::milo_object::OutfitLoader1 outfit;
    outfit.object_fields.type = "guitar";
    outfit.directory = "../../../og";
    gh::milo_object::OutfitLoaderCategory1 category;
    category.outfits.resize(58);
    outfit.categories.push_back(std::move(category));
    directory.entries.push_back(make_entry(
        "OutfitLoader", "guitar.outfit",
        gh::milo_object::serialize_outfit_loader1(outfit)));

    const auto append_weight = [&](const char* name, uint32_t flags) {
        gh::milo_object::CharWeightSetter2 weight;
        weight.weightable.weight = 1.0f;
        weight.weightable.weight_owner = name;
        weight.driver = "main.drv";
        weight.flags = flags;
        directory.entries.push_back(make_entry(
            "CharWeightSetter", name,
            gh::milo_object::serialize_char_weight_setter2(weight)));
    };
    append_weight("left.weight", 0x00400000u);
    append_weight("right.weight", 0x00800000u);
}

std::array<float, 12> invert_affine_transform(
    const std::array<float, 12>& value) {
    const float determinant =
        value[0] * (value[4] * value[8] -
                    value[5] * value[7]) -
        value[1] * (value[3] * value[8] -
                    value[5] * value[6]) +
        value[2] * (value[3] * value[7] -
                    value[4] * value[6]);
    if (std::abs(determinant) <=
        std::numeric_limits<float>::epsilon())
        throw std::runtime_error(
            "meshbundle: cannot invert singular bone bind world");
    const float inverse_determinant = 1.0f / determinant;
    std::array<float, 12> result{};
    result[0] = (value[4] * value[8] -
                 value[5] * value[7]) * inverse_determinant;
    result[1] = (value[2] * value[7] -
                 value[1] * value[8]) * inverse_determinant;
    result[2] = (value[1] * value[5] -
                 value[2] * value[4]) * inverse_determinant;
    result[3] = (value[5] * value[6] -
                 value[3] * value[8]) * inverse_determinant;
    result[4] = (value[0] * value[8] -
                 value[2] * value[6]) * inverse_determinant;
    result[5] = (value[2] * value[3] -
                 value[0] * value[5]) * inverse_determinant;
    result[6] = (value[3] * value[7] -
                 value[4] * value[6]) * inverse_determinant;
    result[7] = (value[1] * value[6] -
                 value[0] * value[7]) * inverse_determinant;
    result[8] = (value[0] * value[4] -
                 value[1] * value[3]) * inverse_determinant;
    for (size_t c = 0; c < 3; ++c)
        result[9 + c] = -(
            value[9] * result[c] +
            value[10] * result[3 + c] +
            value[11] * result[6 + c]);
    return result;
}

struct BinaryCursor {
    explicit BinaryCursor(std::vector<uint8_t> bytes)
        : bytes(std::move(bytes)) {}

    std::vector<uint8_t> bytes;
    size_t offset = 0;

    void require(size_t count, const char* field) {
        if (count > bytes.size() || offset > bytes.size() - count)
            throw std::runtime_error(
                std::string("meshbundle: truncated ") + field);
    }

    uint16_t u16(const char* field) {
        require(2, field);
        const uint16_t value =
            static_cast<uint16_t>(bytes[offset]) |
            (static_cast<uint16_t>(bytes[offset + 1]) << 8);
        offset += 2;
        return value;
    }

    uint8_t u8(const char* field) {
        require(1, field);
        return bytes[offset++];
    }

    uint32_t u32(const char* field) {
        require(4, field);
        const uint32_t value =
            static_cast<uint32_t>(bytes[offset]) |
            (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
            (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
            (static_cast<uint32_t>(bytes[offset + 3]) << 24);
        offset += 4;
        return value;
    }

    float f32(const char* field) {
        const uint32_t raw = u32(field);
        float value = 0.0f;
        static_assert(sizeof(raw) == sizeof(value));
        std::memcpy(&value, &raw, sizeof(value));
        return value;
    }

    std::string string(const char* field) {
        const uint32_t size = u32(field);
        require(size, field);
        std::string value(
            reinterpret_cast<const char*>(bytes.data() + offset),
            reinterpret_cast<const char*>(bytes.data() + offset + size));
        offset += size;
        return value;
    }

    std::vector<uint8_t> byte_vector(const char* field) {
        const uint32_t size = u32(field);
        require(size, field);
        std::vector<uint8_t> value(
            bytes.begin() + static_cast<std::ptrdiff_t>(offset),
            bytes.begin() + static_cast<std::ptrdiff_t>(offset + size));
        offset += size;
        return value;
    }
};

struct MeshBundleChunk {
    gh::milo_object::Mesh28 mesh;
    std::string name;
    std::string material;
    std::string texture;
};

struct MeshBundle {
    struct BoneTransform {
        std::string source_name;
        std::string parent_name;
        std::array<float, 12> local{};
        std::array<float, 12> world{};
    };

    struct Texture {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t bits_per_pixel = 0;
        gh::milo_object::HmxBitmap bitmap;
    };

    std::string outfit;
    std::string package_name;
    std::map<std::string, BoneTransform> bone_transforms;
    std::map<std::string, Texture> textures;
    std::vector<MeshBundleChunk> chunks;
};

MeshBundle parse_meshbundle(const fs::path& path) {
    constexpr std::array<uint8_t, 8> kMagic =
        {'G', 'H', '3', 'M', '2', 'M', 'B', 0};
    BinaryCursor cursor(gh::milo::read_file(path.string()));
    cursor.require(kMagic.size(), "magic");
    for (uint8_t expected : kMagic) {
        if (cursor.bytes[cursor.offset++] != expected)
            throw std::runtime_error("meshbundle: bad magic");
    }
    const uint32_t version = cursor.u32("version");
    if (version != 1 && version != 2 && version != 3 && version != 4)
        throw std::runtime_error("meshbundle: unsupported version");
    MeshBundle bundle;
    bundle.outfit = cursor.string("outfit");
    bundle.package_name = cursor.string("package");
    if (version >= 2) {
        const uint32_t bone_transform_count =
            cursor.u32("bone transform count");
        if (bone_transform_count > 10000)
            throw std::runtime_error(
                "meshbundle: implausible bone transform count");
        for (uint32_t i = 0; i < bone_transform_count; ++i) {
            const std::string name = cursor.string("bone transform name");
            MeshBundle::BoneTransform transform;
            transform.source_name = cursor.string("bone source name");
            if (version >= 4)
                transform.parent_name = cursor.string("bone parent name");
            for (float& value : transform.local)
                value = cursor.f32("bone local");
            for (float& value : transform.world)
                value = cursor.f32("bone world");
            if (!bundle.bone_transforms.emplace(name, transform).second)
                throw std::runtime_error(
                    "meshbundle: duplicate bone transform " + name);
        }
    }
    if (version >= 3) {
        const uint32_t texture_count = cursor.u32("texture count");
        if (texture_count > 10000)
            throw std::runtime_error(
                "meshbundle: implausible texture count");
        for (uint32_t i = 0; i < texture_count; ++i) {
            const std::string name = cursor.string("texture name");
            MeshBundle::Texture texture;
            texture.width = cursor.u32("texture width");
            texture.height = cursor.u32("texture height");
            texture.bits_per_pixel = cursor.u32("texture bits per pixel");
            texture.bitmap.header_kind = cursor.u8("bitmap header kind");
            texture.bitmap.bits_per_pixel =
                cursor.u8("bitmap bits per pixel");
            texture.bitmap.encoding =
                static_cast<int32_t>(cursor.u32("bitmap encoding"));
            texture.bitmap.mipmap_count = cursor.u8("bitmap mip count");
            texture.bitmap.width = cursor.u16("bitmap width");
            texture.bitmap.height = cursor.u16("bitmap height");
            texture.bitmap.bytes_per_line =
                cursor.u16("bitmap bytes per line");
            texture.bitmap.wii_alpha = cursor.u16("bitmap wii alpha");
            texture.bitmap.data = cursor.byte_vector("bitmap data");
            if (texture.width == 0 || texture.height == 0 ||
                texture.bitmap.width != texture.width ||
                texture.bitmap.height != texture.height)
                throw std::runtime_error(
                    "meshbundle: invalid texture dimensions " + name);
            if (!bundle.textures.emplace(name, std::move(texture)).second)
                throw std::runtime_error(
                    "meshbundle: duplicate texture " + name);
        }
    }
    const uint32_t chunk_count = cursor.u32("chunk count");
    if (chunk_count > 10000)
        throw std::runtime_error("meshbundle: implausible chunk count");
    bundle.chunks.reserve(chunk_count);
    for (uint32_t chunk_index = 0; chunk_index < chunk_count;
         ++chunk_index) {
        MeshBundleChunk chunk;
        chunk.name = cursor.string("mesh name");
        chunk.material = cursor.string("material");
        chunk.texture = cursor.string("texture");
        chunk.mesh.bsp_nodes.push_back({});
        set_identity(chunk.mesh.transformable.local);
        set_identity(chunk.mesh.transformable.world);
        chunk.mesh.material = chunk.material;
        chunk.mesh.drawable.sphere = {
            cursor.f32("sphere x"),
            cursor.f32("sphere y"),
            cursor.f32("sphere z"),
            cursor.f32("sphere r")};
        const uint32_t bone_count = cursor.u32("bone count");
        if (bone_count == 0 || bone_count > chunk.mesh.bone_slots.size())
            throw std::runtime_error(
                "meshbundle: invalid Mesh28 bone slot count");
        chunk.mesh.has_bones = true;
        for (uint32_t i = 0; i < bone_count; ++i) {
            chunk.mesh.bone_slots[i].bone = cursor.string("bone");
            const auto transform =
                bundle.bone_transforms.find(chunk.mesh.bone_slots[i].bone);
            if (transform == bundle.bone_transforms.end())
                throw std::runtime_error(
                    "meshbundle: missing bind transform for bone " +
                    chunk.mesh.bone_slots[i].bone);
            chunk.mesh.bone_slots[i].offset =
                invert_affine_transform(transform->second.world);
        }
        for (uint32_t i = bone_count; i < chunk.mesh.bone_slots.size(); ++i)
            set_identity(chunk.mesh.bone_slots[i].offset);
        const uint32_t vertex_count = cursor.u32("vertex count");
        if (vertex_count > 1000000)
            throw std::runtime_error(
                "meshbundle: implausible vertex count");
        chunk.mesh.vertices.reserve(vertex_count);
        for (uint32_t vertex_index = 0; vertex_index < vertex_count;
             ++vertex_index) {
            gh::milo_object::MeshVertex vertex;
            for (float& value : vertex.position)
                value = cursor.f32("position");
            for (float& value : vertex.normal)
                value = cursor.f32("normal");
            for (float& value : vertex.color_or_weights)
                value = cursor.f32("weights");
            for (float& value : vertex.uv)
                value = cursor.f32("uv");
            chunk.mesh.vertices.push_back(vertex);
        }
        const uint32_t face_count = cursor.u32("face count");
        if (face_count > 1000000)
            throw std::runtime_error(
                "meshbundle: implausible face count");
        chunk.mesh.faces.reserve(face_count);
        for (uint32_t face_index = 0; face_index < face_count;
             ++face_index) {
            chunk.mesh.faces.push_back({
                cursor.u16("face a"),
                cursor.u16("face b"),
                cursor.u16("face c")});
        }
        bundle.chunks.push_back(std::move(chunk));
    }
    if (cursor.offset != cursor.bytes.size())
        throw std::runtime_error("meshbundle: trailing bytes");
    return bundle;
}

struct ChannelContext {
    bool position = false;
    bool scale = false;
    bool quat = false;
    bool rotx = false;
    bool roty = false;
    bool rotz = false;
};

void add_channel_context(
    std::map<std::string, ChannelContext>& contexts,
    const std::string& channel) {
    const std::string base = channel_base_name(channel);
    const std::string type = channel_suffix(channel);
    ChannelContext& context = contexts[base];
    if (type == ".pos") context.position = true;
    else if (type == ".scale") context.scale = true;
    else if (type == ".quat") context.quat = true;
    else if (type == ".rotx" || type == ".drotx")
        context.rotx = true;
    else if (type == ".roty" || type == ".droty")
        context.roty = true;
    else if (type == ".rotz" || type == ".drotz")
        context.rotz = true;
    else
        throw std::runtime_error(
            "unsupported character channel " + channel);
}

int32_t rotation_context(const ChannelContext& context) {
    const int count =
        (context.quat ? 1 : 0) +
        (context.rotx ? 1 : 0) +
        (context.roty ? 1 : 0) +
        (context.rotz ? 1 : 0);
    if (count > 1)
        throw std::runtime_error(
            "multiple rotation encodings for one bone");
    if (context.quat) return 2;
    if (context.rotx) return 3;
    if (context.roty) return 4;
    if (context.rotz) return 5;
    return 9;
}

std::vector<gh::milo_object::ObjectDirViewport16>
standard_clip_set_viewports() {
    constexpr int32_t kLegacyViewportValue = 5583746;
    const std::array<std::array<float, 12>, 7> transforms = {{
        {0.70710677f, -0.70710677f, 0.0f,
         0.57735026f, 0.57735026f, -0.57735026f,
         0.40824828f, 0.40824828f, 0.81649655f,
         -443.405f, -443.405f, 443.405f},
        {0, -1, 0, 1, 0, 0, 0, 0, 1, -768, 0, 0},
        {0, 1, 0, -1, 0, 0, 0, 0, 1, 768, 0, 0},
        {1, 0, 0, 0, 0, -1, 0, 1, 0, 0, 0, 768},
        {1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, -768},
        {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -768, 0},
        {-1, 0, 0, 0, -1, 0, 0, 0, 1, 0, 768, 0},
    }};
    std::vector<gh::milo_object::ObjectDirViewport16> result;
    for (const auto& transform : transforms)
        result.push_back(
            {transform, kLegacyViewportValue});
    return result;
}

gh::milo_convert::Gh2ClipSetRole parse_clipset_role(
    const std::string& role) {
    using gh::milo_convert::Gh2ClipSetRole;
    if (role == "guitar-main") return Gh2ClipSetRole::GuitarMain;
    if (role == "guitar-ui") return Gh2ClipSetRole::GuitarUi;
    if (role == "guitar-fret") return Gh2ClipSetRole::GuitarFret;
    if (role == "guitar-strum") return Gh2ClipSetRole::GuitarStrum;
    if (role == "band") return Gh2ClipSetRole::Band;
    if (role == "crowd") return Gh2ClipSetRole::Crowd;
    if (role == "generic") return Gh2ClipSetRole::Generic;
    throw std::runtime_error("unknown clipset role: " + role);
}

std::pair<std::string, int32_t> clipset_legacy_type(
    gh::milo_convert::Gh2ClipSetRole role) {
    using gh::milo_convert::Gh2ClipSetRole;
    switch (role) {
        case Gh2ClipSetRole::GuitarMain:
            return {"guitarist", 15};
        case Gh2ClipSetRole::GuitarUi:
            return {"guitarist_ui", 2};
        case Gh2ClipSetRole::GuitarStrum:
            return {"guitarist_strum", 1};
        case Gh2ClipSetRole::Band:
            return {"band", 4};
        case Gh2ClipSetRole::Crowd:
            return {"crowd", 1};
        case Gh2ClipSetRole::GuitarFret:
        case Gh2ClipSetRole::Generic:
            return {"", -1};
    }
    throw std::runtime_error("unknown clipset role");
}

uint32_t root_play_flags_for_role(
    gh::milo_convert::Gh2ClipSetRole role,
    const std::vector<gh::acp::File>& clips) {
    using gh::milo_convert::Gh2ClipSetRole;
    if (role == Gh2ClipSetRole::GuitarFret)
        return 32;
    if (!clips.empty())
        return gh::milo_convert::convert_gh1_clip_time_flags_to_gh2(
            clips.front().play_flags);
    return 0;
}

std::vector<std::pair<std::string, uint32_t>>
guitar_group_masks_for_generated_clipsets() {
    return {
        {"walk_turn", 0x00000020u},
        {"walk_stop", 0x00000040u},
        {"walk_walk", 0x00000080u},
        {"bad", 0x00004000u},
        {"extreme", 0x00008000u},
        {"idle", 0x00010000u},
        {"intro", 0x00020000u},
        {"lose", 0x00040000u},
        {"normal", 0x00080000u},
        {"solo", 0x00100000u},
        {"win", 0x00200000u},
        {"star_power", 0x02000000u},
        {"win_finals", 0x04000000u},
    };
}

void add_generated_guitar_clip_groups(
    gh::milo::Directory& directory,
    const std::vector<gh::acp::File>& sources) {
    for (const auto& [name, mask] :
         guitar_group_masks_for_generated_clipsets()) {
        gh::milo_object::CharClipGroup1 group;
        for (const auto& source : sources) {
            if ((source.flags & mask) != 0)
                group.clips.push_back(source.object_name);
        }
        if (group.clips.empty()) continue;
        group.which = 0;
        directory.entries.push_back(make_entry(
            "CharClipGroup", name,
            gh::milo_object::serialize_char_clip_group1(group)));
    }
}

void usage() {
    std::cerr
        << "Usage:\n"
        << "  milo_convert_tool convert <GH1.rnd_ps2> --name <dir-name> "
           "--out <GH2.milo_ps2> --manifest <manifest.tsv>\n"
        << "  milo_convert_tool build-clipset-from-acp <acp-dir> "
           "--name <dir-name> --role <role> --out <GH2.milo_ps2>\n"
        << "  milo_convert_tool build-character-from-meshbundle "
           "<meshbundle> --name <dir-name> --out <GH2.milo_ps2> "
           "[--main-anim <milo>] [--strum-anim <milo>] "
           "[--fret-anim <milo>]\n"
        << "  milo_convert_tool inspect-clipset <GH2.milo_ps2> "
           "[--channels] [--events]\n"
        << "  milo_convert_tool sample-clip <GH2.milo_ps2> "
           "<clip> <sample-index> [channel-filter]\n"
        << "  milo_convert_tool inspect-character <GH2.milo_ps2> "
           "[--controllers] [--transforms]\n"
        << "  milo_convert_tool inspect-groups <GH2.milo_ps2>\n"
        << "  milo_convert_tool inspect-skeleton <GH1.rnd_ps2> [--all]\n"
        << "  milo_convert_tool extract-entry <MILO> <type> <name> "
            "--out <object-body>\n"
        << "  milo_convert_tool rebuild-venue-waypoints <bundle-dir> "
           "<venue> --out <venue_chars.milo_ps2>\n"
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
    if (command == "rebuild-venue-waypoints") {
        try {
            if (argc != 6 || std::string(argv[4]) != "--out")
                usage();
            const std::string venue = argv[3];
            const fs::path venue_dir =
                input / "world" / venue / "gen";
            if (!fs::is_directory(venue_dir))
                throw std::runtime_error(
                    "venue directory not found: " +
                    venue_dir.string());
            std::vector<gh::milo::Directory> sections;
            for (const auto& item :
                 fs::directory_iterator(venue_dir)) {
                if (!item.is_regular_file() ||
                    item.path().extension() != ".milo_ps2" ||
                    item.path().filename() ==
                        venue + "_chars.milo_ps2")
                    continue;
                const auto container = gh::milo::parse_container(
                    gh::milo::read_file(item.path().string()));
                sections.push_back(gh::milo::parse_directory(
                    gh::milo::container_payload(container)));
            }
            const auto converted =
                gh::milo_convert::
                    convert_gh1_venue_spots_to_gh2_waypoints(
                        venue, sections);
            const auto payload = gh::milo::serialize_directory(
                converted.characters_directory);
            const auto bytes = gh::milo::serialize_container(
                gh::milo::make_container(payload));
            write_file(argv[5], bytes);
            std::cout
                << "venue=" << venue
                << " sections=" << sections.size()
                << " waypoints=" << converted.waypoints
                << " bytes=" << bytes.size()
                << " output=" << argv[5] << '\n';
            return 0;
        } catch (const std::exception& ex) {
            std::cerr
                << "milo_convert_tool: " << ex.what() << "\n";
            return 2;
        }
    }
    if (command == "extract-entry") {
        try {
            if (argc != 7 || std::string(argv[5]) != "--out")
                usage();
            const auto container = gh::milo::parse_container(
                gh::milo::read_file(input.string()));
            const auto directory = gh::milo::parse_directory(
                gh::milo::container_payload(container));
            const std::string type = argv[3];
            const std::string name = argv[4];
            const auto found = std::find_if(
                directory.entries.begin(), directory.entries.end(),
                [&](const gh::milo::Entry& entry) {
                    return entry.type == type && entry.name == name;
                });
            if (found == directory.entries.end())
                throw std::runtime_error(
                    "entry not found: " + type + " " + name);
            write_file(argv[6], found->body_bytes);
            std::cout << "type=" << found->type
                      << " name=" << found->name
                      << " bytes=" << found->body_bytes.size()
                      << " output=" << argv[6] << '\n';
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "milo_convert_tool: " << ex.what() << "\n";
            return 2;
        }
    }
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
        bool print_controllers = false;
        bool print_transforms = false;
        for (int arg_index = 3; arg_index < argc; ++arg_index) {
            const std::string option = argv[arg_index];
            if (option == "--controllers") {
                print_controllers = true;
            } else if (option == "--transforms") {
                print_transforms = true;
            } else {
                usage();
            }
        }
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
            if (print_transforms) {
                const auto print_transform =
                    [](const char* type, const std::string& name,
                       const gh::milo_object::Transformable9& transform) {
                        std::cout << type << ' ' << name
                                  << " parent=" << transform.parent
                                  << " local=[";
                        for (size_t value_index = 0;
                             value_index < transform.local.size();
                             ++value_index) {
                            if (value_index) std::cout << ',';
                            std::cout << transform.local[value_index];
                        }
                        std::cout << "] world=[";
                        for (size_t value_index = 0;
                             value_index < transform.world.size();
                             ++value_index) {
                            if (value_index) std::cout << ',';
                            std::cout << transform.world[value_index];
                        }
                        std::cout << "]\n";
                    };
                for (const auto& entry : directory.entries) {
                    if (entry.type == "Trans") {
                        const auto transform =
                            gh::milo_object::parse_trans9(entry.body_bytes);
                        gh::milo_object::Transformable9 fields;
                        fields.revision = transform.revision;
                        fields.local = transform.local;
                        fields.world = transform.world;
                        fields.constraint = transform.constraint;
                        fields.target = transform.target;
                        fields.preserve_scale = transform.preserve_scale;
                        fields.parent = transform.parent;
                        print_transform(
                            entry.type.c_str(), entry.name, fields);
                    } else if (entry.type == "Mesh") {
                        const auto mesh = gh::milo_object::parse_mesh28(
                            entry.body_bytes,
                            static_cast<uint32_t>(directory.dir_version));
                        print_transform(
                            entry.type.c_str(), entry.name,
                            mesh.transformable);
                    }
                }
            }
            for (const auto& entry : directory.entries) {
                if (entry.type == "Tex") {
                    const auto tex =
                        gh::milo_object::parse_tex10(entry.body_bytes);
                    std::cout << "Tex " << entry.name
                              << " width=" << tex.width
                              << " height=" << tex.height
                              << " bpp=" << tex.bits_per_pixel
                              << " use_external=" << tex.use_external
                              << " has_bitmap=" << tex.has_bitmap
                              << " bitmap_bpp="
                              << static_cast<int>(
                                     tex.bitmap.bits_per_pixel)
                              << " bitmap_encoding="
                              << tex.bitmap.encoding
                              << " bitmap_bytes="
                              << tex.bitmap.data.size()
                              << " external_path="
                              << tex.external_path << '\n';
                }
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
    if (command == "sample-clip") {
        if (argc != 5 && argc != 6) usage();
        const std::string clip_name = argv[3];
        const uint32_t sample_index =
            static_cast<uint32_t>(std::stoul(argv[4]));
        const std::string filter = argc == 6 ? argv[5] : std::string();
        try {
            const auto container = gh::milo::parse_container(
                gh::milo::read_file(input.string()));
            const auto directory = gh::milo::parse_directory(
                gh::milo::container_payload(container));
            const gh::milo::Entry* entry = nullptr;
            for (const auto& candidate : directory.entries) {
                if (candidate.type == "CharClipSamples" &&
                    candidate.name == clip_name) {
                    entry = &candidate;
                    break;
                }
            }
            if (!entry)
                throw std::runtime_error(
                    "CharClipSamples not found: " + clip_name);
            const auto body =
                gh::milo_object::parse_char_clip_samples10(
                    entry->body_bytes);
            auto print_set =
                [&](const char* label,
                    const gh::milo_object::CharBonesSamples10& source,
                    uint32_t requested_sample) {
                    if (source.channels.empty()) return;
                    gh::acp::ChannelSet set;
                    set.channels = source.channels;
                    set.sample_count = source.sample_count;
                    set.compression = source.compression;
                    set.sample_bytes = source.sample_bytes;
                    for (const auto& channel : set.channels)
                        set.frame_size += gh::acp::channel_file_size(
                            channel, set.compression);
                    for (size_t index = 0;
                         index < set.channels.size(); ++index) {
                        if (!filter.empty() &&
                            set.channels[index].find(filter) ==
                                std::string::npos) {
                            continue;
                        }
                        const auto sample =
                            gh::acp::decode_channel_sample(
                                set, index, requested_sample);
                        std::cout << label << "\tsample="
                                  << requested_sample << "\t"
                                  << set.channels[index];
                        for (size_t component = 0;
                             component < sample.component_count;
                             ++component) {
                            std::cout
                                << (component == 0 ? "\t" : ",")
                                << sample.values[component];
                        }
                        std::cout << '\n';
                    }
                };
            print_set("full", body.full, sample_index);
            print_set("one", body.one, 0);
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "milo_convert_tool: " << ex.what() << "\n";
            return 2;
        }
    }
    if (command == "inspect-clipset") {
        bool print_channels = false;
        bool print_events = false;
        for (int index = 3; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--channels")
                print_channels = true;
            else if (argument == "--events")
                print_events = true;
            else
                usage();
        }
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
                          << "\tevents=" << body.events.size()
                          << "\tenter=" << body.legacy_enter_event
                          << "\texit=" << body.legacy_exit_event
                          << '\n';
                if (print_events) {
                    for (const auto& event : body.events)
                        std::cout << "event\t" << clip.clip
                                  << "\t" << event.frame
                                  << "\t" << event.script << '\n';
                }
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
    if (command == "build-clipset-from-acp") {
        try {
            fs::path output;
            std::string name;
            std::string role_name;
            for (int i = 3; i < argc; ++i) {
                const std::string arg = argv[i];
                if (arg == "--name" && i + 1 < argc) name = argv[++i];
                else if (arg == "--role" && i + 1 < argc)
                    role_name = argv[++i];
                else if (arg == "--out" && i + 1 < argc)
                    output = argv[++i];
                else usage();
            }
            if (output.empty() || name.empty() || role_name.empty())
                usage();
            if (!fs::is_directory(input))
                throw std::runtime_error(
                    "ACP directory not found: " + input.string());
            const auto role = parse_clipset_role(role_name);
            std::vector<fs::path> paths;
            for (const auto& item : fs::directory_iterator(input)) {
                if (item.is_regular_file() &&
                    item.path().extension() == ".acp")
                    paths.push_back(item.path());
            }
            std::sort(paths.begin(), paths.end());
            if (paths.empty())
                throw std::runtime_error(
                    "ACP directory has no .acp files: " +
                    input.string());

            std::vector<gh::acp::File> sources;
            sources.reserve(paths.size());
            for (const auto& path : paths)
                sources.push_back(
                    gh::acp::parse(gh::acp::read_file(path.string())));

            gh::milo::Directory directory;
            directory.dir_version = 24;
            directory.dir_type = "CharClipSet";
            directory.dir_name = name;
            directory.boundaries_exact = true;
            directory.dir_terminator_value = 0xDEADDEADu;

            gh::milo_object::CharClipSet14 root;
            root.object_directory.object_fields.type =
                clipset_legacy_type(role).first;
            root.object_directory.viewports =
                standard_clip_set_viewports();
            root.blend_width = sources.front().blend_width;
            root.play_flags = root_play_flags_for_role(role, sources);
            root.move_self =
                role == gh::milo_convert::Gh2ClipSetRole::GuitarMain;
            const auto [legacy_type, legacy_type_version] =
                clipset_legacy_type(role);
            root.legacy_type = legacy_type;
            root.legacy_type_version = legacy_type_version;

            std::map<std::string, ChannelContext> contexts;
            for (const auto& source : sources) {
                const auto clip =
                    gh::milo_convert::
                        convert_gh1_acp_to_gh2_char_clip_samples10(
                            source);
                for (const auto& channel : clip.full.channels)
                    add_channel_context(contexts, channel);
                for (const auto& channel : clip.one.channels)
                    add_channel_context(contexts, channel);
                const auto body =
                    gh::milo_object::
                        serialize_char_clip_samples10(clip);
                const uint32_t allocation_size =
                    static_cast<uint32_t>(
                        gh::milo_object::
                            char_clip_samples10_ps2_allocate_size(
                                clip));
                directory.entries.push_back(make_entry(
                    "CharClipSamples", source.object_name, body));
                root.clips.push_back(
                    {source.object_name, clip.flags, allocation_size});
            }

            if (role == gh::milo_convert::Gh2ClipSetRole::GuitarMain)
                add_generated_guitar_clip_groups(directory, sources);

            gh::milo_object::CharClipFilter0 filter;
            directory.entries.push_back(make_entry(
                "CharClipFilter", "clip_filter.ccf",
                gh::milo_object::serialize_char_clip_filter0(filter)));

            const std::string bone_extension =
                role == gh::milo_convert::Gh2ClipSetRole::GuitarFret
                    ? ".mesh"
                    : ".trans";
            for (const auto& [base, context] : contexts) {
                gh::milo_object::CharBone2 bone;
                set_identity(bone.legacy_transform.local);
                set_identity(bone.legacy_transform.world);
                bone.position_context = context.position;
                bone.scale_context = context.scale;
                bone.rotation = rotation_context(context);
                bone.legacy_rotation = 9;
                directory.entries.push_back(make_entry(
                    "CharBone", base + bone_extension,
                    gh::milo_object::serialize_char_bone2(bone)));
            }

            directory.dir_body_bytes =
                gh::milo_object::serialize_char_clip_set14(root);
            const auto target_payload =
                gh::milo::serialize_directory(directory);
            const auto verify_directory =
                gh::milo::parse_directory(target_payload);
            if (!verify_directory.boundaries_exact ||
                gh::milo::serialize_directory(verify_directory) !=
                    target_payload)
                throw std::runtime_error(
                    "generated CharClipSet payload round trip differs");
            const auto target_bytes =
                serialize_generated_milo(target_payload);
            const auto verify_container =
                gh::milo::parse_container(target_bytes);
            const auto verify_container_directory =
                gh::milo::parse_directory(
                    gh::milo::container_payload(verify_container));
            if (!verify_container_directory.boundaries_exact ||
                gh::milo::serialize_directory(
                    verify_container_directory) != target_payload)
                throw std::runtime_error(
                    "generated CharClipSet container round trip differs");
            write_file(output, target_bytes);
            std::cout
                << "clipset=" << name
                << " role="
                << gh::milo_convert::gh2_clip_set_role_name(role)
                << " clips=" << sources.size()
                << " bones=" << contexts.size()
                << " entries=" << directory.entries.size()
                << " bytes=" << target_bytes.size()
                << " output=" << output.string() << '\n';
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "milo_convert_tool: " << ex.what() << "\n";
            return 2;
        }
    }
    if (command == "build-character-from-meshbundle") {
        try {
            fs::path output;
            std::string name;
            std::string main_anim;
            std::string strum_anim;
            std::string fret_anim;
            for (int i = 3; i < argc; ++i) {
                const std::string arg = argv[i];
                if (arg == "--name" && i + 1 < argc) name = argv[++i];
                else if (arg == "--out" && i + 1 < argc)
                    output = argv[++i];
                else if (arg == "--main-anim" && i + 1 < argc)
                    main_anim = argv[++i];
                else if (arg == "--strum-anim" && i + 1 < argc)
                    strum_anim = argv[++i];
                else if (arg == "--fret-anim" && i + 1 < argc)
                    fret_anim = argv[++i];
                else usage();
            }
            if (output.empty() || name.empty()) usage();
            const bool has_guitarist_graph =
                !main_anim.empty() || !strum_anim.empty() ||
                !fret_anim.empty();
            if (has_guitarist_graph &&
                (main_anim.empty() || strum_anim.empty() ||
                 fret_anim.empty())) {
                throw std::runtime_error(
                    "build-character-from-meshbundle: guitarist graph "
                    "requires --main-anim, --strum-anim, and --fret-anim");
            }

            MeshBundle bundle = parse_meshbundle(input);
            if (bundle.chunks.empty())
                throw std::runtime_error(
                    "meshbundle has no Mesh28 chunks");

            gh::milo::Directory directory;
            directory.dir_version = 24;
            directory.dir_type =
                has_guitarist_graph ? "BandCharacter" : "Character";
            directory.dir_name = name;
            directory.boundaries_exact = true;
            directory.dir_terminator_value = 0xDEADDEADu;

            std::set<std::string> material_names;
            std::set<std::string> texture_names;
            std::set<std::string> bone_names;
            std::vector<std::string> mesh_names;
            std::array<float, 4> aggregate_sphere = {0, 0, 0, 0};
            bool have_sphere = false;
            for (auto& chunk : bundle.chunks) {
                material_names.insert(chunk.material);
                texture_names.insert(chunk.texture);
                mesh_names.push_back(chunk.name);
                if (!have_sphere) {
                    aggregate_sphere = chunk.mesh.drawable.sphere;
                    have_sphere = true;
                } else {
                    aggregate_sphere[3] = std::max(
                        aggregate_sphere[3],
                        chunk.mesh.drawable.sphere[3]);
                }
                for (const auto& slot : chunk.mesh.bone_slots) {
                    if (!slot.bone.empty())
                        bone_names.insert(slot.bone);
                }
            }
            for (const auto& [bone_name, transform] :
                 bundle.bone_transforms) {
                (void)transform;
                if (!bone_name.empty())
                    bone_names.insert(bone_name);
            }

            for (const auto& texture_name : texture_names) {
                gh::milo_object::Tex10 tex;
                const auto texture =
                    bundle.textures.find(texture_name);
                if (texture != bundle.textures.end()) {
                    tex.width = texture->second.width;
                    tex.height = texture->second.height;
                    tex.bits_per_pixel = texture->second.bits_per_pixel;
                    tex.has_bitmap = true;
                    tex.bitmap = texture->second.bitmap;
                    tex.use_external = false;
                } else {
                    tex.width = 512;
                    tex.height = 512;
                    tex.bits_per_pixel = 32;
                    tex.external_path =
                        "textures/" + texture_name + ".png";
                    tex.use_external = true;
                }
                directory.entries.push_back(make_entry(
                    "Tex", texture_name,
                    gh::milo_object::serialize_tex10(tex)));
            }
            for (const auto& material_name : material_names) {
                gh::milo_object::Mat27 mat;
                mat.use_environment = false;
                mat.prelit = true;
                mat.diffuse_texture.clear();
                for (const auto& chunk : bundle.chunks) {
                    if (chunk.material == material_name) {
                        mat.diffuse_texture = chunk.texture;
                        break;
                    }
                }
                directory.entries.push_back(make_entry(
                    "Mat", material_name,
                    gh::milo_object::serialize_mat27(mat)));
            }
            for (const auto& bone_name : bone_names) {
                gh::milo_object::Trans9 trans;
                const auto transform =
                    bundle.bone_transforms.find(bone_name);
                if (transform != bundle.bone_transforms.end()) {
                    trans.local = transform->second.local;
                    trans.world = transform->second.world;
                    trans.parent = transform->second.parent_name;
                } else {
                    set_identity(trans.local);
                    set_identity(trans.world);
                }
                directory.entries.push_back(make_entry(
                    "Trans", bone_name,
                    gh::milo_object::serialize_trans9(trans)));
            }
            for (const auto& chunk : bundle.chunks) {
                directory.entries.push_back(make_entry(
                    "Mesh", chunk.name,
                    gh::milo_object::serialize_mesh28(chunk.mesh)));
            }

            const std::string group_name = name + ".grp";
            gh::milo_object::Group12 group;
            set_identity(group.transformable.local);
            set_identity(group.transformable.world);
            group.objects = mesh_names;
            group.drawable.sphere = aggregate_sphere;
            directory.entries.push_back(make_entry(
                "Group", group_name,
                gh::milo_object::serialize_group12(group)));

            gh::milo_object::Character9 character;
            character.render_directory.object_directory.object_fields.type =
                "guitarist";
            character.render_directory.object_directory.viewports =
                standard_clip_set_viewports();
            set_identity(character.render_directory.transformable.local);
            set_identity(character.render_directory.transformable.world);
            character.render_directory.drawable.sphere = aggregate_sphere;
            character.lods.push_back({0.0f, group_name});
            character.sphere_base = group_name;
            if (has_guitarist_graph) {
                gh::milo_object::BandCharacter1 band_character;
                band_character.character = character;
                directory.dir_body_bytes =
                    gh::milo_object::serialize_band_character1(
                        band_character);
            } else {
                directory.dir_body_bytes =
                    gh::milo_object::serialize_character9(character);
            }

            if (has_guitarist_graph)
                append_guitarist_runtime_graph(
                    directory, main_anim, strum_anim, fret_anim);

            const auto target_payload =
                gh::milo::serialize_directory(directory);
            const auto verify_directory =
                gh::milo::parse_directory(target_payload);
            if (!verify_directory.boundaries_exact ||
                gh::milo::serialize_directory(verify_directory) !=
                    target_payload)
                throw std::runtime_error(
                    "generated Character payload round trip differs");
            const auto target_bytes =
                serialize_generated_milo(target_payload);
            const auto verify_container =
                gh::milo::parse_container(target_bytes);
            const auto verify_container_directory =
                gh::milo::parse_directory(
                    gh::milo::container_payload(verify_container));
            if (!verify_container_directory.boundaries_exact ||
                gh::milo::serialize_directory(
                    verify_container_directory) != target_payload)
                throw std::runtime_error(
                    "generated Character container round trip differs");
            write_file(output, target_bytes);
            std::cout
                << "character=" << name
                << " outfit=" << bundle.outfit
                << " meshes=" << bundle.chunks.size()
                << " materials=" << material_names.size()
                << " textures=" << texture_names.size()
                << " bones=" << bone_names.size()
                << " guitarist_graph="
                << (has_guitarist_graph ? 1 : 0)
                << " entries=" << directory.entries.size()
                << " bytes=" << target_bytes.size()
                << " output=" << output.string() << '\n';
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
