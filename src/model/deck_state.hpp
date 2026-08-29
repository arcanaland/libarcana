// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/card.hpp>
#include <arcana/deck.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace arcana::detail
{

// The parsed deck.toml
struct deck_document;

// Everything a load produces
struct deck_state
{
    std::filesystem::path root_path;
    deck_metadata metadata;
    std::vector<esoterica_companion> companions;

    std::vector<card_back_design> card_backs;

    excluded_cards excluded;

    // Every suit this deck has, canonical suits first
    std::vector<suit_info> suits;

    // The 78 standard cards minus exclusions, plus the deck's own cards
    std::vector<card> cards;

    // `[card_backs].default`
    std::optional<std::string> default_card_back;

    // Rank key -> the display name this deck resolved for it. Filled at load
    // from [aliases.courts] in 1.0 and from a name file's [name.rank] in 2.0;
    // v2 gives ranks no manifest field, so there is no public map
    std::unordered_map<std::string, std::string> rank_names;

    // So toml++ stays out of the public header
    std::shared_ptr<deck_document const> document;

    // The queries deck forwards. A reader calls the naming ones while it is
    // still building, which is why they live here rather than on the handle
    [[nodiscard]] std::string display_suit_name(suit s) const;
    [[nodiscard]] std::string display_suit_name(std::string_view suit_key) const;
    [[nodiscard]] std::string display_rank_name(rank r) const;
    [[nodiscard]] std::string display_rank_name(std::string_view rank_key) const;
    [[nodiscard]] std::optional<std::string> exclusion_reason(std::string_view canonical_id) const;
    [[nodiscard]] std::vector<card> cards_of_kind(arcana_kind kind) const;
    [[nodiscard]] std::vector<card> cards_in_suit(std::string_view key) const;
    [[nodiscard]] std::optional<card> find_card(card_id const& id) const;
    [[nodiscard]] std::optional<card> random_card(std::uint64_t seed) const;
    [[nodiscard]] std::optional<card_back_design> default_card_back_design() const;

    [[nodiscard]] std::string source_toml() const;
};

struct deck_builder
{
    [[nodiscard]] static deck make(std::shared_ptr<deck_state const> state) noexcept
    {
        return deck{std::move(state)};
    }

    [[nodiscard]] static deck make(deck_state&& state)
    {
        return deck{std::make_shared<deck_state const>(std::move(state))};
    }
};

}  // namespace arcana::detail
