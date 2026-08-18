#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

set -euo pipefail

image="${LIBARCANA_IMAGE:-libarcana-builder}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# this is a file that `just` creates from the recipe body and passes to us.
script="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"

# forward a tty when there is one so we get color and stuff
tty_flag=()
[ -t 0 ] && [ -t 1 ] && tty_flag=(-t)

# The benchmarks want real decks, and the reference corpus is all 1.0. Mount the
# host's deck library read-only at /decks when there is one, so `just bench` can
# measure a 2.0 deck without one being vendored. Inert for every other recipe:
# nothing else looks at /decks.
library="${ARCANA_BENCH_LIBRARY:-${XDG_DATA_HOME:-$HOME/.local/share}/tarot/decks}"
library_flag=()
[ -d "$library" ] && library_flag=(-v "$library:/decks:ro,z")

exec podman run --rm -i "${tty_flag[@]}" \
  -v "$root:/src:Z" \
  -v libarcana-conan:/root/.conan2 \
  -v "$script:/tmp/recipe.sh:ro,z" \
  "${library_flag[@]}" \
  -w /src \
  "$image" \
  bash -euo pipefail /tmp/recipe.sh
