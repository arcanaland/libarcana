# libarcana documentation

> **TODO(adam): this page is unwritten.**
>
> Its prose was generated and has been removed under
> [AI-POLICY](https://github.com/arcanaland/arcanaland/blob/main/AI-POLICY.md) §2 — text
> meant for a human to read is written by a human. The outline below is the plan, not the
> page. The runnable programs in [`examples/`](../examples) are the working reference in the
> meantime, and CI builds and runs them, so they cannot rot.

- [C++ quickstart](quickstart-cpp.md)
- [Python quickstart](quickstart-python.md) — `pip install arcana-tarot`

## TODO(adam): the model in one page

Points this section is meant to land, in roughly this order:

- Deck directory, `deck.toml`, name files; a library is roots searched in order like `PATH`,
  default `$XDG_DATA_HOME/tarot/decks`.
- Directory name vs. `[deck].identifier` — you load by the former; the latter is not unique,
  and `find_all_by_identifier` returns every deck carrying one. A 1.0 deck never has one.
- Scanning reads manifests only and yields a `deck_summary`; nothing touches an image until
  you load. An unparseable directory surfaces as a malformed deck rather than vanishing.
- A deck's cards are 78 minus `[excluded_cards]` plus `[custom_cards]` — do not assume 78,
  and do not assume any given card exists.
- Images come in `scalable` / `raster` / `ansi`; pick with `best_raster_for_height`,
  `best_ansi_for_lines`, `scalable_image`, never by directory name.
- `card_id` is canonically `major_arcana.00` / `minor_arcana.cups.ace` and round-trips
  through `to_canonical()` / `parse()`.
- Display names belong to the deck: print a card's `display_name`, a suit's `name`, and
  `display_suit_name` / `display_rank_name` — never the lowercase key.

## TODO(adam): deck versions

`[deck].schema_version` is required and the loader dispatches on the declared major: v1.0
through a frozen compatibility path, v2.0 (still a draft) through its own. Both normalize
into one model, so nothing above the loader branches on the version. `CONFORMANCE.md` in the
`agents/` knowledge-base tracks obligations against what is implemented.

## TODO(adam): validation

The C++ API judges a deck against a catalogue of diagnostic rules — see
[`examples/validate_deck.cpp`](../examples/validate_deck.cpp). The catalogue is deliberately
ahead of the implemented checks, so its size is not a count of what actually gets judged.
Validation is **not** exposed in the Python binding.
