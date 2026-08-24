# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_internal_library(
    identifiers
    HEADERS identifiers.hpp
    SOURCES identifiers.cpp
)

arcana_link_internal(arcana-identifiers PRIVATE arcana-text)
