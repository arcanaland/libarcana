// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "backs.hpp"

#include "../../data/identifiers.hpp"
#include "../../data/image_signature.hpp"
#include "../assets.hpp"

#include <arcana/deck.hpp>

#include <toml++/toml.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <fstream>
#include <ios>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace arcana::validation
{

namespace
{

// The table that names card back designs. 1.0 spelled it `variants`; 2.0
// spells it `designs`, and the two rules that reach 1.0 decks read whichever
// this deck's schema version defines.
toml::table const* designs_table(check_context const& ctx)
{
    auto const major = schema_major(ctx.d.metadata).value_or(2);

    return ctx.doc["card_backs"][major < 2 ? "variants" : "designs"].as_table();
}

// The design keys deck.toml declares an `image` path for, with that path.
//
// A key declared without one creates no design: DECK.md 4.2 says declaring a
// design does not create it.
std::map<std::string, std::string> declared_images(check_context const& ctx)
{
    std::map<std::string, std::string> found;

    auto const* designs = designs_table(ctx);
    if (designs == nullptr)
        return found;

    for (auto const& [key, value] : *designs)
    {
        auto const* one = value.as_table();
        if (one == nullptr)
            continue;

        if (auto const* image = (*one)["image"].as_string())
            found.emplace(key.str(), image->get());
    }

    return found;
}

// One file sitting in a card back directory.
struct back_file
{
    deck_file const* file;
    std::string stem;
    std::optional<root_kind> kind;

    // True where discovery passes the file over, so that it defines no design.
    bool ignored;
};

std::vector<back_file> card_back_files(check_context const& ctx)
{
    auto const declared = declared_images(ctx);

    auto const is_declared_image = [&declared](deck_file const& file)
    {
        auto const shown = file.relative.generic_string();

        return std::ranges::any_of(
            declared, [&shown](auto const& one) { return one.second == shown; }
        );
    };

    std::vector<back_file> found;
    for (auto const& file : ctx.files)
    {
        auto const where = locate_asset(file.relative);
        if (!where || !where->card_back)
            continue;

        auto const name = file.relative.filename().string();
        auto const stem = stem_of(name);

        // DECK.md 5.5: the whole stem is the design key, card backs have no
        // variants, and the extension chain applies. A file deck.toml points
        // an `image` at is shown whatever it is named, so none of the three
        // makes it ignored.
        bool const ignored =
            !is_declared_image(file) && (stem.contains('.') || !data::is_custom_name(stem) ||
                                         !chain_admits(where->kind, extension_of(name)));

        found.push_back(
            {.file = &file, .stem = std::string{stem}, .kind = where->kind, .ignored = ignored}
        );
    }

    return found;
}

// The design keys this deck has: the stems discovery finds across every card
// back directory, plus every declared key carrying an `image` path.
std::vector<std::string> designs(check_context const& ctx)
{
    std::vector<std::string> found;

    for (auto const& back : card_back_files(ctx))
        if (!back.ignored)
            found.push_back(back.stem);

    for (auto const& [key, image] : declared_images(ctx)) found.push_back(key);

    std::ranges::sort(found);
    auto const dupes = std::ranges::unique(found);
    found.erase(dupes.begin(), dupes.end());

    return found;
}

// The file a deck-relative path names, or nothing where the deck has no such
// file. Reads `ctx.files` rather than the filesystem, so a path that escapes
// the deck root resolves to nothing here.
deck_file const* find_file(check_context const& ctx, std::string_view relative)
{
    auto const found = std::ranges::find_if(
        ctx.files,
        [relative](deck_file const& one) { return one.relative.generic_string() == relative; }
    );

    return found == ctx.files.end() ? nullptr : &*found;
}

// How many bytes the longest signature `sniff_image_format` knows needs.
constexpr std::size_t signature_bytes = 8;

bool is_baseline_format(deck_file const& file)
{
    std::ifstream stream{file.absolute, std::ios::binary};
    if (!stream)
        return false;

    std::array<std::byte, signature_bytes> head{};
    stream.read(reinterpret_cast<char*>(head.data()), head.size());  // NOLINT(*-reinterpret-cast)

    auto const got = static_cast<std::size_t>(stream.gcount());

    return data::sniff_image_format(std::span{head}.first(got)) != data::image_format::unknown;
}

}  // namespace

void check_bad_card_back_design_key(check_context const& ctx)
{
    auto const* designs = ctx.doc["card_backs"]["designs"].as_table();
    if (designs == nullptr)
        return;

    for (auto const& [key, value] : *designs)
    {
        auto const design = std::string_view{key.str()};
        if (data::is_custom_name(design))
            continue;

        ctx.report({
            .message = std::format(
                "card back design key '{}' is not a custom name: lowercase ASCII letters, digits "
                "and underscores, never starting with a digit",
                design
            ),
            .key = std::format("card_backs.designs.{}", design),
        });
    }
}

void check_missing_card_back_image(check_context const& ctx)
{
    auto const major = schema_major(ctx.d.metadata).value_or(2);
    auto const* const table = major < 2 ? "variants" : "designs";

    for (auto const& [design, image] : declared_images(ctx))
    {
        if (find_file(ctx, image) != nullptr)
            continue;

        ctx.report({
            .message = std::format(
                "card back design '{}' points at '{}', which the deck does not have", design, image
            ),
            .key = std::format("card_backs.{}.{}.image", table, design),
        });
    }
}

void check_unknown_default_card_back(check_context const& ctx)
{
    auto const* chosen = ctx.doc["card_backs"]["default"].as_string();
    if (chosen == nullptr)
        return;

    auto const& wanted = chosen->get();
    if (std::ranges::contains(designs(ctx), wanted))
        return;

    ctx.report({
        .message = std::format("default card back '{}' names no design the deck has", wanted),
        .key = "card_backs.default",
    });
}

void check_card_back_default_by_collation(check_context const& ctx)
{
    // A default that names nothing is unknown-default-card-back's to report;
    // this rule is about the deck that declares none at all.
    if (ctx.doc["card_backs"]["default"])
        return;

    auto const has = designs(ctx);
    if (has.size() < 2 || std::ranges::contains(has, std::string_view{"default"}))
        return;

    ctx.report({
        .message = std::format(
            "the deck has {} card back designs and declares no default, so '{}' wins on collation "
            "order alone",
            has.size(), has.front()
        ),
        .key = "card_backs.default",
    });
}

void check_ignored_card_back_file(check_context const& ctx)
{
    for (auto const& back : card_back_files(ctx))
    {
        if (!back.ignored)
            continue;

        ctx.report({
            .message = std::format(
                "'{}' is in a card back directory but defines no design, so it is never shown",
                back.file->relative.generic_string()
            ),
            .path = back.file->relative,
        });
    }
}

void check_card_back_not_baseline_format(check_context const& ctx)
{
    auto const backs = card_back_files(ctx);
    auto const declared = declared_images(ctx);

    for (auto const& design : designs(ctx))
    {
        std::vector<deck_file const*> files;
        bool ansi_only = true;

        for (auto const& back : backs)
        {
            if (back.ignored || back.stem != design)
                continue;

            files.push_back(back.file);
            ansi_only = ansi_only && back.kind == root_kind::ansi;
        }

        if (auto const named_by = declared.find(design); named_by != declared.end())
            if (auto const* named = find_file(ctx, named_by->second))
            {
                files.push_back(named);
                ansi_only = false;
            }

        // A design with no file at all is a resolution failure rather than a
        // format problem, and ANSI art is not an image the extension chain
        // governs (DECK.md 5.4).
        if (files.empty() || ansi_only)
            continue;

        if (std::ranges::any_of(
                files, [](deck_file const* one) { return is_baseline_format(*one); }
            ))
            continue;

        std::ranges::sort(files, {}, &deck_file::relative);
        ctx.report({
            .message = std::format(
                "card back design '{}' is supplied in neither PNG nor JPEG, so an application that "
                "cannot decode it shows a generic back",
                design
            ),
            .path = files.front()->relative,
        });
    }
}

}  // namespace arcana::validation
