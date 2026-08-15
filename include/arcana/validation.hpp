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
    // An actionable suggestion that may not apply to this deck.
    pedantic,

    // An observation that certainly applies. Nothing is wrong.
    info,

    // The deck violates a SHOULD, or does something the packager probably did
    // not intend. An application must still load it.
    warning,

    // The deck violates a MUST. It is non-conforming.
    error,
};

// What a validation check needs before it can run.
enum class phase : std::uint8_t
{
    // The parsed deck.toml and names/*.toml suffice.
    document,

    // The check must stat or read the deck's own tree.
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

// What this build of the library actually does about a rule.
//
// The catalogue is written from the specification and is therefore always ahead
// of the checks: a code exists here long before anything judges a deck against
// it. A consumer that reports the catalogue must be able to say which is which,
// or it claims a silent deck is a clean one.
enum class rule_state : std::uint8_t
{
    // A check runs for this rule. Silence about it means the deck passed.
    checked,

    // Catalogued, no check written yet. Silence means nothing was looked at.
    pending,

    // Deliberately not implemented here, and not expected to be. Silence means
    // this library is the wrong thing to ask.
    deferred,
};

// The rules catalogue sorted ascending by code.
[[nodiscard]] std::span<rule const> rules() noexcept;

// The rule with this code, or nullptr where no rule carries it.
[[nodiscard]] rule const* find_rule(std::string_view code) noexcept;

// What this build does about the rule with this code, or nullopt where no rule
// carries it. Derived from the dispatch table, never declared beside the rule,
// so it cannot drift from what `validate` runs.
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
