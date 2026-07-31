// engine/src/ui/screen_loader.cpp -- see screen_loader.h.

#include "ui/screen_loader.h"

#include "core/class_reg.h"
#include "dtb_bridge/dtb_bridge.h"
#include "milo.h"
#include "script/preprocess.h"
#include "ui/menu_labels.h"
#include "milo_scene/milo_scene.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

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
         std::strcmp(t, "Environ") == 0 || std::strcmp(t, "EnvAnim") == 0 ||
         std::strcmp(t, "Light") == 0;
}

std::int32_t i32_at(const std::vector<std::uint8_t>& bytes, std::size_t pos) {
  std::int32_t out = 0;
  if (pos + sizeof(out) <= bytes.size())
    std::memcpy(&out, bytes.data() + pos, sizeof(out));
  return out;
}

struct TextEntryStyle {
  std::string text_resource;
  std::string characters;
  int max_length = 0;
};
using TextEntryStyles = std::unordered_map<std::string, TextEntryStyle>;

void collect_text_entry_styles(const Node& node, TextEntryStyles& out) {
  if (!gh::dtb::is_array(node)) return;
  const NodeList& kids = gh::dtb::children(node);
  if (!kids.empty() && nstr(*kids[0]) == "styles") {
    for (std::size_t i = 1; i < kids.size(); ++i) {
      if (!kids[i] || !gh::dtb::is_array(*kids[i])) continue;
      const NodeList& style = gh::dtb::children(*kids[i]);
      if (style.empty()) continue;
      TextEntryStyle decoded;
      for (std::size_t field_index = 1; field_index < style.size();
           ++field_index) {
        if (!style[field_index] ||
            !gh::dtb::is_array(*style[field_index]))
          continue;
        const NodeList& field = gh::dtb::children(*style[field_index]);
        if (field.size() < 2) continue;
        const std::string key = nstr(*field[0]);
        if (key == "text")
          decoded.text_resource = nstr(*field[1]);
        else if (key == "characters")
          decoded.characters = nstr(*field[1]);
        else if (key == "length")
          decoded.max_length = gh::dtb::as_int(*field[1]).value_or(0);
      }
      if (!decoded.characters.empty() && decoded.max_length > 0)
        out[nstr(*style[0])] = std::move(decoded);
    }
  }
  for (const auto& child : kids)
    if (child) collect_text_entry_styles(*child, out);
}

TextEntryStyles load_text_entry_styles(
    const gh::ark::ArkV3Reader& ark,
    const std::vector<std::string>& ark_paths) {
  TextEntryStyles out;
  try {
    auto entry = ark.find("ui/gen/config.dtb");
    if (!entry) return out;
    const std::vector<std::uint8_t> bytes =
        ark.read_entry(*entry, ark_paths);
    const gh::dtb::Tree tree = gh::dtb::parse(bytes);
    for (const auto& root : tree.root)
      if (root) collect_text_entry_styles(*root, out);
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[ui] text-entry style load failed: %s\n",
                 ex.what());
  }
  return out;
}

bool body_contains_cstring(const std::vector<std::uint8_t>& body,
                           const std::string& value) {
  if (value.empty()) return false;
  const auto begin = std::search(body.begin(), body.end(), value.begin(),
                                 value.end());
  return begin != body.end() &&
         begin + static_cast<std::ptrdiff_t>(value.size()) != body.end() &&
         *(begin + static_cast<std::ptrdiff_t>(value.size())) == 0;
}

void seed_milo_widget_state(Object* child, const gh::milo::Entry& entry,
                            const std::vector<std::uint8_t>& payload,
                            const TextEntryStyles& text_entry_styles) {
  if (!child || entry.offset > payload.size() ||
      entry.size > payload.size() - entry.offset)
    return;
  const std::size_t base = static_cast<std::size_t>(entry.offset);
  const std::size_t size = static_cast<std::size_t>(entry.size);
  const std::string type = entry.type;
  const std::vector<std::uint8_t> body(
      payload.begin() + static_cast<std::ptrdiff_t>(base),
      payload.begin() + static_cast<std::ptrdiff_t>(base + size));

  if (type == "BandTextEntry") {
    for (const auto& [name, style] : text_entry_styles) {
      if (!body_contains_cstring(body, name)) continue;
      child->set_property(Symbol("characters"),
                          DataNode::Str(style.characters));
      child->set_property(Symbol("max_length"),
                          DataNode::Int(style.max_length));
      child->set_property(Symbol("text_resource"),
                          DataNode::Sym(Symbol(style.text_resource)));
      child->set_property(Symbol("text_entry_style"),
                          DataNode::Sym(Symbol(name)));
      break;
    }
  }

  if (type == "UITrigger") {
    const MenuUiTrigger trigger = decode_menu_ui_trigger_body(body, entry.name);
    if (!trigger.valid) return;
    child->set_property(Symbol("event"), DataNode::Sym(Symbol(trigger.event)));
    child->set_property(Symbol("anim_ref"),
                        DataNode::Sym(Symbol(trigger.anim_ref)));
    child->set_property(Symbol("block_transition"),
                        DataNode::Int(trigger.block_transition ? 1 : 0));
    child->set_property(Symbol("end_time"), DataNode::Float(0.0f));
    return;
  }

  if (type == "AnimFilter") {
    const MenuAnimFilter filter =
        decode_menu_anim_filter_body(body, entry.name);
    if (!filter.valid) return;
    child->set_property(Symbol("anim_ref"),
                        DataNode::Sym(Symbol(filter.trans_anim)));
    child->set_property(Symbol("frame"), DataNode::Float(filter.frame));
    child->set_property(Symbol("scale"), DataNode::Float(filter.scale));
    child->set_property(Symbol("offset"), DataNode::Float(filter.offset));
    child->set_property(Symbol("start"), DataNode::Float(filter.start));
    child->set_property(Symbol("end"), DataNode::Float(filter.end));
    child->set_property(Symbol("filter_type"), DataNode::Int(filter.type));
    child->set_property(Symbol("period"), DataNode::Float(filter.period));
    float scale = filter.scale;
    if (!std::isfinite(scale) || std::fabs(scale) <= 0.0001f) scale = 1.0f;
    if (filter.end < filter.start) scale = -std::fabs(scale);
    const float frame_offset =
        filter.offset +
        (filter.end < filter.start ? filter.start - filter.end : 0.0f);
    float start_frame = (filter.start - frame_offset) / scale;
    float end_frame = (filter.end - frame_offset) / scale;
    if (filter.type == 2) end_frame *= 2.0f;
    child->set_property(Symbol("start_frame"), DataNode::Float(start_frame));
    child->set_property(Symbol("end_frame"), DataNode::Float(end_frame));
    return;
  }

  if (type == "TransAnim") {
    const MenuSliderAnim anim = decode_menu_trans_anim_body(body, entry.name);
    if (!anim.valid) return;
    child->set_property(Symbol("target"), DataNode::Sym(Symbol(anim.target)));
    child->set_property(Symbol("keys_owner"),
                        DataNode::Sym(Symbol(anim.keys_owner)));
    child->set_property(Symbol("start_frame"),
                        DataNode::Float(anim.first_frame));
    child->set_property(Symbol("end_frame"), DataNode::Float(anim.last_frame));
    return;
  }

  if (type == "MatAnim") {
    const MenuMaterialAnim anim =
        decode_menu_material_anim_body(body, entry.name);
    if (!anim.valid) return;
    child->set_property(Symbol("target"), DataNode::Sym(Symbol(anim.material)));
    child->set_property(Symbol("keys_owner"),
                        DataNode::Sym(Symbol(anim.keys_owner)));
    child->set_property(Symbol("start_frame"),
                        DataNode::Float(anim.first_frame));
    child->set_property(Symbol("end_frame"),
                        DataNode::Float(anim.last_frame));
    return;
  }

  if (type == "EnvAnim") {
    const auto anim = milo_scene::decode_env_anim(entry.name, body);
    if (!anim.decoded) return;
    child->set_property(Symbol("target"),
                        DataNode::Sym(Symbol(anim.environment)));
    child->set_property(Symbol("keys_owner"),
                        DataNode::Sym(Symbol(anim.keys_owner)));
    child->set_property(Symbol("frame"), DataNode::Float(anim.frame));
    float end_frame = 0.0f;
    for (const auto& key : anim.ambient_color_keys)
      end_frame = std::max(end_frame, key.frame);
    for (const auto& key : anim.fog_color_keys)
      end_frame = std::max(end_frame, key.frame);
    for (const auto& key : anim.fog_range_keys)
      end_frame = std::max(end_frame, key.frame);
    child->set_property(Symbol("start_frame"), DataNode::Float(0.0f));
    child->set_property(Symbol("end_frame"), DataNode::Float(end_frame));
    return;
  }

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

namespace {
bool stock_route_head(const std::string& head) {
  return head == "meta_loading_goto" ||
         head == "meta_loading_nosave_goto" || head == "autosave_goto";
}

void collect_route_nodes(const Node& node, const std::string& owner,
                         std::vector<UiRouteRef>& out) {
  if (!gh::dtb::is_array(node)) return;
  const NodeList& children = gh::dtb::children(node);
  if (node.tag == 0x11 && !children.empty()) {
    std::string operation;
    const Node* target_node = nullptr;
    const std::string head = nstr(*children[0]);
    if (head == "ui" && children.size() >= 2) {
      const std::string method = nstr(*children[1]);
      if (method == "goto_screen" || method == "push_screen" ||
          method == "pop_screen" || method == "reset_screen") {
        operation = method;
        if (children.size() >= 3) target_node = children[2].get();
      }
    } else if (stock_route_head(head)) {
      operation = head;
      if (children.size() >= 2) target_node = children[1].get();
    }
    if (!operation.empty()) {
      UiRouteRef route;
      route.owner = owner;
      route.operation = operation;
      route.source_line = node.line;
      if (target_node)
        route.target = gh::dtb::as_string(*target_node).value_or("");
      // DTB tag 0x02 is a variable. gh::dtb::as_string returns its stored name
      // without the decompiler's leading '$', so classify from the source tag
      // instead of from the rendered spelling.
      route.dynamic = target_node &&
                      (target_node->tag == 0x02 || route.target.empty());
      out.push_back(std::move(route));
    }
  }
  for (const auto& child : children)
    if (child) collect_route_nodes(*child, owner, out);
}

std::string function_owner(const Node& block) {
  const NodeList& children = gh::dtb::children(block);
  if (children.size() >= 2 && nstr(*children[0]) == "func")
    return "func:" + nstr(*children[1]);
  return "func:?";
}
}  // namespace

std::vector<UiRouteRef> collect_ui_route_refs(const ScreenManager& mgr) {
  std::vector<UiRouteRef> out;
  const ObjectDir& registry = mgr.registry();
  for (std::size_t i = 0; i < registry.size(); ++i) {
    Object* object = registry.at(i);
    auto* ui = dynamic_cast<UiObject*>(object);
    if (!ui) continue;
    for (const auto& block : ui->handler_blocks())
      if (block) collect_route_nodes(*block, object->name().c_str(), out);
  }
  for (const auto& block : mgr.function_blocks())
    if (block) collect_route_nodes(*block, function_owner(*block), out);
  return out;
}

int load_panel_milo_widgets(const gh::ark::ArkV3Reader& ark,
                            const std::vector<std::string>& ark_paths,
                            ScreenManager& mgr) {
  int loaded = 0;
  const TextEntryStyles text_entry_styles =
      load_text_entry_styles(ark, ark_paths);
  ClassReg& reg = ClassReg::instance();
  static const Symbol kUIComponent("UIComponent");
  ObjectDir& objects = mgr.registry();
  for (std::size_t i = 0; i < objects.size(); ++i) {
    Object* owner = objects.at(i);
    auto* panel = dynamic_cast<ObjectDir*>(owner);
    if (!panel) continue;
    DataNode file_node = owner->get_property(Symbol("file"));
    if (file_node.empty())
      file_node = owner->handle_property(Symbol("file"), DataArray());
    std::string file;
    if (auto text = file_node.as_string()) file = std::string(*text);
    if (auto sym = file_node.as_symbol()) file = sym->c_str();
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
        Object* child = panel->find(Symbol(e.name));
        if (!child) {
          child = panel->create_child(type, Symbol(e.name));
          if (auto* ui = dynamic_cast<UiObject*>(child)) ui->set_manager(&mgr);
          if (child) ++loaded;
        }
        seed_milo_widget_state(child, e, payload, text_entry_styles);
      }

      // RndGroup::SetFrame propagates to the animatable objects named in its
      // authored object list, and RndGroup::EndFrame is the maximum of those
      // children. Preserve that graph so no-argument stock `animate` commands
      // use the shipped range instead of silently becoming zero-length tasks.
      for (const auto& e : dir.entries) {
        if (e.type != "Group" || e.offset + e.size > payload.size()) continue;
        Object* group_object = panel->find(Symbol(e.name));
        if (!group_object) continue;
        std::vector<std::uint8_t> body(payload.begin() + e.offset,
                                       payload.begin() + e.offset + e.size);
        const milo_scene::GroupObj group =
            milo_scene::decode_group(e.name, body, dir.dir_version);
        if (!group.decoded) continue;
        auto children = std::make_shared<DataArray>();
        for (const std::string& name : group.children)
          children->push(DataNode::Sym(Symbol(name)));
        group_object->set_property(Symbol("anim_children"),
                                   DataNode::Array(children));
        group_object->set_property(Symbol("showing"),
                                   DataNode::Int(group.showing ? 1 : 0));
      }

      const auto is_animatable = [](Object* object) {
        if (!object) return false;
        const Symbol type = object->class_name();
        return type == Symbol("TransAnim") || type == Symbol("MatAnim") ||
               type == Symbol("EnvAnim") || type == Symbol("AnimFilter") ||
               type == Symbol("Group");
      };
      std::function<float(Object*, std::unordered_set<Object*>&)> end_frame_of;
      end_frame_of = [&](Object* object,
                         std::unordered_set<Object*>& visiting) -> float {
        if (!object || !visiting.insert(object).second) return 0.0f;
        float end = object->get_property(Symbol("end_frame"))
                        .as_float().value_or(0.0f);
        const Symbol keys_owner =
            object->get_property(Symbol("keys_owner"))
                .as_symbol().value_or(Symbol());
        if (keys_owner.valid() && keys_owner != object->name()) {
          if (Object* owner_anim = panel->find(keys_owner))
            end = std::max(end, end_frame_of(owner_anim, visiting));
        }
        if (object->class_name() == Symbol("Group")) {
          if (auto children =
                  object->get_property(Symbol("anim_children")).as_array()) {
            for (std::size_t child_i = 0; child_i < children->size(); ++child_i) {
              const Symbol child_name =
                  children->at(child_i).as_symbol().value_or(Symbol());
              Object* child = child_name.valid() ? panel->find(child_name) : nullptr;
              if (is_animatable(child))
                end = std::max(end, end_frame_of(child, visiting));
            }
          }
        }
        visiting.erase(object);
        object->set_property(Symbol("start_frame"), DataNode::Float(0.0f));
        object->set_property(Symbol("end_frame"), DataNode::Float(end));
        return end;
      };
      float panel_end = 0.0f;
      for (std::size_t child_i = 0; child_i < panel->size(); ++child_i) {
        Object* child = panel->at(child_i);
        if (!is_animatable(child)) continue;
        std::unordered_set<Object*> visiting;
        panel_end = std::max(panel_end, end_frame_of(child, visiting));
      }
      owner->set_property(Symbol("start_frame"), DataNode::Float(0.0f));
      owner->set_property(Symbol("end_frame"), DataNode::Float(panel_end));
    } catch (const std::exception& ex) {
      std::fprintf(stderr, "[ui] panel widgets %s failed: %s\n",
                   milo_path.c_str(), ex.what());
    }
  }
  return loaded;
}

}  // namespace ghogx::ui
