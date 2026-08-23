# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

add_subdirectory(text)
add_subdirectory(identifiers)
add_subdirectory(imagesig)
add_subdirectory(spdx)
add_subdirectory(bcp47)
add_subdirectory(uri)
add_subdirectory(model)
add_subdirectory(vocab)
add_subdirectory(loader)
add_subdirectory(validation)

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
            ${PROJECT_SOURCE_DIR}/include/arcana/loader.hpp
            ${PROJECT_SOURCE_DIR}/include/arcana/library.hpp
            ${PROJECT_SOURCE_DIR}/include/arcana/validation.hpp
    PRIVATE
        version.cpp
        paths.cpp
        library.cpp
        validation.cpp
)

target_compile_features(arcana PUBLIC cxx_std_26)

arcana_link_internal(
    arcana
    PRIVATE
        arcana-identifiers
        arcana-imagesig
        arcana-spdx
        arcana-bcp47
        arcana-uri
        arcana-model
        arcana-vocab
        arcana-loader
        arcana-validation-core
        arcana-check-ansi
        arcana-check-backs
        arcana-check-cards
        arcana-check-deck
        arcana-check-ids
        arcana-check-images
        arcana-check-names
        arcana-check-surrogate
        arcana-validation
)

# BUILD_INTERFACE so we keep toml++ out of the exports
target_link_libraries(arcana PRIVATE $<BUILD_INTERFACE:tomlplusplus::tomlplusplus>)
target_compile_definitions(arcana PRIVATE TOML_EXCEPTIONS=0)

# PIC is required for the shared build and for linking this archive into a Python
# extension module. Keeps toml++'s symbols out of the dynamic symbol table
set_target_properties(
    arcana
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
)
