// engine/src/ui/screen_manager.h
//
// ScreenManager -- the runtime heart of the menu system. It IS the `ui` object
// the scripts talk to ({ui goto_screen X}, {ui push_screen X}, {ui pop_screen})
// AND it implements the interpreter's Host (resolving object names, holding the
// global $variables, locale, and the unhandled-message worklist). It owns the
// registry of every {new ...} screen/panel object and the singleton stubs
// (game/gamecfg/campaign/synth/taskmgr/...), drives the per-frame poll, and
// runs the screen-transition protocol.

#pragma once

#include "core/object.h"
#include "core/object_dir.h"
#include "script/interp.h"

#include "dtb.h"  // gh::dtb::Node

#include <functional>
#include <memory>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace ghogx::ui {

class ScreenManager : public Object, public script::Host {
 public:
  ScreenManager();
  Symbol class_name() const override { return Symbol("UIManager"); }

  // --- the {new ...} object registry -------------------------------------
  void add_object(std::unique_ptr<Object> obj);  // keyed by obj->name()
  Object* find_object(Symbol name);
  ObjectDir& registry() { return registry_; }
  using RuntimeCreator = std::function<std::unique_ptr<Object>(Symbol name)>;
  void register_runtime_class(Symbol cls, RuntimeCreator creator);

  // --- singletons (ui/taskmgr/game/...) ----------------------------------
  void add_singleton(Symbol name, std::unique_ptr<Object> obj);
  // Register a NON-owning alias name for an already-owned singleton (gamecfg ->
  // the game object, player0 -> a player config).
  void alias_singleton(Symbol name, Object* obj) { singletons_[name.id()] = obj; }

  // --- handler firing (used by UiObject::handle_property) ----------------
  DataNode run_object_handler(const std::shared_ptr<gh::dtb::Node>& block,
                              Object* self, const DataArray& args);

  // --- screen navigation (the `ui` messages) -----------------------------
  void goto_screen(Symbol name);
  void push_screen(Symbol name);
  void pop_screen();
  bool go_back();
  void mark_next_goto_back() { next_goto_is_back_ = true; }
  Object* current_screen() const { return current_; }
  bool in_transition() const { return transition_.active; }
  Object* transition_exiting_screen() const { return transition_.exiting_screen; }
  Object* transition_entering_screen() const { return transition_.entering_screen; }
  bool transition_is_back() const { return transition_.back; }
  float transition_remaining() const { return transition_.remaining; }
  float transition_progress() const;
  void set_transition_time(float seconds);
  float transition_time() const { return transition_time_seconds_; }
  void run_global_handler(Symbol name, const DataArray& args = DataArray());
  void note_audio_event(Symbol source, Symbol cue);
  void set_audio_sink(std::function<void(Symbol, Symbol)> sink);
  const std::vector<std::string>& audio_events() const { return audio_events_; }

  // --- per-frame ---------------------------------------------------------
  void update(float dt) override;
  float ui_seconds() const { return ui_seconds_; }

  // Evaluate a top-level boot/statement list (e.g. init.dtb's body: the
  // {foreach ...}{meta set_defaults}{set $first_screen ...}{ui goto_screen ...}).
  DataNode run_script(const gh::dtb::NodeList& roots);

  void set_locale(const std::map<std::string, std::string>& locale);

  // --- Object ------------------------------------------------------------
  DataNode handle_property(Symbol msg, const DataArray& args) override;

  // --- script::Host ------------------------------------------------------
  Object* resolve_object(Symbol name) override;
  DataNode get_global(Symbol name) override;
  void set_global(Symbol name, DataNode value) override;
  std::shared_ptr<gh::dtb::Node> resolve_function(Symbol name) override;
  Object* create_object(Symbol cls, Symbol name) override;
  std::string localize(Symbol token) override;
  void on_unhandled(const std::string& what) override;
  void add_function(Symbol name, std::shared_ptr<gh::dtb::Node> block);

  // Distinct unhandled builtins/messages seen so far -- the fan-out worklist.
  const std::vector<std::string>& unhandled() const { return unhandled_; }

 private:
  // The verified transition protocol (docs/subsystems/menus.md).
  //   exit:  screen_change|screen_back -> exit(screen+panels) -> ui_exit[_back] -> unload
  //   enter: change_proxies -> load -> finish_load -> ui_enter[_back] -> enter
  void enter_sequence(Object* screen, bool back, bool defer_complete);
  void exit_sequence(Object* screen, bool back, bool defer_unload);
  void finish_transition();
  void goto_screen_internal(Symbol name, bool back, bool record_history);
  // Send `msg` to the screen and each of its (panels ...), in the given order.
  void send_screen_panels(Object* screen, Symbol msg, bool screen_first);
  // Panel names listed in a screen's (panels ...) property.
  std::vector<Symbol> screen_panels(Object* screen);

  script::Interp interp_;
  ObjectDir registry_;
  std::vector<std::unique_ptr<Object>> singletons_owned_;
  std::unordered_map<const void*, Object*> singletons_;
  std::unordered_map<const void*, DataNode> globals_;
  std::unordered_map<const void*, std::shared_ptr<gh::dtb::Node>> functions_;
  std::unordered_map<const void*, RuntimeCreator> runtime_creators_;
  std::unordered_map<std::string, std::string> locale_;
  Object* current_ = nullptr;
  std::vector<Object*> stack_;
  std::vector<Object*> history_;
  bool next_goto_is_back_ = false;
  std::function<void(Symbol, Symbol)> audio_sink_;
  struct TransitionState {
    bool active = false;
    bool back = false;
    float remaining = 0.0f;
    Object* exiting_screen = nullptr;
    Object* entering_screen = nullptr;
  } transition_;
  float transition_time_seconds_ = 0.5f;
  int scene_state_ = 11;  // scene-state ID (harmonix_symbols.h:904); 11=SPLASH at boot
  float ui_seconds_ = 0.0f;
  std::unordered_map<std::string, bool> unhandled_seen_;
  std::vector<std::string> unhandled_;
  std::vector<std::string> audio_events_;
};

// Install the standard singleton stubs (taskmgr/game/gamecfg/campaign/synth/
// profilemgr/song_provider/content_mgr/options/leaderboards). Real DTB-authored
// panels such as `meta` and `helpbar` stay in the UI object registry.
void install_default_singletons(ScreenManager& mgr);

}  // namespace ghogx::ui
