// ps2_ark_hook.h - app-side init for the PS2 ARK hook.
//
// See ps2_ark_hook.cpp for what gets hooked and why. The hook itself is
// statically registered via REX_HOOK_RAW, so no runtime install step is
// needed -- but it does need to know the game data root so it can find
// gen/main.hdr + gen/main_0.ark. Call this from the app's OnPostSetup once
// the path config has been resolved.

#pragma once

#include <string>

namespace ps2_ark {

void set_game_data_root(const std::string& path);

}  // namespace ps2_ark
