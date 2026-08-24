// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "context.hpp"

#include <filesystem>
#include <vector>

namespace arcana::validation
{

// Every regular file under the deck root, sorted by relative path.
[[nodiscard]] std::vector<deck_file> walk_deck(std::filesystem::path const& root);

}  // namespace arcana::validation
