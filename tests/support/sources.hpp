// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Walking every line the project writes, so a gate can read the whole tree.
//
// Test-only. Nothing in src/ reads its own sources.

#pragma once

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace arcana_test
{

// One line of one walked file.
struct source_line
{
    // Path relative to SOURCE_ROOT
    std::string file;

    // 1-based, as an editor counts
    std::size_t number;

    std::string text;

    // "<file>:<number>", for an INFO
    std::string where() const
    {
        return file + ":" + std::to_string(number);
    }
};

inline std::string read_file(std::string const& path)
{
    std::ifstream in{path, std::ios::binary};

    std::ostringstream contents;
    contents << in.rdbuf();

    return contents.str();
}

// The directories walked, relative to SOURCE_ROOT. The root itself is walked
// too, but only its own files: everything below it that is ours is named here,
// and agents/ and build/ are not ours to gate.
inline std::vector<std::string_view> const& source_roots()
{
    static std::vector<std::string_view> const roots{
        "src", "include", "tests", "bench", "python", "tools", "cmake", "just", ".github"
    };

    return roots;
}

// Whether the walk descends into a directory of this name.
inline bool is_source_directory(std::string const& name)
{
    // A configure leaves empty CMakeFiles/ trees behind in the source dir.
    return !name.starts_with('.') && name != "build" && name != "CMakeFiles";
}

// Whether the walk reads a file of this name. Prose is gated too: a stale
// specification anchor reads the same in a comment, a README and a workflow.
inline bool is_source_file(std::filesystem::path const& name)
{
    static std::vector<std::string_view> const extensions{
        ".cpp", ".hpp", ".md", ".toml", ".cmake", ".py", ".just", ".yml", ".yaml", ".in"
    };

    static std::vector<std::string_view> const names{"CMakeLists.txt", "justfile", "Containerfile"};

    // `std::ranges::contains` would read better, but it is in the tail of C++23
    // that gcc-toolset-14 lacks, and a test is no place to learn that twice.
    auto const holds = [](std::vector<std::string_view> const& all, std::string const& one)
    { return std::ranges::find(all, one) != all.end(); };

    return holds(names, name.string()) || holds(extensions, name.extension().string());
}

// Every line of every walked file below dir, appended to out.
inline void lines_below(
    std::filesystem::path const& dir, bool descend, std::vector<source_line>& out
)
{
    namespace fs = std::filesystem;

    if (!fs::is_directory(dir))
        return;

    for (auto entry = fs::recursive_directory_iterator{dir};
         entry != fs::recursive_directory_iterator{}; ++entry)
    {
        if (entry->is_directory())
        {
            if (!descend || !is_source_directory(entry->path().filename().string()))
                entry.disable_recursion_pending();

            continue;
        }

        if (!entry->is_regular_file() || !is_source_file(entry->path().filename()))
            continue;

        std::string const file = fs::relative(entry->path(), SOURCE_ROOT).string();
        std::istringstream lines{read_file(entry->path().string())};

        std::size_t number = 0;
        for (std::string line; std::getline(lines, line);)
            out.push_back({.file = file, .number = ++number, .text = std::move(line)});
    }
}

// Every line of every source file in the project.
inline std::vector<source_line> source_lines()
{
    namespace fs = std::filesystem;

    std::vector<source_line> found;

    lines_below(fs::path{SOURCE_ROOT}, false, found);

    for (std::string_view const root : source_roots())
        lines_below(fs::path{SOURCE_ROOT} / root, true, found);

    return found;
}

}  // namespace arcana_test
