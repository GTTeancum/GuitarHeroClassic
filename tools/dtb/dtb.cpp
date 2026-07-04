// dtb.cpp - see dtb.h for format provenance notes.

#include "dtb.h"

#include <cstring>
#include <fstream>
#include <sstream>

namespace gh::dtb {

namespace {

constexpr uint32_t kLcgMul = 0x41C64E6Du;
constexpr uint32_t kLcgAdd = 0x3039u;
constexpr int      kCryptIndex2Init = 0x67;
constexpr int      kCryptWrap = 0xF9;

// PS2 DTB stream cipher. The first 4 bytes of an encrypted DTB are a 32-bit
// LCG seed; this builds a 256-entry permutation table and XORs every
// subsequent byte against an evolving entry. Algorithm shape matches the
// ArkTool v6 lineage documented across the Harmonix modding community.
struct Ps2Crypt {
    uint32_t table[256];
    int      i1 = 0;
    int      i2 = kCryptIndex2Init;

    explicit Ps2Crypt(uint32_t seed) {
        uint32_t v = seed;
        for (int i = 0; i < 256; ++i) {
            uint32_t a = v * kLcgMul + kLcgAdd;
            uint32_t b = a * kLcgMul + kLcgAdd;
            table[i] = (b & 0x7FFF0000u) | (a >> 16);
            v = b;
        }
    }

    void apply(uint8_t* data, size_t n) {
        for (size_t k = 0; k < n; ++k) {
            table[i1] ^= table[i2];
            data[k] ^= static_cast<uint8_t>(table[i1] & 0xFF);
            i1 = (i1 + 1 >= kCryptWrap) ? 0 : (i1 + 1);
            i2 = (i2 + 1 >= kCryptWrap) ? 0 : (i2 + 1);
        }
    }
};

class Cursor {
public:
    Cursor(const uint8_t* p, size_t n) : p_(p), n_(n) {}

    uint8_t  u8()  { need(1); return p_[i_++]; }
    uint16_t u16() { need(2); uint16_t v; std::memcpy(&v, p_ + i_, 2); i_ += 2; return v; }
    uint32_t u32() { need(4); uint32_t v; std::memcpy(&v, p_ + i_, 4); i_ += 4; return v; }
    int32_t  s32() { return static_cast<int32_t>(u32()); }
    float    f32() { need(4); float v; std::memcpy(&v, p_ + i_, 4); i_ += 4; return v; }

    std::string str() {
        uint32_t len = u32();
        need(len);
        std::string out(reinterpret_cast<const char*>(p_ + i_), len);
        i_ += len;
        return out;
    }

    size_t pos() const { return i_; }
    size_t remaining() const { return n_ - i_; }

private:
    void need(size_t n) {
        if (i_ + n > n_) {
            std::ostringstream oss;
            oss << "DTB read past end (need " << n << " at offset " << i_
                << " of " << n_ << ")";
            throw std::runtime_error(oss.str());
        }
    }
    const uint8_t* p_;
    size_t n_;
    size_t i_ = 0;
};

// Tag classification.
bool is_string_tag(uint32_t t) {
    return t == 0x02 || t == 0x04 || t == 0x05 || t == 0x06 || t == 0x07 ||
           t == 0x08 || t == 0x09 || t == 0x12 || t == 0x20 || t == 0x21 ||
           t == 0x22 || t == 0x23 || t == 0x24;
}
bool is_array_tag(uint32_t t) { return t == 0x10 || t == 0x11 || t == 0x13; }

std::shared_ptr<Node> read_node(Cursor& c);

std::vector<std::shared_ptr<Node>> read_children(Cursor& c, uint16_t count) {
    std::vector<std::shared_ptr<Node>> out;
    out.reserve(count);
    for (uint16_t i = 0; i < count; ++i) {
        out.push_back(read_node(c));
    }
    return out;
}

std::shared_ptr<Node> read_node(Cursor& c) {
    auto n = std::make_shared<Node>();
    n->tag = c.u32();
    if (n->tag == 0x00) {
        n->value = c.s32();
    } else if (n->tag == 0x01) {
        n->value = c.f32();
    } else if (is_string_tag(n->tag)) {
        n->value = c.str();
    } else if (is_array_tag(n->tag)) {
        uint16_t count = c.u16();
        n->line = c.u32();
        n->value = read_children(c, count);
    } else {
        std::ostringstream oss;
        oss << "DTB: unknown node tag 0x" << std::hex << n->tag
            << " at offset 0x" << (c.pos() - 4);
        throw std::runtime_error(oss.str());
    }
    return n;
}

}  // anonymous namespace

Tree parse(const std::vector<uint8_t>& src) {
    if (src.empty()) throw std::runtime_error("DTB: empty input");

    auto parse_payload = [](const std::vector<uint8_t>& work) {
        if (work.empty() || work[0] != 0x01) {
            std::ostringstream oss;
            oss << "DTB: first plaintext byte = 0x"
                << std::hex << (work.empty() ? 0 : work[0]) << " (expected 0x01)";
            throw std::runtime_error(oss.str());
        }
        Cursor c(work.data(), work.size());
        (void)c.u8();  // consume the 0x01 marker
        uint16_t root_count = c.u16();
        uint32_t version = c.u32();

        Tree tree;
        tree.embedded = (version == 0);
        tree.root_line = 0;
        tree.root = read_children(c, root_count);
        return tree;
    };

    auto decrypt_payload = [&src]() {
        if (src.size() < 4) throw std::runtime_error("DTB: too short for cipher seed");
        uint32_t seed;
        std::memcpy(&seed, src.data(), 4);
        // The bytes after the seed are the encrypted payload; once decrypted
        // they should begin with the 0x01 plaintext marker.
        std::vector<uint8_t> work(src.begin() + 4, src.end());
        Ps2Crypt cipher(seed);
        cipher.apply(work.data(), work.size());
        return work;
    };

    if (src[0] == 0x01) {
        try {
            return parse_payload(src);
        } catch (const std::exception& plain_ex) {
            try {
                return parse_payload(decrypt_payload());
            } catch (const std::exception& decrypt_ex) {
                std::ostringstream oss;
                oss << plain_ex.what() << "; encrypted fallback: "
                    << decrypt_ex.what();
                throw std::runtime_error(oss.str());
            }
        }
    }
    return parse_payload(decrypt_payload());
}

// ---------------------------------------------------------------------------
// Pretty printer (DTA-style)
// ---------------------------------------------------------------------------

namespace {

std::string escape_string(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char ch : s) {
        if (ch == '"' || ch == '\\') out.push_back('\\');
        out.push_back(ch);
    }
    return out;
}

void render(std::ostringstream& os, const Node& n, int depth, bool show_lines) {
    auto indent = [&](int d) { for (int i = 0; i < d; ++i) os << "   "; };

    switch (n.tag) {
        case 0x00: os << std::get<int32_t>(n.value); break;
        case 0x01: os << std::get<float>(n.value); break;
        case 0x02: os << '$' << std::get<std::string>(n.value); break;            // variable
        case 0x05: os << std::get<std::string>(n.value); break;                   // symbol/keyword
        case 0x12: os << '"' << escape_string(std::get<std::string>(n.value)) << '"'; break;
        case 0x04: os << "kDataUnhandled"; break;   // tag with embedded name; uncommon
        case 0x06: os << "kDataUnhandled"; break;
        case 0x07: os << "#ifdef "  << std::get<std::string>(n.value); break;
        case 0x08: os << "#else";   break;
        case 0x09: os << "#endif";  break;
        case 0x20: os << "#define " << std::get<std::string>(n.value); break;
        case 0x21: os << "#include " << std::get<std::string>(n.value); break;
        case 0x22: os << "#merge "  << std::get<std::string>(n.value); break;
        case 0x23: os << "#ifndef " << std::get<std::string>(n.value); break;
        case 0x24: os << "#?24 "    << std::get<std::string>(n.value); break;
        case 0x10: case 0x11: case 0x13: {
            const char *o, *cstr;
            if      (n.tag == 0x10) { o = "(";  cstr = ")"; }
            else if (n.tag == 0x11) { o = "{";  cstr = "}"; }
            else                    { o = "[";  cstr = "]"; }

            const auto& kids = std::get<std::vector<std::shared_ptr<Node>>>(n.value);
            if (kids.empty()) { os << o << cstr; break; }

            // Compact form for small leaf-only arrays.
            bool all_leaf = true;
            for (const auto& k : kids) {
                if (is_array_tag(k->tag)) { all_leaf = false; break; }
            }
            if (all_leaf && kids.size() <= 6) {
                os << o;
                for (size_t i = 0; i < kids.size(); ++i) {
                    if (i) os << ' ';
                    render(os, *kids[i], depth, show_lines);
                }
                os << cstr;
                if (show_lines) os << " ;line " << n.line;
                break;
            }

            os << o;
            if (show_lines) os << " ;line " << n.line;
            os << '\n';
            for (const auto& k : kids) {
                indent(depth + 1);
                render(os, *k, depth + 1, show_lines);
                os << '\n';
            }
            indent(depth);
            os << cstr;
            break;
        }
        default:
            os << "<?tag 0x" << std::hex << n.tag << ">";
            break;
    }
}

}  // anonymous namespace

std::string to_dta(const Tree& tree, bool show_lines) {
    std::ostringstream os;
    for (const auto& n : tree.root) {
        render(os, *n, 0, show_lines);
        os << '\n';
    }
    return os.str();
}

// ---------------------------------------------------------------------------
// Query helpers
// ---------------------------------------------------------------------------

bool is_array(const Node& n) {
    return n.tag == 0x10 || n.tag == 0x11 || n.tag == 0x13;
}

bool is_string_like(const Node& n) {
    return n.tag == 0x02 || n.tag == 0x04 || n.tag == 0x05 || n.tag == 0x06 ||
           n.tag == 0x07 || n.tag == 0x08 || n.tag == 0x09 || n.tag == 0x12 ||
           n.tag == 0x20 || n.tag == 0x21 || n.tag == 0x22 || n.tag == 0x23 ||
           n.tag == 0x24;
}

static const NodeList kEmptyList;

const NodeList& children(const Node& n) {
    if (!is_array(n)) return kEmptyList;
    return std::get<NodeList>(n.value);
}

std::optional<int32_t> as_int(const Node& n) {
    if (n.tag == 0x00) return std::get<int32_t>(n.value);
    return std::nullopt;
}

std::optional<float> as_float(const Node& n) {
    if (n.tag == 0x01) return std::get<float>(n.value);
    if (n.tag == 0x00) return static_cast<float>(std::get<int32_t>(n.value));
    return std::nullopt;
}

std::optional<std::string> as_string(const Node& n) {
    if (is_string_like(n)) return std::get<std::string>(n.value);
    return std::nullopt;
}

std::shared_ptr<Node> find_keyed(const Node& parent, std::string_view key) {
    if (!is_array(parent)) return nullptr;
    for (const auto& child : std::get<NodeList>(parent.value)) {
        if (!child || !is_array(*child)) continue;
        const auto& gc = std::get<NodeList>(child->value);
        if (gc.empty() || !is_string_like(*gc[0])) continue;
        if (std::get<std::string>(gc[0]->value) == key) return child;
    }
    return nullptr;
}

std::shared_ptr<Node> find_keyed(const Tree& tree, std::string_view key) {
    for (const auto& root_node : tree.root) {
        if (!root_node || !is_array(*root_node)) continue;
        const auto& gc = std::get<NodeList>(root_node->value);
        if (gc.empty() || !is_string_like(*gc[0])) continue;
        if (std::get<std::string>(gc[0]->value) == key) return root_node;
    }
    return nullptr;
}

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("cannot open " + path);
    auto sz = static_cast<std::streamsize>(f.tellg());
    std::vector<uint8_t> buf(static_cast<size_t>(sz));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    if (!f) throw std::runtime_error("short read on " + path);
    return buf;
}

}  // namespace gh::dtb
