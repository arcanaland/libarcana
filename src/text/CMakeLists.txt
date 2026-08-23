# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# ASCII predicates and the small string helpers every other vocabulary is built
# out of. Header-only, and the bottom of the graph -- it depends on nothing.
arcana_add_internal_library(
    text
    HEADERS
        ascii.hpp
        text.hpp
)
