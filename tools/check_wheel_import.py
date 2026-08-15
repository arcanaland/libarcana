#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

"""Check that a pip-installed `arcana_tarot` imports and works.

Run from a directory that is not the repo root.
"""

import sys
from pathlib import Path

import arcana_tarot

init = Path(arcana_tarot.__file__).resolve()
package_dir = init.parent
print(f"arcana_tarot.__file__ = {init}")

venv = Path(sys.prefix).resolve()
if venv not in package_dir.parents:
    sys.exit(f"imported arcana_tarot from {init}, which is not under the venv {venv}")

repo_root = Path(__file__).resolve().parent.parent
for build_dir in (repo_root / "build" / "python", repo_root / "build" / "RelWithDebInfo"):
    if build_dir in package_dir.parents:
        sys.exit(f"imported arcana_tarot from a CMake build tree: {init}")

if init.name != "__init__.py":
    sys.exit(f"arcana_tarot is not a package: {init}")

extensions = sorted(p.name for p in package_dir.glob("*.so"))
print(f"extensions in {package_dir.name}/ = {extensions}")
if len(extensions) != 1 or not extensions[0].startswith("_core."):
    sys.exit(f"expected exactly one extension named _core.*.so, found {extensions}")

missing = [name for name in arcana_tarot.__all__ if not hasattr(arcana_tarot, name)]
if missing:
    sys.exit(f"__all__ names that the package does not export: {missing}")

# quick smoke test
alt_root = repo_root / "tests" / "fixtures" / "loader" / "library-root-alt"
library = arcana_tarot.deck_library(arcana_tarot.library_options(roots=[alt_root]))
names = [deck.directory_name for deck in library.decks()]
print(f"decks under {alt_root.name} = {names}")

expected = ["deck-broken", "deck-three", "deck-two"]
if names != expected:
    sys.exit(f"expected {expected}, got {names}")

print("ok")
