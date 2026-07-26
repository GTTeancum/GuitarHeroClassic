#include "milo_object.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace gh::milo_object {
namespace {

class Cursor {
public:
    explicit Cursor(const std::vector<uint8_t>& bytes) : bytes_(bytes) {}

    uint8_t u8() {
        need(1);
        return bytes_[pos_++];
    }
    uint32_t u32() {
        need(4);
        uint32_t value = 0;
        std::memcpy(&value, bytes_.data() + pos_, 4);
        pos_ += 4;
        return value;
    }
    uint16_t u16() {
        need(2);
        const uint16_t value = static_cast<uint16_t>(bytes_[pos_]) |
                               static_cast<uint16_t>(bytes_[pos_ + 1] << 8);
        pos_ += 2;
        return value;
    }
    float f32() {
        const uint32_t bits = u32();
        float value = 0.0f;
        std::memcpy(&value, &bits, 4);
        return value;
    }
    std::string string() {
        const uint32_t length = u32();
        need(length);
        std::string value(
            reinterpret_cast<const char*>(bytes_.data() + pos_), length);
        pos_ += length;
        return value;
    }
    size_t remaining() const { return bytes_.size() - pos_; }
    size_t position() const { return pos_; }
    std::vector<uint8_t> bytes(size_t count) {
        need(count);
        std::vector<uint8_t> value(bytes_.begin() + pos_,
                                   bytes_.begin() + pos_ + count);
        pos_ += count;
        return value;
    }

private:
    void need(size_t count) const {
        if (count > bytes_.size() - pos_)
            throw std::runtime_error(
                "MILO object: read past body end at=" +
                std::to_string(pos_) + " need=" +
                std::to_string(count) + " size=" +
                std::to_string(bytes_.size()));
    }
    const std::vector<uint8_t>& bytes_;
    size_t pos_ = 0;
};

void append_u8(std::vector<uint8_t>& out, uint8_t value) {
    out.push_back(value);
}

void append_u32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 24));
}

void append_u16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8));
}

void append_f32(std::vector<uint8_t>& out, float value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &value, 4);
    append_u32(out, bits);
}

void append_string(std::vector<uint8_t>& out, const std::string& value) {
    if (value.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("MILO object: string too large");
    append_u32(out, static_cast<uint32_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

uint32_t bounded_count(Cursor& cursor, const char* label) {
    const uint32_t count = cursor.u32();
    if (count > 1000000)
        throw std::runtime_error(
            std::string("MILO object: implausible ") + label + " count");
    return count;
}

LegacyAnimatable parse_legacy_animatable(Cursor& cursor) {
    LegacyAnimatable anim;
    anim.revision = cursor.u32();
    if (anim.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Animatable revision");
    const uint32_t entry_count = bounded_count(cursor, "animation entry");
    anim.entries.reserve(entry_count);
    for (uint32_t i = 0; i < entry_count; ++i) {
        LegacyAnimEntry entry;
        entry.object = cursor.string();
        entry.start_frame = cursor.f32();
        entry.end_frame = cursor.f32();
        anim.entries.push_back(std::move(entry));
    }
    const uint32_t object_count = bounded_count(cursor, "animation object");
    anim.objects.reserve(object_count);
    for (uint32_t i = 0; i < object_count; ++i)
        anim.objects.push_back(cursor.string());
    return anim;
}

void serialize_legacy_animatable(
    std::vector<uint8_t>& out, const LegacyAnimatable& anim) {
    if (anim.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Animatable revision");
    if (anim.entries.size() > std::numeric_limits<uint32_t>::max() ||
        anim.objects.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("MILO object: Animatable vector too large");
    append_u32(out, anim.revision);
    append_u32(out, static_cast<uint32_t>(anim.entries.size()));
    for (const auto& entry : anim.entries) {
        append_string(out, entry.object);
        append_f32(out, entry.start_frame);
        append_f32(out, entry.end_frame);
    }
    append_u32(out, static_cast<uint32_t>(anim.objects.size()));
    for (const auto& object : anim.objects) append_string(out, object);
}

LegacyDrawable parse_legacy_drawable(Cursor& cursor) {
    LegacyDrawable drawable;
    drawable.revision = cursor.u32();
    if (drawable.revision > 4)
        throw std::runtime_error(
            "MILO object: unsupported legacy Drawable revision");
    drawable.showing = cursor.u8() != 0;
    if (drawable.revision < 2) {
        const uint32_t count = bounded_count(cursor, "Drawable object");
        drawable.objects.reserve(count);
        for (uint32_t i = 0; i < count; ++i)
            drawable.objects.push_back(cursor.string());
    }
    if (drawable.revision > 0) {
        for (float& value : drawable.sphere) value = cursor.f32();
    }
    if (drawable.revision > 2)
        drawable.draw_order = cursor.f32();
    if (drawable.revision > 3)
        drawable.legacy_target = cursor.string();
    return drawable;
}

void serialize_legacy_drawable(
    std::vector<uint8_t>& out, const LegacyDrawable& drawable) {
    if (drawable.revision > 4)
        throw std::runtime_error(
            "MILO object: unsupported legacy Drawable revision");
    append_u32(out, drawable.revision);
    append_u8(out, drawable.showing ? 1 : 0);
    if (drawable.revision < 2) {
        if (drawable.objects.size() >
            std::numeric_limits<uint32_t>::max())
            throw std::runtime_error(
                "MILO object: too many Drawable objects");
        append_u32(out, static_cast<uint32_t>(drawable.objects.size()));
        for (const auto& object : drawable.objects)
            append_string(out, object);
    } else if (!drawable.objects.empty()) {
        throw std::runtime_error(
            "MILO object: Drawable revision does not store object list");
    }
    if (drawable.revision > 0) {
        for (float value : drawable.sphere) append_f32(out, value);
    }
    if (drawable.revision > 2)
        append_f32(out, drawable.draw_order);
    if (drawable.revision > 3)
        append_string(out, drawable.legacy_target);
}

LegacyTransformable parse_legacy_transformable(Cursor& cursor) {
    LegacyTransformable trans;
    trans.revision = cursor.u32();
    if (trans.revision != 8)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Transformable revision");
    for (float& value : trans.local) value = cursor.f32();
    for (float& value : trans.world) value = cursor.f32();
    const uint32_t child_count =
        bounded_count(cursor, "Transformable child");
    trans.children.reserve(child_count);
    for (uint32_t i = 0; i < child_count; ++i)
        trans.children.push_back(cursor.string());
    trans.constraint = cursor.u32();
    trans.target = cursor.string();
    trans.preserve_scale = cursor.u8() != 0;
    trans.parent = cursor.string();
    return trans;
}

void serialize_legacy_transformable(
    std::vector<uint8_t>& out, const LegacyTransformable& trans) {
    if (trans.revision != 8)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Transformable revision");
    if (trans.children.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many Transformable children");
    append_u32(out, trans.revision);
    for (float value : trans.local) append_f32(out, value);
    for (float value : trans.world) append_f32(out, value);
    append_u32(out, static_cast<uint32_t>(trans.children.size()));
    for (const auto& child : trans.children) append_string(out, child);
    append_u32(out, trans.constraint);
    append_string(out, trans.target);
    append_u8(out, trans.preserve_scale ? 1 : 0);
    append_string(out, trans.parent);
}

template <size_t N>
std::vector<std::array<float, N + 1>> parse_float_keys(
    Cursor& cursor, const char* label) {
    const uint32_t count = bounded_count(cursor, label);
    std::vector<std::array<float, N + 1>> keys;
    keys.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        std::array<float, N + 1> key{};
        for (float& value : key) value = cursor.f32();
        keys.push_back(key);
    }
    return keys;
}

template <size_t N, typename Key>
std::vector<Key> parse_typed_float_keys(Cursor& cursor,
                                        const char* label) {
    const auto packed = parse_float_keys<N>(cursor, label);
    std::vector<Key> keys;
    keys.reserve(packed.size());
    for (const auto& source : packed) {
        Key key;
        for (size_t i = 0; i < N; ++i) key.value[i] = source[i];
        key.frame = source[N];
        keys.push_back(key);
    }
    return keys;
}

template <typename Key>
void serialize_typed_float_keys(
    std::vector<uint8_t>& out, const std::vector<Key>& keys) {
    if (keys.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("MILO object: too many animation keys");
    append_u32(out, static_cast<uint32_t>(keys.size()));
    for (const auto& key : keys) {
        for (float value : key.value) append_f32(out, value);
        append_f32(out, key.frame);
    }
}

std::vector<MorphKey> parse_scalar_keys(
    Cursor& cursor, const char* label) {
    const uint32_t count = bounded_count(cursor, label);
    std::vector<MorphKey> keys;
    keys.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
        keys.push_back({cursor.f32(), cursor.f32()});
    return keys;
}

void serialize_scalar_keys(
    std::vector<uint8_t>& out, const std::vector<MorphKey>& keys) {
    if (keys.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("MILO object: too many scalar keys");
    append_u32(out, static_cast<uint32_t>(keys.size()));
    for (const auto& key : keys) {
        append_f32(out, key.value);
        append_f32(out, key.frame);
    }
}

std::vector<ObjectKey> parse_object_keys(
    Cursor& cursor, const char* label) {
    const uint32_t count = bounded_count(cursor, label);
    std::vector<ObjectKey> keys;
    keys.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
        keys.push_back({cursor.string(), cursor.f32()});
    return keys;
}

void serialize_object_keys(
    std::vector<uint8_t>& out, const std::vector<ObjectKey>& keys) {
    if (keys.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("MILO object: too many object keys");
    append_u32(out, static_cast<uint32_t>(keys.size()));
    for (const auto& key : keys) {
        append_string(out, key.object);
        append_f32(out, key.frame);
    }
}

}  // namespace

Morph parse_morph(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Morph morph;
    morph.revision = cursor.u32();
    if (morph.revision != 3)
        throw std::runtime_error("MILO object: unsupported Morph revision");
    morph.animatable = parse_legacy_animatable(cursor);
    const uint32_t pose_count = bounded_count(cursor, "Morph pose");
    morph.poses.reserve(pose_count);
    for (uint32_t i = 0; i < pose_count; ++i) {
        MorphPose pose;
        pose.mesh = cursor.string();
        const uint32_t key_count = bounded_count(cursor, "Morph key");
        pose.keys.reserve(key_count);
        for (uint32_t key = 0; key < key_count; ++key) {
            pose.keys.push_back({cursor.f32(), cursor.f32()});
        }
        morph.poses.push_back(std::move(pose));
    }
    morph.target = cursor.string();
    morph.normals = cursor.u8() != 0;
    morph.spline = cursor.u8() != 0;
    morph.intensity = cursor.f32();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: Morph reader did not consume body end");
    return morph;
}

std::vector<uint8_t> serialize_morph(const Morph& morph) {
    if (morph.revision != 3)
        throw std::runtime_error("MILO object: unsupported Morph revision");
    if (morph.poses.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("MILO object: too many Morph poses");
    std::vector<uint8_t> out;
    append_u32(out, morph.revision);
    serialize_legacy_animatable(out, morph.animatable);
    append_u32(out, static_cast<uint32_t>(morph.poses.size()));
    for (const auto& pose : morph.poses) {
        if (pose.keys.size() > std::numeric_limits<uint32_t>::max())
            throw std::runtime_error("MILO object: too many Morph keys");
        append_string(out, pose.mesh);
        append_u32(out, static_cast<uint32_t>(pose.keys.size()));
        for (const auto& key : pose.keys) {
            append_f32(out, key.value);
            append_f32(out, key.frame);
        }
    }
    append_string(out, morph.target);
    append_u8(out, morph.normals ? 1 : 0);
    append_u8(out, morph.spline ? 1 : 0);
    append_f32(out, morph.intensity);
    return out;
}

TransAnim parse_trans_anim(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    TransAnim anim;
    anim.revision = cursor.u32();
    if (anim.revision != 4)
        throw std::runtime_error(
            "MILO object: unsupported GH1 TransAnim revision");
    anim.animatable = parse_legacy_animatable(cursor);
    anim.drawable = parse_legacy_drawable(cursor);
    anim.target = cursor.string();

    const auto rotations = parse_float_keys<4>(cursor, "rotation key");
    anim.rotation_keys.reserve(rotations.size());
    for (const auto& packed : rotations) {
        QuatKey key;
        for (size_t i = 0; i < 4; ++i) key.value[i] = packed[i];
        key.frame = packed[4];
        anim.rotation_keys.push_back(key);
    }
    const auto translations =
        parse_float_keys<3>(cursor, "translation key");
    anim.translation_keys.reserve(translations.size());
    for (const auto& packed : translations) {
        Vec3Key key;
        for (size_t i = 0; i < 3; ++i) key.value[i] = packed[i];
        key.frame = packed[3];
        anim.translation_keys.push_back(key);
    }
    anim.keys_owner = cursor.string();
    anim.translation_spline = cursor.u8() != 0;
    anim.repeat_translation = cursor.u8() != 0;
    const auto scales = parse_float_keys<3>(cursor, "scale key");
    anim.scale_keys.reserve(scales.size());
    for (const auto& packed : scales) {
        Vec3Key key;
        for (size_t i = 0; i < 3; ++i) key.value[i] = packed[i];
        key.frame = packed[3];
        anim.scale_keys.push_back(key);
    }
    anim.scale_spline = cursor.u8() != 0;
    anim.follow_path = cursor.u8() != 0;
    anim.rotation_slerp = cursor.u8() != 0;
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: TransAnim reader did not consume body end");
    return anim;
}

std::vector<uint8_t> serialize_trans_anim(const TransAnim& anim) {
    if (anim.revision != 4)
        throw std::runtime_error(
            "MILO object: unsupported GH1 TransAnim revision");
    const auto check_count = [](size_t count, const char* label) {
        if (count > std::numeric_limits<uint32_t>::max())
            throw std::runtime_error(
                std::string("MILO object: too many ") + label);
    };
    check_count(anim.rotation_keys.size(), "rotation keys");
    check_count(anim.translation_keys.size(), "translation keys");
    check_count(anim.scale_keys.size(), "scale keys");

    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_legacy_animatable(out, anim.animatable);
    serialize_legacy_drawable(out, anim.drawable);
    append_string(out, anim.target);
    append_u32(out, static_cast<uint32_t>(anim.rotation_keys.size()));
    for (const auto& key : anim.rotation_keys) {
        for (float value : key.value) append_f32(out, value);
        append_f32(out, key.frame);
    }
    append_u32(out, static_cast<uint32_t>(anim.translation_keys.size()));
    for (const auto& key : anim.translation_keys) {
        for (float value : key.value) append_f32(out, value);
        append_f32(out, key.frame);
    }
    append_string(out, anim.keys_owner);
    append_u8(out, anim.translation_spline ? 1 : 0);
    append_u8(out, anim.repeat_translation ? 1 : 0);
    append_u32(out, static_cast<uint32_t>(anim.scale_keys.size()));
    for (const auto& key : anim.scale_keys) {
        for (float value : key.value) append_f32(out, value);
        append_f32(out, key.frame);
    }
    append_u8(out, anim.scale_spline ? 1 : 0);
    append_u8(out, anim.follow_path ? 1 : 0);
    append_u8(out, anim.rotation_slerp ? 1 : 0);
    return out;
}

MultiMesh parse_multi_mesh(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    MultiMesh multi;
    multi.revision = cursor.u32();
    if (multi.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH1 MultiMesh revision");
    multi.drawable = parse_legacy_drawable(cursor);
    multi.mesh = cursor.string();
    const uint32_t transform_count =
        bounded_count(cursor, "MultiMesh transform");
    multi.transforms.reserve(transform_count);
    for (uint32_t i = 0; i < transform_count; ++i) {
        std::array<float, 12> transform{};
        for (float& value : transform) value = cursor.f32();
        multi.transforms.push_back(transform);
    }
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: MultiMesh reader did not consume body end");
    return multi;
}

std::vector<uint8_t> serialize_multi_mesh(const MultiMesh& multi) {
    if (multi.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH1 MultiMesh revision");
    if (multi.transforms.size() >
        std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many MultiMesh transforms");
    std::vector<uint8_t> out;
    append_u32(out, multi.revision);
    serialize_legacy_drawable(out, multi.drawable);
    append_string(out, multi.mesh);
    append_u32(out, static_cast<uint32_t>(multi.transforms.size()));
    for (const auto& transform : multi.transforms)
        for (float value : transform) append_f32(out, value);
    return out;
}

MeshAnim parse_mesh_anim(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    MeshAnim anim;
    anim.revision = cursor.u32();
    if (anim.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH1 MeshAnim revision");
    anim.animatable = parse_legacy_animatable(cursor);
    anim.mesh = cursor.string();

    const auto read_vec3_page = [&]() {
        std::vector<VectorKey<std::array<float, 3>>> page;
        const uint32_t key_count = bounded_count(cursor, "MeshAnim key");
        page.reserve(key_count);
        for (uint32_t i = 0; i < key_count; ++i) {
            VectorKey<std::array<float, 3>> key;
            const uint32_t value_count =
                bounded_count(cursor, "MeshAnim vector value");
            key.values.reserve(value_count);
            for (uint32_t value = 0; value < value_count; ++value) {
                std::array<float, 3> vector{};
                for (float& component : vector)
                    component = cursor.f32();
                key.values.push_back(vector);
            }
            key.frame = cursor.f32();
            page.push_back(std::move(key));
        }
        return page;
    };
    const auto read_vec2_page = [&]() {
        std::vector<VectorKey<std::array<float, 2>>> page;
        const uint32_t key_count = bounded_count(cursor, "MeshAnim key");
        page.reserve(key_count);
        for (uint32_t i = 0; i < key_count; ++i) {
            VectorKey<std::array<float, 2>> key;
            const uint32_t value_count =
                bounded_count(cursor, "MeshAnim vector value");
            key.values.reserve(value_count);
            for (uint32_t value = 0; value < value_count; ++value) {
                std::array<float, 2> vector{};
                for (float& component : vector)
                    component = cursor.f32();
                key.values.push_back(vector);
            }
            key.frame = cursor.f32();
            page.push_back(std::move(key));
        }
        return page;
    };
    const auto read_color_page = [&]() {
        std::vector<VectorKey<uint32_t>> page;
        const uint32_t key_count = bounded_count(cursor, "MeshAnim key");
        page.reserve(key_count);
        for (uint32_t i = 0; i < key_count; ++i) {
            VectorKey<uint32_t> key;
            const uint32_t value_count =
                bounded_count(cursor, "MeshAnim color value");
            key.values.reserve(value_count);
            for (uint32_t value = 0; value < value_count; ++value)
                key.values.push_back(cursor.u32());
            key.frame = cursor.f32();
            page.push_back(std::move(key));
        }
        return page;
    };

    anim.point_keys = read_vec3_page();
    anim.texcoord_keys = read_vec2_page();
    anim.color_keys = read_color_page();
    anim.keys_owner = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: MeshAnim reader did not consume body end");
    return anim;
}

std::vector<uint8_t> serialize_mesh_anim(const MeshAnim& anim) {
    if (anim.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH1 MeshAnim revision");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_legacy_animatable(out, anim.animatable);
    append_string(out, anim.mesh);

    const auto write_vec3_page =
        [&](const std::vector<VectorKey<std::array<float, 3>>>& page) {
            if (page.size() > std::numeric_limits<uint32_t>::max())
                throw std::runtime_error(
                    "MILO object: too many MeshAnim keys");
            append_u32(out, static_cast<uint32_t>(page.size()));
            for (const auto& key : page) {
                if (key.values.size() >
                    std::numeric_limits<uint32_t>::max())
                    throw std::runtime_error(
                        "MILO object: too many MeshAnim values");
                append_u32(
                    out, static_cast<uint32_t>(key.values.size()));
                for (const auto& vector : key.values)
                    for (float value : vector) append_f32(out, value);
                append_f32(out, key.frame);
            }
        };
    const auto write_vec2_page =
        [&](const std::vector<VectorKey<std::array<float, 2>>>& page) {
            if (page.size() > std::numeric_limits<uint32_t>::max())
                throw std::runtime_error(
                    "MILO object: too many MeshAnim keys");
            append_u32(out, static_cast<uint32_t>(page.size()));
            for (const auto& key : page) {
                if (key.values.size() >
                    std::numeric_limits<uint32_t>::max())
                    throw std::runtime_error(
                        "MILO object: too many MeshAnim values");
                append_u32(
                    out, static_cast<uint32_t>(key.values.size()));
                for (const auto& vector : key.values)
                    for (float value : vector) append_f32(out, value);
                append_f32(out, key.frame);
            }
        };
    const auto write_color_page =
        [&](const std::vector<VectorKey<uint32_t>>& page) {
            if (page.size() > std::numeric_limits<uint32_t>::max())
                throw std::runtime_error(
                    "MILO object: too many MeshAnim keys");
            append_u32(out, static_cast<uint32_t>(page.size()));
            for (const auto& key : page) {
                if (key.values.size() >
                    std::numeric_limits<uint32_t>::max())
                    throw std::runtime_error(
                        "MILO object: too many MeshAnim values");
                append_u32(
                    out, static_cast<uint32_t>(key.values.size()));
                for (uint32_t value : key.values) append_u32(out, value);
                append_f32(out, key.frame);
            }
        };
    write_vec3_page(anim.point_keys);
    write_vec2_page(anim.texcoord_keys);
    write_color_page(anim.color_keys);
    append_string(out, anim.keys_owner);
    return out;
}

CamAnim parse_cam_anim(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CamAnim anim;
    anim.revision = cursor.u32();
    if (anim.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH1 CamAnim revision");
    anim.animatable = parse_legacy_animatable(cursor);
    anim.camera = cursor.string();
    const uint32_t key_count = bounded_count(cursor, "CamAnim FOV key");
    anim.fov_keys.reserve(key_count);
    for (uint32_t i = 0; i < key_count; ++i)
        anim.fov_keys.push_back({cursor.f32(), cursor.f32()});
    anim.keys_owner = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: CamAnim reader did not consume body end");
    return anim;
}

std::vector<uint8_t> serialize_cam_anim(const CamAnim& anim) {
    if (anim.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH1 CamAnim revision");
    if (anim.fov_keys.size() >
        std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many CamAnim FOV keys");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_legacy_animatable(out, anim.animatable);
    append_string(out, anim.camera);
    append_u32(out, static_cast<uint32_t>(anim.fov_keys.size()));
    for (const auto& key : anim.fov_keys) {
        append_f32(out, key.value);
        append_f32(out, key.frame);
    }
    append_string(out, anim.keys_owner);
    return out;
}

EnvAnim parse_env_anim(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    EnvAnim anim;
    anim.revision = cursor.u32();
    if (anim.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH1 EnvAnim revision");
    anim.animatable = parse_legacy_animatable(cursor);
    anim.environment = cursor.string();
    anim.ambient_color_keys =
        parse_typed_float_keys<4, ColorKey>(
            cursor, "EnvAnim ambient color key");
    anim.keys_owner = cursor.string();
    anim.fog_color_keys =
        parse_typed_float_keys<4, ColorKey>(
            cursor, "EnvAnim fog color key");
    anim.fog_range_keys =
        parse_typed_float_keys<2, Vec2Key>(
            cursor, "EnvAnim fog range key");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: EnvAnim reader did not consume body end");
    return anim;
}

std::vector<uint8_t> serialize_env_anim(const EnvAnim& anim) {
    if (anim.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH1 EnvAnim revision");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_legacy_animatable(out, anim.animatable);
    append_string(out, anim.environment);
    serialize_typed_float_keys(out, anim.ambient_color_keys);
    append_string(out, anim.keys_owner);
    serialize_typed_float_keys(out, anim.fog_color_keys);
    serialize_typed_float_keys(out, anim.fog_range_keys);
    return out;
}

LightAnim parse_light_anim(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    LightAnim anim;
    anim.revision = cursor.u32();
    if (anim.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH1 LightAnim revision");
    anim.animatable = parse_legacy_animatable(cursor);
    anim.light = cursor.string();
    anim.color_keys =
        parse_typed_float_keys<4, ColorKey>(
            cursor, "LightAnim color key");
    anim.keys_owner = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: LightAnim reader did not consume body end");
    return anim;
}

std::vector<uint8_t> serialize_light_anim(const LightAnim& anim) {
    if (anim.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH1 LightAnim revision");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_legacy_animatable(out, anim.animatable);
    append_string(out, anim.light);
    serialize_typed_float_keys(out, anim.color_keys);
    append_string(out, anim.keys_owner);
    return out;
}

ParticleSysAnim parse_particle_sys_anim(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    ParticleSysAnim anim;
    anim.revision = cursor.u32();
    if (anim.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH1 ParticleSysAnim revision");
    anim.animatable = parse_legacy_animatable(cursor);
    anim.particle_system = cursor.string();
    anim.start_color_keys =
        parse_typed_float_keys<4, ColorKey>(
            cursor, "ParticleSysAnim start color key");
    anim.end_color_keys =
        parse_typed_float_keys<4, ColorKey>(
            cursor, "ParticleSysAnim end color key");
    anim.emit_rate_keys =
        parse_typed_float_keys<2, Vec2Key>(
            cursor, "ParticleSysAnim emit rate key");
    anim.keys_owner = cursor.string();
    anim.speed_keys =
        parse_typed_float_keys<2, Vec2Key>(
            cursor, "ParticleSysAnim speed key");
    anim.life_keys =
        parse_typed_float_keys<2, Vec2Key>(
            cursor, "ParticleSysAnim life key");
    anim.start_size_keys =
        parse_typed_float_keys<2, Vec2Key>(
            cursor, "ParticleSysAnim start size key");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: ParticleSysAnim reader did not consume body end");
    return anim;
}

std::vector<uint8_t> serialize_particle_sys_anim(
    const ParticleSysAnim& anim) {
    if (anim.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH1 ParticleSysAnim revision");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_legacy_animatable(out, anim.animatable);
    append_string(out, anim.particle_system);
    serialize_typed_float_keys(out, anim.start_color_keys);
    serialize_typed_float_keys(out, anim.end_color_keys);
    serialize_typed_float_keys(out, anim.emit_rate_keys);
    append_string(out, anim.keys_owner);
    serialize_typed_float_keys(out, anim.speed_keys);
    serialize_typed_float_keys(out, anim.life_keys);
    serialize_typed_float_keys(out, anim.start_size_keys);
    return out;
}

MatAnim parse_mat_anim(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    MatAnim anim;
    anim.revision = cursor.u32();
    if (anim.revision != 5)
        throw std::runtime_error(
            "MILO object: unsupported GH1 MatAnim revision");
    anim.animatable = parse_legacy_animatable(cursor);
    anim.material = cursor.string();
    const uint32_t stage_count = bounded_count(cursor, "MatAnim stage");
    anim.stages.reserve(stage_count);
    for (uint32_t i = 0; i < stage_count; ++i) {
        MatAnimStage stage;
        stage.translation_keys =
            parse_typed_float_keys<3, Vec3Key>(
                cursor, "MatAnim translation key");
        stage.scale_keys =
            parse_typed_float_keys<3, Vec3Key>(
                cursor, "MatAnim scale key");
        stage.rotation_keys =
            parse_typed_float_keys<3, Vec3Key>(
                cursor, "MatAnim rotation key");
        stage.texture_keys =
            parse_object_keys(cursor, "MatAnim texture key");
        anim.stages.push_back(std::move(stage));
    }
    anim.keys_owner = cursor.string();
    anim.color_keys =
        parse_typed_float_keys<4, ColorKey>(
            cursor, "MatAnim color key");
    anim.alpha_keys = parse_scalar_keys(cursor, "MatAnim alpha key");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: MatAnim reader did not consume body end");
    return anim;
}

std::vector<uint8_t> serialize_mat_anim(const MatAnim& anim) {
    if (anim.revision != 5)
        throw std::runtime_error(
            "MILO object: unsupported GH1 MatAnim revision");
    if (anim.stages.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("MILO object: too many MatAnim stages");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_legacy_animatable(out, anim.animatable);
    append_string(out, anim.material);
    append_u32(out, static_cast<uint32_t>(anim.stages.size()));
    for (const auto& stage : anim.stages) {
        serialize_typed_float_keys(out, stage.translation_keys);
        serialize_typed_float_keys(out, stage.scale_keys);
        serialize_typed_float_keys(out, stage.rotation_keys);
        serialize_object_keys(out, stage.texture_keys);
    }
    append_string(out, anim.keys_owner);
    serialize_typed_float_keys(out, anim.color_keys);
    serialize_scalar_keys(out, anim.alpha_keys);
    return out;
}

Text parse_text(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Text text;
    text.revision = cursor.u32();
    if (text.revision != 15)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Text revision");
    text.drawable = parse_legacy_drawable(cursor);
    text.transformable = parse_legacy_transformable(cursor);
    text.font = cursor.string();
    text.alignment = static_cast<int32_t>(cursor.u32());
    text.text = cursor.string();
    for (float& value : text.color) value = cursor.f32();
    text.wrap_width = cursor.f32();
    text.leading = cursor.f32();
    text.fixed_length = static_cast<int32_t>(cursor.u32());
    text.italics = cursor.f32();
    text.size = cursor.f32();
    text.markup = cursor.u8() != 0;
    text.caps_mode = static_cast<int32_t>(cursor.u32());
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: Text reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return text;
}

std::vector<uint8_t> serialize_text(const Text& text) {
    if (text.revision != 15)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Text revision");
    std::vector<uint8_t> out;
    append_u32(out, text.revision);
    serialize_legacy_drawable(out, text.drawable);
    serialize_legacy_transformable(out, text.transformable);
    append_string(out, text.font);
    append_u32(out, static_cast<uint32_t>(text.alignment));
    append_string(out, text.text);
    for (float value : text.color) append_f32(out, value);
    append_f32(out, text.wrap_width);
    append_f32(out, text.leading);
    append_u32(out, static_cast<uint32_t>(text.fixed_length));
    append_f32(out, text.italics);
    append_f32(out, text.size);
    append_u8(out, text.markup ? 1 : 0);
    append_u32(out, static_cast<uint32_t>(text.caps_mode));
    return out;
}

Movie parse_movie(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Movie movie;
    movie.revision = cursor.u32();
    if (movie.revision != 6)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Movie revision");
    movie.animatable = parse_legacy_animatable(cursor);
    movie.file = cursor.string();
    movie.texture = cursor.string();
    movie.stream = cursor.u8() != 0;
    movie.loop = cursor.u8() != 0;
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: Movie reader did not consume body end");
    return movie;
}

std::vector<uint8_t> serialize_movie(const Movie& movie) {
    if (movie.revision != 6)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Movie revision");
    std::vector<uint8_t> out;
    append_u32(out, movie.revision);
    serialize_legacy_animatable(out, movie.animatable);
    append_string(out, movie.file);
    append_string(out, movie.texture);
    append_u8(out, movie.stream ? 1 : 0);
    append_u8(out, movie.loop ? 1 : 0);
    return out;
}

Font parse_font(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Font font;
    font.revision = cursor.u32();
    if (font.revision != 7)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Font revision");
    font.material = cursor.string();
    for (float& value : font.cell_size) value = cursor.f32();
    font.deprecated_size = cursor.f32();
    font.base_kerning = cursor.f32();
    font.characters = cursor.string();
    font.has_kerning_table = cursor.u8() != 0;
    if (font.has_kerning_table) {
        const uint32_t count = bounded_count(cursor, "Font kerning");
        font.kerning.reserve(count);
        for (uint32_t i = 0; i < count; ++i)
            font.kerning.push_back({cursor.u32(), cursor.f32()});
    }
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: Font reader did not consume body end");
    return font;
}

std::vector<uint8_t> serialize_font(const Font& font) {
    if (font.revision != 7)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Font revision");
    if (font.kerning.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many Font kerning rows");
    if (!font.has_kerning_table && !font.kerning.empty())
        throw std::runtime_error(
            "MILO object: Font kerning rows without table");
    std::vector<uint8_t> out;
    append_u32(out, font.revision);
    append_string(out, font.material);
    for (float value : font.cell_size) append_f32(out, value);
    append_f32(out, font.deprecated_size);
    append_f32(out, font.base_kerning);
    append_string(out, font.characters);
    append_u8(out, font.has_kerning_table ? 1 : 0);
    if (font.has_kerning_table) {
        append_u32(out, static_cast<uint32_t>(font.kerning.size()));
        for (const auto& row : font.kerning) {
            append_u32(out, row.packed_char_pair);
            append_f32(out, row.kerning);
        }
    }
    return out;
}

Tex parse_tex(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Tex tex;
    tex.revision = cursor.u32();
    if (tex.revision != 8)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Tex revision");
    tex.width = cursor.u32();
    tex.height = cursor.u32();
    tex.bits_per_pixel = cursor.u32();
    tex.external_path = cursor.string();
    tex.mipmap_bias = cursor.f32();
    tex.type = static_cast<int32_t>(cursor.u32());
    tex.use_external = cursor.u8() != 0;
    if (cursor.remaining() == 0)
        return tex;
    if (cursor.remaining() < 32)
        throw std::runtime_error(
            "MILO object: truncated Tex HMX bitmap header");
    tex.has_bitmap = true;
    tex.bitmap.header_kind = cursor.u8();
    if (tex.bitmap.header_kind != 1 && tex.bitmap.header_kind != 2)
        throw std::runtime_error(
            "MILO object: unsupported Tex HMX bitmap header kind");
    tex.bitmap.bits_per_pixel = cursor.u8();
    tex.bitmap.encoding = static_cast<int32_t>(cursor.u32());
    tex.bitmap.mipmap_count = cursor.u8();
    tex.bitmap.width = cursor.u16();
    tex.bitmap.height = cursor.u16();
    tex.bitmap.bytes_per_line = cursor.u16();
    tex.bitmap.wii_alpha = cursor.u16();
    const auto reserved = cursor.bytes(tex.bitmap.reserved.size());
    std::copy(reserved.begin(), reserved.end(),
              tex.bitmap.reserved.begin());
    tex.bitmap.data = cursor.bytes(cursor.remaining());
    return tex;
}

std::vector<uint8_t> serialize_tex(const Tex& tex) {
    if (tex.revision != 8)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Tex revision");
    std::vector<uint8_t> out;
    append_u32(out, tex.revision);
    append_u32(out, tex.width);
    append_u32(out, tex.height);
    append_u32(out, tex.bits_per_pixel);
    append_string(out, tex.external_path);
    append_f32(out, tex.mipmap_bias);
    append_u32(out, static_cast<uint32_t>(tex.type));
    append_u8(out, tex.use_external ? 1 : 0);
    if (!tex.has_bitmap) {
        if (!tex.bitmap.data.empty())
            throw std::runtime_error(
                "MILO object: Tex bitmap data without bitmap header");
        return out;
    }
    if (tex.bitmap.header_kind != 1 && tex.bitmap.header_kind != 2)
        throw std::runtime_error(
            "MILO object: unsupported Tex HMX bitmap header kind");
    append_u8(out, tex.bitmap.header_kind);
    append_u8(out, tex.bitmap.bits_per_pixel);
    append_u32(out, static_cast<uint32_t>(tex.bitmap.encoding));
    append_u8(out, tex.bitmap.mipmap_count);
    append_u16(out, tex.bitmap.width);
    append_u16(out, tex.bitmap.height);
    append_u16(out, tex.bitmap.bytes_per_line);
    append_u16(out, tex.bitmap.wii_alpha);
    out.insert(out.end(), tex.bitmap.reserved.begin(),
               tex.bitmap.reserved.end());
    out.insert(out.end(), tex.bitmap.data.begin(), tex.bitmap.data.end());
    return out;
}

View parse_view(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    View view;
    view.revision = cursor.u32();
    if (view.revision != 7)
        throw std::runtime_error(
            "MILO object: unsupported GH1 View revision");
    view.animatable = parse_legacy_animatable(cursor);
    view.transformable = parse_legacy_transformable(cursor);
    view.drawable = parse_legacy_drawable(cursor);
    view.children_owner = cursor.string();
    for (float& value : view.showing_range) value = cursor.f32();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: View reader did not consume body end");
    return view;
}

std::vector<uint8_t> serialize_view(const View& view) {
    if (view.revision != 7)
        throw std::runtime_error(
            "MILO object: unsupported GH1 View revision");
    std::vector<uint8_t> out;
    append_u32(out, view.revision);
    serialize_legacy_animatable(out, view.animatable);
    serialize_legacy_transformable(out, view.transformable);
    serialize_legacy_drawable(out, view.drawable);
    append_string(out, view.children_owner);
    for (float value : view.showing_range) append_f32(out, value);
    return out;
}

Cam parse_cam(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Cam cam;
    cam.revision = cursor.u32();
    if (cam.revision != 9)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Cam revision");
    cam.transformable = parse_legacy_transformable(cursor);
    cam.drawable = parse_legacy_drawable(cursor);
    cam.near_plane = cursor.f32();
    cam.far_plane = cursor.f32();
    cam.fov = cursor.f32();
    for (float& value : cam.screen_rect) value = cursor.f32();
    for (float& value : cam.z_range) value = cursor.f32();
    cam.target_texture = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: Cam reader did not consume body end");
    return cam;
}

std::vector<uint8_t> serialize_cam(const Cam& cam) {
    if (cam.revision != 9)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Cam revision");
    std::vector<uint8_t> out;
    append_u32(out, cam.revision);
    serialize_legacy_transformable(out, cam.transformable);
    serialize_legacy_drawable(out, cam.drawable);
    append_f32(out, cam.near_plane);
    append_f32(out, cam.far_plane);
    append_f32(out, cam.fov);
    for (float value : cam.screen_rect) append_f32(out, value);
    for (float value : cam.z_range) append_f32(out, value);
    append_string(out, cam.target_texture);
    return out;
}

Flare parse_flare(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Flare flare;
    flare.revision = cursor.u32();
    if (flare.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Flare revision");
    flare.transformable = parse_legacy_transformable(cursor);
    flare.drawable = parse_legacy_drawable(cursor);
    flare.material = cursor.string();
    for (float& value : flare.sizes) value = cursor.f32();
    for (float& value : flare.range) value = cursor.f32();
    flare.steps = static_cast<int32_t>(cursor.u32());
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: Flare reader did not consume body end");
    return flare;
}

std::vector<uint8_t> serialize_flare(const Flare& flare) {
    if (flare.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Flare revision");
    std::vector<uint8_t> out;
    append_u32(out, flare.revision);
    serialize_legacy_transformable(out, flare.transformable);
    serialize_legacy_drawable(out, flare.drawable);
    append_string(out, flare.material);
    for (float value : flare.sizes) append_f32(out, value);
    for (float value : flare.range) append_f32(out, value);
    append_u32(out, static_cast<uint32_t>(flare.steps));
    return out;
}

Light parse_light(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Light light;
    light.revision = cursor.u32();
    if (light.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Light revision");
    light.transformable = parse_legacy_transformable(cursor);
    for (float& value : light.color) value = cursor.f32();
    light.range = cursor.f32();
    light.serialized_type = static_cast<int32_t>(cursor.u32());
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: Light reader did not consume body end");
    return light;
}

std::vector<uint8_t> serialize_light(const Light& light) {
    if (light.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Light revision");
    std::vector<uint8_t> out;
    append_u32(out, light.revision);
    serialize_legacy_transformable(out, light.transformable);
    for (float value : light.color) append_f32(out, value);
    append_f32(out, light.range);
    append_u32(out, static_cast<uint32_t>(light.serialized_type));
    return out;
}

Environ parse_environ(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Environ environment;
    environment.revision = cursor.u32();
    if (environment.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Environ revision");
    environment.legacy_drawable = parse_legacy_drawable(cursor);
    const uint32_t light_count = bounded_count(cursor, "Environ light");
    environment.lights.reserve(light_count);
    for (uint32_t i = 0; i < light_count; ++i)
        environment.lights.push_back(cursor.string());
    for (float& value : environment.ambient_color) value = cursor.f32();
    for (float& value : environment.fog_range) value = cursor.f32();
    for (float& value : environment.fog_color) value = cursor.f32();
    environment.fog_enabled = cursor.u8() != 0;
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: Environ reader did not consume body end");
    return environment;
}

std::vector<uint8_t> serialize_environ(const Environ& environment) {
    if (environment.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Environ revision");
    if (environment.lights.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many Environ lights");
    std::vector<uint8_t> out;
    append_u32(out, environment.revision);
    serialize_legacy_drawable(out, environment.legacy_drawable);
    append_u32(out, static_cast<uint32_t>(environment.lights.size()));
    for (const auto& light : environment.lights) append_string(out, light);
    for (float value : environment.ambient_color) append_f32(out, value);
    for (float value : environment.fog_range) append_f32(out, value);
    for (float value : environment.fog_color) append_f32(out, value);
    append_u8(out, environment.fog_enabled ? 1 : 0);
    return out;
}

Mat parse_mat(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Mat mat;
    mat.revision = cursor.u32();
    if (mat.revision != 21)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Mat revision");
    const uint32_t texture_count =
        bounded_count(cursor, "Mat texture");
    mat.textures.reserve(texture_count);
    for (uint32_t i = 0; i < texture_count; ++i) {
        MatTexture texture;
        texture.slot = cursor.u32();
        texture.map_type = cursor.u32();
        for (float& value : texture.transform) value = cursor.f32();
        texture.wrap = cursor.u32();
        texture.texture = cursor.string();
        mat.textures.push_back(std::move(texture));
    }
    mat.primary_blend = cursor.u32();
    for (float& value : mat.color) value = cursor.f32();
    mat.use_environment = cursor.u8() != 0;
    mat.prelit = cursor.u8() != 0;
    mat.z_mode = cursor.u8();
    mat.legacy_state_0 = static_cast<int32_t>(cursor.u32());
    mat.legacy_state_1 = cursor.u16();
    mat.tail_blend = cursor.u32();
    mat.legacy_state_2 = cursor.u16();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: Mat reader did not consume body end");
    return mat;
}

std::vector<uint8_t> serialize_mat(const Mat& mat) {
    if (mat.revision != 21)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Mat revision");
    if (mat.textures.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many Mat textures");
    std::vector<uint8_t> out;
    append_u32(out, mat.revision);
    append_u32(out, static_cast<uint32_t>(mat.textures.size()));
    for (const auto& texture : mat.textures) {
        append_u32(out, texture.slot);
        append_u32(out, texture.map_type);
        for (float value : texture.transform) append_f32(out, value);
        append_u32(out, texture.wrap);
        append_string(out, texture.texture);
    }
    append_u32(out, mat.primary_blend);
    for (float value : mat.color) append_f32(out, value);
    append_u8(out, mat.use_environment ? 1 : 0);
    append_u8(out, mat.prelit ? 1 : 0);
    append_u8(out, mat.z_mode);
    append_u32(out, static_cast<uint32_t>(mat.legacy_state_0));
    append_u16(out, mat.legacy_state_1);
    append_u32(out, mat.tail_blend);
    append_u16(out, mat.legacy_state_2);
    return out;
}

ParticleSys parse_particle_sys(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    ParticleSys particles;
    particles.revision = cursor.u32();
    if (particles.revision != 22)
        throw std::runtime_error(
            "MILO object: unsupported GH1 ParticleSys revision");
    particles.animatable = parse_legacy_animatable(cursor);
    particles.transformable = parse_legacy_transformable(cursor);
    particles.drawable = parse_legacy_drawable(cursor);
    for (float& value : particles.life) value = cursor.f32();
    for (float& value : particles.box_extent_1) value = cursor.f32();
    for (float& value : particles.box_extent_2) value = cursor.f32();
    for (float& value : particles.speed) value = cursor.f32();
    for (float& value : particles.pitch) value = cursor.f32();
    for (float& value : particles.yaw) value = cursor.f32();
    for (float& value : particles.emit_rate) value = cursor.f32();
    for (float& value : particles.start_size) value = cursor.f32();
    for (float& value : particles.delta_size) value = cursor.f32();
    for (float& value : particles.start_color_low) value = cursor.f32();
    for (float& value : particles.start_color_high) value = cursor.f32();
    for (float& value : particles.end_color_low) value = cursor.f32();
    for (float& value : particles.end_color_high) value = cursor.f32();
    particles.bounce_enabled = cursor.u8() != 0;
    for (float& value : particles.bounce_plane) value = cursor.f32();
    for (float& value : particles.force_direction) value = cursor.f32();
    particles.material = cursor.string();
    particles.type = cursor.u32();
    particles.grow_ratio = cursor.f32();
    particles.shrink_ratio = cursor.f32();
    particles.mid_color_ratio = cursor.f32();
    for (float& value : particles.mid_color_low) value = cursor.f32();
    for (float& value : particles.mid_color_high) value = cursor.f32();
    particles.max_particles = cursor.u32();
    for (float& value : particles.bubble_period) value = cursor.f32();
    for (float& value : particles.bubble_size) value = cursor.f32();
    particles.bubble = cursor.u8() != 0;
    particles.relative_motion = cursor.f32();
    particles.emitter_mesh = cursor.string();
    particles.preserve_particles = cursor.u8() != 0;
    if (particles.preserve_particles) {
        const uint32_t count =
            bounded_count(cursor, "preserved ParticleSys particle");
        particles.particles.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            Particle particle;
            for (float& value : particle.position) value = cursor.f32();
            for (float& value : particle.color) value = cursor.f32();
            particle.size = cursor.f32();
            particles.particles.push_back(particle);
        }
    }
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: ParticleSys reader did not consume body end");
    return particles;
}

std::vector<uint8_t> serialize_particle_sys(
    const ParticleSys& particles) {
    if (particles.revision != 22)
        throw std::runtime_error(
            "MILO object: unsupported GH1 ParticleSys revision");
    if (particles.particles.size() >
        std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many preserved ParticleSys particles");
    if (!particles.preserve_particles && !particles.particles.empty())
        throw std::runtime_error(
            "MILO object: ParticleSys rows without preserve flag");
    std::vector<uint8_t> out;
    append_u32(out, particles.revision);
    serialize_legacy_animatable(out, particles.animatable);
    serialize_legacy_transformable(out, particles.transformable);
    serialize_legacy_drawable(out, particles.drawable);
    for (float value : particles.life) append_f32(out, value);
    for (float value : particles.box_extent_1) append_f32(out, value);
    for (float value : particles.box_extent_2) append_f32(out, value);
    for (float value : particles.speed) append_f32(out, value);
    for (float value : particles.pitch) append_f32(out, value);
    for (float value : particles.yaw) append_f32(out, value);
    for (float value : particles.emit_rate) append_f32(out, value);
    for (float value : particles.start_size) append_f32(out, value);
    for (float value : particles.delta_size) append_f32(out, value);
    for (float value : particles.start_color_low) append_f32(out, value);
    for (float value : particles.start_color_high) append_f32(out, value);
    for (float value : particles.end_color_low) append_f32(out, value);
    for (float value : particles.end_color_high) append_f32(out, value);
    append_u8(out, particles.bounce_enabled ? 1 : 0);
    for (float value : particles.bounce_plane) append_f32(out, value);
    for (float value : particles.force_direction) append_f32(out, value);
    append_string(out, particles.material);
    append_u32(out, particles.type);
    append_f32(out, particles.grow_ratio);
    append_f32(out, particles.shrink_ratio);
    append_f32(out, particles.mid_color_ratio);
    for (float value : particles.mid_color_low) append_f32(out, value);
    for (float value : particles.mid_color_high) append_f32(out, value);
    append_u32(out, particles.max_particles);
    for (float value : particles.bubble_period) append_f32(out, value);
    for (float value : particles.bubble_size) append_f32(out, value);
    append_u8(out, particles.bubble ? 1 : 0);
    append_f32(out, particles.relative_motion);
    append_string(out, particles.emitter_mesh);
    append_u8(out, particles.preserve_particles ? 1 : 0);
    if (particles.preserve_particles) {
        append_u32(out,
                   static_cast<uint32_t>(particles.particles.size()));
        for (const auto& particle : particles.particles) {
            for (float value : particle.position) append_f32(out, value);
            for (float value : particle.color) append_f32(out, value);
            append_f32(out, particle.size);
        }
    }
    return out;
}

Mesh parse_mesh(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Mesh mesh;
    mesh.revision = cursor.u32();
    if (mesh.revision != 25)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Mesh revision");
    mesh.transformable = parse_legacy_transformable(cursor);
    mesh.drawable = parse_legacy_drawable(cursor);
    mesh.material = cursor.string();
    mesh.geometry_owner = cursor.string();
    mesh.mutable_flags = cursor.u32();
    mesh.volume = cursor.u32();
    mesh.has_bsp_tree = cursor.u8() != 0;
    if (mesh.has_bsp_tree)
        throw std::runtime_error(
            "MILO object: non-empty GH1 Mesh BSP tree not yet decoded");
    const uint32_t vertex_count =
        bounded_count(cursor, "Mesh vertex");
    mesh.vertices.reserve(vertex_count);
    for (uint32_t i = 0; i < vertex_count; ++i) {
        MeshVertex vertex;
        for (float& value : vertex.position) value = cursor.f32();
        for (float& value : vertex.normal) value = cursor.f32();
        for (float& value : vertex.color_or_weights) value = cursor.f32();
        for (float& value : vertex.uv) value = cursor.f32();
        mesh.vertices.push_back(vertex);
    }
    const uint32_t face_count = bounded_count(cursor, "Mesh face");
    mesh.faces.reserve(face_count);
    for (uint32_t i = 0; i < face_count; ++i)
        mesh.faces.push_back(
            {cursor.u16(), cursor.u16(), cursor.u16()});
    const uint32_t patch_count = bounded_count(cursor, "Mesh patch");
    mesh.patches = cursor.bytes(patch_count);
    mesh.bone_slots[0].bone = cursor.string();
    mesh.has_bones = !mesh.bone_slots[0].bone.empty();
    if (mesh.has_bones) {
        for (size_t i = 1; i < mesh.bone_slots.size(); ++i)
            mesh.bone_slots[i].bone = cursor.string();
        for (auto& slot : mesh.bone_slots)
            for (float& value : slot.offset) value = cursor.f32();
    }
    // Cached platform strips are emitted only when CacheStrips accepts the
    // mesh (geometry owner is self, vertices/faces are non-empty, and mutable
    // state permits caching). A patch vector may still exist on transform-only
    // bone meshes, so the strip-result block is explicitly optional.
    if (cursor.remaining() != 0) {
        mesh.strip_results.reserve(mesh.patches.size());
        for (size_t i = 0; i < mesh.patches.size(); ++i) {
            MeshStripResult result;
            const uint32_t strip_count =
                bounded_count(cursor, "Mesh strip");
            const uint32_t run_count =
                bounded_count(cursor, "Mesh strip run");
            result.cumulative_strip_lengths.reserve(strip_count);
            for (uint32_t j = 0; j < strip_count; ++j)
                result.cumulative_strip_lengths.push_back(cursor.u32());
            result.strip_runs.reserve(run_count);
            for (uint32_t j = 0; j < run_count; ++j)
                result.strip_runs.push_back(cursor.u16());
            mesh.strip_results.push_back(std::move(result));
        }
    }
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: Mesh reader residual bytes=" +
            std::to_string(cursor.remaining()) + " at=" +
            std::to_string(cursor.position()));
    return mesh;
}

std::vector<uint8_t> serialize_mesh(const Mesh& mesh) {
    if (mesh.revision != 25)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Mesh revision");
    if (mesh.has_bsp_tree)
        throw std::runtime_error(
            "MILO object: non-empty GH1 Mesh BSP tree not yet encoded");
    if (mesh.vertices.size() > std::numeric_limits<uint32_t>::max() ||
        mesh.faces.size() > std::numeric_limits<uint32_t>::max() ||
        mesh.patches.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("MILO object: Mesh vector too large");
    if (!mesh.strip_results.empty() &&
        mesh.strip_results.size() != mesh.patches.size())
        throw std::runtime_error(
            "MILO object: Mesh strip result/patch count mismatch");
    if (!mesh.has_bones && !mesh.bone_slots[0].bone.empty())
        throw std::runtime_error(
            "MILO object: Mesh bone slots without bone flag");
    if (mesh.has_bones && mesh.bone_slots[0].bone.empty())
        throw std::runtime_error(
            "MILO object: Mesh bone block has empty first bone");
    std::vector<uint8_t> out;
    append_u32(out, mesh.revision);
    serialize_legacy_transformable(out, mesh.transformable);
    serialize_legacy_drawable(out, mesh.drawable);
    append_string(out, mesh.material);
    append_string(out, mesh.geometry_owner);
    append_u32(out, mesh.mutable_flags);
    append_u32(out, mesh.volume);
    append_u8(out, 0);
    append_u32(out, static_cast<uint32_t>(mesh.vertices.size()));
    for (const auto& vertex : mesh.vertices) {
        for (float value : vertex.position) append_f32(out, value);
        for (float value : vertex.normal) append_f32(out, value);
        for (float value : vertex.color_or_weights) append_f32(out, value);
        for (float value : vertex.uv) append_f32(out, value);
    }
    append_u32(out, static_cast<uint32_t>(mesh.faces.size()));
    for (const auto& face : mesh.faces)
        for (uint16_t index : face) append_u16(out, index);
    append_u32(out, static_cast<uint32_t>(mesh.patches.size()));
    out.insert(out.end(), mesh.patches.begin(), mesh.patches.end());
    append_string(out, mesh.has_bones ? mesh.bone_slots[0].bone :
                                      std::string{});
    if (mesh.has_bones) {
        for (size_t i = 1; i < mesh.bone_slots.size(); ++i)
            append_string(out, mesh.bone_slots[i].bone);
        for (const auto& slot : mesh.bone_slots)
            for (float value : slot.offset) append_f32(out, value);
    }
    for (const auto& result : mesh.strip_results) {
        if (result.cumulative_strip_lengths.size() >
                std::numeric_limits<uint32_t>::max() ||
            result.strip_runs.size() >
                std::numeric_limits<uint32_t>::max())
            throw std::runtime_error(
                "MILO object: Mesh strip result too large");
        append_u32(
            out, static_cast<uint32_t>(
                     result.cumulative_strip_lengths.size()));
        append_u32(out,
                   static_cast<uint32_t>(result.strip_runs.size()));
        for (uint32_t length : result.cumulative_strip_lengths)
            append_u32(out, length);
        for (uint16_t run : result.strip_runs) append_u16(out, run);
    }
    return out;
}

}  // namespace gh::milo_object
