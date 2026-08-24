# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

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
