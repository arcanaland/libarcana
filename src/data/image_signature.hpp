// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace arcana::data
{

// The baseline raster formats a card back may be supplied in.
enum class image_format : std::uint8_t
{
    unknown,
    png,
    jpeg,
};

// Sniff a stream of bytes for magic numbers of png/jpeg
[[nodiscard]] image_format sniff_image_format(std::span<std::byte const> head) noexcept;

}  // namespace arcana::data
