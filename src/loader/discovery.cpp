// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "discovery.hpp"

#include "../data/ascii.hpp"

#include <algorithm>
#include <charconv>
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

// The <height> of an h<n>/ root or the <lines> of an ansi<n>/ root
std::optional<int> parse_root_size(std::string_view digits)
{
    if (digits.empty() || !std::ranges::all_of(digits, data::is_digit))
        return std::nullopt;

    if (digits.front() == '0')
        return std::nullopt;

    int value = 0;
    auto const [_, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (ec != std::errc{} || value <= 0)
        return std::nullopt;

    return value;
}

// Where an extension sits in the chain for `kind`, or nullopt where discovery
// ignores it entirely. Lower ranks win.
std::optional<int> extension_rank(std::string_view extension, image_kind kind)
{
    switch (kind)
    {
        case image_kind::scalable:
            return extension == "svg" ? std::optional{0} : std::nullopt;

        case image_kind::raster:
            if (extension == "png")
                return 0;
            if (extension == "webp")
                return 1;
            if (extension == "avif")
                return 2;
            if (extension == "jpeg" || extension == "jpg")
                return 3;
            return std::nullopt;

        case image_kind::ansi:
            // ANSI files take any extension or none
            return 0;
    }

    return std::nullopt;
}

}  // namespace

std::optional<image_root> classify_image_root(fs::path const& path)
{
    auto const name = path.filename().string();

    if (name == "scalable")
        return image_root{.path = path, .name = name, .kind = image_kind::scalable};

    if (name.starts_with("h"))
    {
        if (auto const height = parse_root_size(std::string_view{name}.substr(1)))
            return image_root{
                .path = path, .name = name, .kind = image_kind::raster, .height = height
            };
    }

    if (name.starts_with("ansi"))
    {
        if (auto const lines = parse_root_size(std::string_view{name}.substr(4)))
            return image_root{.path = path, .name = name, .kind = image_kind::ansi, .lines = lines};
    }

    return std::nullopt;
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

asset_filename split_asset_filename(std::string_view filename) noexcept
{
    asset_filename result;

    auto const last_dot = filename.rfind('.');
    auto const stem = last_dot == std::string_view::npos ? filename : filename.substr(0, last_dot);
    if (last_dot != std::string_view::npos)
        result.extension = filename.substr(last_dot + 1);

    auto const first_dot = stem.find('.');
    result.base = stem.substr(0, first_dot);
    if (first_dot != std::string_view::npos)
        result.variant_key = stem.substr(first_dot + 1);

    return result;
}

std::vector<image_root> find_image_roots(fs::path const& deck_root)
{
    std::vector<image_root> roots;

    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(deck_root, ec))
    {
        if (!entry.is_directory(ec))
            continue;

        // TODO
        if (entry.path().filename() == "surrogate")
            continue;

        if (auto root = classify_image_root(entry.path()))
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
        auto const parts = split_asset_filename(filename);
        if (parts.base.empty())
            continue;

        if (!parts.variant_key.empty() && !allow_variants)
            continue;

        auto const chain_rank = extension_rank(parts.extension, kind);
        if (!chain_rank)
            continue;


        auto key = std::pair{std::string{parts.base}, std::string{parts.variant_key}};
        auto const it = best.find(key);

        // break tie by filename
        if (it == best.end() || *chain_rank < it->second.first ||
            (*chain_rank == it->second.first && entry.path() < it->second.second))
            best.insert_or_assign(std::move(key), std::pair{*chain_rank, entry.path()});
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
void collect_card_ids(image_root const& root, std::set<std::string>& ids)
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

std::set<std::string> discover_card_ids(fs::path const& deck_root)
{
    std::set<std::string> ids;

    for (auto const& root : find_image_roots(deck_root)) collect_card_ids(root, ids);

    return ids;
}

}  // namespace arcana::detail
