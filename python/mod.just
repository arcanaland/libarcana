# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

import '../just/common.just'

# Recipe bodies and the podman shim's paths are all written from the repo root
set working-directory := '..'

python_build_dir := build_root / "python"
wheel_venv := build_root / "wheel-venv"

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
