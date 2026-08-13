// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "ansi.hpp"

#include "../../data/ascii.hpp"
#include "../probe.hpp"

#include <algorithm>
#include <format>
#include <string_view>

namespace arcana::validation
{

namespace
{

// Tighter than looks_like_ansi_root()
bool is_ansi_root_name(std::string_view name)
{
    if (!name.starts_with("ansi"))
        return false;

    auto const lines = name.substr(4);
    if (lines.empty() || lines.front() == '0')
        return false;

    return std::ranges::all_of(lines, data::is_digit);
}

}  // namespace

void check_ansi_outside_image_root(check_context const& ctx)
{
    for (auto const& file : ctx.files)
    {
        if (is_ansi_root_name(file.relative.begin()->string()))
            continue;

        if (!contains_ansi_escapes(file.absolute))
            continue;

        auto const shown = file.relative.generic_string();
        ctx.report({
            .message =
                std::format("'{}' has ANSI escape codes but is under no ansi<lines>/ root", shown),
            .path = file.relative,
        });
    }
}

}  // namespace arcana::validation
