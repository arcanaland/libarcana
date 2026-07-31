// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <toml++/toml.hpp>

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

}  // namespace arcana::detail
