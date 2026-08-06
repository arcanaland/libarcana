// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <scanners.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

using arcana::data::canonicalize_language_tag;
using arcana::data::image_format;
using arcana::data::is_absolute_http_url;
using arcana::data::is_canonical_id;
using arcana::data::is_canonical_language_tag;
using arcana::data::is_card_reference;
using arcana::data::is_custom_name;
using arcana::data::is_qualified_identifier;
using arcana::data::is_realm;
using arcana::data::is_reserved_canonical_key;
using arcana::data::is_srgb_hex_triplet;
using arcana::data::is_variant_reference;
using arcana::data::is_well_formed_language_tag;
using arcana::data::parse_qualified_identifier;
using arcana::data::sniff_image_format;

namespace
{

// Wraps the bytes a signature test cares about, which is all sniffing reads.
std::span<std::byte const> bytes_of(std::string_view s)
{
    return {reinterpret_cast<std::byte const*>(s.data()), s.size()};
}

}  // namespace

TEST_CASE("custom names accept the grammar of DECK.md 3.5", "[scanners]")
{
    CHECK(is_custom_name("happy_squirrel"));
    CHECK(is_custom_name("stars"));
    CHECK(is_custom_name("_private"));
    CHECK(is_custom_name("a"));
    CHECK(is_custom_name("two_women"));
    CHECK(is_custom_name("x_kickstarter"));
    CHECK(is_custom_name("edition2"));
}

TEST_CASE("custom names reject everything outside it", "[scanners]")
{
    CHECK_FALSE(is_custom_name(""));
    CHECK_FALSE(is_custom_name("2women"));
    CHECK_FALSE(is_custom_name("Stars"));
    CHECK_FALSE(is_custom_name("happy-squirrel"));
    CHECK_FALSE(is_custom_name("happy squirrel"));
    CHECK_FALSE(is_custom_name("major_arcana.00"));
    CHECK_FALSE(is_custom_name("caf\xc3\xa9"));
}

TEST_CASE("the reserved canonical keys are the twenty of DECK.md 3.2", "[scanners]")
{
    CHECK(is_reserved_canonical_key("major_arcana"));
    CHECK(is_reserved_canonical_key("minor_arcana"));
    CHECK(is_reserved_canonical_key("wands"));
    CHECK(is_reserved_canonical_key("pentacles"));
    CHECK(is_reserved_canonical_key("ace"));
    CHECK(is_reserved_canonical_key("king"));
    CHECK(is_reserved_canonical_key("knight"));

    CHECK_FALSE(is_reserved_canonical_key("stars"));
    CHECK_FALSE(is_reserved_canonical_key("Wands"));
    CHECK_FALSE(is_reserved_canonical_key(""));
    CHECK_FALSE(is_reserved_canonical_key("arcana"));
}

TEST_CASE("canonical IDs cover both arcana", "[scanners]")
{
    CHECK(is_canonical_id("major_arcana.00"));
    CHECK(is_canonical_id("major_arcana.21"));
    CHECK(is_canonical_id("major_arcana.happy_squirrel"));
    CHECK(is_canonical_id("minor_arcana.wands.ace"));
    CHECK(is_canonical_id("minor_arcana.stars.ace"));
    CHECK(is_canonical_id("minor_arcana.wands.knight"));

    // The grammar admits any two digits; only 00 to 21 mean anything shared,
    // and that is a separate rule rather than a grammar question.
    CHECK(is_canonical_id("major_arcana.99"));
}

TEST_CASE("canonical IDs reject malformed positions", "[scanners]")
{
    CHECK_FALSE(is_canonical_id(""));
    CHECK_FALSE(is_canonical_id("major_arcana"));
    CHECK_FALSE(is_canonical_id("major_arcana."));
    CHECK_FALSE(is_canonical_id("major_arcana.0"));
    CHECK_FALSE(is_canonical_id("major_arcana.000"));
    CHECK_FALSE(is_canonical_id("major_arcana.00.extra"));
    CHECK_FALSE(is_canonical_id("minor_arcana.wands"));
    CHECK_FALSE(is_canonical_id("minor_arcana.wands.ace.extra"));
    CHECK_FALSE(is_canonical_id("minor_arcana..ace"));
    CHECK_FALSE(is_canonical_id("trumps.00"));
}

TEST_CASE("a card reference may carry a variant suffix and a variant reference must", "[scanners]")
{
    CHECK(is_card_reference("major_arcana.06"));
    CHECK(is_card_reference("major_arcana.06:two_women"));
    CHECK(is_card_reference("minor_arcana.cups.two:alternate"));

    CHECK_FALSE(is_variant_reference("major_arcana.06"));
    CHECK(is_variant_reference("major_arcana.06:two_women"));

    CHECK_FALSE(is_card_reference("major_arcana.06:"));
    CHECK_FALSE(is_card_reference("major_arcana.06:Two_Women"));
    CHECK_FALSE(is_card_reference(":two_women"));
}

TEST_CASE("a realm is two or more labels", "[scanners]")
{
    CHECK(is_realm("land.arcana"));
    CHECK(is_realm("org.example.my.domain"));
    CHECK(is_realm("example.xn--bcher-kva"));
    CHECK(is_realm("a.b"));

    CHECK_FALSE(is_realm("arcana"));
    CHECK_FALSE(is_realm(""));
    CHECK_FALSE(is_realm("land."));
    CHECK_FALSE(is_realm(".arcana"));
    CHECK_FALSE(is_realm("land..arcana"));
    CHECK_FALSE(is_realm("Land.Arcana"));
    CHECK_FALSE(is_realm("land.-arcana"));
    CHECK_FALSE(is_realm("land.arcana-"));
    CHECK_FALSE(is_realm("land.9arcana"));
}

TEST_CASE("qualified identifiers split into realm, path and fragment", "[scanners]")
{
    auto const deck = parse_qualified_identifier("land.arcana/deck/rider-waite-smith");
    REQUIRE(deck.has_value());
    CHECK(deck->realm == "land.arcana");
    CHECK(deck->path == "deck/rider-waite-smith");
    CHECK(deck->fragment.empty());

    auto const card = parse_qualified_identifier("land.arcana/deck/inclusive#major_arcana.06:two");
    REQUIRE(card.has_value());
    CHECK(card->realm == "land.arcana");
    CHECK(card->path == "deck/inclusive");
    CHECK(card->fragment == "major_arcana.06:two");
}

TEST_CASE("qualified identifiers reject malformed shapes", "[scanners]")
{
    CHECK_FALSE(is_qualified_identifier(""));
    CHECK_FALSE(is_qualified_identifier("land.arcana"));
    CHECK_FALSE(is_qualified_identifier("arcana/deck/x"));
    CHECK_FALSE(is_qualified_identifier("land.arcana/"));
    CHECK_FALSE(is_qualified_identifier("land.arcana//deck"));
    CHECK_FALSE(is_qualified_identifier("land.arcana/deck/x#"));
    CHECK_FALSE(is_qualified_identifier("land.arcana/deck/Rider"));
    CHECK_FALSE(is_qualified_identifier("land.arcana/deck/rider_waite"));
    CHECK_FALSE(is_qualified_identifier("https://land.arcana/deck/x"));
}

TEST_CASE("well-formed language tags follow RFC 5646", "[scanners]")
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

    // Grandfathered, and not otherwise expressible in the grammar.
    CHECK(is_well_formed_language_tag("i-klingon"));
    CHECK(is_well_formed_language_tag("en-GB-oed"));
    CHECK(is_well_formed_language_tag("sgn-BE-FR"));
}

TEST_CASE("malformed language tags are rejected", "[scanners]")
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

TEST_CASE("canonicalization applies the case rules of DECK.md 6.1", "[scanners]")
{
    CHECK(canonicalize_language_tag("EN") == "en");
    CHECK(canonicalize_language_tag("pt-br") == "pt-BR");
    CHECK(canonicalize_language_tag("ZH-hant-tw") == "zh-Hant-TW");
    CHECK(canonicalize_language_tag("es-419") == "es-419");
    CHECK(canonicalize_language_tag("DE-ch-1901") == "de-CH-1901");
}

TEST_CASE("canonicalization takes the shortest ISO 639 subtag", "[scanners]")
{
    CHECK(canonicalize_language_tag("eng") == "en");
    CHECK(canonicalize_language_tag("ger") == "de");
    CHECK(canonicalize_language_tag("deu") == "de");
    CHECK(canonicalize_language_tag("fra-CA") == "fr-CA");

    // No two-letter form exists, so the three-letter one is already shortest.
    CHECK(canonicalize_language_tag("haw") == "haw");
    CHECK(canonicalize_language_tag("ceb") == "ceb");
}

TEST_CASE("a tag that is not well-formed has no canonical form", "[scanners]")
{
    CHECK(canonicalize_language_tag("en_US").empty());
    CHECK(canonicalize_language_tag("").empty());
    CHECK_FALSE(is_canonical_language_tag("en_US"));
}

TEST_CASE("canonicality is exactly agreement with the canonical form", "[scanners]")
{
    CHECK(is_canonical_language_tag("en"));
    CHECK(is_canonical_language_tag("pt-BR"));
    CHECK(is_canonical_language_tag("zh-Hant-TW"));

    CHECK_FALSE(is_canonical_language_tag("EN"));
    CHECK_FALSE(is_canonical_language_tag("pt-br"));
    CHECK_FALSE(is_canonical_language_tag("zh-hant-TW"));
    CHECK_FALSE(is_canonical_language_tag("eng"));
}

TEST_CASE("sRGB hex triplets are lower case and exactly six digits", "[scanners]")
{
    CHECK(is_srgb_hex_triplet("#e8d5a3"));
    CHECK(is_srgb_hex_triplet("#000000"));
    CHECK(is_srgb_hex_triplet("#ffffff"));

    CHECK_FALSE(is_srgb_hex_triplet("#E8D5A3"));
    CHECK_FALSE(is_srgb_hex_triplet("e8d5a3"));
    CHECK_FALSE(is_srgb_hex_triplet("#e8d5a"));
    CHECK_FALSE(is_srgb_hex_triplet("#e8d5a33"));
    CHECK_FALSE(is_srgb_hex_triplet("#gggggg"));
    CHECK_FALSE(is_srgb_hex_triplet(""));
}

TEST_CASE("link URLs are absolute http or https", "[scanners]")
{
    CHECK(is_absolute_http_url("https://example.com/shop/the-deck"));
    CHECK(is_absolute_http_url("http://example.com"));
    CHECK(is_absolute_http_url("HTTPS://example.com/about"));
    CHECK(is_absolute_http_url("https://example.com/a?b=c#d"));

    CHECK_FALSE(is_absolute_http_url(""));
    CHECK_FALSE(is_absolute_http_url("example.com"));
    CHECK_FALSE(is_absolute_http_url("/shop/the-deck"));
    CHECK_FALSE(is_absolute_http_url("ftp://example.com"));
    CHECK_FALSE(is_absolute_http_url("mailto:someone@example.com"));
    CHECK_FALSE(is_absolute_http_url("https://"));
    CHECK_FALSE(is_absolute_http_url("https:///path"));
    CHECK_FALSE(is_absolute_http_url("https://example.com/a b"));
}

TEST_CASE("image formats are read from signature bytes", "[scanners]")
{
    CHECK(sniff_image_format(bytes_of("\x89PNG\r\n\x1a\n\x00\x00")) == image_format::png);
    CHECK(sniff_image_format(bytes_of("\xff\xd8\xff\xe0JFIF")) == image_format::jpeg);

    CHECK(sniff_image_format(bytes_of("<svg xmlns=")) == image_format::unknown);
    CHECK(sniff_image_format(bytes_of("GIF89a")) == image_format::unknown);
    CHECK(sniff_image_format(bytes_of("")) == image_format::unknown);

    // A truncated signature is not a signature.
    CHECK(sniff_image_format(bytes_of("\x89PNG")) == image_format::unknown);
    CHECK(sniff_image_format(bytes_of("\xff\xd8")) == image_format::unknown);
}
