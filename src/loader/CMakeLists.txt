# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_internal_library(
    loader
    HEADERS
        deck_access.hpp
        discovery.hpp
        document.hpp
        manifest.hpp
        names.hpp
        ordering.hpp
        reader.hpp
        schema_version.hpp
        standard_cards.hpp
        summary.hpp
        toml_read.hpp
        v1_compat/reader.hpp
        v1_compat/summary.hpp
        v1_compat/tables.hpp
    SOURCES
        discovery.cpp
        document.cpp
        loader.cpp
        names.cpp
        ordering.cpp
        reader.cpp
        schema_version.cpp
        summary.cpp
        v1_compat/reader.cpp
        v1_compat/summary.cpp
)

arcana_link_internal(arcana-loader PUBLIC arcana-model)
arcana_link_internal(arcana-loader PRIVATE arcana-data arcana-vocab)

target_link_libraries(arcana-loader PUBLIC tomlplusplus::tomlplusplus)
target_compile_definitions(arcana-loader PUBLIC TOML_EXCEPTIONS=0)
