// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "document.hpp"

#include <arcana/deck.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace arcana::detail
{

// The 2.0 front end: reads a 2.0 document into the one normalized deck model.
//
// A stub today. It delegates to the 1.0 front end so that load_deck's dispatch
// is complete and testable before the real v2 reader exists; TASK-032 layer 4
// replaces the body. What that costs meanwhile is metadata, not cards — the 1.0
// front end materializes the 78 canonical slots and discovers images under any
// major, so a 2.0 deck already loads partly, and partly wrongly
[[nodiscard]] deck build_v2_deck(
    std::filesystem::path deck_root, std::shared_ptr<deck_document const> document,
    std::vector<std::string> const& languages
);

}  // namespace arcana::detail
