// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "backs.hpp"

#include "../../data/identifiers.hpp"
#include "../assets.hpp"
#include "../facts.hpp"
#include "../probe.hpp"
#include "../spec.hpp"

#include <arcana/deck.hpp>

#include <toml++/toml.hpp>

#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace arcana::validation
{

void check_bad_card_back_design_key(check_context const& ctx)
{
    auto const* designs = card_back_designs(ctx.doc, major_of(ctx.d));
    if (designs == nullptr)
        return;

    for (auto const& [key, value] : *designs)
    {
        auto const design = std::string_view{key.str()};
        if (data::is_custom_name(design))
            continue;

        ctx.report({
            .message = std::format(
                "card back design key '{}' does not conform to the custom name grammar: lowercase "
                "ASCII letters, digits "
                "and underscores (not starting with a digit)",
                design
            ),
            .key = std::format("card_backs.designs.{}", design),
        });
    }
}

void check_missing_card_back_image(check_context const& ctx)
{
    auto const table = card_back_designs_key(major_of(ctx.d));

    for (auto const& [design, image] : ctx.facts.back_images)
    {
        if (ctx.facts.file_at(image) != nullptr)
            continue;

        ctx.report({
            .message =
                std::format("card back design '{}' refers to missing image '{}'", design, image),
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
    if (std::ranges::contains(ctx.facts.designs, wanted))
        return;

    ctx.report({
        .message = std::format("default card back '{}' does not exist", wanted),
        .key = "card_backs.default",
    });
}

void check_card_back_default_by_collation(check_context const& ctx)
{
    if (ctx.doc["card_backs"]["default"])
        return;

    auto const& has = ctx.facts.designs;
    if (has.size() < 2 || std::ranges::contains(has, std::string_view{"default"}))
        return;

    ctx.report({
        .message = std::format(
            "the deck has {} card back designs but no default, so we chose '{}'", has.size(),
            has.front()
        ),
        .key = "card_backs.default",
    });
}

void check_ignored_card_back_file(check_context const& ctx)
{
    for (auto const& back : ctx.facts.back_files)
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
    auto const& declared = ctx.facts.back_images;

    for (auto const& design : ctx.facts.designs)
    {
        std::vector<deck_file const*> files;
        bool ansi_only = true;

        for (auto const& back : ctx.facts.back_files)
        {
            if (back.ignored || back.stem != design)
                continue;

            files.push_back(back.file);
            ansi_only = ansi_only && back.kind == root_kind::ansi;
        }

        if (auto const named_by = declared.find(design); named_by != declared.end())
            if (auto const* named = ctx.facts.file_at(named_by->second))
            {
                files.push_back(named);
                ansi_only = false;
            }

        // A design with no file at all is a resolution failure.
        if (files.empty() || ansi_only)
            continue;

        if (std::ranges::any_of(
                files, [](deck_file const* one) { return is_baseline_image_format(one->absolute); }
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
