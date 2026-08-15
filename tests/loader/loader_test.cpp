// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "temp_dir.hpp"

#include <loader.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using arcana::detail::deck_document;
using arcana::detail::deck_loader;

namespace
{

std::filesystem::path const no_deck_dir{"/nonexistent/tarot-deck"};

arcana::deck build_from(
    std::string_view toml_text, std::filesystem::path const& root = no_deck_dir,
    std::vector<std::string> const& languages = {}
)
{
    auto parsed = toml::parse(toml_text);
    REQUIRE(parsed);

    auto document = std::make_shared<deck_document>(std::move(parsed).table());
    REQUIRE(document->table["deck"].as_table() != nullptr);

    return deck_loader{root, std::move(document), languages}.build();
}

arcana::card const& card_named(arcana::deck const& d, std::string_view canonical_id)
{
    auto const it = std::ranges::find_if(
        d.cards, [canonical_id](arcana::card const& c) { return c.canonical_id() == canonical_id; }
    );
    REQUIRE(it != d.cards.end());
    return *it;
}

}  // namespace

TEST_CASE("a minimal deck yields the 78 standard cards", "[loader]")
{
    auto const d = build_from(R"([deck]
id = "minimal"
name = "Minimal"
)");

    // 1.0's [deck].id is a library handle, not an identifier, and is dropped
    CHECK_FALSE(d.metadata.identifier.has_value());
    CHECK(d.metadata.name == "Minimal");
    CHECK(d.cards.size() == 78);
    CHECK(d.cards_of_kind(arcana::arcana_kind::major_arcana).size() == 22);
    CHECK(d.cards_of_kind(arcana::arcana_kind::minor_arcana).size() == 56);
}

TEST_CASE("aspect_ratio falls back when absent or the wrong type", "[loader]")
{
    SECTION("absent")
    {
        auto const d = build_from(R"([deck]
name = "n"
)");
        CHECK(d.metadata.aspect_ratio == arcana::default_aspect_ratio);
    }

    SECTION("wrong type is not an error")
    {
        auto const d = build_from(R"([deck]
name = "n"
aspect_ratio = "wide"
)");
        CHECK(d.metadata.aspect_ratio == arcana::default_aspect_ratio);
    }

    SECTION("declared")
    {
        auto const d = build_from(R"([deck]
name = "n"
aspect_ratio = 0.6
)");
        CHECK(d.metadata.aspect_ratio == 0.6);
    }
}

TEST_CASE("optional metadata stays nullopt when the key is missing", "[loader]")
{
    auto const d = build_from(R"([deck]
name = "n"
creator = "Arthur"
artist = "Pamela"
tags = ["classic", 7, "rider"]
)");

    CHECK(d.metadata.creator == "Arthur");
    CHECK(d.metadata.artist == "Pamela");
    CHECK_FALSE(d.metadata.license.has_value());
    CHECK_FALSE(d.metadata.website.has_value());

    // A non-string element is skipped
    CHECK(d.metadata.tags == std::vector<std::string>{"classic", "rider"});
}

TEST_CASE("excluded cards are dropped and carry the deck's reason", "[loader]")
{
    auto const d = build_from(R"([deck]
name = "n"

[deck.excluded_cards]
cards = ["major_arcana.00", "minor_arcana.cups.ace"]
reason = "not in this printing"
)");

    CHECK(d.cards.size() == 76);
    CHECK_FALSE(d.find_card(arcana::card_id::standard_major(0)).has_value());
    CHECK(d.exclusion_reason("major_arcana.00") == "not in this printing");
    CHECK_FALSE(d.exclusion_reason("major_arcana.01").has_value());
}

TEST_CASE("aliases rename suits and courts on every affected card", "[loader]")
{
    auto const d = build_from(R"([deck]
name = "n"

[aliases.suits]
pentacles = "Coins"

[aliases.courts]
page = "Princess"
)");

    auto const& card = card_named(d, "minor_arcana.pentacles.page");
    CHECK(card.display_suit == "Coins");
    CHECK(card.display_rank == "Princess");

    // An unaliased suit is title-cased from its canonical key
    CHECK(card_named(d, "minor_arcana.wands.ace").display_suit == "Wands");
}

TEST_CASE("remap_major_arcana moves display position but not canonical id", "[loader]")
{
    auto const d = build_from(R"([deck]
name = "n"

[remap_major_arcana]
8 = "Justice"
11 = "Strength"
)");

    // Strength keeps major_arcana.08 as its id while sitting at 11
    auto const& strength = card_named(d, "major_arcana.08");
    CHECK(strength.display_name == "Strength");
    CHECK(strength.position == 11);

    auto const& justice = card_named(d, "major_arcana.11");
    CHECK(justice.display_name == "Justice");
    CHECK(justice.position == 8);

    // Untouched majors keep their ordinal
    CHECK(card_named(d, "major_arcana.00").position == 0);

    // 1.0 declares no face number anywhere
    CHECK_FALSE(strength.number.has_value());
}

TEST_CASE("an unparseable remap key is skipped rather than failing the load", "[loader]")
{
    auto const d = build_from(R"([deck]
name = "n"

[remap_major_arcana]
"eight" = "Justice"
"8x" = "Strength"
11 = "Strength"
)");

    CHECK(card_named(d, "major_arcana.08").position == 11);

    // The two unparseable keys moved nothing
    CHECK(card_named(d, "major_arcana.11").position == 11);
}

TEST_CASE("custom major arcana default their id to the table key", "[loader]")
{
    auto const d = build_from(R"([deck]
name = "n"

[custom_cards.major_arcana.the_void]
name = "The Void"
position = 22

[custom_cards.major_arcana.explicit_id]
id = "the_well"
name = "The Well"
)");

    CHECK(d.cards.size() == 78 + 2);

    auto const& void_card = card_named(d, "major_arcana.the_void");
    CHECK(void_card.display_name == "The Void");
    CHECK(void_card.position == 22);

    auto const& well = card_named(d, "major_arcana.the_well");
    CHECK(well.display_name == "The Well");
    CHECK_FALSE(well.position.has_value());
}

TEST_CASE("a custom suit contributes cards and a suit_info entry", "[loader]")
{
    auto const d = build_from(R"([deck]
name = "n"

[custom_cards.minor_arcana.stars]
name = "Stars"
cards = [
  { id = "ace", name = "Ace of Stars" },
  { id = "two", name = "Two of Stars" },
]
)");

    CHECK(d.cards.size() == 80);

    auto const in_suit = d.cards_in_suit("stars");
    REQUIRE(in_suit.size() == 2);
    CHECK(in_suit.front().display_name == "Ace of Stars");
    CHECK(in_suit.front().display_suit == "Stars");

    auto const& suits = d.suits;
    REQUIRE(suits.size() == 5);
    CHECK(suits.back().key == "stars");
    CHECK(suits.back().name == "Stars");
    CHECK(suits.back().ranks == std::vector<std::string>{"ace", "two"});
    CHECK_FALSE(suits.back().standard);
    CHECK_FALSE(suits.back().excluded);

    // A canonical suit carries the canonical fourteen
    CHECK(suits.front().ranks.size() == 14);
}

TEST_CASE("card back designs are read from the deck.toml", "[loader]")
{
    auto const d = build_from(R"([deck]
name = "n"

[card_backs]
default = "plain"

[card_backs.variants.plain]
name = "Plain"
image = "card_backs/plain.png"
alt_text = "a plain back"

[card_backs.variants.ornate]
name = "Ornate"
)");

    REQUIRE(d.card_backs.size() == 2);
    CHECK(d.default_card_back == "plain");

    auto const chosen = d.default_card_back_design();
    REQUIRE(chosen.has_value());
    CHECK(chosen->name == "Plain");
    CHECK(chosen->declared);
    CHECK(chosen->image_ref == "card_backs/plain.png");
    CHECK(chosen->image == no_deck_dir / "card_backs/plain.png");

    // A design with no image declared gets no resolved path
    auto const ornate =
        std::ranges::find(d.card_backs, std::string{"ornate"}, &arcana::card_back_design::id);

    REQUIRE(ornate != d.card_backs.end());
    CHECK(ornate->image.empty());
}

TEST_CASE("keys no parser reads survive into source_toml", "[loader]")
{
    auto const d = build_from(R"([deck]
name = "n"
some_future_field = "keep me"

[a_future_section]
nested = true
)");

    auto const round_tripped = d.source_toml();
    CHECK(round_tripped.find("some_future_field") != std::string::npos);
    CHECK(round_tripped.find("a_future_section") != std::string::npos);
}

// --- assets, which need a real directory ----------------------------------------

TEST_CASE("image roots on disk are classified by their directory name", "[loader]")
{
    arcana_test::temp_dir deck;
    deck.write("scalable/major_arcana/00.svg");
    deck.write("h1200/major_arcana/00.png");
    deck.write("ansi32/major_arcana/00.txt");
    deck.write("notanimageroot/major_arcana/00.png");

    auto const d = build_from(
        R"([deck]
name = "n"
)",
        deck.path()
    );

    auto const& fool = card_named(d, "major_arcana.00");
    REQUIRE(fool.images.size() == 3);

    auto const scalable = fool.scalable_image();
    REQUIRE(scalable.has_value());
    CHECK(scalable->source_dir == "scalable");

    auto const raster = fool.best_raster_for_height(1200);
    REQUIRE(raster.has_value());
    CHECK(raster->kind == arcana::image_kind::raster);
    CHECK(raster->height == 1200);

    auto const ansi = fool.best_ansi_for_lines(32);
    REQUIRE(ansi.has_value());
    CHECK(ansi->kind == arcana::image_kind::ansi);
    CHECK(ansi->lines == 32);
}

TEST_CASE("card backs on disk are discovered and sorted after declared ones", "[loader]")
{
    arcana_test::temp_dir deck;
    deck.write("card_backs/plain.png");
    deck.write("card_backs/zeta.png");
    deck.write("card_backs/alpha.png");

    auto const d = build_from(
        R"([deck]
name = "n"

[card_backs.variants.plain]
name = "Plain"
)",
        deck.path()
    );

    REQUIRE(d.card_backs.size() == 3);

    CHECK(d.card_backs[0].id == "plain");
    CHECK(d.card_backs[0].name == "Plain");
    CHECK(d.card_backs[0].declared);

    CHECK(d.card_backs[1].id == "alpha");
    CHECK_FALSE(d.card_backs[1].declared);
    CHECK(d.card_backs[2].id == "zeta");
}

TEST_CASE("a names file overrides display names and alt text", "[loader]")
{
    arcana_test::temp_dir deck;
    deck.write("names/en.toml", R"([major_arcana]
00 = "Le Mat"

[minor_arcana.cups]
ace = "As de Coupe"

[alt_text]
00 = "a wanderer"
)");

    auto const d = build_from(
        R"([deck]
name = "n"
)",
        deck.path()
    );

    auto const& fool = card_named(d, "major_arcana.00");
    CHECK(fool.display_name == "Le Mat");
    CHECK(fool.alt_text == "a wanderer");

    CHECK(card_named(d, "minor_arcana.cups.ace").display_name == "As de Coupe");

    // Anything the names file omits keeps its canonical name
    CHECK(card_named(d, "major_arcana.01").display_name == "The Magician");
    CHECK_FALSE(card_named(d, "major_arcana.01").alt_text.has_value());
}

TEST_CASE("a custom card's declared alt_text is used only when names has none", "[loader]")
{
    arcana_test::temp_dir deck;
    deck.write("names/en.toml", R"([alt_text]
the_void = "from the catalog"
)");

    auto const d = build_from(
        R"([deck]
name = "n"

[custom_cards.major_arcana.the_void]
name = "The Void"
alt_text = "from deck.toml"

[custom_cards.major_arcana.the_well]
name = "The Well"
alt_text = "from deck.toml"

[custom_cards.major_arcana.the_gate]
name = "The Gate"
)",
        deck.path()
    );

    CHECK(card_named(d, "major_arcana.the_void").alt_text == "from the catalog");
    CHECK(card_named(d, "major_arcana.the_well").alt_text == "from deck.toml");

    // No alt text anywhere means none, not an empty string
    CHECK_FALSE(card_named(d, "major_arcana.the_gate").alt_text.has_value());
}
