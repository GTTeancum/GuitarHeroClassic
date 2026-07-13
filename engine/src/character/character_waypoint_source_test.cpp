#include "character/char_mesh.h"

#include <array>
#include <cmath>
#include <iostream>
#include <string>

namespace {

ghogx::milo_scene::Xfm identity(float x = 0.0f, float y = 0.0f,
                                float z = 0.0f) {
  ghogx::milo_scene::Xfm out;
  out.pos[0] = x;
  out.pos[1] = y;
  out.pos[2] = z;
  return out;
}

ghogx::milo_scene::Xfm z_rot(float angle, float x = 0.0f, float y = 0.0f,
                             float z = 0.0f) {
  ghogx::milo_scene::Xfm out = identity(x, y, z);
  const float ca = std::cos(angle);
  const float sa = std::sin(angle);
  out.rot[0][0] = ca;
  out.rot[0][1] = sa;
  out.rot[1][0] = -sa;
  out.rot[1][1] = ca;
  return out;
}

bool near(float got, float want, const char* label) {
  if (std::fabs(got - want) <= 0.0001f) return true;
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

bool expect_bool(bool got, bool want, const char* label) {
  if (got == want) return true;
  std::cerr << label << " got " << (got ? "true" : "false")
            << " want " << (want ? "true" : "false") << "\n";
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

bool vec_near(const std::array<float, 3>& got,
              const std::array<float, 3>& want,
              const char* label) {
  bool ok = true;
  ok &= near(got[0], want[0], label);
  ok &= near(got[1], want[1], label);
  ok &= near(got[2], want[2], label);
  return ok;
}

}  // namespace

int main() {
  using ghogx::character::SourceWaypointState;
  using ghogx::character::source_waypoint_constrain;
  using ghogx::character::source_waypoint_construct;
  using ghogx::character::source_waypoint_copy_plan;
  using ghogx::character::source_waypoint_default_state;
  using ghogx::character::source_waypoint_find_by_flags;
  using ghogx::character::source_waypoint_handler_plan;
  using ghogx::character::source_waypoint_init_registry;
  using ghogx::character::source_waypoint_load_plan;
  using ghogx::character::source_waypoint_load_revision_known;
  using ghogx::character::source_waypoint_prop_sync_plan;
  using ghogx::character::source_waypoint_save_plan;
  using ghogx::character::source_waypoint_shape_delta_ang;
  using ghogx::character::source_waypoint_shape_delta_box;
  using ghogx::character::source_waypoint_terminate_registry;

  constexpr float kPi = 3.14159265358979323846f;
  bool ok = true;

  const auto defaults = source_waypoint_default_state();
  ok &= near(defaults.radius, 12.0f, "Waypoint default radius");
  ok &= near(defaults.y_radius, 0.0f, "Waypoint default y radius");
  ok &= near(defaults.ang_radius, 0.0f, "Waypoint default angle radius");

  auto registry = source_waypoint_init_registry();
  ok &= expect_bool(registry.allocated, true, "Waypoint registry allocated");
  ok &= expect_size(registry.registered_functions.size(), 3,
                    "Waypoint registered function count");
  ok &= expect_string(registry.registered_functions[0], "waypoint_find",
                      "Waypoint registers find function");
  ok &= expect_string(registry.registered_functions[2], "waypoint_last",
                      "Waypoint registers last function");
  ok &= expect_bool(registry.exit_callback_registered, true,
                    "Waypoint registers terminate callback");
  const auto constructed = source_waypoint_construct(registry);
  ok &= expect_bool(constructed.registry_push, true,
                    "Waypoint constructor pushes into registry");
  ok &= expect_bool(constructed.random_branch_is_noop, true,
                    "Waypoint constructor random branch is source no-op");
  ok &= expect_size(constructed.registry_size, 1,
                    "Waypoint constructor registry size");
  ok &= near(constructed.waypoint.radius, 12.0f,
             "Waypoint constructor default radius");
  registry.waypoints.push_back(defaults);
  registry.waypoints.push_back(defaults);
  registry.waypoints[0].flags = 0x02;
  registry.waypoints[1].flags = 0x04;
  registry.waypoints[2].flags = 0x08;
  auto found = source_waypoint_find_by_flags(registry, 0x0C);
  ok &= expect_bool(found.found, true, "Waypoint find flags found");
  ok &= near(static_cast<float>(found.index), 1.0f,
             "Waypoint find flags first match");
  ok &= near(static_cast<float>(found.mask), 12.0f,
             "Waypoint find records mask");
  found = source_waypoint_find_by_flags(registry, 0x10);
  ok &= expect_bool(found.found, false, "Waypoint find flags missing");
  ok &= near(static_cast<float>(found.index), -1.0f,
             "Waypoint find missing index");
  source_waypoint_terminate_registry(registry);
  ok &= expect_bool(registry.allocated, false,
                    "Waypoint terminate clears allocation");
  ok &= expect_size(registry.waypoints.size(), 0,
                    "Waypoint terminate clears waypoints");

  ok &= expect_bool(source_waypoint_load_revision_known(-1), false,
                    "Waypoint revision -1 rejected");
  ok &= expect_bool(source_waypoint_load_revision_known(0), true,
                    "Waypoint revision 0 accepted");
  ok &= expect_bool(source_waypoint_load_revision_known(5), true,
                    "Waypoint revision 5 accepted");
  ok &= expect_bool(source_waypoint_load_revision_known(6), false,
                    "Waypoint revision 6 rejected");

  auto load = source_waypoint_load_plan(1);
  ok &= expect_bool(load.known_revision, true, "Waypoint load rev1 known");
  ok &= expect_size(load.read_order.size(), 4, "Waypoint load rev1 rows");
  ok &= expect_string(load.read_order[0], "Hmx::Object",
                      "Waypoint load starts object");
  ok &= expect_string(load.read_order[1], "RndTransformable",
                      "Waypoint load transform");
  ok &= expect_size(load.revision_branches.size(), 2,
                    "Waypoint load rev1 branches");
  ok &= expect_string(load.revision_branches[1], "default mRadius=12",
                      "Waypoint load rev1 default radius");
  load = source_waypoint_load_plan(5);
  ok &= expect_size(load.read_order.size(), 9, "Waypoint load rev5 rows");
  ok &= expect_string(load.read_order[8], "mStrictAngDelta",
                      "Waypoint load rev5 strict angle");
  ok &= expect_size(load.revision_branches.size(), 0,
                    "Waypoint load rev5 no legacy branches");

  const auto copy = source_waypoint_copy_plan();
  ok &= expect_size(copy.copied_superclasses.size(), 2,
                    "Waypoint copy superclass count");
  ok &= expect_string(copy.copied_superclasses[1], "RndTransformable",
                      "Waypoint copy transform superclass");
  ok &= expect_string(copy.copied_members[0], "mFlags",
                      "Waypoint copy first member");
  ok &= expect_string(copy.copied_members[6], "mStrictAngDelta",
                      "Waypoint copy strict angle member");

  const auto handlers = source_waypoint_handler_plan();
  ok &= expect_size(handlers.superclasses.size(), 2,
                    "Waypoint handler superclass count");
  ok &= expect_string(handlers.superclasses[0], "RndTransformable",
                      "Waypoint handler transform superclass");
  ok &= expect_string(handlers.superclasses[1], "Hmx::Object",
                      "Waypoint handler object superclass");
  ok &= near(static_cast<float>(handlers.check), 524.0f,
             "Waypoint handler check");
  ok &= expect_bool(source_waypoint_save_plan().save_id == 460, true,
                    "Waypoint save id");

  const auto props = source_waypoint_prop_sync_plan();
  ok &= expect_size(props.properties.size(), 5,
                    "Waypoint direct prop count");
  ok &= expect_string(props.properties[0], "flags", "Waypoint prop flags");
  ok &= expect_string(props.properties[4], "connections",
                      "Waypoint prop connections");
  ok &= expect_size(props.set_properties.size(), 2,
                    "Waypoint set prop count");
  ok &= expect_string(props.set_properties[0], "ang_radius",
                      "Waypoint set prop angle radius");
  ok &= expect_string(props.superclasses[0], "RndTransformable",
                      "Waypoint prop superclass");

  ok &= vec_near(source_waypoint_shape_delta_box(
                     identity(), {15.0f, 0.0f, 7.0f}, 10.0f, 0.0f),
                 {-5.0f, 0.0f, 0.0f},
                 "Waypoint circular branch pulls to radius");
  ok &= vec_near(source_waypoint_shape_delta_box(
                     identity(), {4.0f, 3.0f, 1.0f}, 10.0f, 0.0f),
                 {0.0f, 0.0f, 0.0f},
                 "Waypoint circular branch leaves in-range point");
  ok &= vec_near(source_waypoint_shape_delta_box(
                     identity(), {8.0f, -5.0f, 7.0f}, 5.0f, 2.0f),
                 {-3.0f, 3.0f, 0.0f},
                 "Waypoint box branch clamps local x/y only");

  ok &= near(source_waypoint_shape_delta_ang(kPi * 0.5f, kPi / 6.0f, 0.0f),
             kPi / 3.0f, "Waypoint angle delta clamps around waypoint");
  ok &= near(source_waypoint_shape_delta_ang(0.0f, kPi / 6.0f, kPi * 0.5f),
             -kPi / 3.0f, "Waypoint angle delta preserves sign");

  SourceWaypointState waypoint;
  waypoint.radius = 10.0f;
  waypoint.y_radius = 0.0f;
  waypoint.ang_radius = kPi / 6.0f;
  waypoint.strict_radius_delta = 2.0f;
  waypoint.strict_ang_delta = kPi / 6.0f;
  const auto constrained =
      source_waypoint_constrain(waypoint, z_rot(kPi * 0.5f),
                                identity(20.0f, 0.0f, 0.0f));
  ok &= expect_bool(constrained.applied_radius, true,
                    "Waypoint constrain applies strict radius");
  ok &= expect_bool(constrained.applied_angle, true,
                    "Waypoint constrain applies strict angle");
  ok &= vec_near(constrained.position_delta, {-8.0f, 0.0f, 0.0f},
                 "Waypoint constrain source position delta");
  ok &= near(constrained.constrained.pos[0], 12.0f,
             "Waypoint constrain adjusted x");
  ok &= near(constrained.angle_delta, kPi / 6.0f,
             "Waypoint constrain strict angle delta");
  ok &= near(constrained.constrained.rot[0][0], std::cos(kPi / 6.0f),
             "Waypoint constrain rotated x row x");
  ok &= near(constrained.constrained.rot[0][1], std::sin(kPi / 6.0f),
             "Waypoint constrain rotated x row y");

  return ok ? 0 : 1;
}
