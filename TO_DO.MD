# TO-DO

This file contains open work only. Completed work belongs in Git history,
proof manifests, and technical documentation—not in this queue.

## Open work at a glance

1. Certify calibration on real hardware.
2. Add a modern in-theme audio/video calibration menu.
3. Fix and visually accept live venue-camera motion.
4. Add original-Xbox support for Xbox 360 guitar hardware.
5. Add the in-theme Band Management/preferences menu.
6. Finish the two singers as unique, non-destructive playable guitarists.
7. Decide whether to add a later-game-style HOPO glow.
8. Prove the complete mixed-content gameplay matrix.
9. Fix GH1 Johnny Napalm's displaced teeth from source facts.

## Details

1. **Real-hardware calibration certification**

   Test representative guitars, displays, and audio devices. Record measured
   audio/video defaults and confirm note judgment without widening the hit
   window. The software timing path is implemented; hardware certification is
   the remaining release work.

2. **Modern calibration menu**

   Add separate audio and video calibration while preserving GH2's visual
   identity. Clearly show both values and make each effect independently
   testable.

3. **Live venue-camera correction and acceptance**

   Fix authored gameplay shots entering or looking through venue geometry,
   then run the corrected camera path visibly and obtain user review. This is
   reproducible with the imported GH80s song `I Wanna Rock` in native GH2
   `fest`: the setlist, source handoff, venue intro, highway, HUD, notes, and
   scoring all work, but the later active-play camera is inside geometry in
   `stage-complete-20260901-10-functional-proof/dlc-gh80-i-wanna-rock-gameplay/`
   frames 1440 and 1600. GH1 Arena and Basement also reproduce obstructed or
   out-of-bounds gameplay views. The diagnostic same-shot restart fault is
   fixed; shot validation/selection still needs a source-accurate correction.

4. **Original-Xbox support for Xbox 360 guitars**

   a. Implement wired Xbox 360 X-Plorer enumeration, initialization, report
   parsing, frets, strum, Start/Back, whammy, and tilt/star power.

   b. Implement the Xbox 360 Wireless Receiver: enumeration, initialization,
   connection state, input packets, pairing, and required LED/status commands.

   c. Support one receiver and one active wireless instrument per physical Xbox
   controller port. Rebind after disconnect; do not implement multitap fanout.

   d. Treat standard-enumerating PS2-to-Xbox guitar adapters as protocol
   translators and add only mapping/whammy validation unless their descriptors
   prove that driver work is required.

5. **Band Management/preferences menu**

   Add persistent, data-driven Quickplay preferences for favorite
   character/outfit, venue, band-member outfits, guitar/finish, bass/finish,
   and Scruffy's bass. New DLC characters, outfits, instruments, finishes, and
   venues must not require menu-specific code.

6. **Unique singer guitarists**

   Publish the female and male singers as unique guitarist IDs and assets, with
   GH2 outfits first and GH1 alternates. They must not overwrite or mutate the
   original singer-role models. Remove each microphone subtree from the unique
   asset rather than hiding it at runtime.

   The Blender finger-rig experiment is paused and not release-ready. Its
   current staging file still has unresolved facial/jaw and helper-transform
   presentation, so do not use it as a shipping asset. Resume only with a
   systemic source-space import/rigging pass—not manual character offsets.

7. **Optional HOPO glow**

   Decide whether to add a distinct HOPO glow in the style of Guitar Hero III
   and later games. Remove this item if the feature is rejected.

8. **Complete mixed-content matrix**

    Prove GH1/GH2/GH80s songs, characters, and venues can be independently
    combined while GH2 remains authoritative for gameplay, highway, HUD, title
    card, scoring, and UI. A foreign character may supply only its authored
    unique highway texture. Run converted packages with GH1 compatibility paths
    disabled and cover the full supported gameplay cross-product.

9. **GH1 Johnny Napalm teeth**

    One teeth set appears outside the mouth during character select. Determine
    the authored face object, visibility state, animation/controller, and
    transform ownership from matched GH1 source. Do not add a character-name
    offset or mesh-hiding exception.
