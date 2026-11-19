# arcana-tarot

`arcana-tarot` is Python library implementing Arcana Land's [Tarot Deck spec](https://github.com/arcanaland/specifications).

## Examples

### List all decks in `~/.local/share/tarot`

```python
import arcana_tarot as arcana

library = arcana.deck_library()

for handle in library.decks():
    deck = arcana.load_deck(handle.path)
    print(f"{deck.metadata.name}: {len(deck.cards)} cards")

```

Output:

```bash
Aquatic Tarot: 78 cards
ASCII Tarot: 78 cards
CBD Tarot de Marseille: 78 cards
This Might Hurt Tarot: 79 cards
Thoth Tarot: 78 cards
Wisdom of Pooh Tarot: 81 cards
```


### Print all cards in the Thoth deck

```python
library = arcana.deck_library()

matches = library.find_all_by_identifier("cards.wands/deck/thoth")
deck = arcana.load_deck(matches[0].path)

for card in deck.cards:
    print(f"{card.display_name} ({card.id.to_canonical()})")
```

Output:

```
major_arcana.00 (The Fool)
major_arcana.01 (The Magus)
major_arcana.02 (The High Priestess)
major_arcana.03 (The Empress)
major_arcana.04 (The Emperor)
major_arcana.05 (The Hierophant)
major_arcana.06 (The Lovers)
major_arcana.07 (The Chariot)
major_arcana.08 (Adjustment)
major_arcana.09 (The Hermit)
major_arcana.10 (Fortune)
major_arcana.11 (Lust)
major_arcana.12 (The Hanged Man)
major_arcana.13 (Death)
major_arcana.14 (Art)
major_arcana.15 (The Devil)
major_arcana.16 (The Tower)
major_arcana.17 (The Star)
major_arcana.18 (The Moon)
major_arcana.19 (The Sun)
major_arcana.20 (The Æon)
major_arcana.21 (The Universe)
[ ... ]
```
