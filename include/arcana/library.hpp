// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/deck.hpp>
#include <arcana/error.hpp>

#include <cstddef>
#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace arcana
{

// A deck summary created just from the manifest without loading images or aux files
struct deck_summary
{
    std::string directory_name;
    std::filesystem::path path;
    std::string id;

    std::string name;
    std::string version;
    std::optional<std::string> artist;

    std::optional<std::filesystem::path> icon;

    // The 78 standard cards minus [excluded_cards] plus [custom_cards]
    std::size_t card_count = 0;
};

// A directory containing a manifest that could not be read
struct malformed_deck
{
    std::string directory_name;
    std::filesystem::path path;
    error problem;
};

// A configured library root that could not be scanned
struct malformed_root
{
    std::filesystem::path path;
    error problem;
};

struct library_options
{
    // Searched in order, like PATH: when the same directory name appears under more
    // than one root the first wins, and the shadowed one is not reported.
    //
    // If empty, we use the standard XDG library
    std::vector<std::filesystem::path> roots;

    // The deck to fall back to when another deck does not have a card
    std::optional<std::filesystem::path> reference_deck;

    // Language preference chain
    std::vector<std::string> languages;
};

// Library of Tarot decks installed on the system
class deck_library
{
  public:
    explicit deck_library(library_options options = {});

    // Decks sorted by directory name
    //
    // Invalidated by refresh()
    [[nodiscard]] std::span<deck_summary const> decks() const
    {
        return decks_;
    }

    // Decks whose manifest could not be read, sorted by directory name
    //
    // Invalidated by refresh()
    [[nodiscard]] std::span<malformed_deck const> malformed_decks() const
    {
        return malformed_;
    }

    // The reference deck's summary
    //
    // @return std::nullopt when no reference deck is available
    [[nodiscard]] std::optional<deck_summary> const& reference() const
    {
        return reference_;
    }

    // Look up a deck
    //
    // @return std::nullopt for a deck that is absent or malformed
    [[nodiscard]] std::optional<deck_summary> find(std::string_view directory_name) const;

    // Every readable deck carrying this [deck].id
    //
    // @return Empty vector when no deck declares the id.
    [[nodiscard]] std::vector<deck_summary> find_all_by_id(std::string_view deck_id) const;

    // Fully load a deck from this library
    [[nodiscard]] std::expected<std::shared_ptr<deck const>, error> load(
        std::string_view directory_name
    ) const;

    // Fully load a deck external to this library, in this library's languages
    //
    // @param deck_directory A directory that can exist outside of the library
    [[nodiscard]] std::expected<std::shared_ptr<deck const>, error> load_external(
        std::filesystem::path const& deck_directory
    ) const;

    // Fully load the configured reference deck
    [[nodiscard]] std::expected<std::shared_ptr<deck const>, error> load_reference() const;

    // The library roots used to search for decks
    [[nodiscard]] std::span<std::filesystem::path const> roots() const
    {
        return roots_;
    }

    // Where the reference deck was configured to be, whether or not it is readable
    [[nodiscard]] std::optional<std::filesystem::path> const& reference_path() const
    {
        return reference_path_;
    }

    [[nodiscard]] std::span<std::string const> languages() const
    {
        return languages_;
    }

    // Re-scan the roots and the reference deck, invalidating cached spans
    void refresh();

  private:
    [[nodiscard]] std::expected<std::shared_ptr<deck const>, error> load_cached(
        std::filesystem::path const& deck_directory
    ) const;

    std::vector<std::filesystem::path> roots_;
    std::optional<std::filesystem::path> reference_path_;
    std::vector<std::string> languages_;

    std::vector<deck_summary> decks_;
    std::vector<malformed_deck> malformed_;
    std::optional<deck_summary> reference_;

    mutable std::unordered_map<std::string, std::shared_ptr<deck const>> loaded_;
};

}  // namespace arcana
