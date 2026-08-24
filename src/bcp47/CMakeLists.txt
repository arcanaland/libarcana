# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# BCP 47 language tags and the subtag tables they are validated against.
arcana_add_internal_library(
    bcp47
    HEADERS
        language_tag.hpp
        tables.hpp
    SOURCES
        language_tag.cpp
        tables.cpp
)

arcana_link_internal(arcana-bcp47 PRIVATE arcana-text)
