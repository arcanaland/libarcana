// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The anchor gate.
//
// Every citation the catalogue carries names a heading of the specification
// revision it was derived from. The pins live in src/validation/spec_pin.hpp;
// the text itself is fetched by tests/CMakeLists.txt at the same two commits.

#include "markdown.hpp"

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

}  // namespace

TEST_CASE("slugify follows GitHub's anchor rule", "[spec]")
{
    CHECK(slugify("5.7.4 The Extension Chain") == "574-the-extension-chain");
    CHECK(slugify("9.4 Validation Rules") == "94-validation-rules");

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
    CHECK(has_slug(headings, "574-the-extension-chain"));
    CHECK(has_slug(headings, "94-validation-rules"));
}

TEST_CASE("the pinned v1 text yields its unnumbered anchors", "[spec]")
{
    auto const headings = pinned_headings(SPECIFICATION_V1_FILE);

    REQUIRE_FALSE(headings.empty());
    CHECK(has_slug(headings, "schema-versioning"));
    CHECK(has_slug(headings, "directory-skeleton"));
}

TEST_CASE("no heading of either pinned file collides with another", "[spec]")
{
    // GitHub would disambiguate a collision with a -1 suffix, which slugify
    // deliberately does not implement. If this ever fires, it must be taught to.
    for (char const* path : {SPECIFICATION_V1_FILE, SPECIFICATION_V2_FILE})
    {
        INFO("file: " << path);

        auto const headings = pinned_headings(path);

        std::vector<std::string> slugs;
        slugs.reserve(headings.size());
        for (auto const& h : headings) slugs.push_back(h.slug);

        std::ranges::sort(slugs);
        CHECK(std::ranges::adjacent_find(slugs) == slugs.end());
    }
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
    // Indexed by schema major; index 0 is unused.
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

            // Deliberately not "update the anchor". A citation that no longer
            // resolves means the section moved or was rewritten, and the rule's
            // basis has to be found again in the new text. Editing the string
            // until this passes hides exactly the drift the gate is here for.
            INFO("re-read the section: this citation no longer resolves");
            CHECK(has_slug(by_major[section.schema_major], section.anchor));
        }
    }
}

TEST_CASE("a citation builds a URL into its major's pinned revision", "[spec]")
{
    CHECK(
        spec_url(spec_section{2, "94-validation-rules"}) ==
        "https://github.com/arcanaland/specifications/blob/"
        "f32d330cdcd84190a80e357ec7b826c4befe5446/DECK.md#94-validation-rules"
    );

    // The v1.0 text predates the README.md -> DECK.md rename.
    CHECK(
        spec_url(spec_section{1, "schema-versioning"}) ==
        "https://github.com/arcanaland/specifications/blob/"
        "29f2184b8fc29e1db016c1f4a3d0c96bac8a4217/README.md#schema-versioning"
    );

    CHECK(spec_url(spec_section{3, "94-validation-rules"}).empty());
}

TEST_CASE("each major reports the revision it was read against", "[spec]")
{
    CHECK(spec_revision(1) == "29f2184b8fc29e1db016c1f4a3d0c96bac8a4217");
    CHECK(spec_revision(2) == "f32d330cdcd84190a80e357ec7b826c4befe5446");
    CHECK(spec_revision(3).empty());
}
