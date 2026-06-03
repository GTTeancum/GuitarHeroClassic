// engine/src/ui/ui_classes.cpp -- see ui_classes.h.

#include "ui/ui_classes.h"

#include "core/class_reg.h"
#include "ui/screen_manager.h"

#include <array>
#include <cstring>
#include <string>

namespace ghogx::ui {

namespace {
DataNode kTrue() { return DataNode::Sym(Symbol("TRUE")); }
DataNode kFalse() { return DataNode::Sym(Symbol("FALSE")); }
DataNode arg0(const DataArray& a) { return a.size() > 0 ? a.at(0) : DataNode(); }
std::string arg0_name(const DataArray& a) {
  return a.size() > 0 ? std::string(a.at(0).as_string().value_or("")) : std::string();
}
}  // namespace

DataNode UiObject::handle_property(Symbol msg, const DataArray& args) {
  // 1. A scripted handler block from the DTB wins -- run verbatim.
  if (auto h = handler(msg)) {
    if (mgr_) return mgr_->run_object_handler(h, this, args);
    return DataNode();
  }
  // 2. Common engine built-in.
  DataNode out;
  if (handle_builtin(msg, args, out)) return out;
  // 3. Universal Object/ObjectDir messages (get/set/has/name/...).
  return ObjectDir::handle_property(msg, args);
}

bool UiObject::handle_builtin(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();

  // --- self visual/text state ---
  if (std::strcmp(m, "set_showing") == 0) {
    set_property(Symbol("showing"), args.size() ? arg0(args) : kTrue());
    return true;
  }
  if (std::strcmp(m, "get_showing") == 0) { out = get_property(Symbol("showing")); return true; }
  if (std::strcmp(m, "set_state") == 0) { set_property(Symbol("state"), arg0(args)); return true; }
  if (std::strcmp(m, "set_text") == 0 || std::strcmp(m, "set_localized_text") == 0 ||
      std::strcmp(m, "set_token") == 0) {
    set_property(Symbol("text"), arg0(args));
    return true;
  }

  // --- focus: a panel stores the focused child's name ---
  if (std::strcmp(m, "set_focus") == 0 || std::strcmp(m, "focus") == 0 ||
      std::strcmp(m, "update_focus") == 0) {
    set_property(Symbol("focus"), arg0(args));
    return true;
  }

  // --- enable/disable: a named child if it resolves, else self. Derived from
  //     the argument (panel {$this disable a.btn} vs component {b disable}),
  //     not from the class -- so it is correct for both without guessing. ---
  if (std::strcmp(m, "enable") == 0 || std::strcmp(m, "disable") == 0) {
    DataNode v = (m[0] == 'd') ? kTrue() : kFalse();  // disable -> disabled TRUE
    std::string child = arg0_name(args);
    Object* tgt = this;
    if (!child.empty()) {
      if (Object* c = find_path(child)) tgt = c;
    }
    tgt->set_property(Symbol("disabled"), v);
    return true;
  }

  // --- transition lifecycle messages we accept as no-ops at this layer (a
  //     screen that needs them defines them as handlers, which win above). ---
  if (std::strcmp(m, "load") == 0 || std::strcmp(m, "unload") == 0 ||
      std::strcmp(m, "finish_load") == 0 || std::strcmp(m, "change_proxies") == 0) {
    return true;
  }

  return false;
}

// --- registration ----------------------------------------------------------
void register_ui_classes() {
  ClassReg& reg = ClassReg::instance();

  auto define_uiobject = [&](const char* name, const char* super) {
    Symbol cls(name);
    reg.define(cls, Symbol(super));
    reg.set_creator(cls, [cls] { return std::make_unique<UiObject>(cls); });
  };

  // Plain {new Object ...} containers.
  define_uiobject("Object", "");

  // The complete {new <Class>} screen/panel roster (STOCK_SURFACE.txt). Super
  // chain is provisional (-> Object) pending step-3 recomp grounding; it does
  // not affect verbatim loading/handler execution.
  static const char* kScreens[] = {"GHScreen", "MultiSelectScreen", "TrackBudgetScreen"};
  static const char* kPanels[] = {
      "GHPanel", "UIPanel", "MultiSelectPanel", "SliderPanel", "GuitarDisplayPanel",
      "EndGamePanel", "CharsysPanel", "GuitarSelectPanel", "TutorialPanel", "TrackPanel",
      "StorePanel", "MultiSelectListPanel", "MultiCharSelPanel", "MetaPanel", "LagPanel",
      "HudPanel", "HelpBarPanel", "GamePanel", "FadePanel", "CreditsPanel"};
  for (const char* s : kScreens) define_uiobject(s, "Object");
  for (const char* p : kPanels) define_uiobject(p, "Object");

  // MILO-side widget classes (instantiated from panel MILOs; some appear in
  // {new} too). Grouped under UIComponent for is_a() purposes.
  define_uiobject("UIComponent", "Object");
  static const char* kWidgets[] = {
      "UILabel", "UIButton", "UIPicture", "UIList", "UISlider", "CheckBox", "UIProxy",
      "ScreenMask", "UITrigger", "EventTrigger", "PanelDir", "UIColor",
      "BandLabel", "BandButton", "BandSlider", "BandList", "BandTextEntry",
      "BandCharacter", "BandPlacer", "BandStarDisplay", "BandScoreDisplay",
      "BandStreakDisplay", "BandStarMeterDir", "BandCrowdMeterDir"};
  for (const char* w : kWidgets) define_uiobject(w, "UIComponent");
}

}  // namespace ghogx::ui
