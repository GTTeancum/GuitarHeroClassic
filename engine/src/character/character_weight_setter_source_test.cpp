#include "character/char_clip.h"

#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>

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

}  // namespace

int main() {
  using ghogx::character::Character;
  using ghogx::character::CharWeightSetter;
  using ghogx::character::apply_character_controllers;
  using ghogx::character::source_char_weight_setter_poll;
  using ghogx::character::source_char_weightable_weight;

  bool ok = true;

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

  return ok ? 0 : 1;
}
