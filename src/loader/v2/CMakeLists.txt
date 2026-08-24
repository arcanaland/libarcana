# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# BASE_DIR is the loader directory so the header is <v2/reader.hpp>
arcana_add_internal_library(
    loader-v2
    BASE_DIR ${PROJECT_SOURCE_DIR}/src/loader
    HEADERS reader.hpp
    SOURCES reader.cpp
)

arcana_link_internal(arcana-loader-v2 PUBLIC arcana-loader-core)
arcana_link_internal(arcana-loader-v2 PRIVATE arcana-discovery arcana-text)
