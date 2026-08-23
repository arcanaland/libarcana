# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_internal_library(
    validation
    HEADERS
        assets.hpp
        catalogue.hpp
        context.hpp
        facts.hpp
        probe.hpp
        registry.hpp
        spec.hpp
        tree.hpp
        checks/ansi.hpp
        checks/backs.hpp
        checks/cards.hpp
        checks/deck.hpp
        checks/ids.hpp
        checks/images.hpp
        checks/names.hpp
        checks/surrogate.hpp
    SOURCES
        assets.cpp
        catalogue.cpp
        facts.cpp
        probe.cpp
        registry.cpp
        spec.cpp
        tree.cpp
        checks/ansi.cpp
        checks/backs.cpp
        checks/cards.cpp
        checks/deck.cpp
        checks/ids.cpp
        checks/images.cpp
        checks/names.cpp
        checks/surrogate.cpp
)

arcana_link_internal(arcana-validation PUBLIC arcana-model)
arcana_link_internal(arcana-validation PRIVATE arcana-data arcana-vocab)

target_link_libraries(arcana-validation PUBLIC tomlplusplus::tomlplusplus)
target_compile_definitions(arcana-validation PUBLIC TOML_EXCEPTIONS=0)
