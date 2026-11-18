// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Which revision of the specification each schema major's citations were read
// against, and where that text lives.

#pragma once

#include <arcana/validation.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace arcana::validation
{

struct pinned_text
{
    std::uint8_t schema_major;

    // The repository, minus the revision and the filename.
    std::string_view repository;

    std::string_view revision;
    std::string_view label;
    std::string_view file;
};

inline constexpr std::array pinned_texts{
    pinned_text{
        .schema_major = 1,
        .repository = "https://github.com/arcanaland/specifications",
        .revision = "5396d0acb378a7d4b7fe139b4918c1f0f5627198",
        .label = "v1.0",
        .file = "DECK.md",
    },
    pinned_text{
        .schema_major = 2,
        .repository = "https://github.com/arcanaland/specifications",
        .revision = "2d203ddde772a74607c2cc3755e2260be04481fa",
        .label = "v2.0-rc.1",
        .file = "DECK.md",
    },
};

static_assert(
    std::ranges::is_sorted(pinned_texts, {}, &pinned_text::schema_major) &&
        std::ranges::adjacent_find(pinned_texts, {}, &pinned_text::schema_major) ==
            pinned_texts.end(),
    "pins must be strictly ascending by schema_major"
);

// The same pins, as the public record.
inline constexpr auto public_pins = []
{
    std::array<spec_pin, pinned_texts.size()> out{};
    for (std::size_t i = 0; i < pinned_texts.size(); ++i)
        out[i] = spec_pin{
            .schema_major = pinned_texts[i].schema_major,
            .revision = pinned_texts[i].revision,
            .label = pinned_texts[i].label,
        };
    return out;
}();

// Whether this file pins any text for this major.
[[nodiscard]] constexpr bool has_pin(std::uint8_t schema_major) noexcept
{
    return std::ranges::find(pinned_texts, schema_major, &pinned_text::schema_major) !=
           pinned_texts.end();
}

// The pin for this major.
[[nodiscard]] constexpr pinned_text const& pin_for(std::uint8_t schema_major) noexcept
{
    auto const found = std::ranges::find(pinned_texts, schema_major, &pinned_text::schema_major);
    if (found == pinned_texts.end())
        std::unreachable();

    return *found;
}

}  // namespace arcana::validation
