// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace arcana_test
{

// A reference to the spec written in a source file
struct spec_reference
{
    // DECK.md or README.md for v1
    std::string file;

    // GitHub anchor without leading #
    std::string anchor;

    // <file>:<line>
    std::string where;
};

inline bool is_anchor_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
}

inline bool is_document_char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
           c == '_';
}

// Every specification reference in line, appended to out.
inline void references_in(
    std::string_view line, std::string const& where, std::vector<spec_reference>& out
)
{
    std::string const needle = std::string{".md"} + "#";

    for (std::size_t at = line.find(needle); at != std::string_view::npos;
         at = line.find(needle, at + 1))
    {
        std::size_t start = at;
        while (start > 0 && is_document_char(line[start - 1])) --start;

        std::size_t const anchor_at = at + needle.size();
        std::size_t end = anchor_at;
        while (end < line.size() && is_anchor_char(line[end])) ++end;

        if (start == at || end == anchor_at)
            continue;

        out.push_back({
            .file = std::string{line.substr(start, at - start)} + ".md",
            .anchor = std::string{line.substr(anchor_at, end - anchor_at)},
            .where = where,
        });
    }
}

// Whether line has a well-formed reference.
inline bool has_reference(std::string_view line)
{
    std::vector<spec_reference> found;
    references_in(line, "", found);

    return !found.empty();
}

}  // namespace arcana_test
