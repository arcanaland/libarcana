// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The query side of a loaded deck. Building one lives in src/loader, and this file
// deliberately does not include toml++: everything below reads the deck struct's own
// fields. The one exception is deck::source_toml(), which needs the retained document
// and so is defined in src/loader/document.cpp.

#include <arcana/deck.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <random>
#include <ranges>

namespace arcana
{

namespace
{

std::string titlecase_key(std::string_view key)
{
    std::string result;
    result.reserve(key.size());

    bool at_word_start = true;
    for (char const c : key)
    {
        if (c == '_')
        {
            result.push_back(' ');
            at_word_start = true;
            continue;
        }
        result.push_back(
            at_word_start ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c
        );
        at_word_start = false;
    }

    return result;
}

}  // namespace

std::string deck::display_suit_name(suit s) const
{
    auto canonical = std::string(to_string(s));
    if (auto const it = suit_aliases.find(canonical); it != suit_aliases.end())
        return it->second;
    return titlecase_key(canonical);
}

std::string deck::display_suit_name(std::string_view custom_suit_key) const
{
    if (auto const it = suit_aliases.find(std::string(custom_suit_key)); it != suit_aliases.end())
        return it->second;
    for (auto const& suit_def : custom_suits)
        if (suit_def.key == custom_suit_key)
            return suit_def.name;
    return titlecase_key(custom_suit_key);
}

std::string deck::display_rank_name(rank r) const
{
    return display_rank_name(to_string(r));
}

std::string deck::display_rank_name(std::string_view custom_rank_key) const
{
    if (auto const it = court_aliases.find(std::string(custom_rank_key)); it != court_aliases.end())
        return it->second;
    return titlecase_key(custom_rank_key);
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

std::vector<suit_info> deck::suits() const
{
    auto const suit_of = [](card const& c) -> std::string_view
    {
        switch (c.id.cls)
        {
            case card_class::standard_minor:
                return to_string(c.id.standard_suit);
            case card_class::custom_minor:
                return c.id.suit_key;
            case card_class::standard_major:
            case card_class::custom_major:
                break;
        }
        return {};
    };

    auto const has_any_card = [this, &suit_of](std::string_view key)
    {
        return std::ranges::any_of(
            cards, [key, &suit_of](card const& c) { return suit_of(c) == key; }
        );
    };

    std::vector<suit_info> result;

    constexpr std::array<suit, 4> canonical{suit::wands, suit::cups, suit::swords, suit::pentacles};
    for (auto const s : canonical)
    {
        auto key = std::string(to_string(s));
        result.push_back(
            suit_info{
                .key = key,
                .display_name = display_suit_name(s),
                .standard = true,
                .excluded = !has_any_card(key)
            }
        );
    }

    // deck.toml order, not sorted: a deck author's ordering of their own suits is the
    // only ordering information there is.
    for (auto const& custom : custom_suits)
    {
        result.push_back(
            suit_info{
                .key = custom.key,
                .display_name = display_suit_name(custom.key),
                .standard = false,
                .excluded = !has_any_card(custom.key)
            }
        );
    }

    return result;
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

std::optional<card_back_variant> deck::default_card_back_variant() const
{
    if (default_card_back)
    {
        auto const it = std::ranges::find(card_backs, *default_card_back, &card_back_variant::id);
        if (it != card_backs.end())
            return *it;
    }

    if (card_backs.size() == 1)
        return card_backs.front();

    return std::nullopt;
}

}  // namespace arcana
