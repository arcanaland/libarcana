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

// One scan of the roots, and the options that produced it
//
// Immutable once a deck_library points at it. refresh() builds the next one
// rather than editing this one, which is what lets a span outlive the call.
struct library_snapshot
{
    std::vector<std::filesystem::path> roots;
    std::optional<std::filesystem::path> reference_path;
    std::vector<std::string> languages;

    std::vector<deck_summary> decks;
    std::vector<malformed_deck> malformed;
    std::optional<deck_summary> reference;
};

// The memoized loads
//
// Shared between copies of a library, which is what anyone copying one expects.
// Not synchronized: nothing in the tree is threaded yet, and a shared mutable
// cache across threads is a question RFC-039 parked.
struct deck_cache
{
    std::unordered_map<std::string, deck> loaded;
};

}  // namespace arcana::detail
