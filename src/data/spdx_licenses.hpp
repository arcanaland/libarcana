// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>

namespace arcana::data
{

// Membership in the vendored SPDX License List.
//
// DECK.md section 7.1 requires that the identifiers in a deck's `license`
// expression come from the list, and says they are case-sensitive, so both
// lookups compare bytewise.
//
// The list is pinned at generation time. See src/data/spdx_licenses.cpp for
// which release, and tools/generate_spdx_data.py for how to move the pin. There
// is deliberately no way to ask which release this is: that would be new public
// API, and the validation surface is frozen.

// True when `id` is a licence identifier on the list, deprecated ones included.
[[nodiscard]] bool is_spdx_license_id(std::string_view id) noexcept;

// True when `id` is an exception identifier, the operand of a WITH.
[[nodiscard]] bool is_spdx_exception_id(std::string_view id) noexcept;

}  // namespace arcana::data
