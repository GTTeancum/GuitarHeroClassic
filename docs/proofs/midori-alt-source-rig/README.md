# PS2 Midori alternate outfit gameplay probe — 2026-09-08

The alternate PS2 model midori_2 uses the same four animation banks as the base candidate, verified byte-for-byte. The generic source converter and probe builder now accept an outfit selection; no alternate-specific pose adjustments were added.

Proof: midori-alt-playing.gif and midori-alt-playing.mp4, three seconds at 30 fps in native Shout at the Devil gameplay with normal source servo, source hand clips, and a stock Xplorer guitar. HUD/highway hidden and proof lighting enabled. Final camera frames 60, 120, and 238 inspected; this is a sampled visual review, not an exhaustive frame review.

Source: verified PS2 models/guitarists/midori_2.skin.ps2 from the fresh source IR. 5,041 triangles and 4,271 output vertices. The independent-source-geometry.png render corroborates the large bow geometry in the raw PS2 mesh; it does not establish retail runtime parity.

midori-both-outfits-candidate.zip contains both outfits with shared banks. conversion-recipe.json rebuilds the standalone alternate probe through the generic builder. No live DLC installed. Visual approval, full action coverage, UI clips, loop seams, and retail GH2 compatibility remain pending.
