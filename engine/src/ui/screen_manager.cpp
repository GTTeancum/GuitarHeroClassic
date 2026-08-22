// engine/src/ui/screen_manager.cpp -- see screen_manager.h.

#include "ui/screen_manager.h"

#include "core/class_reg.h"
#include "ui/ui_classes.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <utility>

namespace ghogx::ui {

ScreenManager::ScreenManager() = default;

namespace {

int direct_audit_score(ScreenManager* mgr) {
  if (!mgr) return 0;
  if (Object* player = mgr->resolve_object(Symbol("player0"))) {
    int score = player->handle_property(Symbol("score"), DataArray())
                    .as_int()
                    .value_or(0);
    if (score > 0) return score;
  }
  if (Object* campaign = mgr->resolve_object(Symbol("campaign"))) {
    return campaign->handle_property(Symbol("career_score"), DataArray())
        .as_int()
        .value_or(0);
  }
  return 0;
}

void seed_cashaward_from_campaign(ScreenManager* mgr, Object* screen) {
  if (!mgr || !screen) return;
  if (screen->name() != Symbol("cashaward_screen") ||
      screen->get_property(Symbol("new_cash")).as_int().value_or(0) > 0) {
    return;
  }
  Object* campaign = mgr->resolve_object(Symbol("campaign"));
  if (!campaign) return;
  DataArray args;
  args.push(DataNode::Int(direct_audit_score(mgr)));
  auto result = campaign->handle_property(Symbol("finish_song"), args).as_array();
  if (!result || result->size() < 3) return;
  screen->set_property(Symbol("new_cash"), result->at(1));
  screen->set_property(Symbol("new_cash_reason"), result->at(2));
}

void seed_direct_screen_state(ScreenManager* mgr, Object* screen) {
  if (!screen) return;
  const char* name = screen->name().c_str();
  if (Object* gamecfg = mgr ? mgr->resolve_object(Symbol("gamecfg")) : nullptr) {
    if (std::strcmp(name, "endgame_screen") == 0 ||
        std::strcmp(name, "cashaward_screen") == 0) {
      gamecfg->set_property(Symbol("mode"), DataNode::Sym(Symbol("career")));
    } else if (std::strcmp(name, "multi_compete_screen") == 0) {
      gamecfg->set_property(Symbol("mode"), DataNode::Sym(Symbol("multi_vs")));
    } else if (std::strcmp(name, "multi_compete_coop_screen") == 0) {
      gamecfg->set_property(Symbol("mode"), DataNode::Sym(Symbol("multi_coop")));
    }
  }
  seed_cashaward_from_campaign(mgr, screen);
  if (std::strcmp(name, "multi_sel_character_screen") == 0) {
    if (Object* p = mgr ? mgr->find_object(Symbol("multi_char_outfit0")) : nullptr)
      p->set_property(Symbol("active"), DataNode::Int(0));
    if (Object* p = mgr ? mgr->find_object(Symbol("multi_char_outfit1")) : nullptr)
      p->set_property(Symbol("active"), DataNode::Int(0));
  }
}

}  // namespace

// --- registry --------------------------------------------------------------
void ScreenManager::add_object(std::unique_ptr<Object> obj) {
  registry_.add(std::move(obj));
}
Object* ScreenManager::find_object(Symbol name) { return registry_.find(name); }

void ScreenManager::register_runtime_class(Symbol cls, RuntimeCreator creator) {
  if (!cls.valid() || !creator) return;
  runtime_creators_[cls.id()] = std::move(creator);
}

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

void ScreenManager::send_screen_panels(Object* screen, Symbol msg, bool screen_first) {
  auto panels = [&] {
    for (Symbol pn : screen_panels(screen))
      if (Object* p = find_object(pn)) p->handle_property(msg, DataArray());
  };
  if (screen_first) { screen->handle_property(msg, DataArray()); panels(); }
  else { panels(); screen->handle_property(msg, DataArray()); }
}

namespace {
bool node_falsey(const DataNode& node) {
  if (node.empty()) return false;
  if (auto i = node.as_int()) return *i == 0;
  if (auto f = node.as_float()) return *f == 0.0f;
  if (auto s = node.as_string())
    return s->empty() || *s == "FALSE" || *s == "false" || *s == "0";
  return false;
}

bool loading_completion_is_external(Object* screen) {
  if (!screen) return false;
  const char* screen_name = screen->name().c_str();
  return std::strcmp(screen_name, "loading_screen") == 0 ||
         std::strcmp(screen_name, "practice_loading_screen") == 0;
}
}  // namespace

void ScreenManager::exit_sequence(Object* screen, bool back, bool defer_unload) {
  if (!screen) return;
  // screen_change for both directions, EXCEPT screens that define a screen_back
  // handler fire it on back (chooseprof/mem_card/bonus_material/credits) -- the
  // rule "run screen_back only if defined" reproduces those exceptions without a
  // name list (menus.md).
  auto* u = dynamic_cast<UiObject*>(screen);
  bool has_back = u && u->has_handler(Symbol("screen_back"));
  screen->handle_property(back && has_back ? Symbol("screen_back") : Symbol("screen_change"),
                          DataArray());
  send_screen_panels(screen, Symbol("exit"), /*screen_first=*/true);
  screen->handle_property(back ? Symbol("ui_exit_back") : Symbol("ui_exit"), DataArray());
  if (!defer_unload) {
    send_screen_panels(screen, Symbol("exit_complete"), /*screen_first=*/true);
    send_screen_panels(screen, Symbol("unload"), /*screen_first=*/true);
  }
}

void ScreenManager::enter_sequence(Object* screen, bool back, bool defer_complete) {
  if (!screen) return;
  seed_direct_screen_state(this, screen);
  send_screen_panels(screen, Symbol("change_proxies"), /*screen_first=*/false);
  send_screen_panels(screen, Symbol("load"), /*screen_first=*/false);
  send_screen_panels(screen, Symbol("finish_load"), /*screen_first=*/false);
  screen->handle_property(back ? Symbol("ui_enter_back") : Symbol("ui_enter"), DataArray());
  send_screen_panels(screen, Symbol("enter"), /*screen_first=*/false);  // sub-objects then screen
  // Loading completion is driven by async bank/song load state in the retail
  // game. The menu editor has no gameplay load to wait on, so keep these
  // screens inspectable instead of immediately firing their game-screen jump.
  if (!defer_complete && !loading_completion_is_external(screen)) {
    send_screen_panels(screen, Symbol("TRANSITION_COMPLETE_MSG"),
                       /*screen_first=*/false);
  }
  // Scene-state ID is refined per-screen (harmonix_symbols.h:904) when the
  // gameplay handoff is wired; menus are NORMAL/MENU here.
  scene_state_ = 1;
}

void ScreenManager::set_transition_time(float seconds) {
  if (std::isfinite(seconds) && seconds >= 0.0f)
    transition_time_seconds_ = seconds;
}

float ScreenManager::transition_progress() const {
  if (!transition_.active || transition_time_seconds_ <= 0.0f) return 1.0f;
  const float elapsed = transition_time_seconds_ - transition_.remaining;
  return std::clamp(elapsed / transition_time_seconds_, 0.0f, 1.0f);
}

void ScreenManager::finish_transition() {
  if (!transition_.active) return;
  Object* entering = transition_.entering_screen;
  Object* exiting = transition_.exiting_screen;
  transition_ = TransitionState{};
  if (entering && !loading_completion_is_external(entering)) {
    send_screen_panels(entering, Symbol("TRANSITION_COMPLETE_MSG"),
                       /*screen_first=*/false);
  }
  if (exiting) {
    send_screen_panels(exiting, Symbol("exit_complete"),
                       /*screen_first=*/true);
    send_screen_panels(exiting, Symbol("unload"), /*screen_first=*/true);
  }
}

void ScreenManager::run_global_handler(Symbol name, const DataArray& args) {
  auto fn = resolve_function(name);
  if (!fn) return;
  auto kids = gh::dtb::children(*fn);
  if (kids.empty()) return;
  gh::dtb::NodeList handler;
  std::string head;
  if (kids[0]) {
    if (auto s = gh::dtb::as_string(*kids[0])) head = std::string(*s);
  }
  if (kids[0] && kids[0]->tag == 0x05 && head == "func") {
    if (kids.size() < 2) return;
    handler.assign(kids.begin() + 1, kids.end());
  } else {
    handler = kids;
  }
  script::Env env;
  env.host = this;
  env.self = nullptr;
  env.scope = nullptr;
  interp_.run_handler(handler, nullptr, args, env);
}

void ScreenManager::note_audio_event(Symbol source, Symbol cue) {
  std::string event = std::string(source.c_str()) + ":" + cue.c_str();
  audio_events_.push_back(event);
  if (std::getenv("GHOGX_LOG_MENU_AUDIO")) {
    std::fprintf(stderr, "[menu-audio] %s\n", event.c_str());
  }
  if (audio_sink_) audio_sink_(source, cue);
}

void ScreenManager::set_audio_sink(std::function<void(Symbol, Symbol)> sink) {
  audio_sink_ = std::move(sink);
}

void ScreenManager::goto_screen_internal(Symbol name, bool back,
                                         bool record_history) {
  Object* target = find_object(name);
  if (!target) { on_unhandled(std::string("goto_screen?:") + name.c_str()); return; }
  if (target == current_) return;
  if (transition_.active) finish_transition();
  if (record_history && current_) history_.push_back(current_);
  Object* exiting = current_;
  if (current_) exit_sequence(current_, back, /*defer_unload=*/true);  // goto replaces the screen
  current_ = target;
  history_.erase(std::remove(history_.begin(), history_.end(), current_),
                 history_.end());
  const bool has_screen_transition = exiting != nullptr;
  const bool animate_transition =
      has_screen_transition &&
      !node_falsey(current_->get_property(Symbol("animate_transition"))) &&
      !node_falsey(exiting->get_property(Symbol("animate_transition")));
  enter_sequence(current_, back, /*defer_complete=*/animate_transition);
  if (animate_transition && transition_time_seconds_ > 0.0f) {
    transition_.active = true;
    transition_.back = back;
    transition_.remaining = transition_time_seconds_;
    transition_.exiting_screen = exiting;
    transition_.entering_screen = current_;
    if (std::getenv("GHOGX_LOG_MENU_TRANSITIONS")) {
      std::fprintf(stderr,
                   "[menu-transition] start %s -> %s back=%d seconds=%.3f\n",
                   exiting ? exiting->name().c_str() : "<none>",
                   current_ ? current_->name().c_str() : "<none>",
                   back ? 1 : 0, transition_time_seconds_);
    }
  } else if (has_screen_transition) {
    transition_.active = true;
    transition_.back = back;
    transition_.remaining = 0.0f;
    transition_.exiting_screen = exiting;
    transition_.entering_screen = current_;
    finish_transition();
  }
}

void ScreenManager::goto_screen(Symbol name) {
  const bool back = next_goto_is_back_;
  next_goto_is_back_ = false;
  goto_screen_internal(name, back, !back);
}

void ScreenManager::push_screen(Symbol name) {
  // Overlay: the underlying screen stays loaded (and unpolled); it is NOT exited.
  Object* target = find_object(name);
  if (!target) { on_unhandled(std::string("push_screen?:") + name.c_str()); return; }
  if (transition_.active) finish_transition();
  if (current_) stack_.push_back(current_);
  current_ = target;
  enter_sequence(current_, /*back=*/false, /*defer_complete=*/false);
}

void ScreenManager::pop_screen() {
  // Exit the overlay; the underlying screen (still loaded) resumes as current.
  if (transition_.active) finish_transition();
  if (current_) exit_sequence(current_, /*back=*/true, /*defer_unload=*/false);
  if (!stack_.empty()) {
    current_ = stack_.back();
    stack_.pop_back();
  } else {
    current_ = nullptr;
  }
}

bool ScreenManager::go_back() {
  if (history_.empty()) {
    if (std::getenv("GHOGX_LOG_MENU_BACK")) {
      std::fprintf(stderr, "[menu-back] go_back history empty\n");
    }
    return false;
  }
  Object* target = history_.back();
  history_.pop_back();
  if (!target) return false;
  if (std::getenv("GHOGX_LOG_MENU_BACK")) {
    std::fprintf(stderr, "[menu-back] go_back target=%s remaining=%zu\n",
                 target->name().c_str(), history_.size());
  }
  if (transition_.active) finish_transition();
  if (current_) exit_sequence(current_, /*back=*/true, /*defer_unload=*/true);
  Object* exiting = current_;
  current_ = target;
  const bool animate_transition =
      !node_falsey(current_->get_property(Symbol("animate_transition"))) &&
      (!exiting || !node_falsey(exiting->get_property(Symbol("animate_transition"))));
  enter_sequence(current_, /*back=*/true, /*defer_complete=*/animate_transition);
  if (animate_transition && transition_time_seconds_ > 0.0f) {
    transition_.active = true;
    transition_.back = true;
    transition_.remaining = transition_time_seconds_;
    transition_.exiting_screen = exiting;
    transition_.entering_screen = current_;
    if (std::getenv("GHOGX_LOG_MENU_TRANSITIONS")) {
      std::fprintf(stderr,
                   "[menu-transition] start %s -> %s back=1 seconds=%.3f\n",
                   exiting ? exiting->name().c_str() : "<none>",
                   current_ ? current_->name().c_str() : "<none>",
                   transition_time_seconds_);
    }
  } else if (exiting) {
    transition_.active = true;
    transition_.exiting_screen = exiting;
    transition_.entering_screen = current_;
    finish_transition();
  }
  return true;
}

void ScreenManager::update(float dt) {
  ui_seconds_ += dt;
  if (!current_) return;
  for (Symbol pn : screen_panels(current_)) {
    if (Object* panel = find_object(pn)) panel->handle_property(Symbol("poll"), DataArray());
  }
  current_->handle_property(Symbol("poll"), DataArray());
  if (transition_.active) {
    transition_.remaining -= std::max(0.0f, dt);
    if (transition_.remaining <= 0.0f) {
      if (std::getenv("GHOGX_LOG_MENU_TRANSITIONS")) {
        std::fprintf(stderr, "[menu-transition] complete %s\n",
                     current_ ? current_->name().c_str() : "<none>");
      }
      finish_transition();
    }
  }
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

void ScreenManager::set_locale(const std::map<std::string, std::string>& locale) {
  locale_.clear();
  for (const auto& kv : locale) locale_.emplace(kv.first, kv.second);
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
  if (std::strcmp(m, "focus_panel") == 0) {
    if (!current_) return DataNode();
    return current_->get_property(Symbol("focus"));
  }
  if (std::strcmp(m, "music_start") == 0) {
    note_audio_event(Symbol("ui"), Symbol("music_start"));
    return DataNode();
  }
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
Object* ScreenManager::create_object(Symbol cls, Symbol name) {
  if (!cls.valid() || !name.valid()) return nullptr;
  std::unique_ptr<Object> obj;
  auto creator = runtime_creators_.find(cls.id());
  if (creator != runtime_creators_.end()) {
    obj = creator->second(name);
  } else {
    obj = ClassReg::instance().create(cls);
  }
  if (!obj) return nullptr;
  obj->set_name(name);
  if (auto* ui = dynamic_cast<UiObject*>(obj.get())) ui->set_manager(this);
  return registry_.add(std::move(obj));
}
std::shared_ptr<gh::dtb::Node> ScreenManager::resolve_function(Symbol name) {
  auto it = functions_.find(name.id());
  return it == functions_.end() ? nullptr : it->second;
}
std::string ScreenManager::localize(Symbol token) {
  auto it = locale_.find(token.c_str());
  return it == locale_.end() ? std::string(token.c_str()) : it->second;
}
void ScreenManager::on_unhandled(const std::string& what) {
  if (unhandled_seen_.emplace(what, true).second) unhandled_.push_back(what);
}
void ScreenManager::add_function(Symbol name, std::shared_ptr<gh::dtb::Node> block) {
  functions_[name.id()] = std::move(block);
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
    if (std::strcmp(c, "synth") == 0) {
      if (std::strcmp(m, "play_sequence") == 0 ||
          std::strcmp(m, "stop_sequence") == 0) {
        if (args.size()) {
          if (auto cue = args.at(0).as_symbol())
            mgr_->note_audio_event(Symbol(m), *cue);
        }
        return DataNode();
      }
      if (std::strcmp(m, "stop_all_sfx") == 0) {
        mgr_->note_audio_event(Symbol("synth"), Symbol("stop_all_sfx"));
        return DataNode();
      }
    }
    if (std::strcmp(c, "play_sfx") == 0 ||
        std::strcmp(c, "stop_sfx") == 0 ||
        std::strcmp(c, "song_preview") == 0) {
      mgr_->note_audio_event(Symbol(c), msg);
      return DataNode();
    }
    if (std::strcmp(c, "world") == 0 &&
        (std::strcmp(m, "play_sfx") == 0 ||
         std::strcmp(m, "play_meta_sfx") == 0 ||
         std::strcmp(m, "stop_sfx") == 0)) {
      if (args.size()) {
        if (auto cue = args.at(0).as_symbol())
          mgr_->note_audio_event(Symbol(m), *cue);
      }
      return DataNode();
    }
    if (std::strcmp(c, "options") == 0 &&
        std::strcmp(m, "get_sync_offset") == 0) {
      DataNode offset = get_property(Symbol("sync_offset"));
      return offset.empty() ? DataNode::Int(0) : offset;
    }
    if (std::strcmp(c, "options") == 0 &&
        std::strcmp(m, "set_sync_offset") == 0) {
      set_property(Symbol("sync_offset"),
                   args.size() ? args.at(0) : DataNode::Int(0));
      return DataNode();
    }
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
                                 "synth",        "profilemgr", "song_provider",
                                 "content_mgr",  "options",   "leaderboards",
                                 "world",        "play_sfx",  "stop_sfx",
                                 "song_preview"};
  for (const char* n : kNames)
    mgr.add_singleton(Symbol(n), std::make_unique<StubObject>(Symbol(n), &mgr));
}

}  // namespace ghogx::ui
