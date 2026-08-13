// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "assets.hpp"

#include "../data/ascii.hpp"

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

bool is_root_size(std::string_view digits) noexcept
{
    if (digits.empty() || digits.front() == '0')
        return false;

    return std::ranges::all_of(digits, data::is_digit);
}

// The components of a deck-root-relative path.
struct path_parts
{
    // The deepest a location for a file, for example:
    //    <deck-root>/minor_arcana/<suit>/<file>
    static constexpr std::size_t location_depth = 4;
    std::array<std::string_view, location_depth> parts;
    std::size_t size = 0;

    std::string_view operator[](std::size_t index) const noexcept
    {
        return parts[index];
    }
};

path_parts components_of(std::filesystem::path const& relative) noexcept
{
    // We'll need to update this strategy when we support Windows
    static_assert(std::filesystem::path::preferred_separator == '/');

    path_parts found;
    for (auto const one : std::views::split(std::string_view{relative.native()}, '/'))
    {
        if (found.size == found.parts.size())
            return {};

        found.parts[found.size++] = std::string_view{one};
    }

    return found;
}

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

std::optional<root_kind> parse_image_root(std::string_view name) noexcept
{
    if (name == "scalable")
        return root_kind::scalable;

    if (name == "surrogate")
        return root_kind::surrogate;

    if (name.starts_with("h") && is_root_size(name.substr(1)))
        return root_kind::raster;

    if (name.starts_with("ansi") && is_root_size(name.substr(4)))
        return root_kind::ansi;

    return std::nullopt;
}

std::optional<asset_location> locate_asset(std::filesystem::path const& relative)
{
    auto const parts = components_of(relative);

    // The top-level card back directory: card_backs/<file>.
    if (parts.size == 2 && parts[0] == "card_backs")
        return asset_location{.kind = std::nullopt, .card_back = true};

    if (parts.size < 3)
        return std::nullopt;

    auto const kind = parse_image_root(parts[0]);
    if (!kind)
        return std::nullopt;

    if (parts.size == 3 && (parts[1] == "major_arcana" || parts[1] == "card_backs"))
        return asset_location{.kind = kind, .card_back = parts[1] == "card_backs"};

    // `<root>/minor_arcana/<suit>/<file>`.
    if (parts.size == 4 && parts[1] == "minor_arcana")
        return asset_location{.kind = kind, .card_back = false};

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

bool chain_admits(std::optional<root_kind> kind, std::string_view extension) noexcept
{
    auto const format = chain_format_of(extension);

    // The top-level card back directory carries backs of no declared kind, so
    // it takes the raster chain.
    if (!kind)
        return format != chain_format::none && format != chain_format::svg &&
               format != chain_format::toml;

    switch (*kind)
    {
        case root_kind::ansi:
            return true;

        case root_kind::scalable:
            return format == chain_format::svg;

        case root_kind::surrogate:
            return format == chain_format::toml;

        case root_kind::raster:
            return format == chain_format::png || format == chain_format::webp ||
                   format == chain_format::avif || format == chain_format::jpeg;
    }

    return false;
}

std::string_view extension_of(std::string_view filename) noexcept
{
    auto const dot = filename.find_last_of('.');
    if (dot == std::string_view::npos || dot == 0)
        return {};

    return filename.substr(dot + 1);
}

std::string_view stem_of(std::string_view filename) noexcept
{
    auto const dot = filename.find_last_of('.');
    if (dot == std::string_view::npos || dot == 0)
        return filename;

    return filename.substr(0, dot);
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
