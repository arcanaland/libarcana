// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/card.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <format>
#include <vector>

using namespace arcana;

TEST_CASE("all 22 major arcana canonical ids round-trip", "[card]")
{
    for (int i = 0; i <= 21; ++i)
    {
        auto const text = std::format("major_arcana.{:02d}", i);
        auto const parsed = parse_card_id(text);
        REQUIRE(parsed.has_value());
        CHECK(parsed->kind == arcana_kind::major_arcana);
        CHECK(parsed->major_number == i);
        CHECK_FALSE(parsed->is_custom());
        CHECK(parsed->to_canonical() == text);
    }
}

TEST_CASE("all 56 minor arcana canonical ids round-trip", "[card]")
{
    constexpr std::array<suit, 4> suits{suit::wands, suit::cups, suit::swords, suit::pentacles};
    constexpr std::array<rank, 14> ranks{rank::ace,   rank::two, rank::three, rank::four,
                                         rank::five,  rank::six, rank::seven, rank::eight,
                                         rank::nine,  rank::ten, rank::page,  rank::knight,
                                         rank::queen, rank::king};

    int count = 0;
    for (auto const s : suits)
    {
        for (auto const r : ranks)
        {
            ++count;
            auto const text = std::format("minor_arcana.{}.{}", to_string(s), to_string(r));
            auto const parsed = parse_card_id(text);
            REQUIRE(parsed.has_value());
            CHECK(parsed->kind == arcana_kind::minor_arcana);
            CHECK(parsed->standard_suit == s);
            CHECK(parsed->standard_rank == r);
            CHECK_FALSE(parsed->is_custom());
            CHECK(parsed->to_canonical() == text);
        }
    }
    REQUIRE(count == 56);
}

TEST_CASE("custom card ids round-trip and are marked custom", "[card]")
{
    auto const squirrel = parse_card_id("major_arcana.happy_squirrel");
    REQUIRE(squirrel.has_value());
    CHECK(squirrel->kind == arcana_kind::major_arcana);
    CHECK(squirrel->is_custom());
    CHECK(squirrel->custom_id.value_or("") == "happy_squirrel");
    CHECK_FALSE(squirrel->major_number.has_value());
    CHECK(squirrel->to_canonical() == "major_arcana.happy_squirrel");

    auto const stars_ace = parse_card_id("minor_arcana.stars.ace");
    REQUIRE(stars_ace.has_value());
    CHECK(stars_ace->kind == arcana_kind::minor_arcana);
    CHECK(stars_ace->is_custom());
    CHECK(stars_ace->suit_key.value_or("") == "stars");
    CHECK(stars_ace->custom_id.value_or("") == "ace");
    CHECK_FALSE(stars_ace->standard_suit.has_value());
    CHECK(stars_ace->to_canonical() == "minor_arcana.stars.ace");
}

TEST_CASE("named constructors and equality agree with parsing", "[card]")
{
    CHECK(card_id::standard_major(0) == *parse_card_id("major_arcana.00"));
    CHECK(
        card_id::standard_minor(suit::wands, rank::ace) == *parse_card_id("minor_arcana.wands.ace")
    );
    CHECK(card_id::custom_major("happy_squirrel") == *parse_card_id("major_arcana.happy_squirrel"));
    CHECK(card_id::custom_minor("stars", "ace") == *parse_card_id("minor_arcana.stars.ace"));

    // A standard minor and a custom card that spell out differently never compare equal.
    CHECK_FALSE(
        card_id::standard_minor(suit::wands, rank::ace) == card_id::custom_minor("wands", "ace")
    );
}

TEST_CASE("malformed canonical ids are errors, not garbage", "[card]")
{
    // Two digits under major_arcana is always a standard major, so this is out of range
    // rather than a custom card named "22".
    CHECK_FALSE(parse_card_id("major_arcana.22").has_value());
    // A canonical suit commits the id to the standard 56, so an unknown rank is malformed.
    CHECK_FALSE(parse_card_id("minor_arcana.wands.jack").has_value());
    // Wrong arity, or not an identifier.
    CHECK_FALSE(parse_card_id("nonsense").has_value());
    CHECK_FALSE(parse_card_id("").has_value());
    CHECK_FALSE(parse_card_id("major_arcana.").has_value());
    CHECK_FALSE(parse_card_id("major_arcana.The-Fool").has_value());
    CHECK_FALSE(parse_card_id("major_arcana.HappySquirrel").has_value());
    CHECK_FALSE(parse_card_id("minor_arcana.stars.").has_value());
    CHECK_FALSE(parse_card_id("minor_arcana.stars.Ace").has_value());

    // Single-digit "1" is not the two-digit standard form, but it is a valid identifier,
    // so it parses as a custom major. Decks are expected to write "01".
    auto const one = parse_card_id("major_arcana.1");
    REQUIRE(one.has_value());
    CHECK(one->is_custom());
}

TEST_CASE("best_variant_for_height picks the smallest that meets the target", "[card]")
{
    std::vector<image_variant> const variants{
        {.variant_name = "h750", .path = "a", .height = 750},
        {.variant_name = "h1200", .path = "b", .height = 1200},
        {.variant_name = "h2400", .path = "c", .height = 2400},
    };

    auto const best = best_variant_for_height(variants, 1000);
    REQUIRE(best.has_value());
    CHECK(best->height == 1200);
}

TEST_CASE("best_variant_for_height falls back to the largest available", "[card]")
{
    std::vector<image_variant> const variants{
        {.variant_name = "h750", .path = "a", .height = 750},
        {.variant_name = "h1200", .path = "b", .height = 1200},
    };

    auto const best = best_variant_for_height(variants, 3000);
    REQUIRE(best.has_value());
    CHECK(best->height == 1200);
}

TEST_CASE("best_variant_for_height is nullopt with no raster entries", "[card]")
{
    std::vector<image_variant> const variants{
        {.variant_name = "scalable", .path = "a", .height = std::nullopt},
    };
    CHECK_FALSE(best_variant_for_height(variants, 1200).has_value());
}

TEST_CASE("card::best_image_for_height delegates to best_variant_for_height", "[card]")
{
    card c;
    c.id = card_id::standard_major(0);
    c.images = {
        {.variant_name = "scalable", .path = "a", .height = std::nullopt},
        {.variant_name = "h750", .path = "b", .height = 750},
        {.variant_name = "h2400", .path = "c", .height = 2400},
    };

    auto const best = c.best_image_for_height(1000);
    REQUIRE(best.has_value());
    CHECK(best->height == 2400);
    CHECK(c.canonical_id() == "major_arcana.00");

    CHECK_FALSE(card{}.best_image_for_height(1000).has_value());
}
