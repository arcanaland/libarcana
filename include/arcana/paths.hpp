// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace arcana
{

// All functions below consult the real environment (HOME, XDG_DATA_HOME, XDG_CONFIG_HOME)
// unless `root_override` is set, in which case they are resolved entirely underneath
// `root_override` and the environment is not read at all. This is what makes them testable.

std::filesystem::path xdg_data_home(
    std::optional<std::filesystem::path> const& root_override = std::nullopt
);

std::filesystem::path xdg_config_home(
    std::optional<std::filesystem::path> const& root_override = std::nullopt
);

std::filesystem::path deck_library_path(
    std::optional<std::filesystem::path> const& root_override = std::nullopt
);

// Directory names directly under the deck library path, unsorted, that look like decks
// (contain a deck.toml). Does not parse or validate anything.
std::vector<std::filesystem::path> enumerate_deck_directories(
    std::optional<std::filesystem::path> const& root_override = std::nullopt
);

}  // namespace arcana
