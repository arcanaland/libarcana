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
#include <ranges>
#include <span>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>

namespace arcana
{

namespace
{

std::vector<std::filesystem::path> add_default_root(std::vector<std::filesystem::path>&& roots)
{
    if (roots.empty())
        roots.push_back(paths::deck_library_path());

    return roots;
}

// Return a vector of directories that contain deck manifest
//
// Earlier roots shadow later ones by directory name, like PATH
std::vector<std::filesystem::path> scan_deck_directories(
    std::span<std::filesystem::path const> roots
)
{
    std::vector<std::filesystem::path> directories;
    std::unordered_set<std::string> claimed;

    for (auto const& root : roots)
    {
        std::error_code ec;
        std::filesystem::directory_iterator entries{root, ec};

        for (auto const end = std::filesystem::directory_iterator{}; !ec && entries != end;
             entries.increment(ec))
        {
            auto candidate = entries->path();
            if (!std::filesystem::is_regular_file(candidate / detail::deck_manifest_filename, ec))
            {
                ec.clear();
                continue;
            }

            if (claimed.insert(candidate.filename().string()).second)
                directories.push_back(std::move(candidate));
        }
    }

    return directories;
}

}  // namespace

deck_library::deck_library(library_options options)
    : roots_(add_default_root(std::move(options.roots))),
      reference_path_(std::move(options.reference_deck)),
      languages_(std::move(options.languages))
{
    refresh();
}

void deck_library::refresh()
{
    decks_.clear();
    malformed_.clear();
    reference_.reset();
    loaded_.clear();

    for (auto const& dir : scan_deck_directories(roots_))
    {
        auto summary = detail::load_deck_summary(dir);

        if (summary)
        {
            decks_.push_back(*std::move(summary));
            continue;
        }

        malformed_.push_back(
            malformed_deck{
                .directory_name = dir.filename().string(),
                .path = dir,
                .problem = std::move(summary.error())
            }
        );
    }

    std::ranges::sort(decks_, {}, &deck_summary::directory_name);
    std::ranges::sort(malformed_, {}, &malformed_deck::directory_name);

    if (reference_path_)
        if (auto summary = detail::load_deck_summary(*reference_path_))
            reference_ = *std::move(summary);
}

std::optional<deck_summary> deck_library::find(std::string_view directory_name) const
{
    auto const found = std::ranges::find(decks_, directory_name, &deck_summary::directory_name);
    if (found == decks_.end())
        return std::nullopt;

    return *found;
}

std::vector<deck_summary> deck_library::find_all_by_identifier(std::string_view identifier) const
{
    auto matches = decks_ | std::views::filter([identifier](deck_summary const& summary)
                                               { return summary.identifier == identifier; });

    return {matches.begin(), matches.end()};
}

std::expected<std::shared_ptr<deck const>, error> deck_library::load(
    std::string_view directory_name
) const
{
    if (auto const found = std::ranges::find(decks_, directory_name, &deck_summary::directory_name);
        found != decks_.end())
        return load_cached(found->path);

    // useful to return details about a busted deck in the library
    if (auto const found =
            std::ranges::find(malformed_, directory_name, &malformed_deck::directory_name);
        found != malformed_.end())
        return load_cached(found->path);

    return std::unexpected(
        error{
            .code = error_code::not_found,
            .message =
                std::format("no deck directory named '{}' in the deck library", directory_name)
        }
    );
}

std::expected<std::shared_ptr<deck const>, error> deck_library::load_external(
    std::filesystem::path const& deck_directory
) const
{
    return load_cached(deck_directory);
}

std::expected<std::shared_ptr<deck const>, error> deck_library::load_reference() const
{
    if (!reference_path_)
        return std::unexpected(
            error{
                .code = error_code::not_found,
                .message = "this library has no reference deck configured"
            }
        );

    return load_cached(*reference_path_);
}

std::expected<std::shared_ptr<deck const>, error> deck_library::load_cached(
    std::filesystem::path const& deck_directory
) const
{
    auto key = deck_directory.lexically_normal().string();

    if (auto const cached = loaded_.find(key); cached != loaded_.end())
        return cached->second;

    auto loaded = load_deck(deck_directory, languages_);
    if (!loaded)
        return std::unexpected(std::move(loaded).error());

    // Failures stay uncached
    auto shared = std::make_shared<deck const>(*std::move(loaded));
    loaded_.emplace(std::move(key), shared);

    return shared;
}

}  // namespace arcana
