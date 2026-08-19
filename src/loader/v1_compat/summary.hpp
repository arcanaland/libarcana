// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <toml++/toml.hpp>

#include <cstddef>

namespace arcana::detail::v1_compat
{

// How many cards a 1.0 deck has
[[nodiscard]] std::size_t count_cards(toml::table const& document);

}  // namespace arcana::detail::v1_compat
