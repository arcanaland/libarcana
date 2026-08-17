// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "probe.hpp"

#include "../data/image_signature.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <span>
#include <string_view>

namespace arcana::validation
{

namespace
{

// How much of a file is read before giving up on finding an ESC.
constexpr std::size_t ansi_sniff_bytes = 64UL * 1024UL;

// How much is read at a time
constexpr std::size_t ansi_sniff_chunk = 4096;

}  // namespace

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

bool is_baseline_image_format(std::filesystem::path const& file)
{
    std::ifstream stream{file, std::ios::binary};
    if (!stream)
        return false;

    std::array<std::byte, data::image_signature_bytes> head{};
    stream.read(reinterpret_cast<char*>(head.data()), head.size());  // NOLINT(*-reinterpret-cast)

    auto const got = static_cast<std::size_t>(stream.gcount());

    return data::sniff_image_format(std::span{head}.first(got)) != data::image_format::unknown;
}

}  // namespace arcana::validation
