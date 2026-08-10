// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Checks for the `backs` area.

#pragma once

#include "../context.hpp"

namespace arcana::validation
{

void check_bad_card_back_design_key(check_context const& ctx);

void check_card_back_default_by_collation(check_context const& ctx);

void check_card_back_not_baseline_format(check_context const& ctx);

void check_ignored_card_back_file(check_context const& ctx);

void check_missing_card_back_image(check_context const& ctx);

void check_unknown_default_card_back(check_context const& ctx);

void check_unknown_edition_card_back(check_context const& ctx);

}  // namespace arcana::validation
