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
#include <vector>

namespace arcana
{

namespace detail
{

// A scan of the library roots
struct library_snapshot;

// The memoized loads
struct deck_cache;

}  // namespace detail

// A deck summary created just from the manifest without loading images or aux files
struct deck_summary
{
    std::string directory_name;
    std::filesystem::path path;

    std::optional<std::string> identifier;

    std::string name;
    std::string version;
    std::optional<std::string> artist;

    std::optional<std::filesystem::path> icon;

    // The cards this deck has: the standard 78 minus exclusions, plus its own
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
    [[nodiscard]] std::span<deck_summary const> decks() const noexcept;

    // Decks whose manifest could not be read, sorted by directory name
    [[nodiscard]] std::span<malformed_deck const> malformed_decks() const noexcept;

    // The reference deck's summary
    //
    // @return std::nullopt when no reference deck is available
    [[nodiscard]] std::optional<deck_summary> const& reference() const noexcept;

    // Look up a deck
    //
    // @return std::nullopt for a deck that is absent or malformed
    [[nodiscard]] std::optional<deck_summary> find(std::string_view directory_name) const;

    // Every readable deck carrying this [deck].identifier
    //
    // @return Empty vector when no deck declares the identifier.
    [[nodiscard]] std::vector<deck_summary> find_all_by_identifier(
        std::string_view identifier
    ) const;

    // Fully load a deck from this library
    //
    // Loads are cached
    [[nodiscard]] std::expected<deck, error> load(std::string_view directory_name) const;

    // Fully load a deck external to this library, in this library's languages
    //
    // @param deck_directory A directory that can exist outside of the library
    [[nodiscard]] std::expected<deck, error> load_external(
        std::filesystem::path const& deck_directory
    ) const;

    // Fully load the configured reference deck
    [[nodiscard]] std::expected<deck, error> load_reference() const;

    // The library roots used to search for decks
    [[nodiscard]] std::span<std::filesystem::path const> roots() const noexcept;

    // Where the reference deck was configured to be, whether or not it is readable
    [[nodiscard]] std::optional<std::filesystem::path> const& reference_path() const noexcept;

    [[nodiscard]] std::span<std::string const> languages() const noexcept;

    // Re-scan the roots and the reference deck, and drop the cached loads
    void refresh();

  private:
    [[nodiscard]] std::expected<deck, error> load_cached(
        std::filesystem::path const& deck_directory
    ) const;

    std::shared_ptr<detail::library_snapshot const> state_;
    std::vector<std::shared_ptr<detail::library_snapshot const>> retired_;
    std::shared_ptr<detail::deck_cache> cache_;
};

}  // namespace arcana
