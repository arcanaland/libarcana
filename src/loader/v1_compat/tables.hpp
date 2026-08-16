// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace arcana::detail::v1_compat
{

// One [custom_cards.major_arcana.<key>] or [custom_cards.minor_arcana.<suit>] (v1)
struct custom_card_def
{
    std::string id;
    std::string name;

    // The deck-relative reference exactly as written in deck.toml
    std::string image_ref;

    std::optional<std::string> alt_text;
    std::optional<int> position;
};

// An entire new suit from [custom_cards.minor_arcana] (v1)
struct custom_suit_def
{
    std::string key;
    std::string name;
    std::vector<custom_card_def> cards;
};

}  // namespace arcana::detail::v1_compat
