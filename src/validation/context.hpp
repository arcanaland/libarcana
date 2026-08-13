// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

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

// a regular file inside a deck root.
struct deck_file
{
    // Deck-root-relative.
    std::filesystem::path relative;
    std::filesystem::path absolute;
};

// What a check reports.
struct finding
{
    std::string message;
    std::optional<std::string> card;
    std::optional<std::filesystem::path> path;
    std::optional<std::string> key;
};

struct deck_facts;

struct check_context
{
    deck const& d;

    // The deck's own tree
    std::span<deck_file const> files;

    // deck.toml as parsed
    toml::table const& doc;

    // The tree and the document, indexed once per deck
    deck_facts const& facts;

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
