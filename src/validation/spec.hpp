// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/deck.hpp>

#include <toml++/toml.hpp>

#include <cstdint>
#include <string_view>

namespace arcana::validation
{

// The major this library implements
constexpr std::uint8_t current_schema_major = 2;

// The major a deck is read under
[[nodiscard]] std::uint8_t major_of(deck const& d) noexcept;

// The key under [card_backs] that we need:
//   for 2.0+, it's "designs"
//   for 1.0, it's "variants"
[[nodiscard]] std::string_view card_back_designs_key(std::uint8_t major) noexcept;

[[nodiscard]] toml::table const* card_back_designs_table(
    toml::table const& doc, std::uint8_t major
);

}  // namespace arcana::validation
