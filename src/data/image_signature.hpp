// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace arcana::data
{

// The image formats of DECK.md section 5.5.

// The baseline raster formats a card back may be supplied in.
enum class image_format : std::uint8_t
{
    unknown,
    png,
    jpeg,
};

// The format `head` begins with, judged by its signature bytes.
//
// Signature bytes, not decoding: no image library enters this tree. `head` need
// only be the first few bytes of the file.
[[nodiscard]] image_format sniff_image_format(std::span<std::byte const> head) noexcept;

}  // namespace arcana::data
