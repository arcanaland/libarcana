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
    std::string key;  // e.g. "stars"
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

    // Position -> the canonical major arcana name shown at that position, e.g. `8 ->
    // "justice"` for a deck that swaps Justice and Strength. Names are matched loosely:
    // lower_cased, spaces or underscores either way, with or without a leading "the".
    // Already applied to `card::number`; a consumer wanting the deck's ordering should
    // read that rather than re-deriving it here.
    std::map<int, std::string> major_arcana_remap;

    excluded_cards excluded;

    std::vector<custom_card_def> custom_major_cards;
    std::vector<custom_suit_def> custom_suits;

    std::vector<deck_variant> variants;

    // The 78 standard cards minus exclusions, plus custom cards
    std::vector<card> cards;

    // `s`'s display name under this deck's `[aliases.suits]`, or its title-cased canonical
    // name if the deck defines no alias for it. The string_view overload takes a custom
    // suit key and falls back to that suit's `[custom_cards.minor_arcana.<key>].name`,
    // then to the title-cased key.
    [[nodiscard]] std::string display_suit_name(suit s) const;
    [[nodiscard]] std::string display_suit_name(std::string_view custom_suit_key) const;

    // `r`'s display name under this deck's `[aliases.courts]`, or its title-cased
    // canonical name otherwise. The string_view overload takes a custom card's own id and
    // falls back to the title-cased id.
    //
    // The alias map is consulted for any rank key, not just the four courts: a custom
    // suit's cards are looked up here by their own ids, which need not be ranks at all.
    // Decks conventionally only alias courts, but nothing enforces that, and a custom id
    // that collides with a canonical rank picks up that rank's alias.
    //
    // Every card already carries its resolved display_suit and display_rank, so these two
    // families are for callers holding a suit or rank with no card in hand — validation,
    // and a CLI formatter printing a suit heading over an empty group.
    [[nodiscard]] std::string display_rank_name(rank r) const;
    [[nodiscard]] std::string display_rank_name(std::string_view custom_rank_key) const;

    // nullopt if `canonical_id` is not excluded by this deck.
    [[nodiscard]] std::optional<std::string> exclusion_reason(std::string_view canonical_id) const;

    // Every suit this deck has cards for: the four canonical suits in canonical order,
    // then custom suits in deck.toml order. Never alphabetical -- a display consumer that
    // has to re-sort this has been handed the wrong answer.
    [[nodiscard]] std::vector<suit_info> suits() const;

    [[nodiscard]] std::vector<card> cards_of_kind(arcana_kind kind) const;

    // `key` is a suit_info::key: a canonical suit name or a custom suit's key.
    [[nodiscard]] std::vector<card> cards_in_suit(std::string_view key) const;

    // Returned by value: nothing aliases deck-owned storage, so a binding needs no
    // lifetime coupling and a consumer can wrap the result in its own placement type
    // (reversed, position in a spread) without touching the deck's copy. The string_view
    // overload returns nullopt both for an id that does not parse and for one this deck
    // does not define; use parse_card_id first if you need to tell them apart.
    [[nodiscard]] std::optional<card> find_card(card_id const& id) const;
    [[nodiscard]] std::optional<card> find_card(std::string_view canonical_id) const;

    // Deterministic in `seed`: the library owns no RNG state and never reaches for
    // std::random_device, so a caller can reproduce a draw. nullopt for an empty deck.
    [[nodiscard]] std::optional<card> random_card(std::uint64_t seed) const;

    // The variant named by `[card_backs].default`, or the only variant when the deck
    // defines exactly one, or nullopt.
    [[nodiscard]] std::optional<card_back_variant> default_card_back_variant() const;

    // This deck's deck.toml as parsed, re-serialized.
    //
    // load_deck retains the parsed document, so keys and whole sections that no parser
    // here names -- a field from a future spec version, say -- survive a load instead of
    // being dropped on the floor. That is what keeps a future write layer from having to
    // parse deck.toml a second time, and it is why this is worth doing before anything is
    // built on the parser.
    //
    // Measured limits, toml++ 3.4.0: comments are not represented in the node tree at all,
    // and keys come back sorted rather than in source order. Unknown keys and unknown
    // sections do round-trip. Empty for a default-constructed deck.
    [[nodiscard]] std::string source_toml() const;

    // Binding policy, recorded for whenever nanobind bindings are built (RFC-007 §1e):
    // load_deck's failure becomes a Python exception carrying error_code as an attribute,
    // so a caller loading a directory of decks can log-and-skip with a plain `except`.
    // Lookups that can legitimately miss -- find_card, default_card_back_variant,
    // random_card -- return Optional and never raise.

  private:
    friend std::expected<deck, error> load_deck(
        std::filesystem::path const& deck_directory, std::optional<std::string> const& language
    );

    // toml++ stays out of this header
    std::shared_ptr<detail::deck_document const> document_;
};

// Loads and fully parses a deck directory.
std::expected<deck, error> load_deck(
    std::filesystem::path const& deck_directory,
    std::optional<std::string> const& language = std::nullopt
);

}  // namespace arcana
