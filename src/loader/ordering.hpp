// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/card.hpp>
#include <arcana/deck.hpp>

#include <vector>

namespace arcana::detail
{

// Sorts a deck's cards into presentation order
//
// 1. Major arcana
//  - sorted by position (declared wins against implied)
//  - custom cards with no positio follows every card
// 2. Minor arcana
//   - wands, cups, swords, pentacles
//   - custom suits after by key
//   - within a suit, by ranks sequence
//
// Ties break by canonical ID
void sort_cards(
    std::vector<card>::iterator first, std::vector<card>::iterator last,
    std::vector<suit_info>::const_iterator suits_first,
    std::vector<suit_info>::const_iterator suits_last
);

}  // namespace arcana::detail
