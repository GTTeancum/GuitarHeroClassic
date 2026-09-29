#include "ui/config_db.h"
#include "ui/menu_labels.h"
#include "ui/meta_objects.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>

using namespace ghogx;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
#ifdef _WIN32
  _putenv_s("GHOGX_DISABLE_PROFILE_PERSISTENCE", "1");
#else
  setenv("GHOGX_DISABLE_PROFILE_PERSISTENCE", "1", 1);
#endif
  const fs::path assets = argc > 1 ? argv[1] :
      "C:/Programming/GitHub/Guitar Hero II/gh2_ps2_hybrid_assets";
  const auto hdr = (assets / "GEN/main.hdr").string();
  const auto ark_path = (assets / "GEN/main_0.ark").string();
  const auto scratch = fs::temp_directory_path() /
      ("ghogx-setlist-test-" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(scratch);
  int checks = 0;
  auto check = [&](bool ok, const char* reason) {
    ++checks;
    if (!ok) throw std::runtime_error(reason);
  };
  try {
    const auto ark = gh::ark::ArkV3Reader::load(hdr);
    ui::ConfigDb db;
    db.load(ark, {ark_path});
    const auto catalog = db.quickplay_songs();
    check(catalog.size() >= 3, "test needs stock songs");
    const auto a = catalog[0], b = catalog[1], c = catalog[2];
    const auto venue = db.default_venue();
    auto file = std::ofstream(scratch / "01-test.json");
    file << "{\"schema_version\":1,\"setlists\":["
         << "{\"id\":\"test_a\",\"label\":\"First\",\"sections\":["
         << "{\"label\":\"Section One\",\"default_venue\":\"" << venue.c_str()
         << "\",\"songs\":[\"" << b.c_str() << "\",\"" << a.c_str() << "\"]},"
         << "{\"label\":\"Section Two\",\"default_venue\":\"" << venue.c_str()
         << "\",\"songs\":[\"" << c.c_str() << "\"]}]},"
         << "{\"id\":\"test_b\",\"label\":\"Second\",\"sections\":["
         << "{\"label\":\"Shared Song\",\"default_venue\":\"" << venue.c_str()
         << "\",\"songs\":[\"" << a.c_str() << "\"]}]}]}";
    file.close();
    db.load_quickplay_setlists(scratch);
    auto buckets = db.quickplay_buckets();
    check(buckets.size() >= 3, "stock plus two JSON buckets");
    check(db.active_quickplay_bucket().id == Symbol("gh2"), "stock initially selected");
    check(db.next_quickplay_bucket(), "Yellow switches bucket");
    check(db.active_quickplay_bucket().id == Symbol("test_a"), "first JSON bucket");
    check(db.active_quickplay_songs() == std::vector<Symbol>({b, a, c}), "authored section/song order");
    check(db.active_quickplay_bucket().sections.size() == 2, "section headers preserved");
    check(db.quickplay_default_venue(c) == venue, "tier venue resolved by song");
    ui::register_ui_classes();
    ui::ScreenManager mgr;
    ui::install_default_singletons(mgr);
    ui::install_meta_singletons(mgr, db);
    auto* game = mgr.resolve_object(Symbol("game"));
    game->handle_property(Symbol("set_quickplay"), DataArray());
    DataArray index; index.push(DataNode::Int(1));
    game->handle_property(Symbol("set_song_index"), index);
    check(game->get_property(Symbol("song")).as_symbol().value_or(Symbol()) == a,
          "game/provider resolve active bucket index");
    check(db.next_quickplay_bucket(), "second switch");
    check(db.active_quickplay_songs() == std::vector<Symbol>({a}), "shared song allowed across buckets");
    for (std::size_t i = 0; i < buckets.size() - 2; ++i) db.next_quickplay_bucket();
    check(db.active_quickplay_bucket().id == Symbol("gh2"), "Yellow wraps to first");
    check(db.select_quickplay_bucket_for_song(c), "direct song launch finds bucket");
    check(db.active_quickplay_bucket().id == Symbol("gh2"), "shared song keeps current bucket");
    std::ofstream(scratch / "02-invalid.json") <<
      "{\"schema_version\":1,\"setlists\":[{\"id\":\"partial\",\"label\":\"Partial\",\"sections\":[]},{\"id\":\"test_a\",\"label\":\"Duplicate\",\"sections\":[]}]}";
    db.load_quickplay_setlists(scratch);
    check(db.quickplay_buckets().size() == buckets.size(), "invalid file rolled back atomically");
    std::ofstream bad_venue(scratch / "03-venue.json");
    bad_venue << "{\"schema_version\":1,\"setlists\":[{\"id\":\"bad_venue\",\"label\":\"Bad\",\"sections\":[{\"label\":\"Bad\",\"default_venue\":\"nonexistent_venue\",\"songs\":[\""
              << a.c_str() << "\"]}]}]}";
    bad_venue.close();
    std::ofstream(scratch / "04-optional.json") <<
      "{\"schema_version\":1,\"setlists\":[{\"id\":\"optional_disc\",\"label\":\"Optional\",\"sections\":[{\"label\":\"Optional\",\"default_venue\":\"optional_venue\",\"songs\":[\"uninstalled_song\"]}]}]}";
    db.load_quickplay_setlists(scratch);
    check(db.quickplay_buckets().size() == buckets.size(), "invalid venue rejected; uninstalled bucket hidden");
    check(db.active_quickplay_bucket().id == Symbol("gh2"), "reload resets bucket selection");
    db.load_quickplay_setlists(assets / "Setlists");
    for (const auto& bucket : db.quickplay_buckets()) {
      std::printf("bucket %s: %zu songs, %zu sections\n", bucket.id.c_str(), bucket.songs.size(), bucket.sections.size());
      for (const auto& section : bucket.sections)
        std::printf("  %s -> %s (%zu)\n", section.label.c_str(), section.default_venue.c_str(), section.songs.size());
    }
    for (const auto& label : ui::extract_menu_labels(hdr, ark_path, "ui/gen/sel_song_quickplay.milo_ps2"))
      std::printf("label %s: %s\n", label.name.c_str(), label.text.c_str());
    std::printf("PASS %d setlist checks\n", checks);
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "FAIL: %s\n", ex.what());
    fs::remove_all(scratch);
    return 1;
  }
  fs::remove_all(scratch);
  return 0;
}
