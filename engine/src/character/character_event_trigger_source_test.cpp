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
  using ghogx::character::source_event_trigger_load_plan;

  bool ok = true;

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
