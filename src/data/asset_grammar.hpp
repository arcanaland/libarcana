// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

// The asset-discovery grammar

#include <arcana/card.hpp>

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string_view>

namespace arcana::data
{

// A top-level directory name
struct image_root_name
{
    image_kind kind = image_kind::scalable;

    // height of h<n>/ or lines of ansi<n>/
    // nullopt for the others
    std::optional<int> size;
};

// Reads a top-level directory name as an image root if it is one
[[nodiscard]] std::optional<image_root_name> parse_image_root(std::string_view name) noexcept;

// Where an extension sits in `kind`'s chain, or nullopt where the chain ignores it
[[nodiscard]] std::optional<int> chain_rank(image_kind kind, std::string_view extension) noexcept;

// if is one of the baseline formats
[[nodiscard]] bool is_baseline_extension(std::string_view extension) noexcept;

// A card asset filename split into parts
struct asset_filename
{
    // Everything before the last .
    std::string_view stem;

    // The part of the stem before the first .
    std::string_view base;

    // Empty if no variant
    std::string_view variant_key;

    std::string_view extension;

    // Splits a filename at the first and last '.'
    [[nodiscard]] static asset_filename from_filename(std::string_view filename) noexcept;
};

// The components of a deck-root-relative path
struct path_parts
{
    // The deepest a location for a file, for example:
    //    <deck-root>/minor_arcana/<suit>/<file>
    static constexpr std::size_t location_depth = 4;

    std::array<std::string_view, location_depth> parts;
    std::size_t size = 0;

    [[nodiscard]] std::string_view operator[](std::size_t index) const noexcept
    {
        return parts[index];
    }
};

// Splits a deck-relative path
[[nodiscard]] path_parts components_of(std::filesystem::path const& relative) noexcept;

path_parts components_of(std::filesystem::path&&) = delete;  // must outlive input

}  // namespace arcana::data
