// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The diagnostic catalogue.
//
// Declarations only, deliberately. The table itself is ~1,000 lines of
// constexpr `rule` and is compiled in catalogue.cpp alone; a header carrying it
// would be paid for once per translation unit that included it.

#pragma once

#include <arcana/validation.hpp>

#include <optional>
#include <span>
#include <string_view>

namespace arcana::validation
{

// Every rule, sorted strictly ascending by code.
[[nodiscard]] std::span<rule const> all_rules() noexcept;

// The rule with this code, or nullptr.
[[nodiscard]] rule const* lookup(std::string_view code) noexcept;

// What the dispatch table does about this code, or nullopt where the catalogue
// does not carry it.
[[nodiscard]] std::optional<rule_state> state_of_code(std::string_view code) noexcept;

}  // namespace arcana::validation
