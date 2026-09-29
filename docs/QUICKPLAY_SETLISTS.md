# Quickplay setlists

Quickplay shows one setlist bucket at a time. Yellow cycles to the next bucket
and resets the song selection to its first song. Up/Down navigates songs inside
that bucket; it never changes buckets. Section headers are not selectable.
The active bucket name remains visible above the scrolling list.

Controls, in footer order:

- Green / Defaults: launch using the song's character and guitar, and the
  section's default venue.
- Red / Back: return to the previous screen.
- Yellow / Setlist: switch buckets, wrapping after the last one.
- Blue / My Band: launch with saved Manage Band character, guitar and venue
  preferences. Missing preferences fall back to the song/section defaults.
- Up/Down / Songs: move through the current bucket.

Backing-band preferences remain active in both launch modes. There is no
Orange action or per-song character/guitar/venue selection screen.

## JSON structure

The game reads `Setlists/*.json` relative to its working directory after loading
the song and DLC catalogs. `GHOGX_SETLISTS_DIR` overrides the directory for tests.
Files are ordered by filename; buckets, sections and songs follow array order.
Restart the game after editing a file.

```json
{
  "schema_version": 1,
  "setlists": [
    {
      "id": "example_gh1",
      "label": "GH1 Favorites",
      "sections": [
        {
          "label": "Small Club",
          "default_venue": "gh1_small_club",
          "songs": ["ironman", "morethanafeeling"]
        }
      ]
    },
    {
      "id": "example_80s",
      "label": "80s Favorites",
      "sections": [
        {
          "label": "Finale",
          "default_venue": "arena",
          "songs": ["caughtinamosh", "playwithme"]
        }
      ]
    }
  ]
}
```

Use catalog song IDs and installed venue IDs, not display names or file paths.
Every section requires `label`, `default_venue`, and a `songs` array. A song may
appear in different buckets, but only once within an individual bucket.
Bucket IDs must be unique across these files. Invalid files are rejected as a
whole and logged with their filename and reason; earlier valid files remain.

Songs from uninstalled optional discs are omitted, along with empty sections
and buckets. A venue must exist whenever its section has installed songs.
Unrecognized song IDs are also omitted: check spelling if a song is absent.

GH2's stock campaign and bonus songs form the first bucket automatically.
An explicit bucket with ID `gh2` replaces that automatic definition. Existing
DLC manifest `setlists` remain supported; songs already assigned in these JSON
files are not repeated in those legacy buckets. Unassigned Quickplay songs go
into one implicit `DLC` bucket. The ID `dlc` is reserved for that bucket.

## Setup integration

[`Setlists/10-disc-setlists.json`](../Setlists/10-disc-setlists.json) defines
**both GH1 and GH80s in one file**: 47 GH1 songs in seven sections and 30 GH80s
songs in six sections. Campaign order and song IDs were read from the local
USA source discs. GH1 uses the installed converted `gh1_*` venues; GH80s uses
its stock GH2 venue IDs. GH1 bonus songs use `gh1_big_club` as their section
default. Optional disc imports supply the actual song content separately.

First-Time Setup installs the shared file in the game's `Setlists` directory.
Running setup again preserves existing user edits and records
`preserved_user_file` in its audit. The frozen setup builder bundles `Setlists`
alongside its other resources. No game images need to be rebuilt to edit the
setlist organization.

The current local install reuses existing imported GH1/GH80s song packages
through directory junctions from its `DLC` directory to
`four-disc-acceptance-20260901-final/DLC`. Keep those existing source packages
in place. A future normal setup installs the packages directly from the chosen
user-owned media and does not require these development junctions.
