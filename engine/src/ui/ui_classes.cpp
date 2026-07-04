// engine/src/ui/ui_classes.cpp -- see ui_classes.h.

#include "ui/ui_classes.h"

#include "core/class_reg.h"
#include "ui/screen_manager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace ghogx::ui {

namespace {
DataNode kTrue() { return DataNode::Sym(Symbol("TRUE")); }
DataNode kFalse() { return DataNode::Sym(Symbol("FALSE")); }
DataNode arg0(const DataArray& a) { return a.size() > 0 ? a.at(0) : DataNode(); }
std::string arg0_name(const DataArray& a) {
  if (a.size() == 0) return {};
  if (auto s = a.at(0).as_symbol()) return s->c_str();
  if (auto s = a.at(0).as_string()) return std::string(s->data(), s->size());
  if (Object* obj = a.at(0).as_object()) return obj->name().c_str();
  return {};
}
int int_arg(const DataArray& a, std::size_t index, int fallback = 0) {
  return index < a.size() ? a.at(index).as_int().value_or(fallback) : fallback;
}
bool truthy(const DataNode& n) {
  if (auto i = n.as_int()) return *i != 0;
  if (auto f = n.as_float()) return *f != 0.0f;
  if (auto s = n.as_string())
    return !s->empty() && *s != "FALSE" && *s != "false" && *s != "0";
  return false;
}

Symbol indexed_symbol(const char* stem, int index) {
  return Symbol((std::string(stem) + std::to_string(std::max(0, index))).c_str());
}

Object* player_config(ScreenManager* mgr, int player) {
  Object* game = mgr ? mgr->resolve_object(Symbol("game")) : nullptr;
  if (!game) return nullptr;
  DataArray args;
  args.push(DataNode::Int(player));
  return game->handle_property(Symbol("get_player_config"), args).as_object();
}

DataNode player_value(ScreenManager* mgr, int player, Symbol getter,
                      Symbol game_fallback) {
  if (Object* cfg = player_config(mgr, player)) {
    DataNode out = cfg->handle_property(getter, DataArray());
    if (!out.empty()) return out;
  }
  if (Object* game = mgr ? mgr->resolve_object(Symbol("game")) : nullptr)
    return game->handle_property(game_fallback, DataArray());
  return DataNode();
}

void set_resolved_showing(ScreenManager* mgr, const char* name, bool showing) {
  if (!mgr || !name || !name[0]) return;
  if (Object* obj = mgr->resolve_object(Symbol(name))) {
    obj->set_property(Symbol("showing"),
                      showing ? DataNode::Sym(Symbol("TRUE"))
                              : DataNode::Sym(Symbol("FALSE")));
  }
}

void initialize_tutorial_overlay_state(ScreenManager* mgr) {
  // Lesson DTBs opt these back in as needed. A direct siloed boot of
  // tut_script_screen runs the panel enter path but not the task timeline, so
  // default the tutorial callouts to the stock script's hidden baseline.
  static const char* kHidden[] = {
      "tut_banner.lbl",
      "tut_highlights.view",
      "tut_star_highlight.view",
      "tut_rock_highlight.view",
      "t1_highlights.view",
      "t1_note_highlight.view",
      "t1_long_note_highlight.view",
      "t1_now_highlight.view",
      "t1_chord_highlight.view",
      "t1_score_highlight.view",
      "t1_fret_highlight.view",
      "t1_strum_highlight.view",
      "t1_fret_highlight1.view",
      "t1_fret_highlight2.view",
      "t1_fret_highlight3.view",
      "t1_stamps.view",
      "t1_lets_rock.view",
  };
  for (const char* name : kHidden) set_resolved_showing(mgr, name, false);
}

std::string upper_ascii(std::string text) {
  for (char& c : text)
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return text;
}

std::string localized_venue_title(ScreenManager* mgr) {
  Symbol venue("battle");
  if (Object* game = mgr ? mgr->resolve_object(Symbol("game")) : nullptr)
    venue = game->get_property(Symbol("venue")).as_symbol().value_or(venue);
  std::string text = mgr ? mgr->localize(venue) : std::string(venue.c_str());
  if (text == venue.c_str() && venue == Symbol("battle")) text = "Battle of the Bands";
  return text;
}

std::string default_headline(ScreenManager* mgr) {
  return "LIVELY SET FROM AAAAA AT " + upper_ascii(localized_venue_title(mgr));
}

Object* resolve_target(ScreenManager* mgr, ObjectDir* self, const std::string& name) {
  if (name.empty()) return nullptr;
  if (mgr) {
    if (Object* obj = mgr->resolve_object(Symbol(name.c_str()))) return obj;
  }
  return self ? self->find_path(name) : nullptr;
}

Object* resolve_local_target_first(ScreenManager* mgr, ObjectDir* self,
                                   const std::string& name) {
  if (name.empty()) return nullptr;
  if (self) {
    if (Object* obj = self->find_path(name)) return obj;
  }
  return resolve_target(mgr, self, name);
}

std::string node_string(const DataNode& node) {
  if (auto s = node.as_string()) return std::string(s->data(), s->size());
  if (auto i = node.as_int()) return std::to_string(*i);
  if (auto f = node.as_float()) return std::to_string(*f);
  return {};
}

std::string keyed_arg_value(const DataArray& args, const char* key) {
  for (std::size_t i = 1; i < args.size(); ++i) {
    auto arr = args.at(i).as_array();
    if (!arr || arr->size() < 2) continue;
    std::string found = node_string(arr->at(0));
    if (found == key) return node_string(arr->at(1));
  }
  return {};
}

int object_int(ScreenManager* mgr, const char* object_name, Symbol msg,
               int fallback) {
  Object* obj = mgr ? mgr->resolve_object(Symbol(object_name)) : nullptr;
  return obj ? obj->handle_property(msg, DataArray()).as_int().value_or(fallback)
             : fallback;
}

std::string versus_winner_num(ScreenManager* mgr) {
  int p0 = object_int(mgr, "player0", Symbol("score"), 0);
  int p1 = object_int(mgr, "player1", Symbol("score"), 0);
  return p1 > p0 ? "2" : "1";
}

std::string value_or_default(std::string value, const char* fallback) {
  if (value.empty() || value == "winner" || value == "spread") return fallback;
  return value;
}

void replace_all(std::string& text, const std::string& needle,
                 const std::string& value) {
  if (needle.empty()) return;
  std::size_t pos = 0;
  while ((pos = text.find(needle, pos)) != std::string::npos) {
    text.replace(pos, needle.size(), value);
    pos += value.size();
  }
}

std::string generated_headline(ScreenManager* mgr, const DataArray& args) {
  Symbol token = args.size() ? args.at(0).as_symbol().value_or(Symbol()) : Symbol();
  std::string text = token.valid() && mgr ? mgr->localize(token) : std::string();
  if (text.empty() || (token.valid() && text == token.c_str()))
    text = default_headline(mgr);

  const std::pair<const char*, std::string> replacements[] = {
      {"{GH_BAND}", "AAAA"},
      {"{GH_VENUE}", localized_venue_title(mgr)},
      {"{GH_ADJ}", value_or_default(keyed_arg_value(args, "adj"), "Lively")},
      {"{GH_NOUN}", value_or_default(keyed_arg_value(args, "noun"), "set")},
      {"{GH_VERB}", value_or_default(keyed_arg_value(args, "verb"), "rocks")},
      {"{GH_VERB_PAST}",
       value_or_default(keyed_arg_value(args, "verb_past"), "rocked")},
      {"{GH_WIN_PHRASE}",
       value_or_default(keyed_arg_value(args, "win_phrase"),
                        "takes home gold at Battle of the Bands")},
      {"{GH_NUM}",
       value_or_default(keyed_arg_value(args, "num"),
                        versus_winner_num(mgr).c_str())},
      {"{GH_VS_VERB}",
       value_or_default(keyed_arg_value(args, "vs_verb"), "wins")},
      {"{GH_VS_NOUN}",
       value_or_default(keyed_arg_value(args, "vs_noun"), "in tight match")},
  };
  for (const auto& [needle, value] : replacements)
    replace_all(text, needle, value);
  return text;
}

void sync_ready_label_visibility(ScreenManager* mgr, UiObject* self) {
  if (!self) return;
  const std::string ready_label =
      node_string(self->get_property(Symbol("ready_label")));
  if (ready_label.empty()) return;
  const int player = self->get_property(Symbol("player_num")).as_int().value_or(0);
  DataNode done = self->get_property(indexed_symbol("select_done", player));
  if (done.empty()) done = self->get_property(Symbol("select_done"));
  Object* label = resolve_local_target_first(mgr, self, ready_label);
  if (!label) return;
  label->set_property(Symbol("showing"), truthy(done) ? kTrue() : kFalse());
}
}  // namespace

DataNode UiObject::handle_property(Symbol msg, const DataArray& args) {
  // 1. A scripted handler block from the DTB wins -- run verbatim.
  if (auto h = handler(msg)) {
    DataNode result = mgr_ ? mgr_->run_object_handler(h, this, args) : DataNode();
    if (std::strcmp(msg.c_str(), "start_tutorial") == 0) {
      initialize_tutorial_overlay_state(mgr_);
      set_property(Symbol("running"), kTrue());
    }
    if (std::strcmp(msg.c_str(), "hide") == 0) {
      set_property(Symbol("showing"), kFalse());
    }
    if (std::strcmp(msg.c_str(), "enter") == 0 ||
        std::strcmp(msg.c_str(), "load") == 0 ||
        std::strcmp(msg.c_str(), "finish_load") == 0) {
      sync_ready_label_visibility(mgr_, this);
    }
    return result;
  }
  // 2. Common engine built-in.
  DataNode out;
  if (handle_builtin(msg, args, out)) return out;
  // 3. Universal Object/ObjectDir messages (get/set/has/name/...).
  return ObjectDir::handle_property(msg, args);
}

bool UiObject::handle_builtin(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();

  // --- self visual/text state ---
  if (std::strcmp(m, "set_showing") == 0) {
    set_property(Symbol("showing"), args.size() ? arg0(args) : kTrue());
    return true;
  }
  if (std::strcmp(m, "get_showing") == 0) { out = get_property(Symbol("showing")); return true; }
  if (std::strcmp(m, "showing") == 0) {
    out = get_property(Symbol("showing"));
    if (out.empty()) out = kTrue();
    return true;
  }
  if (std::strcmp(m, "set_state") == 0) { set_property(Symbol("state"), arg0(args)); return true; }
  if (std::strcmp(m, "set_text") == 0 ||
      std::strcmp(m, "set_localized") == 0 ||
      std::strcmp(m, "set_localized_text") == 0 ||
      std::strcmp(m, "set_token") == 0) {
    set_property(Symbol("text"), arg0(args));
    return true;
  }
  if (std::strcmp(m, "set_display") == 0) {
    set_property(Symbol("display"), arg0(args));
    return true;
  }
  if (std::strcmp(m, "generate_headline") == 0) {
    DataNode text = DataNode::Str(generated_headline(mgr_, args));
    set_property(Symbol("headline_text"), text);
    out = text;
    return true;
  }
  if (std::strcmp(m, "set_headline") == 0) {
    set_property(Symbol("headline"), arg0(args));
    DataNode text = get_property(Symbol("headline_text"));
    if (text.empty()) text = DataNode::Str(default_headline(mgr_));
    if (Object* target = resolve_target(mgr_, this, arg0_name(args)))
      target->set_property(Symbol("text"), text);
    return true;
  }
  if (std::strcmp(m, "set_local_scale") == 0) {
    set_property(Symbol("scale_x"), args.size() > 0 ? args.at(0) : DataNode::Float(1.0f));
    set_property(Symbol("scale_y"), args.size() > 1 ? args.at(1) : DataNode::Float(1.0f));
    set_property(Symbol("scale_z"), args.size() > 2 ? args.at(2) : DataNode::Float(1.0f));
    return true;
  }
  if (std::strcmp(m, "set_provider") == 0) {
    set_property(Symbol("provider"), arg0(args));
    return true;
  }
  if (std::strcmp(m, "set_selected") == 0) {
    set_property(Symbol("selected_pos"), arg0(args));
    return true;
  }
  if (std::strcmp(m, "selected_pos") == 0) {
    out = get_property(Symbol("selected_pos"));
    if (!out.as_int()) out = DataNode::Int(0);
    return true;
  }
  if (std::strcmp(m, "num_lines") == 0) {
    out = get_property(Symbol("num_lines"));
    if (!out.as_int()) out = DataNode::Int(0);
    return true;
  }

  if (std::strcmp(m, "find") == 0) {
    const std::string path = arg0_name(args);
    out = path.empty() ? DataNode() : DataNode::Obj(find_path(path));
    return true;
  }

  // --- focus: a panel stores the focused child's name ---
  if (std::strcmp(m, "set_focus") == 0 || std::strcmp(m, "focus") == 0 ||
      std::strcmp(m, "update_focus") == 0) {
    set_property(Symbol("focus"), arg0(args));
    return true;
  }

  // --- enable/disable: a named child if it resolves, else self. Derived from
  //     the argument (panel {$this disable a.btn} vs component {b disable}),
  //     not from the class -- so it is correct for both without guessing. ---
  if (std::strcmp(m, "enable") == 0 || std::strcmp(m, "disable") == 0) {
    DataNode v = (m[0] == 'd') ? kTrue() : kFalse();  // disable -> disabled TRUE
    std::string child = arg0_name(args);
    Object* tgt = this;
    if (!child.empty()) {
      if (Object* c = find_path(child)) tgt = c;
    }
    tgt->set_property(Symbol("disabled"), v);
    return true;
  }

  if (std::strcmp(m, "set_skin_select") == 0) {
    const int player = int_arg(args, 0, 0);
    DataNode value = args.size() > 1 ? args.at(1) : (args.size() ? args.at(0) : kTrue());
    set_property(Symbol("skin_select"), value);
    set_property(indexed_symbol("skin_select", player), value);
    return true;
  }
  if (std::strcmp(m, "is_skin_select") == 0) {
    DataNode value = args.size() ? get_property(indexed_symbol("skin_select", int_arg(args, 0, 0)))
                                 : get_property(Symbol("skin_select"));
    out = truthy(value) ? kTrue() : kFalse();
    return true;
  }
  if (std::strcmp(m, "set_select_done") == 0) {
    const int player = int_arg(args, 0, 0);
    DataNode value = args.size() > 1 ? args.at(1) : (args.size() ? args.at(0) : kTrue());
    set_property(indexed_symbol("select_done", player), value);
    sync_ready_label_visibility(mgr_, this);
    return true;
  }
  if (std::strcmp(m, "is_select_done") == 0) {
    out = truthy(get_property(indexed_symbol("select_done", int_arg(args, 0, 0))))
              ? kTrue()
              : kFalse();
    return true;
  }
  if (std::strcmp(m, "get_instrument_type") == 0) {
    out = player_value(mgr_, int_arg(args, 0, 0), Symbol("get_instrument_type"),
                       Symbol("get_instrument_type"));
    if (out.empty()) out = DataNode::Sym(Symbol("guitar"));
    return true;
  }
  if (std::strcmp(m, "get_selected_guitar") == 0 ||
      std::strcmp(m, "get_selected_skin") == 0 ||
      std::strcmp(m, "get_num_guitars") == 0 ||
      std::strcmp(m, "get_num_skins") == 0) {
    Object* game = mgr_ ? mgr_->resolve_object(Symbol("game")) : nullptr;
    if (game) {
      if (std::strcmp(m, "get_selected_guitar") == 0) {
        out = player_value(mgr_, int_arg(args, 0, 0), Symbol("get_guitar"), Symbol("get_guitar"));
        return true;
      }
      if (std::strcmp(m, "get_selected_skin") == 0) {
        out = player_value(mgr_, int_arg(args, 0, 0), Symbol("get_guitar_skin"),
                           Symbol("get_guitar_skin"));
        return true;
      }
      if (std::strcmp(m, "get_num_guitars") == 0) {
        out = game->handle_property(Symbol("get_num_guitars"), args);
        return true;
      }
      out = game->handle_property(Symbol("get_num_skins"), args);
      return true;
    }
    out = std::strcmp(m, "get_selected_skin") == 0
              ? DataNode::Sym(Symbol("sg_cherry"))
              : DataNode::Sym(Symbol("sg"));
    return true;
  }
  if (std::strcmp(m, "price") == 0 || std::strcmp(m, "low_cost") == 0 ||
      std::strcmp(m, "high_cost") == 0) {
    Object* provider = mgr_ ? mgr_->resolve_object(Symbol("store_item_provider")) : nullptr;
    out = provider ? provider->handle_property(msg, args) : DataNode::Int(0);
    return true;
  }
  if (std::strcmp(m, "loaded_dir") == 0) {
    out = DataNode::Obj(this);
    return true;
  }
  if (std::strcmp(m, "can_buy_item") == 0) {
    out = kFalse();
    return true;
  }
  if (std::strcmp(m, "are_chars_loaded") == 0 ||
      std::strcmp(m, "is_char_loaded") == 0) {
    out = kTrue();
    return true;
  }
  if (std::strcmp(m, "show_char") == 0) {
    const int player = int_arg(args, 0, 0);
    DataNode value = args.size() > 1 ? args.at(1) : DataNode();
    set_property(indexed_symbol("char", player), value);
    set_property(indexed_symbol("char_outfit", player), value);
    out = kTrue();
    return true;
  }
  if (std::strcmp(m, "get_char") == 0) {
    const int player = int_arg(args, 0, 0);
    out = get_property(indexed_symbol("char", player));
    if (out.empty()) out = get_property(indexed_symbol("char_outfit", player));
    return true;
  }
  if (std::strcmp(m, "show_guitar") == 0) {
    const int player = int_arg(args, 0, 0);
    if (args.size() > 1) set_property(indexed_symbol("guitar", player), args.at(1));
    if (args.size() == 4) {
      set_property(indexed_symbol("guitar_proxy", player), args.at(2));
      set_property(indexed_symbol("guitar_filter", player), args.at(3));
    } else {
      if (args.size() > 2)
        set_property(indexed_symbol("guitar_skin", player), args.at(2));
      if (args.size() > 3)
        set_property(indexed_symbol("guitar_proxy", player), args.at(3));
      if (args.size() > 4)
        set_property(indexed_symbol("guitar_filter", player), args.at(4));
    }
    out = kTrue();
    return true;
  }
  if (std::strcmp(m, "set_env") == 0) {
    const int player = int_arg(args, 0, 0);
    set_property(indexed_symbol("env", player),
                 args.size() > 1 ? args.at(1) : DataNode());
    return true;
  }
  if (std::strcmp(m, "set_placer") == 0) {
    const int player = int_arg(args, 0, 0);
    set_property(indexed_symbol("placer", player),
                 args.size() > 1 ? args.at(1) : DataNode());
    return true;
  }
  if (std::strcmp(m, "set_door") == 0) {
    const int player = int_arg(args, 0, 0);
    set_property(indexed_symbol("door", player),
                 args.size() > 1 ? args.at(1) : DataNode());
    return true;
  }
  if (std::strcmp(m, "set_active") == 0) {
    set_property(Symbol("active"), args.size() ? args.at(0) : kTrue());
    return true;
  }
  if (std::strcmp(m, "set_frozen") == 0) {
    const int player = int_arg(args, 0, 0);
    DataNode value = args.size() > 1 ? args.at(1) : (args.size() ? args.at(0) : kTrue());
    set_property(indexed_symbol("frozen", player), value);
    return true;
  }
  if (std::strcmp(m, "force_select") == 0) {
    set_property(Symbol("focus"), arg0(args));
    return true;
  }
  if (std::strcmp(m, "start_tutorial") == 0) {
    initialize_tutorial_overlay_state(mgr_);
    set_property(Symbol("running"), kTrue());
    return true;
  }
  if (std::strcmp(m, "is_missing_guitar") == 0 ||
      std::strcmp(m, "is_vo_playing") == 0) {
    out = kFalse();
    return true;
  }
  if (std::strcmp(m, "update_tut_score") == 0) {
    return true;
  }
  if (std::strcmp(m, "set_frame") == 0) {
    set_property(Symbol("frame"), args.size() ? args.at(0) : DataNode::Float(0.0f));
    return true;
  }

  // --- transition lifecycle messages we accept as no-ops at this layer (a
  //     screen that needs them defines them as handlers, which win above). ---
  if (std::strcmp(m, "load") == 0 || std::strcmp(m, "unload") == 0 ||
      std::strcmp(m, "finish_load") == 0 || std::strcmp(m, "change_proxies") == 0 ||
      std::strcmp(m, "animate") == 0 ||
      std::strcmp(m, "set_mat") == 0 || std::strcmp(m, "set_tex") == 0 ||
      std::strcmp(m, "char_event") == 0 ||
      std::strcmp(m, "play") == 0 || std::strcmp(m, "hide") == 0 ||
      std::strcmp(m, "play_vo") == 0 || std::strcmp(m, "poll") == 0 ||
      std::strcmp(m, "set_paused") == 0 ||
      std::strcmp(m, "animate_video") == 0 || std::strcmp(m, "hide_models") == 0 ||
      std::strcmp(m, "update_helpbar") == 0 ||
      std::strcmp(m, "update_store_item_sold") == 0 ||
      std::strcmp(m, "update_total_cash_display") == 0) {
    return true;
  }

  return false;
}

// --- registration ----------------------------------------------------------
void register_ui_classes() {
  ClassReg& reg = ClassReg::instance();

  auto define_uiobject = [&](const char* name, const char* super) {
    Symbol cls(name);
    reg.define(cls, Symbol(super));
    reg.set_creator(cls, [cls] { return std::make_unique<UiObject>(cls); });
  };

  // Plain {new Object ...} containers.
  define_uiobject("Object", "");

  // The complete {new <Class>} screen/panel roster (STOCK_SURFACE.txt). Super
  // chain is provisional (-> Object) pending step-3 recomp grounding; it does
  // not affect verbatim loading/handler execution.
  static const char* kScreens[] = {"GHScreen", "MultiSelectScreen", "TrackBudgetScreen"};
  static const char* kPanels[] = {
      "GHPanel", "UIPanel", "MultiSelectPanel", "SliderPanel", "GuitarDisplayPanel",
      "EndGamePanel", "CharsysPanel", "GuitarSelectPanel", "TutorialPanel", "TrackPanel",
      "StorePanel", "MultiSelectListPanel", "MultiCharSelPanel", "MetaPanel", "LagPanel",
      "HudPanel", "HelpBarPanel", "GamePanel", "FadePanel", "CreditsPanel"};
  for (const char* s : kScreens) define_uiobject(s, "Object");
  for (const char* p : kPanels) define_uiobject(p, "Object");

  // MILO-side widget classes (instantiated from panel MILOs; some appear in
  // {new} too). Grouped under UIComponent for is_a() purposes.
  define_uiobject("UIComponent", "Object");
  static const char* kWidgets[] = {
      "UILabel", "UIButton", "UIPicture", "UIList", "UISlider", "CheckBox", "UIProxy",
      "ScreenMask", "UITrigger", "EventTrigger", "PanelDir", "UIColor",
      "BandLabel", "BandButton", "BandSlider", "BandList", "BandTextEntry",
      "BandCharacter", "BandPlacer", "BandStarDisplay", "BandScoreDisplay",
      "BandStreakDisplay", "BandStarMeterDir", "BandCrowdMeterDir"};
  for (const char* w : kWidgets) define_uiobject(w, "UIComponent");
}

}  // namespace ghogx::ui
