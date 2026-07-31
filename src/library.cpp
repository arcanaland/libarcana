// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Which directories hold decks. Reading what is in one of them belongs to src/loader,
// which is why nothing here needs toml++.

#include <arcana/library.hpp>
#include <arcana/paths.hpp>

#include "loader/summary.hpp"

#include <format>
#include <utility>

namespace arcana
{

std::vector<deck_summary> enumerate_decks(std::vector<std::filesystem::path> const& roots)
{
    std::vector<deck_summary> result;

    for (auto const& dir : paths::enumerate_deck_directories(roots))
        if (auto summary = detail::read_deck_summary(dir))
            result.push_back(*std::move(summary));

    return result;
}

std::expected<deck, error> load_deck_by_name(
    std::string const& directory_name, std::vector<std::filesystem::path> const& roots,
    std::optional<std::string> const& language
)
{
    // Deliberately matches on the directory name alone, without checking that the deck
    // is readable the way enumerate_decks() does. A malformed deck is therefore absent
    // from the listing but still reports its parse error when asked for by name, which
    // is a better diagnostic than claiming the deck does not exist.
    for (auto const& dir : paths::enumerate_deck_directories(roots))
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
