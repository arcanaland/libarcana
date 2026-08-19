// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

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

constexpr std::string_view build_type =
#ifdef NDEBUG
    "optimized";
#else
    "DEBUG BUILD";
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
        std::println(stderr, "arcana-bench: '{}' wrong path?", chosen->name);
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
