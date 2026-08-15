# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

"""Every deck installed on this system.

    python list_decks.py                # the XDG deck library
    python list_decks.py ~/decks        # a library root of your own
"""

import sys

import arcana_tarot as arcana


def main(argv: list[str]) -> int:
    # An empty `roots` means the XDG library, $XDG_DATA_HOME/tarot/decks.
    roots = [argv[1]] if len(argv) > 1 else []

    print("searching", arcana.deck_library_path())

    # Scanning happens in the constructor: it reads each deck.toml, and nothing
    # else. No images are touched.
    library = arcana.deck_library(arcana.library_options(roots=roots))

    for root in library.roots():
        print("  root", root)

    # Sorted by directory name. Roots are searched in order like PATH, so a
    # directory name found in an earlier root shadows a later one.
    for deck in library.decks():
        print(f"{deck.directory_name:24} {deck.name:32} v{deck.version:8} {deck.card_count:3} cards")
        print(f"{'':24} id={deck.id} at {deck.path}")

    # A directory whose manifest could not be read is reported rather than
    # dropped, so a broken deck is visible to the user instead of missing.
    for deck in library.malformed_decks():
        print(f"malformed {deck.directory_name}: {deck.problem.message}")

    print(f"{len(library.decks())} readable, {len(library.malformed_decks())} malformed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
