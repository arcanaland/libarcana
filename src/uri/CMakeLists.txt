# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_internal_library(
    uri
    HEADERS uri.hpp
    SOURCES uri.cpp
)

arcana_link_internal(arcana-uri PRIVATE arcana-text)
