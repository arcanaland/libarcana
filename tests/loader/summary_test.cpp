// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "temp_dir.hpp"

#include <document.hpp>
#include <summary.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

using arcana::detail::load_deck_document;
using arcana::detail::load_deck_summary;

TEST_CASE("a summary has the directory's name and path", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
id = "com.example.rider"
name = "Rider-Waite"
)");

    auto const summary = load_deck_summary(deck.path());

    REQUIRE(summary.has_value());
    CHECK(summary->id == "com.example.rider");
    CHECK(summary->name == "Rider-Waite");
    CHECK(summary->directory_name == deck.path().filename().string());
    CHECK(summary->path == deck.path());
}

TEST_CASE("missing summary fields read as empty", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
version = "1.0"
)");

    auto const summary = load_deck_summary(deck.path());

    REQUIRE(summary.has_value());
    CHECK(summary->id.empty());
    CHECK(summary->name.empty());
    CHECK_FALSE(summary->author.has_value());
    CHECK_FALSE(summary->icon.has_value());
}

TEST_CASE("an icon is resolved against the deck directory", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
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
name = "Rider-Waite"
icon = ""
)");

    REQUIRE(load_deck_summary(deck.path()).has_value());
    CHECK_FALSE(load_deck_summary(deck.path())->icon.has_value());
}

TEST_CASE("the card count comes from the manifest alone", "[summary]")
{
    SECTION("a deck that says nothing has the standard 78")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
name = "Plain"
)");

        CHECK(load_deck_summary(deck.path())->card_count == 78);
    }

    SECTION("exclusions")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
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
name = "Confused"

[deck.excluded_cards]
cards = ["major_arcana.00", "major_arcana.00", "minor_arcana.stars.ace", "nonsense"]
)");

        CHECK(load_deck_summary(deck.path())->card_count == 77);
    }
}

TEST_CASE("the card count agrees with load_deck", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
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
