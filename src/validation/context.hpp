// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// What a check is handed and what it hands back.

#pragma once

#include <arcana/deck.hpp>
#include <arcana/validation.hpp>

#include <toml++/toml.hpp>

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace arcana::validation
{

// One regular file inside a deck root.
struct deck_file
{
    // Deck-root-relative. This is what a diagnostic's `path` carries.
    std::filesystem::path relative;

    std::filesystem::path absolute;
};

// Every regular file under the deck root, sorted by relative path.
[[nodiscard]] std::vector<deck_file> walk_deck(std::filesystem::path const& root);

// What a check reports.
struct finding
{
    std::string message;
    std::optional<std::string> card;
    std::optional<std::filesystem::path> path;
    std::optional<std::string> key;
};

struct check_context
{
    deck const& d;

    // The deck's own tree
    std::span<deck_file const> files;

    // deck.toml as parsed, which is the only place a key this library's model
    // does not carry can be seen. `deck` is a v1.0-era model: it has no
    // `signifies`, no `[app]`, no `[card_variants]` and no `[editions]`, and
    // its `metadata.id` reads `[deck].id` rather than 2.0's
    // `[deck].identifier`. A document check reads keys, so it reads them here.
    toml::table const& doc;

    // The rule being run.
    rule const& r;

    std::vector<diagnostic>& out;

    void report(finding what) const
    {
        out.push_back(
            diagnostic{
                .level = r.default_level,
                .code = r.code,
                .message = std::move(what.message),
                .card = std::move(what.card),
                .path = std::move(what.path),
                .key = std::move(what.key),
            }
        );
    }
};

using check_fn = void (*)(check_context const&);

}  // namespace arcana::validation
