// engine/src/dtb_bridge/dtb_bridge.cpp

#include "dtb_bridge/dtb_bridge.h"

namespace ghogx::dtb_bridge {

// DTB tag values (gh::dtb / Harmonix "Classic" encoding). Only the few we
// disambiguate by tag are named here; the rest route through the typed helpers.
namespace tag {
constexpr uint32_t kSymbol   = 0x05;  // bareword
constexpr uint32_t kVariable = 0x02;  // $foo
// (String 0x12 and the other string-class tags fall through to the Str path.)
}  // namespace tag

DataNode from_node(const gh::dtb::Node& n) {
  if (gh::dtb::is_array(n)) {
    return DataNode::Array(from_node_list(gh::dtb::children(n)));
  }
  if (auto i = gh::dtb::as_int(n)) return DataNode::Int(*i);
  if (auto f = gh::dtb::as_float(n)) return DataNode::Float(*f);
  if (auto s = gh::dtb::as_string(n)) {
    // Barewords and variables are names -> Symbols; quoted and other
    // string-class tags are string values.
    if (n.tag == tag::kSymbol || n.tag == tag::kVariable) {
      return DataNode::Sym(Symbol(*s));
    }
    return DataNode::Str(*s);
  }
  return DataNode();  // monostate / unsupported -> empty
}

std::shared_ptr<DataArray> from_node_list(const gh::dtb::NodeList& nodes) {
  auto out = std::make_shared<DataArray>();
  for (const auto& child : nodes) {
    if (child) out->push(from_node(*child));
  }
  return out;
}

std::shared_ptr<DataArray> from_tree(const gh::dtb::Tree& tree) {
  return from_node_list(tree.root);
}

void load_property_table(const DataArray& keyed, PropertyTable& out) {
  for (std::size_t i = 0; i < keyed.size(); ++i) {
    auto entry = keyed.at(i).as_array();
    if (!entry || entry->empty()) continue;
    auto key = entry->at(0).as_symbol();
    if (!key) continue;  // not a keyed (key value...) form

    const std::size_t n = entry->size();
    if (n == 2) {
      out.set(*key, entry->at(1));  // single value
    } else if (n > 2) {
      // Several values -> store the tail as an array.
      auto tail = std::make_shared<DataArray>();
      for (std::size_t j = 1; j < n; ++j) tail->push(entry->at(j));
      out.set(*key, DataNode::Array(tail));
    } else {
      // (key) with no value -> empty marker.
      out.set(*key, DataNode());
    }
  }
}

}  // namespace ghogx::dtb_bridge
