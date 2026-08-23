# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Deck-spec vocabulary that is stated in terms of the model: the asset filename
# and image-root grammar. It lives above arcana-model, which is why it is not
# one of the leaf vocabularies -- asset_grammar.hpp includes <arcana/card.hpp>
# while arcana-model needs arcana-identifiers, and the two together are a cycle.
arcana_add_internal_library(
    vocab
    HEADERS asset_grammar.hpp
    SOURCES asset_grammar.cpp
)

# PUBLIC: asset_grammar.hpp includes <arcana/card.hpp>, so a consumer of this
# target needs the model's headers too.
arcana_link_internal(arcana-vocab PUBLIC arcana-model)
arcana_link_internal(arcana-vocab PRIVATE arcana-text)
