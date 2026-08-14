// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Checks for the `ids` area.

#pragma once

#include "../context.hpp"

namespace arcana::validation
{

void check_bad_app_realm(check_context const& ctx);
void check_bad_cards_table_key(check_context const& ctx);
void check_bad_custom_name(check_context const& ctx);
void check_bad_deck_identifier(check_context const& ctx);
void check_bad_follows(check_context const& ctx);
void check_bad_signifies(check_context const& ctx);
void check_cards_key_path(check_context const& ctx);
void check_deck_identifier_path_shape(check_context const& ctx);
void check_follows_self(check_context const& ctx);
void check_missing_deck_identifier(check_context const& ctx);
void check_non_canonical_card_reference(check_context const& ctx);
void check_reserved_custom_name(check_context const& ctx);
void check_signifies_self(check_context const& ctx);

}  // namespace arcana::validation
