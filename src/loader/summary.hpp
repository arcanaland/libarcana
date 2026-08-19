// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/error.hpp>
#include <arcana/library.hpp>

#include <expected>
#include <filesystem>

namespace arcana::detail
{

// Load a summary of a deck
[[nodiscard]] auto load_deck_summary(std::filesystem::path const& deck_directory)
    -> std::expected<deck_summary, error>;

}  // namespace arcana::detail
