// engine/src/script/interp.h
//
// Interp -- a faithful interpreter for Harmonix's Sandbox DataArray script
// language, the Lisp-like language GH2's compiled DTB menu/config files are
// written in. This is the execution core the menu UI runs on: every screen is
// a {new GHPanel ...}/{new GHScreen ...} object tree whose (enter)/(poll)/
// (SELECT_START_MSG ...) handlers are DataArray scripts evaluated here.
//
// We interpret directly off gh::dtb::Node (the tools/dtb parse tree) rather
// than a parallel AST: the parser already preserves the load-bearing array-tag
// distinction the language depends on --
//   0x10 "(...)"  data array        (a list of values; case/decl/list literal)
//   0x11 "{...}"  command/script     (evaluate: builtin, or {obj msg args...})
//   0x13 "[...]"  property reference  ([foo] == get_property foo on $this)
// -- which dtb_bridge::from_tree() collapses. Symbols/strings/vars/ints/floats
// map to ghogx::DataNode runtime values.
//
// Object resolution, global $variables, locale and diagnostics are delegated to
// a Host so the script core stays free of UI/ARK/screen-manager dependencies
// (and unit-testable headless). The screen manager implements Host in Phase 3.

#pragma once

#include "core/data_node.h"
#include "core/symbol.h"

#include "dtb.h"  // gh::dtb::Node / NodeList (tools/dtb)

#include <string>
#include <utility>
#include <vector>

namespace ghogx {
class Object;
}

namespace ghogx::script {

using Node = gh::dtb::Node;
using NodeList = gh::dtb::NodeList;  // std::vector<std::shared_ptr<gh::dtb::Node>>

// --- node-kind helpers (raw dtb tags; see header comment) ------------------
inline bool is_command(const Node& n) { return n.tag == 0x11; }
inline bool is_data_array(const Node& n) { return n.tag == 0x10; }
inline bool is_prop_ref(const Node& n) { return n.tag == 0x13; }
inline bool is_variable(const Node& n) { return n.tag == 0x02; }
inline bool is_symbol(const Node& n) { return n.tag == 0x05; }
inline bool is_int(const Node& n) { return n.tag == 0x00; }
inline bool is_float(const Node& n) { return n.tag == 0x01; }

// --- runtime truthiness / equality on DataNode -----------------------------
// Sandbox truthiness: 0 / 0.0 / the symbol FALSE / empty / null-object are
// false; everything else (incl. any other symbol, non-empty string, nonzero
// number, the symbol TRUE, a live object) is true.
bool truthy(const DataNode& v);
bool node_equal(const DataNode& a, const DataNode& b);

// --- local variable scope chain --------------------------------------------
// Handler parameters ((a $b) leading list) and {do (locals) ...} bindings live
// here. $this is NOT a scope var (it resolves to Env::self); other $names fall
// through to the Host's global store on a miss.
class Scope {
 public:
  explicit Scope(Scope* parent = nullptr) : parent_(parent) {}

  // Declare/define a variable in THIS scope (shadows parents).
  void declare(Symbol name, DataNode value = DataNode());
  // Find a variable in this scope or any ancestor; nullptr if absent.
  DataNode* find(Symbol name);
  // Assign to the nearest scope that already declares `name`; returns false if
  // none does (caller then writes a global).
  bool assign(Symbol name, DataNode value);

  Scope* parent() const { return parent_; }

 private:
  std::vector<std::pair<Symbol, DataNode>> vars_;
  Scope* parent_;
};

// --- host services the interpreter needs from its environment --------------
class Host {
 public:
  virtual ~Host() = default;

  // Resolve a bare-symbol command target to an object: a global singleton
  // (ui/game/gamecfg/synth/taskmgr/campaign/...), a named screen/panel, or a
  // dotted child path (e.g. "main_msg.view"). nullptr if unresolved.
  virtual Object* resolve_object(Symbol name) = 0;

  // Global $variables: persist across handler invocations. {set $g v} writes a
  // global when no local scope declares g.
  virtual DataNode get_global(Symbol name) = 0;
  virtual void set_global(Symbol name, DataNode value) = 0;

  // {localize token} -> display string. Default returns the token text so a
  // missing locale entry renders visibly rather than crashing.
  virtual std::string localize(Symbol token) { return std::string(token.c_str()); }

  // Diagnostics worklist: a builtin or class::message the interpreter hit but
  // does not implement. Hosts should dedup. Drives the screen fan-out.
  virtual void on_unhandled(const std::string& what) { (void)what; }
};

// Per-evaluation context. self = $this / [prop] target; component-style params
// live in `scope` as ordinary locals (e.g. $component for *_MSG handlers).
struct Env {
  Object* self = nullptr;
  Scope* scope = nullptr;
  Host* host = nullptr;
};

class Interp {
 public:
  // Evaluate one node to a value.
  DataNode eval(const Node& n, Env& env);

  // Evaluate a statement sequence (e.g. a handler body), returning the last
  // value. `start` skips leading nodes already consumed (handler name/params).
  DataNode eval_seq(const NodeList& body, std::size_t start, Env& env);

  // Run a located handler body on `self`, binding `params` (the leading
  // (a $b ...) list, if present) from `args`. Creates a child scope under
  // parent_env.scope. Used by the screen manager to fire (enter)/(SELECT_START
  // _MSG)/custom handlers.
  DataNode run_handler(const NodeList& handler_body, Object* self,
                       const DataArray& args, Env& parent_env);

 private:
  DataNode eval_command(const Node& cmd, Env& env);
  // Resolve a node used in command-head position to an object pointer.
  Object* eval_object(const Node& head, Env& env);
  // Evaluate children[arg_start..] of `cmd` into a positional DataArray.
  DataArray eval_args(const Node& cmd, std::size_t arg_start, Env& env);

  // Builtins (each controls evaluation of its own operands).
  DataNode bi_if(const Node& c, Env&, bool has_else);
  DataNode bi_do(const Node& c, Env&);
  DataNode bi_switch(const Node& c, Env&);
  DataNode bi_foreach(const Node& c, Env&);
  DataNode bi_set(const Node& c, Env&);
  DataNode bi_compare(const Node& c, Env&, int op);   // == != < > <= >=
  DataNode bi_logic(const Node& c, Env&, int op);     // ! && ||
  DataNode bi_arith(const Node& c, Env&, int op);     // + - * /
  DataNode bi_sprintf(const Node& c, Env&);
  DataNode bi_localize(const Node& c, Env&);
  DataNode bi_print(const Node& c, Env&);
};

}  // namespace ghogx::script
