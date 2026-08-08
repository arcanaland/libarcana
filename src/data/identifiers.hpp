// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <optional>
#include <string_view>

namespace arcana::data
{

[[nodiscard]] bool is_custom_name(std::string_view s) noexcept;

[[nodiscard]] bool is_reserved_canonical_key(std::string_view s) noexcept;

[[nodiscard]] bool is_canonical_id(std::string_view s) noexcept;

[[nodiscard]] bool is_card_reference(std::string_view s) noexcept;

[[nodiscard]] bool is_variant_reference(std::string_view s) noexcept;

[[nodiscard]] bool is_realm(std::string_view s) noexcept;

// The pieces of a qualified identifier.
//
// The views point into the string that was parsed, which must outlive this.
struct qualified_identifier
{
    std::string_view realm;
    std::string_view path;

    // TODO: optional?
    std::string_view fragment;
};

[[nodiscard]] std::optional<qualified_identifier> parse_qualified_identifier(
    std::string_view s
) noexcept;

[[nodiscard]] bool is_qualified_identifier(std::string_view s) noexcept;

}  // namespace arcana::data
