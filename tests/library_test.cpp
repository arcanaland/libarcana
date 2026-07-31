// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/library.hpp>
#include <arcana/paths.hpp>

#include "temp_dir.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <vector>

using namespace arcana;

namespace
{

// A home-shaped fake root
std::filesystem::path primary_root()
{
    return paths::deck_library_path(std::filesystem::path(FIXTURES_DIR) / "library-root");
}

// A second library that is not under any XDG path
std::filesystem::path alt_root()
{
    return std::filesystem::path(FIXTURES_DIR) / "library-root-alt";
}

deck_library open_library(std::vector<std::filesystem::path> roots)
{
    return deck_library{library_options{.roots = std::move(roots)}};
}

}  // namespace

TEST_CASE("decks() lists every readable deck under the library root", "[library]")
{
    auto const lib = open_library({primary_root()});
    auto const& decks = lib.decks();
    REQUIRE(decks.size() == 2);

    auto const one =
        std::ranges::find(decks, std::string("deck-one"), &deck_summary::directory_name);

    REQUIRE(one != decks.end());
    CHECK(one->id == "deck-one-id");
    CHECK(one->name == "Deck One");

    auto const two =
        std::ranges::find(decks, std::string("deck-two"), &deck_summary::directory_name);

    REQUIRE(two != decks.end());
    CHECK(two->id == "deck-two-id");
}

TEST_CASE("both lists are sorted by directory name", "[library]")
{
    auto const lib = open_library({primary_root(), alt_root()});
    CHECK(std::ranges::is_sorted(lib.decks(), {}, &deck_summary::directory_name));
    CHECK(std::ranges::is_sorted(lib.broken_decks(), {}, &broken_deck::directory_name));
}

TEST_CASE("an unreadable deck is reported, not dropped", "[library]")
{
    auto const lib = open_library({primary_root()});

    REQUIRE(lib.broken_decks().size() == 1);

    auto const& broken = lib.broken_decks().front();
    CHECK(broken.directory_name == "deck-broken");
    CHECK(broken.path.filename() == "deck-broken");
    CHECK(broken.problem.code == error_code::parse_error);
    CHECK(broken.problem.message.find("deck.toml") != std::string::npos);

    // It is absent from the readable list: the two lists partition the directories
    CHECK(
        std::ranges::find(lib.decks(), std::string("deck-broken"), &deck_summary::directory_name) ==
        lib.decks().end()
    );
}

TEST_CASE("load() reports a broken deck's parse error, not not_found", "[library]")
{
    auto const lib = open_library({primary_root()});

    // The whole reason resolution matches on directory name across broken decks: the
    // deck is installed, and saying why it will not load beats claiming it is missing.
    auto const result = lib.load("deck-broken");
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == error_code::parse_error);

    // find() cannot answer this -- it has no summary to hand back
    CHECK_FALSE(lib.find("deck-broken").has_value());
}

TEST_CASE("load() loads by directory name, not by [deck].id", "[library]")
{
    auto const lib = open_library({primary_root()});

    auto const result = lib.load("deck-one");
    REQUIRE(result.has_value());
    CHECK(result->metadata.id == "deck-one-id");

    CHECK_FALSE(lib.load("deck-one-id").has_value());
}

TEST_CASE("load() fails with not_found for a directory that doesn't exist", "[library]")
{
    auto const lib = open_library({primary_root()});

    auto const result = lib.load("no-such-deck");
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == error_code::not_found);
}

TEST_CASE("find() and find_by_id() look a deck up two ways", "[library]")
{
    auto const lib = open_library({primary_root()});

    auto const by_directory = lib.find("deck-one");
    REQUIRE(by_directory.has_value());
    CHECK(by_directory->id == "deck-one-id");

    auto const by_id = lib.find_by_id("deck-one-id");
    REQUIRE(by_id.has_value());
    CHECK(by_id->directory_name == "deck-one");

    // The two keys are not interchangeable
    CHECK_FALSE(lib.find("deck-one-id").has_value());
    CHECK_FALSE(lib.find_by_id("deck-one").has_value());
    CHECK_FALSE(lib.find("no-such-deck").has_value());
}

TEST_CASE("load_path() loads a deck from outside every root", "[library]")
{
    auto const lib = open_library({primary_root()});

    // A CLI pointed at a checkout, or a reference deck shipped outside the library
    auto const result = lib.load_path(alt_root() / "deck-three");
    REQUIRE(result.has_value());
    CHECK(result->metadata.id == "deck-three-id");
}

TEST_CASE("a library with nothing installed is empty", "[library]")
{
    auto const lib = open_library({"/fake/root"});
    CHECK(lib.decks().empty());
    CHECK(lib.broken_decks().empty());
}

TEST_CASE("several roots are searched, in order", "[library]")
{
    // a consumer whose decks do not all live under one XDG path
    auto const lib = open_library({primary_root(), alt_root()});

    // deck-one and deck-two from the first root, deck-three from the second.
    // deck-two exists in both and is listed once
    REQUIRE(lib.decks().size() == 3);
    CHECK(
        std::ranges::count(lib.decks(), std::string("deck-two"), &deck_summary::directory_name) == 1
    );

    auto const three =
        std::ranges::find(lib.decks(), std::string("deck-three"), &deck_summary::directory_name);

    REQUIRE(three != lib.decks().end());
    CHECK(three->id == "deck-three-id");
}

TEST_CASE("an earlier root shadows a later one", "[library]")
{
    // Roots are a search path like PATH
    auto const first_wins = open_library({primary_root(), alt_root()});
    auto const two = std::ranges::find(
        first_wins.decks(), std::string("deck-two"), &deck_summary::directory_name
    );
    REQUIRE(two != first_wins.decks().end());
    CHECK(two->id == "deck-two-id");

    auto const reversed = open_library({alt_root(), primary_root()});
    auto const shadowed =
        std::ranges::find(reversed.decks(), std::string("deck-two"), &deck_summary::directory_name);
    REQUIRE(shadowed != reversed.decks().end());
    CHECK(shadowed->id == "deck-two-shadowed-id");

    CHECK(first_wins.load("deck-two")->metadata.id == "deck-two-id");
    CHECK(reversed.load("deck-two")->metadata.id == "deck-two-shadowed-id");
}

TEST_CASE("shadowing is decided before a deck is read", "[library]")
{
    // deck-broken is unreadable under the primary root and readable under the alt one.
    // Precedence goes by directory name alone, so the broken copy wins when it comes
    // first -- the way a broken executable earlier in PATH still shadows a working one.
    auto const broken_first = open_library({primary_root(), alt_root()});
    CHECK(broken_first.decks().size() == 3);
    REQUIRE(broken_first.broken_decks().size() == 1);
    CHECK(broken_first.broken_decks().front().directory_name == "deck-broken");
    CHECK_FALSE(broken_first.load("deck-broken").has_value());

    auto const readable_first = open_library({alt_root(), primary_root()});
    CHECK(readable_first.broken_decks().empty());

    auto const found = readable_first.find("deck-broken");
    REQUIRE(found.has_value());
    CHECK(found->id == "deck-broken-but-fine-here");
    CHECK(readable_first.load("deck-broken").has_value());
}

TEST_CASE("an empty root list falls back to the XDG deck library", "[library]")
{
    deck_library const defaulted{};

    // Resolved once, at construction, so roots() reports what is really searched
    CHECK(defaulted.roots() == std::vector{paths::deck_library_path()});

    auto const explicit_xdg = open_library({paths::deck_library_path()});
    CHECK(defaulted.decks().size() == explicit_xdg.decks().size());
}

TEST_CASE("the language belongs to the library, not to each load", "[library]")
{
    deck_library const lib{
        library_options{.roots = {primary_root()}, .language = std::string("fr")}
    };

    CHECK(lib.language() == std::string("fr"));
    CHECK(lib.load("deck-one").has_value());
}

TEST_CASE("refresh() picks up a deck installed after construction", "[library]")
{
    arcana_test::temp_dir const root;

    deck_library lib{library_options{.roots = {root.path()}}};
    CHECK(lib.decks().empty());

    root.write("late-deck/deck.toml", R"([deck]
id = "late-deck-id"
schema_version = "1.0"
name = "Late Deck"
version = "1.0"
)");

    // The scan is a snapshot: the new deck stays invisible until asked for
    CHECK(lib.decks().empty());

    lib.refresh();
    REQUIRE(lib.decks().size() == 1);
    CHECK(lib.decks().front().id == "late-deck-id");
}
