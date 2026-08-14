# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

import 'just/common.just'

mod python 'python/mod.just'

stage := build_dir / "stage"
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
