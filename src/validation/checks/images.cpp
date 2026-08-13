// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "images.hpp"

#include "../../data/ascii.hpp"
#include "../assets.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace arcana::validation
{

namespace
{

// A directory and a stem
using stem_key = std::pair<std::string, std::string>;

std::string lowered(std::string_view s)
{
    std::string folded{s};
    for (auto& c : folded) c = data::to_lower(c);

    return folded;
}

// The files of a deck grouped by directory and stem
std::map<stem_key, std::vector<deck_file const*>> group_by_stem(
    check_context const& ctx, bool fold_case
)
{
    std::map<stem_key, std::vector<deck_file const*>> groups;

    for (auto const& file : ctx.files)
    {
        auto const name = file.relative.filename().string();
        auto const stem = stem_of(name);

        groups[stem_key{
                   file.relative.parent_path().generic_string(),
                   fold_case ? lowered(stem) : std::string{stem},
               }]
            .push_back(&file);
    }

    return groups;
}

// file is in the manifest or discovery would pick it up
bool is_reachable(deck_file const& file, std::vector<std::string> const& declared, root_kind wanted)
{
    auto const where = locate_asset(file.relative);

    // The top-level card back directory has no kind and takes what it is given.
    if (where && (!where->kind || *where->kind == wanted))
        return true;

    return std::ranges::contains(declared, file.relative.generic_string());
}

// Reports one file per group that discovery cannot reach.
void report_misplaced(
    check_context const& ctx, root_kind wanted, chain_format format, std::string_view what
)
{
    auto const declared = declared_paths(ctx.doc);

    for (auto const& file : ctx.files)
    {
        auto const name = file.relative.filename().string();
        if (chain_format_of(extension_of(name)) != format)
            continue;

        if (is_reachable(file, declared, wanted))
            continue;

        ctx.report({
            .message = std::format("'{}' {}", file.relative.generic_string(), what),
            .path = file.relative,
        });
    }
}

}  // namespace

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
    for (auto const& [where, group] : group_by_stem(ctx, true))
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

    for (auto const& [where, group] : group_by_stem(ctx, false))
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
                    "'{}' and '{}' have the same name with different extensions, is that intentional?",
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
