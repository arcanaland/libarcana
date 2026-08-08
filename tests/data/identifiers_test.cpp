// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <identifiers.hpp>

#include <catch2/catch_test_macros.hpp>

using arcana::data::is_canonical_id;
using arcana::data::is_card_reference;
using arcana::data::is_custom_name;
using arcana::data::is_qualified_identifier;
using arcana::data::is_realm;
using arcana::data::is_reserved_canonical_key;
using arcana::data::is_variant_reference;
using arcana::data::parse_qualified_identifier;

TEST_CASE("custom names accept the grammar of DECK.md 3.5", "[identifiers]")
{
    CHECK(is_custom_name("happy_squirrel"));
    CHECK(is_custom_name("stars"));
    CHECK(is_custom_name("_private"));
    CHECK(is_custom_name("a"));
    CHECK(is_custom_name("two_women"));
    CHECK(is_custom_name("x_kickstarter"));
    CHECK(is_custom_name("edition2"));
}

TEST_CASE("custom names reject everything outside it", "[identifiers]")
{
    CHECK_FALSE(is_custom_name(""));
    CHECK_FALSE(is_custom_name("2women"));
    CHECK_FALSE(is_custom_name("Stars"));
    CHECK_FALSE(is_custom_name("happy-squirrel"));
    CHECK_FALSE(is_custom_name("happy squirrel"));
    CHECK_FALSE(is_custom_name("major_arcana.00"));
    CHECK_FALSE(is_custom_name("caf\xc3\xa9"));
}

TEST_CASE("the reserved canonical keys are the twenty of DECK.md 3.2", "[identifiers]")
{
    CHECK(is_reserved_canonical_key("major_arcana"));
    CHECK(is_reserved_canonical_key("minor_arcana"));
    CHECK(is_reserved_canonical_key("wands"));
    CHECK(is_reserved_canonical_key("pentacles"));
    CHECK(is_reserved_canonical_key("ace"));
    CHECK(is_reserved_canonical_key("king"));
    CHECK(is_reserved_canonical_key("knight"));

    CHECK_FALSE(is_reserved_canonical_key("stars"));
    CHECK_FALSE(is_reserved_canonical_key("Wands"));
    CHECK_FALSE(is_reserved_canonical_key(""));
    CHECK_FALSE(is_reserved_canonical_key("arcana"));
}

TEST_CASE("canonical IDs cover both arcana", "[identifiers]")
{
    CHECK(is_canonical_id("major_arcana.00"));
    CHECK(is_canonical_id("major_arcana.21"));
    CHECK(is_canonical_id("major_arcana.happy_squirrel"));
    CHECK(is_canonical_id("minor_arcana.wands.ace"));
    CHECK(is_canonical_id("minor_arcana.stars.ace"));
    CHECK(is_canonical_id("minor_arcana.wands.knight"));

    CHECK(is_canonical_id("major_arcana.99"));
}

TEST_CASE("canonical IDs reject malformed positions", "[identifiers]")
{
    CHECK_FALSE(is_canonical_id(""));
    CHECK_FALSE(is_canonical_id("major_arcana"));
    CHECK_FALSE(is_canonical_id("major_arcana."));
    CHECK_FALSE(is_canonical_id("major_arcana.0"));
    CHECK_FALSE(is_canonical_id("major_arcana.000"));
    CHECK_FALSE(is_canonical_id("major_arcana.00.extra"));
    CHECK_FALSE(is_canonical_id("minor_arcana.wands"));
    CHECK_FALSE(is_canonical_id("minor_arcana.wands.ace.extra"));
    CHECK_FALSE(is_canonical_id("minor_arcana..ace"));
    CHECK_FALSE(is_canonical_id("trumps.00"));
}

TEST_CASE(
    "a card reference may carry a variant suffix and a variant reference must", "[identifiers]"
)
{
    CHECK(is_card_reference("major_arcana.06"));
    CHECK(is_card_reference("major_arcana.06:two_women"));
    CHECK(is_card_reference("minor_arcana.cups.two:alternate"));

    CHECK_FALSE(is_variant_reference("major_arcana.06"));
    CHECK(is_variant_reference("major_arcana.06:two_women"));

    CHECK_FALSE(is_card_reference("major_arcana.06:"));
    CHECK_FALSE(is_card_reference("major_arcana.06:Two_Women"));
    CHECK_FALSE(is_card_reference(":two_women"));
}

TEST_CASE("a realm is two or more labels", "[identifiers]")
{
    CHECK(is_realm("land.arcana"));
    CHECK(is_realm("org.example.my.domain"));
    CHECK(is_realm("example.xn--bcher-kva"));
    CHECK(is_realm("a.b"));

    CHECK_FALSE(is_realm("arcana"));
    CHECK_FALSE(is_realm(""));
    CHECK_FALSE(is_realm("land."));
    CHECK_FALSE(is_realm(".arcana"));
    CHECK_FALSE(is_realm("land..arcana"));
    CHECK_FALSE(is_realm("Land.Arcana"));
    CHECK_FALSE(is_realm("land.-arcana"));
    CHECK_FALSE(is_realm("land.arcana-"));
    CHECK_FALSE(is_realm("land.9arcana"));
}

TEST_CASE("qualified identifiers split into realm, path and fragment", "[identifiers]")
{
    auto const deck = parse_qualified_identifier("land.arcana/deck/rider-waite-smith");
    REQUIRE(deck.has_value());
    CHECK(deck->realm == "land.arcana");
    CHECK(deck->path == "deck/rider-waite-smith");
    CHECK(deck->fragment.empty());

    auto const card = parse_qualified_identifier("land.arcana/deck/inclusive#major_arcana.06:two");
    REQUIRE(card.has_value());
    CHECK(card->realm == "land.arcana");
    CHECK(card->path == "deck/inclusive");
    CHECK(card->fragment == "major_arcana.06:two");
}

TEST_CASE("qualified identifiers reject malformed shapes", "[identifiers]")
{
    CHECK_FALSE(is_qualified_identifier(""));
    CHECK_FALSE(is_qualified_identifier("land.arcana"));
    CHECK_FALSE(is_qualified_identifier("arcana/deck/x"));
    CHECK_FALSE(is_qualified_identifier("land.arcana/"));
    CHECK_FALSE(is_qualified_identifier("land.arcana//deck"));
    CHECK_FALSE(is_qualified_identifier("land.arcana/deck/x#"));
    CHECK_FALSE(is_qualified_identifier("land.arcana/deck/Rider"));
    CHECK_FALSE(is_qualified_identifier("land.arcana/deck/rider_waite"));
    CHECK_FALSE(is_qualified_identifier("https://land.arcana/deck/x"));
}
