// engine/src/script/preprocess.cpp -- see preprocess.h.

#include "script/preprocess.h"

#include <map>
#include <memory>
#include <vector>

namespace ghogx::script {

namespace {

using Node = gh::dtb::Node;
using NodePtr = std::shared_ptr<Node>;

std::string dir_name(const Node& n) {
  auto s = gh::dtb::as_string(n);
  return s ? *s : std::string();
}

struct Ctx {
  std::set<std::string> defined;             // #ifdef-visible names
  std::map<std::string, NodePtr> macros;     // #define NAME -> body node
  const PreprocessOptions* opts = nullptr;
};

// One #ifdef/#ifndef frame on the conditional stack.
struct Cond {
  bool branch_active;  // emit nodes in the current branch?
  bool ever_taken;     // has any branch in this if/else chain been active?
  bool parent_active;  // was the enclosing region active?
};

NodeList process(const NodeList& in, Ctx& ctx);

bool is_macro_symbol(const Node& n, Ctx& ctx) {
  return n.tag == 0x05 && ctx.macros.find(dir_name(n)) != ctx.macros.end();
}

bool command_preserves_macro_arg(const NodeList& kids, std::size_t index,
                                 Ctx& ctx) {
  if (index == 0 || kids.empty() || kids[0]->tag != 0x05 ||
      !is_macro_symbol(*kids[index], ctx)) {
    return false;
  }
  const std::string head = dir_name(*kids[0]);
  if ((head == "elem" || head == "random_elem" || head == "find_elem" ||
       head == "size" || head == "remove_elem" || head == "push_back" ||
       head == "resize") &&
      index == 1) {
    return true;
  }
  if (head == "foreach" && index == 2) return true;
  return false;
}

// Recurse a single (kept) node: rebuild arrays with processed children; leaves
// pass through by shared reference (immutable).
NodePtr process_node(const NodePtr& sp, Ctx& ctx, bool preserve_macro_symbol = false) {
  const Node& n = *sp;
  if (!preserve_macro_symbol && is_macro_symbol(n, ctx)) {
    return process_node(ctx.macros[dir_name(n)], ctx);
  }
  if (n.tag == 0x10 || n.tag == 0x11 || n.tag == 0x13) {
    auto nn = std::make_shared<Node>();
    nn->tag = n.tag;
    nn->line = n.line;
    if (n.tag == 0x11) {
      NodeList out;
      const NodeList& kids = gh::dtb::children(n);
      out.reserve(kids.size());
      for (std::size_t i = 0; i < kids.size(); ++i) {
        out.push_back(process_node(kids[i], ctx,
                                   command_preserves_macro_arg(kids, i, ctx)));
      }
      nn->value = std::move(out);
    } else {
      nn->value = process(gh::dtb::children(n), ctx);
    }
    return nn;
  }
  return sp;
}

NodeList process(const NodeList& in, Ctx& ctx) {
  NodeList out;
  std::vector<Cond> conds;
  auto active = [&] { return conds.empty() ? true : conds.back().branch_active; };
  auto is_defined = [&](const std::string& nm) {
    return ctx.defined.count(nm) || ctx.macros.count(nm) ||
           (ctx.opts && ctx.opts->defines.count(nm));
  };

  for (std::size_t i = 0; i < in.size(); ++i) {
    const Node& n = *in[i];
    switch (n.tag) {
      case 0x07: {  // #ifdef
        bool pa = active();
        bool t = pa && is_defined(dir_name(n));
        conds.push_back({t, t, pa});
        continue;
      }
      case 0x23: {  // #ifndef
        bool pa = active();
        bool t = pa && !is_defined(dir_name(n));
        conds.push_back({t, t, pa});
        continue;
      }
      case 0x08: {  // #else
        if (!conds.empty()) {
          Cond& c = conds.back();
          c.branch_active = c.parent_active && !c.ever_taken;
          c.ever_taken = c.ever_taken || c.branch_active;
        }
        continue;
      }
      case 0x09:  // #endif
        if (!conds.empty()) conds.pop_back();
        continue;
      case 0x20: {  // #define NAME ; body is the following sibling node
        if (active() && i + 1 < in.size()) {
          std::string nm = dir_name(n);
          ctx.defined.insert(nm);
          ctx.macros[nm] = in[i + 1];
        }
        if (i + 1 < in.size()) ++i;  // consume the body node either way
        continue;
      }
      case 0x21:    // #include
      case 0x22: {  // #merge
        if (active() && ctx.opts && ctx.opts->include_resolver) {
          NodeList inc = ctx.opts->include_resolver(dir_name(n));
          NodeList p = process(inc, ctx);
          out.insert(out.end(), p.begin(), p.end());
        }
        continue;
      }
      default:
        break;
    }

    if (!active()) continue;

    out.push_back(process_node(in[i], ctx));
  }
  return out;
}

}  // namespace

NodeList preprocess(const NodeList& roots, const PreprocessOptions& opts) {
  Ctx ctx;
  ctx.opts = &opts;
  for (const auto& d : opts.defines) ctx.defined.insert(d);
  if (opts.macro_table) {
    ctx.macros = *opts.macro_table;  // seed from prior files
    for (const auto& kv : *opts.macro_table) ctx.defined.insert(kv.first);
  }
  NodeList out = process(roots, ctx);
  if (opts.macro_table) *opts.macro_table = ctx.macros;  // accumulate back
  return out;
}

}  // namespace ghogx::script
