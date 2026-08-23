# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Asset discovery: what a deck directory's image roots actually contain. Named
# for the deck-side walk, not for the XDG library scan -- that is src/library.cpp.
arcana_add_internal_library(
    discovery
    HEADERS discovery.hpp
    SOURCES discovery.cpp
)

arcana_link_internal(arcana-discovery PUBLIC arcana-model)
arcana_link_internal(arcana-discovery PRIVATE arcana-vocab)
