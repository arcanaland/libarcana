// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "document.hpp"

#include <arcana/deck.hpp>

#include <sstream>
#include <string>
#include <utility>

namespace arcana
{

// deck::source_toml() is the one query that reaches into the retained document, so it
// is defined here beside deck_document rather than in deck.cpp with the rest of the
// deck API -- that is what lets deck.cpp compile without toml++.
std::string deck::source_toml() const
{
    if (!document_)
        return {};

    std::ostringstream out;
    out << document_->table;
    return std::move(out).str();
}

}  // namespace arcana
