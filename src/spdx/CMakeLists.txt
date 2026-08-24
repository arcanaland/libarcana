# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_internal_library(
    spdx
    HEADERS
        spdx_expression.hpp
        spdx_licenses.hpp
    SOURCES
        spdx_expression.cpp
        spdx_licenses.cpp
)

arcana_link_internal(arcana-spdx PRIVATE arcana-text)
