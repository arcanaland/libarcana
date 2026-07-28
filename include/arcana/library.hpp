// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/deck.hpp>
#include <arcana/error.hpp>

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace arcana
{

// Cheap summary of an installed deck: its [deck].id and [deck].name, without scanning
// image directories or loading names/<lang>.toml the way load_deck does.
struct deck_summary
{
    std::string directory_name;
    std::string id;
    std::string name;
    std::filesystem::path path;
};

// Enumerates every directory under the deck library that contains a deck.toml, peeking
// only [deck].id and [deck].name from each. A deck.toml that fails to parse is skipped,
// not treated as a hard error — one broken deck must not hide the rest of the library.
std::vector<deck_summary> enumerate_decks(
    std::optional<std::filesystem::path> const& root_override = std::nullopt
);

// Loads a deck by its directory name within the deck library (not by [deck].id, which
// need not match the directory name).
std::expected<deck, error> load_deck_by_name(
    std::string const& directory_name,
    std::optional<std::filesystem::path> const& root_override = std::nullopt,
    std::optional<std::string> const& language = std::nullopt
);

}  // namespace arcana
