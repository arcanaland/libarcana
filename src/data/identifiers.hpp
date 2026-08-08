// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <optional>
#include <string_view>

namespace arcana::data
{

// The identifier grammars of DECK.md sections 3.2, 3.3 and 3.5.

// True when `s` matches the custom-name production: lowercase ASCII letters,
// digits and underscores, never starting with a digit.
[[nodiscard]] bool is_custom_name(std::string_view s) noexcept;

// True when `s` is one of the twenty reserved canonical keys section 3.2 says
// a custom name MUST NOT be: the two arcana, the four suits, the fourteen
// ranks. Grammar and reservation are separate questions and have separate
// diagnostics, so this does not imply is_custom_name.
[[nodiscard]] bool is_reserved_canonical_key(std::string_view s) noexcept;

// True when `s` is a canonical ID: major_arcana.<two digits or custom name>,
// or minor_arcana.<suit>.<rank>.
[[nodiscard]] bool is_canonical_id(std::string_view s) noexcept;

// True when `s` is a card reference: a canonical ID with an optional variant
// suffix, `:` followed by a variant key.
[[nodiscard]] bool is_card_reference(std::string_view s) noexcept;

// True when `s` is a variant reference, which is a card reference whose variant
// suffix is required rather than optional. Name files key [card_variants] by it.
[[nodiscard]] bool is_variant_reference(std::string_view s) noexcept;

// True when `s` is a realm: two or more dot-separated labels, each starting
// with a lowercase letter and neither starting nor ending with a hyphen.
[[nodiscard]] bool is_realm(std::string_view s) noexcept;

// The pieces of a qualified identifier.
//
// The views point into the string that was parsed, which must outlive this.
struct qualified_identifier
{
    std::string_view realm;
    std::string_view path;

    // Empty where the identifier carries none. A deck's fragment is a card
    // reference, but nothing here checks that: section 3.3 leaves the meaning
    // of a fragment to whichever specification owns the entity.
    std::string_view fragment;
};

// The pieces of `s`, or nothing when it is not a well-formed qualified
// identifier.
[[nodiscard]] std::optional<qualified_identifier> parse_qualified_identifier(
    std::string_view s
) noexcept;

// True when `s` is a well-formed qualified identifier.
[[nodiscard]] bool is_qualified_identifier(std::string_view s) noexcept;

}  // namespace arcana::data
