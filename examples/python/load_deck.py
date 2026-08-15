# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

"""Load one deck and print what it is made of.

    python load_deck.py                                 # the first deck in the XDG library
    python load_deck.py ~/decks rider-waite-smith       # a root and a directory name
"""

import sys

import arcana_tarot as arcana


def main(argv: list[str]) -> int:
    roots = [argv[1]] if len(argv) > 1 else []

    # Name files are read in this order, falling back to English.
    library = arcana.deck_library(arcana.library_options(roots=roots, languages=["en"]))

    if not library.decks():
        print("no decks found")
        return 1

    directory_name = argv[2] if len(argv) > 2 else library.decks()[0].directory_name

    try:
        # A deck is loaded by its directory name, not by [deck].id: ids are not
        # unique across a library, and find_all_by_id() returns every deck
        # carrying one.
        #
        # Loads are cached, so asking twice hands back the same object. The deck
        # owns itself and outlives the library it came from.
        deck = library.load(directory_name)
    except arcana.deck_error as problem:
        # Everything that can fail raises deck_error, which carries a `code` to
        # branch on and a `message` to show.
        print(f"could not load {directory_name}: {problem.message} ({problem.code})")
        return 1

    metadata = deck.metadata
    print(metadata.name, metadata.version)
    print("  id            ", metadata.id)
    print("  schema_version", metadata.schema_version)
    print("  artist        ", metadata.artist or "(unset)")
    print("  license       ", metadata.license or "(unset)")
    print("  aspect_ratio  ", metadata.aspect_ratio)
    print("  root          ", deck.root_path)

    # The 78 standard cards, minus [excluded_cards], plus [custom_cards].
    majors = deck.cards_of_kind(arcana.arcana_kind.major_arcana)
    print(f"{len(deck.cards)} cards, {len(majors)} major")

    # Standard suits first, then any custom suit this deck declares. A deck may
    # rename a suit or a court rank, so print the display name, never the key.
    for suit in deck.suits():
        excluded = " (excluded)" if suit.excluded else ""
        cards = deck.cards_in_suit(suit.key)
        print(f"  suit {suit.key:10} {suit.display_name:12} {len(cards):2} cards{excluded}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
