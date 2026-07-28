# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 4.3)
project(
    arcana
    VERSION 0.1.0
    LANGUAGES CXX
    COMPAT_VERSION 0.1.0
    SPDX_LICENSE "MIT"
  )

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(ARCANA_BUILD_TESTS "Build the arcana test suite" ${PROJECT_IS_TOP_LEVEL})
option(ARCANA_INSTALL "Generate arcana install rules" ${PROJECT_IS_TOP_LEVEL})

find_package(tomlplusplus REQUIRED)

add_subdirectory(src)

if(ARCANA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ARCANA_INSTALL)
    include(${PROJECT_SOURCE_DIR}/cmake/install-rules.cmake)
endif()
