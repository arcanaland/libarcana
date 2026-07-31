// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <filesystem>
#include <optional>

namespace arcana::paths
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

}  // namespace arcana::paths
