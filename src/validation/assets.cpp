// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "assets.hpp"

#include "../data/asset_grammar.hpp"

#include <toml++/toml.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace arcana::validation
{

namespace
{

// Collects a path-valued key where it holds a string.
void add_path(std::vector<std::string>& found, toml::node_view<toml::node const> value)
{
    if (auto const* text = value.as_string())
        found.push_back(text->get());
}

// Collects the `image` path off every subtable of `table`.
void add_image_paths(std::vector<std::string>& found, toml::node_view<toml::node const> table)
{
    auto const* subtables = table.as_table();
    if (subtables == nullptr)
        return;

    for (auto const& [key, value] : *subtables)
        if (auto const* one = value.as_table())
            add_path(found, (*one)["image"]);
}

}  // namespace

std::optional<asset_location> locate_asset(std::filesystem::path const& relative)
{
    auto const parts = data::components_of(relative);

    // The top-level card back directory: card_backs/<file>.
    if (parts.size == 2 && parts[0] == "card_backs")
        return asset_location{.kind = std::nullopt, .card_back = true};

    if (parts.size < 3)
        return std::nullopt;

    auto const root = data::parse_image_root(parts[0]);
    if (!root)
        return std::nullopt;

    if (parts.size == 3 && (parts[1] == "major_arcana" || parts[1] == "card_backs"))
        return asset_location{.kind = root->kind, .card_back = parts[1] == "card_backs"};

    // `<root>/minor_arcana/<suit>/<file>`.
    if (parts.size == 4 && parts[1] == "minor_arcana")
        return asset_location{.kind = root->kind, .card_back = false};

    return std::nullopt;
}

chain_format chain_format_of(std::string_view extension) noexcept
{
    if (extension == "png")
        return chain_format::png;

    if (extension == "webp")
        return chain_format::webp;

    if (extension == "avif")
        return chain_format::avif;

    // the spec ranks these together
    if (extension == "jpeg" || extension == "jpg")
        return chain_format::jpeg;

    if (extension == "svg")
        return chain_format::svg;

    if (extension == "toml")
        return chain_format::toml;

    return chain_format::none;
}

bool chain_admits(std::optional<image_kind> kind, std::string_view extension) noexcept
{
    // The top-level card back directory carries backs of no declared kind, so
    // it takes the raster chain
    return data::chain_rank(kind.value_or(image_kind::raster), extension).has_value();
}

std::string_view extension_of(std::string_view filename) noexcept
{
    return data::split_asset_filename(filename).extension;
}

std::string_view stem_of(std::string_view filename) noexcept
{
    return data::split_asset_filename(filename).stem;
}

std::vector<std::string> declared_paths(toml::table const& doc)
{
    std::vector<std::string> found;

    add_image_paths(found, doc["cards"]);

    add_path(found, doc["deck"]["icon"]);

    if (auto const* licenses = doc["deck"]["license_files"].as_array())
        for (auto const& element : *licenses)
            if (auto const* text = element.as_string())
                found.push_back(text->get());

    // designs for 2.0 and variants for 1.0
    add_image_paths(found, doc["card_backs"]["designs"]);
    add_image_paths(found, doc["card_backs"]["variants"]);


    std::ranges::sort(found);
    auto const dupes = std::ranges::unique(found);
    found.erase(dupes.begin(), dupes.end());

    return found;
}

}  // namespace arcana::validation
