// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/deck.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

using namespace arcana;

namespace
{

std::string fixture(std::string const& name)
{
    return std::string(FIXTURES_DIR) + "/" + name;
}

std::optional<card> find(deck const& d, std::string_view canonical_id)
{
    auto const id = card_id::parse(canonical_id);
    if (!id)
        return std::nullopt;
    return d.find_card(*id);
}

bool has_card(deck const& d, std::string const& canonical_id)
{
    return find(d, canonical_id).has_value();
}

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

TEST_CASE("excluded cards are omitted from enumeration", "[deck]")
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

TEST_CASE("parse and find_card", "[deck]")
{
    auto const result = load_deck(fixture("excluded-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    auto const malformed = card_id::parse("minor_arcana.wands.jack");
    REQUIRE_FALSE(malformed.has_value());
    CHECK(malformed.error().code == error_code::parse_error);

    auto const excluded = card_id::parse("minor_arcana.pentacles.page");
    REQUIRE(excluded.has_value());
    CHECK_FALSE(d.find_card(*excluded).has_value());
    CHECK(d.exclusion_reason("minor_arcana.pentacles.page").has_value());

    // a valid custom card this deck never declared
    auto const absent = card_id::parse("major_arcana.happy_squirrel");
    REQUIRE(absent.has_value());
    CHECK_FALSE(d.find_card(*absent).has_value());
    CHECK_FALSE(d.exclusion_reason("major_arcana.happy_squirrel").has_value());

    CHECK_FALSE(d.find_card(card_id::standard_minor(suit::pentacles, rank::page)).has_value());
}

TEST_CASE("custom cards and custom suits are enumerated alongside the standard 78", "[deck]")
{
    auto const result = load_deck(fixture("custom-suit-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    CHECK(d.cards.size() == 78 + 3);

    auto const squirrel = find(d, "major_arcana.happy_squirrel");
    REQUIRE(squirrel.has_value());
    CHECK(squirrel->display_name == "The Happy Squirrel");
    CHECK(squirrel->alt_text.value_or("") == "A cheerful squirrel standing on a branch proudly holding an acorn.");
    CHECK(squirrel->id.is_custom());
    CHECK(squirrel->id.custom_id == "happy_squirrel");

    auto const stars_ace = find(d, "minor_arcana.stars.ace");
    REQUIRE(stars_ace.has_value());
    CHECK(stars_ace->display_name == "Ace of Stars");
    CHECK(stars_ace->id.suit_key == "stars");
    CHECK(stars_ace->id.custom_id == "ace");

    CHECK(find(d, "minor_arcana.stars.two").has_value());

    // Round trip every card's canonical_id
    for (auto const& c : d.cards)
    {
        auto const found = find(d, c.canonical_id());
        REQUIRE(found.has_value());
        CHECK(found->id == c.id);
    }
}

TEST_CASE("aliases, remapping, card backs and variants", "[deck]")
{
    auto const result = load_deck(fixture("aliased-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    CHECK(d.display_suit_name(suit::wands) == "Staves");
    CHECK(d.display_suit_name(suit::pentacles) == "Disks");
    CHECK(d.display_suit_name(suit::cups) == "Cups");  // no alias defined

    CHECK(d.display_rank_name(rank::page) == "Princess");
    CHECK(d.display_rank_name(rank::knight) == "Prince");
    CHECK(d.display_rank_name(rank::king) == "King");  // no alias defined

    // A key this deck does not define at all still renders
    CHECK(d.display_suit_name("shooting_stars") == "Shooting Stars");

    REQUIRE(d.major_arcana_remap.contains(8));
    CHECK(d.major_arcana_remap.at(8) == "justice");
    REQUIRE(d.major_arcana_remap.contains(11));
    CHECK(d.major_arcana_remap.at(11) == "strength");

    REQUIRE(d.default_card_back.has_value());
    CHECK(*d.default_card_back == "classic");

    // One declared plus one discovered
    REQUIRE(d.card_backs.size() == 2);
    CHECK(d.card_backs.front().name == "Classic Back");

    REQUIRE(d.variants.size() == 1);
    CHECK(d.variants.front().id == "aliased-deck-standard");
    CHECK(d.variants.front().card_back.value_or("") == "classic");
}

TEST_CASE("file-location-based defaults", "[deck]")
{
    auto const result = load_deck(fixture("file-location-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    auto const fool = find(d, "major_arcana.00");
    REQUIRE(fool.has_value());
    REQUIRE_FALSE(fool->images.empty());
    CHECK(fool->images.front().source_dir == "h1200");
    CHECK(fool->images.front().height == 1200);
    CHECK(fool->images.front().kind == image_kind::raster);
    CHECK_FALSE(fool->images.front().lines.has_value());

    auto const ace_of_wands = find(d, "minor_arcana.wands.ace");
    REQUIRE(ace_of_wands.has_value());
    REQUIRE_FALSE(ace_of_wands->images.empty());
    CHECK(ace_of_wands->images.front().source_dir == "h1200");
}

TEST_CASE("cards carry display-ready suit and rank", "[deck]")
{
    auto const result = load_deck(fixture("aliased-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    auto const page_of_wands = find(d, "minor_arcana.wands.page");
    REQUIRE(page_of_wands.has_value());
    CHECK(page_of_wands->display_rank == "Princess");
    CHECK(page_of_wands->display_suit == "Staves");

    // A rank with no alias falls back to its canonical form
    auto const king_of_cups = find(d, "minor_arcana.cups.king");
    REQUIRE(king_of_cups.has_value());
    CHECK(king_of_cups->display_rank == "King");
    CHECK(king_of_cups->display_suit == "Cups");

    // Majors have no suit or rank
    auto const fool = find(d, "major_arcana.00");
    REQUIRE(fool.has_value());
    CHECK(fool->display_suit.empty());
    CHECK(fool->display_rank.empty());
    REQUIRE(fool->number.has_value());
    CHECK(*fool->number == 0);
}

TEST_CASE("[remap_major_arcana] moves display positions, not canonical ids", "[deck]")
{
    auto const result = load_deck(fixture("aliased-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    // The fixture puts justice at 8 and strength at 11
    auto const strength = find(d, "major_arcana.08");
    REQUIRE(strength.has_value());
    CHECK(strength->display_name == "Strength");
    REQUIRE(strength->number.has_value());
    CHECK(*strength->number == 11);

    auto const justice = find(d, "major_arcana.11");
    REQUIRE(justice.has_value());
    CHECK(justice->display_name == "Justice");
    REQUIRE(justice->number.has_value());
    CHECK(*justice->number == 8);

    // other cards keep their canonical position
    auto const tower = find(d, "major_arcana.16");
    REQUIRE(tower.has_value());
    CHECK(*tower->number == 16);

    // A deck with no [remap_major_arcana] is the identity case.
    auto const plain = load_deck(fixture("custom-suit-deck"));
    REQUIRE(plain.has_value());
    CHECK(*find(*plain, "major_arcana.08")->number == 8);
}

TEST_CASE("custom cards get display strings and a declared position", "[deck]")
{
    auto const result = load_deck(fixture("custom-suit-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    auto const stars_ace = find(d, "minor_arcana.stars.ace");
    REQUIRE(stars_ace.has_value());
    CHECK(stars_ace->display_suit == "Stars");
    CHECK(stars_ace->display_rank == "Ace");

    auto const squirrel = find(d, "major_arcana.happy_squirrel");
    REQUIRE(squirrel.has_value());
    REQUIRE(squirrel->number.has_value());
    CHECK(*squirrel->number == 22);
    CHECK(squirrel->display_suit.empty());
}

TEST_CASE("canonical suits in order with customs at the end", "[deck]")
{
    auto const result = load_deck(fixture("custom-suit-deck"));
    REQUIRE(result.has_value());
    auto const suits = result->suits();

    REQUIRE(suits.size() == 5);
    CHECK(suits[0].key == "wands");
    CHECK(suits[1].key == "cups");
    CHECK(suits[2].key == "swords");
    CHECK(suits[3].key == "pentacles");

    CHECK(suits[4].key == "stars");

    CHECK(suits[0].standard);
    CHECK_FALSE(suits[4].standard);
    CHECK(suits[4].display_name == "Stars");
    CHECK(std::ranges::none_of(suits, &suit_info::excluded));
}

TEST_CASE("suits() uses aliases", "[deck]")
{
    auto const result = load_deck(fixture("aliased-deck"));
    REQUIRE(result.has_value());
    auto const suits = result->suits();

    REQUIRE(suits.size() == 4);
    CHECK(suits[0].display_name == "Staves");
    CHECK(suits[3].display_name == "Disks");
}

TEST_CASE("partly-excluded suit", "[deck]")
{
    // excluded-deck drops two pentacles court cards
    auto const result = load_deck(fixture("excluded-deck"));
    REQUIRE(result.has_value());

    auto const suits = result->suits();
    auto const pentacles = std::ranges::find(suits, std::string("pentacles"), &suit_info::key);
    REQUIRE(pentacles != suits.end());
    CHECK_FALSE(pentacles->excluded);
    CHECK(result->cards_in_suit("pentacles").size() == 12);
}

TEST_CASE("cards_of_kind and cards_in_suit partition the deck", "[deck]")
{
    auto const result = load_deck(fixture("custom-suit-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    // 22 standard majors plus the happy squirrel
    CHECK(d.cards_of_kind(arcana_kind::major_arcana).size() == 23);
    CHECK(d.cards_of_kind(arcana_kind::minor_arcana).size() == 58);

    CHECK(d.cards_in_suit("wands").size() == 14);
    CHECK(d.cards_in_suit("stars").size() == 2);
    CHECK(d.cards_in_suit("no_such_suit").empty());
}

TEST_CASE("random_card", "[deck]")
{
    auto const result = load_deck(fixture("aliased-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    auto const first = d.random_card(12345);
    REQUIRE(first.has_value());
    CHECK(d.random_card(12345)->canonical_id() == first->canonical_id());
    CHECK(d.find_card(first->id).has_value());

    // Different seeds
    bool differs = false;
    for (std::uint64_t seed = 0; seed < 20 && !differs; ++seed)
        differs = d.random_card(seed)->canonical_id() != first->canonical_id();

    CHECK(differs);

    CHECK_FALSE(deck{}.random_card(1).has_value());
}

TEST_CASE("undeclared card backs ones are discovered", "[deck]")
{
    auto const result = load_deck(fixture("aliased-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    // alternative.png has no deck.toml entry
    REQUIRE(d.card_backs.size() == 2);

    auto const classic =
        std::ranges::find(d.card_backs, std::string("classic"), &card_back_variant::id);

    REQUIRE(classic != d.card_backs.end());
    CHECK(classic->declared);
    CHECK(classic->name == "Classic Back");
    CHECK(classic->image_ref == "card_backs/classic.png");
    CHECK(classic->image.is_absolute());
    CHECK(std::filesystem::exists(classic->image));

    auto const alternative =
        std::ranges::find(d.card_backs, std::string("alternative"), &card_back_variant::id);

    REQUIRE(alternative != d.card_backs.end());
    CHECK_FALSE(alternative->declared);
    CHECK(alternative->image_ref.empty());
    CHECK(std::filesystem::exists(alternative->image));

    auto const chosen = d.default_card_back_variant();
    REQUIRE(chosen.has_value());
    CHECK(chosen->id == "classic");

    CHECK_FALSE(deck{}.default_card_back_variant().has_value());
}

TEST_CASE("custom card images keep the raw reference and resolve it", "[deck]")
{
    auto const result = load_deck(fixture("custom-suit-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    REQUIRE(d.custom_major_cards.size() == 1);
    auto const& squirrel = d.custom_major_cards.front();
    CHECK(squirrel.image_ref == "scalable/major_arcana/happy_squirrel.svg");
    CHECK(squirrel.image.is_absolute());
    CHECK(squirrel.image == d.root_path / squirrel.image_ref);

    REQUIRE(d.custom_suits.size() == 1);
    REQUIRE_FALSE(d.custom_suits.front().cards.empty());
    CHECK_FALSE(d.custom_suits.front().cards.front().image.empty());
}

TEST_CASE("unknown keys and unknown sections survive a load", "[deck]")
{
    auto const result = load_deck(fixture("unknown-keys-deck"));
    REQUIRE(result.has_value());

    auto const source = result->source_toml();
    CHECK(source.find("future_v11_field") != std::string::npos);
    CHECK(source.find("future_v11_section") != std::string::npos);
    CHECK(source.find("nested_key") != std::string::npos);

    CHECK(source.find("unknown-keys-deck") != std::string::npos);

    // unfortunately, comments are stripped...
    CHECK(source.find("standing in for a deck") == std::string::npos);

    CHECK(deck{}.source_toml().empty());
}

TEST_CASE("rider-waite-smith enumerates all 78 standard cards", "[deck][reference-decks]")
{
    auto const result = load_deck(reference_deck("rider-waite-smith"));
    REQUIRE(result.has_value());
    CHECK(result->cards.size() == 78);
}

TEST_CASE("ascii-tarot resolves ansi32 card images", "[deck][reference-decks]")
{
    auto const result = load_deck(reference_deck("ascii-tarot"));
    REQUIRE(result.has_value());

    auto const fool = find(*result, "major_arcana.00");
    REQUIRE(fool.has_value());
    auto const ansi =
        std::ranges::find(fool->images, std::string("ansi32"), &card_image::source_dir);
    REQUIRE(ansi != fool->images.end());
    CHECK(ansi->kind == image_kind::ansi);
    CHECK(ansi->lines == 32);

    REQUIRE(fool->best_ansi_for_lines(40).has_value());
    CHECK(fool->best_ansi_for_lines(40)->lines == 32);
    CHECK_FALSE(fool->best_raster_for_height(1200).has_value());
    CHECK_FALSE(fool->scalable_image().has_value());
}

TEST_CASE("aquatic-tarot resolves raster heights", "[deck][reference-decks]")
{
    auto const result = load_deck(reference_deck("aquatic-tarot"));
    REQUIRE(result.has_value());

    auto const fool = find(*result, "major_arcana.00");
    REQUIRE(fool.has_value());
    CHECK_FALSE(fool->images.empty());

    auto const raster = fool->best_raster_for_height(1200);
    REQUIRE(raster.has_value());
    CHECK(raster->height == 800);
    CHECK(raster->source_dir == "h800");
}
