# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 4.3)
project(
    arcana
    VERSION 0.1.0 # expect ABI breaking changes before 1.0
    LANGUAGES CXX
    COMPAT_VERSION 0.1.0
    SPDX_LICENSE "MIT"
  )

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(ARCANA_BUILD_TESTS "Build the arcana test suite" ${PROJECT_IS_TOP_LEVEL})
option(ARCANA_BUILD_EXAMPLES "Build the programs under examples/" ${PROJECT_IS_TOP_LEVEL})
option(ARCANA_INSTALL "Generate arcana install rules" ${PROJECT_IS_TOP_LEVEL})
option(ARCANA_BUILD_PYTHON "Build the Python binding" OFF)

set(ARCANA_TOMLPLUSPLUS_TAG v3.4.0 CACHE STRING
    "Tag of marzer/tomlplusplus to fetch when find_package(tomlplusplus) fails")

include(FetchContent)

# The main build shouldn't need to fetch anything because it uses
# conan but just in case you don't use conan
FetchContent_Declare(tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG ${ARCANA_TOMLPLUSPLUS_TAG}
    GIT_SHALLOW TRUE
    FIND_PACKAGE_ARGS)
FetchContent_MakeAvailable(tomlplusplus)

if(tomlplusplus_SOURCE_DIR)
    message(STATUS "arcana: tomlplusplus fetched at ${ARCANA_TOMLPLUSPLUS_TAG}")
else()
    message(STATUS "arcana: tomlplusplus resolved by find_package")
endif()

add_subdirectory(src)

# Both the tests and the examples run against real decks.
if(ARCANA_BUILD_TESTS OR ARCANA_BUILD_EXAMPLES)
    enable_testing()
    include(${PROJECT_SOURCE_DIR}/cmake/reference-decks.cmake)
endif()

if(ARCANA_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(ARCANA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(ARCANA_BUILD_PYTHON)
    add_subdirectory(python)
endif()

if(ARCANA_INSTALL)
    include(${PROJECT_SOURCE_DIR}/cmake/install-rules.cmake)
endif()
