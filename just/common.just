# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Recipes marked [script] run inside a podman container
set script-interpreter := ['./just/podman-shim.sh']

image := "libarcana-builder"
build_root := "build"
build_type := "RelWithDebInfo"
build_dir := build_root / build_type
preset := "conan-" + lowercase(build_type)
prefix := "/usr/local"

export LIBARCANA_IMAGE := image

# Conan and cmake
[group('build')]
[script]
configure:
    conan profile path default >/dev/null 2>&1 || conan profile detect

    conan install . --build=missing \
        -s build_type={{build_type}} \
        -s compiler.cppstd=26

    cmake --preset {{preset}} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_INSTALL_PREFIX={{prefix}}
