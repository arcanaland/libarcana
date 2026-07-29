// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/library.hpp>
#include <arcana/paths.hpp>

#include <toml++/toml.hpp>

namespace arcana
{

std::vector<deck_summary> enumerate_decks(std::vector<std::filesystem::path> const& roots)
{
    std::vector<deck_summary> result;

    for (auto const& dir : enumerate_deck_directories(roots))
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
    std::string const& directory_name, std::vector<std::filesystem::path> const& roots,
    std::optional<std::string> const& language
)
{
    for (auto const& dir : enumerate_deck_directories(roots))
        if (dir.filename() == directory_name)
            return load_deck(dir, language);

    return std::unexpected(
        error{
            .code = error_code::not_found,
            .message =
                std::format("no deck directory named '{}' in the deck library", directory_name)
        }
    );
}

}  // namespace arcana
