// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <language_tag.hpp>

#include <catch2/catch_test_macros.hpp>

using arcana::data::canonicalize_language_tag;
using arcana::data::is_canonical_language_tag;
using arcana::data::is_well_formed_language_tag;

TEST_CASE("well-formed language tags follow RFC 5646", "[language_tag]")
{
    CHECK(is_well_formed_language_tag("en"));
    CHECK(is_well_formed_language_tag("pt-BR"));
    CHECK(is_well_formed_language_tag("zh-Hant-TW"));
    CHECK(is_well_formed_language_tag("sr-Latn"));
    CHECK(is_well_formed_language_tag("es-419"));
    CHECK(is_well_formed_language_tag("de-CH-1901"));
    CHECK(is_well_formed_language_tag("en-US-u-va-posix"));
    CHECK(is_well_formed_language_tag("x-private"));
    CHECK(is_well_formed_language_tag("en-x-custom"));
    CHECK(is_well_formed_language_tag("eng"));

    // Grandfathered
    CHECK(is_well_formed_language_tag("i-klingon"));
    CHECK(is_well_formed_language_tag("en-GB-oed"));
    CHECK(is_well_formed_language_tag("sgn-BE-FR"));
}

TEST_CASE("malformed language tags are rejected", "[language_tag]")
{
    CHECK_FALSE(is_well_formed_language_tag(""));
    CHECK_FALSE(is_well_formed_language_tag("e"));
    CHECK_FALSE(is_well_formed_language_tag("en-"));
    CHECK_FALSE(is_well_formed_language_tag("-en"));
    CHECK_FALSE(is_well_formed_language_tag("en--US"));
    CHECK_FALSE(is_well_formed_language_tag("en_US"));
    CHECK_FALSE(is_well_formed_language_tag("toolongsubtag"));
    CHECK_FALSE(is_well_formed_language_tag("en-US-"));
    CHECK_FALSE(is_well_formed_language_tag("x"));
    CHECK_FALSE(is_well_formed_language_tag("en-u"));
    CHECK_FALSE(is_well_formed_language_tag("en-x"));
    CHECK_FALSE(is_well_formed_language_tag("1234"));
    CHECK_FALSE(is_well_formed_language_tag("en.US"));
}

TEST_CASE("canonicalization applies the case rules of DECK.md 6.1", "[language_tag]")
{
    CHECK(canonicalize_language_tag("EN") == "en");
    CHECK(canonicalize_language_tag("pt-br") == "pt-BR");
    CHECK(canonicalize_language_tag("ZH-hant-tw") == "zh-Hant-TW");
    CHECK(canonicalize_language_tag("es-419") == "es-419");
    CHECK(canonicalize_language_tag("DE-ch-1901") == "de-CH-1901");
}

TEST_CASE("canonicalization takes the shortest ISO 639 subtag", "[language_tag]")
{
    CHECK(canonicalize_language_tag("eng") == "en");
    CHECK(canonicalize_language_tag("ger") == "de");
    CHECK(canonicalize_language_tag("deu") == "de");
    CHECK(canonicalize_language_tag("fra-CA") == "fr-CA");

    // No two-letter form exists
    CHECK(canonicalize_language_tag("haw") == "haw");
    CHECK(canonicalize_language_tag("ceb") == "ceb");
}

TEST_CASE("a tag that is not well-formed has no canonical form", "[language_tag]")
{
    CHECK(canonicalize_language_tag("en_US").empty());
    CHECK(canonicalize_language_tag("").empty());
    CHECK_FALSE(is_canonical_language_tag("en_US"));
}

TEST_CASE("canonicality is exactly agreement with the canonical form", "[language_tag]")
{
    CHECK(is_canonical_language_tag("en"));
    CHECK(is_canonical_language_tag("pt-BR"));
    CHECK(is_canonical_language_tag("zh-Hant-TW"));

    CHECK_FALSE(is_canonical_language_tag("EN"));
    CHECK_FALSE(is_canonical_language_tag("pt-br"));
    CHECK_FALSE(is_canonical_language_tag("zh-hant-TW"));
    CHECK_FALSE(is_canonical_language_tag("eng"));
}
