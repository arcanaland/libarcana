// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <asset_grammar.hpp>

#include <arcana/card.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <filesystem>
#include <optional>
#include <string_view>

using arcana::image_kind;
using arcana::data::asset_filename;
using arcana::data::chain_rank;
using arcana::data::components_of;
using arcana::data::is_baseline_extension;
using arcana::data::parse_image_root;

namespace
{

std::optional<image_kind> kind_of_directory(std::string_view name)
{
    auto const root = parse_image_root(name);

    return root ? std::optional{root->kind} : std::nullopt;
}

std::optional<int> size_of_directory(std::string_view name)
{
    auto const root = parse_image_root(name);

    return root ? root->size : std::nullopt;
}

}  // namespace

TEST_CASE("the four image root forms", "[asset_grammar]")
{
    CHECK(kind_of_directory("scalable") == image_kind::scalable);
    CHECK(size_of_directory("scalable") == std::nullopt);

    CHECK(kind_of_directory("surrogate") == image_kind::surrogate);
    CHECK(size_of_directory("surrogate") == std::nullopt);

    CHECK(kind_of_directory("h1200") == image_kind::raster);
    CHECK(size_of_directory("h1200") == 1200);
    CHECK(kind_of_directory("h1") == image_kind::raster);
    CHECK(size_of_directory("h1") == 1);

    CHECK(kind_of_directory("ansi32") == image_kind::ansi);
    CHECK(size_of_directory("ansi32") == 32);
    CHECK(kind_of_directory("ansi1") == image_kind::ansi);
    CHECK(size_of_directory("ansi1") == 1);
}

TEST_CASE("a directory that is not one of the four forms is no image root", "[asset_grammar]")
{
    auto const name = GENERATE(
        // A size must be present, positive and free of leading zeroes
        "h", "h0", "h00", "h01", "h-1", "h+1", "h1.5", "hx", "h 1", "h1_200",
        // doesn't fit in an int
        "h99999999999999999999", "ansi99999999999999999999",
        // ansi
        "ansi", "ansi0", "ansi01", "ansiX",
        // misc
        "", "scalable2", "Scalable", "surrogates", "svg", "major_arcana", "card_backs", "names"
    );

    CAPTURE(name);
    CHECK_FALSE(parse_image_root(name).has_value());
}

TEST_CASE("the raster chain is png, webp, avif, then jpeg and jpg", "[asset_grammar]")
{
    CHECK(chain_rank(image_kind::raster, "png") == 0);
    CHECK(chain_rank(image_kind::raster, "webp") == 1);
    CHECK(chain_rank(image_kind::raster, "avif") == 2);
    CHECK(chain_rank(image_kind::raster, "jpeg") == 3);

    // The spec ranks jpeg and jpg together and leaves the choice unspecified
    CHECK(chain_rank(image_kind::raster, "jpg") == chain_rank(image_kind::raster, "jpeg"));

    CHECK_FALSE(chain_rank(image_kind::raster, "svg").has_value());
    CHECK_FALSE(chain_rank(image_kind::raster, "toml").has_value());
    CHECK_FALSE(chain_rank(image_kind::raster, "").has_value());
    CHECK_FALSE(chain_rank(image_kind::raster, "PNG").has_value());
}

TEST_CASE("the scalable chain is svg alone", "[asset_grammar]")
{
    CHECK(chain_rank(image_kind::scalable, "svg") == 0);

    for (auto const extension : {"png", "webp", "avif", "jpeg", "jpg", "toml", ""})
        CHECK_FALSE(chain_rank(image_kind::scalable, extension).has_value());
}

TEST_CASE("the surrogate chain is toml alone", "[asset_grammar]")
{
    CHECK(chain_rank(image_kind::surrogate, "toml") == 0);

    for (auto const extension : {"png", "webp", "avif", "jpeg", "jpg", "svg", ""})
        CHECK_FALSE(chain_rank(image_kind::surrogate, extension).has_value());
}

TEST_CASE("an ANSI root takes any extension or none", "[asset_grammar]")
{
    for (auto const extension : {"ansi", "txt", "png", "svg", "toml", ""})
        CHECK(chain_rank(image_kind::ansi, extension) == 0);
}

TEST_CASE("the baseline formats are png, webp and jpeg", "[asset_grammar]")
{
    CHECK(is_baseline_extension("png"));
    CHECK(is_baseline_extension("webp"));
    CHECK(is_baseline_extension("jpeg"));
    CHECK(is_baseline_extension("jpg"));

    // Decoding these two is optional, so neither is baseline
    CHECK_FALSE(is_baseline_extension("avif"));
    CHECK_FALSE(is_baseline_extension("svg"));

    CHECK_FALSE(is_baseline_extension("toml"));
    CHECK_FALSE(is_baseline_extension(""));
}

// --- extensions, stems and bases ------------------------------

TEST_CASE("a filename splits at the first and last dot", "[asset_grammar]")
{
    auto const bare = asset_filename::from_filename("00.png");
    CHECK(bare.base == "00");
    CHECK(bare.variant_key.empty());
    CHECK(bare.extension == "png");

    auto const variant = asset_filename::from_filename("00.gold.png");
    CHECK(variant.base == "00");
    CHECK(variant.variant_key == "gold");
    CHECK(variant.extension == "png");

    // The stem splits on the first dot, the extension comes off the last
    auto const deep = asset_filename::from_filename("a.b.c.png");
    CHECK(deep.base == "a");
    CHECK(deep.variant_key == "b.c");
    CHECK(deep.extension == "png");

    // No dot at all (ANSI?)
    auto const none = asset_filename::from_filename("00");
    CHECK(none.base == "00");
    CHECK(none.variant_key.empty());
    CHECK(none.extension.empty());

    // A dotfile is all extension and no base
    auto const hidden = asset_filename::from_filename(".hidden");
    CHECK(hidden.base.empty());
    CHECK(hidden.variant_key.empty());
    CHECK(hidden.extension == "hidden");

    auto const empty = asset_filename::from_filename("");
    CHECK(empty.base.empty());
    CHECK(empty.extension.empty());
}

TEST_CASE("a deck-relative path splits into its components", "[asset_grammar]")
{
    std::filesystem::path const major_path{"h1200/major_arcana/00.png"};
    auto const majors = components_of(major_path);
    REQUIRE(majors.size == 3);
    CHECK(majors[0] == "h1200");
    CHECK(majors[1] == "major_arcana");
    CHECK(majors[2] == "00.png");

    std::filesystem::path const minor_path{"h1200/minor_arcana/cups/ace.png"};
    auto const minors = components_of(minor_path);
    REQUIRE(minors.size == 4);
    CHECK(minors[2] == "cups");
    CHECK(minors[3] == "ace.png");

    std::filesystem::path const manifest{"deck.toml"};
    auto const one = components_of(manifest);
    REQUIRE(one.size == 1);
    CHECK(one[0] == "deck.toml");
}

TEST_CASE("a path deeper than a discovery location yields nothing", "[asset_grammar]")
{
    std::filesystem::path const deep{"h1200/minor_arcana/cups/extra/ace.png"};
    CHECK(components_of(deep).size == 0);
}
