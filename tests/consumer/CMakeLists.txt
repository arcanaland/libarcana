# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# A standalone project that consumes an installed libarcana

cmake_minimum_required(VERSION 4.3)
project(arcana-consumer LANGUAGES CXX)

list(PREPEND CMAKE_PREFIX_PATH ${ARCANA_PREFIX})

if(NOT ARCANA_EXPECTED_VERSION)
    message(FATAL_ERROR "ARCANA_EXPECTED_VERSION is not set")
endif()

if(ARCANA_FORCE_SCRIPT_CONFIG)
    find_package(arcana ${ARCANA_EXPECTED_VERSION} REQUIRED CONFIGS arcana-config.cmake)
    set(expected_extension ".cmake")
else()
    find_package(arcana ${ARCANA_EXPECTED_VERSION} REQUIRED)
    set(expected_extension ".cps")
endif()

# Make sure we're using the correct .cps or .cmake
cmake_path(GET arcana_CONFIG EXTENSION LAST_ONLY actual_extension)
if(NOT actual_extension STREQUAL expected_extension)
    message(FATAL_ERROR
            "expected a ${expected_extension} package description, resolved "
            "${arcana_CONFIG}")
endif()

message(STATUS "arcana resolved via ${arcana_CONFIG}")

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE arcana::arcana)
target_compile_definitions(
    consumer PRIVATE ARCANA_EXPECTED_VERSION="${ARCANA_EXPECTED_VERSION}")

enable_testing()
add_test(NAME consumer_runs COMMAND consumer)
