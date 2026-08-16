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

    // The directory's own name, which is what card_image::source_dir carries
    std::string name;

    image_kind kind = image_kind::scalable;

    std::optional<int> height;  // raster only
    std::optional<int> lines;   // ansi only
};

// A card asset filename read as its four parts
//
// Given "06.two_women.png": extension "png" (after the last '.'), stem
// "06.two_women" (before it), base "06" (before the stem's first '.') and
// variant key "two_women" (the rest of the stem).
//
// The views point into the filename this was split from, which must outlive it.
struct asset_filename
{
    std::string_view base;

    // Empty where the stem carries no variant key
    std::string_view variant_key;

    // Empty where the filename holds no '.' at all
    std::string_view extension;
};

[[nodiscard]] asset_filename split_asset_filename(std::string_view filename) noexcept;

// One file discovery chose to supply a (base, variant key) pair
struct discovered_asset
{
    std::string base;

    // Empty for the unsuffixed file, which is the card's default artwork
    std::string variant_key;

    std::filesystem::path path;
};

// Every image root the deck has, sorted by directory name
//
// surrogate/ is recognized so that discovery does not mistake it for deck
// material, and then dropped: surrogate assets are not read yet.
[[nodiscard]] std::vector<image_root> find_image_roots(std::filesystem::path const& deck_root);

// The files `dir` supplies, one per (base, variant key), sorted by base
//
// Where a directory offers several files for one stem the choice is made by the
// extension chain -- png, webp, avif, then jpeg/jpg for a raster root, svg
// alone in scalable/ -- and never by filesystem order. An ANSI root matches on
// stem alone and takes any extension or none.
//
// @param allow_variants False for a card back directory, whose whole stem is
//                       the design key: a stem holding a '.' names no design
[[nodiscard]] std::vector<discovered_asset> discover_directory(
    std::filesystem::path const& dir, image_kind kind, bool allow_variants
);

}  // namespace arcana::detail
