// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The in-process front end: per-call cost with Catch2's own timing.
//
// This is the one to reach for when the question is "which function costs
// what". Catch2 estimates the clock's resolution, warms up, times each
// iteration and reports mean/median/stddev with outlier analysis, none of which
// is contaminated by process startup. Use hyperfine over `arcana-bench` when
// the question is instead "is this build faster than that build".
//
// Every case is tagged [.bench] - the leading dot is Catch2's hidden tag, so
// `just test` never runs these. `just bench micro` asks for them by name.

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

// Where the decks are, overridable so this can be pointed at any library
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
