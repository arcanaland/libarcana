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

    // A WebP file is a RIFF container whose four-byte form type is `WEBP`, sitting
    // after the four-byte chunk size that follows the `RIFF` tag.
    constexpr std::array riff_tag{
        std::byte{0x52}, std::byte{0x49}, std::byte{0x46}, std::byte{0x46}
    };
    constexpr std::array webp_form{
        std::byte{0x57}, std::byte{0x45}, std::byte{0x42}, std::byte{0x50}
    };
    constexpr std::size_t form_offset = 8;

    if (std::ranges::starts_with(head, riff_tag) && head.size() >= form_offset + webp_form.size() &&
        std::ranges::equal(head.subspan(form_offset, webp_form.size()), webp_form))
        return image_format::webp;

    return image_format::unknown;
}

}  // namespace arcana::data
