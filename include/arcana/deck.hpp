// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/card.hpp>
#include <arcana/error.hpp>

#include <expected>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace arcana
{

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
    std::string image;
    std::optional<std::string> description;
    std::optional<std::string> alt_text;
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
    std::string image;
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

    std::map<int, std::string> major_arcana_remap;  // position -> canonical major arcana name

    excluded_cards excluded;

    std::vector<custom_card_def> custom_major_cards;
    std::vector<custom_suit_def> custom_suits;

    std::vector<deck_variant> variants;

    // The 78 standard cards minus exclusions, plus custom cards, names and alt text
    // resolved, image variants attached. Populated by load_deck; never partially filled.
    std::vector<card> cards;

    // `s`'s display name under this deck's `[aliases.suits]`, or its canonical name if
    // the deck defines no alias for it. The string_view overload takes a custom suit key
    // and falls back to that suit's `[custom_cards.minor_arcana.<key>].name`.
    [[nodiscard]] std::string display_suit_name(suit s) const;
    [[nodiscard]] std::string display_suit_name(std::string_view custom_suit_key) const;

    // `r`'s display name under this deck's `[aliases.courts]` (page/knight/queen/king
    // only; numeric ranks have no alias concept), or its canonical name otherwise.
    [[nodiscard]] std::string display_rank_name(rank r) const;

    // nullopt if `canonical_id` is not excluded by this deck.
    [[nodiscard]] std::optional<std::string> exclusion_reason(std::string_view canonical_id) const;

    // Points into `cards` and is invalidated by any change to it, so the returned pointer
    // must not outlive this deck. Language bindings must tie the two lifetimes together —
    // in nanobind that is nb::rv_policy::reference_internal, which is available precisely
    // because these are member functions. The string_view overload returns nullptr for an
    // id that does not parse, so a malformed id and an absent card are not distinguished;
    // use parse_card_id first if you need to tell them apart.
    [[nodiscard]] card const* find_card(card_id const& id) const;
    [[nodiscard]] card const* find_card(std::string_view canonical_id) const;
};

// Loads and fully parses a deck directory.
std::expected<deck, error> load_deck(
    std::filesystem::path const& deck_directory,
    std::optional<std::string> const& language = std::nullopt
);

}  // namespace arcana
