#include "character/char_mesh.h"

#include <iostream>
#include <string>
#include <vector>

namespace {

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_int(int32_t got, int32_t want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_size(size_t got, size_t want, const char* label) {
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

bool expect_vec(const std::vector<int32_t>& got,
                const std::vector<int32_t>& want,
                const char* label) {
  if (got == want) return true;
  std::cerr << label << " vector mismatch\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::SourceCharMeshCacher;
  using ghogx::character::source_char_mesh_cache_default_state;
  using ghogx::character::source_char_mesh_cache_disable;
  using ghogx::character::source_char_mesh_cache_get_verts;
  using ghogx::character::source_char_mesh_cache_has_mesh;
  using ghogx::character::source_char_mesh_cache_stuff_meshes;
  using ghogx::character::source_char_mesh_cache_sync_mesh;

  bool ok = true;

  auto state = source_char_mesh_cache_default_state();
  ok &= expect_size(state.cache.size(), 0, "default cache size");
  ok &= expect_bool(state.disabled, false, "default disabled");

  auto disable = source_char_mesh_cache_disable(state, true);
  ok &= expect_bool(disable.accepted, true, "empty disable accepted");
  ok &= expect_bool(disable.asserted_non_empty_cache, false,
                    "empty disable assert");
  ok &= expect_bool(state.disabled, true, "state disabled");

  auto sync = source_char_mesh_cache_sync_mesh(state, "hair_front.mesh", 0x24);
  ok &= expect_bool(sync.added, true, "sync adds first mesh");
  ok &= expect_bool(sync.asserted_null_mesh, false, "sync first assert");
  ok &= expect_bool(sync.inline_cacher_body_visible, false,
                    "sync inline cacher body remains fenced");
  ok &= expect_int(sync.mask, 0x24, "sync preserves source mask argument");
  ok &= expect_size(sync.index_after_scan, 0, "sync first scan index");
  ok &= expect_size(state.cache.size(), 1, "cache after first sync");
  ok &= expect_string(state.cache[0].mesh, "hair_front.mesh",
                      "first mesh name");
  ok &= expect_int(state.cache[0].unk4, 0, "cacher unk4 default");
  ok &= expect_bool(state.cache[0].disabled, true,
                    "cacher captures disabled flag");

  ok &= expect_bool(source_char_mesh_cache_has_mesh(state, "hair_front.mesh"),
                    true, "has mesh");
  ok &= expect_bool(source_char_mesh_cache_has_mesh(state, "hair_back.mesh"),
                    false, "missing mesh");

  state.cache[0].verts = {3, 5, 8};
  auto verts = source_char_mesh_cache_get_verts(state, "hair_front.mesh");
  ok &= expect_bool(verts.found, true, "verts found");
  ok &= expect_vec(verts.verts, {3, 5, 8}, "verts payload");
  verts = source_char_mesh_cache_get_verts(state, "hair_back.mesh");
  ok &= expect_bool(verts.found, false, "missing verts");
  ok &= expect_size(verts.verts.size(), 0, "missing verts empty");

  sync = source_char_mesh_cache_sync_mesh(state, "hair_front.mesh");
  ok &= expect_bool(sync.added, true,
                    "source loop appends one-entry matching mesh");
  ok &= expect_size(sync.index_after_scan, 1,
                    "one-entry matching mesh scan index");
  ok &= expect_size(state.cache.size(), 2, "cache after source duplicate");

  sync = source_char_mesh_cache_sync_mesh(state, "hair_front.mesh");
  ok &= expect_bool(sync.added, false,
                    "two-entry first match does not append");
  ok &= expect_size(sync.index_after_scan, 1,
                    "two-entry first match scan index");
  ok &= expect_size(state.cache.size(), 2, "cache unchanged");

  SourceCharMeshCacher tail;
  tail.mesh = "hair_tail.mesh";
  state.cache.push_back(tail);
  sync = source_char_mesh_cache_sync_mesh(state, "hair_tail.mesh");
  ok &= expect_bool(sync.added, true,
                    "source loop appends when match is last entry");
  ok &= expect_size(sync.index_after_scan, 3, "last-entry scan index");

  auto meshes = source_char_mesh_cache_stuff_meshes(state);
  ok &= expect_size(meshes.size(), 4, "stuffed mesh count");
  ok &= expect_string(meshes[0], "hair_front.mesh", "stuffed first");
  ok &= expect_string(meshes[2], "hair_tail.mesh", "stuffed last source");

  disable = source_char_mesh_cache_disable(state, false);
  ok &= expect_bool(disable.accepted, false,
                    "disable asserts when cache non-empty");
  ok &= expect_bool(disable.asserted_non_empty_cache, true,
                    "disable non-empty assert flag");
  ok &= expect_bool(state.disabled, true,
                    "failed disable preserves state flag");

  auto null_state = source_char_mesh_cache_default_state();
  sync = source_char_mesh_cache_sync_mesh(null_state, "");
  ok &= expect_bool(sync.added, false, "null mesh not added");
  ok &= expect_bool(sync.asserted_null_mesh, true, "null mesh assert");
  ok &= expect_size(null_state.cache.size(), 0, "null mesh leaves cache empty");

  return ok ? 0 : 1;
}
