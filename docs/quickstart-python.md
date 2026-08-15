# Python quickstart

```bash
pip install arcana-tarot
```

The distribution is **`arcana-tarot`**; the module you import is **`arcana_tarot`**. They
differ because `arcana` was already taken on PyPI. Requires Python 3.12+; the published
wheels are manylinux x86-64 (`abi3`, built against 3.12), so other platforms build from the
sdist and need a GCC that speaks C++23.

```python
import arcana_tarot as arcana

print(arcana.__version__)
```

Every runnable script quoted below is in [`examples/python/`](../examples/python) and is run
in CI. The API is the C++ API, transliterated: `snake_case` throughout, `std::optional`
arrives as `None`, `std::filesystem::path` as `pathlib.Path`, and anything returning
`std::expected` raises instead.

## Listing the decks on the system

[`examples/python/list_decks.py`](../examples/python/list_decks.py)

```python
from pathlib import Path

import arcana_tarot as arcana

# No roots means the XDG library: $XDG_DATA_HOME/tarot/decks
library = arcana.deck_library()

# Or point it somewhere. Roots are searched in order, like PATH, and take a
# str or a Path. Nothing expands `~` for you.
library = arcana.deck_library(arcana.library_options(roots=[Path.home() / "decks"]))

for deck in library.decks():
    print(deck.directory_name, deck.name, deck.version, deck.card_count, deck.path)

for broken in library.malformed_decks():
    print("malformed:", broken.directory_name, broken.problem.message)
```

```
rider-waite-smith        Rider-Waite-Smith Tarot          v1.1        78 cards
                         id=rider-waite-smith at /home/you/decks/rider-waite-smith
```

Constructing the library does the scan, reading each `deck.toml` and nothing else. Call
`library.refresh()` after decks are installed or removed. `arcana.deck_library_path()` gives
the default root without constructing anything, and `library.find("name")` returns one
summary or `None`.

## Loading a deck

[`examples/python/load_deck.py`](../examples/python/load_deck.py)

```python
library = arcana.deck_library(arcana.library_options(roots=[root], languages=["en"]))

try:
    deck = library.load("rider-waite-smith")   # by directory name, not by [deck].id
except arcana.deck_error as problem:
    print(problem.code, problem.message)       # error_code.not_found, error_code.parse_error, …
    raise

print(deck.metadata.name, deck.metadata.version, deck.metadata.schema_version)
print(len(deck.cards), "cards")

for suit in deck.suits():
    print(suit.key, suit.display_name, len(deck.cards_in_suit(suit.key)))
```

```
Rider-Waite-Smith Tarot 1.1 1.0
78 cards
wands Wands 14
cups Cups 14
swords Swords 14
pentacles Pentacles 14
```

`load` caches, so loading the same deck twice hands back the same object (`first is second`),
and a loaded deck owns itself — it stays valid after the library that produced it is gone.
`load_external(path)` loads a deck directory from anywhere, and the free function
`arcana.load_deck(path, languages=[])` skips the library entirely.

Failures raise `arcana.deck_error`, which carries `.code` (an `error_code`) and `.message`.
Everything that returns `std::expected` in C++ raises here, including
`arcana.card_id.parse`.

## Finding a card and its assets

[`examples/python/card_assets.py`](../examples/python/card_assets.py)

```python
fool = deck.find_card(arcana.card_id.standard_major(0))
ace = deck.find_card(arcana.card_id.standard_minor(arcana.suit.cups, arcana.rank.ace))
king = deck.find_card(arcana.card_id.parse("minor_arcana.swords.king"))

if fool is None:
    print("not in this deck:", deck.exclusion_reason("major_arcana.00"))
else:
    print(fool.canonical_id(), fool.display_name, fool.alt_text)

    for image in fool.images:
        print(image.kind.name, image.source_dir, image.path)

    raster = fool.best_raster_for_height(600)     # smallest at or above 600px
    ansi = fool.best_ansi_for_lines(40)           # largest at or below 40 lines
    svg = fool.scalable_image()                   # None if the deck ships no SVG
```

```
major_arcana.00 The Fool None
ansi ansi32 /home/you/decks/rider-waite-smith/ansi32/major_arcana/00.ansi
raster h1200 /home/you/decks/rider-waite-smith/h1200/major_arcana/00.jpg
for a 600px slot: …/h1200/major_arcana/00.jpg (1200px tall)
```

The paths are absolute and already resolved against the deck root, so they go straight to
whatever opens the file. `find_card` returns `None` for a card this deck excludes or never
declared — that is the answer, not an error.

Other things a deck will answer:

```python
deck.cards_of_kind(arcana.arcana_kind.major_arcana)   # 22 for a standard deck
deck.cards_in_suit("cups")
deck.random_card(42)                                  # deterministic in the seed
deck.suit_aliases, deck.court_aliases                 # what this deck renamed
deck.major_arcana_remap
deck.source_toml()                                    # the manifest, re-serialized
```

## What is not bound

Deck validation, `arcana::paths` beyond `deck_library_path()`, card backs, deck variants and
custom-suit declarations have no Python surface yet. The C++ API has all of them.

## Building the binding from this repo

```bash
just python test     # build the nanobind module and run the pytest suite
just python wheel    # pip install . into a throwaway venv and import it
```
