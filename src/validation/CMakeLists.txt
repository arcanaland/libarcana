# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

add_subdirectory(core)
add_subdirectory(checks)

arcana_add_internal_library(
    validation
    HEADERS
        catalogue.hpp
        registry.hpp
    SOURCES
        catalogue.cpp
        registry.cpp
)

arcana_link_internal(
    arcana-validation
    PUBLIC
        arcana-validation-core
        arcana-check-ansi
        arcana-check-backs
        arcana-check-cards
        arcana-check-deck
        arcana-check-ids
        arcana-check-images
        arcana-check-names
        arcana-check-surrogate
)
