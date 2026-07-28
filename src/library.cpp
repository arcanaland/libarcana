// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/library.hpp>
#include <arcana/paths.hpp>

#include <toml++/toml.hpp>

namespace arcana
{

std::vector<deck_summary> enumerate_decks(std::optional<std::filesystem::path> const& root_override)
{
    std::vector<deck_summary> result;

    for (auto const& dir : enumerate_deck_directories(root_override))
    {
        auto parsed = toml::parse_file((dir / "deck.toml").string());
        if (!parsed)
            continue;

        auto const* deck_table = parsed.table()["deck"].as_table();
        if (deck_table == nullptr)
            continue;

        result.push_back(
            deck_summary{
                .directory_name = dir.filename().string(),
                .id = (*deck_table)["id"].value<std::string>().value_or(""),
                .name = (*deck_table)["name"].value<std::string>().value_or(""),
                .path = dir,
            }
        );
    }

    return result;
}

std::expected<deck, error> load_deck_by_name(
    std::string const& directory_name, std::optional<std::filesystem::path> const& root_override,
    std::optional<std::string> const& language
)
{
    auto const path = deck_library_path(root_override) / directory_name;
    return load_deck(path, language);
}

}  // namespace arcana
