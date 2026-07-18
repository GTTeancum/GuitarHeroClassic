// engine/src/ui/screen_loader.cpp -- see screen_loader.h.

#include "ui/screen_loader.h"

#include "core/class_reg.h"
#include "dtb_bridge/dtb_bridge.h"
#include "milo.h"
#include "script/preprocess.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include <algorithm>
#include <cstdio>
#include <cstdint>
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

void apply_object_body_entry(Object* obj, const std::shared_ptr<Node>& entry) {
  if (!obj || !entry || entry->tag != 0x10) return;
  const NodeList& ek = gh::dtb::children(*entry);
  if (ek.empty()) return;

  // #define bodies such as AUDIO_SETTINGS_PANEL_HANDLERS expand to a grouped
  // list of keyed entries: ((focus ...)(FOCUS_MSG ...)(enter ...)). Flatten
  // those groups so stock panel macros become real properties/handlers.
  if (!gh::dtb::is_string_like(*ek[0])) {
    for (const auto& child : ek) apply_object_body_entry(obj, child);
    return;
  }

  Symbol key(nstr(*ek[0]));
  if (entry_is_handler(ek)) {
    if (auto* ui = dynamic_cast<UiObject*>(obj)) ui->add_handler(key, entry);
  } else if (ek.size() == 2) {
    obj->set_property(key, dtb_bridge::from_node(*ek[1]));
  } else if (ek.size() > 2) {
    NodeList vals(ek.begin() + 1, ek.end());
    obj->set_property(key, DataNode::Array(dtb_bridge::from_node_list(vals)));
  }
}
}  // namespace

void load_ui_objects(const NodeList& roots, ScreenManager& mgr) {
  for (const auto& rp : roots) {
    const Node& r = *rp;
    if (r.tag != 0x11) continue;  // top-level script commands: {new ...}, {func ...}
    const NodeList& k = gh::dtb::children(r);
    if (k.size() >= 2 && k[0]->tag == 0x05 && nstr(*k[0]) == "func") {
      mgr.add_function(Symbol(nstr(*k[1])), rp);
      continue;
    }
    if (k.size() < 3 || k[0]->tag != 0x05 || nstr(*k[0]) != "new") continue;

    Symbol cls(nstr(*k[1]));
    Symbol name(nstr(*k[2]));
    std::unique_ptr<Object> obj = ClassReg::instance().create(cls);
    if (!obj) { mgr.on_unhandled(std::string("new?:") + cls.c_str()); continue; }
    obj->set_name(name);
    if (auto* ui = dynamic_cast<UiObject*>(obj.get())) ui->set_manager(&mgr);

    for (std::size_t i = 3; i < k.size(); ++i)
      apply_object_body_entry(obj.get(), k[i]);
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

gh::dtb::NodeList load_ui_script_roots_from_ark(
    const gh::ark::ArkV3Reader& ark,
    const std::vector<std::string>& ark_paths,
    const std::string& dtb_path) {
  gh::dtb::NodeList out;
  auto entry = ark.find(dtb_path);
  if (!entry) return out;

  std::vector<uint8_t> bytes = ark.read_entry(*entry, ark_paths);
  gh::dtb::Tree tree = gh::dtb::parse(bytes);
  script::PreprocessOptions opts;
  opts.defines = {"HX_EE", "PS2"};
  gh::dtb::NodeList resolved = script::preprocess(tree.root, opts);
  for (const auto& rp : resolved) {
    if (!rp || rp->tag != 0x11) continue;
    const NodeList& k = gh::dtb::children(*rp);
    if (!k.empty() && k[0]->tag == 0x05) {
      const std::string head = nstr(*k[0]);
      if (head == "new" || head == "func") continue;
    }
    out.push_back(rp);
  }
  return out;
}

namespace {
bool ends_with(const std::string& s, const char* suf) {
  std::size_t n = std::strlen(suf);
  return s.size() >= n && s.compare(s.size() - n, n, suf) == 0;
}
std::string panel_milo_path(const std::string& file) {
  if (file.empty()) return {};
  if (ends_with(file, "_ps2")) return "ui/gen/" + file;
  if (ends_with(file, ".milo")) return "ui/gen/" + file + "_ps2";
  return {};
}
bool is_script_visible_milo_object(Symbol type) {
  const char* t = type.c_str();
  return std::strcmp(t, "Group") == 0 || std::strcmp(t, "Mesh") == 0 ||
         std::strcmp(t, "Mat") == 0 || std::strcmp(t, "Tex") == 0 ||
         std::strcmp(t, "Trans") == 0 || std::strcmp(t, "TransAnim") == 0 ||
         std::strcmp(t, "MeshAnim") == 0 || std::strcmp(t, "MatAnim") == 0 ||
         std::strcmp(t, "PollAnim") == 0 || std::strcmp(t, "AnimFilter") == 0 ||
         std::strcmp(t, "Environ") == 0 || std::strcmp(t, "Light") == 0;
}

std::int32_t i32_at(const std::vector<std::uint8_t>& bytes, std::size_t pos) {
  std::int32_t out = 0;
  if (pos + sizeof(out) <= bytes.size())
    std::memcpy(&out, bytes.data() + pos, sizeof(out));
  return out;
}

void seed_milo_widget_state(Object* child, const gh::milo::Entry& entry,
                            const std::vector<std::uint8_t>& payload) {
  if (!child || entry.offset > payload.size() ||
      entry.size > payload.size() - entry.offset)
    return;
  const std::size_t base = static_cast<std::size_t>(entry.offset);
  const std::size_t size = static_cast<std::size_t>(entry.size);
  const std::string type = entry.type;

  if ((type == "CheckBox" || type == "CheckboxDisplay") && size >= 5) {
    // MiloLib CheckboxDisplay names this serialized field `isChecked`; GH2's
    // older CheckBox stores the same state in the low byte of the first int.
    child->set_property(Symbol("checked"),
                        DataNode::Int(payload[base + 4] != 0 ? 1 : 0));
    return;
  }

  if ((type == "BandSlider" || type == "UISlider") && size >= 16) {
    // Harmonix UISlider source fields: mCurrent, mNumSteps, mVertical.
    child->set_property(Symbol("current"),
                        DataNode::Int(i32_at(payload, base + 4)));
    child->set_property(Symbol("num_steps"),
                        DataNode::Int(std::max(1, i32_at(payload, base + 8))));
    child->set_property(Symbol("vertical"),
                        DataNode::Int(i32_at(payload, base + 12) != 0 ? 1 : 0));
  }
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

void collect_macros_from_dtb(const gh::ark::ArkV3Reader& ark,
                             const std::vector<std::string>& ark_paths,
                             const std::string& path,
                             script::MacroTable& macros) {
  try {
    auto entry = ark.find(path);
    if (!entry) return;
    std::vector<uint8_t> bytes = ark.read_entry(*entry, ark_paths);
    gh::dtb::Tree tree = gh::dtb::parse(bytes);
    script::PreprocessOptions opts;
    opts.defines = {"HX_EE", "PS2"};
    opts.macro_table = &macros;
    (void)script::preprocess(tree.root, opts);
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[ui] macro scan %s failed: %s\n", path.c_str(), ex.what());
  }
}

void load_locale_dtb(const gh::ark::ArkV3Reader& ark,
                     const std::vector<std::string>& ark_paths,
                     ScreenManager& mgr) {
  try {
    auto entry = ark.find("ui/eng/gen/locale.dtb");
    if (!entry) return;
    auto bytes = ark.read_entry(*entry, ark_paths);
    gh::dtb::Tree tree = gh::dtb::parse(bytes);
    std::shared_ptr<DataArray> root = dtb_bridge::from_tree(tree);
    if (!root) return;
    for (std::size_t i = 0; i < root->size(); ++i) {
      auto row = root->at(i).as_array();
      if (!row || row->size() < 2) continue;
      auto key = row->at(0).as_string();
      auto value = row->at(1).as_string();
      if (!key || !value) continue;
      mgr.set_locale_string(Symbol(*key), std::string(*value));
    }
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[ui] locale load failed: %s\n", ex.what());
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
  if (ark.find(ui)) collect_macros_from_dtb(ark, ark_paths, ui, macros);
  for (const auto& p : paths) {
    if (p == ui) continue;
    collect_macros_from_dtb(ark, ark_paths, p, macros);
  }
  if (ark.find(ui)) { load_one_dtb(ark, ark_paths, ui, mgr, &macros); ++n; }
  for (const auto& p : paths) {
    if (p == ui) continue;
    load_one_dtb(ark, ark_paths, p, mgr, &macros);
    ++n;
  }
  load_locale_dtb(ark, ark_paths, mgr);
  return n;
}

int load_panel_milo_widgets(const gh::ark::ArkV3Reader& ark,
                            const std::vector<std::string>& ark_paths,
                            ScreenManager& mgr) {
  int loaded = 0;
  ClassReg& reg = ClassReg::instance();
  static const Symbol kUIComponent("UIComponent");
  ObjectDir& objects = mgr.registry();
  for (std::size_t i = 0; i < objects.size(); ++i) {
    Object* owner = objects.at(i);
    auto* panel = dynamic_cast<ObjectDir*>(owner);
    if (!panel) continue;
    std::string file(owner->get_property(Symbol("file")).as_string().value_or(""));
    const std::string milo_path = panel_milo_path(file);
    if (milo_path.empty()) continue;
    try {
      auto entry = ark.find(milo_path);
      if (!entry) entry = ark.find("../../system/run/" + milo_path);
      if (!entry) continue;
      auto bytes = ark.read_entry(*entry, ark_paths);
      auto h = gh::milo::parse_header(bytes);
      auto payload = gh::milo::inflate_payload(bytes, h);
      auto dir = gh::milo::parse_directory(payload);
      for (const auto& e : dir.entries) {
        Symbol type(e.type);
        const bool is_widget = reg.is_a(type, kUIComponent);
        const bool is_script_object = is_script_visible_milo_object(type);
        if (!reg.creatable(type) || (!is_widget && !is_script_object))
          continue;
        if (panel->find(Symbol(e.name))) continue;
        Object* child = panel->create_child(type, Symbol(e.name));
        if (auto* ui = dynamic_cast<UiObject*>(child)) ui->set_manager(&mgr);
        seed_milo_widget_state(child, e, payload);
        if (child) ++loaded;
      }
    } catch (const std::exception& ex) {
      std::fprintf(stderr, "[ui] panel widgets %s failed: %s\n",
                   milo_path.c_str(), ex.what());
    }
  }
  return loaded;
}

}  // namespace ghogx::ui
