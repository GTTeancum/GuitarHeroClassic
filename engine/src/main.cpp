// ghogx - GuitarHeroOGX native engine entrypoint.
//
// Multi-mode CLI over a PS2 Harmonix ARK. All reads go through the static
// libraries under tools/; no 360-binary involvement, no shim layer.
//
// Subcommands:
//   songs        List the song catalog (config/gen/songs.dtb)
//   venues       List venues discovered via world/<x>/gen/<x>.dtb
//   chars        Aggregate character outfits / guitars / venues referenced
//                across all songs
//   all          songs + venues + chars in one run
//   tex-from-milo --milo-path <p> --out-dir <d>
//                Decompress a milo, extract every Tex-class entry, decode
//                and write each as a 32-bit BMP.

#include "ark_v3.h"
#include "dtb.h"
#include "milo.h"
#include "ps2_texture.h"
#include "catalog.h"
#include "venue_catalog.h"
#include "character_catalog.h"
#include "milo_tex.h"
#include "milo_bridge/milo_bridge.h"
#include "milo_scene/milo_scene.h"
#include "core/object_dir.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

void usage() {
    std::fprintf(stderr,
        "ghogx - GuitarHeroOGX native engine\n"
        "\n"
        "Usage:\n"
        "  ghogx <subcommand> [--ark-dir <dir> | --hdr <p> --ark <p>] [opts]\n"
        "\n"
        "Subcommands:\n"
        "  songs                              song catalog (default)\n"
        "  venues                             venue list\n"
        "  chars                              character/guitar/venue aggregate from songs\n"
        "  all                                songs + venues + chars\n"
        "  tex-from-milo --milo-path <p>\n"
        "                --out-dir <d>        extract textures from a milo\n"
        "  dump --milo-path <p>\n"
        "       [--filter <type>]             hex-dump MILO entry bodies\n"
        "  objdir --milo-path <p>             load a milo's object directory and\n"
        "                                     print the runtime ObjectDir tree\n"
        "  mesh --milo-path <p> [--name <m>]  decode Mesh/Trans/Mat render objects;\n"
        "                                     report vtx/face counts + bbox + material\n"
        "  list [--filter <substr>]           enumerate ARK entry paths\n"
        "\n"
        "Common options:\n"
        "  --ark-dir <dir>                    dir with MAIN.HDR + MAIN_0.ARK\n"
        "  --hdr <p> --ark <p>                explicit paths\n"
        "  --json                             JSON output (songs/chars/venues)\n"
        "  --catalog <path>                   override songs DTB path\n");
    std::exit(2);
}

struct Args {
    std::string sub = "songs";
    std::string hdr;
    std::string ark;
    std::string catalog_path = "config/gen/songs.dtb";
    std::string milo_path;
    std::string out_dir;
    std::string filter;  // substring filter for the `list` subcommand
    std::string name;    // exact entry-name match for `dump` / `mesh`
    bool json = false;
};

Args parse_args(int argc, char** argv) {
    Args a;
    int i = 1;
    if (argc > 1 && argv[1][0] != '-') { a.sub = argv[1]; i = 2; }

    std::string ark_dir;
    for (; i < argc; ++i) {
        std::string_view k = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) { std::fprintf(stderr, "%s requires a value\n", name); std::exit(2); }
            return argv[++i];
        };
        if (k == "--ark-dir")        ark_dir = need("--ark-dir");
        else if (k == "--hdr")       a.hdr = need("--hdr");
        else if (k == "--ark")       a.ark = need("--ark");
        else if (k == "--catalog")   a.catalog_path = need("--catalog");
        else if (k == "--milo-path") a.milo_path = need("--milo-path");
        else if (k == "--out-dir")   a.out_dir = need("--out-dir");
        else if (k == "--filter")    a.filter = need("--filter");
        else if (k == "--name")      a.name = need("--name");
        else if (k == "--json")      a.json = true;
        else if (k == "-h" || k == "--help") usage();
        else { std::fprintf(stderr, "unknown arg: %s\n", argv[i]); usage(); }
    }

    if (!ark_dir.empty()) {
        fs::path d = ark_dir;
        for (auto hdr_name : {"main.hdr", "MAIN.HDR"}) {
            if (fs::exists(d / hdr_name)) { a.hdr = (d / hdr_name).string(); break; }
        }
        for (auto ark_name : {"main_0.ark", "MAIN_0.ARK"}) {
            if (fs::exists(d / ark_name)) { a.ark = (d / ark_name).string(); break; }
        }
    }
    if (a.hdr.empty() || a.ark.empty()) {
        std::fprintf(stderr, "need --ark-dir or both --hdr and --ark\n");
        usage();
    }
    return a;
}

std::optional<gh::ark::Entry> find_in_ark(const gh::ark::ArkV3Reader& ark,
                                          const std::string& path) {
    auto e = ark.find(path);
    if (!e) {
        std::string alt = "../../system/run/" + path;
        e = ark.find(alt);
    }
    return e;
}

std::vector<ghogx::catalog::Song> load_songs(const gh::ark::ArkV3Reader& ark,
                                             const std::string& ark_path,
                                             const std::string& dtb_path) {
    auto e = find_in_ark(ark, dtb_path);
    if (!e) throw std::runtime_error("catalog DTB not found: " + dtb_path);
    auto bytes = ark.read_entry(*e, {ark_path});
    auto tree = gh::dtb::parse(bytes);
    return ghogx::catalog::extract_songs(tree);
}

// ----- printers ------------------------------------------------------------

void print_songs_table(const std::vector<ghogx::catalog::Song>& songs) {
    std::printf("\n%-22s  %-32s  %-22s  %-10s  %-10s  %s\n",
                "shortname", "name", "artist", "guitar", "venue", "preview (s)");
    std::printf("%s\n", std::string(120, '-').c_str());
    for (const auto& s : songs) {
        std::string g, v;
        if (s.quickplay) { g = s.quickplay->guitar; v = s.quickplay->venue; }
        char preview[32] = "";
        if (s.preview_start_ms && s.preview_end_ms) {
            std::snprintf(preview, sizeof(preview), "%5.1f - %5.1f",
                          *s.preview_start_ms / 1000.0,
                          *s.preview_end_ms   / 1000.0);
        }
        std::printf("%-22s  %-32s  %-22s  %-10s  %-10s  %s\n",
                    s.shortname.c_str(),
                    s.display_name.substr(0, 32).c_str(),
                    s.artist.substr(0, 22).c_str(),
                    g.c_str(), v.c_str(), preview);
    }
    std::printf("\n%zu songs total\n", songs.size());
}

void print_venues_table(const std::vector<ghogx::catalog::Venue>& vs) {
    std::printf("\n%-16s  %-22s  %s\n", "shortname", "sound_bank", "crowd_levels");
    std::printf("%s\n", std::string(60, '-').c_str());
    for (const auto& v : vs) {
        std::printf("%-16s  %-22s  %d\n",
                    v.shortname.c_str(),
                    v.sound_bank ? v.sound_bank->c_str() : "(unknown)",
                    v.crowd_levels);
    }
    std::printf("\n%zu venues total\n", vs.size());
}

void print_chars_table(const ghogx::catalog::CharacterAggregate& agg) {
    auto print_list = [](const char* label,
                         const std::vector<std::pair<std::string, int>>& xs) {
        std::printf("\n%s (%zu unique):\n", label, xs.size());
        for (const auto& [k, n] : xs) std::printf("  %4d  %s\n", n, k.c_str());
    };
    print_list("Character outfits",       agg.outfits);
    print_list("Guitar models",           agg.guitars);
    print_list("Default venues (by song)", agg.venues);
}

// ----- tex-from-milo -------------------------------------------------------

int run_tex_from_milo(const Args& a, const gh::ark::ArkV3Reader& ark) {
    if (a.milo_path.empty() || a.out_dir.empty()) {
        std::fprintf(stderr, "tex-from-milo needs --milo-path and --out-dir\n");
        return 2;
    }
    auto e = find_in_ark(ark, a.milo_path);
    if (!e) {
        std::fprintf(stderr, "milo not found in ARK: %s\n", a.milo_path.c_str());
        return 1;
    }
    std::fprintf(stderr, "[ghogx] reading milo %s (%u bytes)\n",
                 a.milo_path.c_str(), e->size);
    auto bytes = ark.read_entry(*e, {a.ark});

    auto hdr = gh::milo::parse_header(bytes);
    std::fprintf(stderr, "[ghogx] milo structure 0x%08X, %u blocks\n",
                 static_cast<uint32_t>(hdr.structure), hdr.block_count);
    auto payload = gh::milo::inflate_payload(bytes, hdr);
    std::fprintf(stderr, "[ghogx] inflated to %zu bytes\n", payload.size());
    auto dir = gh::milo::parse_directory(payload);
    std::fprintf(stderr, "[ghogx] dir v%d type=%s name=%s entries=%zu\n",
                 dir.dir_version, dir.dir_type.c_str(),
                 dir.dir_name.c_str(), dir.entries.size());

    fs::create_directories(a.out_dir);
    int ok = 0, fail = 0, skipped = 0;
    for (const auto& entry : dir.entries) {
        if (entry.type != "Tex") continue;
        try {
            std::vector<uint8_t> tex_bytes(payload.data() + entry.offset,
                                           payload.data() + entry.offset + entry.size);
            auto tex = ghogx::milo::parse_tex_entry(entry.name, tex_bytes);
            if (tex.use_external) {
                std::fprintf(stderr, "  [skip] %s -> external %s\n",
                             entry.name.c_str(), tex.external_path.c_str());
                ++skipped; continue;
            }
            if (tex.bitmap.encoding != 3) {
                std::fprintf(stderr, "  [skip] %s -> encoding %d (not indexed)\n",
                             entry.name.c_str(), tex.bitmap.encoding);
                ++skipped; continue;
            }
            auto rgba = gh::tex::decode_to_rgba(tex.bitmap);
            std::string safe = entry.name;
            for (auto& c : safe) { if (c == '/' || c == '\\') c = '_'; }
            fs::path dst = fs::path(a.out_dir) / (safe + ".bmp");
            gh::tex::write_bmp32(dst.string(), tex.bitmap.width, tex.bitmap.height, rgba);
            ++ok;
        } catch (const std::exception& ex) {
            std::fprintf(stderr, "  [fail] %s: %s\n", entry.name.c_str(), ex.what());
            ++fail;
        }
    }
    std::printf("decoded %d, skipped %d, failed %d  -> %s\n",
                ok, skipped, fail, a.out_dir.c_str());
    return fail > 0 ? 1 : 0;
}

// ----- dump ----------------------------------------------------------------
// Hex-dump the raw bytes of named entries from a MILO. Crucial for decoding
// binary formats (Trans/Mesh/Mat/...) that have no community reader yet.
// Usage: ghogx dump --milo-path <p> [--filter <type>]
// With --filter, only entries whose TYPE matches the filter are dumped.
// Prints up to 256 bytes per entry in hexdump format, with ASCII.
int run_dump(const Args& a, const gh::ark::ArkV3Reader& ark) {
    if (a.milo_path.empty()) {
        std::fprintf(stderr, "dump needs --milo-path\n");
        return 2;
    }
    auto e = find_in_ark(ark, a.milo_path);
    if (!e) {
        std::fprintf(stderr, "milo not found: %s\n", a.milo_path.c_str());
        return 1;
    }
    auto bytes = ark.read_entry(*e, {a.ark});
    auto hdr = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, hdr);
    auto dir = gh::milo::parse_directory(payload);

    const std::string& flt = a.filter;  // filter by entry type

    // Special filter "@dir": dump the directory's OWN object body (holds the
    // dir instance properties, e.g. TrackDir y_per_second / slots).
    if (flt == "@dir") {
        std::printf("\n=== %s dir-object  type=%s name=%s  offset=%llu size=%llu ===\n",
                    a.milo_path.c_str(), dir.dir_type.c_str(), dir.dir_name.c_str(),
                    (unsigned long long)dir.dir_entry_offset,
                    (unsigned long long)dir.dir_entry_size);
        const uint8_t* src = payload.data() + dir.dir_entry_offset;
        const size_t show = std::min<size_t>(dir.dir_entry_size, 1024);
        for (size_t row = 0; row < show; row += 16) {
            const size_t cols = std::min(show - row, (size_t)16);
            std::printf("  %06zx  ", row);
            for (size_t c = 0; c < cols; ++c) std::printf("%02x%s", src[row+c], c==7?"  ":" ");
            for (size_t c = cols; c < 16; ++c) std::printf("   ");
            std::printf(" |");
            for (size_t c = 0; c < cols; ++c) { char ch=(char)src[row+c]; std::printf("%c",(ch>=0x20&&ch<0x7f)?ch:'.'); }
            std::printf("|\n");
        }
        // Also interpret every 4-byte window as a float in a plausible range,
        // to spot config values (top_y ~110, remove_y ~-15, slot x ~+-8/+-4).
        std::printf("\n  plausible floats (|v| in [0.01, 100000], at each offset):\n");
        for (size_t o = 0; o + 4 <= dir.dir_entry_size; ++o) {
            float f; std::memcpy(&f, src + o, 4);
            const float a2 = f < 0 ? -f : f;
            if (a2 >= 0.01f && a2 <= 100000.0f) {
                std::printf("    +%04zx  %g\n", o, f);
            }
        }
        return 0;
    }

    // With --name, dump the FULL entry body (no 256 cap) plus a float column,
    // for hand-decoding a specific Mesh/Trans/Mat. Otherwise cap at 256.
    const bool full = !a.name.empty();

    int dumped = 0;
    for (const auto& de : dir.entries) {
        if (full) {
            if (de.name != a.name) continue;
        } else if (!flt.empty()) {
            // case-insensitive type filter
            std::string t = de.type;
            std::string f = flt;
            std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c){ return std::tolower(c); });
            std::transform(f.begin(), f.end(), f.begin(), [](unsigned char c){ return std::tolower(c); });
            if (t.find(f) == std::string::npos) continue;
        }
        std::printf("\n=== %s  '%s'  offset=%llu  size=%llu ===\n",
                    de.type.c_str(), de.name.c_str(),
                    static_cast<unsigned long long>(de.offset),
                    static_cast<unsigned long long>(de.size));
        if (de.size == 0) { std::printf("  (empty)\n"); continue; }

        const uint8_t* src = payload.data() + de.offset;
        const size_t show = full ? de.size : std::min(de.size, static_cast<uint64_t>(256));
        for (size_t row = 0; row < show; row += 16) {
            const size_t cols = std::min(show - row, static_cast<size_t>(16));
            std::printf("  %06zx  ", row);
            for (size_t c = 0; c < cols; ++c) {
                std::printf("%02x%s", src[row+c], (c == 7 ? "  " : " "));
            }
            for (size_t c = cols; c < 16; ++c) std::printf("   ");
            std::printf(" |");
            for (size_t c = 0; c < cols; ++c) {
                char ch = static_cast<char>(src[row+c]);
                std::printf("%c", (ch >= 0x20 && ch < 0x7f) ? ch : '.');
            }
            std::printf("|\n");
        }
        if (!full && de.size > 256)
            std::printf("  ... (%llu more bytes)\n",
                        static_cast<unsigned long long>(de.size - 256));
        ++dumped;
    }
    std::printf("\n%d entr%s dumped from %s\n", dumped, dumped==1?"y":"ies",
                a.milo_path.c_str());
    return 0;
}

// ----- dtb -----------------------------------------------------------------
// Parse a DTB config file from the ARK and print it as readable DTA text.
// Usage: ghogx dtb --milo-path config/gen/track_graphics.dtb
int run_dtb(const Args& a, const gh::ark::ArkV3Reader& ark) {
    if (a.milo_path.empty()) {
        std::fprintf(stderr, "dtb needs --milo-path <dtb-path>\n");
        return 2;
    }
    auto e = find_in_ark(ark, a.milo_path);
    if (!e) {
        std::fprintf(stderr, "dtb not found: %s\n", a.milo_path.c_str());
        return 1;
    }
    auto bytes = ark.read_entry(*e, {a.ark});
    auto tree = gh::dtb::parse(bytes);
    std::string text = gh::dtb::to_dta(tree);
    std::fwrite(text.data(), 1, text.size(), stdout);
    std::fputc('\n', stdout);
    return 0;
}

// ----- objdir --------------------------------------------------------------
// Load a MILO's object directory into the runtime ObjectDir tree and print it
// (the structural MILO load: dir metadata + each child's class + name). Real
// asset exercise of ghogx_milo_bridge; the unit test covers it hermetically.
int run_objdir(const Args& a) {
    if (a.milo_path.empty()) {
        std::fprintf(stderr, "objdir needs --milo-path\n");
        return 2;
    }
    auto od = ghogx::milo_bridge::load_object_dir(a.hdr, a.ark, a.milo_path);
    if (!od) return 1;

    std::printf("\nObjectDir  name=%s  dir_type=%s  children=%zu\n",
                od->name().c_str(), od->dir_type().c_str(), od->size());
    std::printf("%s\n", std::string(56, '-').c_str());
    for (std::size_t i = 0; i < od->size(); ++i) {
        const ghogx::Object* c = od->at(i);
        std::printf("  %3zu  %-18s  %s\n", i, c->class_name().c_str(),
                    c->name().c_str());
    }
    std::printf("\n");
    return 0;
}

// ----- mesh ----------------------------------------------------------------
// Decode the 3-D render objects of a MILO and report mesh stats: vertex_count,
// face_count, bounding box (min/max xyz), and material name. With --name, only
// that one Mesh entry; otherwise every Mesh in the MILO (summary). Verifies the
// Mesh/Trans/Mat byte decode against entry size.
//   ghogx mesh --milo-path <p> [--name <mesh>]
int run_mesh(const Args& a) {
    if (a.milo_path.empty()) {
        std::fprintf(stderr, "mesh needs --milo-path\n");
        return 2;
    }
    ghogx::milo_scene::Scene scene;
    if (!ghogx::milo_scene::load_scene(a.hdr, a.ark, a.milo_path, scene)) return 1;

    std::printf("\nScene  name=%s  dir_type=%s  meshes=%zu  trans=%zu  mat=%zu\n",
                scene.dir_name.c_str(), scene.dir_type.c_str(),
                scene.meshes.size(), scene.transes.size(), scene.mats.size());
    std::printf("%s\n", std::string(96, '-').c_str());
    std::printf("  %-26s %7s %7s  %-16s  %s\n", "mesh", "verts", "faces",
                "material", "bbox (min..max xyz)");

    int shown = 0, ok = 0, fail = 0;
    for (const auto& m : scene.meshes) {
        if (!a.name.empty() && m.name != a.name) continue;
        ++shown;
        if (m.decoded) ++ok; else ++fail;
        if (m.decoded) {
            std::printf("  %-26s %7u %7u  %-16s  [%.2f %.2f %.2f]..[%.2f %.2f %.2f]\n",
                        m.name.substr(0, 26).c_str(), m.vertex_count, m.face_count,
                        m.material.substr(0, 16).c_str(),
                        m.bb_min[0], m.bb_min[1], m.bb_min[2],
                        m.bb_max[0], m.bb_max[1], m.bb_max[2]);
            if (!a.name.empty()) {
                std::printf("      parent=%s\n", m.parent.c_str());
                std::printf("      local pos=[%.4f %.4f %.4f]\n",
                            m.local.pos[0], m.local.pos[1], m.local.pos[2]);
                std::printf("      local row0=[%.4f %.4f %.4f]\n",
                            m.local.rot[0][0], m.local.rot[0][1], m.local.rot[0][2]);
                std::printf("      local row1=[%.4f %.4f %.4f]\n",
                            m.local.rot[1][0], m.local.rot[1][1], m.local.rot[1][2]);
                std::printf("      local row2=[%.4f %.4f %.4f]\n",
                            m.local.rot[2][0], m.local.rot[2][1], m.local.rot[2][2]);
                std::printf("      world pos=[%.4f %.4f %.4f]\n",
                            m.world_stored.pos[0], m.world_stored.pos[1],
                            m.world_stored.pos[2]);
                std::printf("      world row0=[%.4f %.4f %.4f]\n",
                            m.world_stored.rot[0][0], m.world_stored.rot[0][1],
                            m.world_stored.rot[0][2]);
                std::printf("      world row1=[%.4f %.4f %.4f]\n",
                            m.world_stored.rot[1][0], m.world_stored.rot[1][1],
                            m.world_stored.rot[1][2]);
                std::printf("      world row2=[%.4f %.4f %.4f]\n",
                            m.world_stored.rot[2][0], m.world_stored.rot[2][1],
                            m.world_stored.rot[2][2]);
            }
        } else {
            std::printf("  %-26s   FAIL: %s\n", m.name.substr(0, 26).c_str(),
                        m.error.c_str());
        }
    }
    if (!a.name.empty()) {
        for (const auto& t : scene.transes) {
            if (t.name != a.name) continue;
            ++shown;
            ++ok;
            std::printf("  %-26s %7s %7s  %-16s  %s\n",
                        t.name.substr(0, 26).c_str(), "-", "-", "Trans",
                        "(transform)");
            std::printf("      parent=%s\n", t.parent.c_str());
            std::printf("      local pos=[%.4f %.4f %.4f]\n",
                        t.local.pos[0], t.local.pos[1], t.local.pos[2]);
            std::printf("      local row0=[%.4f %.4f %.4f]\n",
                        t.local.rot[0][0], t.local.rot[0][1],
                        t.local.rot[0][2]);
            std::printf("      local row1=[%.4f %.4f %.4f]\n",
                        t.local.rot[1][0], t.local.rot[1][1],
                        t.local.rot[1][2]);
            std::printf("      local row2=[%.4f %.4f %.4f]\n",
                        t.local.rot[2][0], t.local.rot[2][1],
                        t.local.rot[2][2]);
            std::printf("      world pos=[%.4f %.4f %.4f]\n",
                        t.world_stored.pos[0], t.world_stored.pos[1],
                        t.world_stored.pos[2]);
            std::printf("      world row0=[%.4f %.4f %.4f]\n",
                        t.world_stored.rot[0][0], t.world_stored.rot[0][1],
                        t.world_stored.rot[0][2]);
            std::printf("      world row1=[%.4f %.4f %.4f]\n",
                        t.world_stored.rot[1][0], t.world_stored.rot[1][1],
                        t.world_stored.rot[1][2]);
            std::printf("      world row2=[%.4f %.4f %.4f]\n",
                        t.world_stored.rot[2][0], t.world_stored.rot[2][1],
                        t.world_stored.rot[2][2]);
        }
    }
    std::printf("\n%d mesh%s shown  (%d decoded, %d failed)\n", shown,
                shown == 1 ? "" : "es", ok, fail);
    return fail > 0 && shown == fail ? 1 : 0;
}

}  // anonymous namespace

int main(int argc, char** argv) {
    Args a = parse_args(argc, argv);

    try {
        std::fprintf(stderr, "[ghogx] loading ARK\n");
        std::fprintf(stderr, "          hdr = %s\n", a.hdr.c_str());
        std::fprintf(stderr, "          ark = %s\n", a.ark.c_str());
        auto ark = gh::ark::ArkV3Reader::load(a.hdr);
        std::fprintf(stderr, "[ghogx] %zu entries indexed (ARK v%u)\n",
                     ark.entries().size(), ark.version());

        if (a.sub == "tex-from-milo") return run_tex_from_milo(a, ark);
        if (a.sub == "dump") return run_dump(a, ark);
        if (a.sub == "dtb") return run_dtb(a, ark);
        if (a.sub == "objdir") return run_objdir(a);
        if (a.sub == "mesh") return run_mesh(a);

        if (a.sub == "list") {
            std::string f = a.filter;
            std::transform(f.begin(), f.end(), f.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            std::size_t n = 0;
            for (const auto& e : ark.entries()) {
                if (!f.empty()) {
                    std::string lp = e.full_path;
                    std::transform(lp.begin(), lp.end(), lp.begin(),
                                   [](unsigned char c) { return std::tolower(c); });
                    if (lp.find(f) == std::string::npos) continue;
                }
                std::printf("%9u  %s\n", e.size, e.full_path.c_str());
                ++n;
            }
            std::printf("\n%zu entr%s%s\n", n, n == 1 ? "y" : "ies",
                        a.filter.empty() ? "" : " (filtered)");
            return 0;
        }

        // Lazy-load whatever each sub needs.
        std::vector<ghogx::catalog::Song> songs;
        auto need_songs = [&]() {
            if (songs.empty()) songs = load_songs(ark, a.ark, a.catalog_path);
        };

        if (a.sub == "songs" || a.sub == "all") {
            need_songs();
            print_songs_table(songs);
        }
        if (a.sub == "venues" || a.sub == "all") {
            auto vs = ghogx::catalog::extract_venues(ark, a.ark);
            print_venues_table(vs);
        }
        if (a.sub == "chars" || a.sub == "all") {
            need_songs();
            auto agg = ghogx::catalog::aggregate_from_songs(songs);
            print_chars_table(agg);
        }
        if (a.sub != "songs" && a.sub != "venues" && a.sub != "chars" &&
            a.sub != "all") {
            std::fprintf(stderr, "unknown subcommand: %s\n", a.sub.c_str());
            usage();
        }
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[ghogx] FATAL: %s\n", e.what());
        return 1;
    }
}
