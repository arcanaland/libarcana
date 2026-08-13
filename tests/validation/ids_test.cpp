// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "fixture.hpp"

#include <arcana/validation.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <optional>
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

TEST_CASE("a well-formed deck fires no ids diagnostic", "[validation][ids]")
{
    CHECK(validate_fixture("validation/ids/identifiers-valid").empty());
}

TEST_CASE("malformed identifiers and app realms are reported", "[validation][ids]")
{
    auto const found = validate_fixture("validation/ids/identifiers-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "bad-app-realm",
                               "bad-deck-identifier",
                               "bad-signifies",
                           }
    );

    CHECK(
        keys_of(found) == std::vector<std::string>{
                              "app.land",
                              "deck.identifier",
                              "deck.signifies",
                          }
    );

    for (auto const& one : found)
    {
        INFO("code: " << one.code);
        CHECK(one.level == severity::error);
        CHECK_FALSE(one.card.has_value());
        CHECK_FALSE(one.path.has_value());
    }

    CHECK(found[0].message.find("'land'") != std::string::npos);
    CHECK(found[1].message.find("notarealm/deck/bad-identifiers") != std::string::npos);
    CHECK(found[2].message.find("also_bad") != std::string::npos);
}

TEST_CASE("a deck with no identifier is warned about", "[validation][ids]")
{
    auto const found = validate_fixture("validation/ids/no-identifier-error");

    REQUIRE(codes_of(found) == std::vector<std::string_view>{"missing-deck-identifier"});

    CHECK(found.front().level == severity::warning);
    CHECK(found.front().key == "deck.identifier");
}

TEST_CASE("an off-convention path and a self-signifying deck are reported", "[validation][ids]")
{
    auto const found = validate_fixture("validation/ids/identifier-shape-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "deck-identifier-path-shape",
                               "signifies-self",
                           }
    );

    CHECK(
        keys_of(found) == std::vector<std::string>{
                              "deck.identifier",
                              "deck.signifies",
                          }
    );

    // The identifier is well formed, so neither shape rule is an error.
    CHECK(found[0].level == severity::warning);
    CHECK(found[0].message.find("tarot/odd-shape") != std::string::npos);

    CHECK(found[1].level == severity::error);
    CHECK(found[1].message.find("org.example/tarot/odd-shape") != std::string::npos);
}

TEST_CASE("coined names outside the grammar and reserved ones are reported", "[validation][ids]")
{
    auto const found = validate_fixture("validation/ids/custom-names-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "bad-custom-name",
                               "bad-custom-name",
                               "bad-custom-name",
                               "bad-custom-name",
                               "bad-custom-name",
                               "reserved-custom-name",
                               "reserved-custom-name",
                           }
    );

    CHECK(
        keys_of(found) == std::vector<std::string>{
                              R"(cards."major_arcana.06".default_variant)",
                              "suits.Stars",
                              "suits.Stars.ranks",
                              "<none>",
                              "<none>",
                              "card_backs.designs.major_arcana",
                              "suits.Stars.ranks",
                          }
    );

    REQUIRE(found[3].path.has_value());
    REQUIRE(found[4].path.has_value());
    CHECK(found[3].path->generic_string() == "h1200/major_arcana/Morning.png");
    CHECK(found[4].path->generic_string() == "h1200/minor_arcana/stars/Knave.png");

    CHECK(found[1].message.find("'Stars'") != std::string::npos);
    CHECK(found[2].message.find("'2nd'") != std::string::npos);

    // `wands` is a reserved suit used as a rank key; `major_arcana` is a
    // reserved key used as a card back design.
    CHECK(found[5].message.find("'major_arcana'") != std::string::npos);
    CHECK(found[6].message.find("'wands'") != std::string::npos);

    for (auto const& one : found)
    {
        INFO("message: " << one.message);
        CHECK(one.level == severity::error);
    }

    // `ace` in the same ranks list is a canonical rank where one belongs, and
    // `stars` is a well-formed custom suit key discovered from the tree.
    for (auto const& one : found)
    {
        INFO("message: " << one.message);
        CHECK(one.message.find("'ace'") == std::string::npos);
        CHECK(one.message.find("'stars'") == std::string::npos);
    }
}

TEST_CASE("card references outside the canonical grammar are reported", "[validation][ids]")
{
    auto const found = validate_fixture("validation/ids/card-references-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "bad-cards-table-key",
                               "non-canonical-card-reference",
                           }
    );

    CHECK(
        cards_of(found) == std::vector<std::string>{
                               "major_arcana.6",
                               "minor_arcana.Pentacles.page",
                           }
    );

    CHECK(
        keys_of(found) == std::vector<std::string>{
                              R"(cards."major_arcana.6")",
                              "excluded_cards.cards",
                          }
    );

    CHECK(found[0].message.find("is not a card reference") != std::string::npos);
    CHECK(found[1].message.find("is not a canonical ID") != std::string::npos);

    for (auto const& one : found)
    {
        INFO("code: " << one.code);
        CHECK(one.level == severity::error);
    }
}

TEST_CASE("a fragment on a field that names a deck is reported", "[validation][ids]")
{
    auto const found = validate_fixture("validation/ids/fragment-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "bad-deck-identifier",
                               "bad-signifies",
                           }
    );

    CHECK(
        keys_of(found) == std::vector<std::string>{
                              "deck.identifier",
                              "deck.signifies",
                          }
    );

    using Catch::Matchers::ContainsSubstring;

    REQUIRE_THAT(found[0].message, ContainsSubstring("major_arcana.00"));
    REQUIRE_THAT(found[1].message, ContainsSubstring("major_arcana.06:two_enbys"));

    for (auto const& one : found)
    {
        INFO("code: " << one.code);
        CHECK(one.level == severity::error);
        CHECK_FALSE(one.card.has_value());
    }
}
