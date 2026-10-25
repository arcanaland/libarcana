// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "cards.hpp"

#include <assets.hpp>
#include <discovery.hpp>
#include <facts.hpp>
#include <spec.hpp>
#include <text.hpp>

#include <arcana/card.hpp>
#include <arcana/deck.hpp>

#include <toml++/toml.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace arcana::validation
{

namespace
{

// The `[cards]` subtable an entry was written as, or null where the entry is
// not a table. The index carries the keys; the values are read from here.
toml::table const* fields_of(check_context const& ctx, card_entry const& entry)
{
    auto const* cards = ctx.doc["cards"].as_table();
    if (cards == nullptr)
        return nullptr;

    auto const found = cards->find(entry.reference);

    return found == cards->end() ? nullptr : found->second.as_table();
}

// The string an entry declared at `field`, or null where it declared none, or
// declared something that is not a string.
std::string const* declared_string(
    check_context const& ctx, card_entry const& entry, std::string_view field
)
{
    auto const* fields = fields_of(ctx, entry);
    if (fields == nullptr)
        return nullptr;

    auto const* value = (*fields)[field].as_string();

    return value == nullptr ? nullptr : &value->get();
}

// The card an entry references, or nothing where the key is not a card
// reference at all. A malformed key is `bad-cards-table-key`'s to report.
std::optional<card_id> card_of(card_entry const& entry)
{
    auto parsed = card_id::parse(entry.card);
    if (!parsed)
        return std::nullopt;

    return *parsed;
}

// The card table's entry for a card, or null where the deck wrote none. Only a
// bare reference declares the card; a variant reference declares an artwork.
card_entry const* entry_for_card(check_context const& ctx, std::string_view canonical)
{
    for (auto const& entry : ctx.facts.card_entries)
        if (!entry.variant && entry.card == canonical)
            return &entry;

    return nullptr;
}

// True for the slots that exist for every deck, which an entry may annotate
// whether or not the deck ships artwork for them.
bool is_always_present(card_id const& id)
{
    if (id.cls == card_class::standard_minor)
        return true;

    return id.cls == card_class::standard_major && id.number >= 0 &&
           id.number <= max_canonical_major_arcana_number;
}

// True where this artwork is a file the deck actually has. A declared `image`
// path is attached to the model whether or not it leads anywhere, so a card
// annotated with a broken path is a card with no artwork.
bool resolves(check_context const& ctx, card_image const& image)
{
    auto const relative = image.path.lexically_relative(ctx.d.root_path()).generic_string();

    return !relative.empty() && ctx.facts.file_at(relative) != nullptr;
}

// True where the deck's artwork for this card resolves to at least one file.
bool has_artwork(check_context const& ctx, card const& c)
{
    return std::ranges::any_of(c.images, [&](card_image const& one) { return resolves(ctx, one); });
}

// True where a reference carrying no variant suffix finds a file.
bool has_unsuffixed_artwork(check_context const& ctx, card const& c)
{
    return std::ranges::any_of(
        c.images, [&](card_image const& one) { return !one.variant_key && resolves(ctx, one); }
    );
}

// The card's variant keys that a file backs, sorted and without repeats.
std::vector<std::string> variant_keys_with_artwork(check_context const& ctx, card const& c)
{
    std::vector<std::string> keys;

    for (auto const& image : c.images)
        if (image.variant_key && resolves(ctx, image) &&
            !std::ranges::contains(keys, *image.variant_key))
            keys.push_back(*image.variant_key);

    std::ranges::sort(keys);

    return keys;
}

// One place a deck declared where a major arcanum sits.
struct declared_position
{
    // How the diagnostic names the card.
    std::string label;

    // The canonical ID, where the deck's key gives one. A 1.0 remap names its
    // card by display name, which is not an ID and is not made into one.
    std::optional<std::string> canonical;

    std::string key;
    int at;
};

// The positions a 2.x manifest declares. Only a bare reference to a major
// arcanum declares one: on a minor and on a variant reference a position is
// ignored, and its own code says so.
void positions_of_v2(check_context const& ctx, std::vector<declared_position>& found)
{
    for (auto const& entry : ctx.facts.card_entries)
    {
        if (entry.variant)
            continue;

        auto const id = card_of(entry);
        if (!id || !id->is_major())
            continue;

        auto const* fields = fields_of(ctx, entry);
        if (fields == nullptr)
            continue;

        auto const position = (*fields)["position"].value<std::int64_t>();
        if (!position)
            continue;

        found.push_back(
            {.label = entry.card,
             .canonical = entry.card,
             .key = std::format(R"(cards."{}".position)", entry.reference),
             .at = static_cast<int>(*position)}
        );
    }
}

// The positions a 1.0 manifest declares. `[remap_major_arcana]` is keyed by the
// position and holds the card, and a custom major declares its own.
void positions_of_v1(check_context const& ctx, std::vector<declared_position>& found)
{
    if (auto const* remapped = ctx.doc["remap_major_arcana"].as_table())
        for (auto const& [key, value] : *remapped)
        {
            auto const* card = value.as_string();
            if (card == nullptr)
                continue;

            auto const text = std::string_view{key.str()};

            int position = 0;
            auto const [end, ec] =
                std::from_chars(text.data(), text.data() + text.size(), position);

            if (ec != std::errc{} || end != text.data() + text.size())
                continue;

            found.push_back(
                {.label = card->get(),
                 .canonical = std::nullopt,
                 .key = std::format("remap_major_arcana.{}", text),
                 .at = position}
            );
        }

    auto const* custom = ctx.doc["custom_cards"]["major_arcana"].as_table();
    if (custom == nullptr)
        return;

    for (auto const& [key, value] : *custom)
    {
        auto const* one = value.as_table();
        if (one == nullptr)
            continue;

        auto const position = (*one)["position"].value<std::int64_t>();
        if (!position)
            continue;

        found.push_back(
            {.label = std::string{key.str()},
             .canonical = std::format("major_arcana.{}", key.str()),
             .key = std::format("custom_cards.major_arcana.{}.position", key.str()),
             .at = static_cast<int>(*position)}
        );
    }
}

std::vector<declared_position> declared_positions(check_context const& ctx)
{
    std::vector<declared_position> found;

    if (major_of(ctx.d) < current_schema_major)
        positions_of_v1(ctx, found);
    else
        positions_of_v2(ctx, found);

    return found;
}

// One suit's `ranks` list as the deck declared it. The model fills a canonical
// suit's list in with the canonical fourteen, so only the document says which
// suits actually stated an order.
struct declared_ranks
{
    std::string suit;
    std::vector<std::string> ranks;
};

std::vector<declared_ranks> declared_rank_lists(check_context const& ctx)
{
    std::vector<declared_ranks> found;

    auto const* suits = ctx.doc["suits"].as_table();
    if (suits == nullptr)
        return found;

    for (auto const& [key, value] : *suits)
    {
        auto const* one = value.as_table();
        if (one == nullptr)
            continue;

        auto const* ranks = (*one)["ranks"].as_array();
        if (ranks == nullptr)
            continue;

        declared_ranks list{.suit = std::string{key.str()}};
        for (auto const& element : *ranks)
            if (auto const* rank = element.as_string())
                list.ranks.push_back(rank->get());

        found.push_back(std::move(list));
    }

    return found;
}

// The canonical ID a file under an image root creates, or nothing where the
// file names no card.
std::optional<std::string> card_of_file(std::filesystem::path const& relative)
{
    auto const where = locate_asset(relative);

    // A card back names no card, and neither does a surrogate, which stands in
    // for an image the package does not ship.
    if (!where || where->card_back || where->kind == image_kind::surrogate)
        return std::nullopt;

    // Path shape is not enough: discovery passes over an extension outside the
    // chain, so such a file is not artwork the deck resolves.
    auto const name = relative.filename().string();
    if (!chain_admits(where->kind, extension_of(name)))
        return std::nullopt;

    std::vector<std::string> parts;
    for (auto const& part : relative) parts.push_back(part.string());

    auto const base = before(parts.back(), '.');

    if (parts.size() == 3 && parts[1] == "major_arcana")
        if (auto const id = detail::major_asset_id(base))
            return id->to_canonical();

    if (parts.size() == 4 && parts[1] == "minor_arcana")
        if (auto const id = detail::minor_asset_id(parts[2], base))
            return id->to_canonical();

    return std::nullopt;
}

}  // namespace

void check_declared_card_without_image(check_context const& ctx)
{
    for (auto const& entry : ctx.facts.card_entries)
    {
        auto const id = card_of(entry);
        if (!id)
            continue;

        // An entry for an excluded card is a contradiction rather than a
        // dangling annotation, and its own code says so.
        if (std::ranges::contains(ctx.d.excluded().cards, entry.card))
            continue;

        // On a variant reference a declared `image` names the artwork whether
        // or not the path leads anywhere, and the dangling path is
        // `missing-variant-image`'s to report. On a bare reference nothing else
        // reports it, so a path leading nowhere leaves the card with no artwork
        // and the declaration annotating nothing.
        if (entry.variant && entry.image)
            continue;

        auto const found = ctx.d.find_card(*id);

        if (entry.variant)
        {
            // A variant reference is never exempt: the variant is one the deck
            // has a file for or declares an `image` path to.
            if (found &&
                std::ranges::contains(variant_keys_with_artwork(ctx, *found), *entry.variant))
                continue;

            ctx.report({
                .message = std::format(
                    "variant '{}' names artwork the deck does not have, so the entry annotates "
                    "nothing",
                    entry.reference
                ),
                .card = entry.card,
                .key = std::format(R"(cards."{}")", entry.reference),
            });

            continue;
        }

        if (is_always_present(*id) || (found && has_artwork(ctx, *found)))
            continue;

        ctx.report({
            .message = std::format(
                "the cards table declares '{}', which the deck ships no files for, so the "
                "declaration annotates nothing",
                entry.reference
            ),
            .card = entry.card,
            .key = std::format(R"(cards."{}")", entry.reference),
        });
    }
}

void check_duplicate_card_position(check_context const& ctx)
{
    auto const declared = declared_positions(ctx);

    for (auto one = declared.begin(); one != declared.end(); ++one)
    {
        // Only a declared position collides. A declared position that merely
        // coincides with an implicit one is how most cards get theirs, and
        // reporting those would fire on ordinary decks.
        auto const first =
            std::ranges::find(declared.begin(), one, one->at, &declared_position::at);

        if (first == one)
            continue;

        ctx.report({
            .message = std::format(
                "'{}' and '{}' both declare position {}. Ordering stays well defined, since ties "
                "break by key, but the order is not the one either declaration asked for",
                first->label, one->label, one->at
            ),
            .card = one->canonical,
            .key = one->key,
        });
    }
}

void check_duplicate_rank_in_ranks(check_context const& ctx)
{
    for (auto const& list : declared_rank_lists(ctx))
    {
        std::vector<std::string> seen;
        std::vector<std::string> reported;

        for (auto const& rank : list.ranks)
        {
            if (!std::ranges::contains(seen, rank))
            {
                seen.push_back(rank);
                continue;
            }

            // One report per list per repeated key, however often it repeats.
            if (std::ranges::contains(reported, rank))
                continue;

            reported.push_back(rank);

            ctx.report({
                .message = std::format(
                    "'{}' appears twice in the ranks list of suit '{}', which makes the order "
                    "ambiguous. The list states ordering only, so the repeat adds no card",
                    rank, list.suit
                ),
                .key = std::format("suits.{}.ranks", list.suit),
            });
        }
    }
}

void check_empty_card_number(check_context const& ctx)
{
    for (auto const& entry : ctx.facts.card_entries)
    {
        auto const* number = declared_string(ctx, entry, "number");
        if (number == nullptr || !number->empty())
            continue;

        ctx.report({
            .message = std::format(
                "'{}' declares an empty number. A card is made unnumbered by the shape of its key "
                "or by 'unnumbered', never by an empty value",
                entry.reference
            ),
            .card = entry.card,
            .key = std::format(R"(cards."{}".number)", entry.reference),
        });
    }
}

void check_excluded_card_also_declared(check_context const& ctx)
{
    for (auto const& canonical : ctx.d.excluded().cards)
    {
        // A variant reference declares the card it names as much as a bare one
        // does, so the first entry naming the card is the one reported.
        auto const declared =
            std::ranges::find(ctx.facts.card_entries, canonical, &card_entry::card);
        if (declared == ctx.facts.card_entries.end())
            continue;

        ctx.report({
            .message = std::format(
                "'{}' is excluded and declared in the same deck. There is no reading that "
                "satisfies both",
                canonical
            ),
            .card = canonical,
            .key = std::format(R"(cards."{}")", declared->reference),
        });
    }
}

void check_excluded_card_has_image(check_context const& ctx)
{
    for (auto const& canonical : ctx.d.excluded().cards)
    {
        // The first file in tree order stands for the card; a card shipped at
        // several sizes is one changed mind, not several.
        std::filesystem::path const* shipped = nullptr;

        for (auto const& file : ctx.files)
            if (card_of_file(file.relative) == canonical)
            {
                shipped = &file.relative;
                break;
            }

        if (shipped == nullptr)
            continue;

        ctx.report({
            .message = std::format(
                "'{}' is listed as excluded and ships artwork anyway, so discovery resolves it. "
                "Remove it from the list, or remove its files",
                canonical
            ),
            .card = canonical,
            .path = *shipped,
        });
    }
}

void check_ignored_key_on_variant(check_context const& ctx)
{
    // The two facets of the slot. Every other field of a `[cards]` entry is a
    // facet of the artwork and is legal on a variant reference, a number among
    // them.
    constexpr std::array<std::string_view, 2> of_the_slot{"default_variant", "position"};

    for (auto const& entry : ctx.facts.card_entries)
    {
        if (!entry.variant)
            continue;

        for (auto const& field : of_the_slot)
        {
            if (!entry.declares(field))
                continue;

            ctx.report({
                .message = std::format(
                    "variant '{}' declares '{}', which an application ignores. It is a facet of "
                    "the card's slot, not of this artwork",
                    entry.reference, field
                ),
                .card = entry.card,
                .key = std::format(R"(cards."{}".{})", entry.reference, field),
            });
        }
    }
}

void check_missing_variant_image(check_context const& ctx)
{
    for (auto const& entry : ctx.facts.card_entries)
    {
        if (!entry.variant || !entry.image)
            continue;

        if (ctx.facts.file_at(*entry.image) != nullptr)
            continue;

        ctx.report({
            .message = std::format(
                "variant '{}' declares the image '{}', which is not a file the deck has, so the "
                "variant is unresolvable",
                entry.reference, *entry.image
            ),
            .card = entry.card,
            .key = std::format(R"(cards."{}".image)", entry.reference),
        });
    }
}

void check_position_on_minor_arcanum(check_context const& ctx)
{
    for (auto const& entry : ctx.facts.card_entries)
    {
        // A position on a variant reference is ignored whatever the card is,
        // and `ignored-key-on-variant` is the code that says so.
        if (entry.variant || !entry.declares("position"))
            continue;

        auto const id = card_of(entry);
        if (!id || id->is_major())
            continue;

        ctx.report({
            .message = std::format(
                "minor arcanum '{}' declares a position, which an application ignores. A minor "
                "arcanum takes its place from its suit's ranks sequence",
                entry.reference
            ),
            .card = entry.card,
            .key = std::format(R"(cards."{}".position)", entry.reference),
        });
    }
}

void check_rank_without_image(check_context const& ctx)
{
    for (auto const& list : declared_rank_lists(ctx))
    {
        auto const cards = ctx.d.cards_in_suit(list.suit);

        // A suit the deck ships no artwork for at all is a resolution failure
        // rather than a list naming a card the deck does not have.
        if (std::ranges::none_of(cards, [&](card const& c) { return has_artwork(ctx, c); }))
            continue;

        std::vector<std::string> reported;

        for (auto const& rank : list.ranks)
        {
            if (std::ranges::contains(reported, rank))
                continue;

            auto const found = std::ranges::find_if(
                cards, [&](card const& c) { return rank_key(c.id) == rank && has_artwork(ctx, c); }
            );

            if (found != cards.end())
                continue;

            reported.push_back(rank);

            ctx.report({
                .message = std::format(
                    "the ranks list of suit '{}' names '{}', which the deck has no artwork for, so "
                    "the list specifies a card the deck does not have",
                    list.suit, rank
                ),
                .card = std::format("minor_arcana.{}.{}", list.suit, rank),
                .key = std::format("suits.{}.ranks", list.suit),
            });
        }
    }
}

void check_unknown_variant_default(check_context const& ctx)
{
    for (auto const& c : ctx.d.cards())
    {
        // A card with no files at all is a resolution failure, not a violation
        // of the default variant rule.
        if (!has_artwork(ctx, c))
            continue;

        auto const canonical = c.canonical_id();

        auto const* entry = entry_for_card(ctx, canonical);
        if (entry == nullptr || !entry->declares("default_variant"))
            continue;

        auto const* declared = declared_string(ctx, *entry, "default_variant");
        if (declared == nullptr ||
            std::ranges::contains(variant_keys_with_artwork(ctx, c), *declared))
            continue;

        ctx.report({
            .message = std::format(
                "'{}' names '{}' as its default variant, which is not a variant it has, so a "
                "reference carrying no variant suffix is unresolvable",
                canonical, *declared
            ),
            .card = canonical,
            .key = std::format(R"(cards."{}".default_variant)", canonical),
        });
    }
}

void check_variant_card_without_default(check_context const& ctx)
{
    for (auto const& c : ctx.d.cards())
    {
        // Variant files and no unsuffixed file. A card with no files at all
        // falls out here, which is where the rule leaves it.
        if (variant_keys_with_artwork(ctx, c).empty() || has_unsuffixed_artwork(ctx, c))
            continue;

        auto const canonical = c.canonical_id();

        // The model supplies a default where the deck declares none, so the
        // entry is what says whether the deck actually wrote one.
        auto const* entry = entry_for_card(ctx, canonical);
        if (entry != nullptr && entry->declares("default_variant"))
            continue;

        ctx.report({
            .message = std::format(
                "'{}' has variant artwork and no unsuffixed artwork, and declares no default "
                "variant, so a reference carrying no variant suffix names nothing",
                canonical
            ),
            .card = canonical,
            .key = std::format(R"(cards."{}".default_variant)", canonical),
        });
    }
}

}  // namespace arcana::validation
