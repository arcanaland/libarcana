// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Which directories hold decks. Reading what is in one of them belongs to src/loader,
// which is why nothing here needs toml++.

#include "library_state.hpp"

#include <arcana/library.hpp>
#include <arcana/loader.hpp>
#include <arcana/paths.hpp>

#include <manifest.hpp>
#include <summary.hpp>

#include <algorithm>
#include <format>
#include <memory>
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

namespace
{

// The scan a snapshot is built out of
std::shared_ptr<detail::library_snapshot const> scan(
    std::vector<std::filesystem::path> roots, std::optional<std::filesystem::path> reference_path,
    std::vector<std::string> languages
)
{
    detail::library_snapshot next{
        .roots = std::move(roots),
        .reference_path = std::move(reference_path),
        .languages = std::move(languages)
    };

    for (auto const& dir : scan_deck_directories(next.roots))
    {
        auto summary = detail::load_deck_summary(dir);

        if (summary)
        {
            next.decks.push_back(*std::move(summary));
            continue;
        }

        next.malformed.push_back(
            malformed_deck{
                .directory_name = dir.filename().string(),
                .path = dir,
                .problem = std::move(summary.error())
            }
        );
    }

    std::ranges::sort(next.decks, {}, &deck_summary::directory_name);
    std::ranges::sort(next.malformed, {}, &malformed_deck::directory_name);

    if (next.reference_path)
        if (auto summary = detail::load_deck_summary(*next.reference_path))
            next.reference = *std::move(summary);

    return std::make_shared<detail::library_snapshot const>(std::move(next));
}

}  // namespace

deck_library::deck_library(library_options options)
    : state_{scan(
          add_default_root(std::move(options.roots)), std::move(options.reference_deck),
          std::move(options.languages)
      )},
      cache_{std::make_shared<detail::deck_cache>()}
{
}

void deck_library::refresh()
{
    // TODO: we probably shouldn't grow this unbounded...
    retired_.push_back(state_);

    state_ = scan(state_->roots, state_->reference_path, state_->languages);
    cache_->loaded.clear();
}

std::span<deck_summary const> deck_library::decks() const noexcept
{
    return state_->decks;
}

std::span<malformed_deck const> deck_library::malformed_decks() const noexcept
{
    return state_->malformed;
}

std::optional<deck_summary> const& deck_library::reference() const noexcept
{
    return state_->reference;
}

std::span<std::filesystem::path const> deck_library::roots() const noexcept
{
    return state_->roots;
}

std::optional<std::filesystem::path> const& deck_library::reference_path() const noexcept
{
    return state_->reference_path;
}

std::span<std::string const> deck_library::languages() const noexcept
{
    return state_->languages;
}

std::optional<deck_summary> deck_library::find(std::string_view directory_name) const
{
    auto const found =
        std::ranges::find(state_->decks, directory_name, &deck_summary::directory_name);
    if (found == state_->decks.end())
        return std::nullopt;

    return *found;
}

std::vector<deck_summary> deck_library::find_all_by_identifier(std::string_view identifier) const
{
    auto matches = state_->decks | std::views::filter([identifier](deck_summary const& summary)
                                                      { return summary.identifier == identifier; });

    return {matches.begin(), matches.end()};
}

std::expected<deck, error> deck_library::load(std::string_view directory_name) const
{
    if (auto const found =
            std::ranges::find(state_->decks, directory_name, &deck_summary::directory_name);
        found != state_->decks.end())
        return load_cached(found->path);

    // useful to return details about a busted deck in the library
    if (auto const found =
            std::ranges::find(state_->malformed, directory_name, &malformed_deck::directory_name);
        found != state_->malformed.end())
        return load_cached(found->path);

    return std::unexpected(
        error{
            .code = error_code::not_found,
            .message =
                std::format("no deck directory named '{}' in the deck library", directory_name)
        }
    );
}

std::expected<deck, error> deck_library::load_external(
    std::filesystem::path const& deck_directory
) const
{
    return load_cached(deck_directory);
}

std::expected<deck, error> deck_library::load_reference() const
{
    if (!state_->reference_path)
        return std::unexpected(
            error{
                .code = error_code::not_found,
                .message = "this library has no reference deck configured"
            }
        );

    return load_cached(*state_->reference_path);
}

std::expected<deck, error> deck_library::load_cached(
    std::filesystem::path const& deck_directory
) const
{
    auto key = deck_directory.lexically_normal().string();

    if (auto const cached = cache_->loaded.find(key); cached != cache_->loaded.end())
        return cached->second;

    auto loaded = load_deck(deck_directory, state_->languages);
    if (!loaded)
        return std::unexpected(std::move(loaded).error());

    // Failures stay uncached
    return cache_->loaded.emplace(std::move(key), *std::move(loaded)).first->second;
}

}  // namespace arcana
