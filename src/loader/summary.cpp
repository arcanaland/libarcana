// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "summary.hpp"

#include "document.hpp"
#include "standard_cards.hpp"
#include "toml_read.hpp"

#include <arcana/card.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace arcana::detail
{

namespace
{

bool is_excluded(std::vector<std::string> const& excluded, std::string const& canonical_id)
{
    return std::ranges::find(excluded, canonical_id) != excluded.end();
}

// How many standard cards there are after exclusions
std::size_t count_standard_cards(std::vector<std::string> const& excluded)
{
    std::size_t count = 0;

    for (int number = 0; number <= max_major_arcana_number; ++number)
        if (!is_excluded(excluded, card_id::standard_major(number).to_canonical()))
            ++count;

    for (auto const s : standard_suits)
        for (auto const r : standard_ranks)
            if (!is_excluded(excluded, card_id::standard_minor(s, r).to_canonical()))
                ++count;

    return count;
}

std::size_t count_custom_cards(toml::table const& document)
{
    auto const* custom = document["custom_cards"].as_table();
    if (custom == nullptr)
        return 0;

    std::size_t count = 0;

    if (auto const* major = (*custom)["major_arcana"].as_table())
        for (auto const& [key, value] : *major)
            if (value.as_table() != nullptr)
                ++count;

    if (auto const* minor = (*custom)["minor_arcana"].as_table())
    {
        for (auto const& [key, value] : *minor)
        {
            auto const* suit_table = value.as_table();
            if (suit_table == nullptr)
                continue;

            if (auto const* cards = (*suit_table)["cards"].as_array())
                for (auto const& element : *cards)
                    if (element.as_table() != nullptr)
                        ++count;
        }
    }

    return count;
}

}  // namespace

std::expected<deck_summary, error> load_deck_summary(std::filesystem::path const& deck_directory)
{
    auto document = load_deck_document(deck_directory);
    if (!document)
        return std::unexpected(std::move(document.error()));

    toml::table const& table = (*document)->table;

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
            count_standard_cards(get_string_array(deck_table["excluded_cards"]["cards"])) +
            count_custom_cards(table)
    };

    if (auto const icon = get_string(deck_table["icon"]); icon && !icon->empty())
        summary.icon = deck_directory / *icon;

    return summary;
}

}  // namespace arcana::detail
