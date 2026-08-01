#include "character/char_clip.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_string(const std::string& got, const std::string& want,
                   const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharClipDisplayGlobals;
  using ghogx::character::SourceCharClipDisplayMsgSource;
  using ghogx::character::SourceCharClipDisplayState;
  using ghogx::character::source_char_clip_display_find_source;
  using ghogx::character::source_char_clip_display_init;
  using ghogx::character::source_char_clip_display_line_spacing;
  using ghogx::character::source_char_clip_display_set_clip;
  using ghogx::character::source_char_clip_display_set_text;
  using ghogx::character::source_char_task_mgr_default_state;
  using ghogx::character::source_char_task_mgr_init;
  using ghogx::character::source_char_task_mgr_toggle_graph;

  bool ok = true;

  SourceCharClipDisplayGlobals globals;
  source_char_clip_display_init(globals, "main.dir", 9.5f);
  ok &= expect_string(globals.dir, "main.dir", "display init dir");
  ok &= near(globals.em, 9.5f, "display init em");
  ok &= near(source_char_clip_display_line_spacing(globals), 19.0f,
             "display line spacing");

  std::vector<SourceCharClipDisplayMsgSource> sources = {
      {"source_a", {"sink_1", "sink_2"}},
      {"source_b", {"target_obj", "sink_3"}},
      {"source_c", {"target_obj"}}};
  auto found = source_char_clip_display_find_source(sources, "target_obj");
  ok &= expect_bool(found.found, true, "find source found");
  ok &= expect_string(found.source, "source_b", "find source first match");
  found = source_char_clip_display_find_source(sources, "missing_obj");
  ok &= expect_bool(found.found, false, "find source missing");
  ok &= expect_string(found.source, "", "find source missing source");

  SourceCharClipDisplayState display;
  source_char_clip_display_set_text(display, globals, "idle.clip", 42.0f);
  ok &= expect_string(display.text, "idle.clip", "display text");
  ok &= near(display.text_width_plus_em, 51.5f, "display width plus em");
  ok &= expect_bool(display.start_end_called, false,
                    "set text does not set start/end");

  source_char_clip_display_set_clip(display, globals, "solo.clip", 12.0f,
                                    24.5f, true, 31.0f);
  ok &= expect_string(display.clip, "solo.clip", "display clip");
  ok &= expect_string(display.text, "solo.clip", "display clip text");
  ok &= near(display.text_width_plus_em, 40.5f,
             "display clip width plus em");
  ok &= near(display.start_beat, 12.0f, "display start beat");
  ok &= near(display.end_beat, 24.5f, "display end beat");
  ok &= expect_bool(display.start_end_called, true,
                    "display start/end called");
  ok &= expect_bool(display.start_end_flag, true, "display start/end flag");

  auto task = source_char_task_mgr_default_state();
  ok &= expect_bool(task.show_graph, false, "task graph default");
  ok &= expect_bool(task.registered_toggle_char_task_graph, false,
                    "task init default");
  source_char_task_mgr_init(task);
  ok &= expect_bool(task.registered_toggle_char_task_graph, true,
                    "task init registers toggle");
  ok &= expect_bool(source_char_task_mgr_toggle_graph(task), true,
                    "task first toggle returns true");
  ok &= expect_bool(task.show_graph, true, "task first toggle state");
  ok &= expect_bool(source_char_task_mgr_toggle_graph(task), false,
                    "task second toggle returns false");
  ok &= expect_bool(task.show_graph, false, "task second toggle state");

  return ok ? 0 : 1;
}
