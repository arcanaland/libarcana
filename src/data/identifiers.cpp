// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "identifiers.hpp"

#include "ascii.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace arcana::data
{

namespace
{

// Splits `s` on `sep` and requires every piece to satisfy `piece_ok`.
//
// @returns The number of pieces, or 0 where any piece is empty or rejected.
template <typename Predicate>
std::size_t count_pieces(std::string_view s, char sep, Predicate piece_ok) noexcept
{
    std::size_t pieces = 0;

    while (true)
    {
        auto const sep_at = s.find(sep);
        auto const piece = s.substr(0, sep_at);
        if (!piece_ok(piece))
            return 0;

        ++pieces;
        if (sep_at == std::string_view::npos)
            return pieces;

        s.remove_prefix(sep_at + 1);
    }
}

}  // namespace

bool is_custom_name(std::string_view s) noexcept
{
    if (s.empty())
        return false;

    if (!is_lcalpha(s.front()) && s.front() != '_')
        return false;

    return std::ranges::all_of(s, [](char c) { return is_lcalpha(c) || is_digit(c) || c == '_'; });
}

bool is_reserved_canonical_key(std::string_view s) noexcept
{
    constexpr auto reserved = std::to_array<std::string_view>(
        {"ace",          "cups",         "eight", "five",  "four",      "king",  "knight",
         "major_arcana", "minor_arcana", "nine",  "page",  "pentacles", "queen", "seven",
         "six",          "swords",       "ten",   "three", "two",       "wands"}
    );

    static_assert(std::ranges::is_sorted(reserved), "binary_search needs a sorted table");

    return std::ranges::binary_search(reserved, s);
}

namespace
{

// major-key = canonical-major / custom-name, where canonical-major is 2DIGIT.
bool is_major_key(std::string_view key) noexcept
{
    if (key.size() == 2 && is_digit(key.front()) && is_digit(key.back()))
        return true;

    return is_custom_name(key);
}

}  // namespace

bool is_canonical_id(std::string_view s) noexcept
{
    constexpr std::string_view major_prefix{"major_arcana."};
    constexpr std::string_view minor_prefix{"minor_arcana."};

    if (s.starts_with(major_prefix))
        return is_major_key(s.substr(major_prefix.size()));

    if (!s.starts_with(minor_prefix))
        return false;

    // suit-key and rank-key are both custom-name productions, and a custom name
    // cannot contain a dot, so there is exactly one dot left to split on.
    auto const rest = s.substr(minor_prefix.size());
    auto const dot = rest.find('.');
    if (dot == std::string_view::npos)
        return false;

    return is_custom_name(rest.substr(0, dot)) && is_custom_name(rest.substr(dot + 1));
}

bool is_card_reference(std::string_view s) noexcept
{
    auto const colon = s.find(':');
    if (colon == std::string_view::npos)
        return is_canonical_id(s);

    return is_variant_reference(s);
}

bool is_variant_reference(std::string_view s) noexcept
{
    auto const colon = s.find(':');
    if (colon == std::string_view::npos)
        return false;

    return is_canonical_id(s.substr(0, colon)) && is_custom_name(s.substr(colon + 1));
}

namespace
{

// label = lcalpha [ *61( lcalpha / DIGIT / "-" ) ( lcalpha / DIGIT ) ]
bool is_label(std::string_view label) noexcept
{
    constexpr std::size_t max_label = 63;

    if (label.empty() || label.size() > max_label || !is_lcalpha(label.front()))
        return false;

    if (label.size() == 1)
        return true;

    if (!is_lcalpha(label.back()) && !is_digit(label.back()))
        return false;

    auto const middle = label.substr(1, label.size() - 2);
    return std::ranges::all_of(
        middle, [](char c) { return is_lcalpha(c) || is_digit(c) || c == '-'; }
    );
}

bool is_path_segment(std::string_view segment) noexcept
{
    return !segment.empty() &&
           std::ranges::all_of(
               segment, [](char c) { return is_lcalpha(c) || is_digit(c) || c == '-'; }
           );
}

bool is_fragment(std::string_view fragment) noexcept
{
    return !fragment.empty() && std::ranges::all_of(
                                    fragment,
                                    [](char c)
                                    {
                                        return is_lcalpha(c) || is_digit(c) || c == '.' ||
                                               c == '_' || c == '-' || c == ':';
                                    }
                                );
}

}  // namespace

bool is_realm(std::string_view s) noexcept
{
    // A realm has two labels or more
    return count_pieces(s, '.', is_label) >= 2;
}

std::optional<qualified_identifier> parse_qualified_identifier(std::string_view s) noexcept
{
    qualified_identifier parts;

    // A fragment cannot contain a hash and no other production admits one, so
    // the first hash is the separator.
    if (auto const hash = s.find('#'); hash != std::string_view::npos)
    {
        parts.fragment = s.substr(hash + 1);
        if (!is_fragment(parts.fragment))
            return std::nullopt;

        s = s.substr(0, hash);
    }

    // The realm ends at the first slash.
    auto const slash = s.find('/');
    if (slash == std::string_view::npos)
        return std::nullopt;

    parts.realm = s.substr(0, slash);
    parts.path = s.substr(slash + 1);

    if (!is_realm(parts.realm) || count_pieces(parts.path, '/', is_path_segment) == 0)
        return std::nullopt;

    return parts;
}

bool is_qualified_identifier(std::string_view s) noexcept
{
    return parse_qualified_identifier(s).has_value();
}

}  // namespace arcana::data
