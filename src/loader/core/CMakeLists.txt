# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# TOML-to-model plumbing shared by both schema majors. Touches no directory
# tree: everything here works from a document that is already in memory.
arcana_add_internal_library(
    loader-core
    HEADERS
        deck_access.hpp
        document.hpp
        manifest.hpp
        names.hpp
        ordering.hpp
        schema_version.hpp
        standard_cards.hpp
        toml_read.hpp
    SOURCES
        document.cpp
        names.cpp
        ordering.cpp
        schema_version.cpp
)

arcana_link_internal(arcana-loader-core PUBLIC arcana-model)
target_link_libraries(arcana-loader-core PUBLIC tomlplusplus::tomlplusplus)
target_compile_definitions(arcana-loader-core PUBLIC TOML_EXCEPTIONS=0)

arcana_link_internal(arcana-loader-core PRIVATE arcana-text)
