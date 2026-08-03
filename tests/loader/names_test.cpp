// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "temp_dir.hpp"

#include <names.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

using arcana::detail::name_catalog;

namespace
{

constexpr std::string_view english = R"([major_arcana]
00 = "The Fool"

[minor_arcana.cups]
ace = "Ace of Cups"
)";

constexpr std::string_view french = R"([major_arcana]
00 = "Le Mat"
)";

constexpr std::string_view portuguese = R"([major_arcana]
00 = "O Louco"
)";

}  // namespace

TEST_CASE("a deck with no names directory yields an unloaded catalog", "[names]")
{
    arcana_test::temp_dir deck;

    auto const names = name_catalog::load(deck.path(), {"en"});

    CHECK_FALSE(names.loaded());
    CHECK_FALSE(names.lookup("major_arcana", "00").has_value());
    CHECK_FALSE(names.lookup_minor("minor_arcana", "cups", "ace").has_value());
}

TEST_CASE("a names directory with no toml in it yields an unloaded catalog", "[names]")
{
    arcana_test::temp_dir deck;
    deck.write("names/readme.md", "not a names file");

    CHECK_FALSE(name_catalog::load(deck.path(), {"en"}).loaded());
}

TEST_CASE("the requested language wins when it is present", "[names]")
{
    arcana_test::temp_dir deck;
    deck.write("names/en.toml", english);
    deck.write("names/fr.toml", french);

    auto const names = name_catalog::load(deck.path(), {"fr"});

    REQUIRE(names.loaded());
    CHECK(names.lookup("major_arcana", "00") == "Le Mat");
}

TEST_CASE("english is the fallback when the requested language is missing", "[names]")
{
    arcana_test::temp_dir deck;
    deck.write("names/en.toml", english);
    deck.write("names/fr.toml", french);

    SECTION("an unavailable language")
    {
        CHECK(name_catalog::load(deck.path(), {"es"}).lookup("major_arcana", "00") == "The Fool");
    }

    SECTION("no language requested")
    {
        CHECK(name_catalog::load(deck.path(), {}).lookup("major_arcana", "00") == "The Fool");
    }
}

TEST_CASE("a region-qualified tag falls back to its base language", "[names]")
{
    arcana_test::temp_dir deck;
    deck.write("names/en.toml", english);
    deck.write("names/pt.toml", portuguese);

    SECTION("underscore")
    {
        CHECK(name_catalog::load(deck.path(), {"pt_BR"}).lookup("major_arcana", "00") == "O Louco");
    }

    SECTION("hyphen")
    {
        CHECK(name_catalog::load(deck.path(), {"pt-BR"}).lookup("major_arcana", "00") == "O Louco");
    }

    SECTION("the exact tag")
    {
        deck.write("names/pt_BR.toml", R"([major_arcana]
00 = "Some Special String")");

        CHECK(
            name_catalog::load(deck.path(), {"pt_BR"}).lookup("major_arcana", "00") ==
            "Some Special String"
        );
    }
}

TEST_CASE("the language chain is tried in order", "[names]")
{
    arcana_test::temp_dir deck;
    deck.write("names/en.toml", english);
    deck.write("names/fr.toml", french);
    deck.write("names/pt.toml", portuguese);

    CHECK(name_catalog::load(deck.path(), {"fr", "pt"}).lookup("major_arcana", "00") == "Le Mat");
    CHECK(name_catalog::load(deck.path(), {"pt", "fr"}).lookup("major_arcana", "00") == "O Louco");

    CHECK(
        name_catalog::load(deck.path(), {"pt_BR", "fr"}).lookup("major_arcana", "00") == "O Louco"
    );

    CHECK(name_catalog::load(deck.path(), {"es", "de"}).lookup("major_arcana", "00") == "The Fool");
}

TEST_CASE("any toml is used when neither the request nor english is present", "[names]")
{
    arcana_test::temp_dir deck;
    deck.write("names/fr.toml", french);

    auto const names = name_catalog::load(deck.path(), {"es"});

    REQUIRE(names.loaded());
    CHECK(names.lookup("major_arcana", "00") == "Le Mat");
}

TEST_CASE("a names file that fails to parse yields an unloaded catalog", "[names]")
{
    arcana_test::temp_dir deck;
    deck.write("names/en.toml", "[major_arcana\n00 = broken");

    CHECK_FALSE(name_catalog::load(deck.path(), {"en"}).loaded());
}

TEST_CASE("lookups miss without failing", "[names]")
{
    arcana_test::temp_dir deck;
    deck.write("names/en.toml", english);

    auto const names = name_catalog::load(deck.path(), {"en"});
    REQUIRE(names.loaded());

    CHECK(names.lookup_minor("minor_arcana", "cups", "ace") == "Ace of Cups");
    CHECK_FALSE(names.lookup("major_arcana", "21").has_value());
    CHECK_FALSE(names.lookup_minor("minor_arcana", "stars", "ace").has_value());
    CHECK_FALSE(names.lookup("garbage", "00").has_value());
    CHECK_FALSE(names.lookup("major_arcana", "").has_value());
}
