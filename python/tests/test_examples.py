# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

"""The examples under examples/python are documentation, so they are gated.

Docs that quote code nobody runs go stale silently. These run the scripts as a
reader would, first against the loader fixtures and then, when a real deck is
available, against the reference decks.
"""

import os
import subprocess
import sys
from pathlib import Path

import pytest

EXAMPLES = ["list_decks.py", "load_deck.py", "card_assets.py"]


@pytest.fixture(scope="session")
def examples_dir() -> Path:
    root = os.environ.get("ARCANA_EXAMPLES_DIR")
    assert root, "ARCANA_EXAMPLES_DIR must point at libarcana's examples/python"
    return Path(root)


@pytest.fixture(scope="session")
def reference_decks_dir() -> Path:
    root = os.environ.get("ARCANA_REFERENCE_DECKS_DIR")
    if not root or not Path(root).is_dir():
        pytest.skip("no reference decks configured")
    return Path(root)


def run(script: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(script), *args],
        capture_output=True,
        text=True,
        check=False,
    )


@pytest.mark.parametrize("name", EXAMPLES)
def test_example_runs_against_the_fixtures(name: str, examples_dir: Path, alt_root: Path) -> None:
    result = run(examples_dir / name, str(alt_root), "deck-two")

    assert result.returncode == 0, result.stderr
    assert result.stdout


@pytest.mark.parametrize("name", EXAMPLES)
def test_example_runs_against_a_reference_deck(
    name: str, examples_dir: Path, reference_decks_dir: Path
) -> None:
    result = run(examples_dir / name, str(reference_decks_dir), "rider-waite-smith")

    assert result.returncode == 0, result.stderr
    assert "rider-waite-smith" in result.stdout


def test_card_assets_resolves_a_real_image(examples_dir: Path, reference_decks_dir: Path) -> None:
    result = run(examples_dir / "card_assets.py", str(reference_decks_dir), "rider-waite-smith")

    assert "The Fool" in result.stdout
    assert "for a 600px slot" in result.stdout
