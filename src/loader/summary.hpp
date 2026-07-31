// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/error.hpp>
#include <arcana/library.hpp>

#include <expected>
#include <filesystem>

namespace arcana::detail
{

// Load a summary of a deck computed from its manifest without walking the directory tree
[[nodiscard]] std::expected<deck_summary, error> load_deck_summary(
    std::filesystem::path const& deck_directory
);

}  // namespace arcana::detail
