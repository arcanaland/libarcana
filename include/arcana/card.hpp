// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/error.hpp>

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace arcana
{

inline constexpr int max_major_arcana_number = 21;

enum class arcana_kind : std::uint8_t
{
    major_arcana,
    minor_arcana,
};

// The four canonical suits
enum class suit : std::uint8_t
{
    wands,
    cups,
    swords,
    pentacles,
};

enum class rank : std::uint8_t
{
    ace,
    two,
    three,
    four,
    five,
    six,
    seven,
    eight,
    nine,
    ten,
    page,
    knight,
    queen,
    king,
};

// Stable, lower_case, spec-form string (not display name). Display names come from a
// deck's aliases and live in deck.hpp.
std::string_view to_string(suit s) noexcept;
std::string_view to_string(rank r) noexcept;

std::optional<suit> suit_from_string(std::string_view text) noexcept;
std::optional<rank> rank_from_string(std::string_view text) noexcept;

// True for a non-empty `[a-z0-9_]+` string. The deck spec never states a grammar for the
// identifiers used as custom card and custom suit keys, but every example it gives is
// snake_case; libarcana requires that form so that a canonical id can be parsed without a
// deck in hand. See parse_card_id.
bool is_valid_identifier(std::string_view text) noexcept;

// The identity of a card within a deck, covering all four shapes the deck spec allows:
// a standard major (`major_arcana.07`), a standard minor (`minor_arcana.wands.ace`), a
// custom major (`major_arcana.happy_squirrel`) and a card of a custom suit
// (`minor_arcana.stars.ace`). Use the named constructors rather than filling the
// optionals by hand; which ones are engaged is determined by kind and is_custom().
struct card_id
{
    arcana_kind kind = arcana_kind::major_arcana;

    std::optional<int> major_number;       // standard major only, 0..21
    std::optional<suit> standard_suit;     // standard minor only
    std::optional<rank> standard_rank;     // standard minor only
    std::optional<std::string> suit_key;   // custom minor only: the custom suit's key
    std::optional<std::string> custom_id;  // custom major and custom minor: the card's own id

    static card_id standard_major(int number);
    static card_id standard_minor(suit s, rank r);
    static card_id custom_major(std::string id);
    static card_id custom_minor(std::string suit_key, std::string id);

    // True when this card is defined by a deck's [custom_cards] rather than by the spec's
    // standard 78.
    [[nodiscard]] bool is_custom() const noexcept;

    // The canonical id string, e.g. "major_arcana.00" or "minor_arcana.stars.ace". Round
    // trips through parse_card_id.
    [[nodiscard]] std::string to_canonical() const;

    friend bool operator==(card_id const&, card_id const&) = default;
};

// Parses a canonical id with no deck in hand, so it cannot consult a deck's [custom_cards]
// to tell a custom id from a typo. It resolves that structurally:
//
//   - `major_arcana.<NN>`, exactly two digits, is always a standard major and must be
//     00..21 — `major_arcana.22` is an error, not a custom card named "22".
//   - `major_arcana.<id>` otherwise is a custom major if <id> is a valid identifier.
//   - `minor_arcana.<suit>.<rank>` with a canonical suit requires a canonical rank —
//     `minor_arcana.wands.jack` is an error, not a custom card.
//   - `minor_arcana.<key>.<id>` with a non-canonical suit is a custom suit's card if both
//     components are valid identifiers.
//
// A parse therefore proves the id is well-formed, never that the deck defines it. Use
// deck::find_card for that.
std::expected<card_id, error> parse_card_id(std::string_view canonical_id);

// One resolved image on disk for a card
struct image_variant
{
    std::string variant_name;  // "scalable", "ansi32", "h750", "h1200" or "h2400"
    std::filesystem::path path;
    std::optional<int> height;  // set only for the h<N> raster variants
};

// Among the raster (height-bearing) entries of `variants`, returns the one closest to
// `target_height`, preferring the smallest available height that is >= target_height;
// if none is that large, returns the largest available. Returns nullopt if `variants`
// has no raster entries. card::best_image_for_height is the usual way to call this.
std::optional<image_variant> best_variant_for_height(
    std::vector<image_variant> const& variants, int target_height
);

// The main card model
struct card
{
    card_id id;
    std::string display_name;
    std::optional<std::string> alt_text;
    std::vector<image_variant> images;

    // Shorthand for id.to_canonical().
    [[nodiscard]] std::string canonical_id() const;

    // best_variant_for_height over this card's images.
    [[nodiscard]] std::optional<image_variant> best_image_for_height(int target_height) const;
};

}  // namespace arcana
