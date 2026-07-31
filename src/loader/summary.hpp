// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/error.hpp>
#include <arcana/library.hpp>

#include <expected>
#include <filesystem>

namespace arcana::detail
{

// The id and name of the deck in a directory, without building any of its cards
//
// deck_library scans every deck on the system, so it cannot afford a full load per
// entry: deck_loader materializes 78 cards and stats every image root along the way.
// This reads the manifest, takes two fields, and drops it.
//
// The error is read_deck_document's, forwarded unchanged -- deck_library reports it as
// a broken_deck rather than dropping the directory on the floor.
[[nodiscard]] std::expected<deck_summary, error> read_deck_summary(
    std::filesystem::path const& deck_directory
);

}  // namespace arcana::detail
