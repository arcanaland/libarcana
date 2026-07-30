// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/card.hpp>
#include <arcana/error.hpp>

#include <expected>
#include <string_view>

namespace arcana::detail
{

// Deck-free structural parse of a canonical id. Private: every consumer call site that
// looked like this one wanted deck::find_card instead, so the rules this enforces are
// documented there, on the public surface that applies them. See TASK-008's update log for
// the consumer survey that demoted this.
//
// Kept out of include/arcana so it can gain a deck-aware overload, or lose the
// standard/custom disambiguation entirely, without an API break.
std::expected<card_id, error> parse_card_id(std::string_view canonical_id);

}  // namespace arcana::detail
