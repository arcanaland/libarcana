// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/deck.hpp>

#include "data/text.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <limits>
#include <random>
#include <ranges>
#include <system_error>

namespace arcana
{

std::optional<std::uint8_t> schema_major(deck_metadata const& metadata) noexcept
{
    auto const& text = metadata.schema_version;

    auto const split = cut(text, '.');
    if (!split)
        return std::nullopt;

    auto const all_digits = [](std::string_view part)
    {
        return !part.empty() &&
               std::ranges::all_of(part, [](char const c) { return c >= '0' && c <= '9'; });
    };

    if (!all_digits(split->first) || !all_digits(split->second))
        return std::nullopt;

    unsigned major = 0;
    auto const major_text = split->first;
    auto const [_, ec] =
        std::from_chars(major_text.data(), major_text.data() + major_text.size(), major);
    if (ec != std::errc{} || major > std::numeric_limits<std::uint8_t>::max())
        return std::nullopt;

    return static_cast<std::uint8_t>(major);
}

std::string deck::display_suit_name(suit s) const
{
    return display_suit_name(to_string(s));
}

std::string deck::display_suit_name(std::string_view suit_key) const
{
    auto const it = std::ranges::find(suits, suit_key, &suit_info::key);
    if (it != suits.end() && !it->name.empty())
        return it->name;

    return titlecase_key(suit_key);
}

std::string deck::display_rank_name(rank r) const
{
    return display_rank_name(to_string(r));
}

std::string deck::display_rank_name(std::string_view rank_key) const
{
    if (auto const it = rank_names_.find(std::string(rank_key)); it != rank_names_.end())
        return it->second;

    return titlecase_key(rank_key);
}

std::optional<std::string> deck::exclusion_reason(std::string_view canonical_id) const
{
    if (std::ranges::find(excluded.cards, canonical_id) == excluded.cards.end())
        return std::nullopt;

    return excluded.reason.value_or(std::string{});
}

std::optional<card> deck::find_card(card_id const& id) const
{
    auto const it = std::ranges::find(cards, id, &card::id);
    if (it == cards.end())
        return std::nullopt;

    return *it;
}

std::vector<card> deck::cards_of_kind(arcana_kind kind) const
{
    return cards | std::views::filter([kind](card const& c) { return c.id.kind() == kind; }) |
           std::ranges::to<std::vector>();
}

std::vector<card> deck::cards_in_suit(std::string_view key) const
{
    std::vector<card> result;

    auto const canonical = suit_from_string(key);
    for (auto const& c : cards)
    {
        bool const match =
            canonical ? c.id.cls == card_class::standard_minor && c.id.standard_suit == *canonical
                      : c.id.cls == card_class::custom_minor && c.id.suit_key == key;
        if (match)
            result.push_back(c);
    }

    return result;
}

std::optional<card> deck::random_card(std::uint64_t seed) const
{
    if (cards.empty())
        return std::nullopt;

    std::mt19937_64 engine{seed};
    std::uniform_int_distribution<std::size_t> pick{0, cards.size() - 1};
    return cards[pick(engine)];
}

std::optional<card_back_design> deck::default_card_back_design() const
{
    if (default_card_back)
    {
        auto const it = std::ranges::find(card_backs, *default_card_back, &card_back_design::id);
        if (it != card_backs.end())
            return *it;
    }

    if (card_backs.size() == 1)
        return card_backs.front();

    return std::nullopt;
}

}  // namespace arcana
