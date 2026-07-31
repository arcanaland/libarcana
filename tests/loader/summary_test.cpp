// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Reading a deck directory's manifest without building its cards, and the shared
// validity rule that decides whether a directory holds a deck at all.

#include "temp_dir.hpp"

#include <document.hpp>
#include <summary.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

using arcana::detail::read_deck_document;
using arcana::detail::read_deck_summary;

TEST_CASE("a summary carries the directory's own name and path", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
id = "com.example.rider"
name = "Rider-Waite"
)");

    auto const summary = read_deck_summary(deck.path());

    REQUIRE(summary.has_value());
    CHECK(summary->id == "com.example.rider");
    CHECK(summary->name == "Rider-Waite");
    CHECK(summary->directory_name == deck.path().filename().string());
    CHECK(summary->path == deck.path());
}

TEST_CASE("missing summary fields read as empty rather than failing", "[summary]")
{
    arcana_test::temp_dir deck;
    deck.write("deck.toml", R"([deck]
version = "1.0"
)");

    auto const summary = read_deck_summary(deck.path());

    REQUIRE(summary.has_value());
    CHECK(summary->id.empty());
    CHECK(summary->name.empty());
}

TEST_CASE("a directory that does not hold a deck yields no summary", "[summary]")
{
    SECTION("no deck.toml at all")
    {
        arcana_test::temp_dir deck;
        CHECK_FALSE(read_deck_summary(deck.path()).has_value());
    }

    SECTION("a deck.toml that does not parse")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", "[deck\nname = broken");
        CHECK_FALSE(read_deck_summary(deck.path()).has_value());
    }

    SECTION("a deck.toml with no [deck] table")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([card_backs]
default = "plain"
)");
        CHECK_FALSE(read_deck_summary(deck.path()).has_value());
    }

    SECTION("a directory that is not there")
    {
        CHECK_FALSE(read_deck_summary("/nonexistent/arcana-deck").has_value());
    }
}

TEST_CASE("read_deck_document reports why a deck is unreadable", "[summary]")
{
    SECTION("a malformed file names itself in the error")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", "[deck\nname = broken");

        auto const document = read_deck_document(deck.path());

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

        auto const document = read_deck_document(deck.path());

        REQUIRE_FALSE(document.has_value());
        CHECK(document.error().code == arcana::error_code::parse_error);
        CHECK(document.error().message.find("has no [deck] table") != std::string::npos);
    }

    SECTION("a readable deck yields a document with a [deck] table")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
name = "Rider-Waite"
)");

        auto const document = read_deck_document(deck.path());

        REQUIRE(document.has_value());
        REQUIRE(*document != nullptr);
        CHECK((*document)->table["deck"].as_table() != nullptr);
    }
}

TEST_CASE("summary reading agrees with load_deck on what is a deck", "[summary]")
{
    // enumerate_decks() skips what load_deck() would reject, because both ask
    // read_deck_document() the same question.
    arcana_test::temp_dir good;
    good.write("deck.toml", R"([deck]
name = "Fine"
)");

    arcana_test::temp_dir bad;
    bad.write("deck.toml", R"([not_deck]
name = "Nope"
)");

    CHECK(read_deck_summary(good.path()).has_value());
    CHECK(arcana::load_deck(good.path()).has_value());

    CHECK_FALSE(read_deck_summary(bad.path()).has_value());
    CHECK_FALSE(arcana::load_deck(bad.path()).has_value());
}
