// engine/src/ui/ui_test.cpp
//
// Headless boot test for the UI object layer + screen manager, run against the
// REAL stock GH2 DTBs from the PS2 ARK. Loads ui/gen/{main,splash,quickplay}.dtb,
// asserts the {new GHPanel/GHScreen ...} objects + their parsed handler blocks
// exist, then drives the manager: goto_screen(main_screen) runs the authored
// (enter)/reset_player_settings scripts, and a simulated SELECT_START on
// main_quickspin.btn runs the real SELECT_START_MSG switch -> {ui goto_screen
// qp_selsong_screen}. Skips (exit 0) when the ARK is absent.

#include "core/data_node.h"
#include "core/object_dir.h"
#include "core/symbol.h"
#include "ui/config_db.h"
#include "ui/meta_objects.h"
#include "ui/screen_loader.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include "ark_v3.h"

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace ghogx;
namespace fs = std::filesystem;

static int g_failures = 0;
#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      std::fprintf(stderr, "FAIL %s:%d  CHECK(%s)\n", __FILE__, __LINE__, \
                   #cond);                                                \
      ++g_failures;                                                       \
    }                                                                     \
  } while (0)

static std::string first_existing(const std::string& dir, std::vector<std::string> names) {
  for (auto& n : names) {
    std::string p = dir + "/" + n;
    if (fs::exists(p)) return p;
  }
  return {};
}

static bool array_contains_symbol(const DataNode& node, Symbol needle) {
  if (auto s = node.as_symbol()) return *s == needle;
  if (auto text = node.as_string()) return *text == needle.c_str();
  if (auto arr = node.as_array()) {
    for (std::size_t i = 0; i < arr->size(); ++i)
      if (array_contains_symbol(arr->at(i), needle)) return true;
  }
  return false;
}

static bool truthy(const DataNode& node) {
  if (auto i = node.as_int()) return *i != 0;
  if (auto f = node.as_float()) return *f != 0.0f;
  if (auto s = node.as_symbol())
    return !(*s == Symbol("FALSE") || *s == Symbol("false") ||
             *s == Symbol("0"));
  if (auto text = node.as_string())
    return !(*text == "FALSE" || *text == "false" || *text == "0" ||
             text->empty());
  return node.as_object() != nullptr;
}

static bool near(float a, float b, float eps = 0.0001f) {
  return a > b - eps && a < b + eps;
}

class RecordingObject : public Object {
 public:
  explicit RecordingObject(std::vector<std::string>* log) : log_(log) {}
  Symbol class_name() const override { return Symbol("RecordingObject"); }
  DataNode handle_property(Symbol msg, const DataArray& args) override {
    (void)args;
    if (log_) {
      log_->push_back(std::string(name().c_str()) + ":" +
                      std::string(msg.c_str()));
    }
    return DataNode();
  }

 private:
  std::vector<std::string>* log_;
};

class RecordingUiObject : public ui::UiObject {
 public:
  RecordingUiObject(Symbol cls, std::vector<std::string>* log)
      : ui::UiObject(cls), log_(log) {}
  DataNode handle_property(Symbol msg, const DataArray& args) override {
    if (log_) {
      log_->push_back(std::string(name().c_str()) + ":" +
                      std::string(msg.c_str()));
    }
    return ui::UiObject::handle_property(msg, args);
  }

 private:
  std::vector<std::string>* log_;
};

static void add_recording_screen(ui::ScreenManager& mgr,
                                 std::vector<std::string>* log,
                                 const char* screen_name,
                                 const char* panel_name) {
  auto panel = std::make_unique<RecordingObject>(log);
  panel->set_name(Symbol(panel_name));
  mgr.add_object(std::move(panel));

  auto panels = std::make_shared<DataArray>();
  panels->push(DataNode::Sym(Symbol(panel_name)));
  auto screen = std::make_unique<RecordingObject>(log);
  screen->set_name(Symbol(screen_name));
  screen->set_property(Symbol("panels"), DataNode::Array(panels));
  mgr.add_object(std::move(screen));
}

static void add_recording_ui_screen(ui::ScreenManager& mgr,
                                    std::vector<std::string>* log,
                                    const char* screen_name,
                                    const char* panel_name) {
  auto panel = std::make_unique<RecordingUiObject>(Symbol("GHPanel"), log);
  panel->set_name(Symbol(panel_name));
  panel->set_manager(&mgr);
  mgr.add_object(std::move(panel));

  auto panels = std::make_shared<DataArray>();
  panels->push(DataNode::Sym(Symbol(panel_name)));
  auto screen = std::make_unique<RecordingUiObject>(Symbol("GHScreen"), log);
  screen->set_name(Symbol(screen_name));
  screen->set_manager(&mgr);
  screen->set_property(Symbol("panels"), DataNode::Array(panels));
  mgr.add_object(std::move(screen));
}

static int log_index(const std::vector<std::string>& log,
                     const char* needle) {
  for (std::size_t i = 0; i < log.size(); ++i)
    if (log[i] == needle) return static_cast<int>(i);
  return -1;
}

static void check_transition_lifecycle_smoke() {
  ui::ScreenManager mgr;
  std::vector<std::string> transition_log;
  add_recording_screen(mgr, &transition_log, "trace_old_screen",
                       "trace_old_panel");
  add_recording_screen(mgr, &transition_log, "trace_new_screen",
                       "trace_new_panel");
  mgr.set_transition_time(0.5f);
  mgr.goto_screen(Symbol("trace_old_screen"));
  transition_log.clear();
  mgr.goto_screen(Symbol("trace_new_screen"));
  const int ui_exit_i =
      log_index(transition_log, "trace_old_screen:ui_exit");
  CHECK(ui_exit_i >= 0);
  CHECK(mgr.in_transition());
  auto snapshot = mgr.transition_snapshot();
  CHECK(snapshot.active);
  CHECK(!snapshot.back);
  CHECK(snapshot.exiting_screen != nullptr &&
        snapshot.exiting_screen->name() == Symbol("trace_old_screen"));
  CHECK(snapshot.entering_screen != nullptr &&
        snapshot.entering_screen->name() == Symbol("trace_new_screen"));
  CHECK(near(snapshot.duration, 0.5f));
  CHECK(near(snapshot.remaining, 0.5f));
  CHECK(near(snapshot.progress, 0.0f));
  CHECK(log_index(transition_log, "trace_old_screen:exit_complete") < 0);
  CHECK(log_index(transition_log, "trace_old_screen:unload") < 0);
  CHECK(log_index(transition_log, "trace_new_screen:TRANSITION_COMPLETE_MSG") < 0);
  mgr.update(0.25f);
  CHECK(mgr.in_transition());
  snapshot = mgr.transition_snapshot();
  CHECK(snapshot.active);
  CHECK(near(snapshot.remaining, 0.25f));
  CHECK(near(snapshot.progress, 0.5f));
  CHECK(log_index(transition_log, "trace_old_screen:exit_complete") < 0);
  mgr.update(0.25f);
  CHECK(!mgr.in_transition());
  CHECK(!mgr.transition_snapshot().active);
  const int exit_complete_i =
      log_index(transition_log, "trace_old_screen:exit_complete");
  const int unload_i =
      log_index(transition_log, "trace_old_screen:unload");
  const int panel_exit_complete_i =
      log_index(transition_log, "trace_old_panel:exit_complete");
  const int panel_unload_i =
      log_index(transition_log, "trace_old_panel:unload");
  const int transition_complete_i =
      log_index(transition_log, "trace_new_screen:TRANSITION_COMPLETE_MSG");
  CHECK(transition_complete_i > ui_exit_i);
  CHECK(exit_complete_i > ui_exit_i);
  CHECK(unload_i > exit_complete_i);
  CHECK(panel_exit_complete_i >= 0);
  CHECK(panel_unload_i > panel_exit_complete_i);
}

static void check_backwards_anim_routes_goto_as_back_smoke() {
  ui::ScreenManager mgr;
  std::vector<std::string> transition_log;
  add_recording_ui_screen(mgr, &transition_log, "back_old_screen",
                          "back_old_panel");
  add_recording_ui_screen(mgr, &transition_log, "back_new_screen",
                          "back_new_panel");
  mgr.set_transition_time(0.0f);
  mgr.goto_screen(Symbol("back_old_screen"));
  Object* old_screen = mgr.current_screen();
  CHECK(old_screen != nullptr);
  transition_log.clear();

  if (old_screen)
    old_screen->handle_property(Symbol("backwards_anim"), DataArray());
  CHECK(old_screen == nullptr ||
        old_screen->get_property(Symbol("last_transition_anim"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("back"));
  mgr.goto_screen(Symbol("back_new_screen"));

  CHECK(log_index(transition_log, "back_old_screen:ui_exit_back") >= 0);
  CHECK(log_index(transition_log, "back_new_screen:ui_enter_back") >= 0);
  CHECK(log_index(transition_log, "back_old_screen:ui_exit") < 0);
  CHECK(log_index(transition_log, "back_new_screen:ui_enter") < 0);

  Object* new_screen = mgr.current_screen();
  CHECK(new_screen != nullptr);
  if (new_screen)
    mgr.goto_screen(Symbol("back_old_screen"));
  transition_log.clear();
  mgr.goto_screen(Symbol("back_new_screen"));
  CHECK(log_index(transition_log, "back_old_screen:ui_exit") >= 0);
  CHECK(log_index(transition_log, "back_new_screen:ui_enter") >= 0);
  CHECK(log_index(transition_log, "back_old_screen:ui_exit_back") < 0);
  CHECK(log_index(transition_log, "back_new_screen:ui_enter_back") < 0);
}

static void check_menu_audio_surface_smoke() {
  ui::ScreenManager mgr;
  ui::install_default_singletons(mgr);

  Object* synth = mgr.resolve_object(Symbol("synth"));
  Object* world = mgr.resolve_object(Symbol("world"));
  Object* sync_click = mgr.resolve_object(Symbol("sync_click.cue"));
  CHECK(synth != nullptr);
  CHECK(world != nullptr);
  CHECK(sync_click != nullptr);

  if (synth) {
    DataArray pause_args;
    pause_args.push(DataNode::Sym(Symbol("TRUE")));
    synth->handle_property(Symbol("pause_all_sfx"), pause_args);
    CHECK(synth->get_property(Symbol("last_control"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("pause_all_sfx"));
    CHECK(synth->get_property(Symbol("paused")).as_int().value_or(0) == 1);
    CHECK(synth->get_property(Symbol("pause_all_count")).as_int().value_or(0) == 1);

    synth->handle_property(Symbol("stop_all_sfx"), DataArray());
    CHECK(synth->get_property(Symbol("last_control"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("stop_all_sfx"));
    CHECK(synth->get_property(Symbol("stop_all_count")).as_int().value_or(0) == 1);
  }

  if (world) {
    DataArray sfx_args;
    sfx_args.push(DataNode::Sym(Symbol("vroom.cue")));
    world->handle_property(Symbol("play_sfx"), sfx_args);
    CHECK(world->get_property(Symbol("last_played"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("vroom.cue"));
    CHECK(world->get_property(Symbol("play_count")).as_int().value_or(0) == 1);

    DataArray meta_args;
    meta_args.push(DataNode::Sym(Symbol("encore_yes")));
    world->handle_property(Symbol("play_meta_sfx"), meta_args);
    CHECK(world->get_property(Symbol("last_meta_played"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("encore_yes"));
    CHECK(world->get_property(Symbol("meta_play_count")).as_int().value_or(0) == 1);
  }

  if (sync_click) {
    sync_click->handle_property(Symbol("play"), DataArray());
    CHECK(sync_click->get_property(Symbol("last_control"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("play"));
    CHECK(sync_click->get_property(Symbol("play_count")).as_int().value_or(0) == 1);
  }
}

static void check_focus_panel_surface_smoke() {
  ui::register_ui_classes();
  ui::ScreenManager mgr;
  ui::install_default_singletons(mgr);

  auto panel_a = std::make_unique<ui::UiObject>(Symbol("GHPanel"));
  panel_a->set_name(Symbol("focus_panel_a"));
  mgr.add_object(std::move(panel_a));

  auto panel_b = std::make_unique<ui::UiObject>(Symbol("GHPanel"));
  panel_b->set_name(Symbol("focus_panel_b"));
  mgr.add_object(std::move(panel_b));

  auto panels = std::make_shared<DataArray>();
  panels->push(DataNode::Sym(Symbol("focus_panel_a")));
  panels->push(DataNode::Sym(Symbol("focus_panel_b")));

  auto screen = std::make_unique<ui::UiObject>(Symbol("GHScreen"));
  screen->set_name(Symbol("focus_surface_screen"));
  screen->set_property(Symbol("panels"), DataNode::Array(panels));
  screen->set_property(Symbol("focus"), DataNode::Sym(Symbol("focus_panel_a")));
  mgr.add_object(std::move(screen));

  mgr.goto_screen(Symbol("focus_surface_screen"));
  Object* current = mgr.current_screen();
  CHECK(current != nullptr);

  DataNode focused = mgr.handle_property(Symbol("focus_panel"), DataArray());
  CHECK(focused.as_object() == mgr.find_object(Symbol("focus_panel_a")));
  CHECK(script::node_equal(focused, DataNode::Sym(Symbol("focus_panel_a"))));

  DataArray focus_panel_arg;
  focus_panel_arg.push(DataNode::Sym(Symbol("focus_panel_b")));
  current->handle_property(Symbol("set_focus_panel"), focus_panel_arg);
  DataNode focused_b = mgr.handle_property(Symbol("focus_panel"), DataArray());
  CHECK(focused_b.as_object() == mgr.find_object(Symbol("focus_panel_b")));
  CHECK(script::node_equal(focused_b, DataNode::Sym(Symbol("focus_panel_b"))));
}

static void check_bad_select_surface_smoke() {
  ui::ScreenManager mgr;
  ui::install_default_singletons(mgr);

  std::vector<std::string> bad_select_log;
  add_recording_screen(mgr, &bad_select_log, "bad_select_screen",
                       "bad_select_panel");
  Object* screen = mgr.find_object(Symbol("bad_select_screen"));
  CHECK(screen != nullptr);
  if (!screen) return;

  screen->set_property(Symbol("focus"), DataNode::Sym(Symbol("bad_select_panel")));
  mgr.goto_screen(Symbol("bad_select_screen"));
  bad_select_log.clear();
  mgr.handle_property(Symbol("BAD_SELECT_START_MSG"), DataArray());
  CHECK(log_index(bad_select_log, "bad_select_panel:BAD_SELECT_MSG") >= 0);
  CHECK(log_index(bad_select_log, "bad_select_screen:BAD_SELECT_MSG") >= 0);

  Object* play_sfx = mgr.resolve_object(Symbol("play_sfx"));
  CHECK(play_sfx != nullptr);
  if (!play_sfx) return;
  CHECK(play_sfx->get_property(Symbol("last_played"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("button_error"));
  const int play_count =
      play_sfx->get_property(Symbol("play_count")).as_int().value_or(-1);
  CHECK(play_count == 1);

  auto nameprof_panel = std::make_unique<RecordingObject>(&bad_select_log);
  nameprof_panel->set_name(Symbol("nameprof_panel"));
  mgr.add_object(std::move(nameprof_panel));
  screen->set_property(Symbol("focus"), DataNode::Sym(Symbol("nameprof_panel")));
  bad_select_log.clear();
  mgr.handle_property(Symbol("BAD_SELECT_START_MSG"), DataArray());
  CHECK(log_index(bad_select_log, "nameprof_panel:BAD_SELECT_MSG") >= 0);
  CHECK(log_index(bad_select_log, "bad_select_screen:BAD_SELECT_MSG") >= 0);
  CHECK(play_sfx->get_property(Symbol("play_count")).as_int().value_or(-1) ==
        play_count);
}

static void check_shared_menu_sfx_surface_smoke() {
  ui::ScreenManager mgr;
  ui::install_default_singletons(mgr);
  mgr.set_transition_time(0.0f);

  std::vector<std::string> sfx_log;
  add_recording_screen(mgr, &sfx_log, "sfx_main_screen", "sfx_panel");
  add_recording_screen(mgr, &sfx_log, "sel_song_screen", "sfx_song_panel");
  add_recording_screen(mgr, &sfx_log, "sel_character_new_screen",
                       "sfx_character_panel");

  auto credits_panel = std::make_unique<RecordingObject>(&sfx_log);
  credits_panel->set_name(Symbol("credits_panel"));
  mgr.add_object(std::move(credits_panel));

  Object* main_screen = mgr.find_object(Symbol("sfx_main_screen"));
  Object* song_screen = mgr.find_object(Symbol("sel_song_screen"));
  Object* character_screen = mgr.find_object(Symbol("sel_character_new_screen"));
  Object* synth = mgr.resolve_object(Symbol("synth"));
  CHECK(main_screen != nullptr);
  CHECK(song_screen != nullptr);
  CHECK(character_screen != nullptr);
  CHECK(synth != nullptr);
  if (!main_screen || !song_screen || !character_screen || !synth) return;

  main_screen->set_property(Symbol("focus"), DataNode::Sym(Symbol("sfx_panel")));
  song_screen->set_property(Symbol("focus"),
                            DataNode::Sym(Symbol("sfx_song_panel")));
  character_screen->set_property(Symbol("focus"),
                                 DataNode::Sym(Symbol("sfx_character_panel")));

  mgr.goto_screen(Symbol("sfx_main_screen"));
  int count = synth->get_property(Symbol("sequence_count")).as_int().value_or(0);

  mgr.set_global(Symbol("component"),
                 DataNode::Sym(Symbol("main_quickspin.btn")));
  mgr.handle_property(Symbol("SELECT_START_MSG"), DataArray());
  CHECK(synth->get_property(Symbol("last_sequence"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("button_select"));
  CHECK(synth->get_property(Symbol("sequence_count")).as_int().value_or(0) ==
        ++count);

  mgr.set_global(Symbol("component"),
                 DataNode::Sym(Symbol("pause_restart.btn")));
  mgr.handle_property(Symbol("SELECT_START_MSG"), DataArray());
  CHECK(synth->get_property(Symbol("last_sequence"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("button_play"));
  CHECK(synth->get_property(Symbol("sequence_count")).as_int().value_or(0) ==
        ++count);

  mgr.handle_property(Symbol("FOCUS_MSG"), DataArray());
  CHECK(synth->get_property(Symbol("last_sequence"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("button_toggle"));
  CHECK(synth->get_property(Symbol("sequence_count")).as_int().value_or(0) ==
        ++count);

  main_screen->set_property(Symbol("focus"),
                            DataNode::Sym(Symbol("credits_panel")));
  mgr.handle_property(Symbol("SCROLL_MSG"), DataArray());
  CHECK(synth->get_property(Symbol("sequence_count")).as_int().value_or(0) ==
        count);

  main_screen->set_property(Symbol("focus"), DataNode::Sym(Symbol("sfx_panel")));
  mgr.handle_property(Symbol("SCROLL_MSG"), DataArray());
  CHECK(synth->get_property(Symbol("last_sequence"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("button_toggle"));
  CHECK(synth->get_property(Symbol("sequence_count")).as_int().value_or(0) ==
        ++count);

  mgr.handle_property(Symbol("SCREEN_BACK_MSG"), DataArray());
  CHECK(synth->get_property(Symbol("last_sequence"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("button_back.cue"));
  CHECK(synth->get_property(Symbol("sequence_count")).as_int().value_or(0) ==
        ++count);

  mgr.goto_screen(Symbol("sel_song_screen"));
  mgr.set_global(Symbol("component"), DataNode::Sym(Symbol("song.btn")));
  mgr.handle_property(Symbol("SELECT_START_MSG"), DataArray());
  CHECK(synth->get_property(Symbol("last_sequence"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("button_play"));
  CHECK(synth->get_property(Symbol("sequence_count")).as_int().value_or(0) ==
        ++count);

  mgr.goto_screen(Symbol("sel_character_new_screen"));
  mgr.handle_property(Symbol("FOCUS_MSG"), DataArray());
  CHECK(synth->get_property(Symbol("sequence_count")).as_int().value_or(0) ==
        count);
}

static void check_named_pop_screen_surface_smoke() {
  ui::ScreenManager mgr;
  std::vector<std::string> pop_log;
  add_recording_screen(mgr, &pop_log, "pop_base_screen", "pop_base_panel");
  add_recording_screen(mgr, &pop_log, "pop_pause_screen", "pop_pause_panel");
  add_recording_screen(mgr, &pop_log, "pop_confirm_screen",
                       "pop_confirm_panel");

  mgr.goto_screen(Symbol("pop_base_screen"));
  mgr.push_screen(Symbol("pop_pause_screen"));
  CHECK(mgr.current_screen() != nullptr &&
        mgr.current_screen()->name() == Symbol("pop_pause_screen"));

  DataArray named_pop;
  named_pop.push(DataNode::Sym(Symbol("pop_confirm_screen")));
  pop_log.clear();
  mgr.handle_property(Symbol("pop_screen"), named_pop);
  CHECK(mgr.current_screen() != nullptr &&
        mgr.current_screen()->name() == Symbol("pop_confirm_screen"));
  CHECK(log_index(pop_log, "pop_pause_screen:ui_exit_back") >= 0);
  CHECK(log_index(pop_log, "pop_pause_screen:unload") >= 0);
  CHECK(log_index(pop_log, "pop_confirm_screen:ui_enter") >= 0);

  mgr.pop_screen();
  CHECK(mgr.current_screen() != nullptr &&
        mgr.current_screen()->name() == Symbol("pop_base_screen"));
}

static void check_pause_task_surface_smoke() {
  ui::register_ui_classes();
  ui::ScreenManager mgr;
  ui::install_default_singletons(mgr);

  Object* beatmatch = mgr.resolve_object(Symbol("beatmatch"));
  Object* taskmgr = mgr.resolve_object(Symbol("taskmgr"));
  CHECK(beatmatch != nullptr);
  CHECK(taskmgr != nullptr);
  if (beatmatch) {
    DataArray pause_true;
    pause_true.push(DataNode::Sym(Symbol("TRUE")));
    beatmatch->handle_property(Symbol("set_paused"), pause_true);
    CHECK(beatmatch->handle_property(Symbol("paused"), DataArray())
              .as_int()
              .value_or(0) == 1);

    DataArray volume;
    volume.push(DataNode::Sym(Symbol("kDbSilence")));
    beatmatch->handle_property(Symbol("set_volume"), volume);
    CHECK(beatmatch->get_property(Symbol("volume"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("kDbSilence"));

    DataArray speed;
    speed.push(DataNode::Float(0.5f));
    beatmatch->handle_property(Symbol("set_music_speed"), speed);
    CHECK(near(beatmatch->get_property(Symbol("music_speed"))
                   .as_float()
                   .value_or(0.0f),
               0.5f));
  }

  if (taskmgr) {
    const float seconds =
        taskmgr->handle_property(Symbol("seconds"), DataArray())
            .as_float()
            .value_or(-1.0f);
    CHECK(seconds >= 0.0f);
    taskmgr->handle_property(Symbol("clear_tasks"), DataArray());
    CHECK(taskmgr->get_property(Symbol("clear_tasks_count"))
              .as_int()
              .value_or(0) == 1);
  }

  auto panel = std::make_unique<ui::UiObject>(Symbol("HudPanel"));
  panel->set_name(Symbol("pause_probe_panel"));
  panel->set_manager(&mgr);
  DataArray panel_pause_true;
  panel_pause_true.push(DataNode::Sym(Symbol("TRUE")));
  panel->handle_property(Symbol("set_paused"), panel_pause_true);
  CHECK(panel->handle_property(Symbol("paused"), DataArray()).as_int().value_or(0) == 1);
  DataArray panel_pause_false;
  panel_pause_false.push(DataNode::Sym(Symbol("FALSE")));
  panel->handle_property(Symbol("set_paused"), panel_pause_false);
  CHECK(panel->handle_property(Symbol("paused"), DataArray()).as_int().value_or(1) == 0);
}

static void check_named_animation_exists_surface_smoke() {
  ui::register_ui_classes();
  ui::ScreenManager mgr;
  auto anim = std::make_unique<ui::UiObject>(Symbol("Group"));
  anim->set_name(Symbol("unlock_anim.grp"));
  anim->set_manager(&mgr);
  Object* anim_obj = anim.get();
  mgr.add_object(std::move(anim));

  auto row = [](Symbol key, DataNode a, DataNode b = DataNode()) {
    auto arr = std::make_shared<DataArray>();
    arr->push(DataNode::Sym(key));
    arr->push(a);
    if (!b.empty()) arr->push(b);
    return DataNode::Array(arr);
  };
  DataArray animate_args;
  animate_args.push(row(Symbol("name"), DataNode::Sym(Symbol("unlock_anim"))));
  animate_args.push(row(Symbol("period"), DataNode::Float(3.0f)));
  animate_args.push(row(Symbol("range"), DataNode::Float(0.0f),
                        DataNode::Float(200.0f)));

  anim_obj->handle_property(Symbol("animate"), animate_args);
  CHECK(mgr.symbol_exists(Symbol("unlock_anim")));
  CHECK(near(anim_obj->handle_property(Symbol("frame"), DataArray())
                 .as_float()
                 .value_or(-1.0f),
             0.0f));
  mgr.update(1.5f);
  CHECK(mgr.symbol_exists(Symbol("unlock_anim")));
  CHECK(near(anim_obj->handle_property(Symbol("frame"), DataArray())
                 .as_float()
                 .value_or(-1.0f),
             100.0f));
  mgr.update(1.6f);
  CHECK(!mgr.symbol_exists(Symbol("unlock_anim")));
  CHECK(near(anim_obj->handle_property(Symbol("frame"), DataArray())
                 .as_float()
                 .value_or(-1.0f),
             200.0f));
  CHECK(anim_obj->get_property(Symbol("finished_anim_task"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("unlock_anim"));
}

static void check_tutorial_panel_surface_smoke() {
  ui::register_ui_classes();
  ui::ScreenManager mgr;
  ui::install_default_singletons(mgr);

  auto owned_panel = std::make_unique<ui::UiObject>(Symbol("TutorialPanel"));
  ui::UiObject* panel = owned_panel.get();
  panel->set_name(Symbol("tutorial"));
  panel->set_manager(&mgr);
  mgr.add_object(std::move(owned_panel));

  panel->set_property(Symbol("lesson"), DataNode::Sym(Symbol("intro")));
  DataArray next_one;
  next_one.push(DataNode::Int(1));
  CHECK(panel->handle_property(Symbol("get_next_state"), next_one)
            .as_symbol()
            .value_or(Symbol()) == Symbol("playing_notes"));

  DataArray prev_one;
  prev_one.push(DataNode::Int(-1));
  CHECK(panel->handle_property(Symbol("get_next_state"), prev_one)
            .as_symbol()
            .value_or(Symbol()) == Symbol("pulloff"));

  DataArray set_diff;
  set_diff.push(DataNode::Sym(Symbol("diff_notes")));
  panel->handle_property(Symbol("set_state"), set_diff);
  CHECK(panel->get_property(Symbol("tutorial_state"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("diff_notes"));
  CHECK(panel->get_property(Symbol("lesson"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("diff_notes"));
  CHECK(panel->get_property(Symbol("state"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("start_lesson"));
  CHECK(panel->get_property(Symbol("disabled"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("FALSE"));
  CHECK(panel->handle_property(Symbol("get_next_state"), next_one)
            .as_symbol()
            .value_or(Symbol()) == Symbol("held_notes"));

  DataArray play_vo;
  play_vo.push(DataNode::Sym(Symbol("intro_1")));
  panel->handle_property(Symbol("play_vo"), play_vo);
  CHECK(panel->get_property(Symbol("last_vo"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("intro_1"));
  CHECK(panel->get_property(Symbol("vo_play_count")).as_int().value_or(0) == 1);
  CHECK(panel->handle_property(Symbol("is_vo_playing"), DataArray())
            .as_int()
            .value_or(1) == 0);

  panel->handle_property(Symbol("reset_vo"), DataArray());
  CHECK(panel->get_property(Symbol("last_vo")).empty());
  CHECK(panel->get_property(Symbol("vo_reset_count")).as_int().value_or(0) == 1);

  CHECK(panel->handle_property(Symbol("is_missing_guitar"), DataArray())
            .as_symbol()
            .value_or(Symbol()) == Symbol("FALSE"));
  if (Object* game = mgr.resolve_object(Symbol("game"))) {
    DataArray missing_true;
    missing_true.push(DataNode::Sym(Symbol("TRUE")));
    game->handle_property(Symbol("set_missing_controller"), missing_true);
    CHECK(panel->handle_property(Symbol("is_missing_guitar"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("TRUE"));
  }
}

int main(int argc, char** argv) {
  std::string ark_dir =
      argc > 1 ? argv[1]
               : "C:/Programming/GitHub/Guitar Hero II/Guitar Hero II PS2 (USA)/GEN";
  std::string hdr = first_existing(ark_dir, {"MAIN.HDR", "main.hdr"});
  std::string ark0 = first_existing(ark_dir, {"MAIN_0.ARK", "main_0.ark"});
  check_transition_lifecycle_smoke();
  check_backwards_anim_routes_goto_as_back_smoke();
  check_menu_audio_surface_smoke();
  check_focus_panel_surface_smoke();
  check_bad_select_surface_smoke();
  check_shared_menu_sfx_surface_smoke();
  check_named_pop_screen_surface_smoke();
  check_pause_task_surface_smoke();
  check_named_animation_exists_surface_smoke();
  check_tutorial_panel_surface_smoke();
  if (hdr.empty() || ark0.empty()) {
    std::printf("ghogx_ui_test: SKIP (no stock ARK at %s)\n", ark_dir.c_str());
    return 0;
  }

  ui::register_ui_classes();
  ui::ScreenManager mgr;
  ui::install_default_singletons(mgr);

  gh::ark::ArkV3Reader ark = gh::ark::ArkV3Reader::load(hdr);
  std::vector<std::string> arks = {ark0};

  // Load the FULL stock screen set verbatim (every ui/gen/*.dtb).
  int n = ui::load_all_ui_screens(ark, arks, mgr);
  int widget_n = ui::load_panel_milo_widgets(ark, arks, mgr);
  std::printf("ghogx_ui_test: loaded %d ui/gen DTBs, %d panel MILO widgets, "
              "%zu objects registered\n",
              n, widget_n, mgr.registry().size());
  CHECK(n >= 35);                      // ~40 ui/gen DTBs
  CHECK(widget_n > 100);
  CHECK(mgr.registry().size() > 100);  // ~180 screen/panel objects
  for (const char* s : {"main_screen", "main_panel", "qp_selsong_screen", "options_screen"})
    CHECK(mgr.find_object(Symbol(s)) != nullptr);
  if (auto* audio = dynamic_cast<ui::UiObject*>(
          mgr.find_object(Symbol("audio_settings_panel")))) {
    CHECK(audio->has_handler(Symbol("FOCUS_MSG")));
    CHECK(audio->get_property(Symbol("focus")).as_symbol().value_or(Symbol()) ==
          Symbol("gs_band.sld"));
  } else {
    CHECK(false);
  }

  // Game-side data layer: config/gen-DTB-backed objects (no canned constants).
  ui::ConfigDb db;
  db.load(ark, arks);
  ui::install_meta_singletons(mgr, db);
  CHECK(db.song_count() > 40);  // GH2 songs.dtb has ~74 songs
  CHECK(db.first_guitar() == Symbol("lespaul"));
  CHECK(db.first_guitar_skin(Symbol("lespaul")) == Symbol("lp_cherry"));
  CHECK(db.guitar_skin_count(Symbol("lespaul")) == 4);
  const DataArray* credits = db.table(Symbol("credits"));
  CHECK(credits != nullptr);
  if (credits) {
    CHECK(credits->size() > 1000);
    CHECK(credits->at(0).as_array() && credits->at(0).as_array()->empty());
    CHECK(credits->at(16).as_array());
    if (auto first_title = credits->at(16).as_array()) {
      CHECK(first_title->size() == 1);
      CHECK(first_title->at(0).as_string().value_or("") == "GUITAR HERO II");
    }
  }
  Object* credits_screen_obj = mgr.find_object(Symbol("credits_screen"));
  CHECK(credits_screen_obj != nullptr);
  if (credits && credits_screen_obj) {
    CHECK(credits_screen_obj->handle_property(Symbol("num_lines"), DataArray())
              .as_int()
              .value_or(0) == static_cast<int>(credits->size()));
  }
  const DataArray* tips = db.table(Symbol("tips"));
  CHECK(tips != nullptr);
  if (tips) {
    auto general_tips = tips->find_keyed(Symbol("tips_general"));
    CHECK(general_tips != nullptr);
    CHECK(general_tips && general_tips->size() > 1);
  }
  CHECK(db.venue_count() == 8);
  CHECK(db.venue_index(Symbol("battle")) == 0);
  CHECK(db.venue_index(Symbol("small2")) == 2);
  CHECK(db.default_venue() == Symbol("small2"));
  const std::string mc_checking = mgr.localize(Symbol("mc_checking"));
  CHECK(mc_checking != "mc_checking");
  CHECK(mc_checking.find("memory card") != std::string::npos);

  gh::dtb::NodeList init_roots =
      ui::load_ui_script_roots_from_ark(ark, arks, "ui/gen/init.dtb");
  CHECK(init_roots.size() == 6);
  mgr.run_script(init_roots);
  CHECK(mgr.get_global(Symbol("first_screen"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("bootup_load"));
  Object* init_screen = mgr.current_screen();
  CHECK(init_screen != nullptr);
  CHECK(init_screen == nullptr ||
        init_screen->name() == Symbol("bootup_load"));

  if (Object* game = mgr.resolve_object(Symbol("game"))) {
    game->set_property(Symbol("song_index"), DataNode::Int(0));
    DataNode title = game->handle_property(Symbol("get_song_text"), DataArray());
    std::printf("ghogx_ui_test: songs=%zu  song[0]=\"%s\"\n", db.song_count(),
                std::string(title.as_string().value_or("")).c_str());
    CHECK(title.as_string().has_value() && !title.as_string()->empty());
    CHECK(game->handle_property(Symbol("get_guitar"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("lespaul"));
    CHECK(game->handle_property(Symbol("get_guitar_skin"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("lp_cherry"));
    CHECK(game->handle_property(Symbol("get_guitar_desc"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("lespaul_desc"));
    CHECK(game->handle_property(Symbol("get_guitar_skin_desc"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("lp_cherry_desc"));
    DataArray guitar_arg;
    guitar_arg.push(DataNode::Sym(Symbol("lespaul")));
    CHECK(game->handle_property(Symbol("get_num_skins"), guitar_arg)
              .as_int()
              .value_or(-1) == 4);
    CHECK(game->handle_property(Symbol("get_controller"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("guitar"));
    CHECK(game->handle_property(Symbol("get_venue"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("small2"));
    CHECK(game->handle_property(Symbol("get_venue_index"), DataArray())
              .as_int()
              .value_or(-1) == 2);
    DataArray venue_arg;
    venue_arg.push(DataNode::Sym(Symbol("battle")));
    game->handle_property(Symbol("set_venue"), venue_arg);
    CHECK(game->handle_property(Symbol("get_venue"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("battle"));
    Object* bank_loader =
        game->handle_property(Symbol("get_bank_loader"), DataArray()).as_object();
    CHECK(bank_loader == game);
    game->handle_property(Symbol("reset"), DataArray());
    DataArray p0_arg;
    p0_arg.push(DataNode::Int(0));
    CHECK(game->handle_property(Symbol("get_difficulty"), p0_arg)
              .as_symbol()
              .value_or(Symbol()) == Symbol("kDifficultyMedium"));
    DataArray hard_arg;
    hard_arg.push(DataNode::Sym(Symbol("kDifficultyHard")));
    game->handle_property(Symbol("set_difficulty"), hard_arg);
    CHECK(game->handle_property(Symbol("get_difficulty_sym"), p0_arg)
              .as_symbol()
              .value_or(Symbol()) == Symbol("kDifficultyHard"));
    if (Object* player0 = mgr.resolve_object(Symbol("player0"))) {
      CHECK(player0->get_property(Symbol("difficulty"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("kDifficultyHard"));
      DataArray score_arg;
      score_arg.push(DataNode::Int(123456));
      player0->handle_property(Symbol("set_score"), score_arg);
      CHECK(player0->handle_property(Symbol("score"), DataArray())
                .as_int()
                .value_or(-1) == 123456);
      CHECK(player0->handle_property(Symbol("percent_hit"), DataArray())
                .as_int()
                .value_or(-1) == 0);
      CHECK(player0->handle_property(Symbol("player_matcher"), DataArray())
                .as_object() == player0);
      player0->handle_property(Symbol("fill_star_power"), DataArray());
      CHECK(player0->handle_property(Symbol("star_power_ready"), DataArray())
                .as_int()
                .value_or(0) == 1);
      player0->handle_property(Symbol("empty_star_power"), DataArray());
      CHECK(player0->handle_property(Symbol("star_power_ready"), DataArray())
                .as_int()
                .value_or(1) == 0);
    }
    DataArray medium_arg;
    medium_arg.push(DataNode::Sym(Symbol("kDifficultyMedium")));
    game->handle_property(Symbol("set_difficulty"), medium_arg);
    game->handle_property(Symbol("set_quickplay"), DataArray());
    CHECK(game->get_property(Symbol("mode"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("quickplay"));
    DataArray tutorial_on;
    tutorial_on.push(DataNode::Sym(Symbol("TRUE")));
    game->handle_property(Symbol("set_tutorial_running"), tutorial_on);
    CHECK(game->handle_property(Symbol("is_tutorial_running"), DataArray())
              .as_int()
              .value_or(0) == 1);
    DataArray tutorial_off;
    tutorial_off.push(DataNode::Sym(Symbol("FALSE")));
    game->handle_property(Symbol("set_tutorial_running"), tutorial_off);
    CHECK(game->handle_property(Symbol("is_tutorial_running"), DataArray())
              .as_int()
              .value_or(1) == 0);
  } else {
    CHECK(false);
  }
  if (Object* tips_obj = mgr.resolve_object(Symbol("tips"))) {
    CHECK(tips_obj->handle_property(Symbol("random_tip"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("loading_tip1"));
  } else {
    CHECK(false);
  }
  if (Object* campaign = mgr.resolve_object(Symbol("campaign"))) {
    CHECK(campaign->handle_property(Symbol("tutorial_access"), DataArray())
              .as_int()
              .value_or(-1) == 0);
    CHECK(campaign->handle_property(Symbol("tutorials_done"), DataArray())
              .as_int()
              .value_or(-1) == 1);
    CHECK(campaign->handle_property(Symbol("last_difficulty"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("kDifficultyMedium"));
    DataArray status_arg;
    status_arg.push(DataNode::Sym(Symbol("kDifficultyEasy")));
    CHECK(campaign->handle_property(Symbol("get_status_progress"), status_arg)
              .as_string()
              .value_or("") == "0%");
    std::vector<Symbol> campaign_songs;
    std::vector<Symbol> first_tier_regular_songs;
    Symbol first_encore;
    Symbol first_venue;
    Symbol final_campaign_song;
    Symbol medium_tour_reward;
    int medium_required_encore = 0;
    int medium_max_status = 0;
    if (const DataArray* campaign_table = db.table(Symbol("campaign"))) {
      if (auto order = campaign_table->find_keyed(Symbol("order"))) {
        for (std::size_t i = 1; i < order->size(); ++i) {
          auto tier = order->at(i).as_array();
          if (!tier || tier->size() < 2) continue;
          if (!first_venue.valid()) {
            first_venue = tier->at(0).as_symbol().value_or(Symbol());
            for (std::size_t j = 1; j + 1 < tier->size(); ++j) {
              Symbol regular_song = tier->at(j).as_symbol().value_or(Symbol());
              if (regular_song.valid())
                first_tier_regular_songs.push_back(regular_song);
            }
          }
          Symbol encore = tier->at(tier->size() - 1).as_symbol().value_or(Symbol());
          if (!first_encore.valid()) first_encore = encore;
          for (std::size_t j = 1; j < tier->size(); ++j) {
            Symbol song = tier->at(j).as_symbol().value_or(Symbol());
            if (song.valid()) campaign_songs.push_back(song);
          }
        }
      }
      if (auto required = campaign_table->find_keyed(Symbol("required_songs"))) {
        for (std::size_t i = 1; i < required->size(); ++i) {
          auto row = required->at(i).as_array();
          if (!row || row->size() < 2) continue;
          if (row->at(0).as_symbol().value_or(Symbol()) ==
              Symbol("kDifficultyMedium")) {
            medium_required_encore = row->at(1).as_int().value_or(0);
            break;
          }
        }
      }
      if (auto cash = campaign_table->find_keyed(Symbol("cash"))) {
        if (auto awards = cash->find_keyed(Symbol("status_awards"))) {
          for (std::size_t i = 1; i < awards->size(); ++i) {
            auto row = awards->at(i).as_array();
            if (!row || row->empty()) continue;
            if (row->at(0).as_symbol().value_or(Symbol()) ==
                Symbol("kDifficultyMedium")) {
              medium_max_status =
                  row->size() > 1 ? static_cast<int>(row->size() - 2) : 0;
              break;
            }
          }
        }
      }
    }
    for (Symbol item : db.store_items(Symbol("guitar"))) {
      const DataArray* guitar = db.guitar(item);
      if (!guitar) continue;
      Symbol require = ui::ConfigDb::field(guitar, Symbol("require"))
                           .as_symbol()
                           .value_or(Symbol());
      Symbol difficulty = ui::ConfigDb::field(guitar, Symbol("difficulty"))
                              .as_symbol()
                              .value_or(Symbol());
      if (require == Symbol("tour_passed") &&
          difficulty == Symbol("kDifficultyMedium")) {
        medium_tour_reward = item;
        break;
      }
    }
    if (!campaign_songs.empty()) final_campaign_song = campaign_songs.back();
    CHECK(!campaign_songs.empty());
    CHECK(first_encore.valid());
    CHECK(first_venue.valid());
    CHECK(medium_required_encore > 0);
    CHECK(first_tier_regular_songs.size() >=
          static_cast<std::size_t>(medium_required_encore));
    CHECK(final_campaign_song.valid());
    CHECK(medium_tour_reward.valid());
    CHECK(medium_max_status > 0);
    if (campaign_songs.size() >= 2) {
      campaign->set_property(Symbol("attract_song_index"), DataNode::Int(0));
      CHECK(campaign->handle_property(Symbol("pick_attract_song"),
                                      DataArray())
                .as_symbol()
                .value_or(Symbol()) == campaign_songs[0]);
      CHECK(campaign->get_property(Symbol("last_attract_song"))
                .as_symbol()
                .value_or(Symbol()) == campaign_songs[0]);
      CHECK(campaign->handle_property(Symbol("pick_attract_song"),
                                      DataArray())
                .as_symbol()
                .value_or(Symbol()) == campaign_songs[1]);
      campaign->set_property(Symbol("attract_song_index"), DataNode::Int(0));
    }
    if (!campaign_songs.empty()) {
      const Symbol first_campaign_song = campaign_songs.front();
      DataArray beat_first;
      beat_first.push(DataNode::Sym(first_campaign_song));
      beat_first.push(DataNode::Int(12345));
      CHECK(campaign->handle_property(Symbol("cheat_beat_song"), beat_first)
                .as_symbol()
                .value_or(Symbol()) == first_campaign_song);
      CHECK(campaign->handle_property(Symbol("career_score"), DataArray())
                .as_int()
                .value_or(-1) == 12345);
      DataArray medium_status;
      medium_status.push(DataNode::Sym(Symbol("kDifficultyMedium")));
      const std::string expected_progress =
          std::to_string(static_cast<int>(100 / campaign_songs.size())) + "%";
      CHECK(campaign->handle_property(Symbol("get_status_progress"),
                                      medium_status)
                .as_string()
                .value_or("") == expected_progress);
      CHECK(campaign->handle_property(Symbol("is_unlocked"), beat_first)
                .as_int()
                .value_or(-1) == 1);
      campaign->set_property(
          Symbol(std::string("beat.kDifficultyMedium.") +
                 first_campaign_song.c_str()),
          DataNode::Int(0));
      campaign->set_property(
          Symbol(std::string("score.kDifficultyMedium.") +
                 first_campaign_song.c_str()),
          DataNode::Int(0));
      campaign->set_property(first_campaign_song, DataNode::Int(0));
      campaign->set_property(Symbol("career_score"), DataNode::Int(0));
    }
    if (first_encore.valid()) {
      DataArray encore_arg;
      encore_arg.push(DataNode::Sym(first_encore));
      CHECK(campaign->handle_property(Symbol("is_encore_song"), encore_arg)
                .as_int()
                .value_or(-1) == 1);
    }
    if (first_venue.valid() && medium_required_encore > 0 &&
        first_tier_regular_songs.size() >=
            static_cast<std::size_t>(medium_required_encore)) {
      const Symbol unlock_song =
          first_tier_regular_songs[static_cast<std::size_t>(
              medium_required_encore - 1)];
      for (int i = 0; i < medium_required_encore - 1; ++i) {
        DataArray beat_regular;
        beat_regular.push(DataNode::Sym(
            first_tier_regular_songs[static_cast<std::size_t>(i)]));
        campaign->handle_property(Symbol("cheat_beat_song"), beat_regular);
      }
      if (Object* game = mgr.resolve_object(Symbol("game"))) {
        DataArray set_venue;
        set_venue.push(DataNode::Sym(first_venue));
        game->handle_property(Symbol("set_venue"), set_venue);
        DataArray set_unlock_song;
        set_unlock_song.push(DataNode::Sym(unlock_song));
        game->handle_property(Symbol("set_song"), set_unlock_song);
      }
      CHECK(campaign->handle_property(Symbol("encore_unlock_potential"),
                                      DataArray())
                .as_int()
                .value_or(-1) == 1);
      DataArray finish_unlock_song;
      finish_unlock_song.push(DataNode::Int(0));
      finish_unlock_song.push(DataNode::Int(0));
      finish_unlock_song.push(DataNode::Sym(Symbol("career_cash_reason")));
      finish_unlock_song.push(DataNode::Str(""));
      finish_unlock_song.push(DataNode::Int(0));
      finish_unlock_song.push(DataNode::Int(0));
      campaign->handle_property(Symbol("finish_song"), finish_unlock_song);
      CHECK(campaign->handle_property(Symbol("encore_newly_unlocked"),
                                      DataArray())
                .as_int()
                .value_or(-1) == 1);
      CHECK(campaign->get_property(Symbol("last_encore_freebird"))
                .as_int()
                .value_or(-1) == 0);
      CHECK(campaign->handle_property(Symbol("get_cur_encore"), DataArray())
                .as_symbol()
                .value_or(Symbol()) == first_encore);
      for (int i = 0; i < medium_required_encore; ++i) {
        Symbol song = first_tier_regular_songs[static_cast<std::size_t>(i)];
        campaign->set_property(song, DataNode::Int(0));
        campaign->set_property(
            Symbol(std::string("beat.kDifficultyMedium.") + song.c_str()),
            DataNode::Int(0));
      }
      campaign->set_property(Symbol("last_finished_song"), DataNode());
      campaign->set_property(Symbol("last_finished_newly_beaten"),
                             DataNode::Int(0));
      campaign->set_property(Symbol("last_encore_newly_unlocked"),
                             DataNode::Int(0));
      campaign->set_property(Symbol("last_encore_freebird"), DataNode::Int(0));
      if (Object* game = mgr.resolve_object(Symbol("game")))
        game->set_property(Symbol("song"), DataNode());
    }
    if (final_campaign_song.valid()) {
      DataArray final_arg;
      final_arg.push(DataNode::Sym(final_campaign_song));
      CHECK(campaign->handle_property(Symbol("final_song"), final_arg)
                .as_int()
                .value_or(-1) == 1);
    }
    DataArray store_song;
    store_song.push(DataNode::Sym(Symbol("rawdog")));
    CHECK(campaign->handle_property(Symbol("is_store_song"), store_song)
              .as_int()
              .value_or(-1) == 1);
    if (Object* game = mgr.resolve_object(Symbol("game"))) {
      DataArray set_final_song;
      set_final_song.push(DataNode::Sym(final_campaign_song));
      game->handle_property(Symbol("set_song"), set_final_song);
    }
    if (medium_tour_reward.valid()) {
      campaign->set_property(medium_tour_reward, DataNode::Int(0));
      campaign->set_property(
          Symbol(std::string("pending_guitar_award.") +
                 medium_tour_reward.c_str()),
          DataNode::Int(0));
    }
    CHECK(campaign->handle_property(Symbol("num_guitar_awards"), DataArray())
              .as_int()
              .value_or(-1) == 0);
    DataArray finish_final;
    finish_final.push(DataNode::Int(100000));
    finish_final.push(DataNode::Int(500));
    finish_final.push(DataNode::Sym(Symbol("career_cash_reason")));
    finish_final.push(DataNode::Str(""));
    finish_final.push(DataNode::Int(medium_max_status));
    finish_final.push(DataNode::Int(300));
    campaign->handle_property(Symbol("finish_song"), finish_final);
    CHECK(campaign->handle_property(Symbol("won_campaign"), DataArray())
              .as_int()
              .value_or(-1) == 1);
    CHECK(campaign->handle_property(Symbol("status"), DataArray())
              .as_int()
              .value_or(-1) == medium_max_status);
    if (medium_tour_reward.valid()) {
      CHECK(campaign->handle_property(Symbol("num_guitar_awards"), DataArray())
                .as_int()
                .value_or(-1) == 1);
      CHECK(campaign->handle_property(Symbol("next_guitar_award"), DataArray())
                .as_symbol()
                .value_or(Symbol()) == medium_tour_reward);
      CHECK(campaign->handle_property(Symbol("num_guitar_awards"), DataArray())
                .as_int()
                .value_or(-1) == 0);
      DataArray reward_arg;
      reward_arg.push(DataNode::Sym(medium_tour_reward));
      CHECK(campaign->handle_property(Symbol("is_unlocked"), reward_arg)
                .as_int()
                .value_or(-1) == 1);
      campaign->set_property(medium_tour_reward, DataNode::Int(0));
      campaign->set_property(
          Symbol(std::string("pending_guitar_award.") +
                 medium_tour_reward.c_str()),
          DataNode::Int(0));
    }
    campaign->set_property(Symbol("won_campaign"), DataNode::Int(0));
    campaign->set_property(Symbol("status"), DataNode::Int(0));
    campaign->set_property(Symbol("cash"), DataNode::Int(0));
    campaign->set_property(Symbol("career_score"), DataNode::Int(0));
    if (final_campaign_song.valid()) {
      campaign->set_property(final_campaign_song, DataNode::Int(0));
      campaign->set_property(
          Symbol(std::string("beat.kDifficultyMedium.") +
                 final_campaign_song.c_str()),
          DataNode::Int(0));
      campaign->set_property(
          Symbol(std::string("score.kDifficultyMedium.") +
                 final_campaign_song.c_str()),
          DataNode::Int(0));
    }
    if (Object* game = mgr.resolve_object(Symbol("game")))
      game->set_property(Symbol("song"), DataNode());
    CHECK(campaign->handle_property(Symbol("cash"), DataArray())
              .as_int()
              .value_or(-1) == 0);
    CHECK(campaign->handle_property(Symbol("starting_cash"), DataArray())
              .as_int()
              .value_or(-1) == 50);
    DataArray add_cash;
    add_cash.push(DataNode::Int(3000));
    CHECK(campaign->handle_property(Symbol("add_cash"), add_cash)
              .as_int()
              .value_or(-1) == 3000);
    DataArray buy_video3;
    buy_video3.push(DataNode::Sym(Symbol("video3")));
    buy_video3.push(db.store_field(Symbol("video"), Symbol("video3"),
                                   Symbol("price")));
    CHECK(campaign->handle_property(Symbol("buy_item"), buy_video3)
              .as_int()
              .value_or(-1) == 0);
    DataArray video3_arg;
    video3_arg.push(DataNode::Sym(Symbol("video3")));
    CHECK(campaign->handle_property(Symbol("is_video_unlocked"), video3_arg)
              .as_int()
              .value_or(-1) == 1);
    CHECK(campaign->handle_property(Symbol("is_unlocked"), video3_arg)
              .as_int()
              .value_or(-1) == 1);
    campaign->set_property(Symbol("cash"), DataNode::Int(0));
    campaign->set_property(Symbol("video3"), DataNode::Int(0));
    DataArray unlock_arg;
    unlock_arg.push(DataNode::Sym(Symbol("multi_fo")));
    CHECK(campaign->handle_property(Symbol("is_unlocked"), unlock_arg)
              .as_int()
              .value_or(-1) == 0);
    campaign->set_property(Symbol("multi_fo"), DataNode::Int(1));
    CHECK(campaign->handle_property(Symbol("is_unlocked"), unlock_arg)
              .as_int()
              .value_or(-1) == 1);
    campaign->set_property(Symbol("multi_fo"), DataNode::Int(0));
    DataArray venue_unlock;
    venue_unlock.push(DataNode::Sym(Symbol("small2")));
    CHECK(campaign->handle_property(Symbol("is_unlocked"), venue_unlock)
              .as_int()
              .value_or(-1) == 1);
    DataArray empty_slot;
    empty_slot.push(DataNode::Int(7));
    CHECK(campaign->handle_property(Symbol("is_empty_profile"), empty_slot)
              .as_int()
              .value_or(0) == 1);
    CHECK(std::string(campaign->handle_property(Symbol("profile_name"), empty_slot)
                          .as_string()
                          .value_or("filled")) == "");
  } else {
    CHECK(false);
  }

  // 1. The {new ...} objects exist.
  Object* main_panel = mgr.find_object(Symbol("main_panel"));
  Object* main_screen = mgr.find_object(Symbol("main_screen"));
  CHECK(main_panel != nullptr);
  CHECK(main_screen != nullptr);
  CHECK(main_panel && main_panel->class_name() == Symbol("GHPanel"));
  CHECK(main_screen && main_screen->class_name() == Symbol("GHScreen"));
  Object* main_buttons_view = nullptr;
  if (auto* main_dir = dynamic_cast<ObjectDir*>(main_panel)) {
    main_buttons_view = main_dir->find(Symbol("main_buttons.view"));
  }
  CHECK(main_buttons_view != nullptr);
  CHECK(main_buttons_view &&
        main_buttons_view->class_name() == Symbol("Group"));

  // 2. main_panel's authored handler blocks parsed (enter/poll/SELECT_START_MSG
  //    + the custom reset_player_settings).
  if (auto* mp = dynamic_cast<ui::UiObject*>(main_panel)) {
    CHECK(mp->has_handler(Symbol("enter")));
    CHECK(mp->has_handler(Symbol("poll")));
    CHECK(mp->has_handler(Symbol("SELECT_START_MSG")));
    CHECK(mp->has_handler(Symbol("reset_player_settings")));
    // ...but config entries are properties, not handlers.
    CHECK(!mp->has_handler(Symbol("file")));
    CHECK(main_panel->get_property(Symbol("file")).as_symbol().has_value());
  }

  // 3. goto_screen(main_screen) runs the real (enter) scripts. reset_player_
  //    settings hits the game stub -> proves the authored script executed.
  mgr.goto_screen(Symbol("main_screen"));
  CHECK(mgr.current_screen() == main_screen);
  if (Object* synth = mgr.resolve_object(Symbol("synth"))) {
    CHECK(synth->get_property(Symbol("last_sequence"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("meta_lick"));
    CHECK(synth->get_property(Symbol("sequence_count"))
              .as_int()
              .value_or(0) > 0);
  } else {
    CHECK(false);
  }
  CHECK(mgr.resolve_object(Symbol("main_buttons.view")) == main_buttons_view);
  if (main_buttons_view) {
    DataArray group_hide_arg;
    group_hide_arg.push(DataNode::Sym(Symbol("FALSE")));
    main_buttons_view->handle_property(Symbol("set_showing"), group_hide_arg);
    CHECK(main_buttons_view->handle_property(Symbol("showing"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("FALSE"));
    DataArray group_show_arg;
    group_show_arg.push(DataNode::Sym(Symbol("TRUE")));
    main_buttons_view->handle_property(Symbol("set_showing"), group_show_arg);
    CHECK(main_buttons_view->handle_property(Symbol("showing"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("TRUE"));
  }

  mgr.goto_screen(Symbol("chooseprof_screen"));
  CHECK(mgr.current_screen() != nullptr &&
        mgr.current_screen()->name() == Symbol("chooseprof_screen"));
  Object* chooseprof_panel = mgr.find_object(Symbol("chooseprof_panel"));
  CHECK(chooseprof_panel != nullptr);
  if (chooseprof_panel) {
    CHECK(chooseprof_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("cp_band0.btn"));
    auto* dir = dynamic_cast<ObjectDir*>(chooseprof_panel);
    Object* first_band = dir ? dir->find(Symbol("cp_band0.btn")) : nullptr;
    Object* last_band = dir ? dir->find(Symbol("cp_band7.btn")) : nullptr;
    CHECK(first_band != nullptr);
    CHECK(last_band != nullptr);
    CHECK(first_band &&
          first_band->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("new_band"));
    CHECK(last_band &&
          last_band->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("new_band"));
  }

  Object* nameprof_screen = mgr.find_object(Symbol("nameprof_screen"));
  CHECK(nameprof_screen != nullptr);
  if (nameprof_screen) {
    nameprof_screen->set_property(Symbol("profile_slot"), DataNode::Int(0));
    nameprof_screen->set_property(Symbol("is_editing"), DataNode::Sym(Symbol("FALSE")));
    nameprof_screen->set_property(Symbol("next_screen"), DataNode::Sym(Symbol("options_screen")));
    mgr.goto_screen(Symbol("nameprof_screen"));
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("nameprof_screen"));
    Object* nameprof_panel = mgr.find_object(Symbol("nameprof_panel"));
    CHECK(nameprof_panel != nullptr);
    Object* profile_ten = nullptr;
    if (auto* dir = dynamic_cast<ObjectDir*>(nameprof_panel))
      profile_ten = dir->find(Symbol("profile.ten"));
    CHECK(profile_ten != nullptr);
    if (profile_ten) {
      DataArray text_arg;
      text_arg.push(DataNode::Str("THE CODEX"));
      profile_ten->handle_property(Symbol("set_text"), text_arg);
      CHECK(profile_ten->handle_property(Symbol("length"), DataArray())
                .as_int()
                .value_or(-1) == 9);
      CHECK(profile_ten->handle_property(Symbol("no_text_entered"), DataArray())
                .as_int()
                .value_or(1) == 0);
    }
    nameprof_screen->handle_property(Symbol("TEXT_ENTRY_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("options_screen"));
    if (Object* campaign = mgr.resolve_object(Symbol("campaign"))) {
      DataArray slot0;
      slot0.push(DataNode::Int(0));
      CHECK(campaign->handle_property(Symbol("profile_name"), slot0)
                .as_string()
                .value_or("") == "THE CODEX");
      DataArray named;
      named.push(DataNode::Str("THE CODEX"));
      named.push(DataNode::Sym(Symbol("FALSE")));
      CHECK(campaign->handle_property(Symbol("has_profile_name"), named)
                .as_int()
                .value_or(0) == 1);
      CHECK(campaign->handle_property(Symbol("num_profiles"), DataArray())
                .as_int()
                .value_or(0) == 1);
      CHECK(campaign->get_property(Symbol("profile_dirty"))
                .as_int()
                .value_or(0) == 1);
      campaign->handle_property(Symbol("save_complete"), DataArray());
      CHECK(campaign->get_property(Symbol("profile_dirty"))
                .as_int()
                .value_or(-1) == 0);
      CHECK(campaign->get_property(Symbol("save_complete_count"))
                .as_int()
                .value_or(0) == 1);

      Object* delete_confirm = mgr.find_object(Symbol("delete_confirm"));
      CHECK(delete_confirm != nullptr);
      if (delete_confirm) {
        delete_confirm->set_property(Symbol("selected_slot"),
                                     DataNode::Int(0));
        mgr.goto_screen(Symbol("delete_confirm"));
        Object* confirm_yes = mgr.resolve_object(Symbol("dl_button1.btn"));
        CHECK(confirm_yes != nullptr);
        if (confirm_yes) {
          mgr.set_global(Symbol("component"), DataNode::Obj(confirm_yes));
          delete_confirm->handle_property(Symbol("SELECT_START_MSG"),
                                          DataArray());
          CHECK(mgr.current_screen() != nullptr &&
                mgr.current_screen()->name() == Symbol("options_screen"));
          CHECK(campaign->handle_property(Symbol("is_empty_profile"), slot0)
                    .as_int()
                    .value_or(0) == 1);
          CHECK(campaign->handle_property(Symbol("profile_name"), slot0)
                    .as_string()
                    .value_or("still-filled") == "");
          CHECK(campaign->handle_property(Symbol("num_profiles"),
                                          DataArray())
                    .as_int()
                    .value_or(-1) == 0);
          CHECK(campaign->get_property(Symbol("last_deleted_slot"))
                    .as_int()
                    .value_or(-1) == 0);
        }
      }
    }
  }

  Object* cashaward_screen = mgr.find_object(Symbol("cashaward_screen"));
  CHECK(cashaward_screen != nullptr);
  if (cashaward_screen) {
    cashaward_screen->set_property(Symbol("new_cash"), DataNode::Int(500));
    cashaward_screen->set_property(Symbol("new_cash_reason"), DataNode::Str("career_cash_reason"));
    mgr.goto_screen(Symbol("cashaward_screen"));
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("cashaward_screen"));
    Object* cashaward_panel = mgr.find_object(Symbol("cashaward_panel"));
    CHECK(cashaward_panel != nullptr);
    auto* dir = dynamic_cast<ObjectDir*>(cashaward_panel);
    Object* reason = dir ? dir->find(Symbol("ca_reason.lbl")) : nullptr;
    Object* amount = dir ? dir->find(Symbol("ca_amount.lbl")) : nullptr;
    Object* venue = dir ? dir->find(Symbol("ca_venue.lbl")) : nullptr;
    Object* deduction = dir ? dir->find(Symbol("ca_blurb2.lbl")) : nullptr;
    Object* total = dir ? dir->find(Symbol("ca_num1.lbl")) : nullptr;
    CHECK(reason != nullptr);
    CHECK(amount != nullptr);
    CHECK(venue != nullptr);
    CHECK(deduction != nullptr);
    CHECK(total != nullptr);
    CHECK(reason && reason->get_property(Symbol("text"))
                        .as_string()
                        .value_or("") == "career_cash_reason");
    CHECK(amount && amount->get_property(Symbol("text"))
                        .as_string()
                        .value_or("") == "$500");
    CHECK(venue && venue->get_property(Symbol("text"))
                      .as_symbol()
                      .value_or(Symbol()) == Symbol("small2"));
    CHECK(deduction && deduction->get_property(Symbol("text"))
                          .as_symbol()
                          .value_or(Symbol()).valid());
    CHECK(total && total->get_property(Symbol("text"))
                     .as_string()
                     .value_or("")
                     .size() > 0);
    mgr.set_global(Symbol("button"), DataNode::Sym(Symbol("kPad_X")));
    cashaward_screen->handle_property(Symbol("BUTTON_DOWN_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("highscore_screen"));
  }

  {
    DataArray loader_args;
    DataNode previous;
    loader_args.push(DataNode::Float(21.0f));
    CHECK(mgr.handle_command(Symbol("set_loader_period"), loader_args,
                             previous));
    loader_args.at(0) = DataNode::Float(8.0f);
    CHECK(mgr.handle_command(Symbol("set_loader_period"), loader_args,
                             previous));
    CHECK(near(previous.as_float().value_or(-1.0f), 21.0f));
    CHECK(near(mgr.get_global(Symbol("loader_period"))
                   .as_float()
                   .value_or(-1.0f),
               8.0f));
    Object* taskmgr = mgr.resolve_object(Symbol("taskmgr"));
    CHECK(taskmgr != nullptr);
    if (taskmgr) {
      CHECK(near(taskmgr->get_property(Symbol("loader_period"))
                     .as_float()
                     .value_or(-1.0f),
                 8.0f));
      CHECK(near(taskmgr->get_property(Symbol("previous_loader_period"))
                     .as_float()
                     .value_or(-1.0f),
                 21.0f));
    }
  }

  mgr.goto_screen(Symbol("splash_screen"));
  mgr.update(0.016f);
  CHECK(mgr.current_screen() != nullptr &&
        mgr.current_screen()->name() == Symbol("splash_screen"));
  CHECK(near(mgr.get_global(Symbol("loader_period"))
                 .as_float()
                 .value_or(-1.0f),
             5.0f));
  if (Object* taskmgr = mgr.resolve_object(Symbol("taskmgr"))) {
    CHECK(near(taskmgr->get_property(Symbol("loader_period"))
                   .as_float()
                   .value_or(-1.0f),
               5.0f));
    CHECK(near(taskmgr->get_property(Symbol("previous_loader_period"))
                   .as_float()
                   .value_or(-1.0f),
               13.0f));
  }
  Object* splash_panel = mgr.find_object(Symbol("splash_panel"));
  Object* splash_view = mgr.resolve_object(Symbol("splash.view"));
  CHECK(splash_panel != nullptr);
  CHECK(splash_view != nullptr);
  CHECK(splash_view && splash_view->class_name() == Symbol("Group"));
  if (splash_view) {
    const float splash_frame =
        splash_view->handle_property(Symbol("frame"), DataArray())
            .as_float()
            .value_or(-1.0f);
    CHECK(splash_view->get_property(Symbol("animating")).as_int().value_or(0) == 1);
    CHECK(splash_view->get_property(Symbol("period")).as_float().value_or(0.0f) > 3.0f);
    const float splash_end =
        splash_view->handle_property(Symbol("end_frame"), DataArray())
            .as_float()
            .value_or(-1.0f);
    CHECK(splash_end == 100.0f);
    CHECK(splash_frame >= 0.0f && splash_frame <= splash_end);
  }
  Object* sel_guitar_anim = nullptr;
  if (splash_view) {
    DataArray pos_args;
    pos_args.push(DataNode::Float(-1.0f));
    pos_args.push(DataNode::Float(2.0f));
    pos_args.push(DataNode::Float(3.5f));
    splash_view->handle_property(Symbol("set_local_pos"), pos_args);
    CHECK(splash_view->get_property(Symbol("local_pos_x"))
              .as_float()
              .value_or(0.0f) == -1.0f);
    CHECK(splash_view->get_property(Symbol("local_pos_z"))
              .as_float()
              .value_or(0.0f) == 3.5f);
  }
  for (std::size_t i = 0; i < mgr.registry().size(); ++i) {
    if (auto* dir = dynamic_cast<ObjectDir*>(mgr.registry().at(i))) {
      if (!sel_guitar_anim) sel_guitar_anim = dir->find(Symbol("sel_guitar.tnm"));
    }
  }
  CHECK(sel_guitar_anim != nullptr);
  CHECK(sel_guitar_anim &&
        sel_guitar_anim->class_name() == Symbol("TransAnim"));
  if (sel_guitar_anim) {
    DataArray frame_arg;
    frame_arg.push(DataNode::Float(2.0f));
    sel_guitar_anim->handle_property(Symbol("set_frame"), frame_arg);
    CHECK(sel_guitar_anim->handle_property(Symbol("frame"), DataArray())
              .as_float()
              .value_or(-1.0f) == 2.0f);
  }
  Symbol first_attract_song;
  if (const DataArray* campaign_table = db.table(Symbol("campaign"))) {
    if (auto order = campaign_table->find_keyed(Symbol("order"))) {
      for (std::size_t i = 1; i < order->size() && !first_attract_song.valid();
           ++i) {
        auto tier = order->at(i).as_array();
        if (!tier || tier->size() < 2) continue;
        first_attract_song = tier->at(1).as_symbol().value_or(Symbol());
      }
    }
  }
  if (splash_panel && first_attract_song.valid()) {
    if (Object* campaign = mgr.resolve_object(Symbol("campaign")))
      campaign->set_property(Symbol("attract_song_index"), DataNode::Int(0));
    splash_panel->handle_property(Symbol("enter_attract_mode"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("loading_screen"));
    if (Object* game = mgr.resolve_object(Symbol("game"))) {
      CHECK(game->handle_property(Symbol("get_song"), DataArray())
                .as_symbol()
                .value_or(Symbol()) == first_attract_song);
      CHECK(game->get_property(Symbol("mode"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("quickplay"));
    }
    if (Object* player0 = mgr.resolve_object(Symbol("player0"))) {
      CHECK(player0->get_property(Symbol("difficulty"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("kDifficultyExpert"));
    }
    if (Object* game_screen = mgr.find_object(Symbol("game_screen"))) {
      CHECK(truthy(game_screen->get_property(Symbol("attract_mode"))));
    }
    mgr.update(0.0f);
    Object* loading_word = mgr.resolve_object(Symbol("loading_word.grp"));
    Object* flying_tape = mgr.resolve_object(Symbol("flyingtape2.grp"));
    CHECK(loading_word != nullptr);
    CHECK(flying_tape != nullptr);
    if (loading_word && flying_tape) {
      CHECK(loading_word->get_property(Symbol("animate_forever_30fps"))
                .as_int()
                .value_or(0) == 1);
      CHECK(flying_tape->get_property(Symbol("animate_forever_30fps"))
                .as_int()
                .value_or(0) == 1);
      CHECK(loading_word->get_property(Symbol("anim_rate"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("k30_fps_ui"));
      const float frame0 = loading_word->handle_property(Symbol("frame"),
                                                        DataArray())
                               .as_float()
                               .value_or(-1.0f);
      mgr.update(0.5f);
      const float frame1 = loading_word->handle_property(Symbol("frame"),
                                                        DataArray())
                               .as_float()
                               .value_or(-1.0f);
      CHECK(frame1 > frame0 + 14.9f && frame1 < frame0 + 15.1f);
    }
  }

  mgr.goto_screen(Symbol("guitar_help_screen"));
  Object* guitar_help_panel = mgr.find_object(Symbol("guitar_help_panel"));
  CHECK(mgr.current_screen() != nullptr &&
        mgr.current_screen()->name() == Symbol("guitar_help_screen"));
  CHECK(guitar_help_panel != nullptr);
  if (guitar_help_panel) {
    mgr.set_global(Symbol("button"), DataNode::Sym(Symbol("kPad_X")));
    guitar_help_panel->handle_property(Symbol("BUTTON_DOWN_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("splash_screen"));
  }

  mgr.goto_screen(Symbol("sel_guitar_new_screen"));
  CHECK(mgr.current_screen() == mgr.find_object(Symbol("sel_guitar_new_screen")));
  Object* sel_guitar_panel = mgr.find_object(Symbol("sel_guitar_panel"));
  CHECK(sel_guitar_panel != nullptr);
  if (sel_guitar_panel) {
    DataArray player_arg;
    player_arg.push(DataNode::Int(0));
    CHECK(sel_guitar_panel->handle_property(Symbol("is_skin_select"), player_arg)
              .as_int()
              .value_or(-1) == 0);
    CHECK(sel_guitar_panel->handle_property(Symbol("get_selected_guitar"),
                                            player_arg)
              .as_symbol()
              .value_or(Symbol()) == Symbol("lespaul"));
    CHECK(sel_guitar_panel->handle_property(Symbol("get_selected_skin"),
                                            player_arg)
              .as_symbol()
              .value_or(Symbol()) == Symbol("lp_cherry"));
    DataArray skins_args;
    skins_args.push(DataNode::Int(0));
    skins_args.push(DataNode::Sym(Symbol("lespaul")));
    CHECK(sel_guitar_panel->handle_property(Symbol("get_num_skins"),
                                            skins_args)
              .as_int()
              .value_or(-1) == 4);
  }
  if (Object* guitar_name = mgr.resolve_object(Symbol("sg_guitar_nm.lbl")))
    CHECK(guitar_name->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lespaul"));
  else
    CHECK(false);
  if (Object* guitar_desc = mgr.resolve_object(Symbol("sg_guitar_desc.lbl")))
    CHECK(guitar_desc->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lespaul_desc"));
  else
    CHECK(false);
  if (Object* skin_name = mgr.resolve_object(Symbol("sg_skin_nm.lbl")))
    CHECK(skin_name->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lp_cherry"));
  else
    CHECK(false);
  if (Object* skin_desc = mgr.resolve_object(Symbol("sg_skin_desc.lbl")))
    CHECK(skin_desc->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lp_cherry_desc"));
  else
    CHECK(false);
  if (Object* guitar_text = mgr.resolve_object(Symbol("sg_text_guitar.grp")))
    CHECK(truthy(guitar_text->handle_property(Symbol("showing"), DataArray())));
  else
    CHECK(false);
  if (Object* skin_text = mgr.resolve_object(Symbol("sg_text_skin.grp")))
    CHECK(!truthy(skin_text->handle_property(Symbol("showing"), DataArray())));
  else
    CHECK(false);
  Object* guitar_display = mgr.find_object(Symbol("guitar_display_panel"));
  CHECK(guitar_display != nullptr);
  if (guitar_display) {
    CHECK(guitar_display->get_property(Symbol("guitar"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lespaul"));
    CHECK(guitar_display->get_property(Symbol("guitar_skin"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lp_cherry"));
    CHECK(guitar_display->get_property(Symbol("guitar_0"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lespaul"));
    CHECK(guitar_display->get_property(Symbol("guitar_skin_0"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lp_cherry"));
    CHECK(guitar_display->get_property(Symbol("guitar_proxy"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("guitar.pxy"));
    CHECK(guitar_display->get_property(Symbol("guitar_filter"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("guitar_single.filt"));
    CHECK(guitar_display->get_property(Symbol("guitar_display_env"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("guitar.env"));
  }

  mgr.goto_screen(Symbol("main_screen"));
  // the authored (enter) -> reset_player_settings ran on the REAL game-side
  // objects: {game set_venue small2}{game set_character punk1 TRUE} and
  // {{game get_player_config 0} set_difficulty kDifficultyMedium}.
  if (Object* game = mgr.resolve_object(Symbol("game"))) {
    CHECK(game->get_property(Symbol("venue")).as_symbol().value_or(Symbol()) == Symbol("small2"));
    CHECK(game->get_property(Symbol("character")).as_symbol().value_or(Symbol()) == Symbol("punk1"));
  }
  if (Object* p0 = mgr.resolve_object(Symbol("player0")))
    CHECK(p0->get_property(Symbol("difficulty")).as_symbol().value_or(Symbol()) ==
          Symbol("kDifficultyMedium"));

  // UISlider is source-shaped as integer current/num_steps state
  // (mCurrent/mNumSteps in the Harmonix source dump). The stock audio settings
  // scripts call these methods on gs_*.sld.
  ui::UiObject slider_probe(Symbol("UISlider"));
  DataArray one_arg;
  one_arg.push(DataNode::Int(11));
  slider_probe.handle_property(Symbol("set_num_steps"), one_arg);
  CHECK(slider_probe.get_property(Symbol("num_steps")).as_int().value_or(-1) == 11);
  DataArray cur_arg;
  cur_arg.push(DataNode::Int(6));
  slider_probe.handle_property(Symbol("set_current"), cur_arg);
  CHECK(slider_probe.handle_property(Symbol("current"), DataArray()).as_int().value_or(-1) == 6);
  // Harmonix UISlider::Frame() derives from mCurrent/mNumSteps, and
  // UISlider::SetFrame() writes mCurrent = frame * (steps - 1) + 0.5.
  CHECK(near(slider_probe.handle_property(Symbol("frame"), DataArray())
                 .as_float()
                 .value_or(-1.0f),
             0.6f));
  DataArray frame_arg;
  frame_arg.push(DataNode::Float(0.25f));
  slider_probe.handle_property(Symbol("set_frame"), frame_arg);
  CHECK(slider_probe.handle_property(Symbol("current"), DataArray()).as_int().value_or(-1) == 3);
  CHECK(near(slider_probe.handle_property(Symbol("frame"), DataArray())
                 .as_float()
                 .value_or(-1.0f),
             0.3f));

  Object* band_slider = nullptr;
  for (std::size_t i = 0; i < mgr.registry().size() && !band_slider; ++i) {
    if (auto* dir = dynamic_cast<ObjectDir*>(mgr.registry().at(i)))
      band_slider = dir->find(Symbol("gs_band.sld"));
  }
  if (band_slider) {
    CHECK(band_slider->handle_property(Symbol("current"), DataArray()).as_int().value_or(-1) ==
          0);
    CHECK(band_slider->handle_property(Symbol("num_steps"), DataArray()).as_int().value_or(-1) ==
          1);
    band_slider->handle_property(Symbol("set_num_steps"), one_arg);
    band_slider->handle_property(Symbol("set_current"), cur_arg);
    CHECK(band_slider->handle_property(Symbol("current"), DataArray()).as_int().value_or(-1) ==
          6);
  } else {
    CHECK(false);
  }

  // Checkboxes are source-shaped as a `checked` bool (MiloLib
  // CheckboxDisplay.isChecked / Harmonix CheckboxDisplay::mChecked). GH2's
  // stock scripts call get_check/set_check/toggle on the concrete .chk widgets.
  ui::UiObject checkbox(Symbol("CheckboxDisplay"));
  CHECK(checkbox.handle_property(Symbol("get_check"), DataArray()).as_int().value_or(-1) == 0);
  DataArray check_arg;
  check_arg.push(DataNode::Int(1));
  checkbox.handle_property(Symbol("set_check"), check_arg);
  CHECK(checkbox.get_property(Symbol("checked")).as_int().value_or(-1) == 1);
  CHECK(checkbox.handle_property(Symbol("get_check"), DataArray()).as_int().value_or(-1) == 1);
  CHECK(checkbox.handle_property(Symbol("toggle"), DataArray()).as_int().value_or(-1) == 0);
  CHECK(checkbox.handle_property(Symbol("get_check"), DataArray()).as_int().value_or(-1) == 0);

  if (Object* video_panel = mgr.find_object(Symbol("video_settings_panel"))) {
    DataArray find_args;
    find_args.push(DataNode::Sym(Symbol("p_scan.chk")));
    DataNode found = video_panel->handle_property(Symbol("find"), find_args);
    CHECK(found.as_object() != nullptr);
    CHECK(found.as_object() &&
          found.as_object()->name() == Symbol("p_scan.chk"));
    if (Object* pscan = found.as_object()) {
      CHECK(pscan->handle_property(Symbol("get_check"), DataArray()).as_int().value_or(-1) == 1);
      pscan->handle_property(Symbol("set_check"), check_arg);
      CHECK(pscan->handle_property(Symbol("get_check"), DataArray()).as_int().value_or(-1) == 1);
      CHECK(pscan->handle_property(Symbol("toggle"), DataArray()).as_int().value_or(-1) == 0);
    }
  } else {
    CHECK(false);
  }

  ui::UiObject showing_widget(Symbol("BandLabel"));
  DataArray hide_arg;
  hide_arg.push(DataNode::Sym(Symbol("FALSE")));
  showing_widget.handle_property(Symbol("set_showing"), hide_arg);
  CHECK(showing_widget.handle_property(Symbol("get_showing"), DataArray())
            .as_symbol()
            .value_or(Symbol()) == Symbol("FALSE"));
  CHECK(showing_widget.handle_property(Symbol("showing"), DataArray())
            .as_symbol()
            .value_or(Symbol()) == Symbol("FALSE"));
  DataArray show_arg;
  show_arg.push(DataNode::Sym(Symbol("TRUE")));
  showing_widget.handle_property(Symbol("set_showing"), show_arg);
  CHECK(showing_widget.handle_property(Symbol("showing"), DataArray())
            .as_symbol()
            .value_or(Symbol()) == Symbol("TRUE"));

  // Stock options.dtb uses both `{panel disable child}` and
  // `{button set_state kDisabled}`. `set_state` must therefore drive the same
  // live disabled flag used by rendering, focus navigation, and confirm gating.
  ui::UiObject state_button(Symbol("BandButton"));
  DataArray disabled_state;
  disabled_state.push(DataNode::Sym(Symbol("kDisabled")));
  state_button.handle_property(Symbol("set_state"), disabled_state);
  CHECK(state_button.get_property(Symbol("state"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("kDisabled"));
  CHECK(state_button.get_property(Symbol("disabled"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("TRUE"));
  DataArray normal_state;
  normal_state.push(DataNode::Sym(Symbol("kNormal")));
  state_button.handle_property(Symbol("set_state"), normal_state);
  CHECK(state_button.get_property(Symbol("disabled"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("FALSE"));

  // ihatecompvir's PanelDir::SetFocusComponent clears the old component back to
  // normal and marks the new component focused. Keep that native state in sync
  // with the panel's authored `focus` field instead of treating focus as a
  // renderer-only color choice.
  ui::UiObject focus_panel(Symbol("GHPanel"));
  focus_panel.set_name(Symbol("focus_probe_panel"));
  auto focus_a = std::make_unique<ui::UiObject>(Symbol("BandButton"));
  focus_a->set_name(Symbol("focus_a.btn"));
  Object* focus_a_ptr = focus_panel.add(std::move(focus_a));
  auto focus_b = std::make_unique<ui::UiObject>(Symbol("BandButton"));
  focus_b->set_name(Symbol("focus_b.btn"));
  Object* focus_b_ptr = focus_panel.add(std::move(focus_b));
  auto focus_label = std::make_unique<ui::UiObject>(Symbol("BandLabel"));
  focus_label->set_name(Symbol("decorative.lbl"));
  Object* focus_label_ptr = focus_panel.add(std::move(focus_label));
  CHECK(focus_a_ptr->handle_property(Symbol("can_have_focus"), DataArray())
            .as_int()
            .value_or(-1) == 1);
  CHECK(focus_label_ptr->handle_property(Symbol("can_have_focus"), DataArray())
            .as_int()
            .value_or(-1) == 0);
  DataNode focusable =
      focus_panel.handle_property(Symbol("get_focusable_components"), DataArray());
  CHECK(focusable.as_array() && focusable.as_array()->size() == 2);
  DataArray set_focus_a;
  set_focus_a.push(DataNode::Sym(Symbol("focus_a.btn")));
  focus_panel.handle_property(Symbol("set_focus"), set_focus_a);
  CHECK(focus_a_ptr->get_property(Symbol("state"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("focused"));
  CHECK(focus_panel.handle_property(Symbol("focus_name"), DataArray())
            .as_symbol()
            .value_or(Symbol()) == Symbol("focus_a.btn"));
  DataArray set_focus_b;
  set_focus_b.push(DataNode::Sym(Symbol("focus_b.btn")));
  focus_panel.handle_property(Symbol("set_focus"), set_focus_b);
  CHECK(focus_a_ptr->get_property(Symbol("state"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("normal"));
  CHECK(focus_b_ptr->get_property(Symbol("state"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("focused"));
  DataArray hide_focus_state;
  hide_focus_state.push(DataNode::Int(0));
  focus_panel.handle_property(Symbol("set_show_focus_component"),
                              hide_focus_state);
  CHECK(focus_b_ptr->get_property(Symbol("state"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("normal"));
  DataArray show_focus_state;
  show_focus_state.push(DataNode::Int(1));
  focus_panel.handle_property(Symbol("set_show_focus_component"),
                              show_focus_state);
  CHECK(focus_b_ptr->get_property(Symbol("state"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("focused"));
  DataArray disable_focus_b;
  disable_focus_b.push(DataNode::Sym(Symbol("focus_b.btn")));
  focus_panel.handle_property(Symbol("disable"), disable_focus_b);
  CHECK(focus_b_ptr->get_property(Symbol("state"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("disabled"));
  DataArray enable_focus_b;
  enable_focus_b.push(DataNode::Sym(Symbol("focus_b.btn")));
  focus_panel.handle_property(Symbol("enable"), enable_focus_b);
  CHECK(focus_b_ptr->get_property(Symbol("state"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("normal"));
  focus_panel.handle_property(Symbol("set_focus"), set_focus_b);
  focus_b_ptr->handle_property(Symbol("send_select"), DataArray());
  CHECK(focus_b_ptr->get_property(Symbol("state"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("selecting"));
  CHECK(focus_b_ptr->handle_property(Symbol("get_state"), DataArray())
            .as_int()
            .value_or(-1) == 3);
  for (int i = 0; i < 10; ++i)
    focus_panel.handle_property(Symbol("poll"), DataArray());
  CHECK(focus_b_ptr->get_property(Symbol("state"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("focused"));

  // Script-set label text is runtime state on the MILO child object. The menu
  // renderer resolves this same object name and prefers the live `text` and
  // `showing` values over the static MILO token/visibility.
  if (Object* msg = mgr.resolve_object(Symbol("mm_msg.lbl"))) {
    DataArray localized;
    localized.push(DataNode::Sym(Symbol("QUICK_PLAY")));
    msg->handle_property(Symbol("set_localized_text"), localized);
    CHECK(std::string(msg->get_property(Symbol("text")).as_string().value_or("")) ==
          mgr.localize(Symbol("QUICK_PLAY")));
    DataArray raw_text;
    raw_text.push(DataNode::Str("Runtime Label"));
    msg->handle_property(Symbol("set_text"), raw_text);
    CHECK(std::string(msg->get_property(Symbol("text")).as_string().value_or("")) ==
          "Runtime Label");
    msg->handle_property(Symbol("set_showing"), hide_arg);
    CHECK(msg->handle_property(Symbol("showing"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("FALSE"));
    msg->handle_property(Symbol("set_showing"), show_arg);
    CHECK(msg->handle_property(Symbol("showing"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("TRUE"));
  } else {
    CHECK(false);
  }

  if (Object* options = mgr.resolve_object(Symbol("options"))) {
    DataArray set_wide;
    set_wide.push(DataNode::Int(1));
    options->handle_property(Symbol("set_widescreen"), set_wide);
    CHECK(options->handle_property(Symbol("get_widescreen"), DataArray()).as_int().value_or(-1) ==
          1);
    DataArray set_lefty;
    set_lefty.push(DataNode::Int(1));
    set_lefty.push(DataNode::Int(1));
    options->handle_property(Symbol("set_lefty"), set_lefty);
    DataArray get_lefty;
    get_lefty.push(DataNode::Int(1));
    CHECK(options->handle_property(Symbol("get_lefty"), get_lefty).as_int().value_or(-1) == 1);
    CHECK(options->handle_property(Symbol("get_pscan"), DataArray()).as_int().value_or(-1) == 0);
    CHECK(options->handle_property(Symbol("get_stereo"), DataArray()).as_int().value_or(-1) == 1);
  } else {
    CHECK(false);
  }

  // The stock options panel treats $component as the focused object. Confirm
  // needs to supply that object, while script switch cases still compare
  // against the authored button names.
  mgr.goto_screen(Symbol("options_screen"));
  Object* options_panel = mgr.find_object(Symbol("options_panel"));
  CHECK(options_panel != nullptr);
  if (options_panel) {
    CHECK(options_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("op_audio.btn"));
  }
  Object* audio_button = mgr.resolve_object(Symbol("op_audio.btn"));
  Object* video_button = mgr.resolve_object(Symbol("video_settings.btn"));
  Object* data_button = mgr.resolve_object(Symbol("op_data.btn"));
  Object* memory_button = mgr.resolve_object(Symbol("memory_card.btn"));
  CHECK(audio_button != nullptr);
  CHECK(video_button != nullptr);
  CHECK(data_button != nullptr);
  CHECK(memory_button != nullptr);
  if (options_panel && audio_button) {
    mgr.set_global(Symbol("component"), DataNode::Obj(audio_button));
    options_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("audio_settings_screen"));
    mgr.goto_screen(Symbol("options_screen"));
  }
  if (options_panel && video_button) {
    mgr.set_global(Symbol("component"), DataNode::Obj(video_button));
    options_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("video_settings_screen"));
    Object* video_screen = mgr.current_screen();
    video_screen->handle_property(Symbol("go_back"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("options_screen"));
  }
  if (options_panel && data_button) {
    mgr.set_global(Symbol("component"), DataNode::Obj(data_button));
    options_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("options_chooseprof_screen"));
    mgr.goto_screen(Symbol("options_screen"));
  }
  if (options_panel && memory_button) {
    mgr.set_global(Symbol("component"), DataNode::Obj(memory_button));
    options_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("mem_card_screen"));
    mgr.goto_screen(Symbol("options_screen"));
  }

  Object* bonus_button = mgr.resolve_object(Symbol("op_bonus.btn"));
  CHECK(bonus_button != nullptr);
  if (options_panel && bonus_button) {
    mgr.set_global(Symbol("component"), DataNode::Obj(bonus_button));
    options_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("bonus_material_screen"));
    if (Object* bonus_screen = mgr.current_screen()) {
      if (Object* game = mgr.resolve_object(Symbol("game"))) {
        DataNode provider = game->handle_property(Symbol("video_provider"), DataArray());
        CHECK(provider.as_object() == game);
        DataArray video_arg;
        video_arg.push(DataNode::Sym(Symbol("video3")));
        CHECK(game->handle_property(Symbol("get_video_file"), video_arg)
                  .as_string()
                  .value_or("") == "hmx.pss");
      }
      if (Object* campaign = mgr.resolve_object(Symbol("campaign"))) {
        DataArray video_arg;
        video_arg.push(DataNode::Sym(Symbol("video3")));
        CHECK(campaign->handle_property(Symbol("is_video_unlocked"), video_arg)
                  .as_int()
                  .value_or(-1) == 0);
      }
      if (Object* video3 = mgr.resolve_object(Symbol("bm_video3.btn"))) {
        CHECK(video3->get_property(Symbol("disabled"))
                  .as_symbol()
                  .value_or(Symbol()) == Symbol("TRUE"));
      } else {
        CHECK(false);
      }
      if (Object* buy = mgr.resolve_object(Symbol("bm_buy.lbl"))) {
        CHECK(buy->handle_property(Symbol("showing"), DataArray())
                  .as_symbol()
                  .value_or(Symbol()) == Symbol("TRUE"));
      } else {
        CHECK(false);
      }
      bonus_screen->handle_property(Symbol("go_back"), DataArray());
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("options_screen"));
    }
  }

  Object* credit_button = mgr.resolve_object(Symbol("op_credit.btn"));
  CHECK(credit_button != nullptr);
  if (options_panel && credit_button) {
    mgr.set_global(Symbol("component"), DataNode::Obj(credit_button));
    options_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("credits_screen"));
    if (auto* credits_screen =
            dynamic_cast<ui::UiObject*>(mgr.current_screen())) {
      CHECK(credits_screen->has_handler(Symbol("go_back")));
    } else {
      CHECK(false);
    }
    if (Object* credits_list = mgr.resolve_object(Symbol("credits.lst"))) {
      CHECK(credits_list->handle_property(Symbol("selected_pos"), DataArray())
                .as_int()
                .value_or(-1) == 0);
      CHECK(credits_list->get_property(Symbol("auto_scrolling"))
                .as_int()
                .value_or(0) == 1);
      CHECK(credits_list->handle_property(Symbol("first_showing"), DataArray())
                .as_int()
                .value_or(-1) == 0);
      CHECK(credits_list->handle_property(Symbol("selected_display"), DataArray())
                .as_int()
                .value_or(-1) == 0);
      CHECK(credits_list->handle_property(Symbol("is_scrolling"), DataArray())
                .as_int()
                .value_or(-1) == 0);
      // ihatecompvir CreditsPanel::Enter calls mList->AutoScroll(); UIList's
      // constructor default mAutoScrollPause is 2.0s for old rev-2 lists.
      // After the pause, UIList::Poll calls Scroll() and UIListState moves
      // from first_showing to target_showing over the serialized speed.
      mgr.update(2.1f);
      CHECK(credits_list->handle_property(Symbol("selected_pos"), DataArray())
                .as_int()
                .value_or(-1) == 1);
      CHECK(credits_list->handle_property(Symbol("first_showing"), DataArray())
                .as_int()
                .value_or(-1) == 0);
      CHECK(credits_list->get_property(Symbol("target_showing"))
                .as_int()
                .value_or(-1) == 1);
      CHECK(credits_list->handle_property(Symbol("selected_display"), DataArray())
                .as_int()
                .value_or(-1) == 1);
      CHECK(credits_list->handle_property(Symbol("is_scrolling"), DataArray())
                .as_int()
                .value_or(-1) == 1);
      mgr.update(0.1f);
      CHECK(credits_list->get_property(Symbol("scroll_step_percent"))
                .as_float()
                .value_or(0.0f) > 0.0f);
    } else {
      CHECK(false);
    }
    if (Object* credits_screen = mgr.current_screen()) {
      Object* final_credits_list = mgr.resolve_object(Symbol("credits.lst"));
      if (credits && final_credits_list) {
        DataArray select_last_credit;
        select_last_credit.push(
            DataNode::Int(static_cast<int>(credits->size() - 1)));
        final_credits_list->handle_property(Symbol("set_selected"),
                                            select_last_credit);
        credits_screen->handle_property(Symbol("SCROLL_MSG"), DataArray());
      } else {
        credits_screen->handle_property(Symbol("go_back"), DataArray());
      }
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("options_screen"));
    }
  }

  mgr.goto_screen(Symbol("multi_screen"));
  Object* multi_screen = mgr.current_screen();
  Object* multi_panel = mgr.find_object(Symbol("multi_panel"));
  Object* coop_button = mgr.resolve_object(Symbol("coop.btn"));
  Object* versus_button = mgr.resolve_object(Symbol("versus.btn"));
  Object* faceoff_button = mgr.resolve_object(Symbol("faceoff.btn"));
  CHECK(multi_screen != nullptr);
  CHECK(multi_panel != nullptr);
  CHECK(coop_button != nullptr);
  CHECK(versus_button != nullptr);
  CHECK(faceoff_button != nullptr);
  if (multi_screen && multi_panel && coop_button && versus_button &&
      faceoff_button) {
    CHECK(multi_screen->name() == Symbol("multi_screen"));
    CHECK(multi_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("coop.btn"));
    CHECK(faceoff_button->get_property(Symbol("disabled"))
              .as_symbol()
              .value_or(Symbol("FALSE")) == Symbol("TRUE"));
    CHECK(faceoff_button->handle_property(Symbol("showing"), DataArray())
              .as_symbol()
              .value_or(Symbol("TRUE")) == Symbol("FALSE"));

    mgr.set_global(Symbol("component"), DataNode::Obj(coop_button));
    multi_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("multi_coop_venue_screen"));
    if (Object* gamecfg = mgr.resolve_object(Symbol("gamecfg")))
      CHECK(gamecfg->get_property(Symbol("mode"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("multi_coop"));

    mgr.goto_screen(Symbol("multi_screen"));
    mgr.set_global(Symbol("component"), DataNode::Obj(versus_button));
    multi_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("multi_sel_character_screen"));
    if (Object* gamecfg = mgr.resolve_object(Symbol("gamecfg")))
      CHECK(gamecfg->get_property(Symbol("mode"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("multi_vs"));

    if (Object* campaign = mgr.resolve_object(Symbol("campaign")))
      campaign->set_property(Symbol("multi_fo"), DataNode::Int(1));
    mgr.goto_screen(Symbol("multi_screen"));
    CHECK(faceoff_button->get_property(Symbol("disabled"))
              .as_symbol()
              .value_or(Symbol("TRUE")) == Symbol("FALSE"));
    CHECK(faceoff_button->handle_property(Symbol("showing"), DataArray())
              .as_symbol()
              .value_or(Symbol("FALSE")) == Symbol("TRUE"));
    mgr.set_global(Symbol("component"), DataNode::Obj(faceoff_button));
    multi_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("multi_sel_character_screen"));
    if (Object* gamecfg = mgr.resolve_object(Symbol("gamecfg")))
      CHECK(gamecfg->get_property(Symbol("mode"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("multi_fo"));
    if (Object* campaign = mgr.resolve_object(Symbol("campaign")))
      campaign->set_property(Symbol("multi_fo"), DataNode::Int(0));
  }

  mgr.goto_screen(Symbol("multi_sel_guitar_screen"));
  Object* multi_guitar_screen = mgr.current_screen();
  Object* multi_guitar_panel = mgr.find_object(Symbol("multi_sel_guitar_panel"));
  Object* multi_guitar_display =
      mgr.find_object(Symbol("multi_guitar_display_panel"));
  CHECK(multi_guitar_screen != nullptr);
  CHECK(multi_guitar_panel != nullptr);
  CHECK(multi_guitar_display != nullptr);
  if (multi_guitar_screen && multi_guitar_display) {
    CHECK(multi_guitar_screen->name() == Symbol("multi_sel_guitar_screen"));
    CHECK(multi_guitar_display->get_property(Symbol("guitar_0"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lespaul"));
    CHECK(multi_guitar_display->get_property(Symbol("guitar_1"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lespaul"));
    Object* p1_proxy =
        multi_guitar_display->get_property(Symbol("guitar_proxy_0")).as_object();
    Object* p2_proxy =
        multi_guitar_display->get_property(Symbol("guitar_proxy_1")).as_object();
    Object* p1_filter =
        multi_guitar_display->get_property(Symbol("guitar_filter_0")).as_object();
    Object* p2_filter =
        multi_guitar_display->get_property(Symbol("guitar_filter_1")).as_object();
    Object* p1_env =
        multi_guitar_display->get_property(Symbol("guitar_display_env_0")).as_object();
    Object* p2_env =
        multi_guitar_display->get_property(Symbol("guitar_display_env_1")).as_object();
    CHECK(p1_proxy != nullptr &&
          p1_proxy->name() == Symbol("guitar_multi0.pxy"));
    CHECK(p2_proxy != nullptr &&
          p2_proxy->name() == Symbol("guitar_multi1.pxy"));
    CHECK(p1_filter != nullptr &&
          p1_filter->name() == Symbol("guitar_multi0.filt"));
    CHECK(p2_filter != nullptr &&
          p2_filter->name() == Symbol("guitar_multi1.filt"));
    CHECK(p1_env != nullptr && p1_env->name() == Symbol("guitar01.env"));
    CHECK(p2_env != nullptr && p2_env->name() == Symbol("guitar02.env"));
  }

  auto check_multi_venue_screen = [&](Symbol screen_name, Symbol next_screen) {
    mgr.goto_screen(screen_name);
    Object* screen = mgr.current_screen();
    CHECK(screen != nullptr);
    if (!screen) return;
    CHECK(screen->name() == screen_name);
    CHECK(array_contains_symbol(screen->get_property(Symbol("panels")),
                                Symbol("sel_venue_panel")));
    CHECK(screen->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("sel_venue_panel"));
    Object* venue_panel = mgr.find_object(Symbol("sel_venue_panel"));
    CHECK(venue_panel != nullptr);
    if (venue_panel) {
      CHECK(venue_panel->get_property(Symbol("focus"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("sv_small2.btn"));
      auto* dir = dynamic_cast<ObjectDir*>(venue_panel);
      Object* small2_button = dir ? dir->find(Symbol("sv_small2.btn")) : nullptr;
      CHECK(small2_button != nullptr);
      CHECK(small2_button &&
            small2_button->get_property(Symbol("disabled"))
                .as_symbol()
                .value_or(Symbol("TRUE")) == Symbol("FALSE"));
    }
    CHECK(screen->get_property(Symbol("next_screen"))
              .as_symbol()
              .value_or(Symbol()) == next_screen);
  };
  check_multi_venue_screen(Symbol("multi_vs_venue_screen"),
                           Symbol("multi_vs_selsong_screen"));
  check_multi_venue_screen(Symbol("multi_coop_venue_screen"),
                           Symbol("multi_coop_selsong_screen"));
  check_multi_venue_screen(Symbol("multi_fo_venue_screen"),
                           Symbol("multi_fo_selsong_screen"));

  if (options_panel && data_button && memory_button) {
    mgr.goto_screen(Symbol("options_screen"));
    mgr.set_global(Symbol("disable_save"), DataNode::Sym(Symbol("TRUE")));
    mgr.update(0.0f);
    CHECK(data_button->get_property(Symbol("state"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("kDisabled"));
    CHECK(data_button->get_property(Symbol("disabled"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("TRUE"));
    CHECK(memory_button->get_property(Symbol("state"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("kDisabled"));
    CHECK(memory_button->get_property(Symbol("disabled"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("TRUE"));
    mgr.set_global(Symbol("disable_save"), DataNode::Sym(Symbol("FALSE")));
  }

  mgr.set_global(Symbol("autosave"), DataNode::Sym(Symbol("TRUE")));
  mgr.goto_screen(Symbol("mem_card_screen"));
  Object* mem_card_screen = mgr.current_screen();
  Object* mem_card_panel = mgr.find_object(Symbol("mem_card_panel"));
  Object* autosave_button = mgr.resolve_object(Symbol("autosave.btn"));
  Object* autosave_check = mgr.resolve_object(Symbol("autosave.chk"));
  Object* save_bands = mgr.resolve_object(Symbol("save_bands.btn"));
  Object* load_bands = mgr.resolve_object(Symbol("load_bands.btn"));
  CHECK(mem_card_screen != nullptr);
  CHECK(mem_card_panel != nullptr);
  CHECK(autosave_button != nullptr);
  CHECK(autosave_check != nullptr);
  CHECK(save_bands != nullptr);
  CHECK(load_bands != nullptr);
  if (mem_card_screen && mem_card_panel && autosave_button &&
      autosave_check && save_bands && load_bands) {
    CHECK(mem_card_screen->name() == Symbol("mem_card_screen"));
    CHECK(mem_card_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("save_bands.btn"));
    CHECK(autosave_check->handle_property(Symbol("get_check"), DataArray())
              .as_int()
              .value_or(-1) == 1);

    mgr.set_global(Symbol("component"), DataNode::Obj(autosave_button));
    mem_card_screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(autosave_check->handle_property(Symbol("get_check"), DataArray())
              .as_int()
              .value_or(-1) == 0);
    CHECK(mgr.get_global(Symbol("autosave")).as_int().value_or(-1) == 0);

    mgr.set_global(Symbol("component"), DataNode::Obj(save_bands));
    mem_card_screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("memcard_save_confirm"));
    CHECK(mgr.get_global(Symbol("mc_save_success_screen")).as_object() ==
          mem_card_screen);
    CHECK(mgr.get_global(Symbol("mc_save_failed_screen")).as_object() ==
          mem_card_screen);

    mgr.goto_screen(Symbol("mem_card_screen"));
    mem_card_screen = mgr.current_screen();
    mgr.set_global(Symbol("component"), DataNode::Obj(load_bands));
    mem_card_screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("memcard_load_confirm"));
    CHECK(mgr.get_global(Symbol("mc_post_load_screen")).as_object() ==
          mem_card_screen);
    CHECK(mgr.get_global(Symbol("mc_load_failed_screen")).as_object() ==
          mem_card_screen);
  }

  mgr.goto_screen(Symbol("store_screen"));
  Object* store_screen = mgr.current_screen();
  Object* store_panel = mgr.find_object(Symbol("store_panel"));
  Object* st_screen1 = mgr.resolve_object(Symbol("st_screen1.view"));
  Object* st_screen2 = mgr.resolve_object(Symbol("st_screen2.view"));
  Object* st_guitars = mgr.resolve_object(Symbol("st_guitars.btn"));
  Object* store_helpbar = mgr.resolve_object(Symbol("helpbar"));
  Object* store_guitar_provider = mgr.resolve_object(Symbol("store_guitar_provider"));
  Object* store_guitar_display =
      mgr.find_object(Symbol("store_guitar_display_panel"));
  Object* store_cash = mgr.resolve_object(Symbol("st_cash.lbl"));
  Object* store_cash_view = mgr.resolve_object(Symbol("cash.view"));
  CHECK(store_screen != nullptr);
  CHECK(store_panel != nullptr);
  CHECK(st_screen1 != nullptr);
  CHECK(st_screen2 != nullptr);
  CHECK(st_guitars != nullptr);
  CHECK(store_helpbar != nullptr);
  CHECK(store_guitar_provider != nullptr);
  CHECK(store_guitar_display != nullptr);
  CHECK(store_cash != nullptr);
  CHECK(store_cash_view != nullptr);
  if (Object* play_sfx = mgr.resolve_object(Symbol("play_sfx"))) {
    CHECK(play_sfx->get_property(Symbol("last_played"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("store_grate.cue"));
  } else {
    CHECK(false);
  }
  if (store_screen && store_panel && st_screen1 && st_screen2 && st_guitars) {
    CHECK(store_screen->name() == Symbol("store_screen"));
    CHECK(store_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("st_guitars.btn"));
    CHECK(st_screen1->handle_property(Symbol("showing"), DataArray())
              .as_symbol()
              .value_or(Symbol("FALSE")) == Symbol("TRUE"));
    CHECK(st_screen2->handle_property(Symbol("showing"), DataArray())
              .as_symbol()
              .value_or(Symbol("TRUE")) == Symbol("FALSE"));
    if (store_helpbar) {
      CHECK(array_contains_symbol(store_helpbar->get_property(Symbol("display")),
                                  Symbol("help_select")));
      CHECK(array_contains_symbol(store_helpbar->get_property(Symbol("display")),
                                  Symbol("help_back")));
      CHECK(array_contains_symbol(store_helpbar->get_property(Symbol("display")),
                                  Symbol("help_updown")));
    }
    if (store_cash) {
      CHECK(store_cash->get_property(Symbol("text"))
                .as_string()
                .value_or("") == "CASH: $0");
    }
    if (store_cash_view) {
      CHECK(truthy(store_cash_view->handle_property(Symbol("showing"),
                                                    DataArray())));
    }

    store_panel->handle_property(Symbol("show_store_screen_2"), DataArray());
    CHECK(st_screen1->handle_property(Symbol("showing"), DataArray())
              .as_symbol()
              .value_or(Symbol("TRUE")) == Symbol("FALSE"));
    CHECK(st_screen2->handle_property(Symbol("showing"), DataArray())
              .as_symbol()
              .value_or(Symbol("FALSE")) == Symbol("TRUE"));
    CHECK(store_panel->get_property(Symbol("itemIdx")).as_int().value_or(-1) == 0);
    if (store_helpbar) {
      CHECK(!array_contains_symbol(store_helpbar->get_property(Symbol("display")),
                                   Symbol("help_select")));
      CHECK(array_contains_symbol(store_helpbar->get_property(Symbol("display")),
                                  Symbol("help_back")));
      CHECK(array_contains_symbol(store_helpbar->get_property(Symbol("display")),
                                  Symbol("help_updown")));
    }
    Symbol first_store_guitar = db.store_item(Symbol("guitar"), 0);
    CHECK(first_store_guitar.valid());
    if (store_guitar_provider && first_store_guitar.valid()) {
      DataArray first_index;
      first_index.push(DataNode::Int(0));
      CHECK(store_guitar_provider->handle_property(Symbol("get_symbol"),
                                                   first_index)
                .as_symbol()
                .value_or(Symbol()) == first_store_guitar);
    }
    auto expected_store_price = [&](Symbol category, Symbol item) {
      return db.store_field(category, item, Symbol("price")).as_int().value_or(0);
    };
    auto expected_cost_extreme = [&](Symbol category, bool high) {
      bool has_price = false;
      int best = 0;
      const std::size_t count = db.store_item_count(category);
      for (std::size_t i = 0; i < count; ++i) {
        const Symbol item = db.store_item(category, i);
        const int price = expected_store_price(category, item);
        if (!has_price || (high ? price > best : price < best)) {
          best = price;
          has_price = true;
        }
      }
      return has_price ? best : 0;
    };
    DataArray guitar_category;
    guitar_category.push(DataNode::Sym(Symbol("guitar")));
    CHECK(store_panel->handle_property(Symbol("low_cost"), guitar_category)
              .as_int()
              .value_or(-1) == expected_cost_extreme(Symbol("guitar"), false));
    CHECK(store_panel->handle_property(Symbol("high_cost"), guitar_category)
              .as_int()
              .value_or(-1) == expected_cost_extreme(Symbol("guitar"), true));
    if (first_store_guitar.valid()) {
      DataArray price_args;
      price_args.push(DataNode::Sym(Symbol("guitar")));
      price_args.push(DataNode::Sym(first_store_guitar));
      CHECK(store_panel->handle_property(Symbol("price"), price_args)
                .as_int()
                .value_or(-1) ==
            expected_store_price(Symbol("guitar"), first_store_guitar));
    }
    mgr.set_global(Symbol("new_focus"), DataNode::Obj(st_guitars));
    store_panel->handle_property(Symbol("FOCUS_MSG"), DataArray());
    CHECK(store_panel->get_property(Symbol("category"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("guitar"));
    mgr.set_global(Symbol("component"), DataNode::Obj(st_guitars));
    store_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    if (store_guitar_display && first_store_guitar.valid()) {
      CHECK(truthy(store_guitar_display->handle_property(Symbol("showing"),
                                                         DataArray())));
      CHECK(store_guitar_display->get_property(Symbol("guitar"))
                .as_symbol()
                .value_or(Symbol()) == first_store_guitar);
      CHECK(!store_guitar_display->get_property(Symbol("guitar_skin"))
                 .as_symbol()
                 .value_or(Symbol())
                 .valid());
      Object* proxy =
          store_guitar_display->get_property(Symbol("guitar_proxy")).as_object();
      Object* filter =
          store_guitar_display->get_property(Symbol("guitar_filter")).as_object();
      Object* env =
          store_guitar_display->get_property(Symbol("guitar_display_env")).as_object();
      CHECK(proxy != nullptr && proxy->name() == Symbol("guitar.pxy"));
      CHECK(filter != nullptr && filter->name() == Symbol("guitar_single.filt"));
      CHECK(env != nullptr && env->name() == Symbol("guitar.env"));
    }

    store_panel->handle_property(Symbol("show_store_screen_1"), DataArray());
    CHECK(st_screen1->handle_property(Symbol("showing"), DataArray())
              .as_symbol()
              .value_or(Symbol("FALSE")) == Symbol("TRUE"));
    CHECK(st_screen2->handle_property(Symbol("showing"), DataArray())
              .as_symbol()
              .value_or(Symbol("TRUE")) == Symbol("FALSE"));
  }

  mgr.goto_screen(Symbol("qp_diff_screen"));
  Object* qp_diff_screen = mgr.current_screen();
  Object* qp_diff_panel = mgr.find_object(Symbol("sel_difficulty_panel"));
  Object* diff_hard = mgr.resolve_object(Symbol("sd_diff3.btn"));
  Object* diff_expert = mgr.resolve_object(Symbol("sd_diff4.btn"));
  CHECK(qp_diff_screen != nullptr);
  CHECK(qp_diff_panel != nullptr);
  CHECK(diff_hard != nullptr);
  CHECK(diff_expert != nullptr);
  if (qp_diff_screen && qp_diff_panel && diff_expert) {
    CHECK(qp_diff_screen->name() == Symbol("qp_diff_screen"));
    CHECK(qp_diff_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("sd_diff2.btn"));
    mgr.set_global(Symbol("component"), DataNode::Obj(diff_expert));
    qp_diff_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    if (Object* player0 = mgr.resolve_object(Symbol("player0"))) {
      CHECK(player0->get_property(Symbol("difficulty"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("kDifficultyExpert"));
    }
    qp_diff_screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("loading_screen"));
    if (Object* loading_screen = mgr.current_screen()) {
      CHECK(loading_screen->get_property(Symbol("allow_back"))
                .as_symbol()
                .value_or(Symbol("TRUE")) == Symbol("FALSE"));
      CHECK(loading_screen->get_property(Symbol("animate_transition"))
                .as_symbol()
                .value_or(Symbol("TRUE")) == Symbol("FALSE"));
    }
    if (Object* tip_lbl = mgr.resolve_object(Symbol("tip.lbl"))) {
      CHECK(tip_lbl->get_property(Symbol("text"))
                .as_string()
                .value_or("") == mgr.localize(Symbol("loading_tip1")));
    } else {
      CHECK(false);
    }
    if (Object* game = mgr.resolve_object(Symbol("game"))) {
      CHECK(game->get_property(Symbol("mode"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("quickplay"));
    }
    if (Object* loading_screen = mgr.current_screen()) {
      loading_screen->handle_property(Symbol("TRANSITION_COMPLETE_MSG"),
                                      DataArray());
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("game_screen"));
    }
  }

  if (Object* game = mgr.resolve_object(Symbol("game"))) {
    DataArray medium_arg;
    medium_arg.push(DataNode::Sym(Symbol("kDifficultyMedium")));
    game->handle_property(Symbol("set_difficulty"), medium_arg);
  }
  mgr.goto_screen(Symbol("sel_difficulty_screen"));
  Object* career_diff_screen = mgr.current_screen();
  Object* career_diff_panel = mgr.find_object(Symbol("sel_diff_career_panel"));
  Object* diff_easy = mgr.resolve_object(Symbol("sd_diff1.btn"));
  CHECK(career_diff_screen != nullptr);
  CHECK(career_diff_panel != nullptr);
  CHECK(diff_easy != nullptr);
  if (career_diff_screen && career_diff_panel && diff_easy) {
    CHECK(career_diff_screen->name() == Symbol("sel_difficulty_screen"));
    CHECK(career_diff_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("sd_diff2.btn"));
    if (Object* easy_status = mgr.resolve_object(Symbol("sd_easy_status.lbl"))) {
      CHECK(easy_status->get_property(Symbol("text"))
                .as_string()
                .value_or("") == "0%");
    } else {
      CHECK(false);
    }
    mgr.set_global(Symbol("component"), DataNode::Obj(diff_easy));
    career_diff_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    if (Object* campaign = mgr.resolve_object(Symbol("campaign"))) {
      CHECK(campaign->handle_property(Symbol("last_difficulty"), DataArray())
                .as_symbol()
                .value_or(Symbol()) == Symbol("kDifficultyEasy"));
    }
    if (diff_hard) {
      mgr.set_global(Symbol("component"), DataNode::Obj(diff_hard));
      career_diff_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
      if (Object* campaign = mgr.resolve_object(Symbol("campaign"))) {
        CHECK(campaign->handle_property(Symbol("last_difficulty"), DataArray())
                  .as_symbol()
                  .value_or(Symbol()) == Symbol("kDifficultyHard"));
      }
    }
  }

  mgr.goto_screen(Symbol("training_screen"));
  Object* training_panel = mgr.find_object(Symbol("training_panel"));
  CHECK(training_panel != nullptr);
  Object* tutorials_button = mgr.resolve_object(Symbol("tutorials.btn"));
  Object* practice_button = mgr.resolve_object(Symbol("practice.btn"));
  CHECK(tutorials_button != nullptr);
  CHECK(practice_button != nullptr);
  if (training_panel && tutorials_button && practice_button) {
    CHECK(training_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("tutorials.btn"));
    mgr.update(0.0f);
    CHECK(tutorials_button->get_property(Symbol("disabled"))
              .as_symbol()
              .value_or(Symbol("FALSE")) == Symbol("FALSE"));

    mgr.set_global(Symbol("component"), DataNode::Obj(tutorials_button));
    training_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("tutorials_screen"));
    Object* tutorials_panel = mgr.find_object(Symbol("tutorials_panel"));
    CHECK(tutorials_panel != nullptr);
    if (tutorials_panel) {
      CHECK(tutorials_panel->get_property(Symbol("file"))
                .as_string()
                .value_or("") == "tutorials.milo");
      CHECK(mgr.resolve_object(Symbol("tut_1.btn")) != nullptr);
      CHECK(mgr.resolve_object(Symbol("tut_blurb.lbl")) != nullptr);
      Object* tut1 = mgr.resolve_object(Symbol("tut_1.btn"));
      if (tut1) {
        mgr.set_global(Symbol("component"), DataNode::Obj(tut1));
        tutorials_panel->handle_property(Symbol("SELECT_START_MSG"),
                                         DataArray());
        CHECK(mgr.current_screen() != nullptr &&
              mgr.current_screen()->name() == Symbol("tut_script_screen"));
        CHECK(tutorials_panel->get_property(Symbol("start_state"))
                  .as_symbol()
                  .value_or(Symbol()) == Symbol("intro"));
        CHECK(tutorials_panel->get_property(Symbol("tut_song"))
                  .as_symbol()
                  .value_or(Symbol()) == Symbol("tutorial102"));
        if (Object* campaign = mgr.resolve_object(Symbol("campaign"))) {
          CHECK(campaign->get_property(Symbol("tutorial"))
                    .as_int()
                    .value_or(-1) == 1);
        }
        if (Object* game = mgr.resolve_object(Symbol("game"))) {
          CHECK(game->get_property(Symbol("song"))
                    .as_symbol()
                    .value_or(Symbol()) == Symbol("tutorial102"));
        }
        if (Object* player0 = mgr.resolve_object(Symbol("player0"))) {
          CHECK(player0->get_property(Symbol("difficulty"))
                    .as_symbol()
                    .value_or(Symbol()) == Symbol("kDifficultyEasy"));
          CHECK(player0->get_property(Symbol("track_type"))
                    .as_symbol()
                    .value_or(Symbol()) == Symbol("guitar"));
        }
      }
    }

    mgr.goto_screen(Symbol("training_screen"));
    mgr.set_global(Symbol("component"), DataNode::Obj(practice_button));
    training_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("practice_selsong_screen"));
    if (Object* game = mgr.resolve_object(Symbol("game"))) {
      CHECK(game->get_property(Symbol("mode"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("practice"));
    }
  }

  Object* section_provider = mgr.resolve_object(Symbol("section_provider"));
  CHECK(section_provider != nullptr);
  if (section_provider) {
    const int section_count =
        section_provider->handle_property(Symbol("list_length"), DataArray())
            .as_int()
            .value_or(0);
    CHECK(section_count > 1);
    DataArray first_section_arg;
    first_section_arg.push(DataNode::Int(0));
    CHECK(section_provider->handle_property(Symbol("get_symbol"), first_section_arg)
              .as_symbol()
              .value_or(Symbol()) == Symbol("full_song"));
  }

  mgr.goto_screen(Symbol("practice_sel_section_screen"));
  Object* section_panel = mgr.find_object(Symbol("practice_sel_section_panel"));
  Object* section_list = mgr.resolve_object(Symbol("sel_section.lst"));
  CHECK(section_panel != nullptr);
  CHECK(section_list != nullptr);
  if (section_panel && section_list && section_provider) {
    CHECK(section_list->get_property(Symbol("provider")).as_object() ==
          section_provider);
    DataArray second_row;
    second_row.push(DataNode::Int(2));
    section_list->handle_property(Symbol("set_selected"), second_row);
    section_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(section_panel->get_property(Symbol("section")).as_int().value_or(-1) == 1);
    CHECK(section_provider->get_property(Symbol("start_section"))
              .as_int()
              .value_or(-1) == 1);
    if (Object* gamecfg = mgr.resolve_object(Symbol("gamecfg"))) {
      auto key = std::make_shared<DataArray>();
      key->push(DataNode::Sym(Symbol("practice_sections")));
      key->push(DataNode::Int(0));
      DataArray get_section_start;
      get_section_start.push(DataNode::Array(key));
      CHECK(gamecfg->handle_property(Symbol("get"), get_section_start)
                .as_int()
                .value_or(-1) == 1);
    }
  }

  mgr.goto_screen(Symbol("practice_sel_speed_screen"));
  Object* speed_screen = mgr.current_screen();
  Object* speed_panel = mgr.find_object(Symbol("sel_speed_panel"));
  Object* speed2_button = mgr.resolve_object(Symbol("speed2.btn"));
  CHECK(speed_screen != nullptr);
  CHECK(speed_panel != nullptr);
  CHECK(speed2_button != nullptr);
  if (speed_screen && speed_panel && speed2_button) {
    CHECK(speed_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("speed0.btn"));
    mgr.set_global(Symbol("component"), DataNode::Obj(speed2_button));
    speed_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    if (Object* gamecfg = mgr.resolve_object(Symbol("gamecfg"))) {
      DataArray get_speed;
      get_speed.push(DataNode::Sym(Symbol("practice_speed")));
      CHECK(gamecfg->handle_property(Symbol("get"), get_speed)
                .as_int()
                .value_or(-1) == 2);
    }
    speed_screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("practice_loading_screen"));
    if (Object* practice_loading = mgr.current_screen()) {
      CHECK(practice_loading->get_property(Symbol("allow_back"))
                .as_symbol()
                .value_or(Symbol("TRUE")) == Symbol("FALSE"));
      CHECK(practice_loading->get_property(Symbol("animate_transition"))
                .as_symbol()
                .value_or(Symbol("TRUE")) == Symbol("FALSE"));
      practice_loading->handle_property(Symbol("TRANSITION_COMPLETE_MSG"),
                                        DataArray());
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("practice_game_screen"));
    }
  }

  if (Object* options = mgr.resolve_object(Symbol("options"))) {
    DataArray set_lefty_p1_off;
    set_lefty_p1_off.push(DataNode::Int(0));
    set_lefty_p1_off.push(DataNode::Int(0));
    options->handle_property(Symbol("set_lefty"), set_lefty_p1_off);
    DataArray set_lefty_p2_off;
    set_lefty_p2_off.push(DataNode::Int(1));
    set_lefty_p2_off.push(DataNode::Int(0));
    options->handle_property(Symbol("set_lefty"), set_lefty_p2_off);
    DataArray set_wide_off;
    set_wide_off.push(DataNode::Int(0));
    options->handle_property(Symbol("set_widescreen"), set_wide_off);
    DataArray set_pscan_off;
    set_pscan_off.push(DataNode::Int(0));
    options->handle_property(Symbol("set_pscan"), set_pscan_off);
  }
  mgr.goto_screen(Symbol("video_settings_screen"));
  Object* video_screen = mgr.current_screen();
  Object* video_panel = mgr.find_object(Symbol("video_settings_panel"));
  Object* video_helpbar = mgr.resolve_object(Symbol("helpbar"));
  CHECK(video_screen != nullptr &&
        video_screen->name() == Symbol("video_settings_screen"));
  CHECK(video_panel != nullptr);
  Object* video_lefty_p1_button = mgr.resolve_object(Symbol("gs_left_p1.btn"));
  Object* video_lefty_p2_button = mgr.resolve_object(Symbol("gs_left_p2.btn"));
  Object* video_widescreen_button =
      mgr.resolve_object(Symbol("widescreen.btn"));
  Object* video_pscan_button = mgr.resolve_object(Symbol("p_scan.btn"));
  Object* video_calibrate_button =
      mgr.resolve_object(Symbol("calibrate_lag.btn"));
  Object* video_lefty1_check = mgr.resolve_object(Symbol("lefty1.chk"));
  Object* video_lefty2_check = mgr.resolve_object(Symbol("lefty2.chk"));
  Object* video_widescreen_check =
      mgr.resolve_object(Symbol("widescreen.chk"));
  Object* video_pscan_check = mgr.resolve_object(Symbol("p_scan.chk"));
  CHECK(video_lefty_p1_button != nullptr);
  CHECK(video_lefty_p2_button != nullptr);
  CHECK(video_widescreen_button != nullptr);
  CHECK(video_pscan_button != nullptr);
  CHECK(video_calibrate_button != nullptr);
  CHECK(video_lefty1_check != nullptr);
  CHECK(video_lefty2_check != nullptr);
  CHECK(video_widescreen_check != nullptr);
  CHECK(video_pscan_check != nullptr);
  if (video_screen && video_panel && video_lefty_p1_button &&
      video_lefty_p2_button && video_widescreen_button && video_pscan_button &&
      video_calibrate_button && video_lefty1_check && video_lefty2_check &&
      video_widescreen_check && video_pscan_check) {
    CHECK(video_screen->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("video_settings_panel"));
    CHECK(video_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("gs_left_p1.btn"));
    CHECK(video_lefty_p2_button->get_property(Symbol("disabled"))
              .as_symbol()
              .value_or(Symbol("FALSE")) != Symbol("TRUE"));
    CHECK(video_lefty1_check->handle_property(Symbol("get_check"),
                                              DataArray())
              .as_int()
              .value_or(-1) == 0);
    CHECK(video_lefty2_check->handle_property(Symbol("get_check"),
                                              DataArray())
              .as_int()
              .value_or(-1) == 0);
    CHECK(video_widescreen_check->handle_property(Symbol("get_check"),
                                                  DataArray())
              .as_int()
              .value_or(-1) == 0);
    CHECK(video_pscan_check->handle_property(Symbol("get_check"),
                                             DataArray())
              .as_int()
              .value_or(-1) == 0);

    mgr.set_global(Symbol("new_focus"), DataNode::Obj(video_calibrate_button));
    video_panel->handle_property(Symbol("FOCUS_MSG"), DataArray());
    if (video_helpbar) {
      CHECK(array_contains_symbol(video_helpbar->get_property(Symbol("display")),
                                  Symbol("help_select")));
      CHECK(!array_contains_symbol(
          video_helpbar->get_property(Symbol("display")), Symbol("help_onoff")));
    }
    mgr.set_global(Symbol("new_focus"), DataNode::Obj(video_widescreen_button));
    video_panel->handle_property(Symbol("FOCUS_MSG"), DataArray());
    if (video_helpbar) {
      CHECK(array_contains_symbol(video_helpbar->get_property(Symbol("display")),
                                  Symbol("help_onoff")));
      CHECK(!array_contains_symbol(
          video_helpbar->get_property(Symbol("display")), Symbol("help_select")));
    }

    mgr.set_global(Symbol("component"), DataNode::Obj(video_lefty_p1_button));
    video_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(video_lefty1_check->handle_property(Symbol("get_check"),
                                              DataArray())
              .as_int()
              .value_or(-1) == 1);
    if (Object* options = mgr.resolve_object(Symbol("options"))) {
      DataArray get_p1;
      get_p1.push(DataNode::Int(0));
      CHECK(options->handle_property(Symbol("get_lefty"), get_p1)
                .as_int()
                .value_or(-1) == 1);
    }
    mgr.set_global(Symbol("component"), DataNode::Obj(video_lefty_p2_button));
    video_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(video_lefty2_check->handle_property(Symbol("get_check"),
                                              DataArray())
              .as_int()
              .value_or(-1) == 1);
    if (Object* options = mgr.resolve_object(Symbol("options"))) {
      DataArray get_p2;
      get_p2.push(DataNode::Int(1));
      CHECK(options->handle_property(Symbol("get_lefty"), get_p2)
                .as_int()
                .value_or(-1) == 1);
    }
    mgr.set_global(Symbol("component"), DataNode::Obj(video_widescreen_button));
    video_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(video_widescreen_check->handle_property(Symbol("get_check"),
                                                  DataArray())
              .as_int()
              .value_or(-1) == 1);
    if (Object* options = mgr.resolve_object(Symbol("options"))) {
      CHECK(options->handle_property(Symbol("get_widescreen"), DataArray())
                .as_int()
                .value_or(-1) == 1);
    }
    mgr.set_global(Symbol("component"), DataNode::Obj(video_pscan_button));
    video_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("pscan_warning_screen"));
    Object* video_pscan_cancel = mgr.resolve_object(Symbol("pscan_cancel.btn"));
    CHECK(video_pscan_cancel != nullptr);
    if (video_pscan_cancel) {
      mgr.set_global(Symbol("component"), DataNode::Obj(video_pscan_cancel));
      mgr.current_screen()->handle_property(Symbol("SELECT_START_MSG"),
                                            DataArray());
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("video_settings_screen"));
    }
  }

  if (Object* options = mgr.resolve_object(Symbol("options"))) {
    DataArray pscan_true;
    pscan_true.push(DataNode::Int(1));
    options->handle_property(Symbol("set_pscan"), pscan_true);
  }
  mgr.goto_screen(Symbol("options_screen"));
  mgr.goto_screen(Symbol("video_settings_screen"));
  video_panel = mgr.find_object(Symbol("video_settings_panel"));
  video_pscan_button = mgr.resolve_object(Symbol("p_scan.btn"));
  video_pscan_check = mgr.resolve_object(Symbol("p_scan.chk"));
  CHECK(video_panel != nullptr);
  CHECK(video_pscan_button != nullptr);
  CHECK(video_pscan_check != nullptr);
  if (video_panel && video_pscan_button && video_pscan_check) {
    CHECK(video_pscan_check->handle_property(Symbol("get_check"),
                                             DataArray())
              .as_int()
              .value_or(-1) == 1);
    mgr.set_global(Symbol("component"), DataNode::Obj(video_pscan_button));
    video_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("video_settings_screen"));
    CHECK(video_pscan_check->handle_property(Symbol("get_check"),
                                             DataArray())
              .as_int()
              .value_or(-1) == 0);
    if (Object* options = mgr.resolve_object(Symbol("options"))) {
      CHECK(options->handle_property(Symbol("get_pscan"), DataArray())
                .as_int()
                .value_or(-1) == 0);
    }
  }

  mgr.goto_screen(Symbol("video_settings_screen"));
  video_panel = mgr.find_object(Symbol("video_settings_panel"));
  video_calibrate_button = mgr.resolve_object(Symbol("calibrate_lag.btn"));
  CHECK(video_panel != nullptr);
  CHECK(video_calibrate_button != nullptr);
  if (video_panel && video_calibrate_button) {
    mgr.set_global(Symbol("component"), DataNode::Obj(video_calibrate_button));
    video_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("lag_screen"));
    Object* routed_lag_panel = mgr.find_object(Symbol("lag_panel"));
    CHECK(routed_lag_panel != nullptr);
    if (routed_lag_panel) {
      DataNode from_panel = routed_lag_panel->get_property(Symbol("from_panel"));
      Object* from_panel_obj = from_panel.as_object();
      CHECK(from_panel_obj != nullptr &&
            from_panel_obj->name() == Symbol("video_settings_panel"));
      mgr.set_global(Symbol("button"), DataNode::Sym(Symbol("kPad_Tri")));
      routed_lag_panel->handle_property(Symbol("BUTTON_DOWN_MSG"),
                                        DataArray());
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("video_settings_screen"));
    }
  }

  mgr.goto_screen(Symbol("video_settings_screen"));
  video_screen = mgr.current_screen();
  CHECK(video_screen != nullptr &&
        video_screen->name() == Symbol("video_settings_screen"));
  if (video_screen) {
    mgr.set_global(Symbol("button"), DataNode::Sym(Symbol("kPad_Tri")));
    video_screen->handle_property(Symbol("BUTTON_DOWN_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("options_screen"));
  }

  if (Object* options = mgr.resolve_object(Symbol("options"))) {
    DataArray set_sync;
    set_sync.push(DataNode::Int(12));
    options->handle_property(Symbol("set_sync_offset"), set_sync);
  }
  mgr.goto_screen(Symbol("lag_screen"));
  Object* lag_panel = mgr.find_object(Symbol("lag_panel"));
  Object* lag_instructions = mgr.resolve_object(Symbol("instructions.lbl"));
  Object* lag_instructions2 = mgr.resolve_object(Symbol("instructions2.lbl"));
  Object* lag_setting = mgr.resolve_object(Symbol("setting.lbl"));
  Object* lag_countdown = mgr.resolve_object(Symbol("countdown.lbl"));
  Object* lag_auto = mgr.resolve_object(Symbol("autocalibrate.btn"));
  Object* lag_reset = mgr.resolve_object(Symbol("reset_to_zero.btn"));
  Object* lag_buttons = mgr.resolve_object(Symbol("buttons.grp"));
  Object* lag_helpbar = mgr.resolve_object(Symbol("helpbar"));
  Object* sync_click = mgr.resolve_object(Symbol("sync_click.cue"));
  Object* practice_hat = mgr.resolve_object(Symbol("practice_hat"));
  CHECK(lag_panel != nullptr);
  CHECK(lag_instructions != nullptr);
  CHECK(lag_instructions2 != nullptr);
  CHECK(lag_setting != nullptr);
  CHECK(lag_countdown != nullptr);
  CHECK(lag_auto != nullptr);
  CHECK(lag_reset != nullptr);
  CHECK(lag_buttons != nullptr);
  CHECK(sync_click != nullptr);
  CHECK(practice_hat != nullptr);
  if (auto* lag_ui = dynamic_cast<ui::UiObject*>(lag_panel))
    CHECK(lag_ui->has_handler(Symbol("SELECT_START_MSG")));
  if (lag_panel && lag_instructions && lag_instructions2 && lag_setting &&
      lag_countdown && lag_auto && lag_reset && lag_buttons) {
    CHECK(lag_panel->get_property(Symbol("state"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("init"));
    CHECK(lag_panel->get_property(Symbol("lag")).as_int().value_or(999) ==
          -12);
    CHECK(lag_instructions->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lag_info_why"));
    CHECK(lag_instructions2->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lag_info_howto"));
    const std::string setting_text(
        lag_setting->get_property(Symbol("text")).as_string().value_or(""));
    CHECK(setting_text.find("%D") == std::string::npos);
    CHECK(setting_text.find("-12") != std::string::npos);
    CHECK(setting_text.find("ms") != std::string::npos);
    CHECK(lag_auto->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lag_button_calibrate"));
    CHECK(lag_countdown->get_property(Symbol("text"))
              .as_string()
              .value_or("not-cleared") == "");
    CHECK(truthy(lag_buttons->get_property(Symbol("showing"))));
    if (lag_helpbar) {
      CHECK(array_contains_symbol(lag_helpbar->get_property(Symbol("display")),
                                  Symbol("help_select")));
      CHECK(array_contains_symbol(lag_helpbar->get_property(Symbol("display")),
                                  Symbol("help_back")));
    }

    DataArray calibrating;
    calibrating.push(DataNode::Sym(Symbol("calibrating")));
    lag_panel->handle_property(Symbol("set_state"), calibrating);
    CHECK(lag_panel->get_property(Symbol("state"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("calibrating"));
    CHECK(!truthy(lag_buttons->get_property(Symbol("showing"))));
    CHECK(lag_instructions->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lag_measuring"));
    CHECK(lag_instructions2->get_property(Symbol("text"))
              .as_string()
              .value_or("") == "");
    CHECK(lag_setting->get_property(Symbol("text")).as_string().value_or("") ==
          "");
    if (lag_helpbar) {
      CHECK(array_contains_symbol(lag_helpbar->get_property(Symbol("display")),
                                  Symbol("help_hitonchange")));
      CHECK(!array_contains_symbol(lag_helpbar->get_property(Symbol("display")),
                                   Symbol("help_select")));
    }

    mgr.set_global(Symbol("button"), DataNode::Sym(Symbol("kPad_DDown")));
    lag_panel->handle_property(Symbol("BUTTON_DOWN_MSG"), DataArray());
    auto hits = lag_panel->get_property(Symbol("hits")).as_array();
    CHECK(hits && hits->size() == 1);

    lag_panel->set_property(Symbol("lag"), DataNode::Int(44));
    CHECK(lag_reset->name() == Symbol("reset_to_zero.btn"));
    mgr.set_global(Symbol("component"), DataNode::Obj(lag_reset));
    CHECK(mgr.get_global(Symbol("component")).as_object() == lag_reset);
    lag_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(near(lag_panel->get_property(Symbol("lag"))
                   .as_float()
                   .value_or(-1.0f),
               0.0f));
    CHECK(lag_panel->get_property(Symbol("state"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("init"));
    CHECK(truthy(lag_buttons->get_property(Symbol("showing"))));

    mgr.set_global(Symbol("component"), DataNode::Obj(lag_auto));
    lag_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(lag_panel->get_property(Symbol("state"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("calibrating"));
    mgr.update(0.0f);
    CHECK(lag_countdown->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lag_3"));
    if (practice_hat) {
      CHECK(practice_hat->get_property(Symbol("play_count"))
                .as_int()
                .value_or(0) == 1);
    }
    mgr.update(0.734f);
    CHECK(lag_countdown->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lag_2"));
    if (practice_hat) {
      CHECK(practice_hat->get_property(Symbol("play_count"))
                .as_int()
                .value_or(0) == 2);
    }
    mgr.update(2.3f);
    if (sync_click) {
      CHECK(sync_click->get_property(Symbol("play_count"))
                .as_int()
                .value_or(0) >= 1);
    }
    mgr.clear_script_tasks();
    DataArray success_state;
    success_state.push(DataNode::Sym(Symbol("success")));
    lag_panel->handle_property(Symbol("set_state"), success_state);
    CHECK(lag_auto->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lag_button_recalibrate"));
    CHECK(lag_instructions->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("lag_success"));

    lag_panel->set_property(Symbol("lag"), DataNode::Int(23));
    mgr.goto_screen(Symbol("options_screen"));
    if (Object* options = mgr.resolve_object(Symbol("options"))) {
      CHECK(options->handle_property(Symbol("get_sync_offset"), DataArray())
                .as_int()
                .value_or(999) == -23);
    }
  }

  if (Object* options = mgr.resolve_object(Symbol("options"))) {
    DataArray pscan_false;
    pscan_false.push(DataNode::Int(0));
    options->handle_property(Symbol("set_pscan"), pscan_false);
  }
  mgr.goto_screen(Symbol("pscan_warning_screen"));
  Object* pscan_warning_screen = mgr.current_screen();
  Object* pscan_warning_panel = mgr.find_object(Symbol("pscan_warning_panel"));
  Object* pscan_ok = mgr.resolve_object(Symbol("pscan_ok.btn"));
  Object* pscan_cancel = mgr.resolve_object(Symbol("pscan_cancel.btn"));
  CHECK(pscan_warning_screen != nullptr);
  CHECK(pscan_warning_panel != nullptr);
  CHECK(pscan_ok != nullptr);
  CHECK(pscan_cancel != nullptr);
  if (pscan_warning_screen && pscan_warning_panel && pscan_ok && pscan_cancel) {
    const std::string pscan_warning_text =
        mgr.localize(Symbol("pscan_warning"));
    CHECK(pscan_warning_text.find("doesn't") != std::string::npos);
    CHECK(pscan_warning_text.find("we'll") != std::string::npos);
    CHECK(pscan_warning_screen->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("pscan_warning_panel"));
    CHECK(pscan_warning_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("pscan_ok.btn"));
    mgr.set_global(Symbol("component"), DataNode::Obj(pscan_cancel));
    pscan_warning_screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("video_settings_screen"));

    auto pscan_test_time = [](Object* screen) {
      if (!screen) return -1.0f;
      DataNode node = screen->get_property(Symbol("test_time"));
      if (auto f = node.as_float()) return *f;
      if (auto i = node.as_int()) return static_cast<float>(*i);
      return -1.0f;
    };
    auto enter_pscan_switch_from_warning = [&]() -> Object* {
      mgr.goto_screen(Symbol("pscan_warning_screen"));
      Object* warning_screen = mgr.current_screen();
      Object* ok_button = mgr.resolve_object(Symbol("pscan_ok.btn"));
      CHECK(warning_screen != nullptr);
      CHECK(ok_button != nullptr);
      if (!warning_screen || !ok_button) return nullptr;
      mgr.set_global(Symbol("component"), DataNode::Obj(ok_button));
      warning_screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("pscan_switch_screen"));
      mgr.update(0.5f);
      CHECK(!mgr.in_transition());
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("pscan_switch_screen"));
      return mgr.current_screen();
    };

    Object* pscan_switch_screen = enter_pscan_switch_from_warning();
    Object* pscan_switch_panel =
        mgr.find_object(Symbol("pscan_switching_panel"));
    Object* pscan_yes = mgr.resolve_object(Symbol("pscan_yes.btn"));
    Object* pscan_no = mgr.resolve_object(Symbol("pscan_no.btn"));
    Object* pscan_helpbar = mgr.resolve_object(Symbol("helpbar"));
    CHECK(pscan_switch_screen != nullptr);
    CHECK(pscan_switch_panel != nullptr);
    CHECK(pscan_yes != nullptr);
    CHECK(pscan_no != nullptr);
    if (pscan_switch_screen && pscan_switch_panel && pscan_yes && pscan_no) {
      CHECK(pscan_switch_screen->get_property(Symbol("focus"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("pscan_switching_panel"));
      CHECK(pscan_switch_panel->get_property(Symbol("focus"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("pscan_yes.btn"));
      CHECK(pscan_switch_screen->get_property(Symbol("allow_back"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("FALSE"));
      CHECK(pscan_switch_screen->get_property(Symbol("done_screen"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("video_settings_screen"));
      if (pscan_helpbar) {
        CHECK(array_contains_symbol(pscan_helpbar->get_property(Symbol("display")),
                                    Symbol("help_continue")));
        CHECK(array_contains_symbol(pscan_helpbar->get_property(Symbol("display")),
                                    Symbol("help_updown")));
      }
      if (Object* options = mgr.resolve_object(Symbol("options"))) {
        CHECK(options->handle_property(Symbol("get_pscan"), DataArray())
                  .as_int()
                  .value_or(-1) == 1);
      }
      CHECK(near(pscan_test_time(pscan_switch_screen),
                 mgr.ui_seconds() + 15.0f, 0.01f));

      mgr.set_global(Symbol("component"), DataNode::Obj(pscan_yes));
      pscan_switch_screen->handle_property(Symbol("SELECT_START_MSG"),
                                           DataArray());
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("video_settings_screen"));
      if (Object* options = mgr.resolve_object(Symbol("options"))) {
        CHECK(options->handle_property(Symbol("get_pscan"), DataArray())
                  .as_int()
                  .value_or(-1) == 1);
      }
    }

    pscan_switch_screen = enter_pscan_switch_from_warning();
    if (pscan_switch_screen) {
      CHECK(pscan_test_time(pscan_switch_screen) > mgr.ui_seconds());
      mgr.update(15.1f);
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("video_settings_screen"));
      if (Object* options = mgr.resolve_object(Symbol("options"))) {
        CHECK(options->handle_property(Symbol("get_pscan"), DataArray())
                  .as_int()
                  .value_or(-1) == 0);
      }
    }

    pscan_switch_screen = enter_pscan_switch_from_warning();
    pscan_no = mgr.resolve_object(Symbol("pscan_no.btn"));
    CHECK(pscan_switch_screen != nullptr);
    CHECK(pscan_no != nullptr);
    if (pscan_switch_screen && pscan_no) {
      if (Object* options = mgr.resolve_object(Symbol("options"))) {
        CHECK(options->handle_property(Symbol("get_pscan"), DataArray())
                  .as_int()
                  .value_or(-1) == 1);
      }
      mgr.set_global(Symbol("component"), DataNode::Obj(pscan_no));
      pscan_switch_screen->handle_property(Symbol("SELECT_START_MSG"),
                                           DataArray());
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("video_settings_screen"));
      if (Object* options = mgr.resolve_object(Symbol("options"))) {
        CHECK(options->handle_property(Symbol("get_pscan"), DataArray())
                  .as_int()
                  .value_or(-1) == 0);
      }
    }
  }

  if (Object* game = mgr.resolve_object(Symbol("game"))) {
    CHECK(game->handle_property(Symbol("is_missing_controller"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("FALSE"));
    DataArray missing_true;
    missing_true.push(DataNode::Sym(Symbol("TRUE")));
    game->handle_property(Symbol("set_missing_controller"), missing_true);
    CHECK(game->handle_property(Symbol("is_missing_controller"), DataArray())
              .as_symbol()
              .value_or(Symbol()) == Symbol("TRUE"));
  }
  mgr.goto_screen(Symbol("pause_controller_screen"));
  mgr.update(0.0f);
  Object* pause_controller_screen = mgr.current_screen();
  Object* pause_controller_panel = mgr.find_object(Symbol("pause_controller_panel"));
  Object* pause_controller_msg = mgr.resolve_object(Symbol("pause_controller_msg.lbl"));
  Object* pause_controller_resume = mgr.resolve_object(Symbol("resume.btn"));
  CHECK(pause_controller_screen != nullptr &&
        pause_controller_screen->name() == Symbol("pause_controller_screen"));
  CHECK(pause_controller_panel != nullptr);
  CHECK(pause_controller_msg != nullptr);
  CHECK(pause_controller_resume != nullptr);
  if (pause_controller_screen && pause_controller_panel) {
    CHECK(pause_controller_screen->get_property(Symbol("allow_back"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("FALSE"));
    CHECK(pause_controller_screen->get_property(Symbol("animate_transition"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("FALSE"));
    CHECK(pause_controller_screen->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("pause_controller_panel"));
    CHECK(pause_controller_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("resume.btn"));
  }
  if (pause_controller_msg)
    CHECK(pause_controller_msg->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("controller_loss_msg"));
  if (pause_controller_resume) {
    CHECK(pause_controller_resume->get_property(Symbol("state"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("kDisabled"));
    CHECK(pause_controller_resume->get_property(Symbol("disabled"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("TRUE"));
  }
  if (pause_controller_screen) {
    mgr.set_global(Symbol("button"), DataNode::Sym(Symbol("kPad_Start")));
    pause_controller_screen->handle_property(Symbol("BUTTON_DOWN_MSG"),
                                             DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("pause_controller_screen"));
  }
  if (Object* game = mgr.resolve_object(Symbol("game"))) {
    DataArray missing_false;
    missing_false.push(DataNode::Sym(Symbol("FALSE")));
    game->handle_property(Symbol("set_missing_controller"), missing_false);
  }

  mgr.goto_screen(Symbol("pause_screen"));
  Object* pause_screen = mgr.current_screen();
  Object* pause_panel = mgr.find_object(Symbol("pause_panel"));
  Object* pause_resume_button = mgr.resolve_object(Symbol("resume.btn"));
  Object* pause_restart_button = mgr.resolve_object(Symbol("restart.btn"));
  Object* pause_audio_button = mgr.resolve_object(Symbol("audio_options.btn"));
  Object* pause_video_button = mgr.resolve_object(Symbol("video_options.btn"));
  CHECK(pause_screen != nullptr &&
        pause_screen->name() == Symbol("pause_screen"));
  CHECK(pause_panel != nullptr);
  CHECK(pause_resume_button != nullptr);
  CHECK(pause_restart_button != nullptr);
  CHECK(pause_audio_button != nullptr);
  CHECK(pause_video_button != nullptr);
  if (pause_screen && pause_panel && pause_resume_button &&
      pause_restart_button && pause_audio_button && pause_video_button) {
    Object* game = mgr.resolve_object(Symbol("game"));
    Object* synth = mgr.resolve_object(Symbol("synth"));
    CHECK(game != nullptr);
    CHECK(synth != nullptr);
    CHECK(pause_screen->get_property(Symbol("allow_back"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("FALSE"));
    CHECK(pause_screen->get_property(Symbol("animate_transition"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("FALSE"));
    CHECK(pause_screen->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("pause_panel"));
    CHECK(pause_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("resume.btn"));

    const std::size_t unhandled_before = mgr.unhandled().size();
    mgr.set_global(Symbol("button"), DataNode::Sym(Symbol("kPad_Start")));
    pause_screen->handle_property(Symbol("BUTTON_DOWN_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("game_screen"));
    CHECK(mgr.unhandled().size() == unhandled_before);

    mgr.goto_screen(Symbol("pause_screen"));
    pause_screen = mgr.current_screen();
    CHECK(pause_screen != nullptr &&
          pause_screen->name() == Symbol("pause_screen"));
    if (!pause_screen) return g_failures == 0 ? 0 : 1;
    mgr.set_global(Symbol("component"), DataNode::Obj(pause_resume_button));
    pause_screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("game_screen"));
    CHECK(mgr.unhandled().size() == unhandled_before);

    mgr.goto_screen(Symbol("pause_screen"));
    pause_screen = mgr.current_screen();
    CHECK(pause_screen != nullptr &&
          pause_screen->name() == Symbol("pause_screen"));
    if (!pause_screen) return g_failures == 0 ? 0 : 1;
    mgr.set_global(Symbol("component"), DataNode::Obj(pause_audio_button));
    pause_screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("pause_audio_settings_screen"));
    CHECK(mgr.unhandled().size() == unhandled_before);

    mgr.goto_screen(Symbol("pause_screen"));
    pause_screen = mgr.current_screen();
    CHECK(pause_screen != nullptr &&
          pause_screen->name() == Symbol("pause_screen"));
    if (!pause_screen) return g_failures == 0 ? 0 : 1;
    mgr.set_global(Symbol("component"), DataNode::Obj(pause_video_button));
    pause_screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("pause_video_settings_screen"));
    CHECK(mgr.unhandled().size() == unhandled_before);

    mgr.goto_screen(Symbol("pause_screen"));
    pause_screen = mgr.current_screen();
    CHECK(pause_screen != nullptr &&
          pause_screen->name() == Symbol("pause_screen"));
    if (!pause_screen) return g_failures == 0 ? 0 : 1;
    if (game) {
      game->set_property(Symbol("intro_complete"),
                         DataNode::Sym(Symbol("TRUE")));
    }
    mgr.set_global(Symbol("component"), DataNode::Obj(pause_restart_button));
    pause_screen->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("game_screen"));
    if (game) {
      CHECK(game->get_property(Symbol("last_restart_command"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("game_restart_fast"));
      CHECK(game->get_property(Symbol("restart_fast")).as_int().value_or(0) ==
            1);
      CHECK(game->get_property(Symbol("restart_count")).as_int().value_or(0) >
            0);
      CHECK(game->get_property(Symbol("intro_complete"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("FALSE"));
    }
    if (synth) {
      CHECK(synth->get_property(Symbol("last_sequence"))
                .as_symbol()
                .value_or(Symbol()) == Symbol("button_play"));
      CHECK(synth->get_property(Symbol("stop_all_count")).as_int().value_or(0) >
            0);
    }
    CHECK(mgr.unhandled().size() == unhandled_before);
  }

  if (Object* options = mgr.resolve_object(Symbol("options"))) {
    DataArray set_lefty_p1_off;
    set_lefty_p1_off.push(DataNode::Int(0));
    set_lefty_p1_off.push(DataNode::Int(0));
    options->handle_property(Symbol("set_lefty"), set_lefty_p1_off);
    DataArray set_lefty_p2_off;
    set_lefty_p2_off.push(DataNode::Int(1));
    set_lefty_p2_off.push(DataNode::Int(0));
    options->handle_property(Symbol("set_lefty"), set_lefty_p2_off);
    DataArray set_wide_off;
    set_wide_off.push(DataNode::Int(0));
    options->handle_property(Symbol("set_widescreen"), set_wide_off);
    DataArray set_pscan_off;
    set_pscan_off.push(DataNode::Int(0));
    options->handle_property(Symbol("set_pscan"), set_pscan_off);
  }
  mgr.goto_screen(Symbol("pause_video_settings_screen"));
  Object* pause_video_screen = mgr.current_screen();
  Object* pause_video_panel =
      mgr.find_object(Symbol("pause_video_settings_panel"));
  Object* pause_lefty_p1_button = mgr.resolve_object(Symbol("gs_left_p1.btn"));
  Object* pause_lefty_p2_button = mgr.resolve_object(Symbol("gs_left_p2.btn"));
  Object* pause_widescreen_button =
      mgr.resolve_object(Symbol("widescreen.btn"));
  Object* pause_calibrate_button =
      mgr.resolve_object(Symbol("calibrate_lag.btn"));
  Object* pause_lefty1_check = mgr.resolve_object(Symbol("lefty1.chk"));
  Object* pause_lefty2_check = mgr.resolve_object(Symbol("lefty2.chk"));
  Object* pause_widescreen_check =
      mgr.resolve_object(Symbol("widescreen.chk"));
  Object* pause_pscan_check = mgr.resolve_object(Symbol("p_scan.chk"));
  CHECK(pause_video_screen != nullptr &&
        pause_video_screen->name() == Symbol("pause_video_settings_screen"));
  CHECK(pause_video_panel != nullptr);
  CHECK(pause_lefty_p1_button != nullptr);
  CHECK(pause_lefty_p2_button != nullptr);
  CHECK(pause_widescreen_button != nullptr);
  CHECK(pause_calibrate_button != nullptr);
  CHECK(pause_lefty1_check != nullptr);
  CHECK(pause_lefty2_check != nullptr);
  CHECK(pause_widescreen_check != nullptr);
  CHECK(pause_pscan_check != nullptr);
  if (pause_video_screen && pause_video_panel && pause_lefty_p1_button &&
      pause_lefty_p2_button && pause_widescreen_button &&
      pause_calibrate_button && pause_lefty1_check && pause_lefty2_check &&
      pause_widescreen_check && pause_pscan_check) {
    CHECK(pause_video_screen->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("pause_video_settings_panel"));
    CHECK(pause_video_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("gs_left_p1.btn"));
    CHECK(pause_lefty_p2_button->get_property(Symbol("disabled"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("TRUE"));
    CHECK(pause_video_screen->get_property(Symbol("disabled"))
              .as_symbol()
              .value_or(Symbol("FALSE")) != Symbol("TRUE"));
    CHECK(pause_lefty1_check->handle_property(Symbol("get_check"),
                                              DataArray())
              .as_int()
              .value_or(-1) == 0);
    CHECK(pause_lefty2_check->handle_property(Symbol("get_check"),
                                              DataArray())
              .as_int()
              .value_or(-1) == 0);
    CHECK(pause_widescreen_check->handle_property(Symbol("get_check"),
                                                  DataArray())
              .as_int()
              .value_or(-1) == 0);
    CHECK(pause_pscan_check->handle_property(Symbol("get_check"),
                                             DataArray())
              .as_int()
              .value_or(-1) == 0);
    mgr.set_global(Symbol("component"), DataNode::Obj(pause_lefty_p1_button));
    pause_video_panel->handle_property(Symbol("SELECT_START_MSG"),
                                       DataArray());
    CHECK(pause_lefty1_check->handle_property(Symbol("get_check"),
                                              DataArray())
              .as_int()
              .value_or(-1) == 1);
    if (Object* options = mgr.resolve_object(Symbol("options"))) {
      DataArray get_p1;
      get_p1.push(DataNode::Int(0));
      CHECK(options->handle_property(Symbol("get_lefty"), get_p1)
                .as_int()
                .value_or(-1) == 1);
    }
    mgr.set_global(Symbol("component"), DataNode::Obj(pause_widescreen_button));
    pause_video_panel->handle_property(Symbol("SELECT_START_MSG"),
                                       DataArray());
    CHECK(pause_widescreen_check->handle_property(Symbol("get_check"),
                                                  DataArray())
              .as_int()
              .value_or(-1) == 1);
    if (Object* options = mgr.resolve_object(Symbol("options"))) {
      CHECK(options->handle_property(Symbol("get_widescreen"), DataArray())
                .as_int()
                .value_or(-1) == 1);
    }
  }

  mgr.goto_screen(Symbol("pause_screen"));
  Object* routed_pause_for_video = mgr.current_screen();
  Object* routed_pause_video_button =
      mgr.resolve_object(Symbol("video_options.btn"));
  CHECK(routed_pause_for_video != nullptr &&
        routed_pause_for_video->name() == Symbol("pause_screen"));
  CHECK(routed_pause_video_button != nullptr);
  if (routed_pause_for_video && routed_pause_video_button) {
    mgr.set_global(Symbol("component"),
                   DataNode::Obj(routed_pause_video_button));
    routed_pause_for_video->handle_property(Symbol("SELECT_START_MSG"),
                                            DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() ==
              Symbol("pause_video_settings_screen"));
    if (Object* routed_video_screen = mgr.current_screen()) {
      routed_video_screen->handle_property(Symbol("go_back"), DataArray());
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("pause_screen"));
    }
  }

  mgr.goto_screen(Symbol("audio_settings_screen"));
  Object* helpbar = mgr.resolve_object(Symbol("helpbar"));
  CHECK(helpbar != nullptr);
  Object* real_helpbar_panel = mgr.find_object(Symbol("helpbar"));
  CHECK(real_helpbar_panel != nullptr);
  if (real_helpbar_panel) {
    CHECK(real_helpbar_panel->get_property(Symbol("max_labels"))
              .as_int()
              .value_or(-1) == 4);
    CHECK(real_helpbar_panel->get_property(Symbol("max_buttons"))
              .as_int()
              .value_or(-1) == 4);
    CHECK(real_helpbar_panel->get_property(Symbol("button_spacing"))
              .as_float()
              .value_or(-1.0f) == 35.0f);
    CHECK(real_helpbar_panel->get_property(Symbol("strumbar_spacing"))
              .as_float()
              .value_or(-1.0f) == 70.0f);
    CHECK(real_helpbar_panel->get_property(Symbol("text_spacing"))
              .as_float()
              .value_or(-1.0f) == 30.0f);
    auto display = std::make_shared<DataArray>();
    auto select = std::make_shared<DataArray>();
    select->push(DataNode::Sym(Symbol("fret1")));
    select->push(DataNode::Sym(Symbol("help_select")));
    display->push(DataNode::Array(select));
    DataArray set_display;
    set_display.push(DataNode::Array(display));
    real_helpbar_panel->handle_property(Symbol("set_display"), set_display);
    CHECK(array_contains_symbol(
        real_helpbar_panel->handle_property(Symbol("display"), DataArray()),
        Symbol("help_select")));
    CHECK(array_contains_symbol(
        real_helpbar_panel->handle_property(Symbol("get_display"), DataArray()),
        Symbol("fret1")));
    mgr.goto_screen(Symbol("loading_screen"));
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("loading_screen"));
    if (mgr.current_screen()) {
      CHECK(!array_contains_symbol(mgr.current_screen()->get_property(
                                       Symbol("panels")),
                                   Symbol("helpbar")));
    }
    CHECK(array_contains_symbol(real_helpbar_panel->get_property(Symbol("display")),
                                Symbol("help_select")));
    CHECK(array_contains_symbol(real_helpbar_panel->get_property(Symbol("display")),
                                Symbol("fret1")));
    mgr.goto_screen(Symbol("audio_settings_screen"));
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("audio_settings_screen"));
    if (mgr.current_screen()) {
      CHECK(array_contains_symbol(mgr.current_screen()->get_property(
                                      Symbol("panels")),
                                  Symbol("helpbar")));
    }
  }
  if (Object* options = mgr.resolve_object(Symbol("options"))) {
    DataArray vol_idx;
    vol_idx.push(DataNode::Int(11));
    CHECK(options->handle_property(Symbol("get_volume_from_idx"), vol_idx)
              .as_float()
              .value_or(-1.0f) == 1.0f);
  }
  if (Object* panel = mgr.find_object(Symbol("audio_settings_panel"))) {
    Object* band = mgr.resolve_object(Symbol("gs_band.sld"));
    CHECK(band != nullptr);
    Object* guitar = mgr.resolve_object(Symbol("gs_guitar.sld"));
    CHECK(guitar != nullptr);
    Object* sfx = mgr.resolve_object(Symbol("gs_sfx.sld"));
    CHECK(sfx != nullptr);
    for (Object* slider : {band, guitar, sfx}) {
      if (!slider) continue;
      CHECK(slider->handle_property(Symbol("num_steps"), DataArray())
                .as_int()
                .value_or(-1) == 12);
      CHECK(slider->handle_property(Symbol("current"), DataArray())
                .as_int()
                .value_or(-1) == 11);
    }
    mgr.set_global(Symbol("new_focus"), DataNode::Obj(band));
    panel->handle_property(Symbol("FOCUS_MSG"), DataArray());
    if (helpbar) {
      CHECK(array_contains_symbol(helpbar->get_property(Symbol("display")),
                                  Symbol("help_select")));
      CHECK(array_contains_symbol(helpbar->get_property(Symbol("display")),
                                  Symbol("help_updown")));
    }
    CHECK(panel->handle_property(Symbol("slider_selected"), DataArray())
              .as_int()
              .value_or(-1) == 0);
    band->handle_property(Symbol("send_select"), DataArray());
    mgr.set_global(Symbol("component"), DataNode::Obj(band));
    panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(panel->handle_property(Symbol("slider_selected"), DataArray())
              .as_int()
              .value_or(-1) == 1);
    if (helpbar) {
      CHECK(array_contains_symbol(helpbar->get_property(Symbol("display")),
                                  Symbol("help_confirm")));
      CHECK(array_contains_symbol(helpbar->get_property(Symbol("display")),
                                  Symbol("help_cancel")));
    }
    DataArray set_band_volume;
    set_band_volume.push(DataNode::Int(7));
    band->handle_property(Symbol("set_current"), set_band_volume);
    panel->handle_property(Symbol("set_volumes"), DataArray());
    if (Object* options = mgr.resolve_object(Symbol("options"))) {
      CHECK(options->handle_property(Symbol("get_band_volume_idx"),
                                     DataArray())
                .as_int()
                .value_or(-1) == 7);
    }
    band->handle_property(Symbol("confirm"), DataArray());
    CHECK(panel->handle_property(Symbol("slider_selected"), DataArray())
              .as_int()
              .value_or(-1) == 0);
    guitar->handle_property(Symbol("store"), DataArray());
    DataArray set_guitar_volume;
    set_guitar_volume.push(DataNode::Int(5));
    guitar->handle_property(Symbol("set_current"), set_guitar_volume);
    guitar->handle_property(Symbol("undo"), DataArray());
    CHECK(guitar->handle_property(Symbol("current"), DataArray())
              .as_int()
              .value_or(-1) == 11);
    Object* stereo = mgr.resolve_object(Symbol("gs_stereo.btn"));
    CHECK(stereo != nullptr);
    mgr.set_global(Symbol("new_focus"), DataNode::Obj(stereo));
    panel->handle_property(Symbol("FOCUS_MSG"), DataArray());
    if (helpbar) {
      CHECK(array_contains_symbol(helpbar->get_property(Symbol("display")),
                                  Symbol("help_onoff")));
      CHECK(!array_contains_symbol(helpbar->get_property(Symbol("display")),
                                   Symbol("help_select")));
    }
  } else {
    CHECK(false);
  }

  if (Object* options = mgr.resolve_object(Symbol("options"))) {
    options->set_property(Symbol("band_volume_idx"), DataNode::Int(11));
    options->set_property(Symbol("guitar_volume_idx"), DataNode::Int(11));
    options->set_property(Symbol("fx_volume_idx"), DataNode::Int(11));
    options->set_property(Symbol("stereo"), DataNode::Int(1));
  }
  mgr.goto_screen(Symbol("pause_audio_settings_screen"));
  Object* pause_audio_screen = mgr.current_screen();
  Object* pause_audio_panel =
      mgr.find_object(Symbol("pause_audio_settings_panel"));
  CHECK(pause_audio_screen != nullptr &&
        pause_audio_screen->name() == Symbol("pause_audio_settings_screen"));
  CHECK(pause_audio_panel != nullptr);
  if (pause_audio_screen && pause_audio_panel) {
    CHECK(pause_audio_screen->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("pause_audio_settings_panel"));
    CHECK(pause_audio_panel->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("gs_band.sld"));
    Object* band = mgr.resolve_object(Symbol("gs_band.sld"));
    Object* guitar = mgr.resolve_object(Symbol("gs_guitar.sld"));
    Object* sfx = mgr.resolve_object(Symbol("gs_sfx.sld"));
    Object* stereo = mgr.resolve_object(Symbol("gs_stereo.btn"));
    CHECK(band != nullptr);
    CHECK(guitar != nullptr);
    CHECK(sfx != nullptr);
    CHECK(stereo != nullptr);
    for (Object* slider : {band, guitar, sfx}) {
      if (!slider) continue;
      CHECK(slider->handle_property(Symbol("num_steps"), DataArray())
                .as_int()
                .value_or(-1) == 12);
      CHECK(slider->handle_property(Symbol("current"), DataArray())
                .as_int()
                .value_or(-1) == 11);
    }
    mgr.set_global(Symbol("new_focus"), DataNode::Obj(band));
    pause_audio_panel->handle_property(Symbol("FOCUS_MSG"), DataArray());
    if (helpbar) {
      CHECK(array_contains_symbol(helpbar->get_property(Symbol("display")),
                                  Symbol("help_select")));
      CHECK(array_contains_symbol(helpbar->get_property(Symbol("display")),
                                  Symbol("help_updown")));
    }
    CHECK(pause_audio_panel->handle_property(Symbol("slider_selected"),
                                             DataArray())
              .as_int()
              .value_or(-1) == 0);
    band->handle_property(Symbol("send_select"), DataArray());
    mgr.set_global(Symbol("component"), DataNode::Obj(band));
    pause_audio_panel->handle_property(Symbol("SELECT_START_MSG"),
                                       DataArray());
    CHECK(pause_audio_panel->handle_property(Symbol("slider_selected"),
                                             DataArray())
              .as_int()
              .value_or(-1) == 1);
    DataArray set_band_volume;
    set_band_volume.push(DataNode::Int(7));
    band->handle_property(Symbol("set_current"), set_band_volume);
    pause_audio_panel->handle_property(Symbol("set_volumes"), DataArray());
    if (Object* options = mgr.resolve_object(Symbol("options"))) {
      CHECK(options->handle_property(Symbol("get_band_volume_idx"),
                                     DataArray())
                .as_int()
                .value_or(-1) == 7);
    }
    band->handle_property(Symbol("confirm"), DataArray());
    CHECK(pause_audio_panel->handle_property(Symbol("slider_selected"),
                                             DataArray())
              .as_int()
              .value_or(-1) == 0);
    guitar->handle_property(Symbol("store"), DataArray());
    DataArray set_guitar_volume;
    set_guitar_volume.push(DataNode::Int(5));
    guitar->handle_property(Symbol("set_current"), set_guitar_volume);
    guitar->handle_property(Symbol("undo"), DataArray());
    CHECK(guitar->handle_property(Symbol("current"), DataArray())
              .as_int()
              .value_or(-1) == 11);
    mgr.set_global(Symbol("new_focus"), DataNode::Obj(stereo));
    pause_audio_panel->handle_property(Symbol("FOCUS_MSG"), DataArray());
    if (helpbar) {
      CHECK(array_contains_symbol(helpbar->get_property(Symbol("display")),
                                  Symbol("help_onoff")));
      CHECK(!array_contains_symbol(helpbar->get_property(Symbol("display")),
                                   Symbol("help_select")));
    }
    mgr.set_global(Symbol("component"), DataNode::Obj(stereo));
    pause_audio_panel->handle_property(Symbol("SELECT_START_MSG"),
                                       DataArray());
    if (Object* options = mgr.resolve_object(Symbol("options"))) {
      CHECK(options->handle_property(Symbol("get_stereo"), DataArray())
                .as_int()
                .value_or(-1) == 0);
    }
  }

  if (Object* options = mgr.resolve_object(Symbol("options"))) {
    options->set_property(Symbol("band_volume_idx"), DataNode::Int(11));
    options->set_property(Symbol("guitar_volume_idx"), DataNode::Int(11));
    options->set_property(Symbol("fx_volume_idx"), DataNode::Int(11));
    options->set_property(Symbol("stereo"), DataNode::Int(1));
  }
  mgr.goto_screen(Symbol("pause_screen"));
  Object* routed_pause_screen = mgr.current_screen();
  Object* routed_pause_audio_button =
      mgr.resolve_object(Symbol("audio_options.btn"));
  CHECK(routed_pause_screen != nullptr &&
        routed_pause_screen->name() == Symbol("pause_screen"));
  CHECK(routed_pause_audio_button != nullptr);
  if (routed_pause_screen && routed_pause_audio_button) {
    mgr.set_global(Symbol("component"),
                   DataNode::Obj(routed_pause_audio_button));
    routed_pause_screen->handle_property(Symbol("SELECT_START_MSG"),
                                         DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() ==
              Symbol("pause_audio_settings_screen"));
    if (Object* routed_audio_screen = mgr.current_screen()) {
      routed_audio_screen->handle_property(Symbol("go_back"), DataArray());
      CHECK(mgr.current_screen() != nullptr &&
            mgr.current_screen()->name() == Symbol("pause_screen"));
    }
  }

  mgr.goto_screen(Symbol("bootup_load"));
  Object* boot_screen = mgr.current_screen();
  Object* dialog = mgr.find_object(Symbol("dialog"));
  Object* dialog_message = mgr.resolve_object(Symbol("dl_message.lbl"));
  Object* dialog_title = mgr.resolve_object(Symbol("dl_title.lbl"));
  Object* dialog_button1 = mgr.resolve_object(Symbol("dl_button1.btn"));
  Object* dialog_button2 = mgr.resolve_object(Symbol("dl_button2.btn"));
  CHECK(boot_screen != nullptr &&
        boot_screen->name() == Symbol("bootup_load"));
  CHECK(dialog != nullptr);
  CHECK(dialog_message != nullptr);
  CHECK(dialog_title != nullptr);
  CHECK(dialog_button1 != nullptr);
  CHECK(dialog_button2 != nullptr);
  if (boot_screen && dialog && dialog_message && dialog_title &&
      dialog_button1 && dialog_button2) {
    CHECK(boot_screen->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("dialog"));
    CHECK(dialog_message->get_property(Symbol("text"))
              .as_string()
              .value_or("") == mgr.localize(Symbol("mc_checking")));
    CHECK(dialog_title->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("mc_title_loading"));
    CHECK(dialog_button1->has_property(Symbol("showing")));
    CHECK(dialog_button2->has_property(Symbol("showing")));
    CHECK(!truthy(dialog_button1->get_property(Symbol("showing"))));
    CHECK(!truthy(dialog_button2->get_property(Symbol("showing"))));
    DataArray get_button1;
    get_button1.push(DataNode::Sym(Symbol("button1")));
    DataNode button1_name =
        dialog->handle_property(Symbol("get_button"), get_button1);
    CHECK(button1_name.as_string().value_or("") == "dl_button1.btn");
    DataArray set_button_text;
    set_button_text.push(button1_name);
    set_button_text.push(DataNode::Sym(Symbol("mc_retry")));
    dialog->handle_property(Symbol("set_button_text"), set_button_text);
    CHECK(truthy(dialog_button1->get_property(Symbol("showing"))));
    CHECK(dialog_button1->get_property(Symbol("state"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("kNormal"));
    CHECK(dialog_button1->get_property(Symbol("text"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("mc_retry"));
    DataArray focus_button1;
    focus_button1.push(button1_name);
    dialog->handle_property(Symbol("set_button_focus"), focus_button1);
    CHECK(dialog->get_property(Symbol("focus"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("dl_button1.btn"));
    CHECK(dialog_button1->get_property(Symbol("state"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("kFocused"));
    dialog->handle_property(Symbol("hide_button"), focus_button1);
    CHECK(!truthy(dialog_button1->get_property(Symbol("showing"))));
    CHECK(dialog_button1->get_property(Symbol("state"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("kDisabled"));
    CHECK(dialog_button1->get_property(Symbol("disabled"))
              .as_symbol()
              .value_or(Symbol()) == Symbol("TRUE"));
  }

  // 4. Simulate a SELECT_START on main_quickspin.btn: the real SELECT_START_MSG
  //    switch must route to {ui goto_screen qp_selsong_screen}.
  mgr.goto_screen(Symbol("main_screen"));
  CHECK(mgr.find_object(Symbol("qp_selsong_screen")) != nullptr);  // quickplay.dtb loaded it
  Object* quickplay_button = mgr.resolve_object(Symbol("main_quickspin.btn"));
  CHECK(quickplay_button != nullptr);
  if (quickplay_button) {
    mgr.set_global(Symbol("component"), DataNode::Obj(quickplay_button));
    main_panel->handle_property(Symbol("SELECT_START_MSG"), DataArray());
    CHECK(mgr.current_screen() != nullptr &&
          mgr.current_screen()->name() == Symbol("qp_selsong_screen"));
  }

  if (g_failures == 0) {
    std::printf("ghogx_ui_test: stock main.dtb boots -> object tree + handlers + "
                "goto_screen + SELECT_START all run -- passed\n");
    return 0;
  }
  std::fprintf(stderr, "ghogx_ui_test: %d check(s) failed\n", g_failures);
  return 1;
}
