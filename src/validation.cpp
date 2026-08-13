// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The public face of deck validation. The catalogue, the harness and the checks
// themselves are in validation/; this file is only the three entry points
// include/arcana/validation.hpp declares.

#include <arcana/validation.hpp>

#include "validation/catalogue.hpp"
#include "validation/context.hpp"
#include "validation/registry.hpp"
#include "validation/spec.hpp"
#include "validation/tree.hpp"

#include <algorithm>
#include <span>
#include <string_view>
#include <tuple>
#include <vector>

namespace arcana
{

std::span<rule const> rules() noexcept
{
    return validation::all_rules();
}

rule const* find_rule(std::string_view code) noexcept
{
    return validation::lookup(code);
}

std::vector<diagnostic> validate(deck const& d)
{
    auto const major = validation::major_of(d);

    auto const files = validation::walk_deck(d.root_path);

    std::vector<diagnostic> found;
    validation::run_all(d, major, files, found);

    // Ascending by (code, card, path, key)
    std::ranges::sort(
        found,
        [](diagnostic const& left, diagnostic const& right)
        {
            return std::tie(left.code, left.card, left.path, left.key, left.message) <
                   std::tie(right.code, right.card, right.path, right.key, right.message);
        }
    );

    return found;
}

}  // namespace arcana
