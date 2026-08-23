# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

import 'just/common.just'

mod bench 'bench/mod.just'
mod python 'python/mod.just'

stage := build_dir / "stage"
graph_dir := build_dir / "graph"
staged_prefix := stage + prefix

default:
    @just --list

# Build the container image.
[group('env')]
build-image:
    podman build -t {{image}} -f Containerfile .

# Run an arbitrary command inside the build container.
[group('env')]
[script]
sh +cmd:
    {{cmd}}

# Trash build artifacts
[group('env')]
clean:
    rm -rf {{build_root}} CMakeUserPresets.json

# Full CMake build.
[group('build')]
[script]
build: configure
    cmake --build --preset {{preset}}

# Run the test suite.
[group('test')]
[script]
test: build
    ctest --preset {{preset}} --output-on-failure --verbose

# Run the ctest cases matching a regex, e.g. `just test-match 'ids'`.
[group('test')]
[script]
test-match pattern: build
    ctest --preset {{preset}} --output-on-failure -R '{{pattern}}'

# e.g. `just run-test validation/ids_test --list-tests`
#      `just run-test validation/ids_test '[ids]' -s`
[doc('Run one test binary directly, passing the rest to Catch2.')]
[group('test')]
[script]
run-test bin *args: build
    {{build_dir}}/tests/{{bin}} {{args}}

# e.g. `just debug-test validation/ids_test '[ids]'`
[doc('Run one test binary under gdb, stopping where it fails.')]
[group('test')]
[script]
debug-test bin *args: build
    gdb -q -ex run --args {{build_dir}}/tests/{{bin}} --break {{args}}

# e.g. `just graph`      internal libraries only
#      `just graph all`  plus test executables and external packages
[doc('Render the CMake target dependency graph to build/<type>/graph/arcana.png.')]
[group('build')]
[script]
graph mode="libs": configure
    mkdir -p {{ graph_dir }}

    # --graphviz reads its knobs from a file in the binary dir
    cat > {{ build_dir }}/CMakeGraphVizOptions.cmake <<'OPTS'
    set(GRAPHVIZ_GRAPH_NAME "arcana")
    set(GRAPHVIZ_GENERATE_PER_TARGET OFF)
    set(GRAPHVIZ_GENERATE_DEPENDERS OFF)
    set(GRAPHVIZ_EXECUTABLES {{ if mode == "all" { "ON" } else { "OFF" } }})
    set(GRAPHVIZ_EXTERNAL_LIBS {{ if mode == "all" { "ON" } else { "OFF" } }})
    # Conan's generated shim targets say nothing about our own layering.
    set(GRAPHVIZ_IGNORE_TARGETS "^CONAN_LIB::" "_DEPS_TARGET$")
    OPTS

    cmake --graphviz={{ graph_dir }}/arcana.dot --preset {{ preset }} >/dev/null

    # Left-to-right
    dot -Tpng -Grankdir=LR -o {{ graph_dir }}/arcana.png {{ graph_dir }}/arcana.dot
    echo "wrote {{ graph_dir }}/arcana.png"

# clang-format in place.
[group('lint')]
[script]
format *files:
    clang-format -i {{ if files == "" { "$(git ls-files '*.cpp' '*.hpp')" } else { files } }}

# Check the clang-format
[group('lint')]
[script]
check-format:
    clang-format --dry-run --Werror $(git ls-files '*.cpp' '*.hpp')

# clang-tidy in place
[group('lint')]
[script]
tidy: configure
    run-clang-tidy -quiet -p {{build_dir}} $(git ls-files 'src/*.cpp')

# Check that every file declares its copyright and licence (REUSE 3.3).
[group('lint')]
[script]
lint-reuse:
    reuse lint

# Stage the install tree.
[group('dist')]
[script]
install: build
    rm -rf {{stage}}
    env DESTDIR=/src/{{stage}} cmake --install {{build_dir}}

# Regenerate the vendored SPDX License List.
[group('dist')]
[script]
generate-spdx *tag:
    python3 tools/generate_spdx_data.py {{tag}}
