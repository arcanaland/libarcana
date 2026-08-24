// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Which revision of the specification each schema major's citations were read
// against, and where that text lives.

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace arcana::validation
{

// A commit and not a branch. deck-v2's tip moved a dozen times while this
// catalogue was written, so a branch URL would routinely resolve to text no
// rule here was derived from.
struct spec_pin
{
    std::uint8_t schema_major;

    // The repository, minus the revision and the filename.
    std::string_view repository;

    std::string_view revision;

    // The v1.0 text predates the README.md -> DECK.md rename at e7da516, so
    // the two majors are not the same file.
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

// The pin for this major, or nullptr where the catalogue cites no text for it.
[[nodiscard]] constexpr spec_pin const* pin_for(std::uint8_t schema_major) noexcept
{
    auto const found = std::ranges::find(spec_pins, schema_major, &spec_pin::schema_major);

    return found == spec_pins.end() ? nullptr : &*found;
}

}  // namespace arcana::validation
