// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/error.hpp>

#include <toml++/toml.hpp>

#include <cstdint>
#include <expected>
#include <filesystem>

namespace arcana::detail
{

// [deck].schema_version, split into its two parts
struct schema_version
{
    std::uint32_t major;
    std::uint32_t minor;
};

// Reads [deck].schema_version, which is required.
//
// Absent, not a string, or anything other than "two decimal integers separated
// by a dot" is a parse_error rather than a silent default: a deck that does not
// say which major it is written for cannot be read under either one.
[[nodiscard]] std::expected<schema_version, error> read_schema_version(
    toml::table const& document, std::filesystem::path const& deck_directory
);

}  // namespace arcana::detail
