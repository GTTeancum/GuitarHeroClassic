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
