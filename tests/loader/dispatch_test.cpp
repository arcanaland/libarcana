// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "temp_dir.hpp"

#include <arcana/deck.hpp>
#include <arcana/error.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <expected>
#include <format>
#include <string_view>

namespace
{

// Loads a deck whose [deck].schema_version is written exactly as given
std::expected<arcana::deck, arcana::error> load_declaring(std::string_view schema_version_line)
{
    arcana_test::temp_dir deck;
    deck.write(
        "deck.toml", std::format(
                         R"([deck]
{}
name = "Dispatch"
)",
                         schema_version_line
                     )
    );

    return arcana::load_deck(deck.path());
}

}  // namespace

TEST_CASE("a deck must declare a schema_version", "[loader][dispatch]")
{
    SECTION("absent")
    {
        arcana_test::temp_dir deck;
        deck.write("deck.toml", R"([deck]
name = "Undeclared"
)");

        auto const result = arcana::load_deck(deck.path());

        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == arcana::error_code::parse_error);
        CHECK(result.error().message.contains("schema_version"));
    }

    SECTION("a bare integer is not a version")
    {
        // What deckle emits today, and 2 is not 2.0
        auto const result = load_declaring("schema_version = 2");

        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == arcana::error_code::parse_error);
    }
}

TEST_CASE("a malformed schema_version is a hard error", "[loader][dispatch]")
{
    auto const rejected = GENERATE(
        as<std::string_view>{}, "", "1", "1.", ".0", "1.0.0", "x.y", "1.0-beta", " 1.0", "+1.0",
        "-1.0", "1.0 "
    );

    CAPTURE(rejected);

    auto const result = load_declaring(std::format(R"(schema_version = "{}")", rejected));

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == arcana::error_code::parse_error);
}

TEST_CASE("a well-formed schema_version loads under some front end", "[loader][dispatch]")
{
    // Every one of these loads: a known major routes to its own front end, an
    // unknown minor routes to its major silently, and an unknown major is read
    // best-effort under the newest
    auto const accepted = GENERATE(
        as<std::string_view>{}, "1.0", "1.7", "2.0", "2.9", "3.0", "256.0", "0.1", "01.00"
    );

    CAPTURE(accepted);

    auto const result = load_declaring(std::format(R"(schema_version = "{}")", accepted));

    REQUIRE(result.has_value());
    CHECK(result->metadata.schema_version == accepted);
    CHECK(result->metadata.name == "Dispatch");
}
