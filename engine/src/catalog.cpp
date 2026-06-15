// catalog.cpp - see catalog.h.

#include "catalog.h"

#include "dtb.h"

namespace ghogx::catalog {

namespace {

// Helper: from an array like `(name "18 And Life")`, return the SECOND
// child as a string. Returns empty optional if the array shape isn't what
// we expected (e.g. fewer than 2 children or not a string second element).
std::optional<std::string> keyed_string(const gh::dtb::Node& parent,
                                        std::string_view key) {
    auto kn = gh::dtb::find_keyed(parent, key);
    if (!kn) return std::nullopt;
    const auto& kids = gh::dtb::children(*kn);
    if (kids.size() < 2) return std::nullopt;
    return gh::dtb::as_string(*kids[1]);
}

std::optional<int32_t> keyed_int(const gh::dtb::Node& parent,
                                 std::string_view key, size_t index = 1) {
    auto kn = gh::dtb::find_keyed(parent, key);
    if (!kn) return std::nullopt;
    const auto& kids = gh::dtb::children(*kn);
    if (kids.size() <= index) return std::nullopt;
    return gh::dtb::as_int(*kids[index]);
}

}  // anonymous namespace

std::vector<Song> extract_songs(const gh::dtb::Tree& tree) {
    std::vector<Song> out;
    out.reserve(tree.root.size());

    for (const auto& root_node : tree.root) {
        if (!root_node || !gh::dtb::is_array(*root_node)) continue;
        const auto& kids = gh::dtb::children(*root_node);
        if (kids.empty()) continue;

        // First child is the shortname symbol/keyword.
        auto shortname = gh::dtb::as_string(*kids[0]);
        if (!shortname) continue;

        Song s{};
        s.shortname    = *shortname;
        s.display_name = keyed_string(*root_node, "name").value_or("");
        s.artist       = keyed_string(*root_node, "artist").value_or("");

        // (song ...) sub-array holds the master mix paths.
        if (auto song_arr = gh::dtb::find_keyed(*root_node, "song")) {
            s.master_audio_path = keyed_string(*song_arr, "name").value_or("");
            s.midi_path         = keyed_string(*song_arr, "midi_file").value_or("");
        }

        // (preview start_ms end_ms)
        if (auto prev = gh::dtb::find_keyed(*root_node, "preview")) {
            const auto& pkids = gh::dtb::children(*prev);
            if (pkids.size() >= 3) {
                s.preview_start_ms = gh::dtb::as_int(*pkids[1]);
                s.preview_end_ms   = gh::dtb::as_int(*pkids[2]);
            }
        }

        // (quickplay (character_outfit X) (guitar Y) (venue Z))
        if (auto qp = gh::dtb::find_keyed(*root_node, "quickplay")) {
            Quickplay q{};
            q.character_outfit = keyed_string(*qp, "character_outfit").value_or("");
            q.guitar           = keyed_string(*qp, "guitar").value_or("");
            q.venue            = keyed_string(*qp, "venue").value_or("");
            if (!q.character_outfit.empty() || !q.guitar.empty() || !q.venue.empty()) {
                s.quickplay = std::move(q);
            }
        }

        // Some song records can override the backing band as `(band singer bass
        // drummer)`. `shoutatthedevil` does not, so gameplay falls back to
        // config/gen/gh2.dtb's `(default_band ...)`.
        if (auto band = gh::dtb::find_keyed(*root_node, "band")) {
            const auto& bkids = gh::dtb::children(*band);
            for (size_t i = 1; i < bkids.size(); ++i) {
                if (auto name = gh::dtb::as_string(*bkids[i]))
                    s.band.push_back(*name);
            }
        }

        out.push_back(std::move(s));
    }
    return out;
}

}  // namespace ghogx::catalog
