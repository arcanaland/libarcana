// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Where the deck spec puts things, and which major puts it there. Every section
// and key name a check needs is named here rather than inline, so that a spec
// revision is a diff in this file instead of a grep across the check bodies.

#pragma once

#include <arcana/deck.hpp>

#include <toml++/toml.hpp>

#include <cstdint>
#include <string_view>

namespace arcana::validation
{

// The major this library implements, and what a deck declaring no schema
// version is read as.
constexpr std::uint8_t current_schema_major = 2;

// The major a deck is read under: the one it declares, or the current one.
[[nodiscard]] std::uint8_t major_of(deck const& d) noexcept;

// The key under `[card_backs]` holding the designs: `variants` before 2.0,
// `designs` from 2.0. DECK.md 5.5.
[[nodiscard]] std::string_view card_back_designs_key(std::uint8_t major) noexcept;

// That table, where the deck has one.
[[nodiscard]] toml::table const* card_back_designs(toml::table const& doc, std::uint8_t major);

}  // namespace arcana::validation
