# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

add_subdirectory(loader)

add_library(arcana)
add_library(arcana::arcana ALIAS arcana)

configure_file(${PROJECT_SOURCE_DIR}/include/arcana/version.hpp.in
               ${PROJECT_BINARY_DIR}/generated/include/arcana/version.hpp @ONLY)

target_sources(
    arcana
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_BINARY_DIR}/generated/include
        FILES
            ${PROJECT_BINARY_DIR}/generated/include/arcana/version.hpp
            ${PROJECT_SOURCE_DIR}/include/arcana/error.hpp
            ${PROJECT_SOURCE_DIR}/include/arcana/paths.hpp
            ${PROJECT_SOURCE_DIR}/include/arcana/card.hpp
            ${PROJECT_SOURCE_DIR}/include/arcana/deck.hpp
            ${PROJECT_SOURCE_DIR}/include/arcana/library.hpp
    PRIVATE
        version.cpp
        paths.cpp
        card.cpp
        deck.cpp
        library.cpp
        $<TARGET_OBJECTS:arcana-loader>
)

target_compile_features(arcana PUBLIC cxx_std_26)

# No toml++ here, deliberately. Every TU that parses TOML lives in src/loader, whose
# objects arrive above already compiled against it, so the dependency -- and the
# TOML_EXCEPTIONS setting that comes with it -- is confined to that one component.
# Anything added here that needs toml++ belongs on the other side of that seam.

set_target_properties(arcana PROPERTIES POSITION_INDEPENDENT_CODE ON)
