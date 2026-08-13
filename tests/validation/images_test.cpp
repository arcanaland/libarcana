// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

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

std::vector<std::string> paths_of(std::vector<diagnostic> const& found)
{
    std::vector<std::string> paths;
    paths.reserve(found.size());
    for (auto const& one : found) paths.push_back(one.path ? one.path->generic_string() : "<none>");

    return paths;
}

}  // namespace

TEST_CASE("assets in the places discovery looks fire nothing", "[validation][images]")
{
    CHECK(validate_fixture("validation/images/images-valid").empty());
}

TEST_CASE("rasters and SVGs outside their roots are ignored", "[validation][images]")
{
    auto const found = validate_fixture("validation/images/outside-roots-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "raster-outside-image-root",
                               "raster-outside-image-root",
                               "svg-outside-scalable",
                           }
    );

    CHECK(
        paths_of(found) == std::vector<std::string>{
                               "previews/promo.png",
                               // A raster inside a root of the wrong kind.
                               "scalable/major_arcana/00.png",
                               "art/hero.svg",
                           }
    );

    for (auto const& one : found)
    {
        INFO("path: " << one.path->generic_string());
        CHECK(one.level == severity::info);
        CHECK(one.message.find(one.path->generic_string()) != std::string::npos);
        CHECK_FALSE(one.key.has_value());
    }
}

TEST_CASE("directories that nearly name an image root are reported", "[validation][images]")
{
    auto const found = validate_fixture("validation/images/root-lookalike-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "ignored-image-root-lookalike",
                               "ignored-image-root-lookalike",
                               "raster-outside-image-root",
                               "raster-outside-image-root",
                           }
    );

    CHECK(
        paths_of(found) == std::vector<std::string>{
                               // A leading zero is not a root size.
                               "h0300",
                               "images",
                               "h0300/minor_arcana/wands/ace.png",
                               "images/major_arcana/00.png",
                           }
    );

    CHECK(found[0].level == severity::info);
    CHECK(found[0].message.find("minor_arcana") != std::string::npos);
    CHECK(found[1].message.find("major_arcana") != std::string::npos);
}

TEST_CASE("stems colliding in case and in chain format", "[validation][images]")
{
    auto const found = validate_fixture("validation/images/stem-collisions-error");

    REQUIRE(
        codes_of(found) == std::vector<std::string_view>{
                               "duplicate-chain-extension",
                               "stem-case-collision",
                           }
    );

    CHECK(
        paths_of(found) == std::vector<std::string>{
                               "h1200/major_arcana/00.webp",
                               "docs/notes.txt",
                           }
    );

    CHECK(found[0].level == severity::warning);
    CHECK(found[0].message.find("h1200/major_arcana/00.png") != std::string::npos);

    CHECK(found[1].level == severity::error);
    CHECK(found[1].message.find("docs/Notes.txt") != std::string::npos);
}
