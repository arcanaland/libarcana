// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "ordering.hpp"

#include "standard_cards.hpp"

#include <algorithm>
#include <compare>
#include <format>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace arcana::detail
{

namespace
{

struct card_order
{
    int arcana = 0;      // 0 for a major arcanum, 1 for a minor
    int suit_order = 0;  // minor only

    // 1 where nothing ranks this card: a major with no position, or a rank its
    // suit's sequence does not name.
    int unranked = 0;

    // A major's position, or a minor's index in its suit's `ranks` sequence
    long long position = 0;

    // Major only: 1 for a position implied by a two-digit key, so that a
    // declared position precedes an implicit one at the same value
    int implicit = 0;

    // The final tie-break: a major's key, or a minor's rank key
    std::string key;

    friend auto operator<=>(card_order const&, card_order const&) = default;
};

std::string major_key_of(card_id const& id)
{
    if (id.cls == card_class::standard_major)
        return std::format("{:02}", id.number);

    return id.custom_id;
}

std::string suit_key_of(card_id const& id)
{
    if (id.cls == card_class::standard_minor)
        return std::string{to_string(id.standard_suit)};

    return id.suit_key;
}

std::string rank_key_of(card_id const& id)
{
    if (id.cls == card_class::standard_minor)
        return std::string{to_string(id.standard_rank)};

    return id.custom_id;
}

// The four canonical suits come first in their canonical order, and every other
// suit the deck has follows them sorted by key
std::vector<std::string> suit_order_of(std::span<suit_info const> suits)
{
    std::vector<std::string> order;
    order.reserve(suits.size());

    for (auto const s : standard_suits) order.emplace_back(to_string(s));

    std::vector<std::string> custom;
    for (auto const& info : suits)
        if (std::ranges::find(order, info.key) == order.end())
            custom.push_back(info.key);

    std::ranges::sort(custom);
    order.insert(order.end(), custom.begin(), custom.end());

    return order;
}

card_order order_of(
    card const& c, std::vector<std::string> const& suit_order, std::span<suit_info const> suits
)
{
    if (c.id.is_major())
    {
        card_order result{.arcana = 0, .key = major_key_of(c.id)};

        if (c.position)
        {
            result.position = *c.position;
        }
        else if (c.id.cls == card_class::standard_major)
        {
            // A two-digit key carries an implicit position equal to its value
            result.position = c.id.number;
            result.implicit = 1;
        }
        else
        {
            result.unranked = 1;
        }

        return result;
    }

    auto const suit_key = suit_key_of(c.id);
    auto const rank_key = rank_key_of(c.id);

    card_order result{.arcana = 1, .key = rank_key};

    auto const ordered = std::ranges::find(suit_order, suit_key);
    result.suit_order = ordered == suit_order.end()
                            ? static_cast<int>(suit_order.size())
                            : static_cast<int>(std::ranges::distance(suit_order.begin(), ordered));

    result.unranked = 1;
    if (auto const info = std::ranges::find(suits, suit_key, &suit_info::key); info != suits.end())
    {
        if (auto const ranked = std::ranges::find(info->ranks, rank_key);
            ranked != info->ranks.end())
        {
            result.position = std::ranges::distance(info->ranks.begin(), ranked);
            result.unranked = 0;
        }
    }

    return result;
}

}  // namespace

void sort_cards(std::span<card> cards, std::span<suit_info const> suits)
{
    auto const suit_order = suit_order_of(suits);

    std::vector<std::pair<card_order, card>> decorated;
    decorated.reserve(cards.size());
    for (auto& c : cards) decorated.emplace_back(order_of(c, suit_order, suits), std::move(c));

    std::ranges::sort(decorated, {}, &std::pair<card_order, card>::first);

    std::ranges::move(decorated | std::views::values, cards.begin());
}

}  // namespace arcana::detail
