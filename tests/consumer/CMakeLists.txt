# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# A standalone project that consumes an installed libarcana

cmake_minimum_required(VERSION 4.3)
project(arcana-consumer LANGUAGES CXX)

list(PREPEND CMAKE_PREFIX_PATH ${ARCANA_PREFIX} ${ARCANA_DEP_PREFIX})

if(ARCANA_FORCE_SCRIPT_CONFIG)
    find_package(arcana 0.1.0 REQUIRED CONFIGS arcanaConfig.cmake)
    set(expected_extension ".cmake")
else()
    find_package(arcana 0.1.0 REQUIRED)
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

enable_testing()
add_test(NAME consumer_runs COMMAND consumer)
