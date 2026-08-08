// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>

namespace arcana::data
{

// The license expression grammar of DECK.md section 7.1.

// The outcome of checking a `license` field.
//
// Section 7.1 states two obligations and this reports on both separately: the
// expression must be well-formed, and its identifiers must come from the SPDX
// License List.
struct spdx_expression_check
{
    // False when the expression does not parse. The identifier below is then
    // empty: an expression that does not parse has no identifiers to judge.
    bool well_formed;

    // The first identifier that is not on the list, or empty when every one is.
    // A view into the string that was checked.
    //
    // LicenseRef- and DocumentRef- identifiers are well-formed by construction
    // and are never looked up.
    std::string_view unknown_identifier;
};

// Check `s` against the SPDX license expression grammar and the vendored list.
[[nodiscard]] spdx_expression_check check_spdx_expression(std::string_view s);

}  // namespace arcana::data
