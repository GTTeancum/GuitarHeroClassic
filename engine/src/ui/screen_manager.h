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

#include <memory>
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

  // --- singletons (ui/taskmgr/game/...) ----------------------------------
  void add_singleton(Symbol name, std::unique_ptr<Object> obj);
  // Register a NON-owning alias name for an already-owned singleton (gamecfg ->
  // the game object, player0 -> a player config).
  void alias_singleton(Symbol name, Object* obj) { singletons_[name.id()] = obj; }

  // --- handler firing (used by UiObject::handle_property) ----------------
  DataNode run_object_handler(const std::shared_ptr<gh::dtb::Node>& block,
                              Object* self, const DataArray& args);
  void add_function(Symbol name, std::shared_ptr<gh::dtb::Node> block);

  // --- screen navigation (the `ui` messages) -----------------------------
  void goto_screen(Symbol name);
  void push_screen(Symbol name);
  void pop_screen();
  void pop_screen(Symbol next_overlay);
  void go_back();
  void request_backwards_anim();
  Object* current_screen() const { return current_; }
  bool in_transition() const { return transition_.active; }
  struct TransitionSnapshot {
    bool active = false;
    bool back = false;
    float remaining = 0.0f;
    float duration = 0.0f;
    float progress = 1.0f;
    Object* exiting_screen = nullptr;
    Object* entering_screen = nullptr;
  };
  TransitionSnapshot transition_snapshot() const;
  void set_transition_time(float seconds);

  // --- per-frame ---------------------------------------------------------
  void update(float dt) override;
  float ui_seconds() const { return ui_seconds_; }
  void set_locale_string(Symbol token, std::string text);
  void clear_script_tasks();
  void start_animation_task(Object* target, const DataArray& args);

  // Evaluate a top-level boot/statement list (e.g. init.dtb's body: the
  // {foreach ...}{meta set_defaults}{set $first_screen ...}{ui goto_screen ...}).
  DataNode run_script(const gh::dtb::NodeList& roots);

  // --- Object ------------------------------------------------------------
  DataNode handle_property(Symbol msg, const DataArray& args) override;

  // --- script::Host ------------------------------------------------------
  Object* resolve_object(Symbol name) override;
  DataNode get_global(Symbol name) override;
  void set_global(Symbol name, DataNode value) override;
  std::string localize(Symbol token) override;
  void on_unhandled(const std::string& what) override;
  bool handle_command(Symbol name, const DataArray& args,
                      DataNode& out) override;
  bool symbol_exists(Symbol name) override;
  void schedule_script_task(const gh::dtb::NodeList& body, Object* self,
                            float delay_seconds) override;
  const gh::dtb::NodeList* resolve_function(Symbol name) override;

  // Distinct unhandled builtins/messages seen so far -- the fan-out worklist.
  const std::vector<std::string>& unhandled() const { return unhandled_; }

 private:
  // The verified transition protocol (docs/subsystems/menus.md).
  //   exit:  screen_change|screen_back -> exit(screen+panels) -> ui_exit[_back]
  //          -> exit_complete(screen+panels) -> unload(screen+panels)
  //   enter: change_proxies -> load -> finish_load -> ui_enter[_back] -> enter
  void enter_sequence(Object* screen, bool back, bool defer_complete);
  void exit_sequence(Object* screen, bool back, bool defer_unload);
  void finish_transition();
  bool consume_backwards_anim();
  // Send `msg` to the screen and each of its (panels ...), in the given order.
  void send_screen_panels(Object* screen, Symbol msg, bool screen_first);
  // Panel names listed in a screen's (panels ...) property.
  std::vector<Symbol> screen_panels(Object* screen);
  void run_due_script_tasks();
  void poll_active_animations();

  struct ScheduledScriptTask {
    gh::dtb::NodeList body;
    Object* self = nullptr;
    float due_seconds = 0.0f;
  };
  struct ActiveAnimation {
    Object* target = nullptr;
    Symbol task_name;
    float start_frame = 0.0f;
    float end_frame = 0.0f;
    float frames_per_second = 30.0f;
    float start_seconds = 0.0f;
    float duration_seconds = 0.0f;
    bool forever = true;
    bool loop = false;
  };

  script::Interp interp_;
  ObjectDir registry_;
  std::vector<std::unique_ptr<Object>> singletons_owned_;
  std::unordered_map<const void*, Object*> singletons_;
  std::unordered_map<const void*, DataNode> globals_;
  std::unordered_map<const void*, std::shared_ptr<gh::dtb::Node>> functions_;
  std::unordered_map<const void*, std::string> locale_;
  Object* current_ = nullptr;
  std::vector<Object*> stack_;
  std::vector<Object*> history_;
  struct TransitionState {
    bool active = false;
    bool back = false;
    float remaining = 0.0f;
    float duration = 0.0f;
    Object* exiting_screen = nullptr;
    Object* entering_screen = nullptr;
  } transition_;
  bool pending_backwards_anim_ = false;
  float transition_time_seconds_ = 0.5f;
  int scene_state_ = 11;  // scene-state ID (harmonix_symbols.h:904); 11=SPLASH at boot
  float ui_seconds_ = 0.0f;
  std::vector<ScheduledScriptTask> script_tasks_;
  std::vector<ActiveAnimation> active_animations_;
  std::unordered_map<std::string, bool> unhandled_seen_;
  std::vector<std::string> unhandled_;
};

// Install the standard singleton stubs (taskmgr/game/gamecfg/campaign/synth/
// profilemgr/meta/song_provider/content_mgr/helpbar). They answer the handful
// of queries the menu scripts need and log everything else to the worklist.
void install_default_singletons(ScreenManager& mgr);

}  // namespace ghogx::ui
