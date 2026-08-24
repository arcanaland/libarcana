// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/validation.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace arcana::validation
{

// Every validation rule
[[nodiscard]] std::span<rule const> all_rules() noexcept;

// Find the validation rule from a code
[[nodiscard]] rule const* lookup(std::string_view code) noexcept;

// Whether or not a validation rule is implemented
[[nodiscard]] std::optional<rule_state> state_of_code(std::string_view code) noexcept;

// The pinned commit this schema major's citations were read against
[[nodiscard]] std::string_view revision_of(std::uint8_t schema_major) noexcept;

// A resolvable URL for one citation
[[nodiscard]] std::string url_of(spec_section section);

}  // namespace arcana::validation
