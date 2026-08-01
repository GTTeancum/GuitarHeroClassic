// engine/src/ui/screen_loader.h
//
// Instantiates the menu object tree from stock UI DTBs. A screen DTB is a list
// of top-level {new GHPanel ...}/{new GHScreen ...} commands; this walks them,
// creates each object via ClassReg, splits its body into config properties
// (file/focus/panels/...) and scripted handler blocks (enter/poll/SELECT_START
// _MSG/custom), and registers it in the ScreenManager. {new} runs ONCE at
// preload (per the trace doc); goto_screen later only shows/loads/enters.

#pragma once

#include "ark_v3.h"  // gh::ark::ArkV3Reader
#include "dtb.h"     // gh::dtb::NodeList

#include <string>
#include <vector>

namespace ghogx::ui {

class ScreenManager;

struct UiRouteRef {
  std::string owner;
  std::string operation;
  std::string target;
  bool dynamic = false;
  uint32_t source_line = 0;
};

// Instantiate every {new ...} object in `roots` into the manager's registry.
void load_ui_objects(const gh::dtb::NodeList& roots, ScreenManager& mgr);

// Read ui/gen/<dtb_path> from the ARK, parse + preprocess it (screen DTBs have
// no #includes; #ifdef/#define resolved against the PS2 define set), and load
// its {new ...} objects. Returns false if the entry is missing.
bool load_ui_dtb_from_ark(const gh::ark::ArkV3Reader& ark,
                          const std::vector<std::string>& ark_paths,
                          const std::string& dtb_path, ScreenManager& mgr);

// Read a stock UI script DTB (normally ui/gen/init.dtb), preprocess it against
// the PS2 define set, and return only executable top-level commands. Authored
// declarations ({new ...}/{func ...}) are still handled by load_all_ui_screens.
gh::dtb::NodeList load_ui_script_roots_from_ark(
    const gh::ark::ArkV3Reader& ark,
    const std::vector<std::string>& ark_paths,
    const std::string& dtb_path);

// Load EVERY ui/gen/*.dtb in the ARK into the manager -- the full stock screen
// set, verbatim. ui/gen/ui.dtb is processed first so its #define macros are
// visible to the rest (shared macro table). Returns the number of DTBs loaded.
int load_all_ui_screens(const gh::ark::ArkV3Reader& ark,
                        const std::vector<std::string>& ark_paths,
                        ScreenManager& mgr);

// Populate each DTB panel's ObjectDir with lightweight children for creatable
// UIComponent entries and script-addressed Rnd perObjs found in its authored
// panel MILO. Returns child count.
int load_panel_milo_widgets(const gh::ark::ArkV3Reader& ark,
                            const std::vector<std::string>& ark_paths,
                            ScreenManager& mgr);

// Extract every authored navigation command from the already-preprocessed
// object handlers and global function bodies. Literal destinations can be
// checked against the 130-screen registry; variable/expression destinations
// remain explicitly marked dynamic instead of being guessed.
std::vector<UiRouteRef> collect_ui_route_refs(const ScreenManager& mgr);

}  // namespace ghogx::ui
