// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

namespace arcana::bench
{

// What a workload runs against
//
// A workload uses one or the other, never both: `deck` names a single deck
// directory and `library` a directory holding many.
struct context
{
    std::filesystem::path deck;
    std::filesystem::path library;
};

// One measurable unit of loader work
//
// `run` returns a value derived from the result so that neither front end can
// have the call optimized away, and so a workload that silently stopped doing
// anything shows up as a zero rather than as a speed-up.
struct workload
{
    std::string_view name;
    std::string_view description;

    // Whether this one measures a library rather than a single deck
    bool wants_library = false;

    std::function<std::size_t(context const&)> run;
};

[[nodiscard]] std::span<workload const> workloads();

// nullptr when no workload carries that name
[[nodiscard]] workload const* find_workload(std::string_view name);

// Every deck directory directly under `library`, sorted by name
[[nodiscard]] std::vector<std::filesystem::path> decks_in(std::filesystem::path const& library);

}  // namespace arcana::bench
