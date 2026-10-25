// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Checks for the `cards` area.

#pragma once

#include <context.hpp>

namespace arcana::validation
{

void check_declared_card_without_image(check_context const& ctx);

void check_duplicate_card_position(check_context const& ctx);

void check_duplicate_rank_in_ranks(check_context const& ctx);

void check_empty_card_number(check_context const& ctx);

void check_excluded_card_also_declared(check_context const& ctx);

void check_excluded_card_has_image(check_context const& ctx);

void check_ignored_key_on_variant(check_context const& ctx);

void check_missing_variant_image(check_context const& ctx);

void check_position_on_minor_arcanum(check_context const& ctx);

void check_rank_without_image(check_context const& ctx);

void check_unknown_variant_default(check_context const& ctx);

void check_variant_card_without_default(check_context const& ctx);

}  // namespace arcana::validation
