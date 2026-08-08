// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "ansi.hpp"

#include "../../data/ascii.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <string_view>

namespace arcana::validation
{

namespace
{

// Tighter than looks_like_ansi_root()
bool is_ansi_root_name(std::string_view name)
{
    if (!name.starts_with("ansi"))
        return false;

    auto const lines = name.substr(4);
    if (lines.empty() || lines.front() == '0')
        return false;

    return std::ranges::all_of(lines, data::is_digit);
}

// How much of a file is read before giving up on finding an ESC.
constexpr std::size_t ansi_sniff_bytes = 64UL * 1024UL;

// How much is read at a time
constexpr std::size_t ansi_sniff_chunk = 4096;

bool contains_ansi_escapes(std::filesystem::path const& file)
{
    std::ifstream stream{file, std::ios::binary};
    if (!stream)
        return false;

    std::array<char, ansi_sniff_chunk> buffer{};
    for (std::size_t seen = 0; seen < ansi_sniff_bytes;)
    {
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        auto const got = static_cast<std::size_t>(stream.gcount());
        if (got == 0)
            break;

        seen += got;

        auto const chunk = std::string_view(buffer.data(), got);
        auto const stop = chunk.find_first_of(std::string_view("\0\x1b", 2));
        if (stop != std::string_view::npos)
            return chunk[stop] == '\x1b';
    }

    return false;
}

}  // namespace

void check_ansi_outside_image_root(check_context const& ctx)
{
    for (auto const& file : ctx.files)
    {
        if (is_ansi_root_name(file.relative.begin()->string()))
            continue;

        if (!contains_ansi_escapes(file.absolute))
            continue;

        auto const shown = file.relative.generic_string();
        ctx.report({
            .message =
                std::format("'{}' has ANSI escape codes but is under no ansi<lines>/ root", shown),
            .path = file.relative,
        });
    }
}

}  // namespace arcana::validation
