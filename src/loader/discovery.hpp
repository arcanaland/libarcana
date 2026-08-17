// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/card.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace arcana::detail
{

// A top-level directory a deck's card assets live in
//
// The four forms are scalable/, h<height>/, ansi<lines>/ and surrogate/.
struct image_root
{
    std::filesystem::path path;

    // The directory's own name
    std::string name;

    image_kind kind = image_kind::scalable;

    std::optional<int> height;  // raster only
    std::optional<int> lines;   // ansi only
};

// A card asset filename split from its four parts
struct asset_filename
{
    std::string_view base;

    // Empty where the stem has no variant key
    std::string_view variant_key;

    // Empty where the filename holds no '.' at all
    std::string_view extension;
};

[[nodiscard]] asset_filename split_asset_filename(std::string_view filename) noexcept;

struct discovered_asset
{
    std::string base;
    // Empty for the unsuffixed file
    std::string variant_key;

    std::filesystem::path path;
};

// nullopt if directory is not one of the four forms
[[nodiscard]] std::optional<image_root> classify_image_root(std::filesystem::path const& path);

// Every image root the deck has, sorted by directory name
[[nodiscard]] std::vector<image_root> find_image_roots(std::filesystem::path const& deck_root);

// The model's view of one file found under `root`
[[nodiscard]] card_image image_at(image_root const& root, std::filesystem::path path);

// All files nested in dir as (base, variant key), sorted by base
//
// @param allow_variants False for a card back directory
[[nodiscard]] std::vector<discovered_asset> discover_directory(
    std::filesystem::path const& dir, image_kind kind, bool allow_variants
);

}  // namespace arcana::detail
