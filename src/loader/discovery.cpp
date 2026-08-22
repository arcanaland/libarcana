// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "discovery.hpp"

#include "../data/asset_grammar.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <string>
#include <system_error>
#include <utility>

namespace arcana::detail
{

namespace
{

namespace fs = std::filesystem;

}  // namespace

std::optional<image_root> classify_image_root(fs::path const& path)
{
    auto const name = path.filename().string();

    auto const parsed = data::parse_image_root(name);
    if (!parsed)
        return std::nullopt;

    // The shared grammar carries one size; the model splits it by kind
    return image_root{
        .path = path,
        .name = name,
        .kind = parsed->kind,
        .height = parsed->kind == image_kind::raster ? parsed->size : std::nullopt,
        .lines = parsed->kind == image_kind::ansi ? parsed->size : std::nullopt,
    };
}

card_image image_at(image_root const& root, fs::path path)
{
    return {
        .source_dir = root.name,
        .path = std::move(path),
        .kind = root.kind,
        .height = root.height,
        .lines = root.lines
    };
}

std::expected<card_id, error> major_asset_id(std::string_view base)
{
    return card_id::parse(std::format("major_arcana.{}", base));
}

std::expected<card_id, error> minor_asset_id(std::string_view suit_key, std::string_view base)
{
    // TODO: A canonical suit holding a custom rank key
    return card_id::parse(std::format("minor_arcana.{}.{}", suit_key, base));
}

bool is_suit_directory(std::string_view name)
{
    return suit_from_string(name).has_value() || is_custom_name(name);
}

std::vector<image_root> find_image_roots(fs::path const& deck_root)
{
    std::vector<image_root> roots;

    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(deck_root, ec))
    {
        if (!entry.is_directory(ec))
            continue;

        auto root = classify_image_root(entry.path());
        if (!root)
            continue;

        // Surrogate assets are representable but not yet loaded; see the
        // follow-up to RFC-034 for what a surrogate card_image would mean
        if (root->kind == image_kind::surrogate)
            continue;

        roots.push_back(*std::move(root));
    }

    std::ranges::sort(roots, {}, &image_root::name);
    return roots;
}

std::vector<discovered_asset> discover_directory(
    fs::path const& dir, image_kind kind, bool allow_variants
)
{
    // (base, variant key) -> the best candidate seen so far, with the chain rank
    // that won it. Not unordered_map so the result comes out deterministically
    std::map<std::pair<std::string, std::string>, std::pair<int, fs::path>> best;

    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(dir, ec))
    {
        if (!entry.is_regular_file(ec))
            continue;

        auto const filename = entry.path().filename().string();
        auto const parts = data::split_asset_filename(filename);
        if (parts.base.empty())
            continue;

        if (!parts.variant_key.empty() && !allow_variants)
            continue;

        auto const rank = data::chain_rank(kind, parts.extension);
        if (!rank)
            continue;


        auto key = std::pair{std::string{parts.base}, std::string{parts.variant_key}};
        auto const it = best.find(key);

        // break tie by filename
        if (it == best.end() || *rank < it->second.first ||
            (*rank == it->second.first && entry.path() < it->second.second))
            best.insert_or_assign(std::move(key), std::pair{*rank, entry.path()});
    }

    std::vector<discovered_asset> assets;
    assets.reserve(best.size());
    for (auto& [key, candidate] : best)
        assets.push_back(
            {.base = key.first, .variant_key = key.second, .path = std::move(candidate.second)}
        );

    return assets;
}

namespace
{

// One image root's major_arcana/ and minor_arcana/<suit>/ contribution
void collect_card_ids(image_root const& root, std::unordered_set<std::string>& ids)
{
    for (auto const& asset :
         discover_directory(root.path / "major_arcana", root.kind, /*allow_variants=*/true))
        if (auto const id = major_asset_id(asset.base))
            ids.insert(id->to_canonical());

    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(root.path / "minor_arcana", ec))
    {
        if (!entry.is_directory(ec))
            continue;

        auto const suit_key = entry.path().filename().string();
        if (!is_suit_directory(suit_key))
            continue;

        for (auto const& asset :
             discover_directory(entry.path(), root.kind, /*allow_variants=*/true))
            if (auto const id = minor_asset_id(suit_key, asset.base))
                ids.insert(id->to_canonical());
    }
}

}  // namespace

std::unordered_set<std::string> discover_card_ids(fs::path const& deck_root)
{
    std::unordered_set<std::string> ids;

    for (auto const& root : find_image_roots(deck_root)) collect_card_ids(root, ids);

    return ids;
}

}  // namespace arcana::detail
