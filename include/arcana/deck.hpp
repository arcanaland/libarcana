// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/card.hpp>
#include <arcana/error.hpp>

#include <cstdint>
#include <expected>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace arcana
{

namespace detail
{

// The parsed deck.toml
struct deck_document;

// Builds a deck from a parsed deck.toml
class deck_loader;

}  // namespace detail

inline constexpr double default_aspect_ratio = 0.5789;

// The [deck] section
struct deck_metadata
{
    std::string id;
    std::string schema_version;
    std::string name;
    std::string version;
    std::optional<std::string> icon;
    std::optional<std::string> author;
    std::optional<std::string> license;
    std::optional<std::string> attribution;
    double aspect_ratio = default_aspect_ratio;
    std::optional<std::string> description;
    std::optional<std::string> created_date;
    std::optional<std::string> updated_date;
    std::optional<std::string> publisher;
    std::optional<std::string> website;
    std::vector<std::string> tags;
};

// The major component of [deck].schema_version
[[nodiscard]] std::optional<std::uint8_t> schema_major(deck_metadata const& metadata) noexcept;

// One entry of [deck.companions].esoterica
struct esoterica_companion
{
    std::string id;
    std::string name;
    std::string uri;
};

struct card_back_variant
{
    std::string id;
    std::string name;

    // The deck-relative reference exactly as written in deck.toml
    std::string image_ref;

    // For a discovered variant this is the file that was found.
    std::filesystem::path image;

    std::optional<std::string> description;
    std::optional<std::string> alt_text;

    // If this back was declared in [card_backs.variants].
    bool declared = true;
};

struct excluded_cards
{
    std::vector<std::string> cards;  // canonical IDs
    std::optional<std::string> reason;
};

// One of
//   [custom_cards.major_arcana.<foo>] or
//   [custom_cards.minor_arcana.<foo>] or
struct custom_card_def
{
    std::string id;
    std::string name;

    // The deck-relative reference exactly as written in deck.toml
    std::string image_ref;

    // For a discovered variant this is the file that was found.
    std::filesystem::path image;

    std::optional<std::string> alt_text;
    std::optional<int> position;
};

// An entire new suit.
struct custom_suit_def
{
    std::string key;
    std::string name;
    std::vector<custom_card_def> cards;
};

struct deck_variant
{
    std::string key;  // the `[variants.<key>]` table key
    std::string id;
    std::string name;
    std::optional<std::string> card_back;
    std::optional<std::string> publisher;
    std::optional<std::string> created_date;
};

// One suit of a loaded deck, standard or custom
struct suit_info
{
    std::string key;  // "wands" for a canonical suit, or the custom suit's key
    std::string display_name;
    bool standard = true;

    // True when every card of this suit is excluded
    bool excluded = false;
};

// The full deck model
struct deck
{
    std::filesystem::path root_path;
    deck_metadata metadata;
    std::vector<esoterica_companion> companions;

    std::optional<std::string> default_card_back;  // `[card_backs].default`
    std::vector<card_back_variant> card_backs;

    std::unordered_map<std::string, std::string> suit_aliases;   // canonical suit -> display name
    std::unordered_map<std::string, std::string> court_aliases;  // canonical rank -> display name

    std::map<int, std::string> major_arcana_remap;

    excluded_cards excluded;

    std::vector<custom_card_def> custom_major_cards;
    std::vector<custom_suit_def> custom_suits;

    std::vector<deck_variant> variants;

    // The 78 standard cards minus exclusions, plus custom cards
    std::vector<card> cards;

    // A suit's title-cased canonical name (or alias if defined)
    [[nodiscard]] std::string display_suit_name(suit s) const;

    // A custom suit's title-cased display name
    [[nodiscard]] std::string display_suit_name(std::string_view custom_suit_key) const;

    // A rank's title-cased canonical name (or alias if defined)
    [[nodiscard]] std::string display_rank_name(rank r) const;

    // A custom rank's title-cased canonical name
    [[nodiscard]] std::string display_rank_name(std::string_view custom_rank_key) const;

    // Reason why a card is excluded
    // nullopt if not excluded
    [[nodiscard]] std::optional<std::string> exclusion_reason(std::string_view canonical_id) const;

    // Every suit this deck has cards for
    [[nodiscard]] std::vector<suit_info> suits() const;

    // Every major arcana or minor arcana card
    [[nodiscard]] std::vector<card> cards_of_kind(arcana_kind kind) const;

    // Every card in a given suit
    // key is a lowercase suit name or a custom suit's key.
    [[nodiscard]] std::vector<card> cards_in_suit(std::string_view key) const;

    // Find a card given an id
    //
    // nullopt when this deck does not define it
    [[nodiscard]] std::optional<card> find_card(card_id const& id) const;

    // nullopt for an empty deck
    [[nodiscard]] std::optional<card> random_card(std::uint64_t seed) const;

    [[nodiscard]] std::optional<card_back_variant> default_card_back_variant() const;

    // The deck.toml re-serialized
    [[nodiscard]] std::string source_toml() const;

  private:
    friend class detail::deck_loader;

    // So toml++ stays out of this header
    std::shared_ptr<detail::deck_document const> document_;
};

// Load and fully parse a deck directory
//
// @param deck_directory The dir to load
// @param languages A preference chain of languages. If empty or cannot be
//                  satisfied, falls back to English
std::expected<deck, error> load_deck(
    std::filesystem::path const& deck_directory, std::vector<std::string> const& languages = {}
);

}  // namespace arcana
