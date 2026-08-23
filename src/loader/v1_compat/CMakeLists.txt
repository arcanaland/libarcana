# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Frozen by ADR-029: schema 1.x reading does not change again. Its own target so
# it cannot regress while the v2 reader moves under the drifting spec pin.
arcana_add_internal_library(
    loader-v1
    BASE_DIR ${PROJECT_SOURCE_DIR}/src/loader
    HEADERS
        reader.hpp
        summary.hpp
        tables.hpp
    SOURCES
        reader.cpp
        summary.cpp
)

# PUBLIC: reader.hpp is stated in terms of <document.hpp> and <discovery.hpp>.
arcana_link_internal(arcana-loader-v1 PUBLIC arcana-loader-core arcana-discovery)
arcana_link_internal(arcana-loader-v1 PRIVATE arcana-text)
