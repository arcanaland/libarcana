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
using namespace std::string_literals;

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

std::filesystem::path fixture(std::string const& name)
{
    return std::filesystem::path(FIXTURES_DIR) / name;
}

deck_library open_library(std::vector<std::filesystem::path> roots)
{
    return deck_library{library_options{.roots = std::move(roots)}};
}

}  // namespace

TEST_CASE("decks() lists every readable deck under the library root", "[library]")
{
    auto const lib = open_library({primary_root()});
    auto const decks = lib.decks();
    REQUIRE(decks.size() == 2);

    auto const one = std::ranges::find(decks, "deck-one"s, &deck_summary::directory_name);

    REQUIRE(one != decks.end());
    CHECK(one->id == "deck-one-id");
    CHECK(one->name == "Deck One");

    auto const two = std::ranges::find(decks, "deck-two"s, &deck_summary::directory_name);

    REQUIRE(two != decks.end());
    CHECK(two->id == "deck-two-id");
}

TEST_CASE("a deck declaring no author or icon leaves them empty", "[library]")
{
    auto const lib = open_library({primary_root()});

    auto const two = lib.find("deck-two");
    REQUIRE(two.has_value());

    CHECK_FALSE(two->author.has_value());
    CHECK_FALSE(two->icon.has_value());
    CHECK(two->card_count == 78);
}

TEST_CASE("both lists are sorted by directory name", "[library]")
{
    auto const lib = open_library({primary_root(), alt_root()});
    CHECK(std::ranges::is_sorted(lib.decks(), {}, &deck_summary::directory_name));
    CHECK(std::ranges::is_sorted(lib.malformed_decks(), {}, &malformed_deck::directory_name));
}

TEST_CASE("unreadable decks are reported", "[library]")
{
    auto const lib = open_library({primary_root()});

    REQUIRE(lib.malformed_decks().size() == 1);

    auto const& malformed = lib.malformed_decks().front();
    CHECK(malformed.directory_name == "deck-broken");
    CHECK(malformed.path.filename() == "deck-broken");
    CHECK(malformed.problem.code == error_code::parse_error);
    CHECK(malformed.problem.message.find("deck.toml") != std::string::npos);

    CHECK(
        std::ranges::find(lib.decks(), "deck-broken"s, &deck_summary::directory_name) ==
        lib.decks().end()
    );
}

TEST_CASE("load() reports a malformed deck's parse error", "[library]")
{
    auto const lib = open_library({primary_root()});

    auto const result = lib.load("deck-broken");
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == error_code::parse_error);

    CHECK_FALSE(lib.find("deck-broken").has_value());
}

TEST_CASE("load() loads by directory name", "[library]")
{
    auto const lib = open_library({primary_root()});

    auto const result = lib.load("deck-one");
    REQUIRE(result.has_value());
    CHECK((*result)->metadata.id == "deck-one-id");

    CHECK_FALSE(lib.load("deck-one-id").has_value());
}

TEST_CASE("load() fails for a directory that doesn't exist", "[library]")
{
    auto const lib = open_library({primary_root()});

    auto const result = lib.load("no-such-deck");
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == error_code::not_found);
}

TEST_CASE("loading the same deck twice parses it once", "[library]")
{
    auto const lib = open_library({primary_root()});

    auto const first = lib.load("deck-one");
    auto const second = lib.load("deck-one");

    REQUIRE(first.has_value());
    REQUIRE(second.has_value());

    CHECK(first->get() == second->get());
}

TEST_CASE("cache internal and external decks", "[library]")
{
    auto const lib = open_library({primary_root()});

    auto const by_name = lib.load("deck-one");
    auto const by_path = lib.load_external(primary_root() / "." / "deck-one");

    REQUIRE(by_name.has_value());
    REQUIRE(by_path.has_value());
    CHECK(by_name->get() == by_path->get());
}

TEST_CASE("refresh() drops cached loads without disturbing handed-out ones", "[library]")
{
    auto lib = open_library({primary_root()});

    auto const before = lib.load("deck-one");
    REQUIRE(before.has_value());

    lib.refresh();

    auto const after = lib.load("deck-one");
    REQUIRE(after.has_value());
    CHECK(before->get() != after->get());

    // The earlier load stays alive and usable
    CHECK((*before)->metadata.id == "deck-one-id");
}

TEST_CASE("a failed load is not cached", "[library]")
{
    arcana_test::temp_dir const root;
    root.write("wip-deck/deck.toml", "[deck\nname = broken");

    deck_library const lib{library_options{.roots = {root.path()}}};
    CHECK_FALSE(lib.load("wip-deck").has_value());

    // Repairing the deck while the process runs
    root.write("wip-deck/deck.toml", R"([deck]
id = "wip-deck-id"
name = "Repaired"
)");

    auto const repaired = lib.load("wip-deck");
    REQUIRE(repaired.has_value());
    CHECK((*repaired)->metadata.id == "wip-deck-id");
}

TEST_CASE("find() looks a deck up by its directory name", "[library]")
{
    auto const lib = open_library({primary_root()});

    auto const found = lib.find("deck-one");
    REQUIRE(found.has_value());
    CHECK(found->id == "deck-one-id");

    // The directory name is the key; the id is not interchangeable with it
    CHECK_FALSE(lib.find("deck-one-id").has_value());
    CHECK_FALSE(lib.find("no-such-deck").has_value());
}

TEST_CASE("find_all_by_id() keeps a duplicated id visible", "[library]")
{
    arcana_test::temp_dir const root;

    // Nothing enforces that [deck].id is unique across a library
    root.write("fork-a/deck.toml", R"([deck]
id = "shared-id"
name = "Fork A"
)");
    root.write("fork-b/deck.toml", R"([deck]
id = "shared-id"
name = "Fork B"
)");
    root.write("solo/deck.toml", R"([deck]
id = "solo-id"
name = "Solo"
)");

    deck_library const lib{library_options{.roots = {root.path()}}};

    auto const shared = lib.find_all_by_id("shared-id");
    REQUIRE(shared.size() == 2);
    CHECK(shared[0].directory_name == "fork-a");
    CHECK(shared[1].directory_name == "fork-b");

    auto const solo = lib.find_all_by_id("solo-id");
    REQUIRE(solo.size() == 1);
    CHECK(solo.front().directory_name == "solo");

    CHECK(lib.find_all_by_id("no-such-id").empty());

    // The directory name is not an id
    CHECK(lib.find_all_by_id("solo").empty());
}

TEST_CASE("load_external() loads a deck from outside every root", "[library]")
{
    auto const lib = open_library({primary_root()});

    auto const result = lib.load_external(alt_root() / "deck-three");
    REQUIRE(result.has_value());
    CHECK((*result)->metadata.id == "deck-three-id");
}

TEST_CASE("a library with nothing installed is empty", "[library]")
{
    arcana_test::temp_dir const root;
    auto const lib = open_library({root.path()});

    CHECK(lib.decks().empty());
    CHECK(lib.malformed_decks().empty());
}

TEST_CASE("several roots are searched in order", "[library]")
{
    // a consumer whose decks do not all live under one XDG path
    auto const lib = open_library({primary_root(), alt_root()});

    // deck-one and deck-two from the first root, deck-three from the second.
    // deck-two exists in both and is listed once
    REQUIRE(lib.decks().size() == 3);
    CHECK(std::ranges::count(lib.decks(), "deck-two"s, &deck_summary::directory_name) == 1);

    auto const three = std::ranges::find(lib.decks(), "deck-three"s, &deck_summary::directory_name);

    REQUIRE(three != lib.decks().end());
    CHECK(three->id == "deck-three-id");
}

TEST_CASE("an earlier root shadows a later one", "[library]")
{
    // Roots are a search path like PATH
    auto const first_wins = open_library({primary_root(), alt_root()});
    auto const two =
        std::ranges::find(first_wins.decks(), "deck-two"s, &deck_summary::directory_name);
    REQUIRE(two != first_wins.decks().end());
    CHECK(two->id == "deck-two-id");

    auto const reversed = open_library({alt_root(), primary_root()});
    auto const shadowed =
        std::ranges::find(reversed.decks(), "deck-two"s, &deck_summary::directory_name);
    REQUIRE(shadowed != reversed.decks().end());
    CHECK(shadowed->id == "deck-two-shadowed-id");

    CHECK((*first_wins.load("deck-two"))->metadata.id == "deck-two-id");
    CHECK((*reversed.load("deck-two"))->metadata.id == "deck-two-shadowed-id");
}

TEST_CASE("shadowing is decided before a deck is read", "[library]")
{
    // deck-broken is unreadable under the primary root and readable under the alt one.
    auto const malformed_first = open_library({primary_root(), alt_root()});
    CHECK(malformed_first.decks().size() == 3);
    REQUIRE(malformed_first.malformed_decks().size() == 1);
    CHECK(malformed_first.malformed_decks().front().directory_name == "deck-broken");
    CHECK_FALSE(malformed_first.load("deck-broken").has_value());

    auto const readable_first = open_library({alt_root(), primary_root()});
    CHECK(readable_first.malformed_decks().empty());

    auto const found = readable_first.find("deck-broken");
    REQUIRE(found.has_value());
    CHECK(found->id == "deck-broken-but-fine-here");
    CHECK(readable_first.load("deck-broken").has_value());
}

TEST_CASE("an empty root list falls back to the XDG deck library", "[library]")
{
    deck_library const defaulted{};

    CHECK(std::ranges::equal(defaulted.roots(), std::vector{paths::deck_library_path()}));

    auto const explicit_xdg = open_library({paths::deck_library_path()});
    CHECK(defaulted.decks().size() == explicit_xdg.decks().size());
}

TEST_CASE("the reference deck is configured apart from the roots", "[library]")
{
    deck_library const lib{
        library_options{.roots = {primary_root()}, .reference_deck = fixture("reference-deck")}
    };

    REQUIRE(lib.reference().has_value());
    CHECK(lib.reference()->id == "reference-deck-id");
    CHECK(lib.reference()->name == "Reference Deck");
    CHECK(lib.reference()->version == "2.1");

    auto const loaded = lib.load_reference();
    REQUIRE(loaded.has_value());
    CHECK((*loaded)->metadata.id == "reference-deck-id");

    // It stays out of the installed listing
    CHECK(lib.decks().size() == 2);
    CHECK(
        std::ranges::find(lib.decks(), "reference-deck"s, &deck_summary::directory_name) ==
        lib.decks().end()
    );
}

TEST_CASE("a library with no reference deck", "[library]")
{
    auto const lib = open_library({primary_root()});

    CHECK_FALSE(lib.reference().has_value());
    CHECK_FALSE(lib.reference_path().has_value());

    auto const loaded = lib.load_reference();
    REQUIRE_FALSE(loaded.has_value());
    CHECK(loaded.error().code == error_code::not_found);
}

TEST_CASE("an unreadable reference deck", "[library]")
{
    deck_library const lib{library_options{
        .roots = {primary_root()}, .reference_deck = fixture("broken-reference-deck")
    }};

    CHECK_FALSE(lib.reference().has_value());
    CHECK(lib.reference_path() == fixture("broken-reference-deck"));

    auto const loaded = lib.load_reference();
    REQUIRE_FALSE(loaded.has_value());
    CHECK(loaded.error().code == error_code::parse_error);
}


TEST_CASE("refresh() picks up a deck installed after construction", "[library]")
{
    arcana_test::temp_dir const root;

    deck_library lib{library_options{.roots = {root.path()}}};

    root.write("late-deck/deck.toml", R"([deck]
id = "late-deck-id"
schema_version = "1.0"
name = "Late Deck"
version = "1.0"
)");

    CHECK(lib.decks().empty());

    lib.refresh();
    REQUIRE(lib.decks().size() == 1);
    CHECK(lib.decks().front().id == "late-deck-id");
}
