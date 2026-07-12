# Deferred Notes

## Rendering / Asset Format

- 2026-07-12: RedOctane venue gears are still not complete. Several gears have wrong-facing/local-axis behavior and need active user visual review against original gameplay before marking the venue animation pass done.
- 2026-07-12: RedOctane audience floor is verified present/renderable in freecam. Pink surfaces in the venue inspector are active crosshair highlights, not missing-material or bad-floor evidence.
- 2026-06-23: Investigate apparent UV mapping issues visible in native venue screenshots. The user-provided example at `C:\Users\smmel\AppData\Local\Temp\codex-clipboard-58e40d45-095e-4972-bd59-5e7577f4ca9a.png` shows stretched/repeated striped textures across venue surfaces and props. Treat this as a later shared texture coordinate/material decode audit, not a venue-specific visual hack. Do not pull it into the current character hand/venue lighting priority unless a traced venue-format fix points directly at it.
