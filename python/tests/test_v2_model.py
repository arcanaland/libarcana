# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

from pathlib import Path

import arcana_tarot as arcana
import pytest

from conftest import PNG_HEADER, write_v2_deck


def test_a_deck_without_schema_version_no_longer_loads(tmp_path: Path) -> None:
    deck_dir = tmp_path / "no-version"
    deck_dir.mkdir()
    (deck_dir / "deck.toml").write_text('[deck]\nname = "No Version"\nversion = "1.0"\n')

    with pytest.raises(arcana.deck_error) as excinfo:
        arcana.load_deck(deck_dir)

    assert excinfo.value.code == arcana.error_code.parse_error

    # A library scan reports it rather than listing it
    lib = arcana.deck_library(arcana.library_options(roots=[tmp_path]))
    assert lib.decks() == []
    (bad,) = lib.malformed_decks()
    assert bad.directory_name == "no-version"
    assert bad.problem.code == arcana.error_code.parse_error


def test_schema_major_reports_the_major(tmp_path: Path, alt_root: Path) -> None:
    two = arcana.load_deck(write_v2_deck(tmp_path))
    assert two.metadata.schema_version == "2.0"
    assert arcana.schema_major(two.metadata) == 2

    one = arcana.load_deck(alt_root / "deck-two")
    assert arcana.schema_major(one.metadata) == 1


def test_identifier_is_a_2_0_field(tmp_path: Path, alt_root: Path) -> None:
    two = arcana.load_deck(
        write_v2_deck(tmp_path, manifest='identifier = "net.example.jdoe/deck/reader"')
    )
    assert two.metadata.identifier == "net.example.jdoe/deck/reader"

    assert arcana.load_deck(alt_root / "deck-two").metadata.identifier is None


def test_find_all_by_identifier(tmp_path: Path) -> None:
    root = tmp_path / "library"
    write_v2_deck(root, "deck-a", manifest='identifier = "net.example/deck/shared"')
    write_v2_deck(root, "deck-b", manifest='identifier = "net.example/deck/shared"')
    write_v2_deck(root, "deck-c", manifest='identifier = "net.example/deck/other"')

    lib = arcana.deck_library(arcana.library_options(roots=[root]))

    found = lib.find_all_by_identifier("net.example/deck/shared")
    assert [d.directory_name for d in found] == ["deck-a", "deck-b"]
    assert lib.find_all_by_identifier("net.example/deck/nothing") == []


def test_number_is_the_printed_face_as_a_string(tmp_path: Path) -> None:
    deck = arcana.load_deck(
        write_v2_deck(
            tmp_path,
            manifest='[cards."major_arcana.23"]\nnumber = "XXIII"\n',
            files={"h1200/major_arcana/23.png": PNG_HEADER},
        )
    )

    extended = deck.find_card(arcana.card_id.standard_major(23))
    assert extended is not None
    assert extended.number == "XXIII"  # opaque and unlocalized
    assert isinstance(extended.number, str)

    fool = deck.find_card(arcana.card_id.standard_major(0))
    assert fool is not None
    assert fool.number is None


def test_position_is_an_optional_int(tmp_path: Path) -> None:
    deck = arcana.load_deck(
        write_v2_deck(
            tmp_path,
            manifest='[cards."major_arcana.the_morning"]\nposition = 2\n',
            files={
                "scalable/major_arcana/the_morning.svg": "<svg/>",
                "scalable/major_arcana/the_night.svg": "<svg/>",
            },
        )
    )

    morning = deck.find_card(arcana.card_id.custom_major("the_morning"))
    assert morning is not None
    assert morning.position == 2

    night = deck.find_card(arcana.card_id.custom_major("the_night"))
    assert night is not None
    assert night.position is None

    # A standard major's place is its number
    fool = deck.find_card(arcana.card_id.standard_major(0))
    assert fool is not None
    assert fool.position is None

    ace = deck.find_card(arcana.card_id.standard_minor(arcana.suit.cups, arcana.rank.ace))
    assert ace is not None
    assert ace.position is None  # minors have no place in the major sequence


# --- origin -------------------------------------------------------------------


def _term(terms: list[arcana.origin_term], system: str) -> str | None:
    return next((t.term for t in terms if t.system == system), None)


def test_origin_is_a_sequence_of_origin_term_everywhere(tmp_path: Path) -> None:
    deck = arcana.load_deck(
        write_v2_deck(
            tmp_path,
            manifest="""
[deck.origin]
"iptc-dst" = "print"

[cards."major_arcana.13"]
origin = { "iptc-dst" = "digitalCreation" }

[card_backs.designs.classic.origin]
"iptc-dst" = "trainedAlgorithmicMedia"
""",
            files={"card_backs/classic.png": PNG_HEADER},
        )
    )

    assert _term(deck.metadata.origin, "iptc-dst") == "print"

    fool = deck.find_card(arcana.card_id.standard_major(0))
    assert fool is not None
    assert _term(fool.origin, "iptc-dst") == "print"

    death = deck.find_card(arcana.card_id.standard_major(13))
    assert death is not None
    assert _term(death.origin, "iptc-dst") == "digitalCreation"

    (classic,) = deck.card_backs
    assert _term(classic.origin, "iptc-dst") == "trainedAlgorithmicMedia"

    assert arcana.origin_term() != classic.origin[0]
    assert classic.origin[0] == classic.origin[0]



# --- Card back designs --------------------------------------------------------


def test_card_back_designs(tmp_path: Path) -> None:
    deck = arcana.load_deck(
        write_v2_deck(
            tmp_path,
            manifest="""
[card_backs]
default = "classic"

[card_backs.designs.classic]
name = "Classic RWS Back"
description = "The one on the box."
""",
            files={
                "card_backs/classic.png": PNG_HEADER,
                "card_backs/plain.png": PNG_HEADER,
            },
        )
    )

    assert deck.default_card_back == "classic"

    by_id = {back.id: back for back in deck.card_backs}
    assert sorted(by_id) == ["classic", "plain"]

    classic = by_id["classic"]
    assert isinstance(classic, arcana.card_back_design)
    assert classic.name == "Classic RWS Back"
    assert classic.description == "The one on the box."
    assert classic.declared is True
    assert isinstance(classic.image, Path)
    assert classic.image.name == "classic.png"

    # not in manifest so it is discovered
    assert by_id["plain"].declared is False
    assert by_id["plain"].name == "Plain"

    chosen = deck.default_card_back_design()
    assert chosen is not None
    assert chosen.id == "classic"


def test_a_deck_with_no_backs_has_no_default_design(tmp_path: Path) -> None:
    deck = arcana.load_deck(write_v2_deck(tmp_path))
    assert deck.card_backs == []
    assert deck.default_card_back is None
    assert deck.default_card_back_design() is None


# --- Artwork variants ---------------------------------------------------------


@pytest.fixture
def lovers_deck(tmp_path: Path) -> arcana.deck:
    """The Lovers with a default artwork and two variants of it."""
    return arcana.load_deck(
        write_v2_deck(
            tmp_path,
            manifest="""
[cards."major_arcana.06"]
default_variant = "two_women"
alt_text = "Two figures beneath an angel."

[cards."major_arcana.06:two_women"]
name = "The Lovers, Two Women"
alt_text = "Two women beneath an angel."
""",
            files={
                "scalable/major_arcana/06.svg": "<svg/>",
                "scalable/major_arcana/06.two_women.svg": "<svg/>",
                "scalable/major_arcana/06.two_men.svg": "<svg/>",
                "h1200/major_arcana/06.png": PNG_HEADER,
                "h1200/major_arcana/06.two_women.png": PNG_HEADER,
                "ansi20/major_arcana/06.txt": "ansi",
                "ansi20/major_arcana/06.two_women.txt": "ansi",
            },
        )
    )


def _lovers(deck: arcana.deck) -> arcana.card:
    card = deck.find_card(arcana.card_id.standard_major(6))
    assert card is not None
    return card


def test_variant_key_rides_on_the_image(lovers_deck: arcana.deck) -> None:
    lovers = _lovers(lovers_deck)

    keys = {image.variant_key for image in lovers.images}
    assert keys == {None, "two_women", "two_men"}

    # the default artwork is the one with no key
    assert any(image.variant_key is None for image in lovers.images)


def test_variants_are_entities_with_their_own_strings(lovers_deck: arcana.deck) -> None:
    lovers = _lovers(lovers_deck)

    assert lovers.default_variant == "two_women"
    assert lovers.variant_keys() == ["two_men", "two_women"]

    assert [v.key for v in lovers.variants] == ["two_men", "two_women"]
    by_key = {v.key: v for v in lovers.variants}

    two_women = by_key["two_women"]
    assert isinstance(two_women, arcana.card_variant)
    assert two_women.display_name == "The Lovers, Two Women"
    assert two_women.alt_text == "Two women beneath an angel."


def test_images_for_variant_falls_back_to_the_default(lovers_deck: arcana.deck) -> None:
    lovers = _lovers(lovers_deck)

    requested = lovers.images_for_variant("two_men")
    assert [i.variant_key for i in requested] == ["two_men"]

    # a variant the card lacks resolves to its default rather than raising
    missing = lovers.images_for_variant("no_such_key")
    assert [i.path for i in missing] == [i.path for i in lovers.images_for_variant("two_women")]


def test_each_accessor_takes_an_optional_variant_key(lovers_deck: arcana.deck) -> None:
    lovers = _lovers(lovers_deck)

    # Bare: the card's default artwork, which this deck declares is two_women
    assert lovers.scalable_image().path.name == "06.two_women.svg"
    assert lovers.best_raster_for_height(1200).path.name == "06.two_women.png"
    assert lovers.best_ansi_for_lines(20).path.name == "06.two_women.txt"

    # Keyed: the named variant
    assert lovers.scalable_image("two_men").path.name == "06.two_men.svg"
    assert lovers.scalable_image("two_women").path.name == "06.two_women.svg"

    assert lovers.best_raster_for_height(1200, "two_men") is None
    assert lovers.best_ansi_for_lines(20, "two_men") is None

    # A key the card has no artwork under falls back to the default set
    assert lovers.best_raster_for_height(1200, "nope").path.name == "06.two_women.png"
    assert lovers.best_ansi_for_lines(20, "nope").path.name == "06.two_women.txt"


def test_a_card_with_no_variants(tmp_path: Path) -> None:
    deck = arcana.load_deck(
        write_v2_deck(tmp_path, files={"scalable/major_arcana/00.svg": "<svg/>"})
    )
    fool = deck.find_card(arcana.card_id.standard_major(0))
    assert fool is not None

    assert fool.default_variant is None
    assert fool.variants == []
    assert fool.variant_keys() == []
    assert fool.images[0].variant_key is None
