# Midori visual review — 2026-09-08

## Right-hand timing correction

The user found the prior strumming insufficiently visible. New right-hand-motion
GIF/MP4 shows 4 seconds at30fps; midori-playing-strum-fixed.mp4 shows the updated
full body at15fps for6 seconds. Same normal song autoplay; no exaggerated poses.
Removed unintended one-beat alignment and body crossfade from delta hand strokes.
Native wrist travel relative to the guitar roughly doubles; see
right-hand-validation.json. All three banks pass strict binding/all-pages tests.
Current candidate is midori-strumming-candidate.zip; still unapproved/incomplete.

## Latest playing proof

midori-playing-progress.gif / .mp4 supersede the failed gameplay diagnostic below.
About 6 seconds, 90 native frames at 15 fps from normal song autoplay. Source
servo enabled, separate hand banks, actual chart-driven fretting and strumming.
HUD/highway hidden to make both hands visible; no fallback animation mode.
Frames60,120,180,240,300,360,416 inspected. This is not an every-frame approval.

The exporter fixes the invalid Control_Root CharBone name, converts the guitar
and fret target frames, and derives positive-X/Z-hinge arm frames from source
bind data. Three banks pass strict binding and all-pages decoding. A fresh build
from playing-conversion-recipe.json reproduces all five captured asset hashes.
The candidate ZIP preserves that exact probe; it is NOT an approved release.

Remaining: full animation/action library, UI bank, loop seams, detailed hand-map
correspondence, source delta-order confirmation, and retail GH2 testing.

Current-turn conversion/capture scratch and compiler outputs were removed after
validation. Retained candidate ZIP is about1 MB and contains the exact reviewed
probe package, not a live installation.

## Earlier viewer and failed gameplay captures

UNAPPROVED. Native ghogx_app character-viewer captures of the PS2-source
conversion probe, not song gameplay or retail GH2 validation. No live DLC changed.

- full-body-motion: 44 successive native captures at fixed 0.125-second steps.
  Selection intro2 clip with source face-idle and acc01 hair layers; 5.5 seconds.
- face-hair-motion: same clip and time steps, closer camera; 5.5 seconds.
- guitar-contact-unresolved: 56 captures at the same time step; 7 seconds.
  Performance med_idle01 with candidate source hand layers and stock Xplorer.
  Visible failure: guitar below hands, poor arm alignment, dark prop rendering.

GIF previews and H.264 MP4s encode the same original frames, with no interpolated
motion. GIFs repeat for convenient review; that repeat is not proof of a seamless
authored loop. First captured frame is after the first simulation step.

Assistant inspected full-body and close-up frames 1, 22, 44 and guitar frame 28.
This was a framing/progress check, not an exhaustive every-frame approval.
User visual approval remains required. Eye/face/hair fidelity and arm mechanics
remain open; visible motion alone does not prove correspondence to retail GH3.

Exact native commands are in visual-proof-manifest.json. Temporary capture frames
and encoder logs (192,392,598 bytes) were removed after successful encoding.
The older conversion scratch from the preceding turn remains untouched.

## Song gameplay request

gameplay-diagnostic.gif / .mp4: 8 seconds, 64 native frames at 8 fps, silent.
Shout at the Devil expert chart, autoplay, diagnostic start at 30 seconds,
guitarist0 front camera, requested neutral proof lighting, Xplorer prop.
This is actual song gameplay with a diagnostic animation fallback, NOT a correct
performance or normal-runtime pass. Midori's arms remain spread and her instrument
is incorrectly positioned. The highway obscures much of the lower body.

Normal source servo playback exited 3221226505 before frame one, after loading
Midori and the four-entry main catalog. Missing gameplay/hand clips are logged;
the precise crash cause is not established. Existing diagnostic environment
GHOGX_DISABLE_SOURCE_SERVO_RUNTIME=1 permits capture using fallback playback.
No engine code or live DLC was changed. Command/environment recorded separately.
The final front-camera framing was checked at capture frame 32. Earlier oblique
run frames 2,32,65 were also viewed. No exhaustive animation approval performed.
