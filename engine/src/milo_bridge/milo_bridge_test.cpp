// engine/src/milo_bridge/milo_bridge_test.cpp
//
// Unit test for the MILO-directory -> ObjectDir structural loader. Hermetic:
// builds gh::milo::Directory structs by hand (no ARK / asset files) and checks
// the resulting object tree -- dir metadata, child count + order, placeholder
// vs registered-class instantiation, and name lookup.

#include "milo.h"
#include "milo_bridge/milo_bridge.h"

#include "core/class_reg.h"
#include "core/object.h"
#include "core/object_dir.h"
#include "core/symbol.h"

#include <cstdio>
#include <memory>
#include <utility>
#include <vector>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      std::fprintf(stderr, "FAIL %s:%d  CHECK(%s)\n", __FILE__, __LINE__, \
                   #cond);                                                \
      ++g_failures;                                                       \
    }                                                                     \
  } while (0)

using namespace ghogx;

// Build a synthetic MILO directory from (type, name) pairs.
gh::milo::Directory make_dir(
    const char* dir_type, const char* dir_name,
    std::vector<std::pair<const char*, const char*>> entries) {
  gh::milo::Directory d;
  d.dir_version = 24;  // GH2
  d.dir_type = dir_type;
  d.dir_name = dir_name;
  for (const auto& [type, name] : entries) {
    gh::milo::Entry e;
    e.type = type;
    e.name = name;
    d.entries.push_back(std::move(e));
  }
  return d;
}

// A real registered class, to prove the factory path is taken when a creator
// exists (vs. the MiloObject placeholder otherwise).
class GroupStub : public Object {
 public:
  Symbol class_name() const override { return Symbol("Group"); }
};

// Every entry's class is unregistered -> all children are placeholders that
// still carry the true class + name, and dir metadata is stamped through.
void test_all_placeholders() {
  ClassReg::instance().clear();  // ensure none of these types are creatable

  auto dir = make_dir("PanelDir", "splash",
                      {{"Tex", "background.tex"},
                       {"Mat", "logo.mat"},
                       {"Label", "title.lbl"}});
  auto od = milo_bridge::object_dir_from_directory(dir);
  CHECK(od != nullptr);

  CHECK(od->name() == Symbol("splash"));
  CHECK(od->dir_type() == Symbol("PanelDir"));
  CHECK(od->class_name() == Symbol("ObjectDir"));
  CHECK(od->size() == 3);

  // Lookup by name + class fidelity.
  Object* tex = od->find(Symbol("background.tex"));
  Object* mat = od->find(Symbol("logo.mat"));
  Object* lbl = od->find(Symbol("title.lbl"));
  CHECK(tex && mat && lbl);
  CHECK(tex->class_name() == Symbol("Tex"));
  CHECK(mat->class_name() == Symbol("Mat"));
  CHECK(lbl->class_name() == Symbol("Label"));

  // Unregistered classes -> MiloObject placeholders.
  CHECK(dynamic_cast<milo_bridge::MiloObject*>(tex) != nullptr);
  CHECK(dynamic_cast<milo_bridge::MiloObject*>(lbl) != nullptr);

  // Insertion order matches the MILO entry order.
  CHECK(od->at(0) == tex);
  CHECK(od->at(1) == mat);
  CHECK(od->at(2) == lbl);
}

// A registered class is instantiated for real; siblings without a creator fall
// back to placeholders -- mixed in one directory.
void test_registered_class_instantiated() {
  ClassReg& reg = ClassReg::instance();
  reg.clear();
  reg.define(Symbol("Group"));
  reg.set_creator(Symbol("Group"), [] { return std::make_unique<GroupStub>(); });

  auto dir = make_dir("ObjectDir", "scene",
                      {{"Group", "buttons.grp"}, {"Tex", "art.tex"}});
  auto od = milo_bridge::object_dir_from_directory(dir);
  CHECK(od->size() == 2);

  Object* grp = od->find(Symbol("buttons.grp"));
  Object* tex = od->find(Symbol("art.tex"));
  CHECK(grp && tex);

  // The registered class is a real GroupStub, NOT a placeholder; it is named.
  CHECK(dynamic_cast<GroupStub*>(grp) != nullptr);
  CHECK(dynamic_cast<milo_bridge::MiloObject*>(grp) == nullptr);
  CHECK(grp->name() == Symbol("buttons.grp"));

  // The unregistered sibling is a placeholder.
  CHECK(dynamic_cast<milo_bridge::MiloObject*>(tex) != nullptr);
  CHECK(tex->class_name() == Symbol("Tex"));
}

// An empty directory still produces a named, typed ObjectDir with no children.
void test_empty_directory() {
  ClassReg::instance().clear();
  auto dir = make_dir("WorldDir", "empty", {});
  auto od = milo_bridge::object_dir_from_directory(dir);
  CHECK(od != nullptr);
  CHECK(od->name() == Symbol("empty"));
  CHECK(od->dir_type() == Symbol("WorldDir"));
  CHECK(od->size() == 0);
}

// Duplicate entry names: ObjectDir::add replaces, so the last one wins and the
// child count reflects the dedup (mirrors the container's name-collision rule).
void test_duplicate_names_replace() {
  ClassReg::instance().clear();
  auto dir = make_dir("ObjectDir", "dups",
                      {{"Tex", "shared"}, {"Mat", "shared"}});
  auto od = milo_bridge::object_dir_from_directory(dir);
  CHECK(od->size() == 1);
  CHECK(od->find(Symbol("shared"))->class_name() == Symbol("Mat"));  // last wins
}

}  // namespace

int main() {
  test_all_placeholders();
  test_registered_class_instantiated();
  test_empty_directory();
  test_duplicate_names_replace();

  if (g_failures == 0) {
    std::printf("ghogx_milo_bridge_test: all checks passed\n");
    return 0;
  }
  std::fprintf(stderr, "ghogx_milo_bridge_test: %d check(s) failed\n", g_failures);
  return 1;
}
