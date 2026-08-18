// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// arcana-bench: the payload hyperfine drives.
//
// Starting and exiting a linked binary in the build container costs ~1.2 ms,
// which is twice what load_deck costs. A benchmark of one call would therefore
// be two-thirds process noise, so this repeats the workload in-process and
// hyperfine measures the whole run. --repeat 1000 puts startup at ~0.2% of it.
//
// For the per-call cost of one function, use the Catch2 front end instead
// (`just bench micro`), which times each iteration in-process.

#include "workloads.hpp"

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace
{

namespace fs = std::filesystem;
using namespace arcana::bench;

int usage(int status)
{
    std::println("usage: arcana-bench <workload> [--deck PATH] [--library PATH] [--repeat N]");
    std::println("       arcana-bench --list");
    std::println("");
    std::println("workloads:");

    for (auto const& w : workloads())
        std::println("  {:<14} {}{}", w.name, w.description, w.wants_library ? "  [library]" : "");

    return status;
}

// The build type, so nobody publishes a number measured on a debug build
constexpr std::string_view build_type =
#ifdef NDEBUG
    "optimized";
#else
    "DEBUG - do not trust these numbers";
#endif

}  // namespace

int main(int argc, char** argv)
{
    std::vector<std::string_view> const args{argv + 1, argv + argc};

    if (args.empty())
        return usage(EXIT_FAILURE);

    if (args.front() == "--list")
    {
        for (auto const& w : workloads()) std::println("{}", w.name);
        return EXIT_SUCCESS;
    }

    if (args.front() == "--help" || args.front() == "-h")
        return usage(EXIT_SUCCESS);

    auto const* const chosen = find_workload(args.front());
    if (chosen == nullptr)
    {
        std::println(stderr, "arcana-bench: no workload named '{}'", args.front());
        return usage(EXIT_FAILURE);
    }

    context ctx;
    std::size_t repeat = 1;
    bool quiet = false;

    for (std::size_t i = 1; i < args.size(); ++i)
    {
        auto const value = [&]() -> std::string_view
        {
            if (i + 1 >= args.size())
            {
                std::println(stderr, "arcana-bench: {} needs a value", args[i]);
                std::exit(EXIT_FAILURE);
            }
            return args[++i];
        };

        if (args[i] == "--deck")
            ctx.deck = fs::path{value()};
        else if (args[i] == "--library")
            ctx.library = fs::path{value()};
        else if (args[i] == "--repeat")
            repeat = std::stoull(std::string{value()});
        else if (args[i] == "--quiet")
            quiet = true;
        else
            return usage(EXIT_FAILURE);
    }

    if (chosen->wants_library ? ctx.library.empty() : ctx.deck.empty())
    {
        std::println(
            stderr, "arcana-bench: workload '{}' needs {}", chosen->name,
            chosen->wants_library ? "--library" : "--deck"
        );
        return EXIT_FAILURE;
    }

    // Summed rather than discarded so the optimizer has to do the work, and so
    // a workload that quietly stopped finding anything reads as a zero instead
    // of as a speed-up
    std::size_t sink = 0;

    auto const start = std::chrono::steady_clock::now();

    try
    {
        for (std::size_t i = 0; i < repeat; ++i) sink += chosen->run(ctx);
    }
    catch (std::exception const& e)
    {
        std::println(stderr, "arcana-bench: {} threw: {}", chosen->name, e.what());
        return EXIT_FAILURE;
    }

    auto const elapsed = std::chrono::steady_clock::now() - start;

    if (sink == 0)
    {
        std::println(stderr, "arcana-bench: '{}' produced nothing - wrong path?", chosen->name);
        return EXIT_FAILURE;
    }

    if (!quiet)
    {
        auto const per_call =
            std::chrono::duration<double, std::micro>(elapsed).count() / double(repeat);

        std::println(
            "{:<14} {:>9.1f} us/call  x{:<6} sink={}  [{}]", chosen->name, per_call, repeat, sink,
            build_type
        );
    }

    return EXIT_SUCCESS;
}
