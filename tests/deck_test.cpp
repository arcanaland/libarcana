// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/deck.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>

using namespace arcana;

namespace
{

std::string fixture(std::string const& name)
{
    return std::string(FIXTURES_DIR) + "/" + name;
}

bool has_card(deck const& d, std::string const& canonical_id)
{
    return d.find_card(canonical_id) != nullptr;
}

// CMake fetches the reference decks by default, so this is empty only when someone
// configured with -DARCANA_FETCH_REFERENCE_DECKS=OFF. That's deliberate, so skipping
// rather than failing is right -- and because it takes an explicit opt-out, a skip here
// can't quietly retire this coverage on a normal build.
std::string reference_deck(std::string const& name)
{
    std::string const dir = REFERENCE_DECKS_DIR;
    if (dir.empty())
        SKIP("configured with ARCANA_FETCH_REFERENCE_DECKS=OFF");
    return dir + "/" + name;
}

}  // namespace

TEST_CASE("a deck.toml that fails to parse is a hard error", "[deck]")
{
    auto const result = load_deck(fixture("broken-deck"));
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == error_code::parse_error);
}

TEST_CASE("excluded cards are omitted from enumeration but reason is queryable", "[deck]")
{
    auto const result = load_deck(fixture("excluded-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    CHECK(d.cards.size() == 76);
    CHECK_FALSE(has_card(d, "minor_arcana.pentacles.page"));
    CHECK_FALSE(has_card(d, "minor_arcana.pentacles.knight"));

    auto const reason = d.exclusion_reason("minor_arcana.pentacles.page");
    REQUIRE(reason.has_value());
    CHECK(*reason == "Test fixture excluding two court cards.");

    auto const reason2 = d.exclusion_reason("minor_arcana.pentacles.knight");
    REQUIRE(reason2.has_value());
    CHECK(*reason2 == "Test fixture excluding two court cards.");

    CHECK_FALSE(d.exclusion_reason("major_arcana.00").has_value());
}

TEST_CASE("custom cards and custom suits are enumerated alongside the standard 78", "[deck]")
{
    auto const result = load_deck(fixture("custom-suit-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    CHECK(d.cards.size() == 78 + 3);

    auto const* squirrel = d.find_card("major_arcana.happy_squirrel");
    REQUIRE(squirrel != nullptr);
    CHECK(squirrel->display_name == "The Happy Squirrel");
    CHECK(squirrel->alt_text.value_or("") == "A cheerful squirrel gathering acorns.");
    CHECK(squirrel->id.is_custom());
    CHECK(squirrel->id.custom_id.value_or("") == "happy_squirrel");

    auto const* stars_ace = d.find_card("minor_arcana.stars.ace");
    REQUIRE(stars_ace != nullptr);
    CHECK(stars_ace->display_name == "Ace of Stars");
    CHECK(stars_ace->id.suit_key.value_or("") == "stars");
    CHECK(stars_ace->id.custom_id.value_or("") == "ace");

    CHECK(d.find_card("minor_arcana.stars.two") != nullptr);

    // The bug this shape was meant to remove: every enumerated card's canonical id now
    // round-trips through parse_card_id, custom cards included.
    for (auto const& c : d.cards)
    {
        auto const parsed = parse_card_id(c.canonical_id());
        REQUIRE(parsed.has_value());
        CHECK(*parsed == c.id);
    }
}

TEST_CASE("aliases, remapping, card backs and variants parse", "[deck]")
{
    auto const result = load_deck(fixture("aliased-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    CHECK(d.display_suit_name(suit::wands) == "Staves");
    CHECK(d.display_suit_name(suit::pentacles) == "Disks");
    CHECK(d.display_suit_name(suit::cups) == "cups");  // no alias defined: canonical form

    CHECK(d.display_rank_name(rank::page) == "Princess");
    CHECK(d.display_rank_name(rank::knight) == "Prince");
    CHECK(d.display_rank_name(rank::king) == "king");  // no alias defined: canonical form

    REQUIRE(d.major_arcana_remap.contains(8));
    CHECK(d.major_arcana_remap.at(8) == "justice");
    REQUIRE(d.major_arcana_remap.contains(11));
    CHECK(d.major_arcana_remap.at(11) == "strength");

    REQUIRE(d.default_card_back.has_value());
    CHECK(*d.default_card_back == "classic");
    REQUIRE(d.card_backs.size() == 1);
    CHECK(d.card_backs.front().name == "Classic Back");

    REQUIRE(d.variants.size() == 1);
    CHECK(d.variants.front().id == "aliased-deck-standard");
    CHECK(d.variants.front().card_back.value_or("") == "classic");
}

TEST_CASE("file-location-based defaults: images map to cards with no deck.toml entry", "[deck]")
{
    auto const result = load_deck(fixture("file-location-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    auto const* fool = d.find_card("major_arcana.00");
    REQUIRE(fool != nullptr);
    REQUIRE_FALSE(fool->images.empty());
    CHECK(fool->images.front().variant_name == "h1200");
    CHECK(fool->images.front().height == 1200);

    auto const* ace_of_wands = d.find_card("minor_arcana.wands.ace");
    REQUIRE(ace_of_wands != nullptr);
    REQUIRE_FALSE(ace_of_wands->images.empty());
    CHECK(ace_of_wands->images.front().variant_name == "h1200");
}

TEST_CASE("rider-waite-smith enumerates all 78 standard cards", "[deck][reference-decks]")
{
    auto const result = load_deck(reference_deck("rider-waite-smith"));
    REQUIRE(result.has_value());
    CHECK(result->cards.size() == 78);
}

TEST_CASE("ascii-tarot resolves ansi32 image variants", "[deck][reference-decks]")
{
    auto const result = load_deck(reference_deck("ascii-tarot"));
    REQUIRE(result.has_value());

    auto const* fool = result->find_card("major_arcana.00");
    REQUIRE(fool != nullptr);
    auto const ansi =
        std::ranges::find(fool->images, std::string("ansi32"), &image_variant::variant_name);
    REQUIRE(ansi != fool->images.end());
}

TEST_CASE("aquatic-tarot resolves raster heights", "[deck][reference-decks]")
{
    auto const result = load_deck(reference_deck("aquatic-tarot"));
    REQUIRE(result.has_value());

    auto const* fool = result->find_card("major_arcana.00");
    REQUIRE(fool != nullptr);
    CHECK_FALSE(fool->images.empty());
}
