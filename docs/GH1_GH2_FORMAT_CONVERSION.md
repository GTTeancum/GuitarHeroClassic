# GH1 to GH2 native asset-format conversion

## Purpose

This document is the executable contract for converting retail Guitar Hero 1
PS2 characters and venues into retail Guitar Hero 2 PS2-native assets. Runtime
compatibility is useful evidence, but it is not the output of this work. The
target is an offline converter whose output can be loaded through the ordinary
GH2 object, directory, archive, animation, and renderer paths without a GH1
runtime adapter.

A conversion is not considered understood when it merely looks plausible. A
format layer is closed only when:

1. Every consumed source byte belongs to a named field, declared padding, or a
   documented opaque field proven irrelevant to GH2 behavior.
2. Every object and directory reference is resolved by type and identity.
3. Parse followed by save is byte-exact when the model is unchanged.
4. A changed model saves deterministically and reparses to the same semantics.
5. The GH2 target revision and inheritance order are supported by a writer.
6. Complete packed-asset sweeps have no unclassified revisions, residual bytes,
   dangling references, or decoder fallbacks.

Names, paths, character identities, venues, meshes, bones, materials, poses,
and byte offsets may be test inputs. They may not select parsing rules or
conversion corrections.

## Evidence policy

Evidence strength, from strongest to weakest:

1. Retail packed bytes plus a byte-accounted parser/writer and round-trip test.
2. Retail executable load/save behavior established by read-only PCSX2 memory
   traces or static disassembly.
3. Matching open-source engine/editor implementation for the same class and
   revision, checked against retail bytes.
4. Cross-file invariants demonstrated over the complete packed inventory.
5. Visual/runtime behavior. This validates semantics but cannot establish field
   order or prove that ignored bytes are irrelevant.

Guessed layouts, marker scans, asset-specific offsets, and visual compensation
are discovery aids only. They are not format contracts.

The source audit includes
[`PikminGuts92/pikaxe`](https://github.com/PikminGuts92/pikaxe) commit
`d35227d8d8dc03e248bb817f98b273b3e27572f9`. It is useful corroboration for
many later class fields and target revisions, but it is not accepted as a
complete contract: its directory reader explicitly guesses object sizes by
searching for `ADDE` markers, its directory metadata/external dependency work
contains TODO paths, and several animation writers select convenient output
revisions or leave later fields unsupported. Those limitations match gaps G3,
G5, G6, and G9 rather than closing them.

Animation-transition semantics are additionally corroborated against
[`DarkRTA/rb3`](https://github.com/DarkRTA/rb3) commit
`ababaa9bb6a669af06aa7def7dd735c4f5265061`. Its `CharGraphNode` declaration
names the two floats as the beat where blending leaves the current clip and the
beat where it enters the next clip. Its `CharClip::Transitions` implementation
groups those nodes by target clip. This later engine source supplies field
meaning; the GH1 packed files independently establish the ACG byte layout.

## Packed source inventory baseline

The audited GH1 retail `MAIN.HDR` is ARK v3 with one ARK part and 3,167
entries. Relevant extensions include 105 `rnd_ps2` object directories, 926
standalone `acp` clips, 25 `acg` graphs, 2 `acs` sets, 156 `dtb` trees, 61
`gh` resources, and 1,523 PS2 bitmap resources (`bmp_ps2` plus `png_ps2`).

The current hybrid GH2 archive is not accepted as the sole GH2 source
inventory because it contains both original GH2 and mounted/ported content. It
contains 488 `milo_ps2` entries and is valid for target-format sampling. A
separate clean retail GH2 inventory and provenance hash remain required.

Sample files extracted during audits live under ignored build directories.
Copyrighted packed assets and decompressed bodies must never be committed.

`tools/format_audit` now performs the archive sweep in memory and emits a TSV
ledger plus a type/revision ledger. The first complete sweep proves:

- all 105 GH1 revision-10 object directories retain byte-exact outer
  containers and structural directory prefixes, and have one unique complete
  type/revision-constrained object-terminator chain to payload EOF;
- all 156 GH1 DTBs retain byte-exact storage;
- all 926 GH1 ACP files parse and save byte-exactly with no trailing bytes;
- all 25 GH1 ACG files parse and save byte-exactly with no trailing bytes;
- both GH1 ACS files parse and save byte-exactly, and both compiled include
  references resolve to packed `gen/*.dtb` entries;
- all 926 ACP bodies are revision 18, use compression mode 1 in both channel
  sets, and carry SampleSet serialization revision 5 at body offset 28;
- all 488 current hybrid revision-24 MILOs retain byte-exact outer containers
  and structural directory prefixes;
- all 179 current hybrid DTBs retain byte-exact storage;
- all 1,214 audited GH1 files in the five format families (`dtb`, `rnd_ps2`,
  `acp`, `acg`, and `acs`) pass their current outer-format contracts;
- all 12,189 object bodies in the 105 GH1 revision-10 directories parse and
  reserialize byte-exactly through semantic, revision-aware readers and
  writers: 21 of 21 observed type/revision rows, zero failures, zero residual
  bytes, and no asset-name or offset exceptions.

The full sweep corrected a sample-based assumption: three Grim wing ACPs have
an empty channel set with a nonzero sample count. Because its frame width is
zero, the set consumes no sample bytes but retains the shared timeline count.

### GH1 object type/revision inventory

The revision-10 directories contain exactly these 21 observed body revisions:

| Type | Revision | Count |
|---|---:|---:|
| Cam | 9 | 20 |
| CamAnim | 0 | 6 |
| EnvAnim | 3 | 82 |
| Environ | 1 | 161 |
| Flare | 3 | 35 |
| Font | 7 | 20 |
| Light | 3 | 127 |
| LightAnim | 1 | 88 |
| Mat | 21 | 1,693 |
| MatAnim | 5 | 156 |
| Mesh | 25 | 7,087 |
| MeshAnim | 0 | 18 |
| Morph | 3 | 14 |
| Movie | 6 | 2 |
| MultiMesh | 0 | 41 |
| ParticleSys | 22 | 76 |
| ParticleSysAnim | 2 | 60 |
| Tex | 8 | 1,067 |
| Text | 15 | 72 |
| TransAnim | 4 | 686 |
| View | 7 | 678 |

The corresponding current revision-24 target inventory uses, among others,
Cam 12, CamAnim 2, EnvAnim 4, Environ 5, Group 12, Light 6, LightAnim 2,
Mat 27, MatAnim 7, Mesh 28, MeshAnim 1, ParticleSys 27,
ParticleSysAnim 3, Tex 10, Trans 9, TransAnim 6, Spotlight 20, and
WorldCrowd 6. Character targets additionally use Character 9,
BandCharacter 1, CharBone 2, and CharClipSamples 10. These are observed target
revisions, not yet proof that each is the correct mapping for every GH1 source
object.

## Layer status

| Layer | Read contract | Write contract | Current evidence | Status |
|---|---|---|---|---|
| ARK v3 HDR/ARK | Lossless header, string tables, entries, split-ARK offsets | Deterministic header/index builder for a planned layout | Byte-exact GH1 and current hybrid headers | Partially closed |
| MILO outer container | NONE/GZIP/A/B/C/D read; exact block retention | Deterministic changed A/B blocks; unchanged structures retain source bytes | Byte-exact real GH1/GH2 character and venue samples | Partially closed |
| MILO object directory | Revision-aware prefix, type/name table, allocation hints, revision 7-16 external resources; exact GH1 body boundaries and semantic bodies | Complete deterministic GH1 directory rebuild; structural-prefix serialization for later revisions | All 105 GH1 directories rebuild byte-exactly; all hybrid revision-24 prefixes byte-exact | GH1 rev10 closed; GH2 rev24 open |
| MILO object bodies | Semantic readers for all 21 observed GH1 type/revision rows | Lossless semantic GH1 writers for all 21 observed rows | 12,189/12,189 packed GH1 bodies byte-exact, zero residuals | GH1 observed revisions closed; GH2 targets open |
| Object/reference graph | String references interpreted ad hoc by runtime loaders | None | Partial transform/material/group resolution | Open |
| Classic DTB | All currently recognized classic node tags, storage form, seed, line metadata, trailing bytes | Plain, zero-prefixed, and encrypted serialization | Byte-exact real GH1/GH2 samples | Closed for recognized tags |
| GH1 ACP | Standalone wrapper, revision-18 body, and revision-5 SampleSet decoding | Lossless deterministic serialization | All 926 packed ACPs byte-exact plus GH1 load/save code | Closed for observed revisions |
| GH1 ACG | Version-1 clip-indexed transition graph | Lossless deterministic serialization | All 25 packed ACGs byte-exact plus later engine declarations | Closed for observed version 1 |
| GH1 ACS | Lossless line grammar for includes, comments, blanks, and macro invocations | Lossless deterministic serialization | Both packed manifests byte-exact; 2/2 includes resolve | Closed for observed manifests |
| PS2 HMX bitmap | Header/payload read and RGBA decode for supported encodings | Only diagnostic BMP output | Runtime texture proofs | Open |
| GH2-native converter | None | None | No native converted asset yet | Open |

## Proven MILO outer-container contract

Retail GH1 and GH2 PS2 character and venue containers use `MILO_B`
(`0xCBBEDEAF`) raw-DEFLATE blocks. Audited retail files begin block data at
`0x210`: a 16-byte header followed by storage for 128 32-bit block-size slots.
Only `block_count` slots are live. The remaining bytes are not reliably zero,
so they are retained verbatim for unchanged files and zeroed only in a newly
constructed deterministic container.

`gh::milo::Container` retains:

- the complete pre-block prefix;
- each original size-table value;
- the original compressed bytes;
- the corresponding uncompressed bytes;
- trailing bytes after the declared blocks.

Unchanged blocks reuse their original compressed representation. Changed
`MILO_A` and `MILO_B` blocks are encoded deterministically. Current real-file
proofs are byte-exact for:

- GH1 `charsys/alterna/gen/alterna.rnd_ps2`;
- GH1 `venues/theatre/gen/theatre.rnd_ps2`;
- GH2 `char/alterna2/og/gen/alterna2.milo_ps2`;
- GH2 `world/theatre/gen/theatre_bank.milo_ps2`.

This closes only the outer block container. It does not validate object
directory boundaries or any object body.

## Proven MILO directory-prefix contract

The directory prefix is now modeled independently from child bodies:

- the directory revision is always first;
- revision 14+ carries the root type, root name, and two allocation hints;
- the object table is a signed count followed by length-prefixed type/name
  pairs;
- revisions 7 through 16 carry a post-table external-resource count and
  length-prefixed paths;
- revision 17+ begins the root directory object's own serialized body
  immediately after the object table.

The parser retains every field above and the writer deterministically
reproduces the prefix. Full sweeps are byte-exact for every GH1 revision-10 and
current hybrid revision-24 directory prefix.

GH1 child framing no longer uses a nearest-marker or Mesh-specific heuristic.
For every declared table entry, the solver requires the body to begin with the
retail revision assigned to that declared type, requires its terminator to
lead to the exact revision of the next declared type, requires the last
terminator to end at payload EOF, and rejects zero or multiple complete
solutions. All 105 packed directories have exactly one solution across all
21 types and 12,189 objects. This establishes the observed GH1 body slices
without confusing marker-shaped texture/sample bytes for terminators.

Each proven slice and terminator is retained independently. The complete
revision-10 writer rebuilds the table, external-resource vector, raw class
bodies, and separators; all 105 decompressed payloads reproduce byte-exactly.
Synthetic edit coverage renames a table object and changes its body, reparses
the result, and reproduces the edited payload. Every observed GH1 child body
also has a semantic reader and writer; the structural layer remains independent
so directory framing is not inferred from class contents.

Directory revision 10 is not treated as proof of GH1 provenance by itself.
Generated/custom UI directories can use the same revision with later classes
such as `LabelEx`. Unknown rows use a packed-revision structural fallback for
read-only consumers and are explicitly marked non-exact/non-writable; only the
complete known 21-row retail contract may enter the revision-10 writer.

Retail `DirLoader` still calls each class loader before checking the
`AD DE AD DE` separator. The unique chain first proved structural framing
independently; the complete semantic sweep below now proves exact consumption
inside every GH1 slice as a second check. Revision-24 GH2 root/child framing
remains in G3 until the root and class readers replace its current scan.

## GH1 object-body consumption ledger

The format audit feeds each proven GH1 slice to a source-order class reader and
requires cursor equality with the body end, then serializes the typed value and
requires byte equality with the original slice. The packed-archive result is:

| Type/revision | Exact bodies | Residual/fail | Proven GH1 body contract |
|---|---:|---:|---|
| Cam 9 | 20 | 0 | Trans8, Drawable1, near/far/FOV, screen rectangle, Z range, target texture |
| CamAnim 0 | 6 | 0 | Animatable0, camera ref, FOV keys, keys owner |
| EnvAnim 3 | 82 | 0 | Animatable0, environment ref, ambient/fog colors and fog-range keys |
| Environ 1 | 161 | 0 | Obsolete Drawable block, drawable refs, light refs, ambient/fog state |
| Flare 3 | 35 | 0 | Trans8, Drawable1, material, two-axis size, range, steps |
| Font 7 | 20 | 0 | Material, cell metrics, character string, optional kerning table |
| Light 3 | 127 | 0 | Trans8, RGBA color, range, serialized light type |
| LightAnim 1 | 88 | 0 | Animatable0, light ref, color keys, keys owner |
| Mat 21 | 1,693 | 0 | Texture stages, transforms/wrap, blends, color, compact render state |
| MatAnim 5 | 156 | 0 | Animatable0, material ref, stage transform/texture keys, color/alpha keys |
| Mesh 25 | 7,087 | 0 | Trans8, Drawable1, geometry refs/state, vertices, faces, patches, bones, PS2 strip caches |
| MeshAnim 0 | 18 | 0 | Animatable0, mesh ref, point/UV/color vector keys, keys owner |
| Morph 3 | 14 | 0 | Animatable0, pose Mesh refs and keys, target, flags/intensity |
| Movie 6 | 2 | 0 | Animatable0, file and texture refs, stream/loop state |
| MultiMesh 0 | 41 | 0 | Drawable block, source Mesh ref, instance transforms |
| ParticleSys 22 | 76 | 0 | Animatable0, Trans8, Drawable1, emission/state, emitter Mesh, preserved rows |
| ParticleSysAnim 2 | 60 | 0 | Animatable0, particle ref and color/rate/speed/life/size keys |
| Tex 8 | 1,067 | 0 | Dimensions/type/external state plus optional 32-byte HMX bitmap header and payload |
| Text 15 | 72 | 0 | Drawable, Trans8, font/text/layout/color/markup state |
| TransAnim 4 | 686 | 0 | Animatable0, Drawable, target and rotation/translation/scale key tracks |
| View 7 | 678 | 0 | Animatable0, Trans8, Drawable1, children owner, showing range |
| **Total** | **12,189** | **0** | **21/21 observed type/revision rows** |

The Environ sweep corrected an earlier renderer assumption: its obsolete
drawable list is typed to `RndDrawable`, not Mesh. Retail Big Club data
legitimately references `light_big.view`; directory fixup must enforce the
target class instead of a filename-suffix allowlist.

Morph revision 3 also corrects an incomplete third-party layout: every pose
starts with its Mesh reference, each key stores value then frame, and the pose
vector is followed by the target Mesh before normals, spline, and intensity.
The later engine `RndMorph::Pose`/`RndMorph::Load` declarations corroborate
that ownership and order; all 14 GH1 bodies independently prove it and
round-trip exactly.

The higher-risk tail contracts are also classified rather than retained as
opaque residuals:

- Tex8 embeds a 32-byte HMX bitmap header followed by its exact platform
  payload when the body is not external-only.
- View7 is exactly Animatable0, Trans8, Drawable1, children owner, and showing
  range. The former apparent four-byte flag prefix was the Drawable revision
  word; no transform-marker scan is needed.
- ParticleSys22 stores `bounceEnabled` plus a four-float plane after its four
  endpoint colors, then force direction and material. It stores emitter Mesh
  after relative motion, and each preserved particle is a 32-byte
  position/color/size row.
- Mesh25 stores 48-byte vertices (position, normal, serialized
  `Hmx::Color32` channels, UV), 16-bit triangle indices, byte patch sizes, up
  to four fixed bone slots with 3x4 matrices, and one last-generation PS2 strip
  cache per populated patch. No packed GH1 Mesh has a non-null BSP tree.

This closes the byte layout and lossless GH1 read/write contract for every
observed class revision. It does not yet close the meaning of every compact
Mat21 legacy state, GH2 target revisions, reference fixups, or GH1-to-GH2
conversion; those remain explicit gaps below.

## Proven ARK v3 header contract

`gh::ark::Index` retains the version, flag, declared part sizes, raw string
blob, string offsets and ordering, five-word file entries, and trailing bytes.
No-edit serialization is byte-exact for the 149,438-byte GH1 retail header and
the 78,602-byte deployed hybrid header. The parser bounds every count and
NUL-terminated string against the header, validates name/folder indices, and
checks every entry range against the concatenated declared part sizes.

`make_index` deterministically builds a v3 header for an already planned ARK
layout by normalizing separators and sorting/deduplicating strings. This does
not yet close archive writing: part placement, alignment, entry compression,
and validation against a clean retail GH2 archive remain open.

## Proven classic DTB contract

The DTB tree retains the raw version, storage form, cipher seed, node tags,
array line values, float bit patterns, and trailing decrypted bytes.
Serialization supports:

- direct plaintext beginning with `0x01`;
- four-zero-byte-prefixed plaintext used by venue sequence data;
- PS2 stream-cipher storage with its original seed.

Synthetic coverage exercises integer, float, string, variable, symbol, array,
script, and encrypted forms. Real byte-exact proofs cover GH1 camera/venue DTBs
and GH2 character/arena DTBs. Any newly encountered tag is still a hard parse
failure and must be classified before the DTB layer is declared complete for
the full archives.

## GH1 ACP contract

Standalone ACP files begin with length-prefixed class and object names. All
currently audited character clips use class `AnimClipSamples` and body revision
18. The revision-18 body has a 32-byte fixed prefix followed by two channel-set
headers and then the two sample blocks. Each channel-set header contains:

1. channel count;
2. length-prefixed typed channel names;
3. sample count;
4. compression mode.

Declared sample bytes exactly consume the remaining file in audited clips.
Position/scale, quaternion, and scalar-axis channels use type-dependent widths
selected by the compression mode. Empty channel sets still serialize sample
count and compression fields. Three retail Grim wing clips prove that an empty
set may retain a nonzero timeline sample count while consuming zero sample
bytes.

The fixed-prefix words are body revision, start beat, end beat, beats per
second, flags, play flags, blend width, and SampleSet serialization revision.
Static analysis of the GH1 retail executable closes the last field:

- `AnimClipSamples::Load` at `0x0017cdb8` first invokes its base loader, reads
  one 32-bit value into the revision gate at `0x00363b94`, then loads both
  SampleSets through the same revision-aware helper;
- that helper at `0x0017c210` branches on the gate when decoding the SampleSet
  header;
- `AnimClipSamples::Save` at `0x0017ced0` first invokes its base saver, writes
  the constant `5` as a 32-bit value, then saves both SampleSet headers and
  both sample blocks.

The tool consequently names this field `sample_set_revision` and rejects
unproven values rather than applying the revision-5 layout to another format.
All 926 packed ACPs use the proven value and round-trip byte-exactly.

## GH1 ACG transition-graph contract

Every packed GH1 ACG is version 1 and has this complete little-endian layout:

1. `u32 version`;
2. `u32 source_clip_count`;
3. for each source clip, in ACS clip order:
   1. `u32 transition_node_count`;
   2. repeated nodes of `u32 target_clip_index`, `f32 current_beat`,
      `f32 next_beat`.

Every target index in the 25-file inventory is in the same clip-index domain.
All files end immediately after the last node. The inventory spans 1 to 81
clips per graph and 1 to 4,439 nodes, so the contract covers both minimal UI
graphs and full performer graphs. Packed-byte accounting proves the framing;
the later `CharGraphNode`/`CharClip::Transitions` implementation supplies the
source/target and beat semantics. `tools/acg` supports exact no-edit round
trips, deterministic edits, target-index validation, and truncation rejection.

## GH1 ACS manifest contract

The packed `charsys/anims.acs` and `arena/anims.acs` files are ASCII
precache manifests, not opaque binary clip data. Their complete observed
grammar is:

1. blank lines;
2. semicolon comment lines;
3. `#include <relative .dta path>` lines;
4. bare macro-invocation symbols, one per line.

Line endings, whitespace, comments, and suffixes are retained independently
from editable include/invocation values, so `tools/acs` round-trips both files
byte-exactly. Compiled include lookup inserts `gen` below the ACS directory
and changes `.dta` to `.dtb`: `charsys/anims.acs` therefore resolves
`anims_macros.dta` to `charsys/gen/anims_macros.dtb`, and the arena manifest
resolves to `arena/gen/anims_macros.dtb`. Both targets exist in the packed
GH1 archive. The manifests contain 30 character/band invocations and one
arena crowd invocation respectively.

## Conversion-blocking gap register

1. **G1 — Clean GH2 provenance.** Inventory a hash-identified unmodified GH2
   PS2 HDR/ARK and distinguish it from hybrid output.
2. **G2 — ARK writer.** Header/string/path-table serialization is implemented.
   Close physical entry ordering, part placement, alignment, and
   compressed-entry rules, then prove a deterministic complete rebuild.
3. **G3 — Directory framing.** GH1 revision-10 child framing is now unique,
   complete, and revision-constrained across the packed archive. Replace the
   revision-24 root/child scan with exact readers/serialization; decode
   recursive directories and account for every residual byte.
4. **G4 — Type/revision inventory.** The complete packed GH1 inventory is
   closed at 12,189/12,189 bodies across 21/21 observed rows, with exact
   inheritance order, byte consumption, and semantic round trips. Inventory
   every corresponding clean-retail GH2 target directory and close every GH2
   row to the same standard.
5. **G5 — Object writers.** Revision-aware lossless GH1 readers/writers are
   complete for all observed rows. Implement the complete GH2 target readers
   and writers, including inherited bases and fields absent from GH1, then add
   field-level edit/reparse coverage.
6. **G6 — Reference graph.** Model directory/object references explicitly,
   including nulls, externals, proxies, owners, parents, targets, and recursive
   subdirectories. Saving must use deterministic fixups with no string-search
   fallback.
7. **G7 — Character contract.** Close skeleton, skin weights/order, transform
   inheritance, twist/controller, face, hair, collide, cuff/sleeve, outfit,
   attachment, event, and clip graph serialization.
8. **G8 — Venue contract.** Close geometry, groups/views, transforms, cameras,
   camera animation, lighting/environments, material animation, particles,
   crowds, props, scripts, events, and draw-state serialization.
9. **G9 — ACP/ACG/ACS.** ACP and ACG now have lossless readers/writers, and
   revision-18 ACP framing, its nested revision-5 SampleSet contract, and ACG
   version-1 framing/transition semantics, and the observed ACS
   include/invocation grammar are closed. Implement deterministic GH1
   ACP/ACG/ACS to GH2 clip-object conversion.
10. **G10 — Texture/material packing.** Implement PS2-native texture encoding,
    palette/swizzle rules, alpha semantics, Mat revision conversion, and
    deterministic texture reference output.
11. **G11 — Native conversion sweep.** Convert every GH1 character and venue,
    reparse all outputs, load them through GH2-native paths with GH1 adapters
    disabled, and reject any residual or dangling reference.
12. **G12 — Runtime proof.** Validate mixed GH1/GH2 characters, venues, songs,
    and unique character highways while GH2 remains authoritative for gameplay
    and UI. Record close, readable video proofs and read-only traces.

## Completion gates

The goal is complete only when all gap-register entries are closed and the
following machine-readable gates pass:

1. Full GH1 and clean GH2 packed inventories with source hashes.
2. Byte-exact no-edit round trips for every supported source file.
3. Semantic edit/reparse tests for every field and reference category.
4. Zero unclassified object revisions and zero unexplained residual bytes.
5. Zero dangling, name-fallback, or asset-specific fixups.
6. Deterministic rebuild hashes from two clean output directories.
7. Complete GH1 character and venue conversion sweeps.
8. GH2-native load with GH1 compatibility decoders disabled.
9. Mixed-content gameplay and visual/video proof.
10. Deployed assets and a linked final proof manifest.
