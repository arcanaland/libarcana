// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

// The asset-discovery grammar DECK.md §5.7 defines: image root names
// (§5.7.1), filename parts (§5.7.2), the extension chain (§5.7.4) and the
// deck-relative paths discovery looks at.
//
// This is the one copy. The loader and the validator both read it here.

#include <arcana/card.hpp>

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string_view>

namespace arcana::data
{

// A top-level directory name read as one of §5.7.1's four root forms
struct image_root_name
{
    image_kind kind = image_kind::scalable;

    // The <height> of h<n>/ or the <lines> of ansi<n>/; nullopt for the other two
    std::optional<int> size;
};

// Reads a top-level directory name as an image root if it is one
//
// §5.7.1: scalable/, h<height>/, ansi<lines>/ and surrogate/, where the size is
// a decimal integer greater than zero written without a sign, leading zeroes or
// separators. A size too large to hold in an `int` names no root.
[[nodiscard]] std::optional<image_root_name> parse_image_root(std::string_view name) noexcept;

// Where an extension sits in `kind`'s chain, or nullopt where the chain ignores it
//
// §5.7.4: png, webp, avif, then jpeg and jpg together for raster; svg alone in
// scalable/; toml alone in surrogate/; anything or nothing in an ANSI root.
// Lower ranks win.
[[nodiscard]] std::optional<int> chain_rank(image_kind kind, std::string_view extension) noexcept;

// Whether an extension names one of §5.7.4's three baseline formats
[[nodiscard]] bool is_baseline_extension(std::string_view extension) noexcept;

// A card asset filename split into §5.7.2's parts
struct asset_filename
{
    // Everything before the last '.'
    std::string_view stem;

    // The part of the stem before the first '.'
    std::string_view base;

    // Empty where the stem has no variant key
    std::string_view variant_key;

    // Empty where the filename holds no '.' at all
    std::string_view extension;
};

// Splits a filename at the first and last '.' per §5.7.2
//
// A name that is nothing but an extension, such as `.hidden`, yields an empty
// base; callers reject that.
[[nodiscard]] asset_filename split_asset_filename(std::string_view filename) noexcept;

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

// Splits a deck-relative path, or yields nothing where it runs deeper than
// `location_depth` — no such path is a place discovery looks.
//
// The pieces borrow from `relative`, which must outlive the result.
[[nodiscard]] path_parts components_of(std::filesystem::path const& relative) noexcept;

// The pieces would dangle
path_parts components_of(std::filesystem::path&&) = delete;

}  // namespace arcana::data
