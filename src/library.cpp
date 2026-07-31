// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Which directories hold decks. Reading what is in one of them belongs to src/loader,
// which is why nothing here needs toml++.

#include <arcana/library.hpp>
#include <arcana/paths.hpp>

#include "loader/manifest.hpp"
#include "loader/summary.hpp"

#include <algorithm>
#include <format>
#include <system_error>
#include <utility>

namespace arcana
{

namespace
{

// An empty root list means the XDG library. Resolving that here, once, is what lets
// roots() report what is actually being searched -- and stops an empty vector from
// reading like "search nothing" at a call site.
std::vector<std::filesystem::path> resolve_roots(std::vector<std::filesystem::path> roots)
{
    if (roots.empty())
        roots.push_back(paths::deck_library_path());

    return roots;
}

// Candidate deck directories directly under one root, appended in directory order.
//
// What makes a directory a deck, and which of two candidates sharing a name wins, are
// both facts about the deck format rather than about the filesystem -- which is why
// this lives beside deck_library instead of in arcana::paths.
void collect_deck_directories(
    std::filesystem::path const& root, std::vector<std::filesystem::path>& out
)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(root, ec))
        return;

    for (auto const& entry : std::filesystem::directory_iterator(root, ec))
    {
        if (!entry.is_directory())
            continue;
        if (!std::filesystem::is_regular_file(entry.path() / detail::deck_manifest_filename))
            continue;

        // Earlier roots shadow later ones, by directory name. Decided here, before
        // anything has read a manifest, so a broken deck shadows a readable one.
        auto const name = entry.path().filename();
        bool const shadowed = std::ranges::any_of(
            out, [&name](std::filesystem::path const& seen) { return seen.filename() == name; }
        );

        if (!shadowed)
            out.push_back(entry.path());
    }
}

std::vector<std::filesystem::path> collect_deck_directories(
    std::vector<std::filesystem::path> const& roots
)
{
    std::vector<std::filesystem::path> result;

    for (auto const& root : roots) collect_deck_directories(root, result);

    return result;
}

}  // namespace

deck_library::deck_library(library_options options)
    : roots_(resolve_roots(std::move(options.roots))), language_(std::move(options.language))
{
    refresh();
}

void deck_library::refresh()
{
    decks_.clear();
    broken_.clear();

    // collect_deck_directories has already applied root precedence, so a directory name
    // reaches us at most once and lands in exactly one of the two lists.
    for (auto const& dir : collect_deck_directories(roots_))
    {
        auto summary = detail::read_deck_summary(dir);

        if (summary)
        {
            decks_.push_back(*std::move(summary));
            continue;
        }

        broken_.push_back(
            broken_deck{
                .directory_name = dir.filename().string(),
                .path = dir,
                .problem = std::move(summary.error())
            }
        );
    }

    std::ranges::sort(decks_, {}, &deck_summary::directory_name);
    std::ranges::sort(broken_, {}, &broken_deck::directory_name);
}

std::optional<deck_summary> deck_library::find(std::string_view directory_name) const
{
    auto const found = std::ranges::find(decks_, directory_name, &deck_summary::directory_name);
    if (found == decks_.end())
        return std::nullopt;

    return *found;
}

std::optional<deck_summary> deck_library::find_by_id(std::string_view deck_id) const
{
    auto const found = std::ranges::find(decks_, deck_id, &deck_summary::id);
    if (found == decks_.end())
        return std::nullopt;

    return *found;
}

std::expected<deck, error> deck_library::load(std::string_view directory_name) const
{
    if (auto const found = std::ranges::find(decks_, directory_name, &deck_summary::directory_name);
        found != decks_.end())
        return load_path(found->path);

    // A broken deck resolves too. The scan settled which directories exist, not what is
    // in them, so this re-reads rather than replaying the stored error: a full load
    // walks much more of the file than the summary did and can say more about why.
    if (auto const found = std::ranges::find(broken_, directory_name, &broken_deck::directory_name);
        found != broken_.end())
        return load_path(found->path);

    return std::unexpected(
        error{
            .code = error_code::not_found,
            .message =
                std::format("no deck directory named '{}' in the deck library", directory_name)
        }
    );
}

std::expected<deck, error> deck_library::load_path(
    std::filesystem::path const& deck_directory
) const
{
    return load_deck(deck_directory, language_);
}

}  // namespace arcana
