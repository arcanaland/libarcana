// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "ids.hpp"

#include "../../data/identifiers.hpp"

#include <arcana/card.hpp>

#include <toml++/toml.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

// Where a name the author coined was written.
enum class name_site : std::uint8_t
{
    suit,
    rank,

    // A card back design key, an edition key, a card variant key or a custom
    // major arcanum
    other,
};

// One key the deck author coined
struct coined_name
{
    std::string name;
    name_site site;
    std::optional<std::string> key;
    std::optional<std::filesystem::path> path;
};

void add_declared(
    std::vector<coined_name>& found, std::string_view name, name_site site, std::string key
)
{
    found.push_back({.name = std::string{name}, .site = site, .key = std::move(key), .path = {}});
}

// `[suits.<key>]` keys and the rank keys in their `ranks` lists.
void collect_suit_names(toml::table const& doc, std::vector<coined_name>& found)
{
    auto const* suits = doc["suits"].as_table();
    if (suits == nullptr)
        return;

    for (auto const& [key, value] : *suits)
    {
        auto const suit_key = std::string_view{key.str()};
        add_declared(found, suit_key, name_site::suit, std::format("suits.{}", suit_key));

        auto const* t = value.as_table();
        if (t == nullptr)
            continue;

        auto const* ranks = (*t)["ranks"].as_array();
        if (ranks == nullptr)
            continue;

        for (auto const& element : *ranks)
            if (auto const* rank_key = element.as_string())
                add_declared(
                    found, rank_key->get(), name_site::rank, std::format("suits.{}.ranks", suit_key)
                );
    }
}

// Variant names, which `cc9081e` moved off the deleted `[card_variants]` table and
// onto `[cards]`: the suffix of a variant-reference key, and each `default_variant`.
void collect_variant_names(toml::table const& doc, std::vector<coined_name>& found)
{
    auto const* cards = doc["cards"].as_table();
    if (cards == nullptr)
        return;

    for (auto const& [card, value] : *cards)
    {
        auto const key = std::string_view{card.str()};

        if (auto const colon = key.find(':'); colon != std::string_view::npos)
            add_declared(
                found, key.substr(colon + 1), name_site::other, std::format(R"(cards."{}")", key)
            );

        auto const* t = value.as_table();
        if (t == nullptr)
            continue;

        if (auto const* fallback = (*t)["default_variant"].as_string())
            add_declared(
                found, fallback->get(), name_site::other,
                std::format(R"(cards."{}".default_variant)", key)
            );
    }
}

// Every custom name deck.toml declares
void collect_declared(toml::table const& doc, std::vector<coined_name>& found)
{
    collect_suit_names(doc, found);

    if (auto const* designs = doc["card_backs"]["designs"].as_table())
        for (auto const& [key, value] : *designs)
            add_declared(
                found, key.str(), name_site::other, std::format("card_backs.designs.{}", key.str())
            );

    collect_variant_names(doc, found);
}

// The stem up to the first .
std::string_view base_of(std::string_view filename)
{
    return filename.substr(0, filename.find('.'));
}

bool is_canonical_major_key(std::string_view base)
{
    return base.size() == 2 && base.find_first_not_of("0123456789") == std::string_view::npos;
}

bool already_collected(std::vector<coined_name> const& found, std::string_view name, name_site site)
{
    return std::ranges::any_of(
        found, [&](coined_name const& one) { return one.name == name && one.site == site; }
    );
}

// The names that can be inferred from a file's path
void collect_from_file(deck_file const& file, std::vector<coined_name>& found)
{
    std::vector<std::string> parts;
    for (auto const& component : file.relative) parts.push_back(component.string());

    if (parts.empty())
        return;

    auto const add = [&](std::string_view name, name_site site)
    {
        if (name.empty() || already_collected(found, name, site))
            return;

        found.push_back(
            {.name = std::string{name}, .site = site, .key = {}, .path = file.relative}
        );
    };

    // The last component is the file
    auto const last = parts.size() - 1;

    for (std::size_t at = 0; at < last; ++at)
    {
        // major_arcana/<base>.<ext>
        if (parts[at] == "major_arcana" && at + 1 == last)
        {
            auto const base = base_of(parts[last]);

            // A two-digit base is a canonical major arcanum
            if (!is_canonical_major_key(base))
                add(base, name_site::other);

            continue;
        }

        // minor_arcana/<suit>/<rank>.<ext>. The suit is a directory, so a
        // file sitting directly in minor_arcana/ names no suit.
        if (parts[at] != "minor_arcana" || at + 1 >= last)
            continue;

        add(parts[at + 1], name_site::suit);

        if (at + 2 == last)
            add(base_of(parts[last]), name_site::rank);
    }
}

// The suits, ranks and custom majors discovered from the file tree
void collect_discovered(std::span<deck_file const> files, std::vector<coined_name>& found)
{
    for (auto const& file : files) collect_from_file(file, found);
}

std::vector<coined_name> coined_names(check_context const& ctx)
{
    std::vector<coined_name> found;
    collect_declared(ctx.doc, found);
    collect_discovered(ctx.files, found);

    return found;
}

// True where a reserved canonical key is legitimate at this site rather than a
// name the author coined.
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
            "deck identifier has a fragment '{}', which is not allowed", parts->fragment
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

void check_bad_custom_name(check_context const& ctx)
{
    for (auto const& one : coined_names(ctx))
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
    for (auto const& one : coined_names(ctx))
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
