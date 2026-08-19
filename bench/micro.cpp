// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The in-process benchmark front end

#include "workloads.hpp"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace
{

namespace fs = std::filesystem;

fs::path library_root()
{
    if (auto const* const from_env = std::getenv("ARCANA_BENCH_LIBRARY_DIR"))
        return fs::path{from_env};

    return fs::path{BENCH_LIBRARY_DIR};
}

}  // namespace

TEST_CASE("per-deck loader cost", "[.bench][deck]")
{
    auto const decks = arcana::bench::decks_in(library_root());

    if (decks.empty())
        SKIP("no decks under " << library_root().string());

    for (auto const& deck : decks)
    {
        DYNAMIC_SECTION(deck.filename().string())
        {
            arcana::bench::context const ctx{.deck = deck};

            for (auto const& w : arcana::bench::workloads())
            {
                if (w.wants_library)
                    continue;

                BENCHMARK(std::string{w.name})
                {
                    return w.run(ctx);
                };
            }
        }
    }
}

TEST_CASE("whole-library cost", "[.bench][library]")
{
    arcana::bench::context const ctx{.library = library_root()};

    if (arcana::bench::decks_in(ctx.library).empty())
        SKIP("no decks under " << ctx.library.string());

    for (auto const& w : arcana::bench::workloads())
    {
        if (!w.wants_library)
            continue;

        BENCHMARK(std::string{w.name})
        {
            return w.run(ctx);
        };
    }
}
