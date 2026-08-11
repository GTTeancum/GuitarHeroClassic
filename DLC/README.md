# DLC packages

Place each package in `DLC/<package-id>/manifest.json`. The runtime loads the
base GH2 DTBs first, then manifests in deterministic folder-name order. Set
`GHOGX_ADDONS_DIR` only when an installation needs a different package root.

```text
DLC/
  package.id/
    manifest.json
    content/
      char/...
      config/...
      songs/...
      track/...
      ui/...
      world/...
```

`content/` mirrors virtual ARK paths. A file at
`content/char/example/og/gen/example.milo_ps2` is therefore requested as
`char/example/og/gen/example.milo_ps2` by the normal loader. Loose mounts are
process-wide, so menus, gameplay, textures, audio, songs, characters, venues,
and instruments all use the same lookup contract without modifying the user's
ARK.

Schema version 1 supports any combination of `characters`, top-level
`outfits`, `guitars`, top-level `finishes`, `venues`, `songs`, and `setlists`
in one manifest. A package can be one new character, an outfit expansion for a
built-in character, a venue, one song, a setlist pack, or a combined release.
Character outfits may opt into skeleton retargeting with
`animation_source_model`, `retarget_animation`, and role-specific
`guitarist_hidden_roots`.

Package IDs, mounted paths, selection IDs, and catalog IDs must be unique.
Replacing an existing ARK path is rejected unless that exact normalized path is
listed in `replaces`. Every referenced character asset must resolve either in
the package content tree or the base archive. A manifest is transactional: if
any later row is invalid, none of that package's earlier catalog or table
changes remain active. Standard JSON string escapes (including Unicode
surrogate pairs) and exponent-form numbers are accepted.

New character and outfit descriptions are blank when omitted; the runtime does
not fabricate localization keys. Character order is base DTB order followed by
successfully loaded packages. Outfit order is manifest order, so place the
default outfit first. The built-in singer package uses GH2 first and GH1
second.

See `examples/everything/manifest.json` for the complete field layout. The
example folder is documentation only; copy it to a direct child of `DLC/` and
provide its referenced content before enabling it.
