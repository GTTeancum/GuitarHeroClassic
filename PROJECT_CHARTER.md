# GuitarHeroOGX Project Charter

This project is a native port of Guitar Hero II to the original Xbox, with a PC
build as a development harness and supported runtime target. The goal is not to
run the 360 recompile as the product. The goal is to build a new engine that
recreates the game using original, reverse-engineered behavior and resource-aware
asset handling.

## North Star

GuitarHeroOGX should behave like Guitar Hero II while fitting the constraints of
the original Xbox.

That means:

- Fresh native runtime code for rendering, audio, input, UI, streaming, and game
  systems.
- PS2 assets as the primary asset source because they are a better fit for the
  original Xbox memory and storage budget.
- Reverse-engineered behavior as the authority for game logic, timing, UI flow,
  and data interpretation.
- Console-style resource discipline even in the PC build.

The PC build is allowed and useful, but it must not become a desktop-only design
with unlimited-memory assumptions.

## What This Is

- A new original-Xbox-oriented engine under `engine/`.
- A PS2 asset consumer: ARK, DTB, MILO, TEX, VGS, animation, venue, character, UI,
  and song data should be loaded directly or converted through project-owned
  tooling.
- A port that can also run on PC for iteration, debugging, capture, comparison,
  and eventual usability.
- A reconstruction of the original game systems using evidence from the shipped
  data and recomp/decode traces.

## What This Is Not

- Not a rexglue-based game runtime.
- Not a 360 menu/game clone.
- Not a viewer-first desktop project.
- Not a "load everything and keep it forever" PC engine.
- Not a rewrite that invents behavior when the stock data or recomp can answer
  the question.

The rexglue and 360 recomp work is evidence. It is not the architecture.

## Evidence Rules

Use evidence in this priority order:

1. Stock PS2 assets and scripts in the ARK: DTB, MILO, TEX, VGS, config data,
   UI definitions, and serialized object data.
2. Local reverse-engineering output produced from those assets.
3. The local 360 recomp and trace worktree as a behavior and decode oracle.
4. User-provided reference images, video frames, notes, or hardware captures.

Do not use external references unless the user explicitly provides or approves
them. If a visual reference is supplied as a breadcrumb, treat it as guidance for
where to investigate, not as a replacement for RE evidence.

When a value is not pinned, say so. Do not invent data citations.

## Resource Model

The original Xbox target drives the resource model.

Design choices should assume:

- Small working sets.
- Explicit ownership and lifetime.
- Streaming from disc or staged asset bundles.
- Timely unload and cleanup when screens, songs, venues, characters, or banks are
  no longer active.
- No unbounded global caches.
- No full-game preloads.
- Texture, geometry, animation, and audio formats chosen with Xbox memory and GPU
  limits in mind.

The PC build should keep these constraints visible. It can have debug overlays,
assertions, captures, and analysis tools, but it should not hide resource mistakes
that would fail on console.

## Role Of The Viewer

The viewer is a tool for the port, not a separate product direction.

It is useful when it helps:

- Inspect PS2 assets.
- Validate decoders.
- Compare renderer output.
- Debug animation, character, venue, UI, texture, or audio data.
- Produce captures for port verification.

If viewer code is folded into the engine, it should follow the same renderer,
audio, asset lifetime, and streaming direction as the port.

## Role Of The 360 Recomp

The 360 recomp and rexglue workspace are a decode and behavior oracle.

Use them to answer questions like:

- What does this UI message do?
- How is this serialized object interpreted?
- What is the gameplay timing or state-machine behavior?
- What transform, color, or animation state exists at runtime?
- Which engine primitive is called by a stock script?

Do not use them as:

- The runtime engine.
- The renderer architecture.
- The audio architecture.
- A visual target when the PS2/original-Xbox port differs.
- A reason to carry desktop or 360-era assumptions into the Xbox port.

## Menus And UI

The menu system should run the stock PS2 UI data and scripts as much as possible.
The renderer should reproduce the original behavior by decoding the data and
grounding missing engine behavior through traces.

For menu work:

- Treat PS2 DTB/MILO data as the asset source.
- Treat recomp traces as a way to pin runtime behavior, not as the final target.
- Keep the five-item PS2 main menu distinct from 360-only items such as
  leaderboards.
- Keep visual constants tied to evidence. If a color, transform, text size, or
  layout rule is not pinned, mark it as unpinned.
- Prefer implementing the underlying UI state and layout model over hand-tuning a
  screenshot.

## Audio And Rendering

Rendering and audio are new native systems.

They should be built around:

- PS2 asset decoding or conversion into Xbox-suitable runtime formats.
- Predictable memory ownership.
- Streaming-friendly upload and eviction.
- Console GPU and audio constraints.
- Clear separation between asset decode, runtime resource creation, and live
  scene/audio state.

The PC backend may use host APIs, but it should exercise the same high-level
resource flow as the Xbox backend.

## Development Discipline

- Keep RE findings close to the code they justify.
- Cite the data, trace, or function that grounds non-obvious behavior.
- Separate temporary inspection tools from shipping runtime paths.
- Do not let a tool or oracle become the project identity.
- Prefer small, verifiable steps that preserve the console resource model.
- When a handoff contradicts local evidence, trust the evidence and document the
  correction.

## One-Sentence Reminder

This is a resource-disciplined native Guitar Hero II port for the original Xbox,
using PS2 assets and RE-grounded behavior, with PC support as a harness and target,
not as an excuse to forget the console.
