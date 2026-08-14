# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

add_library(arcana-data OBJECT)
add_library(arcana::data ALIAS arcana-data)

set_target_properties(
    arcana-data
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
)

target_sources(
    arcana-data
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            identifiers.hpp
            image_signature.hpp
            language_tag.hpp
            scanners.hpp
            spdx_expression.hpp
            spdx_licenses.hpp
            tables.hpp
            text.hpp
            uri.hpp
    PRIVATE
        # ascii.hpp is internal
        ascii.hpp
        identifiers.cpp
        image_signature.cpp
        language_tag.cpp
        spdx_expression.cpp
        spdx_licenses.cpp
        tables.cpp
        uri.cpp
)

target_compile_features(arcana-data PUBLIC cxx_std_26)
