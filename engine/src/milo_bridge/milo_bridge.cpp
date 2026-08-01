// engine/src/milo_bridge/milo_bridge.cpp

#include "milo_bridge/milo_bridge.h"

#include "ark_v3.h"
#include "milo.h"

#include <cstdio>
#include <optional>
#include <stdexcept>

namespace ghogx::milo_bridge {

namespace {

// Some ARK entries are reached only via the "../../system/run/" prefix the
// runtime uses; mirror the catalog/asset tools' fallback.
std::optional<gh::ark::Entry> find_entry(const gh::ark::ArkV3Reader& ark,
                                         const std::string& path) {
  auto e = ark.find(path);
  if (!e) e = ark.find("../../system/run/" + path);
  return e;
}

}  // namespace

std::unique_ptr<ObjectDir> object_dir_from_directory(const gh::milo::Directory& dir) {
  auto od = std::make_unique<ObjectDir>();
  od->set_name(Symbol(dir.dir_name));
  od->set_dir_type(Symbol(dir.dir_type));

  for (const auto& e : dir.entries) {
    const Symbol type(e.type);
    const Symbol name(e.name);
    // A registered class is instantiated for real (binds its prototype
    // defaults); everything else gets a structural placeholder that still
    // carries the true class + name.
    if (!od->create_child(type, name)) {
      auto placeholder = std::make_unique<MiloObject>(type);
      placeholder->set_name(name);
      od->add(std::move(placeholder));
    }
  }
  return od;
}

std::unique_ptr<ObjectDir> load_object_dir(const std::string& hdr_path,
                                           const std::string& ark_path,
                                           const std::string& milo_path) {
  try {
    auto ark = gh::ark::ArkV3Reader::load(hdr_path);
    auto entry = find_entry(ark, milo_path);
    if (!entry) {
      std::fprintf(stderr, "[milo_bridge] milo not found in ARK: %s\n",
                   milo_path.c_str());
      return nullptr;
    }

    auto bytes = ark.read_entry(*entry, {ark_path});
    auto header = gh::milo::parse_header(bytes);
    auto payload = gh::milo::inflate_payload(bytes, header);
    auto dir = gh::milo::parse_directory(payload);

    auto od = object_dir_from_directory(dir);
    std::fprintf(stderr,
                 "[milo_bridge] %s -> dir_type=%s name=%s, %zu children\n",
                 milo_path.c_str(), dir.dir_type.c_str(), dir.dir_name.c_str(),
                 od->size());
    return od;
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "[milo_bridge] load_object_dir(%s): %s\n",
                 milo_path.c_str(), ex.what());
    return nullptr;
  }
}

}  // namespace ghogx::milo_bridge
