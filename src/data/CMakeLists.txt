# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_internal_library(
    data
    HEADERS
        ascii.hpp
        identifiers.hpp
        image_signature.hpp
        language_tag.hpp
        scanners.hpp
        spdx_expression.hpp
        spdx_licenses.hpp
        tables.hpp
        text.hpp
        uri.hpp
    SOURCES
        identifiers.cpp
        image_signature.cpp
        language_tag.cpp
        spdx_expression.cpp
        spdx_licenses.cpp
        tables.cpp
        uri.cpp
)
