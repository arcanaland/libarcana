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

// List decks found in a search path of library root directories.
//
// If no root is passed, the default XDG path is used
std::vector<deck_summary> enumerate_decks(std::vector<std::filesystem::path> const& roots = {});

// Loads a specific deck by its directory name
std::expected<deck, error> load_deck_by_name(
    std::string const& directory_name,
    std::vector<std::filesystem::path> const& roots = {},
    std::optional<std::string> const& language = std::nullopt
);

}  // namespace arcana
