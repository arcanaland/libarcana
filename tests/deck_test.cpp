// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/deck.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
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
    return d.find_card(canonical_id).has_value();
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

    auto const squirrel = d.find_card("major_arcana.happy_squirrel");
    REQUIRE(squirrel.has_value());
    CHECK(squirrel->display_name == "The Happy Squirrel");
    CHECK(squirrel->alt_text.value_or("") == "A cheerful squirrel gathering acorns.");
    CHECK(squirrel->id.is_custom());
    CHECK(squirrel->id.custom_id == "happy_squirrel");

    auto const stars_ace = d.find_card("minor_arcana.stars.ace");
    REQUIRE(stars_ace.has_value());
    CHECK(stars_ace->display_name == "Ace of Stars");
    CHECK(stars_ace->id.suit_key == "stars");
    CHECK(stars_ace->id.custom_id == "ace");

    CHECK(d.find_card("minor_arcana.stars.two").has_value());

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
    // One declared plus one discovered; the split is asserted in its own case below.
    REQUIRE(d.card_backs.size() == 2);
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

    auto const fool = d.find_card("major_arcana.00");
    REQUIRE(fool.has_value());
    REQUIRE_FALSE(fool->images.empty());
    CHECK(fool->images.front().variant_name == "h1200");
    CHECK(fool->images.front().height == 1200);

    auto const ace_of_wands = d.find_card("minor_arcana.wands.ace");
    REQUIRE(ace_of_wands.has_value());
    REQUIRE_FALSE(ace_of_wands->images.empty());
    CHECK(ace_of_wands->images.front().variant_name == "h1200");
}

TEST_CASE("cards carry display-ready suit and rank, resolved through aliases", "[deck]")
{
    auto const result = load_deck(fixture("aliased-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    // The point of F2: a display consumer reads a field. No branch on the card's class,
    // no second call back into the deck, no per-draw alias lookup.
    auto const page_of_wands = d.find_card("minor_arcana.wands.page");
    REQUIRE(page_of_wands.has_value());
    CHECK(page_of_wands->display_suit == "Staves");
    CHECK(page_of_wands->display_rank == "Princess");

    // A rank with no alias falls back to its canonical form, not to empty.
    auto const king_of_cups = d.find_card("minor_arcana.cups.king");
    REQUIRE(king_of_cups.has_value());
    CHECK(king_of_cups->display_suit == "cups");
    CHECK(king_of_cups->display_rank == "king");

    // Majors have no suit or rank, and carry their number instead.
    auto const fool = d.find_card("major_arcana.00");
    REQUIRE(fool.has_value());
    CHECK(fool->display_suit.empty());
    CHECK(fool->display_rank.empty());
    REQUIRE(fool->number.has_value());
    CHECK(*fool->number == 0);
}

TEST_CASE("custom cards get display strings and a declared position", "[deck]")
{
    auto const result = load_deck(fixture("custom-suit-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    auto const stars_ace = d.find_card("minor_arcana.stars.ace");
    REQUIRE(stars_ace.has_value());
    CHECK(stars_ace->display_suit == "Stars");  // from [custom_cards.minor_arcana.stars].name
    CHECK(stars_ace->display_rank == "ace");

    auto const squirrel = d.find_card("major_arcana.happy_squirrel");
    REQUIRE(squirrel.has_value());
    REQUIRE(squirrel->number.has_value());
    CHECK(*squirrel->number == 22);  // the declared `position`
    CHECK(squirrel->display_suit.empty());
}

TEST_CASE("suits() lists canonical suits in canonical order, customs appended", "[deck]")
{
    auto const result = load_deck(fixture("custom-suit-deck"));
    REQUIRE(result.has_value());
    auto const suits = result->suits();

    REQUIRE(suits.size() == 5);
    CHECK(suits[0].key == "wands");
    CHECK(suits[1].key == "cups");
    CHECK(suits[2].key == "swords");
    CHECK(suits[3].key == "pentacles");

    // Alphabetical order would put "stars" between pentacles and swords. It does not, and
    // that is the deck.py:412-423 bug this query exists to end.
    CHECK(suits[4].key == "stars");

    CHECK(suits[0].standard);
    CHECK_FALSE(suits[4].standard);
    CHECK(suits[4].display_name == "Stars");
    CHECK(std::ranges::none_of(suits, &suit_info::excluded));
}

TEST_CASE("suits() reports display names through aliases", "[deck]")
{
    auto const result = load_deck(fixture("aliased-deck"));
    REQUIRE(result.has_value());
    auto const suits = result->suits();

    REQUIRE(suits.size() == 4);
    CHECK(suits[0].display_name == "Staves");
    CHECK(suits[3].display_name == "Disks");
}

TEST_CASE("a partly-excluded suit is not marked excluded", "[deck]")
{
    // excluded-deck drops two pentacles court cards, not the suit.
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

    // 22 standard majors plus the happy squirrel.
    CHECK(d.cards_of_kind(arcana_kind::major_arcana).size() == 23);
    CHECK(d.cards_of_kind(arcana_kind::minor_arcana).size() == 58);

    CHECK(d.cards_in_suit("wands").size() == 14);
    CHECK(d.cards_in_suit("stars").size() == 2);
    CHECK(d.cards_in_suit("no_such_suit").empty());

    // A custom suit keyed by a canonical name is not what this looks up: "wands" always
    // means the canonical suit.
    for (auto const& c : d.cards_in_suit("wands")) CHECK(c.id.cls == card_class::standard_minor);
}

TEST_CASE("random_card is deterministic in its seed and owns no RNG state", "[deck]")
{
    auto const result = load_deck(fixture("aliased-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    auto const first = d.random_card(12345);
    REQUIRE(first.has_value());
    CHECK(d.random_card(12345)->canonical_id() == first->canonical_id());
    CHECK(d.find_card(first->id).has_value());

    // Different seeds should not all collapse to one card.
    bool differs = false;
    for (std::uint64_t seed = 0; seed < 20 && !differs; ++seed)
        differs = d.random_card(seed)->canonical_id() != first->canonical_id();
    CHECK(differs);

    CHECK_FALSE(deck{}.random_card(1).has_value());
}

TEST_CASE("card backs resolve to absolute paths and undeclared ones are discovered", "[deck]")
{
    auto const result = load_deck(fixture("aliased-deck"));
    REQUIRE(result.has_value());
    auto const& d = *result;

    // classic is declared; alternative.png sits in card_backs/ with no deck.toml entry.
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

    // Forward compatibility, which applies today: a deck authored against a future spec
    // version must not lose its fields just by passing through libarcana.
    auto const source = result->source_toml();
    CHECK(source.find("future_v11_field") != std::string::npos);
    CHECK(source.find("future_v11_section") != std::string::npos);
    CHECK(source.find("nested_key") != std::string::npos);

    // Known keys are still there too -- this is the whole document, not a leftovers bin.
    CHECK(source.find("unknown-keys-deck") != std::string::npos);

    // Measured limits of toml++ 3.4.0, asserted so a dependency bump that changes them
    // shows up here rather than in a future write layer: comments are not represented in
    // the node tree at all.
    CHECK(source.find("standing in for a deck") == std::string::npos);

    CHECK(deck{}.source_toml().empty());
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

    auto const fool = result->find_card("major_arcana.00");
    REQUIRE(fool.has_value());
    auto const ansi =
        std::ranges::find(fool->images, std::string("ansi32"), &image_variant::variant_name);
    REQUIRE(ansi != fool->images.end());
}

TEST_CASE("aquatic-tarot resolves raster heights", "[deck][reference-decks]")
{
    auto const result = load_deck(reference_deck("aquatic-tarot"));
    REQUIRE(result.has_value());

    auto const fool = result->find_card("major_arcana.00");
    REQUIRE(fool.has_value());
    CHECK_FALSE(fool->images.empty());
}
