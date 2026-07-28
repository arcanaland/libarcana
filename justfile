# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Recipes marked [script] run inside a podman container
set script-interpreter := ['./scripts/podman-shim.sh']

image := "libarcana-builder"
build_root := "build"
build_type := "RelWithDebInfo"
build_dir := build_root / build_type
preset := "conan-" + lowercase(build_type)

prefix := "/usr/local"
stage := build_dir / "stage"
staged_prefix := stage + prefix

export LIBARCANA_IMAGE := image

default:
    @just --list

# Build the container image.
build-image:
    podman build -t {{image}} -f Containerfile .

# Conan and cmake
[script]
configure:
    conan profile path default >/dev/null 2>&1 || conan profile detect

    conan install . --build=missing \
        -s build_type={{build_type}} \
        -s compiler.cppstd=26

    cmake --preset {{preset}} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_INSTALL_PREFIX={{prefix}}

# Full CMake build.
[script]
build: configure
    cmake --build --preset {{preset}}

# Run the test suite.
[script]
test: build
    ctest --preset {{preset}} --output-on-failure

# Stage the install tree.
[script]
install: build
    rm -rf {{stage}}
    env DESTDIR=/src/{{stage}} cmake --install {{build_dir}}

# clang-format in place.
[script]
format *files:
    clang-format -i {{ if files == "" { "$(git ls-files '*.cpp' '*.hpp')" } else { files } }}

# Check the clang-format
[script]
check-format:
    clang-format --dry-run --Werror $(git ls-files '*.cpp' '*.hpp')

# clang-tidy in place
[script]
tidy: configure
    run-clang-tidy -quiet -p {{build_dir}} $(git ls-files 'src/*.cpp')

# Trash build artifacts
clean:
    rm -rf {{build_root}} CMakeUserPresets.json
