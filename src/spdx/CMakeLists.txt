# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# SPDX license expressions and the vendored SPDX License List. This target is
# the REUSE boundary: spdx_licenses.cpp is the repo's one dual-licensed file
# (MIT for the code shape, CC-BY-3.0 for the Linux Foundation's data).
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
