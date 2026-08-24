# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_internal_library(
    vocab
    HEADERS asset_grammar.hpp
    SOURCES asset_grammar.cpp
)

arcana_link_internal(arcana-vocab PUBLIC arcana-model)
arcana_link_internal(arcana-vocab PRIVATE arcana-text)
