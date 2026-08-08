// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The public face of deck validation. The catalogue, the harness and the checks
// themselves are in validation/; this file is only the three entry points
// include/arcana/validation.hpp declares.

#include <arcana/validation.hpp>

#include "validation/catalogue.hpp"
#include "validation/context.hpp"
#include "validation/registry.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>
#include <tuple>
#include <vector>

namespace arcana
{

namespace
{

constexpr std::uint8_t current_schema_major = 2;

}  // namespace

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
    auto const major = schema_major(d.metadata).value_or(current_schema_major);

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
