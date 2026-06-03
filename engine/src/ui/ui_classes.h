// engine/src/ui/ui_classes.h
//
// The GH2 UI object classes. In Harmonix's Sandbox engine a menu is a tree of
// scripted objects instantiated from the DTB by {new GHPanel ...}/{new GHScreen
// ...}: GHScreen (a screen = a set of panels + focus + helpbar + lifecycle
// handlers) and GHPanel (a panel = a MILO's worth of child widgets + its own
// handlers). The leaf widgets (UIButton/UILabel/UIList/...) and the GH-specific
// Band* subclasses are UIComponents.
//
// Every one of these is an Object whose (enter)/(poll)/(SELECT_START_MSG ...)/
// custom (reset_player_settings ...) handlers are DataArray scripts. We store
// those handler blocks here and fire them through the ScreenManager's
// interpreter when the matching message arrives -- so {$this reset_player_
// settings} or the manager's "enter" both run the authored script verbatim.
//
// Class names are spelled exactly as the stock DTBs use them (verified against
// ui/gen/main.dtb: GHPanel/GHScreen, NOT UIPanel/UIScreen).

#pragma once

#include "core/data_node.h"
#include "core/object_dir.h"
#include "core/symbol.h"

#include "dtb.h"  // gh::dtb::Node

#include <memory>
#include <unordered_map>

namespace ghogx::ui {

class ScreenManager;
using Node = gh::dtb::Node;

// Base for every scripted UI object. Holds the handler blocks parsed from the
// {new ...} body and the child widgets (via ObjectDir). On a message: a
// matching scripted handler wins; else a class built-in; else the universal
// Object messages (get/set/has/name/...).
class UiObject : public ObjectDir {
 public:
  Symbol class_name() const override { return Symbol("UiObject"); }

  void set_manager(ScreenManager* m) { mgr_ = m; }
  ScreenManager* manager() const { return mgr_; }

  // Store a handler block: the full keyed array node `(name [params] body...)`.
  void add_handler(Symbol name, std::shared_ptr<Node> block) {
    handlers_[name.id()] = std::move(block);
  }
  bool has_handler(Symbol name) const { return handlers_.count(name.id()) != 0; }
  std::shared_ptr<Node> handler(Symbol name) const {
    auto it = handlers_.find(name.id());
    return it == handlers_.end() ? nullptr : it->second;
  }
  std::size_t handler_count() const { return handlers_.size(); }

  DataNode handle_property(Symbol msg, const DataArray& args) override;

 protected:
  // Class-specific built-in messages. Return true (and set `out`) if handled.
  virtual bool handle_builtin(Symbol msg, const DataArray& args, DataNode& out);

  ScreenManager* mgr_ = nullptr;
  std::unordered_map<const void*, std::shared_ptr<Node>> handlers_;
};

// A leaf widget. class_name is bound at construction so a single C++ class
// serves UIButton/UILabel/UIPicture/UIList/UISlider/CheckBox/UIProxy/Band*/...
// (the factory creator supplies the name). Self-targeted state messages.
class UIComponentObj : public UiObject {
 public:
  explicit UIComponentObj(Symbol cls) : cls_(cls) {}
  Symbol class_name() const override { return cls_; }

 protected:
  bool handle_builtin(Symbol msg, const DataArray& args, DataNode& out) override;
  Symbol cls_;
};

// A panel: owns its MILO child widgets; (file) names the MILO, (focus) the
// initial child. enable/disable/set_focus operate on NAMED children.
class GHPanelObj : public UiObject {
 public:
  Symbol class_name() const override { return Symbol("GHPanel"); }

 protected:
  bool handle_builtin(Symbol msg, const DataArray& args, DataNode& out) override;
};

// A screen: (panels ...) references panels by name, (focus) the active panel,
// (helpbar ...) the button legend. Lifecycle handlers (enter/exit) run on
// transition; state messages delegate to the focused panel.
class GHScreenObj : public UiObject {
 public:
  Symbol class_name() const override { return Symbol("GHScreen"); }

 protected:
  bool handle_builtin(Symbol msg, const DataArray& args, DataNode& out) override;
};

// Register GHPanel/GHScreen + the widget/Band* classes (name -> factory) with
// the global ClassReg so the {new ...} loader and MILO child instantiation can
// create them by name.
void register_ui_classes();

}  // namespace ghogx::ui
