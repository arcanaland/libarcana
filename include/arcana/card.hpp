// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
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

// Stable, snake_case, spec-form string (not display name).
std::string_view to_string(suit s) noexcept;
std::string_view to_string(rank r) noexcept;

std::optional<suit> suit_from_string(std::string_view text) noexcept;
std::optional<rank> rank_from_string(std::string_view text) noexcept;

// True for a non-empty `[a-z0-9_]+` string.
bool is_valid_identifier(std::string_view text) noexcept;

enum class card_class : std::uint8_t
{
    standard_major,
    custom_major,
    standard_minor,
    custom_minor,
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

    // True for the two major classes.
    [[nodiscard]] bool is_major() const noexcept;

    [[nodiscard]] arcana_kind kind() const noexcept;

    // True when this card is defined by [custom_cards]
    [[nodiscard]] bool is_custom() const noexcept;

    // e.g. "major_arcana.00" or "minor_arcana.stars.ace"
    [[nodiscard]] std::string to_canonical() const;

    friend bool operator==(card_id const&, card_id const&) = default;
};

// One resolved image on disk for a card
struct image_variant
{
    std::string variant_name;  // "scalable", "ansi32", "h750", "h1200" or "h2400"
    std::filesystem::path path;
    std::optional<int> height;  // set only for the h<N> raster variants
};

// Among the height-bearing variants, returns the one closest to a target height.
//
// Prefers the smallest available height that is greater than or equal to the target
//
// Returns nullopt if there are no height-bearing entries
std::optional<image_variant> best_variant_for_height(
    std::vector<image_variant> const& variants, int target_height
);

// The main card model.
struct card
{
    card_id id;
    std::string display_name;

    // Minor arcana only, display-ready: a deck alias if one exists, else the title-cased
    // canonical name. Empty for majors -- the id's class already says there is no suit or
    // rank, so an optional would only add a second way to ask the same question.
    std::string display_suit;

    // Minor arcana only
    std::string display_rank;

    // Major arcana only. The *display* position, which is not always id.number: a deck's
    // `[remap_major_arcana]` is already applied, so Justice reads 8 in a deck that swaps
    // it with Strength while keeping the canonical id `major_arcana.11`.
    //
    // nullopt for all minors, and for a custom major that declares no position -- unlike
    // display_suit, this one is genuinely tri-state, which is why it is an optional.
    std::optional<int> number;
    std::optional<std::string> alt_text;
    std::vector<image_variant> images;

    // Shorthand for id.to_canonical().
    [[nodiscard]] std::string canonical_id() const;

    // Among the height-bearing variants, returns the one closest to a target height.
    //
    // Prefers the smallest available height that is greater than or equal to the target
    //
    // Returns nullopt if there are no height-bearing entries
    [[nodiscard]] std::optional<image_variant> best_image_for_height(int target_height) const;
};

}  // namespace arcana
