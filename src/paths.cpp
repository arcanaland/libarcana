// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/paths.hpp>

#include <algorithm>
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

void collect_deck_directories(
    std::filesystem::path const& library, std::vector<std::filesystem::path>& out
)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(library, ec))
        return;

    for (auto const& entry : std::filesystem::directory_iterator(library, ec))
    {
        if (!entry.is_directory())
            continue;
        if (!std::filesystem::is_regular_file(entry.path() / "deck.toml"))
            continue;

        // Earlier roots shadow later ones, by directory name.
        auto const name = entry.path().filename();
        bool const shadowed = std::ranges::any_of(
            out, [&name](std::filesystem::path const& seen) { return seen.filename() == name; }
        );
        if (!shadowed)
            out.push_back(entry.path());
    }
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


std::vector<std::filesystem::path> enumerate_deck_directories(
    std::optional<std::filesystem::path> const& root_override
)
{
    std::vector<std::filesystem::path> result;
    collect_deck_directories(paths::deck_library_path(root_override), result);
    return result;
}

std::vector<std::filesystem::path> enumerate_deck_directories(
    std::vector<std::filesystem::path> const& roots
)
{
    std::vector<std::filesystem::path> result;

    if (roots.empty())
    {
        collect_deck_directories(paths::deck_library_path(), result);
        return result;
    }

    for (auto const& root : roots) collect_deck_directories(root, result);

    return result;
}

}  // namespace paths

}  // namespace arcana
