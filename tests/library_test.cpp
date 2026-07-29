// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/library.hpp>
#include <arcana/paths.hpp>

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

}  // namespace

TEST_CASE("enumerate_decks finds every deck under the library root", "[library]")
{
    auto const decks = enumerate_decks({primary_root()});
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

TEST_CASE("load_deck_by_name loads by directory name, not by [deck].id", "[library]")
{
    auto const result = load_deck_by_name("deck-one", {primary_root()});
    REQUIRE(result.has_value());
    CHECK(result->metadata.id == "deck-one-id");

    auto const result2 = load_deck_by_name("deck-one-id", {primary_root()});
    REQUIRE(!result2.has_value());
}

TEST_CASE("load_deck_by_name fails for a directory that doesn't exist", "[library]")
{
    auto const result = load_deck_by_name("no-such-deck", {primary_root()});
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == error_code::not_found);
}

TEST_CASE("enumerate_decks is empty under a root with no deck library", "[library]")
{
    CHECK(enumerate_decks({"/fake/root"}).empty());
}

TEST_CASE("enumerate_decks searches several roots, in order", "[library]")
{
    // a consumer whose decks do not all live under one XDG path.
    auto const decks = enumerate_decks({primary_root(), alt_root()});

    // deck-one and deck-two from the first root, deck-three from the second.
    // deck-two exists in both and should be listed once
    REQUIRE(decks.size() == 3);
    CHECK(std::ranges::count(decks, std::string("deck-two"), &deck_summary::directory_name) == 1);

    auto const three =
        std::ranges::find(decks, std::string("deck-three"), &deck_summary::directory_name);

    REQUIRE(three != decks.end());
    CHECK(three->id == "deck-three-id");
}

TEST_CASE("an earlier root shadows a later one", "[library]")
{
    // Roots are a search path like PATH
    auto const first_wins = enumerate_decks({primary_root(), alt_root()});
    auto const two =
        std::ranges::find(first_wins, std::string("deck-two"), &deck_summary::directory_name);
    REQUIRE(two != first_wins.end());
    CHECK(two->id == "deck-two-id");

    auto const reversed = enumerate_decks({alt_root(), primary_root()});
    auto const shadowed =
        std::ranges::find(reversed, std::string("deck-two"), &deck_summary::directory_name);
    REQUIRE(shadowed != reversed.end());
    CHECK(shadowed->id == "deck-two-shadowed-id");

    CHECK(
        load_deck_by_name("deck-two", {primary_root(), alt_root()})->metadata.id == "deck-two-id"
    );

    CHECK(
        load_deck_by_name("deck-two", {alt_root(), primary_root()})->metadata.id ==
        "deck-two-shadowed-id"
    );
}

TEST_CASE("an empty root list falls back to the XDG deck library", "[library]")
{
    auto const defaulted = enumerate_decks();
    auto const explicit_xdg = enumerate_decks({paths::deck_library_path()});
    CHECK(defaulted.size() == explicit_xdg.size());
}
