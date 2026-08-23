# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

add_subdirectory(core)
add_subdirectory(v1_compat)
add_subdirectory(v2)

# The dispatcher: reads schema_version and hands the document to one major's
# reader. Nothing below this target knows the other major exists.
arcana_add_internal_library(
    loader
    HEADERS summary.hpp
    SOURCES
        loader.cpp
        summary.cpp
)

# PUBLIC: summary.hpp is stated in terms of <arcana/library.hpp>.
arcana_link_internal(arcana-loader PUBLIC arcana-model arcana-loader-core)
arcana_link_internal(arcana-loader PRIVATE arcana-discovery arcana-loader-v1 arcana-loader-v2)
