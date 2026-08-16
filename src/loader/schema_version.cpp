// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "schema_version.hpp"

#include "../data/text.hpp"
#include "manifest.hpp"

#include <charconv>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace arcana::detail
{

namespace
{

// from_chars alone is too permissive
std::optional<std::uint32_t> decimal_integer(std::string_view text)
{
    if (text.empty())
        return std::nullopt;

    for (char const c : text)
        if (c < '0' || c > '9')
            return std::nullopt;

    std::uint32_t value = 0;
    auto const [_, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{})
        return std::nullopt;

    return value;
}

error malformed(std::filesystem::path const& deck_directory, std::string detail)
{
    return error{
        .code = error_code::parse_error,
        .message = std::format(
            "{}: {}", (deck_directory / deck_manifest_filename).string(), std::move(detail)
        )
    };
}

}  // namespace

std::expected<schema_version, error> read_schema_version(
    toml::table const& document, std::filesystem::path const& deck_directory
)
{
    auto const node = document["deck"]["schema_version"];
    if (!node)
        return std::unexpected(malformed(deck_directory, "[deck].schema_version is required"));

    auto const text = node.value<std::string>();
    if (!text)
    {
        return std::unexpected(
            malformed(deck_directory, R"([deck].schema_version must be a string, e.g. "2.0")")
        );
    }

    auto const parts = cut(*text, '.');
    auto const major = parts ? decimal_integer(parts->first) : std::nullopt;
    auto const minor = parts ? decimal_integer(parts->second) : std::nullopt;

    if (!major || !minor)
    {
        return std::unexpected(malformed(
            deck_directory,
            std::format(
                R"([deck].schema_version "{}" is not two decimal integers separated by a dot)",
                *text
            )
        ));
    }

    return schema_version{.major = *major, .minor = *minor};
}

}  // namespace arcana::detail
