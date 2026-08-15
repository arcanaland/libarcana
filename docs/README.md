# libarcana documentation

`libarcana` reads tarot decks packaged to Arcana Land's
[deck specification](https://github.com/arcanaland/specifications). It finds the decks
installed on a system, parses their manifests, and tells you which file on disk to draw for
a given card. It does not decode images, render anything, or deal spreads.

- **[Python quickstart](quickstart-python.md)** — `pip install arcana-tarot`
- **[C++ quickstart](quickstart-cpp.md)** — `find_package(arcana)`, `arcana::arcana`

The runnable versions of everything in those two pages live in
[`examples/`](../examples): four C++ programs and three Python scripts, each taking a
library root on the command line. They are built and run in CI against the reference decks,
so a snippet that has rotted fails the build rather than the reader.

```bash
just build && ./build/RelWithDebInfo/examples/list_decks ~/decks
python examples/python/list_decks.py ~/decks
```

## The model in one page

A **deck** is a directory holding a `deck.toml` manifest, image directories, and optional
name files. A **library** is a list of root directories that hold deck directories, searched
in order like `PATH`; the default root is `$XDG_DATA_HOME/tarot/decks`.

Two names identify a deck and they are not interchangeable. Its **directory name** is what
you load it by, and it is unique within a library because a name found in an earlier root
shadows the same name in a later one. Its **`[deck].id`** is a label the packager chose,
and two installed decks may well carry the same one — `find_all_by_id` returns all of them.

Scanning a library reads manifests only, producing a `deck_summary` per deck; nothing
touches an image until you load. A directory whose manifest will not parse is reported as a
**malformed deck** rather than dropped, so a broken install is visible instead of missing.

A loaded **deck** holds its metadata and its **cards** — the 78 standard cards, minus
whatever `[excluded_cards]` removes, plus whatever `[custom_cards]` adds. Do not assume 78,
and do not assume a card exists: ask, and handle the empty answer.

A **card** carries a `card_id`, display strings, and every **image** the loader resolved for
it. Images come in three kinds — `scalable` (SVG), `raster` (`h300`, `h1200`, … named by
pixel height) and `ansi` (`ansi20`, `ansi32`, … named by terminal lines). Pick by the space
you are drawing into, using `best_raster_for_height` / `best_ansi_for_lines` /
`scalable_image`, rather than by directory name: a deck is free to ship any subset.

A `card_id` is canonically `major_arcana.00` or `minor_arcana.cups.ace`, round-trips through
`to_canonical()` / `parse()`, and is also constructible from its parts.

Display names are the deck's business. A deck may rename a suit or a court rank, so print
`display_name` and `display_suit_name` / `display_rank_name`, never the lowercase key.

## Deck versions

Decks declare `[deck].schema_version`. What ships today implements **deck spec v1.0**;
v2.0 is a draft and the loader does not yet dispatch on the declared version. See
`CONFORMANCE.md` in the `agents/` knowledge-base for what the specification obliges against
what is implemented.

## Validation

The C++ API also judges a deck against a catalogue of diagnostic rules — see
[`examples/validate_deck.cpp`](../examples/validate_deck.cpp) and
[the C++ quickstart](quickstart-cpp.md#validating-a-deck). The catalogue is written from the
specification and is deliberately ahead of the checks, so its size is not a count of what
gets judged. Validation is **not** exposed in the Python binding.
