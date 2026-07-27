image := "libarcana-builder"
build_root := "build"
build_type := "RelWithDebInfo"
build_dir := build_root / build_type
preset := "conan-" + lowercase(build_type)

podman_run := "podman run --rm -v " + justfile_directory() + ":/src:Z -w /src -v libarcana-conan:/root/.conan2 " + image

default:
    @just --list

# Build the container image used for all other recipes.
build-image:
    podman build -t {{image}} -f Containerfile .

# conan install, then cmake configure via the Conan-generated preset.
configure:
    {{podman_run}} bash -c 'conan install . --build=missing -s build_type={{build_type}} -s compiler.cppstd=26 && cmake --preset {{preset}} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON'

# Full CMake build (configures first if there is no build dir yet).
build:
    #!/bin/bash
    if [ ! -d {{build_dir}} ]; then
      just configure
    fi
    {{podman_run}} cmake --build --preset {{preset}}

# Run the unit test suite.
test:
    {{podman_run}} ctest --preset {{preset}} --output-on-failure

# clang-format
format *files:
    {{podman_run}} bash -c 'clang-format -i $(if [ -n "{{files}}" ]; then echo "{{files}}"; else find src tests include -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.hpp.in" \); fi)'

# Check clang-format
check-format:
    {{podman_run}} bash -c 'clang-format --dry-run --Werror $(find src tests include -type f \( -name "*.cpp" -o -name "*.hpp" \))'

# Static-analyze with clang-tidy (needs a configured build dir for compile_commands.json).
tidy:
    #!/bin/bash
    if [ ! -d {{build_dir}} ]; then
      just configure
    fi
    {{podman_run}} bash -c 'clang-tidy -p {{build_dir}} $(find src include -type f \( -name "*.cpp" -o -name "*.hpp" \))'

# Install to a throwaway prefix and build a couple of tiny consumer projects against it.
install-check:
    {{podman_run}} bash -c './scripts/install-check.sh {{build_dir}}'

clean:
    rm -rf {{build_root}} CMakeUserPresets.json
