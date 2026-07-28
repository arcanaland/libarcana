image := "libarcana-builder"
build_root := "build"
build_type := "RelWithDebInfo"
build_dir := build_root / build_type
preset := "conan-" + lowercase(build_type)

shebang_podman := "/usr/bin/env -S ./scripts/podman-shim.sh"

export LIBARCANA_IMAGE := image

default:
    @just --list

# Build the container image
build-image:
    podman build -t {{image}} -f Containerfile .

# conan install, then cmake configure via the Conan-generated preset.
configure:
    #!{{shebang_podman}}
    conan install . --build=missing \
        -s build_type={{build_type}} \
        -s compiler.cppstd=26
    cmake --preset {{preset}} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Full CMake build (configures first if there is no build dir yet).
build:
    #!{{shebang_podman}}
    if [ ! -d {{build_dir}} ]; then
        conan install . --build=missing \
            -s build_type={{build_type}} \
            -s compiler.cppstd=26
        cmake --preset {{preset}} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    fi
    cmake --build --preset {{preset}}

# Run the test suite
test:
    #!{{shebang_podman}}
    ctest --preset {{preset}} --output-on-failure

# clang-format
format *files:
    #!{{shebang_podman}}
    files="{{files}}"
    if [ -z "$files" ]; then
        files=$(find src tests include -type f \
            \( -name '*.cpp' -o -name '*.hpp' -o -name '*.hpp.in' \))
    fi
    clang-format -i $files

# Check clang-format
check-format:
    #!{{shebang_podman}}
    clang-format --dry-run --Werror $(find src tests include -type f \
        \( -name '*.cpp' -o -name '*.hpp' \))

# clang-tidy
tidy: build
    #!{{shebang_podman}}
    clang-tidy -p {{build_dir}} $(find src include -type f \
        \( -name '*.cpp' -o -name '*.hpp' \))

clean:
    rm -rf {{build_root}} CMakeUserPresets.json
