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

// A [deck] key's value, or nothing when it is absent or is not a string. A key
// of the wrong type is wrong-value-type's to report and not this area's, so it
// reads here exactly as an absent one does.
//
// The returned view points into `ctx.doc`, which outlives every check.
std::optional<std::string_view> deck_string(check_context const& ctx, std::string_view key)
{
    auto const* value = ctx.doc["deck"][key].as_string();
    if (value == nullptr)
        return std::nullopt;

    return std::string_view{value->get()};
}

// Where a name the author coined was written. Which reserved keys are legal
// depends on it: DECK.md section 4.4 lets a canonical suit key a `[suits]`
// table, and a canonical rank is what a `ranks` list is normally made of.
enum class name_site : std::uint8_t
{
    suit,
    rank,

    // A card back design key, an edition key, a card variant key or a custom
    // major arcanum, none of which may be a reserved canonical key at all.
    other,
};

// One key the deck author coined, with whichever locator names where it came
// from: `key` for one declared in deck.toml, `path` for one discovered in the
// tree.
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

// The variant keys under each `[card_variants."<id>"]`, and the one its
// `default` names.
void collect_variant_names(toml::table const& doc, std::vector<coined_name>& found)
{
    auto const* variants_of = doc["card_variants"].as_table();
    if (variants_of == nullptr)
        return;

    for (auto const& [card, value] : *variants_of)
    {
        auto const* t = value.as_table();
        if (t == nullptr)
            continue;

        if (auto const* fallback = (*t)["default"].as_string())
            add_declared(
                found, fallback->get(), name_site::other,
                std::format(R"(card_variants."{}".default)", card.str())
            );

        auto const* variants = (*t)["variants"].as_table();
        if (variants == nullptr)
            continue;

        for (auto const& [variant_key, unused] : *variants)
            add_declared(
                found, variant_key.str(), name_site::other,
                std::format(R"(card_variants."{}".variants.{})", card.str(), variant_key.str())
            );
    }
}

// Every custom name deck.toml declares: the sites DECK.md section 3.2 lists,
// less the `[cards]` keys, whose shape bad-cards-table-key already judges.
void collect_declared(toml::table const& doc, std::vector<coined_name>& found)
{
    collect_suit_names(doc, found);

    if (auto const* designs = doc["card_backs"]["designs"].as_table())
        for (auto const& [key, value] : *designs)
            add_declared(
                found, key.str(), name_site::other, std::format("card_backs.designs.{}", key.str())
            );

    if (auto const* editions = doc["editions"].as_table())
        for (auto const& [key, value] : *editions)
            // `[editions].default` is a string naming an edition, not an
            // edition. Only the subtables are keyed by a custom name.
            if (value.is_table())
                add_declared(
                    found, key.str(), name_site::other, std::format("editions.{}", key.str())
                );

    collect_variant_names(doc, found);
}

// The stem up to the first `.`, which DECK.md section 5.1 calls the base. What
// follows it is a variant key and belongs to the card the base names.
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

// The names one file's path contributes. A name declared in deck.toml, or
// carried by an earlier file, is already in `found` and is not added twice.
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

    // The last component is the file; everything before it is directories.
    auto const last = parts.size() - 1;

    for (std::size_t at = 0; at < last; ++at)
    {
        // major_arcana/<base>.<ext>
        if (parts[at] == "major_arcana" && at + 1 == last)
        {
            auto const base = base_of(parts[last]);

            // A two-digit base is a canonical major arcanum, not a name the
            // author coined.
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

// The suit, rank and custom major keys DECK.md section 5.1 discovers from the
// tree rather than from deck.toml. This is why the two rules are
// phase::filesystem.
//
// Card back stems are deliberately absent: section 5.5 says an ill-formed one
// defines no design and is a file discovery ignores, reported as a warning by
// ignored-card-back-file rather than as an error here.
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
            // A `ranks` list is normally the canonical sequence.
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
    if (!identifier || data::is_qualified_identifier(*identifier))
        return;

    ctx.report({
        .message = std::format(
            "deck identifier '{}' is not a qualified identifier: a realm, a slash, one or more "
            "path segments, and an optional fragment after a hash",
            *identifier
        ),
        .key = "deck.identifier",
    });
}

void check_missing_deck_identifier(check_context const& ctx)
{
    // Present but of the wrong type is not missing; that is
    // wrong-value-type's finding to make.
    if (ctx.doc["deck"]["identifier"])
        return;

    ctx.report({
        .message = "deck declares no identifier, so nothing can reference it",
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
        // Ill-formed is bad-deck-identifier's to report, and its path cannot
        // be judged.
        return;

    // DECK.md section 3.3: a deck's path SHOULD be `deck/<name>`, so exactly
    // two segments with `deck` first.
    constexpr std::string_view prefix = "deck/";
    if (parts->path.starts_with(prefix) &&
        parts->path.find('/', prefix.size()) == std::string_view::npos)
        return;

    ctx.report({
        .message = std::format(
            "deck identifier path '{}' is not deck/<name>, which is what tells a deck's identifier "
            "from a spread's",
            parts->path
        ),
        .key = "deck.identifier",
    });
}

void check_bad_signifies(check_context const& ctx)
{
    auto const signifies = deck_string(ctx, "signifies");
    if (!signifies || data::is_qualified_identifier(*signifies))
        return;

    ctx.report({
        .message = std::format(
            "signifies '{}' is not a qualified identifier, so it can never match the identifier of "
            "the deck it names",
            *signifies
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
                "[app] subtable key '{}' is not a realm; a realm has two labels or more and must "
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
        if (data::is_canonical_id(card))
            continue;

        auto message =
            data::is_variant_reference(card)
                ? std::format(
                      "[cards] key '{}' is a variant reference; variants take their strings from "
                      "[card_variants]",
                      card
                  )
                : std::format(
                      "[cards] key '{}' is not a canonical ID; a major arcanum's key carries both "
                      "of its digits",
                      card
                  );

        ctx.report({
            .message = std::move(message),
            .card = std::string{card},
            .key = std::format(R"(cards."{}")", card),
        });
    }
}

void check_non_canonical_card_reference(check_context const& ctx)
{
    // Each site takes the tightness the section defining it states. Section
    // 4.7 keys `[card_variants]` by a canonical ID and section 4.5 lists
    // canonical IDs, so a variant suffix is wrong in both; section 3.3 makes
    // the fragment of a deck's qualified identifier a card reference, where
    // one is allowed.
    if (auto const* variants_of = ctx.doc["card_variants"].as_table())
    {
        for (auto const& [key, value] : *variants_of)
        {
            auto const card = std::string_view{key.str()};
            if (data::is_canonical_id(card))
                continue;

            ctx.report({
                .message = std::format("[card_variants] key '{}' is not a canonical ID", card),
                .card = std::string{card},
                .key = std::format(R"(card_variants."{}")", card),
            });
        }
    }

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

    for (auto const* const field : {"identifier", "signifies"})
    {
        auto const identifier = deck_string(ctx, field);
        if (!identifier)
            continue;

        auto const parts = data::parse_qualified_identifier(*identifier);
        if (!parts || parts->fragment.empty())
            continue;

        if (data::is_card_reference(parts->fragment))
            continue;

        ctx.report({
            .message =
                std::format("fragment '{}' of {} is not a card reference", parts->fragment, field),
            .card = std::string{parts->fragment},
            .key = std::format("deck.{}", field),
        });
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
            ctx, one, std::format("'{}' is a reserved canonical key and cannot be coined", one.name)
        );
    }
}

}  // namespace arcana::validation
