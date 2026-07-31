// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/library.hpp>

#include <filesystem>
#include <optional>

namespace arcana::detail
{

// The id and name of the deck in a directory, without building any of its cards
//
// enumerate_decks() renders a list of every deck on the system, so it cannot afford a
// full load per entry: deck_loader materializes 78 cards and stats every image root
// along the way. This reads the manifest, takes two fields, and drops it.
//
// nullopt when the directory holds no readable deck -- callers listing a library skip
// those silently rather than failing the whole enumeration over one bad deck.
[[nodiscard]] std::optional<deck_summary> read_deck_summary(
    std::filesystem::path const& deck_directory
);

}  // namespace arcana::detail
