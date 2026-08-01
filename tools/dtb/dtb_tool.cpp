// dtb_tool - CLI for Harmonix DTB inspection.
//
// Usage:
//   dtb_tool dump    <file.dtb> [--lines]   Print as DTA-style text.
//   dtb_tool compile <file.dta> <file.dtb>   Compile text to a plain DTB.
//   dtb_tool info    <file.dtb>             Print metadata.
//   dtb_tool surface <file.dtb>             Print the (target :: message) call
//                                           surface (for 1:1 class-behavior RE).
//   dtb_tool verify  <file.dtb>             Byte-exact parse/serialize check.

#include "dtb.h"

#include <cstdio>
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
               "  dtb_tool verify  <file.dtb>\n");
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
    usage();
  } catch (const std::exception& e) {
    std::fprintf(stderr, "dtb_tool: %s\n", e.what());
    return 1;
  }
  return 0;
}
