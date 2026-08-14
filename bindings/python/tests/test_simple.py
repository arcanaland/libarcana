# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

"""

This is not a set of exhaustive API tests.
"""

from pathlib import Path

import arcana_tarot
import pytest

from conftest import write_broken_deck, write_deck


# --- Shape: class with private state, invariants and a mutable cache ----------


def test_deck_library_constructs_and_scans(alt_root: Path) -> None:
    lib = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[alt_root]))

    names = [d.directory_name for d in lib.decks()]
    assert names == ["deck-broken", "deck-three", "deck-two"]


def test_mutable_shared_ptr_cache_returns_the_same_object(alt_root: Path) -> None:
    lib = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[alt_root]))

    first = lib.load("deck-two")
    second = lib.load("deck-two")

    assert first is second


# --- Shape: aggregate of strings and optionals --------------------------------


def test_deck_summary_aggregate(alt_root: Path) -> None:
    lib = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[alt_root]))
    (summary,) = [d for d in lib.decks() if d.directory_name == "deck-two"]

    assert summary.id == "deck-two-shadowed-id"
    assert summary.name.startswith("Deck Two")
    assert summary.version == "1.0"
    assert summary.artist is None  # optional<string> -> None
    assert summary.icon is None  # optional<path> -> None
    assert isinstance(summary.card_count, int)


# --- Shape: std::filesystem::path ---------------------------------------------


def test_path_maps_to_pathlib(alt_root: Path) -> None:
    lib = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[alt_root]))
    (summary,) = [d for d in lib.decks() if d.directory_name == "deck-two"]

    assert isinstance(summary.path, Path)
    assert summary.path == alt_root / "deck-two"


def test_path_accepted_as_an_argument(alt_root: Path) -> None:
    # Both a Path and a str must reach std::filesystem::path.
    lib = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[alt_root]))
    assert lib.load_external(alt_root / "deck-two") is not None
    assert lib.load_external(str(alt_root / "deck-two")) is not None


# --- Shape: enum class --------------------------------------------------------


def test_enums_round_trip() -> None:
    assert arcana_tarot.to_string(arcana_tarot.suit.wands) == "wands"
    assert arcana_tarot.to_string(arcana_tarot.rank.queen) == "queen"
    assert arcana_tarot.suit_from_string("cups") == arcana_tarot.suit.cups
    assert arcana_tarot.rank_from_string("nonsense") is None
    assert arcana_tarot.card_id.standard_major(0).kind() == arcana_tarot.arcana_kind.major_arcana


# --- Shape: std::optional<T> by value -----------------------------------------


def test_optional_return(alt_root: Path) -> None:
    lib = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[alt_root]))

    assert lib.find("deck-two") is not None
    assert lib.find("no-such-deck") is None
    assert lib.reference() is None

    deck = lib.load("deck-two")
    assert deck.find_card(arcana_tarot.card_id.standard_major(0)) is not None
    assert deck.find_card(arcana_tarot.card_id.custom_major("nope")) is None
    assert deck.random_card(42) is not None


# --- Shape: std::vector<T> by value -------------------------------------------


def test_vector_returns_a_list(alt_root: Path) -> None:
    lib = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[alt_root]))
    deck = lib.load("deck-two")

    suits = deck.suits()
    assert isinstance(suits, list)
    assert [s.key for s in suits] == ["wands", "cups", "swords", "pentacles"]

    majors = deck.cards_of_kind(arcana_tarot.arcana_kind.major_arcana)
    assert len(majors) == 22
    assert isinstance(majors[0], arcana_tarot.card)


def test_map_shapes(alt_root: Path) -> None:
    lib = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[alt_root]))
    deck = lib.load("deck-two")

    assert isinstance(deck.major_arcana_remap, dict)  # std::map<int, string>
    assert isinstance(deck.suit_aliases, dict)  # std::unordered_map


# --- Shape: std::shared_ptr<deck const> ---------------------------------------


def test_shared_ptr_keeps_the_deck_alive(alt_root: Path) -> None:
    def load_and_drop() -> object:
        lib = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[alt_root]))
        return lib.load("deck-two")

    deck = load_and_drop()  # the owning deck_library is gone

    assert deck.metadata.name.startswith("Deck Two")
    assert len(deck.cards) > 0


# --- Shape: type held by an incomplete-type shared_ptr ------------------------


def test_incomplete_type_member_survives_the_boundary(alt_root: Path) -> None:
    lib = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[alt_root]))
    deck = lib.load("deck-two")

    assert "[deck]" in deck.source_toml()


def test_load_deck_returns_a_deck_by_value(alt_root: Path) -> None:
    deck = arcana_tarot.load_deck(alt_root / "deck-two")
    assert "[deck]" in deck.source_toml()


# --- Shape: std::expected<T, error> -------------------------------------------


def test_expected_error_arm_raises_with_a_code(alt_root: Path, fixtures_dir: Path) -> None:
    lib = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[alt_root]))

    with pytest.raises(arcana_tarot.deck_error) as excinfo:
        lib.load("no-such-deck")

    assert excinfo.value.code == arcana_tarot.error_code.not_found
    assert excinfo.value.message

    with pytest.raises(arcana_tarot.deck_error) as excinfo:
        lib.load_external(fixtures_dir / "broken-deck")

    assert excinfo.value.code == arcana_tarot.error_code.parse_error


def test_expected_on_a_static_factory() -> None:
    parsed = arcana_tarot.card_id.parse("minor_arcana.cups.ace")
    assert parsed == arcana_tarot.card_id.standard_minor(
        arcana_tarot.suit.cups, arcana_tarot.rank.ace
    )

    with pytest.raises(arcana_tarot.deck_error):
        arcana_tarot.card_id.parse("not a card id")


def test_malformed_decks_are_reported(tmp_path: Path) -> None:
    root = tmp_path / "library"
    write_deck(root, "deck-good")
    write_broken_deck(root, "deck-bad")

    lib = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[root]))

    assert [d.directory_name for d in lib.decks()] == ["deck-good"]

    (bad,) = lib.malformed_decks()
    assert bad.directory_name == "deck-bad"
    assert bad.problem.code == arcana_tarot.error_code.parse_error
