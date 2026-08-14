// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "images.hpp"

#include "../../data/text.hpp"
#include "../assets.hpp"
#include "../facts.hpp"
#include "../probe.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <functional>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace arcana::validation
{

namespace
{

// Reports one file per group that discovery cannot reach.
void report_misplaced(
    check_context const& ctx, root_kind wanted, chain_format format, std::string_view what
)
{
    for (auto const& file : ctx.files)
    {
        auto const name = file.relative.filename().string();
        if (chain_format_of(extension_of(name)) != format)
            continue;

        if (ctx.facts.reachable(file, wanted))
            continue;

        ctx.report({
            .message = std::format("'{}' {}", file.relative.generic_string(), what),
            .path = file.relative,
        });
    }
}

// The card a raster asset belongs to, or nothing if the file is not a raster
std::optional<std::string> card_of_raster(deck_file const& file)
{
    auto const where = locate_asset(file.relative);
    if (!where || where->card_back || where->kind != root_kind::raster)
        return std::nullopt;

    std::vector<std::string> parts;
    for (auto const& one : file.relative) parts.push_back(one.string());

    auto const base = before(stem_of(parts.back()), '.');
    if (base.empty())
        return std::nullopt;

    // locate_asset has already vouched for the shape, so the group is parts[1].
    auto const between = parts.size() == 4 ? std::format("{}.", parts[2]) : std::string{};

    return std::format("{}.{}{}", parts[1], between, base);
}

}  // namespace

void check_card_not_baseline_format(check_context const& ctx)
{
    std::map<std::string, std::vector<deck_file const*>, std::less<>> rasters_by_card;

    for (auto const& file : ctx.files)
        if (auto const card = card_of_raster(file))
            rasters_by_card[*card].push_back(&file);

    for (auto const& [card, files] : rasters_by_card)
    {
        if (std::ranges::any_of(
                files, [](deck_file const* one) { return is_baseline_image_format(one->absolute); }
            ))
            continue;

        auto const* first = std::ranges::min(files, {}, &deck_file::relative);

        ctx.report({
            .message = std::format(
                "no raster asset for '{}' is in a baseline format, so an application that decodes "
                "only PNG, JPEG and WebP cannot display it",
                card
            ),
            .card = card,
            .path = first->relative,
        });
    }
}

void check_raster_outside_image_root(check_context const& ctx)
{
    for (auto const format :
         {chain_format::png, chain_format::webp, chain_format::avif, chain_format::jpeg})
        report_misplaced(
            ctx, root_kind::raster, format, "is an image not in a h<height>/ directory"
        );
}

void check_svg_outside_scalable(check_context const& ctx)
{
    report_misplaced(
        ctx, root_kind::scalable, chain_format::svg, "is an SVG outside the scalable/ directory"
    );
}

void check_stem_case_collision(check_context const& ctx)
{
    for (auto const& [where, group] : ctx.facts.by_folded_stem)
    {
        auto const first = group.front()->relative.filename().string();
        auto const kept = stem_of(first);

        std::vector<std::string_view> reported;
        for (auto const* file : group | std::views::drop(1))
        {
            auto const name = file->relative.filename().string();
            auto const stem = stem_of(name);

            if (stem == kept || std::ranges::contains(reported, stem))
                continue;

            reported.push_back(stem);
            ctx.report({
                .message = std::format(
                    "'{}' and '{}' have stems differing only in case",
                    file->relative.generic_string(), group.front()->relative.generic_string()
                ),
                .path = file->relative,
            });
        }
    }
}

void check_duplicate_chain_extension(check_context const& ctx)
{
    auto const raster_format = [](deck_file const& file)
    {
        auto const where = locate_asset(file.relative);
        if (!where || (where->kind && *where->kind != root_kind::raster))
            return chain_format::none;

        auto const name = file.relative.filename().string();
        auto const format = chain_format_of(extension_of(name));

        return format == chain_format::svg || format == chain_format::toml ? chain_format::none
                                                                           : format;
    };

    for (auto const& [where, group] : ctx.facts.by_stem)
    {
        deck_file const* first = nullptr;
        auto kept = chain_format::none;

        for (auto const* file : group)
        {
            auto const format = raster_format(*file);
            if (format == chain_format::none)
                continue;

            if (first == nullptr)
            {
                first = file;
                kept = format;
                continue;
            }

            if (format == kept)
                continue;

            ctx.report({
                .message = std::format(
                    "'{}' and '{}' have the same name with different extensions, is that "
                    "intentional?",
                    file->relative.generic_string(), first->relative.generic_string()
                ),
                .path = file->relative,
            });

            break;
        }
    }
}

void check_ignored_image_root_lookalike(check_context const& ctx)
{
    std::vector<std::string> reported;

    for (auto const& file : ctx.files)
    {
        std::vector<std::string> parts;
        for (auto const& one : file.relative) parts.push_back(one.string());

        // A card subtree needs a directory above it and a file below it.
        if (parts.size() < 3)
            continue;

        if (parts[1] != "major_arcana" && parts[1] != "minor_arcana")
            continue;

        if (parse_image_root(parts[0]) || std::ranges::contains(reported, parts[0]))
            continue;

        reported.push_back(parts[0]);
        ctx.report({
            .message = std::format(
                "'{}' contains a {} subtree but is not an image root, so discovery ignores it",
                parts[0], parts[1]
            ),
            .path = std::filesystem::path{parts[0]},
        });
    }
}

}  // namespace arcana::validation
