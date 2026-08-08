// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "image_signature.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

namespace arcana::data
{

image_format sniff_image_format(std::span<std::byte const> head) noexcept
{
    constexpr std::array png_signature{
        std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47},
        std::byte{0x0D}, std::byte{0x0A}, std::byte{0x1A}, std::byte{0x0A},
    };

    // The SOI marker, followed by the first byte of whichever marker comes next.
    constexpr std::array jpeg_signature{std::byte{0xFF}, std::byte{0xD8}, std::byte{0xFF}};

    if (std::ranges::starts_with(head, png_signature))
        return image_format::png;

    if (std::ranges::starts_with(head, jpeg_signature))
        return image_format::jpeg;

    return image_format::unknown;
}

}  // namespace arcana::data
