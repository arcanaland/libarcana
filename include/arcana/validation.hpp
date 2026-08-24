// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/deck.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace arcana
{

// How much a validation finding matters.
enum class severity : std::uint8_t
{
    pedantic,
    info,
    warning,
    error,
};

// What a validation check needs before it can run.
enum class phase : std::uint8_t
{
    // The parsed deck.toml and names/*.toml suffice.
    document,

    // The check must read the deck's tree.
    filesystem,

    // The check needs sibling decks
    library,
};

// The range of declared `[deck].schema_version` majors a rule applies to.
struct schema_range
{
    std::uint8_t min;
    std::uint8_t max;

    [[nodiscard]] constexpr bool contains(std::uint8_t major) const noexcept
    {
        return major >= min && major <= max;
    }
};

// One citation: a section of one schema major's specification text.
//
// `anchor` is the GitHub slug of that section's heading, without the leading
// '#', e.g. "574-the-extension-chain". Never a bare section number -- the v1.0
// text has no numbered headings at all, and a fragment naming one resolves to
// nothing.
struct spec_section
{
    std::uint8_t schema_major;

    std::string_view anchor;
};

// The v2 text's rule table, which lists every rule rather than any one of them.
// `rule::in_rules_table` stands in for a citation of it.
inline constexpr spec_section rule_table_section{
    .schema_major = 2, .anchor = "94-validation-rules"
};

// The sections one rule cites, in the order the specification writes them.
//
// A fixed-capacity value and not a `std::span`, so that a rule's citations are
// written at the rule. A span would need an array with static storage to point
// at, which is a table of its own hoisted away from the rules it describes.
struct spec_citations
{
    // Four cited sections, plus the slot `rule::citations()` appends the rule
    // table into. No rule cites more than three sections today.
    static constexpr std::size_t capacity = 5;

    std::array<spec_section, capacity> entries;

    std::uint8_t count;

    constexpr spec_citations() noexcept : entries{}, count{0} {}

    // Written as a braced list at the rule: `.cites = {v2("41-deck")}`.
    template <std::same_as<spec_section>... Sections>
        requires(sizeof...(Sections) >= 1)
    constexpr spec_citations(Sections... sections) noexcept
        : entries{sections...}, count{static_cast<std::uint8_t>(sizeof...(sections))}
    {
        static_assert(
            sizeof...(sections) < capacity,
            "a rule cites at most four sections; the last slot is the rule table"
        );
    }

    [[nodiscard]] constexpr spec_section const* begin() const noexcept
    {
        return entries.data();
    }

    [[nodiscard]] constexpr spec_section const* end() const noexcept
    {
        return entries.data() + count;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return count;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return count == 0;
    }
};

// One entry of the diagnostic catalogue.
//
// Every field here is derived from the deck specification and reviewed as prose.
struct rule
{
    // Flat kebab-case, e.g. "orphan-image". This is API and is never renamed.
    std::string_view code;

    // The severity a consumer gets unless it re-levels the rule itself.
    severity default_level;

    // One of: deck, ids, images, backs, cards, names, ansi, surrogate.
    std::string_view area;

    // The strongest phase this check requires over the whole schema range it
    // applies to.
    phase needs;

    // Where the specification states this rule specifically, in the order the
    // sections are written. The rule table is not among these; see
    // `in_rules_table` below and `citations()`.
    spec_citations cites;

    // Whether the v2 text's rule table, section 9.4, lists this rule. That
    // section lists every rule, so citing it distinguishes nothing: 94 of the
    // catalogue's 112 entries carried the identical anchor by hand. It is a
    // flag here, and `citations()` appends it last.
    //
    // The v1.0 text's unnumbered "Validation Rules" section is written inline
    // as an ordinary citation instead. Six rules name it, and at that count it
    // still tells one rule from another.
    bool in_rules_table;

    // Static non-interpolated explanation of rule
    std::string_view explanation;

    // The declared schema_version majors this rule is judged against.
    schema_range applies_to;

    // A new check (excluded from the default set)
    bool experimental;

    // Every section this rule cites, with the rule table appended where
    // `in_rules_table` is set. This is what a consumer rendering a diagnostic
    // wants; `cites` is what the catalogue is written in terms of.
    [[nodiscard]] constexpr spec_citations citations() const noexcept
    {
        if (!in_rules_table)
            return cites;

        spec_citations all = cites;
        all.entries[all.count++] = rule_table_section;

        return all;
    }
};

// Whether a rule is actually implemented
enum class rule_state : std::uint8_t
{
    // A check runs for this rule.
    checked,

    // Catalogued, no check written yet.
    pending,

    // Deliberately not implemented
    deferred,
};

// The rules catalogue sorted ascending by code.
[[nodiscard]] std::span<rule const> rules() noexcept;

// The rule with this code, or nullptr where no rule carries it.
[[nodiscard]] rule const* find_rule(std::string_view code) noexcept;

// Whether this rule is implemented
[[nodiscard]] std::optional<rule_state> state_of(std::string_view code) noexcept;

// The commit of arcanaland/specifications this major's citations were read
// against, or empty for a major the catalogue carries no text for.
[[nodiscard]] std::string_view spec_revision(std::uint8_t schema_major) noexcept;

// A URL resolving to the cited section, built from that major's pin. Empty
// where the major has no pin.
[[nodiscard]] std::string spec_url(spec_section section);

// One finding about one deck.
struct diagnostic
{
    severity level;

    // The rule this finding came from.
    std::string_view code;

    // Short interpolated-message
    std::string message;

    // The canonical card ID this finding is about
    std::optional<std::string> card;

    // Deck-root-relative.
    std::optional<std::filesystem::path> path;

    // A dotted deck.toml or name-file key path, e.g.
    // "card_backs.designs.classic.image".
    std::optional<std::string> key;
};

// Judge a deck against the catalogue.
//
// Runs every check and returns everything it finds.
[[nodiscard]] std::vector<diagnostic> validate(deck const& d);

}  // namespace arcana
