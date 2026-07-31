// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/deck.hpp>
#include <arcana/error.hpp>

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace arcana
{

// A deck found in the library, identified without building any of its cards
struct deck_summary
{
    std::string directory_name;
    std::filesystem::path path;
    std::string id;
    std::string name;
};

// A directory holding a deck.toml that could not be read
//
// There is no id or name here on purpose: both live in the table that failed to parse.
// A directory with no deck.toml at all is not a broken deck -- it is not a deck, and
// the library does not report it either way.
struct broken_deck
{
    std::string directory_name;
    std::filesystem::path path;
    error problem;
};

struct library_options
{
    // Searched in order, like PATH: when the same directory name appears under more
    // than one root the first wins, and the shadowed one is not reported.
    //
    // Empty means the XDG deck library
    std::vector<std::filesystem::path> roots;

    // Applied to every deck this library loads
    std::optional<std::string> language;
};

// The decks installed on the system
class deck_library
{
  public:
    explicit deck_library(library_options options = {});

    // Readable decks sorted by directory name
    [[nodiscard]] std::vector<deck_summary> const& decks() const
    {
        return decks_;
    }

    // Decks whose deck.toml could not be read, sorted by directory name
    //
    // Every directory scanned lands in exactly one of these two lists. Shadowing is
    // resolved before a deck is read, so a broken deck under an earlier root hides a
    // readable one of the same name under a later root -- the same way a broken
    // executable earlier in PATH still wins.
    [[nodiscard]] std::vector<broken_deck> const& broken_decks() const
    {
        return broken_;
    }

    // Look up a readable deck
    //
    // nullopt both for a deck that is absent and for one that is broken; load() tells
    // those apart, through error_code::not_found versus error_code::parse_error
    [[nodiscard]] std::optional<deck_summary> find(std::string_view directory_name) const;

    // Look up a readable deck by its [deck].id rather than by the directory holding it
    //
    // Nothing enforces that ids are unique across a library; the first match wins
    [[nodiscard]] std::optional<deck_summary> find_by_id(std::string_view deck_id) const;

    // Fully load a deck in this library, in this library's language
    //
    // Resolves on the directory name alone, across broken decks too, so a deck whose
    // deck.toml does not parse reports that parse error instead of claiming not to be
    // installed -- the better diagnostic of the two.
    [[nodiscard]] std::expected<deck, error> load(std::string_view directory_name) const;

    // Fully load a deck from anywhere, in this library's language
    //
    // The directory need not be under any root: for a CLI pointed at a checkout, or a
    // reference deck shipped outside the library
    [[nodiscard]] std::expected<deck, error> load_path(
        std::filesystem::path const& deck_directory
    ) const;

    // The roots actually being searched, with the XDG default already resolved
    [[nodiscard]] std::vector<std::filesystem::path> const& roots() const
    {
        return roots_;
    }

    [[nodiscard]] std::optional<std::string> const& language() const
    {
        return language_;
    }

    // Re-scan the roots
    void refresh();

  private:
    std::vector<std::filesystem::path> roots_;
    std::optional<std::string> language_;

    std::vector<deck_summary> decks_;
    std::vector<broken_deck> broken_;
};

}  // namespace arcana
