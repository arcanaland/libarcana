// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/error.hpp>

#include <toml++/toml.hpp>

#include <expected>
#include <filesystem>
#include <memory>

namespace arcana::detail
{

// The parsed deck.toml, retained for the lifetime of the deck it produced
//
// Retained rather than dropped once parsing finishes: keys and sections no parser
// reads -- a field from a future spec version, say -- survive the load, so a writer
// built on this parser does not silently delete what it did not understand.
//
// This is the definition of the type deck.hpp forward-declares, which is what keeps
// toml++ out of the installed headers.
struct deck_document
{
    toml::table table;
};

// Reads and parses <deck_directory>/deck.toml
//
// This is the one place that knows where a deck directory keeps its manifest and what
// makes that manifest a deck at all: a [deck] table has to be there. Both loading a
// whole deck and peeking at one for a summary go through here, so the two cannot drift
// apart on what counts as a readable deck.
//
// Returns parse_error for a file that is missing, malformed, or has no [deck] table.
[[nodiscard]] std::expected<std::shared_ptr<deck_document const>, error> read_deck_document(
    std::filesystem::path const& deck_directory
);

}  // namespace arcana::detail
