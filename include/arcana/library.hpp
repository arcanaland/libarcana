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

struct deck_summary
{
    std::string directory_name;
    std::string id;
    std::string name;
    std::filesystem::path path;
};

// Every directory under the deck library that contains a deck.toml
std::vector<deck_summary> enumerate_decks(
    std::optional<std::filesystem::path> const& root_override = std::nullopt
);

// Loads a deck by its directory name within the deck library
std::expected<deck, error> load_deck_by_name(
    std::string const& directory_name,
    std::optional<std::filesystem::path> const& root_override = std::nullopt,
    std::optional<std::string> const& language = std::nullopt
);

}  // namespace arcana
