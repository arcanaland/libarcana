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

struct schema_version
{
    std::uint32_t major;
    std::uint32_t minor;
};

[[nodiscard]] std::expected<schema_version, error> read_schema_version(
    toml::table const& document, std::filesystem::path const& deck_directory
);

}  // namespace arcana::detail
