// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/card.hpp>

#include <array>
#include <string_view>

namespace arcana::detail
{

inline constexpr std::array<std::string_view, 22> default_major_arcana_names{
    "The Fool",         "The Magician", "The High Priestess", "The Empress", "The Emperor",
    "The Hierophant",   "The Lovers",   "The Chariot",        "Strength",    "The Hermit",
    "Wheel of Fortune", "Justice",      "The Hanged Man",     "Death",       "Temperance",
    "The Devil",        "The Tower",    "The Star",           "The Moon",    "The Sun",
    "Judgement",        "The World"
};

inline constexpr std::array<suit, 4> standard_suits{
    suit::wands, suit::cups, suit::swords, suit::pentacles
};

inline constexpr std::array<rank, 14> standard_ranks{
    rank::ace,   rank::two,  rank::three, rank::four, rank::five,   rank::six,   rank::seven,
    rank::eight, rank::nine, rank::ten,   rank::page, rank::knight, rank::queen, rank::king
};

}  // namespace arcana::detail
