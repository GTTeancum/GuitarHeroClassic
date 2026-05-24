// ghogx - GuitarHeroOGX native engine entrypoint.
//
// Current scope: catalog enumeration MVP. Opens a PS2 Harmonix ARK (GH1 /
// GH2 / GH80s lineage), pulls config/gen/songs.dtb, and prints the song
// catalog parsed out of it. End-to-end validation that PS2 assets load
// natively without any 360-binary involvement.
//
// Future scope: gameplay, rendering, audio. This file is the seed.

#include "ark_v3.h"
#include "dtb.h"
#include "catalog.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

void usage() {
    std::fprintf(stderr,
        "ghogx - GuitarHeroOGX native engine (catalog MVP)\n"
        "\n"
        "Usage:\n"
        "  ghogx --ark-dir <dir>            Directory containing MAIN.HDR / main.hdr\n"
        "                                   and MAIN_0.ARK / main_0.ark\n"
        "  ghogx --hdr <path> --ark <path>  Explicit HDR + ARK paths\n"
        "\n"
        "Options:\n"
        "  --json                           Emit catalog as JSON instead of table\n"
        "  --catalog <dtb-path-in-ark>      Override the catalog DTB lookup path\n"
        "                                   (default: config/gen/songs.dtb)\n");
    std::exit(2);
}

struct Args {
    std::string hdr;
    std::string ark;
    std::string catalog_path = "config/gen/songs.dtb";
    bool json = false;
};

Args parse_args(int argc, char** argv) {
    Args a;
    std::string ark_dir;
    for (int i = 1; i < argc; ++i) {
        std::string_view k = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) { std::fprintf(stderr, "%s requires a value\n", name); std::exit(2); }
            return argv[++i];
        };
        if (k == "--ark-dir")      ark_dir = need("--ark-dir");
        else if (k == "--hdr")     a.hdr = need("--hdr");
        else if (k == "--ark")     a.ark = need("--ark");
        else if (k == "--catalog") a.catalog_path = need("--catalog");
        else if (k == "--json")    a.json = true;
        else if (k == "-h" || k == "--help") usage();
        else { std::fprintf(stderr, "unknown arg: %s\n", argv[i]); usage(); }
    }

    if (!ark_dir.empty()) {
        // Resolve case-insensitively-ish: try both upper- and lower-case
        // file names the way Harmonix ships them across PS2 disc layouts.
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

std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"':  out.append("\\\""); break;
            case '\\': out.append("\\\\"); break;
            case '\n': out.append("\\n");  break;
            case '\r': out.append("\\r");  break;
            case '\t': out.append("\\t");  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out.append(buf);
                } else {
                    out.push_back(c);
                }
        }
    }
    return out;
}

void print_table(const std::vector<ghogx::catalog::Song>& songs) {
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

void print_json(const std::vector<ghogx::catalog::Song>& songs) {
    std::printf("[\n");
    for (size_t i = 0; i < songs.size(); ++i) {
        const auto& s = songs[i];
        std::printf("  {\n");
        std::printf("    \"shortname\": \"%s\",\n", json_escape(s.shortname).c_str());
        std::printf("    \"name\": \"%s\",\n",      json_escape(s.display_name).c_str());
        std::printf("    \"artist\": \"%s\",\n",    json_escape(s.artist).c_str());
        std::printf("    \"midi_path\": \"%s\",\n", json_escape(s.midi_path).c_str());
        std::printf("    \"master_audio_path\": \"%s\"", json_escape(s.master_audio_path).c_str());
        if (s.preview_start_ms && s.preview_end_ms) {
            std::printf(",\n    \"preview_ms\": [%d, %d]",
                        *s.preview_start_ms, *s.preview_end_ms);
        }
        if (s.quickplay) {
            std::printf(",\n    \"quickplay\": {\"character_outfit\":\"%s\",\"guitar\":\"%s\",\"venue\":\"%s\"}",
                        json_escape(s.quickplay->character_outfit).c_str(),
                        json_escape(s.quickplay->guitar).c_str(),
                        json_escape(s.quickplay->venue).c_str());
        }
        std::printf("\n  }%s\n", (i + 1 < songs.size() ? "," : ""));
    }
    std::printf("]\n");
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

        // Catalog lookup. Try the supplied path first; if the ARK uses the
        // legacy "../../system/run/" prefix we'll fall back to that too.
        auto entry = ark.find(a.catalog_path);
        if (!entry) {
            std::string alt = "../../system/run/" + a.catalog_path;
            entry = ark.find(alt);
            if (entry) {
                std::fprintf(stderr, "[ghogx] catalog found at %s\n", alt.c_str());
            }
        } else {
            std::fprintf(stderr, "[ghogx] catalog found at %s\n", a.catalog_path.c_str());
        }
        if (!entry) {
            std::fprintf(stderr, "[ghogx] catalog DTB not found: %s\n", a.catalog_path.c_str());
            return 1;
        }

        std::fprintf(stderr, "[ghogx] reading catalog (%u bytes)\n", entry->size);
        // ark_part is zero-based into the supplied list; v3 single-part ARKs
        // always set ark_part=0 so we pass just the one .ark file.
        auto bytes = ark.read_entry(*entry, {a.ark});

        std::fprintf(stderr, "[ghogx] parsing DTB\n");
        auto tree = gh::dtb::parse(bytes);
        std::fprintf(stderr, "[ghogx] root_count=%zu, embedded=%s\n",
                     tree.root.size(), tree.embedded ? "yes" : "no");

        std::fprintf(stderr, "[ghogx] extracting song records\n");
        auto songs = ghogx::catalog::extract_songs(tree);
        std::fprintf(stderr, "[ghogx] extracted %zu songs\n\n", songs.size());

        if (a.json) print_json(songs); else print_table(songs);
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[ghogx] FATAL: %s\n", e.what());
        return 1;
    }
}
