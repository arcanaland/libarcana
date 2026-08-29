# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_internal_library(
    loader-v1
    BASE_DIRS ${PROJECT_SOURCE_DIR}/src/loader
    HEADERS
        reader.hpp
        summary.hpp
        tables.hpp
    SOURCES
        reader.cpp
        summary.cpp
)

arcana_link_internal(
    arcana-loader-v1
    PUBLIC
        arcana-loader-core
        arcana-discovery
    PRIVATE
        arcana-text
)
