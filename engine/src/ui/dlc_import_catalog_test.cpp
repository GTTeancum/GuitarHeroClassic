#include "ui/config_db.h"

#include "ark_v3.h"
#include "dtb.h"

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace ghogx;

namespace {

std::string first_existing(const fs::path& directory,
                           std::initializer_list<const char*> names) {
  for (const char* name : names) {
    const fs::path candidate = directory / name;
    if (fs::is_regular_file(candidate)) return candidate.string();
  }
  return {};
}

struct TempTree {
  explicit TempTree(fs::path value) : path(std::move(value)) {
    std::error_code error;
    fs::remove_all(path, error);
    fs::create_directories(path, error);
  }
  ~TempTree() {
    std::error_code error;
    fs::remove_all(path, error);
  }
  fs::path path;
};

void write_bytes(const fs::path& path, const std::vector<std::uint8_t>& bytes) {
  fs::create_directories(path.parent_path());
  std::ofstream stream(path, std::ios::binary);
  stream.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
}

}  // namespace

int main(int argc, char** argv) {
  const fs::path default_gen =
      "C:/Programming/GitHub/Guitar Hero II/gh2_ps2_hybrid_assets/gen";
  const std::string hdr = argc >= 3
                              ? argv[1]
                              : first_existing(default_gen,
                                               {"MAIN.HDR", "main.hdr"});
  const std::string ark0 = argc >= 3
                               ? argv[2]
                               : first_existing(default_gen,
                                                {"MAIN_0.ARK", "main_0.ark"});
  if (hdr.empty() || ark0.empty()) {
    std::printf("ghogx_dlc_import_catalog_test: SKIP (no GH2 ARK)\n");
    return 0;
  }
  TempTree root(fs::temp_directory_path() / "ghogx-dlc-import-catalog-test");
  const fs::path empty_addons = root.path / "empty";
  fs::create_directories(empty_addons);
#ifdef _WIN32
  _putenv_s("GHOGX_ADDONS_DIR", empty_addons.string().c_str());
  _putenv_s("GHOGX_DISABLE_PROFILE_PERSISTENCE", "1");
#else
  setenv("GHOGX_ADDONS_DIR", empty_addons.string().c_str(), 1);
  setenv("GHOGX_DISABLE_PROFILE_PERSISTENCE", "1", 1);
#endif
  auto ark = gh::ark::ArkV3Reader::load(hdr);
  ui::ConfigDb db;
  db.load(ark, {ark0});
  const std::size_t base_song_count = db.song_count();

  const fs::path package = root.path / "disc.test.songs";
  const fs::path content = package / "content";
  const std::string catalog_path = "config/dlc/test/songs.dtb";
  const auto catalog = gh::dtb::parse_dta(
      "(dlc_import_test\n"
      "  (name \"DLC Import Test\")\n"
      "  (artist \"Source Artist\")\n"
      "  (song\n"
      "    (name songs/dlc_import_test/dlc_import_test)\n"
      "    (tracks ((guitar 0 1) (rhythm 2))))\n"
      "  (midi_file songs/dlc_import_test/dlc_import_test.mid)\n"
      "  (preview 1234 5678)\n"
      "  (anim_tempo kTempoFast)\n"
      "  (quickplay (character metal) (guitar gibson_sg) "
      "(venue arena)))\n");
  write_bytes(content / catalog_path, gh::dtb::serialize(catalog));
  write_bytes(content / "songs/dlc_import_test/dlc_import_test.mid",
              {'M', 'T', 'h', 'd'});
  write_bytes(content / "songs/dlc_import_test/dlc_import_test.vgs",
              {'V', 'g', 'S', '!'});
  write_bytes(content / "songs/dlc_import_test/not-indexed.bin", {'n', 'o'});
  fs::create_directories(package);
  {
    std::ofstream manifest(package / "manifest.json");
    manifest
        << "{\n"
        << "  \"schema_version\": 1,\n"
        << "  \"id\": \"disc.test.songs\",\n"
        << "  \"source_game\": \"gh1\",\n"
        << "  \"content_root\": \"content\",\n"
        << "  \"files\": [\n"
        << "    \"config/dlc/test/songs.dtb\",\n"
        << "    \"songs/dlc_import_test/dlc_import_test.mid\",\n"
        << "    \"songs/dlc_import_test/dlc_import_test.vgs\"\n"
        << "  ],\n"
        << "  \"song_catalogs\": [\"config/dlc/test/songs.dtb\"],\n"
        << "  \"source_routes\": [\n"
        << "    {\"source\":\"gh1\",\"kind\":\"character\",\"from\":\"metal\",\"to\":\"gh1_metal\"},\n"
        << "    {\"source\":\"gh1\",\"kind\":\"guitar\",\"from\":\"gibson_sg\",\"to\":\"sg\"},\n"
        << "    {\"source\":\"gh1\",\"kind\":\"venue\",\"from\":\"arena\",\"to\":\"gh1_arena\"},\n"
        << "    {\"source\":\"gh1\",\"kind\":\"band_member\",\"from\":\"SINGER_MALE_METAL\",\"to\":\"gh1_metal_singer\"}\n"
        << "  ],\n"
        << "  \"source_default_bands\": [{\"source\":\"gh1\",\"members\":[\"SINGER_MALE_METAL\"]}]\n"
        << "}\n";
  }
  db.load_addon_manifests(package, &ark);
  const int song_index = db.song_index(Symbol("dlc_import_test"));
  const auto loose_midi =
      ark.find("songs/dlc_import_test/dlc_import_test.mid");
  const auto unindexed =
      ark.find("songs/dlc_import_test/not-indexed.bin");
  const auto quickplay = db.quickplay_songs();
  const auto runtime = db.song_runtime_config(Symbol("dlc_import_test"));
  if (db.song_count() != base_song_count + 1 || song_index < 0 ||
      db.song_audio_path(Symbol("dlc_import_test")) !=
          "songs/dlc_import_test/dlc_import_test" ||
      db.song_midi_path(Symbol("dlc_import_test")) !=
          "songs/dlc_import_test/dlc_import_test.mid" ||
      runtime.source_game != Symbol("gh1") ||
      runtime.character_outfit != "gh1_metal" || runtime.guitar != "sg" ||
      runtime.venue != "gh1_arena" || runtime.band.size() != 1 ||
      runtime.band.front() != "gh1_metal_singer" ||
      std::find(quickplay.begin(), quickplay.end(),
                Symbol("dlc_import_test")) == quickplay.end() ||
      !loose_midi || loose_midi->loose_path.empty() || unindexed) {
    std::fprintf(stderr, "FAIL additive indexed song-catalog package\n");
    return 1;
  }
  std::printf(
      "ghogx_dlc_import_catalog_test: PASS base=%zu imported=1 indexed=3\n",
      base_song_count);
  if (argc >= 4) {
    const fs::path release_package = argv[3];
    ui::ConfigDb release_db;
    release_db.load(ark, {ark0});
    release_db.load_addon_manifests(release_package, &ark);
    const auto* punk =
        release_db.character_variant(Symbol("gh1_punk"));
    const auto runtime_venue = release_db.is_venue(Symbol("gh1_arena"));
    if (!punk ||
        punk->model_path !=
            "char/gh1_punk/og/gen/gh1_punk.milo_ps2" ||
        !runtime_venue) {
      std::fprintf(stderr, "FAIL preconverted GH1 release package\n");
      return 1;
    }
    std::printf(
        "ghogx_dlc_import_catalog_test: PASS release-package=%s "
        "files=%zu\n",
        release_package.string().c_str(),
        release_db.dlc_packages().back().mounted_files);
  }
  if (argc >= 5) {
    const fs::path installed_dlc_root = argv[4];
    ui::ConfigDb installed_db;
    installed_db.load(ark, {ark0});
    installed_db.load_addon_manifests(installed_dlc_root, &ark);
    const auto gh80_runtime =
        installed_db.song_runtime_config(Symbol("18andlife"));
    if (gh80_runtime.source_game != Symbol("gh80s") ||
        gh80_runtime.character_outfit != "gh80_glam1" ||
        gh80_runtime.guitar != "flying_v" ||
        gh80_runtime.venue != "arena") {
      std::fprintf(stderr,
                   "FAIL installed GH80s source routing source=%s character=%s "
                   "guitar=%s venue=%s\n",
                   gh80_runtime.source_game.c_str(),
                   gh80_runtime.character_outfit.c_str(),
                   gh80_runtime.guitar.c_str(), gh80_runtime.venue.c_str());
      return 1;
    }
    std::printf(
        "ghogx_dlc_import_catalog_test: PASS installed-dlc=%s "
        "gh80-route=gh80_glam1\n",
        installed_dlc_root.string().c_str());
  }
  return 0;
}
