# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

from collections import Counter
from pathlib import Path

import pytest

import arcana_tarot as at


def validate_fixture(fixtures_dir: Path, relative: str) -> list[at.diagnostic]:
    return at.validate(at.load_deck(fixtures_dir / relative))


def validation_fixtures(fixtures_dir: Path) -> list[Path]:
    found = sorted(p.parent for p in (fixtures_dir / "validation").glob("*/*/deck.toml"))
    assert found
    return found


def test_a_valid_deck_validates_to_nothing(fixtures_dir: Path) -> None:
    assert validate_fixture(fixtures_dir, "validation/backs/backs-valid") == []


def test_card_references_report_codes_cards_and_keys(fixtures_dir: Path) -> None:
    found = validate_fixture(fixtures_dir, "validation/ids/card-references-error")

    assert [d.code for d in found] == ["bad-cards-table-key", "non-canonical-card-reference"]
    assert [d.card for d in found] == ["major_arcana.6", "minor_arcana.Pentacles.page"]
    assert [d.key for d in found] == ['cards."major_arcana.6"', "excluded_cards.cards"]
    assert all(d.level == at.severity.error for d in found)
    assert all(d.path is None for d in found)
    assert "is not a card reference" in found[0].message


def test_a_finding_about_a_file_carries_its_path(fixtures_dir: Path) -> None:
    found = validate_fixture(fixtures_dir, "validation/ids/custom-names-error")

    assert isinstance(found[3].path, Path)
    assert found[3].path.as_posix() == "h1200/major_arcana/Morning.png"
    assert found[4].path.as_posix() == "h1200/minor_arcana/stars/Knave.png"
    assert found[3].key is None


def test_every_fired_code_resolves_to_a_checked_rule(fixtures_dir: Path) -> None:
    fired = 0
    for deck_dir in validation_fixtures(fixtures_dir):
        for d in at.validate(at.load_deck(deck_dir)):
            fired += 1
            assert isinstance(d.code, str)
            found = at.find_rule(d.code)
            assert found is not None, d.code
            assert found.code == d.code
            assert at.state_of(d.code) == at.rule_state.checked
    assert fired > 0


def test_severity_is_ordered(fixtures_dir: Path) -> None:
    assert at.severity.pedantic < at.severity.info < at.severity.warning < at.severity.error

    found = validate_fixture(fixtures_dir, "validation/ids/identifier-shape-error")
    levels = {d.level for d in found}
    assert at.severity.warning in levels
    assert at.severity.error in levels

    errors = [d for d in found if d.level >= at.severity.error]
    assert errors
    assert all(d.level == at.severity.error for d in errors)
    assert len(errors) < len(found)


def test_the_catalogue_is_sorted_and_every_rule_has_a_state() -> None:
    catalogue = at.rules()
    assert catalogue

    codes = [r.code for r in catalogue]
    assert codes == sorted(codes)
    assert len(set(codes)) == len(codes)

    states = Counter(at.state_of(code) for code in codes)
    assert None not in states
    assert set(states) == {at.rule_state.checked, at.rule_state.pending, at.rule_state.deferred}
    assert sum(states.values()) == len(catalogue)

    for r in catalogue:
        assert isinstance(r.default_level, at.severity)
        assert isinstance(r.needs, at.phase)
        assert r.area
        assert r.explanation
        assert r.experimental is False


def test_every_library_phase_rule_is_deferred() -> None:
    # validate() never runs the library phase, so a checked one would overstate coverage
    library = [r for r in at.rules() if r.needs == at.phase.library]
    assert library
    assert all(at.state_of(r.code) == at.rule_state.deferred for r in library)


def test_an_unknown_code_resolves_to_nothing() -> None:
    assert at.find_rule("no-such-code") is None
    assert at.state_of("no-such-code") is None


def test_rule_table_rules_cite_the_rule_table() -> None:
    found = at.find_rule("bad-cards-table-key")
    assert found is not None

    cited = [(s.schema_major, s.anchor) for s in found.citations()]
    assert cited == [
        (2, "312-card-references-and-the-variant-suffix"),
        (2, "43-cards"),
        (2, "104-validation-rules"),
    ]

    url = at.spec_url(found.citations()[-1])
    assert url.startswith("https://")
    assert url.endswith("#104-validation-rules")


def test_spec_revision_is_empty_for_an_unpinned_major() -> None:
    assert at.spec_revision(2)
    assert at.spec_revision(9) == ""


def test_catalogue_types_are_read_only(fixtures_dir: Path) -> None:
    for cls in (at.diagnostic, at.rule, at.spec_section):
        with pytest.raises(TypeError):
            cls()

    found = validate_fixture(fixtures_dir, "validation/ids/card-references-error")
    with pytest.raises(AttributeError):
        found[0].code = "something-else"
    with pytest.raises(AttributeError):
        found[0].level = at.severity.info

    with pytest.raises(AttributeError):
        at.rules()[0].code = "something-else"


def test_reprs_name_the_code_and_level(fixtures_dir: Path) -> None:
    found = validate_fixture(fixtures_dir, "validation/ids/card-references-error")
    assert repr(found[0]) == (
        '<diagnostic bad-cards-table-key error card=major_arcana.6 key=cards."major_arcana.6">'
    )

    found_rule = at.find_rule("bad-cards-table-key")
    assert repr(found_rule) == "<rule bad-cards-table-key error>"
