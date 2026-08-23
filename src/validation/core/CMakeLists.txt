# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# The validation engine: the deck-shaped facts a check reasons about, and the
# machinery that walks a manifest. It knows nothing about any individual check
# -- that direction is the whole point of the split, and it is enforced by this
# target not linking any of them.
arcana_add_internal_library(
    validation-core
    HEADERS
        assets.hpp
        context.hpp
        facts.hpp
        probe.hpp
        spec.hpp
        tree.hpp
    SOURCES
        assets.cpp
        facts.cpp
        probe.cpp
        spec.cpp
        tree.cpp
)

# PUBLIC: context.hpp and assets.hpp are stated in terms of the model and of
# toml++, so a consumer of these headers needs both.
arcana_link_internal(arcana-validation-core PUBLIC arcana-model)
target_link_libraries(arcana-validation-core PUBLIC tomlplusplus::tomlplusplus)
target_compile_definitions(arcana-validation-core PUBLIC TOML_EXCEPTIONS=0)

arcana_link_internal(
    arcana-validation-core
    PRIVATE
        arcana-text
        arcana-identifiers
        arcana-imagesig
        arcana-vocab
)
