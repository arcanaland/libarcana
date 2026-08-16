# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

add_library(arcana-loader OBJECT)
add_library(arcana::loader ALIAS arcana-loader)

set_target_properties(
    arcana-loader
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
)

target_sources(
    arcana-loader
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            deck_access.hpp
            discovery.hpp
            document.hpp
            manifest.hpp
            names.hpp
            ordering.hpp
            reader.hpp
            schema_version.hpp
            standard_cards.hpp
            summary.hpp
            toml_read.hpp
            v1_compat/reader.hpp
            v1_compat/tables.hpp
    PRIVATE
        discovery.cpp
        document.cpp
        loader.cpp
        names.cpp
        ordering.cpp
        reader.cpp
        schema_version.cpp
        summary.cpp
        v1_compat/reader.cpp
)

target_compile_features(arcana-loader PUBLIC cxx_std_26)

target_include_directories(arcana-loader PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(arcana-loader PUBLIC tomlplusplus::tomlplusplus)
target_compile_definitions(arcana-loader PUBLIC TOML_EXCEPTIONS=0)
