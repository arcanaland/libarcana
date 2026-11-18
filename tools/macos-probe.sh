#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Build the library and the cp312 wheel on a local arm64 Mac at several
# deployment targets, and summarise what passed.
#
#   tools/macos-probe.sh                # 11.0 13.3 14.0 15.0
#   tools/macos-probe.sh 13.3 14.0      # just these
#
# Needs uv and the Xcode command line tools. Nothing is installed system-wide;
# everything lands under $PROBE_DIR.

set -uo pipefail

repo=$(cd "$(dirname "$0")/.." && pwd)
out=${PROBE_DIR:-${TMPDIR:-/tmp}/arcana-macos-probe}
targets=("$@")
[ ${#targets[@]} -eq 0 ] && targets=(11.0 13.3 14.0 15.0)

rm -rf "$out"
mkdir -p "$out"
report="$out/report.txt"

{
  echo "== toolchain"
  sw_vers
  xcodebuild -version 2>&1 || echo "xcodebuild: no full Xcode"
  xcode-select -p
  clang++ --version
  uname -m
} | tee "$report"

uv venv -q --python 3.12 "$out/tools"
uv pip install -q --python "$out/tools" 'cmake>=4.3' ninja build delocate
export PATH="$out/tools/bin:$PATH"
cmake --version | head -1 | tee -a "$report"
ninja --version | tee -a "$report"

summary=()
for t in "${targets[@]}"; do
  d="$out/$t"
  mkdir -p "$d"
  export MACOSX_DEPLOYMENT_TARGET=$t
  echo "== $t"

  a=fail
  if cmake -S "$repo" -B "$d/build" -G Ninja \
      -DARCANA_BUILD_TESTS=OFF -DARCANA_BUILD_BENCH=OFF \
      -DARCANA_INSTALL=OFF -DARCANA_BUILD_PYTHON=OFF >"$d/a.log" 2>&1 &&
    cmake --build "$d/build" -- -k 0 >>"$d/a.log" 2>&1; then
    a=pass
  fi

  b=fail
  wheel=-
  if python -m build --wheel --outdir "$d/raw" "$repo" >"$d/b.log" 2>&1 &&
    delocate-wheel -w "$d/dist" "$d"/raw/*.whl >>"$d/b.log" 2>&1; then
    b=pass
    wheel=$(basename "$d"/dist/*.whl)
  fi

  test=-
  if [ "$b" = pass ]; then
    test=fail
    uv venv -q --python 3.12 "$d/venv"
    if uv pip install -q --python "$d/venv" "$d"/dist/*.whl >"$d/test.log" 2>&1 &&
      (cd "$d" && "$d/venv/bin/python" "$repo/tools/check_wheel_import.py") >>"$d/test.log" 2>&1; then
      test=pass
    fi
    so=$(find "$d/venv" -name '_core*.so' | head -1)
    [ -n "$so" ] && otool -l "$so" | grep -A3 LC_BUILD_VERSION >>"$d/test.log"
  fi

  {
    echo "== $t  A=$a  B=$b  test=$test  wheel=$wheel"
    grep -m1 -E 'tomlplusplus|toml\+\+' "$d/a.log" || true
    grep -hE 'error:' "$d/a.log" "$d/b.log" 2>/dev/null | sed "s|$repo/||" | sort -u | head -40
  } | tee -a "$report"
  summary+=("$t A=$a B=$b test=$test $wheel")
done

{
  echo "== summary"
  printf '%s\n' "${summary[@]}"
} | tee -a "$report"
echo "full logs: $out   report: $report"
