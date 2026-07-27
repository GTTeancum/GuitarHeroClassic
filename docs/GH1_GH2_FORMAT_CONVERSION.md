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

GH2 class layouts are not a discovery gap. The authoritative target
read/write contract is the checked-out
`ihatecompvir-public-milo-sources/MiloEditor` implementation at commit
`3c02a5497ede1a5d61023fb066cc8bfbe2e8a8e4`. Retail GH2 bytes are used to
verify that this converter emits compatible target objects and to preserve
platform-specific cache/packing behavior; they are not used to invent a
second GH2 schema. The unknown side of each mapping is GH1 field meaning and
how that meaning is represented by the established GH2 classes.

The source audit includes
[`PikminGuts92/pikaxe`](https://github.com/PikminGuts92/pikaxe) commit
`d35227d8d8dc03e248bb817f98b273b3e27572f9`. It is useful corroboration for
many later class fields and target revisions, but it is not accepted as a
complete contract: its directory reader explicitly guesses object sizes by
searching for `ADDE` markers, its directory metadata/external dependency work
contains TODO paths, and several animation writers select convenient output
revisions or leave later fields unsupported. The local byte-accounted
readers/writers and packed sweeps close those limitations independently;
Pikaxe is corroboration, not a dependency of the converter.

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

A clean GH2 USA retail image is established independently by Redump MD5
`317968cd573b183f3c697bb94e8c8ee6` (`SLUS-21447`, version 1.00). Its extracted
base files are:

- `MAIN.HDR`: SHA-256
  `027176E241DE0BC28AF347403E9732E47260ED14A0FB322BB65FCC20D23FD4FD`;
- `MAIN_0.ARK`: 3,118,656,871 bytes, SHA-256
  `8CADFB195D6783A7A95A72F3A4DBB56695A364452BBE96881F4166A437520D21`.

Those hashes identify the immutable target corpus used to recover and test the
GH2 contracts. The deployed hybrid now contains a manifest-driven overlay:
`main.hdr.pre-overlay.bak` retains the original 78,602-byte index, and the
original ARK byte range remains unchanged while converted entries are appended
and the active header is rebuilt. The clean base contains 488 revision-24
`milo_ps2` directories and 179 DTBs.

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
- expanding the two manifests produces 31 clip-set records and 926 animation
  records; 22 owned graphs cover 691 clips, while three one-node graphs have
  no ACS or exact DTB owner;
- all 926 ACPs use set 0 only for multi-sample channels and set 1 only for
  one-sample constants, with zero cross-set channel overlap and no facing
  channel in set 1;
- all 488 clean GH2 revision-24 MILOs retain byte-exact outer containers
  and structural directory prefixes;
- all 179 clean GH2 DTBs retain byte-exact storage;
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
revisions. The complete conversion sweep, target reparse, reference audit, and
native runtime sweep described below prove the selected mapping for every
observed GH1 source object.

## Layer status

| Layer | Read contract | Write contract | Current evidence | Status |
|---|---|---|---|---|
| ARK v3 HDR/ARK | Lossless header, string tables, entries, split-ARK offsets | Deterministic header/index builder and append-only manifest overlay | Byte-exact GH1/clean-GH2 headers; three overlays are idempotent on the second pass | Closed for the deployed one-part PS2 contract |
| MILO outer container | NONE/GZIP/A/B/C/D read; exact block retention | Deterministic changed A/B blocks; unchanged structures retain source bytes | All emitted character, animation, and venue MILOs reparse; repeated conversion hashes match | Closed for observed GH1/GH2 PS2 storage |
| MILO object directory | Revision-aware prefix, type/name table, allocation hints, revision 7-16 external resources; exact GH1 and GH2 body boundaries | Complete deterministic revision-10 and revision-24 directory rebuild | All 105 GH1 directories rebuild byte-exactly; generated GH2 roots and children reparse exactly | GH1 rev10 and GH2 rev24 write framing closed |
| MILO object bodies | Semantic readers for all 21 observed GH1 type/revision rows; GH2 target layouts sourced from ihatecompvir | Lossless semantic GH1 writers and target serializers for every required mapping | 12,189/12,189 GH1 bodies exact; 13,115 converted plus 625 synthesized target objects, zero blocked | Closed for the complete packed conversion inventory |
| Object/reference graph | Schema-typed null, internal, external, parent, target, owner, material, texture, bone, group, and controller references | Deterministic name/fixup emission | 16,637 character references and 12,979 venue references accounted; zero dangling | Closed |
| Classic DTB | All currently recognized classic node tags, storage form, seed, line metadata, trailing bytes | Plain, zero-prefixed, and encrypted serialization | Byte-exact real GH1/GH2 samples | Closed for recognized tags |
| GH1 ACP | Standalone wrapper, revision-18 body, and revision-5 SampleSet decoding | Lossless deterministic serialization | All 926 packed ACPs byte-exact plus GH1 load/save code | Closed for observed revisions |
| GH1 ACG | Version-1 clip-indexed transition graph | Lossless deterministic serialization | All 25 packed ACGs byte-exact plus later engine declarations | Closed for observed version 1 |
| GH1 ACS | Lossless line grammar for includes, comments, blanks, and macro invocations | Lossless deterministic serialization | Both packed manifests byte-exact; 2/2 includes resolve | Closed for observed manifests |
| PS2 HMX bitmap | Header/payload read and RGBA decode for observed PS2 encodings | Tex8-to-Tex10 retains the byte-identical same-platform bitmap payload and rewrites only the target object wrapper | 1,067 source Tex8 bodies; 2,566 clean-target Tex10 round trips; full native venue/character texture loads | Closed for observed PS2 encodings |
| GH2-native converter | Semantic models for all observed GH1 body rows and required GH2 target rows | Systemic model, animation, controller, face, config, song-MIDI, and venue package emission | 105/105 directories, 926 clips, 13 character packages, 7 venues, zero blocked; native runtime loads use `char/...` and `world/gh1_...` | Closed |

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
clean GH2 revision-24 directory prefix.

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
inside every GH1 slice as a second check. The GH2 root and child class order is
defined by ihatecompvir. Local revision-24 deterministic serialization is now
implemented and reparse-proven for native character clip-set packages; G3
retains only recursive package/reference closure outside that completed path.

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
observed class revision. Mat21's complete field semantics are also closed by
the retail GH1 `RndMat::Load` at `SLUS_212.24:0x001BE900` and its debug routine
at `0x001BE458`: each texture-stage pair is stage blend/TexGen, and the tail is
`useEnviron`, `vertexAmbient`, `vertexDynamic`, `cull`, `multipass`,
`normalize`, `zMode`, `alphaCutout`, and `alphaWrite`. The Mat21-to-Mat27
mapping is closed by the source-backed revision-21 upgrade behavior: the first stage
becomes the root Mat27, every later stage becomes a deterministic
`<root>_<index>.mat`, and `nextPass` links the chain. Stage blend becomes the
pass blend, non-root passes use transparent Z mode, `MultipassSrc` resets
non-root passes to unlit white, and the obsolete `normalize` field is consumed
without a target serialization. Focused tests cover a two-stage multipass
material and its animated-stage routing; the packed reference audit covers the
generated pass links.

## Closed Mesh25-to-Mesh28 mapping

The first target conversion is source-defined and systemic:

- GH1 Mesh25 `Trans8` becomes GH2 `RndTransformable` revision 9. Local/world
  matrices, constraint, target, preserve-scale, and parent are retained.
  Trans8's obsolete serialized child vector is not a GH2 field; parent/owner
  fixups reconstruct the graph at directory-conversion time.
- GH1 `Drawable1` becomes GH2 `RndDrawable` revision 3. Showing state and the
  bounding sphere are retained; the obsolete drawable vector is represented
  by converted Group membership. GH2 draw order starts at the class default.
- Material, geometry owner, mutable flags, volume, the 48-byte PS2 vertex rows,
  16-bit faces, byte group sizes, four fixed legacy bone slots, 3x4 bone
  matrices, and cached PS2 strip rows transfer without reinterpretation.
- Empty GH1 BSP becomes the GH2 null BSP root. All 7,087 packed GH1 meshes have
  a null BSP; no asset-specific BSP rule exists.
- Cached strip sections remain optional. Transform-only/geometry-owner proxies
  can retain nonzero copied group sizes while omitting a local cache payload.

`parse_mesh28`/`serialize_mesh28` are checked against all 11,365 Mesh28 bodies
in the clean GH2 archive: 11,365 exact, zero failures, zero residual bytes.
The boundary proof validates the complete Mesh28 body before accepting an
object terminator, including BSP nodes, bones, and strip caches, so marker
bytes inside cached data cannot truncate a mesh. The complete GH1 regression
remains 7,087/7,087 exact. Synthetic conversion coverage reparses and
round-trips a changed Mesh25-to-Mesh28 model.

## Proven ARK v3 header contract

`gh::ark::Index` retains the version, flag, declared part sizes, raw string
blob, string offsets and ordering, five-word file entries, and trailing bytes.
No-edit serialization is byte-exact for the 149,438-byte GH1 retail header and
the 78,602-byte clean GH2 header. The parser bounds every count and
NUL-terminated string against the header, validates name/folder indices, and
checks every entry range against the concatenated declared part sizes.

`make_index` deterministically builds a v3 header for an already planned ARK
layout by normalizing separators and sorting/deduplicating strings.
`ark_tool overlay` consumes the same TSV manifests as loose-file deployment,
reuses byte-identical entries, appends changed payloads without disturbing the
retail prefix, and writes the header only after the planned layout is valid.
The current format-closure character and venue manifests contain 54 and 55
rows. Their verification pass reports all 54 and 55 entries reused with zero
replacements, additions, or appended bytes. The separately completed
singer-face MIDI sweep retains its own 58-row manifest; it is not counted as a
MILO/DTB/ACP converter output. The one-part PS2 archive integration contract
is therefore deterministic and idempotent; split-part reading remains
supported but no split-part writer is required by this target image.

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

The shared DTB preprocessor resolves nested includes and bare-symbol aliases
recursively, with cycle detection. Expansion yields 31 clip-set records:
eight main guitarist sets, eight UI sets, eight combined hand sets, five band
sets, one wing set, and one crowd set. They account for all 926 ACPs exactly
once.

## GH1 ACP to GH2 CharClipSamples mapping

The mapping is now corpus-proven and independent of clip or character names:

1. remove the standalone wrapper and revision-18/revision-5 gates;
2. clear ACP bit 31, which is an export marker rather than a gameplay flag;
   the lower 31 bits equal the expanded authored flags across the inventory;
3. translate GH1 beat-align 1/2/4/8 to GH2
   `0x1000/0x2000/0x4000/0x8000`, real time to `0x0200`, and GC-lock to
   GH2 user time `0x0400`;
4. map source set 0 to GH2 `full`, retaining multi-sample and virtual facing
   channels;
5. map source set 1 to GH2 `one`, retaining one-sample constants;
6. write GH2's revision-8-through-12 third legacy header without a third data
   block. The loader discards this header, so generated files use an empty
   deterministic header and never duplicate sample bytes;
7. group ACG nodes by target clip in first-target order while preserving node
   order within each group.

The sweep contains 782 populated set-0 bodies, 144 empty set-0 bodies, 926
one-sample set-1 timelines, zero channel overlaps, and 623 clips with exactly
two facing channels in set 0. Every generated revision-10 CharClipSamples body
reparses and reserializes exactly.

Static PS2 analysis establishes generated summary allocation values.
`CharClipSamples::AllocSize` combines a fixed `0x3a0`, transition storage, and
aligned CharBonesSamples allocations. Vector runtime slots are 16 bytes,
compressed quaternions 8, compressed scalar rotations 2, and each sample
stride is 16-byte aligned. GH2 reads and discards the two legacy summary
integers in a CharClipSet; no-edit target round trips preserve them, while new
assets use the deterministic PS2 runtime value.

## Native GH2 character clip-set packaging

`tools/milo_convert` emits complete revision-24 `CharClipSet` directories.
The 31 source records become 39 packages because each of the eight combined
GH1 hand bundles is partitioned by its authored external anchor into separate
fret and strum packages. The packages include:

- revision-14 roots with established GH2 role/type versions;
- revision-10 clips and converted ACG transitions;
- revision-2 CharBones derived from zero-geometry archetype meshes, effective
  transform inheritance, and channel contexts;
- external instrument anchors required by the GH2 fret/strum contract;
- CharClipFilter objects, flag-derived guitarist groups, and authored
  venue-exclusion groups;
- the seven standard GH2 authoring viewports and deterministic MILO_B
  containers.

All authored recenter targets and averaging bones are retained. `move_self`
is not assigned from an asset or role name: an authored true value and the
facing-driven guitarist contract take precedence; otherwise it is true only
when every recenter target has a multi-sample position channel in every clip
in that partition. This reproduces the target distinction evidenced by the
retail bass set (pelvis and carried-instrument targets always in `full`,
`move_self=1`) and singer sets (the fixed microphone target enters `one` in
stationary clips, `move_self=0`). The GH1 sweep independently exhibits the
same channel-set distinction.

The archetype sweep resolves 1,275 of 1,307 channel-base occurrences directly
to zero-geometry meshes. The remaining 32 are only the virtual facing pair or
the two external instrument anchors repeated by guitarist sets. All 39
packages and all 926 clips reparse through GH2-native readers and reproduce
their directory/container bytes on an independent second conversion pass.
The machine-readable ledger is
`tools/milo_convert/build/gh1-to-gh2-conversion-audit.tsv.packages.tsv`.

## Character manifest and GH2-native controller bodies

The shared generic DTB preprocessor now compiles the packed
`charsys/gen/charsys.dtb` character definitions rather than relying on a
character-name table. It resolves includes, merges, macros, and `HX_EE`
branches and extracts the authored outfit directory, skeleton load and
compiled path, LOD thresholds, sphere base, servo channels, driver realign
flag, twist/IK controller declarations, eye setup, walk setup, and face file.
The packed result is 13 character definitions: eight guitarist archetypes and
five band performers. It creates 79 authored controller declarations,
including Grim's wing servo/driver, two knee-to-cloak rods, and position
constraint. It also resolves the authored face and shadow package paths.
Every one of the 13 compiled skeleton RND paths and every declared auxiliary
package exists in the GH1 archive.

The clean GH2 target audit now has semantic readers and writers for every
character/controller family required by those manifests and their immediate
stock package dependencies. The following counts are whole-archive,
revision-24 object-body round trips against the clean retail target:

| Target body | Observed revision | Exact bodies |
| --- | ---: | ---: |
| `CharDriver` | 3 | 83 |
| `CharDriverMidi` | 3 | 76 |
| `CharEyes` | 3 | 19 |
| `CharForeTwist` | 1 | 118 |
| `CharHair` | 2 | 93 |
| `CharIKHand` | 2 | 76 |
| `CharIKMidi` | 4 | 38 |
| `CharIKRod` | 2 | 6 |
| `CharLookAt` | 2 | 38 |
| `CharPosConstraint` | 2 | 11 |
| `CharServoBone` | 1 | 81 |
| `CharUpperTwist` | 1 | 136 |
| `CharWalk` | 1 | 38 |
| `CharWeightSetter` | 2 | 76 |
| `EventTrigger` | 8 | 298 |
| `FaceFxLipSyncServo` | 5 | 42 |
| `OutfitLoader` | 1 | 40 |
| `WorldFx` | 1 | 193 |

All rows above consume the complete body and reserialize byte-for-byte with
zero failures. Important nested contracts are:

- `CharWeightable` revision 2 is `f32 weight` followed by its owner reference.
- `CharDriver` revision 3 is Object, Weightable, bones reference, clip-set
  path/reference, and one-byte realign flag.
- `CharHair` revision 2 stores six simulation floats, a strand vector, and a
  simulate byte. Each strand stores root, angle, points, and two 3-by-3
  matrices. Each point stores position, bone, length, the revision-2 legacy
  integer/string pair, radius, and outer radius.
- `EventTrigger` revision 8 stores its single trigger symbol, animation rows,
  sounds, shows, the legacy hide list, enable/disable/wait event lists, next
  link, and proxy calls. The legacy hide list is the revision-8 branch of the
  later hide-delay representation; omitting it leaves four residual bytes even
  when the list is empty.
- `FaceFxLipSyncServo` revision 5 is Object, Weightable, FAC path, viseme MILO
  path, and target rows of object reference, property-type integer, and
  property symbol.
- `WorldFx` revision 1 contains a complete nested `RndDir` revision-8 body.

`OutfitLoader` revision 1 was recovered from the clean PS2 executable as well
as the packed bytes. Its save routine at `SLUS_214.47:0x0018AEC8` writes:

1. revision 1 and Object fields;
2. the loader directory `FilePath`;
3. a `u16` category count;
4. for each category, one-byte selected and shown states plus a `u16` outfit
   count;
5. for each outfit, one-byte hide, desire, and exclude states.

The reciprocal loader at `0x0018B170` validates the serialized category and
outfit counts against the type-definition-created vectors before applying the
states. This exactly accounts for both clean target layouts: the guitar type
has one category with 58 outfits and a 214-byte body; the drummer type has one
category with eight outfits and a 65-byte body. The corresponding authored
type definitions were independently decoded from `char/gen/char_objects.dtb`.

## GH2-native character model package sweep

The model-package builder now converts all 13 packed GH1 character definitions
without consulting a character-name table. The eight selectable guitarist
archetypes become `BandCharacter` directories; the two singers, bassist,
drummer, and keyboardist become `Character` directories. LOD groups come from
the authored `lodN` view graph, and old-revision thresholds are normalized by
the derived or authored bounding-sphere radius as required by the target
`Character::PostLoad` contract. Each package is generated twice, produces
identical directory/container bytes, reparses with exact boundaries, and
serializes back to the same bytes.

GH1 character-shadow packages are separate revision-10 RND directories. Their
View graph may contain nested helper Views, so the converter finds the unique
unreferenced root and walks the complete reachable graph. Only meshes
reachable as drawables are merged; duplicate transform-only skeleton meshes
are intentionally not merged. Shadow drawables retain their authored
bone-slot offset matrices and refer to the primary character skeleton. This
matches the native target arrangement: clean GH2 character packages contain
shadow drawables but no duplicate shadow skeleton, and
`Character::SyncShadow` wraps each shadow-mesh bone reference around the
primary transform. All merged groups and drawables receive deterministic
role-based names, with group, geometry-owner, parent, and drawable references
rewritten together.

GH1 face packages contain revision-3 `Morph` objects plus their pose meshes.
The converter upgrades them to revision 4, identifies the unique frame-zero
basis pose from the one-hot curves, and binds each morph to the single primary
character mesh having the same vertex count and exact face-index topology.
Pose meshes and their references are then namespaced and merged. This works
across all eight guitarist faces and both singer faces without asset-specific
target names. GH1 selects authored pose frames from excitement and MIDI face
events, whereas stock GH2 normally drives facial bones through
`FaceFxLipSyncServo`, FAC actors, and viseme clip sets. The converter therefore
builds a native Morph/event graph instead of pretending the stock FaceFx input
registers are Morph outputs.

The character manifest now preserves the complete authored face-control
contract instead of only the face package path. The eight guitarist rows each
declare ten poses, five excitement-state pose sets, a 0.5-second blend, a
1.0-second random-pose interval, and hero MIDI mappings
`113 -> good01` / `114 -> good02`. Both singers declare `ref` and `open`, a
0.1-second blend, the `singer` event list, event offset 257, and
`108 -> open`. The apparent missing guitarist `event_list` is not an unknown:
the GH1 executable initializes it to `hero` before applying an optional
authored override. The preprocessed excitement symbols are the exact numeric
states 0 through 4 defined by the packed system macros.

The stock GH1 controller algorithm has also been recovered statically from
`SLUS_212.24`, rather than inferred from screenshots:

- `0x00287378` acquires `face_data` and `excitement_poses`;
- `0x00287490` loads `blend_time`, `pose_length`, and `event_offset`;
- `0x002878C0` resolves the effective event list as `singer` or `hero`;
- `0x00287A80` applies the event offset, selects the active MIDI face event,
  and resolves its pose symbol;
- `0x00287B70` advances the random-pose timer and selects a different pose
  when the authored pose length expires;
- `0x00287C78` maps a pose symbol to its zero-based index in the authored
  `poses` row, which is the Morph frame;
- `0x00287DD8` through `0x00287F38` crossfade the previous and destination
  frames over `blend_time` with the exact half-cosine weight
  `0.5 - 0.5*cos(pi*t)`.

This establishes that a faithful target graph must preserve discrete pose
selection and crossfade two evaluated Morph frames. Sweeping continuously
through the intervening frame numbers, treating FaceFX eye-input rows as
Morph outputs, or substituting a fixed expression would all be incorrect.
The converter now translates that contract into stock GH2 primitives. For
each ordered source/destination pose pair and each authored Morph target, it
emits a revision-4 Morph containing only those two pose meshes. Its keys sample
the exact `0.5 - 0.5*cos(pi*t)` weights at GH2's native 30-fps update frames,
so a nonadjacent transition never evaluates any intervening authored pose.
The pair Morphs are collected in a revision-12 Group, driven over the authored
blend duration by a revision-1 `AnimFilter`, and launched by a revision-8
`EventTrigger`. Self-pairs materialize and retain a single pose without
duplicating it.

The packed sweep produces 808 complete transition graphs: 100 for each of the
eight ten-pose guitarists and four for each two-pose singer. These graphs are
generated from the manifest pose rows and Morph topology; no package, mesh, or
pose-name case table participates in graph construction.

The converter also supplies a deterministic patch for the clean target
`../../system/run/milo/gen/rnd_objects.dtb`. It adds generic
`gh1_guitarist_morph_face` and `gh1_singer_morph_face` Group types without
replacing any stock Group schema. Converted face packages expose their
controller as `lip.servo`, allowing the existing stock `BandCharacter`
messages to reach the new target-native scripts. The guitarist type reproduces
the authored boot/bad, okay, great/peak pose pools, the one-second pose timer,
no-repeat selection, target expression-event mapping, override state, and
resume behavior. The singer type exposes exact `ref`/`open` transition
messages. `OwFace` has no GH1-authored pose and is intentionally not mapped to
a fabricated shape.

The DTA source for those rows is compiled by the new general DTA parser rather
than patched into DTB bytes by offsets. The parser supports Harmonix atom
types, `()`/`{}`/`[]` collections, strings, variables, numeric values, escapes,
line comments, and line metadata; the patched encrypted DTB reparses and
reserializes byte-for-byte. The clean 37,159-byte target file becomes a
51,346-byte deterministic file with the same 39 roots.

Singer song drive is now translated as data too. The `.voc` reader accepts the
two packed FACE archive revisions, 1200 and 1500, and accounts for their
14-byte and 36-byte revision-specific footers. It unions the positive
piecewise-linear support of the 15 non-neutral viseme curves into exact open
intervals. The SMF reader preserves HMX running status across Meta and SysEx
events, evaluates the complete tempo map, and emits a deterministic appended
`GH1 SINGER FACE` track: zero-length pitch 108 events open the mouth and
zero-length pitch 109 events close it. The original MIDI byte prefix is
unchanged; only the header track count and appended `MTrk` are new.

The full clean-target sweep parses 59 `.voc` files: eight revision-1200 and 51
revision-1500 archives. Fifty-eight have paired song MIDIs and produce 1,283
open spans; the remaining `_blinktrack` archive has no song MIDI and is
reported rather than fabricated. Focused real fixtures cover both revisions:
`HeartShapedBox` produces 17 spans and `YouReallyGotMe` produces 14.

Two additional deterministic target config patches complete the event route.
`midi_parsers.dtb` gains a `gh1_singer_face_parser` for the generated track;
pitch 108 sends `singer_face_open` and pitch 109 sends
`singer_face_close` through the existing `singer_parser` `MsgSource`.
`char_objects.dtb` adds matching handlers to the stock singer `Character`
type, forwarding them to its local `lip.servo`. This follows the target
message-source contract: an unhandled message is exported to subscribed
sinks. The generated encrypted outputs reparse and reserialize exactly:
`gh1-face-midi_parsers.dtb` is 20,335 bytes with 28 roots and
`gh1-face-char_objects.dtb` is 46,976 bytes with 23 roots.

The clean-target `FaceFxLipSyncServo` rows are not a hidden Morph bridge.
Across all 42 clean bodies there are 72 target rows, and every row is one of
four eye inputs: property type 0 for `L-eyeX`/`R-eyeX` or type 2 for
`L-eyeZ`/`R-eyeZ`. The target routine at
`SLUS_214.47:0x001901D0` switches only on values 0, 1, and 2, reads the
corresponding transform rotation component from the referenced object, and
feeds that scalar into the named FAC graph node. FAC/viseme setup occurs at
`0x0018CE18`, and the runtime evaluation path begins at `0x0018D3F8`.
Consequently, assigning a Morph object to a stock target row would be
incorrect; a native event/animation control path or a complete equivalent FAC
graph must be recovered.

## GH1 eye-controller lowering

All eight authored GH1 guitarists use the same `create_eyes` facts:
`parent=bone_head.mesh`, `constraint=0.925`, and `lid_lower=0.5`. These values
are compiled into the character manifest rather than inferred from mesh names.
The controller implementation was recovered from `SLUS_212.24`:

- `0x0018DBF0` allocates the 0x110-byte legacy controller;
- `0x0018FE18` resolves `L-eye.mesh`, `R-eye.mesh`, `parent`,
  `constraint`, and `lid_lower`;
- the constraint is stored at `+0xF4`, while
  `sqrt(1 - constraint^2)` is stored at `+0xF0`;
- `0x00190110..0x001906B4` uses those two values to clamp the desired eye
  direction to the authored circular cone;
- the 0x1C-byte helper at `+0xD4` is initialized from `lid_lower`, zero,
  0.075, and 0.5, but is never read or passed by the controller's poll,
  start, or stop paths. Its constructor only initializes its local fields and
  vtable; it does not register the helper with a scheduler.

That last point accounts for `lid_lower` without inventing a target behavior:
it is authored but dormant in the shipped GH1 implementation. GH2
`CharEyes` revision 3 cannot serialize lid descriptors anyway; its old-load
path contains only the `CharLookAt` list and one legacy transform reference.

The native lowering creates two revision-2 `CharLookAt` objects and one
revision-3 `CharEyes`. The eye mesh references come from exact resolved
directory entries, each eye mesh must already have the authored parent, the
empty target is the stock GH2 `CharEyes::NextLook` route, and half-time is
zero because the GH1 poll has no temporal smoothing. The source cone cosine is
converted systemically to symmetric target limits:

`limit_degrees = acos(constraint) * 180 / pi`

For the packed value 0.925 this is 22.3316 degrees on yaw and pitch. The
converter rejects a non-finite/out-of-range cosine, a cone wider than GH2's
80-degree `CharLookAt` representation, a missing parent, or an eye mesh whose
decoded parent disagrees with `create_eyes`. All eight packed guitarist
packages pass these checks; no eye-side, character-name, or offset-specific
rule participates.

Grim's two authored knee-to-cloak rigs now use the target
`CharIKRod::SyncBones` contract directly. The converter interpolates the two
authored endpoint world transforms, constructs the vertical/side-axis rod
frame, and serializes
`destination_world * inverse(rod_frame)` as the controller offset. The result
independently matches the retail GH2 Grim offsets to the source-export float
precision; for example the generated right-knee translation is
`(-2.49629,-0.127955,0.125637)` versus the retail port's
`(-2.49618,-0.127943,0.125631)`. No controller-specific offset is stored.

The packed sweep currently produces 13 deterministic model packages. The
three performers without authored face packages have no unresolved model
dependency; the ten face-bearing packages report only that their generated
face-control configuration must be installed beside them. The corresponding
Rnd, MIDI-parser, and Character config outputs and singer MIDI translation are
now deterministic.

The loose bundle builder owns that dependency explicitly. It emits 13 model
MILOs under `char/<package>/og/gen`, the 38 animation MILOs owned by those
character archetypes under `char/<package>/anims/gen`, and the three patched
target configs under their native `system/run/milo/gen`, `config/gen`, and
`char/gen` paths. The remaining generated animation package belongs to the
crowd archetype and is deliberately reserved for the venue/crowd bundle rather
than assigned to a character by a name rule. A sorted 54-row manifest records
every character/config output and its source fact. The separately completed
singer-face sweep can add 58 translated song MIDIs at their native
`songs/...` paths, but those song-specific files are outside the final
MILO/DTB/ACP bundle accounting.

Two complete final rebuilds produce identical hashes for every character
output. Model inventory is 13 complete, zero incomplete; face-bearing model
rows list their generated config requirement separately from unresolved
dependencies. Native runtime proof loads all four performer roles from the
converted GH2-layout packages.
Every generated root also now carries the stock GH2 runtime type implied by
the authored GH1 role: `guitarist`, `singer`, `bassist`, `drummer`, or
`keyboardist`. This is a role-schema translation, not a package-name rule.

Instrument integration follows the target contract rather than attempting to
reinterpret GH1 instrument objects. Every guitarist package now carries the
stock GH2 `guitar.outfit` loader (`../../../og`, type `guitar`, one 58-row
category), `fret.ik` targeting the external `bone_fret.mesh`, and the
`left.weight`/`right.weight` setters targeting `main.drv` with flags
`0x00400000`/`0x00800000`. The drummer package carries the stock
`drums.outfit` loader (`../../../og`, type `drummer`, one eight-row category
with selected/shown states 1/1). These are role-derived target-schema facts
from `char_objects.dta` and the exact clean target bodies, not character-name
rules. The unexplained empty `crash_static.filt` placeholder is deliberately
not synthesized: instruments are cursory and no source fact establishes that
the placeholder is required for native character loading.

Every generated internal reference is parsed from its owning target object
schema and resolved by name before serialization. The audit now covers both
transform target/parent slots plus the reference fields in camera, flare,
light/environment, transform/material/mesh/particle animation, multi-mesh,
movie, font/text, particle bounce/emitter/relative-parent, hair, FaceFx,
WorldFx, trigger, material, mesh, group, Morph, and character-controller
bodies. File paths, event symbols, and discarded legacy strings are not
misclassified as in-directory object references. Focused synthetic coverage
independently exercises nested
shadow-root discovery, unreachable duplicate-skeleton omission, role-based
shadow reference rewriting, exact-topology Morph binding, and dangling
reference rejection, including TransAnim and particle owner/target edges. It
also verifies the two-pose transition graph, exact
three-frame half-cosine samples, filter/trigger wiring, generated controller
type, DTA compilation, and patched-DTB round trip. The packed sweep accounts
for 16,637 internal references
with no
dangling object, material, texture, geometry-owner, parent, bone, group, LOD,
shadow, morph, or controller reference. The standard GH2 instrument hand/fret
anchors and animation-package paths are the only intentional external
references.
The machine-readable ledger is
`tools/milo_convert/build/gh1-to-gh2-conversion-audit.tsv.models.tsv`.

## Native venue, script, archive, and runtime closure

Venue conversion is a persistent namespace translation, not a runtime cache.
Each retail `venues/<name>/gen` directory becomes
`world/gh1_<name>/gen`. The prefix preserves GH2 as the authoritative world,
gameplay, HUD, and UI corpus and prevents collisions such as GH1 and GH2 both
owning an `arena`. The venue manifest contains 55 rows:

- 26 converted revision-24 MILO directories;
- 21 support files, including each venue's campaths, sequence,
  sound, and event data;
- seven compiled native venue scripts; and
- the one shared native crowd-animation package.

The seven script conversions contain 886,557 bytes and zero blocked handlers.
The compiler lowers GH1 object handlers, state variables, delayed tasks,
section/excitement messages, `switch_anim`, `switch_anim_rt`, and object
messages into deterministic target DTBs. Retail executable evidence fixes the
animation timing contract: `switch_anim` is at `SLUS_212.24:0x0016C6A0`,
`switch_anim_rt` at `0x0016C828`, and their shared helper at `0x0017A1D0`.
Normal timing is 480 frames per beat with blend divided by 480; real-time
timing is 1,000 frames per second with blend divided by 1,000. Scale-only
messages retain the current range/type. No venue or animation name selects a
different lowering rule.

GH1 also passes finite `switch` expressions as `foreach` collections. The
compiler preserves the dynamic selector while distributing the loop body into
each concrete branch, so target objects come only from authored branch values;
the `switch` syntax and selector variable never become bogus object targets.

Native Group fixup classifies every member by the target directory type and
rebuilds the animatable-child graph before events run. Group animation
recursively propagates to transform, material, environment, light, and
particle animations. A source-present empty Group and a channel-empty MatAnim
resolve as documented no-ops, rather than dangling targets. Direct EnvAnim and
LightAnim targets use the same event route. The `finish_loading` section is a
load barrier: latched intro/music/excitement events replay only after the
lighting directory exists.

Every GH1 VenueCam record is converted to a native revision-20 GH2 CamShot.
Camera path, offset, singer-screen compensation, FOV, timing domain, ease,
shake, and supported behavior fields are evaluated into native target keys;
the source `camera.dtb` is not shipped beside those objects. Native CamShots
are authoritative at runtime, with the old DTB reader retained only as a
legacy fallback when no native camera keys exist.

CamShot `ObjectFields` use the real HMX `TypeProps` wire contract recovered
from `TypeProps.cpp`: one flat DataArray whose roots alternate
`Symbol key, DataNode value`. A value may itself be an array, but a property
is not wrapped in a nested pair array. Converter output, the object inspector,
and runtime parsing all use that contract; the runtime accepts the earlier
nested-pair form only to read old generated loose assets.

Target rendering also preserves the source submission boundary. A Mesh with no
material is a transform/editor helper and is not submitted by the platform
renderer. This rule removes Arena's two large helper planes without naming
those meshes or hiding geometry. Tex8-to-Tex10 retains the same-platform PS2
bitmap bytes, and Mat21 stages become linked Mat27 `nextPass` objects, so
texture alpha, blend, palette, swizzle, and stage order are not reconstructed
from screenshots.

The complete bundle is described by two manifests:

| Manifest | Rows | Payload bytes |
|---|---:|---:|
| `gh1-character-bundle.tsv` | 54 | 24,558,784 |
| `gh1-venue-bundle.tsv` | 55 | 14,302,943 |

Including the two manifests, the unique deployable set is 111 files and
38,872,235 bytes. Two complete clean-target conversions produce identical
163-row output ledgers with SHA-256
`0722D07FBA11E1BFCF9813FD5ABC065FA0F87FC69708B140892FAD7689D63269`.
The verification archive overlay reuses all 54 character and 55 venue
manifest rows and appends zero bytes.

The packed audit closes 105/105 source directories and 926/926 ACPs:
13,115 converted objects, 625 source-derived target objects, zero blocked,
13/13 character packages, 79 controllers, 31 animation sets, 926 animations,
25 ACG assets with 29,808 nodes, 26 venue assets, and 12,979 venue references.
Venue camera conversion emits 201 native CamShots containing 181,864
keyframes with zero blocked records. The seven-venue native runtime sweep
loads native world, script, and lighting paths for every venue with zero
legacy raw-script loads, unsupported operations, or unresolved targets.

The final mixed runtime proof loads `alterna`, `metal_singer`, `metal_bass`, and
`metal_drummer` from ordinary `char/<model>/og/gen/<model>.milo_ps2` paths with
`layout=GH2`, while playing GH2 `shoutatthedevil` in
`world/gh1_small_club`. It starts on native `Intro01`, transitions to the
regular native `flr_near_lft01` CamShot, and finishes 18 seconds of gameplay
without a compatibility-script load, unsupported operation, unresolved
animation target, or performer-load failure. Proof data and the linked video
are under
`proofs/gh1-native-conversion-final/native-format-camera-final/`.

## Conversion-blocking gap register

1. **G1 — Clean GH2 provenance — closed.** Redump-identified GH2 USA
   `SLUS-21447` is MD5
   `317968cd573b183f3c697bb94e8c8ee6`; extracted `MAIN.HDR` and `MAIN_0.ARK`
   hashes are recorded above. The pre-overlay header and immutable original ARK
   byte range preserve that corpus after deployment.
2. **G2 — ARK writer — closed.** Header, strings, paths, file entries, declared
   part size, physical append placement, and entry reuse are deterministic for
   the retail one-part PS2 archive. Loose and overlay output consume identical
   manifests; the second overlay appends zero bytes.
3. **G3 — Directory framing — closed.** GH1 revision-10 and GH2 revision-24
   roots/children, allocation metadata, body boundaries, external resources,
   recursive package contents, and cross-directory references reparse with
   complete residual accounting.
4. **G4 — Type/revision inventory — closed.** The complete packed GH1
   inventory is closed at 12,189/12,189 bodies across 21/21 observed rows,
   with exact inheritance order, byte consumption, semantic round trips, and
   source-to-target revision mappings. Mesh25-to-Mesh28, Tex8-to-Tex10, and
   Mat21-to-Mat27 are closed, including all 3,140 clean-target Mat27 bodies.
5. **G5 — Object writers — closed.** All 21 observed source rows and every
   required target body have revision-aware semantic serializers with
   edit/reparse coverage. Complete character, attachment, prop, animation, and
   venue packages are emitted rather than runtime object substitutions.
6. **G6 — Reference graph — closed.** Schema-typed fixups account for nulls,
   externals, proxies, owners, parents, targets, materials, textures, bones,
   controllers, groups, and recursive directories. Character and venue sweeps
   contain zero dangling references and no string-search fallback.
7. **G7 — Character contract — closed.** The packed animation contract has
   31 authored sets, 926 ACP clips, 25 ACG assets/29,808 nodes, all authored
   transitions, skeleton-channel resolution, and 39 deterministic native
   revision-24 clip-set packages. Thirteen model packages close skeleton,
   skinning/order, twist/IK/controller semantics, faces, eyes, collide,
   cuff/sleeve, attachments, props, shadows, and events without
   identity-specific corrections.
8. **G8 — Venue contract — closed.** All seven venues convert geometry,
   Groups/Views, transforms, cameras/campaths, lighting/environments, material
   animation, particles, crowds, props, scripts/events, and draw state. The
   native sweep has zero unresolved or unsupported rows.
9. **G9 — ACP/ACG/ACS — closed.** ACP and ACG have lossless readers/writers;
   revision-18 ACP framing, its nested revision-5 SampleSet contract, ACG
   version-1 framing/transition semantics, and recursive ACS/DTB macro
   expansion are closed. All 926 packed ACPs convert deterministically to
   native revision-10 GH2 clips with exact set, flag, timeline, allocation,
   transition, and export-marker semantics.
10. **G10 — Texture/material packing — closed.** Same-platform HMX bitmap
    payloads, PS2 palette/swizzle bytes, straight-alpha semantics, Mat27
    `nextPass` lowering, animation routing, and all texture/material references
    are retained deterministically.
11. **G11 — Native conversion sweep — closed.** Repeated complete sweeps emit
    identical 163-row conversion ledgers and the same
    111-file/38,872,235-byte deployable set. All models, animations,
    controllers, config patches, song face tracks, and venues load through
    ordinary target paths; zero converted object is blocked.
12. **G12 — Runtime proof — closed.** GH2 remains authoritative for gameplay,
    highway, HUD, and UI while GH1 characters and venues are independently
    selected. The final native video, screenshots, seven-venue matrix, camera
    proof, and deployment hashes are linked from the proof manifest.

## Completion gates

All gap-register entries are closed. The corresponding machine-readable gates
are:

1. Full GH1 and clean GH2 packed inventories with source hashes.
2. Byte-exact no-edit round trips for every supported source file.
3. Semantic edit/reparse tests for every field and reference category.
4. Zero unclassified object revisions and zero unexplained residual bytes.
5. Zero dangling, name-fallback, or asset-specific fixups.
6. Deterministic rebuild hashes from two complete clean-target passes.
7. Complete GH1 character and venue conversion sweeps.
8. GH2-native load with GH1 compatibility decoders disabled.
9. Mixed-content gameplay and visual/video proof.
10. Deployed assets and a linked final proof manifest.

All ten gates are recorded in
`proofs/gh1-native-conversion-final/FINAL_PROOF_MANIFEST.md`.
