# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

"""Find a card and the file on disk you would draw for it.

    python card_assets.py ~/decks rider-waite-smith
"""

import sys

import arcana_tarot as arcana


def show(deck: arcana.deck, card_id: arcana.card_id) -> None:
    # None when the deck excludes the card or never defined it. Asking for a
    # card is how you find out; do not assume 78.
    card = deck.find_card(card_id)
    if card is None:
        reason = deck.exclusion_reason(card_id.to_canonical()) or "not declared"
        print(f"{card_id.to_canonical()}: not in this deck ({reason})")
        return

    print(f"{card.canonical_id()} — {card.display_name}")
    if card.alt_text:
        print("  alt text:", card.alt_text)

    # Every image the loader resolved for this card, across every image
    # directory the deck ships.
    for image in card.images:
        print(f"  {image.kind.name:8} {image.source_dir:8} {image.path}")

    # Pick by what you are drawing into rather than by directory name: a deck is
    # free to ship h300 and h1200, or only one of them.
    raster = card.best_raster_for_height(600)
    if raster is not None:
        print(f"  for a 600px slot: {raster.path} ({raster.height}px tall)")

    # Prefers the largest that still fits, so a terminal never clips.
    ansi = card.best_ansi_for_lines(40)
    if ansi is not None:
        print(f"  for a 40-line terminal: {ansi.path} ({ansi.lines} lines)")

    scalable = card.scalable_image()
    if scalable is not None:
        print("  scalable:", scalable.path)


def main(argv: list[str]) -> int:
    roots = [argv[1]] if len(argv) > 1 else []
    library = arcana.deck_library(arcana.library_options(roots=roots))

    if not library.decks():
        print("no decks found")
        return 1

    directory_name = argv[2] if len(argv) > 2 else library.decks()[0].directory_name

    try:
        deck = library.load(directory_name)
    except arcana.deck_error as problem:
        print(f"could not load {directory_name}: {problem.message}")
        return 1

    # The three ways to name a card.
    show(deck, arcana.card_id.standard_major(0))
    show(deck, arcana.card_id.standard_minor(arcana.suit.cups, arcana.rank.ace))
    show(deck, arcana.card_id.parse("minor_arcana.swords.king"))

    # Deterministic in the seed, so a shuffle is reproducible.
    drawn = deck.random_card(42)
    if drawn is not None:
        print("card for seed 42:", drawn.display_name)

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
