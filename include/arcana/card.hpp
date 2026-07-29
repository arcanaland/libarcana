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

// Stable, lower_case, spec-form string (not display name).
std::string_view to_string(suit s) noexcept;
std::string_view to_string(rank r) noexcept;

std::optional<suit> suit_from_string(std::string_view text) noexcept;
std::optional<rank> rank_from_string(std::string_view text) noexcept;

// True for a non-empty `[a-z0-9_]+` string.
bool is_valid_identifier(std::string_view text) noexcept;

// The four shapes a card id can take. This is a discriminant, not a hint: it names
// exactly which of card_id's fields carry meaning, so a consumer switches once instead of
// probing optionals. The four legal states were previously spelled as five optionals with
// 32 representable combinations and the invariant in comments.
enum class card_class : std::uint8_t
{
    standard_major,  // read `number`
    custom_major,    // read `custom_id`
    standard_minor,  // read `standard_suit` and `standard_rank`
    custom_minor,    // read `suit_key` and `custom_id`
};

struct card_id
{
    card_class cls = card_class::standard_major;

    int number = -1;        // standard_major only, 0..21; -1 otherwise
    suit standard_suit{};   // standard_minor only
    rank standard_rank{};   // standard_minor only
    std::string suit_key;   // custom_minor only: the custom suit's key
    std::string custom_id;  // custom_major and custom_minor: the card's own id

    static card_id standard_major(int number);
    static card_id standard_minor(suit s, rank r);
    static card_id custom_major(std::string id);
    static card_id custom_minor(std::string suit_key, std::string id);

    // True for the two major classes. `kind()` is the same fact as an arcana_kind, for
    // callers that only need major-vs-minor.
    [[nodiscard]] bool is_major() const noexcept;
    [[nodiscard]] arcana_kind kind() const noexcept;

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

// The main card model.
//
// The display_* fields are resolved once, at load, through the deck's [aliases.suits] and
// [aliases.courts]. They exist so a display consumer reads a field instead of calling back
// into the deck per draw and branching on the card's class first; deck::display_suit_name
// and deck::display_rank_name remain for callers that have a suit but no card in hand.
struct card
{
    card_id id;
    std::string display_name;
    std::string display_suit;   // resolved; empty for majors, which have no suit
    std::string display_rank;   // resolved; empty for majors, which have no rank
    std::optional<int> number;  // majors only: the standard number, or a custom major's
                                // declared `position`. nullopt for minors and for a custom
                                // major that declares no position.
    std::optional<std::string> alt_text;
    std::vector<image_variant> images;

    // Shorthand for id.to_canonical().
    [[nodiscard]] std::string canonical_id() const;

    // best_variant_for_height over this card's images.
    [[nodiscard]] std::optional<image_variant> best_image_for_height(int target_height) const;
};

}  // namespace arcana
