// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/validation.hpp>

#include <optional>
#include <span>
#include <string_view>

namespace arcana::validation
{

// Every validation rule
[[nodiscard]] std::span<rule const> all_rules() noexcept;

// Find the validation rule from a code
[[nodiscard]] rule const* lookup(std::string_view code) noexcept;

// Whether or not a validation rule is implemented
[[nodiscard]] std::optional<rule_state> state_of_code(std::string_view code) noexcept;

}  // namespace arcana::validation
