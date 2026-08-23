// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "summary.hpp"

#include "discovery.hpp"
#include "document.hpp"
#include "schema_version.hpp"
#include "standard_cards.hpp"
#include "toml_read.hpp"
#include "v1_compat/summary.hpp"

#include <arcana/card.hpp>

#include <algorithm>
#include <cstddef>
#include <format>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace arcana::detail
{

namespace
{

// Walk the directory tree to count how many cards a deck has
std::size_t count_cards(toml::table const& document, std::filesystem::path const& deck_directory)
{
    auto cards = discover_card_ids(deck_directory);

    for (int number = 0; number <= max_canonical_major_arcana_number; ++number)
        cards.insert(std::format("major_arcana.{:02}", number));

    for (auto const s : standard_suits)
        for (auto const r : standard_ranks)
            cards.insert(std::format("minor_arcana.{}.{}", to_string(s), to_string(r)));

    for (auto const& excluded : get_string_array(document["excluded_cards"]["cards"]))
        cards.erase(excluded);

    return cards.size();
}

}  // namespace

[[nodiscard]] auto load_deck_summary(std::filesystem::path const& deck_directory)
    -> std::expected<deck_summary, error>
{
    auto document = load_deck_document(deck_directory);
    if (!document)
        return std::unexpected(std::move(document.error()));

    toml::table const& table = (*document)->table;

    auto version = read_schema_version(table, deck_directory);
    if (!version)
        return std::unexpected(std::move(version.error()));

    toml::table const& deck_table = *table["deck"].as_table();

    deck_summary summary{
        .directory_name = deck_directory.filename().string(),
        .path = deck_directory,
        .identifier = get_string(deck_table["identifier"]),
        .name = get_string_or(deck_table["name"]),
        .version = get_string_or(deck_table["version"]),
        .artist = get_string(deck_table["artist"]),
        .icon = std::nullopt,
        .card_count =
            version->major == 1 ? v1_compat::count_cards(table) : count_cards(table, deck_directory)
    };

    if (auto const icon = get_string(deck_table["icon"]); icon && !icon->empty())
        summary.icon = deck_directory / *icon;

    return summary;
}

}  // namespace arcana::detail
