// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/library.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>

using namespace arcana;

namespace
{

std::filesystem::path fixture_root()
{
    return std::filesystem::path(FIXTURES_DIR) / "library-root";
}

}  // namespace

TEST_CASE("enumerate_decks finds every deck under the library root", "[library]")
{
    auto const decks = enumerate_decks(fixture_root());
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
    auto const result = load_deck_by_name("deck-one", fixture_root());
    REQUIRE(result.has_value());
    CHECK(result->metadata.id == "deck-one-id");
}

TEST_CASE("load_deck_by_name fails for a directory that doesn't exist", "[library]")
{
    auto const result = load_deck_by_name("no-such-deck", fixture_root());
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("enumerate_decks is empty under a root with no deck library", "[library]")
{
    CHECK(enumerate_decks("/fake/root/with/nothing").empty());
}
