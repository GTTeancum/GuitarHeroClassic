// dtb_tool - CLI for Harmonix DTB inspection.
//
// Usage:
//   dtb_tool dump    <file.dtb> [--lines]   Print as DTA-style text.
//   dtb_tool compile <file.dta> <file.dtb>   Compile text to a plain DTB.
//   dtb_tool info    <file.dtb>             Print metadata.
//   dtb_tool surface <file.dtb>             Print the (target :: message) call
//                                           surface (for 1:1 class-behavior RE).
//   dtb_tool verify  <file.dtb>             Byte-exact parse/serialize check.
//   dtb_tool song-assets <songs.dtb>         Print ID/MIDI/audio TSV.
//   dtb_tool filter-song-catalog <source.dtb> <base.dtb> <output.dtb>

#include "dtb.h"

#include <cstdio>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iterator>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {

std::string sym_of(const gh::dtb::Node& n) {
  auto s = gh::dtb::as_string(n);
  return s ? *s : std::string();
}

// Command heads that are interpreter builtins, not object-message sends.
bool is_builtin_head(const std::string& s) {
  static const std::set<std::string> b = {
      "if", "if_else", "do", "switch", "foreach", "set", "func", "eval",
      "sprintf", "localize", "print", "new", "==", "!=", "<", ">", "<=", ">=",
      "!", "&&", "||", "+", "-", "*", "/", "'||'", "'&&'", "'+'", "'-'", "'*'", "'/'"};
  return b.count(s) > 0;
}

std::string keyed_atom(const gh::dtb::Node& parent,
                       std::string_view key) {
  const auto row = gh::dtb::find_keyed(parent, key);
  if (!row) return {};
  const auto& values = gh::dtb::children(*row);
  if (values.size() < 2) return {};
  return gh::dtb::as_string(*values[1]).value_or(std::string());
}

bool ignored_song_record(const gh::dtb::Node& record) {
  return keyed_atom(record, "validate_ignore") == "TRUE";
}

std::set<std::string> importable_song_ids(const gh::dtb::Tree& tree) {
  std::set<std::string> ids;
  for (const auto& root : tree.root) {
    if (!root || !gh::dtb::is_array(*root) || ignored_song_record(*root))
      continue;
    const auto& values = gh::dtb::children(*root);
    if (values.empty() || !gh::dtb::find_keyed(*root, "song")) continue;
    const std::string id = gh::dtb::as_string(*values[0]).value_or("");
    if (!id.empty()) ids.insert(id);
  }
  return ids;
}

std::string lowercase_path(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::pair<std::string, std::string> song_asset_paths(
    const gh::dtb::Node& record) {
  const auto song = gh::dtb::find_keyed(record, "song");
  if (!song) return {};
  std::string midi = keyed_atom(*song, "midi_file");
  if (midi.empty()) midi = keyed_atom(record, "midi_file");
  std::string audio = keyed_atom(*song, "name");
  if (!audio.empty() &&
      (audio.size() < 4 || audio.substr(audio.size() - 4) != ".vgs"))
    audio += ".vgs";
  return {lowercase_path(midi), lowercase_path(audio)};
}

// Walk the tree collecting "target :: message" for every {target msg ...} command.
// `cur_class` is the class of the nearest enclosing {new Class Name ...}, so a
// {$this msg} resolves to "Class :: msg" (the surface that class must implement).
void collect(const gh::dtb::Node& n, const std::string& cur_class,
             std::set<std::string>& out) {
  using namespace gh::dtb;
  if (n.tag == 0x11) {  // {command}
    const NodeList& k = children(n);
    if (!k.empty()) {
      const Node& h = *k[0];
      if (h.tag == 0x05 && sym_of(h) == "new" && k.size() >= 3) {
        std::string cls = sym_of(*k[1]);
        for (std::size_t i = 3; i < k.size(); ++i) collect(*k[i], cls, out);
        return;
      }
      std::string tgt;
      bool is_msg = false;
      if (h.tag == 0x02) {  // $variable target
        std::string v = sym_of(h);
        tgt = (v == "this") ? (cur_class.empty() ? "$this" : cur_class) : ("$" + v);
        is_msg = true;
      } else if (h.tag == 0x05) {  // bareword: builtin or object name
        std::string s = sym_of(h);
        if (!is_builtin_head(s)) { tgt = s; is_msg = true; }
      } else if (h.tag == 0x13) {
        tgt = "[prop]"; is_msg = true;
      } else if (h.tag == 0x11) {
        tgt = "<expr>"; is_msg = true;
      }
      if (is_msg && k.size() >= 2 && is_string_like(*k[1]))
        out.insert(tgt + " :: " + sym_of(*k[1]));
      for (const auto& kid : k) collect(*kid, cur_class, out);
      return;
    }
  }
  if (is_array(n))
    for (const auto& kid : children(n)) collect(*kid, cur_class, out);
}

void usage() {
  std::fprintf(stderr,
               "Usage:\n"
               "  dtb_tool dump    <file.dtb> [--lines]\n"
               "  dtb_tool compile <file.dta> <file.dtb>\n"
               "  dtb_tool info    <file.dtb>\n"
               "  dtb_tool surface <file.dtb>\n"
               "  dtb_tool verify  <file.dtb>\n"
               "  dtb_tool song-assets <songs.dtb>\n"
               "  dtb_tool filter-song-catalog <source.dtb> <base.dtb> "
               "<output.dtb>\n");
  std::exit(2);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) usage();
  std::string sub = argv[1];
  std::string path = argv[2];

  try {
    if (sub == "compile") {
      if (argc != 4) usage();
      std::ifstream input(path, std::ios::binary);
      if (!input) throw std::runtime_error("cannot open input");
      const std::string text((std::istreambuf_iterator<char>(input)),
                             std::istreambuf_iterator<char>());
      gh::dtb::Tree tree = gh::dtb::parse_dta(text);
      tree.storage = gh::dtb::Storage::Plain;
      const std::vector<uint8_t> bytes = gh::dtb::serialize(tree);
      std::ofstream output(argv[3], std::ios::binary);
      if (!output) throw std::runtime_error("cannot open output");
      output.write(reinterpret_cast<const char*>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()));
      if (!output) throw std::runtime_error("cannot write output");
      std::printf("compiled %s -> %s (%zu bytes)\n", path.c_str(), argv[3],
                  bytes.size());
      return 0;
    }
    if (sub == "filter-song-catalog") {
      if (argc != 5 && argc != 7) usage();
      if (argc == 7 && std::strcmp(argv[5], "--base-paths") != 0) usage();
      gh::dtb::Tree source = gh::dtb::parse_file(path);
      const gh::dtb::Tree base = gh::dtb::parse_file(argv[3]);
      const std::set<std::string> base_ids = importable_song_ids(base);
      std::set<std::string> base_paths;
      if (argc == 7) {
        std::ifstream path_stream(argv[6]);
        if (!path_stream)
          throw std::runtime_error("cannot open base path inventory");
        std::string row;
        while (std::getline(path_stream, row)) {
          row = lowercase_path(row);
          if (!row.empty()) base_paths.insert(row);
        }
      }
      gh::dtb::Tree output = source;
      output.root.clear();
      std::size_t ignored = 0;
      std::size_t duplicates = 0;
      std::size_t base_asset_duplicates = 0;
      for (const auto& root : source.root) {
        if (!root || !gh::dtb::is_array(*root)) continue;
        const auto& values = gh::dtb::children(*root);
        if (values.empty() || !gh::dtb::find_keyed(*root, "song")) continue;
        if (ignored_song_record(*root)) {
          ++ignored;
          continue;
        }
        const std::string id =
            gh::dtb::as_string(*values[0]).value_or("");
        if (id.empty()) continue;
        if (base_ids.count(id)) {
          ++duplicates;
          continue;
        }
        const auto [midi, audio] = song_asset_paths(*root);
        if (!base_paths.empty() && !midi.empty() && !audio.empty() &&
            base_paths.count(midi) && base_paths.count(audio)) {
          ++base_asset_duplicates;
          continue;
        }
        output.root.push_back(root);
      }
      const std::vector<uint8_t> serialized = gh::dtb::serialize(output);
      std::ofstream stream(argv[4], std::ios::binary);
      if (!stream) throw std::runtime_error("cannot open filtered catalog output");
      stream.write(reinterpret_cast<const char*>(serialized.data()),
                   static_cast<std::streamsize>(serialized.size()));
      if (!stream) throw std::runtime_error("cannot write filtered catalog output");
      std::printf("filtered song catalog: imported=%zu duplicates=%zu "
                  "base_asset_duplicates=%zu validate_ignored=%zu\n",
                  output.root.size(), duplicates, base_asset_duplicates,
                  ignored);
      return 0;
    }
    auto bytes = gh::dtb::read_file(path);
    auto tree = gh::dtb::parse(bytes);

    if (sub == "info") {
      const char* storage =
          tree.storage == gh::dtb::Storage::Plain
              ? "plain"
              : (tree.storage == gh::dtb::Storage::ZeroPrefixedPlain
                     ? "zero-prefixed plain"
                     : "encrypted (PS2 cipher)");
      std::printf("file        : %s\n", path.c_str());
      std::printf("size        : %zu bytes\n", bytes.size());
      std::printf("storage     : %s\n", storage);
      std::printf("version     : %u\n", tree.version);
      std::printf("embedded    : %s\n", tree.embedded ? "yes" : "no");
      std::printf("root_count  : %zu\n", tree.root.size());
      std::printf("trailing    : %zu bytes\n", tree.trailing_bytes.size());
      return 0;
    }
    if (sub == "verify") {
      const auto serialized = gh::dtb::serialize(tree);
      if (serialized != bytes) {
        std::fprintf(stderr,
                     "DTB round trip differs: source=%zu output=%zu\n",
                     bytes.size(), serialized.size());
        return 1;
      }
      std::printf("byte-exact DTB round trip: %zu bytes, %zu roots\n",
                  bytes.size(), tree.root.size());
      return 0;
    }
    if (sub == "dump") {
      bool show_lines = false;
      for (int i = 3; i < argc; ++i) {
        if (std::strcmp(argv[i], "--lines") == 0) show_lines = true;
        else { std::fprintf(stderr, "unknown arg: %s\n", argv[i]); return 2; }
      }
      std::cout << gh::dtb::to_dta(tree, show_lines);
      return 0;
    }
    if (sub == "surface") {
      std::set<std::string> out;
      for (const auto& r : tree.root) collect(*r, "", out);
      for (const auto& s : out) std::printf("%s\n", s.c_str());
      return 0;
    }
    if (sub == "song-assets") {
      std::set<std::string> ids;
      std::size_t count = 0;
      std::printf("song_id\tmidi_file\taudio_file\n");
      for (const auto& root : tree.root) {
        if (!root || !gh::dtb::is_array(*root)) continue;
        const auto& record = gh::dtb::children(*root);
        if (record.empty()) continue;
        const std::string song_id =
            gh::dtb::as_string(*record[0]).value_or("");
        const auto song = gh::dtb::find_keyed(*root, "song");
        // Retail GH80s keeps development budget records below _SHIP guards
        // in the compiled tree and marks each one validate_ignore TRUE.  They
        // intentionally reference assets absent from the shipping archive and
        // are not importable songs.
        if (ignored_song_record(*root)) continue;
        if (song_id.empty() || !song)
          throw std::runtime_error(
              "songs catalog contains a record without song identity/data");
        // GH1 authors midi_file beside the song block; GH2 authors it inside
        // that block.  Accept both documented retail layouts while keeping
        // the asset path itself mandatory.
        std::string midi = keyed_atom(*song, "midi_file");
        if (midi.empty()) midi = keyed_atom(*root, "midi_file");
        std::string audio = keyed_atom(*song, "name");
        if (midi.empty() || audio.empty())
          throw std::runtime_error(
              "song " + song_id + " has no MIDI or primary audio path");
        if (!ids.insert(song_id).second)
          throw std::runtime_error("duplicate song ID " + song_id);
        if (audio.size() < 4 ||
            audio.substr(audio.size() - 4) != ".vgs")
          audio += ".vgs";
        std::printf("%s\t%s\t%s\n", song_id.c_str(), midi.c_str(),
                    audio.c_str());
        ++count;
      }
      if (count == 0)
        throw std::runtime_error("songs catalog contains no records");
      return 0;
    }
    usage();
  } catch (const std::exception& e) {
    std::fprintf(stderr, "dtb_tool: %s\n", e.what());
    return 1;
  }
  return 0;
}
