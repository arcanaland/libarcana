# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

from pathlib import Path

import arcana_tarot as arcana
import pytest

from conftest import PNG_HEADER, write_v2_deck


def test_all_is_exactly_the_public_names_of_core() -> None:
    public = {name for name in dir(arcana._core) if not name.startswith("_")}
    assert set(arcana.__all__) == public | {"__version__"}
    assert len(arcana.__all__) == len(set(arcana.__all__))


# --- [deck] -------------------------------------------------------------------


@pytest.fixture
def full_metadata(tmp_path: Path) -> arcana.deck_metadata:
    """A deck declaring every [deck] key the v2 model added."""
    return arcana.load_deck(
        write_v2_deck(
            tmp_path,
            manifest="""
pips = "emblematic"
default_language = "fr"
metadata_language = "de"
artwork_language = ["fr-1694acad", "la"]
card_size_mm = [61.0, 113.5]
packager = "A. Packager"
license_files = ["LICENSE", "LICENSE-art"]
copyright = "(c) 1760 Nicolas Conver"
rights_status = "public-domain"
redistribution = "full"
derivation = "none"
published_date = "1760"
related = [
    { rel = "surrogate_for", target = "net.example/deck/original", title = "The Original" },
    { rel = "companion", target = "net.example/deck/friend" },
]
links = [
    { rel = "homepage", url = "https://example.net/", title = "Home" },
    { rel = "x_forum", url = "https://forum.example.net/" },
]

[deck.product_ids]
isbn = "978-3-16-148410-0"
x_shop = "SKU-1"
""",
        )
    ).metadata


def test_every_new_deck_key_reaches_python(full_metadata: arcana.deck_metadata) -> None:
    m = full_metadata

    assert m.pips == arcana.pip_style.emblematic
    assert m.default_language == "fr"
    assert m.metadata_language == "de"
    assert m.artwork_language == ["fr-1694acad", "la"]

    assert isinstance(m.card_size_mm, arcana.card_size)
    assert (m.card_size_mm.width, m.card_size_mm.height) == (61.0, 113.5)
    assert m.card_size_mm == m.card_size_mm
    assert m.card_size_mm != arcana.card_size()

    assert m.packager == "A. Packager"
    assert m.license_files == ["LICENSE", "LICENSE-art"]
    assert m.copyright == "(c) 1760 Nicolas Conver"
    assert m.rights_status == "public-domain"
    assert m.redistribution == arcana.sharing_terms.full
    assert m.derivation == arcana.sharing_terms.none
    assert m.published_date == "1760"


def test_related(full_metadata: arcana.deck_metadata) -> None:
    surrogate, companion = full_metadata.related

    assert isinstance(surrogate, arcana.deck_relation)
    assert surrogate.rel == arcana.deck_rel.surrogate_for
    assert surrogate.rel_name == "surrogate_for"
    assert surrogate.target == "net.example/deck/original"
    assert surrogate.title == "The Original"
    assert arcana.recognition(surrogate.rel) == arcana.relation_recognition.recognized

    assert companion.rel == arcana.deck_rel.companion
    assert companion.title is None
    assert arcana.recognition(rel=companion.rel) == arcana.relation_recognition.advisory

    assert surrogate == surrogate
    assert surrogate != companion

    # The whole registry, so every enumerator is read once
    assert arcana.recognition(arcana.deck_rel.expands) == arcana.relation_recognition.recognized
    for rel in (arcana.deck_rel.pattern, arcana.deck_rel.other):
        assert arcana.recognition(rel) == arcana.relation_recognition.advisory


def test_links(full_metadata: arcana.deck_metadata) -> None:
    home, forum = full_metadata.links

    assert isinstance(home, arcana.deck_link)
    assert home.rel == arcana.link_rel.homepage
    assert home.rel_name == "homepage"
    assert home.url == "https://example.net/"
    assert home.title == "Home"

    # An unregistered rel keeps its token
    assert forum.rel == arcana.link_rel.other
    assert forum.rel_name == "x_forum"
    assert forum.title is None

    assert home == home
    assert home != forum


def test_product_ids(full_metadata: arcana.deck_metadata) -> None:
    isbn, shop = full_metadata.product_ids

    assert isinstance(isbn, arcana.product_id)
    assert isbn.scheme == arcana.product_scheme.isbn
    assert isbn.scheme_name == "isbn"
    assert isbn.value == "978-3-16-148410-0"

    assert shop.scheme == arcana.product_scheme.other
    assert shop.scheme_name == "x_shop"
    assert shop.value == "SKU-1"

    assert isbn == isbn
    assert isbn != shop


def test_effective_metadata_language(tmp_path: Path) -> None:
    def language(manifest: str, name: str) -> str:
        deck = arcana.load_deck(write_v2_deck(tmp_path, name, manifest=manifest))
        return arcana.effective_metadata_language(metadata=deck.metadata)

    assert language("", "bare") == "en"
    assert language('default_language = "fr"', "default") == "fr"
    assert language('default_language = "fr"\nmetadata_language = "de"', "both") == "de"


def test_a_deck_declaring_none_of_the_keys(tmp_path: Path) -> None:
    deck = arcana.load_deck(write_v2_deck(tmp_path))
    m = deck.metadata

    assert m.related == []
    assert m.pips == arcana.pip_style.unstated
    assert m.default_language is None
    assert m.metadata_language is None
    assert m.artwork_language == []
    assert m.card_size_mm is None
    assert m.packager is None
    assert m.license_files == []
    assert m.copyright is None
    assert m.rights_status is None
    assert m.redistribution == arcana.sharing_terms.unstated
    assert m.derivation == arcana.sharing_terms.unstated
    assert m.published_date is None
    assert m.product_ids == []
    assert m.links == []

    assert deck.strings is None
    assert deck.minor_arcana_name_template is None


def test_every_enumerator_is_bound() -> None:
    expected = {
        arcana.pip_style: ["unstated", "scenic", "emblematic"],
        arcana.sharing_terms: ["unstated", "full", "surrogate", "none"],
        arcana.link_rel: ["homepage", "buy", "artist", "publisher", "source", "other"],
        arcana.deck_rel: ["pattern", "surrogate_for", "expands", "companion", "other"],
        arcana.relation_recognition: ["advisory", "recognized"],
        arcana.product_scheme: ["isbn", "gtin", "publisher_sku", "other"],
        arcana.name_source: ["unstated", "booklet", "packaging", "publisher", "packager", "other"],
    }

    for enum, names in expected.items():
        assert [e.name for e in enum] == names


# --- [strings] ----------------------------------------------------------------


def test_strings(tmp_path: Path) -> None:
    deck = arcana.load_deck(
        write_v2_deck(
            tmp_path,
            manifest="""
[strings]
source = "The 1760 booklet"
license = "CC0-1.0"
license_files = ["LICENSE-strings"]
copyright = "(c) 2026 A. Translator"
rights_status = "copyrighted"
attribution = "Translated by A. Translator"

[strings.origin]
"iptc-dst" = "humanEdits"
""",
        )
    )

    terms = deck.strings
    assert isinstance(terms, arcana.string_terms)
    assert terms.source == "The 1760 booklet"
    assert [(o.system, o.term) for o in terms.origin] == [("iptc-dst", "humanEdits")]
    assert terms.license == "CC0-1.0"
    assert terms.license_files == ["LICENSE-strings"]
    assert terms.copyright == "(c) 2026 A. Translator"
    assert terms.rights_status == "copyrighted"
    assert terms.attribution == "Translated by A. Translator"

    assert terms == deck.strings
    assert terms != arcana.string_terms()


# --- [ranks], [suits], [minor_arcana] -----------------------------------------


def test_ranks_suits_and_the_minor_template(tmp_path: Path) -> None:
    deck = arcana.load_deck(
        write_v2_deck(
            tmp_path,
            manifest="""
[minor_arcana]
name_template = "{rank} de {suit}"

[suits.cups]
name = "Coupe"
name_template = "{rank} des {suit}"

[ranks.page]
name = "Valet"
""",
        )
    )

    ranks = deck.ranks
    assert type(ranks) is list
    assert all(isinstance(r, arcana.rank_info) for r in ranks)

    by_key = {r.key: r for r in ranks}
    page = by_key["page"]
    assert page.name == "Valet"
    assert isinstance(page.declared, arcana.rank_declaration)
    assert page.declared.name == "Valet"
    assert by_key["ace"].declared.name is None

    suits = {s.key: s for s in deck.suits}
    cups = suits["cups"].declared
    assert isinstance(cups, arcana.suit_declaration)
    assert cups.name == "Coupe"
    assert cups.name_template == "{rank} des {suit}"
    assert suits["wands"].declared.name is None
    assert suits["wands"].declared.name_template is None

    assert deck.minor_arcana_name_template == "{rank} de {suit}"


# --- [cards] ------------------------------------------------------------------


@pytest.fixture
def declared_deck(tmp_path: Path) -> arcana.deck:
    return arcana.load_deck(
        write_v2_deck(
            tmp_path,
            manifest="""
[cards."major_arcana.00"]
unnumbered = true
position = 0
default_variant = "gilded"
name = "Le Mat"
alt_text = "A wanderer with a stick."
inscription = "LE MAT"
artwork_language = ["fr"]
image = "h1200/major_arcana/00.png"

[cards."major_arcana.13"]
unnamed = true
number = "XIII"
supplied_name = { text = "La Mort", lang = "fr", source = "booklet" }

[cards."major_arcana.06"]
supplied_name = { text = "L'Amoureux", source = "x_museum" }

[cards."major_arcana.00:gilded"]
name = "Le Mat, gilded"
inscription = ["LE", "MAT"]
""",
            files={
                "h1200/major_arcana/00.png": PNG_HEADER,
                "h1200/major_arcana/00.gilded.png": PNG_HEADER,
            },
        )
    )


def _major(deck: arcana.deck, number: int) -> arcana.card:
    found = deck.find_card(arcana.card_id.standard_major(number))
    assert found is not None
    return found


def test_a_card_declaration_is_an_artwork_declaration(declared_deck: arcana.deck) -> None:
    declared = _major(declared_deck, 0).declared

    assert isinstance(declared, arcana.card_declaration)
    assert isinstance(declared, arcana.artwork_declaration)

    assert declared.position == 0
    assert declared.default_variant == "gilded"

    assert declared.name == "Le Mat"
    assert declared.alt_text == "A wanderer with a stick."
    assert declared.number is None
    assert declared.unnumbered is True
    assert declared.unnamed is False
    assert declared.supplied is None
    assert declared.inscription == ["LE MAT"]
    assert declared.artwork_language == ["fr"]
    assert declared.image == "h1200/major_arcana/00.png"


def test_supplied_names(declared_deck: arcana.deck) -> None:
    death = _major(declared_deck, 13).declared
    assert death.unnamed is True
    assert death.number == "XIII"

    supplied = death.supplied
    assert isinstance(supplied, arcana.supplied_name)
    assert supplied.text == "La Mort"
    assert supplied.lang == "fr"
    assert supplied.source == arcana.name_source.booklet
    assert supplied.source_name == "booklet"

    other = _major(declared_deck, 6).declared.supplied
    assert other is not None
    assert other.lang is None
    assert other.source == arcana.name_source.other
    assert other.source_name == "x_museum"

    assert supplied == supplied
    assert supplied != other


def test_an_undeclared_card(declared_deck: arcana.deck) -> None:
    declared = _major(declared_deck, 1).declared
    assert declared == arcana.card_declaration()
    assert declared != _major(declared_deck, 0).declared


def test_a_variant_declaration_is_not_a_card_declaration(declared_deck: arcana.deck) -> None:
    (gilded,) = _major(declared_deck, 0).variants
    declared = gilded.declared

    assert isinstance(declared, arcana.artwork_declaration)
    assert not isinstance(declared, arcana.card_declaration)
    assert declared.name == "Le Mat, gilded"
    assert declared.inscription == ["LE", "MAT"]

    assert declared == declared
    assert declared != arcana.artwork_declaration()


# --- [card_backs] -------------------------------------------------------------


def test_card_back_declarations(tmp_path: Path) -> None:
    deck = arcana.load_deck(
        write_v2_deck(
            tmp_path,
            manifest="""
[card_backs.designs.yes]
name = "Symmetric"
alt_text = "A rose window."
inscription = "CONVER"
artwork_language = ["fr"]
reversible = true

[card_backs.designs.no]
reversible = false

[card_backs.designs.unstated]
name = "Unstated"
""",
            files={
                "card_backs/yes.png": PNG_HEADER,
                "card_backs/no.png": PNG_HEADER,
                "card_backs/unstated.png": PNG_HEADER,
            },
        )
    )

    backs = {back.id: back.declaration for back in deck.card_backs}

    yes = backs["yes"]
    assert isinstance(yes, arcana.card_back_declaration)
    assert yes.reversible is True
    assert yes.name == "Symmetric"
    assert yes.alt_text == "A rose window."
    assert yes.inscription == ["CONVER"]
    assert yes.artwork_language == ["fr"]
    assert yes.image is None

    assert backs["no"].reversible is False
    assert backs["unstated"].reversible is None
    assert backs["unstated"].inscription == []

    assert yes == yes
    assert yes != backs["no"]


# --- 1.0 ----------------------------------------------------------------------


def test_a_1_0_deck_gives_every_new_member_its_default(alt_root: Path) -> None:
    deck = arcana.load_deck(alt_root / "deck-two")
    m = deck.metadata

    assert m.website is None
    assert m.links == []
    assert m.related == []
    assert m.pips == arcana.pip_style.unstated
    assert m.default_language is None
    assert m.metadata_language is None
    assert m.artwork_language == []
    assert m.card_size_mm is None
    assert m.packager is None
    assert m.license_files == []
    assert m.copyright is None
    assert m.rights_status is None
    assert m.redistribution == arcana.sharing_terms.unstated
    assert m.derivation == arcana.sharing_terms.unstated
    assert m.published_date is None
    assert m.product_ids == []

    assert deck.strings is None
    assert deck.minor_arcana_name_template is None
    assert all(r.declared.name is None for r in deck.ranks)
    assert all(s.declared.name is None for s in deck.suits)
    assert all(c.declared == arcana.card_declaration() for c in deck.cards)
    assert all(b.declaration == arcana.card_back_declaration() for b in deck.card_backs)
