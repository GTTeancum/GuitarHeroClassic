// engine/src/script/interp.cpp -- see interp.h.

#include "script/interp.h"

#include "core/object.h"

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
  if (auto s = v.as_symbol()) return env.host ? env.host->resolve_object(*s) : nullptr;
  return nullptr;
}

// --- builtin table ---------------------------------------------------------
enum Builtin {
  BI_NONE = 0,
  BI_IF, BI_IF_ELSE, BI_DO, BI_SWITCH, BI_FOREACH, BI_SET,
  BI_EQ, BI_NE, BI_LT, BI_GT, BI_LE, BI_GE,
  BI_NOT, BI_AND, BI_OR,
  BI_ADD, BI_SUB, BI_MUL, BI_DIV,
  BI_SPRINTF, BI_LOCALIZE, BI_PRINT,
};

int lookup_builtin(Symbol s) {
  static const std::unordered_map<const void*, int> kTable = [] {
    std::unordered_map<const void*, int> m;
    auto add = [&](const char* name, int id) { m[Symbol(name).id()] = id; };
    add("if", BI_IF);          add("if_else", BI_IF_ELSE);
    add("do", BI_DO);          add("switch", BI_SWITCH);
    add("foreach", BI_FOREACH);add("set", BI_SET);
    add("==", BI_EQ);          add("!=", BI_NE);
    add("<", BI_LT);           add(">", BI_GT);
    add("<=", BI_LE);          add(">=", BI_GE);
    add("!", BI_NOT);          add("&&", BI_AND);   add("||", BI_OR);
    add("+", BI_ADD);          add("-", BI_SUB);
    add("*", BI_MUL);          add("/", BI_DIV);
    add("sprintf", BI_SPRINTF);add("localize", BI_LOCALIZE);
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
  // fall back to text compare (symbol/string)
  auto as = a.as_string(), bs = b.as_string();
  if (as && bs) return *as == *bs;
  return a.type() == DataType::kEmpty && b.type() == DataType::kEmpty;
}

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

DataNode Interp::eval_command(const Node& cmd, Env& env) {
  const NodeList& kids = gh::dtb::children(cmd);
  if (kids.empty()) return DataNode();
  const Node& head = *kids[0];

  if (is_symbol(head)) {
    Symbol h = node_sym(head);
    switch (lookup_builtin(h)) {
      case BI_IF: return bi_if(cmd, env, false);
      case BI_IF_ELSE: return bi_if(cmd, env, true);
      case BI_DO: return bi_do(cmd, env);
      case BI_SWITCH: return bi_switch(cmd, env);
      case BI_FOREACH: return bi_foreach(cmd, env);
      case BI_SET: return bi_set(cmd, env);
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
      case BI_SPRINTF: return bi_sprintf(cmd, env);
      case BI_LOCALIZE: return bi_localize(cmd, env);
      case BI_PRINT: return bi_print(cmd, env);
      default: break;  // not a builtin -> object message via symbol target
    }
    Object* t = env.host ? env.host->resolve_object(h) : nullptr;
    if (!t) {
      if (env.host) env.host->on_unhandled(std::string("target?:") + h.c_str());
      return DataNode();
    }
    if (kids.size() < 2) return DataNode();
    Symbol msg = node_sym(*kids[1]);
    DataArray args = eval_args(cmd, 2, env);
    return t->handle_property(msg, args);
  }

  // head is $var / {command} / [prop] -> object target
  Object* t = eval_object(head, env);
  if (!t) {
    if (env.host) env.host->on_unhandled("target?:<expr>");
    return DataNode();
  }
  if (kids.size() < 2) return DataNode();
  Symbol msg = node_sym(*kids[1]);
  DataArray args = eval_args(cmd, 2, env);
  return t->handle_property(msg, args);
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
  if (k.size() < 3 || !is_variable(*k[1]) || !is_data_array(*k[2])) return DataNode();
  Symbol var = node_sym(*k[1]);
  Scope scope(env.scope);
  Env e = env;
  e.scope = &scope;
  scope.declare(var, DataNode());
  for (const auto& item : gh::dtb::children(*k[2])) {
    scope.assign(var, to_literal(*item));
    eval_seq(k, 3, e);
  }
  return DataNode();
}

DataNode Interp::bi_set(const Node& c, Env& env) {
  const NodeList& k = gh::dtb::children(c);
  if (k.size() < 3) return DataNode();
  DataNode value = eval(*k[2], env);
  const Node& target = *k[1];
  if (is_prop_ref(target)) {
    const NodeList& pr = gh::dtb::children(target);
    if (!pr.empty() && env.self) env.self->set_property(node_sym(*pr[0]), value);
  } else if (is_variable(target)) {
    Symbol name = node_sym(target);
    if (!(env.scope && env.scope->assign(name, value))) {
      if (env.host) env.host->set_global(name, value);
    }
  }
  return value;
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
      if (sp == 's') out += std::string(a.as_string().value_or(""));
      else if (sp == 'd' || sp == 'i') out += std::to_string(a.as_int().value_or(0));
      else if (sp == '%') out += '%';
      else { out += '%'; out += sp; }
    } else {
      out += fmt[i];
    }
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
