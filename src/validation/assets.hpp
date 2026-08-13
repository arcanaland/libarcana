// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <toml++/toml.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace arcana::validation
{

enum class root_kind : std::uint8_t
{
    scalable,
    raster,
    ansi,
    surrogate,
};

// Reads a top-level directory name as an image root if it is
[[nodiscard]] std::optional<root_kind> parse_image_root(std::string_view name) noexcept;

// Where in a deck tree discovery finds an asset.
struct asset_location
{
    // Empty for the top-level card_backs/
    std::optional<root_kind> kind;

    bool card_back;
};

// Reads a deck-root-relative path as a place discovery looks.
[[nodiscard]] std::optional<asset_location> locate_asset(std::filesystem::path const& relative);

enum class chain_format : std::uint8_t
{
    none,
    png,
    webp,
    avif,
    jpeg,
    svg,
    toml,
};

[[nodiscard]] chain_format chain_format_of(std::string_view extension) noexcept;

// True where a root of this kind considers a file with this extension.
[[nodiscard]] bool chain_admits(std::optional<root_kind> kind, std::string_view extension) noexcept;

// Every path that is listed in deck.toml explicitly
[[nodiscard]] std::vector<std::string> declared_paths(toml::table const& doc);

// Filename extraction helpers
[[nodiscard]] std::string_view extension_of(std::string_view filename) noexcept;
[[nodiscard]] std::string_view stem_of(std::string_view filename) noexcept;


}  // namespace arcana::validation
