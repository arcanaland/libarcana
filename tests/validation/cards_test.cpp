// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "fixture.hpp"

#include <arcana/validation.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>
#include <string_view>
#include <vector>

using arcana::diagnostic;
using arcana::severity;
using arcana_test::codes_of;
using arcana_test::validate_fixture;

namespace
{

std::vector<std::string> keys_of(std::vector<diagnostic> const& found)
{
    std::vector<std::string> keys;
    keys.reserve(found.size());
    for (auto const& one : found) keys.push_back(one.key.value_or("<none>"));

    return keys;
}

std::vector<std::string> cards_of(std::vector<diagnostic> const& found)
{
    std::vector<std::string> cards;
    cards.reserve(found.size());
    for (auto const& one : found) cards.push_back(one.card.value_or("<none>"));

    return cards;
}

}  // namespace

TEST_CASE("facets declared where they say nothing are reported", "[validation][cards]")
{
    auto const found = validate_fixture("validation/cards/entry-facets-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "empty-card-number",
                               "ignored-key-on-variant",
                               "ignored-key-on-variant",
                               "position-on-minor-arcanum",
                           }
    );

    CHECK(
        cards_of(found) == std::vector<std::string>{
                               "major_arcana.23",
                               "major_arcana.06",
                               "major_arcana.06",
                               "minor_arcana.wands.ace",
                           }
    );

    CHECK(
        keys_of(found) == std::vector<std::string>{
                              R"(cards."major_arcana.23".number)",
                              R"(cards."major_arcana.06:two_women".default_variant)",
                              R"(cards."major_arcana.06:two_women".position)",
                              R"(cards."minor_arcana.wands.ace".position)",
                          }
    );

    CHECK(found[0].level == severity::error);
    CHECK(found[1].level == severity::warning);
    CHECK(found[2].level == severity::warning);
    CHECK(found[3].level == severity::warning);

    for (auto const& one : found)
    {
        INFO("code: " << one.code);
        CHECK_FALSE(one.path.has_value());
    }
}

TEST_CASE("a number on a variant reference is legal there", "[validation][cards]")
{
    auto const found = validate_fixture("validation/cards/entry-facets-error");

    // The rule names the two facets of the slot and no others. A number is ink,
    // so a variant may print one its card does not.
    using Catch::Matchers::ContainsSubstring;

    for (auto const& one : found)
    {
        INFO("key: " << one.key.value_or("<none>"));
        REQUIRE_THAT(one.key.value_or(""), !ContainsSubstring("two_women\".number"));
    }
}

TEST_CASE("a deck that contradicts its own exclusions is reported", "[validation][cards]")
{
    auto const found = validate_fixture("validation/cards/exclusions-error");

    // The leftover under scalable/ is a raster outside an image root, which is
    // exactly why discovery passes it over and the exclusion still holds.
    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "excluded-card-also-declared",
                               "excluded-card-has-image",
                               "raster-outside-image-root",
                           }
    );

    // The merely-excluded card draws neither, which is what an exclusion is for.
    CHECK(
        cards_of(found) == std::vector<std::string>{
                               "minor_arcana.pentacles.page",
                               "minor_arcana.pentacles.ten",
                               "<none>",
                           }
    );

    CHECK(found[0].level == severity::error);
    CHECK(found[0].key == R"(cards."minor_arcana.pentacles.page")");
    CHECK_FALSE(found[0].path.has_value());

    CHECK(found[1].level == severity::warning);
    CHECK_FALSE(found[1].key.has_value());
    REQUIRE(found[1].path.has_value());
    CHECK(found[1].path->generic_string() == "h1200/minor_arcana/pentacles/ten.png");
}

TEST_CASE("a 1.x deck shipping an excluded card's artwork is reported", "[validation][cards]")
{
    // The image side of the pair reads the tree, so it does not depend on
    // anything the newer schema introduced.
    auto const found = validate_fixture("validation/cards/v1-exclusion-image-error");

    REQUIRE(codes_of(found) == std::vector<std::string_view>{"excluded-card-has-image"});

    CHECK(found.front().level == severity::warning);
    CHECK(found.front().card == "minor_arcana.pentacles.page");
    REQUIRE(found.front().path.has_value());
    CHECK(found.front().path->generic_string() == "h1200/minor_arcana/pentacles/page.png");
}

TEST_CASE("references that resolve to no artwork are reported", "[validation][cards]")
{
    auto const found = validate_fixture("validation/cards/artwork-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "declared-card-without-image",
                               "declared-card-without-image",
                               "declared-card-without-image",
                               "missing-variant-image",
                               "unknown-variant-default",
                               "variant-card-without-default",
                           }
    );

    CHECK(
        keys_of(found) == std::vector<std::string>{
                              R"(cards."major_arcana.01:gilded")",
                              R"(cards."major_arcana.23")",
                              R"(cards."major_arcana.24")",
                              R"(cards."major_arcana.02:inked".image)",
                              R"(cards."major_arcana.08".default_variant)",
                              R"(cards."major_arcana.07".default_variant)",
                          }
    );

    for (auto const& one : found)
    {
        INFO("key: " << one.key.value_or("<none>"));
        CHECK(one.level == severity::error);
        CHECK_FALSE(one.path.has_value());
    }

    using Catch::Matchers::ContainsSubstring;
    REQUIRE_THAT(found[3].message, ContainsSubstring("scalable/major_arcana/02.inked.svg"));
    REQUIRE_THAT(found[4].message, ContainsSubstring("'gone'"));
}

TEST_CASE("the exempt slots and the fileless card draw nothing", "[validation][cards]")
{
    auto const found = validate_fixture("validation/cards/artwork-error");

    // A canonical minor arcanum and a major keyed 00 through 21 are exempt from
    // the declaration rule, and a card with no files at all is a resolution
    // failure rather than a default variant violation.
    for (auto const& one : found)
    {
        INFO("key: " << one.key.value_or("<none>"));
        CHECK(one.card != "major_arcana.00");
        CHECK(one.card != "major_arcana.09");
        CHECK(one.card != "minor_arcana.wands.ace");
    }
}

TEST_CASE("a rank list that is wrong or ambiguous is reported", "[validation][cards]")
{
    auto const found = validate_fixture("validation/cards/rank-lists-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "duplicate-rank-in-ranks",
                               "rank-without-image",
                           }
    );

    CHECK(
        keys_of(found) == std::vector<std::string>{
                              "suits.stars.ranks",
                              "suits.stars.ranks",
                          }
    );

    using Catch::Matchers::ContainsSubstring;

    CHECK(found[0].level == severity::error);
    CHECK_FALSE(found[0].card.has_value());
    REQUIRE_THAT(found[0].message, ContainsSubstring("'ace'"));

    CHECK(found[1].level == severity::error);
    CHECK(found[1].card == "minor_arcana.stars.three");
    REQUIRE_THAT(found[1].message, ContainsSubstring("'three'"));

    // The well-formed list over a suit whose cards the deck ships says nothing.
    for (auto const& one : found)
    {
        INFO("message: " << one.message);
        CHECK(one.key != "suits.wands.ranks");
    }
}

TEST_CASE("the duplicate rank check needs no tree", "[validation][cards]")
{
    auto const* rule = arcana::find_rule("duplicate-rank-in-ranks");

    REQUIRE(rule != nullptr);

    // Finding a repeated key in a list requires no tree at all. Its sibling
    // genuinely does need one and stays where it is.
    CHECK(rule->needs == arcana::phase::document);
    CHECK(arcana::find_rule("rank-without-image")->needs == arcana::phase::filesystem);
}

TEST_CASE("two major arcana claiming one position are reported", "[validation][cards]")
{
    auto const found = validate_fixture("validation/cards/positions-error");

    REQUIRE(codes_of(found) == std::vector<std::string_view>{"duplicate-card-position"});

    CHECK(found.front().level == severity::warning);
    CHECK(found.front().card == "major_arcana.happy_squirrel");
    CHECK(found.front().key == R"(cards."major_arcana.happy_squirrel".position)");

    using Catch::Matchers::ContainsSubstring;

    REQUIRE_THAT(found.front().message, ContainsSubstring("major_arcana.22"));
    REQUIRE_THAT(found.front().message, ContainsSubstring("position 22"));
}

TEST_CASE("two 1.x major arcana claiming one position are reported", "[validation][cards]")
{
    // The code reads declared positions, which a 1.0 deck writes in its own
    // tables, so it depends on nothing the newer schema introduced.
    auto const found = validate_fixture("validation/cards/v1-positions-error");

    REQUIRE(codes_of(found) == std::vector<std::string_view>{"duplicate-card-position"});

    CHECK(found.front().level == severity::warning);
    CHECK(found.front().card == "major_arcana.other_squirrel");
    CHECK(found.front().key == "custom_cards.major_arcana.other_squirrel.position");
}

TEST_CASE("a rank repeated three times is reported once", "[validation][cards]")
{
    auto const found = validate_fixture("validation/cards/rank-lists-error");

    // The diagnostics carry no identity beyond their code, key and message, so
    // a second report of the same repeat would be an exact duplicate.
    std::size_t repeats = 0;
    for (auto const& one : found)
        if (one.code == "duplicate-rank-in-ranks")
            ++repeats;

    CHECK(repeats == 1);
}

TEST_CASE("a suit or rank table naming what the deck lacks is reported", "[validation][cards]")
{
    auto const found = validate_fixture("validation/cards/suit-rank-table-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "undefined-suit-or-rank-table",
                               "undefined-suit-or-rank-table",
                           }
    );

    // [suits.wands], [suits.cups] and [ranks.page] describe the files the deck
    // has, so only the printed-name keys are reported.
    CHECK(
        keys_of(found) == std::vector<std::string>{
                              "ranks.valet",
                              "suits.batons",
                          }
    );

    using Catch::Matchers::ContainsSubstring;

    for (auto const& one : found)
    {
        INFO("message: " << one.message);
        CHECK(one.level == severity::warning);
        CHECK_FALSE(one.card.has_value());
        CHECK_FALSE(one.path.has_value());
    }

    REQUIRE_THAT(found[0].message, ContainsSubstring("[ranks.valet]"));
    REQUIRE_THAT(found[1].message, ContainsSubstring("[suits.batons]"));
}

TEST_CASE("an excluded card's off-chain leftover is not artwork", "[validation][cards]")
{
    auto const found = validate_fixture("validation/cards/exclusions-error");

    // Discovery passes over a file the extension chain does not admit where it
    // sits, so the package still matches the exclusion and the warning would
    // assert something untrue.
    for (auto const& one : found)
    {
        INFO("code: " << one.code);

        bool const names_the_leftover =
            one.code == "excluded-card-has-image" && one.card == "minor_arcana.pentacles.nine";

        CHECK_FALSE(names_the_leftover);
    }
}
