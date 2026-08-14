// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "ids.hpp"

#include "../../data/identifiers.hpp"
#include "../facts.hpp"

#include <arcana/card.hpp>

#include <toml++/toml.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace arcana::validation
{

namespace
{

// A [deck] key's value, or nothing when it is absent or is not a string.
std::optional<std::string_view> deck_string(check_context const& ctx, std::string_view key)
{
    auto const* value = ctx.doc["deck"][key].as_string();
    if (value == nullptr)
        return std::nullopt;

    return std::string_view{value->get()};
}

// True where a reserved canonical key is legitimate at this site rather than a
// name the deck creator coined.
bool reserved_is_legal_here(std::string_view name, name_site site)
{
    switch (site)
    {
        case name_site::suit:
            // DECK.md section 4.4: a canonical suit keys `[suits]` where the
            // intent is to modify that suit.
            return suit_from_string(name).has_value();

        case name_site::rank:
            // A ranks list is normally the canonical sequence.
            return rank_from_string(name).has_value();

        case name_site::other:
            return false;
    }

    return false;
}


constexpr bool is_card_group_prefix(std::string_view const key) noexcept
{
    constexpr std::array<std::string_view, 2> card_group_prefixes{"major_arcana", "minor_arcana"};
    return std::ranges::contains(card_group_prefixes, key);
}

// Walk down the first subtable at each level, joining the keys with dots
//   [cards.major_arcana.00] -> major_arcana.00
std::string joined_key_path(std::string_view head, toml::node const& below)
{
    constexpr std::size_t deepest = 2;

    std::string path{head};

    auto const* t = below.as_table();
    for (std::size_t depth = 0; depth < deepest && t != nullptr && !t->empty(); ++depth)
    {
        auto const entry = t->begin();
        path += '.';
        path += entry->first.str();

        if (data::is_canonical_id(path))
            break;

        t = entry->second.as_table();
    }

    return path;
}

void report_name(check_context const& ctx, coined_name const& one, std::string message)
{
    ctx.report({
        .message = std::move(message),
        .path = one.path,
        .key = one.key,
    });
}

}  // namespace

void check_bad_deck_identifier(check_context const& ctx)
{
    auto const identifier = deck_string(ctx, "identifier");
    if (!identifier)
        return;

    auto const parts = data::parse_qualified_identifier(*identifier);
    if (!parts)
    {
        ctx.report({
            .message = std::format(
                "deck identifier '{}' is not a qualified identifier: a realm, a slash and one or "
                "more path segments",
                *identifier
            ),
            .key = "deck.identifier",
        });

        return;
    }

    if (parts->fragment.empty())
        return;

    // fragments can't be in qualified ids
    ctx.report({
        .message = std::format(
            "deck identifier has a fragment '{}' which is prohibited", parts->fragment
        ),
        .key = "deck.identifier",
    });
}

void check_missing_deck_identifier(check_context const& ctx)
{
    if (ctx.doc["deck"]["identifier"])
        return;

    ctx.report({
        .message = "deck declares no identifier so nothing can reference it",
        .key = "deck.identifier",
    });
}

void check_deck_identifier_path_shape(check_context const& ctx)
{
    auto const identifier = deck_string(ctx, "identifier");
    if (!identifier)
        return;

    auto const parts = data::parse_qualified_identifier(*identifier);
    if (!parts)
        // Ill-formed is bad-deck-identifier's to report
        return;

    // a deck's path SHOULD be deck/<name>
    constexpr std::string_view prefix = "deck/";
    if (parts->path.starts_with(prefix) &&
        parts->path.find('/', prefix.size()) == std::string_view::npos)
        return;

    ctx.report({
        .message = std::format("deck identifier path '{}' is not deck/<name>", parts->path),
        .key = "deck.identifier",
    });
}

void check_bad_signifies(check_context const& ctx)
{
    auto const signifies = deck_string(ctx, "signifies");
    if (!signifies)
        return;

    auto const parts = data::parse_qualified_identifier(*signifies);
    if (!parts)
    {
        ctx.report({
            .message = std::format(
                "signifies '{}' is not a qualified identifier, so it can never match the "
                "identifier of the deck it names",
                *signifies
            ),
            .key = "deck.signifies",
        });

        return;
    }

    if (parts->fragment.empty())
        return;

    ctx.report({
        .message = std::format(
            "signifies must refer to a deck via a qualified identifier, but you passed "
            "one with the fragment '{}'",
            parts->fragment
        ),
        .key = "deck.signifies",
    });
}

void check_signifies_self(check_context const& ctx)
{
    auto const signifies = deck_string(ctx, "signifies");
    auto const identifier = deck_string(ctx, "identifier");

    if (!signifies || !identifier || signifies->empty() || *signifies != *identifier)
        return;

    ctx.report({
        .message = std::format("signifies '{}' is this deck's own identifier", *signifies),
        .key = "deck.signifies",
    });
}

void check_bad_app_realm(check_context const& ctx)
{
    auto const* app = ctx.doc["app"].as_table();
    if (app == nullptr)
        return;

    for (auto const& [key, value] : *app)
    {
        auto const realm = std::string_view{key.str()};
        if (data::is_realm(realm))
            continue;

        ctx.report({
            .message = std::format(
                "[app] subtable key '{}' is not a realm, which must "
                "be written as a quoted TOML key",
                realm
            ),
            .key = std::format("app.{}", realm),
        });
    }
}

void check_bad_cards_table_key(check_context const& ctx)
{
    auto const* cards = ctx.doc["cards"].as_table();
    if (cards == nullptr)
        return;

    for (auto const& [key, value] : *cards)
    {
        auto const card = std::string_view{key.str()};
        if (data::is_canonical_id(card) || data::is_variant_reference(card))
            continue;

        // An unexpected key path will be reported by cards-key-path so skip
        if (is_card_group_prefix(card))
            continue;

        ctx.report({
            .message = std::format("[cards] key '{}' is not a card reference", card),
            .card = std::string{card},
            .key = std::format(R"(cards."{}")", card),
        });
    }
}

void check_non_canonical_card_reference(check_context const& ctx)
{
    if (auto const* excluded = ctx.doc["excluded_cards"]["cards"].as_array())
    {
        for (auto const& element : *excluded)
        {
            auto const* value = element.as_string();
            if (value == nullptr)
                continue;

            auto const card = std::string_view{value->get()};
            if (data::is_canonical_id(card))
                continue;

            ctx.report({
                .message = std::format("excluded card '{}' is not a canonical ID", card),
                .card = std::string{card},
                .key = "excluded_cards.cards",
            });
        }
    }
}

void check_cards_key_path(check_context const& ctx)
{
    auto const* cards = ctx.doc["cards"].as_table();
    if (cards == nullptr)
        return;

    for (auto const& [key, value] : *cards)
    {
        auto const head = std::string_view{key.str()};
        if (!is_card_group_prefix(head))
            continue;

        auto const wanted = joined_key_path(head, value);

        ctx.report({
            .message = std::format(
                "[cards] key path '{}' declares a table named '{}' rather than a card. Write it as "
                "the quoted card reference [cards.\"{}\"]",
                wanted, head, wanted
            ),
            .card = wanted,
            .key = std::format("cards.{}", head),
        });
    }
}

void check_bad_follows(check_context const& ctx)
{
    auto const follows = deck_string(ctx, "follows");
    if (!follows)
        return;

    auto const parts = data::parse_qualified_identifier(*follows);
    if (!parts)
    {
        ctx.report({
            .message = std::format(
                "follows '{}' is not a qualified identifier",
                *follows
            ),
            .key = "deck.follows",
        });

        return;
    }

    if (parts->fragment.empty())
        return;

    // variants not allowed
    ctx.report({
        .message = std::format(
            "follows should be a deck, but '{}' has a fragment '{}'", *follows, parts->fragment
        ),
        .key = "deck.follows",
    });
}

void check_follows_self(check_context const& ctx)
{
    auto const follows = deck_string(ctx, "follows");
    if (!follows || follows->empty())
        return;

    for (auto const* const other : {"identifier", "signifies"})
    {
        auto const against = deck_string(ctx, other);
        if (!against || *against != *follows)
            continue;

        ctx.report({
            .message = std::format(
                "follows '{}' is this deck's own {}, but a deck should not follow itself", *follows,
                other
            ),
            .key = "deck.follows",
        });

        return;
    }
}

void check_bad_custom_name(check_context const& ctx)
{
    for (auto const& one : ctx.facts.coined)
    {
        if (reserved_is_legal_here(one.name, one.site))
            continue;

        if (data::is_custom_name(one.name))
            continue;

        report_name(
            ctx, one,
            std::format(
                "'{}' is not a custom name: lowercase ASCII letters, digits and underscores, never "
                "starting with a digit",
                one.name
            )
        );
    }
}

void check_reserved_custom_name(check_context const& ctx)
{
    for (auto const& one : ctx.facts.coined)
    {
        if (!data::is_reserved_canonical_key(one.name))
            continue;

        if (reserved_is_legal_here(one.name, one.site))
            continue;

        report_name(
            ctx, one, std::format("'{}' is a reserved canonical key in this context", one.name)
        );
    }
}

}  // namespace arcana::validation
