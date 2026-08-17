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

// The highest number of the twenty-two canonical major arcana
inline constexpr int max_major_arcana_number = 21;

// The highest number a two-digit major arcana key grammar admists
inline constexpr int max_extended_major_arcana_number = 99;

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

// True for a non-empty [a-z0-9_]+ string.
bool is_valid_identifier(std::string_view text) noexcept;

// [a-z0-9_]+ with no leading digit
bool is_custom_name(std::string_view text) noexcept;

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

    int number = -1;        // standard_major only, 0..99; -1 otherwise
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

    // The inverse of to_canonical()
    //
    // Returns parse_error when the string is not a canonical card id
    static std::expected<card_id, error> parse(std::string_view canonical_id);

    friend bool operator==(card_id const&, card_id const&) = default;
};

// How one artwork came to exist, in a named vocabulary
//
// One entry per vocabulary system, e.g. system "iptc-dst", term "print".
// Sorted by system. Resolved at load: an artwork that declares no term for a
// system carries the deck's.
struct origin_term
{
    std::string system;
    std::string term;

    friend bool operator==(origin_term const&, origin_term const&) = default;
};

enum class image_kind : std::uint8_t
{
    scalable,
    raster,
    ansi,
};

// One resolved image on disk for a card
struct card_image
{
    // The directory (e.g., h1200, ansi32)
    std::string source_dir;

    std::filesystem::path path;

    image_kind kind = image_kind::scalable;

    std::optional<int> height;  // raster only
    std::optional<int> lines;   // ansi only

    // The artwork variant this file supplies, e.g. "two_women" for
    // 06.two_women.png. nullopt for the unsuffixed file
    std::optional<std::string> variant_key;
};

// The main card model.
struct card
{
    card_id id;
    std::string display_name;

    // Minor arcana only
    std::string display_suit;
    std::string display_rank;

    // The number printed on the card's face, e.g. "XXIII" or "VIII½"
    // Opaque and not localized
    std::optional<std::string> number;

    // Major arcana only: where the card sits in the deck's sequence
    // nullopt for minors and for majors the deck gives no position
    std::optional<int> position;

    std::optional<std::string> alt_text;

    // How this card's artwork came to exist, the deck's unless it says otherwise
    std::vector<origin_term> origin;

    // Which variant a bare reference to this card resolves to
    //
    // nullopt where the unsuffixed file is the default artwork, which is the
    // usual case. Set where the deck declares `default_variant`, and where the
    // card has variant files but no unsuffixed one
    std::optional<std::string> default_variant;

    // Every artwork of the card: the default one and each variant
    std::vector<card_image> images;

    // Shorthand for id.to_canonical()
    [[nodiscard]] std::string canonical_id() const;

    // The keys of the card's variants, sorted, excluding the default artwork
    [[nodiscard]] std::vector<std::string> variant_keys() const;

    // The artwork a request for `variant_key` resolves to
    //
    // Where the card has no variant under that key the default variant's
    // artwork is returned instead, which is not an error
    [[nodiscard]] std::vector<card_image> images_for_variant(std::string_view variant_key) const;

    // The raster image closest to target_height in pixels.
    // Prefers the smallest image at or above the target
    [[nodiscard]] std::optional<card_image> best_raster_for_height(int target_height) const;
    [[nodiscard]] std::optional<card_image> best_raster_for_height(
        int target_height, std::string_view variant_key
    ) const;

    // The ANSI image closest to target_lines in terminal lines.
    // Prefers the largest image at or below the target
    [[nodiscard]] std::optional<card_image> best_ansi_for_lines(int target_lines) const;
    [[nodiscard]] std::optional<card_image> best_ansi_for_lines(
        int target_lines, std::string_view variant_key
    ) const;

    // The scalable image, if it exists
    [[nodiscard]] std::optional<card_image> scalable_image() const;
    [[nodiscard]] std::optional<card_image> scalable_image(std::string_view variant_key) const;
};

}  // namespace arcana
