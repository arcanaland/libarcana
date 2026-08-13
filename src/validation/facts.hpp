// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// What the checks need to know about a deck, derived once. Reads the file tree
// and the parsed document; touches no file contents, which is probe.hpp's job.

#pragma once

#include "assets.hpp"
#include "context.hpp"

#include <toml++/toml.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace arcana::validation
{

// A directory and a stem
using stem_key = std::pair<std::string, std::string>;

// One file sitting in a card back directory.
struct back_file
{
    deck_file const* file;
    std::string stem;
    std::optional<root_kind> kind;

    // True where discovery passes the file over
    bool ignored;
};

struct deck_facts
{
    // Deck-relative path -> file.
    std::map<std::string, deck_file const*, std::less<>> by_path;

    // (directory, stem) -> files, in walk order.
    std::map<stem_key, std::vector<deck_file const*>> by_stem;

    // The same, with the stem case-folded. Kept apart from `by_stem` so that
    // folding stays a decision a check makes rather than a global one.
    std::map<stem_key, std::vector<deck_file const*>> by_folded_stem;

    // Card back design key -> the image path the document declares for it.
    std::map<std::string, std::string, std::less<>> back_images;

    // Files sitting under a card back directory, classified.
    std::vector<back_file> back_files;

    // Every card back design this deck has, declared or discovered, sorted.
    std::vector<std::string> designs;

    // Every path deck.toml names outright, sorted.
    std::vector<std::string> declared;

    // The file at a deck-relative path, or nothing where the deck has none.
    [[nodiscard]] deck_file const* file_at(std::string_view relative) const;

    // True where the file is in the manifest, or discovery of this kind of root
    // would pick it up.
    [[nodiscard]] bool reachable(deck_file const& file, root_kind wanted) const;
};

[[nodiscard]] deck_facts derive(
    std::span<deck_file const> files, toml::table const& doc, std::uint8_t major
);

}  // namespace arcana::validation
