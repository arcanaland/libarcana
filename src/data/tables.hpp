// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace arcana::data
{

// The small frozen reference tables the deck checks read.
//

// True when name is one of the 148 CSS Color 4 named colours.
[[nodiscard]] bool is_css_color_name(std::string_view name) noexcept;

// True when rel is one of the five relations the specification registers.
[[nodiscard]] bool is_registered_link_rel(std::string_view rel) noexcept;

// True when rel uses a `x_` escape hatch
[[nodiscard]] bool is_extension_link_rel(std::string_view rel) noexcept;

// What a rights-status URI says about the artwork it describes.
enum class rights_status_class : std::uint8_t
{
    // The artwork is in copyright
    in_copyright,

    // The artwork is free of copyright or dedicated to the public domain.
    no_copyright,

    // The URI is one that declines to say.
    undetermined,
};

// True when uri is a RightsStatements.org URI or a Creative Commons one,
[[nodiscard]] bool is_rights_status_uri(std::string_view uri) noexcept;

[[nodiscard]] std::optional<rights_status_class> classify_rights_status(
    std::string_view uri
) noexcept;

// The two-letter ISO 639-1 code for a three-letter ISO 639-2 or -3 one, or
// nothing when the three-letter code has no shorter form.
[[nodiscard]] std::optional<std::string_view> shortest_language_subtag(
    std::string_view subtag
) noexcept;

// What a licence lets a downstream user do with the work it covers.
struct license_permissions
{
    bool grants_redistribution;
    bool grants_derivation;
};

// The permissions a licence grants, or nothing when it is outside the curated
// table.
[[nodiscard]] std::optional<license_permissions> find_license_permissions(
    std::string_view spdx_id
) noexcept;

}  // namespace arcana::data
