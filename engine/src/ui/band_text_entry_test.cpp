#include "ui/config_db.h"
#include "ui/meta_objects.h"
#include "ui/screen_loader.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include "ark_v3.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace ghogx;

namespace {
int failures = 0;

#define CHECK(condition)                                                   \
  do {                                                                     \
    if (!(condition)) {                                                    \
      std::fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__,         \
                   #condition);                                            \
      ++failures;                                                          \
    }                                                                      \
  } while (false)

Object* panel_child(ui::ScreenManager& mgr, Symbol panel_name,
                    Symbol child_name) {
  auto* panel = dynamic_cast<ObjectDir*>(mgr.find_object(panel_name));
  return panel ? panel->find(child_name) : nullptr;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::fprintf(stderr,
                 "usage: ghogx_band_text_entry_test <main.hdr> <main_0.ark>\n");
    return 2;
  }

  const gh::ark::ArkV3Reader ark = gh::ark::ArkV3Reader::load(argv[1]);
  const std::vector<std::string> arks = {argv[2]};
  ui::register_ui_classes();
  ui::ScreenManager mgr;
  ui::install_default_singletons(mgr);
  CHECK(ui::load_all_ui_screens(ark, arks, mgr) >= 35);
  CHECK(ui::load_panel_milo_widgets(ark, arks, mgr) > 100);
  ui::ConfigDb db;
  db.load(ark, arks);
  ui::install_meta_singletons(mgr, db);

  Object* name_screen = mgr.find_object(Symbol("nameprof_screen"));
  CHECK(name_screen != nullptr);
  if (!name_screen) return 1;
  name_screen->set_property(Symbol("profile_slot"), DataNode::Int(0));
  name_screen->set_property(Symbol("is_editing"),
                            DataNode::Sym(Symbol("FALSE")));
  name_screen->set_property(Symbol("next_screen"),
                            DataNode::Sym(Symbol("sel_difficulty_screen")));
  name_screen->set_property(Symbol("back_screen"),
                            DataNode::Sym(Symbol("chooseprof_screen")));
  mgr.goto_screen(Symbol("nameprof_screen"));

  Object* entry =
      panel_child(mgr, Symbol("nameprof_panel"), Symbol("profile.ten"));
  CHECK(entry != nullptr);
  if (!entry) return 1;
  CHECK(entry->get_property(Symbol("text_entry_style"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("band_name"));
  CHECK(entry->get_property(Symbol("characters"))
            .as_string()
            .value_or("") ==
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !?.'");
  CHECK(entry->get_property(Symbol("max_length"))
            .as_int()
            .value_or(0) == 20);
  CHECK(entry->get_property(Symbol("text_resource"))
            .as_symbol()
            .value_or(Symbol()) == Symbol("entry_profile.txt"));

  entry->handle_property(Symbol("resume_input"), DataArray());
  CHECK(entry->handle_property(Symbol("no_text_entered"), DataArray())
            .as_int()
            .value_or(0) == 1);
  CHECK(entry->get_property(Symbol("text"))
            .as_string()
            .value_or("") == "A");

  DataArray down;
  down.push(DataNode::Int(1));
  entry->handle_property(Symbol("scroll_character"), down);
  CHECK(entry->get_property(Symbol("text"))
            .as_string()
            .value_or("") == "B");
  entry->handle_property(Symbol("accept_character"), DataArray());
  CHECK(entry->handle_property(Symbol("get_text"), DataArray())
            .as_string()
            .value_or("") == "B");
  CHECK(entry->get_property(Symbol("text"))
            .as_string()
            .value_or("") == "BB");
  entry->handle_property(Symbol("delete_character"), DataArray());
  CHECK(entry->handle_property(Symbol("no_text_entered"), DataArray())
            .as_int()
            .value_or(0) == 1);
  entry->handle_property(Symbol("accept_character"), DataArray());
  CHECK(entry->handle_property(Symbol("get_text"), DataArray())
            .as_string()
            .value_or("") == "B");

  entry->handle_property(Symbol("send_select"), DataArray());
  CHECK(mgr.current_screen() != nullptr);
  CHECK(mgr.current_screen() &&
        mgr.current_screen()->name() == Symbol("sel_difficulty_screen"));
  Object* campaign = mgr.resolve_object(Symbol("campaign"));
  DataArray slot;
  slot.push(DataNode::Int(0));
  CHECK(campaign != nullptr);
  CHECK(campaign &&
        campaign->handle_property(Symbol("profile_name"), slot)
                .as_string()
                .value_or("") == "B");

  Object* difficulty_panel =
      mgr.find_object(Symbol("sel_diff_career_panel"));
  Object* easy = mgr.resolve_object(Symbol("sd_diff1.btn"));
  CHECK(difficulty_panel != nullptr);
  CHECK(easy != nullptr);
  mgr.set_global(Symbol("component"), DataNode::Obj(easy));
  if (difficulty_panel)
    difficulty_panel->handle_property(Symbol("SELECT_START_MSG"),
                                      DataArray());
  if (mgr.current_screen())
    mgr.current_screen()->handle_property(Symbol("SELECT_START_MSG"),
                                          DataArray());
  CHECK(mgr.current_screen() &&
        mgr.current_screen()->name() == Symbol("sel_character_new_screen"));
  CHECK(mgr.find_object(Symbol("sel_character_panel")) != nullptr);

  // The packed error screen's Continue button must return to the name-entry
  // screen instead of trapping the user after an empty submission.
  mgr.goto_screen(Symbol("nameprof_screen"));
  DataArray empty;
  empty.push(DataNode::Str(""));
  entry->handle_property(Symbol("set_text"), empty);
  entry->handle_property(Symbol("send_select"), DataArray());
  CHECK(mgr.current_screen() &&
        mgr.current_screen()->name() == Symbol("error_no_profile_screen"));
  Object* continue_button =
      panel_child(mgr, Symbol("dialog"), Symbol("dl_button1.btn"));
  CHECK(continue_button != nullptr);
  mgr.set_global(Symbol("component"), DataNode::Obj(continue_button));
  if (mgr.current_screen())
    mgr.current_screen()->handle_property(Symbol("SELECT_START_MSG"),
                                          DataArray());
  CHECK(mgr.current_screen() &&
        mgr.current_screen()->name() == Symbol("nameprof_screen"));

  if (failures != 0) {
    std::fprintf(stderr, "FAIL band text entry checks=%d\n", failures);
    return 1;
  }
  std::printf(
      "PASS band text entry style=band_name length=20 "
      "cycle=up/down accept=green delete=red finish=start "
      "route=sel_difficulty->sel_character_new error_return=ok\n");
  return 0;
}
