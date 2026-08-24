# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_internal_library(
    discovery
    HEADERS discovery.hpp
    SOURCES discovery.cpp
)

arcana_link_internal(arcana-discovery PUBLIC arcana-model)
arcana_link_internal(arcana-discovery PRIVATE arcana-vocab)
