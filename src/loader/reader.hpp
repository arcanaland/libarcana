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
//
// Cards are created by files rather than by deck.toml: discovery walks the
// deck's image roots and the `[cards]` and `[suits]` tables annotate what it
// finds. The seventy-eight canonical slots exist for every deck and are
// materialized whether or not a file backs them.
//
// This is *the* reader. The 1.0 compatibility shim beside it reads that closed
// spelling into the same model, and only the dispatcher in loader.cpp names it.
[[nodiscard]] std::expected<deck, error> read_deck(
    std::filesystem::path const& deck_directory, std::shared_ptr<deck_document const> document,
    std::vector<std::string> const& languages
);

}  // namespace arcana::detail
