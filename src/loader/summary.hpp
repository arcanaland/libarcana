// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/error.hpp>
#include <arcana/library.hpp>

#include <expected>
#include <filesystem>

namespace arcana::detail
{

// Load a summary of a deck, dispatched on [deck].schema_version
//
// A 1.0 summary is read from the manifest alone. A 2.0 one walks the image
// roots, because 2.0 discovers its cards from the tree rather than listing
// them and a count that disagreed with load_deck would be worse than a slow
// one — the library list and the loaded deck would describe the same deck
// differently.
[[nodiscard]] std::expected<deck_summary, error> load_deck_summary(
    std::filesystem::path const& deck_directory
);

}  // namespace arcana::detail
