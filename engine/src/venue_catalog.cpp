// venue_catalog.cpp - see venue_catalog.h.

#include "venue_catalog.h"

#include "ark_v3.h"
#include "dtb.h"

#include <regex>
#include <unordered_set>

namespace ghogx::catalog {

namespace {

// Match world/<x>/gen/<x>.dtb (with optional ../../system/run/ prefix that
// some ARKs use for tooling-relative paths).
const std::regex kVenuePathRegex(
    R"((?:\.\./\.\./system/run/)?world/([^/]+)/gen/\1\.dtb)");

std::optional<std::string> read_sound_bank(const gh::dtb::Node& types_arr,
                                           const std::string& shortname) {
    // types_arr is (types (<shortname> WORLD_OBJECT_BASE (sound (bank <name>)) ...))
    // Find the venue entry inside it.
    for (const auto& venue_node : gh::dtb::children(types_arr)) {
        if (!venue_node || !gh::dtb::is_array(*venue_node)) continue;
        const auto& vkids = gh::dtb::children(*venue_node);
        if (vkids.empty()) continue;
        auto first = gh::dtb::as_string(*vkids[0]);
        if (!first || *first != shortname) continue;

        if (auto sound = gh::dtb::find_keyed(*venue_node, "sound")) {
            if (auto bank = gh::dtb::find_keyed(*sound, "bank")) {
                const auto& bkids = gh::dtb::children(*bank);
                if (bkids.size() >= 2) {
                    return gh::dtb::as_string(*bkids[1]);
                }
            }
        }
        break;
    }
    return std::nullopt;
}

int count_crowd_levels(const gh::dtb::Node& types_arr,
                       const std::string& shortname) {
    int n = 0;
    for (const auto& venue_node : gh::dtb::children(types_arr)) {
        if (!venue_node || !gh::dtb::is_array(*venue_node)) continue;
        const auto& vkids = gh::dtb::children(*venue_node);
        if (vkids.empty()) continue;
        auto first = gh::dtb::as_string(*vkids[0]);
        if (!first || *first != shortname) continue;

        if (auto sound = gh::dtb::find_keyed(*venue_node, "sound")) {
            if (auto crowd = gh::dtb::find_keyed(*sound, "crowd")) {
                if (auto levels = gh::dtb::find_keyed(*crowd, "levels")) {
                    // (levels (kExcitementX -N <cue> ...) (kExcitementY ...) ...)
                    for (const auto& lvl : gh::dtb::children(*levels)) {
                        if (lvl && gh::dtb::is_array(*lvl)) ++n;
                    }
                }
            }
        }
        break;
    }
    return n;
}

}  // anonymous namespace

std::vector<Venue> extract_venues(const gh::ark::ArkV3Reader& ark,
                                  const std::string& ark_file_path) {
    std::vector<Venue> out;
    std::unordered_set<std::string> seen;  // dedupe across path-prefix variants

    for (const auto& entry : ark.entries()) {
        std::smatch m;
        if (!std::regex_match(entry.full_path, m, kVenuePathRegex)) continue;
        std::string shortname = m[1].str();
        if (!seen.insert(shortname).second) continue;

        Venue v{};
        v.shortname = shortname;
        v.dtb_path  = entry.full_path;

        try {
            auto bytes = ark.read_entry(entry, {ark_file_path});
            auto tree = gh::dtb::parse(bytes);
            // Expected: (WorldDir (types (<shortname> ...)))
            if (auto world_dir = gh::dtb::find_keyed(tree, "WorldDir")) {
                if (auto types = gh::dtb::find_keyed(*world_dir, "types")) {
                    v.sound_bank   = read_sound_bank(*types, shortname);
                    v.crowd_levels = count_crowd_levels(*types, shortname);
                }
            }
        } catch (const std::exception&) {
            // Leave summary fields empty; caller still gets the shortname.
        }

        out.push_back(std::move(v));
    }
    return out;
}

}  // namespace ghogx::catalog
