// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <spdx_licenses.hpp>
#include <tables.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>

using arcana::data::classify_rights_status;
using arcana::data::find_license_permissions;
using arcana::data::is_css_color_name;
using arcana::data::is_extension_link_rel;
using arcana::data::is_registered_link_rel;
using arcana::data::is_rights_status_uri;
using arcana::data::is_spdx_license_id;
using arcana::data::rights_status_class;
using arcana::data::shortest_language_subtag;

TEST_CASE("CSS Color 4 named colours are known by name", "[tables]")
{
    CHECK(is_css_color_name("wheat"));
    CHECK(is_css_color_name("darkslateblue"));
    CHECK(is_css_color_name("sienna"));
    CHECK(is_css_color_name("lightgray"));
    CHECK(is_css_color_name("rebeccapurple"));
    CHECK(is_css_color_name("aliceblue"));
    CHECK(is_css_color_name("yellowgreen"));

    // Both spellings CSS carries.
    CHECK(is_css_color_name("gray"));
    CHECK(is_css_color_name("grey"));
}

TEST_CASE("names outside CSS Color 4 are not colours", "[tables]")
{
    CHECK_FALSE(is_css_color_name(""));
    CHECK_FALSE(is_css_color_name("Wheat"));
    CHECK_FALSE(is_css_color_name("WHEAT"));
    CHECK_FALSE(is_css_color_name("#e8d5a3"));
    CHECK_FALSE(is_css_color_name("burntsienna"));

    // A CSS system colour is not a named colour.
    CHECK_FALSE(is_css_color_name("canvastext"));
}

TEST_CASE("the link registry is DECK.md 4.1.1's own five relations", "[tables]")
{
    CHECK(is_registered_link_rel("homepage"));
    CHECK(is_registered_link_rel("buy"));
    CHECK(is_registered_link_rel("artist"));
    CHECK(is_registered_link_rel("publisher"));
    CHECK(is_registered_link_rel("source"));

    // IANA's registry is a different registry and nothing here reads it.
    CHECK_FALSE(is_registered_link_rel("alternate"));
    CHECK_FALSE(is_registered_link_rel("license"));
    CHECK_FALSE(is_registered_link_rel(""));
}

TEST_CASE("the x_ prefix is the documented escape hatch", "[tables]")
{
    CHECK(is_extension_link_rel("x_kickstarter"));
    CHECK(is_extension_link_rel("x_"));

    CHECK_FALSE(is_extension_link_rel("buy"));
    CHECK_FALSE(is_extension_link_rel("xkickstarter"));
    CHECK_FALSE(is_extension_link_rel("X_kickstarter"));
    CHECK_FALSE(is_extension_link_rel(""));
}

TEST_CASE("RightsStatements.org URIs classify", "[tables]")
{
    CHECK(
        classify_rights_status("https://rightsstatements.org/vocab/InC/1.0/") ==
        rights_status_class::in_copyright
    );
    CHECK(
        classify_rights_status("https://rightsstatements.org/vocab/InC-EDU/1.0/") ==
        rights_status_class::in_copyright
    );
    CHECK(
        classify_rights_status("https://rightsstatements.org/vocab/NoC-US/1.0/") ==
        rights_status_class::no_copyright
    );
    CHECK(
        classify_rights_status("https://rightsstatements.org/vocab/UND/1.0/") ==
        rights_status_class::undetermined
    );

    // The scheme and the trailing slash are normalized away, because decks in
    // the wild carry every combination.
    CHECK(
        classify_rights_status("http://rightsstatements.org/vocab/InC/1.0") ==
        rights_status_class::in_copyright
    );
}

TEST_CASE("Creative Commons URIs classify", "[tables]")
{
    CHECK(
        classify_rights_status("https://creativecommons.org/publicdomain/mark/1.0/") ==
        rights_status_class::no_copyright
    );
    CHECK(
        classify_rights_status("https://creativecommons.org/publicdomain/zero/1.0/") ==
        rights_status_class::no_copyright
    );

    // A licence is a grant, and there is nothing to grant from unless the work
    // is in copyright.
    CHECK(
        classify_rights_status("https://creativecommons.org/licenses/by-nc-sa/3.0/") ==
        rights_status_class::in_copyright
    );
    CHECK(
        classify_rights_status("https://creativecommons.org/licenses/by/4.0/") ==
        rights_status_class::in_copyright
    );
    CHECK(
        classify_rights_status("https://creativecommons.org/licenses/by-sa/3.0/us/") ==
        rights_status_class::in_copyright
    );
}

TEST_CASE("anything else is not a rights-status URI", "[tables]")
{
    CHECK_FALSE(is_rights_status_uri(""));
    CHECK_FALSE(is_rights_status_uri("public domain"));
    CHECK_FALSE(is_rights_status_uri("https://example.com/rights"));
    CHECK_FALSE(is_rights_status_uri("https://rightsstatements.org/vocab/Nope/1.0/"));
    CHECK_FALSE(is_rights_status_uri("https://creativecommons.org/licenses/by/"));
    CHECK_FALSE(is_rights_status_uri("https://creativecommons.org/licenses/nope/4.0/"));
    CHECK_FALSE(is_rights_status_uri("rightsstatements.org/vocab/InC/1.0/"));

    CHECK(is_rights_status_uri("https://rightsstatements.org/vocab/InC/1.0/"));
}

TEST_CASE("three-letter language subtags reduce to their two-letter form", "[tables]")
{
    CHECK(shortest_language_subtag("eng") == "en");
    CHECK(shortest_language_subtag("por") == "pt");
    CHECK(shortest_language_subtag("jpn") == "ja");

    // Bibliographic and terminological codes both map.
    CHECK(shortest_language_subtag("ger") == "de");
    CHECK(shortest_language_subtag("deu") == "de");
    CHECK(shortest_language_subtag("fre") == "fr");
    CHECK(shortest_language_subtag("fra") == "fr");
}

TEST_CASE("subtags with no shorter form return nothing", "[tables]")
{
    CHECK_FALSE(shortest_language_subtag("haw").has_value());
    CHECK_FALSE(shortest_language_subtag("ceb").has_value());
    CHECK_FALSE(shortest_language_subtag("en").has_value());
    CHECK_FALSE(shortest_language_subtag("").has_value());
    CHECK_FALSE(shortest_language_subtag("ENG").has_value());
    CHECK_FALSE(shortest_language_subtag("zzz").has_value());
}

TEST_CASE("the curated table knows the Creative Commons family", "[tables]")
{
    auto const by = find_license_permissions("CC-BY-4.0");
    REQUIRE(by.has_value());
    CHECK(by->grants_redistribution);
    CHECK(by->grants_derivation);

    auto const zero = find_license_permissions("CC0-1.0");
    REQUIRE(zero.has_value());
    CHECK(zero->grants_redistribution);
    CHECK(zero->grants_derivation);

    // NonCommercial conditions redistribution rather than withholding it.
    auto const non_commercial = find_license_permissions("CC-BY-NC-SA-3.0");
    REQUIRE(non_commercial.has_value());
    CHECK(non_commercial->grants_redistribution);
    CHECK(non_commercial->grants_derivation);

    // NoDerivatives is the one that withholds, which is what makes a surrogate
    // the interesting case for it.
    auto const no_derivatives = find_license_permissions("CC-BY-ND-4.0");
    REQUIRE(no_derivatives.has_value());
    CHECK(no_derivatives->grants_redistribution);
    CHECK_FALSE(no_derivatives->grants_derivation);

    auto const both = find_license_permissions("CC-BY-NC-ND-4.0");
    REQUIRE(both.has_value());
    CHECK(both->grants_redistribution);
    CHECK_FALSE(both->grants_derivation);
}

TEST_CASE("the curated table stays silent outside itself", "[tables]")
{
    // Real licences that this project does not claim to know. Silence is the
    // contract: a wrong warning about someone's licensing is worse than none.
    CHECK_FALSE(find_license_permissions("MIT").has_value());
    CHECK_FALSE(find_license_permissions("Apache-2.0").has_value());
    CHECK_FALSE(find_license_permissions("Artistic-1.0-Perl").has_value());
    CHECK_FALSE(find_license_permissions("CC-BY-3.0-AT").has_value());

    CHECK_FALSE(find_license_permissions("").has_value());
    CHECK_FALSE(find_license_permissions("LicenseRef-Custom").has_value());
    CHECK_FALSE(find_license_permissions("cc-by-4.0").has_value());
}

TEST_CASE("every curated identifier is a real SPDX identifier", "[tables]")
{
    // The table is keyed on SPDX identifiers, so a typo in it would be a row
    // that silently never matches.
    for (std::string_view const id : {
             "CC-BY-1.0",       "CC-BY-2.0",       "CC-BY-2.5",       "CC-BY-3.0",
             "CC-BY-4.0",       "CC-BY-NC-1.0",    "CC-BY-NC-2.0",    "CC-BY-NC-2.5",
             "CC-BY-NC-3.0",    "CC-BY-NC-4.0",    "CC-BY-NC-ND-1.0", "CC-BY-NC-ND-2.0",
             "CC-BY-NC-ND-2.5", "CC-BY-NC-ND-3.0", "CC-BY-NC-ND-4.0", "CC-BY-NC-SA-1.0",
             "CC-BY-NC-SA-2.0", "CC-BY-NC-SA-2.5", "CC-BY-NC-SA-3.0", "CC-BY-NC-SA-4.0",
             "CC-BY-ND-1.0",    "CC-BY-ND-2.0",    "CC-BY-ND-2.5",    "CC-BY-ND-3.0",
             "CC-BY-ND-4.0",    "CC-BY-SA-1.0",    "CC-BY-SA-2.0",    "CC-BY-SA-2.5",
             "CC-BY-SA-3.0",    "CC-BY-SA-4.0",    "CC-PDDC",         "CC0-1.0",
             "Unlicense",
         })
    {
        CAPTURE(id);
        CHECK(is_spdx_license_id(id));
        CHECK(find_license_permissions(id).has_value());
    }
}
