// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/deck.hpp>
#include <arcana/library.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace arcana::detail
{

struct library_snapshot
{
    std::vector<std::filesystem::path> roots;
    std::optional<std::filesystem::path> reference_path;
    std::vector<std::string> languages;

    std::vector<deck_summary> decks;
    std::vector<malformed_deck> malformed;
    std::optional<deck_summary> reference;
};

struct deck_cache
{
    std::unordered_map<std::string, deck> loaded;
};

}  // namespace arcana::detail
