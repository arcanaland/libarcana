// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The `ids` area: ten rules. `duplicate-deck-identifier`, the eleventh, is the
// catalogue's only phase::library rule and validate(deck const&) cannot see a
// sibling deck, so TASK-016 defers it and no fixture here fires it.
//
// Every assertion below is closed-world: the complete set of codes the fixture
// produces, in the order validate() returns them. A fixture that starts firing
// an extra code fails here rather than silently broadening.

#include "fixture.hpp"

#include <arcana/validation.hpp>

#include <catch2/catch_test_macros.hpp>

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
    // Exercises every site the ten checks read, all of them well formed. This
    // is what stops a check from firing on a legitimate deck.
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

    // Each message names the value it judged.
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

    // The three declared names carry a key and no path; the two discovered in
    // the tree carry a path and no key, which is what makes these rules
    // phase::filesystem.
    CHECK(
        keys_of(found) == std::vector<std::string>{
                              "editions.first-edition",
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
                               "bad-cards-table-key",
                               "non-canonical-card-reference",
                               "non-canonical-card-reference",
                               "non-canonical-card-reference",
                           }
    );

    CHECK(
        cards_of(found) == std::vector<std::string>{
                               "major_arcana.06:two_women",
                               "major_arcana.6",
                               "major_arcana.06:two_women",
                               "major_arcana.6",
                               "minor_arcana.Pentacles.page",
                           }
    );

    CHECK(
        keys_of(found) == std::vector<std::string>{
                              R"(cards."major_arcana.06:two_women")",
                              R"(cards."major_arcana.6")",
                              R"(card_variants."major_arcana.06:two_women")",
                              "deck.identifier",
                              "excluded_cards.cards",
                          }
    );

    // A variant reference in [cards] gets its own message: the fix is to move
    // it, not to rewrite it.
    CHECK(found[0].message.find("[card_variants]") != std::string::npos);
    CHECK(found[1].message.find("both of its digits") != std::string::npos);

    // The identifier is a well-formed qualified identifier whose fragment is
    // not a card reference, so only the fragment is reported.
    CHECK(found[3].message.find("fragment") != std::string::npos);

    for (auto const& one : found)
    {
        INFO("code: " << one.code);
        CHECK(one.level == severity::error);
    }
}
