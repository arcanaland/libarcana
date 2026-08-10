// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Where discovery looks and what it will pick up when it gets there.
//
// DECK.md 5.7.1 (image roots), 5.7.2 (extensions, stems and bases) and 5.7.4
// (the extension chain), shared by the `backs` and `images` areas.

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

// The four forms of image root DECK.md 5.7.1 defines.
enum class root_kind : std::uint8_t
{
    scalable,
    raster,
    ansi,
    surrogate,
};

// Reads a top-level directory name as an image root, or nothing where the name
// is not one. The size is deliberately not returned: no check in this layer
// needs it, and parsing it would raise an overflow question the specification
// does not answer.
[[nodiscard]] std::optional<root_kind> parse_image_root(std::string_view name) noexcept;

// Where in a deck tree discovery finds an asset.
struct asset_location
{
    // Empty for the top-level `card_backs/` directory, which DECK.md 5.5 gives
    // no kind and no size.
    std::optional<root_kind> kind;

    // True under a `card_backs/` directory, at the top level or inside a root.
    bool card_back;
};

// Reads a deck-root-relative path as a place discovery looks.
//
// Nothing is returned for any other path: DECK.md 5.7.1 ignores every other
// top-level directory, every other subdirectory of a root, and any file lying
// loose in a root rather than in one of its subdirectories.
[[nodiscard]] std::optional<asset_location> locate_asset(std::filesystem::path const& relative);

// The formats of DECK.md 5.7.4's extension chain. `jpeg` and `jpg` are one
// format, which is why this is not simply the extension.
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

// True where a root of this kind considers a file carrying this extension.
//
// ANSI roots admit anything: DECK.md 5.4 gives ANSI files any extension or
// none and has an application read the kind from the content instead.
[[nodiscard]] bool chain_admits(std::optional<root_kind> kind, std::string_view extension) noexcept;

// The part of a filename after the last `.`, empty where there is none. A
// leading dot is part of the name rather than an extension separator.
[[nodiscard]] std::string_view extension_of(std::string_view filename) noexcept;

// Everything before the last `.`, which is the whole filename where it has no
// extension.
[[nodiscard]] std::string_view stem_of(std::string_view filename) noexcept;

// Every deck-relative path deck.toml points at explicitly, as generic strings.
//
// A file named this way is shown whether or not discovery would have found it,
// so the checks that report an undiscovered file leave these alone.
[[nodiscard]] std::vector<std::string> declared_paths(toml::table const& doc);

}  // namespace arcana::validation
