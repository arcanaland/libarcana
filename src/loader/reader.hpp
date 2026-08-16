// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "document.hpp"

#include <arcana/deck.hpp>
#include <arcana/error.hpp>

#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace arcana::detail
{

// Reads a 2.0 deck directory into the normalized deck model
[[nodiscard]] std::expected<deck, error> read_deck(
    std::filesystem::path const& deck_directory, std::shared_ptr<deck_document const> document,
    std::vector<std::string> const& languages
);

}  // namespace arcana::detail
