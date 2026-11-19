import arcana_tarot as arcana

library = arcana.deck_library()

# load first deck in library
handle = library.decks()[0]
deck = arcana.load_deck(handle.path)

# print every card in the deck
for card in deck.cards:
    print(card.display_name)
