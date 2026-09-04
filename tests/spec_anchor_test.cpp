// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "markdown.hpp"
#include "sources.hpp"
#include "spec_reference.hpp"

#include <arcana/validation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
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
using arcana_test::references_in;
using arcana_test::slugify;
using arcana_test::source_line;
using arcana_test::source_lines;
using arcana_test::spec_reference;

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

// Every reference to the spec in the code
std::vector<spec_reference> references_in_sources()
{
    std::vector<spec_reference> found;

    for (source_line const& line : source_lines()) references_in(line.text, line.where(), found);

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
