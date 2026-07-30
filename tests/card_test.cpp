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
        auto const parsed = card_id::parse(text);
        REQUIRE(parsed.has_value());
        CHECK(parsed->cls == card_class::standard_major);
        CHECK(parsed->kind() == arcana_kind::major_arcana);
        CHECK(parsed->is_major());
        CHECK(parsed->number == i);
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
            auto const text = std::format("minor_arcana.{}.{}", to_string(s), to_string(r));
            auto const parsed = card_id::parse(text);
            REQUIRE(parsed.has_value());
            CHECK(parsed->cls == card_class::standard_minor);
            CHECK(parsed->kind() == arcana_kind::minor_arcana);
            CHECK_FALSE(parsed->is_major());
            CHECK(parsed->standard_suit == s);
            CHECK(parsed->standard_rank == r);
            CHECK_FALSE(parsed->is_custom());
            CHECK(parsed->to_canonical() == text);
        }
    }
}

TEST_CASE("custom card ids round-trip and are marked custom", "[card]")
{
    auto const squirrel = card_id::parse("major_arcana.happy_squirrel");
    REQUIRE(squirrel.has_value());
    CHECK(squirrel->cls == card_class::custom_major);
    CHECK(squirrel->kind() == arcana_kind::major_arcana);
    CHECK(squirrel->is_major());
    CHECK(squirrel->is_custom());
    CHECK(squirrel->custom_id == "happy_squirrel");
    CHECK(squirrel->number == -1);
    CHECK(squirrel->to_canonical() == "major_arcana.happy_squirrel");

    auto const stars_ace = card_id::parse("minor_arcana.stars.ace");
    REQUIRE(stars_ace.has_value());
    CHECK(stars_ace->cls == card_class::custom_minor);
    CHECK(stars_ace->kind() == arcana_kind::minor_arcana);
    CHECK(stars_ace->is_custom());
    CHECK(stars_ace->suit_key == "stars");
    CHECK(stars_ace->custom_id == "ace");
    CHECK(stars_ace->to_canonical() == "minor_arcana.stars.ace");
}

TEST_CASE("card_class is the single discriminant the named constructors set", "[card]")
{
    // The whole point of the discriminant: four legal states, each named once, instead of
    // five optionals with 32 representable combinations and the invariant in comments.
    CHECK(card_id::standard_major(0).cls == card_class::standard_major);
    CHECK(card_id::custom_major("happy_squirrel").cls == card_class::custom_major);
    CHECK(card_id::standard_minor(suit::wands, rank::ace).cls == card_class::standard_minor);
    CHECK(card_id::custom_minor("stars", "ace").cls == card_class::custom_minor);

    // A consumer switches on cls and reads exactly the fields it names -- no probing.
    auto const custom = card_id::custom_minor("stars", "ace");
    CHECK(custom.number == -1);
    CHECK(custom.suit_key == "stars");
}

TEST_CASE("named constructors and equality agree with parsing", "[card]")
{
    CHECK(card_id::standard_major(0) == *card_id::parse("major_arcana.00"));
    CHECK(
        card_id::standard_minor(suit::wands, rank::ace) == *card_id::parse("minor_arcana.wands.ace")
    );
    CHECK(card_id::custom_major("happy_squirrel") == *card_id::parse("major_arcana.happy_squirrel"));
    CHECK(card_id::custom_minor("stars", "ace") == *card_id::parse("minor_arcana.stars.ace"));

    // A standard minor and a custom card that spell out differently never compare equal.
    CHECK_FALSE(
        card_id::standard_minor(suit::wands, rank::ace) == card_id::custom_minor("wands", "ace")
    );
}

TEST_CASE("malformed canonical ids are errors, not garbage", "[card]")
{
    // Two digits under major_arcana is always a standard major, so this is out of range
    // rather than a custom card named "22".
    CHECK_FALSE(card_id::parse("major_arcana.22").has_value());
    // A canonical suit commits the id to the standard 56, so an unknown rank is malformed.
    CHECK_FALSE(card_id::parse("minor_arcana.wands.jack").has_value());
    // Wrong arity, or not an identifier.
    CHECK_FALSE(card_id::parse("nonsense").has_value());
    CHECK_FALSE(card_id::parse("").has_value());
    CHECK_FALSE(card_id::parse("major_arcana.").has_value());
    CHECK_FALSE(card_id::parse("major_arcana.The-Fool").has_value());
    CHECK_FALSE(card_id::parse("major_arcana.HappySquirrel").has_value());
    CHECK_FALSE(card_id::parse("minor_arcana.stars.").has_value());
    CHECK_FALSE(card_id::parse("minor_arcana.stars.Ace").has_value());

    // Single-digit "1" is not the two-digit standard form, but it is a valid identifier,
    // so it parses as a custom major. Decks are expected to write "01".
    auto const one = card_id::parse("major_arcana.1");
    REQUIRE(one.has_value());
    CHECK(one->is_custom());
}

namespace
{

image_variant raster(int height)
{
    return {
        .variant_name = std::format("h{}", height),
        .path = std::format("h{}/major_arcana/00.png", height),
        .kind = image_kind::raster,
        .height = height,
    };
}

image_variant ansi(int lines)
{
    return {
        .variant_name = std::format("ansi{}", lines),
        .path = std::format("ansi{}/major_arcana/00.ansi", lines),
        .kind = image_kind::ansi,
        .lines = lines,
    };
}

image_variant scalable()
{
    return {
        .variant_name = "scalable",
        .path = "scalable/major_arcana/00.svg",
        .kind = image_kind::scalable,
    };
}

card card_with(std::vector<image_variant> images)
{
    card c;
    c.id = card_id::standard_major(0);
    c.images = std::move(images);
    return c;
}

}  // namespace

TEST_CASE("best_raster_for_height picks the smallest that meets the target", "[card]")
{
    auto const c = card_with({raster(750), raster(1200), raster(2400)});

    auto const best = c.best_raster_for_height(1000);
    REQUIRE(best.has_value());
    CHECK(best->height == 1200);
    CHECK(best->variant_name == "h1200");

    // Exact matches win outright, and a target below everything takes the smallest.
    CHECK(c.best_raster_for_height(2400)->height == 2400);
    CHECK(c.best_raster_for_height(10)->height == 750);
}

TEST_CASE("best_raster_for_height falls back to the largest available", "[card]")
{
    auto const c = card_with({raster(750), raster(1200)});

    // Nothing meets a 3000px target, so the closest below it is the least-bad upscale.
    auto const best = c.best_raster_for_height(3000);
    REQUIRE(best.has_value());
    CHECK(best->height == 1200);
}

TEST_CASE("best_ansi_for_lines prefers the largest that fits", "[card]")
{
    auto const c = card_with({ansi(16), ansi(32), ansi(64)});

    // The mirror of the raster rule: art taller than the terminal is unusable, so 32 beats
    // 64 for a 40-line terminal even though 64 is no further away.
    auto const best = c.best_ansi_for_lines(40);
    REQUIRE(best.has_value());
    CHECK(best->lines == 32);
    CHECK(best->variant_name == "ansi32");

    CHECK(c.best_ansi_for_lines(64)->lines == 64);

    // Nothing fits a 10-line terminal, so the closest overflow is all there is.
    CHECK(c.best_ansi_for_lines(10)->lines == 16);
}

TEST_CASE("the three families never answer for each other", "[card]")
{
    // The bug the kind discriminant exists to prevent: ansi32's "32" is terminal lines, so
    // it must not surface as a 32-pixel raster, and h1200 must not surface as ANSI art.
    auto const ansi_only = card_with({ansi(32)});
    CHECK_FALSE(ansi_only.best_raster_for_height(32).has_value());
    CHECK_FALSE(ansi_only.scalable_image().has_value());
    CHECK(ansi_only.best_ansi_for_lines(32).has_value());

    auto const raster_only = card_with({raster(1200)});
    CHECK_FALSE(raster_only.best_ansi_for_lines(1200).has_value());
    CHECK_FALSE(raster_only.scalable_image().has_value());

    auto const scalable_only = card_with({scalable()});
    CHECK_FALSE(scalable_only.best_raster_for_height(1200).has_value());
    CHECK_FALSE(scalable_only.best_ansi_for_lines(32).has_value());
    REQUIRE(scalable_only.scalable_image().has_value());
    CHECK(scalable_only.scalable_image()->variant_name == "scalable");

    // A card with no images at all answers nullopt three times rather than misreporting.
    CHECK_FALSE(card{}.best_raster_for_height(1200).has_value());
    CHECK_FALSE(card{}.best_ansi_for_lines(32).has_value());
    CHECK_FALSE(card{}.scalable_image().has_value());
}

TEST_CASE("the library ranks families but does not choose between them", "[card]")
{
    // The composition the header documents in place of a "just give me something" call.
    auto const both = card_with({scalable(), raster(1200)});
    auto const prefer_svg =
        both.scalable_image().or_else([&] { return both.best_raster_for_height(400); });
    REQUIRE(prefer_svg.has_value());
    CHECK(prefer_svg->kind == image_kind::scalable);

    // Same one-liner on a deck with no SVG falls through to the raster.
    auto const no_svg = card_with({raster(1200)});
    auto const fell_through =
        no_svg.scalable_image().or_else([&] { return no_svg.best_raster_for_height(400); });
    REQUIRE(fell_through.has_value());
    CHECK(fell_through->kind == image_kind::raster);

    CHECK(card_with({raster(1200)}).canonical_id() == "major_arcana.00");
}
