// engine/src/script/interp.cpp -- see interp.h.

#include "script/interp.h"

#include "core/object.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_map>

namespace ghogx::script {

namespace {

// dtb payload string of any string-like node (symbol/var/quoted/directive).
std::string node_str(const Node& n) {
  auto s = gh::dtb::as_string(n);
  return s ? *s : std::string();
}
Symbol node_sym(const Node& n) { return Symbol(node_str(n)); }

// Convert a node to a *literal* DataNode without executing commands. Used when
// a (...) data array appears in value position (e.g. a message argument like
// the helpbar display list); preserves the tree shape as nested DataArrays.
DataNode to_literal(const Node& n) {
  switch (n.tag) {
    case 0x00: return DataNode::Int(gh::dtb::as_int(n).value_or(0));
    case 0x01: return DataNode::Float(gh::dtb::as_float(n).value_or(0.0f));
    case 0x12: return DataNode::Str(node_str(n));
    case 0x05:
    case 0x02: return DataNode::Sym(Symbol(node_str(n)));
    case 0x10:
    case 0x11:
    case 0x13: {
      auto arr = std::make_shared<DataArray>();
      for (const auto& k : gh::dtb::children(n)) arr->push(to_literal(*k));
      return DataNode::Array(arr);
    }
    default: return DataNode();
  }
}

Object* node_to_object(const DataNode& v, Env& env) {
  if (Object* o = v.as_object()) return o;
  if (auto s = v.as_string()) return env.host ? env.host->resolve_object(Symbol(*s)) : nullptr;
  return nullptr;
}

std::shared_ptr<DataArray> array_value(const DataNode& v, Env& env) {
  if (auto a = v.as_array()) return a;
  if (auto s = v.as_string()) {
    if (env.host) {
      if (auto a = env.host->get_global(Symbol(*s)).as_array()) return a;
    }
  }
  return nullptr;
}

int int_value(const DataNode& v, int fallback = 0) {
  if (auto i = v.as_int()) return *i;
  if (auto f = v.as_float()) return static_cast<int>(*f);
  if (auto arr = v.as_array()) {
    if (arr->size() == 1) return int_value(arr->at(0), fallback);
  }
  return fallback;
}

DataNode bi_new(const Node& cmd, Env& env) {
  const auto& kids = gh::dtb::children(cmd);
  if (kids.size() < 3 || !env.host) return DataNode();
  if (!is_symbol(*kids[1]) || !is_symbol(*kids[2])) {
    env.host->on_unhandled("new?:args");
    return DataNode();
  }
  Symbol cls = node_sym(*kids[1]);
  Symbol name = node_sym(*kids[2]);
  if (Object* obj = env.host->create_object(cls, name))
    return DataNode::Obj(obj);
  env.host->on_unhandled(std::string("new?:") + cls.c_str());
  return DataNode();
}

// --- builtin table ---------------------------------------------------------
enum Builtin {
  BI_NONE = 0,
  BI_IF, BI_IF_ELSE, BI_COND, BI_DO, BI_SWITCH, BI_FOREACH, BI_SET,
  BI_EQ, BI_NE, BI_LT, BI_GT, BI_LE, BI_GE,
  BI_NOT, BI_AND, BI_OR,
  BI_ADD, BI_SUB, BI_MUL, BI_DIV,
  BI_TO_INT, BI_MAX, BI_MOD, BI_PLUS_EQ, BI_INC,
  BI_ELEM, BI_RANDOM_ELEM, BI_REMOVE_ELEM, BI_PUSH_BACK, BI_RESIZE,
  BI_FIND_ELEM, BI_SIZE,
  BI_FOREACH_INT, BI_NEW, BI_PRINTF, BI_SCRIPT, BI_SCRIPT_TASK,
  BI_SPRINT, BI_SPRINTF, BI_LOCALIZE, BI_PRINT,
};

int lookup_builtin(Symbol s) {
  static const std::unordered_map<const void*, int> kTable = [] {
    std::unordered_map<const void*, int> m;
    auto add = [&](const char* name, int id) { m[Symbol(name).id()] = id; };
    add("if", BI_IF);          add("if_else", BI_IF_ELSE);
    add("cond", BI_COND);
    add("do", BI_DO);          add("switch", BI_SWITCH);
    add("foreach", BI_FOREACH);add("set", BI_SET);
    add("foreach_int", BI_FOREACH_INT);
    add("==", BI_EQ);          add("!=", BI_NE);
    add("<", BI_LT);           add(">", BI_GT);
    add("<=", BI_LE);          add(">=", BI_GE);
    add("!", BI_NOT);          add("&&", BI_AND);   add("||", BI_OR);
    add("+", BI_ADD);          add("-", BI_SUB);
    add("*", BI_MUL);          add("/", BI_DIV);
    add("int", BI_TO_INT);     add("max", BI_MAX);   add("mod", BI_MOD);
    add("+=", BI_PLUS_EQ);     add("++", BI_INC);
    add("elem", BI_ELEM);      add("random_elem", BI_RANDOM_ELEM);
    add("remove_elem", BI_REMOVE_ELEM);
    add("push_back", BI_PUSH_BACK); add("resize", BI_RESIZE);
    add("find_elem", BI_FIND_ELEM); add("size", BI_SIZE);
    add("new", BI_NEW);        add("printf", BI_PRINTF);
    add("script", BI_SCRIPT);  add("script_task", BI_SCRIPT_TASK);
    add("sprint", BI_SPRINT);  add("sprintf", BI_SPRINTF);
    add("localize", BI_LOCALIZE);
    add("print", BI_PRINT);
    return m;
  }();
  auto it = kTable.find(s.id());
  return it == kTable.end() ? BI_NONE : it->second;
}

}  // namespace

// --- Scope -----------------------------------------------------------------
void Scope::declare(Symbol name, DataNode value) {
  for (auto& kv : vars_) {
    if (kv.first == name) { kv.second = std::move(value); return; }
  }
  vars_.emplace_back(name, std::move(value));
}
DataNode* Scope::find(Symbol name) {
  for (auto& kv : vars_)
    if (kv.first == name) return &kv.second;
  return parent_ ? parent_->find(name) : nullptr;
}
bool Scope::assign(Symbol name, DataNode value) {
  for (auto& kv : vars_) {
    if (kv.first == name) { kv.second = std::move(value); return true; }
  }
  return parent_ ? parent_->assign(name, std::move(value)) : false;
}

// --- truthiness / equality -------------------------------------------------
bool truthy(const DataNode& v) {
  switch (v.type()) {
    case DataType::kInt: return v.as_int().value_or(0) != 0;
    case DataType::kFloat: return v.as_float().value_or(0.0f) != 0.0f;
    case DataType::kSymbol: {
      auto s = v.as_string();
      if (!s) return false;
      return !(*s == "FALSE" || s->empty());  // TRUE and any other symbol -> true
    }
    case DataType::kString: return !v.as_string().value_or("").empty();
    case DataType::kObject: return v.as_object() != nullptr;
    case DataType::kArray: return v.as_array() && v.as_array()->size() > 0;
    case DataType::kEmpty: default: return false;
  }
}

bool node_equal(const DataNode& a, const DataNode& b) {
  // numbers compare by value (int/float coerced); symbols by interned identity;
  // strings by text. Cross-type number compare allowed.
  bool an = a.type() == DataType::kInt || a.type() == DataType::kFloat;
  bool bn = b.type() == DataType::kInt || b.type() == DataType::kFloat;
  if (an && bn) return a.as_float().value() == b.as_float().value();
  if (a.type() == DataType::kSymbol && b.type() == DataType::kSymbol)
    return a.as_symbol().value() == b.as_symbol().value();
  if (a.type() == DataType::kObject || b.type() == DataType::kObject)
    return a.as_object() == b.as_object();
  if (a.type() == DataType::kArray || b.type() == DataType::kArray) {
    auto aa = a.as_array(), ba = b.as_array();
    if (!aa || !ba || aa->size() != ba->size()) return false;
    for (std::size_t i = 0; i < aa->size(); ++i) {
      if (!node_equal(aa->at(i), ba->at(i))) return false;
    }
    return true;
  }
  // fall back to text compare (symbol/string)
  auto as = a.as_string(), bs = b.as_string();
  if (as && bs) return *as == *bs;
  return a.type() == DataType::kEmpty && b.type() == DataType::kEmpty;
}

Symbol message_name(const Node& n, Env& env);

// --- Interp ----------------------------------------------------------------
DataNode Interp::eval(const Node& n, Env& env) {
  switch (n.tag) {
    case 0x00: return DataNode::Int(gh::dtb::as_int(n).value_or(0));
    case 0x01: return DataNode::Float(gh::dtb::as_float(n).value_or(0.0f));
    case 0x12: return DataNode::Str(node_str(n));
    case 0x05: return DataNode::Sym(Symbol(node_str(n)));
    case 0x02: {  // $variable
      std::string nm = node_str(n);
      if (nm == "this") return DataNode::Obj(env.self);
      Symbol s(nm);
      if (env.scope) {
        if (DataNode* v = env.scope->find(s)) return *v;
      }
      return env.host ? env.host->get_global(s) : DataNode();
    }
    case 0x13: {  // [prop] -> get_property on self
      const NodeList& k = gh::dtb::children(n);
      if (k.empty() || !env.self) return DataNode();
      return env.self->get_property(node_sym(*k[0]));
    }
    case 0x11: return eval_command(n, env);  // {command}
    case 0x10: return to_literal(n);          // (data array) literal
    default: return DataNode();
  }
}

DataNode Interp::eval_seq(const NodeList& body, std::size_t start, Env& env) {
  DataNode last;
  for (std::size_t i = start; i < body.size(); ++i) last = eval(*body[i], env);
  return last;
}

DataArray Interp::eval_args(const Node& cmd, std::size_t arg_start, Env& env) {
  DataArray args;
  const NodeList& k = gh::dtb::children(cmd);
  for (std::size_t i = arg_start; i < k.size(); ++i) args.push(eval(*k[i], env));
  return args;
}

Object* Interp::eval_object(const Node& head, Env& env) {
  return node_to_object(eval(head, env), env);
}

DataNode Interp::read_target(const Node& target, Env& env) {
  if (is_prop_ref(target)) {
    const NodeList& pr = gh::dtb::children(target);
    return (!pr.empty() && env.self) ? env.self->get_property(node_sym(*pr[0]))
                                    : DataNode();
  }
  if (is_variable(target)) {
    Symbol name = node_sym(target);
    if (env.scope) {
      if (DataNode* v = env.scope->find(name)) return *v;
    }
    return env.host ? env.host->get_global(name) : DataNode();
  }
  return eval(target, env);
}

void Interp::assign_target(const Node& target, DataNode value, Env& env) {
  if (is_prop_ref(target)) {
    const NodeList& pr = gh::dtb::children(target);
    if (!pr.empty() && env.self) env.self->set_property(node_sym(*pr[0]), std::move(value));
    return;
  }
  if (is_variable(target)) {
    Symbol name = node_sym(target);
    if (!(env.scope && env.scope->assign(name, value))) {
      if (env.host) env.host->set_global(name, std::move(value));
    }
  }
}

DataNode Interp::eval_command(const Node& cmd, Env& env) {
  const NodeList& kids = gh::dtb::children(cmd);
  if (kids.empty()) return DataNode();
  const Node& head = *kids[0];

  if (is_symbol(head)) {
    Symbol h = node_sym(head);
    switch (lookup_builtin(h)) {
      case BI_IF: return bi_if(cmd, env, false);
      case BI_IF_ELSE: return bi_if(cmd, env, true);
      case BI_COND: return bi_cond(cmd, env);
      case BI_DO: return bi_do(cmd, env);
      case BI_SWITCH: return bi_switch(cmd, env);
      case BI_FOREACH: return bi_foreach(cmd, env);
      case BI_FOREACH_INT: return bi_foreach_int(cmd, env);
      case BI_SET: return bi_set(cmd, env);
      case BI_PLUS_EQ: return bi_mutate_number(cmd, env, BI_PLUS_EQ);
      case BI_INC: return bi_mutate_number(cmd, env, BI_INC);
      case BI_EQ: return bi_compare(cmd, env, BI_EQ);
      case BI_NE: return bi_compare(cmd, env, BI_NE);
      case BI_LT: return bi_compare(cmd, env, BI_LT);
      case BI_GT: return bi_compare(cmd, env, BI_GT);
      case BI_LE: return bi_compare(cmd, env, BI_LE);
      case BI_GE: return bi_compare(cmd, env, BI_GE);
      case BI_NOT: return bi_logic(cmd, env, BI_NOT);
      case BI_AND: return bi_logic(cmd, env, BI_AND);
      case BI_OR: return bi_logic(cmd, env, BI_OR);
      case BI_ADD: return bi_arith(cmd, env, BI_ADD);
      case BI_SUB: return bi_arith(cmd, env, BI_SUB);
      case BI_MUL: return bi_arith(cmd, env, BI_MUL);
      case BI_DIV: return bi_arith(cmd, env, BI_DIV);
      case BI_TO_INT: return bi_math_func(cmd, env, BI_TO_INT);
      case BI_MAX: return bi_math_func(cmd, env, BI_MAX);
      case BI_MOD: return bi_math_func(cmd, env, BI_MOD);
      case BI_ELEM: return bi_elem(cmd, env);
      case BI_RANDOM_ELEM: return bi_random_elem(cmd, env);
      case BI_REMOVE_ELEM: return bi_remove_elem(cmd, env);
      case BI_PUSH_BACK: return bi_push_back(cmd, env);
      case BI_RESIZE: return bi_resize(cmd, env);
      case BI_FIND_ELEM: return bi_find_elem(cmd, env);
      case BI_SIZE: return bi_size(cmd, env);
      case BI_NEW: return bi_new(cmd, env);
      case BI_PRINTF:
      case BI_SCRIPT_TASK: return DataNode();
      case BI_SCRIPT: return eval_seq(kids, 1, env);
      case BI_SPRINT: return bi_sprint(cmd, env);
      case BI_SPRINTF: return bi_sprintf(cmd, env);
      case BI_LOCALIZE: return bi_localize(cmd, env);
      case BI_PRINT: return bi_print(cmd, env);
      default: break;  // not a builtin -> object message via symbol target
    }
    if (env.host) {
      if (auto fn = env.host->resolve_function(h)) {
        NodeList fk = gh::dtb::children(*fn);
        if (!fk.empty()) {
          NodeList handler;
          if (fk[0] && is_symbol(*fk[0]) && node_str(*fk[0]) == "func") {
            if (fk.size() < 2) return DataNode();
            handler.assign(fk.begin() + 1, fk.end());
          } else {
            handler = fk;
          }
          DataArray args = eval_args(cmd, 1, env);
          return run_handler(handler, env.self, args, env);
        }
      }
    }
    Object* t = env.host ? env.host->resolve_object(h) : nullptr;
    if (!t) {
      if (env.host) env.host->on_unhandled(std::string("target?:") + h.c_str());
      return DataNode();
    }
    if (kids.size() < 2) return DataNode();
    Symbol msg = message_name(*kids[1], env);
    if (!msg.valid()) {
      if (env.host) env.host->on_unhandled("message?:<expr>");
      return DataNode();
    }
    DataArray args = eval_args(cmd, 2, env);
    DataNode result = t->handle_property(msg, args);
    if (auto arr = result.as_array()) {
      for (std::size_t i = 0; i < arr->size() && 2 + i < kids.size(); ++i)
        assign_target(*kids[2 + i], arr->at(i), env);
    }
    return result;
  }

  // head is $var / {command} / [prop] -> object target
  Object* t = eval_object(head, env);
  if (!t) {
    if (env.host) env.host->on_unhandled("target?:<expr>");
    return DataNode();
  }
  if (kids.size() < 2) return DataNode();
  Symbol msg = message_name(*kids[1], env);
  if (!msg.valid()) {
    if (env.host) env.host->on_unhandled("message?:<expr>");
    return DataNode();
  }
  DataArray args = eval_args(cmd, 2, env);
  DataNode result = t->handle_property(msg, args);
  if (auto arr = result.as_array()) {
    for (std::size_t i = 0; i < arr->size() && 2 + i < kids.size(); ++i)
      assign_target(*kids[2 + i], arr->at(i), env);
  }
  return result;
}

Symbol message_name(const Node& n, Env& env) {
  if (is_symbol(n)) return node_sym(n);
  DataNode value = env.host ? Interp().eval(n, env) : DataNode();
  if (auto s = value.as_symbol()) return *s;
  if (auto text = value.as_string()) return Symbol(std::string(*text).c_str());
  return Symbol();
}

DataNode Interp::run_handler(const NodeList& handler_body, Object* self,
                             const DataArray& args, Env& parent_env) {
  Scope scope(parent_env.scope);
  Env env;
  env.self = self;
  env.scope = &scope;
  env.host = parent_env.host;

  std::size_t start = 1;  // [0] is the handler name symbol
  // Optional parameter list: (a $b ...) as the node right after the name.
  if (handler_body.size() > 1 && is_data_array(*handler_body[1])) {
    const NodeList& params = gh::dtb::children(*handler_body[1]);
    bool all_vars = !params.empty();
    for (const auto& p : params)
      if (!is_variable(*p)) { all_vars = false; break; }
    if (all_vars) {
      for (std::size_t i = 0; i < params.size(); ++i) {
        DataNode v = i < args.size() ? args.at(i) : DataNode();
        scope.declare(node_sym(*params[i]), v);
      }
      start = 2;
    }
  }
  return eval_seq(handler_body, start, env);
}

// --- builtins --------------------------------------------------------------
DataNode Interp::bi_if(const Node& c, Env& env, bool has_else) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 2) return DataNode();
  bool cond = truthy(eval(*k[1], env));
  if (has_else) {
    if (cond) return k.size() > 2 ? eval(*k[2], env) : DataNode();
    return k.size() > 3 ? eval(*k[3], env) : DataNode();
  }
  if (cond) return eval_seq(k, 2, env);
  return DataNode();
}

DataNode Interp::bi_cond(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  for (std::size_t i = 1; i < k.size(); ++i) {
    if (!is_data_array(*k[i])) continue;
    const NodeList& cse = gh::dtb::children(*k[i]);
    if (cse.empty()) continue;
    if (truthy(eval(*cse[0], env)))
      return cse.size() > 1 ? eval_seq(cse, 1, env) : DataNode();
  }
  return DataNode();
}

DataNode Interp::bi_do(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  Scope scope(env.scope);
  Env e = env;
  e.scope = &scope;
  std::size_t i = 1;
  // Leading (var [init]) arrays are local declarations.
  for (; i < k.size(); ++i) {
    if (!is_data_array(*k[i])) break;
    const NodeList& d = gh::dtb::children(*k[i]);
    if (d.empty() || !is_variable(*d[0])) break;
    DataNode init = d.size() > 1 ? eval(*d[1], e) : DataNode();
    scope.declare(node_sym(*d[0]), init);
  }
  return eval_seq(k, i, e);
}

DataNode Interp::bi_switch(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 2) return DataNode();
  DataNode val = eval(*k[1], env);
  const Node* default_case = nullptr;
  for (std::size_t i = 2; i < k.size(); ++i) {
    if (!is_data_array(*k[i])) { default_case = k[i].get(); continue; }
    const NodeList& cse = gh::dtb::children(*k[i]);
    if (cse.empty()) continue;
    if (node_equal(val, eval(*cse[0], env))) return eval_seq(cse, 1, env);
  }
  if (default_case) return eval(*default_case, env);
  return DataNode();
}

DataNode Interp::bi_foreach(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 3 || !is_variable(*k[1])) return DataNode();
  Symbol var = node_sym(*k[1]);
  Scope scope(env.scope);
  Env e = env;
  e.scope = &scope;
  scope.declare(var, DataNode());
  DataNode source = is_data_array(*k[2]) ? to_literal(*k[2]) : eval(*k[2], e);
  auto arr = array_value(source, e);
  if (!arr) return DataNode();
  for (std::size_t i = 0; i < arr->size(); ++i) {
    scope.assign(var, arr->at(i));
    eval_seq(k, 3, e);
  }
  return DataNode();
}

DataNode Interp::bi_foreach_int(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 4 || !is_variable(*k[1])) return DataNode();
  Symbol var = node_sym(*k[1]);
  int begin = int_value(eval(*k[2], env), 0);
  int end = int_value(eval(*k[3], env), begin);
  Scope scope(env.scope);
  Env e = env;
  e.scope = &scope;
  scope.declare(var, DataNode::Int(begin));
  int step = begin <= end ? 1 : -1;
  for (int i = begin;; i += step) {
    scope.assign(var, DataNode::Int(i));
    eval_seq(k, 4, e);
    if (i == end) break;
  }
  return DataNode();
}

DataNode Interp::bi_set(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 3) return DataNode();
  DataNode value = eval(*k[2], env);
  assign_target(*k[1], value, env);
  return value;
}

DataNode Interp::bi_mutate_number(const Node& c, Env& env, int op) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 2) return DataNode();
  int value = read_target(*k[1], env).as_int().value_or(0);
  if (op == BI_INC) {
    ++value;
  } else {
    value += k.size() > 2 ? eval(*k[2], env).as_int().value_or(0) : 0;
  }
  DataNode out = DataNode::Int(value);
  assign_target(*k[1], out, env);
  return out;
}

DataNode Interp::bi_compare(const Node& c, Env& env, int op) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 3) return DataNode::Int(0);
  DataNode a = eval(*k[1], env), b = eval(*k[2], env);
  bool r = false;
  if (op == BI_EQ) r = node_equal(a, b);
  else if (op == BI_NE) r = !node_equal(a, b);
  else {
    float af = a.as_float().value_or(0.0f), bf = b.as_float().value_or(0.0f);
    if (op == BI_LT) r = af < bf;
    else if (op == BI_GT) r = af > bf;
    else if (op == BI_LE) r = af <= bf;
    else if (op == BI_GE) r = af >= bf;
  }
  return DataNode::Int(r ? 1 : 0);
}

DataNode Interp::bi_logic(const Node& c, Env& env, int op) {
  const NodeList& k = gh::dtb::children(c);
  if (op == BI_NOT)
    return DataNode::Int((k.size() >= 2 && truthy(eval(*k[1], env))) ? 0 : 1);
  // && / ||  (short-circuit)
  for (std::size_t i = 1; i < k.size(); ++i) {
    bool t = truthy(eval(*k[i], env));
    if (op == BI_AND && !t) return DataNode::Int(0);
    if (op == BI_OR && t) return DataNode::Int(1);
  }
  return DataNode::Int(op == BI_AND ? 1 : 0);
}

DataNode Interp::bi_arith(const Node& c, Env& env, int op) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 2) return DataNode::Int(0);
  std::vector<DataNode> v;
  bool any_float = false;
  for (std::size_t i = 1; i < k.size(); ++i) {
    v.push_back(eval(*k[i], env));
    if (v.back().type() == DataType::kFloat) any_float = true;
  }
  if (any_float) {
    float acc = v[0].as_float().value_or(0.0f);
    for (std::size_t i = 1; i < v.size(); ++i) {
      float x = v[i].as_float().value_or(0.0f);
      if (op == BI_ADD) acc += x; else if (op == BI_SUB) acc -= x;
      else if (op == BI_MUL) acc *= x; else if (op == BI_DIV) acc = x != 0 ? acc / x : 0;
    }
    return DataNode::Float(acc);
  }
  int32_t acc = v[0].as_int().value_or(0);
  for (std::size_t i = 1; i < v.size(); ++i) {
    int32_t x = v[i].as_int().value_or(0);
    if (op == BI_ADD) acc += x; else if (op == BI_SUB) acc -= x;
    else if (op == BI_MUL) acc *= x; else if (op == BI_DIV) acc = x != 0 ? acc / x : 0;
  }
  return DataNode::Int(acc);
}

DataNode Interp::bi_math_func(const Node& c, Env& env, int op) {
  const NodeList& k = gh::dtb::children(c);
  if (op == BI_TO_INT) {
    if (k.size() < 2) return DataNode::Int(0);
    return DataNode::Int(static_cast<int32_t>(eval(*k[1], env).as_float().value_or(0.0f)));
  }
  if (op == BI_MAX) {
    int best = k.size() > 1 ? eval(*k[1], env).as_int().value_or(0) : 0;
    for (std::size_t i = 2; i < k.size(); ++i)
      best = std::max(best, eval(*k[i], env).as_int().value_or(0));
    return DataNode::Int(best);
  }
  if (op == BI_MOD) {
    if (k.size() < 3) return DataNode::Int(0);
    int a = eval(*k[1], env).as_int().value_or(0);
    int b = eval(*k[2], env).as_int().value_or(1);
    return DataNode::Int(b == 0 ? 0 : a % b);
  }
  return DataNode();
}

DataNode Interp::bi_elem(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 3) return DataNode();
  auto arr = array_value(eval(*k[1], env), env);
  int index = eval(*k[2], env).as_int().value_or(0);
  if (!arr || arr->empty()) return DataNode();
  if (index < 0) index = 0;
  if (index >= static_cast<int>(arr->size())) index = static_cast<int>(arr->size() - 1);
  return arr->at(static_cast<std::size_t>(index));
}

DataNode Interp::bi_random_elem(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 2) return DataNode();
  auto arr = array_value(eval(*k[1], env), env);
  return (arr && !arr->empty()) ? arr->at(0) : DataNode();
}

DataNode Interp::bi_remove_elem(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 3) return DataNode();
  DataNode target = read_target(*k[1], env);
  auto arr = array_value(target, env);
  DataNode needle = eval(*k[2], env);
  if (arr) {
    for (std::size_t i = 0; i < arr->size(); ++i) {
      if (node_equal(arr->at(i), needle)) {
        arr->erase(i);
        break;
      }
    }
    assign_target(*k[1], DataNode::Array(arr), env);
  }
  return DataNode();
}

DataNode Interp::bi_push_back(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 3) return DataNode();
  DataNode target = read_target(*k[1], env);
  auto arr = array_value(target, env);
  if (!arr) arr = std::make_shared<DataArray>();
  arr->push(eval(*k[2], env));
  DataNode out = DataNode::Array(arr);
  assign_target(*k[1], out, env);
  return out;
}

DataNode Interp::bi_resize(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 3) return DataNode();
  DataNode target = read_target(*k[1], env);
  auto arr = array_value(target, env);
  if (!arr) arr = std::make_shared<DataArray>();
  int n = eval(*k[2], env).as_int().value_or(0);
  arr->resize(static_cast<std::size_t>(std::max(0, n)));
  DataNode out = DataNode::Array(arr);
  assign_target(*k[1], out, env);
  return out;
}

DataNode Interp::bi_find_elem(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 3) return DataNode::Int(-1);
  auto arr = array_value(eval(*k[1], env), env);
  DataNode needle = eval(*k[2], env);
  int found = -1;
  if (arr) {
    for (std::size_t i = 0; i < arr->size(); ++i) {
      if (node_equal(arr->at(i), needle)) {
        found = static_cast<int>(i);
        break;
      }
    }
  }
  DataNode out = DataNode::Int(found);
  if (k.size() > 3) assign_target(*k[3], out, env);
  return out;
}

DataNode Interp::bi_size(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 2) return DataNode::Int(0);
  auto arr = array_value(eval(*k[1], env), env);
  return DataNode::Int(arr ? static_cast<int32_t>(arr->size()) : 0);
}

DataNode Interp::bi_sprintf(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 2) return DataNode::Str("");
  std::string fmt(eval(*k[1], env).as_string().value_or(""));
  std::string out;
  std::size_t argi = 2;
  for (std::size_t i = 0; i < fmt.size(); ++i) {
    if (fmt[i] == '%' && i + 1 < fmt.size()) {
      char sp = fmt[++i];
      DataNode a = argi < k.size() ? eval(*k[argi++], env) : DataNode();
      if (sp == 's' || sp == 'S') out += std::string(a.as_string().value_or(""));
      else if (sp == 'd' || sp == 'D' || sp == 'i' || sp == 'I')
        out += std::to_string(a.as_int().value_or(0));
      else if (sp == '%') out += '%';
      else { out += '%'; out += sp; }
    } else {
      out += fmt[i];
    }
  }
  return DataNode::Str(out);
}

DataNode Interp::bi_sprint(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  std::string out;
  for (std::size_t i = 1; i < k.size(); ++i) {
    DataNode v = eval(*k[i], env);
    if (auto s = v.as_string()) out += std::string(*s);
    else if (auto n = v.as_int()) out += std::to_string(*n);
    else if (auto f = v.as_float()) out += std::to_string(*f);
  }
  return DataNode::Str(out);
}

DataNode Interp::bi_localize(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 2 || !env.host) return DataNode::Str("");
  DataNode tok = eval(*k[1], env);
  Symbol s = tok.as_symbol().value_or(Symbol(tok.as_string().value_or("")));
  return DataNode::Str(env.host->localize(s));
}

DataNode Interp::bi_print(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  for (std::size_t i = 1; i < k.size(); ++i) {
    std::string s(eval(*k[i], env).as_string().value_or(""));
    std::fprintf(stderr, "%s", s.c_str());
  }
  std::fprintf(stderr, "\n");
  return DataNode();
}

}  // namespace ghogx::script
