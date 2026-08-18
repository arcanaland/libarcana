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
struct context
{
    std::filesystem::path deck;
    std::filesystem::path library;
};

// One measurable unit of loader work
struct workload
{
    std::string_view name;
    std::string_view description;

    bool wants_library = false;

    std::function<std::size_t(context const&)> run;
};

[[nodiscard]] std::span<workload const> workloads();

[[nodiscard]] workload const* find_workload(std::string_view name);

// Every deck directory directly under library, sorted by name
[[nodiscard]] std::vector<std::filesystem::path> decks_in(std::filesystem::path const& library);

}  // namespace arcana::bench
