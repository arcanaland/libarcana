// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace arcana
{

namespace paths
{

// The functions below use env vars XDG_DATA_HOME, XDG_CONFIG_HOME, etc unless
// you pass in an override
std::filesystem::path xdg_data_home(
    std::optional<std::filesystem::path> const& root_override = std::nullopt
);

std::filesystem::path xdg_config_home(
    std::optional<std::filesystem::path> const& root_override = std::nullopt
);

std::filesystem::path deck_library_path(
    std::optional<std::filesystem::path> const& root_override = std::nullopt
);

// Unsorted list of directory names directly under the deck library path
// that contain a deck.toml
std::vector<std::filesystem::path> enumerate_deck_directories(
    std::optional<std::filesystem::path> const& root_override = std::nullopt
);

// Unsorted list of directory names directly under the deck library path
// that contain a deck.toml
//
// Roots behave like PATH: when the same directory name appears under more than
// one root, the first wins and the shadowed one is not listed
std::vector<std::filesystem::path> enumerate_deck_directories(
    std::vector<std::filesystem::path> const& roots
);

}  // namespace paths

}  // namespace arcana
