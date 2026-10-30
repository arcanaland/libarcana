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

inline constexpr int max_canonical_major_arcana_number = 21;
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

// The major key of a card id.
//
// @return empty for minor arcanum ids
[[nodiscard]] std::string major_key(card_id const& id);

// The suit key of a card id.
//
// @return empty for major arcanum ids
[[nodiscard]] std::string suit_key(card_id const& id);

// The rank key of a card id.
//
// @return empty for major arcanum ids
[[nodiscard]] std::string rank_key(card_id const& id);

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

// Where a supplied name came from, an open registry (DECK.md#433-supplied-names)
enum class name_source : std::uint8_t
{
    unstated,  // the deck names no source
    booklet,
    packaging,
    publisher,
    packager,
    other,  // an x_ token, or any token this library does not recognize
};

// A name the deck gives a card whose face prints none (DECK.md#433-supplied-names)
struct supplied_name
{
    // Verbatim, never case-converted
    std::string text;

    // As written. Where absent the name is in the artwork's language, which is
    // a resolution and is not computed here
    std::optional<std::string> lang;

    name_source source = name_source::unstated;

    // The source token as written, empty only where the deck names no source
    std::string source_name;

    friend bool operator==(supplied_name const&, supplied_name const&) = default;
};

// What a [cards] entry declares about one artwork, as written (DECK.md#43-cards)
struct artwork_declaration
{
    // The deck's own strings
    std::optional<std::string> name;
    std::optional<std::string> alt_text;

    // The number printed on the face, e.g. "XXIII" (DECK.md#431-card-numbers)
    std::optional<std::string> number;

    // The face shows no number, whatever the key's shape implies
    bool unnumbered = false;

    // The face shows no name, which ends name resolution after the manifest
    bool unnamed = false;

    // Never declared alongside `name` in a valid deck
    std::optional<supplied_name> supplied;

    // Text printed on the artwork besides its name and number, in reading
    // order (DECK.md#432-inscriptions). A single string is one element
    std::vector<std::string> inscription;

    // Empty where the artwork inherits the deck's (DECK.md#71-language-tags)
    std::vector<std::string> artwork_language;

    std::optional<std::string> image;

    friend bool operator==(artwork_declaration const&, artwork_declaration const&) = default;
};

// What [cards."<canonical-id>"] declares: the artwork, and the card's slot
struct card_declaration : artwork_declaration
{
    // Major arcana only
    std::optional<int> position;

    std::optional<std::string> default_variant;

    friend bool operator==(card_declaration const&, card_declaration const&) = default;
};

enum class image_kind : std::uint8_t
{
    scalable,
    raster,
    ansi,
    surrogate,
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

    // nullopt if not a variant
    std::optional<std::string> variant_key;
};

// One variant artwork of a card
struct card_variant
{
    std::string key;
    std::string display_name;
    std::optional<std::string> alt_text;

    // Effective: the variant's declared terms, the card's for the rest
    std::vector<origin_term> origin;

    friend bool operator==(card_variant const&, card_variant const&) = default;
};

// The main card model.
struct card
{
    card_id id;

    // The resolved display name
    //
    // Empty where the artwork has no name: it is unnamed, or it is an extended
    // major arcanum nothing names (DECK.md#73-display-name-resolution). Present
    // such a card by its number
    std::string display_name;

    // The card's [cards] entry as written. Empty for a 1.x deck
    card_declaration declared;

    // Minor arcana only
    std::string display_suit;
    std::string display_rank;

    // The number printed on the card's face, e.g. "XXIII" or "VIII½"
    // Opaque and not localized. As declared: see `declared.unnumbered`
    std::optional<std::string> number;

    // Major arcana only: where the card sits in the deck's sequence
    // nullopt for minors and for majors the deck gives no position
    std::optional<int> position;

    // Resolved: a name file outranks the manifest's `declared.alt_text`
    std::optional<std::string> alt_text;

    // How this card's artwork came to exist, the deck's unless it says otherwise
    std::vector<origin_term> origin;

    // Which variant a bare reference to this card resolves to
    //
    // nullopt where the unsuffixed file is the default, which is the usual case
    std::optional<std::string> default_variant;

    // Every artwork of the card: the default one and each variant
    std::vector<card_image> images;

    // One entry per key in variant_keys(), sorted by key
    std::vector<card_variant> variants;

    // Shorthand for id.to_canonical()
    [[nodiscard]] std::string canonical_id() const;

    // The keys of the card's variants excluding the default artwork
    [[nodiscard]] std::vector<std::string> variant_keys() const;

    // The artwork a request for variant_key resolves to
    //
    // Where the card has no variant under that key the default variant's
    // artwork is returned instead
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
