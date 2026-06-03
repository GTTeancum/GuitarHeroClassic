// engine/src/ui/screen_manager.cpp -- see screen_manager.h.

#include "ui/screen_manager.h"

#include "core/class_reg.h"

#include <cstring>

namespace ghogx::ui {

ScreenManager::ScreenManager() = default;

// --- registry --------------------------------------------------------------
void ScreenManager::add_object(std::unique_ptr<Object> obj) {
  registry_.add(std::move(obj));
}
Object* ScreenManager::find_object(Symbol name) { return registry_.find(name); }

void ScreenManager::add_singleton(Symbol name, std::unique_ptr<Object> obj) {
  singletons_[name.id()] = obj.get();
  singletons_owned_.push_back(std::move(obj));
}

// --- handler firing --------------------------------------------------------
DataNode ScreenManager::run_object_handler(const std::shared_ptr<gh::dtb::Node>& block,
                                           Object* self, const DataArray& args) {
  script::Env env;
  env.host = this;
  env.self = nullptr;
  env.scope = nullptr;
  return interp_.run_handler(gh::dtb::children(*block), self, args, env);
}

// --- screen navigation -----------------------------------------------------
std::vector<Symbol> ScreenManager::screen_panels(Object* screen) {
  std::vector<Symbol> out;
  if (!screen) return out;
  DataNode p = screen->get_property(Symbol("panels"));
  if (auto arr = p.as_array()) {
    for (std::size_t i = 0; i < arr->size(); ++i)
      if (auto s = arr->at(i).as_symbol()) out.push_back(*s);
  } else if (auto s = p.as_symbol()) {
    out.push_back(*s);
  }
  return out;
}

void ScreenManager::enter_screen(Object* screen) {
  if (!screen) return;
  // Panels first (load their MILO + run their enter), then the screen.
  for (Symbol pn : screen_panels(screen)) {
    if (Object* panel = find_object(pn)) {
      panel->handle_property(Symbol("load"), DataArray());
      panel->handle_property(Symbol("finish_load"), DataArray());
      panel->handle_property(Symbol("enter"), DataArray());
    }
  }
  screen->handle_property(Symbol("enter"), DataArray());
}

void ScreenManager::exit_screen(Object* screen) {
  if (!screen) return;
  screen->handle_property(Symbol("exit"), DataArray());
  for (Symbol pn : screen_panels(screen)) {
    if (Object* panel = find_object(pn)) panel->handle_property(Symbol("exit"), DataArray());
  }
}

void ScreenManager::goto_screen(Symbol name) {
  Object* target = find_object(name);
  if (!target) { on_unhandled(std::string("goto_screen?:") + name.c_str()); return; }
  if (current_) exit_screen(current_);
  current_ = target;
  enter_screen(current_);
}

void ScreenManager::push_screen(Symbol name) {
  Object* target = find_object(name);
  if (!target) { on_unhandled(std::string("push_screen?:") + name.c_str()); return; }
  if (current_) stack_.push_back(current_);
  current_ = target;
  enter_screen(current_);
}

void ScreenManager::pop_screen() {
  if (current_) exit_screen(current_);
  if (!stack_.empty()) {
    current_ = stack_.back();
    stack_.pop_back();
    enter_screen(current_);
  } else {
    current_ = nullptr;
  }
}

void ScreenManager::update(float dt) {
  ui_seconds_ += dt;
  if (!current_) return;
  for (Symbol pn : screen_panels(current_)) {
    if (Object* panel = find_object(pn)) panel->handle_property(Symbol("poll"), DataArray());
  }
  current_->handle_property(Symbol("poll"), DataArray());
}

DataNode ScreenManager::run_script(const gh::dtb::NodeList& roots) {
  script::Scope root;
  script::Env env;
  env.host = this;
  env.self = nullptr;
  env.scope = &root;
  DataNode last;
  for (const auto& n : roots) last = interp_.eval(*n, env);
  return last;
}

// --- Object (the `ui` object messages) -------------------------------------
DataNode ScreenManager::handle_property(Symbol msg, const DataArray& args) {
  const char* m = msg.c_str();
  if (std::strcmp(m, "goto_screen") == 0) {
    if (args.size()) if (auto s = args.at(0).as_symbol()) goto_screen(*s);
    return DataNode();
  }
  if (std::strcmp(m, "push_screen") == 0) {
    if (args.size()) if (auto s = args.at(0).as_symbol()) push_screen(*s);
    return DataNode();
  }
  if (std::strcmp(m, "pop_screen") == 0) { pop_screen(); return DataNode(); }
  if (std::strcmp(m, "current_screen") == 0)
    return current_ ? DataNode::Sym(current_->name()) : DataNode();
  if (std::strcmp(m, "in_transition") == 0) return DataNode::Int(in_transition() ? 1 : 0);
  if (std::strcmp(m, "my_init") == 0) return DataNode();
  return Object::handle_property(msg, args);
}

// --- script::Host ----------------------------------------------------------
Object* ScreenManager::resolve_object(Symbol name) {
  const char* n = name.c_str();
  if (std::strcmp(n, "ui") == 0) return this;
  auto it = singletons_.find(name.id());
  if (it != singletons_.end()) return it->second;
  if (Object* o = registry_.find(name)) return o;
  // Dotted child path (e.g. main_msg.view): search the current screen's panels.
  if (current_) {
    if (Object* o = registry_.find_path(n)) return o;
    for (Symbol pn : screen_panels(current_)) {
      if (Object* panel = find_object(pn)) {
        if (auto* dir = dynamic_cast<ObjectDir*>(panel)) {
          if (Object* c = dir->find_path(n)) return c;
        }
      }
    }
  }
  return nullptr;
}

DataNode ScreenManager::get_global(Symbol name) {
  auto it = globals_.find(name.id());
  return it == globals_.end() ? DataNode() : it->second;
}
void ScreenManager::set_global(Symbol name, DataNode value) {
  globals_[name.id()] = std::move(value);
}
void ScreenManager::on_unhandled(const std::string& what) {
  if (unhandled_seen_.emplace(what, true).second) unhandled_.push_back(what);
}

// --- singleton stubs -------------------------------------------------------
namespace {

// A placeholder for the non-UI game objects the menu scripts message
// (game/gamecfg/campaign/synth/...). Answers the few queries the menus read so
// boot doesn't fault; everything else falls to the universal Object messages
// (so set/get on it still works) and is logged to the worklist.
class StubObject : public Object {
 public:
  StubObject(Symbol cls, ScreenManager* mgr) : cls_(cls), mgr_(mgr) {}
  Symbol class_name() const override { return cls_; }

  DataNode handle_property(Symbol msg, const DataArray& args) override {
    const char* c = cls_.c_str();
    const char* m = msg.c_str();
    if (std::strcmp(c, "taskmgr") == 0 && std::strcmp(m, "ui_seconds") == 0)
      return DataNode::Float(mgr_->ui_seconds());
    if (std::strcmp(m, "num_profiles") == 0) return DataNode::Int(0);
    if (std::strcmp(m, "tutorials_done") == 0) return DataNode::Int(1);
    if (std::strcmp(m, "is_missing_multi_controller") == 0) return DataNode::Sym(Symbol("TRUE"));
    if (std::strcmp(m, "is_multiple_controllers") == 0) return DataNode::Sym(Symbol("FALSE"));
    if (std::strcmp(m, "get_controller") == 0) return DataNode::Sym(Symbol("guitar"));
    if (std::strcmp(m, "get_player_config") == 0) return DataNode::Obj(this);  // chainable stub
    // Universal get/set/has still work (menus stash state on gamecfg etc.).
    Symbol gs("get"), ss("set"), hs("has");
    if (msg == gs || msg == ss || msg == hs) return Object::handle_property(msg, args);
    mgr_->on_unhandled(std::string(c) + "::" + m);
    return DataNode();
  }

 private:
  Symbol cls_;
  ScreenManager* mgr_;
};

}  // namespace

void install_default_singletons(ScreenManager& mgr) {
  static const char* kNames[] = {"taskmgr",      "game",    "gamecfg",   "campaign",
                                 "synth",        "profilemgr", "meta",   "song_provider",
                                 "content_mgr",  "helpbar", "options",   "leaderboards"};
  for (const char* n : kNames)
    mgr.add_singleton(Symbol(n), std::make_unique<StubObject>(Symbol(n), &mgr));
}

}  // namespace ghogx::ui
