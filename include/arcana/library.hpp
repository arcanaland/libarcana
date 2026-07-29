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

// Every directory containing a deck.toml, across a search path of deck library
// root directories.
//
// If no root is passed, the
//
// `roots` is searched in order and an empty `roots` means
// deck_library_path()'s XDG answer; when a directory name appears under more than one
// root, the first wins and the shadowed deck is listed once.
//
std::vector<deck_summary> enumerate_decks(std::vector<std::filesystem::path> const& roots = {});

// Loads a deck by its directory name, searching `roots` in the same order and with the
// same shadowing rule as enumerate_decks. `directory_name` is the key -- not the deck's
// display name, which is not unique across decks.
std::expected<deck, error> load_deck_by_name(
    std::string const& directory_name, std::vector<std::filesystem::path> const& roots = {},
    std::optional<std::string> const& language = std::nullopt
);

}  // namespace arcana
