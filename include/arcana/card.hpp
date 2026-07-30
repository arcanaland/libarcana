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

    // True for the standard + custom majors
    [[nodiscard]] bool is_major() const noexcept;

    // Major vs minor
    [[nodiscard]] arcana_kind kind() const noexcept;

    // True when this card is defined by [custom_cards]
    [[nodiscard]] bool is_custom() const noexcept;

    // e.g. "major_arcana.00" or "minor_arcana.stars.ace"
    [[nodiscard]] std::string to_canonical() const;

    friend bool operator==(card_id const&, card_id const&) = default;
};

enum class image_kind : std::uint8_t
{
    scalable,
    raster,
    ansi,
};

// One resolved image on disk for a card
struct image_variant
{
    // The directory (e.g., h1200, ansi32)
    std::string variant_name;

    std::filesystem::path path;

    image_kind kind = image_kind::scalable;

    std::optional<int> height;  // raster only
    std::optional<int> lines;   // ansi only
};

// The main card model.
struct card
{
    card_id id;
    std::string display_name;

    // Minor arcana only
    std::string display_suit;
    std::string display_rank;

    // Major arcana only
    //
    // The display position, might differ from id.number due to remappings
    //
    // nullopt for minors and position-less custom majors
    std::optional<int> number;
    std::optional<std::string> alt_text;
    std::vector<image_variant> images;

    // Shorthand for id.to_canonical()
    [[nodiscard]] std::string canonical_id() const;

    // The raster variant closest to target_height in pixels.
    //
    // Prefers the smallest variant at or above the target
    [[nodiscard]] std::optional<image_variant> best_raster_for_height(int target_height) const;

    // The ANSI variant closest to target_lines in terminal lines.
    //
    // Prefers the largest variant at or below the target
    [[nodiscard]] std::optional<image_variant> best_ansi_for_lines(int target_lines) const;

    // The scalable image variant, if it exists
    [[nodiscard]] std::optional<image_variant> scalable_image() const;
};

}  // namespace arcana
