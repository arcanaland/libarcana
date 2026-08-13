// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The `backs` area: seven rules over card back designs and the files behind
// them. Every assertion here is closed-world — the complete set of codes a
// fixture produces, in catalogue order.

#include "fixture.hpp"

#include <arcana/validation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

using arcana::diagnostic;
using arcana::severity;
using arcana_test::codes_of;
using arcana_test::validate_fixture;

namespace
{

std::vector<std::string> keys_of(std::vector<diagnostic> const& found)
{
    std::vector<std::string> keys;
    keys.reserve(found.size());
    for (auto const& one : found) keys.push_back(one.key.value_or("<none>"));

    return keys;
}

std::vector<std::string> paths_of(std::vector<diagnostic> const& found)
{
    std::vector<std::string> paths;
    paths.reserve(found.size());
    for (auto const& one : found) paths.push_back(one.path ? one.path->generic_string() : "<none>");

    return paths;
}

}  // namespace

TEST_CASE("a deck whose backs are in order fires nothing", "[validation][backs]")
{
    // Two designs, one of them ANSI-only: the ANSI design is exempt from the
    // baseline-format rule, and the declared default settles collation.
    CHECK(validate_fixture("validation/backs/backs-valid").empty());
}

TEST_CASE("a design key outside the grammar and a dangling image", "[validation][backs]")
{
    auto const found = validate_fixture("validation/backs/design-keys-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "bad-card-back-design-key",
                               "bad-custom-name",
                               "missing-card-back-image",
                           }
    );

    CHECK(
        keys_of(found) == std::vector<std::string>{
                              "card_backs.designs.Classic-Back",
                              "card_backs.designs.Classic-Back",
                              "card_backs.designs.Classic-Back.image",
                          }
    );

    for (auto const& one : found)
    {
        INFO("code: " << one.code);
        CHECK(one.level == severity::error);
        CHECK_FALSE(one.path.has_value());
    }

    CHECK(found[0].message.find("'Classic-Back'") != std::string::npos);
    CHECK(found[2].message.find("card_backs/gone.png") != std::string::npos);
}

TEST_CASE("a default naming a design the deck lacks", "[validation][backs]")
{
    auto const found = validate_fixture("validation/backs/unknown-back-references-error");

    REQUIRE(codes_of(found) == std::vector<std::string_view>{"unknown-default-card-back"});

    CHECK(keys_of(found) == std::vector<std::string>{"card_backs.default"});

    CHECK(found.front().level == severity::error);
    CHECK(found.front().message.find("'missing'") != std::string::npos);
}

TEST_CASE("several designs and no declared default rest on collation", "[validation][backs]")
{
    auto const found = validate_fixture("validation/backs/default-by-collation-error");

    REQUIRE(codes_of(found) == std::vector<std::string_view>{"card-back-default-by-collation"});

    CHECK(found.front().level == severity::warning);
    CHECK(found.front().key == "card_backs.default");

    // The design that wins on collation order is the one worth naming.
    CHECK(found.front().message.find("'classic'") != std::string::npos);
}

TEST_CASE("files a card back directory ignores, and undecodable designs", "[validation][backs]")
{
    auto const found = validate_fixture("validation/backs/ignored-back-files-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "card-back-not-baseline-format",
                               "card-back-not-baseline-format",
                               "ignored-card-back-file",
                               "ignored-card-back-file",
                           }
    );

    CHECK(
        paths_of(found) == std::vector<std::string>{
                               // Outside the chain, but an `image` path names it.
                               "card_backs/classic.tiff",
                               "scalable/card_backs/vector.svg",
                               // A stem carrying a variant key, which backs have not.
                               "card_backs/06.two_women.png",
                               // A stem that is not a custom name.
                               "card_backs/Fancy.png",
                           }
    );

    for (auto const& one : found)
    {
        INFO("path: " << one.path->generic_string());
        CHECK(one.level == severity::warning);
        CHECK_FALSE(one.card.has_value());
        CHECK_FALSE(one.key.has_value());
    }

    CHECK(found[0].message.find("'classic'") != std::string::npos);
    CHECK(found[1].message.find("'vector'") != std::string::npos);
}
