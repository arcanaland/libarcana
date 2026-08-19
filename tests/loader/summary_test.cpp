// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "temp_dir.hpp"

#include <document.hpp>
#include <summary.hpp>

#include <arcana/deck.hpp>
#include <arcana/error.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <filesystem>
#include <string>
#include <string_view>

using arcana::detail::load_deck_document;
using arcana::detail::load_deck_summary;

TEST_CASE("a summary has the directory's name and path", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
schema_version = "2.0"
identifier = "com.example/deck/rider"
name = "Rider-Waite"
)");

    auto const summary = load_deck_summary(deck.path());

    REQUIRE(summary.has_value());
    CHECK(summary->identifier == "com.example/deck/rider");
    CHECK(summary->name == "Rider-Waite");
    CHECK(summary->directory_name == deck.path().filename().string());
    CHECK(summary->path == deck.path());
}

TEST_CASE("missing summary fields read as empty", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
schema_version = "1.0"
version = "1.0"
)");

    auto const summary = load_deck_summary(deck.path());

    REQUIRE(summary.has_value());
    CHECK_FALSE(summary->identifier.has_value());
    CHECK(summary->name.empty());
    CHECK_FALSE(summary->artist.has_value());
    CHECK_FALSE(summary->icon.has_value());
}

TEST_CASE("an icon is resolved against the deck directory", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
schema_version = "2.0"
name = "Rider-Waite"
icon = "scalable/icon.svg"
)");

    auto const summary = load_deck_summary(deck.path());

    REQUIRE(summary.has_value());
    REQUIRE(summary->icon.has_value());
    CHECK(*summary->icon == deck.path() / "scalable/icon.svg");

    CHECK_FALSE(std::filesystem::exists(*summary->icon));
}

TEST_CASE("an empty icon reads as no icon at all", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
schema_version = "2.0"
name = "Rider-Waite"
icon = ""
)");

    REQUIRE(load_deck_summary(deck.path()).has_value());
    CHECK_FALSE(load_deck_summary(deck.path())->icon.has_value());
}

TEST_CASE("a 1.0 card count comes from the manifest alone", "[summary]")
{
    SECTION("a deck that says nothing has the standard 78")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
schema_version = "1.0"
name = "Plain"
)");

        CHECK(load_deck_summary(deck.path())->card_count == 78);
    }

    SECTION("exclusions")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
schema_version = "1.0"
name = "Trimmed"

[deck.excluded_cards]
cards = ["major_arcana.00", "minor_arcana.cups.ace", "minor_arcana.cups.two"]
)");

        CHECK(load_deck_summary(deck.path())->card_count == 75);
    }

    SECTION("custom cards")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
schema_version = "1.0"
name = "Extended"

[custom_cards.major_arcana.the_void]
name = "The Void"

[custom_cards.minor_arcana.stars]
name = "Stars"
cards = [{ id = "ace", name = "Ace of Stars" }, { id = "two", name = "Two of Stars" }]
)");

        CHECK(load_deck_summary(deck.path())->card_count == 81);
    }

    SECTION("an exclusion naming a card the deck never had does not go negative")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
schema_version = "1.0"
name = "Confused"

[deck.excluded_cards]
cards = ["major_arcana.00", "major_arcana.00", "minor_arcana.stars.ace", "nonsense"]
)");

        CHECK(load_deck_summary(deck.path())->card_count == 77);
    }
}

TEST_CASE("a 1.0 card count agrees with load_deck", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
schema_version = "1.0"
name = "Mixed"

[deck.excluded_cards]
cards = ["major_arcana.00", "minor_arcana.swords.king"]

[custom_cards.major_arcana.the_void]
name = "The Void"

[custom_cards.minor_arcana.stars]
name = "Stars"
cards = [{ id = "ace", name = "Ace of Stars" }]
)");

    auto const summary = load_deck_summary(deck.path());
    auto const loaded = arcana::load_deck(deck.path());

    REQUIRE(summary.has_value());
    REQUIRE(loaded.has_value());
    CHECK(summary->card_count == loaded->cards.size());
}

TEST_CASE("a summary needs a schema_version too", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
name = "Undeclared"
)");

    auto const summary = load_deck_summary(deck.path());

    REQUIRE_FALSE(summary.has_value());
    CHECK(summary.error().code == arcana::error_code::parse_error);
    CHECK(summary.error().message.find("schema_version is required") != std::string::npos);
}

TEST_CASE("a 2.0 card count comes from the directory tree", "[summary]")
{
    SECTION("a deck with no files at all still has the standard 78")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
schema_version = "2.0"
name = "Plain"
)");

        CHECK(load_deck_summary(deck.path())->card_count == 78);
    }

    SECTION("a file outside the 78 adds a card the manifest never mentions")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
schema_version = "2.0"
name = "Extended"
)");
        deck.write("scalable/major_arcana/22.svg");
        deck.write("scalable/major_arcana/happy_squirrel.svg");
        deck.write("scalable/minor_arcana/stars/ace.svg");

        CHECK(load_deck_summary(deck.path())->card_count == 81);
    }

    SECTION("variants of one card are one card")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
schema_version = "2.0"
name = "Varied"
)");
        deck.write("scalable/major_arcana/22.svg");
        deck.write("scalable/major_arcana/22.alternate.svg");
        deck.write("h800/major_arcana/22.png");

        CHECK(load_deck_summary(deck.path())->card_count == 79);
    }

    SECTION("2.0 reads a top-level [excluded_cards], not [deck.excluded_cards]")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
schema_version = "2.0"
name = "Trimmed"

[excluded_cards]
cards = ["major_arcana.00", "minor_arcana.cups.ace"]

[deck.excluded_cards]
cards = ["major_arcana.01"]
)");

        CHECK(load_deck_summary(deck.path())->card_count == 76);
    }

    SECTION("an exclusion naming a card the deck never had does not go negative")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
schema_version = "2.0"
name = "Confused"

[excluded_cards]
cards = ["major_arcana.00", "major_arcana.00", "minor_arcana.stars.ace", "nonsense"]
)");

        CHECK(load_deck_summary(deck.path())->card_count == 77);
    }

    SECTION("a file that names no card is not one")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
schema_version = "2.0"
name = "Littered"
)");
        deck.write("scalable/major_arcana/README.txt");
        deck.write("scalable/major_arcana/Not A Card.svg");
        deck.write("scalable/minor_arcana/Not A Suit/ace.svg");
        deck.write("scalable/card_backs/plain.svg");
        deck.write("icon.svg");

        CHECK(load_deck_summary(deck.path())->card_count == 78);
    }

    SECTION("a [cards] entry alone creates nothing")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
schema_version = "2.0"
name = "Wishful"

[cards."major_arcana.23"]
name = "The Twenty-Third"
)");

        CHECK(load_deck_summary(deck.path())->card_count == 78);
    }
}

TEST_CASE("a 2.0 card count agrees with load_deck", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
schema_version = "2.0"
name = "Mixed"

[excluded_cards]
cards = ["major_arcana.00", "minor_arcana.swords.king"]

[cards."major_arcana.happy_squirrel"]
position = 22
)");
    deck.write("scalable/major_arcana/happy_squirrel.svg");
    deck.write("scalable/major_arcana/06.two_women.svg");
    deck.write("scalable/minor_arcana/stars/ace.svg");
    deck.write("h800/minor_arcana/stars/two.png");
    deck.write("scalable/major_arcana/00.svg");

    auto const summary = load_deck_summary(deck.path());
    auto const loaded = arcana::load_deck(deck.path());

    REQUIRE(summary.has_value());
    REQUIRE(loaded.has_value());
    CHECK(summary->card_count == loaded->cards.size());
    CHECK(summary->card_count == 79);
}

TEST_CASE(
    "every reference deck's summary count agrees with load_deck", "[summary][reference-decks]"
)
{
    std::string const dir = REFERENCE_DECKS_DIR;
    if (dir.empty())
        SKIP("configured with ARCANA_FETCH_REFERENCE_DECKS=OFF");

    auto const name =
        GENERATE(as<std::string_view>{}, "rider-waite-smith", "ascii-tarot", "aquatic-tarot");
    CAPTURE(name);

    auto const path = std::filesystem::path{dir} / name;

    auto const summary = load_deck_summary(path);
    auto const loaded = arcana::load_deck(path);

    REQUIRE(summary.has_value());
    REQUIRE(loaded.has_value());
    CHECK(summary->card_count == loaded->cards.size());
}

TEST_CASE("a directory with no manifest", "[summary]")
{
    SECTION("no deck.toml at all")
    {
        arcana_test::temp_dir deck;
        CHECK_FALSE(load_deck_summary(deck.path()).has_value());
    }

    SECTION("a deck.toml that does not parse")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", "[deck\nname = broken");
        CHECK_FALSE(load_deck_summary(deck.path()).has_value());
    }

    SECTION("a deck.toml with no [deck] table")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([card_backs]
default = "plain"
)");
        CHECK_FALSE(load_deck_summary(deck.path()).has_value());
    }

    SECTION("a directory that is not there")
    {
        CHECK_FALSE(load_deck_summary("/nonexistent/arcana-deck").has_value());
    }
}

TEST_CASE("load_deck_document reports why a deck is unreadable", "[summary]")
{
    SECTION("a malformed file names itself in the error")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", "[deck\nname = broken");

        auto const document = load_deck_document(deck.path());

        REQUIRE_FALSE(document.has_value());
        CHECK(document.error().code == arcana::error_code::parse_error);
        CHECK(document.error().message.find("failed to parse") != std::string::npos);
        CHECK(document.error().message.find("deck.toml") != std::string::npos);
    }

    SECTION("a file with no [deck] table says so")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([card_backs]
default = "plain"
)");

        auto const document = load_deck_document(deck.path());

        REQUIRE_FALSE(document.has_value());
        CHECK(document.error().code == arcana::error_code::parse_error);
        CHECK(document.error().message.find("has no [deck] table") != std::string::npos);
    }
}
