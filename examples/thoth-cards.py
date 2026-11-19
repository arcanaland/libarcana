# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

import arcana_tarot as arcana

library = arcana.deck_library()

matches = library.find_all_by_identifier("cards.wands/deck/thoth")
deck = arcana.load_deck(matches[0].path)

for card in deck.cards:
    name = card.display_name
    id = card.id.to_canonical()
    print(f"{id} ({name})")
