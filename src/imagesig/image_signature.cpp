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

// TODO: use std::ranges::starts_with when we can bump the Python wheel toolchain
[[nodiscard]] constexpr bool starts_with(
    std::span<std::byte const> head, std::span<std::byte const> signature
) noexcept
{
    return head.size() >= signature.size() &&
           std::ranges::equal(head.first(signature.size()), signature);
}

constexpr std::size_t webp_form_offset = 8;

static_assert(image_signature_bytes >= png_signature.size());
static_assert(image_signature_bytes >= jpeg_signature.size());
static_assert(image_signature_bytes >= webp_form_offset + webp_form.size());

image_format sniff_image_format(std::span<std::byte const> head) noexcept
{
    if (starts_with(head, png_signature))
        return image_format::png;

    if (starts_with(head, jpeg_signature))
        return image_format::jpeg;

    if (starts_with(head, riff_tag) && head.size() >= webp_form_offset + webp_form.size() &&
        std::ranges::equal(head.subspan(webp_form_offset, webp_form.size()), webp_form))
        return image_format::webp;

    return image_format::unknown;
}

}  // namespace arcana::data
