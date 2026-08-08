// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>

namespace arcana::data
{

// The outcome of checking a `license` field.
struct spdx_expression_check
{
    // False when the expression does not parse
    bool well_formed;

    // The first identifier that is not on the list, or empty when every one is.
    std::string_view unknown_identifier;
};

// Check `s` against the SPDX license expression grammar and the vendored list.
[[nodiscard]] spdx_expression_check check_spdx_expression(std::string_view s);

}  // namespace arcana::data
