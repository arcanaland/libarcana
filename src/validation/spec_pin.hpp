// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Which revision of the specification each schema major's citations were read
// against, and where that text lives.

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <utility>

namespace arcana::validation
{

struct spec_pin
{
    std::uint8_t schema_major;

    // The repository, minus the revision and the filename.
    std::string_view repository;

    std::string_view revision;
    std::string_view file;
};

inline constexpr std::array spec_pins{
    spec_pin{
        .schema_major = 1,
        .repository = "https://github.com/arcanaland/specifications",
        .revision = "29f2184b8fc29e1db016c1f4a3d0c96bac8a4217",
        .file = "README.md",
    },
    spec_pin{
        .schema_major = 2,
        .repository = "https://github.com/arcanaland/specifications",
        .revision = "f32d330cdcd84190a80e357ec7b826c4befe5446",
        .file = "DECK.md",
    },
};

// Whether this file pins any text for this major.
[[nodiscard]] constexpr bool has_pin(std::uint8_t schema_major) noexcept
{
    return std::ranges::find(spec_pins, schema_major, &spec_pin::schema_major) != spec_pins.end();
}

// The pin for this major.
[[nodiscard]] constexpr spec_pin const& pin_for(std::uint8_t schema_major) noexcept
{
    auto const found = std::ranges::find(spec_pins, schema_major, &spec_pin::schema_major);
    if (found == spec_pins.end())
        std::unreachable();

    return *found;
}

}  // namespace arcana::validation
