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

    // This dispatch and rule::applies_to are the only two places allowed to
    // branch on the major. Major 1 reads under the v1 shim and every other
    // major best-effort under the newest front end -- which is that same shim
    // until TASK-032 layer 3b writes detail::read_deck on the neutral ground
    // this layer freed, so there is one arm to write today
    return detail::v1_compat::read_deck(deck_directory, *std::move(document), languages);
}

}  // namespace arcana
