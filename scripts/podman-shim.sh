#!/usr/bin/env bash
set -euo pipefail

image="${LIBARCANA_IMAGE:-libarcana-builder}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

exec podman run --rm -i \
  -v "$root:/src:Z" \
  -v libarcana-conan:/root/.conan2 \
  -w /src \
  "$image" \
  bash -euo pipefail -s <"$1"
