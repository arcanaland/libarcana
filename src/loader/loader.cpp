// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "document.hpp"
#include "schema_version.hpp"
#include "v1_compat/reader.hpp"

#include <arcana/deck.hpp>

#include <utility>

namespace arcana
{

std::expected<deck, error> load_deck(
    std::filesystem::path const& deck_directory, std::vector<std::string> const& languages
)
{
    auto document = detail::load_deck_document(deck_directory);
    if (!document)
        return std::unexpected(std::move(document.error()));

    auto version = detail::read_schema_version((*document)->table, deck_directory);
    if (!version)
        return std::unexpected(std::move(version.error()));

    return detail::v1_compat::read_deck(deck_directory, *std::move(document), languages);
}

}  // namespace arcana
