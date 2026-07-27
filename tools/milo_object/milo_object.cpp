#include "milo_object.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
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

bool type_property_uses_symbol(uint32_t type) {
    switch (type) {
        case 0x02:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x09:
        case 0x12:
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
            return true;
        default:
            return false;
    }
}

bool type_property_uses_array(uint32_t type) {
    return type == 0x10 || type == 0x11 || type == 0x13;
}

TypePropertyNode parse_type_property_node(
    Cursor& cursor, unsigned depth) {
    if (depth > 128)
        throw std::runtime_error(
            "MILO object: TypeProps nesting exceeds 128");
    TypePropertyNode node;
    node.type = cursor.u32();
    if (node.type == 0) {
        node.integer = cursor.u32();
    } else if (node.type == 1) {
        node.floating = cursor.f32();
    } else if (type_property_uses_symbol(node.type)) {
        node.symbol = cursor.string();
    } else if (type_property_uses_array(node.type)) {
        const uint16_t count = cursor.u16();
        node.array_id = cursor.u32();
        node.children.reserve(count);
        for (uint16_t i = 0; i < count; ++i)
            node.children.push_back(
                parse_type_property_node(cursor, depth + 1));
    } else if (node.type != 3) {
        throw std::runtime_error(
            "MILO object: unsupported TypeProps node type=" +
            std::to_string(node.type));
    }
    return node;
}

void serialize_type_property_node(
    std::vector<uint8_t>& out, const TypePropertyNode& node,
    unsigned depth) {
    if (depth > 128)
        throw std::runtime_error(
            "MILO object: TypeProps nesting exceeds 128");
    append_u32(out, node.type);
    if (node.type == 0) {
        append_u32(out, node.integer);
    } else if (node.type == 1) {
        append_f32(out, node.floating);
    } else if (type_property_uses_symbol(node.type)) {
        append_string(out, node.symbol);
    } else if (type_property_uses_array(node.type)) {
        if (node.children.size() > std::numeric_limits<uint16_t>::max())
            throw std::runtime_error(
                "MILO object: too many TypeProps children");
        append_u16(out, static_cast<uint16_t>(node.children.size()));
        append_u32(out, node.array_id);
        for (const auto& child : node.children)
            serialize_type_property_node(out, child, depth + 1);
    } else if (node.type != 3) {
        throw std::runtime_error(
            "MILO object: unsupported TypeProps node type=" +
            std::to_string(node.type));
    }
}

ObjectFields0 parse_object_fields0(Cursor& cursor, const char* owner) {
    ObjectFields0 fields;
    fields.revision = cursor.u32();
    if (fields.revision != 0)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " ObjectFields revision");
    fields.type = cursor.string();
    fields.has_type_properties = cursor.u8() != 0;
    if (fields.has_type_properties) {
        const uint16_t count = cursor.u16();
        fields.type_property_id = cursor.u32();
        fields.type_properties.reserve(count);
        for (uint16_t i = 0; i < count; ++i)
            fields.type_properties.push_back(
                parse_type_property_node(cursor, 0));
    }
    return fields;
}

void serialize_object_fields0(
    std::vector<uint8_t>& out, const ObjectFields0& fields,
    const char* owner) {
    if (fields.revision != 0)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " ObjectFields");
    append_u32(out, fields.revision);
    append_string(out, fields.type);
    append_u8(out, fields.has_type_properties ? 1 : 0);
    if (fields.has_type_properties) {
        if (fields.type_properties.size() >
            std::numeric_limits<uint16_t>::max())
            throw std::runtime_error(
                "MILO object: too many TypeProps roots");
        append_u16(
            out, static_cast<uint16_t>(fields.type_properties.size()));
        append_u32(out, fields.type_property_id);
        for (const auto& node : fields.type_properties)
            serialize_type_property_node(out, node, 0);
    } else if (!fields.type_properties.empty()) {
        throw std::runtime_error(
            "MILO object: TypeProps roots without tree flag");
    }
}

Transformable9 parse_transformable9(Cursor& cursor, const char* owner) {
    Transformable9 transformable;
    transformable.revision = cursor.u32();
    if (transformable.revision != 9)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " Transformable revision");
    for (float& value : transformable.local) value = cursor.f32();
    for (float& value : transformable.world) value = cursor.f32();
    transformable.constraint = cursor.u32();
    transformable.target = cursor.string();
    transformable.preserve_scale = cursor.u8() != 0;
    transformable.parent = cursor.string();
    return transformable;
}

void serialize_transformable9(
    std::vector<uint8_t>& out, const Transformable9& transformable,
    const char* owner) {
    if (transformable.revision != 9)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " Transformable revision");
    append_u32(out, transformable.revision);
    for (float value : transformable.local) append_f32(out, value);
    for (float value : transformable.world) append_f32(out, value);
    append_u32(out, transformable.constraint);
    append_string(out, transformable.target);
    append_u8(out, transformable.preserve_scale ? 1 : 0);
    append_string(out, transformable.parent);
}

uint32_t convert_transformable_constraint8_to_9_impl(
    uint32_t constraint) {
    // Retail GH2 RndTransformable::Load revision 8 compatibility:
    // old 2/3 become 3/4 and removed old 4 becomes unconstrained 0.
    if (constraint == 4) return 0;
    if (constraint == 2 || constraint == 3) return constraint + 1;
    return constraint;
}

Transformable9 convert_transformable8_to_9(
    const LegacyTransformable& source) {
    if (source.revision != 8)
        throw std::runtime_error(
            "MILO object: transform conversion requires GH1 revision 8");
    Transformable9 target;
    target.local = source.local;
    target.world = source.world;
    target.constraint =
        convert_transformable_constraint8_to_9_impl(source.constraint);
    target.target = source.target;
    target.preserve_scale = source.preserve_scale;
    target.parent = source.parent;
    return target;
}

Animatable4 parse_animatable4(Cursor& cursor, const char* owner) {
    Animatable4 animatable;
    animatable.revision = cursor.u32();
    if (animatable.revision != 4)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " Animatable revision");
    animatable.frame = cursor.f32();
    animatable.rate = static_cast<int32_t>(cursor.u32());
    return animatable;
}

void serialize_animatable4(
    std::vector<uint8_t>& out, const Animatable4& animatable,
    const char* owner) {
    if (animatable.revision != 4)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " Animatable revision");
    append_u32(out, animatable.revision);
    append_f32(out, animatable.frame);
    append_u32(out, static_cast<uint32_t>(animatable.rate));
}

Animatable4 convert_animatable0_to_4(
    const LegacyAnimatable& source) {
    if (source.revision != 0)
        throw std::runtime_error(
            "MILO object: Animatable conversion requires GH1 revision 0");
    if (!source.operations.empty() || !source.objects.empty())
        throw std::runtime_error(
            "MILO object: GH1 Animatable0 filter/group graph expansion "
            "requires directory conversion");
    return {};
}

LegacyAnimSettings reduce_legacy_animatable_impl(
    const LegacyAnimatable& source) {
    if (source.revision != 0)
        throw std::runtime_error(
            "MILO object: legacy animation reduction requires revision 0");
    LegacyAnimSettings settings;
    for (const auto& operation : source.operations) {
        switch (operation.type) {
            case 0:
                settings.scale = operation.first;
                settings.offset = operation.second;
                break;
            case 1:
                settings.minimum = operation.first;
                settings.maximum = operation.second;
                settings.loop = operation.loop;
                break;
            case 2:
            case 3:
            case 4:
                break;
            default:
                throw std::runtime_error(
                    "MILO object: unsupported legacy animation operation");
        }
    }
    return settings;
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
    const uint32_t operation_count =
        bounded_count(cursor, "animation operation");
    anim.operations.reserve(operation_count);
    for (uint32_t i = 0; i < operation_count; ++i) {
        LegacyAnimOperation operation;
        operation.type = cursor.u32();
        switch (operation.type) {
            case 0:
                operation.first = cursor.f32();
                operation.second = cursor.f32();
                break;
            case 1:
                operation.first = cursor.f32();
                operation.second = cursor.f32();
                operation.loop = cursor.u8() != 0;
                break;
            case 2:
            case 3:
                operation.integers[0] =
                    static_cast<int32_t>(cursor.u32());
                operation.integers[1] =
                    static_cast<int32_t>(cursor.u32());
                break;
            case 4:
                for (int32_t& value : operation.integers)
                    value = static_cast<int32_t>(cursor.u32());
                break;
            default:
                throw std::runtime_error(
                    "MILO object: unsupported GH1 Animatable operation " +
                    std::to_string(operation.type));
        }
        anim.operations.push_back(operation);
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
    if (anim.operations.size() >
            std::numeric_limits<uint32_t>::max() ||
        anim.objects.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("MILO object: Animatable vector too large");
    append_u32(out, anim.revision);
    append_u32(out, static_cast<uint32_t>(anim.operations.size()));
    for (const auto& operation : anim.operations) {
        append_u32(out, operation.type);
        switch (operation.type) {
            case 0:
                append_f32(out, operation.first);
                append_f32(out, operation.second);
                break;
            case 1:
                append_f32(out, operation.first);
                append_f32(out, operation.second);
                append_u8(out, operation.loop ? 1 : 0);
                break;
            case 2:
            case 3:
                append_u32(
                    out, static_cast<uint32_t>(operation.integers[0]));
                append_u32(
                    out, static_cast<uint32_t>(operation.integers[1]));
                break;
            case 4:
                for (int32_t value : operation.integers)
                    append_u32(out, static_cast<uint32_t>(value));
                break;
            default:
                throw std::runtime_error(
                    "MILO object: unsupported GH1 Animatable operation " +
                    std::to_string(operation.type));
        }
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

Drawable3 parse_drawable3(Cursor& cursor, const char* owner) {
    Drawable3 drawable;
    drawable.revision = cursor.u32();
    if (drawable.revision != 3)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " Drawable revision");
    drawable.showing = cursor.u8() != 0;
    for (float& value : drawable.sphere) value = cursor.f32();
    drawable.draw_order = cursor.f32();
    return drawable;
}

void serialize_drawable3(
    std::vector<uint8_t>& out, const Drawable3& drawable,
    const char* owner) {
    if (drawable.revision != 3)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " Drawable revision");
    append_u32(out, drawable.revision);
    append_u8(out, drawable.showing ? 1 : 0);
    for (float value : drawable.sphere) append_f32(out, value);
    append_f32(out, drawable.draw_order);
}

Drawable3 convert_drawable_to_3(const LegacyDrawable& source) {
    if (source.revision > 4)
        throw std::runtime_error(
            "MILO object: Drawable conversion source revision unsupported");
    Drawable3 target;
    target.showing = source.showing;
    if (source.revision > 0)
        target.sphere = source.sphere;
    if (source.revision > 2)
        target.draw_order = source.draw_order;
    return target;
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

template <size_t N>
std::vector<VectorKey<std::array<float, N>>> parse_vector_keys(
    Cursor& cursor, const char* label) {
    const uint32_t key_count = bounded_count(cursor, label);
    std::vector<VectorKey<std::array<float, N>>> keys;
    keys.reserve(key_count);
    for (uint32_t i = 0; i < key_count; ++i) {
        VectorKey<std::array<float, N>> key;
        const uint32_t value_count =
            bounded_count(cursor, "MeshAnim vector value");
        key.values.reserve(value_count);
        for (uint32_t value = 0; value < value_count; ++value) {
            std::array<float, N> vector{};
            for (float& component : vector)
                component = cursor.f32();
            key.values.push_back(vector);
        }
        key.frame = cursor.f32();
        keys.push_back(std::move(key));
    }
    return keys;
}

template <size_t N>
void serialize_vector_keys(
    std::vector<uint8_t>& out,
    const std::vector<VectorKey<std::array<float, N>>>& keys) {
    if (keys.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("MILO object: too many MeshAnim keys");
    append_u32(out, static_cast<uint32_t>(keys.size()));
    for (const auto& key : keys) {
        if (key.values.size() > std::numeric_limits<uint32_t>::max())
            throw std::runtime_error(
                "MILO object: too many MeshAnim values");
        append_u32(out, static_cast<uint32_t>(key.values.size()));
        for (const auto& vector : key.values)
            for (float value : vector) append_f32(out, value);
        append_f32(out, key.frame);
    }
}

}  // namespace

uint32_t convert_transformable_constraint8_to_9(
    uint32_t constraint) {
    return convert_transformable_constraint8_to_9_impl(constraint);
}

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

Morph4 parse_morph4(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Morph4 morph;
    morph.revision = cursor.u32();
    if (morph.revision != 4)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Morph revision");
    morph.object_fields = parse_object_fields0(cursor, "Morph");
    morph.animatable = parse_animatable4(cursor, "Morph");
    const uint32_t pose_count =
        bounded_count(cursor, "GH2 Morph pose");
    morph.poses.reserve(pose_count);
    for (uint32_t i = 0; i < pose_count; ++i) {
        MorphPose pose;
        pose.mesh = cursor.string();
        const uint32_t key_count =
            bounded_count(cursor, "GH2 Morph key");
        pose.keys.reserve(key_count);
        for (uint32_t key = 0; key < key_count; ++key)
            pose.keys.push_back({cursor.f32(), cursor.f32()});
        morph.poses.push_back(std::move(pose));
    }
    morph.target = cursor.string();
    morph.normals = cursor.u8() != 0;
    morph.spline = cursor.u8() != 0;
    morph.intensity = cursor.f32();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 Morph reader did not consume body end");
    return morph;
}

std::vector<uint8_t> serialize_morph4(const Morph4& morph) {
    if (morph.revision != 4)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Morph revision");
    if (morph.poses.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many GH2 Morph poses");
    std::vector<uint8_t> out;
    append_u32(out, morph.revision);
    serialize_object_fields0(out, morph.object_fields, "Morph");
    serialize_animatable4(out, morph.animatable, "Morph");
    append_u32(out, static_cast<uint32_t>(morph.poses.size()));
    for (const auto& pose : morph.poses) {
        if (pose.keys.size() > std::numeric_limits<uint32_t>::max())
            throw std::runtime_error(
                "MILO object: too many GH2 Morph keys");
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

Morph4 convert_morph3_to_morph4(const Morph& source) {
    if (source.revision != 3)
        throw std::runtime_error(
            "MILO object: Morph conversion requires GH1 revision 3");
    Morph4 target;
    target.animatable =
        convert_animatable0_to_4(source.animatable);
    target.poses = source.poses;
    target.target = source.target;
    target.normals = source.normals;
    target.spline = source.spline;
    target.intensity = source.intensity;
    return target;
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

TransAnim6 parse_trans_anim6(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    TransAnim6 anim;
    anim.revision = cursor.u32();
    if (anim.revision != 6)
        throw std::runtime_error(
            "MILO object: unsupported GH2 TransAnim revision");
    anim.object_fields = parse_object_fields0(cursor, "TransAnim");
    anim.animatable = parse_animatable4(cursor, "TransAnim");
    anim.target = cursor.string();
    const auto rotations =
        parse_float_keys<4>(cursor, "GH2 TransAnim rotation key");
    anim.rotation_keys.reserve(rotations.size());
    for (const auto& packed : rotations) {
        QuatKey key;
        for (size_t i = 0; i < 4; ++i) key.value[i] = packed[i];
        key.frame = packed[4];
        anim.rotation_keys.push_back(key);
    }
    const auto translations =
        parse_float_keys<3>(
            cursor, "GH2 TransAnim translation key");
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
    const auto scales =
        parse_float_keys<3>(cursor, "GH2 TransAnim scale key");
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
            "MILO object: GH2 TransAnim reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return anim;
}

std::vector<uint8_t> serialize_trans_anim6(const TransAnim6& anim) {
    if (anim.revision != 6)
        throw std::runtime_error(
            "MILO object: unsupported GH2 TransAnim revision");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_object_fields0(out, anim.object_fields, "TransAnim");
    serialize_animatable4(out, anim.animatable, "TransAnim");
    append_string(out, anim.target);
    append_u32(out, static_cast<uint32_t>(anim.rotation_keys.size()));
    for (const auto& key : anim.rotation_keys) {
        for (float value : key.value) append_f32(out, value);
        append_f32(out, key.frame);
    }
    append_u32(
        out, static_cast<uint32_t>(anim.translation_keys.size()));
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

TransAnim6 convert_trans_anim4_to_trans_anim6(
    const TransAnim& source) {
    if (source.revision != 4)
        throw std::runtime_error(
            "MILO object: TransAnim conversion requires GH1 revision 4");
    TransAnim6 target;
    target.animatable =
        convert_animatable0_to_4(source.animatable);
    target.target = source.target;
    target.rotation_keys = source.rotation_keys;
    target.translation_keys = source.translation_keys;
    target.keys_owner = source.keys_owner;
    target.translation_spline = source.translation_spline;
    target.repeat_translation = source.repeat_translation;
    target.scale_keys = source.scale_keys;
    target.scale_spline = source.scale_spline;
    target.follow_path = source.follow_path;
    target.rotation_slerp = source.rotation_slerp;
    return target;
}

CamShot20 parse_cam_shot20(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CamShot20 shot;
    shot.revision = cursor.u32();
    if (shot.revision != 20)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CamShot revision");
    shot.object_fields = parse_object_fields0(cursor, "CamShot");
    shot.animatable = parse_animatable4(cursor, "CamShot");
    const uint32_t keyframe_count =
        bounded_count(cursor, "CamShot keyframe");
    if (keyframe_count == 0 || keyframe_count > 65535)
        throw std::runtime_error(
            "MILO object: invalid CamShot20 keyframe count");
    shot.keyframes.reserve(keyframe_count);
    for (uint32_t i = 0; i < keyframe_count; ++i) {
        CamShotFrame20 frame;
        frame.duration = cursor.f32();
        frame.blend = cursor.f32();
        frame.blend_ease = cursor.f32();
        frame.field_of_view = cursor.f32();
        for (float& value : frame.world_offset) value = cursor.f32();
        for (float& value : frame.screen_offset) value = cursor.f32();
        frame.blur_depth = cursor.f32();
        frame.legacy_blur = static_cast<int32_t>(cursor.u32());
        frame.legacy_focus = static_cast<int32_t>(cursor.u32());
        const uint32_t target_count =
            bounded_count(cursor, "CamShot target");
        if (target_count > 32)
            throw std::runtime_error(
                "MILO object: invalid CamShot20 target count");
        frame.targets.reserve(target_count);
        for (uint32_t target_index = 0;
             target_index < target_count; ++target_index) {
            CamShotSubPart20 target;
            target.legacy_unknown =
                static_cast<int32_t>(cursor.u32());
            target.object = cursor.string();
            target.part = cursor.string();
            frame.targets.push_back(std::move(target));
        }
        frame.parent.legacy_unknown =
            static_cast<int32_t>(cursor.u32());
        frame.parent.object = cursor.string();
        frame.parent.part = cursor.string();
        frame.use_parent_rotation = cursor.u8() != 0;
        frame.shake_noise_amplitude = cursor.f32();
        frame.shake_noise_frequency = cursor.f32();
        for (float& value : frame.maximum_angular_offset)
            value = cursor.f32();
        shot.keyframes.push_back(std::move(frame));
    }
    shot.looping = cursor.u8() != 0;
    shot.legacy_loop_frame = cursor.f32();
    shot.near_plane = cursor.f32();
    shot.far_plane = cursor.f32();
    shot.use_depth_of_field = cursor.u8() != 0;
    shot.filter = cursor.f32();
    shot.clamp_height = cursor.f32();
    shot.path = cursor.string();
    shot.legacy_path_frame = cursor.f32();
    shot.category = cursor.string();
    shot.legacy_category_frame = cursor.f32();
    const uint32_t crowd_pair_count =
        bounded_count(cursor, "CamShot crowd pair");
    if (crowd_pair_count > 64)
        throw std::runtime_error(
            "MILO object: invalid CamShot20 crowd pair count");
    shot.legacy_crowd_pairs.reserve(crowd_pair_count);
    for (uint32_t i = 0; i < crowd_pair_count; ++i) {
        shot.legacy_crowd_pairs.push_back(
            {static_cast<int32_t>(cursor.u32()),
             static_cast<int32_t>(cursor.u32())});
    }
    shot.legacy_crowd_modify_stamp =
        static_cast<int32_t>(cursor.u32());
    const uint32_t hide_count =
        bounded_count(cursor, "CamShot hide");
    if (hide_count > 128)
        throw std::runtime_error(
            "MILO object: invalid CamShot20 hide count");
    shot.hide_list.reserve(hide_count);
    for (uint32_t i = 0; i < hide_count; ++i)
        shot.hide_list.push_back(cursor.string());
    shot.legacy_crowd = cursor.string();
    shot.glow_spot = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CamShot20 reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return shot;
}

std::vector<uint8_t> serialize_cam_shot20(const CamShot20& shot) {
    if (shot.revision != 20)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CamShot revision");
    if (shot.keyframes.empty() || shot.keyframes.size() > 65535)
        throw std::runtime_error(
            "MILO object: invalid CamShot20 keyframe count");
    std::vector<uint8_t> out;
    append_u32(out, shot.revision);
    serialize_object_fields0(out, shot.object_fields, "CamShot");
    serialize_animatable4(out, shot.animatable, "CamShot");
    append_u32(out, static_cast<uint32_t>(shot.keyframes.size()));
    for (const auto& frame : shot.keyframes) {
        append_f32(out, frame.duration);
        append_f32(out, frame.blend);
        append_f32(out, frame.blend_ease);
        append_f32(out, frame.field_of_view);
        for (float value : frame.world_offset) append_f32(out, value);
        for (float value : frame.screen_offset) append_f32(out, value);
        append_f32(out, frame.blur_depth);
        append_u32(out, static_cast<uint32_t>(frame.legacy_blur));
        append_u32(out, static_cast<uint32_t>(frame.legacy_focus));
        if (frame.targets.size() > 32)
            throw std::runtime_error(
                "MILO object: invalid CamShot20 target count");
        append_u32(out, static_cast<uint32_t>(frame.targets.size()));
        for (const auto& target : frame.targets) {
            append_u32(
                out, static_cast<uint32_t>(target.legacy_unknown));
            append_string(out, target.object);
            append_string(out, target.part);
        }
        append_u32(
            out, static_cast<uint32_t>(frame.parent.legacy_unknown));
        append_string(out, frame.parent.object);
        append_string(out, frame.parent.part);
        append_u8(out, frame.use_parent_rotation ? 1 : 0);
        append_f32(out, frame.shake_noise_amplitude);
        append_f32(out, frame.shake_noise_frequency);
        for (float value : frame.maximum_angular_offset)
            append_f32(out, value);
    }
    append_u8(out, shot.looping ? 1 : 0);
    append_f32(out, shot.legacy_loop_frame);
    append_f32(out, shot.near_plane);
    append_f32(out, shot.far_plane);
    append_u8(out, shot.use_depth_of_field ? 1 : 0);
    append_f32(out, shot.filter);
    append_f32(out, shot.clamp_height);
    append_string(out, shot.path);
    append_f32(out, shot.legacy_path_frame);
    append_string(out, shot.category);
    append_f32(out, shot.legacy_category_frame);
    if (shot.legacy_crowd_pairs.size() > 64)
        throw std::runtime_error(
            "MILO object: invalid CamShot20 crowd pair count");
    append_u32(
        out, static_cast<uint32_t>(shot.legacy_crowd_pairs.size()));
    for (const auto& pair : shot.legacy_crowd_pairs) {
        append_u32(out, static_cast<uint32_t>(pair[0]));
        append_u32(out, static_cast<uint32_t>(pair[1]));
    }
    append_u32(
        out, static_cast<uint32_t>(shot.legacy_crowd_modify_stamp));
    if (shot.hide_list.size() > 128)
        throw std::runtime_error(
            "MILO object: invalid CamShot20 hide count");
    append_u32(out, static_cast<uint32_t>(shot.hide_list.size()));
    for (const auto& name : shot.hide_list) append_string(out, name);
    append_string(out, shot.legacy_crowd);
    append_string(out, shot.glow_spot);
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

MultiMesh1 parse_multi_mesh1(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    MultiMesh1 multi;
    multi.revision = cursor.u32();
    if (multi.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 MultiMesh revision");
    multi.object_fields = parse_object_fields0(cursor, "MultiMesh");
    multi.drawable = parse_drawable3(cursor, "MultiMesh");
    multi.mesh = cursor.string();
    const uint32_t transform_count =
        bounded_count(cursor, "GH2 MultiMesh transform");
    multi.transforms.reserve(transform_count);
    for (uint32_t i = 0; i < transform_count; ++i) {
        std::array<float, 12> transform{};
        for (float& value : transform) value = cursor.f32();
        multi.transforms.push_back(transform);
    }
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 MultiMesh reader did not consume body end");
    return multi;
}

std::vector<uint8_t> serialize_multi_mesh1(const MultiMesh1& multi) {
    if (multi.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 MultiMesh revision");
    if (multi.transforms.size() >
        std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many GH2 MultiMesh transforms");
    std::vector<uint8_t> out;
    append_u32(out, multi.revision);
    serialize_object_fields0(out, multi.object_fields, "MultiMesh");
    serialize_drawable3(out, multi.drawable, "MultiMesh");
    append_string(out, multi.mesh);
    append_u32(out, static_cast<uint32_t>(multi.transforms.size()));
    for (const auto& transform : multi.transforms)
        for (float value : transform) append_f32(out, value);
    return out;
}

MultiMesh1 convert_multi_mesh0_to_multi_mesh1(
    const MultiMesh& source) {
    if (source.revision != 0)
        throw std::runtime_error(
            "MILO object: MultiMesh conversion requires GH1 revision 0");
    MultiMesh1 target;
    target.drawable = convert_drawable_to_3(source.drawable);
    target.mesh = source.mesh;
    target.transforms = source.transforms;
    return target;
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
        std::vector<VectorKey<std::array<float, 4>>> page;
        const uint32_t key_count = bounded_count(cursor, "MeshAnim key");
        page.reserve(key_count);
        for (uint32_t i = 0; i < key_count; ++i) {
            VectorKey<std::array<float, 4>> key;
            const uint32_t value_count =
                bounded_count(cursor, "MeshAnim color value");
            key.values.reserve(value_count);
            for (uint32_t value = 0; value < value_count; ++value) {
                std::array<float, 4> color{};
                for (float& component : color)
                    component = cursor.f32();
                key.values.push_back(color);
            }
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
        [&](const std::vector<
            VectorKey<std::array<float, 4>>>& page) {
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
                for (const auto& color : key.values)
                    for (float value : color) append_f32(out, value);
                append_f32(out, key.frame);
            }
        };
    write_vec3_page(anim.point_keys);
    write_vec2_page(anim.texcoord_keys);
    write_color_page(anim.color_keys);
    append_string(out, anim.keys_owner);
    return out;
}

MeshAnim1 parse_mesh_anim1(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    MeshAnim1 anim;
    anim.revision = cursor.u32();
    if (anim.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 MeshAnim revision");
    anim.object_fields = parse_object_fields0(cursor, "MeshAnim");
    anim.animatable = parse_animatable4(cursor, "MeshAnim");
    anim.mesh = cursor.string();
    anim.point_keys =
        parse_vector_keys<3>(
            cursor, "GH2 MeshAnim point key");
    anim.texcoord_keys =
        parse_vector_keys<2>(
            cursor, "GH2 MeshAnim texcoord key");
    anim.color_keys =
        parse_vector_keys<4>(
            cursor, "GH2 MeshAnim color key");
    anim.keys_owner = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 MeshAnim reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return anim;
}

std::vector<uint8_t> serialize_mesh_anim1(const MeshAnim1& anim) {
    if (anim.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 MeshAnim revision");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_object_fields0(out, anim.object_fields, "MeshAnim");
    serialize_animatable4(out, anim.animatable, "MeshAnim");
    append_string(out, anim.mesh);
    serialize_vector_keys<3>(out, anim.point_keys);
    serialize_vector_keys<2>(out, anim.texcoord_keys);
    serialize_vector_keys<4>(out, anim.color_keys);
    append_string(out, anim.keys_owner);
    return out;
}

MeshAnim1 convert_mesh_anim0_to_mesh_anim1(
    const MeshAnim& source) {
    if (source.revision != 0)
        throw std::runtime_error(
            "MILO object: MeshAnim conversion requires GH1 revision 0");
    MeshAnim1 target;
    target.animatable =
        convert_animatable0_to_4(source.animatable);
    target.mesh = source.mesh;
    target.point_keys = source.point_keys;
    target.texcoord_keys = source.texcoord_keys;
    target.color_keys = source.color_keys;
    target.keys_owner = source.keys_owner;
    return target;
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

CamAnim2 parse_cam_anim2(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CamAnim2 anim;
    anim.revision = cursor.u32();
    if (anim.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CamAnim revision");
    anim.object_fields = parse_object_fields0(cursor, "CamAnim");
    anim.animatable = parse_animatable4(cursor, "CamAnim");
    anim.camera = cursor.string();
    const uint32_t key_count =
        bounded_count(cursor, "GH2 CamAnim FOV key");
    anim.fov_keys.reserve(key_count);
    for (uint32_t i = 0; i < key_count; ++i)
        anim.fov_keys.push_back({cursor.f32(), cursor.f32()});
    anim.keys_owner = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CamAnim reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return anim;
}

std::vector<uint8_t> serialize_cam_anim2(const CamAnim2& anim) {
    if (anim.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CamAnim revision");
    if (anim.fov_keys.size() >
        std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many GH2 CamAnim FOV keys");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_object_fields0(out, anim.object_fields, "CamAnim");
    serialize_animatable4(out, anim.animatable, "CamAnim");
    append_string(out, anim.camera);
    append_u32(out, static_cast<uint32_t>(anim.fov_keys.size()));
    for (const auto& key : anim.fov_keys) {
        append_f32(out, key.value);
        append_f32(out, key.frame);
    }
    append_string(out, anim.keys_owner);
    return out;
}

CamAnim2 convert_cam_anim0_to_cam_anim2(const CamAnim& source) {
    if (source.revision != 0)
        throw std::runtime_error(
            "MILO object: CamAnim conversion requires GH1 revision 0");
    CamAnim2 target;
    target.animatable =
        convert_animatable0_to_4(source.animatable);
    target.camera = source.camera;
    target.fov_keys.reserve(source.fov_keys.size());
    for (const auto& source_key : source.fov_keys) {
        target.fov_keys.push_back({
            std::atan(0.75f * std::tan(source_key.value * 0.5f)) *
                2.0f,
            source_key.frame});
    }
    target.keys_owner = source.keys_owner;
    return target;
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

EnvAnim4 parse_env_anim4(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    EnvAnim4 anim;
    anim.revision = cursor.u32();
    if (anim.revision != 4)
        throw std::runtime_error(
            "MILO object: unsupported GH2 EnvAnim revision");
    anim.object_fields = parse_object_fields0(cursor, "EnvAnim");
    anim.animatable = parse_animatable4(cursor, "EnvAnim");
    anim.environment = cursor.string();
    anim.ambient_color_keys =
        parse_typed_float_keys<4, ColorKey>(
            cursor, "GH2 EnvAnim ambient color key");
    anim.keys_owner = cursor.string();
    anim.fog_color_keys =
        parse_typed_float_keys<4, ColorKey>(
            cursor, "GH2 EnvAnim fog color key");
    anim.fog_range_keys =
        parse_typed_float_keys<2, Vec2Key>(
            cursor, "GH2 EnvAnim fog range key");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 EnvAnim reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return anim;
}

std::vector<uint8_t> serialize_env_anim4(const EnvAnim4& anim) {
    if (anim.revision != 4)
        throw std::runtime_error(
            "MILO object: unsupported GH2 EnvAnim revision");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_object_fields0(out, anim.object_fields, "EnvAnim");
    serialize_animatable4(out, anim.animatable, "EnvAnim");
    append_string(out, anim.environment);
    serialize_typed_float_keys(out, anim.ambient_color_keys);
    append_string(out, anim.keys_owner);
    serialize_typed_float_keys(out, anim.fog_color_keys);
    serialize_typed_float_keys(out, anim.fog_range_keys);
    return out;
}

EnvAnim4 convert_env_anim3_to_env_anim4(const EnvAnim& source) {
    if (source.revision != 3)
        throw std::runtime_error(
            "MILO object: EnvAnim conversion requires GH1 revision 3");
    EnvAnim4 target;
    target.animatable =
        convert_animatable0_to_4(source.animatable);
    target.environment = source.environment;
    target.ambient_color_keys = source.ambient_color_keys;
    target.keys_owner = source.keys_owner;
    target.fog_color_keys = source.fog_color_keys;
    target.fog_range_keys = source.fog_range_keys;
    return target;
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

LightAnim2 parse_light_anim2(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    LightAnim2 anim;
    anim.revision = cursor.u32();
    if (anim.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 LightAnim revision");
    anim.object_fields = parse_object_fields0(cursor, "LightAnim");
    anim.animatable = parse_animatable4(cursor, "LightAnim");
    anim.light = cursor.string();
    anim.color_keys =
        parse_typed_float_keys<4, ColorKey>(
            cursor, "GH2 LightAnim color key");
    anim.keys_owner = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 LightAnim reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return anim;
}

std::vector<uint8_t> serialize_light_anim2(const LightAnim2& anim) {
    if (anim.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 LightAnim revision");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_object_fields0(out, anim.object_fields, "LightAnim");
    serialize_animatable4(out, anim.animatable, "LightAnim");
    append_string(out, anim.light);
    serialize_typed_float_keys(out, anim.color_keys);
    append_string(out, anim.keys_owner);
    return out;
}

LightAnim2 convert_light_anim1_to_light_anim2(
    const LightAnim& source) {
    if (source.revision != 1)
        throw std::runtime_error(
            "MILO object: LightAnim conversion requires GH1 revision 1");
    LightAnim2 target;
    target.animatable =
        convert_animatable0_to_4(source.animatable);
    target.light = source.light;
    target.color_keys = source.color_keys;
    target.keys_owner = source.keys_owner;
    return target;
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

ParticleSysAnim3 parse_particle_sys_anim3(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    ParticleSysAnim3 anim;
    anim.revision = cursor.u32();
    if (anim.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH2 ParticleSysAnim revision");
    anim.object_fields =
        parse_object_fields0(cursor, "ParticleSysAnim");
    anim.animatable =
        parse_animatable4(cursor, "ParticleSysAnim");
    anim.particle_system = cursor.string();
    anim.start_color_keys =
        parse_typed_float_keys<4, ColorKey>(
            cursor, "GH2 ParticleSysAnim start color key");
    anim.end_color_keys =
        parse_typed_float_keys<4, ColorKey>(
            cursor, "GH2 ParticleSysAnim end color key");
    anim.emit_rate_keys =
        parse_typed_float_keys<2, Vec2Key>(
            cursor, "GH2 ParticleSysAnim emit rate key");
    anim.keys_owner = cursor.string();
    anim.speed_keys =
        parse_typed_float_keys<2, Vec2Key>(
            cursor, "GH2 ParticleSysAnim speed key");
    anim.life_keys =
        parse_typed_float_keys<2, Vec2Key>(
            cursor, "GH2 ParticleSysAnim life key");
    anim.start_size_keys =
        parse_typed_float_keys<2, Vec2Key>(
            cursor, "GH2 ParticleSysAnim start size key");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 ParticleSysAnim reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return anim;
}

std::vector<uint8_t> serialize_particle_sys_anim3(
    const ParticleSysAnim3& anim) {
    if (anim.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH2 ParticleSysAnim revision");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_object_fields0(out, anim.object_fields, "ParticleSysAnim");
    serialize_animatable4(out, anim.animatable, "ParticleSysAnim");
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

ParticleSysAnim3 convert_particle_sys_anim2_to_particle_sys_anim3(
    const ParticleSysAnim& source) {
    if (source.revision != 2)
        throw std::runtime_error(
            "MILO object: ParticleSysAnim conversion requires GH1 revision 2");
    ParticleSysAnim3 target;
    target.animatable =
        convert_animatable0_to_4(source.animatable);
    target.particle_system = source.particle_system;
    target.start_color_keys = source.start_color_keys;
    target.end_color_keys = source.end_color_keys;
    target.emit_rate_keys = source.emit_rate_keys;
    target.keys_owner = source.keys_owner;
    target.speed_keys = source.speed_keys;
    target.life_keys = source.life_keys;
    target.start_size_keys = source.start_size_keys;
    return target;
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

MatAnim7 parse_mat_anim7(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    MatAnim7 anim;
    anim.revision = cursor.u32();
    if (anim.revision != 7)
        throw std::runtime_error(
            "MILO object: unsupported GH2 MatAnim revision");
    anim.object_fields = parse_object_fields0(cursor, "MatAnim");
    anim.animatable = parse_animatable4(cursor, "MatAnim");
    anim.material = cursor.string();
    anim.keys_owner = cursor.string();
    anim.color_keys =
        parse_typed_float_keys<4, ColorKey>(
            cursor, "GH2 MatAnim color key");
    anim.alpha_keys =
        parse_scalar_keys(cursor, "GH2 MatAnim alpha key");
    anim.translation_keys =
        parse_typed_float_keys<3, Vec3Key>(
            cursor, "GH2 MatAnim translation key");
    anim.scale_keys =
        parse_typed_float_keys<3, Vec3Key>(
            cursor, "GH2 MatAnim scale key");
    anim.rotation_keys =
        parse_typed_float_keys<3, Vec3Key>(
            cursor, "GH2 MatAnim rotation key");
    anim.texture_keys =
        parse_object_keys(cursor, "GH2 MatAnim texture key");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 MatAnim reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return anim;
}

std::vector<uint8_t> serialize_mat_anim7(const MatAnim7& anim) {
    if (anim.revision != 7)
        throw std::runtime_error(
            "MILO object: unsupported GH2 MatAnim revision");
    std::vector<uint8_t> out;
    append_u32(out, anim.revision);
    serialize_object_fields0(out, anim.object_fields, "MatAnim");
    serialize_animatable4(out, anim.animatable, "MatAnim");
    append_string(out, anim.material);
    append_string(out, anim.keys_owner);
    serialize_typed_float_keys(out, anim.color_keys);
    serialize_scalar_keys(out, anim.alpha_keys);
    serialize_typed_float_keys(out, anim.translation_keys);
    serialize_typed_float_keys(out, anim.scale_keys);
    serialize_typed_float_keys(out, anim.rotation_keys);
    serialize_object_keys(out, anim.texture_keys);
    return out;
}

MatAnim7 convert_mat_anim5_to_mat_anim7(const MatAnim& source) {
    if (source.revision != 5)
        throw std::runtime_error(
            "MILO object: MatAnim conversion requires GH1 revision 5");
    if (source.stages.size() > 1)
        throw std::runtime_error(
            "MILO object: GH1 multi-stage MatAnim requires directory "
            "object/material graph expansion");
    MatAnim7 target;
    target.animatable =
        convert_animatable0_to_4(source.animatable);
    target.material = source.material;
    target.keys_owner = source.keys_owner;
    target.color_keys = source.color_keys;
    target.alpha_keys = source.alpha_keys;
    if (!source.stages.empty()) {
        target.translation_keys =
            source.stages[0].translation_keys;
        target.scale_keys = source.stages[0].scale_keys;
        target.rotation_keys = source.stages[0].rotation_keys;
        target.texture_keys = source.stages[0].texture_keys;
    }
    return target;
}

std::vector<ConvertedMatAnimPass>
convert_mat_anim5_to_mat_anim7_passes(
    const MatAnim& source, const std::string& source_name) {
    if (source.revision != 5)
        throw std::runtime_error(
            "MILO object: MatAnim conversion requires GH1 revision 5");
    if (source_name.empty())
        throw std::runtime_error(
            "MILO object: MatAnim conversion requires an object name");

    const auto without_extension = [](const std::string& value) {
        const size_t dot = value.rfind('.');
        return dot == std::string::npos
                   ? value
                   : value.substr(0, dot);
    };
    const auto stage_end_frame = [](const MatAnimStage& stage) {
        float end = 0.0f;
        const auto include = [&end](const auto& keys) {
            for (const auto& key : keys)
                end = std::max(end, key.frame);
        };
        include(stage.translation_keys);
        include(stage.scale_keys);
        include(stage.rotation_keys);
        include(stage.texture_keys);
        return end;
    };
    const auto apply_stage =
        [](MatAnim7& target, const MatAnimStage& stage) {
        target.translation_keys = stage.translation_keys;
        target.scale_keys = stage.scale_keys;
        target.rotation_keys = stage.rotation_keys;
        target.texture_keys = stage.texture_keys;
    };

    MatAnim7 root;
    root.animatable =
        convert_animatable0_to_4(source.animatable);
    root.material = source.material;
    root.keys_owner = source.keys_owner;
    root.color_keys = source.color_keys;
    root.alpha_keys = source.alpha_keys;
    if (!source.stages.empty())
        apply_stage(root, source.stages.back());

    std::vector<ConvertedMatAnimPass> result;
    result.push_back({source_name, std::move(root)});
    if (source.stages.size() < 2) return result;

    const std::string anim_base = without_extension(source_name);
    const std::string mat_base = without_extension(source.material);
    for (size_t i = 0; i + 1 < source.stages.size(); ++i) {
        // HMX's revision-5 loader keeps the final serialized stage on the
        // original MatAnim. Earlier non-static stages are split into
        // one-based "_N.mnm" objects targeting the matching Mat pass.
        if (stage_end_frame(source.stages[i]) == 0.0f) continue;
        ConvertedMatAnimPass pass;
        pass.name =
            anim_base + "_" + std::to_string(i + 1) + ".mnm";
        pass.animation.animatable =
            convert_animatable0_to_4(source.animatable);
        if (!source.material.empty())
            pass.animation.material =
                mat_base + "_" + std::to_string(i + 1) + ".mat";
        pass.animation.keys_owner = pass.name;
        apply_stage(pass.animation, source.stages[i]);
        result.push_back(std::move(pass));
    }
    return result;
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

Text17 parse_text17(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Text17 text;
    text.revision = cursor.u32();
    if (text.revision != 17)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Text revision");
    text.object_fields = parse_object_fields0(cursor, "Text");
    text.drawable = parse_drawable3(cursor, "Text");
    text.transformable = parse_transformable9(cursor, "Text");
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
            "MILO object: GH2 Text reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return text;
}

std::vector<uint8_t> serialize_text17(const Text17& text) {
    if (text.revision != 17)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Text revision");
    std::vector<uint8_t> out;
    append_u32(out, text.revision);
    serialize_object_fields0(out, text.object_fields, "Text");
    serialize_drawable3(out, text.drawable, "Text");
    serialize_transformable9(out, text.transformable, "Text");
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

Text17 convert_text15_to_text17(const Text& source) {
    if (source.revision != 15)
        throw std::runtime_error(
            "MILO object: Text conversion requires GH1 revision 15");
    Text17 target;
    target.drawable = convert_drawable_to_3(source.drawable);
    target.transformable =
        convert_transformable8_to_9(source.transformable);
    target.font = source.font;
    target.alignment = source.alignment;
    target.text = source.text;
    target.color = source.color;
    target.wrap_width = source.wrap_width;
    target.leading = source.leading;
    target.fixed_length = source.fixed_length;
    target.italics = source.italics;
    target.size = source.size;
    target.markup = source.markup;
    target.caps_mode = source.caps_mode;
    return target;
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

Movie8 parse_movie8(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Movie8 movie;
    movie.revision = cursor.u32();
    if (movie.revision != 8)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Movie revision");
    movie.object_fields = parse_object_fields0(cursor, "Movie");
    movie.animatable = parse_animatable4(cursor, "Movie");
    movie.file = cursor.string();
    movie.texture = cursor.string();
    movie.stream = cursor.u8() != 0;
    movie.loop = cursor.u8() != 0;
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 Movie reader did not consume body end");
    return movie;
}

std::vector<uint8_t> serialize_movie8(const Movie8& movie) {
    if (movie.revision != 8)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Movie revision");
    std::vector<uint8_t> out;
    append_u32(out, movie.revision);
    serialize_object_fields0(out, movie.object_fields, "Movie");
    serialize_animatable4(out, movie.animatable, "Movie");
    append_string(out, movie.file);
    append_string(out, movie.texture);
    append_u8(out, movie.stream ? 1 : 0);
    append_u8(out, movie.loop ? 1 : 0);
    return out;
}

Movie8 convert_movie6_to_movie8(const Movie& source) {
    if (source.revision != 6)
        throw std::runtime_error(
            "MILO object: Movie conversion requires GH1 revision 6");
    Movie8 target;
    target.animatable =
        convert_animatable0_to_4(source.animatable);
    target.file = source.file;
    target.texture = source.texture;
    target.stream = source.stream;
    target.loop = source.loop;
    return target;
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

Font15 parse_font15(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Font15 font;
    font.revision = cursor.u32();
    if (font.revision != 15)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Font revision");
    font.object_fields = parse_object_fields0(cursor, "Font");
    font.material = cursor.string();
    for (float& value : font.cell_size) value = cursor.f32();
    font.deprecated_size = cursor.f32();
    font.base_kerning = cursor.f32();
    font.characters = cursor.string();
    font.has_kerning_table = cursor.u8() != 0;
    if (font.has_kerning_table) {
        const uint32_t count =
            bounded_count(cursor, "GH2 Font kerning");
        font.kerning.reserve(count);
        for (uint32_t i = 0; i < count; ++i)
            font.kerning.push_back({cursor.u32(), cursor.f32()});
    }
    font.texture_owner = cursor.string();
    font.monospace = cursor.u8() != 0;
    font.packed = cursor.u8() != 0;
    font.bitmap_width = static_cast<int32_t>(cursor.u32());
    font.bitmap_height = static_cast<int32_t>(cursor.u32());
    for (float& value : font.texture_cell_size)
        value = cursor.f32();
    for (auto& info : font.character_info) {
        info.texture_u = cursor.f32();
        info.texture_v = cursor.f32();
        info.character_width = cursor.f32();
        info.character_advance = cursor.f32();
    }
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 Font reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return font;
}

std::vector<uint8_t> serialize_font15(const Font15& font) {
    if (font.revision != 15)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Font revision");
    if (font.kerning.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many GH2 Font kerning rows");
    if (!font.has_kerning_table && !font.kerning.empty())
        throw std::runtime_error(
            "MILO object: GH2 Font kerning rows without table");
    std::vector<uint8_t> out;
    append_u32(out, font.revision);
    serialize_object_fields0(out, font.object_fields, "Font");
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
    append_string(out, font.texture_owner);
    append_u8(out, font.monospace ? 1 : 0);
    append_u8(out, font.packed ? 1 : 0);
    append_u32(out, static_cast<uint32_t>(font.bitmap_width));
    append_u32(out, static_cast<uint32_t>(font.bitmap_height));
    for (float value : font.texture_cell_size) append_f32(out, value);
    for (const auto& info : font.character_info) {
        append_f32(out, info.texture_u);
        append_f32(out, info.texture_v);
        append_f32(out, info.character_width);
        append_f32(out, info.character_advance);
    }
    return out;
}

Font15 convert_font7_to_font15(
    const Font& source, const std::string& source_name,
    uint32_t bitmap_width, uint32_t bitmap_height,
    const std::vector<uint8_t>& bitmap_rgba) {
    if (source.revision != 7)
        throw std::runtime_error(
            "MILO object: Font conversion requires GH1 revision 7");
    if (source_name.empty())
        throw std::runtime_error(
            "MILO object: Font conversion requires an object name");
    if (bitmap_width == 0 || bitmap_height == 0 ||
        bitmap_width >
            static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) ||
        bitmap_height >
            static_cast<uint32_t>(std::numeric_limits<int32_t>::max()))
        throw std::runtime_error(
            "MILO object: Font bitmap dimensions are invalid");
    const uint64_t expected_bytes =
        static_cast<uint64_t>(bitmap_width) * bitmap_height * 4;
    if (expected_bytes != bitmap_rgba.size())
        throw std::runtime_error(
            "MILO object: Font RGBA bitmap size mismatch");
    const float cell_width = source.cell_size[0];
    const float cell_height = source.cell_size[1];
    if (!std::isfinite(cell_width) || !std::isfinite(cell_height) ||
        cell_width <= 0.0f || cell_height <= 0.0f)
        throw std::runtime_error(
            "MILO object: Font cell dimensions are invalid");

    Font15 target;
    target.material = source.material;
    target.cell_size = source.cell_size;
    target.deprecated_size = source.deprecated_size;
    target.base_kerning = source.base_kerning;
    target.characters = source.characters;
    if (!target.characters.empty() &&
        static_cast<uint8_t>(target.characters.front()) == 0xA0)
        target.characters.front() = ' ';
    target.has_kerning_table = source.has_kerning_table;
    target.kerning = source.kerning;
    target.texture_owner = source_name;
    target.monospace = false;
    target.packed = false;
    target.bitmap_width = static_cast<int32_t>(bitmap_width);
    target.bitmap_height = static_cast<int32_t>(bitmap_height);
    target.texture_cell_size = {
        cell_width / static_cast<float>(bitmap_width),
        cell_height / static_cast<float>(bitmap_height)};

    const auto column_non_transparent =
        [&](int x, int top, int bottom) {
        if (x < 0 || x >= static_cast<int>(bitmap_width))
            return false;
        top = std::max(top, 0);
        bottom = std::min(
            bottom, static_cast<int>(bitmap_height));
        for (int y = top; y < bottom; ++y) {
            const size_t alpha =
                (static_cast<size_t>(y) * bitmap_width +
                 static_cast<uint32_t>(x)) *
                    4 +
                3;
            if (bitmap_rgba[alpha] != 0) return true;
        }
        return false;
    };

    float x = 0.0f;
    float y = 0.0f;
    for (unsigned char character : target.characters) {
        if (x + cell_width > static_cast<float>(bitmap_width)) {
            x = 0.0f;
            y += cell_height;
        }
        if (y + cell_height > static_cast<float>(bitmap_height))
            break;

        const int left_edge = static_cast<int>(std::lrint(x));
        const int right_edge =
            static_cast<int>(std::lrint(x + cell_width));
        const int top_edge = static_cast<int>(std::lrint(y));
        const int bottom_edge =
            static_cast<int>(std::lrint(y + cell_height));
        int left = left_edge;
        while (left < right_edge &&
               !column_non_transparent(left, top_edge, bottom_edge))
            ++left;
        int right = right_edge - 1;
        while (right >= left_edge &&
               !column_non_transparent(right, top_edge, bottom_edge))
            --right;

        FontCharInfo15& info = target.character_info[character];
        const int glyph_width = right + 1 - left;
        info.texture_v =
            y / static_cast<float>(bitmap_height);
        if (glyph_width <= 0) {
            info.texture_u =
                x / static_cast<float>(bitmap_width);
            info.character_width = 0.25f;
            info.character_advance = 0.25f;
        } else {
            info.texture_u =
                static_cast<float>(left) /
                static_cast<float>(bitmap_width);
            info.character_width =
                static_cast<float>(glyph_width) / cell_width;
            info.character_advance = info.character_width;
        }
        x += cell_width;
    }

    target.character_info[9] = target.character_info[32];
    target.character_info[9].character_advance *= 3.0f;
    return target;
}

LegacyAnimSettings reduce_legacy_animatable(
    const LegacyAnimatable& source) {
    return reduce_legacy_animatable_impl(source);
}

AnimFilter1 parse_anim_filter1(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    AnimFilter1 filter;
    filter.revision = cursor.u32();
    if (filter.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 AnimFilter revision");
    filter.object_fields = parse_object_fields0(cursor, "AnimFilter");
    filter.animatable = parse_animatable4(cursor, "AnimFilter");
    filter.anim = cursor.string();
    filter.scale = cursor.f32();
    filter.offset = cursor.f32();
    filter.start = cursor.f32();
    filter.end = cursor.f32();
    filter.type = static_cast<int32_t>(cursor.u32());
    filter.period = cursor.f32();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 AnimFilter reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return filter;
}

std::vector<uint8_t> serialize_anim_filter1(
    const AnimFilter1& filter) {
    if (filter.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 AnimFilter revision");
    if (filter.type < 0 || filter.type > 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 AnimFilter type");
    std::vector<uint8_t> out;
    append_u32(out, filter.revision);
    serialize_object_fields0(out, filter.object_fields, "AnimFilter");
    serialize_animatable4(out, filter.animatable, "AnimFilter");
    append_string(out, filter.anim);
    append_f32(out, filter.scale);
    append_f32(out, filter.offset);
    append_f32(out, filter.start);
    append_f32(out, filter.end);
    append_u32(out, static_cast<uint32_t>(filter.type));
    append_f32(out, filter.period);
    return out;
}

AnimFilter1 convert_legacy_animatable_to_anim_filter1(
    const LegacyAnimatable& source, const std::string& anim) {
    const LegacyAnimSettings settings =
        reduce_legacy_animatable_impl(source);
    if (!settings.requires_filter())
        throw std::runtime_error(
            "MILO object: legacy Animatable0 does not require a filter");
    AnimFilter1 target;
    target.anim = anim;
    target.scale = std::fabs(settings.scale);
    target.offset = settings.offset;
    target.start = settings.minimum;
    target.end = settings.maximum;
    target.type = settings.loop ? 1 : 0;
    return target;
}

Group12 parse_group12(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Group12 group;
    group.revision = cursor.u32();
    if (group.revision != 12)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Group revision");
    group.object_fields = parse_object_fields0(cursor, "Group");
    group.animatable = parse_animatable4(cursor, "Group");
    group.transformable = parse_transformable9(cursor, "Group");
    group.drawable = parse_drawable3(cursor, "Group");
    const uint32_t object_count =
        bounded_count(cursor, "GH2 Group object");
    group.objects.reserve(object_count);
    for (uint32_t i = 0; i < object_count; ++i)
        group.objects.push_back(cursor.string());
    group.environment = cursor.string();
    group.lod = cursor.string();
    group.lod_screen_size = cursor.f32();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 Group reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return group;
}

std::vector<uint8_t> serialize_group12(const Group12& group) {
    if (group.revision != 12)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Group revision");
    if (group.objects.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many GH2 Group objects");
    std::vector<uint8_t> out;
    append_u32(out, group.revision);
    serialize_object_fields0(out, group.object_fields, "Group");
    serialize_animatable4(out, group.animatable, "Group");
    serialize_transformable9(out, group.transformable, "Group");
    serialize_drawable3(out, group.drawable, "Group");
    append_u32(out, static_cast<uint32_t>(group.objects.size()));
    for (const auto& object : group.objects)
        append_string(out, object);
    append_string(out, group.environment);
    append_string(out, group.lod);
    append_f32(out, group.lod_screen_size);
    return out;
}

Group12 convert_view7_to_group12(
    const View& source, const ResolvedViewGraph& effective_graph) {
    if (source.revision != 7)
        throw std::runtime_error(
            "MILO object: View conversion requires GH1 revision 7");
    if (source.showing_range[0] != 0.0f ||
        source.showing_range[1] != 0.0f)
        throw std::runtime_error(
            "MILO object: non-default GH1 View showing range requires "
            "a source-backed target controller mapping");

    Group12 target;
    target.transformable =
        convert_transformable8_to_9(source.transformable);
    target.drawable = convert_drawable_to_3(source.drawable);

    auto append_unique = [&](const std::string& name) {
        if (std::find(target.objects.begin(), target.objects.end(), name) ==
            target.objects.end())
            target.objects.push_back(name);
    };
    for (const auto& object : effective_graph.animation_objects)
        append_unique(object.name);
    for (const auto& object : effective_graph.drawable_objects) {
        if (object.type == "Environ") {
            if (target.environment.empty())
                target.environment = object.name;
            continue;
        }
        if (object.type == "Cam")
            continue;
        const auto existing =
            std::find(target.objects.begin(), target.objects.end(),
                      object.name);
        if (existing != target.objects.end())
            target.objects.erase(existing);
        target.objects.push_back(object.name);
    }
    return target;
}

ObjectDir16 parse_object_dir16_body(
    Cursor& cursor, const char* owner) {
    ObjectDir16 directory;
    directory.revision = cursor.u32();
    if (directory.revision != 16)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " ObjectDir revision");
    directory.object_fields = parse_object_fields0(cursor, owner);
    const uint32_t viewport_count =
        bounded_count(cursor, "GH2 ObjectDir viewport");
    if (viewport_count > 7)
        throw std::runtime_error(
            "MILO object: implausible GH2 ObjectDir viewport count");
    directory.viewports.reserve(viewport_count);
    for (uint32_t i = 0; i < viewport_count; ++i) {
        ObjectDirViewport16 viewport;
        for (float& value : viewport.transform)
            value = cursor.f32();
        viewport.legacy_value =
            static_cast<int32_t>(cursor.u32());
        directory.viewports.push_back(viewport);
    }
    directory.current_viewport = cursor.u32();
    directory.proxy_path = cursor.string();
    const uint32_t subdirectory_count =
        bounded_count(cursor, "GH2 ObjectDir subdirectory");
    directory.subdirectories.reserve(subdirectory_count);
    for (uint32_t i = 0; i < subdirectory_count; ++i)
        directory.subdirectories.push_back(cursor.string());
    directory.legacy_string_5 = cursor.string();
    directory.legacy_string = cursor.string();
    directory.legacy_camera = cursor.string();
    return directory;
}

void serialize_object_dir16_body(
    std::vector<uint8_t>& out, const ObjectDir16& directory,
    const char* owner) {
    if (directory.revision != 16)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " ObjectDir revision");
    if (directory.viewports.size() > 7 ||
        directory.subdirectories.size() >
            std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: invalid GH2 ObjectDir vector size");
    append_u32(out, directory.revision);
    serialize_object_fields0(out, directory.object_fields, owner);
    append_u32(out, static_cast<uint32_t>(directory.viewports.size()));
    for (const auto& viewport : directory.viewports) {
        for (float value : viewport.transform) append_f32(out, value);
        append_u32(
            out, static_cast<uint32_t>(viewport.legacy_value));
    }
    append_u32(out, directory.current_viewport);
    append_string(out, directory.proxy_path);
    append_u32(
        out, static_cast<uint32_t>(directory.subdirectories.size()));
    for (const auto& path : directory.subdirectories)
        append_string(out, path);
    append_string(out, directory.legacy_string_5);
    append_string(out, directory.legacy_string);
    append_string(out, directory.legacy_camera);
}

ObjectDir16 parse_object_dir16(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    ObjectDir16 directory =
        parse_object_dir16_body(cursor, "ObjectDir");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 ObjectDir reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return directory;
}

std::vector<uint8_t> serialize_object_dir16(
    const ObjectDir16& directory) {
    std::vector<uint8_t> out;
    serialize_object_dir16_body(out, directory, "ObjectDir");
    return out;
}

RndDir8 parse_rnd_dir8(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    RndDir8 directory;
    directory.revision = cursor.u32();
    if (directory.revision != 8)
        throw std::runtime_error(
            "MILO object: unsupported GH2 RndDir revision");
    directory.object_directory =
        parse_object_dir16_body(cursor, "RndDir");
    directory.animatable = parse_animatable4(cursor, "RndDir");
    directory.drawable = parse_drawable3(cursor, "RndDir");
    directory.transformable = parse_transformable9(cursor, "RndDir");
    directory.environment = cursor.string();
    directory.test_event = cursor.string();
    directory.legacy_symbol_1 = cursor.string();
    directory.legacy_symbol_2 = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 RndDir reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return directory;
}

std::vector<uint8_t> serialize_rnd_dir8(
    const RndDir8& directory) {
    if (directory.revision != 8)
        throw std::runtime_error(
            "MILO object: unsupported GH2 RndDir revision");
    std::vector<uint8_t> out;
    append_u32(out, directory.revision);
    serialize_object_dir16_body(
        out, directory.object_directory, "RndDir");
    serialize_animatable4(out, directory.animatable, "RndDir");
    serialize_drawable3(out, directory.drawable, "RndDir");
    serialize_transformable9(
        out, directory.transformable, "RndDir");
    append_string(out, directory.environment);
    append_string(out, directory.test_event);
    append_string(out, directory.legacy_symbol_1);
    append_string(out, directory.legacy_symbol_2);
    return out;
}

RndDir8 parse_rnd_dir8_body(Cursor& cursor, const char* owner) {
    RndDir8 directory;
    directory.revision = cursor.u32();
    if (directory.revision != 8)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " RndDir revision");
    directory.object_directory =
        parse_object_dir16_body(cursor, owner);
    directory.animatable = parse_animatable4(cursor, owner);
    directory.drawable = parse_drawable3(cursor, owner);
    directory.transformable = parse_transformable9(cursor, owner);
    directory.environment = cursor.string();
    directory.test_event = cursor.string();
    directory.legacy_symbol_1 = cursor.string();
    directory.legacy_symbol_2 = cursor.string();
    return directory;
}

void serialize_rnd_dir8_body(
    std::vector<uint8_t>& out, const RndDir8& directory,
    const char* owner) {
    if (directory.revision != 8)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " RndDir revision");
    append_u32(out, directory.revision);
    serialize_object_dir16_body(
        out, directory.object_directory, owner);
    serialize_animatable4(out, directory.animatable, owner);
    serialize_drawable3(out, directory.drawable, owner);
    serialize_transformable9(out, directory.transformable, owner);
    append_string(out, directory.environment);
    append_string(out, directory.test_event);
    append_string(out, directory.legacy_symbol_1);
    append_string(out, directory.legacy_symbol_2);
}

PanelDir2 parse_panel_dir2_body(
    Cursor& cursor, const char* owner) {
    PanelDir2 directory;
    directory.revision = cursor.u32();
    if (directory.revision != 2)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " PanelDir revision");
    directory.render_directory = parse_rnd_dir8_body(cursor, owner);
    directory.camera = cursor.string();
    directory.test_event = cursor.string();
    return directory;
}

void serialize_panel_dir2_body(
    std::vector<uint8_t>& out, const PanelDir2& directory,
    const char* owner) {
    if (directory.revision != 2)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " PanelDir revision");
    append_u32(out, directory.revision);
    serialize_rnd_dir8_body(out, directory.render_directory, owner);
    append_string(out, directory.camera);
    append_string(out, directory.test_event);
}

PanelDir2 parse_panel_dir2(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    PanelDir2 directory = parse_panel_dir2_body(cursor, "PanelDir");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 PanelDir reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return directory;
}

std::vector<uint8_t> serialize_panel_dir2(
    const PanelDir2& directory) {
    std::vector<uint8_t> out;
    serialize_panel_dir2_body(out, directory, "PanelDir");
    return out;
}

WorldDir11 parse_world_dir11(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    WorldDir11 directory;
    directory.revision = cursor.u32();
    if (directory.revision != 11)
        throw std::runtime_error(
            "MILO object: unsupported GH2 WorldDir revision");
    directory.legacy_value = cursor.u32();
    directory.legacy_float = cursor.f32();
    directory.fake_hud_filename = cursor.string();
    directory.panel_directory =
        parse_panel_dir2_body(cursor, "WorldDir");
    for (float& value : directory.legacy_transform)
        value = cursor.f32();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 WorldDir reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return directory;
}

std::vector<uint8_t> serialize_world_dir11(
    const WorldDir11& directory) {
    if (directory.revision != 11)
        throw std::runtime_error(
            "MILO object: unsupported GH2 WorldDir revision");
    std::vector<uint8_t> out;
    append_u32(out, directory.revision);
    append_u32(out, directory.legacy_value);
    append_f32(out, directory.legacy_float);
    append_string(out, directory.fake_hud_filename);
    serialize_panel_dir2_body(
        out, directory.panel_directory, "WorldDir");
    for (float value : directory.legacy_transform)
        append_f32(out, value);
    return out;
}

Character9 parse_character9_body(
    Cursor& cursor, const char* owner) {
    Character9 character;
    character.revision = cursor.u32();
    if (character.revision != 9)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " Character revision");
    character.render_directory = parse_rnd_dir8_body(cursor, owner);
    const uint32_t lod_count =
        bounded_count(cursor, "GH2 Character LOD");
    character.lods.reserve(lod_count);
    for (uint32_t i = 0; i < lod_count; ++i)
        character.lods.push_back({cursor.f32(), cursor.string()});
    character.shadow = cursor.string();
    character.self_shadow = cursor.u8() != 0;
    character.sphere_base = cursor.string();
    return character;
}

void serialize_character9_body(
    std::vector<uint8_t>& out, const Character9& character,
    const char* owner) {
    if (character.revision != 9 ||
        character.lods.size() >
            std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            std::string("MILO object: invalid GH2 ") + owner +
            " Character");
    append_u32(out, character.revision);
    serialize_rnd_dir8_body(out, character.render_directory, owner);
    append_u32(out, static_cast<uint32_t>(character.lods.size()));
    for (const auto& lod : character.lods) {
        append_f32(out, lod.screen_size);
        append_string(out, lod.group);
    }
    append_string(out, character.shadow);
    append_u8(out, character.self_shadow ? 1 : 0);
    append_string(out, character.sphere_base);
}

Character9 parse_character9(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Character9 character = parse_character9_body(cursor, "Character");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 Character reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return character;
}

std::vector<uint8_t> serialize_character9(
    const Character9& character) {
    std::vector<uint8_t> out;
    serialize_character9_body(out, character, "Character");
    return out;
}

BandCharacter1 parse_band_character1(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    BandCharacter1 character;
    character.revision = cursor.u32();
    if (character.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 BandCharacter revision");
    character.character =
        parse_character9_body(cursor, "BandCharacter");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 BandCharacter reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return character;
}

std::vector<uint8_t> serialize_band_character1(
    const BandCharacter1& character) {
    if (character.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 BandCharacter revision");
    std::vector<uint8_t> out;
    append_u32(out, character.revision);
    serialize_character9_body(
        out, character.character, "BandCharacter");
    return out;
}

CharWeightable2 parse_char_weightable2_body(
    Cursor& cursor, const char* owner) {
    CharWeightable2 weightable;
    weightable.revision = cursor.u32();
    if (weightable.revision != 2)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " CharWeightable revision");
    weightable.weight = cursor.f32();
    weightable.weight_owner = cursor.string();
    return weightable;
}

void serialize_char_weightable2_body(
    std::vector<uint8_t>& out,
    const CharWeightable2& weightable,
    const char* owner) {
    if (weightable.revision != 2)
        throw std::runtime_error(
            std::string("MILO object: unsupported GH2 ") + owner +
            " CharWeightable revision");
    append_u32(out, weightable.revision);
    append_f32(out, weightable.weight);
    append_string(out, weightable.weight_owner);
}

CharDriver3 parse_char_driver3_body(Cursor& cursor) {
    CharDriver3 driver;
    driver.revision = cursor.u32();
    if (driver.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharDriver revision");
    driver.object_fields =
        parse_object_fields0(cursor, "CharDriver");
    driver.weightable =
        parse_char_weightable2_body(cursor, "CharDriver");
    driver.bones = cursor.string();
    driver.clips = cursor.string();
    driver.realign = cursor.u8() != 0;
    return driver;
}

void serialize_char_driver3_body(
    std::vector<uint8_t>& out, const CharDriver3& driver) {
    if (driver.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharDriver revision");
    append_u32(out, driver.revision);
    serialize_object_fields0(
        out, driver.object_fields, "CharDriver");
    serialize_char_weightable2_body(
        out, driver.weightable, "CharDriver");
    append_string(out, driver.bones);
    append_string(out, driver.clips);
    append_u8(out, driver.realign ? 1 : 0);
}

CharDriver3 parse_char_driver3(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharDriver3 driver = parse_char_driver3_body(cursor);
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharDriver reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return driver;
}

std::vector<uint8_t> serialize_char_driver3(
    const CharDriver3& driver) {
    std::vector<uint8_t> out;
    serialize_char_driver3_body(out, driver);
    return out;
}

CharDriverMidi3 parse_char_driver_midi3(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharDriverMidi3 midi;
    midi.revision = cursor.u32();
    if (midi.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharDriverMidi revision");
    midi.driver = parse_char_driver3_body(cursor);
    midi.default_clip = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharDriverMidi reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return midi;
}

std::vector<uint8_t> serialize_char_driver_midi3(
    const CharDriverMidi3& midi) {
    if (midi.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharDriverMidi revision");
    std::vector<uint8_t> out;
    append_u32(out, midi.revision);
    serialize_char_driver3_body(out, midi.driver);
    append_string(out, midi.default_clip);
    return out;
}

CharWeightSetter2 parse_char_weight_setter2(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharWeightSetter2 setter;
    setter.revision = cursor.u32();
    if (setter.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharWeightSetter revision");
    setter.object_fields =
        parse_object_fields0(cursor, "CharWeightSetter");
    setter.weightable =
        parse_char_weightable2_body(cursor, "CharWeightSetter");
    setter.driver = cursor.string();
    setter.flags = cursor.u32();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharWeightSetter reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return setter;
}

std::vector<uint8_t> serialize_char_weight_setter2(
    const CharWeightSetter2& setter) {
    if (setter.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharWeightSetter revision");
    std::vector<uint8_t> out;
    append_u32(out, setter.revision);
    serialize_object_fields0(
        out, setter.object_fields, "CharWeightSetter");
    serialize_char_weightable2_body(
        out, setter.weightable, "CharWeightSetter");
    append_string(out, setter.driver);
    append_u32(out, setter.flags);
    return out;
}

CharIKHand2 parse_char_ik_hand2(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharIKHand2 hand;
    hand.revision = cursor.u32();
    if (hand.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharIKHand revision");
    hand.object_fields =
        parse_object_fields0(cursor, "CharIKHand");
    hand.weightable =
        parse_char_weightable2_body(cursor, "CharIKHand");
    hand.hand = cursor.string();
    hand.target = cursor.string();
    hand.orientation = cursor.u8() != 0;
    hand.stretch = cursor.u8() != 0;
    hand.scalable = cursor.u8() != 0;
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharIKHand reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return hand;
}

std::vector<uint8_t> serialize_char_ik_hand2(
    const CharIKHand2& hand) {
    if (hand.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharIKHand revision");
    std::vector<uint8_t> out;
    append_u32(out, hand.revision);
    serialize_object_fields0(out, hand.object_fields, "CharIKHand");
    serialize_char_weightable2_body(
        out, hand.weightable, "CharIKHand");
    append_string(out, hand.hand);
    append_string(out, hand.target);
    append_u8(out, hand.orientation ? 1 : 0);
    append_u8(out, hand.stretch ? 1 : 0);
    append_u8(out, hand.scalable ? 1 : 0);
    return out;
}

CharIKMidi4 parse_char_ik_midi4(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharIKMidi4 midi;
    midi.revision = cursor.u32();
    if (midi.revision != 4)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharIKMidi revision");
    midi.object_fields =
        parse_object_fields0(cursor, "CharIKMidi");
    midi.bone = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharIKMidi reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return midi;
}

std::vector<uint8_t> serialize_char_ik_midi4(
    const CharIKMidi4& midi) {
    if (midi.revision != 4)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharIKMidi revision");
    std::vector<uint8_t> out;
    append_u32(out, midi.revision);
    serialize_object_fields0(
        out, midi.object_fields, "CharIKMidi");
    append_string(out, midi.bone);
    return out;
}

CharIKRod2 parse_char_ik_rod2(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharIKRod2 rod;
    rod.revision = cursor.u32();
    if (rod.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharIKRod revision");
    rod.object_fields =
        parse_object_fields0(cursor, "CharIKRod");
    rod.left_end = cursor.string();
    rod.right_end = cursor.string();
    rod.dest_pos = cursor.f32();
    rod.side_axis = cursor.string();
    rod.vertical = cursor.u8() != 0;
    rod.dest = cursor.string();
    for (float& value : rod.transform)
        value = cursor.f32();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharIKRod reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return rod;
}

std::vector<uint8_t> serialize_char_ik_rod2(
    const CharIKRod2& rod) {
    if (rod.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharIKRod revision");
    std::vector<uint8_t> out;
    append_u32(out, rod.revision);
    serialize_object_fields0(
        out, rod.object_fields, "CharIKRod");
    append_string(out, rod.left_end);
    append_string(out, rod.right_end);
    append_f32(out, rod.dest_pos);
    append_string(out, rod.side_axis);
    append_u8(out, rod.vertical ? 1 : 0);
    append_string(out, rod.dest);
    for (const float value : rod.transform)
        append_f32(out, value);
    return out;
}

CharHair2 parse_char_hair2(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharHair2 hair;
    hair.revision = cursor.u32();
    if (hair.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharHair revision");
    hair.object_fields =
        parse_object_fields0(cursor, "CharHair");
    hair.stiffness = cursor.f32();
    hair.torsion = cursor.f32();
    hair.inertia = cursor.f32();
    hair.gravity = cursor.f32();
    hair.weight = cursor.f32();
    hair.friction = cursor.f32();
    const uint32_t strand_count =
        bounded_count(cursor, "GH2 CharHair strands");
    hair.strands.reserve(strand_count);
    for (uint32_t i = 0; i < strand_count; ++i) {
        CharHairStrand2 strand;
        strand.root = cursor.string();
        strand.angle = cursor.f32();
        const uint32_t point_count =
            bounded_count(cursor, "GH2 CharHair strand points");
        strand.points.reserve(point_count);
        for (uint32_t j = 0; j < point_count; ++j) {
            CharHairPoint2 point;
            for (float& value : point.position)
                value = cursor.f32();
            point.bone = cursor.string();
            point.length = cursor.f32();
            point.legacy_value =
                static_cast<int32_t>(cursor.u32());
            point.legacy_name = cursor.string();
            point.radius = cursor.f32();
            point.outer_radius = cursor.f32();
            strand.points.push_back(std::move(point));
        }
        for (float& value : strand.base_matrix)
            value = cursor.f32();
        for (float& value : strand.root_matrix)
            value = cursor.f32();
        hair.strands.push_back(std::move(strand));
    }
    hair.simulate = cursor.u8() != 0;
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharHair reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return hair;
}

std::vector<uint8_t> serialize_char_hair2(
    const CharHair2& hair) {
    if (hair.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharHair revision");
    std::vector<uint8_t> out;
    append_u32(out, hair.revision);
    serialize_object_fields0(
        out, hair.object_fields, "CharHair");
    append_f32(out, hair.stiffness);
    append_f32(out, hair.torsion);
    append_f32(out, hair.inertia);
    append_f32(out, hair.gravity);
    append_f32(out, hair.weight);
    append_f32(out, hair.friction);
    append_u32(out, static_cast<uint32_t>(hair.strands.size()));
    for (const CharHairStrand2& strand : hair.strands) {
        append_string(out, strand.root);
        append_f32(out, strand.angle);
        append_u32(
            out, static_cast<uint32_t>(strand.points.size()));
        for (const CharHairPoint2& point : strand.points) {
            for (const float value : point.position)
                append_f32(out, value);
            append_string(out, point.bone);
            append_f32(out, point.length);
            append_u32(
                out, static_cast<uint32_t>(point.legacy_value));
            append_string(out, point.legacy_name);
            append_f32(out, point.radius);
            append_f32(out, point.outer_radius);
        }
        for (const float value : strand.base_matrix)
            append_f32(out, value);
        for (const float value : strand.root_matrix)
            append_f32(out, value);
    }
    append_u8(out, hair.simulate ? 1 : 0);
    return out;
}

FaceFxLipSyncServo5 parse_facefx_lip_sync_servo5(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    FaceFxLipSyncServo5 servo;
    servo.revision = cursor.u32();
    if (servo.revision != 5)
        throw std::runtime_error(
            "MILO object: unsupported GH2 FaceFxLipSyncServo revision");
    servo.object_fields =
        parse_object_fields0(cursor, "FaceFxLipSyncServo");
    servo.weightable =
        parse_char_weightable2_body(
            cursor, "FaceFxLipSyncServo");
    servo.facefx_path = cursor.string();
    servo.viseme_milo = cursor.string();
    const uint32_t target_count =
        bounded_count(cursor, "GH2 FaceFxLipSyncServo targets");
    servo.targets.reserve(target_count);
    for (uint32_t i = 0; i < target_count; ++i) {
        FaceFxLipSyncServoTarget5 target;
        target.object = cursor.string();
        target.property_type =
            static_cast<int32_t>(cursor.u32());
        target.property = cursor.string();
        servo.targets.push_back(std::move(target));
    }
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 FaceFxLipSyncServo reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return servo;
}

std::vector<uint8_t> serialize_facefx_lip_sync_servo5(
    const FaceFxLipSyncServo5& servo) {
    if (servo.revision != 5)
        throw std::runtime_error(
            "MILO object: unsupported GH2 FaceFxLipSyncServo revision");
    std::vector<uint8_t> out;
    append_u32(out, servo.revision);
    serialize_object_fields0(
        out, servo.object_fields, "FaceFxLipSyncServo");
    serialize_char_weightable2_body(
        out, servo.weightable, "FaceFxLipSyncServo");
    append_string(out, servo.facefx_path);
    append_string(out, servo.viseme_milo);
    append_u32(out, static_cast<uint32_t>(servo.targets.size()));
    for (const FaceFxLipSyncServoTarget5& target : servo.targets) {
        append_string(out, target.object);
        append_u32(
            out, static_cast<uint32_t>(target.property_type));
        append_string(out, target.property);
    }
    return out;
}

EventTrigger8 parse_event_trigger8(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    EventTrigger8 trigger;
    trigger.revision = cursor.u32();
    if (trigger.revision != 8)
        throw std::runtime_error(
            "MILO object: unsupported GH2 EventTrigger revision");
    trigger.object_fields =
        parse_object_fields0(cursor, "EventTrigger");
    trigger.trigger_event = cursor.string();
    const uint32_t animation_count =
        bounded_count(cursor, "GH2 EventTrigger animations");
    trigger.animations.reserve(animation_count);
    for (uint32_t i = 0; i < animation_count; ++i) {
        EventTriggerAnim8 animation;
        animation.animation = cursor.string();
        animation.blend = cursor.f32();
        animation.wait = cursor.u8() != 0;
        animation.delay = cursor.f32();
        trigger.animations.push_back(std::move(animation));
    }
    auto read_string_vector =
        [&cursor](const char* label) {
            const uint32_t count = bounded_count(cursor, label);
            std::vector<std::string> values;
            values.reserve(count);
            for (uint32_t i = 0; i < count; ++i)
                values.push_back(cursor.string());
            return values;
        };
    trigger.sounds =
        read_string_vector("GH2 EventTrigger sounds");
    trigger.shows =
        read_string_vector("GH2 EventTrigger shows");
    trigger.legacy_hides =
        read_string_vector("GH2 EventTrigger legacy hides");
    trigger.enable_events =
        read_string_vector("GH2 EventTrigger enable events");
    trigger.disable_events =
        read_string_vector("GH2 EventTrigger disable events");
    trigger.wait_for_events =
        read_string_vector("GH2 EventTrigger wait events");
    trigger.next_link = cursor.string();
    const uint32_t proxy_count =
        bounded_count(cursor, "GH2 EventTrigger proxy calls");
    trigger.proxy_calls.reserve(proxy_count);
    for (uint32_t i = 0; i < proxy_count; ++i) {
        EventTriggerProxyCall8 call;
        call.proxy = cursor.string();
        call.call = cursor.string();
        trigger.proxy_calls.push_back(std::move(call));
    }
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 EventTrigger reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return trigger;
}

std::vector<uint8_t> serialize_event_trigger8(
    const EventTrigger8& trigger) {
    if (trigger.revision != 8)
        throw std::runtime_error(
            "MILO object: unsupported GH2 EventTrigger revision");
    std::vector<uint8_t> out;
    append_u32(out, trigger.revision);
    serialize_object_fields0(
        out, trigger.object_fields, "EventTrigger");
    append_string(out, trigger.trigger_event);
    append_u32(
        out, static_cast<uint32_t>(trigger.animations.size()));
    for (const EventTriggerAnim8& animation : trigger.animations) {
        append_string(out, animation.animation);
        append_f32(out, animation.blend);
        append_u8(out, animation.wait ? 1 : 0);
        append_f32(out, animation.delay);
    }
    auto append_string_vector =
        [&out](const std::vector<std::string>& values) {
            append_u32(out, static_cast<uint32_t>(values.size()));
            for (const std::string& value : values)
                append_string(out, value);
        };
    append_string_vector(trigger.sounds);
    append_string_vector(trigger.shows);
    append_string_vector(trigger.legacy_hides);
    append_string_vector(trigger.enable_events);
    append_string_vector(trigger.disable_events);
    append_string_vector(trigger.wait_for_events);
    append_string(out, trigger.next_link);
    append_u32(
        out, static_cast<uint32_t>(trigger.proxy_calls.size()));
    for (const EventTriggerProxyCall8& call : trigger.proxy_calls) {
        append_string(out, call.proxy);
        append_string(out, call.call);
    }
    return out;
}

WorldFx1 parse_world_fx1(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    WorldFx1 world_fx;
    world_fx.revision = cursor.u32();
    if (world_fx.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 WorldFx revision");
    world_fx.render_directory =
        parse_rnd_dir8_body(cursor, "WorldFx");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 WorldFx reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return world_fx;
}

std::vector<uint8_t> serialize_world_fx1(
    const WorldFx1& world_fx) {
    if (world_fx.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 WorldFx revision");
    std::vector<uint8_t> out;
    append_u32(out, world_fx.revision);
    serialize_rnd_dir8_body(
        out, world_fx.render_directory, "WorldFx");
    return out;
}

OutfitLoader1 parse_outfit_loader1(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    OutfitLoader1 loader;
    loader.revision = cursor.u32();
    if (loader.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 OutfitLoader revision");
    loader.object_fields =
        parse_object_fields0(cursor, "OutfitLoader");
    loader.directory = cursor.string();
    const uint16_t category_count = cursor.u16();
    loader.categories.reserve(category_count);
    for (uint16_t i = 0; i < category_count; ++i) {
        OutfitLoaderCategory1 category;
        category.selected = cursor.u8();
        category.shown = cursor.u8();
        const uint16_t outfit_count = cursor.u16();
        category.outfits.reserve(outfit_count);
        for (uint16_t j = 0; j < outfit_count; ++j) {
            OutfitLoaderOutfit1 outfit;
            outfit.hide = cursor.u8();
            outfit.desire = cursor.u8();
            outfit.exclude = cursor.u8();
            category.outfits.push_back(outfit);
        }
        loader.categories.push_back(std::move(category));
    }
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 OutfitLoader reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return loader;
}

std::vector<uint8_t> serialize_outfit_loader1(
    const OutfitLoader1& loader) {
    if (loader.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 OutfitLoader revision");
    if (loader.categories.size() >
        std::numeric_limits<uint16_t>::max())
        throw std::runtime_error(
            "MILO object: GH2 OutfitLoader category count exceeds u16");
    std::vector<uint8_t> out;
    append_u32(out, loader.revision);
    serialize_object_fields0(
        out, loader.object_fields, "OutfitLoader");
    append_string(out, loader.directory);
    append_u16(
        out, static_cast<uint16_t>(loader.categories.size()));
    for (const OutfitLoaderCategory1& category :
         loader.categories) {
        if (category.outfits.size() >
            std::numeric_limits<uint16_t>::max())
            throw std::runtime_error(
                "MILO object: GH2 OutfitLoader outfit count exceeds u16");
        append_u8(out, category.selected);
        append_u8(out, category.shown);
        append_u16(
            out, static_cast<uint16_t>(category.outfits.size()));
        for (const OutfitLoaderOutfit1& outfit :
             category.outfits) {
            append_u8(out, outfit.hide);
            append_u8(out, outfit.desire);
            append_u8(out, outfit.exclude);
        }
    }
    return out;
}

CharLookAt2 parse_char_look_at2(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharLookAt2 look_at;
    look_at.revision = cursor.u32();
    if (look_at.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharLookAt revision");
    look_at.object_fields =
        parse_object_fields0(cursor, "CharLookAt");
    look_at.weightable =
        parse_char_weightable2_body(cursor, "CharLookAt");
    look_at.source = cursor.string();
    look_at.pivot = cursor.string();
    look_at.target = cursor.string();
    look_at.half_time = cursor.f32();
    look_at.min_yaw = cursor.f32();
    look_at.max_yaw = cursor.f32();
    look_at.min_pitch = cursor.f32();
    look_at.max_pitch = cursor.f32();
    look_at.min_weight_yaw = cursor.f32();
    look_at.max_weight_yaw = cursor.f32();
    look_at.weight_yaw_speed = cursor.f32();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharLookAt reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return look_at;
}

std::vector<uint8_t> serialize_char_look_at2(
    const CharLookAt2& look_at) {
    if (look_at.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharLookAt revision");
    std::vector<uint8_t> out;
    append_u32(out, look_at.revision);
    serialize_object_fields0(
        out, look_at.object_fields, "CharLookAt");
    serialize_char_weightable2_body(
        out, look_at.weightable, "CharLookAt");
    append_string(out, look_at.source);
    append_string(out, look_at.pivot);
    append_string(out, look_at.target);
    append_f32(out, look_at.half_time);
    append_f32(out, look_at.min_yaw);
    append_f32(out, look_at.max_yaw);
    append_f32(out, look_at.min_pitch);
    append_f32(out, look_at.max_pitch);
    append_f32(out, look_at.min_weight_yaw);
    append_f32(out, look_at.max_weight_yaw);
    append_f32(out, look_at.weight_yaw_speed);
    return out;
}

CharEyes3 parse_char_eyes3(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharEyes3 eyes;
    eyes.revision = cursor.u32();
    if (eyes.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharEyes revision");
    eyes.object_fields =
        parse_object_fields0(cursor, "CharEyes");
    const uint32_t eye_count =
        bounded_count(cursor, "GH2 CharEyes eyes");
    eyes.eyes.reserve(eye_count);
    for (uint32_t i = 0; i < eye_count; ++i)
        eyes.eyes.push_back(cursor.string());
    eyes.legacy_transform = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharEyes reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return eyes;
}

std::vector<uint8_t> serialize_char_eyes3(
    const CharEyes3& eyes) {
    if (eyes.revision != 3 ||
        eyes.eyes.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: invalid GH2 CharEyes");
    std::vector<uint8_t> out;
    append_u32(out, eyes.revision);
    serialize_object_fields0(out, eyes.object_fields, "CharEyes");
    append_u32(out, static_cast<uint32_t>(eyes.eyes.size()));
    for (const auto& eye : eyes.eyes) append_string(out, eye);
    append_string(out, eyes.legacy_transform);
    return out;
}

CharWalk1 parse_char_walk1(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharWalk1 walk;
    walk.revision = cursor.u32();
    if (walk.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharWalk revision");
    walk.object_fields = parse_object_fields0(cursor, "CharWalk");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharWalk reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return walk;
}

std::vector<uint8_t> serialize_char_walk1(
    const CharWalk1& walk) {
    if (walk.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharWalk revision");
    std::vector<uint8_t> out;
    append_u32(out, walk.revision);
    serialize_object_fields0(out, walk.object_fields, "CharWalk");
    return out;
}

CharServoBone2 parse_char_servo_bone2(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharServoBone2 servo;
    servo.revision = cursor.u32();
    if (servo.revision < 1 || servo.revision > 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharServoBone revision");
    servo.object_fields =
        parse_object_fields0(cursor, "CharServoBone");
    if (servo.revision > 1)
        servo.clip_type = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharServoBone reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return servo;
}

std::vector<uint8_t> serialize_char_servo_bone2(
    const CharServoBone2& servo) {
    if (servo.revision < 1 || servo.revision > 2 ||
        (servo.revision == 1 && !servo.clip_type.empty()))
        throw std::runtime_error(
            "MILO object: invalid GH2 CharServoBone");
    std::vector<uint8_t> out;
    append_u32(out, servo.revision);
    serialize_object_fields0(
        out, servo.object_fields, "CharServoBone");
    if (servo.revision > 1)
        append_string(out, servo.clip_type);
    return out;
}

CharUpperTwist1 parse_char_upper_twist1(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharUpperTwist1 twist;
    twist.revision = cursor.u32();
    if (twist.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharUpperTwist revision");
    twist.object_fields =
        parse_object_fields0(cursor, "CharUpperTwist");
    // The RB2 member names are decompiler-shifted; the serialized and
    // property order is upper_arm, twist1, twist2.
    twist.upper_arm = cursor.string();
    twist.twist1 = cursor.string();
    twist.twist2 = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharUpperTwist reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return twist;
}

std::vector<uint8_t> serialize_char_upper_twist1(
    const CharUpperTwist1& twist) {
    if (twist.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharUpperTwist revision");
    std::vector<uint8_t> out;
    append_u32(out, twist.revision);
    serialize_object_fields0(
        out, twist.object_fields, "CharUpperTwist");
    append_string(out, twist.upper_arm);
    append_string(out, twist.twist1);
    append_string(out, twist.twist2);
    return out;
}

CharForeTwist4 parse_char_fore_twist4(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharForeTwist4 twist;
    twist.revision = cursor.u32();
    if (twist.revision < 1 || twist.revision > 4)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharForeTwist revision");
    twist.object_fields =
        parse_object_fields0(cursor, "CharForeTwist");
    twist.offset = cursor.f32();
    twist.hand = cursor.string();
    twist.twist2 = cursor.string();
    if (twist.revision == 2)
        twist.legacy_revision2_value =
            static_cast<int32_t>(cursor.u32());
    if (twist.revision > 3)
        twist.bias = cursor.f32();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharForeTwist reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return twist;
}

std::vector<uint8_t> serialize_char_fore_twist4(
    const CharForeTwist4& twist) {
    if (twist.revision < 1 || twist.revision > 4)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharForeTwist revision");
    std::vector<uint8_t> out;
    append_u32(out, twist.revision);
    serialize_object_fields0(
        out, twist.object_fields, "CharForeTwist");
    append_f32(out, twist.offset);
    append_string(out, twist.hand);
    append_string(out, twist.twist2);
    if (twist.revision == 2)
        append_u32(
            out, static_cast<uint32_t>(
                     twist.legacy_revision2_value));
    if (twist.revision > 3)
        append_f32(out, twist.bias);
    return out;
}

CharPosConstraint2 parse_char_pos_constraint2(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharPosConstraint2 constraint;
    constraint.revision = cursor.u32();
    if (constraint.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharPosConstraint revision");
    constraint.object_fields =
        parse_object_fields0(cursor, "CharPosConstraint");
    const uint32_t target_count =
        bounded_count(cursor, "GH2 CharPosConstraint targets");
    constraint.targets.reserve(target_count);
    for (uint32_t i = 0; i < target_count; ++i)
        constraint.targets.push_back(cursor.string());
    constraint.source = cursor.string();
    for (float& value : constraint.box_min)
        value = cursor.f32();
    for (float& value : constraint.box_max)
        value = cursor.f32();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharPosConstraint reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return constraint;
}

std::vector<uint8_t> serialize_char_pos_constraint2(
    const CharPosConstraint2& constraint) {
    if (constraint.revision != 2 ||
        constraint.targets.size() >
            std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: invalid GH2 CharPosConstraint");
    std::vector<uint8_t> out;
    append_u32(out, constraint.revision);
    serialize_object_fields0(
        out, constraint.object_fields, "CharPosConstraint");
    append_u32(
        out, static_cast<uint32_t>(constraint.targets.size()));
    for (const auto& target : constraint.targets)
        append_string(out, target);
    append_string(out, constraint.source);
    for (float value : constraint.box_min)
        append_f32(out, value);
    for (float value : constraint.box_max)
        append_f32(out, value);
    return out;
}

CharClipSet14 parse_char_clip_set14(
    const std::vector<uint8_t>& bytes, uint32_t clip_count) {
    Cursor cursor(bytes);
    CharClipSet14 clips;
    clips.revision = cursor.u32();
    if (clips.revision != 14)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharClipSet revision");
    clips.object_directory =
        parse_object_dir16_body(cursor, "CharClipSet");
    clips.blend_width = cursor.f32();
    clips.play_flags = cursor.u32();
    clips.clips.reserve(clip_count);
    for (uint32_t i = 0; i < clip_count; ++i) {
        CharClipPointer14 clip;
        clip.clip = cursor.string();
        clip.flags = cursor.u32();
        clip.size_bytes = cursor.u32();
        clips.clips.push_back(clip);
    }
    clips.move_self = cursor.u8() != 0;
    const uint32_t first_count =
        bounded_count(cursor, "GH2 CharClipSet string list 1");
    clips.recenter_targets.reserve(first_count);
    for (uint32_t i = 0; i < first_count; ++i)
        clips.recenter_targets.push_back(cursor.string());
    const uint32_t second_count =
        bounded_count(cursor, "GH2 CharClipSet string list 2");
    clips.recenter_average.reserve(second_count);
    for (uint32_t i = 0; i < second_count; ++i)
        clips.recenter_average.push_back(cursor.string());
    clips.recenter_slide = cursor.u8() != 0;
    clips.legacy_type = cursor.string();
    clips.legacy_type_version =
        static_cast<int32_t>(cursor.u32());
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharClipSet reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return clips;
}

std::vector<uint8_t> serialize_char_clip_set14(
    const CharClipSet14& clips) {
    if (clips.revision != 14 ||
        clips.clips.size() > std::numeric_limits<uint32_t>::max() ||
        clips.recenter_targets.size() >
            std::numeric_limits<uint32_t>::max() ||
        clips.recenter_average.size() >
            std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: invalid GH2 CharClipSet");
    std::vector<uint8_t> out;
    append_u32(out, clips.revision);
    serialize_object_dir16_body(
        out, clips.object_directory, "CharClipSet");
    append_f32(out, clips.blend_width);
    append_u32(out, clips.play_flags);
    for (const auto& clip : clips.clips) {
        append_string(out, clip.clip);
        append_u32(out, clip.flags);
        append_u32(out, clip.size_bytes);
    }
    append_u8(out, clips.move_self ? 1 : 0);
    append_u32(
        out, static_cast<uint32_t>(clips.recenter_targets.size()));
    for (const auto& value : clips.recenter_targets)
        append_string(out, value);
    append_u32(
        out, static_cast<uint32_t>(clips.recenter_average.size()));
    for (const auto& value : clips.recenter_average)
        append_string(out, value);
    append_u8(out, clips.recenter_slide ? 1 : 0);
    append_string(out, clips.legacy_type);
    append_u32(
        out, static_cast<uint32_t>(clips.legacy_type_version));
    return out;
}

namespace {

bool clip_channel_ends_with(
    const std::string& value, const char* suffix) {
    const size_t size = std::char_traits<char>::length(suffix);
    return value.size() >= size &&
           value.compare(value.size() - size, size, suffix) == 0;
}

size_t char_clip_sample_size(
    const CharBonesSamples10& samples) {
    if (samples.compression > 4)
        throw std::runtime_error(
            "MILO object: GH2 CharBonesSamples compression exceeds 4");
    size_t size = 0;
    for (const auto& channel : samples.channels) {
        if (clip_channel_ends_with(channel, ".pos") ||
            clip_channel_ends_with(channel, ".scale")) {
            size += 12;
        } else if (clip_channel_ends_with(channel, ".quat")) {
            size += samples.compression == 0 ? 16 : 8;
        } else if (
            clip_channel_ends_with(channel, ".rotx") ||
            clip_channel_ends_with(channel, ".roty") ||
            clip_channel_ends_with(channel, ".rotz") ||
            clip_channel_ends_with(channel, ".drotx") ||
            clip_channel_ends_with(channel, ".droty") ||
            clip_channel_ends_with(channel, ".drotz")) {
            size += samples.compression == 0 ? 4 : 2;
        } else {
            throw std::runtime_error(
                "MILO object: unknown GH2 CharBonesSamples channel " +
                channel);
        }
    }
    return size;
}

CharBonesSamples10 parse_char_bones_samples10_header(
    Cursor& cursor) {
    CharBonesSamples10 samples;
    const uint32_t channel_count =
        bounded_count(cursor, "GH2 CharBonesSamples channel");
    samples.channels.reserve(channel_count);
    for (uint32_t i = 0; i < channel_count; ++i)
        samples.channels.push_back(cursor.string());
    for (uint32_t& count : samples.counts) count = cursor.u32();
    samples.compression = cursor.u32();
    samples.sample_count = cursor.u32();
    return samples;
}

void parse_char_bones_samples10_data(
    Cursor& cursor, CharBonesSamples10& samples) {
    const uint64_t byte_count =
        static_cast<uint64_t>(char_clip_sample_size(samples)) *
        samples.sample_count;
    if (byte_count > std::numeric_limits<size_t>::max())
        throw std::runtime_error(
            "MILO object: GH2 CharBonesSamples data exceeds address space");
    samples.sample_bytes =
        cursor.bytes(static_cast<size_t>(byte_count));
}

void serialize_char_bones_samples10_header(
    std::vector<uint8_t>& out,
    const CharBonesSamples10& samples,
    bool has_data = true) {
    if (samples.channels.size() >
        std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many GH2 CharBonesSamples channels");
    if (has_data) {
        const uint64_t expected =
            static_cast<uint64_t>(char_clip_sample_size(samples)) *
            samples.sample_count;
        if (expected != samples.sample_bytes.size())
            throw std::runtime_error(
                "MILO object: GH2 CharBonesSamples byte count differs");
    } else if (!samples.sample_bytes.empty()) {
        throw std::runtime_error(
            "MILO object: legacy discarded CharBonesSamples has data");
    }
    append_u32(out, static_cast<uint32_t>(samples.channels.size()));
    for (const auto& channel : samples.channels)
        append_string(out, channel);
    for (uint32_t count : samples.counts) append_u32(out, count);
    append_u32(out, samples.compression);
    append_u32(out, samples.sample_count);
}

}  // namespace

size_t char_bones_samples10_ps2_allocate_size(
    const CharBonesSamples10& samples) {
    if (samples.compression > 4)
        throw std::runtime_error(
            "MILO object: GH2 CharBonesSamples compression exceeds 4");
    size_t stride = 0;
    for (const auto& channel : samples.channels) {
        size_t field_size = 0;
        if (clip_channel_ends_with(channel, ".pos") ||
            clip_channel_ends_with(channel, ".scale")) {
            // GH2's PS2 CharBones::TypeSize stores vectors in a padded
            // 16-byte runtime slot even though the stream carries 12 bytes.
            field_size = 16;
        } else if (clip_channel_ends_with(channel, ".quat")) {
            field_size = samples.compression == 0 ? 16 : 8;
        } else if (
            clip_channel_ends_with(channel, ".rotx") ||
            clip_channel_ends_with(channel, ".roty") ||
            clip_channel_ends_with(channel, ".rotz") ||
            clip_channel_ends_with(channel, ".drotx") ||
            clip_channel_ends_with(channel, ".droty") ||
            clip_channel_ends_with(channel, ".drotz")) {
            field_size = samples.compression == 0 ? 4 : 2;
        } else {
            throw std::runtime_error(
                "MILO object: unknown GH2 CharBonesSamples channel " +
                channel);
        }
        if (stride > std::numeric_limits<size_t>::max() - field_size)
            throw std::runtime_error(
                "MILO object: GH2 CharBonesSamples stride overflow");
        stride += field_size;
    }
    if (stride > std::numeric_limits<size_t>::max() - 15)
        throw std::runtime_error(
            "MILO object: GH2 CharBonesSamples stride overflow");
    stride = (stride + 15) & ~size_t{15};
    if (samples.sample_count != 0 &&
        stride > std::numeric_limits<size_t>::max() /
                     samples.sample_count)
        throw std::runtime_error(
            "MILO object: GH2 CharBonesSamples allocation overflow");
    return stride * samples.sample_count;
}

size_t char_clip_samples10_ps2_allocate_size(
    const CharClipSamples10& clip) {
    size_t size = 0x3a0;
    for (const auto& transition : clip.transitions) {
        constexpr size_t kTransitionBytes = 0x1c;
        constexpr size_t kNodeBytes = 8;
        if (size >
                std::numeric_limits<size_t>::max() -
                    kTransitionBytes ||
            transition.nodes.size() >
                (std::numeric_limits<size_t>::max() - size -
                 kTransitionBytes) /
                    kNodeBytes)
            throw std::runtime_error(
                "MILO object: GH2 CharClip transition allocation overflow");
        size += kTransitionBytes +
                transition.nodes.size() * kNodeBytes;
    }
    for (const auto* samples : {&clip.full, &clip.one}) {
        const size_t allocation =
            char_bones_samples10_ps2_allocate_size(*samples);
        if (size > std::numeric_limits<size_t>::max() - allocation)
            throw std::runtime_error(
                "MILO object: GH2 CharClip allocation overflow");
        size += allocation;
    }
    return size;
}

CharClipSamples10 parse_char_clip_samples10(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharClipSamples10 clip;
    clip.revision = cursor.u32();
    clip.char_clip_revision = cursor.u32();
    if (clip.revision != 10 || clip.char_clip_revision != 5)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharClipSamples revision");
    clip.object_fields =
        parse_object_fields0(cursor, "CharClipSamples");
    clip.start_beat = cursor.f32();
    clip.end_beat = cursor.f32();
    clip.beats_per_second = cursor.f32();
    clip.flags = cursor.u32();
    clip.play_flags = cursor.u32();
    clip.blend_width = cursor.f32();
    clip.range = cursor.f32();
    clip.legacy_flag = cursor.u8() != 0;
    const uint32_t transition_count =
        bounded_count(cursor, "GH2 CharClip transition");
    clip.transitions.reserve(transition_count);
    for (uint32_t i = 0; i < transition_count; ++i) {
        CharClipTransition5 transition;
        transition.clip = cursor.string();
        const uint32_t node_count =
            bounded_count(cursor, "GH2 CharClip transition node");
        transition.nodes.reserve(node_count);
        for (uint32_t j = 0; j < node_count; ++j)
            transition.nodes.push_back(
                {cursor.f32(), cursor.f32()});
        clip.transitions.push_back(std::move(transition));
    }
    clip.legacy_enter_event = cursor.string();
    clip.legacy_exit_event = cursor.string();
    const uint32_t event_count =
        bounded_count(cursor, "GH2 CharClip frame event");
    clip.events.reserve(event_count);
    for (uint32_t i = 0; i < event_count; ++i)
        clip.events.push_back({cursor.f32(), cursor.string()});
    clip.full = parse_char_bones_samples10_header(cursor);
    clip.one = parse_char_bones_samples10_header(cursor);
    clip.duplicate = parse_char_bones_samples10_header(cursor);
    parse_char_bones_samples10_data(cursor, clip.full);
    parse_char_bones_samples10_data(cursor, clip.one);
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharClipSamples residual bytes=" +
            std::to_string(cursor.remaining()));
    return clip;
}

std::vector<uint8_t> serialize_char_clip_samples10(
    const CharClipSamples10& clip) {
    if (clip.revision != 10 || clip.char_clip_revision != 5 ||
        clip.transitions.size() >
            std::numeric_limits<uint32_t>::max() ||
        clip.events.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: invalid GH2 CharClipSamples");
    std::vector<uint8_t> out;
    append_u32(out, clip.revision);
    append_u32(out, clip.char_clip_revision);
    serialize_object_fields0(
        out, clip.object_fields, "CharClipSamples");
    append_f32(out, clip.start_beat);
    append_f32(out, clip.end_beat);
    append_f32(out, clip.beats_per_second);
    append_u32(out, clip.flags);
    append_u32(out, clip.play_flags);
    append_f32(out, clip.blend_width);
    append_f32(out, clip.range);
    append_u8(out, clip.legacy_flag ? 1 : 0);
    append_u32(out, static_cast<uint32_t>(clip.transitions.size()));
    for (const auto& transition : clip.transitions) {
        if (transition.nodes.size() >
            std::numeric_limits<uint32_t>::max())
            throw std::runtime_error(
                "MILO object: too many GH2 CharClip transition nodes");
        append_string(out, transition.clip);
        append_u32(
            out, static_cast<uint32_t>(transition.nodes.size()));
        for (const auto& node : transition.nodes) {
            append_f32(out, node.current_beat);
            append_f32(out, node.next_beat);
        }
    }
    append_string(out, clip.legacy_enter_event);
    append_string(out, clip.legacy_exit_event);
    append_u32(out, static_cast<uint32_t>(clip.events.size()));
    for (const auto& event : clip.events) {
        append_f32(out, event.frame);
        append_string(out, event.script);
    }
    serialize_char_bones_samples10_header(out, clip.full);
    serialize_char_bones_samples10_header(out, clip.one);
    serialize_char_bones_samples10_header(
        out, clip.duplicate, false);
    out.insert(
        out.end(), clip.full.sample_bytes.begin(),
        clip.full.sample_bytes.end());
    out.insert(
        out.end(), clip.one.sample_bytes.begin(),
        clip.one.sample_bytes.end());
    return out;
}

CharBone2 parse_char_bone2(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharBone2 bone;
    bone.revision = cursor.u32();
    if (bone.revision != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharBone revision");
    bone.object_fields = parse_object_fields0(cursor, "CharBone");
    bone.legacy_transform =
        parse_transformable9(cursor, "CharBone");
    bone.position_context = cursor.u8() != 0;
    bone.scale_context = cursor.u8() != 0;
    bone.rotation = static_cast<int32_t>(cursor.u32());
    bone.legacy_rotation = static_cast<int32_t>(cursor.u32());
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharBone residual bytes=" +
            std::to_string(cursor.remaining()));
    return bone;
}

std::vector<uint8_t> serialize_char_bone2(const CharBone2& bone) {
    if (bone.revision != 2)
        throw std::runtime_error("MILO object: invalid GH2 CharBone");
    std::vector<uint8_t> out;
    append_u32(out, bone.revision);
    serialize_object_fields0(out, bone.object_fields, "CharBone");
    serialize_transformable9(out, bone.legacy_transform, "CharBone");
    append_u8(out, bone.position_context ? 1 : 0);
    append_u8(out, bone.scale_context ? 1 : 0);
    append_u32(out, static_cast<uint32_t>(bone.rotation));
    append_u32(out, static_cast<uint32_t>(bone.legacy_rotation));
    return out;
}

CharClipFilter0 parse_char_clip_filter0(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharClipFilter0 filter;
    filter.object_fields =
        parse_object_fields0(cursor, "CharClipFilter");
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharClipFilter residual bytes=" +
            std::to_string(cursor.remaining()));
    return filter;
}

std::vector<uint8_t> serialize_char_clip_filter0(
    const CharClipFilter0& filter) {
    std::vector<uint8_t> out;
    serialize_object_fields0(
        out, filter.object_fields, "CharClipFilter");
    return out;
}

CharClipGroup1 parse_char_clip_group1(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    CharClipGroup1 group;
    group.revision = cursor.u32();
    if (group.revision != 1)
        throw std::runtime_error(
            "MILO object: unsupported GH2 CharClipGroup revision");
    group.object_fields =
        parse_object_fields0(cursor, "CharClipGroup");
    const uint32_t count =
        bounded_count(cursor, "GH2 CharClipGroup clip");
    group.clips.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
        group.clips.push_back(cursor.string());
    group.which = static_cast<int32_t>(cursor.u32());
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 CharClipGroup residual bytes=" +
            std::to_string(cursor.remaining()));
    return group;
}

std::vector<uint8_t> serialize_char_clip_group1(
    const CharClipGroup1& group) {
    if (group.revision != 1 ||
        group.clips.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: invalid GH2 CharClipGroup");
    std::vector<uint8_t> out;
    append_u32(out, group.revision);
    serialize_object_fields0(
        out, group.object_fields, "CharClipGroup");
    append_u32(out, static_cast<uint32_t>(group.clips.size()));
    for (const auto& clip : group.clips) append_string(out, clip);
    append_u32(out, static_cast<uint32_t>(group.which));
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

Tex10 parse_tex10(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Tex10 tex;
    tex.revision = cursor.u32();
    if (tex.revision != 10)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Tex revision");
    tex.object_fields.revision = cursor.u32();
    if (tex.object_fields.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Tex ObjectFields revision");
    tex.object_fields.type = cursor.string();
    tex.object_fields.has_type_properties = cursor.u8() != 0;
    if (tex.object_fields.has_type_properties)
        throw std::runtime_error(
            "MILO object: GH2 Tex TypeProps tree not yet decoded");
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
            "MILO object: truncated GH2 Tex HMX bitmap header");
    tex.has_bitmap = true;
    tex.bitmap.header_kind = cursor.u8();
    if (tex.bitmap.header_kind != 1 && tex.bitmap.header_kind != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Tex HMX bitmap header kind");
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

std::vector<uint8_t> serialize_tex10(const Tex10& tex) {
    if (tex.revision != 10 || tex.object_fields.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Tex revisions");
    if (tex.object_fields.has_type_properties)
        throw std::runtime_error(
            "MILO object: GH2 Tex TypeProps tree not yet encoded");
    std::vector<uint8_t> out;
    append_u32(out, tex.revision);
    append_u32(out, tex.object_fields.revision);
    append_string(out, tex.object_fields.type);
    append_u8(out, 0);
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
                "MILO object: GH2 Tex bitmap data without bitmap header");
        return out;
    }
    if (tex.bitmap.header_kind != 1 && tex.bitmap.header_kind != 2)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Tex HMX bitmap header kind");
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

Tex10 convert_tex8_to_tex10(const Tex& source) {
    if (source.revision != 8)
        throw std::runtime_error(
            "MILO object: Tex conversion requires GH1 revision 8");
    Tex10 target;
    target.width = source.width;
    target.height = source.height;
    target.bits_per_pixel = source.bits_per_pixel;
    target.external_path = source.external_path;
    target.mipmap_bias = source.mipmap_bias;
    target.type = source.type;
    target.use_external = source.use_external;
    target.has_bitmap = source.has_bitmap;
    target.bitmap = source.bitmap;
    return target;
}

Mat27 parse_mat27(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Mat27 mat;
    mat.revision = cursor.u32();
    if (mat.revision != 27)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Mat revision");
    mat.object_fields.revision = cursor.u32();
    if (mat.object_fields.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Mat ObjectFields revision");
    mat.object_fields.type = cursor.string();
    mat.object_fields.has_type_properties = cursor.u8() != 0;
    if (mat.object_fields.has_type_properties)
        throw std::runtime_error(
            "MILO object: GH2 Mat TypeProps tree not yet decoded");
    mat.blend = static_cast<int32_t>(cursor.u32());
    for (float& value : mat.color) value = cursor.f32();
    mat.use_environment = cursor.u8() != 0;
    mat.prelit = cursor.u8() != 0;
    mat.z_mode = static_cast<int32_t>(cursor.u32());
    mat.alpha_cut = cursor.u8() != 0;
    mat.alpha_write = cursor.u8() != 0;
    mat.tex_gen = static_cast<int32_t>(cursor.u32());
    mat.tex_wrap = static_cast<int32_t>(cursor.u32());
    for (float& value : mat.texture_transform) value = cursor.f32();
    mat.diffuse_texture = cursor.string();
    mat.next_pass = cursor.string();
    mat.intensify = cursor.u8() != 0;
    mat.cull = cursor.u8() != 0;
    mat.emissive_multiplier = cursor.f32();
    for (float& value : mat.specular_rgb) value = cursor.f32();
    mat.specular_power = cursor.f32();
    mat.normal_map = cursor.string();
    mat.emissive_map = cursor.string();
    mat.specular_map = cursor.string();
    mat.legacy_unknown_map = cursor.string();
    mat.environment_map = cursor.string();
    mat.per_pixel_lit = cursor.u8() != 0;
    mat.legacy_unknown_bool = cursor.u8() != 0;
    // Retail PS2 Mat27 ends after unkBool1. Later revisions carry the fur
    // symbol; accept it only when bytes are actually present so the same
    // semantic model can represent editor-normalized bodies as well.
    if (cursor.remaining() != 0) {
        mat.has_fur_field = true;
        mat.fur = cursor.string();
    }
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 Mat reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return mat;
}

std::vector<uint8_t> serialize_mat27(const Mat27& mat) {
    if (mat.revision != 27 || mat.object_fields.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Mat revisions");
    if (mat.object_fields.has_type_properties)
        throw std::runtime_error(
            "MILO object: GH2 Mat TypeProps tree not yet encoded");
    std::vector<uint8_t> out;
    append_u32(out, mat.revision);
    append_u32(out, mat.object_fields.revision);
    append_string(out, mat.object_fields.type);
    append_u8(out, 0);
    append_u32(out, static_cast<uint32_t>(mat.blend));
    for (float value : mat.color) append_f32(out, value);
    append_u8(out, mat.use_environment ? 1 : 0);
    append_u8(out, mat.prelit ? 1 : 0);
    append_u32(out, static_cast<uint32_t>(mat.z_mode));
    append_u8(out, mat.alpha_cut ? 1 : 0);
    append_u8(out, mat.alpha_write ? 1 : 0);
    append_u32(out, static_cast<uint32_t>(mat.tex_gen));
    append_u32(out, static_cast<uint32_t>(mat.tex_wrap));
    for (float value : mat.texture_transform) append_f32(out, value);
    append_string(out, mat.diffuse_texture);
    append_string(out, mat.next_pass);
    append_u8(out, mat.intensify ? 1 : 0);
    append_u8(out, mat.cull ? 1 : 0);
    append_f32(out, mat.emissive_multiplier);
    for (float value : mat.specular_rgb) append_f32(out, value);
    append_f32(out, mat.specular_power);
    append_string(out, mat.normal_map);
    append_string(out, mat.emissive_map);
    append_string(out, mat.specular_map);
    append_string(out, mat.legacy_unknown_map);
    append_string(out, mat.environment_map);
    append_u8(out, mat.per_pixel_lit ? 1 : 0);
    append_u8(out, mat.legacy_unknown_bool ? 1 : 0);
    if (mat.has_fur_field)
        append_string(out, mat.fur);
    return out;
}

std::vector<ConvertedMatPass> convert_mat21_to_mat27_passes(
    const Mat& source, const std::string& source_name) {
    if (source.revision != 21)
        throw std::runtime_error(
            "MILO object: Mat conversion requires GH1 revision 21");
    if (source_name.empty())
        throw std::runtime_error(
            "MILO object: Mat conversion requires an object name");
    if (source.multipass < 0 || source.multipass > 2)
        throw std::runtime_error(
            "MILO object: unsupported GH1 Mat multipass mode");

    const size_t dot = source_name.rfind('.');
    const std::string base =
        dot == std::string::npos
            ? source_name
            : source_name.substr(0, dot);

    auto target_constructor_defaults = [] {
        Mat27 target;
        // RndMat constructor defaults from the authoritative HMX target
        // implementation. These matter for passes synthesized before the
        // legacy root material's global fields are loaded.
        target.blend = 1;
        target.color = {1, 1, 1, 1};
        target.use_environment = true;
        target.prelit = false;
        target.z_mode = 1;
        target.alpha_cut = false;
        target.alpha_write = false;
        target.tex_gen = 0;
        target.tex_wrap = 1;
        target.intensify = false;
        target.cull = true;
        target.emissive_multiplier = 1.0f;
        return target;
    };

    std::vector<ConvertedMatPass> result;
    result.push_back({source_name, target_constructor_defaults()});
    Mat27& root = result.front().material;
    root.blend = static_cast<int32_t>(source.blend);
    root.color = source.color;
    root.use_environment = source.use_environment;
    root.prelit =
        source.vertex_ambient || !source.use_environment;
    root.z_mode = static_cast<int32_t>(source.z_mode);
    root.alpha_cut = source.alpha_cut;
    root.alpha_write = source.alpha_write;
    root.cull = source.cull;

    auto apply_stage =
        [](Mat27& target, const MatTexture& stage) {
        target.tex_gen = static_cast<int32_t>(stage.tex_gen);
        target.tex_wrap = static_cast<int32_t>(stage.wrap);
        target.texture_transform = stage.transform;
        target.diffuse_texture = stage.texture;
    };

    if (source.textures.empty()) return result;

    const MatTexture& first = source.textures.front();
    apply_stage(root, first);
    if (first.stage_blend == 0) {
        root.diffuse_texture.clear();
    } else if (first.stage_blend == 1) {
        // GH2 retail's revision-21 upgrade marks the root as a source pass,
        // then applies its ResetColor/lighting operation after all global
        // fields have loaded.
        root.color = {1, 1, 1, 1};
        root.use_environment = false;
        root.prelit = false;
    } else if (first.stage_blend == 3) {
        // The root's screen blend is supplied by the global Mat field, so a
        // first-stage SrcAlpha selector is retained through Intensify.
        root.intensify = true;
    }

    for (size_t i = 1; i < source.textures.size(); ++i) {
        ConvertedMatPass pass;
        pass.name =
            base + "_" + std::to_string(i) + ".mat";
        pass.material = target_constructor_defaults();
        pass.material.blend =
            static_cast<int32_t>(source.textures[i].stage_blend);
        // Every pass after the root is converted from the constructor's
        // Normal Z mode to Transparent by the retail revision upgrader.
        pass.material.z_mode = 2;
        apply_stage(pass.material, source.textures[i]);
        result.back().material.next_pass = pass.name;
        result.push_back(std::move(pass));
    }

    if (source.multipass == 1) {
        // MultipassSrc resets all non-root passes to unlit white.
        for (size_t i = 1; i < result.size(); ++i) {
            result[i].material.color = {1, 1, 1, 1};
            result[i].material.use_environment = false;
            result[i].material.prelit = false;
        }
    }

    // GH2's revision-21 loader consumes and discards Normalize because the
    // target material contract has no corresponding serialized field.
    return result;
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

Cam12 parse_cam12(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Cam12 cam;
    cam.revision = cursor.u32();
    if (cam.revision != 12)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Cam revision");
    cam.object_fields = parse_object_fields0(cursor, "Cam");
    cam.transformable = parse_transformable9(cursor, "Cam");
    cam.near_plane = cursor.f32();
    cam.far_plane = cursor.f32();
    cam.y_fov = cursor.f32();
    for (float& value : cam.screen_rect) value = cursor.f32();
    for (float& value : cam.z_range) value = cursor.f32();
    cam.target_texture = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 Cam reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return cam;
}

std::vector<uint8_t> serialize_cam12(const Cam12& cam) {
    if (cam.revision != 12)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Cam revision");
    std::vector<uint8_t> out;
    append_u32(out, cam.revision);
    serialize_object_fields0(out, cam.object_fields, "Cam");
    serialize_transformable9(out, cam.transformable, "Cam");
    append_f32(out, cam.near_plane);
    append_f32(out, cam.far_plane);
    append_f32(out, cam.y_fov);
    for (float value : cam.screen_rect) append_f32(out, value);
    for (float value : cam.z_range) append_f32(out, value);
    append_string(out, cam.target_texture);
    return out;
}

Cam12 convert_cam9_to_cam12(const Cam& source) {
    if (source.revision != 9)
        throw std::runtime_error(
            "MILO object: Cam conversion requires GH1 revision 9");
    Cam12 target;
    target.transformable =
        convert_transformable8_to_9(source.transformable);
    target.near_plane = source.near_plane;
    target.far_plane = source.far_plane;
    // RndCam::Load converts every pre-revision-12 horizontal FOV into the
    // revision-12 vertical representation using the authored 4:3 ratio.
    target.y_fov =
        std::atan(0.75f * std::tan(source.fov * 0.5f)) * 2.0f;
    target.screen_rect = source.screen_rect;
    target.z_range = source.z_range;
    target.target_texture = source.target_texture;
    return target;
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

Flare4 parse_flare4(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Flare4 flare;
    flare.revision = cursor.u32();
    if (flare.revision != 4)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Flare revision");
    flare.object_fields = parse_object_fields0(cursor, "Flare");
    flare.transformable = parse_transformable9(cursor, "Flare");
    flare.drawable = parse_drawable3(cursor, "Flare");
    flare.material = cursor.string();
    for (float& value : flare.sizes) value = cursor.f32();
    for (float& value : flare.range) value = cursor.f32();
    flare.steps = static_cast<int32_t>(cursor.u32());
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 Flare reader did not consume body end");
    return flare;
}

std::vector<uint8_t> serialize_flare4(const Flare4& flare) {
    if (flare.revision != 4)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Flare revision");
    std::vector<uint8_t> out;
    append_u32(out, flare.revision);
    serialize_object_fields0(out, flare.object_fields, "Flare");
    serialize_transformable9(out, flare.transformable, "Flare");
    serialize_drawable3(out, flare.drawable, "Flare");
    append_string(out, flare.material);
    for (float value : flare.sizes) append_f32(out, value);
    for (float value : flare.range) append_f32(out, value);
    append_u32(out, static_cast<uint32_t>(flare.steps));
    return out;
}

Flare4 convert_flare3_to_flare4(const Flare& source) {
    if (source.revision != 3)
        throw std::runtime_error(
            "MILO object: Flare conversion requires GH1 revision 3");
    Flare4 target;
    target.transformable =
        convert_transformable8_to_9(source.transformable);
    target.drawable = convert_drawable_to_3(source.drawable);
    target.material = source.material;
    target.sizes = source.sizes;
    target.range = source.range;
    target.steps = source.steps;
    return target;
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

Light6 parse_light6(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Light6 light;
    light.revision = cursor.u32();
    if (light.revision != 6)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Light revision");
    light.object_fields = parse_object_fields0(cursor, "Light");
    light.transformable = parse_transformable9(cursor, "Light");
    for (float& value : light.color) value = cursor.f32();
    light.range = cursor.f32();
    light.serialized_type = static_cast<int32_t>(cursor.u32());
    light.animate_color_from_preset = cursor.u8() != 0;
    light.animate_position_from_preset = cursor.u8() != 0;
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 Light reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return light;
}

std::vector<uint8_t> serialize_light6(const Light6& light) {
    if (light.revision != 6)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Light revision");
    std::vector<uint8_t> out;
    append_u32(out, light.revision);
    serialize_object_fields0(out, light.object_fields, "Light");
    serialize_transformable9(out, light.transformable, "Light");
    for (float value : light.color) append_f32(out, value);
    append_f32(out, light.range);
    append_u32(out, static_cast<uint32_t>(light.serialized_type));
    append_u8(out, light.animate_color_from_preset ? 1 : 0);
    append_u8(out, light.animate_position_from_preset ? 1 : 0);
    return out;
}

Light6 convert_light3_to_light6(const Light& source) {
    if (source.revision != 3)
        throw std::runtime_error(
            "MILO object: Light conversion requires GH1 revision 3");
    Light6 target;
    target.transformable =
        convert_transformable8_to_9(source.transformable);
    target.color = source.color;
    target.range = source.range;
    // Both revisions use the same pre-revision-14 serialized enum encoding;
    // retain the serialized value so runtime type conversion is identical.
    target.serialized_type = source.serialized_type;
    // Revision 6 introduced these fields with true constructor defaults.
    target.animate_color_from_preset = true;
    target.animate_position_from_preset = true;
    return target;
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

Environ5 parse_environ5(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Environ5 environment;
    environment.revision = cursor.u32();
    if (environment.revision != 5)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Environ revision");
    environment.object_fields =
        parse_object_fields0(cursor, "Environ");
    const uint32_t light_count =
        bounded_count(cursor, "GH2 Environ light");
    environment.lights.reserve(light_count);
    for (uint32_t i = 0; i < light_count; ++i)
        environment.lights.push_back(cursor.string());
    for (float& value : environment.ambient_color)
        value = cursor.f32();
    for (float& value : environment.fog_range)
        value = cursor.f32();
    for (float& value : environment.fog_color)
        value = cursor.f32();
    environment.fog_enabled = cursor.u8() != 0;
    environment.animate_from_preset = cursor.u8() != 0;
    environment.fade_out = cursor.u8() != 0;
    environment.fade_start = cursor.f32();
    environment.fade_end = cursor.f32();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 Environ reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return environment;
}

std::vector<uint8_t> serialize_environ5(
    const Environ5& environment) {
    if (environment.revision != 5)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Environ revision");
    if (environment.lights.size() >
        std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: GH2 Environ light vector too large");
    std::vector<uint8_t> out;
    append_u32(out, environment.revision);
    serialize_object_fields0(
        out, environment.object_fields, "Environ");
    append_u32(out, static_cast<uint32_t>(
                        environment.lights.size()));
    for (const auto& light : environment.lights)
        append_string(out, light);
    for (float value : environment.ambient_color)
        append_f32(out, value);
    for (float value : environment.fog_range)
        append_f32(out, value);
    for (float value : environment.fog_color)
        append_f32(out, value);
    append_u8(out, environment.fog_enabled ? 1 : 0);
    append_u8(out, environment.animate_from_preset ? 1 : 0);
    append_u8(out, environment.fade_out ? 1 : 0);
    append_f32(out, environment.fade_start);
    append_f32(out, environment.fade_end);
    return out;
}

Environ5 convert_environ1_to_environ5(const Environ& source) {
    if (source.revision != 1)
        throw std::runtime_error(
            "MILO object: Environ conversion requires GH1 revision 1");
    Environ5 target;
    target.lights = source.lights;
    target.ambient_color = source.ambient_color;
    target.fog_range = source.fog_range;
    target.fog_color = source.fog_color;
    target.fog_enabled = source.fog_enabled;
    // These fields do not exist in Environ1. Use the target class's
    // constructor defaults rather than deriving behavior from asset names.
    target.animate_from_preset = true;
    target.fade_out = false;
    target.fade_start = 0.0f;
    target.fade_end = 1000.0f;
    return target;
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
        texture.stage_blend = cursor.u32();
        texture.tex_gen = cursor.u32();
        for (float& value : texture.transform) value = cursor.f32();
        texture.wrap = cursor.u32();
        texture.texture = cursor.string();
        mat.textures.push_back(std::move(texture));
    }
    mat.blend = cursor.u32();
    for (float& value : mat.color) value = cursor.f32();
    mat.use_environment = cursor.u8() != 0;
    mat.vertex_ambient = cursor.u8() != 0;
    mat.vertex_dynamic = cursor.u8() != 0;
    mat.cull = cursor.u8() != 0;
    mat.multipass = static_cast<int32_t>(cursor.u32());
    mat.normalize = cursor.u8() != 0;
    mat.z_mode = cursor.u32();
    mat.alpha_cut = cursor.u8() != 0;
    mat.alpha_write = cursor.u8() != 0;
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
        append_u32(out, texture.stage_blend);
        append_u32(out, texture.tex_gen);
        for (float value : texture.transform) append_f32(out, value);
        append_u32(out, texture.wrap);
        append_string(out, texture.texture);
    }
    append_u32(out, mat.blend);
    for (float value : mat.color) append_f32(out, value);
    append_u8(out, mat.use_environment ? 1 : 0);
    append_u8(out, mat.vertex_ambient ? 1 : 0);
    append_u8(out, mat.vertex_dynamic ? 1 : 0);
    append_u8(out, mat.cull ? 1 : 0);
    append_u32(out, static_cast<uint32_t>(mat.multipass));
    append_u8(out, mat.normalize ? 1 : 0);
    append_u32(out, mat.z_mode);
    append_u8(out, mat.alpha_cut ? 1 : 0);
    append_u8(out, mat.alpha_write ? 1 : 0);
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

ParticleSys27 parse_particle_sys27(
    const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    ParticleSys27 particles;
    particles.revision = cursor.u32();
    if (particles.revision != 27)
        throw std::runtime_error(
            "MILO object: unsupported GH2 ParticleSys revision");
    particles.object_fields =
        parse_object_fields0(cursor, "ParticleSys");
    particles.animatable =
        parse_animatable4(cursor, "ParticleSys");
    particles.transformable =
        parse_transformable9(cursor, "ParticleSys");
    particles.drawable = parse_drawable3(cursor, "ParticleSys");
    for (float& value : particles.life) value = cursor.f32();
    for (float& value : particles.box_extent_1) value = cursor.f32();
    for (float& value : particles.box_extent_2) value = cursor.f32();
    for (float& value : particles.speed) value = cursor.f32();
    for (float& value : particles.pitch) value = cursor.f32();
    for (float& value : particles.yaw) value = cursor.f32();
    for (float& value : particles.emit_rate) value = cursor.f32();
    for (float& value : particles.start_size) value = cursor.f32();
    for (float& value : particles.delta_size) value = cursor.f32();
    for (float& value : particles.start_color_low)
        value = cursor.f32();
    for (float& value : particles.start_color_high)
        value = cursor.f32();
    for (float& value : particles.end_color_low)
        value = cursor.f32();
    for (float& value : particles.end_color_high)
        value = cursor.f32();
    particles.bounce = cursor.string();
    for (float& value : particles.force_direction) value = cursor.f32();
    particles.material = cursor.string();
    particles.type = cursor.u32();
    particles.grow_ratio = cursor.f32();
    particles.shrink_ratio = cursor.f32();
    particles.mid_color_ratio = cursor.f32();
    for (float& value : particles.mid_color_low)
        value = cursor.f32();
    for (float& value : particles.mid_color_high)
        value = cursor.f32();
    particles.max_particles = cursor.u32();
    for (float& value : particles.bubble_period) value = cursor.f32();
    for (float& value : particles.bubble_size) value = cursor.f32();
    particles.bubble = cursor.u8() != 0;
    particles.relative_motion = cursor.f32();
    particles.relative_parent = cursor.string();
    particles.emitter_mesh = cursor.string();
    particles.preserve_particles = cursor.u8() != 0;
    if (particles.preserve_particles) {
        const uint32_t count =
            bounded_count(cursor, "GH2 preserved ParticleSys particle");
        particles.particles.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            Particle27 particle;
            for (float& value : particle.position) value = cursor.f32();
            for (float& value : particle.color) value = cursor.f32();
            particle.size = cursor.f32();
            particles.particles.push_back(particle);
        }
    }
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 ParticleSys reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return particles;
}

std::vector<uint8_t> serialize_particle_sys27(
    const ParticleSys27& particles) {
    if (particles.revision != 27)
        throw std::runtime_error(
            "MILO object: unsupported GH2 ParticleSys revision");
    if (particles.particles.size() >
        std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(
            "MILO object: too many GH2 preserved particles");
    if (!particles.preserve_particles && !particles.particles.empty())
        throw std::runtime_error(
            "MILO object: GH2 ParticleSys rows without preserve flag");
    std::vector<uint8_t> out;
    append_u32(out, particles.revision);
    serialize_object_fields0(
        out, particles.object_fields, "ParticleSys");
    serialize_animatable4(out, particles.animatable, "ParticleSys");
    serialize_transformable9(
        out, particles.transformable, "ParticleSys");
    serialize_drawable3(out, particles.drawable, "ParticleSys");
    for (float value : particles.life) append_f32(out, value);
    for (float value : particles.box_extent_1) append_f32(out, value);
    for (float value : particles.box_extent_2) append_f32(out, value);
    for (float value : particles.speed) append_f32(out, value);
    for (float value : particles.pitch) append_f32(out, value);
    for (float value : particles.yaw) append_f32(out, value);
    for (float value : particles.emit_rate) append_f32(out, value);
    for (float value : particles.start_size) append_f32(out, value);
    for (float value : particles.delta_size) append_f32(out, value);
    for (float value : particles.start_color_low)
        append_f32(out, value);
    for (float value : particles.start_color_high)
        append_f32(out, value);
    for (float value : particles.end_color_low)
        append_f32(out, value);
    for (float value : particles.end_color_high)
        append_f32(out, value);
    append_string(out, particles.bounce);
    for (float value : particles.force_direction) append_f32(out, value);
    append_string(out, particles.material);
    append_u32(out, particles.type);
    append_f32(out, particles.grow_ratio);
    append_f32(out, particles.shrink_ratio);
    append_f32(out, particles.mid_color_ratio);
    for (float value : particles.mid_color_low)
        append_f32(out, value);
    for (float value : particles.mid_color_high)
        append_f32(out, value);
    append_u32(out, particles.max_particles);
    for (float value : particles.bubble_period) append_f32(out, value);
    for (float value : particles.bubble_size) append_f32(out, value);
    append_u8(out, particles.bubble ? 1 : 0);
    append_f32(out, particles.relative_motion);
    append_string(out, particles.relative_parent);
    append_string(out, particles.emitter_mesh);
    append_u8(out, particles.preserve_particles ? 1 : 0);
    if (particles.preserve_particles) {
        append_u32(
            out, static_cast<uint32_t>(particles.particles.size()));
        for (const auto& particle : particles.particles) {
            for (float value : particle.position) append_f32(out, value);
            for (float value : particle.color) append_f32(out, value);
            append_f32(out, particle.size);
        }
    }
    return out;
}

ParticleSys27 convert_particle_sys22_to_particle_sys27(
    const ParticleSys& source, const std::string& bounce_name) {
    if (source.revision != 22)
        throw std::runtime_error(
            "MILO object: ParticleSys conversion requires GH1 revision 22");
    if (source.bounce_enabled && bounce_name.empty()) {
        const float normal_squared =
            source.bounce_plane[0] * source.bounce_plane[0] +
            source.bounce_plane[1] * source.bounce_plane[1] +
            source.bounce_plane[2] * source.bounce_plane[2];
        if (normal_squared > 0.0f)
            throw std::runtime_error(
                "MILO object: GH1 ParticleSys bounce plane requires a "
                "synthesized Trans reference");
        if (!std::isfinite(normal_squared))
            throw std::runtime_error(
                "MILO object: GH1 ParticleSys bounce plane is non-finite");
    }
    ParticleSys27 target;
    target.animatable =
        convert_animatable0_to_4(source.animatable);
    target.transformable =
        convert_transformable8_to_9(source.transformable);
    target.drawable = convert_drawable_to_3(source.drawable);
    target.life = source.life;
    target.box_extent_1 = source.box_extent_1;
    target.box_extent_2 = source.box_extent_2;
    target.speed = source.speed;
    target.pitch = source.pitch;
    target.yaw = source.yaw;
    target.emit_rate = source.emit_rate;
    target.start_size = source.start_size;
    target.delta_size = source.delta_size;
    target.start_color_low = source.start_color_low;
    target.start_color_high = source.start_color_high;
    target.end_color_low = source.end_color_low;
    target.end_color_high = source.end_color_high;
    target.bounce = source.bounce_enabled ? bounce_name : std::string();
    target.force_direction = source.force_direction;
    target.material = source.material;
    target.type = source.type;
    target.grow_ratio = source.grow_ratio;
    target.shrink_ratio = source.shrink_ratio;
    target.mid_color_ratio = source.mid_color_ratio;
    target.mid_color_low = source.mid_color_low;
    target.mid_color_high = source.mid_color_high;
    target.max_particles = source.max_particles;
    target.bubble_period = source.bubble_period;
    target.bubble_size = source.bubble_size;
    target.bubble = source.bubble;
    target.relative_motion = source.relative_motion;
    target.emitter_mesh = source.emitter_mesh;
    target.preserve_particles = source.preserve_particles;
    target.particles.reserve(source.particles.size());
    for (const auto& source_particle : source.particles) {
        Particle27 particle;
        particle.position = source_particle.position;
        particle.color = source_particle.color;
        particle.size = source_particle.size;
        target.particles.push_back(particle);
    }
    return target;
}

Trans9 parse_trans9(const std::vector<uint8_t>& bytes) {
    Cursor cursor(bytes);
    Trans9 trans;
    trans.revision = cursor.u32();
    if (trans.revision != 9)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Trans revision");
    trans.object_fields = parse_object_fields0(cursor, "Trans");
    for (float& value : trans.local) value = cursor.f32();
    for (float& value : trans.world) value = cursor.f32();
    trans.constraint = cursor.u32();
    trans.target = cursor.string();
    trans.preserve_scale = cursor.u8() != 0;
    trans.parent = cursor.string();
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 Trans reader residual bytes=" +
            std::to_string(cursor.remaining()));
    return trans;
}

std::vector<uint8_t> serialize_trans9(const Trans9& trans) {
    if (trans.revision != 9)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Trans revision");
    std::vector<uint8_t> out;
    append_u32(out, trans.revision);
    serialize_object_fields0(out, trans.object_fields, "Trans");
    for (float value : trans.local) append_f32(out, value);
    for (float value : trans.world) append_f32(out, value);
    append_u32(out, trans.constraint);
    append_string(out, trans.target);
    append_u8(out, trans.preserve_scale ? 1 : 0);
    append_string(out, trans.parent);
    return out;
}

Trans9 convert_bounce_plane_to_trans9(
    const std::array<float, 4>& plane) {
    const float length_squared =
        plane[0] * plane[0] +
        plane[1] * plane[1] +
        plane[2] * plane[2];
    if (!(length_squared > 0.0f) ||
        !std::isfinite(length_squared))
        throw std::runtime_error(
            "MILO object: bounce plane has no finite normal");
    const float inverse_length = 1.0f / std::sqrt(length_squared);
    const std::array<float, 3> normal = {
        plane[0] * inverse_length,
        plane[1] * inverse_length,
        plane[2] * inverse_length};
    const float point_scale = -plane[3] / length_squared;
    const std::array<float, 3> point = {
        plane[0] * point_scale,
        plane[1] * point_scale,
        plane[2] * point_scale};
    const std::array<float, 3> helper =
        std::fabs(normal[2]) < 0.999f
            ? std::array<float, 3>{0, 0, 1}
            : std::array<float, 3>{0, 1, 0};
    std::array<float, 3> x = {
        helper[1] * normal[2] - helper[2] * normal[1],
        helper[2] * normal[0] - helper[0] * normal[2],
        helper[0] * normal[1] - helper[1] * normal[0]};
    const float x_length =
        std::sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    for (float& value : x) value /= x_length;
    const std::array<float, 3> y = {
        normal[1] * x[2] - normal[2] * x[1],
        normal[2] * x[0] - normal[0] * x[2],
        normal[0] * x[1] - normal[1] * x[0]};

    Trans9 target;
    target.local = {
        x[0], x[1], x[2],
        y[0], y[1], y[2],
        normal[0], normal[1], normal[2],
        point[0], point[1], point[2]};
    target.world = target.local;
    return target;
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

Mesh28 parse_mesh28(const std::vector<uint8_t>& bytes,
                    uint32_t parent_directory_revision) {
    Cursor cursor(bytes);
    Mesh28 mesh;
    mesh.revision = cursor.u32();
    if (mesh.revision != 28)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Mesh revision");

    mesh.object_fields.revision = cursor.u32();
    if (mesh.object_fields.revision != 0)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Mesh ObjectFields revision");
    mesh.object_fields.type = cursor.string();
    mesh.object_fields.has_type_properties = cursor.u8() != 0;
    if (mesh.object_fields.has_type_properties)
        throw std::runtime_error(
            "MILO object: GH2 Mesh TypeProps tree not yet decoded");

    mesh.transformable.revision = cursor.u32();
    if (mesh.transformable.revision != 9)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Mesh Transformable revision");
    for (float& value : mesh.transformable.local) value = cursor.f32();
    for (float& value : mesh.transformable.world) value = cursor.f32();
    mesh.transformable.constraint = cursor.u32();
    mesh.transformable.target = cursor.string();
    mesh.transformable.preserve_scale = cursor.u8() != 0;
    mesh.transformable.parent = cursor.string();

    mesh.drawable.revision = cursor.u32();
    if (mesh.drawable.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Mesh Drawable revision");
    mesh.drawable.showing = cursor.u8() != 0;
    for (float& value : mesh.drawable.sphere) value = cursor.f32();
    mesh.drawable.draw_order = cursor.f32();

    mesh.material = cursor.string();
    mesh.geometry_owner = cursor.string();
    mesh.mutable_flags = cursor.u32();
    mesh.volume = cursor.u32();

    std::function<uint32_t(uint32_t)> read_bsp =
        [&](uint32_t depth) -> uint32_t {
            if (depth > 4096)
                throw std::runtime_error(
                    "MILO object: GH2 Mesh BSP depth exceeds limit");
            if (mesh.bsp_nodes.size() >= 1000000)
                throw std::runtime_error(
                    "MILO object: GH2 Mesh BSP node count exceeds limit");
            const uint32_t index =
                static_cast<uint32_t>(mesh.bsp_nodes.size());
            mesh.bsp_nodes.emplace_back();
            const bool has_value = cursor.u8() != 0;
            mesh.bsp_nodes[index].has_value = has_value;
            if (has_value) {
                for (float& value : mesh.bsp_nodes[index].plane)
                    value = cursor.f32();
                const uint32_t left = read_bsp(depth + 1);
                const uint32_t right = read_bsp(depth + 1);
                mesh.bsp_nodes[index].left = left;
                mesh.bsp_nodes[index].right = right;
            }
            return index;
        };
    (void)read_bsp(0);

    const uint32_t vertex_count =
        bounded_count(cursor, "GH2 Mesh vertex");
    mesh.vertices.reserve(vertex_count);
    for (uint32_t i = 0; i < vertex_count; ++i) {
        MeshVertex vertex;
        for (float& value : vertex.position) value = cursor.f32();
        for (float& value : vertex.normal) value = cursor.f32();
        for (float& value : vertex.color_or_weights) value = cursor.f32();
        for (float& value : vertex.uv) value = cursor.f32();
        mesh.vertices.push_back(vertex);
    }
    const uint32_t face_count =
        bounded_count(cursor, "GH2 Mesh face");
    mesh.faces.reserve(face_count);
    for (uint32_t i = 0; i < face_count; ++i)
        mesh.faces.push_back(
            {cursor.u16(), cursor.u16(), cursor.u16()});
    const uint32_t group_count =
        bounded_count(cursor, "GH2 Mesh group");
    mesh.group_sizes = cursor.bytes(group_count);

    mesh.bone_slots[0].bone = cursor.string();
    mesh.has_bones = !mesh.bone_slots[0].bone.empty();
    if (mesh.has_bones) {
        for (size_t i = 1; i < mesh.bone_slots.size(); ++i)
            mesh.bone_slots[i].bone = cursor.string();
        for (auto& slot : mesh.bone_slots)
            for (float& value : slot.offset) value = cursor.f32();
    }

    // PS2 cached strip sections are optional even when copied group-size
    // metadata is non-zero. Geometry-owner/proxy meshes retain the owner's
    // group sizes but have no local cache payload.
    if (cursor.remaining() != 0 &&
        !mesh.group_sizes.empty() && mesh.group_sizes[0] > 0 &&
        parent_directory_revision < 25) {
        mesh.group_sections.reserve(mesh.group_sizes.size());
        for (size_t i = 0; i < mesh.group_sizes.size(); ++i) {
            MeshStripResult result;
            const uint32_t section_count =
                bounded_count(cursor, "GH2 Mesh group section");
            const uint32_t offset_count =
                bounded_count(cursor, "GH2 Mesh vertex offset");
            result.cumulative_strip_lengths.reserve(section_count);
            for (uint32_t j = 0; j < section_count; ++j)
                result.cumulative_strip_lengths.push_back(cursor.u32());
            result.strip_runs.reserve(offset_count);
            for (uint32_t j = 0; j < offset_count; ++j)
                result.strip_runs.push_back(cursor.u16());
            mesh.group_sections.push_back(std::move(result));
        }
    }
    if (cursor.remaining() != 0)
        throw std::runtime_error(
            "MILO object: GH2 Mesh reader residual bytes=" +
            std::to_string(cursor.remaining()) + " at=" +
            std::to_string(cursor.position()));
    return mesh;
}

std::vector<uint8_t> serialize_mesh28(
    const Mesh28& mesh, uint32_t parent_directory_revision) {
    if (mesh.revision != 28 ||
        mesh.object_fields.revision != 0 ||
        mesh.transformable.revision != 9 ||
        mesh.drawable.revision != 3)
        throw std::runtime_error(
            "MILO object: unsupported GH2 Mesh inheritance revision");
    if (mesh.object_fields.has_type_properties)
        throw std::runtime_error(
            "MILO object: GH2 Mesh TypeProps tree not yet encoded");
    if (mesh.vertices.size() > std::numeric_limits<uint32_t>::max() ||
        mesh.faces.size() > std::numeric_limits<uint32_t>::max() ||
        mesh.group_sizes.size() > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("MILO object: GH2 Mesh vector too large");
    if (mesh.bsp_nodes.empty())
        throw std::runtime_error("MILO object: GH2 Mesh missing BSP root");
    const bool sections_allowed =
        !mesh.group_sizes.empty() && mesh.group_sizes[0] > 0 &&
        parent_directory_revision < 25;
    if (!mesh.group_sections.empty() &&
        mesh.group_sections.size() != mesh.group_sizes.size())
        throw std::runtime_error(
            "MILO object: GH2 Mesh group section/count mismatch");
    if (!sections_allowed && !mesh.group_sections.empty())
        throw std::runtime_error(
            "MILO object: GH2 Mesh has inapplicable group sections");
    if (mesh.has_bones && mesh.bone_slots[0].bone.empty())
        throw std::runtime_error(
            "MILO object: GH2 Mesh bone block has empty first bone");
    if (!mesh.has_bones && !mesh.bone_slots[0].bone.empty())
        throw std::runtime_error(
            "MILO object: GH2 Mesh bone slots without bone flag");

    std::vector<uint8_t> out;
    append_u32(out, mesh.revision);
    append_u32(out, mesh.object_fields.revision);
    append_string(out, mesh.object_fields.type);
    append_u8(out, 0);
    append_u32(out, mesh.transformable.revision);
    for (float value : mesh.transformable.local) append_f32(out, value);
    for (float value : mesh.transformable.world) append_f32(out, value);
    append_u32(out, mesh.transformable.constraint);
    append_string(out, mesh.transformable.target);
    append_u8(out, mesh.transformable.preserve_scale ? 1 : 0);
    append_string(out, mesh.transformable.parent);
    append_u32(out, mesh.drawable.revision);
    append_u8(out, mesh.drawable.showing ? 1 : 0);
    for (float value : mesh.drawable.sphere) append_f32(out, value);
    append_f32(out, mesh.drawable.draw_order);
    append_string(out, mesh.material);
    append_string(out, mesh.geometry_owner);
    append_u32(out, mesh.mutable_flags);
    append_u32(out, mesh.volume);

    std::vector<uint8_t> visited(mesh.bsp_nodes.size(), 0);
    std::function<void(uint32_t, uint32_t)> write_bsp =
        [&](uint32_t index, uint32_t depth) {
            if (index >= mesh.bsp_nodes.size() || depth > 4096)
                throw std::runtime_error(
                    "MILO object: invalid GH2 Mesh BSP reference");
            if (visited[index])
                throw std::runtime_error(
                    "MILO object: cyclic/shared GH2 Mesh BSP node");
            visited[index] = 1;
            const auto& node = mesh.bsp_nodes[index];
            append_u8(out, node.has_value ? 1 : 0);
            if (node.has_value) {
                for (float value : node.plane) append_f32(out, value);
                write_bsp(node.left, depth + 1);
                write_bsp(node.right, depth + 1);
            }
        };
    write_bsp(0, 0);
    if (std::find(visited.begin(), visited.end(), 0) != visited.end())
        throw std::runtime_error(
            "MILO object: unreferenced GH2 Mesh BSP node");

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
    append_u32(out, static_cast<uint32_t>(mesh.group_sizes.size()));
    out.insert(out.end(), mesh.group_sizes.begin(), mesh.group_sizes.end());
    append_string(out, mesh.has_bones ? mesh.bone_slots[0].bone :
                                       std::string{});
    if (mesh.has_bones) {
        for (size_t i = 1; i < mesh.bone_slots.size(); ++i)
            append_string(out, mesh.bone_slots[i].bone);
        for (const auto& slot : mesh.bone_slots)
            for (float value : slot.offset) append_f32(out, value);
    }
    for (const auto& section : mesh.group_sections) {
        if (section.cumulative_strip_lengths.size() >
                std::numeric_limits<uint32_t>::max() ||
            section.strip_runs.size() >
                std::numeric_limits<uint32_t>::max())
            throw std::runtime_error(
                "MILO object: GH2 Mesh group section too large");
        append_u32(
            out, static_cast<uint32_t>(
                     section.cumulative_strip_lengths.size()));
        append_u32(out,
                   static_cast<uint32_t>(section.strip_runs.size()));
        for (uint32_t value : section.cumulative_strip_lengths)
            append_u32(out, value);
        for (uint16_t value : section.strip_runs) append_u16(out, value);
    }
    return out;
}

Mesh28 convert_mesh25_to_mesh28(const Mesh& source) {
    if (source.revision != 25)
        throw std::runtime_error(
            "MILO object: Mesh conversion requires GH1 revision 25");
    if (source.has_bsp_tree)
        throw std::runtime_error(
            "MILO object: GH1 non-empty BSP conversion is unsupported");
    Mesh28 target;
    target.transformable =
        convert_transformable8_to_9(source.transformable);
    target.drawable.showing = source.drawable.showing;
    target.drawable.sphere = source.drawable.sphere;
    target.material = source.material;
    target.geometry_owner = source.geometry_owner;
    target.mutable_flags = source.mutable_flags;
    target.volume = source.volume;
    target.bsp_nodes.emplace_back();
    target.vertices = source.vertices;
    target.faces = source.faces;
    target.group_sizes = source.patches;
    target.has_bones = source.has_bones;
    target.bone_slots = source.bone_slots;
    target.group_sections = source.strip_results;
    return target;
}

std::vector<uint8_t> round_trip_gh2_object_body(
    const std::string& type, const std::vector<uint8_t>& bytes,
    uint32_t parent_directory_revision) {
    if (type == "AnimFilter")
        return serialize_anim_filter1(parse_anim_filter1(bytes));
    if (type == "Cam")
        return serialize_cam12(parse_cam12(bytes));
    if (type == "CamAnim")
        return serialize_cam_anim2(parse_cam_anim2(bytes));
    if (type == "CharBone")
        return serialize_char_bone2(parse_char_bone2(bytes));
    if (type == "CharClipFilter")
        return serialize_char_clip_filter0(
            parse_char_clip_filter0(bytes));
    if (type == "CharClipGroup")
        return serialize_char_clip_group1(
            parse_char_clip_group1(bytes));
    if (type == "CharClipSamples")
        return serialize_char_clip_samples10(
            parse_char_clip_samples10(bytes));
    if (type == "CharDriver")
        return serialize_char_driver3(parse_char_driver3(bytes));
    if (type == "CharDriverMidi")
        return serialize_char_driver_midi3(
            parse_char_driver_midi3(bytes));
    if (type == "CharHair")
        return serialize_char_hair2(parse_char_hair2(bytes));
    if (type == "CharEyes")
        return serialize_char_eyes3(parse_char_eyes3(bytes));
    if (type == "CharForeTwist")
        return serialize_char_fore_twist4(
            parse_char_fore_twist4(bytes));
    if (type == "CharPosConstraint")
        return serialize_char_pos_constraint2(
            parse_char_pos_constraint2(bytes));
    if (type == "CharServoBone")
        return serialize_char_servo_bone2(
            parse_char_servo_bone2(bytes));
    if (type == "CharUpperTwist")
        return serialize_char_upper_twist1(
            parse_char_upper_twist1(bytes));
    if (type == "CharIKHand")
        return serialize_char_ik_hand2(parse_char_ik_hand2(bytes));
    if (type == "CharIKMidi")
        return serialize_char_ik_midi4(parse_char_ik_midi4(bytes));
    if (type == "CharIKRod")
        return serialize_char_ik_rod2(parse_char_ik_rod2(bytes));
    if (type == "CharLookAt")
        return serialize_char_look_at2(parse_char_look_at2(bytes));
    if (type == "CharWalk")
        return serialize_char_walk1(parse_char_walk1(bytes));
    if (type == "CharWeightSetter")
        return serialize_char_weight_setter2(
            parse_char_weight_setter2(bytes));
    if (type == "CamShot")
        return serialize_cam_shot20(parse_cam_shot20(bytes));
    if (type == "Environ")
        return serialize_environ5(parse_environ5(bytes));
    if (type == "EventTrigger")
        return serialize_event_trigger8(parse_event_trigger8(bytes));
    if (type == "EnvAnim")
        return serialize_env_anim4(parse_env_anim4(bytes));
    if (type == "Flare")
        return serialize_flare4(parse_flare4(bytes));
    if (type == "Font")
        return serialize_font15(parse_font15(bytes));
    if (type == "FaceFxLipSyncServo")
        return serialize_facefx_lip_sync_servo5(
            parse_facefx_lip_sync_servo5(bytes));
    if (type == "Group")
        return serialize_group12(parse_group12(bytes));
    if (type == "Light")
        return serialize_light6(parse_light6(bytes));
    if (type == "LightAnim")
        return serialize_light_anim2(parse_light_anim2(bytes));
    if (type == "Mat")
        return serialize_mat27(parse_mat27(bytes));
    if (type == "MatAnim")
        return serialize_mat_anim7(parse_mat_anim7(bytes));
    if (type == "Mesh")
        return serialize_mesh28(
            parse_mesh28(bytes, parent_directory_revision),
            parent_directory_revision);
    if (type == "MeshAnim")
        return serialize_mesh_anim1(parse_mesh_anim1(bytes));
    if (type == "Morph")
        return serialize_morph4(parse_morph4(bytes));
    if (type == "Movie")
        return serialize_movie8(parse_movie8(bytes));
    if (type == "MultiMesh")
        return serialize_multi_mesh1(parse_multi_mesh1(bytes));
    if (type == "OutfitLoader")
        return serialize_outfit_loader1(parse_outfit_loader1(bytes));
    if (type == "ParticleSys")
        return serialize_particle_sys27(parse_particle_sys27(bytes));
    if (type == "ParticleSysAnim")
        return serialize_particle_sys_anim3(
            parse_particle_sys_anim3(bytes));
    if (type == "Tex")
        return serialize_tex10(parse_tex10(bytes));
    if (type == "Text")
        return serialize_text17(parse_text17(bytes));
    if (type == "Trans")
        return serialize_trans9(parse_trans9(bytes));
    if (type == "TransAnim")
        return serialize_trans_anim6(parse_trans_anim6(bytes));
    if (type == "WorldFx")
        return serialize_world_fx1(parse_world_fx1(bytes));
    throw std::runtime_error(
        "MILO object: no GH2 round-trip contract for type " + type);
}

}  // namespace gh::milo_object
