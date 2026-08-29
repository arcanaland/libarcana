# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_internal_library(
    model
    BASE_DIRS ${PROJECT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}
    HEADERS
        ${PROJECT_SOURCE_DIR}/include/arcana/card.hpp
        ${PROJECT_SOURCE_DIR}/include/arcana/deck.hpp
        ${PROJECT_SOURCE_DIR}/include/arcana/error.hpp
        deck_state.hpp
    SOURCES
        card.cpp
        deck.cpp
)

arcana_link_internal(arcana-model PRIVATE arcana-identifiers arcana-text)
