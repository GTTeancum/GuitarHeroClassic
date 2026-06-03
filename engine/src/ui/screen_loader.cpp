// engine/src/ui/screen_loader.cpp -- see screen_loader.h.

#include "ui/screen_loader.h"

#include "core/class_reg.h"
#include "dtb_bridge/dtb_bridge.h"
#include "script/preprocess.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace ghogx::ui {

namespace {
using Node = gh::dtb::Node;
using NodeList = gh::dtb::NodeList;

std::string nstr(const Node& n) { return std::string(gh::dtb::as_string(n).value_or("")); }

// A keyed body entry is a HANDLER if any of its elements (after the key, past
// an optional (params) list) is a {} command; otherwise it is a config property.
bool entry_is_handler(const NodeList& ek) {
  for (std::size_t t = 1; t < ek.size(); ++t)
    if (ek[t]->tag == 0x11) return true;  // 0x11 == {command}
  return false;
}
}  // namespace

void load_ui_objects(const NodeList& roots, ScreenManager& mgr) {
  for (const auto& rp : roots) {
    const Node& r = *rp;
    if (r.tag != 0x11) continue;  // only {new ...} commands at the top level
    const NodeList& k = gh::dtb::children(r);
    if (k.size() < 3 || k[0]->tag != 0x05 || nstr(*k[0]) != "new") continue;

    Symbol cls(nstr(*k[1]));
    Symbol name(nstr(*k[2]));
    std::unique_ptr<Object> obj = ClassReg::instance().create(cls);
    if (!obj) { mgr.on_unhandled(std::string("new?:") + cls.c_str()); continue; }
    obj->set_name(name);
    if (auto* ui = dynamic_cast<UiObject*>(obj.get())) ui->set_manager(&mgr);

    for (std::size_t i = 3; i < k.size(); ++i) {
      if (k[i]->tag != 0x10) continue;  // keyed entries are (key ...) data arrays
      const NodeList& ek = gh::dtb::children(*k[i]);
      if (ek.empty() || !gh::dtb::is_string_like(*ek[0])) continue;
      Symbol key(nstr(*ek[0]));

      if (entry_is_handler(ek)) {
        if (auto* ui = dynamic_cast<UiObject*>(obj.get())) ui->add_handler(key, k[i]);
      } else if (ek.size() == 2) {
        obj->set_property(key, dtb_bridge::from_node(*ek[1]));
      } else if (ek.size() > 2) {
        NodeList vals(ek.begin() + 1, ek.end());
        obj->set_property(key, DataNode::Array(dtb_bridge::from_node_list(vals)));
      }
    }
    mgr.add_object(std::move(obj));
  }
}

bool load_ui_dtb_from_ark(const gh::ark::ArkV3Reader& ark,
                          const std::vector<std::string>& ark_paths,
                          const std::string& dtb_path, ScreenManager& mgr) {
  auto entry = ark.find(dtb_path);
  if (!entry) return false;
  std::vector<uint8_t> bytes = ark.read_entry(*entry, ark_paths);
  gh::dtb::Tree tree = gh::dtb::parse(bytes);

  // Stock PS2 per-screen DTBs are largely directive-free; resolve any residual
  // #ifdef/#define and (defensively) #include against the PS2 define set.
  script::PreprocessOptions opts;
  opts.defines = {"HX_EE", "PS2"};
  NodeList resolved = script::preprocess(tree.root, opts);
  load_ui_objects(resolved, mgr);
  return true;
}

namespace {
bool ends_with(const std::string& s, const char* suf) {
  std::size_t n = std::strlen(suf);
  return s.size() >= n && s.compare(s.size() - n, n, suf) == 0;
}
// Load one ui/gen DTB, sharing `macros` across files (ui.dtb seeds CHARACTERS
// etc. for sel_character.dtb). Missing entry -> no-op.
void load_one_dtb(const gh::ark::ArkV3Reader& ark, const std::vector<std::string>& ark_paths,
                  const std::string& path, ScreenManager& mgr, script::MacroTable* macros) {
  try {
    auto entry = ark.find(path);
    if (!entry) return;
    std::vector<uint8_t> bytes = ark.read_entry(*entry, ark_paths);
    gh::dtb::Tree tree = gh::dtb::parse(bytes);
    script::PreprocessOptions opts;
    opts.defines = {"HX_EE", "PS2"};
    opts.macro_table = macros;
    gh::dtb::NodeList resolved = script::preprocess(tree.root, opts);
    load_ui_objects(resolved, mgr);
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[ui] load %s failed: %s\n", path.c_str(), ex.what());
  }
}
}  // namespace

int load_all_ui_screens(const gh::ark::ArkV3Reader& ark,
                        const std::vector<std::string>& ark_paths, ScreenManager& mgr) {
  script::MacroTable macros;
  std::vector<std::string> paths;
  for (const auto& e : ark.entries()) {
    if (e.full_path.rfind("ui/gen/", 0) == 0 && ends_with(e.full_path, ".dtb"))
      paths.push_back(e.full_path);
  }
  int n = 0;
  const std::string ui = "ui/gen/ui.dtb";  // first: seeds the shared macro table
  if (ark.find(ui)) { load_one_dtb(ark, ark_paths, ui, mgr, &macros); ++n; }
  for (const auto& p : paths) {
    if (p == ui) continue;
    load_one_dtb(ark, ark_paths, p, mgr, &macros);
    ++n;
  }
  return n;
}

}  // namespace ghogx::ui
