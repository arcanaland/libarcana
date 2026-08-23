# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# The deck specification's identifier grammar (card ids, scale ids, custom
# names). Below the model: <arcana/card.hpp> is stated in terms of it.
arcana_add_internal_library(
    identifiers
    HEADERS identifiers.hpp
    SOURCES identifiers.cpp
)

arcana_link_internal(arcana-identifiers PRIVATE arcana-text)
