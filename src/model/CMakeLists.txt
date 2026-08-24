# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_internal_library(
    model
    BASE_DIR ${PROJECT_SOURCE_DIR}/include
    HEADERS
        ${PROJECT_SOURCE_DIR}/include/arcana/card.hpp
        ${PROJECT_SOURCE_DIR}/include/arcana/deck.hpp
        ${PROJECT_SOURCE_DIR}/include/arcana/error.hpp
    SOURCES
        card.cpp
        deck.cpp
)

arcana_link_internal(arcana-model PRIVATE arcana-identifiers arcana-text)
