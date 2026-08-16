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
// The major arcana come first, ordered by position: a two-digit key has an
// implicit position equal to its value, a declared position wins a tie against
// an implicit one, and a custom-keyed card with no position follows every card
// that has one. Then the minor arcana, with the four canonical suits in
// wands, cups, swords, pentacles order and every other suit after them sorted
// by key; within a suit by that suit's `ranks` sequence, with a rank the
// sequence does not name following every rank it does.
//
// Ties break by canonical ID, so the order does not depend on how the cards
// were discovered.
void sort_cards(std::vector<card>& cards, std::vector<suit_info> const& suits);

}  // namespace arcana::detail
