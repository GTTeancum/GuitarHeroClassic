// engine/src/dtb_bridge/dtb_bridge.h
//
// Bridge: PS2 DTB parse tree (gh::dtb, from tools/dtb) -> runtime value model
// (ghogx::DataNode / DataArray / PropertyTable).
//
// tools/dtb already parses the compiled PS2 DTA (DTB) into a gh::dtb::Tree of
// tagged Nodes. The runtime engine consumes its own DataNode/DataArray, so this
// module is the one-way translation that lets PS2 config data drive the engine.
// PS2 assets only -- never the 360 format (the readers under tools/ are all
// PS2-native by design).

#pragma once

#include "core/data_node.h"
#include "core/property_table.h"

#include "dtb.h"  // gh::dtb (tools/dtb)

#include <memory>

namespace ghogx::dtb_bridge {

// Translate one DTB node into a runtime DataNode. Arrays recurse. Symbols and
// variables become Symbol values; quoted/other strings become string values;
// ints and floats map directly.
DataNode from_node(const gh::dtb::Node& n);

// Wrap a DTB node-list (array children, or a Tree's roots) into a DataArray.
std::shared_ptr<DataArray> from_node_list(const gh::dtb::NodeList& nodes);

// Wrap a whole parsed Tree's top level into a DataArray.
std::shared_ptr<DataArray> from_tree(const gh::dtb::Tree& tree);

// Populate `out` from a keyed DataArray, the Harmonix `(key value...)` config
// convention. For each child array whose first element is a Symbol:
//   - exactly one trailing value  -> that value
//   - several trailing values     -> a DataArray of them
// Non-keyed children are skipped. Existing entries are overwritten.
void load_property_table(const DataArray& keyed, PropertyTable& out);

}  // namespace ghogx::dtb_bridge
