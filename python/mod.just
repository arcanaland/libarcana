# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

import '../just/common.just'

# Recipe bodies and the podman shim's paths are all written from the repo root
set working-directory := '..'

python_build_dir := build_root / "python"
wheel_venv := build_root / "wheel-venv"
pypi_venv := build_root / "pypi-venv"
examples_venv := build_root / "examples-venv"

default:
    @just --list python

# Configure the nanobind binding env against the Conan toolchain.
[script]
configure-binding: configure
    cmake -S . -B {{python_build_dir}} -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$PWD/{{build_dir}}/generators/conan_toolchain.cmake" \
        -DCMAKE_BUILD_TYPE={{build_type}} \
        -DARCANA_BUILD_PYTHON=ON \
        -DARCANA_INSTALL=OFF \
        -DARCANA_FETCH_REFERENCE_DECKS=OFF

# Build the nanobind binding.
[script]
build: configure-binding
    cmake --build {{python_build_dir}}

# Run the binding's pytest suite against the build tree.
[script]
test: build
    ctest --test-dir {{python_build_dir}} -L python --output-on-failure

[doc('`pip install .` into a throwaway venv and import it.')]
[script]
wheel:
    rm -rf {{wheel_venv}}
    python3 -m venv {{wheel_venv}}
    {{wheel_venv}}/bin/pip install -v --disable-pip-version-check .
    cd /tmp && /src/{{wheel_venv}}/bin/python /src/tools/check_wheel_import.py

# e.g. `just python shell-pypi`        latest release
#      `just python shell-pypi 0.1.0`  a pinned one
[doc('Python REPL on the host with `arcana-tarot` installed from PyPI.')]
shell-pypi version="":
    rm -rf {{pypi_venv}}
    uv venv -q --python 3.12 {{pypi_venv}}
    uv pip install -q --python {{pypi_venv}}/bin/python \
        'arcana-tarot{{ if version != "" { "==" + version } else { "" } }}'
    venv="$PWD/{{pypi_venv}}"; cd /tmp && "$venv/bin/python" -q

# e.g. `just python example list-decks`
#      `just python example examples/simple.py --some-arg`
[doc('Run a script from examples/ on the host against `arcana-tarot` from PyPI.')]
example name *args:
    #!/usr/bin/env bash
    set -euo pipefail
    [ -x {{examples_venv}}/bin/python ] || uv venv -q --python 3.12 {{examples_venv}}
    uv pip install -q --python {{examples_venv}}/bin/python arcana-tarot
    script='{{name}}'
    [ -f "$script" ] || script="examples/${script%.py}.py"
    {{examples_venv}}/bin/python "$script" {{args}}

[doc('Python REPL with `arcana-tarot` built from this checkout.')]
[script]
shell-local: wheel
    cd /tmp && /src/{{wheel_venv}}/bin/python -q
