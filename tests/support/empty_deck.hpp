// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <deck_state.hpp>

#include <arcana/deck.hpp>

namespace arcana::testing
{

// A deck that loaded nothing
//
// deck has no default constructor -- there is no empty deck in the public API
// -- so the edge cases that ask what an empty one answers reach for the
// internal state the loaders fill in.
[[nodiscard]] inline deck empty_deck()
{
    return detail::deck_builder::make(detail::deck_state{});
}

}  // namespace arcana::testing
