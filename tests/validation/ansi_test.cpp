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

    // ansi32/ is a well-formed ANSI root, so neither the card art nor the card
    // back under it is reported. `ansi/` names no line count and `ansi032/`
    // carries a leading zero, so DECK.md section 5.7.1 makes both of them
    // ordinary directories. previews/ was never a root. notes.txt carries no
    // ESC and is plain text, which section 5.4 does not distinguish from any
    // other text file.
    //
    // Closed world: this is the complete set, in catalogue order.
    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "ansi-outside-image-root",
                               "ansi-outside-image-root",
                               "ansi-outside-image-root",
                           }
    );

    std::vector<std::string> paths;
    for (auto const& one : found)
    {
        REQUIRE(one.path.has_value());
        paths.push_back(one.path->generic_string());
    }

    // Sorted by path, which is what the diagnostic ordering gives.
    CHECK(
        paths == std::vector<std::string>{
                     "ansi/major_arcana/00.ans",
                     "ansi032/major_arcana/00.ans",
                     "previews/banner.ans",
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
