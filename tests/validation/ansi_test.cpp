// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The `ansi` area: one rule, ansi-outside-image-root.

#include "fixture.hpp"

#include <arcana/validation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

using arcana::severity;
using arcana_test::codes_of;
using arcana_test::validate_fixture;

TEST_CASE("ansi-outside-image-root fires on its fixture", "[validation][ansi]")
{
    auto const found = validate_fixture("validation/ansi/ansi-outside-root-error");

    // The two lookalikes are `images`' to report and arrived with that area:
    // `ansi/` names no line count and `ansi032/` writes one with a leading
    // zero, so neither is an image root and neither holds cards.
    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "ansi-outside-image-root",
                               "ansi-outside-image-root",
                               "ansi-outside-image-root",
                               "ignored-image-root-lookalike",
                               "ignored-image-root-lookalike",
                           }
    );

    std::vector<std::string> paths;
    for (auto const& one : found)
    {
        REQUIRE(one.path.has_value());
        paths.push_back(one.path->generic_string());
    }

    CHECK(
        paths == std::vector<std::string>{
                     "ansi/major_arcana/00.ans",
                     "ansi032/major_arcana/00.ans",
                     "previews/banner.ans",
                     "ansi",
                     "ansi032",
                 }
    );

    for (auto const& one : found)
    {
        INFO("path: " << one.path->generic_string());
        CHECK(one.level == severity::info);
        CHECK(one.message.find(one.path->generic_string()) != std::string::npos);
        CHECK_FALSE(one.card.has_value());
        CHECK_FALSE(one.key.has_value());
    }
}
