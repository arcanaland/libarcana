import arcana_tarot as arcana

library = arcana.deck_library()

for handle in library.decks():
    deck = arcana.load_deck(handle.path)
    print(f"{deck.metadata.name}: {len(deck.cards)} cards")

