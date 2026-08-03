// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/error.hpp>

#include <toml++/toml.hpp>

#include <expected>
#include <filesystem>
#include <memory>

namespace arcana::detail
{

// The parsed deck.toml
struct deck_document
{
    toml::table table;
};

// Loads and parses a <deck_directory>/deck.toml
//
// Returns parse_error for a file that is missing, malformed, or has no [deck] table.
[[nodiscard]] std::expected<std::shared_ptr<deck_document const>, error> load_deck_document(
    std::filesystem::path const& deck_directory
);

}  // namespace arcana::detail
