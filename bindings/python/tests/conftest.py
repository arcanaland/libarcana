# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

import os
from pathlib import Path

import pytest


@pytest.fixture(scope="session")
def fixtures_dir() -> Path:
    root = os.environ.get("ARCANA_FIXTURES_DIR")
    assert root, "ARCANA_FIXTURES_DIR must point at libarcana's tests/fixtures"
    return Path(root)


@pytest.fixture
def alt_root(fixtures_dir: Path) -> Path:
    """A library root holding deck-two, deck-three and deck-broken."""
    return fixtures_dir / "library-root-alt"


@pytest.fixture
def reference_deck(fixtures_dir: Path) -> Path:
    return fixtures_dir / "reference-deck"


def write_deck(root: Path, directory_name: str, deck_id: str | None = None) -> Path:
    """The minimum a directory needs to be a readable deck."""
    deck_dir = root / directory_name
    deck_dir.mkdir(parents=True)
    (deck_dir / "deck.toml").write_text(
        "\n".join(
            [
                "[deck]",
                f'id = "{deck_id or directory_name}-id"',
                'schema_version = "1.0"',
                f'name = "{directory_name}"',
                'version = "1.0"',
                "",
            ]
        )
    )
    return deck_dir


def write_broken_deck(root: Path, directory_name: str) -> Path:
    """A directory that looks like a deck but whose manifest will not parse."""
    deck_dir = root / directory_name
    deck_dir.mkdir(parents=True)
    (deck_dir / "deck.toml").write_text("this is not [valid toml at all\n")
    return deck_dir
