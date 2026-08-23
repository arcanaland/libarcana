# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

add_subdirectory(core)
add_subdirectory(checks)

# The catalogue and the compile-time registry that aggregates the checks by
# name. This is the top of the validation graph: registry.hpp includes every
# check header, so this target -- and nothing below it -- knows the check list.
arcana_add_internal_library(
    validation
    HEADERS
        catalogue.hpp
        registry.hpp
    SOURCES
        catalogue.cpp
        registry.cpp
)

# PUBLIC: registry.hpp includes every check header, which in turn need the
# engine, so a consumer of <registry.hpp> needs all of them on its include path.
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
