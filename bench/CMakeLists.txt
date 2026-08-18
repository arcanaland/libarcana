# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Which decks the benchmarks measure by default.
#
# /decks is where the podman shim mounts the host's deck library, which is the
# only corpus with a 2.0 deck in it. The reference decks are the fallback, and
# are all 1.0.
set(ARCANA_BENCH_LIBRARY_DIR "/decks" CACHE PATH
    "Deck library the benchmarks run against by default")

find_package(Catch2 3 REQUIRED)

add_library(arcana-bench-workloads STATIC workloads.cpp)
target_link_libraries(arcana-bench-workloads PUBLIC arcana::arcana arcana::loader)
target_include_directories(arcana-bench-workloads PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# The payload hyperfine drives
add_executable(arcana-bench main.cpp)
target_link_libraries(arcana-bench PRIVATE arcana-bench-workloads)

# The in-process front end
add_executable(arcana-bench-micro micro.cpp)
target_link_libraries(
    arcana-bench-micro PRIVATE arcana-bench-workloads Catch2::Catch2WithMain
)
target_compile_definitions(
    arcana-bench-micro PRIVATE BENCH_LIBRARY_DIR="${ARCANA_BENCH_LIBRARY_DIR}"
)

# Deliberately not registered with ctest: a benchmark is not a test, and `just
# test` must stay fast. Both binaries are driven by `just bench`.
