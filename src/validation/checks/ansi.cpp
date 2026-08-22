// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "ansi.hpp"

#include "../../data/asset_grammar.hpp"
#include "../probe.hpp"

#include <arcana/card.hpp>

#include <format>
#include <string_view>

namespace arcana::validation
{

namespace
{

// Whether a top-level directory name is an ansi<lines>/ root (DECK.md 5.7.1)
bool is_ansi_root_name(std::string_view name)
{
    auto const root = data::parse_image_root(name);

    return root && root->kind == image_kind::ansi;
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
