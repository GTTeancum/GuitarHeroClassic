// engine/src/ui/ui_classes.cpp -- see ui_classes.h.

#include "ui/ui_classes.h"

#include "core/class_reg.h"
#include "ui/screen_manager.h"

#include <array>
#include <cstring>
#include <string>

namespace ghogx::ui {

namespace {
// Booleans the scripts use are the symbols TRUE / FALSE.
DataNode kTrue() { return DataNode::Sym(Symbol("TRUE")); }
DataNode kFalse() { return DataNode::Sym(Symbol("FALSE")); }
DataNode arg0(const DataArray& a) { return a.size() > 0 ? a.at(0) : DataNode(); }
std::string arg0_name(const DataArray& a) {
  return a.size() > 0 ? std::string(a.at(0).as_string().value_or("")) : std::string();
}
}  // namespace

// --- UiObject --------------------------------------------------------------
DataNode UiObject::handle_property(Symbol msg, const DataArray& args) {
  // 1. A scripted handler block from the DTB wins (enter/poll/SELECT_START_MSG
  //    and custom handlers like reset_player_settings/display_cheat_msg).
  if (auto h = handler(msg)) {
    if (mgr_) return mgr_->run_object_handler(h, this, args);
    return DataNode();
  }
  // 2. Class-specific built-in.
  DataNode out;
  if (handle_builtin(msg, args, out)) return out;
  // 3. Universal Object/ObjectDir messages (get/set/has/name/...).
  return ObjectDir::handle_property(msg, args);
}

bool UiObject::handle_builtin(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();
  if (std::strcmp(m, "set_showing") == 0) {
    set_property(Symbol("showing"), args.size() ? arg0(args) : kTrue());
    return true;
  }
  if (std::strcmp(m, "get_showing") == 0) {
    out = get_property(Symbol("showing"));
    return true;
  }
  if (std::strcmp(m, "set_state") == 0) {
    set_property(Symbol("state"), arg0(args));
    return true;
  }
  if (std::strcmp(m, "set_text") == 0 || std::strcmp(m, "set_localized_text") == 0 ||
      std::strcmp(m, "set_token") == 0) {
    set_property(Symbol("text"), arg0(args));
    return true;
  }
  return false;
}

// --- UIComponentObj (leaf widget; self-targeted) ---------------------------
bool UIComponentObj::handle_builtin(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();
  if (std::strcmp(m, "enable") == 0) { set_property(Symbol("disabled"), kFalse()); return true; }
  if (std::strcmp(m, "disable") == 0) { set_property(Symbol("disabled"), kTrue()); return true; }
  return UiObject::handle_builtin(msg, args, out);
}

// --- GHPanelObj (operations target NAMED children) -------------------------
bool GHPanelObj::handle_builtin(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();
  if (std::strcmp(m, "set_focus") == 0 || std::strcmp(m, "focus") == 0) {
    set_property(Symbol("focus"), arg0(args));
    return true;
  }
  if (std::strcmp(m, "enable") == 0 || std::strcmp(m, "disable") == 0) {
    DataNode v = (m[0] == 'd') ? kTrue() : kFalse();  // disable -> disabled TRUE
    if (Object* child = find_path(arg0_name(args)))
      child->set_property(Symbol("disabled"), v);
    return true;
  }
  if (std::strcmp(m, "load") == 0 || std::strcmp(m, "unload") == 0) {
    // MILO load/unload is wired in Phase 5; accept the lifecycle message now.
    return true;
  }
  return UiObject::handle_builtin(msg, args, out);
}

// --- GHScreenObj -----------------------------------------------------------
bool GHScreenObj::handle_builtin(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();
  if (std::strcmp(m, "load") == 0 || std::strcmp(m, "unload") == 0) return true;
  return UiObject::handle_builtin(msg, args, out);
}

// --- registration ----------------------------------------------------------
void register_ui_classes() {
  ClassReg& reg = ClassReg::instance();

  reg.define(Symbol("GHPanel"), Symbol("Object"));
  reg.set_creator(Symbol("GHPanel"), [] { return std::make_unique<GHPanelObj>(); });

  reg.define(Symbol("GHScreen"), Symbol("Object"));
  reg.set_creator(Symbol("GHScreen"), [] { return std::make_unique<GHScreenObj>(); });

  // Widget + Band* classes: one C++ class, name bound by the creator. (Super-
  // class chain is refined from the recomp as behavior demands; these defaults
  // give correct is_a("UIComponent") grouping.)
  reg.define(Symbol("UIComponent"), Symbol("Object"));
  reg.set_creator(Symbol("UIComponent"),
                  [] { return std::make_unique<UIComponentObj>(Symbol("UIComponent")); });

  static const std::array<const char*, 18> kWidgets = {
      "UILabel", "UIButton", "UIPicture", "UIList", "UISlider", "CheckBox",
      "UIProxy", "ScreenMask", "UITrigger", "EventTrigger", "PanelDir",
      "BandLabel", "BandButton", "BandSlider", "BandTextEntry", "BandCharacter",
      "BandPlacer", "UIPanel"};
  for (const char* w : kWidgets) {
    Symbol cls(w);
    reg.define(cls, Symbol("UIComponent"));
    reg.set_creator(cls, [cls] { return std::make_unique<UIComponentObj>(cls); });
  }
}

}  // namespace ghogx::ui
