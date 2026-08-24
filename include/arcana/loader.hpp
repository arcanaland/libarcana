// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/deck.hpp>
#include <arcana/error.hpp>

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace arcana
{

// Load and fully parse a deck directory
//
// @param deck_directory The dir to load
// @param languages A preference chain of languages. If empty or cannot be
//                  satisfied, falls back to English
std::expected<deck, error> load_deck(
    std::filesystem::path const& deck_directory, std::vector<std::string> const& languages = {}
);

}  // namespace arcana
