// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "assets.hpp"
#include "context.hpp"

#include <toml++/toml.hpp>

#include <cstdint>
#include <filesystem>
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

// Where a name the deck creator coined was written.
enum class name_site : std::uint8_t
{
    suit,
    rank,

    // A card back design key, a variant suffix, a default_variant or a custom
    // major arcanum
    other,
};

// One key the deck creator coined
struct coined_name
{
    std::string name;
    name_site site;
    std::optional<std::string> key;
    std::optional<std::filesystem::path> path;
};

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
    deck_facts(std::span<deck_file const> files, toml::table const& doc, std::uint8_t major);

    // Deck-relative path -> file.
    std::map<std::string, deck_file const*, std::less<>> by_path;

    // (directory, stem) -> files
    std::map<stem_key, std::vector<deck_file const*>> by_stem;

    // (directory, stem) -> files with the stem case-folded
    std::map<stem_key, std::vector<deck_file const*>> by_folded_stem;

    // Card back design key -> the image path the document declares for it.
    std::map<std::string, std::string, std::less<>> back_images;

    // Files under a card back directory
    std::vector<back_file> back_files;

    // Every card back design in the deck
    std::vector<std::string> designs;

    // Every path that deck.toml declares
    std::vector<std::string> declared;

    // Every name the deck creator coined, declared or discovered
    std::vector<coined_name> coined;

    // The file at a deck-relative path or nothing where the deck has none.
    [[nodiscard]] deck_file const* file_at(std::string_view relative) const;

    // True if in the manifest or discovery would find it
    [[nodiscard]] bool reachable(deck_file const& file, root_kind wanted) const;
};


}  // namespace arcana::validation
