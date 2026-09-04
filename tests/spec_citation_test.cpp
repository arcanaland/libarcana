// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "sources.hpp"
#include "spec_reference.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using arcana_test::has_reference;
using arcana_test::source_line;
using arcana_test::source_lines;

namespace
{

bool is_word_char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' ||
           c == '-';
}

char lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

std::size_t find_insensitive(std::string_view line, std::string_view word, std::size_t from = 0)
{
    if (word.size() > line.size())
        return std::string_view::npos;

    for (std::size_t at = from; at + word.size() <= line.size(); ++at)
    {
        std::size_t i = 0;
        while (i < word.size() && lower(line[at + i]) == lower(word[i])) ++i;

        if (i == word.size())
            return at;
    }

    return std::string_view::npos;
}

// Whether word occurs in line as a word rather than inside an identifier
bool mentions_word(std::string_view line, std::string_view word)
{
    for (std::size_t at = find_insensitive(line, word); at != std::string_view::npos;
         at = find_insensitive(line, word, at + 1))
    {
        bool const before = at > 0 && is_word_char(line[at - 1]);
        std::size_t const end = at + word.size();
        bool const after = end < line.size() && is_word_char(line[end]);

        if (!before && !after)
            return true;
    }

    return false;
}

// Whether the line is talking about the deck spec at all
bool mentions_specification(std::string_view line)
{
    for (std::string_view const word : {"spec", "specs", "specification", "specifications"})
        if (mentions_word(line, word))
            return true;

    for (std::string_view const name : {"deck.md", "readme.md", "deck-spec"})
        if (find_insensitive(line, name) != std::string_view::npos)
            return true;

    return false;
}

// two dot-separated runs of digits that begin a word of prose
bool names_a_section_number(std::string_view line)
{
    for (std::size_t at = 0; at < line.size(); ++at)
    {
        if (!is_digit(line[at]))
            continue;

        char const before = at > 0 ? line[at - 1] : ' ';
        bool const opens =
            !is_word_char(before) && before != '.' && before != '"' && before != '\'';

        std::size_t end = at;
        while (end < line.size() && is_digit(line[end])) ++end;

        bool const numbered = end + 1 < line.size() && line[end] == '.' && is_digit(line[end + 1]);

        if (opens && numbered)
            return true;

        at = end;
    }

    return false;
}

std::optional<std::string> style_violation(std::string_view line)
{
    if (find_insensitive(line, "§") != std::string_view::npos)
        return "a section sign";

    if (mentions_specification(line) && names_a_section_number(line) && !has_reference(line))
        return "a specification section named by number";

    return std::nullopt;
}

}  // namespace

TEST_CASE("the citation gate fires", "[spec]")
{
    CHECK(style_violation("// per §5.3 a name file may be absent").has_value());
    CHECK(style_violation("// the spec's 10.4 rule table lists it").has_value());
    CHECK(style_violation("// See DECK.md 5.3").has_value());
    CHECK(style_violation("# The specification requires this in 4.1.2.").has_value());
}

TEST_CASE("the citation gate leaves everything else alone", "[spec]")
{
    // The shape it is asking for
    CHECK_FALSE(style_violation("// DECK.md#104-validation-rules lists this rule").has_value());

    // Another document's sections are not this gate's business
    CHECK_FALSE(style_violation("    // CSS Color 4 section 6.1.").has_value());
    CHECK_FALSE(style_violation("// RFC 5646 2.2.9 bounds the subtag").has_value());

    // A version is not a section
    CHECK_FALSE(
        style_violation("// the v2.0 specification dispatches on schema_version").has_value()
    );
    CHECK_FALSE(style_violation("schema_version = \"2.0\"  # the spec's major").has_value());

    // Nor is a word inside an identifier a mention
    CHECK_FALSE(style_violation("    \"Community-Spec-1.0\",").has_value());
    CHECK_FALSE(
        style_violation("set(ARCANA_SPECIFICATION_V2_TAG 92a2c34 CACHE STRING \"4.3\")").has_value()
    );
    CHECK_FALSE(
        style_violation("    CHECK(spec_url(one) == base + \"/DECK.md#5-ordering\");").has_value()
    );
}

TEST_CASE("no source file names a specification section by number", "[spec]")
{
    auto const lines = source_lines();

    REQUIRE_FALSE(lines.empty());

    for (source_line const& line : lines)
    {
        // Google: recursion
        // (mind-blown emoji)
        // ~~ WOOAHH ~~
        if (line.file == "tests/spec_citation_test.cpp")
            continue;

        if (auto const problem = style_violation(line.text))
            FAIL_CHECK(
                line.where() << " seems like it has a spec quote bruh"
            );
    }
}
