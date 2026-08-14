// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

namespace arcana
{

// Splits `text` on `delim`, keeping empty pieces.
[[nodiscard]] constexpr auto pieces(std::string_view text, char delim) noexcept
{
    return text | std::views::split(delim) |
           std::views::transform([](auto part) { return std::string_view{part}; });
}

// The view holds a copy of the string_view
auto pieces(std::string&&, char) = delete;

// Splits `text` at the first `delim`.
//
// @returns before and after, or nothing if delim not found
[[nodiscard]] constexpr std::optional<std::pair<std::string_view, std::string_view>> cut(
    std::string_view text, char delim
) noexcept
{
    auto const at = text.find(delim);
    if (at == std::string_view::npos)
        return std::nullopt;

    return std::pair{text.substr(0, at), text.substr(at + 1)};
}

// The part of `text` before the first `delim`, or all of it if delim not found
[[nodiscard]] constexpr std::string_view before(std::string_view text, char delim) noexcept
{
    return text.substr(0, text.find(delim));
}

// Splits `text` on `delim` and requires every piece to satisfy `piece_ok`.
//
// @returns The number of pieces, or 0 where any piece is rejected.
template <typename Predicate>
[[nodiscard]] std::size_t count_pieces(std::string_view text, char delim, Predicate piece_ok)
{
    auto all = pieces(text, delim);
    if (!std::ranges::all_of(all, piece_ok))
        return 0;

    return static_cast<std::size_t>(std::ranges::distance(all));
}

}  // namespace arcana
