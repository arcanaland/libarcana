// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/card.hpp>
#include <arcana/error.hpp>

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
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

// The card a file based `base` under an image root's major_arcana/ names
[[nodiscard]] std::expected<card_id, error> major_asset_id(std::string_view base);

// The card a file based `base` under minor_arcana/<suit_key>/ names
[[nodiscard]] std::expected<card_id, error> minor_asset_id(
    std::string_view suit_key, std::string_view base
);

// Whether a directory under an image root's minor_arcana/ names a suit
[[nodiscard]] bool is_suit_directory(std::string_view name);

// All files nested in dir as (base, variant key), sorted by base
//
// @param allow_variants False for a card back directory
[[nodiscard]] std::vector<discovered_asset> discover_directory(
    std::filesystem::path const& dir, image_kind kind, bool allow_variants
);

// Every canonical ID the files under a deck's image roots create
[[nodiscard]] std::unordered_set<std::string> discover_card_ids(
    std::filesystem::path const& deck_root
);

}  // namespace arcana::detail
