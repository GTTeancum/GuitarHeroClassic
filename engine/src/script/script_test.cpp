// engine/src/script/script_test.cpp
//
// Headless tests for the DataArray script interpreter. The trees are built to
// mirror the EXACT shapes found in stock GH2 ui/gen/main.dtb + init.dtb (the
// SELECT_START_MSG switch, the {if_else {> {campaign num_profiles} 0} ...}
// career branch, reset_player_settings' {do ($p {game get_player_config 1})...},
// init.dtb's {foreach $p (...) {$p load}}), so passing means the interpreter
// runs the real menu scripts. Plus a preprocessor #ifdef/#define check.

#include "core/object.h"
#include "script/interp.h"
#include "script/preprocess.h"

#include "dtb.h"

#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace ghogx;
using namespace ghogx::script;
using Node = gh::dtb::Node;
using NodePtr = std::shared_ptr<Node>;

static int g_failures = 0;
#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      std::fprintf(stderr, "FAIL %s:%d  CHECK(%s)\n", __FILE__, __LINE__, \
                   #cond);                                                \
      ++g_failures;                                                       \
    }                                                                     \
  } while (0)

// --- node builders (match dtb tags) ----------------------------------------
static NodePtr mkint(int v) { auto n = std::make_shared<Node>(); n->tag = 0x00; n->value = (int32_t)v; return n; }
static NodePtr mkflt(float v){ auto n = std::make_shared<Node>(); n->tag = 0x01; n->value = v; return n; }
static NodePtr mkvar(const char* s){ auto n = std::make_shared<Node>(); n->tag = 0x02; n->value = std::string(s); return n; }
static NodePtr mksym(const char* s){ auto n = std::make_shared<Node>(); n->tag = 0x05; n->value = std::string(s); return n; }
static NodePtr mkstr(const char* s){ auto n = std::make_shared<Node>(); n->tag = 0x12; n->value = std::string(s); return n; }
static NodePtr mkarr(std::vector<NodePtr> k){ auto n = std::make_shared<Node>(); n->tag = 0x10; n->value = std::move(k); return n; }
static NodePtr mkcmd(std::vector<NodePtr> k){ auto n = std::make_shared<Node>(); n->tag = 0x11; n->value = std::move(k); return n; }
static NodePtr mkprop(const char* name){ auto n = std::make_shared<Node>(); n->tag = 0x13; n->value = std::vector<NodePtr>{mksym(name)}; return n; }
static NodePtr mkdir(uint32_t tag, const char* payload){ auto n = std::make_shared<Node>(); n->tag = tag; n->value = std::string(payload ? payload : ""); return n; }

static std::string to_str(const DataNode& v) {
  switch (v.type()) {
    case DataType::kInt: return std::to_string(v.as_int().value_or(0));
    case DataType::kFloat: { char b[32]; std::snprintf(b, sizeof b, "%g", v.as_float().value_or(0)); return b; }
    case DataType::kSymbol:
    case DataType::kString: return std::string(v.as_string().value_or(""));
    case DataType::kObject: return "<obj>";
    default: return "<>";
  }
}

// --- mocks -----------------------------------------------------------------
class MockObject : public Object {
 public:
  explicit MockObject(const char* cls) : cls_(cls) {}
  Symbol class_name() const override { return cls_; }
  void ret(const char* msg, DataNode v) { returns_[Symbol(msg).id()] = std::move(v); }

  std::vector<std::string> calls;  // "msg:arg0,arg1"
  DataNode handle_property(Symbol msg, const DataArray& args) override {
    std::string rec = msg.c_str();
    rec += ':';
    for (std::size_t i = 0; i < args.size(); ++i) {
      if (i) rec += ',';
      rec += to_str(args.at(i));
    }
    calls.push_back(rec);
    auto it = returns_.find(msg.id());
    if (it != returns_.end()) return it->second;
    return Object::handle_property(msg, args);  // universal get/set/has/...
  }
  bool called(const std::string& rec) const {
    for (auto& c : calls) if (c == rec) return true;
    return false;
  }

 private:
  Symbol cls_;
  std::map<const void*, DataNode> returns_;
};

class MockHost : public Host {
 public:
  std::map<const void*, Object*> objs;
  std::map<const void*, DataNode> globals;
  std::vector<std::string> unhandled;

  void bind(const char* name, Object* o) { objs[Symbol(name).id()] = o; }
  Object* resolve_object(Symbol n) override { auto it = objs.find(n.id()); return it == objs.end() ? nullptr : it->second; }
  DataNode get_global(Symbol n) override { auto it = globals.find(n.id()); return it == globals.end() ? DataNode() : it->second; }
  void set_global(Symbol n, DataNode v) override { globals[n.id()] = std::move(v); }
  void on_unhandled(const std::string& w) override { unhandled.push_back(w); }
};

// --- tests -----------------------------------------------------------------
static void test_operators() {
  Interp ip; MockHost host; Scope root; Env env; env.host = &host; env.scope = &root;
  CHECK(ip.eval(*mkcmd({mksym("=="), mkint(2), mkint(2)}), env).as_int().value() == 1);
  CHECK(ip.eval(*mkcmd({mksym("=="), mkint(2), mkint(3)}), env).as_int().value() == 0);
  CHECK(ip.eval(*mkcmd({mksym(">"), mkint(5), mkint(0)}), env).as_int().value() == 1);
  CHECK(ip.eval(*mkcmd({mksym("!"), mkcmd({mksym("=="), mkint(1), mkint(2)})}), env).as_int().value() == 1);
  CHECK(ip.eval(*mkcmd({mksym("&&"), mkint(1), mkint(1)}), env).as_int().value() == 1);
  CHECK(ip.eval(*mkcmd({mksym("&&"), mkint(1), mkint(0)}), env).as_int().value() == 0);
  CHECK(ip.eval(*mkcmd({mksym("+"), mkint(2), mkint(3)}), env).as_int().value() == 5);
  CHECK(ip.eval(*mkcmd({mksym("+"), mkflt(1.5f), mkint(2)}), env).as_float().value() == 3.5f);
  // truthiness of the FALSE symbol (initial value of e.g. [already_entered])
  CHECK(ip.eval(*mkcmd({mksym("!"), mksym("FALSE")}), env).as_int().value() == 1);
  CHECK(ip.eval(*mkcmd({mksym("!"), mksym("TRUE")}), env).as_int().value() == 0);
  // if_else returns the taken branch value
  CHECK(ip.eval(*mkcmd({mksym("if_else"), mkint(1), mkint(10), mkint(20)}), env).as_int().value() == 10);
  CHECK(ip.eval(*mkcmd({mksym("if_else"), mkint(0), mkint(10), mkint(20)}), env).as_int().value() == 20);
}

static void test_vars_and_props() {
  Interp ip; MockHost host; Scope root; MockObject self("GHPanel");
  Env env; env.host = &host; env.scope = &root; env.self = &self;
  // global var round-trip: {set $x 7}; {== $x 7}
  ip.eval(*mkcmd({mksym("set"), mkvar("x"), mkint(7)}), env);
  CHECK(ip.eval(*mkcmd({mksym("=="), mkvar("x"), mkint(7)}), env).as_int().value() == 1);
  // self property via {set [foo] 42} then [foo] and {$this get foo}
  ip.eval(*mkcmd({mksym("set"), mkprop("foo"), mkint(42)}), env);
  CHECK(ip.eval(*mkprop("foo"), env).as_int().value() == 42);
  CHECK(ip.eval(*mkcmd({mkvar("this"), mksym("get"), mksym("foo")}), env).as_int().value() == 42);
  // {$this set already_entered TRUE} then {! [already_entered]} == 0
  ip.eval(*mkcmd({mkvar("this"), mksym("set"), mksym("already_entered"), mksym("TRUE")}), env);
  CHECK(ip.eval(*mkcmd({mksym("!"), mkprop("already_entered")}), env).as_int().value() == 0);
}

// Exact shape of stock main.dtb (SELECT_START_MSG ...): pick quickplay.
static void test_main_select_switch() {
  Interp ip; MockHost host; Scope root;
  MockObject ui("UIManager"), gamecfg("GameCfg");
  host.bind("ui", &ui); host.bind("gamecfg", &gamecfg);
  host.set_global(Symbol("component"), DataNode::Sym(Symbol("main_quickspin.btn")));
  Env env; env.host = &host; env.scope = &root;

  NodePtr sw = mkcmd({
    mksym("switch"), mkvar("component"),
    mkarr({mksym("main_career.btn"),
           mkcmd({mksym("gamecfg"), mksym("set"), mksym("mode"), mksym("career")}),
           mkcmd({mksym("ui"), mksym("goto_screen"), mksym("chooseprof_screen")})}),
    mkarr({mksym("main_quickspin.btn"),
           mkcmd({mksym("gamecfg"), mksym("set"), mksym("mode"), mksym("quickplay")}),
           mkcmd({mksym("ui"), mksym("goto_screen"), mksym("qp_selsong_screen")})}),
    mkarr({mksym("main_options.btn"),
           mkcmd({mksym("ui"), mksym("goto_screen"), mksym("options_screen")})}),
  });
  ip.eval(*sw, env);
  CHECK(gamecfg.called("set:mode,quickplay"));
  CHECK(ui.called("goto_screen:qp_selsong_screen"));
  CHECK(!ui.called("goto_screen:chooseprof_screen"));
  CHECK(!ui.called("goto_screen:options_screen"));
}

// Exact shape of main_career.btn: {if_else {> {campaign num_profiles} 0} A B}.
static void test_career_branch() {
  // num_profiles == 0 -> else branch (nameprof flow)
  {
    Interp ip; MockHost host; Scope root;
    MockObject ui("UIManager"), campaign("Campaign"), nameprof("GHScreen");
    campaign.ret("num_profiles", DataNode::Int(0));
    host.bind("ui", &ui); host.bind("campaign", &campaign); host.bind("nameprof_screen", &nameprof);
    Env env; env.host = &host; env.scope = &root;
    NodePtr e = mkcmd({mksym("if_else"),
        mkcmd({mksym(">"), mkcmd({mksym("campaign"), mksym("num_profiles")}), mkint(0)}),
        mkcmd({mksym("ui"), mksym("goto_screen"), mksym("chooseprof_screen")}),
        mkcmd({mksym("do"),
               mkcmd({mksym("nameprof_screen"), mksym("set"), mksym("back_screen"), mksym("main_screen")}),
               mkcmd({mksym("ui"), mksym("goto_screen"), mksym("nameprof_screen")})})});
    ip.eval(*e, env);
    CHECK(ui.called("goto_screen:nameprof_screen"));
    CHECK(!ui.called("goto_screen:chooseprof_screen"));
    CHECK(nameprof.called("set:back_screen,main_screen"));
  }
  // num_profiles == 2 -> then branch (chooseprof)
  {
    Interp ip; MockHost host; Scope root;
    MockObject ui("UIManager"), campaign("Campaign");
    campaign.ret("num_profiles", DataNode::Int(2));
    host.bind("ui", &ui); host.bind("campaign", &campaign);
    Env env; env.host = &host; env.scope = &root;
    NodePtr e = mkcmd({mksym("if_else"),
        mkcmd({mksym(">"), mkcmd({mksym("campaign"), mksym("num_profiles")}), mkint(0)}),
        mkcmd({mksym("ui"), mksym("goto_screen"), mksym("chooseprof_screen")}),
        mkint(0)});
    ip.eval(*e, env);
    CHECK(ui.called("goto_screen:chooseprof_screen"));
  }
}

// reset_player_settings shape: {do ($p2 {game get_player_config 1}) {$p2 set_difficulty ...}}
static void test_do_local_object() {
  Interp ip; MockHost host; Scope root;
  MockObject game("Game"), p2("PlayerCfg");
  game.ret("get_player_config", DataNode::Obj(&p2));
  host.bind("game", &game);
  Env env; env.host = &host; env.scope = &root;
  NodePtr e = mkcmd({mksym("do"),
      mkarr({mkvar("p2"), mkcmd({mksym("game"), mksym("get_player_config"), mkint(1)})}),
      mkcmd({mkvar("p2"), mksym("set_character"), mksym("rockabill1"), mksym("TRUE")}),
      mkcmd({mkvar("p2"), mksym("set_difficulty"), mksym("kDifficultyMedium")})});
  ip.eval(*e, env);
  CHECK(p2.called("set_character:rockabill1,TRUE"));
  CHECK(p2.called("set_difficulty:kDifficultyMedium"));
}

// init.dtb shape: {foreach $p (a.panel b.panel) {$p load}}
static void test_foreach_load() {
  Interp ip; MockHost host; Scope root;
  MockObject a("GHPanel"), b("GHPanel");
  host.bind("a.panel", &a); host.bind("b.panel", &b);
  Env env; env.host = &host; env.scope = &root;
  NodePtr e = mkcmd({mksym("foreach"), mkvar("p"), mkarr({mksym("a.panel"), mksym("b.panel")}),
                     mkcmd({mkvar("p"), mksym("load")})});
  ip.eval(*e, env);
  CHECK(a.called("load:"));
  CHECK(b.called("load:"));
}

// run_handler with a (params) list, display_cheat_msg shape.
static void test_run_handler_params() {
  Interp ip; MockHost host; MockObject self("GHPanel"), lbl("UILabel");
  host.bind("mm.lbl", &lbl);
  Scope root; Env env; env.host = &host; env.scope = &root;
  // (display_cheat_msg ($cheat $enable) {mm.lbl set_text $cheat})
  NodeList handler = {mksym("display_cheat_msg"), mkarr({mkvar("cheat"), mkvar("enable")}),
                      mkcmd({mksym("mm.lbl"), mksym("set_text"), mkvar("cheat")})};
  DataArray args; args.push(DataNode::Sym(Symbol("my_cheat"))); args.push(DataNode::Int(1));
  ip.run_handler(handler, &self, args, env);
  CHECK(lbl.called("set_text:my_cheat"));
}

// sprintf + localize (display_cheat_msg uses {sprintf {localize ...} {localize $cheat}}).
static void test_sprintf() {
  Interp ip; MockHost host; Scope root; Env env; env.host = &host; env.scope = &root;
  DataNode r = ip.eval(*mkcmd({mksym("sprintf"), mkstr("%s=%d"), mksym("foo"), mkint(5)}), env);
  CHECK(to_str(r) == "foo=5");
}

static void test_preprocess() {
  // [ #ifdef HX_XBOX (a 1) #else (a 2) #endif ]
  NodeList roots = {mkdir(0x07, "HX_XBOX"), mkarr({mksym("a"), mkint(1)}),
                    mkdir(0x08, ""), mkarr({mksym("a"), mkint(2)}), mkdir(0x09, "")};
  {
    PreprocessOptions o;  // HX_XBOX undefined -> else branch
    NodeList out = preprocess(roots, o);
    CHECK(out.size() == 1);
    CHECK(out.size() == 1 && gh::dtb::children(*out[0]).size() == 2 &&
          gh::dtb::as_int(*gh::dtb::children(*out[0])[1]).value_or(-1) == 2);
  }
  {
    PreprocessOptions o; o.defines.insert("HX_XBOX");  // -> then branch
    NodeList out = preprocess(roots, o);
    CHECK(out.size() == 1 && gh::dtb::children(*out[0]).size() == 2 &&
          gh::dtb::as_int(*gh::dtb::children(*out[0])[1]).value_or(-1) == 1);
  }
  // #define FOO (1 2 3) ; {x FOO}  -> FOO substituted by the array
  {
    NodeList r2 = {mkdir(0x20, "FOO"), mkarr({mkint(1), mkint(2), mkint(3)}),
                   mkcmd({mksym("x"), mksym("FOO")})};
    PreprocessOptions o;
    NodeList out = preprocess(r2, o);
    CHECK(out.size() == 1);  // the #define + body are consumed; only {x FOO} remains
    if (out.size() == 1) {
      const auto& cmd = gh::dtb::children(*out[0]);  // x , <array>
      CHECK(cmd.size() == 2 && gh::dtb::is_array(*cmd[1]) &&
            gh::dtb::children(*cmd[1]).size() == 3);
    }
  }
}

int main() {
  test_operators();
  test_vars_and_props();
  test_main_select_switch();
  test_career_branch();
  test_do_local_object();
  test_foreach_load();
  test_run_handler_params();
  test_sprintf();
  test_preprocess();
  if (g_failures == 0) {
    std::printf("ghogx_script_test: interpreter runs real main.dtb script shapes -- all checks passed\n");
    return 0;
  }
  std::fprintf(stderr, "ghogx_script_test: %d check(s) failed\n", g_failures);
  return 1;
}
