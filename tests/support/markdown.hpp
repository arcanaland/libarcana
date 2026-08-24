// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Reading ATX headings out of a Markdown file and slugifying them the way
// GitHub does, so a citation's anchor can be checked against the text it cites.
//
// Test-only. Nothing in src/ reads the specification.

#pragma once

#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace arcana_test
{

struct heading
{
    // How many leading '#' the heading carried.
    std::size_t level;

    // The heading text, trimmed, exactly as the document writes it.
    std::string text;

    // The GitHub anchor the text produces, without the leading '#'.
    std::string slug;
};

// Lowercase, drop everything outside [a-z0-9 -_], spaces to hyphens.
//
// This is GitHub's rule with one omission: a repeated slug gets a "-1", "-2"
// suffix there and nothing here. The caller asserts no two headings collide
// instead, which is the honest thing to do while no collision exists.
inline std::string slugify(std::string_view text)
{
    std::string slug;
    slug.reserve(text.size());

    for (char const c : text)
        if (c >= 'A' && c <= 'Z')
            slug.push_back(static_cast<char>(c - 'A' + 'a'));
        else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_')
            slug.push_back(c);
        else if (c == ' ')
            slug.push_back('-');

    return slug;
}

// Every ATX heading in `text`, skipping fenced code blocks.
//
// A '#' opening a comment inside a ``` fence is not a heading, and the
// specification's TOML examples are full of them.
inline std::vector<heading> headings_of(std::string_view text)
{
    std::vector<heading> found;

    bool fenced = false;
    std::istringstream lines{std::string{text}};
    for (std::string line; std::getline(lines, line);)
    {
        // A trailing \r would ride along into the slug.
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

        std::size_t const start = line.find_first_not_of(' ');
        if (start == std::string::npos)
            continue;

        if (line.compare(start, 3, "```") == 0)
        {
            fenced = !fenced;
            continue;
        }

        if (fenced || line[start] != '#')
            continue;

        std::size_t const hashes = line.find_first_not_of('#', start);
        if (hashes == std::string::npos || line[hashes] != ' ')
            continue;

        std::size_t const level = hashes - start;
        if (level > 6)
            continue;

        std::string const heading_text = line.substr(line.find_first_not_of(' ', hashes));
        found.push_back({.level = level, .text = heading_text, .slug = slugify(heading_text)});
    }

    return found;
}

// The whole file, or an empty string where it cannot be opened.
inline std::string read_file(std::string const& path)
{
    std::ifstream in{path, std::ios::binary};

    std::ostringstream contents;
    contents << in.rdbuf();

    return contents.str();
}

}  // namespace arcana_test
