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

// The [cards] subtable an entry was written as
toml::table const* fields_of(check_context const& ctx, card_entry const& entry)
{
    auto const* cards = ctx.doc["cards"].as_table();
    if (cards == nullptr)
        return nullptr;

    auto const found = cards->find(entry.reference);

    return found == cards->end() ? nullptr : found->second.as_table();
}

// The string an entry declares for a field
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

std::optional<card_id> card_of(card_entry const& entry)
{
    auto parsed = card_id::parse(entry.card);
    if (!parsed)
        return std::nullopt;

    return *parsed;
}

// The card table's entry for a card
card_entry const* entry_for_card(check_context const& ctx, std::string_view canonical)
{
    for (auto const& entry : ctx.facts.card_entries)
        if (!entry.variant && entry.card == canonical)
            return &entry;

    return nullptr;
}

// True for slots that exist for every deck
bool is_always_present(card_id const& id)
{
    if (id.cls == card_class::standard_minor)
        return true;

    return id.cls == card_class::standard_major && id.number >= 0 &&
           id.number <= max_canonical_major_arcana_number;
}

// True where this artwork is a file the deck actually has
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

// The card's variant keys
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

struct declared_position
{
    std::string label;

    // The canonical ID
    std::optional<std::string> canonical;

    std::string key;
    int at;
};

// The positions a 2.x manifest declares.
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

// The positions a 1.0 manifest declares.
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

// The canonical ID a file under an image root
std::optional<std::string> card_of_file(std::filesystem::path const& relative)
{
    auto const where = locate_asset(relative);

    if (!where || where->card_back || where->kind == image_kind::surrogate)
        return std::nullopt;
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

        // An entry for an excluded card is a contradiction, another check should catch this
        if (std::ranges::contains(ctx.d.excluded().cards, entry.card))
            continue;

        if (entry.variant && entry.image)
            continue;

        auto const found = ctx.d.find_card(*id);

        // if this is a variant
        if (entry.variant)
        {
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
                "the cards table declares '{}' but no image exists for it", entry.reference
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
        auto const first =
            std::ranges::find(declared.begin(), one, one->at, &declared_position::at);

        // Only a declared position collides.
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

            // One report per list per repeated key
            if (std::ranges::contains(reported, rank))
                continue;

            reported.push_back(rank);

            ctx.report({
                .message = std::format(
                    "'{}' appears twice in the ranks list of suit '{}'", rank, list.suit
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
                "or by 'unnumbered'.",
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
            .message = std::format("'{}' is excluded and declared in the same deck.", canonical),
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
    // The two facets of the slot.
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

void check_undefined_suit_or_rank_table(check_context const& ctx)
{
    if (auto const* suits = ctx.doc["suits"].as_table())
        for (auto const& [key, value] : *suits)
        {
            if (!value.is_table())
                continue;

            auto const suit = std::string_view{key.str()};
            if (std::ranges::any_of(
                    ctx.d.cards_in_suit(suit), [&](card const& c) { return has_artwork(ctx, c); }
                ))
                continue;

            ctx.report({
                .message =
                    std::format("[suits.{}] describes a suit the deck has no artwork for.", suit),
                .key = std::format("suits.{}", suit),
            });
        }

    // TODO: refactor this loop since it's basically the same as the above one
    if (auto const* ranks = ctx.doc["ranks"].as_table())
        for (auto const& [key, value] : *ranks)
        {
            if (!value.is_table())
                continue;

            auto const rank = std::string_view{key.str()};
            if (std::ranges::any_of(
                    ctx.d.cards(), [&](card const& c)
                    { return !c.id.is_major() && rank_key(c.id) == rank && has_artwork(ctx, c); }
                ))
                continue;

            ctx.report({
                .message =
                    std::format("[ranks.{}] describes a rank the deck has no artwork for.", rank),
                .key = std::format("ranks.{}", rank),
            });
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
                "'{}' names '{}' as its default variant, which is not a variant it has.", canonical,
                *declared
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

        auto const* entry = entry_for_card(ctx, canonical);
        if (entry != nullptr && entry->declares("default_variant"))
            continue;

        ctx.report({
            .message = std::format("'{}' has variant artwork but there is no default.", canonical),
            .card = canonical,
            .key = std::format(R"(cards."{}".default_variant)", canonical),
        });
    }
}

}  // namespace arcana::validation
