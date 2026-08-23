# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Image format detection by magic bytes. Depends on nothing -- it reads a span
# of bytes and names a format.
arcana_add_internal_library(
    imagesig
    HEADERS image_signature.hpp
    SOURCES image_signature.cpp
)
