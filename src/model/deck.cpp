// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/deck.hpp>

#include <deck_state.hpp>
#include <text.hpp>

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

std::string detail::deck_state::display_suit_name(suit s) const
{
    return display_suit_name(to_string(s));
}

std::string detail::deck_state::display_suit_name(std::string_view suit_key) const
{
    auto const it = std::ranges::find(suits, suit_key, &suit_info::key);
    if (it != suits.end() && !it->name.empty())
        return it->name;

    return titlecase_key(suit_key);
}

std::string detail::deck_state::display_rank_name(rank r) const
{
    return display_rank_name(to_string(r));
}

std::string detail::deck_state::display_rank_name(std::string_view rank_key) const
{
    if (auto const it = rank_names.find(std::string(rank_key)); it != rank_names.end())
        return it->second;

    return titlecase_key(rank_key);
}

std::vector<rank_info> detail::deck_state::ordered_ranks(
    std::span<std::string const> extra_keys
) const
{
    std::vector<rank_info> result;

    auto const add = [&](std::string const& key)
    {
        if (std::ranges::find(result, key, &rank_info::key) != result.end())
            return;

        result.push_back({.key = key, .name = display_rank_name(key), .declared = {}});
    };

    for (auto const& info : suits)
        for (auto const& key : info.ranks) add(key);

    auto remaining = std::vector<std::string>(extra_keys.begin(), extra_keys.end());
    std::ranges::sort(remaining);
    for (auto const& key : remaining) add(key);

    return result;
}

std::optional<std::string> detail::deck_state::exclusion_reason(std::string_view canonical_id) const
{
    if (std::ranges::find(excluded.cards, canonical_id) == excluded.cards.end())
        return std::nullopt;

    return excluded.reason.value_or(std::string{});
}

std::optional<card> detail::deck_state::find_card(card_id const& id) const
{
    auto const it = std::ranges::find(cards, id, &card::id);
    if (it == cards.end())
        return std::nullopt;

    return *it;
}

std::vector<card> detail::deck_state::cards_of_kind(arcana_kind kind) const
{
    return cards | std::views::filter([kind](card const& c) { return c.id.kind() == kind; }) |
           std::ranges::to<std::vector>();
}

std::vector<card> detail::deck_state::cards_in_suit(std::string_view key) const
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

std::optional<card> detail::deck_state::random_card(std::uint64_t seed) const
{
    if (cards.empty())
        return std::nullopt;

    std::mt19937_64 engine{seed};
    std::uniform_int_distribution<std::size_t> pick{0, cards.size() - 1};
    return cards[pick(engine)];
}

std::optional<card_back_design> detail::deck_state::default_card_back_design() const
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

// --- The handle forwards ---------------------------------------------------

std::filesystem::path const& deck::root_path() const noexcept
{
    return state_->root_path;
}

deck_metadata const& deck::metadata() const noexcept
{
    return state_->metadata;
}

std::span<esoterica_companion const> deck::companions() const noexcept
{
    return state_->companions;
}

std::span<card_back_design const> deck::card_backs() const noexcept
{
    return state_->card_backs;
}

excluded_cards const& deck::excluded() const noexcept
{
    return state_->excluded;
}

std::span<suit_info const> deck::suits() const noexcept
{
    return state_->suits;
}

std::span<rank_info const> deck::ranks() const noexcept
{
    return state_->ranks;
}

std::optional<std::string_view> deck::minor_arcana_name_template() const noexcept
{
    if (!state_->minor_arcana_name_template)
        return std::nullopt;

    return *state_->minor_arcana_name_template;
}

std::span<card const> deck::cards() const noexcept
{
    return state_->cards;
}

std::string deck::display_suit_name(suit s) const
{
    return state_->display_suit_name(s);
}

std::string deck::display_suit_name(std::string_view suit_key) const
{
    return state_->display_suit_name(suit_key);
}

std::string deck::display_rank_name(rank r) const
{
    return state_->display_rank_name(r);
}

std::string deck::display_rank_name(std::string_view rank_key) const
{
    return state_->display_rank_name(rank_key);
}

std::optional<std::string> deck::exclusion_reason(std::string_view canonical_id) const
{
    return state_->exclusion_reason(canonical_id);
}

std::vector<card> deck::cards_of_kind(arcana_kind kind) const
{
    return state_->cards_of_kind(kind);
}

std::vector<card> deck::cards_in_suit(std::string_view key) const
{
    return state_->cards_in_suit(key);
}

std::optional<card> deck::find_card(card_id const& id) const
{
    return state_->find_card(id);
}

std::optional<card> deck::random_card(std::uint64_t seed) const
{
    return state_->random_card(seed);
}

std::optional<std::string> const& deck::default_card_back() const noexcept
{
    return state_->default_card_back;
}

std::optional<card_back_design> deck::default_card_back_design() const
{
    return state_->default_card_back_design();
}

std::string deck::source_toml() const
{
    return state_->source_toml();
}

}  // namespace arcana
