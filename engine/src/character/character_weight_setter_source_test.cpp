#include "character/char_clip.h"

#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

ghogx::character::CharWeightSetter make_setter(const std::string& name) {
  ghogx::character::CharWeightSetter setter;
  setter.name = name;
  setter.weight = 0.25f;
  setter.weight_owner = name;
  setter.base_weight = 0.75f;
  return setter;
}

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_bool(bool got, bool want, const char* label) {
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
  std::cerr << label << " got '" << got << "' want '" << want << "'\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::Character;
  using ghogx::character::CharWeightSetter;
  using ghogx::character::SourceCharWeightableState;
  using ghogx::character::SourceCharWeightSetterPollDeps;
  using ghogx::character::SourceCharWeightSetterRefOwner;
  using ghogx::character::SourceCharWeightSetterState;
  using ghogx::character::apply_character_controllers;
  using ghogx::character::source_char_weightable_copy;
  using ghogx::character::source_char_weightable_default_state;
  using ghogx::character::source_char_weightable_replace;
  using ghogx::character::source_char_weightable_set_weight;
  using ghogx::character::source_char_weightable_set_weight_owner;
  using ghogx::character::source_char_weight_setter_poll;
  using ghogx::character::source_char_weight_setter_poll_deps;
  using ghogx::character::source_char_weight_setter_default_state;
  using ghogx::character::source_char_weight_setter_copy_plan;
  using ghogx::character::source_char_weight_setter_handler_plan;
  using ghogx::character::source_char_weight_setter_load_plan;
  using ghogx::character::source_char_weight_setter_prop_sync_plan;
  using ghogx::character::source_char_weightable_copy_plan;
  using ghogx::character::source_char_weightable_handler_plan;
  using ghogx::character::source_char_weightable_load_plan;
  using ghogx::character::source_char_weightable_prop_sync_plan;
  using ghogx::character::source_char_weight_setter_set_weight;
  using ghogx::character::source_char_weightable_weight;

  bool ok = true;

  const auto weightable_load_v1 = source_char_weightable_load_plan(1);
  ok &= expect_bool(weightable_load_v1.revision_supported, true,
                    "weightable v1 load supported");
  ok &= expect_size(weightable_load_v1.read_order.size(), 1,
                    "weightable v1 read count");
  ok &= expect_string(weightable_load_v1.read_order[0], "mWeight",
                      "weightable v1 reads weight");
  const auto weightable_load_v2 = source_char_weightable_load_plan(2);
  ok &= expect_size(weightable_load_v2.read_order.size(), 2,
                    "weightable v2 read count");
  ok &= expect_string(weightable_load_v2.read_order[1], "mWeightOwner",
                      "weightable v2 reads owner");
  ok &= expect_bool(source_char_weightable_load_plan(3).revision_supported,
                    false, "weightable rejects high revision");
  const auto weightable_copy = source_char_weightable_copy_plan();
  ok &= expect_string(weightable_copy.shallow_actions[0],
                      "SetWeightOwner(source.mWeightOwner)",
                      "weightable shallow copy owner action");
  ok &= expect_string(weightable_copy.deep_actions[1],
                      "mWeight=source.mWeightOwner->mWeight",
                      "weightable deep copy weight action");
  const auto weightable_handler = source_char_weightable_handler_plan();
  ok &= expect_size(weightable_handler.superclasses.size(), 1,
                    "weightable handler superclass count");
  ok &= expect_string(weightable_handler.superclasses[0], "Hmx::Object",
                      "weightable handler superclass");
  ok &= expect_bool(weightable_handler.check == 0x43, true,
                    "weightable handler check");
  const auto weightable_props = source_char_weightable_prop_sync_plan();
  ok &= expect_size(weightable_props.properties.size(), 2,
                    "weightable prop count");
  ok &= expect_string(weightable_props.properties[0], "weight",
                      "weightable prop weight");
  ok &= expect_string(weightable_props.properties[1], "weight_owner",
                      "weightable prop owner");
  ok &= expect_string(weightable_props.set_actions[0],
                      "weight:SetWeight(_val.Float(0))",
                      "weightable prop SetWeight action");
  ok &= expect_string(weightable_props.set_actions[1],
                      "weight_owner:SetWeightOwner(_val.Obj<CharWeightable>(0))",
                      "weightable prop SetWeightOwner action");
  ok &= expect_string(weightable_props.get_actions[0],
                      "weight:DataNode(mWeight)",
                      "weightable prop weight get");
  ok &= expect_string(weightable_props.blocked_ops[1],
                      "weight_owner:op0x40 returns false",
                      "weightable prop owner blocked op");

  SourceCharWeightableState weightable =
      source_char_weightable_default_state("self.weight");
  ok &= near(weightable.weight, 1.0f, "weightable default weight");
  if (weightable.weight_owner != "self.weight") {
    std::cerr << "weightable default owner mismatch\n";
    ok = false;
  }
  source_char_weightable_set_weight(weightable, 0.33f);
  ok &= near(weightable.weight, 0.33f, "weightable SetWeight");
  source_char_weightable_set_weight_owner(weightable, "owner.weight");
  if (weightable.weight_owner != "owner.weight") {
    std::cerr << "weightable SetWeightOwner mismatch\n";
    ok = false;
  }
  source_char_weightable_set_weight_owner(weightable, "");
  if (weightable.weight_owner != "self.weight") {
    std::cerr << "weightable SetWeightOwner null fallback mismatch\n";
    ok = false;
  }
  source_char_weightable_set_weight_owner(weightable, "old.owner");
  source_char_weightable_replace(weightable, "old.owner", "new.owner", true);
  if (weightable.weight_owner != "new.owner") {
    std::cerr << "weightable Replace owner mismatch\n";
    ok = false;
  }
  source_char_weightable_replace(weightable, "new.owner", "not.weightable",
                                 false);
  if (weightable.weight_owner != "self.weight") {
    std::cerr << "weightable Replace null fallback mismatch\n";
    ok = false;
  }
  SourceCharWeightableState source =
      source_char_weightable_default_state("source.weight");
  source_char_weightable_set_weight_owner(source, "source.owner");
  SourceCharWeightableState dest =
      source_char_weightable_default_state("dest.weight");
  source_char_weightable_set_weight(dest, 0.20f);
  source_char_weightable_copy(dest, source, true, 0.90f);
  if (dest.weight_owner != "source.owner" ||
      !near(dest.weight, 0.20f, "weightable shallow copy")) {
    std::cerr << "weightable shallow copy mismatch\n";
    ok = false;
  }
  source_char_weightable_copy(dest, source, false, 0.90f);
  if (dest.weight_owner != "dest.weight" ||
      !near(dest.weight, 0.90f, "weightable deep copy")) {
    std::cerr << "weightable deep copy mismatch\n";
    ok = false;
  }

  SourceCharWeightSetterState setter_state =
      source_char_weight_setter_default_state("setter.weight");
  if (setter_state.weightable.name != "setter.weight" ||
      !near(setter_state.weightable.weight, 1.0f,
            "weight setter inherited weight") ||
      setter_state.weightable.weight_owner != "setter.weight" ||
      setter_state.has_base || setter_state.has_driver ||
      setter_state.min_weight_count != 0 || setter_state.max_weight_count != 0 ||
      setter_state.flags != 0 || !near(setter_state.offset, 0.0f,
                                       "weight setter default offset") ||
      !near(setter_state.scale, 1.0f, "weight setter default scale") ||
      !near(setter_state.base_weight, 0.0f,
            "weight setter default base weight") ||
      !near(setter_state.beats_per_weight, 0.0f,
            "weight setter default smoothing")) {
    std::cerr << "weight setter default state mismatch\n";
    ok = false;
  }
  source_char_weight_setter_set_weight(setter_state, 0.42f);
  if (!near(setter_state.base_weight, 0.42f, "weight setter SetWeight base") ||
      !near(setter_state.weightable.weight, 0.42f,
            "weight setter SetWeight inherited")) {
    std::cerr << "weight setter SetWeight mismatch\n";
    ok = false;
  }

  const auto setter_load_v0 = source_char_weight_setter_load_plan(0);
  ok &= expect_bool(setter_load_v0.revision_supported, true,
                    "weight setter v0 load supported");
  ok &= expect_string(setter_load_v0.read_order[0], "Hmx::Object",
                      "weight setter v0 loads object first");
  ok &= expect_string(setter_load_v0.read_order[1], "mDriver",
                      "weight setter v0 skips weightable");
  ok &= expect_string(setter_load_v0.read_order[3],
                      "legacyWeightableOwnerList",
                      "weight setter v0 reads owner list");
  ok &= expect_string(setter_load_v0.branches[0], "mScale=1.0",
                      "weight setter v0 default scale");
  ok &= expect_string(setter_load_v0.branches[3], "mBaseWeight=mWeight",
                      "weight setter v0 base weight branch");

  const auto setter_load_v3 = source_char_weight_setter_load_plan(3);
  ok &= expect_string(setter_load_v3.read_order[1], "CharWeightable",
                      "weight setter v3 reads weightable");
  ok &= expect_string(setter_load_v3.read_order[4], "legacyInvertBool",
                      "weight setter v3 reads invert bool");
  ok &= expect_string(setter_load_v3.branches[0],
                      "legacy bool true -> mScale=-1.0,mOffset=1.0",
                      "weight setter v3 true branch");

  const auto setter_load_v9 = source_char_weight_setter_load_plan(9);
  ok &= expect_string(setter_load_v9.read_order[4], "mOffset",
                      "weight setter v9 reads offset");
  ok &= expect_string(setter_load_v9.read_order[5], "mScale",
                      "weight setter v9 reads scale");
  ok &= expect_string(setter_load_v9.read_order[8], "mBase",
                      "weight setter v9 reads base");
  ok &= expect_string(setter_load_v9.read_order[9], "mMinWeights",
                      "weight setter v9 reads min list");
  ok &= expect_string(setter_load_v9.read_order[10], "mMaxWeights",
                      "weight setter v9 reads max list");
  ok &= expect_bool(source_char_weight_setter_load_plan(10).revision_supported,
                    false, "weight setter rejects high revision");

  const auto setter_load_v8 = source_char_weight_setter_load_plan(8);
  ok &= expect_string(setter_load_v8.read_order[9], "legacyMinWeight",
                      "weight setter v8 reads legacy min");
  ok &= expect_string(setter_load_v8.read_order[10], "legacyMaxWeight",
                      "weight setter v8 reads legacy max");
  const auto setter_copy = source_char_weight_setter_copy_plan();
  ok &= expect_string(setter_copy.copied_superclasses[0], "Hmx::Object",
                      "weight setter copy object superclass");
  ok &= expect_string(setter_copy.copied_superclasses[1], "CharWeightable",
                      "weight setter copy weightable superclass");
  ok &= expect_string(setter_copy.copied_members[0], "mDriver",
                      "weight setter copy driver");
  ok &= expect_string(setter_copy.copied_members[8], "mMaxWeights",
                      "weight setter copy max weights");
  const auto setter_handlers = source_char_weight_setter_handler_plan();
  ok &= expect_size(setter_handlers.superclasses.size(), 1,
                    "weight setter handler superclass count");
  ok &= expect_string(setter_handlers.superclasses[0], "Hmx::Object",
                      "weight setter handler superclass");
  ok &= expect_bool(setter_handlers.check == 0xF4, true,
                    "weight setter handler check");
  const auto setter_props = source_char_weight_setter_prop_sync_plan();
  ok &= expect_size(setter_props.properties.size(), 9,
                    "weight setter prop count");
  ok &= expect_string(setter_props.properties[0], "driver",
                      "weight setter prop driver");
  ok &= expect_string(setter_props.properties[5], "base_weight",
                      "weight setter prop base weight");
  ok &= expect_string(setter_props.properties.back(), "max_weights",
                      "weight setter prop max weights");
  ok &= expect_string(setter_props.superclasses[0], "CharWeightable",
                      "weight setter prop superclass");

  std::unordered_map<std::string, float> weights;
  CharWeightSetter setter = make_setter("owned.weight");
  weights["owned.weight"] = 0.40f;
  ok &= near(source_char_weightable_weight(setter, weights), 0.40f,
             "owner weight");

  float out = 0.0f;
  ok &= source_char_weight_setter_poll(setter, weights, 0.0f, out);
  ok &= near(out, 0.75f, "snap base weight");

  CharWeightSetter base = make_setter("base.weight");
  base.base = "driverless.base";
  base.offset = 0.10f;
  base.scale = 0.50f;
  weights["driverless.base"] = 0.80f;
  ok &= source_char_weight_setter_poll(base, weights, 0.0f, out);
  ok &= near(out, 0.50f, "base scale offset");

  CharWeightSetter limited = make_setter("limited.weight");
  limited.base_weight = 0.90f;
  limited.min_weights.push_back("min.weight");
  limited.max_weights.push_back("max.weight");
  weights["min.weight"] = 0.60f;
  weights["max.weight"] = 0.70f;
  ok &= source_char_weight_setter_poll(limited, weights, 0.0f, out);
  ok &= near(out, 0.70f, "min then max");

  CharWeightSetter smooth = make_setter("smooth.weight");
  smooth.weight = 0.20f;
  smooth.weight_owner = "smooth.weight";
  smooth.base_weight = 0.80f;
  smooth.beats_per_weight = 2.0f;
  weights["smooth.weight"] = 0.20f;
  ok &= source_char_weight_setter_poll(smooth, weights, 0.5f, out);
  ok &= near(out, 0.45f, "beat smoothing");

  CharWeightSetter driver = make_setter("driver.weight");
  driver.driver = "main.drv";
  ok &= !source_char_weight_setter_poll(driver, weights, 0.0f, out);

  Character character;
  character.weight_setters.push_back(make_setter("live.weight"));
  apply_character_controllers(character, 0.0f, nullptr);
  const auto runtime = character.runtime_weight_props.find("live.weight");
  ok &= runtime != character.runtime_weight_props.end();
  if (runtime != character.runtime_weight_props.end()) {
    ok &= near(runtime->second, 0.75f, "controller writeback");
  }

  CharWeightSetter deps_setter = make_setter("deps.weight");
  deps_setter.driver = "main.driver";
  deps_setter.base = "base.weight";
  deps_setter.min_weights = {"min.a", "min.b"};
  deps_setter.max_weights = {"max.a"};
  SourceCharWeightSetterPollDeps deps;
  source_char_weight_setter_poll_deps(
      deps, deps_setter,
      {SourceCharWeightSetterRefOwner{"ignored.owner", false},
       SourceCharWeightSetterRefOwner{"first.change", true},
       SourceCharWeightSetterRefOwner{"last.change", true}});
  const std::vector<std::string> want_changed_by = {
      "main.driver", "base.weight", "min.a", "min.b", "max.a"};
  const std::vector<std::string> want_change = {"last.change", "first.change"};
  if (deps.changed_by != want_changed_by) {
    std::cerr << "PollDeps changed_by order mismatch\n";
    ok = false;
  }
  if (deps.change != want_change) {
    std::cerr << "PollDeps change reverse-ref order mismatch\n";
    ok = false;
  }

  return ok ? 0 : 1;
}
