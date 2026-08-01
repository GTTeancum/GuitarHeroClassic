// engine/src/milo_bridge/milo_bridge.h
//
// milo_bridge — load a PS2 MILO container's object directory into the runtime
// ObjectDir tree.
//
// A MILO's decompressed payload is an object directory: a dir_version, a
// dir_type ("ObjectDir" / "PanelDir" / "WorldDir" / ...), a dir_name, then a
// flat list of (type-string, name-string) entries that are the dir's direct
// children. This bridge performs the STRUCTURAL load: build an ObjectDir,
// stamp its name + dir_type, and create one child Object per entry. Children
// whose class has a registered ClassReg creator are instantiated as that class;
// the rest become MiloObject placeholders that still carry the real class +
// name, so the tree is faithful before per-class binary bodies (Tex / Mesh /
// Mat / Label / ...) are decoded. See memory/subsystems/milo_format.md.
//
// Reimplemented fresh from the decoded format; not derived from reference code.

#pragma once

#include "core/object.h"
#include "core/object_dir.h"
#include "core/symbol.h"

#include <memory>
#include <string>

namespace gh::milo {
struct Directory;  // tools/milo/milo.h
}

namespace ghogx::milo_bridge {

// Structural placeholder for a MILO entry whose class has no registered runtime
// creator yet. Preserves the entry's class Symbol + name so the object tree is
// faithful before the per-class body readers exist.
class MiloObject : public Object {
 public:
  explicit MiloObject(Symbol cls) : class_(cls) {}
  Symbol class_name() const override { return class_; }

 private:
  Symbol class_;
};

// Build an ObjectDir from an already-parsed MILO directory. Pure (no I/O) so it
// is hermetically testable from a hand-built Directory. Sets the dir's name
// (dir_name) and dir_type; for each entry, instantiates a child via the
// ClassReg factory when `type` is creatable, else a MiloObject placeholder.
// Child insertion order matches the MILO entry order.
std::unique_ptr<ObjectDir> object_dir_from_directory(const gh::milo::Directory& dir);

// Load a MILO from a PS2 ARK (hdr/ark) and build its ObjectDir: ARK lookup +
// read + inflate + parse_directory + object_dir_from_directory. Mirrors the
// catalog tool's "../../system/run/" path fallback. Returns nullptr on any
// failure (reason logged to stderr).
std::unique_ptr<ObjectDir> load_object_dir(const std::string& hdr_path,
                                           const std::string& ark_path,
                                           const std::string& milo_path);

}  // namespace ghogx::milo_bridge
