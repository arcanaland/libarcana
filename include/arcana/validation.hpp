// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/deck.hpp>

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

    // Sections of the spec joined by semicolons: "DECK.md#5.5; DECK.md#9.4"
    std::string_view spec_ref;

    // Static non-interpolated explanation of rule
    std::string_view explanation;

    // The declared schema_version majors this rule is judged against.
    schema_range applies_to;

    // A new check (excluded from the default set)
    bool experimental;
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
