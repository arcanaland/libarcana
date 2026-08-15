// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "image_signature.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

namespace arcana::data
{
constexpr std::array png_signature{
    std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47},
    std::byte{0x0D}, std::byte{0x0A}, std::byte{0x1A}, std::byte{0x0A},
};

constexpr std::array riff_tag{std::byte{0x52}, std::byte{0x49}, std::byte{0x46}, std::byte{0x46}};
constexpr std::array webp_form{std::byte{0x57}, std::byte{0x45}, std::byte{0x42}, std::byte{0x50}};
constexpr std::array jpeg_signature{std::byte{0xFF}, std::byte{0xD8}, std::byte{0xFF}};

// Spelled out rather than `std::ranges::starts_with`, which libstdc++ does not
// carry until GCC 15. The wheel builds in manylinux, whose gcc-toolset-14 is a
// lower compiler floor than the fedora:44 image the C++ gates use.
[[nodiscard]] constexpr bool starts_with(
    std::span<std::byte const> head, std::span<std::byte const> signature
) noexcept
{
    return head.size() >= signature.size() &&
           std::ranges::equal(head.first(signature.size()), signature);
}

image_format sniff_image_format(std::span<std::byte const> head) noexcept
{
    if (starts_with(head, png_signature))
        return image_format::png;

    if (starts_with(head, jpeg_signature))
        return image_format::jpeg;

    constexpr std::size_t webp_form_offset = 8;

    if (starts_with(head, riff_tag) && head.size() >= webp_form_offset + webp_form.size() &&
        std::ranges::equal(head.subspan(webp_form_offset, webp_form.size()), webp_form))
        return image_format::webp;

    return image_format::unknown;
}

}  // namespace arcana::data
