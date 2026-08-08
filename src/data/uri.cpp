// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "uri.hpp"

#include "ascii.hpp"

#include <algorithm>
#include <string_view>

namespace arcana::data
{

bool is_absolute_http_url(std::string_view s) noexcept
{
    // RFC 3986 reserves the scheme case-insensitively.
    constexpr std::string_view plain{"http://"};
    constexpr std::string_view secure{"https://"};

    std::string_view rest = s;
    if (equal_ignoring_case(s.substr(0, plain.size()), plain))
        rest.remove_prefix(plain.size());
    else if (equal_ignoring_case(s.substr(0, secure.size()), secure))
        rest.remove_prefix(secure.size());
    else
        return false;

    // A URL with no authority has no host to reach.
    if (rest.empty() || rest.front() == '/' || rest.front() == '?' || rest.front() == '#')
        return false;

    // The characters RFC 3986 excludes outright, plus everything outside
    // printable ASCII. Nothing here is fetched, so this goes no further.
    return std::ranges::all_of(
        s,
        [](char c)
        {
            constexpr std::string_view excluded{"<>\"{}|\\^`"};
            constexpr unsigned char del = 0x7F;

            auto const byte = static_cast<unsigned char>(c);
            return byte > ' ' && byte < del && !excluded.contains(c);
        }
    );
}

}  // namespace arcana::data
