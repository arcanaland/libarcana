#!/bin/bash
# Installs libarcana to a throwaway prefix and builds two tiny consumer
# projects against it: one via the CMake-script config, one via CPS alone
# (with the script config removed from the install tree). Run through
# `just install-check` inside the container — never on the host.
set -euo pipefail

build_dir="${1:-build/RelWithDebInfo}"
build_type="$(basename "$build_dir")"

# arcana is a STATIC library and tomlplusplus is linked PRIVATE; CMake's own
# export mechanism still requires consumers to resolve tomlplusplus at
# configure time (see cmake/arcanaConfig.cmake.in for why). A consumer
# picking up libarcana via Conan gets this for free via its own dependency
# graph; here we point CMAKE_PREFIX_PATH at the same Conan-generated config
# this repo's own build used, to mimic that.
conan_generators="$(cd "$build_dir/generators" && pwd)"

prefix="$(mktemp -d)"
work="$(mktemp -d)"
trap 'rm -rf "$prefix" "$work"' EXIT

echo "== Installing to $prefix =="
cmake --install "$build_dir" --prefix "$prefix"

echo
echo "== Install tree =="
find "$prefix" -type f | sort

cps_file="$(find "$prefix" -name '*.cps' | head -n1 || true)"
if [ -n "$cps_file" ]; then
    echo
    echo "== Generated CPS file: $cps_file =="
    cat "$cps_file"
else
    echo "WARNING: no .cps file found under $prefix"
fi

# --- Consumer 1: via the CMake-script config -------------------------------
echo
echo "== Consumer 1: CMake-script config (find_package(arcana)) =="
c1="$work/consumer1"
mkdir -p "$c1"
cat >"$c1/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 4.3)
project(consumer1 LANGUAGES CXX)
find_package(arcana REQUIRED)
add_executable(consumer1 main.cpp)
target_link_libraries(consumer1 PRIVATE arcana::arcana)
EOF
cat >"$c1/main.cpp" <<'EOF'
#include <arcana/version.hpp>
#include <cstdio>
int main()
{
    std::printf("%s\n", arcana::library_version().data());
    return arcana::library_version() == "0.1.0" ? 0 : 1;
}
EOF
cmake -S "$c1" -B "$c1/build" -G Ninja -DCMAKE_PREFIX_PATH="$prefix;$conan_generators" -DCMAKE_BUILD_TYPE="$build_type"
cmake --build "$c1/build"
"$c1/build/consumer1"
echo "Consumer 1 OK (script config)"

# --- Consumer 2: via CPS alone, script config removed -----------------------
echo
echo "== Consumer 2: CPS alone (script config removed from install tree) =="
if [ -z "$cps_file" ]; then
    echo "SKIPPED: no .cps file was generated; cannot attempt CPS-only consumption."
    exit 0
fi

rm -f "$prefix"/lib*/cmake/arcana/arcanaConfig.cmake \
      "$prefix"/lib*/cmake/arcana/arcanaConfigVersion.cmake \
      "$prefix"/lib*/cmake/arcana/arcanaTargets.cmake \
      "$prefix"/lib*/cmake/arcana/arcanaTargets-*.cmake

c2="$work/consumer2"
mkdir -p "$c2"
cat >"$c2/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 4.3)
project(consumer2 LANGUAGES CXX)
find_package(arcana REQUIRED)
add_executable(consumer2 main.cpp)
target_link_libraries(consumer2 PRIVATE arcana::arcana)
EOF
cp "$c1/main.cpp" "$c2/main.cpp"

set +e
cmake -S "$c2" -B "$c2/build" -G Ninja -DCMAKE_PREFIX_PATH="$prefix;$conan_generators" -DCMAKE_BUILD_TYPE="$build_type" \
    > "$work/consumer2-configure.log" 2>&1
configure_status=$?
set -e

if [ $configure_status -ne 0 ]; then
    echo "CPS-only consumer FAILED to configure. Log:"
    cat "$work/consumer2-configure.log"
    echo
    echo "This is the expected/acceptable outcome per TASK-003 step 6: CPS is emitted"
    echo "as a bonus artifact, the CMake-script config remains the supported path, and"
    echo "the second consumer check is the one most acceptable to abandon."
    exit 0
fi

cmake --build "$c2/build"
"$c2/build/consumer2"
echo "Consumer 2 OK (CPS alone)"
