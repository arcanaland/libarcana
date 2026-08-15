# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT


import os
import subprocess
import sys
import textwrap
from pathlib import Path

import arcana_tarot as arcana
import pytest

from conftest import write_deck


@pytest.fixture
def growable_root(tmp_path: Path) -> Path:
    root = tmp_path / "library"
    write_deck(root, "deck-a")
    write_deck(root, "deck-b")
    return root


def test_span_view_survives_construction(growable_root: Path) -> None:
    """The control: without refresh(), the view is fine and copies nothing."""
    lib = arcana.deck_library(arcana.library_options(roots=[growable_root]))
    view = lib.decks_view()

    assert len(view) == 2
    assert view[0].directory_name == "deck-a"


def test_keep_alive_protects_the_view_from_the_owner_dying(growable_root: Path) -> None:
    """keep_alive does the one thing it can do, and it does it correctly."""
    options = arcana.library_options(roots=[growable_root])
    view = arcana.deck_library(options).decks_view()

    # the deck_library has no Python reference left
    assert view[0].directory_name == "deck-a"


def test_copying_binding_is_immune_to_refresh(growable_root: Path) -> None:
    lib = arcana.deck_library(arcana.library_options(roots=[growable_root]))
    decks = lib.decks()

    write_deck(growable_root, "deck-c")
    lib.refresh()

    assert [d.directory_name for d in decks] == ["deck-a", "deck-b"]
    assert [d.directory_name for d in lib.decks()] == ["deck-a", "deck-b", "deck-c"]
