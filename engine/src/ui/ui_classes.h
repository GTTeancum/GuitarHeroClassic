// engine/src/ui/ui_classes.h
//
// The GH2 UI object classes. In Harmonix's Sandbox engine a menu is a tree of
// scripted objects instantiated from the DTB by {new <Class> <name> ...}. The
// stock UI uses ~24 such classes (GHScreen/GHPanel/UIPanel + specialized panels
// StorePanel/TrackPanel/HudPanel/LagPanel/MultiSelectPanel/SliderPanel/...; see
// STOCK_SURFACE.txt for the full roster), plus the MILO-side widget classes
// (UIButton/UILabel/UIList/... and the Band* variants).
//
// They share one machinery: an Object that stores the (enter)/(poll)/
// (SELECT_START_MSG ...)/custom handler blocks parsed from its {new ...} body
// and fires them VERBATIM through the ScreenManager's interpreter when the
// matching message arrives. So a single C++ class (UiObject) backs every menu
// object, with the class name bound at construction; class-SPECIFIC engine
// primitives (the messages a class receives that are NOT authored as handlers)
// are grounded per the fidelity log (FIDELITY.md) as they are implemented.

#pragma once

#include "core/data_node.h"
#include "core/object_dir.h"
#include "core/symbol.h"

#include "dtb.h"  // gh::dtb::Node

#include <memory>
#include <unordered_map>
#include <vector>

namespace ghogx::ui {

class ScreenManager;
using Node = gh::dtb::Node;

// One concrete class backs every scripted menu object; `cls_` is its registered
// class name (GHScreen/GHPanel/StorePanel/UIButton/...). Holds the handler
// blocks parsed from the {new ...} body and the child widgets (via ObjectDir).
// Dispatch order on a message: a matching scripted handler wins (run verbatim);
// else a common engine built-in (handle_builtin); else the universal Object
// messages (get/set/has/name/...).
class UiObject : public ObjectDir {
 public:
  explicit UiObject(Symbol cls = Symbol("UiObject")) : cls_(cls) {}
  Symbol class_name() const override { return cls_; }

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
  std::vector<std::shared_ptr<Node>> handler_blocks() const {
    std::vector<std::shared_ptr<Node>> out;
    out.reserve(handlers_.size());
    for (const auto& [key, block] : handlers_) {
      (void)key;
      if (block) out.push_back(block);
    }
    return out;
  }

  DataNode handle_property(Symbol msg, const DataArray& args) override;

 protected:
  // Common engine built-ins (state/focus/enable-disable/lifecycle). Class-
  // specific primitives are layered on in step 3; see FIDELITY.md. Returns true
  // (and sets `out`) when handled.
  virtual bool handle_builtin(Symbol msg, const DataArray& args, DataNode& out);

  Symbol cls_;
  ScreenManager* mgr_ = nullptr;
  std::unordered_map<const void*, std::shared_ptr<Node>> handlers_;
};

// Register every stock {new}-able class (the full screen/panel roster from
// STOCK_SURFACE.txt) + the MILO widget classes with ClassReg, name -> factory,
// so the loader and MILO instantiation can create them by name.
void register_ui_classes();

}  // namespace ghogx::ui
