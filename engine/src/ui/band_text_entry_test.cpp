#include "ui/config_db.h"
#include "ui/meta_objects.h"
#include "ui/screen_loader.h"
#include "ui/screen_manager.h"
#include "ui/ui_classes.h"

#include "ark_v3.h"

#include <cstdlib>
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

#ifdef _WIN32
  _putenv_s("GHOGX_DISABLE_PROFILE_PERSISTENCE", "1");
#else
  setenv("GHOGX_DISABLE_PROFILE_PERSISTENCE", "1", 1);
#endif

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
        mgr.current_screen()->name() == Symbol("manage_band_screen"));
  Object* manage_screen = mgr.find_object(Symbol("manage_band_screen"));
  Object* manage_panel =
      mgr.find_object(Symbol("manage_band_preferences_panel"));
  CHECK(manage_screen != nullptr);
  CHECK(manage_panel != nullptr);
  CHECK(manage_screen &&
        manage_screen->get_property(Symbol("profile_slot"))
                .as_int()
                .value_or(-1) == 0);
  Object* campaign = mgr.resolve_object(Symbol("campaign"));
  DataArray slot;
  slot.push(DataNode::Int(0));
  CHECK(campaign != nullptr);
  CHECK(campaign &&
        campaign->handle_property(Symbol("profile_name"), slot)
                .as_string()
                .value_or("") == "B");

  // Manage Band retains only the active character plus the two candidates
  // most likely to be selected next.  The panel publishes that bounded queue
  // explicitly so the render layer can prefetch without guessing roster
  // order or retaining every character ever visited.
  if (manage_panel) {
    DataArray open_characters;
    open_characters.push(DataNode::Int(0));
    open_characters.push(DataNode::Int(1));
    manage_panel->handle_property(Symbol("debug_open_category"),
                                  open_characters);
    Object* preview =
        mgr.find_object(Symbol("manage_band_char_preview"));
    CHECK(preview != nullptr);
    CHECK(preview &&
          preview->get_property(Symbol("char_outfit_0"))
              .as_symbol()
              .value_or(Symbol())
              .valid());
    CHECK(preview &&
          preview->get_property(Symbol("preview_previous_outfit"))
              .as_symbol()
              .value_or(Symbol())
              .valid());
    CHECK(preview &&
          preview->get_property(Symbol("preview_next_outfit"))
              .as_symbol()
              .value_or(Symbol())
              .valid());

    DataArray open_guitars;
    open_guitars.push(DataNode::Int(1));
    open_guitars.push(DataNode::Int(0));
    manage_panel->handle_property(Symbol("debug_open_category"),
                                  open_guitars);
    CHECK(manage_panel->get_property(Symbol("visible_rows"))
              .as_int()
              .value_or(0) == 8);
    CHECK(!manage_panel->get_property(Symbol("row_text_7"))
               .as_string()
               .value_or("")
               .empty());
    Object* guitar_preview =
        mgr.find_object(Symbol("manage_band_guitar_preview"));
    CHECK(guitar_preview != nullptr);
    CHECK(guitar_preview &&
          guitar_preview->get_property(Symbol("guitar"))
              .as_symbol()
              .value_or(Symbol())
              .valid());
    CHECK(guitar_preview &&
          guitar_preview->get_property(Symbol("preview_previous_guitar"))
              .as_symbol()
              .value_or(Symbol())
              .valid());
    CHECK(guitar_preview &&
          guitar_preview->get_property(Symbol("preview_next_guitar"))
              .as_symbol()
              .value_or(Symbol())
              .valid());

    DataArray open_venues;
    open_venues.push(DataNode::Int(8));
    open_venues.push(DataNode::Int(0));
    manage_panel->handle_property(Symbol("debug_open_category"), open_venues);
    CHECK(manage_panel->get_property(Symbol("visible_rows"))
              .as_int()
              .value_or(0) == 7);
    CHECK(manage_panel->get_property(Symbol("preview_venue"))
              .as_symbol()
              .value_or(Symbol())
              .valid());
    CHECK(manage_panel->get_property(Symbol("preview_previous_venue"))
              .as_symbol()
              .value_or(Symbol())
              .valid());
    CHECK(manage_panel->get_property(Symbol("preview_next_venue"))
              .as_symbol()
              .value_or(Symbol())
              .valid());

    // Unload is a hard lifecycle boundary. It clears every live selection and
    // queued-neighbor property so no later screen can retain heavyweight
    // preview state. Enter rebuilds the normal initial state for the remaining
    // route assertions below.
    manage_panel->handle_property(Symbol("unload"), DataArray());
    CHECK(manage_panel->get_property(Symbol("visible_rows"))
              .as_int()
              .value_or(-1) == 0);
    CHECK(manage_panel->get_property(Symbol("row_text_7"))
              .as_string()
              .value_or("not-cleared")
              .empty());
    CHECK(!manage_panel->get_property(Symbol("preview_venue"))
               .as_symbol()
               .value_or(Symbol())
               .valid());
    CHECK(preview &&
          !preview->get_property(Symbol("char_outfit_0"))
               .as_symbol()
               .value_or(Symbol())
               .valid());
    CHECK(guitar_preview &&
          !guitar_preview->get_property(Symbol("guitar"))
               .as_symbol()
               .value_or(Symbol())
               .valid());
    manage_panel->handle_property(Symbol("enter"), DataArray());
  }

  // Save & Return is Continue in Career context: it resumes the stock flow at
  // difficulty selection. Options-context Manage Band retains ordinary Back.
  if (manage_panel) {
    DataArray save_return;
    save_return.push(DataNode::Int(11));
    manage_panel->handle_property(Symbol("debug_select_category"),
                                  save_return);
    mgr.set_global(Symbol("button"), DataNode::Sym(Symbol("kPad_X")));
    manage_panel->handle_property(Symbol("BUTTON_DOWN_MSG"), DataArray());
  }
  CHECK(mgr.current_screen() &&
        mgr.current_screen()->name() == Symbol("sel_difficulty_screen"));

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

  // Selecting the profile on a later Career visit enters the same Manage Band
  // screen directly, without reopening band-name entry.
  mgr.goto_screen(Symbol("chooseprof_screen"));
  Object* chooseprof_panel = mgr.find_object(Symbol("chooseprof_panel"));
  Object* band0 = panel_child(mgr, Symbol("chooseprof_panel"),
                              Symbol("cp_band0.btn"));
  CHECK(chooseprof_panel != nullptr);
  CHECK(band0 != nullptr);
  if (chooseprof_panel && band0) {
    chooseprof_panel->set_property(Symbol("focus"),
                                   DataNode::Sym(Symbol("cp_band0.btn")));
    mgr.set_global(Symbol("component"), DataNode::Obj(band0));
    mgr.current_screen()->handle_property(Symbol("SELECT_START_MSG"),
                                          DataArray());
  }
  CHECK(mgr.current_screen() == manage_screen);
  CHECK(manage_screen &&
        manage_screen->get_property(Symbol("profile_slot"))
                .as_int()
                .value_or(-1) == 0);

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
      "route=new/existing->manage_band->sel_difficulty "
      "error_return=ok\n");
  return 0;
}
