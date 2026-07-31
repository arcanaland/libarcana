// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/paths.hpp>

#include <cstdlib>

namespace arcana
{

namespace
{

std::optional<std::filesystem::path> getenv_path(char const* name)
{
    char const* value = std::getenv(name);  // NOLINT(concurrency-mt-unsafe)
    if (value == nullptr || *value == '\0')
        return std::nullopt;
    return std::filesystem::path(value);
}

std::optional<std::filesystem::path> home_directory()
{
    return getenv_path("HOME");
}

}  // namespace

namespace paths
{

std::filesystem::path xdg_data_home(std::optional<std::filesystem::path> const& root_override)
{
    if (root_override)
        return *root_override / ".local" / "share";

    if (auto xdg = getenv_path("XDG_DATA_HOME"))
        return *xdg;

    return home_directory().value_or(std::filesystem::path{}) / ".local" / "share";
}

std::filesystem::path xdg_config_home(std::optional<std::filesystem::path> const& root_override)
{
    if (root_override)
        return *root_override / ".config";

    if (auto xdg = getenv_path("XDG_CONFIG_HOME"))
        return *xdg;

    return home_directory().value_or(std::filesystem::path{}) / ".config";
}

std::filesystem::path deck_library_path(std::optional<std::filesystem::path> const& root_override)
{
    return xdg_data_home(root_override) / "tarot" / "decks";
}

}  // namespace paths

}  // namespace arcana
