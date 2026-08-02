// engine/src/ui/ui_classes.cpp -- see ui_classes.h.

#include "ui/ui_classes.h"

#include "asset/milo_image.h"
#include "core/class_reg.h"
#include "ui/screen_manager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>

namespace ghogx::ui {

namespace {
DataNode kTrue() { return DataNode::Sym(Symbol("TRUE")); }
DataNode kFalse() { return DataNode::Sym(Symbol("FALSE")); }
constexpr int kSelectFrames = 10;  // ui_objects.dta: BandButton select_frames
DataNode arg0(const DataArray& a) { return a.size() > 0 ? a.at(0) : DataNode(); }
int arg0_int(const DataArray& a) {
  if (a.size() == 0) return 0;
  if (auto i = a.at(0).as_int()) return *i;
  if (auto f = a.at(0).as_float()) return static_cast<int>(*f);
  if (auto arr = a.at(0).as_array(); arr && arr->size() == 1) {
    if (auto i = arr->at(0).as_int()) return *i;
    if (auto f = arr->at(0).as_float()) return static_cast<int>(*f);
  }
  return 0;
}
float arg0_float(const DataArray& a) {
  if (a.size() == 0) return 0.0f;
  if (auto f = a.at(0).as_float()) return *f;
  if (auto i = a.at(0).as_int()) return static_cast<float>(*i);
  if (auto arr = a.at(0).as_array(); arr && arr->size() == 1) {
    if (auto f = arr->at(0).as_float()) return *f;
    if (auto i = arr->at(0).as_int()) return static_cast<float>(*i);
  }
  return 0.0f;
}
bool node_bool(const DataNode& n) {
  if (auto i = n.as_int()) return *i != 0;
  if (auto f = n.as_float()) return *f != 0.0f;
  if (auto s = n.as_symbol())
    return !(*s == Symbol("FALSE") || *s == Symbol("false") ||
             *s == Symbol("0"));
  if (auto s = n.as_string()) {
    return !(*s == "FALSE" || *s == "false" || *s == "0" || s->empty());
  }
  return n.as_object() != nullptr;
}
bool arg0_bool(const DataArray& a) {
  return a.size() > 0 && node_bool(a.at(0));
}
Symbol arg_symbol(const DataArray& a, std::size_t index,
                  Symbol fallback = Symbol()) {
  if (index >= a.size()) return fallback;
  if (auto s = a.at(index).as_symbol()) return *s;
  if (auto text = a.at(index).as_string()) return Symbol(*text);
  return fallback;
}
bool state_is_disabled(const DataNode& n) {
  if (auto i = n.as_int()) return *i == 2;  // UIComponent::kDisabled
  if (auto s = n.as_symbol())
    return *s == Symbol("disabled") || *s == Symbol("kDisabled");
  if (auto text = n.as_string())
    return *text == "disabled" || *text == "kDisabled";
  return false;
}
bool state_is_focused(const DataNode& n) {
  if (auto i = n.as_int()) return *i == 1;  // UIComponent::kFocused
  if (auto s = n.as_symbol())
    return *s == Symbol("focused") || *s == Symbol("kFocused");
  if (auto text = n.as_string())
    return *text == "focused" || *text == "kFocused";
  return false;
}
bool state_is_selecting(const DataNode& n) {
  if (auto i = n.as_int()) return *i == 3;  // UIComponent::kSelecting
  if (auto s = n.as_symbol())
    return *s == Symbol("selecting") || *s == Symbol("kSelecting");
  if (auto text = n.as_string())
    return *text == "selecting" || *text == "kSelecting";
  return false;
}
int state_code(const DataNode& n) {
  if (auto i = n.as_int()) return *i;
  if (state_is_focused(n)) return 1;
  if (state_is_disabled(n)) return 2;
  if (state_is_selecting(n)) return 3;
  if (auto s = n.as_symbol())
    if (*s == Symbol("selected") || *s == Symbol("kSelected")) return 4;
  if (auto text = n.as_string())
    if (*text == "selected" || *text == "kSelected") return 4;
  return 0;
}
bool component_disabled(const Object* obj) {
  if (!obj) return false;
  return node_bool(obj->get_property(Symbol("disabled"))) ||
         state_is_disabled(obj->get_property(Symbol("state")));
}
bool component_can_have_focus(const Object& obj) {
  if (!obj.is_a(Symbol("UIComponent"))) return false;
  const Symbol cls = obj.class_name();
  if (cls == Symbol("UILabel") || cls == Symbol("BandLabel")) return false;
  return true;
}
bool panel_show_focus_component(const ObjectDir& panel) {
  const DataNode show = panel.get_property(Symbol("show_focus_component"));
  return show.empty() || node_bool(show);
}
void set_component_state(Object* obj, Symbol state) {
  if (!obj || !state.valid()) return;
  obj->set_property(Symbol("state"), DataNode::Sym(state));
  obj->set_property(Symbol("disabled"),
                    state == Symbol("disabled") ? kTrue() : kFalse());
}
void start_component_select(Object& obj) {
  if (component_disabled(&obj)) return;
  if (!state_is_focused(obj.get_property(Symbol("state")))) return;
  set_component_state(&obj, Symbol("selecting"));
  obj.set_property(Symbol("select_frames_remaining"),
                   DataNode::Int(kSelectFrames));
}
void finish_component_select(Object& obj) {
  if (state_is_selecting(obj.get_property(Symbol("state"))))
    set_component_state(&obj, Symbol("focused"));
  obj.set_property(Symbol("select_frames_remaining"), DataNode::Int(0));
}
void poll_component_select(Object& obj) {
  if (!state_is_selecting(obj.get_property(Symbol("state")))) return;
  int frames =
      obj.get_property(Symbol("select_frames_remaining")).as_int().value_or(0);
  if (frames <= 1) {
    finish_component_select(obj);
    return;
  }
  obj.set_property(Symbol("select_frames_remaining"),
                   DataNode::Int(frames - 1));
}
std::string arg0_name(const DataArray& a) {
  return a.size() > 0 ? std::string(a.at(0).as_string().value_or("")) : std::string();
}
std::shared_ptr<DataArray> copy_args(const DataArray& args,
                                     std::size_t first = 0) {
  auto out = std::make_shared<DataArray>();
  for (std::size_t i = first; i < args.size(); ++i) out->push(args.at(i));
  return out;
}
DataNode array_value_or_tail(const DataArray& values, std::size_t first) {
  const std::size_t n = values.size() - first;
  if (n == 0) return DataNode();
  if (n == 1) return values.at(first);
  auto tail = std::make_shared<DataArray>();
  for (std::size_t i = first; i < values.size(); ++i) tail->push(values.at(i));
  return DataNode::Array(tail);
}
void store_keyed_anim_arg(Object& obj, const DataArray& keyed) {
  if (keyed.empty()) return;
  auto key = keyed.at(0).as_symbol();
  if (!key) return;
  obj.set_property(*key, array_value_or_tail(keyed, 1));
  if (*key == Symbol("range") && keyed.size() >= 3) {
    obj.set_property(Symbol("start_frame"), keyed.at(1));
    obj.set_property(Symbol("end_frame"), keyed.at(2));
  } else if (*key == Symbol("loop") && keyed.size() >= 3) {
    obj.set_property(Symbol("loop_start_frame"), keyed.at(1));
    obj.set_property(Symbol("loop_end_frame"), keyed.at(2));
  }
}
void store_vec3(Object& obj, Symbol key, const DataArray& args) {
  obj.set_property(key, DataNode::Array(copy_args(args, 0)));
  if (args.size() >= 1) obj.set_property(Symbol(std::string(key.c_str()) + "_x"), args.at(0));
  if (args.size() >= 2) obj.set_property(Symbol(std::string(key.c_str()) + "_y"), args.at(1));
  if (args.size() >= 3) obj.set_property(Symbol(std::string(key.c_str()) + "_z"), args.at(2));
}
int node_int(const DataNode& n, int fallback = 0) {
  if (auto i = n.as_int()) return *i;
  if (auto f = n.as_float()) return static_cast<int>(*f);
  return fallback;
}
float node_float(const DataNode& n, float fallback = 0.0f) {
  if (auto f = n.as_float()) return *f;
  if (auto i = n.as_int()) return static_cast<float>(*i);
  return fallback;
}
DataArray one_arg(DataNode node) {
  DataArray args;
  args.push(std::move(node));
  return args;
}
DataNode game_message(ScreenManager* mgr, Symbol msg, const DataArray& args) {
  if (!mgr) return DataNode();
  Object* game = mgr->resolve_object(Symbol("game"));
  return game ? game->handle_property(msg, args) : DataNode();
}
Symbol game_symbol(ScreenManager* mgr, Symbol msg, Symbol fallback = Symbol()) {
  DataNode out = game_message(mgr, msg, DataArray());
  if (auto s = out.as_symbol()) return *s;
  if (auto text = out.as_string()) return Symbol(*text);
  return fallback;
}
Object* store_provider_for(ScreenManager* mgr, Symbol category) {
  if (!mgr) return nullptr;
  if (category == Symbol("guitar"))
    return mgr->resolve_object(Symbol("store_guitar_provider"));
  if (category == Symbol("skin"))
    return mgr->resolve_object(Symbol("store_skin_provider"));
  if (category == Symbol("video"))
    return mgr->resolve_object(Symbol("store_video_provider"));
  Object* provider = mgr->resolve_object(Symbol("store_item_provider"));
  if (provider && category.valid()) {
    DataArray set_category;
    set_category.push(DataNode::Sym(category));
    provider->handle_property(Symbol("set_category"), set_category);
  }
  return provider;
}
int store_provider_count(Object* provider) {
  if (!provider) return 0;
  return std::max(0, node_int(provider->handle_property(Symbol("list_length"),
                                                        DataArray())));
}
Symbol store_provider_symbol(Object* provider, int index) {
  if (!provider || index < 0) return Symbol();
  DataArray args;
  args.push(DataNode::Int(index));
  return provider->handle_property(Symbol("get_symbol"), args)
      .as_symbol()
      .value_or(Symbol());
}
int store_provider_price(Object* provider, int index) {
  if (!provider || index < 0) return 0;
  DataArray args;
  args.push(DataNode::Int(index));
  return node_int(provider->handle_property(Symbol("price"), args));
}
int store_category_price_extreme(ScreenManager* mgr, Symbol category,
                                 bool high) {
  Object* provider = store_provider_for(mgr, category);
  const int count = store_provider_count(provider);
  bool has_price = false;
  int best = 0;
  for (int i = 0; i < count; ++i) {
    const int price = store_provider_price(provider, i);
    if (!has_price || (high ? price > best : price < best)) {
      best = price;
      has_price = true;
    }
  }
  return has_price ? best : 0;
}
int store_item_price(ScreenManager* mgr, Symbol category, Symbol item) {
  Object* provider = store_provider_for(mgr, category);
  const int count = store_provider_count(provider);
  for (int i = 0; i < count; ++i) {
    if (store_provider_symbol(provider, i) == item)
      return store_provider_price(provider, i);
  }
  return 0;
}
std::string indexed_key(const char* prefix, int player) {
  return std::string(prefix) + "_" + std::to_string(std::max(0, player));
}
int selected_pos(Object& obj) {
  return node_int(obj.get_property(Symbol("selected_pos")), 0);
}
int provider_num_data(Object& obj) {
  // UIList's constructor defaults mNumData to 100; the actual CreditsPanel
  // provider overrides it after load, and menu_app seeds the real count when
  // the stock config table is available.
  int n = node_int(obj.get_property(Symbol("provider_num_data")), -1);
  if (n < 0) n = node_int(obj.get_property(Symbol("num_data")), 100);
  return std::max(0, n);
}
int num_display(Object& obj) {
  return std::max(1, node_int(obj.get_property(Symbol("num_display")), 5));
}
float auto_scroll_pause(Object& obj) {
  // UIList ctor: mAutoScrollPause(2.0f); rev < 14 lists do not serialize it.
  const float v = node_float(obj.get_property(Symbol("auto_scroll_pause")), 2.0f);
  return v > 0.0f ? v : 2.0f;
}
float list_speed(Object& obj) {
  // UIListState ctor: mSpeed(0.25f), overridden by the serialized UIList tail.
  const float v = node_float(obj.get_property(Symbol("speed")), 0.25f);
  return v > 0.0f ? v : 0.25f;
}
int list_max_first_showing(Object& obj) {
  return std::max(0, provider_num_data(obj) - num_display(obj));
}
int list_min_display(Object& obj) {
  return std::clamp(node_int(obj.get_property(Symbol("min_display")), 0), 0,
                    num_display(obj) - 1);
}
int list_max_display(Object& obj) {
  int max_display = node_int(obj.get_property(Symbol("max_display")), -1);
  if (max_display < 0) max_display = num_display(obj) - 1;
  return std::clamp(max_display, list_min_display(obj), num_display(obj) - 1);
}
int list_first_showing(Object& obj) {
  if (auto i = obj.get_property(Symbol("first_showing")).as_int())
    return std::clamp(*i, 0, list_max_first_showing(obj));
  return 0;
}
int list_target_showing(Object& obj) {
  if (auto i = obj.get_property(Symbol("target_showing")).as_int())
    return std::clamp(*i, 0, list_max_first_showing(obj));
  return list_first_showing(obj);
}
int list_first_for_selected(Object& obj, int selected) {
  int first = list_first_showing(obj);
  const int min_display = list_min_display(obj);
  const int max_display = list_max_display(obj);
  if (selected - first > max_display)
    first = selected - max_display;
  else if (selected - first < min_display)
    first = selected - min_display;
  return std::clamp(first, 0, list_max_first_showing(obj));
}
void set_list_settled(Object& obj, int pos, int first) {
  const int max_pos = std::max(0, provider_num_data(obj) - 1);
  pos = std::clamp(pos, 0, max_pos);
  first = std::clamp(first, 0, list_max_first_showing(obj));
  obj.set_property(Symbol("selected_pos"), DataNode::Int(pos));
  obj.set_property(Symbol("first_showing"), DataNode::Int(first));
  obj.set_property(Symbol("target_showing"), DataNode::Int(first));
  obj.set_property(Symbol("selected_display"), DataNode::Int(pos - first));
  obj.set_property(Symbol("current_scroll"), DataNode::Int(0));
  obj.set_property(Symbol("scroll_step_percent"), DataNode::Float(1.0f));
  obj.set_property(Symbol("scroll_start_seconds"), DataNode::Float(-1.0f));
  obj.set_property(Symbol("scroll_end_seconds"), DataNode::Float(-1.0f));
}
void set_selected(Object& obj, int pos) {
  set_list_settled(obj, pos, list_first_for_selected(obj, pos));
}
bool list_is_scrolling(Object& obj) {
  return list_first_showing(obj) != list_target_showing(obj);
}
bool update_list_scroll(Object& obj, float now) {
  const int first = list_first_showing(obj);
  const int target = list_target_showing(obj);
  if (first == target) {
    obj.set_property(Symbol("current_scroll"), DataNode::Int(0));
    obj.set_property(Symbol("scroll_step_percent"), DataNode::Float(1.0f));
    return false;
  }
  const float start =
      node_float(obj.get_property(Symbol("scroll_start_seconds")), now);
  const float end = node_float(obj.get_property(Symbol("scroll_end_seconds")),
                               start + list_speed(obj));
  if (now >= end || end <= start) {
    set_list_settled(obj, selected_pos(obj), target);
    return true;
  }
  const float pct = std::clamp((now - start) / (end - start), 0.0f, 1.0f);
  obj.set_property(Symbol("scroll_step_percent"), DataNode::Float(pct));
  obj.set_property(Symbol("current_scroll"),
                   DataNode::Int(target > first ? 1 : -1));
  obj.set_property(Symbol("selected_display"),
                   DataNode::Int(selected_pos(obj) - first));
  return false;
}
void start_list_scroll_to(Object& obj, int pos, int target_first, float now,
                          float duration) {
  const int max_pos = std::max(0, provider_num_data(obj) - 1);
  pos = std::clamp(pos, 0, max_pos);
  target_first = std::clamp(target_first, 0, list_max_first_showing(obj));
  update_list_scroll(obj, now);
  const int first = list_first_showing(obj);
  obj.set_property(Symbol("selected_pos"), DataNode::Int(pos));
  if (first == target_first || duration <= 0.0f) {
    set_list_settled(obj, pos, target_first);
    return;
  }
  obj.set_property(Symbol("target_showing"), DataNode::Int(target_first));
  obj.set_property(Symbol("selected_display"), DataNode::Int(pos - first));
  obj.set_property(Symbol("current_scroll"),
                   DataNode::Int(target_first > first ? 1 : -1));
  obj.set_property(Symbol("scroll_step_percent"), DataNode::Float(0.0f));
  obj.set_property(Symbol("scroll_start_seconds"), DataNode::Float(now));
  obj.set_property(Symbol("scroll_end_seconds"),
                   DataNode::Float(now + duration));
}
void list_scroll(Object& obj, int dir, ScreenManager* mgr) {
  if (dir == 0) return;
  const float now = mgr ? mgr->ui_seconds() : 0.0f;
  const int max_pos = std::max(0, provider_num_data(obj) - 1);
  const int next = std::clamp(selected_pos(obj) + dir, 0, max_pos);
  start_list_scroll_to(obj, next, list_first_for_selected(obj, next), now,
                       list_speed(obj));
}
void list_auto_scroll_step(Object& obj, ScreenManager* mgr) {
  const float now = mgr ? mgr->ui_seconds() : 0.0f;
  int dir = node_int(obj.get_property(Symbol("auto_scroll_dir")), 1);
  if (dir == 0) dir = 1;
  const int first = list_first_showing(obj);
  const int max_first = list_max_first_showing(obj);
  if ((dir > 0 && first >= max_first) || (dir < 0 && first <= 0)) {
    dir = -dir;
    obj.set_property(Symbol("auto_scroll_dir"), DataNode::Int(dir));
    obj.set_property(Symbol("auto_scroll_deadline"),
                     DataNode::Float(now + auto_scroll_pause(obj)));
    return;
  }
  const int selected_display =
      node_int(obj.get_property(Symbol("selected_display")),
               selected_pos(obj) - first);
  const int target_first = std::clamp(first + dir, 0, max_first);
  const int max_pos = std::max(0, provider_num_data(obj) - 1);
  const int target_selected =
      std::clamp(target_first + selected_display, 0, max_pos);
  start_list_scroll_to(obj, target_selected, target_first, now,
                       list_speed(obj));
  obj.set_property(Symbol("auto_scroll_deadline"), DataNode::Float(-1.0f));
}
void list_auto_scroll(Object& obj, ScreenManager* mgr) {
  if (provider_num_data(obj) <= num_display(obj)) {
    obj.set_property(Symbol("auto_scrolling"), DataNode::Int(0));
    return;
  }
  obj.set_property(Symbol("auto_scrolling"), DataNode::Int(1));
  obj.set_property(Symbol("auto_scroll_dir"), DataNode::Int(1));
  const float now = mgr ? mgr->ui_seconds() : 0.0f;
  update_list_scroll(obj, now);
  obj.set_property(Symbol("auto_scroll_deadline"),
                   DataNode::Float(now + auto_scroll_pause(obj)));
}
void list_poll(Object& obj, ScreenManager* mgr) {
  const float now = mgr ? mgr->ui_seconds() : 0.0f;
  const bool completed_scroll = update_list_scroll(obj, now);
  if (!node_bool(obj.get_property(Symbol("auto_scrolling")))) return;
  if (list_is_scrolling(obj)) return;
  if (completed_scroll) {
    const int first = list_first_showing(obj);
    int dir = node_int(obj.get_property(Symbol("auto_scroll_dir")), 1);
    if (dir == 0) dir = 1;
    const int edge = dir > 0 ? list_max_first_showing(obj) : 0;
    if (first == edge) {
      obj.set_property(Symbol("auto_scroll_dir"), DataNode::Int(-dir));
      obj.set_property(Symbol("auto_scroll_deadline"),
                       DataNode::Float(now + auto_scroll_pause(obj)));
    } else {
      list_auto_scroll_step(obj, mgr);
    }
    return;
  }
  float deadline = node_float(obj.get_property(Symbol("auto_scroll_deadline")), -1.0f);
  if (deadline < 0.0f) {
    deadline = now + auto_scroll_pause(obj);
    obj.set_property(Symbol("auto_scroll_deadline"), DataNode::Float(deadline));
  }
  if (now < deadline) return;
  list_auto_scroll_step(obj, mgr);
}
bool credits_panel_lifecycle(Symbol cls, Symbol msg) {
  if (cls != Symbol("CreditsPanel")) return false;
  return msg == Symbol("enter") || msg == Symbol("poll") ||
         msg == Symbol("exit");
}
bool slider_class(Symbol cls) {
  return cls == Symbol("UISlider") || cls == Symbol("BandSlider");
}

std::vector<Symbol> symbol_array(const DataNode& node) {
  std::vector<Symbol> out;
  auto values = node.as_array();
  if (!values) return out;
  out.reserve(values->size());
  for (std::size_t i = 0; i < values->size(); ++i) {
    Symbol value = values->at(i).as_symbol().value_or(Symbol());
    if (!value.valid()) {
      if (auto text = values->at(i).as_string()) value = Symbol(*text);
      else if (Object* object = values->at(i).as_object()) value = object->name();
    }
    if (value.valid()) out.push_back(value);
  }
  return out;
}

Symbol button_selection(Symbol button) {
  std::string value = button.c_str();
  constexpr const char* suffix = ".btn";
  if (value.size() >= 4 && value.compare(value.size() - 4, 4, suffix) == 0)
    value.resize(value.size() - 4);
  return Symbol(value);
}

Symbol selection_button(Symbol selection) {
  std::string value = selection.c_str();
  if (value.size() < 4 || value.compare(value.size() - 4, 4, ".btn") != 0)
    value += ".btn";
  return Symbol(value);
}

bool text_entry_class(Symbol cls) {
  return cls == Symbol("UITextEntry") || cls == Symbol("BandTextEntry");
}

// GH2's BandTextEntry owns a committed prefix and one active character.
// manage_bands.dtb confirms the input contract: Green advances a character,
// Red deletes, and strum Up/Down changes the active character. Keep the
// committed value separate from the displayed prefix+active-character so
// get_text/length/no_text_entered expose the value the stock scripts expect.
constexpr const char* kTextEntryCharacters =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !?.'";
constexpr int kDefaultTextEntryMaxLength = 20;

std::string text_entry_characters(Object& obj) {
  std::string characters =
      std::string(obj.get_property(Symbol("characters"))
                      .as_string()
                      .value_or(""));
  if (characters.empty()) characters = kTextEntryCharacters;
  return characters;
}

std::string text_entry_prefix(Object& obj) {
  return std::string(
      obj.get_property(Symbol("text_entry_value")).as_string().value_or(""));
}

bool text_entry_active_accepted(Object& obj) {
  return node_bool(
      obj.get_property(Symbol("text_entry_active_accepted")));
}

int text_entry_character_index(Object& obj) {
  const int count = static_cast<int>(text_entry_characters(obj).size());
  int index =
      obj.get_property(Symbol("text_entry_character_index"))
          .as_int()
          .value_or(0);
  index %= count;
  if (index < 0) index += count;
  return index;
}

std::string text_entry_value(Object& obj) {
  std::string value = text_entry_prefix(obj);
  if (text_entry_active_accepted(obj)) {
    const std::string characters = text_entry_characters(obj);
    value.push_back(characters[text_entry_character_index(obj)]);
  }
  return value;
}

int text_entry_max_length(Object& obj) {
  return std::max(
      1, obj.get_property(Symbol("max_length"))
             .as_int()
             .value_or(kDefaultTextEntryMaxLength));
}

void refresh_text_entry_display(Object& obj) {
  std::string display = text_entry_prefix(obj);
  const bool active_accepted = text_entry_active_accepted(obj);
  if (active_accepted ||
      (!node_bool(obj.get_property(Symbol("done"))) &&
       static_cast<int>(display.size()) < text_entry_max_length(obj))) {
    const std::string characters = text_entry_characters(obj);
    display.push_back(characters[text_entry_character_index(obj)]);
  }
  obj.set_property(Symbol("text"), DataNode::Str(std::move(display)));
}

void initialize_text_entry(Object& obj) {
  if (!obj.has_property(Symbol("text_entry_value"))) {
    obj.set_property(Symbol("text_entry_value"), DataNode::Str(""));
    obj.set_property(Symbol("text_entry_active_accepted"), DataNode::Int(0));
  }
  if (!obj.has_property(Symbol("text_entry_character_index")))
    obj.set_property(Symbol("text_entry_character_index"), DataNode::Int(0));
  refresh_text_entry_display(obj);
}

void set_text_entry_value(Object& obj, std::string value) {
  const int max_length = text_entry_max_length(obj);
  if (static_cast<int>(value.size()) > max_length)
    value.resize(static_cast<std::size_t>(max_length));
  int current_index = 0;
  bool active_accepted = false;
  if (!value.empty()) {
    const char current = value.back();
    value.pop_back();
    const std::string characters = text_entry_characters(obj);
    const std::size_t found = characters.find(current);
    if (found != std::string::npos) {
      current_index = static_cast<int>(found);
      active_accepted = true;
    } else {
      value.push_back(current);
    }
  }
  obj.set_property(Symbol("text_entry_value"), DataNode::Str(std::move(value)));
  obj.set_property(Symbol("text_entry_character_index"),
                   DataNode::Int(current_index));
  obj.set_property(Symbol("text_entry_active_accepted"),
                   DataNode::Int(active_accepted ? 1 : 0));
  refresh_text_entry_display(obj);
}

void scroll_text_entry_character(Object& obj, int direction) {
  initialize_text_entry(obj);
  const int count = static_cast<int>(text_entry_characters(obj).size());
  int index =
      (text_entry_character_index(obj) + (direction >= 0 ? 1 : -1)) % count;
  if (index < 0) index += count;
  obj.set_property(Symbol("text_entry_character_index"),
                   DataNode::Int(index));
  refresh_text_entry_display(obj);
}

void accept_text_entry_character(Object& obj) {
  initialize_text_entry(obj);
  std::string prefix = text_entry_prefix(obj);
  const std::string value = text_entry_value(obj);
  if (static_cast<int>(value.size()) >= text_entry_max_length(obj)) return;
  const std::string characters = text_entry_characters(obj);
  prefix.push_back(characters[text_entry_character_index(obj)]);
  obj.set_property(Symbol("text_entry_value"),
                   DataNode::Str(std::move(prefix)));
  obj.set_property(Symbol("text_entry_active_accepted"), DataNode::Int(0));
  refresh_text_entry_display(obj);
}

void delete_text_entry_character(Object& obj) {
  initialize_text_entry(obj);
  if (text_entry_active_accepted(obj)) {
    obj.set_property(Symbol("text_entry_active_accepted"), DataNode::Int(0));
  } else {
    std::string prefix = text_entry_prefix(obj);
    if (!prefix.empty()) {
      const char current = prefix.back();
      prefix.pop_back();
      const std::string characters = text_entry_characters(obj);
      const std::size_t found = characters.find(current);
      if (found != std::string::npos) {
        obj.set_property(
            Symbol("text_entry_character_index"),
            DataNode::Int(static_cast<int>(found)));
      }
      obj.set_property(Symbol("text_entry_value"),
                       DataNode::Str(std::move(prefix)));
    }
  }
  refresh_text_entry_display(obj);
}

int slider_steps(Object& obj) {
  return std::max(1, node_int(obj.get_property(Symbol("num_steps")), 1));
}
int slider_current(Object& obj) {
  return std::clamp(node_int(obj.get_property(Symbol("current")), 0), 0,
                    slider_steps(obj) - 1);
}
int slider_stored_current(Object& obj) {
  return node_int(obj.get_property(Symbol("scroll_selected_current")), -1);
}
bool slider_scroll_selected(Object& obj) {
  return slider_stored_current(obj) >= 0;
}
void slider_store_selection(Object& obj) {
  if (!slider_scroll_selected(obj))
    obj.set_property(Symbol("scroll_selected_current"),
                     DataNode::Int(slider_current(obj)));
}
void slider_reset_selection(Object& obj) {
  obj.set_property(Symbol("scroll_selected_current"), DataNode::Int(-1));
}
void slider_undo_selection(Object& obj) {
  const int stored = slider_stored_current(obj);
  if (stored >= 0)
    obj.set_property(Symbol("current"),
                     DataNode::Int(std::clamp(stored, 0,
                                              slider_steps(obj) - 1)));
  slider_reset_selection(obj);
}
float slider_frame(Object& obj) {
  const int steps = slider_steps(obj);
  if (steps == 1) return 0.0f;
  return static_cast<float>(slider_current(obj)) /
         static_cast<float>(steps - 1);
}
void set_slider_frame(Object& obj, float frame) {
  const int steps = slider_steps(obj);
  const float clamped = std::clamp(frame, 0.0f, 1.0f);
  const int current = static_cast<int>(
      std::lround(clamped * static_cast<float>(steps - 1)));
  obj.set_property(Symbol("current"), DataNode::Int(current));
}

const std::array<Symbol, 14>& tutorial_states() {
  // GH2 tutorials.dta TUTORIAL_STATES. TutorialPanel owns the state-step
  // message; the stock DTB owns this actual lesson order.
  static const std::array<Symbol, 14> kStates = {
      Symbol("intro"),       Symbol("playing_notes"),
      Symbol("diff_notes"),  Symbol("held_notes"),
      Symbol("chords"),      Symbol("interface"),
      Symbol("wrapup"),      Symbol("star_intro"),
      Symbol("star_combos"), Symbol("whammy"),
      Symbol("wail"),        Symbol("freestyle"),
      Symbol("sustain"),     Symbol("pulloff")};
  return kStates;
}

Symbol node_symbol_value(const DataNode& node) {
  if (auto sym = node.as_symbol()) return *sym;
  if (auto text = node.as_string()) return Symbol(*text);
  return Symbol();
}

Object* child_from_node(ObjectDir& dir, const DataNode& node) {
  if (Object* obj = node.as_object()) return obj;
  Symbol name = node_symbol_value(node);
  return name.valid() ? dir.find_path(name.c_str()) : nullptr;
}

bool sync_panel_focus_state(ObjectDir& panel) {
  Symbol focus = node_symbol_value(panel.get_property(Symbol("focus")));
  if (!focus.valid()) return false;
  Object* focused = panel.find_path(focus.c_str());
  if (!focused) return false;

  for (std::size_t i = 0; i < panel.size(); ++i) {
    Object* child = panel.at(i);
    if (!child || child == focused) continue;
    if (state_is_focused(child->get_property(Symbol("state"))) &&
        !component_disabled(child)) {
      set_component_state(child, Symbol("normal"));
    }
  }
  if (!component_disabled(focused)) {
    set_component_state(focused, panel_show_focus_component(panel)
                                     ? Symbol("focused")
                                     : Symbol("normal"));
  }
  return true;
}

void poll_component_tree(Object* obj) {
  if (!obj) return;
  poll_component_select(*obj);
  if (auto* dir = dynamic_cast<ObjectDir*>(obj)) {
    for (std::size_t i = 0; i < dir->size(); ++i)
      poll_component_tree(dir->at(i));
  }
}

bool set_panel_focus_component(ObjectDir& panel, const DataNode& target_node) {
  Object* target = child_from_node(panel, target_node);
  Symbol target_name = target ? target->name() : node_symbol_value(target_node);
  if (!target_name.valid()) return false;

  Symbol old_name = node_symbol_value(panel.get_property(Symbol("focus")));
  Object* old_focus = old_name.valid() ? panel.find_path(old_name.c_str()) : nullptr;
  if (old_focus && old_focus != target && !component_disabled(old_focus))
    set_component_state(old_focus, Symbol("normal"));

  panel.set_property(Symbol("focus"), DataNode::Sym(target_name));
  if (target && !component_disabled(target)) {
    set_component_state(target, panel_show_focus_component(panel)
                                    ? Symbol("focused")
                                    : Symbol("normal"));
  }
  return true;
}

bool set_screen_focus_target(UiObject& screen, const DataNode& target_node) {
  ScreenManager* mgr = screen.manager();
  if (!mgr) return false;
  Object* target = target_node.as_object();
  Symbol target_name = target ? target->name() : node_symbol_value(target_node);
  if (!target_name.valid()) return false;

  DataNode panels = screen.get_property(Symbol("panels"));
  if (auto arr = panels.as_array()) {
    for (std::size_t i = 0; i < arr->size(); ++i) {
      Symbol panel_name = arr->at(i).as_symbol().value_or(Symbol());
      if (!panel_name.valid()) continue;
      Object* panel = mgr->find_object(panel_name);
      if (!panel) continue;
      if (target_name == panel_name || target == panel) {
        screen.set_property(Symbol("focus"), DataNode::Sym(panel_name));
        if (auto* dir = dynamic_cast<ObjectDir*>(panel))
          sync_panel_focus_state(*dir);
        return true;
      }
      auto* dir = dynamic_cast<ObjectDir*>(panel);
      Object* child = dir ? dir->find(target_name) : nullptr;
      if (child && (!target || child == target)) {
        screen.set_property(Symbol("focus"), DataNode::Sym(panel_name));
        set_panel_focus_component(*dir, DataNode::Obj(child));
        return true;
      }
    }
  }

  if (mgr->find_object(target_name)) {
    screen.set_property(Symbol("focus"), DataNode::Sym(target_name));
    return true;
  }
  return false;
}

int tutorial_state_index(Symbol state) {
  const auto& states = tutorial_states();
  for (std::size_t i = 0; i < states.size(); ++i)
    if (states[i] == state) return static_cast<int>(i);
  return -1;
}

Symbol tutorial_current_lesson(Object& obj) {
  for (Symbol key : {Symbol("lesson"), Symbol("tutorial_state"),
                     Symbol("state")}) {
    Symbol state = node_symbol_value(obj.get_property(key));
    if (tutorial_state_index(state) >= 0) return state;
  }
  return Symbol("intro");
}

Symbol tutorial_next_state(Object& obj, int inc) {
  const auto& states = tutorial_states();
  int index = tutorial_state_index(tutorial_current_lesson(obj));
  if (index < 0) index = 0;
  const int count = static_cast<int>(states.size());
  int next = (index + inc) % count;
  if (next < 0) next += count;
  return states[static_cast<std::size_t>(next)];
}

Object* dialog_child(UiObject& dialog, const char* name) {
  return dialog.find(Symbol(name));
}

std::string node_text(const DataNode& node) {
  return std::string(node.as_string().value_or(""));
}

std::string localized_node_text(UiObject& obj, const DataNode& node) {
  Symbol token = node.as_symbol().value_or(Symbol());
  if (!token.valid()) {
    if (auto text = node.as_string()) token = Symbol(*text);
  }
  if (token.valid() && obj.manager()) return obj.manager()->localize(token);
  return node_text(node);
}

void replace_all(std::string& text, const std::string& needle,
                 const std::string& replacement) {
  if (needle.empty()) return;
  std::size_t pos = 0;
  while ((pos = text.find(needle, pos)) != std::string::npos) {
    text.replace(pos, needle.size(), replacement);
    pos += replacement.size();
  }
}

std::string headline_placeholder(Symbol key) {
  std::string upper;
  for (char c : std::string(key.c_str())) {
    upper.push_back(c == '-' ? '_' : static_cast<char>(
        std::toupper(static_cast<unsigned char>(c))));
  }
  return "{GH_" + upper + "}";
}

Object* dialog_button_from_node(UiObject& dialog, const DataNode& node) {
  if (Object* obj = node.as_object()) return obj;
  std::string name = node_text(node);
  if (name.empty()) return nullptr;
  if (name.find(".btn") == std::string::npos)
    name = "dl_" + name + ".btn";
  return dialog.find_path(name);
}

void dialog_hide_button(Object* button) {
  if (!button) return;
  button->set_property(Symbol("showing"), kFalse());
  button->set_property(Symbol("state"), DataNode::Sym(Symbol("kDisabled")));
  button->set_property(Symbol("disabled"), kTrue());
}

void dialog_set_button_text(Object* button, const DataNode& text) {
  if (!button) return;
  button->set_property(Symbol("showing"), kTrue());
  button->set_property(Symbol("state"), DataNode::Sym(Symbol("kNormal")));
  button->set_property(Symbol("disabled"), kFalse());
  button->set_property(Symbol("text"), text);
}

void dialog_set_button_focus(UiObject& dialog, Object* button) {
  if (!button) return;
  dialog.set_property(Symbol("focus"), DataNode::Sym(button->name()));
  button->set_property(Symbol("state"), DataNode::Sym(Symbol("kFocused")));
}

bool handle_dialog_builtin(UiObject& dialog, Symbol msg, const DataArray& args,
                           DataNode& out) {
  if (dialog.name() != Symbol("dialog")) return false;
  const char* m = msg.c_str();
  Object* button1 = dialog_child(dialog, "dl_button1.btn");
  Object* button2 = dialog_child(dialog, "dl_button2.btn");
  Object* title = dialog_child(dialog, "dl_title.lbl");
  Object* message = dialog_child(dialog, "dl_message.lbl");

  auto set_message = [&](const DataNode& token_node) {
    if (!message) return;
    Symbol token = token_node.as_symbol().value_or(
        Symbol(token_node.as_string().value_or("")));
    std::string text = token.valid() && dialog.manager()
                           ? dialog.manager()->localize(token)
                           : node_text(token_node);
    message->set_property(Symbol("text"), DataNode::Str(text));
  };

  if (std::strcmp(m, "set_message") == 0) {
    set_message(arg0(args));
    return true;
  }

  if (std::strcmp(m, "get_button") == 0) {
    out = DataNode::Str("dl_" + arg0_name(args) + ".btn");
    return true;
  }
  if (std::strcmp(m, "set_button_text") == 0) {
    if (args.size() >= 2)
      dialog_set_button_text(dialog_button_from_node(dialog, args.at(0)),
                             args.at(1));
    return true;
  }
  if (std::strcmp(m, "hide_button") == 0) {
    if (args.size() >= 1)
      dialog_hide_button(dialog_button_from_node(dialog, args.at(0)));
    return true;
  }
  if (std::strcmp(m, "set_button_focus") == 0) {
    if (args.size() >= 1)
      dialog_set_button_focus(dialog,
                              dialog_button_from_node(dialog, args.at(0)));
    return true;
  }

  if (std::strcmp(m, "setup") == 0) {
    const DataNode btn1 = args.size() > 0 ? args.at(0) : DataNode::Str("");
    const DataNode btn2 = args.size() > 1 ? args.at(1) : DataNode::Str("");
    const Symbol def = arg_symbol(args, 2, Symbol("none"));
    const DataNode msg_token = args.size() > 3 ? args.at(3) : DataNode();
    const DataNode title_token = args.size() > 4 ? args.at(4) : DataNode();

    if (node_text(btn1).empty())
      dialog_hide_button(button1);
    else
      dialog_set_button_text(button1, btn1);
    if (node_text(btn2).empty())
      dialog_hide_button(button2);
    else
      dialog_set_button_text(button2, btn2);

    if (def == Symbol("button1"))
      dialog_set_button_focus(dialog, button1);
    else if (def == Symbol("button2"))
      dialog_set_button_focus(dialog, button2);
    else
      dialog.set_property(Symbol("focus"), DataNode());

    set_message(msg_token);
    if (title) title->set_property(Symbol("text"), title_token);
    return true;
  }

  if (std::strcmp(m, "message") == 0) {
    DataArray setup;
    setup.push(DataNode::Str(""));
    setup.push(DataNode::Str(""));
    setup.push(DataNode::Sym(Symbol("none")));
    setup.push(args.size() > 0 ? args.at(0) : DataNode());
    setup.push(args.size() > 1 ? args.at(1) : DataNode());
    return handle_dialog_builtin(dialog, Symbol("setup"), setup, out);
  }

  return false;
}
}  // namespace

DataNode UiObject::handle_property(Symbol msg, const DataArray& args) {
  DataNode out;
  if (handle_dialog_builtin(*this, msg, args, out)) return out;

  // 1. A scripted handler block from the DTB wins -- run verbatim.
  if (auto h = handler(msg)) {
    out = mgr_ ? mgr_->run_object_handler(h, this, args) : DataNode();
    // CreditsPanel::Enter/Poll/Exit are class methods in ihatecompvir's source;
    // UIPanel's authored handler data runs as part of that class lifecycle, not
    // instead of it. Preserve both for the stock credits menu.
    if (credits_panel_lifecycle(cls_, msg) ||
        ((cls_ == Symbol("MultiSelectPanel") ||
          cls_ == Symbol("MultiCharSelPanel") ||
          cls_ == Symbol("CharsysPanel") ||
          cls_ == Symbol("GuitarSelectPanel")) &&
         msg == Symbol("enter"))) {
      DataNode ignored;
      handle_builtin(msg, args, ignored);
    }
    if (msg == Symbol("enter")) sync_panel_focus_state(*this);
    if (msg == Symbol("poll")) poll_component_tree(this);
    return out;
  }
  // 2. Common engine built-in.
  if (handle_builtin(msg, args, out)) return out;
  // Stock DTB message syntax uses a zero-argument property name as its getter
  // (for example `{$this title}` and `{game_screen attract_mode}`). Scripted
  // handlers and native class methods above retain priority; only unresolved
  // zero-argument messages fall through to the authored property table.
  if (args.empty() && has_property(msg)) return get_property(msg);
  // 3. Universal Object/ObjectDir messages (get/set/has/name/...).
  return ObjectDir::handle_property(msg, args);
}

bool UiObject::handle_builtin(Symbol msg, const DataArray& args, DataNode& out) {
  const char* m = msg.c_str();

  // CharsysPanel is the native asynchronous character-display boundary used by
  // sel_character.dtb, multiplayer.dtb, and store.dtb.  Stock scripts select an
  // outfit with show_char, assign authored BandPlacer/Environ objects after the
  // screen transition, and poll is_char_loaded to drive their LOADING labels.
  // The D3D menu presenter consumes these per-slot properties and reports the
  // synchronous PC-side decode result through char_loaded_N.
  if (cls_ == Symbol("CharsysPanel")) {
    if (std::strcmp(m, "show_char") == 0) {
      const int player = arg0_int(args);
      const DataNode outfit = args.size() > 1 ? args.at(1) : DataNode();
      const DataNode old_character =
          get_property(Symbol(indexed_key("char_object", player)));
      const Symbol old_outfit = node_symbol_value(old_character);
      const Symbol new_outfit = node_symbol_value(outfit);
      const bool transfer_pending =
          old_outfit.valid() && new_outfit.valid() && old_outfit != new_outfit;
      set_property(Symbol(indexed_key("char_transfer_pending", player)),
                   DataNode::Int(transfer_pending ? 1 : 0));
      set_property(Symbol(indexed_key("char_transfer_source", player)),
                   transfer_pending ? old_character : DataNode());
      set_property(Symbol(indexed_key("char_outfit", player)), outfit);
      set_property(Symbol(indexed_key("char_loaded", player)), DataNode::Int(0));
      set_property(Symbol("char_outfit"), outfit);
      // Stock scripts branch on show_char's return before transferring the old
      // main.drv and sending the animate/select/store event. A valid requested
      // outfit means the asynchronous character swap was accepted.
      out = DataNode::Int(new_outfit.valid() ? 1 : 0);
      return true;
    }
    if (std::strcmp(m, "get_char") == 0) {
      const int player = arg0_int(args);
      out = get_property(Symbol(indexed_key("char_object", player)));
      return true;
    }
    if (std::strcmp(m, "is_char_loaded") == 0) {
      const int player = arg0_int(args);
      out = DataNode::Int(node_bool(get_property(
                              Symbol(indexed_key("char_loaded", player))))
                              ? 1
                              : 0);
      return true;
    }
    if (std::strcmp(m, "are_chars_loaded") == 0) {
      const int slots = std::max(
          1, get_property(Symbol("num_placers")).as_int().value_or(1));
      bool loaded = true;
      for (int player = 0; player < slots; ++player) {
        loaded = loaded && node_bool(get_property(
                               Symbol(indexed_key("char_loaded", player))));
      }
      out = DataNode::Int(loaded ? 1 : 0);
      return true;
    }
    if (std::strcmp(m, "set_placer") == 0 ||
        std::strcmp(m, "set_env") == 0) {
      const int player = arg0_int(args);
      const char* stem = m[4] == 'p' ? "char_placer" : "char_env";
      set_property(Symbol(indexed_key(stem, player)),
                   args.size() > 1 ? args.at(1) : DataNode());
      return true;
    }
    if (std::strcmp(m, "char_event") == 0) {
      const int player = arg0_int(args);
      set_property(Symbol(indexed_key("char_event", player)),
                   args.size() > 1 ? args.at(1) : DataNode());
      return true;
    }
    if (std::strcmp(m, "set_paused") == 0) {
      set_property(Symbol("char_paused"),
                   DataNode::Int(arg0_bool(args) ? 1 : 0));
      return true;
    }
    if (std::strcmp(m, "set_door") == 0) {
      const int player = arg0_int(args);
      const DataNode door = args.size() > 1 ? args.at(1) : DataNode();
      set_property(Symbol(indexed_key("char_door", player)), door);
      set_property(Symbol("char_door"), door);
      return true;
    }
  }

  if (cls_ == Symbol("GHScreen") &&
      (std::strcmp(m, "reset_ambient") == 0 ||
       std::strcmp(m, "turn_off_ambient") == 0)) {
    // Loading/memory-card screens call this native GHScreen boundary rather
    // than addressing MetaMusic directly. Stop only the ambient controller;
    // one-shot UI cues remain owned by Synth and keep their scripted lifetime.
    if (mgr_)
      mgr_->emit_audio_event(Symbol("meta_music"), Symbol("stop"), false);
    set_property(Symbol("ambient"), DataNode::Int(0));
    return true;
  }

  // Harmonix MultiSelectScreen owns controller-to-player routing while each
  // MultiSelectPanel owns one player's focus/ready state.  multiplayer.dtb
  // supplies the panel_base_name, sel_buttons, player_num, ready_label and
  // all_ready callbacks; these are native class responsibilities in retail,
  // not scripted handlers.
  if (cls_ == Symbol("MultiSelectPanel")) {
    auto player_config = [&]() -> Object* {
      if (!mgr_) return nullptr;
      Object* game = mgr_->resolve_object(Symbol("game"));
      if (!game) return nullptr;
      DataArray player;
      player.push(DataNode::Int(
          get_property(Symbol("player_num")).as_int().value_or(0)));
      return game->handle_property(Symbol("get_player_config"), player)
          .as_object();
    };
    auto outfit_count = [&](Object* config) {
      if (!mgr_ || !config || !mgr_->current_screen()) return 0;
      DataArray args;
      args.push(config->get_property(Symbol("character")));
      return std::max(
          0, mgr_->current_screen()
                 ->handle_property(Symbol("num_outfits"), args)
                 .as_int()
                 .value_or(0));
    };
    auto outfit_at = [&](Object* config, int index) {
      if (!mgr_ || !config || !mgr_->current_screen()) return Symbol();
      DataArray args;
      args.push(config->get_property(Symbol("character")));
      args.push(DataNode::Int(std::max(0, index)));
      return node_symbol_value(mgr_->current_screen()->handle_property(
          Symbol("get_outfit"), args));
    };
    auto outfit_label_at = [&](Object* config, int index) {
      if (!mgr_ || !config) return std::string();
      Object* provider =
          mgr_->resolve_object(Symbol("character_provider"));
      if (!provider) return std::string();
      DataArray args;
      args.push(config->get_property(Symbol("character")));
      args.push(DataNode::Int(std::max(0, index)));
      return node_text(provider->handle_property(
          Symbol("get_outfit_label"), args));
    };
    auto refresh_outfit_window = [&](Object* config, int selected) {
      const int count = outfit_count(config);
      if (count <= 0) return;
      const int start = count > 2 ? selected : 0;
      set_property(Symbol("outfit_window_start"), DataNode::Int(start));
      for (int row = 0; row < 2; ++row) {
        Object* button = find_path(row == 0 ? "outfit1.btn"
                                            : "outfit2.btn");
        if (!button) continue;
        const int index = count > 2 ? (start + row) % count : row;
        if (index >= count) continue;
        const std::string label = outfit_label_at(config, index);
        if (!label.empty())
          button->set_property(Symbol("text"), DataNode::Str(label));
      }
    };
    auto set_ready_visible = [&](bool visible) {
      Symbol ready_name = node_symbol_value(get_property(Symbol("ready_label")));
      if (!ready_name.valid()) return;
      if (Object* ready = find_path(ready_name.c_str()))
        ready->set_property(Symbol("showing"),
                            DataNode::Sym(Symbol(visible ? "TRUE" : "FALSE")));
    };
    auto buttons = [&]() { return symbol_array(get_property(Symbol("sel_buttons"))); };
    auto set_selection = [&](Symbol selection) {
      const std::vector<Symbol> authored = buttons();
      if (authored.empty()) return;
      Symbol button = selection_button(selection);
      auto it = std::find(authored.begin(), authored.end(), button);
      if (it == authored.end()) return;
      set_property(Symbol("focus_button_name"),
                   DataNode::Sym(button_selection(*it)));
      set_property(Symbol("focus"), DataNode::Sym(*it));
      sync_panel_focus_state(*this);
      if (mgr_) {
        mgr_->set_global(Symbol("new_focus"),
                         DataNode::Obj(find_path(it->c_str())));
        mgr_->set_global(Symbol("panel_dir"), DataNode::Obj(this));
      }
      if (has_handler(Symbol("FOCUS_MSG")))
        handle_property(Symbol("FOCUS_MSG"), DataArray());
    };
    if (std::strcmp(m, "enter") == 0) {
      set_property(Symbol("ready"), DataNode::Int(0));
      if (!has_property(Symbol("active")))
        set_property(Symbol("active"), DataNode::Int(1));
      set_ready_visible(false);
      Symbol selection =
          node_symbol_value(get_property(Symbol("focus_button_name")));
      if (!selection.valid()) {
        const auto authored = buttons();
        if (!authored.empty()) selection = button_selection(authored.front());
      }
      if (selection.valid()) set_selection(selection);
      return true;
    }
    if (std::strcmp(m, "set_active") == 0) {
      const bool active = arg0_bool(args);
      set_property(Symbol("active"), DataNode::Int(active ? 1 : 0));
      if (!active) {
        set_property(Symbol("ready"), DataNode::Int(0));
        set_ready_visible(false);
      }
      return true;
    }
    if (std::strcmp(m, "is_ready") == 0) {
      out = DataNode::Int(node_bool(get_property(Symbol("ready"))) ? 1 : 0);
      return true;
    }
    if (std::strcmp(m, "force_select") == 0) {
      set_selection(arg_symbol(args, 0));
      set_property(Symbol("ready"), DataNode::Int(1));
      set_ready_visible(true);
      return true;
    }
    if (std::strcmp(m, "multi_button_down") == 0) {
      if (has_property(Symbol("active")) &&
          !node_bool(get_property(Symbol("active"))))
        return true;
      const Symbol button = arg_symbol(args, 0);
      const std::vector<Symbol> authored = buttons();
      if (authored.empty()) return true;
      const bool ready = node_bool(get_property(Symbol("ready")));
      if (button == Symbol("kPad_X")) {
        if (!ready) {
          Symbol selection =
              node_symbol_value(get_property(Symbol("focus_button_name")));
          if (!selection.valid()) selection = button_selection(authored.front());
          set_selection(selection);
          set_property(Symbol("ready"), DataNode::Int(1));
          set_ready_visible(true);
        }
        return true;
      }
      if (button == Symbol("kPad_Tri")) {
        if (ready) {
          set_property(Symbol("ready"), DataNode::Int(0));
          set_ready_visible(false);
        }
        return true;
      }
      if (ready) return true;
      const int delta =
          (button == Symbol("kPad_DU") || button == Symbol("kPad_LU"))
              ? -1
              : (button == Symbol("kPad_DD") || button == Symbol("kPad_LD"))
                    ? 1
                    : 0;
      if (delta == 0) return true;
      if ((name() == Symbol("multi_char_outfit0") ||
           name() == Symbol("multi_char_outfit1"))) {
        Object* config = player_config();
        const int count = outfit_count(config);
        if (config && count > 2) {
          int selected =
              config->get_property(Symbol("outfit_index"))
                  .as_int()
                  .value_or(0);
          selected =
              (selected + delta + count) % count;
          DataArray select_index;
          select_index.push(DataNode::Int(selected));
          const Symbol outfit = node_symbol_value(
              config->handle_property(Symbol("set_outfit_index"),
                                      select_index));
          set_property(Symbol("focus_button_name"),
                       DataNode::Sym(Symbol("outfit1")));
          set_property(Symbol("focus"),
                       DataNode::Sym(Symbol("outfit1.btn")));
          sync_panel_focus_state(*this);
          refresh_outfit_window(config, selected);
          if (Object* chars = mgr_->resolve_object(Symbol("char_multi"))) {
            const int player =
                get_property(Symbol("player_num")).as_int().value_or(0);
            DataArray show;
            show.push(DataNode::Int(player));
            show.push(DataNode::Sym(outfit));
            chars->handle_property(Symbol("show_char"), show);
            DataArray event;
            event.push(DataNode::Int(player));
            event.push(DataNode::Sym(Symbol("select")));
            chars->handle_property(Symbol("char_event"), event);
          }
          return true;
        }
      }
      Symbol current =
          selection_button(node_symbol_value(get_property(Symbol("focus_button_name"))));
      auto it = std::find(authored.begin(), authored.end(), current);
      int index = it == authored.end() ? 0 : static_cast<int>(it - authored.begin());
      index = (index + delta + static_cast<int>(authored.size())) %
              static_cast<int>(authored.size());
      set_selection(button_selection(authored[static_cast<std::size_t>(index)]));
      return true;
    }
  }

  if (cls_ == Symbol("MultiCharSelPanel")) {
    auto navigator = [&]() {
      return symbol_array(get_property(Symbol("char_navigator")));
    };
    auto player_config = [&](int player) -> Object* {
      if (!mgr_) return nullptr;
      Object* game = mgr_->resolve_object(Symbol("game"));
      if (!game) return nullptr;
      DataArray player_arg;
      player_arg.push(DataNode::Int(player));
      return game->handle_property(Symbol("get_player_config"), player_arg)
          .as_object();
    };
    auto selected_index = [&](int player) {
      const std::vector<Symbol> chars = navigator();
      if (chars.empty()) return 0;
      int value = get_property(Symbol(indexed_key("char_index", player)))
                      .as_int()
                      .value_or(player == 0 ? 0 : 1);
      return std::clamp(value, 0, static_cast<int>(chars.size()) - 1);
    };
    auto apply_character = [&](int player, int index) {
      const std::vector<Symbol> chars = navigator();
      if (chars.empty()) return;
      index = std::clamp(index, 0, static_cast<int>(chars.size()) - 1);
      set_property(Symbol(indexed_key("char_index", player)),
                   DataNode::Int(index));
      if (Object* config = player_config(player)) {
        const Symbol character = chars[static_cast<std::size_t>(index)];
        Symbol outfit = character;
        if (mgr_ && mgr_->current_screen()) {
          DataArray outfit_args;
          outfit_args.push(DataNode::Sym(character));
          outfit_args.push(DataNode::Int(0));
          outfit = node_symbol_value(mgr_->current_screen()->handle_property(
              Symbol("get_outfit"), outfit_args));
          if (!outfit.valid()) outfit = character;
        }
        config->set_property(Symbol("character"), DataNode::Sym(character));
        config->set_property(Symbol("character_outfit"),
                             DataNode::Sym(outfit));
        config->set_property(Symbol("outfit_index"), DataNode::Int(0));
      }
    };
    if (std::strcmp(m, "enter") == 0) {
      const std::vector<Symbol> chars = navigator();
      for (int player = 0; player < 2; ++player) {
        int index = selected_index(player);
        if (Object* config = player_config(player)) {
          const Symbol current = node_symbol_value(
              config->get_property(Symbol("character")));
          auto it = std::find(chars.begin(), chars.end(), current);
          if (it != chars.end()) index = static_cast<int>(it - chars.begin());
        }
        apply_character(player, index);
      }
      return true;
    }
    if (std::strcmp(m, "get_char_idx") == 0) {
      out = DataNode::Int(selected_index(arg0_int(args)));
      return true;
    }
    if (std::strcmp(m, "BUTTON_DOWN_MSG") == 0) {
      if (!mgr_) return true;
      const int player = std::clamp(
          mgr_->get_global(Symbol("player_num")).as_int().value_or(0), 0, 1);
      const Symbol button =
          node_symbol_value(mgr_->get_global(Symbol("button")));
      if (button == Symbol("kPad_X")) {
        if (Object* screen = mgr_->current_screen()) {
          DataArray selected;
          selected.push(DataNode::Int(player));
          screen->handle_property(Symbol("multi_char_selected"), selected);
        }
        return true;
      }
      const int delta =
          (button == Symbol("kPad_DU") || button == Symbol("kPad_DL") ||
           button == Symbol("kPad_LU") || button == Symbol("kPad_LL"))
              ? -1
              : (button == Symbol("kPad_DD") || button == Symbol("kPad_DR") ||
                 button == Symbol("kPad_LD") || button == Symbol("kPad_LR"))
                    ? 1
                    : 0;
      const std::vector<Symbol> chars = navigator();
      if (delta != 0 && !chars.empty()) {
        int index = (selected_index(player) + delta +
                     static_cast<int>(chars.size())) %
                    static_cast<int>(chars.size());
        apply_character(player, index);
        if (Object* screen = mgr_->current_screen()) {
          DataArray scrolled;
          scrolled.push(DataNode::Int(player));
          screen->handle_property(Symbol("multi_char_scroll"), scrolled);
        }
      }
      return true;
    }
  }

  if (cls_ == Symbol("MultiSelectScreen") &&
      std::strcmp(m, "BUTTON_DOWN_MSG") == 0) {
    if (!mgr_) return true;
    const int player = std::clamp(
        mgr_->get_global(Symbol("player_num")).as_int().value_or(0), 0, 1);
    const Symbol button =
        node_symbol_value(mgr_->get_global(Symbol("button")));
    const Symbol base =
        node_symbol_value(get_property(Symbol("panel_base_name")));
    Object* player_panel = nullptr;
    if (base.valid()) {
      const std::string panel_name = std::string(base.c_str()) +
                                     std::to_string(player);
      player_panel = mgr_->find_object(Symbol(panel_name));
    }
    const bool panel_active =
        player_panel &&
        (!player_panel->has_property(Symbol("active")) ||
         node_bool(player_panel->get_property(Symbol("active"))));
    if (panel_active) {
      DataArray routed;
      routed.push(DataNode::Sym(button));
      player_panel->handle_property(Symbol("multi_button_down"), routed);
      if (button == Symbol("kPad_X") || button == Symbol("kPad_DU") ||
          button == Symbol("kPad_DD") || button == Symbol("kPad_LU") ||
          button == Symbol("kPad_LD")) {
        const Symbol selection = node_symbol_value(
            player_panel->get_property(Symbol("focus_button_name")));
        if (selection.valid()) {
          DataArray set_selection;
          set_selection.push(DataNode::Int(player));
          set_selection.push(DataNode::Sym(selection));
          handle_property(Symbol("set_selection"), set_selection);
        }
      }
      if (button == Symbol("kPad_X")) {
        bool all_ready = true;
        for (int i = 0; i < 2; ++i) {
          const std::string panel_name = std::string(base.c_str()) +
                                         std::to_string(i);
          Object* panel = mgr_->find_object(Symbol(panel_name));
          all_ready = all_ready && panel &&
                      node_bool(panel->get_property(Symbol("ready")));
        }
        if (all_ready) handle_property(Symbol("all_ready"), DataArray());
      }
      return true;
    }
    Symbol focus = node_symbol_value(get_property(Symbol("focus")));
    if (Object* focused = focus.valid() ? mgr_->find_object(focus) : nullptr)
      focused->handle_property(Symbol("BUTTON_DOWN_MSG"), DataArray());
    return true;
  }

  if (cls_ == Symbol("MultiSelectScreen")) {
    if (std::strcmp(m, "get_char_idx") == 0) {
      Object* panel = mgr_ ? mgr_->find_object(Symbol("multi_sel_character_panel"))
                           : nullptr;
      out = panel ? panel->handle_property(Symbol("get_char_idx"), args)
                  : DataNode::Int(0);
      return true;
    }
    if (std::strcmp(m, "get_outfit") == 0 ||
        std::strcmp(m, "num_outfits") == 0) {
      Object* provider =
          mgr_ ? mgr_->resolve_object(Symbol("character_provider"))
               : nullptr;
      if (!provider) return true;
      DataArray provider_args;
      provider_args.push(args.size() ? args.at(0) : DataNode());
      if (std::strcmp(m, "get_outfit") == 0)
        provider_args.push(args.size() > 1 ? args.at(1)
                                          : DataNode::Int(0));
      out = provider->handle_property(msg, provider_args);
      return true;
    }
  }

  if (cls_ == Symbol("EndGamePanel") &&
      std::strcmp(m, "generate_headline") == 0) {
    // endgame.dta supplies a localized headline token plus optional keyed
    // substitutions.  EndGamePanel supplies the venue automatically; the
    // remaining values arrive as (adj ...), (noun ...), etc.  This is the
    // stock script/native boundary used by every newspaper headline.
    const Symbol token = arg_symbol(args, 0);
    std::string headline =
        token.valid() && mgr_ ? mgr_->localize(token) : std::string();

    if (mgr_) {
      const DataNode venue_node =
          game_message(mgr_, Symbol("get_venue"), DataArray());
      Symbol venue = node_symbol_value(venue_node);
      if (venue.valid())
        replace_all(headline, "{GH_VENUE}", mgr_->localize(venue));

      if (Object* band = mgr_->resolve_object(Symbol("band"))) {
        DataNode band_name = band->get_property(Symbol("name"));
        if (band_name.empty())
          band_name = band->get_property(Symbol("band_name"));
        const std::string value = node_text(band_name);
        if (!value.empty()) replace_all(headline, "{GH_BAND}", value);
      }
    }

    for (std::size_t i = 1; i < args.size(); ++i) {
      auto keyed = args.at(i).as_array();
      if (!keyed || keyed->size() < 2) continue;
      Symbol key = keyed->at(0).as_symbol().value_or(Symbol());
      if (!key.valid()) continue;
      replace_all(headline, headline_placeholder(key),
                  node_text(keyed->at(1)));
    }
    out = DataNode::Str(headline);
    return true;
  }

  // --- self visual/text state ---
  if (std::strcmp(m, "set_showing") == 0) {
    set_property(Symbol("showing"), args.size() ? arg0(args) : kTrue());
    return true;
  }
  if (std::strcmp(m, "get_showing") == 0 ||
      std::strcmp(m, "showing") == 0) {
    out = get_property(Symbol("showing"));
    return true;
  }
  if (std::strcmp(m, "set_paused") == 0) {
    set_property(Symbol("paused"), DataNode::Int(arg0_bool(args) ? 1 : 0));
    return true;
  }
  if (std::strcmp(m, "paused") == 0) {
    out = DataNode::Int(node_bool(get_property(Symbol("paused"))) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "reset") == 0) {
    if (auto h = handler(Symbol("on_reset"))) {
      if (mgr_) out = mgr_->run_object_handler(h, this, args);
    }
    if (slider_class(cls_)) slider_reset_selection(*this);
    set_property(Symbol("reset_count"),
                 DataNode::Int(get_property(Symbol("reset_count"))
                                       .as_int()
                                       .value_or(0) +
                               1));
    return true;
  }
  if (cls_ == Symbol("TutorialPanel")) {
    if (std::strcmp(m, "get_next_state") == 0) {
      out = DataNode::Sym(tutorial_next_state(*this, arg0_int(args)));
      return true;
    }
    if (std::strcmp(m, "set_state") == 0) {
      Symbol state = arg_symbol(args, 0);
      if (!state.valid()) state = node_symbol_value(arg0(args));
      if (tutorial_state_index(state) >= 0) {
        set_property(Symbol("tutorial_state"), DataNode::Sym(state));
        set_property(Symbol("lesson"), DataNode::Sym(state));
        set_property(Symbol("state"), DataNode::Sym(Symbol("start_lesson")));
      } else {
        set_property(Symbol("state"), arg0(args));
      }
      set_property(Symbol("disabled"), kFalse());
      return true;
    }
    if (std::strcmp(m, "play_vo") == 0) {
      set_property(Symbol("last_vo"), arg0(args));
      set_property(Symbol("vo_play_count"),
                   DataNode::Int(get_property(Symbol("vo_play_count"))
                                         .as_int()
                                         .value_or(0) +
                                 1));
      set_property(Symbol("vo_playing"), DataNode::Int(0));
      return true;
    }
    if (std::strcmp(m, "reset_vo") == 0) {
      set_property(Symbol("last_vo"), DataNode());
      set_property(Symbol("vo_playing"), DataNode::Int(0));
      set_property(Symbol("vo_reset_count"),
                   DataNode::Int(get_property(Symbol("vo_reset_count"))
                                         .as_int()
                                         .value_or(0) +
                                 1));
      return true;
    }
    if (std::strcmp(m, "is_vo_playing") == 0) {
      out = DataNode::Int(node_bool(get_property(Symbol("vo_playing"))) ? 1 : 0);
      return true;
    }
    if (std::strcmp(m, "is_missing_guitar") == 0) {
      out = game_message(mgr_, Symbol("is_missing_controller"), DataArray());
      if (out.empty()) out = kFalse();
      return true;
    }
  }
  if (std::strcmp(m, "set_state") == 0) {
    const DataNode state = arg0(args);
    set_property(Symbol("state"), state);
    set_property(Symbol("disabled"),
                 state_is_disabled(state) ? kTrue() : kFalse());
    return true;
  }
  if (std::strcmp(m, "get_state") == 0) {
    out = DataNode::Int(state_code(get_property(Symbol("state"))));
    return true;
  }
  if (std::strcmp(m, "can_have_focus") == 0) {
    out = DataNode::Int(component_can_have_focus(*this) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "send_select") == 0 ||
      std::strcmp(m, "mock_select") == 0) {
    if (text_entry_class(cls_)) {
      initialize_text_entry(*this);
      set_property(Symbol("done"), kTrue());
      refresh_text_entry_display(*this);
      finish_component_select(*this);
      if (mgr_ && mgr_->current_screen())
        mgr_->current_screen()->handle_property(Symbol("TEXT_ENTRY_MSG"),
                                                DataArray());
      return true;
    }
    if (slider_class(cls_)) {
      if (slider_scroll_selected(*this)) {
        slider_reset_selection(*this);
        if (!component_disabled(this)) set_component_state(this, Symbol("focused"));
      } else {
        slider_store_selection(*this);
        if (!component_disabled(this)) set_component_state(this, Symbol("selected"));
      }
      return true;
    }
    start_component_select(*this);
    return true;
  }
  if (std::strcmp(m, "finish_selecting") == 0) {
    finish_component_select(*this);
    return true;
  }
  if (std::strcmp(m, "set_localized_text") == 0 ||
      std::strcmp(m, "set_localized") == 0) {
    set_property(Symbol("text"),
                 DataNode::Str(localized_node_text(*this, arg0(args))));
    return true;
  }
  if (std::strcmp(m, "set_text") == 0 ||
      std::strcmp(m, "set_token") == 0) {
    if (text_entry_class(cls_))
      set_text_entry_value(*this, node_text(arg0(args)));
    else
      set_property(Symbol("text"), arg0(args));
    return true;
  }
  if (std::strcmp(m, "get_text") == 0) {
    if (text_entry_class(cls_)) {
      initialize_text_entry(*this);
      out = DataNode::Str(text_entry_value(*this));
    } else {
      out = get_property(Symbol("text"));
    }
    if (out.empty()) out = DataNode::Str("");
    return true;
  }
  if (std::strcmp(m, "set_display") == 0) {
    set_property(Symbol("display"), arg0(args));
    return true;
  }
  if (std::strcmp(m, "display") == 0 ||
      std::strcmp(m, "get_display") == 0) {
    out = get_property(Symbol("display"));
    return true;
  }
  if (std::strcmp(m, "length") == 0) {
    if (text_entry_class(cls_)) initialize_text_entry(*this);
    const std::string value =
        text_entry_class(cls_) ? text_entry_value(*this)
                               : node_text(get_property(Symbol("text")));
    out = DataNode::Int(static_cast<int>(value.size()));
    return true;
  }
  if (std::strcmp(m, "no_text_entered") == 0) {
    if (text_entry_class(cls_)) initialize_text_entry(*this);
    const std::string value =
        text_entry_class(cls_) ? text_entry_value(*this)
                               : node_text(get_property(Symbol("text")));
    out = DataNode::Int(value.empty() ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "user_can_scroll") == 0) {
    if (text_entry_class(cls_)) initialize_text_entry(*this);
    out = !text_entry_class(cls_) ||
                  static_cast<int>(text_entry_value(*this).size()) <
                      text_entry_max_length(*this)
              ? kTrue()
              : kFalse();
    return true;
  }
  if (std::strcmp(m, "resume_input") == 0) {
    set_property(Symbol("done"), kFalse());
    if (text_entry_class(cls_)) initialize_text_entry(*this);
    return true;
  }
  if (std::strcmp(m, "scroll_character") == 0) {
    if (text_entry_class(cls_))
      scroll_text_entry_character(*this, arg0_int(args));
    return true;
  }
  if (std::strcmp(m, "accept_character") == 0) {
    if (text_entry_class(cls_)) accept_text_entry_character(*this);
    return true;
  }
  if (std::strcmp(m, "delete_character") == 0) {
    if (text_entry_class(cls_)) delete_text_entry_character(*this);
    return true;
  }
  if (std::strcmp(m, "is_done") == 0) {
    if (text_entry_class(cls_)) initialize_text_entry(*this);
    out = node_bool(get_property(Symbol("done"))) ? kTrue() : kFalse();
    return true;
  }
  if (std::strcmp(m, "set_text_entry") == 0) {
    set_property(Symbol("text_entry"), args.size() ? args.at(0) : DataNode::Int(-1));
    return true;
  }
  if (std::strcmp(m, "get_text_entry") == 0) {
    out = get_property(Symbol("text_entry"));
    if (out.empty()) out = DataNode::Int(-1);
    return true;
  }
  if (std::strcmp(m, "set") == 0 && args.size() >= 2) {
    if (auto key = args.at(0).as_symbol()) {
      if (*key == Symbol("text_token")) {
        set_property(*key, args.at(1));
        set_property(Symbol("text"), args.at(1));
        return true;
      }
      if (*key == Symbol("text")) {
        set_property(Symbol("text"), args.at(1));
        return true;
      }
    }
  }
  if (std::strcmp(m, "set_provider") == 0) {
    set_property(Symbol("provider"), arg0(args));
    if (Object* provider = arg0(args).as_object()) {
      const int count = provider->handle_property(Symbol("num_data"), DataArray())
                            .as_int()
                            .value_or(0);
      set_property(Symbol("provider_num_data"), DataNode::Int(std::max(0, count)));
    }
    return true;
  }
  if (std::strcmp(m, "set_selected") == 0) {
    set_selected(*this, arg0_int(args));
    return true;
  }
  if (std::strcmp(m, "set_selected_simulate_scroll") == 0) {
    const int pos = arg0_int(args);
    const float now = mgr_ ? mgr_->ui_seconds() : 0.0f;
    start_list_scroll_to(*this, pos, list_first_for_selected(*this, pos), now,
                         list_speed(*this));
    return true;
  }
  if (std::strcmp(m, "focus_name") == 0) {
    Symbol focus = node_symbol_value(get_property(Symbol("focus")));
    if (focus.valid()) {
      out = DataNode::Sym(focus);
    } else {
      out = DataNode::Str("");
    }
    return true;
  }
  if (std::strcmp(m, "get_focusable_components") == 0) {
    auto components = std::make_shared<DataArray>();
    for (std::size_t i = 0; i < size(); ++i) {
      Object* child = at(i);
      if (child && component_can_have_focus(*child))
        components->push(DataNode::Obj(child));
    }
    out = DataNode::Array(components);
    return true;
  }
  if (std::strcmp(m, "set_show_focus_component") == 0) {
    set_property(Symbol("show_focus_component"),
                 DataNode::Int(arg0_bool(args) ? 1 : 0));
    sync_panel_focus_state(*this);
    return true;
  }
  if (std::strcmp(m, "selected_pos") == 0) {
    out = get_property(Symbol("selected_pos"));
    if (!out.as_int()) out = DataNode::Int(0);
    return true;
  }
  if (std::strcmp(m, "first_showing") == 0) {
    update_list_scroll(*this, mgr_ ? mgr_->ui_seconds() : 0.0f);
    out = DataNode::Int(list_first_showing(*this));
    return true;
  }
  if (std::strcmp(m, "selected_display") == 0) {
    update_list_scroll(*this, mgr_ ? mgr_->ui_seconds() : 0.0f);
    out = get_property(Symbol("selected_display"));
    if (!out.as_int())
      out = DataNode::Int(selected_pos(*this) - list_first_showing(*this));
    return true;
  }
  if (std::strcmp(m, "num_lines") == 0) {
    out = get_property(Symbol("num_lines"));
    if (!out.as_int()) out = DataNode::Int(0);
    return true;
  }
  if (std::strcmp(m, "scroll") == 0) {
    list_scroll(*this, arg0_int(args), mgr_);
    return true;
  }
  if (std::strcmp(m, "auto_scroll") == 0) {
    list_auto_scroll(*this, mgr_);
    return true;
  }
  if (std::strcmp(m, "stop_auto_scroll") == 0) {
    set_property(Symbol("auto_scrolling"), DataNode::Int(0));
    return true;
  }
  if (std::strcmp(m, "is_scrolling") == 0) {
    update_list_scroll(*this, mgr_ ? mgr_->ui_seconds() : 0.0f);
    out = DataNode::Int(list_is_scrolling(*this) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "is_scrolling_down") == 0) {
    update_list_scroll(*this, mgr_ ? mgr_->ui_seconds() : 0.0f);
    out = DataNode::Int(
        node_int(get_property(Symbol("current_scroll")), 0) > 0 ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "set_current") == 0) {
    if (slider_class(cls_))
      set_property(Symbol("current"),
                   DataNode::Int(std::clamp(arg0_int(args), 0,
                                            slider_steps(*this) - 1)));
    else
      set_property(Symbol("current"), DataNode::Int(arg0_int(args)));
    return true;
  }
  if (std::strcmp(m, "current") == 0) {
    out = get_property(Symbol("current"));
    if (!out.as_int()) out = DataNode::Int(0);
    return true;
  }
  if (std::strcmp(m, "set_num_steps") == 0) {
    const int steps = slider_class(cls_) ? std::max(1, arg0_int(args))
                                         : arg0_int(args);
    set_property(Symbol("num_steps"), DataNode::Int(steps));
    if (slider_class(cls_))
      set_property(Symbol("current"), DataNode::Int(slider_current(*this)));
    return true;
  }
  if (std::strcmp(m, "num_steps") == 0) {
    out = get_property(Symbol("num_steps"));
    if (!out.as_int()) out = DataNode::Int(1);
    return true;
  }
  if (std::strcmp(m, "set_check") == 0) {
    set_property(Symbol("checked"), DataNode::Int(arg0_bool(args) ? 1 : 0));
    return true;
  }
  if (std::strcmp(m, "get_check") == 0) {
    out = DataNode::Int(node_bool(get_property(Symbol("checked"))) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "toggle") == 0) {
    const bool checked = node_bool(get_property(Symbol("checked")));
    out = DataNode::Int(checked ? 0 : 1);
    set_property(Symbol("checked"), out);
    return true;
  }
  if (std::strcmp(m, "set_frame") == 0) {
    if (slider_class(cls_)) {
      set_slider_frame(*this, arg0_float(args));
      return true;
    }
    const float frame = args.size() ? arg0_float(args) : 0.0f;
    if (mgr_)
      mgr_->set_animation_frame(this, frame);
    else
      set_property(Symbol("frame"), DataNode::Float(frame));
    return true;
  }
  if (std::strcmp(m, "frame") == 0) {
    if (slider_class(cls_)) {
      out = DataNode::Float(slider_frame(*this));
      return true;
    }
    out = get_property(Symbol("frame"));
    if (out.empty()) out = DataNode::Float(0.0f);
    return true;
  }
  if (std::strcmp(m, "end_frame") == 0) {
    out = get_property(Symbol("end_frame"));
    if (out.empty()) out = DataNode::Float(0.0f);
    return true;
  }
  if (std::strcmp(m, "animate") == 0) {
    set_property(Symbol("animating"), DataNode::Int(1));
    set_property(Symbol("animate_args"), DataNode::Array(copy_args(args, 0)));
    for (std::size_t i = 0; i < args.size(); ++i) {
      if (auto keyed = args.at(i).as_array()) store_keyed_anim_arg(*this, *keyed);
    }
    if (mgr_) mgr_->start_animation_task(this, args);
    return true;
  }
  if (std::strcmp(m, "stop_animation") == 0) {
    if (mgr_) mgr_->stop_animation_task(this);
    return true;
  }
  if (std::strcmp(m, "is_animating") == 0) {
    out = DataNode::Int(mgr_ && mgr_->is_animation_task_active(this) ? 1 : 0);
    return true;
  }
  if (std::strcmp(m, "set_local_pos") == 0) {
    store_vec3(*this, Symbol("local_pos"), args);
    return true;
  }
  if (std::strcmp(m, "set_local_scale") == 0) {
    store_vec3(*this, Symbol("local_scale"), args);
    return true;
  }
  if (std::strcmp(m, "set_mat") == 0) {
    set_property(Symbol("mat"), arg0(args));
    return true;
  }
  if (std::strcmp(m, "set_tex") == 0) {
    set_property(Symbol("tex"), arg0(args));
    return true;
  }
  if (std::strcmp(m, "set_wrap_width") == 0) {
    set_property(Symbol("wrap_width"), DataNode::Float(arg0_float(args)));
    return true;
  }
  if (std::strcmp(m, "find") == 0) {
    const std::string child = arg0_name(args);
    out = child.empty() ? DataNode() : DataNode::Obj(find_path(child));
    return true;
  }
  if (std::strcmp(m, "loaded_dir") == 0) {
    out = DataNode::Obj(this);
    return true;
  }
  if (std::strcmp(m, "backwards_anim") == 0) {
    set_property(Symbol("last_transition_anim"), DataNode::Sym(Symbol("back")));
    if (mgr_) mgr_->request_backwards_anim();
    return true;
  }
  if (std::strcmp(m, "go_back") == 0) {
    if (mgr_) mgr_->go_back();
    return true;
  }

  if (cls_ == Symbol("StorePanel")) {
    if (std::strcmp(m, "price") == 0) {
      const Symbol category =
          arg_symbol(args, 0, get_property(Symbol("category"))
                                  .as_symbol()
                                  .value_or(Symbol()));
      const Symbol item =
          arg_symbol(args, 1, get_property(Symbol("item_name"))
                                  .as_symbol()
                                  .value_or(Symbol()));
      out = DataNode::Int(store_item_price(mgr_, category, item));
      return true;
    }
    if (std::strcmp(m, "low_cost") == 0 ||
        std::strcmp(m, "high_cost") == 0) {
      const Symbol category =
          arg_symbol(args, 0, get_property(Symbol("category"))
                                  .as_symbol()
                                  .value_or(Symbol()));
      out = DataNode::Int(store_category_price_extreme(
          mgr_, category, std::strcmp(m, "high_cost") == 0));
      return true;
    }
  }

  if (slider_class(cls_)) {
    if (std::strcmp(m, "store") == 0) {
      slider_store_selection(*this);
      return true;
    }
    if (std::strcmp(m, "confirm") == 0) {
      slider_reset_selection(*this);
      if (!component_disabled(this)) set_component_state(this, Symbol("focused"));
      return true;
    }
    if (std::strcmp(m, "undo") == 0) {
      slider_undo_selection(*this);
      if (!component_disabled(this)) set_component_state(this, Symbol("focused"));
      return true;
    }
    if (std::strcmp(m, "is_scroll_selected") == 0) {
      out = DataNode::Int(slider_scroll_selected(*this) ? 1 : 0);
      return true;
    }
  }

  if (cls_ == Symbol("SliderPanel") && std::strcmp(m, "slider_selected") == 0) {
    Object* focused =
        child_from_node(*this, get_property(Symbol("focus")));
    out = DataNode::Int(focused && slider_class(focused->class_name()) &&
                                slider_scroll_selected(*focused)
                            ? 1
                            : 0);
    return true;
  }

  if (cls_ == Symbol("CreditsPanel")) {
    Object* list = find_path("credits.lst");
    if (std::strcmp(m, "enter") == 0) {
      set_property(Symbol("cheat_on"), DataNode::Int(0));
      set_property(Symbol("paused"), DataNode::Int(0));
      if (list) {
        list->handle_property(Symbol("set_selected"),
                              one_arg(DataNode::Int(0)));
        list->handle_property(Symbol("auto_scroll"), DataArray());
      }
      return true;
    }
    if (std::strcmp(m, "poll") == 0) {
      if (list) list->handle_property(Symbol("poll"), DataArray());
      return true;
    }
    if (std::strcmp(m, "exit") == 0) {
      if (list) list->handle_property(Symbol("stop_auto_scroll"), DataArray());
      return true;
    }
    if (std::strcmp(m, "pause_panel") == 0) {
      const bool pause = arg0_bool(args);
      set_property(Symbol("paused"), DataNode::Int(pause ? 1 : 0));
      if (list)
        list->handle_property(Symbol(pause ? "stop_auto_scroll" : "auto_scroll"),
                              DataArray());
      return true;
    }
    if (std::strcmp(m, "debug_toggle_autoscroll") == 0) {
      const bool scrolling =
          list && node_bool(list->get_property(Symbol("auto_scrolling")));
      if (list)
        list->handle_property(Symbol(scrolling ? "stop_auto_scroll" : "auto_scroll"),
                              DataArray());
      set_property(Symbol("cheat_on"), DataNode::Int(scrolling ? 1 : 0));
      return true;
    }
    if (std::strcmp(m, "is_cheat_on") == 0) {
      out = DataNode::Int(node_bool(get_property(Symbol("cheat_on"))) ? 1 : 0);
      return true;
    }
  }

  if (cls_ == Symbol("GuitarSelectPanel")) {
    const bool multiplayer =
        node_bool(get_property(Symbol("multiplayer")));
    auto player_key = [&](const char* base, int player) {
      return multiplayer ? Symbol(indexed_key(base, std::clamp(player, 0, 1)))
                         : Symbol(base);
    };
    auto instrument_type = [&](int player) {
      const Symbol mode = game_symbol(mgr_, Symbol("get_mode"));
      return multiplayer && mode == Symbol("multi_coop") && player == 1
                 ? Symbol("bass")
                 : Symbol("guitar");
    };
    auto player_config = [&](int player) -> Object* {
      if (!multiplayer) return mgr_ ? mgr_->resolve_object(Symbol("game")) : nullptr;
      DataArray config_args;
      config_args.push(DataNode::Int(std::clamp(player, 0, 1)));
      return game_message(mgr_, Symbol("get_player_config"), config_args)
          .as_object();
    };
    auto guitar_count = [&](int player) {
      DataArray count_args;
      count_args.push(DataNode::Sym(instrument_type(player)));
      return std::max(
          0, node_int(game_message(mgr_, Symbol("get_num_guitars"),
                                   count_args)));
    };
    auto guitar_at = [&](int player, int index) {
      DataArray at_args;
      at_args.push(DataNode::Sym(instrument_type(player)));
      at_args.push(DataNode::Int(index));
      return game_message(mgr_, Symbol("get_guitar_at"), at_args)
          .as_symbol()
          .value_or(Symbol());
    };
    auto skin_at = [&](Symbol guitar, int index) {
      DataArray at_args;
      at_args.push(DataNode::Sym(guitar));
      at_args.push(DataNode::Int(index));
      return game_message(mgr_, Symbol("get_guitar_skin_at"), at_args)
          .as_symbol()
          .value_or(Symbol());
    };
    auto paint_default = [&](Symbol guitar, Symbol skin, bool secondary) {
      DataArray paint_args;
      paint_args.push(DataNode::Sym(guitar));
      paint_args.push(DataNode::Sym(skin));
      return node_int(
          game_message(
              mgr_,
              Symbol(
                  secondary
                      ? "get_guitar_paint_secondary_for_skin"
                      : "get_guitar_paint_primary_for_skin"),
              paint_args),
          -1);
    };
    auto refresh_paint_labels = [&](int player) {
      const int stage = node_int(
          get_property(player_key("paint_select", player)), 0);
      if (stage <= 0 || !mgr_) return;
      const int primary = node_int(
          get_property(player_key("paint_primary", player)), 0);
      if (Object* name = mgr_->resolve_object(Symbol("sg_skin_nm.lbl"))) {
        const std::string text =
            ghogx::asset::rb2_paint_color_name(primary);
        name->set_property(Symbol("text"), DataNode::Str(text));
      }
      if (Object* description =
              mgr_->resolve_object(Symbol("sg_skin_desc.lbl"))) {
        description->set_property(
            Symbol("text"),
            DataNode::Str("UP/DOWN: CHANGE COLOR\nCONTINUE: SAVE"));
      }
    };
    auto ensure_selection = [&](int player) {
      player = std::clamp(player, 0, 1);
      const Symbol guitar_key = player_key("selected_guitar", player);
      const Symbol skin_key = player_key("selected_skin", player);
      Symbol selected =
          get_property(guitar_key).as_symbol().value_or(Symbol());
      Object* config = player_config(player);
      Symbol configured =
          config ? config->handle_property(Symbol("get_guitar"), DataArray())
                       .as_symbol()
                       .value_or(Symbol())
                 : Symbol();
      const int count = guitar_count(player);
      int selected_index = 0;
      const Symbol index_target =
          selected.valid() ? selected : configured;
      for (int i = 0; i < count; ++i) {
        if (guitar_at(player, i) != index_target) continue;
        selected_index = i;
        break;
      }
      if (!selected.valid())
        selected = count > 0 ? guitar_at(player, selected_index) : configured;
      if (selected.valid()) set_property(guitar_key, DataNode::Sym(selected));
      set_property(player_key("selected_guitar_index", player),
                   DataNode::Int(selected_index));

      Symbol selected_skin =
          get_property(skin_key).as_symbol().value_or(Symbol());
      if (!selected_skin.valid() && config)
        selected_skin =
            config->handle_property(Symbol("get_guitar_skin"), DataArray())
                .as_symbol()
                .value_or(Symbol());
      if (!selected_skin.valid()) selected_skin = skin_at(selected, 0);
      if (selected_skin.valid())
        set_property(skin_key, DataNode::Sym(selected_skin));
    };
    if (std::strcmp(m, "enter") == 0) {
      ensure_selection(0);
      if (multiplayer) ensure_selection(1);
      set_property(player_key("paint_select", 0), DataNode::Int(0));
      if (multiplayer)
        set_property(player_key("paint_select", 1), DataNode::Int(0));
      set_property(player_key("select_done", 0), DataNode::Int(0));
      if (multiplayer)
        set_property(player_key("select_done", 1), DataNode::Int(0));
      return true;
    }
    if (std::strcmp(m, "BUTTON_DOWN_MSG") == 0) {
      const Symbol button =
          mgr_ ? node_symbol_value(mgr_->get_global(Symbol("button")))
               : Symbol();
      const int player =
          mgr_ ? std::clamp(node_int(mgr_->get_global(Symbol("player_num"))),
                            0, 1)
               : 0;
      if (button == Symbol("kPad_X") && mgr_ && mgr_->current_screen()) {
        const Symbol guitar =
            get_property(player_key("selected_guitar", player))
                .as_symbol()
                .value_or(Symbol());
        const Symbol skin =
            get_property(player_key("selected_skin", player))
                .as_symbol()
                .value_or(Symbol());
        const bool selecting_skin =
            node_bool(get_property(player_key("skin_select", player)));
        int paint_stage = node_int(
            get_property(player_key("paint_select", player)), 0);
        const int default_primary = paint_default(guitar, skin, false);
        if (paint_stage == 0 && selecting_skin && default_primary >= 0) {
          Object* config = player_config(player);
          int primary =
              config
                  ? node_int(config->handle_property(
                                 Symbol("get_guitar_paint_primary"),
                                 DataArray()),
                             -1)
                  : -1;
          if (primary < 0) primary = default_primary;
          set_property(player_key("paint_primary", player),
                       DataNode::Int(primary));
          set_property(player_key("paint_secondary", player),
                       DataNode::Int(primary));
          set_property(player_key("paint_select", player),
                       DataNode::Int(1));
          DataArray refresh;
          refresh.push(DataNode::Int(1));
          handle_property(Symbol("update_display"), refresh);
          refresh_paint_labels(player);
          return true;
        }
        if (paint_stage == 1) {
          if (Object* config = player_config(player)) {
            DataArray colors;
            colors.push(get_property(player_key("paint_primary", player)));
            colors.push(get_property(player_key("paint_primary", player)));
            config->handle_property(Symbol("set_guitar_paint"), colors);
          }
          set_property(player_key("paint_select", player),
                       DataNode::Int(0));
        }
        DataArray selected;
        selected.push(DataNode::Int(player));
        mgr_->current_screen()->handle_property(Symbol("guitar_selected"),
                                                selected);
        return true;
      }
      if (button == Symbol("kPad_DDown") || button == Symbol("kPad_DUp")) {
        ensure_selection(player);
        const int direction = button == Symbol("kPad_DDown") ? 1 : -1;
        const int paint_stage = node_int(
            get_property(player_key("paint_select", player)), 0);
        if (paint_stage > 0) {
          const Symbol color_key = player_key("paint_primary", player);
          const int count = ghogx::asset::rb2_paint_color_count();
          int color = node_int(get_property(color_key), 0);
          color = (color + direction + count) % count;
          set_property(color_key, DataNode::Int(color));
          set_property(player_key("paint_secondary", player),
                       DataNode::Int(color));
          DataArray refresh;
          refresh.push(DataNode::Int(1));
          handle_property(Symbol("update_display"), refresh);
          refresh_paint_labels(player);
          return true;
        }
        const Symbol guitar_key = player_key("selected_guitar", player);
        const Symbol skin_key = player_key("selected_skin", player);
        const bool selecting_skin =
            node_bool(get_property(player_key("skin_select", player)));
        if (selecting_skin) {
          const Symbol guitar =
              get_property(guitar_key).as_symbol().value_or(Symbol());
          DataArray count_args;
          count_args.push(DataNode::Sym(guitar));
          const int count = std::max(
              0, node_int(game_message(mgr_, Symbol("get_num_skins"),
                                       count_args)));
          if (count > 0) {
            const Symbol current =
                get_property(skin_key).as_symbol().value_or(Symbol());
            int index = 0;
            for (int i = 0; i < count; ++i) {
              if (skin_at(guitar, i) == current) {
                index = i;
                break;
              }
            }
            index = (index + direction + count) % count;
            const Symbol skin = skin_at(guitar, index);
            if (skin.valid()) set_property(skin_key, DataNode::Sym(skin));
          }
        } else {
          const int count = guitar_count(player);
          if (count > 0) {
            const Symbol index_key =
                player_key("selected_guitar_index", player);
            int index =
                node_int(get_property(index_key), 0);
            index = (index + direction + count) % count;
            const Symbol guitar = guitar_at(player, index);
            set_property(index_key, DataNode::Int(index));
            if (guitar.valid()) {
              set_property(guitar_key, DataNode::Sym(guitar));
              const Symbol skin = skin_at(guitar, 0);
              if (skin.valid()) set_property(skin_key, DataNode::Sym(skin));
            }
          }
        }
        DataArray refresh;
        refresh.push(DataNode::Int(1));
        handle_property(Symbol("update_display"), refresh);
        return true;
      }
    }
    if (std::strcmp(m, "set_skin_select") == 0) {
      const int player = arg0_int(args);
      const bool selecting_skin = args.size() > 1 && node_bool(args.at(1));
      set_property(player_key("skin_select", player),
                   DataNode::Int(selecting_skin ? 1 : 0));
      return true;
    }
    if (std::strcmp(m, "is_skin_select") == 0) {
      const int player = arg0_int(args);
      out = DataNode::Int(node_bool(get_property(
                              player_key("skin_select", player)))
                              ? 1
                              : 0);
      return true;
    }
    if (std::strcmp(m, "get_selected_guitar") == 0) {
      const int player = std::clamp(arg0_int(args), 0, 1);
      ensure_selection(player);
      out = get_property(player_key("selected_guitar", player));
      return true;
    }
    if (std::strcmp(m, "get_paint_select") == 0) {
      const int player = std::clamp(arg0_int(args), 0, 1);
      out = get_property(player_key("paint_select", player));
      return true;
    }
    if (std::strcmp(m, "set_paint_select") == 0) {
      const int player = std::clamp(arg0_int(args), 0, 1);
      const int stage =
          args.size() > 1 ? std::clamp(node_int(args.at(1)), 0, 1) : 0;
      set_property(player_key("paint_select", player),
                   DataNode::Int(stage));
      if (stage > 0) refresh_paint_labels(player);
      return true;
    }
    if (std::strcmp(m, "get_selected_skin") == 0) {
      const int player = std::clamp(arg0_int(args), 0, 1);
      ensure_selection(player);
      out = get_property(player_key("selected_skin", player));
      return true;
    }
    if (std::strcmp(m, "get_num_guitars") == 0) {
      out = DataNode::Int(guitar_count(std::clamp(arg0_int(args), 0, 1)));
      return true;
    }
    if (std::strcmp(m, "get_num_skins") == 0) {
      const int player = std::clamp(arg0_int(args), 0, 1);
      Symbol guitar = arg_symbol(args, 1);
      if (!guitar.valid()) {
        ensure_selection(player);
        guitar = get_property(player_key("selected_guitar", player))
                     .as_symbol()
                     .value_or(Symbol());
      }
      DataArray gargs;
      if (guitar.valid()) gargs.push(DataNode::Sym(guitar));
      out = game_message(mgr_, Symbol("get_num_skins"), gargs);
      if (!out.as_int()) out = DataNode::Int(0);
      return true;
    }
    if (std::strcmp(m, "is_select_done") == 0) {
      const int player = std::clamp(arg0_int(args), 0, 1);
      out = DataNode::Int(
          node_bool(get_property(player_key("select_done", player))) ? 1 : 0);
      return true;
    }
    if (std::strcmp(m, "set_select_done") == 0) {
      const int player = multiplayer ? std::clamp(arg0_int(args), 0, 1) : 0;
      const bool done =
          multiplayer ? (args.size() > 1 && node_bool(args.at(1)))
                      : arg0_bool(args);
      set_property(player_key("select_done", player),
                   DataNode::Int(done ? 1 : 0));
      return true;
    }
    if (std::strcmp(m, "get_instrument_type") == 0) {
      out = DataNode::Sym(
          instrument_type(std::clamp(arg0_int(args), 0, 1)));
      return true;
    }
    if (std::strcmp(m, "update_guitar_label") == 0 ||
        std::strcmp(m, "update_player_display") == 0) {
      return true;
    }
  }

  if (cls_ == Symbol("GuitarDisplayPanel")) {
    if (std::strcmp(m, "set_env") == 0) {
      const int player = arg0_int(args);
      if (args.size() > 0)
        set_property(Symbol("guitar_display_player"), args.at(0));
      if (args.size() > 1) {
        set_property(Symbol("guitar_display_env"), args.at(1));
        set_property(Symbol(indexed_key("guitar_display_env", player)),
                     args.at(1));
      }
      return true;
    }
    if (std::strcmp(m, "show_guitar") == 0) {
      const int player = arg0_int(args);
      set_property(Symbol("guitar_skin"), DataNode());
      set_property(Symbol("selected_skin"), DataNode());
      set_property(Symbol("guitar_proxy"), DataNode());
      set_property(Symbol("guitar_filter"), DataNode());
      set_property(Symbol(indexed_key("guitar_skin", player)), DataNode());
      set_property(Symbol(indexed_key("selected_skin", player)), DataNode());
      set_property(Symbol(indexed_key("guitar_proxy", player)), DataNode());
      set_property(Symbol(indexed_key("guitar_filter", player)), DataNode());
      if (args.size() > 0)
        set_property(Symbol("guitar_display_player"), args.at(0));
      if (args.size() > 1) {
        set_property(Symbol("guitar"), args.at(1));
        set_property(Symbol("selected_guitar"), args.at(1));
        set_property(Symbol(indexed_key("guitar", player)), args.at(1));
        set_property(Symbol(indexed_key("selected_guitar", player)), args.at(1));
      }
      const bool arg2_is_proxy =
          args.size() > 2 && args.at(2).as_object() != nullptr;
      if (args.size() > 2 && !arg2_is_proxy) {
        set_property(Symbol("guitar_skin"), args.at(2));
        set_property(Symbol("selected_skin"), args.at(2));
        set_property(Symbol(indexed_key("guitar_skin", player)), args.at(2));
        set_property(Symbol(indexed_key("selected_skin", player)), args.at(2));
      }
      const std::size_t proxy_index = arg2_is_proxy ? 2u : 3u;
      const std::size_t filter_index = arg2_is_proxy ? 3u : 4u;
      if (args.size() > proxy_index) {
        set_property(Symbol("guitar_proxy"), args.at(proxy_index));
        set_property(Symbol(indexed_key("guitar_proxy", player)),
                     args.at(proxy_index));
      }
      if (args.size() > filter_index) {
        set_property(Symbol("guitar_filter"), args.at(filter_index));
        set_property(Symbol(indexed_key("guitar_filter", player)),
                     args.at(filter_index));
      }
      return true;
    }
  }

  // --- focus: a panel stores the focused child's name ---
  if (cls_ == Symbol("GHScreen") && std::strcmp(m, "set_focus") == 0) {
    if (!set_screen_focus_target(*this, arg0(args)))
      set_property(Symbol("focus"), arg0(args));
    return true;
  }
  if (std::strcmp(m, "set_focus") == 0 || std::strcmp(m, "focus") == 0 ||
      std::strcmp(m, "update_focus") == 0) {
    const Symbol old_name =
        node_symbol_value(get_property(Symbol("focus")));
    if (args.size() > 0 && !set_panel_focus_component(*this, arg0(args)))
      set_property(Symbol("focus"), arg0(args));
    else if (args.size() == 0) {
      sync_panel_focus_state(*this);
      return true;
    }
    const Symbol new_name =
        node_symbol_value(get_property(Symbol("focus")));
    if (mgr_ && new_name.valid() && new_name != old_name) {
      Object* old_focus =
          old_name.valid() ? find_path(old_name.c_str()) : nullptr;
      Object* new_focus = find_path(new_name.c_str());
      mgr_->set_global(Symbol("old_focus"),
                       old_focus ? DataNode::Obj(old_focus) : DataNode());
      mgr_->set_global(Symbol("new_focus"),
                       new_focus ? DataNode::Obj(new_focus) : DataNode());
      mgr_->handle_property(Symbol("FOCUS_MSG"), DataArray());
      if (has_handler(Symbol("FOCUS_MSG")))
        handle_property(Symbol("FOCUS_MSG"), DataArray());
    }
    return true;
  }
  if (cls_ == Symbol("GHScreen") && std::strcmp(m, "set_focus_panel") == 0) {
    if (args.size() > 0) {
      if (Object* panel = args.at(0).as_object()) {
        set_property(Symbol("focus"), DataNode::Sym(panel->name()));
        if (auto* dir = dynamic_cast<ObjectDir*>(panel))
          sync_panel_focus_state(*dir);
      } else {
        set_property(Symbol("focus"), arg0(args));
        if (Object* named_panel =
                mgr_ ? mgr_->find_object(node_symbol_value(arg0(args))) : nullptr)
          if (auto* dir = dynamic_cast<ObjectDir*>(named_panel))
            sync_panel_focus_state(*dir);
      }
    }
    return true;
  }

  // --- enable/disable: a named child if it resolves, else self. Derived from
  //     the argument (panel {$this disable a.btn} vs component {b disable}),
  //     not from the class -- so it is correct for both without guessing. ---
  if (std::strcmp(m, "enable") == 0 || std::strcmp(m, "disable") == 0) {
    DataNode v = (m[0] == 'd') ? kTrue() : kFalse();  // disable -> disabled TRUE
    Object* tgt = this;
    if (args.size() > 0) {
      if (Object* obj = args.at(0).as_object()) {
        tgt = obj;
      } else {
        std::string child = arg0_name(args);
        if (!child.empty()) {
          if (Object* c = find_path(child)) tgt = c;
        }
      }
    }
    if (m[0] == 'd') {
      set_component_state(tgt, Symbol("disabled"));
    } else {
      tgt->set_property(Symbol("disabled"), v);
      if (state_is_disabled(tgt->get_property(Symbol("state"))))
        set_component_state(tgt, Symbol("normal"));
    }
    return true;
  }

  // --- transition lifecycle messages we accept as no-ops at this layer (a
  //     screen that needs them defines them as handlers, which win above). ---
  if (std::strcmp(m, "poll") == 0) {
    list_poll(*this, mgr_);
    poll_component_tree(this);
    return true;
  }
  if (std::strcmp(m, "enter") == 0) {
    sync_panel_focus_state(*this);
    return true;
  }
  if (std::strcmp(m, "load") == 0 || std::strcmp(m, "unload") == 0 ||
      std::strcmp(m, "finish_load") == 0 ||
      std::strcmp(m, "change_proxies") == 0 ||
      std::strcmp(m, "exit_complete") == 0) {
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
  // MILO RndGroup is serialized as type "Group" (MiloLib RndGroup.cs). Stock
  // UI scripts address these .view/.grp parents directly for visibility and
  // animation, so they need lightweight runtime objects even though they are
  // not UIComponents.
  define_uiobject("Group", "Object");
  // Other Rnd perObjs that stock UI scripts address directly. Type strings here
  // match the serialized MILO directory entries; MiloLib classes are the source
  // map (RndMesh/RndMat/RndTex/RndTransAnim/RndMeshAnim/RndMatAnim).
  static const char* kMiloScriptObjects[] = {
      "Mesh", "Mat", "Tex", "Trans", "TransAnim", "MeshAnim", "MatAnim",
      "PollAnim", "AnimFilter", "Environ", "EnvAnim", "Light"};
  for (const char* o : kMiloScriptObjects) define_uiobject(o, "Object");

  // The complete {new <Class>} screen/panel roster (STOCK_SURFACE.txt). Super
  // chain is provisional (-> Object) pending step-3 recomp grounding; it does
  // not affect verbatim loading/handler execution.
  static const char* kScreens[] = {"GHScreen", "MultiSelectScreen", "TrackBudgetScreen"};
  static const char* kPanels[] = {
      "GHPanel", "UIPanel", "MultiSelectPanel", "SliderPanel", "GuitarDisplayPanel",
      "EndGamePanel", "CharsysPanel", "GuitarSelectPanel", "TutorialPanel", "TrackPanel",
      "StorePanel", "MultiSelectListPanel", "MultiCharSelPanel", "MetaPanel", "LagPanel",
      "HudPanel", "HelpBarPanel", "GamePanel", "FadePanel", "CreditsPanel",
      "MidiLoaderPanel", "TrackMaskPanel"};
  for (const char* s : kScreens) define_uiobject(s, "Object");
  for (const char* p : kPanels) define_uiobject(p, "Object");

  // MILO-side widget classes (instantiated from panel MILOs; some appear in
  // {new} too). Grouped under UIComponent for is_a() purposes.
  define_uiobject("UIComponent", "Object");
  static const char* kWidgets[] = {
      "UILabel", "UIButton", "UIPicture", "UIList", "UISlider", "CheckBox",
      "CheckboxDisplay", "UIProxy",
      "ScreenMask", "UITrigger", "EventTrigger", "PanelDir", "UIColor",
      "BandLabel", "BandButton", "BandSlider", "BandList", "BandTextEntry",
      "BandCharacter", "BandPlacer", "BandStarDisplay", "BandScoreDisplay",
      "BandStreakDisplay", "BandStarMeterDir", "BandCrowdMeterDir"};
  for (const char* w : kWidgets) define_uiobject(w, "UIComponent");
}

}  // namespace ghogx::ui
