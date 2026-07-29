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
// The parsed deck.toml, kept alive behind an incomplete type so no toml++ name reaches a
// public header. See deck::source_toml.
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

    // The deck-relative reference exactly as written in deck.toml, e.g.
    // "card_backs/classic.png". Empty for a discovered variant. A validator needs the
    // author's unresolved text; a display consumer wants `image`.
    std::string image_ref;

    // `image_ref` resolved against the deck root, so it can go straight to a QPixmap.
    // For a discovered variant this is the file that was found.
    std::filesystem::path image;

    std::optional<std::string> description;
    std::optional<std::string> alt_text;

    // False when this variant was discovered by scanning card_backs/ rather than declared
    // in [card_backs.variants]. Deck spec v1.0 puts card backs in a flat card_backs/
    // directory, not under the h<N>/ variant roots, so there is no per-height variant list
    // here and no height selection.
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

    // As written in deck.toml, and that reference resolved against the deck root. Same
    // split, and the same reason, as card_back_variant.
    std::string image_ref;
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

// One suit of a loaded deck, standard or custom, keyed uniformly so a consumer iterating
// suits never branches on which kind it has.
struct suit_info
{
    std::string key;  // "wands" for a canonical suit, or the custom suit's key
    std::string display_name;
    bool standard = true;

    // True when no card of this suit survives into deck::cards, whether it was excluded
    // wholesale or card by card.
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
    // only; numeric ranks have no alias concept), or its canonical name otherwise. The
    // string_view overload takes a custom card's own id and falls back to that id.
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

    // shared_ptr to an incomplete type: deck stays copyable with no out-of-line special
    // members, and toml++ stays out of this header.
    std::shared_ptr<detail::deck_document const> document_;
};

// Loads and fully parses a deck directory.
std::expected<deck, error> load_deck(
    std::filesystem::path const& deck_directory,
    std::optional<std::string> const& language = std::nullopt
);

}  // namespace arcana
