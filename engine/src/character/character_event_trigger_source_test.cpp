#include "character/char_mesh.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool has(const std::vector<std::string>& values, const std::string& value) {
  return std::find(values.begin(), values.end(), value) != values.end();
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
  std::cerr << label << " got " << got << " want " << want << "\n";
  return false;
}

}  // namespace

int main() {
  using ghogx::character::EventTrigger;
  using ghogx::character::source_event_trigger_copy_plan;
  using ghogx::character::source_event_trigger_default_state;
  using ghogx::character::source_event_trigger_handler_plan;
  using ghogx::character::source_event_trigger_load_plan;
  using ghogx::character::source_event_trigger_prop_sync_plan;

  bool ok = true;

  const auto defaults = source_event_trigger_default_state();
  ok &= expect_bool(defaults.constructor_registers_events, true,
                    "constructor registers events");
  ok &= expect_bool(defaults.reset_self, false, "default unkdf/reset-self");
  ok &= expect_bool(defaults.enabled, true, "default enabled");
  ok &= expect_bool(defaults.enabled_at_start, true,
                    "default enabled-at-start");
  ok &= expect_string(std::to_string(defaults.unkde), "-1",
                      "default unkde sentinel");

  const EventTrigger native_default;
  ok &= expect_bool(native_default.reset_self, defaults.reset_self,
                    "native reset-self default");
  ok &= expect_string(std::to_string(native_default.trigger_order),
                      std::to_string(defaults.trigger_order),
                      "native trigger-order default");
  ok &= expect_string(std::to_string(native_default.anim_trigger),
                      std::to_string(defaults.anim_trigger),
                      "native anim-trigger default");
  ok &= expect_string(std::to_string(native_default.anim_frame),
                      std::to_string(defaults.anim_frame),
                      "native anim-frame default");

  const auto copy = source_event_trigger_copy_plan();
  ok &= expect_size(copy.copied_superclasses.size(), 2,
                    "copy superclass count");
  ok &= expect_string(copy.copied_superclasses[0], "Hmx::Object",
                      "copy first superclass");
  ok &= expect_string(copy.copied_superclasses[1], "RndAnimatable",
                      "copy second superclass");
  ok &= expect_size(copy.pre_copy_steps.size(), 1, "copy pre-step count");
  ok &= expect_string(copy.pre_copy_steps[0], "UnregisterEvents",
                      "copy unregister pre-step");
  ok &= expect_string(copy.copied_members[0], "mTriggerEvents",
                      "copy first member");
  ok &= expect_bool(has(copy.copied_members, "mAnims"), true,
                    "copy anim vector");
  ok &= expect_bool(has(copy.copied_members, "mProxyCalls"), true,
                    "copy proxy vector");
  ok &= expect_bool(has(copy.copied_members, "unkdf"), true,
                    "copy reset-self bit");
  ok &= expect_bool(has(copy.copied_members, "mPartLaunchers"), true,
                    "copy part launchers");
  ok &= expect_bool(has(copy.copied_members, "mEnabled"), false,
                    "copy does not copy enabled");
  ok &= expect_size(copy.post_copy_steps.size(), 2, "copy post-step count");
  ok &= expect_string(copy.post_copy_steps[0], "RegisterEvents",
                      "copy register post-step");
  ok &= expect_string(copy.post_copy_steps[1], "CleanupHideShow",
                      "copy cleanup post-step");
  ok &= expect_bool(has(copy.not_copied_members, "mSpawnedTasks"), true,
                    "copy omits spawned tasks");
  ok &= expect_bool(has(copy.not_copied_members, "unkbc"), true,
                    "copy omits unkbc");
  ok &= expect_bool(has(copy.not_copied_members, "mEnabledAtStart"), true,
                    "copy omits enabled-at-start");

  const auto handlers = source_event_trigger_handler_plan();
  ok &= expect_size(handlers.handlers.size(), 2, "handler count");
  ok &= expect_string(handlers.handlers[0], "trigger:OnTrigger",
                      "trigger handler");
  ok &= expect_string(handlers.handlers[1], "proxy_calls:OnProxyCalls",
                      "proxy calls handler");
  ok &= expect_bool(has(handlers.action_handlers, "enable:unkdf=true"), true,
                    "enable action handler");
  ok &= expect_bool(has(handlers.action_handlers, "disable:unkdf=false"),
                    true, "disable action handler");
  ok &= expect_bool(has(handlers.action_handlers,
                        "wait_for:unkdf=true;Trigger()"),
                    true, "wait_for action handler");
  ok &= expect_bool(has(handlers.action_handlers,
                        "basic_cleanup:BasicReset"),
                    true, "basic cleanup handler");
  ok &= expect_string(handlers.direct_returns[0],
                      "supported_events:SupportedEvents",
                      "supported events direct return");
  ok &= expect_string(handlers.superclasses[0], "RndAnimatable",
                      "handler first superclass");
  ok &= expect_string(handlers.superclasses[1], "Hmx::Object",
                      "handler second superclass");
  ok &= expect_string(std::to_string(handlers.check), "943",
                      "handler check constant");

  const auto props = source_event_trigger_prop_sync_plan();
  ok &= expect_string(props.anim_props[0], "anim:ResetAnim",
                      "anim prop reset side-effect");
  ok &= expect_bool(has(props.anim_props, "period"), true,
                    "anim prop period");
  ok &= expect_bool(has(props.anim_props, "type"), true, "anim prop type");
  ok &= expect_string(props.proxy_call_props[0], "proxy:clear_call",
                      "proxy prop clears call");
  ok &= expect_string(props.hide_delay_props[0], "hide",
                      "hide-delay first prop");
  ok &= expect_size(props.event_list_props.size(), 4,
                    "event-list prop count");
  ok &= expect_bool(has(props.event_list_props, "trigger_events"), true,
                    "trigger events prop");
  ok &= expect_bool(has(props.event_list_props, "enable_events"), true,
                    "enable events prop");
  ok &= expect_bool(has(props.event_list_props, "disable_events"), true,
                    "disable events prop");
  ok &= expect_bool(has(props.event_list_props, "wait_for_events"), true,
                    "wait-for events prop");
  ok &= expect_bool(props.event_lists_unregister_before_mutation, true,
                    "event-list unregister before mutation");
  ok &= expect_bool(props.event_lists_register_after_mutation, true,
                    "event-list register after mutation");
  ok &= expect_bool(has(props.properties, "anims:CheckAnims"), true,
                    "anims prop check");
  ok &= expect_bool(has(props.properties, "next_link:SetNextLink"), true,
                    "next-link prop setter");
  ok &= expect_bool(has(props.properties, "enabled_at_start"), true,
                    "enabled-at-start prop");
  ok &= expect_bool(has(props.properties, "anim_frame"), true,
                    "anim frame prop");
  ok &= expect_string(props.superclasses[0], "RndAnimatable",
                      "prop-sync superclass");

  const auto bad_low = source_event_trigger_load_plan(-1);
  ok &= expect_bool(bad_low.known_revision, false, "revision -1 rejected");
  ok &= expect_size(bad_low.load_steps.size(), 0,
                    "invalid low revision has no steps");

  const auto bad_high = source_event_trigger_load_plan(0x12);
  ok &= expect_bool(bad_high.known_revision, false, "revision 18 rejected");
  ok &= expect_size(bad_high.load_steps.size(), 0,
                    "invalid high revision has no steps");

  const auto rev6 = source_event_trigger_load_plan(6);
  ok &= expect_bool(rev6.known_revision, true, "revision 6 accepted");
  ok &= expect_string(rev6.load_steps[0], "LOAD_REVS", "rev6 first step");
  ok &= expect_string(rev6.load_steps[1], "Hmx::Object",
                      "rev6 object load");
  ok &= expect_string(rev6.load_steps[2], "UnregisterEvents",
                      "rev6 unregister before rows");
  ok &= expect_bool(has(rev6.load_steps, "mEnableEvents"), true,
                    "rev6 enable events");
  ok &= expect_bool(has(rev6.load_steps, "mDisableEvents"), true,
                    "rev6 disable events");
  ok &= expect_bool(has(rev6.load_steps, "mWaitForEvents"), true,
                    "rev6 wait-for events");
  ok &= expect_bool(has(rev6.load_steps, "legacyHideDelayGrossBranch"), true,
                    "rev6 hide-delay legacy branch");
  ok &= expect_bool(has(rev6.load_steps, "legacyIteratorJank"), true,
                    "rev6 iterator legacy branch");
  ok &= expect_bool(has(rev6.load_steps, "mAnims"), false,
                    "rev6 no anim vector");
  ok &= expect_bool(has(rev6.load_steps, "mProxyCalls"), false,
                    "rev6 no proxy vector");
  ok &= expect_bool(rev6.anim.reset_anim_for_legacy, true,
                    "rev6 legacy anim reset");

  const auto rev7 = source_event_trigger_load_plan(7);
  ok &= expect_bool(has(rev7.load_steps, "legacyTriggerEvent"), true,
                    "rev7 legacy trigger symbol");
  ok &= expect_bool(has(rev7.load_steps, "mAnims"), true,
                    "rev7 anim vector");
  ok &= expect_bool(has(rev7.load_steps, "mSounds"), true,
                    "rev7 sound list");
  ok &= expect_bool(has(rev7.load_steps, "mShows"), true,
                    "rev7 show list");
  ok &= expect_bool(has(rev7.load_steps, "mNextLink"), true,
                    "rev7 next link");
  ok &= expect_bool(has(rev7.load_steps, "legacyIteratorJank"), false,
                    "rev7 no iterator legacy branch");
  ok &= expect_bool(has(rev7.load_steps, "mProxyCalls"), false,
                    "rev7 no proxy vector");

  const auto rev10 = source_event_trigger_load_plan(10);
  ok &= expect_bool(has(rev10.load_steps, "mTriggerEvents"), true,
                    "rev10 trigger vector");
  ok &= expect_bool(has(rev10.load_steps, "legacyTriggerEvent"), false,
                    "rev10 no legacy trigger");
  ok &= expect_bool(has(rev10.load_steps, "RemoveNullEvents(mEnableEvents)"),
                    false, "rev10 no remove-null pass");
  ok &= expect_bool(has(rev10.load_steps, "mProxyCalls"), true,
                    "rev10 proxy vector");
  ok &= expect_bool(rev10.anim.reset_anim_for_legacy, false,
                    "rev10 no anim reset");
  ok &= expect_size(rev10.anim.read_order.size(), 11,
                    "rev10 extended anim reads");
  ok &= expect_string(rev10.anim.read_order[4], "mEnable",
                      "rev10 anim enable read");
  ok &= expect_string(rev10.anim.read_order[5], "mRateInt",
                      "rev10 anim rate int");
  ok &= expect_size(rev10.proxy_call.read_order.size(), 2,
                    "rev10 proxy call rows");

  const auto rev17 = source_event_trigger_load_plan(0x11);
  ok &= expect_bool(has(rev17.load_steps, "RndAnimatable"), true,
                    "rev17 animatable superclass");
  ok &= expect_bool(has(rev17.load_steps, "mHideDelays"), true,
                    "rev17 hide delays");
  ok &= expect_bool(has(rev17.load_steps, "mTriggerOrderInt"), true,
                    "rev17 trigger order int");
  ok &= expect_bool(has(rev17.load_steps, "mResetTriggers"), true,
                    "rev17 reset triggers");
  ok &= expect_bool(has(rev17.load_steps, "unkdfBitfield"), true,
                    "rev17 bool bitfield");
  ok &= expect_bool(has(rev17.load_steps, "mAnimTriggerInt"), true,
                    "rev17 anim trigger int");
  ok &= expect_bool(has(rev17.load_steps, "mAnimFrame"), true,
                    "rev17 anim frame");
  ok &= expect_bool(has(rev17.load_steps, "mPartLaunchers"), true,
                    "rev17 part launchers");
  ok &= expect_bool(has(rev17.load_steps, "ConvertParticleTriggerType"), true,
                    "rev17 post-load particle conversion");
  ok &= expect_size(rev17.proxy_call.read_order.size(), 3,
                    "rev17 proxy event rows");
  ok &= expect_string(rev17.proxy_call.read_order[2], "mEvent",
                      "rev17 proxy event row");
  ok &= expect_size(rev17.hide_delay_read_order.size(), 3,
                    "hide-delay row count");
  ok &= expect_string(rev17.hide_delay_read_order[0], "mHide",
                      "hide-delay hide row");
  ok &= expect_string(rev17.hide_delay_read_order[1], "mDelay",
                      "hide-delay delay row");
  ok &= expect_string(rev17.hide_delay_read_order[2], "mRate",
                      "hide-delay rate row");

  return ok ? 0 : 1;
}
