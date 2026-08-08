// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "registry.hpp"

#include "catalogue.hpp"
#include "context.hpp"

#include <arcana/deck.hpp>
#include <arcana/validation.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace arcana::validation
{

void run_all(
    deck const& d, std::uint8_t major, std::span<deck_file const> files,
    std::vector<diagnostic>& out
)
{
    auto const catalogue = all_rules();

    for (std::size_t i = 0; i < catalogue.size(); ++i)
    {
        rule const& r = catalogue[i];

        // TODO
        if (r.needs == phase::library)
            continue;

        if (!r.applies_to.contains(major))
            continue;

        if (checks[i].run == nullptr)
            continue;

        check_context const ctx{.d = d, .files = files, .r = r, .out = out};
        checks[i].run(ctx);
    }
}

}  // namespace arcana::validation
