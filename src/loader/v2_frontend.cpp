// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "v2_frontend.hpp"

#include "loader.hpp"

#include <utility>

namespace arcana::detail
{

deck build_v2_deck(
    std::filesystem::path deck_root, std::shared_ptr<deck_document const> document,
    std::vector<std::string> const& languages
)
{
    // TASK-032 layer 4: discovery creates the cards, [cards] and [suits]
    // annotate, [deck.origin] resolves. Until then, the 1.0 reader
    return deck_loader{std::move(deck_root), std::move(document), languages}.build();
}

}  // namespace arcana::detail
