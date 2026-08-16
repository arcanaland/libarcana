// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/card.hpp>
#include <arcana/error.hpp>

#include <cstdint>
#include <expected>
#include <filesystem>
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

struct deck_access;

}  // namespace detail

inline constexpr double default_aspect_ratio = 0.5789;

// The [deck] section
struct deck_metadata
{
    // The globally unique identifier of [deck].identifier
    //
    // Always nullopt for a 1.0 deck: v1's [deck].id is a bare library handle,
    // not a qualified identifier, and one is never synthesized for a deck that
    // lacks one
    std::optional<std::string> identifier;

    std::string schema_version;
    std::string name;
    std::string version;
    std::optional<std::string> icon;
    std::optional<std::string> creator;
    std::optional<std::string> artist;
    std::optional<std::string> license;
    std::optional<std::string> attribution;
    double aspect_ratio = default_aspect_ratio;
    std::optional<std::string> description;
    std::optional<std::string> publisher;
    std::optional<std::string> website;
    std::vector<std::string> tags;

    // How the deck's artwork came to exist. The default every card and card
    // back design inherits where it declares nothing of its own
    std::vector<origin_term> origin;

    // v1.0-source fields, populated only for a 1.0 deck. v2's published_date is
    // a different claim about a different event and is never derived from these
    std::optional<std::string> created_date;
    std::optional<std::string> updated_date;
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

// One of the back images a deck ships, named by a design key
struct card_back_design
{
    std::string id;
    std::string name;

    // The deck-relative reference exactly as written in deck.toml
    std::string image_ref;

    // For a discovered design this is the file that was found.
    std::filesystem::path image;

    std::optional<std::string> description;
    std::optional<std::string> alt_text;

    // How this design came to exist, the deck's unless it says otherwise
    std::vector<origin_term> origin;

    // If this back was declared rather than only discovered
    bool declared = true;
};

struct excluded_cards
{
    std::vector<std::string> cards;  // canonical IDs
    std::optional<std::string> reason;
};

// One suit of a loaded deck, canonical or custom
struct suit_info
{
    std::string key;  // "wands" for a canonical suit, or the custom suit's key
    std::string name;

    // The suit's rank keys, in the order the deck ranks them. The canonical
    // fourteen for a canonical suit the deck does not re-rank
    std::vector<std::string> ranks;

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
    std::vector<card_back_design> card_backs;

    excluded_cards excluded;

    // Every suit this deck has, canonical suits first
    std::vector<suit_info> suits;

    // The 78 standard cards minus exclusions, plus the deck's own cards
    std::vector<card> cards;

    // A canonical suit's display name, or its title-cased key
    [[nodiscard]] std::string display_suit_name(suit s) const;

    // A suit's display name by key, or its title-cased key
    [[nodiscard]] std::string display_suit_name(std::string_view suit_key) const;

    // A canonical rank's display name, or its title-cased key
    [[nodiscard]] std::string display_rank_name(rank r) const;

    // A rank's display name by key, or its title-cased key
    [[nodiscard]] std::string display_rank_name(std::string_view rank_key) const;

    // Reason why a card is excluded
    // nullopt if not excluded
    [[nodiscard]] std::optional<std::string> exclusion_reason(std::string_view canonical_id) const;

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

    [[nodiscard]] std::optional<card_back_design> default_card_back_design() const;

    // The deck.toml re-serialized
    [[nodiscard]] std::string source_toml() const;

  private:
    friend struct detail::deck_access;

    // Rank key -> the display name this deck resolved for it. Filled at load
    // from [aliases.courts] in 1.0 and from a name file's [name.rank] in 2.0;
    // v2 gives ranks no manifest field, so there is no public map
    std::unordered_map<std::string, std::string> rank_names_;

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
