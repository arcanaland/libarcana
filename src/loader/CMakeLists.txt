# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

add_subdirectory(core)
add_subdirectory(v1_compat)
add_subdirectory(v2)

arcana_add_internal_library(
    loader
    HEADERS summary.hpp
    SOURCES
        loader.cpp
        summary.cpp
)

arcana_link_internal(
    arcana-loader
    PUBLIC
        arcana-model
        arcana-loader-core
    PRIVATE
        arcana-discovery
        arcana-loader-v1
        arcana-loader-v2
)
