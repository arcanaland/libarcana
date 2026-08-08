// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <string_view>

namespace arcana::data
{

// The character classes the scanners share.
//
// ASCII only, and deliberately so: every grammar the deck specification defines
// is an ASCII one, so nothing here consults a locale.

constexpr bool is_lcalpha(char c) noexcept
{
    return c >= 'a' && c <= 'z';
}

constexpr bool is_digit(char c) noexcept
{
    return c >= '0' && c <= '9';
}

constexpr bool is_alpha(char c) noexcept
{
    return is_lcalpha(c) || (c >= 'A' && c <= 'Z');
}

constexpr bool is_alnum(char c) noexcept
{
    return is_alpha(c) || is_digit(c);
}

constexpr char to_lower(char c) noexcept
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

constexpr char to_upper(char c) noexcept
{
    return is_lcalpha(c) ? static_cast<char>(c - 'a' + 'A') : c;
}

constexpr bool equal_ignoring_case(std::string_view left, std::string_view right) noexcept
{
    return std::ranges::equal(
        left, right, [](char one, char two) { return to_lower(one) == to_lower(two); }
    );
}

}  // namespace arcana::data
