// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "summary.hpp"

#include "document.hpp"
#include "toml_read.hpp"

namespace arcana::detail
{

std::expected<deck_summary, error> read_deck_summary(std::filesystem::path const& deck_directory)
{
    auto document = read_deck_document(deck_directory);
    if (!document)
        return std::unexpected(std::move(document.error()));

    // Non-null by read_deck_document's postcondition
    toml::table const& deck_table = *(*document)->table["deck"].as_table();

    return deck_summary{
        .directory_name = deck_directory.filename().string(),
        .path = deck_directory,
        .id = get_string_or(deck_table["id"]),
        .name = get_string_or(deck_table["name"])
    };
}

}  // namespace arcana::detail
