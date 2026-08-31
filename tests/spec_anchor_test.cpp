// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "markdown.hpp"

#include <arcana/validation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

using arcana::rules;
using arcana::spec_revision;
using arcana::spec_section;
using arcana::spec_url;

using arcana_test::heading;
using arcana_test::headings_of;
using arcana_test::read_file;
using arcana_test::slugify;

namespace
{

// The pinned text of one schema major, or a skip where the fetch is off.
std::vector<heading> pinned_headings(std::string const& path)
{
    if (path.empty())
        SKIP("configured with ARCANA_FETCH_SPECIFICATION=OFF");

    std::string const text = read_file(path);
    if (text.empty())
        FAIL("cannot read the pinned specification at " + path);

    return headings_of(text);
}

bool has_slug(std::vector<heading> const& headings, std::string_view slug)
{
    return std::ranges::any_of(headings, [slug](heading const& h) { return h.slug == slug; });
}

// A reference to the specification written in a source file
struct source_reference
{
    std::string file;
    std::string anchor;
    std::string where;
};

bool is_anchor_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
}

bool is_file_char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
           c == '_';
}

// Every spec reference in the code (<file>.md#<anchor>)
void references_in(
    std::string_view line, std::string const& where, std::vector<source_reference>& out
)
{
    std::string const needle = std::string{".md"} + "#";

    for (std::size_t at = line.find(needle); at != std::string_view::npos;
         at = line.find(needle, at + 1))
    {
        std::size_t start = at;
        while (start > 0 && is_file_char(line[start - 1])) --start;

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

// Every reference to the spec in the code
std::vector<source_reference> references_in_sources()
{
    namespace fs = std::filesystem;

    std::vector<source_reference> found;

    for (std::string_view const root : {"src", "include", "tests", "bench", "python"})
    {
        fs::path const dir = fs::path{SOURCE_ROOT} / root;
        if (!fs::is_directory(dir))
            continue;

        for (auto const& entry : fs::recursive_directory_iterator{dir})
        {
            if (!entry.is_regular_file())
                continue;

            auto const extension = entry.path().extension();
            if (extension != ".cpp" && extension != ".hpp")
                continue;

            std::string const text = read_file(entry.path().string());
            std::istringstream lines{text};

            std::size_t number = 0;
            for (std::string line; std::getline(lines, line);)
            {
                ++number;
                references_in(
                    line,
                    fs::relative(entry.path(), SOURCE_ROOT).string() + ":" + std::to_string(number),
                    found
                );
            }
        }
    }

    return found;
}

}  // namespace

TEST_CASE("slugify follows GitHub's anchor rule", "[spec]")
{
    CHECK(slugify("6.7.4 The Extension Chain") == "674-the-extension-chain");
    CHECK(slugify("10.4 Validation Rules") == "104-validation-rules");

    // Dots vanish; the underscore of a TOML table name does not.
    CHECK(slugify("4. deck.toml Reference") == "4-decktoml-reference");
    CHECK(slugify("4.2 `[card_backs]`") == "42-card_backs");

    // Nor does the hyphen of a hyphenated word.
    CHECK(slugify("A.2 Rider-Waite-Smith") == "a2-rider-waite-smith");
}

TEST_CASE("the pinned v2 text yields the anchors the specification writes", "[spec]")
{
    auto const headings = pinned_headings(SPECIFICATION_V2_FILE);

    REQUIRE_FALSE(headings.empty());
    CHECK(has_slug(headings, "674-the-extension-chain"));
    CHECK(has_slug(headings, "104-validation-rules"));
}

TEST_CASE("the pinned v1 text yields its unnumbered anchors", "[spec]")
{
    auto const headings = pinned_headings(SPECIFICATION_V1_FILE);

    REQUIRE_FALSE(headings.empty());
    CHECK(has_slug(headings, "schema-versioning"));
    CHECK(has_slug(headings, "directory-skeleton"));
}


TEST_CASE("a fenced TOML comment is not a heading", "[spec]")
{
    constexpr std::string_view text = "# Real\n```toml\n# not a heading\n```\n## Also Real\n";

    auto const headings = headings_of(text);

    REQUIRE(headings.size() == 2);
    CHECK(headings[0].text == "Real");
    CHECK(headings[1].text == "Also Real");
}


TEST_CASE("every citation names a heading of the text it cites", "[spec]")
{
    // Indexed by schema major (index 0 is unused)
    std::vector<std::vector<heading>> const by_major{
        {}, pinned_headings(SPECIFICATION_V1_FILE), pinned_headings(SPECIFICATION_V2_FILE)
    };

    for (auto const& r : rules())
    {
        for (spec_section const& section : r.citations())
        {
            INFO(
                "rule " << r.code << " cites #" << section.anchor << " of schema major "
                        << static_cast<int>(section.schema_major)
            );

            REQUIRE(section.schema_major < by_major.size());
            REQUIRE_FALSE(by_major[section.schema_major].empty());

            INFO("re-read the spec: this citation no longer resolves");
            CHECK(has_slug(by_major[section.schema_major], section.anchor));
        }
    }
}

TEST_CASE("a citation builds a URL into its major's pinned revision", "[spec]")
{
    constexpr std::string_view base = "https://github.com/arcanaland/specifications/blob/";

    CHECK(
        spec_url(spec_section{2, "104-validation-rules"}) ==
        std::string{base} + SPECIFICATION_V2_TAG + "/DECK.md#104-validation-rules"
    );

    CHECK(
        spec_url(spec_section{1, "schema-versioning"}) ==
        std::string{base} + SPECIFICATION_V1_TAG + "/README.md#schema-versioning"
    );

    CHECK(spec_url(spec_section{3, "104-validation-rules"}).empty());
}

TEST_CASE("each major is pinned to the revision the build fetched", "[spec]")
{
    CHECK(spec_revision(1) == SPECIFICATION_V1_TAG);
    CHECK(spec_revision(2) == SPECIFICATION_V2_TAG);
    CHECK(spec_revision(3).empty());
}

// Prevent stale spec refs to appear in the code
TEST_CASE("every specification reference in a source file resolves", "[spec]")
{
    std::vector<std::vector<heading>> const by_file{
        pinned_headings(SPECIFICATION_V1_FILE), pinned_headings(SPECIFICATION_V2_FILE)
    };

    auto const found = references_in_sources();

    REQUIRE_FALSE(found.empty());

    for (auto const& one : found)
    {
        INFO(one.where << " names #" << one.anchor << " of " << one.file);

        std::size_t index = 0;
        if (one.file == "README.md")
            index = 0;
        else if (one.file == "DECK.md")
            index = 1;
        else
            FAIL("only the two pinned specification files may be cited, not " + one.file);

        REQUIRE_FALSE(by_file[index].empty());

        INFO("re-read the spec: this reference no longer resolves");
        CHECK(has_slug(by_file[index], one.anchor));
    }
}
